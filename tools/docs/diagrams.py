#******************************************************************************
# DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
#
# Description: Generates the SVG diagrams of the documentation
#              (docs/assets/images/*.svg). Run after changing a diagram:
#                  python tools/docs/diagrams.py
#
# Author:      Anton Vynohradov
# Email:       avynohradov@systemfromscratch.com
# License:     Apache License 2.0
# Copyright (c) 2026 Anton Vynohradov
#
# SPDX-License-Identifier: Apache-2.0
#*****************************************************************************

"""Documentation diagrams of DXL2-C, built with svgkit (sfs-protocol-library skill).
Preview every diagram (headless Chrome --screenshot) and check its labels against the
code after a change."""

from __future__ import annotations

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from svgkit import (BLUE, GREEN, GREY, LIGHT, LINE, NAVY, ORANGE, RED, TEAL, WHITE,  # noqa: E402
                    Svg, hero_scene)

OUT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..",
                                    "docs", "assets", "images"))


def packet_structure() -> None:
    s = Svg(960, 330, "DYNAMIXEL Protocol 2.0 packet structure")
    fields = [  # name, x, width, cells, color
        ("Header", 20, 240, ["FF", "FF", "FD", "00"], NAVY),
        ("ID", 270, 80, ["ID"], TEAL),
        ("Length", 360, 120, ["LEN_L", "LEN_H"], BLUE),
        ("Instruction", 490, 100, ["INST"], ORANGE),
        ("Parameters", 600, 200, ["P1", "P2", "…", "Pn"], GREY),
        ("CRC", 810, 130, ["CRC_L", "CRC_H"], NAVY),
    ]
    sizes = ["4 bytes", "1 byte", "2 bytes", "1 byte", "0 … n bytes", "2 bytes"]
    for (name, x, w, cells, color), size in zip(fields, sizes):
        s.text(x + w / 2, 40, name, size=15, weight="bold", fill=color)
        cell = w / len(cells)
        for i, value in enumerate(cells):
            s.rect(x + i * cell, 52, cell, 46, fill=color, stroke=WHITE, rx=4, sw=2)
            s.text(x + i * cell + cell / 2, 81, value, size=14, fill=WHITE, mono=True,
                   weight="bold")
        s.text(x + w / 2, 118, size, size=12, fill=GREY)
    s.bracket(490, 940, 150, "Length = Instruction + Parameters (after stuffing) + CRC", BLUE)
    s.bracket(20, 800, 200, "CRC-16 (poly 0x8005) over Header … last Parameter, on the wire bytes",
              NAVY)

    s.text(20, 262, "Example: PING to ID 1", size=14, weight="bold", anchor="start")
    example = [("FF FF FD 00", 20, 240, NAVY), ("01", 270, 80, TEAL), ("03 00", 360, 120, BLUE),
               ("01", 490, 100, ORANGE), ("none", 600, 200, GREY), ("19 4E", 810, 130, NAVY)]
    for value, x, w, color in example:
        s.rect(x, 274, w, 36, fill=WHITE, stroke=color, rx=4)
        s.text(x + w / 2, 298, value, size=14, fill=color, mono=True)
    s.save(OUT + "/packet-structure.svg")


def byte_stuffing() -> None:
    s = Svg(900, 300, "Byte stuffing in DYNAMIXEL Protocol 2.0")

    def row(y, label, values, highlight=None):
        s.text(20, y + 28, label, size=14, weight="bold", anchor="start")
        x = 250
        for i, value in enumerate(values):
            hot = highlight is not None and i == highlight
            s.rect(x, y, 70, 42, fill=ORANGE if hot else LIGHT,
                   stroke=ORANGE if hot else LINE, rx=5)
            s.text(x + 35, y + 27, value, size=15, mono=True, weight="bold",
                   fill=WHITE if hot else NAVY)
            x += 78

    row(30, "Application data", ["55", "FF", "FF", "FD", "12"])
    row(190, "On the wire", ["55", "FF", "FF", "FD", "FD", "12"], highlight=4)
    s.path("M560,82 C560,130 600,140 600,180", color=TEAL, arrow="teal", width=2)
    s.text(612, 126, "dxl2_encode(): after FF FF FD, insert FD", size=13, fill=TEAL,
           anchor="start")
    s.path("M380,186 C380,140 340,130 340,84", color=BLUE, arrow="end", width=2)
    s.text(330, 126, "parser: check CRC, then remove the FD", size=13, fill=BLUE, anchor="end")
    s.text(450, 268, "The Length field and the CRC cover the stuffed bytes. FF FF FD not "
           "followed by FD is a malformed packet.", size=12.5, fill=GREY)
    s.save(OUT + "/byte-stuffing.svg")


def architecture() -> None:
    s = Svg(960, 560, "DXL2-C architecture")
    # application
    s.rect(140, 20, 680, 90, fill="#FFF7EC", stroke=ORANGE)
    s.text(480, 52, "Application (your firmware or host tool)", size=17, weight="bold",
           fill=ORANGE)
    s.text(480, 80, "packet handler · Control Table · motor control · request/response logic",
           size=13.5, fill=GREY)
    # library boundary
    s.rect(120, 140, 720, 300, fill=WHITE, stroke=BLUE, rx=14, dash=True)
    s.text(140, 166, "DXL2-C library", size=14, weight="bold", fill=BLUE, anchor="start")
    # link
    s.rect(160, 180, 640, 80, fill=LIGHT, stroke=BLUE)
    s.text(480, 210, "dxl2_link — transport-independent link", size=16, weight="bold")
    s.text(480, 238, "dxl2_link_receive() · dxl2_link_send() · dxl2_link_send_status() · counters",
           size=13, fill=GREY, mono=True)
    # protocol
    for x, title, sub in ((160, "Parser", "dxl2_parser_feed()"),
                          (378, "Encoder", "dxl2_encode()"),
                          (596, "CRC-16", "dxl2_crc16()")):
        s.rect(x, 290, 204, 70, fill=LIGHT, stroke=TEAL)
        s.text(x + 102, 318, title, size=15, weight="bold", fill=TEAL)
        s.text(x + 102, 342, sub, size=12.5, fill=GREY, mono=True)
    s.text(480, 384, "dxl2_protocol — codec: framing, byte stuffing, CRC", size=13.5, fill=TEAL)
    s.rect(160, 396, 640, 32, fill=WHITE, stroke=LINE)
    s.text(480, 417, "dxl2_defs — instructions, error codes, IDs, little-endian helpers",
           size=12.5, fill=GREY)
    # transport
    s.rect(140, 470, 680, 70, fill=WHITE, stroke=GREY, dash=True)
    s.text(480, 498, "Transport (application-owned)", size=15, weight="bold", fill=GREY)
    s.text(480, 522, "UART / RS-485 / USB / TCP driver · RX interrupt · queue · thread",
           size=13, fill=GREY)
    # flows
    s.line(70, 500, 70, 70, color=TEAL, arrow="teal", width=2.4)
    s.line(70, 500, 138, 500, color=TEAL, width=2.4)
    s.line(70, 70, 138, 70, color=TEAL, arrow="teal", width=2.4)
    s.text(48, 290, "receive: bytes → packets", size=13, fill=TEAL, weight="bold", rotate=-90)
    s.line(890, 70, 890, 500, color=ORANGE, arrow="orange", width=2.4)
    s.line(822, 70, 890, 70, color=ORANGE, width=2.4)
    s.line(890, 500, 822, 500, color=ORANGE, arrow="orange", width=2.4)
    s.text(912, 290, "transmit: packets → bytes", size=13, fill=ORANGE, weight="bold", rotate=90)
    s.save(OUT + "/architecture.svg")


def data_flow() -> None:
    s = Svg(990, 310, "Receive and reply data flow")
    xs = [20, 214, 408, 602, 796]
    rx = [("UART RX ISR", "queue bytes"), ("main loop / thread", "drain the queue"),
          ("dxl2_link_receive()", "bytes in"), ("dxl2_parser_feed()", "one byte at a time"),
          ("packet handler", "your code")]
    s.text(20, 28, "Receive", size=14, weight="bold", fill=TEAL, anchor="start")
    for i, ((title, sub), x) in enumerate(zip(rx, xs)):
        s.rect(x, 40, 172, 64, fill=LIGHT, stroke=TEAL if i in (2, 3) else LINE)
        s.text(x + 86, 68, title, size=13.5, weight="bold", mono=title.endswith(")"))
        s.text(x + 86, 90, sub, size=12, fill=GREY)
        if i < 4:
            s.line(x + 172, 72, x + 192, 72, color=TEAL, arrow="teal", width=2)
    tx = [("dxl2_link_send_status()", "error + data"), ("dxl2_encode()", "stuffing + CRC"),
          ("write callback", "your transmit function"), ("UART TX driver", "bytes out")]
    s.text(990 - 20, 158, "Transmit", size=14, weight="bold", fill=ORANGE, anchor="end")
    s.line(882, 104, 882, 168, color=ORANGE, arrow="orange", width=2)
    for i, ((title, sub), x) in enumerate(zip(tx, reversed(xs[1:]))):
        s.rect(x, 170, 172, 64, fill=LIGHT, stroke=ORANGE if i in (0, 1) else LINE)
        s.text(x + 86, 198, title, size=13 if len(title) > 20 else 13.5, weight="bold",
               mono=title.endswith(")"))
        s.text(x + 86, 220, sub, size=12, fill=GREY)
        if i < 3:
            s.line(x, 202, x - 20, 202, color=ORANGE, arrow="orange", width=2)
    s.text(20, 284, "The handler runs inside dxl2_link_receive(), in the caller's context: "
           "reply from it, or queue the work and reply later.", size=12.5, fill=GREY,
           anchor="start")
    s.save(OUT + "/data-flow.svg")


def parser_flow() -> None:
    s = Svg(830, 840, "dxl2_parser_feed() algorithm")
    cx = 290

    def outcome(y, label, color):
        s.rect(580, y - 20, 230, 40, fill=WHITE, stroke=color, rx=20)
        s.text(695, y + 5, label, size=13, fill=color, weight="bold")

    def no_branch(y, half_width, label, color, answer="no"):
        s.line(cx + half_width, y, 578, y, arrow="end")
        s.text(cx + half_width + 10, y - 8, answer, size=12.5, fill=GREY, anchor="start")
        outcome(y, label, color)

    def down(y1, y2, label="yes"):
        s.line(cx, y1, cx, y2, arrow="end")
        s.text(cx + 8, y1 + 16, label, size=12.5, fill=GREY, anchor="start")

    s.rect(cx - 60, 14, 120, 36, fill=NAVY, stroke=NAVY, rx=18)
    s.text(cx, 37, "byte in", size=14, fill=WHITE, weight="bold")
    s.line(cx, 50, cx, 72, arrow="end")
    s.rect(cx - 150, 74, 300, 44, fill=LIGHT, stroke=BLUE)
    s.text(cx, 101, "append the byte to the buffer", size=13.5, weight="bold")
    s.line(cx, 118, cx, 146, arrow="end")
    s.diamond(cx, 196, 340, 96, ["buffer has ≥ 4 bytes and does", "not start with FF FF FD 00?"])
    s.line(cx + 170, 196, 578, 196, arrow="end")
    s.text(cx + 180, 188, "yes", size=12.5, fill=GREY, anchor="start")
    s.rect(580, 172, 230, 48, fill=LIGHT, stroke=BLUE)
    s.text(695, 194, "drop the first byte", size=13, weight="bold")
    s.text(695, 211, "(resynchronize on noise)", size=11.5, fill=GREY)
    s.path(f"M695,172 L695,132 L{cx + 2},132", arrow="end")
    down(244, 278, "no")
    s.diamond(cx, 322, 300, 84, ["7 bytes received", "(header, ID, Length)?"])
    no_branch(322, 150, "return 0 (need more)", GREY)
    down(364, 398)
    s.diamond(cx, 442, 300, 84, ["3 ≤ Length ≤", "MAX_PACKET_SIZE − 7?"])
    no_branch(442, 150, "reset, return ERROR", RED)
    down(484, 518)
    s.diamond(cx, 560, 300, 80, ["whole packet", "received?"])
    no_branch(560, 150, "return 0 (need more)", GREY)
    down(600, 634)
    s.diamond(cx, 676, 300, 80, ["CRC valid and", "byte stuffing valid?"])
    no_branch(676, 150, "reset, return ERROR", RED)
    down(716, 750)
    s.rect(cx - 170, 752, 340, 52, fill="#EEF7EF", stroke=GREEN)
    s.text(cx, 774, "unstuff into struct dxl2_packet", size=13.5, weight="bold")
    s.text(cx, 794, "reset the parser", size=12, fill=GREY)
    s.line(cx + 170, 778, 578, 778, arrow="end")
    outcome(778, "return 1 (packet ready)", GREEN)
    s.save(OUT + "/parser-flow.svg")


def bus_topology() -> None:
    s = Svg(980, 360, "DYNAMIXEL Protocol 2.0 bus topology")
    # controller
    s.rect(20, 110, 200, 130, fill=NAVY, stroke=NAVY, rx=12)
    s.text(120, 150, "Controller", size=17, weight="bold", fill=WHITE)
    s.text(120, 174, "MCU or PC", size=13, fill="#C9D6EA")
    s.text(120, 196, "DXL2-C + UART", size=13, fill="#C9D6EA")
    s.text(120, 218, "half-duplex transceiver", size=12, fill="#C9D6EA")
    # bus
    for y, label in ((150, "DATA"), (175, "VDD"), (200, "GND")):
        s.line(220, y, 940, y, color=LINE if label != "DATA" else TEAL,
               width=3 if label == "DATA" else 2)
        s.text(232, y - 5, label, size=10.5, fill=GREY, anchor="start")
    # servos
    for i, x in enumerate((340, 540, 740)):
        s.line(x + 60, 150, x + 60, 250, color=TEAL, width=2)
        s.rect(x, 250, 120, 90, fill=LIGHT, stroke=BLUE, rx=10)
        s.parts.append(f'<circle cx="{x + 60}" cy="{282}" r="18" fill="{WHITE}" '
                       f'stroke="{BLUE}" stroke-width="2"/>')
        s.parts.append(f'<circle cx="{x + 60}" cy="{282}" r="5" fill="{BLUE}"/>')
        s.text(x + 60, 326, f"Device ID {i + 1}", size=13, weight="bold")
    # packets
    s.rect(300, 60, 250, 38, fill=WHITE, stroke=ORANGE, rx=6)
    s.text(425, 84, "Instruction Packet → ID 2", size=13.5, fill=ORANGE, weight="bold")
    s.line(300, 120, 560, 120, color=ORANGE, arrow="orange", width=2.4)
    s.rect(610, 60, 250, 38, fill=WHITE, stroke=GREEN, rx=6)
    s.text(735, 84, "Status Packet ← ID 2", size=13.5, fill=GREEN, weight="bold")
    s.line(860, 120, 620, 120, color=GREEN, width=2.4)
    s.path("M620,120 L630,115 L630,125 z", color=GREEN, fill=GREEN)
    s.text(490, 30, "One shared half-duplex line: the controller asks, the addressed device "
           "answers.", size=13.5, fill=GREY)
    s.save(OUT + "/bus-topology.svg")


def logo() -> None:
    parts = [
        '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" width="64" height="64" '
        'role="img" aria-label="DXL2-C">',
        "<title>DXL2-C</title>",
        f'<rect x="2" y="2" width="60" height="60" rx="14" fill="{NAVY}"/>',
        f'<circle cx="32" cy="32" r="17" fill="none" stroke="{WHITE}" stroke-width="4"/>',
        f'<circle cx="32" cy="32" r="5" fill="{WHITE}"/>',
    ]
    for dx, dy in ((0, -11), (11, 0), (0, 11), (-11, 0)):
        parts.append(f'<circle cx="{32 + dx}" cy="{32 + dy}" r="2.4" fill="#5FD3DE"/>')
    parts.append(f'<path d="M6,52 L16,52 L20,46 L24,58 L28,52 L58,52" fill="none" '
                 f'stroke="#5FD3DE" stroke-width="3" stroke-linejoin="round" '
                 f'stroke-linecap="round"/>')
    parts.append("</svg>")
    path = os.path.join(OUT, "logo.svg")
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(parts) + "\n")
    print("wrote", os.path.relpath(path))


def main() -> None:
    os.makedirs(OUT, exist_ok=True)
    hero_scene(os.path.join(OUT, "hero-3d.svg"), chip_label="DXL2-C",
               chip_sub="Protocol 2.0", transceiver="RS-485",
               devices=["ID 1", "ID 2", "ID 3"], device_style="servo")
    packet_structure()
    byte_stuffing()
    architecture()
    data_flow()
    parser_flow()
    bus_topology()
    logo()


if __name__ == "__main__":
    main()
