#******************************************************************************
# DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
#
# Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
# Description: Byte transports of the GUI: simulated device and serial port.
#
# Author:      Anton Vynohradov
# Email:       avynohradov@systemfromscratch.com
#
# License:     Apache License 2.0
# Copyright (c) 2026 Anton Vynohradov
#
# SPDX-License-Identifier: Apache-2.0
#*****************************************************************************

"""Transports move raw bytes; the session decodes them with the C library."""

from __future__ import annotations

import queue
import random
import threading
import time
from dataclasses import dataclass
from typing import List, Optional, Tuple

from .native import Native, SimDevice

try:
    import serial  # type: ignore
    import serial.tools.list_ports  # type: ignore
except ImportError:  # pragma: no cover - optional dependency
    serial = None

BAUD_RATES = (9600, 57600, 115200, 1000000, 2000000, 3000000, 4000000, 4500000)


class TransportError(RuntimeError):
    pass


class Transport:
    """Raw byte pipe to one bus."""

    name = "transport"

    def open(self) -> None:
        raise NotImplementedError

    def close(self) -> None:
        raise NotImplementedError

    def write(self, data: bytes) -> None:
        raise NotImplementedError

    def poll(self) -> bytes:
        """Bytes received since the last call (non-blocking)."""
        raise NotImplementedError

    def notes(self) -> List[str]:
        """Debug notes produced since the last call (fault injection etc.)."""
        return []


@dataclass
class FaultConfig:
    """Fault injection of the simulated bus."""

    drop_percent: int = 0
    corrupt_percent: int = 0
    split_reply: bool = False
    delay_ms: int = 0


class SimulationTransport(Transport):
    """Simulated device driven by the native dxl2_sim (its own dxl2_link)."""

    TICK_PERIOD = 0.02

    def __init__(self, native: Native, device_id: int = 1) -> None:
        self.device = SimDevice(native, device_id)
        self.faults = FaultConfig()
        self.name = f"Simulation (ID {device_id})"
        self._pending: List[Tuple[float, bytes]] = []
        self._notes: List[str] = []
        self._last_tick = time.monotonic()
        self._rng = random.Random()

    def open(self) -> None:
        self._last_tick = time.monotonic()

    def close(self) -> None:
        self._pending.clear()

    def write(self, data: bytes) -> None:
        rejected = self.device.invalid_packets
        reply = self.device.process(data)
        rejected = self.device.invalid_packets - rejected
        if rejected:
            self._notes.append(f"device: {rejected} malformed/CRC-invalid request(s) rejected")
        if not reply:
            return
        if self._rng.randrange(100) < self.faults.drop_percent:
            self._notes.append(f"fault: dropped {len(reply)} reply bytes")
            return
        if self._rng.randrange(100) < self.faults.corrupt_percent:
            corrupted = bytearray(reply)
            index = self._rng.randrange(7, len(corrupted))
            corrupted[index] ^= 1 << self._rng.randrange(8)
            reply = bytes(corrupted)
            self._notes.append(f"fault: flipped one bit in reply byte {index}")
        due = time.monotonic() + self.faults.delay_ms / 1000.0
        if self.faults.split_reply and len(reply) > 3:
            cut = self._rng.randrange(1, len(reply))
            self._pending.append((due, reply[:cut]))
            self._pending.append((due + 0.005, reply[cut:]))
        else:
            self._pending.append((due, reply))

    def poll(self) -> bytes:
        now = time.monotonic()
        while now - self._last_tick >= self.TICK_PERIOD:
            self.device.tick()
            self._last_tick += self.TICK_PERIOD
        ready = [chunk for due, chunk in self._pending if due <= now]
        self._pending = [(due, chunk) for due, chunk in self._pending if due > now]
        return b"".join(ready)

    def notes(self) -> List[str]:
        notes, self._notes = self._notes, []
        return notes


def list_ports() -> List[Tuple[str, str]]:
    """Available serial ports as (device, description)."""
    if serial is None:
        return []
    return [(p.device, p.description or p.device)
            for p in sorted(serial.tools.list_ports.comports(), key=lambda p: p.device)]


class SerialTransport(Transport):
    """Serial port (USB-UART, U2D2, RS-485 adapter, embedded target)."""

    def __init__(self, port: str, baudrate: int, discard_echo: bool = False) -> None:
        if serial is None:
            raise TransportError("pyserial is not installed: python -m pip install pyserial")
        self.port = port
        self.baudrate = baudrate
        self.discard_echo = discard_echo
        self.name = f"{port} @ {baudrate}"
        self._serial: Optional["serial.Serial"] = None
        self._rx: "queue.Queue[bytes]" = queue.Queue()
        self._thread: Optional[threading.Thread] = None
        self._running = False
        self._echo = bytearray()
        self._notes: List[str] = []

    def open(self) -> None:
        try:
            # Accepts COM3 or /dev/ttyUSB0 and pyserial URLs such as socket://host:port
            self._serial = serial.serial_for_url(self.port, self.baudrate, timeout=0.01)
        except (serial.SerialException, ValueError) as exc:
            raise TransportError(str(exc)) from exc
        self._serial.reset_input_buffer()
        self._running = True
        self._thread = threading.Thread(target=self._reader, name="dxl2-serial", daemon=True)
        self._thread.start()

    def close(self) -> None:
        self._running = False
        if self._thread is not None:
            self._thread.join(timeout=0.5)
        if self._serial is not None:
            self._serial.close()
        self._serial = None

    def _reader(self) -> None:
        while self._running and self._serial is not None:
            try:
                data = self._serial.read(self._serial.in_waiting or 1)
            except (serial.SerialException, OSError, TypeError) as exc:
                self._notes.append(f"serial error: {exc}")
                self._running = False
                return
            if data:
                self._rx.put(data)

    def write(self, data: bytes) -> None:
        if self._serial is None:
            raise TransportError("port is not open")
        if self.discard_echo:
            self._echo += data
        try:
            self._serial.write(data)
            self._serial.flush()
        except (serial.SerialException, OSError) as exc:
            raise TransportError(str(exc)) from exc

    def poll(self) -> bytes:
        chunks = []
        while True:
            try:
                chunks.append(self._rx.get_nowait())
            except queue.Empty:
                break
        data = b"".join(chunks)
        if self._echo and data:
            # Half-duplex buses read back what was sent; strip it byte by byte
            matched = 0
            while matched < min(len(data), len(self._echo)) and data[matched] == self._echo[matched]:
                matched += 1
            if matched:
                self._notes.append(f"echo: discarded {matched} bytes")
            del self._echo[:matched]
            if matched < len(data):
                self._echo.clear()
            data = data[matched:]
        return data

    def notes(self) -> List[str]:
        notes, self._notes = self._notes, []
        return notes

    @property
    def alive(self) -> bool:
        return self._running
