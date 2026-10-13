#******************************************************************************
# DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
#
# Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
# Description: Pure-Python DYNAMIXEL Protocol 2.0 codec (twin of the C library).
#
# Author:      Anton Vynohradov
# Email:       avynohradov@systemfromscratch.com
#
# License:     Apache License 2.0
#
# Copyright (c) 2026 Anton Vynohradov
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
#*****************************************************************************

"""DYNAMIXEL Protocol 2.0 packet codec.

Implements the wire format described at
https://docs.robotis.com/docs/dxl/protocol/protocol2/ :

    FF FF FD 00 | ID | LEN_L LEN_H | INST | PARAM... | CRC_L CRC_H

* LEN counts INST + (stuffed) parameters + CRC.
* Byte stuffing inserts an extra 0xFD after every FF FF FD sequence in the
  instruction/parameter area.
* The CRC-16 (polynomial 0x8005, initial value 0, not reflected) covers the
  stuffed packet from the header up to, but not including, the CRC.

The module has no third-party dependencies so it can be unit tested anywhere.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Iterable, List, Optional

HEADER = bytes((0xFF, 0xFF, 0xFD, 0x00))
BROADCAST_ID = 0xFE
MAX_ID = 252
MAX_PACKET_SIZE = 1024  # generous; the C library (DXL2_MAX_PACKET_SIZE) is limited to 256 bytes

# Instructions
INST_PING = 0x01
INST_READ = 0x02
INST_WRITE = 0x03
INST_REG_WRITE = 0x04
INST_ACTION = 0x05
INST_FACTORY_RESET = 0x06
INST_REBOOT = 0x08
INST_CLEAR = 0x10
INST_BACKUP = 0x20
INST_STATUS = 0x55
INST_SYNC_READ = 0x82
INST_SYNC_WRITE = 0x83
INST_FAST_SYNC_READ = 0x8A
INST_BULK_READ = 0x92
INST_BULK_WRITE = 0x93
INST_FAST_BULK_READ = 0x9A

INSTRUCTION_NAMES = {
    INST_PING: "PING",
    INST_READ: "READ",
    INST_WRITE: "WRITE",
    INST_REG_WRITE: "REG_WRITE",
    INST_ACTION: "ACTION",
    INST_FACTORY_RESET: "FACTORY_RESET",
    INST_REBOOT: "REBOOT",
    INST_CLEAR: "CLEAR",
    INST_BACKUP: "BACKUP",
    INST_STATUS: "STATUS",
    INST_SYNC_READ: "SYNC_READ",
    INST_SYNC_WRITE: "SYNC_WRITE",
    INST_FAST_SYNC_READ: "FAST_SYNC_READ",
    INST_BULK_READ: "BULK_READ",
    INST_BULK_WRITE: "BULK_WRITE",
    INST_FAST_BULK_READ: "FAST_BULK_READ",
}

# Status packet error numbers (bits 0-6); bit 7 is the hardware alert flag.
ERR_ALERT_BIT = 0x80
ERROR_NAMES = {
    0x01: "Result Fail",
    0x02: "Instruction Error",
    0x03: "CRC Error",
    0x04: "Data Range Error",
    0x05: "Data Length Error",
    0x06: "Data Limit Error",
    0x07: "Access Error",
}


def error_text(error: int) -> str:
    """Human readable text for a status error byte."""
    if error == 0:
        return "OK"
    number = error & 0x7F
    text = ERROR_NAMES.get(number, f"Unknown error 0x{number:02X}") if number else ""
    if error & ERR_ALERT_BIT:
        text = (text + " + " if text else "") + "Hardware Alert"
    return text


def _make_crc_table() -> List[int]:
    table = []
    for i in range(256):
        crc = i << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x8005) if crc & 0x8000 else (crc << 1)
        table.append(crc & 0xFFFF)
    return table


_CRC_TABLE = _make_crc_table()


def crc16(data: Iterable[int], crc: int = 0) -> int:
    """DYNAMIXEL CRC-16 (polynomial 0x8005, init 0)."""
    for byte in data:
        crc = ((crc << 8) ^ _CRC_TABLE[((crc >> 8) ^ byte) & 0xFF]) & 0xFFFF
    return crc


def stuff(body: bytes) -> bytes:
    """Insert 0xFD after every FF FF FD sequence (instruction + parameters)."""
    out = bytearray()
    for byte in body:
        out.append(byte)
        if len(out) >= 3 and out[-3:] == b"\xFF\xFF\xFD":
            out.append(0xFD)
    return bytes(out)


def unstuff(body: bytes) -> bytes:
    """Remove stuffing bytes. Raises ValueError for malformed stuffing."""
    out = bytearray()
    i = 0
    while i < len(body):
        out.append(body[i])
        if len(out) >= 3 and out[-3:] == b"\xFF\xFF\xFD":
            if i + 1 >= len(body) or body[i + 1] != 0xFD:
                raise ValueError("missing stuffing byte after FF FF FD")
            i += 1  # skip the stuffing byte
        i += 1
    return bytes(out)


def build_packet(packet_id: int, instruction: int, params: bytes = b"") -> bytes:
    """Encode one instruction packet (stuffing and CRC added)."""
    if not 0 <= packet_id <= 0xFE:
        raise ValueError(f"invalid ID {packet_id}")
    if not 0 <= instruction <= 0xFF:
        raise ValueError(f"invalid instruction {instruction}")
    body = stuff(bytes((instruction,)) + bytes(params))
    length = len(body) + 2
    packet = bytearray(HEADER)
    packet.append(packet_id)
    packet += length.to_bytes(2, "little")
    packet += body
    packet += crc16(packet).to_bytes(2, "little")
    if len(packet) > MAX_PACKET_SIZE:
        raise ValueError("packet too large")
    return bytes(packet)


def build_status(packet_id: int, error: int, params: bytes = b"") -> bytes:
    """Encode a status packet (error byte followed by params)."""
    return build_packet(packet_id, INST_STATUS, bytes((error,)) + bytes(params))


@dataclass
class Packet:
    """One decoded packet with stuffing removed."""

    id: int
    instruction: int
    params: bytes = b""
    raw: bytes = field(default=b"", repr=False)

    @property
    def is_status(self) -> bool:
        return self.instruction == INST_STATUS

    @property
    def error(self) -> int:
        """Status error byte (status packets only)."""
        return self.params[0] if self.is_status and self.params else 0

    @property
    def data(self) -> bytes:
        """Status data (after the error byte) or instruction parameters."""
        return self.params[1:] if self.is_status else self.params


class Parser:
    """Incremental stream parser.

    Feed arbitrary chunks; complete, CRC-valid packets are returned. Invalid
    packets are counted in ``crc_errors`` / ``framing_errors`` and skipped.
    """

    def __init__(self) -> None:
        self._buf = bytearray()
        self.crc_errors = 0
        self.framing_errors = 0

    def reset(self) -> None:
        self._buf.clear()

    def feed(self, data: bytes) -> List[Packet]:
        self._buf += data
        packets: List[Packet] = []
        while True:
            start = self._buf.find(HEADER)
            if start < 0:
                # keep a possible partial header at the end
                keep = 0
                for n in (3, 2, 1):
                    if self._buf.endswith(HEADER[:n]):
                        keep = n
                        break
                del self._buf[: len(self._buf) - keep]
                return packets
            if start:
                del self._buf[:start]
            if len(self._buf) < 7:
                return packets
            length = self._buf[5] | (self._buf[6] << 8)
            if length < 3 or length + 7 > MAX_PACKET_SIZE:
                self.framing_errors += 1
                del self._buf[:1]
                continue
            total = 7 + length
            if len(self._buf) < total:
                return packets
            raw = bytes(self._buf[:total])
            expected = raw[-2] | (raw[-1] << 8)
            if crc16(raw[:-2]) != expected:
                self.crc_errors += 1
                del self._buf[:1]
                continue
            try:
                body = unstuff(raw[7:-2])
            except ValueError:
                self.framing_errors += 1
                del self._buf[:total]
                continue
            del self._buf[:total]
            packets.append(Packet(raw[4], body[0], body[1:], raw))


def parse_one(data: bytes) -> Optional[Packet]:
    """Parse the first valid packet in ``data`` (convenience for tests)."""
    packets = Parser().feed(data)
    return packets[0] if packets else None


def describe(packet: Packet) -> str:
    """One-line human readable description of a packet."""
    name = INSTRUCTION_NAMES.get(packet.instruction, f"0x{packet.instruction:02X}")
    target = "BROADCAST" if packet.id == BROADCAST_ID else f"ID {packet.id}"
    if packet.is_status:
        err = error_text(packet.error)
        data = packet.data.hex(" ").upper()
        return f"STATUS {target} err={err}" + (f" data=[{data}]" if data else "")
    p = packet.params
    if packet.instruction in (INST_READ,) and len(p) >= 4:
        return f"{name} {target} addr={int.from_bytes(p[0:2], 'little')} len={int.from_bytes(p[2:4], 'little')}"
    if packet.instruction in (INST_WRITE, INST_REG_WRITE) and len(p) >= 2:
        return f"{name} {target} addr={int.from_bytes(p[0:2], 'little')} data=[{p[2:].hex(' ').upper()}]"
    if packet.instruction in (INST_SYNC_READ, INST_SYNC_WRITE) and len(p) >= 4:
        return (f"{name} addr={int.from_bytes(p[0:2], 'little')} "
                f"len={int.from_bytes(p[2:4], 'little')} params=[{p[4:].hex(' ').upper()}]")
    return f"{name} {target}" + (f" params=[{p.hex(' ').upper()}]" if p else "")


def hexdump(data: bytes) -> str:
    return data.hex(" ").upper()
