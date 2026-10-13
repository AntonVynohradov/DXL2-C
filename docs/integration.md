# Integration

The library does not touch hardware. An integration has three parts:

1. **Receive:** move bytes from the transport into `dxl2_link_receive()`.
   Do it in a thread or the main loop, not in an interrupt handler: the
   packet handler runs in the caller's context and usually sends a reply.
2. **Transmit:** a `dxl2_write_fn` that writes a complete packet. Serialize it
   (mutex) if more than one thread can send.
3. **Application:** a `dxl2_packet_handler_t` that interprets the instruction
   for its IDs (Control Table) and answers with `dxl2_link_send_status()`.

## Zephyr: UART with an RX interrupt, byte queue and RX thread

This is the structure used in the SmartManipulator-X firmware, reduced to the
library API. The ISR only queues bytes; a thread owns the link.

```c
#include "dxl2.h"

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

#include <errno.h>

#define HOST_UART_NODE DT_CHOSEN(zephyr_console) /* console output must be disabled */

K_MSGQ_DEFINE(rx_byte_queue, sizeof(uint8_t), 512, 1);
K_THREAD_STACK_DEFINE(rx_stack, 2048);
K_MUTEX_DEFINE(tx_mutex);

static const struct device *const host_uart = DEVICE_DT_GET(HOST_UART_NODE);
static struct k_thread rx_thread;
static struct dxl2_link link;

static void uart_rx_isr(const struct device *dev, void *user_data)
{
	uint8_t bytes[32];
	int count;

	ARG_UNUSED(user_data);
	while (uart_irq_update(dev) && uart_irq_rx_ready(dev)) {
		count = uart_fifo_read(dev, bytes, sizeof(bytes));
		if (count <= 0) {
			break;
		}
		for (int i = 0; i < count; ++i) {
			(void)k_msgq_put(&rx_byte_queue, &bytes[i], K_NO_WAIT); /* count overruns here */
		}
	}
}

static int uart_write(void *context, const uint8_t *data, size_t length)
{
	ARG_UNUSED(context);
	k_mutex_lock(&tx_mutex, K_FOREVER);
	for (size_t i = 0; i < length; ++i) {
		uart_poll_out(host_uart, data[i]);
	}
	k_mutex_unlock(&tx_mutex);
	return 0;
}

static void rx_thread_main(void *a, void *b, void *c)
{
	uint8_t byte;

	for (;;) {
		if (k_msgq_get(&rx_byte_queue, &byte, K_FOREVER) == 0) {
			dxl2_link_receive(&link, &byte, 1U);
		}
	}
}

int protocol_init(dxl2_packet_handler_t handler, void *context)
{
	const struct dxl2_link_config config = {
		.write = uart_write,
		.handler = handler,
		.handler_context = context,
	};

	if (!device_is_ready(host_uart)) {
		return -ENODEV;
	}
	dxl2_link_init(&link, &config);
	if (uart_irq_callback_user_data_set(host_uart, uart_rx_isr, NULL) != 0) {
		return -EIO;
	}
	k_thread_create(&rx_thread, rx_stack, K_THREAD_STACK_SIZEOF(rx_stack), rx_thread_main,
			NULL, NULL, NULL, K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
	uart_irq_rx_enable(host_uart);
	return 0;
}
```

Notes from the hardware bring-up:

- Size the byte queue for at least one maximum packet plus the time the
  handler blocks (for example while it waits for a motor driver). A dropped
  byte makes the next packet fail its CRC; the parser recovers on the next
  header.
- `uart_poll_out()` is simple and deterministic. At 230400 bit/s a 256-byte
  packet takes about 11 ms; use interrupt or DMA TX if that is too long.
- Check the real bit rate of the UART divisor. A 16550-style UART at a 50 MHz
  clock gives 223214 bit/s for 230400 (-3.1 %), which is too far off; some
  UARTs (for example the SmartFusion2 MMUART) have a fractional divisor.
- Text output (console, logs, `printk`) on the same UART corrupts the framing.

## Adding the library to a CMake build (Zephyr or any other)

`dxl2.cmake` defines the `dxl2` static library target with its include
directories (`.` and `inc/`) and the version definitions:

```cmake
include(path/to/dxl2/dxl2.cmake)
target_link_libraries(app PRIVATE dxl2)
```

Without CMake, compile `src/*.c` and add the repository root and `inc/` to
the include path. Buffer sizes are set in `inc/dxl2_config.h` and can be
overridden with compiler definitions (for example `-DDXL2_MAX_PACKET_SIZE=1024U`).

## Bare metal / other RTOS

The same pattern applies: ring buffer from the RX interrupt, drain it from
the main loop into `dxl2_link_receive()`, and a blocking or buffered write
function. On a host (Linux/Windows serial port) read whatever the port
returns and pass it to `dxl2_link_receive()` in one call.

## Answering a device request

A device implementation typically does this in the handler:

```c
static void on_packet(const struct dxl2_packet *packet, void *context)
{
	const uint8_t *p = packet->parameters;

	if (packet->id != MY_ID && packet->id != DXL2_BROADCAST_ID) {
		return;
	}
	switch (packet->instruction) {
	case DXL2_INST_PING:
		/* model number (2), firmware version (1) */
		break;
	case DXL2_INST_READ:
		if (packet->parameter_length != 4U) {
			dxl2_link_send_status(&link, MY_ID, DXL2_ERR_DATA_LENGTH, NULL, 0U);
			return;
		}
		/* address = dxl2_get_u16(&p[0]), length = dxl2_get_u16(&p[2]) */
		break;
	default:
		dxl2_link_send_status(&link, MY_ID, DXL2_ERR_INSTRUCTION, NULL, 0U);
		break;
	}
}
```

Do not answer instructions sent to the broadcast ID, except PING (all devices
answer, ordered by ID) and the read instructions that name the device in
their ID list. Respect the device's Status Return Level and Return Delay Time
registers if it emulates them.
