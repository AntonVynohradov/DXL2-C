#******************************************************************************
# DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
#
# Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
# Description: ctypes binding of the native bridge (static DXL2 library linked
#              into dxl2_gui_native) and the helper that builds it with CMake.
#
# Author:      Anton Vynohradov
# Email:       avynohradov@systemfromscratch.com
#
# License:     Apache License 2.0
# Copyright (c) 2026 Anton Vynohradov
#
# SPDX-License-Identifier: Apache-2.0
#*****************************************************************************

"""Access to the C library from Python.

Every packet the GUI sends is encoded by ``dxl2_link_send`` and every byte it
receives is decoded by ``dxl2_link_receive``; the simulated device runs on its
own ``dxl2_link`` as well. Nothing here re-implements the protocol.
"""

from __future__ import annotations

import ctypes
import os
import shutil
import subprocess
import sys
from dataclasses import dataclass
from typing import Callable, List, Optional, Tuple

TOOL_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NATIVE_DIR = os.path.join(TOOL_DIR, "native")
BUILD_DIR = os.path.join(TOOL_DIR, "build")

if os.name == "nt":
    LIB_NAME = "dxl2_gui_native.dll"
elif sys.platform == "darwin":
    LIB_NAME = "dxl2_gui_native.dylib"
else:
    LIB_NAME = "dxl2_gui_native.so"

_u8 = ctypes.c_uint8
_u16 = ctypes.c_uint16
_u32 = ctypes.c_uint32
_size = ctypes.c_size_t
_ptr = ctypes.c_void_p
_bytes = ctypes.POINTER(ctypes.c_uint8)


class NativeLibraryError(RuntimeError):
    """The native bridge is missing or could not be loaded."""


def library_path() -> str:
    """Expected location of the built shared library."""
    return os.path.join(BUILD_DIR, "lib", LIB_NAME)


def _pick_generator() -> Optional[str]:
    if shutil.which("ninja"):
        return "Ninja"
    if shutil.which("make"):
        return "Unix Makefiles"
    if shutil.which("mingw32-make"):
        return "MinGW Makefiles"
    return None  # CMake default (Visual Studio on Windows)


def build(log: Callable[[str], None] = print) -> str:
    """Configure and build the native bridge with CMake; returns the library path."""
    cmake = shutil.which("cmake")
    if cmake is None:
        raise NativeLibraryError("CMake not found in PATH")

    configure = [cmake, "-S", NATIVE_DIR, "-B", BUILD_DIR, "-DCMAKE_BUILD_TYPE=Release"]
    generator = _pick_generator()
    if generator and not os.path.exists(os.path.join(BUILD_DIR, "CMakeCache.txt")):
        configure += ["-G", generator]

    for command in (configure, [cmake, "--build", BUILD_DIR, "--config", "Release"]):
        log("$ " + " ".join(command))
        process = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                 text=True, check=False)
        for line in process.stdout.splitlines():
            log(line)
        if process.returncode != 0:
            raise NativeLibraryError(f"command failed with exit code {process.returncode}")

    path = library_path()
    if not os.path.exists(path):
        # Multi-config generators put the file into a configuration folder
        alternative = os.path.join(BUILD_DIR, "lib", "Release", LIB_NAME)
        if os.path.exists(alternative):
            return alternative
        raise NativeLibraryError(f"build finished but {path} is missing")
    return path


def _find_library() -> Optional[str]:
    for path in (library_path(), os.path.join(BUILD_DIR, "lib", "Release", LIB_NAME)):
        if os.path.exists(path):
            return path
    return None


@dataclass(frozen=True)
class TableItem:
    """One Control Table item of the simulated device."""

    address: int
    size: int
    writable: bool
    name: str


class Native:
    """Loaded bridge library with typed function prototypes."""

    def __init__(self, path: Optional[str] = None) -> None:
        path = path or _find_library()
        if path is None:
            raise NativeLibraryError(f"{LIB_NAME} not built yet (expected in {BUILD_DIR})")
        try:
            self.lib = ctypes.CDLL(path)
        except OSError as exc:
            raise NativeLibraryError(f"cannot load {path}: {exc}") from exc
        self.path = path
        self._declare()
        self.max_packet_size = int(self.lib.dxl2_host_max_packet_size())
        self.table_items = self._load_items()

    def _declare(self) -> None:
        lib = self.lib
        signatures = {
            "dxl2_host_size": (_size, []),
            "dxl2_host_max_packet_size": (_size, []),
            "dxl2_host_init": (ctypes.c_int, [_ptr]),
            "dxl2_host_build": (ctypes.c_int, [_ptr, _u8, _u8, _bytes, _u16, _bytes, _size]),
            "dxl2_host_receive": (_size, [_ptr, _bytes, _size]),
            "dxl2_host_take": (ctypes.c_int, [_ptr, _bytes, _bytes, _bytes, _size]),
            "dxl2_host_flush": (None, [_ptr]),
            "dxl2_host_rx_packets": (_u32, [_ptr]),
            "dxl2_host_invalid_packets": (_u32, [_ptr]),
            "dxl2_host_crc16": (_u16, [_bytes, _size]),
            "dxl2_sim_size": (_size, []),
            "dxl2_sim_init": (ctypes.c_int, [_ptr, _u8]),
            "dxl2_sim_process": (_size, [_ptr, _bytes, _size, _bytes, _size]),
            "dxl2_sim_tick": (None, [_ptr]),
            "dxl2_sim_id": (_u8, [_ptr]),
            "dxl2_sim_read_table": (ctypes.c_int, [_ptr, _u16, _bytes, _u16]),
            "dxl2_sim_set_hardware_error": (None, [_ptr, _u8]),
            "dxl2_sim_invalid_packets": (_u32, [_ptr]),
            "dxl2_sim_item_count": (_size, []),
            "dxl2_sim_item_info": (ctypes.c_int, [_size, ctypes.POINTER(_u16), _bytes, _bytes,
                                                  ctypes.POINTER(ctypes.c_char_p)]),
        }
        for name, (restype, argtypes) in signatures.items():
            function = getattr(lib, name)
            function.restype = restype
            function.argtypes = argtypes

    def _load_items(self) -> List[TableItem]:
        items = []
        address, size, writable = _u16(), _u8(), _u8()
        name = ctypes.c_char_p()
        for index in range(self.lib.dxl2_sim_item_count()):
            if self.lib.dxl2_sim_item_info(index, ctypes.byref(address), ctypes.byref(size),
                                           ctypes.byref(writable), ctypes.byref(name)) == 0:
                items.append(TableItem(address.value, size.value, bool(writable.value),
                                       name.value.decode("ascii")))
        return items

    def crc16(self, data: bytes) -> int:
        return int(self.lib.dxl2_host_crc16(_in(data), len(data)))


def _in(data: bytes):
    """Read-only byte buffer for a ``const uint8_t*`` argument."""
    return (ctypes.c_uint8 * max(len(data), 1)).from_buffer_copy(bytes(data) or b"\x00")


def _state(size: int):
    """Zeroed, 8-byte aligned storage for an opaque C structure."""
    return (ctypes.c_uint64 * ((size + 7) // 8))()


class Host:
    """Controller-side link (struct dxl2_host)."""

    def __init__(self, native: Native) -> None:
        self.native = native
        self._lib = native.lib
        self._state = _state(self._lib.dxl2_host_size())
        if self._lib.dxl2_host_init(self._state) != 0:
            raise NativeLibraryError("dxl2_host_init failed")
        self._params = (ctypes.c_uint8 * native.max_packet_size)()

    def build(self, packet_id: int, instruction: int, params: bytes = b"") -> bytes:
        """Encode an Instruction Packet with dxl2_link_send."""
        out = (ctypes.c_uint8 * self.native.max_packet_size)()
        n = self._lib.dxl2_host_build(self._state, packet_id & 0xFF, instruction & 0xFF,
                                      _in(params), len(params), out, len(out))
        if n < 0:
            raise ValueError(f"packet does not fit the library limit "
                             f"({self.native.max_packet_size} bytes)")
        return bytes(out[:n])

    def receive(self, data: bytes) -> List[Tuple[int, int, bytes]]:
        """Feed bytes; returns the decoded packets as (id, instruction, params)."""
        if data:
            self._lib.dxl2_host_receive(self._state, _in(data), len(data))
        packets = []
        packet_id, instruction = _u8(), _u8()
        while True:
            n = self._lib.dxl2_host_take(self._state, ctypes.byref(packet_id),
                                         ctypes.byref(instruction), self._params,
                                         len(self._params))
            if n < 0:
                return packets
            packets.append((packet_id.value, instruction.value, bytes(self._params[:n])))

    def flush(self) -> None:
        self._lib.dxl2_host_flush(self._state)

    @property
    def rx_packets(self) -> int:
        return int(self._lib.dxl2_host_rx_packets(self._state))

    @property
    def invalid_packets(self) -> int:
        return int(self._lib.dxl2_host_invalid_packets(self._state))


class SimDevice:
    """Simulated DYNAMIXEL device (struct dxl2_sim) running on its own link."""

    TX_CAPACITY = 4096

    def __init__(self, native: Native, device_id: int = 1) -> None:
        self.native = native
        self._lib = native.lib
        self._state = _state(self._lib.dxl2_sim_size())
        self._out = (ctypes.c_uint8 * self.TX_CAPACITY)()
        self.reset(device_id)

    def reset(self, device_id: int) -> None:
        if self._lib.dxl2_sim_init(self._state, device_id) != 0:
            raise ValueError(f"invalid device ID {device_id}")

    def process(self, data: bytes) -> bytes:
        """Feed request bytes; returns the Status Packet bytes the device sends."""
        n = self._lib.dxl2_sim_process(self._state, _in(data), len(data), self._out,
                                       len(self._out))
        return bytes(self._out[:n])

    def tick(self) -> None:
        self._lib.dxl2_sim_tick(self._state)

    @property
    def device_id(self) -> int:
        return int(self._lib.dxl2_sim_id(self._state))

    def read_table(self, address: int = 0, length: Optional[int] = None) -> bytes:
        last = self.native.table_items[-1]
        length = (last.address + last.size - address) if length is None else length
        out = (ctypes.c_uint8 * length)()
        if self._lib.dxl2_sim_read_table(self._state, address, out, length) != 0:
            raise ValueError("range outside the Control Table")
        return bytes(out)

    def set_hardware_error(self, value: int) -> None:
        self._lib.dxl2_sim_set_hardware_error(self._state, value & 0xFF)

    @property
    def invalid_packets(self) -> int:
        return int(self._lib.dxl2_sim_invalid_packets(self._state))
