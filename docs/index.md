---
title: DXL2-C — DYNAMIXEL Protocol 2.0 in C
description: Hardware-agnostic DYNAMIXEL Protocol 2.0 library in C for embedded systems.
template: home.html
hide:
  - navigation
  - toc
---

<span class="sfs-eyebrow">Quick start</span>

## A minimal device in one file

Two callbacks connect the library to your hardware: a write function for the UART and a
handler for received packets. This device answers PING with its model number and firmware
version.

```c
#include "dxl2.h"

static struct dxl2_link link;

static int uart_write(void* context, const uint8_t* data, size_t length)
{
    (void) context;
    return board_uart_write(data, length); /* 0 on success */
}

static void on_packet(const struct dxl2_packet* packet, void* context)
{
    static const uint8_t ping_reply[] = {0x06, 0x04, 0x26}; /* model 1030, firmware 38 */

    (void) context;
    if ((packet->id == 1U) && (packet->instruction == DXL2_INST_PING))
    {
        (void) dxl2_link_send_status(&link, 1U, DXL2_ERR_NONE, ping_reply, sizeof(ping_reply));
    }
}

int main(void)
{
    const struct dxl2_link_config config = {.write = uart_write, .handler = on_packet};

    (void) dxl2_link_init(&link, &config);
    for (;;)
    {
        uint8_t bytes[32];
        const size_t count = board_uart_read(bytes, sizeof(bytes));

        dxl2_link_receive(&link, bytes, count);
    }
}
```

`board_uart_write()` and `board_uart_read()` stand for your board's UART driver. The
[examples](examples.md) are complete programs for bare metal, Zephyr and a PC serial port.

## Find your way

| If you want to… | Read |
| --- | --- |
| Understand what the library does and does not do | [Introduction](overview.md) |
| Build it, run the tests, add it to your project | [Getting Started](guide.md) |
| Understand the layers, data flow and parser | [How It Works](architecture.md) |
| Write a device or a controller with it | [Using the Library](usage.md) |
| Add instructions, transports or contribute | [Extending the Library](extending.md) |
| Look up the bytes on the wire | [Protocol](protocol.md) |
| Look up a function | [API Reference](api.md) |
| Read everything offline or print it | [PDF user manual](downloads/DXL2-C_User_Manual.pdf) |
