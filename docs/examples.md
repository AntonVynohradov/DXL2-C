# Examples

The repository contains three complete example programs in
[`examples/`](https://github.com/AntonVynohradov/DXL2-C/tree/main/examples). They show the
integration patterns from the [integration guide](integration.md) in working code.

| Example | Role | Platform | Built and run in CI |
| --- | --- | --- | --- |
| [Bare metal](#bare-metal) | Device | Any MCU; runs on a PC against a simulated board | Yes, as a self-checking test |
| [Host serial port](#host-serial-port) | Controller | Linux, macOS, Windows | Built |
| [Zephyr](#zephyr) | Device | Any Zephyr board with a UART | No, needs a Zephyr workspace |

Build the host examples and run the bare-metal test:

```bash
cmake --preset "DXL2[Examples]"
cmake --build "build/DXL2[Examples]"
ctest --test-dir "build/DXL2[Examples]" -L example --output-on-failure
```

## Bare metal

`examples/bare_metal` is a DYNAMIXEL device for a microcontroller without an operating
system. It answers PING, READ and WRITE on a 128-byte Control Table whose first 64 bytes
are read-only.

| File | Contents |
| --- | --- |
| `main.c` | The application: ring buffer, main loop, packet handler, Control Table |
| `board.h` | The four board functions the application needs |
| `board_host.c` | A PC implementation of `board.h` that turns the example into a test |

**Structure.** The UART interrupt calls `app_uart_rx_isr()` for every byte. It stores the
byte in a single-producer, single-consumer ring buffer and returns. The main loop takes
bytes out of the buffer and passes them to `dxl2_link_receive()`; when the buffer is
empty it calls `board_idle()`, which on a real MCU executes `WFI` until the next
interrupt. The packet handler therefore runs in the main loop, never in the interrupt.

```c
for (;;)
{
    if (rx_tail != rx_head)
    {
        const uint8_t byte = rx_queue[rx_tail];

        rx_tail = (uint16_t) ((rx_tail + 1U) & (RX_QUEUE_SIZE - 1U));
        dxl2_link_receive(&link, &byte, 1U);
    }
    else
    {
        board_idle();
    }
}
```

The ring buffer needs no lock on a single-core 32-bit MCU: the interrupt writes only
`rx_head`, the main loop writes only `rx_tail`, and 16-bit loads and stores are atomic.
On an 8-bit MCU, disable the RX interrupt around the comparison.

**Porting.** Implement `board.h` for your MCU:

| Function | What it must do |
| --- | --- |
| `board_init(baud_rate)` | Configure the UART (8N1) and enable its RX interrupt; call `app_uart_rx_isr()` from it |
| `board_uart_write(data, length)` | Transmit the bytes and wait until the last one has left the shift register, then switch a half-duplex transceiver back to receive |
| `board_idle()` | Sleep until the next interrupt (`__WFI()` on Cortex-M) |

**The simulated board.** `board_host.c` acts as the controller. Each time the main loop
goes idle, it checks the reply to the previous request and injects the next one —
preceded by line noise, so the parser must resynchronize. The test covers PING (direct and
broadcast), a PING for another ID, READ, WRITE, a broadcast WRITE without a reply, a write
to a read-only address, out-of-range and malformed READs, and an unsupported instruction:

```text
bare-metal example on the host simulator, 57600 bit/s
PING                         ok
broadcast PING               ok
PING to another ID           ok
READ model number            ok
WRITE RAM                    ok
READ RAM                     ok
broadcast WRITE              ok
READ after broadcast WRITE   ok
WRITE read-only ID           ok
READ out of range            ok
READ with bad length         ok
unsupported instruction      ok
0 failure(s)
```

## Host serial port

`examples/host_serial` builds `dxl2_ping`, a command-line controller that sends a PING
through a serial port and lists every device that answers.

```bash
dxl2_ping /dev/ttyUSB0            # broadcast PING at 57600 bit/s
dxl2_ping COM3 1000000 1          # PING ID 1 at 1 Mbit/s
```

Example output with two XL430-W250 servos on the bus:

```text
ID   1: model 1060, firmware 52
ID   2: model 1060, firmware 52
2 device(s) answered, 0 invalid packet(s)
```

| File | Contents |
| --- | --- |
| `main.c` | Argument parsing, the link, the Status Packet handler |
| `serial_port.h` | Four functions: open, write, read with timeout, close |
| `serial_port_posix.c` | Linux and macOS implementation with `termios` and `poll()` |
| `serial_port_win32.c` | Windows implementation with the Win32 communications API |

After sending, the program reads with a 50 ms timeout and passes whatever arrives to
`dxl2_link_receive()`. It stops after four empty reads in a row, which leaves enough time
for every device to answer a broadcast PING. The handler ignores packets whose
instruction is not `0x55`, which removes the echo of the request on single-wire
adapters. The exit code is 0 if at least one device answered.

The POSIX version supports 9600, 57600, 115200 and 230400 bit/s, and 1, 2, 3 and
4 Mbit/s where `termios` defines them (Linux). The Windows version accepts any rate the
adapter supports.

## Zephyr

`examples/zephyr` is a Zephyr application that makes a board a DYNAMIXEL device
answering PING. It is the structure used in production firmware, reduced to the library
API.

- The UART interrupt reads the FIFO and puts each byte into a `k_msgq`. It never calls
  the library.
- An RX thread, created with `K_THREAD_DEFINE`, owns the link: it waits on the queue and
  passes each byte to `dxl2_link_receive()`. The packet handler runs in this thread.
- The write function sends with `uart_poll_out()` under a mutex, so other threads may send
  on the same link.

Select the UART with a `dxl2,uart` chosen node in a board overlay (a template is in
`app.overlay.example`) and build with `west`:

```bash
west build -b <board> examples/zephyr -- -DDTC_OVERLAY_FILE=<board>.overlay
```

The application's `CMakeLists.txt` includes `dxl2.cmake` and links the `dxl2` target to
`zephyr_interface`, so the library is compiled with the board's architecture flags. The
console must not use the same UART: text output corrupts the packet framing.
