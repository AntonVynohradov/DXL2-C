# DXL2 Studio

A desktop GUI for testing and debugging the DXL2-C library. It works in two modes:

| Mode | What runs on each side |
|------|------------------------|
| **Simulation** | The master and a simulated DYNAMIXEL device (XL430-W250 Control Table) each run the static `libdxl2.a` on their own `dxl2_link`. You don't need any hardware. |
| **Serial port** | The master runs on the static library and talks to a real device over a COM port: an embedded target with DXL2-C, a U2D2, or a servo. |

In both modes, every Instruction Packet is encoded by `dxl2_link_send`, and every received byte goes through `dxl2_link_receive`. The tool does not reimplement any part of the protocol.

## Quick start

```bash
# from the repository root
python -m pip install -r tools/dxl2_gui/requirements.txt   # pyserial, needed for the serial mode only
python tools/dxl2_gui/dxl2_gui.py
```

The first time you start the tool, it offers to build the native bridge with CMake. You need CMake and a C compiler: MinGW-w64/GCC, Clang, or MSVC. Use the same bitness as your Python. You can also build the bridge by hand:

```bash
cmake -S tools/dxl2_gui/native -B tools/dxl2_gui/build
cmake --build tools/dxl2_gui/build --config Release
```

Command-line options start the tool in a given mode and connect right away:

```bash
python tools/dxl2_gui/dxl2_gui.py --mode sim --id 1 --connect
python tools/dxl2_gui/dxl2_gui.py --mode serial --port COM3 --baud 1000000 --connect
```

In VS Code, press **Ctrl+Shift+B** to open the task menu. Pick *Python: Setup venv* once, then *GUI: Run (Simulation)* or *GUI: Run (Serial port)*. These tasks build the native bridge first. An empty port in the serial task means the first detected port. All tasks are in the build group and none is the default, so VS Code shows the list instead of running a build.

The tasks work on Windows, Linux and macOS. On Debian/Ubuntu, install the system packages first. To use a USB serial adapter without root, add your user to the `dialout` group:

```bash
sudo apt install python3-venv python3-tk cmake build-essential doxygen
sudo usermod -aG dialout $USER   # log out and back in afterwards
```

The build output goes to `tools/dxl2_gui/build/lib/dxl2_gui_native.(dll|so)`. Git ignores this folder.

Requirements: Python 3.8 or newer with Tkinter (the python.org installers include it), CMake 3.20 or newer, and a C11 compiler.

## Features

**Connection bar.** Choose the mode, the simulated device ID or the COM port and baud rate, and the reply timeout. Press **F5** to connect or disconnect. For the serial mode:

- *Discard echo* removes your own bytes from the receive stream on single-wire half-duplex buses.
- The port field also accepts pyserial URLs, for example `socket://192.168.1.10:5000` for a serial-over-TCP bridge.

**Quick control.** Ping, reboot, factory reset, torque and LED on or off, and reading the Hardware Error Status. A goal-position slider sends the goal, and *Auto-poll* shows Present Position every 200 ms.

**Packet builder.** Pick an instruction and fill in its fields, or type the raw parameters in hex for SYNC/BULK instructions. A live preview shows the encoded packet. The *Raw bytes* sender bypasses the encoder, and its *Break CRC*, *Truncate* and *Add noise* buttons let you check how the receiver handles bad input.

**Control table.** Read all items in one READ, with optional auto refresh. Items whose value changed are highlighted. Double-click a RW item to write it.

**Bus scan.** Ping every ID in a range, or send one broadcast ping. Double-click a result to make that device the target.

**Simulator.** Fault injection for the replies: drop a percentage, corrupt one bit, add a delay, or split a reply into two chunks. You can also set the Hardware Error Status (this sets the alert bit), watch the device-side state, and power-cycle the device.

**Communication debugger.**

- The log shows TX, RX, TIMEOUT, ERR and INFO entries, color coded. Each entry has the time, Δt from the previous entry, and the round-trip time (`rt`) for replies.
- Status replies are interpreted against their request. For example, a READ at address 132 shows `Present Position=2048`.
- Select a packet to see it field by field: header, ID, length check, instruction, error, parameters (with a note when byte stuffing is present), and CRC check.
- The log also reports packets that `dxl2_link_receive` rejected, on both the master side and the device side.
- *Raw RX chunks* shows the bytes exactly as they came from the transport, before any decoding.
- You can filter the log, pause it, hide the auto-poll traffic, and export it to CSV or TXT (Ctrl+E).
- The status bar shows packet and byte counters, invalid packets, timeouts, status errors, the last and average latency, and the queue depth.

## Layout

```
tools/dxl2_gui/
├── dxl2_gui.py          # entry point
├── requirements.txt
├── dxl2gui/
│   ├── app.py           # Tkinter UI
│   ├── session.py       # request queue, timeouts, statistics, log entries
│   ├── transport.py     # simulation and serial transports, fault injection
│   ├── native.py        # ctypes binding + CMake build helper
│   └── decode.py        # names, packet breakdown, value formatting
└── native/
    ├── CMakeLists.txt   # links the static dxl2 target into dxl2_gui_native
    ├── dxl2_host.c/.h   # master side: dxl2_link_send / dxl2_link_receive
    ├── dxl2_sim.c/.h    # simulated device on its own dxl2_link
    └── dxl2_tool_api.h  # export macro
```

The native bridge exports only `dxl2_host_*` and `dxl2_sim_*` functions. Python never relies on the memory layout of the library structures: it asks for their sizes and treats them as opaque.

## Notes

- The simulated device implements PING, READ, WRITE, REG_WRITE/ACTION, FACTORY_RESET, REBOOT, CLEAR, BACKUP, SYNC_READ/WRITE and BULK_READ/WRITE. It also applies Status Return Level, the EEPROM lock while torque is on, and the ID, torque and LED range checks. It answers FAST_SYNC_READ and FAST_BULK_READ with an Instruction Error.
- The Control Table view uses the X-series layout. For other models, use the packet builder.
- The library limits packets to `DXL2_MAX_PACKET_SIZE` (256 bytes by default). Larger requests are reported as encode errors.
