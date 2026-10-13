#******************************************************************************
# DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
#
# Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
# Description: Request/response engine of the GUI: queue, timeouts, statistics
#              and the debug log entries.
#
# Author:      Anton Vynohradov
# Email:       avynohradov@systemfromscratch.com
#
# License:     Apache License 2.0
# Copyright (c) 2026 Anton Vynohradov
#
# SPDX-License-Identifier: Apache-2.0
#*****************************************************************************

"""Transport independent master logic.

One request is on the bus at a time. ``poll`` is called periodically by the
UI; it reads the transport, decodes bytes with the C library, matches Status
Packets to the request in flight and reports everything as ``LogEntry``.
"""

from __future__ import annotations

import collections
import time
from dataclasses import dataclass, field
from typing import Callable, Deque, List, Optional, Tuple

from . import decode
from .native import Host, Native
from .transport import Transport, TransportError


@dataclass
class LogEntry:
    """One row of the debug log."""

    time: float
    direction: str  # TX, RX, ERR, TIMEOUT, INFO, RAW
    packet_id: Optional[int] = None
    instruction: Optional[int] = None
    error: str = ""
    summary: str = ""
    raw: bytes = b""
    latency_ms: Optional[float] = None
    tag: str = ""  # tag of the request the entry belongs to


@dataclass
class Reply:
    packet_id: int
    error: int
    data: bytes
    latency_ms: float


@dataclass
class Request:
    packet_id: int
    instruction: int
    params: bytes = b""
    expect: int = 1  # replies to wait for; -1 collects until the timeout
    callback: Optional[Callable[["Request"], None]] = None
    tag: str = ""
    replies: List[Reply] = field(default_factory=list)
    raw: bytes = b""
    sent_at: float = 0.0
    timed_out: bool = False
    failed: str = ""

    @property
    def ok(self) -> bool:
        return not self.timed_out and not self.failed

    @property
    def reply(self) -> Optional[Reply]:
        return self.replies[0] if self.replies else None


@dataclass
class Statistics:
    tx_packets: int = 0
    tx_bytes: int = 0
    rx_packets: int = 0
    rx_bytes: int = 0
    invalid_packets: int = 0
    timeouts: int = 0
    status_errors: int = 0
    last_latency_ms: Optional[float] = None
    latency_sum_ms: float = 0.0
    latency_count: int = 0

    @property
    def average_latency_ms(self) -> Optional[float]:
        return self.latency_sum_ms / self.latency_count if self.latency_count else None


def expected_replies(packet_id: int, instruction: int, params: bytes) -> int:
    """Number of Status Packets a request produces (Status Return Level 2)."""
    if packet_id != decode.BROADCAST_ID:
        return 1
    if instruction == 0x01:
        return -1
    if instruction == 0x82 and len(params) > 4:
        return len(params) - 4
    if instruction == 0x92:
        return len(params) // 5
    return 0


class Session:
    """Master side of one connection."""

    MAX_QUEUE = 256

    def __init__(self, native: Native, transport: Transport,
                 on_log: Callable[[LogEntry], None], timeout_ms: int = 100) -> None:
        self.native = native
        self.host = Host(native)
        self.transport = transport
        self.on_log = on_log
        self.timeout_ms = timeout_ms
        self.stats = Statistics()
        self.log_raw = False
        self._queue: Deque[Request] = collections.deque()
        self._current: Optional[Request] = None
        self._invalid_seen = 0
        self._rx_window = bytearray()  # raw bytes since the last valid packet

    # ------------------------------------------------------------------ requests

    @property
    def pending(self) -> int:
        return len(self._queue) + (1 if self._current else 0)

    def request(self, packet_id: int, instruction: int, params: bytes = b"",
                callback: Optional[Callable[[Request], None]] = None,
                expect: Optional[int] = None, tag: str = "") -> Optional[Request]:
        """Queue an Instruction Packet; ``callback`` runs when it completes."""
        if len(self._queue) >= self.MAX_QUEUE:
            self._log("ERR", summary="request queue full, request dropped")
            return None
        if expect is None:
            expect = expected_replies(packet_id, instruction, params)
        req = Request(packet_id, instruction, bytes(params), expect, callback, tag)
        self._queue.append(req)
        self._start_next()
        return req

    def cancel_all(self, tag: Optional[str] = None) -> int:
        kept = [r for r in self._queue if tag is not None and r.tag != tag]
        removed = len(self._queue) - len(kept)
        self._queue = collections.deque(kept)
        return removed

    def send_raw(self, data: bytes) -> None:
        """Put arbitrary bytes on the bus (bypasses the encoder, for fault tests)."""
        try:
            self.transport.write(data)
        except TransportError as exc:
            self._log("ERR", summary=f"write failed: {exc}")
            return
        self.stats.tx_bytes += len(data)
        frames = decode.scan_frames(data)
        summary = f"{len(data)} raw bytes" + (f", {len(frames)} frame(s)" if frames else "")
        self._log("TX", packet_id=data[4] if len(data) > 7 else None,
                  instruction=data[7] if len(data) > 7 else None,
                  summary="[raw] " + summary, raw=data)

    def _start_next(self) -> None:
        while self._current is None and self._queue:
            req = self._queue.popleft()
            try:
                req.raw = self.host.build(req.packet_id, req.instruction, req.params)
            except ValueError as exc:
                req.failed = str(exc)
                self._log("ERR", req.packet_id, req.instruction, summary=f"encode failed: {exc}")
                self._finish(req)
                continue
            try:
                self.transport.write(req.raw)
            except TransportError as exc:
                req.failed = str(exc)
                self._log("ERR", req.packet_id, req.instruction, summary=f"write failed: {exc}")
                self._finish(req)
                continue
            req.sent_at = time.perf_counter()
            self.stats.tx_packets += 1
            self.stats.tx_bytes += len(req.raw)
            self._log("TX", req.packet_id, req.instruction,
                      summary=decode.describe_instruction(req.packet_id, req.instruction,
                                                          req.params),
                      raw=req.raw, tag=req.tag)
            if req.expect == 0:
                self._finish(req)
            else:
                self._current = req

    def _finish(self, req: Request) -> None:
        if req is self._current:
            self._current = None
        if req.callback is not None:
            req.callback(req)

    # ------------------------------------------------------------------ polling

    def poll(self) -> None:
        for note in self.transport.notes():
            self._log("INFO", summary=note)

        data = self.transport.poll()
        now = time.perf_counter()
        if data:
            self.stats.rx_bytes += len(data)
            self._rx_window += data
            if self.log_raw:
                self._log("RAW", summary=f"{len(data)} bytes received", raw=data)
            for packet_id, instruction, params in self.host.receive(data):
                self._on_packet(packet_id, instruction, params, now)
            invalid = self.host.invalid_packets
            if invalid != self._invalid_seen:
                count = invalid - self._invalid_seen
                self._invalid_seen = invalid
                self.stats.invalid_packets += count
                self._log("ERR", summary=f"{count} malformed/CRC-invalid packet(s) rejected "
                                         f"by dxl2_link_receive",
                          raw=bytes(self._rx_window[-64:]))
                self._rx_window.clear()

        req = self._current
        if req is not None and (now - req.sent_at) * 1000.0 >= self.timeout_ms:
            if req.expect < 0:
                self._finish(req)
            else:
                req.timed_out = True
                self.stats.timeouts += 1
                self._log("TIMEOUT", req.packet_id, req.instruction,
                          summary=f"no reply within {self.timeout_ms} ms "
                                  f"({len(req.replies)}/{req.expect} received)",
                          tag=req.tag)
                self._finish(req)
        self._start_next()

    def _on_packet(self, packet_id: int, instruction: int, params: bytes, now: float) -> None:
        self._rx_window.clear()
        self.stats.rx_packets += 1
        raw = self.host.build(packet_id, instruction, params)  # identical: encoding is canonical
        req = self._current

        if instruction != decode.INST_STATUS:
            self._log("RX", packet_id, instruction,
                      summary="instruction from another master: "
                              + decode.describe_instruction(packet_id, instruction, params),
                      raw=raw)
            return

        error = params[0] if params else 0
        if error:
            self.stats.status_errors += 1
        matches = req is not None and (req.packet_id in (packet_id, decode.BROADCAST_ID))
        latency = (now - req.sent_at) * 1000.0 if req is not None else None
        request_info: Optional[Tuple[int, int, bytes]] = (
            (req.packet_id, req.instruction, req.params) if matches and req else None)
        error_text, summary = decode.describe_status(params, request_info, self._name_values)
        if not matches:
            summary = ("unexpected status " if req is None else
                       f"status from ID {packet_id} while waiting for {req.packet_id}; ") + summary
        self._log("RX", packet_id, instruction, error=error_text, summary=summary, raw=raw,
                  latency_ms=latency if matches else None,
                  tag=req.tag if matches and req is not None else "")

        if matches and req is not None and latency is not None:
            self.stats.last_latency_ms = latency
            self.stats.latency_sum_ms += latency
            self.stats.latency_count += 1
            req.replies.append(Reply(packet_id, error, params[1:], latency))
            if 0 < req.expect <= len(req.replies):
                self._finish(req)

    def _name_values(self, address: int, data: bytes) -> str:
        return decode.format_items(self.native.table_items, address, data)

    def _log(self, direction: str, packet_id: Optional[int] = None,
             instruction: Optional[int] = None, error: str = "", summary: str = "",
             raw: bytes = b"", latency_ms: Optional[float] = None, tag: str = "") -> None:
        self.on_log(LogEntry(time.time(), direction, packet_id, instruction, error, summary, raw,
                             latency_ms, tag))

    def close(self) -> None:
        self._queue.clear()
        self._current = None
        try:
            self.transport.close()
        except TransportError:
            pass
