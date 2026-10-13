#!/usr/bin/env python3
#******************************************************************************
# DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
#
# Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
# Description: Entry point of the DXL2 Studio GUI (simulation and serial port).
#
# Author:      Anton Vynohradov
# Email:       avynohradov@systemfromscratch.com
#
# License:     Apache License 2.0
# Copyright (c) 2026 Anton Vynohradov
#
# SPDX-License-Identifier: Apache-2.0
#*****************************************************************************

"""Run from the repository root:  python tools/dxl2_gui/dxl2_gui.py"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from dxl2gui.app import main  # noqa: E402

if __name__ == "__main__":
    main()
