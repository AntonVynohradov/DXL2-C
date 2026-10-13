#******************************************************************************
# DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
#
# Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
# Description: pytest suite: Python codec tests and the C/Python cross-check.
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

"""Unit tests for the Python Protocol 2.0 codec and the C cross-check.

Run from the repository root:   python -m pytest tests/integration -v
"""

from __future__ import annotations

import ctypes
import os
import random
import shutil
import subprocess
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))

from dxl2 import protocol as p  # noqa: E402

LIBRARY_ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))


class ProtocolTests(unittest.TestCase):
    def test_ping_reference_packet(self):
        # Example from https://docs.robotis.com/docs/dxl/protocol/protocol2/
        self.assertEqual(p.build_packet(1, p.INST_PING),
                         bytes.fromhex("FF FF FD 00 01 03 00 01 19 4E"))

    def test_ping_reference_status(self):
        raw = bytes.fromhex("FF FF FD 00 01 07 00 55 00 06 04 26 65 5D")
        pk = p.parse_one(raw)
        self.assertIsNotNone(pk)
        self.assertEqual(pk.id, 1)
        self.assertTrue(pk.is_status)
        self.assertEqual(pk.error, 0)
        self.assertEqual(pk.data, bytes((0x06, 0x04, 0x26)))
        self.assertEqual(p.build_status(1, 0, bytes((0x06, 0x04, 0x26))), raw)

    def test_stuffing_round_trip(self):
        params = bytes((0x74, 0x00, 0xFF, 0xFF, 0xFD, 0xFF, 0xFF, 0xFD))
        raw = p.build_packet(1, p.INST_WRITE, params)
        self.assertIn(b"\xFF\xFF\xFD\xFD", raw)
        pk = p.parse_one(raw)
        self.assertEqual(pk.params, params)

    def test_crc_error_is_rejected(self):
        raw = bytearray(p.build_packet(1, p.INST_PING))
        raw[-1] ^= 0xFF
        parser = p.Parser()
        self.assertEqual(parser.feed(bytes(raw)), [])
        self.assertEqual(parser.crc_errors, 1)

    def test_chunked_stream_with_garbage(self):
        a = p.build_status(1, 0, b"\x01\x02")
        b = p.build_status(2, 0x07)
        stream = b"\x00\xFF\x13" + a + b"\xFF\xFF" + b
        parser = p.Parser()
        out = []
        for i in range(len(stream)):
            out += parser.feed(stream[i:i + 1])
        self.assertEqual([(x.id, x.error) for x in out], [(1, 0), (2, 7)])

    def test_error_text(self):
        self.assertEqual(p.error_text(0x07), "Access Error")
        self.assertIn("Hardware Alert", p.error_text(0x80))


@unittest.skipUnless(shutil.which("gcc"), "gcc not available")
class CCodecCrossTest(unittest.TestCase):
    """Builds the C library sources as a shared library and checks that the
    Python codec and the C codec produce/accept identical packets."""

    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.mkdtemp()
        lib = os.path.join(cls.tmp, "dxl2.dll" if os.name == "nt" else "libdxl2.so")
        subprocess.check_call(["gcc", "-std=c11", "-shared", "-fPIC", "-O1",
                               "-I", LIBRARY_ROOT,
                               "-I", os.path.join(LIBRARY_ROOT, "inc"),
                               os.path.join(LIBRARY_ROOT, "src", "dxl2_protocol.c"), "-o", lib])
        cls.lib = ctypes.CDLL(lib)
        cls.lib.dxl2_encode.argtypes = [ctypes.c_uint8, ctypes.c_uint8, ctypes.c_char_p,
                                        ctypes.c_uint16, ctypes.c_char_p, ctypes.c_size_t,
                                        ctypes.POINTER(ctypes.c_size_t)]
        cls.lib.dxl2_parser_init.argtypes = [ctypes.c_void_p]
        cls.lib.dxl2_parser_feed.argtypes = [ctypes.c_void_p, ctypes.c_uint8, ctypes.c_void_p]

    @classmethod
    def tearDownClass(cls):
        # Windows keeps the DLL locked while it is loaded; leave it behind there.
        shutil.rmtree(cls.tmp, ignore_errors=True)

    def _c_encode(self, dxl_id, inst, params):
        out = ctypes.create_string_buffer(256)
        n = ctypes.c_size_t()
        rc = self.lib.dxl2_encode(dxl_id, inst, params, len(params), out, 256, ctypes.byref(n))
        self.assertEqual(rc, 0)
        return out.raw[:n.value]

    def test_encoders_match_and_c_parser_accepts_python_packets(self):
        rng = random.Random(1234)
        parser = ctypes.create_string_buffer(512)  # >= sizeof(struct dxl2_parser)
        packet = ctypes.create_string_buffer(512)  # >= sizeof(struct dxl2_packet)
        for _ in range(300):
            n = rng.randint(0, 60)
            params = bytes(rng.choice((0xFF, 0xFD, 0x00, rng.randint(0, 255)))
                           for _ in range(n))
            dxl_id, inst = rng.randint(0, 252), rng.choice((1, 2, 3, 0x55, 0x82, 0x83))
            py = p.build_packet(dxl_id, inst, params)
            self.assertEqual(py, self._c_encode(dxl_id, inst, params), params.hex())
            self.lib.dxl2_parser_init(parser)
            results = [self.lib.dxl2_parser_feed(parser, b, packet) for b in py]
            self.assertEqual(results[-1], 1)
            self.assertEqual(packet.raw[0], dxl_id)
            self.assertEqual(packet.raw[1], inst)


if __name__ == "__main__":
    unittest.main()
