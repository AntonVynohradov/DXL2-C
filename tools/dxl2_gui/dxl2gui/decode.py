#******************************************************************************
# DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
#
# Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
# Description: Human readable descriptions of packets for the debug view.
#
# Author:      Anton Vynohradov
# Email:       avynohradov@systemfromscratch.com
#
# License:     Apache License 2.0
# Copyright (c) 2026 Anton Vynohradov
#
# SPDX-License-Identifier: Apache-2.0
#*****************************************************************************

"""Packet naming, field breakdown and Control Table value formatting.

Decoding itself is done by the C library; this module only labels the bytes.
"""

from __future__ import annotations

from typing import Callable, Dict, List, Optional, Sequence, Tuple

BROADCAST_ID = 0xFE
MAX_ID = 252
INST_STATUS = 0x55

INSTRUCTIONS: Dict[int, str] = {
    0x01: "PING",
    0x02: "READ",
    0x03: "WRITE",
    0x04: "REG_WRITE",
    0x05: "ACTION",
    0x06: "FACTORY_RESET",
    0x08: "REBOOT",
    0x10: "CLEAR",
    0x20: "BACKUP",
    0x55: "STATUS",
    0x82: "SYNC_READ",
    0x83: "SYNC_WRITE",
    0x8A: "FAST_SYNC_READ",
    0x92: "BULK_READ",
    0x93: "BULK_WRITE",
    0x9A: "FAST_BULK_READ",
}

ERRORS: Dict[int, str] = {
    0x00: "OK",
    0x01: "Result Fail",
    0x02: "Instruction Error",
    0x03: "CRC Error",
    0x04: "Data Range Error",
    0x05: "Data Length Error",
    0x06: "Data Limit Error",
    0x07: "Access Error",
}

HARDWARE_ERRORS: Dict[int, str] = {
    0x01: "Input Voltage",
    0x04: "Overheating",
    0x08: "Motor Encoder",
    0x10: "Electrical Shock",
    0x20: "Overload",
}

#: Expected status data size for instructions that return data (None: no data).
READ_INSTRUCTIONS = (0x02, 0x82, 0x92)


def instruction_name(instruction: int) -> str:
    return INSTRUCTIONS.get(instruction, f"0x{instruction:02X}")


def id_text(packet_id: int) -> str:
    return "BCAST" if packet_id == BROADCAST_ID else str(packet_id)


def error_text(error: int) -> str:
    text = ERRORS.get(error & 0x7F, f"Unknown error 0x{error & 0x7F:02X}")
    if error & 0x80:
        text += " + Hardware Alert"
    return text


def hexdump(data: bytes) -> str:
    return data.hex(" ").upper()


def parse_hex(text: str) -> bytes:
    """Parse '01 02 0x03,4' style hex input."""
    cleaned = text.replace(",", " ").replace(";", " ").replace("0x", " ").replace("0X", " ")
    tokens = cleaned.split()
    if len(tokens) == 1 and len(tokens[0]) > 2:
        return bytes.fromhex(tokens[0])
    return bytes(int(token, 16) for token in tokens)


def u16(data: bytes, offset: int = 0) -> int:
    return int.from_bytes(data[offset:offset + 2], "little")


def describe_instruction(packet_id: int, instruction: int, params: bytes) -> str:
    """One-line summary of an Instruction Packet."""
    p = params
    if instruction == 0x02 and len(p) >= 4:
        return f"addr={u16(p)} len={u16(p, 2)}"
    if instruction in (0x03, 0x04) and len(p) >= 2:
        data = p[2:]
        value = f" (={int.from_bytes(data, 'little')})" if 1 <= len(data) <= 4 else ""
        return f"addr={u16(p)} data=[{hexdump(data)}]{value}"
    if instruction == 0x06 and p:
        modes = {0xFF: "all", 0x01: "all except ID", 0x02: "all except ID and baud"}
        return f"reset {modes.get(p[0], hex(p[0]))}"
    if instruction in (0x82, 0x8A) and len(p) >= 4:
        return f"addr={u16(p)} len={u16(p, 2)} ids={list(p[4:])}"
    if instruction == 0x83 and len(p) >= 4:
        return f"addr={u16(p)} len={u16(p, 2)} payload=[{hexdump(p[4:])}]"
    if instruction in (0x92, 0x9A):
        parts = [f"id{p[i]}@{u16(p, i + 1)}x{u16(p, i + 3)}" for i in range(0, len(p) - 4, 5)]
        return " ".join(parts)
    return f"params=[{hexdump(p)}]" if p else ""


def describe_status(params: bytes, request: Optional[Tuple[int, int, bytes]] = None,
                    names: Optional[Callable[[int, bytes], str]] = None) -> Tuple[str, str]:
    """(error text, summary) of a Status Packet, interpreted against its request."""
    error = params[0] if params else 0
    data = params[1:]
    summary = f"data=[{hexdump(data)}]" if data else ""
    if request is not None and data:
        _, req_inst, req_params = request
        if req_inst == 0x01 and len(data) >= 3:
            summary = f"model={u16(data)} fw={data[2]}"
        elif req_inst == 0x02 and len(req_params) >= 4 and names is not None:
            summary = names(u16(req_params), data) or summary
        elif req_inst == 0x82 and len(req_params) >= 4 and names is not None:
            summary = names(u16(req_params), data) or summary
    return error_text(error), summary


def breakdown(raw: bytes, crc: Callable[[bytes], int]) -> List[Tuple[str, str, str]]:
    """Field-by-field view of a wire packet: (field, bytes, meaning)."""
    rows: List[Tuple[str, str, str]] = []
    if len(raw) < 10:
        rows.append(("Raw", hexdump(raw), "incomplete packet"))
        return rows
    header_ok = raw[:4] == b"\xFF\xFF\xFD\x00"
    rows.append(("Header", hexdump(raw[:4]), "OK" if header_ok else "INVALID"))
    rows.append(("ID", hexdump(raw[4:5]), id_text(raw[4])))
    length = u16(raw, 5)
    rows.append(("Length", hexdump(raw[5:7]),
                 f"{length} (instruction + params + CRC)"
                 + ("" if length + 7 == len(raw) else f" - MISMATCH, got {len(raw) - 7}")))
    rows.append(("Instruction", hexdump(raw[7:8]), instruction_name(raw[7])))
    body = raw[8:-2]
    if raw[7] == INST_STATUS and body:
        rows.append(("Error", hexdump(body[:1]), error_text(body[0])))
        body = body[1:]
    if body:
        stuffed = b"\xFF\xFF\xFD\xFD" in raw[7:-2]
        rows.append(("Parameters", hexdump(body),
                     f"{len(body)} bytes" + (" (contains byte stuffing)" if stuffed else "")))
    received = u16(raw, len(raw) - 2)
    calculated = crc(raw[:-2])
    rows.append(("CRC", hexdump(raw[-2:]),
                 f"0x{received:04X} " + ("OK" if received == calculated
                                         else f"BAD (expected 0x{calculated:04X})")))
    return rows


def scan_frames(data: bytes) -> List[bytes]:
    """Split raw bytes at Protocol 2.0 headers (for labelling raw debug chunks)."""
    frames = []
    start = data.find(b"\xFF\xFF\xFD\x00")
    while start >= 0:
        if start + 7 > len(data):
            break
        end = start + 7 + u16(data, start + 5)
        frames.append(data[start:end])
        start = data.find(b"\xFF\xFF\xFD\x00", max(end, start + 1))
    return frames


def signed(value: int, size: int) -> int:
    bits = size * 8
    return value - (1 << bits) if value & (1 << (bits - 1)) else value


def format_items(items: Sequence, address: int, data: bytes) -> str:
    """'Present Position=2048, ...' for Control Table items fully inside data."""
    parts = []
    for item in items:
        offset = item.address - address
        if offset >= 0 and offset + item.size <= len(data):
            value = int.from_bytes(data[offset:offset + item.size], "little")
            parts.append(f"{item.name}={value}")
    return ", ".join(parts[:4]) + (f", ... (+{len(parts) - 4})" if len(parts) > 4 else "")
