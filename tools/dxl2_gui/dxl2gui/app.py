#******************************************************************************
# DXL2-C: DYNAMIXEL Protocol 2.0 Implementation
#
# Project:     DXL2-C - Hardware-Agnostic DYNAMIXEL Protocol 2.0 Library
# Description: Tkinter user interface of the DXL2 GUI tool.
#
# Author:      Anton Vynohradov
# Email:       avynohradov@systemfromscratch.com
#
# License:     Apache License 2.0
# Copyright (c) 2026 Anton Vynohradov
#
# SPDX-License-Identifier: Apache-2.0
#*****************************************************************************

"""Main window: connection bar, control tabs and the communication debugger."""

from __future__ import annotations

import argparse
import csv
import datetime
import threading
import time
import tkinter as tk
from tkinter import filedialog, messagebox, simpledialog, ttk
from typing import Dict, List, Optional

from . import decode, native, transport
from .session import LogEntry, Request, Session, Statistics

POLL_MS = 5
LOG_LIMIT = 5000
MONO = ("Consolas", 10)

COLORS = {
    "TX": "#1f5fbf",
    "RX": "#1a7f37",
    "ERR": "#c62828",
    "TIMEOUT": "#d9730d",
    "INFO": "#6b6b6b",
    "RAW": "#7b3fa0",
    "STATUSERR": "#b00020",
}

ADDR_TORQUE = 64
ADDR_LED = 65
ADDR_HW_ERROR = 70
ADDR_GOAL_POSITION = 116
ADDR_MOVING = 122
ADDR_PRESENT_POSITION = 132


class App(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title("DXL2 Studio - DYNAMIXEL Protocol 2.0")
        self.geometry("1360x820")
        self.minsize(1080, 640)

        self.native: Optional[native.Native] = None
        self.session: Optional[Session] = None
        self.sim: Optional[transport.SimulationTransport] = None
        self.log_entries: List[LogEntry] = []
        self.log_rows: Dict[str, LogEntry] = {}
        self.log_start: Optional[float] = None
        self.last_log_time: Optional[float] = None
        self.table_values: Dict[int, bytes] = {}
        self.scan_running = False

        self._init_style()
        self._init_vars()
        self._build_ui()
        self._load_native()
        self.protocol("WM_DELETE_WINDOW", self._on_close)
        self._poll_job = self.after(POLL_MS, self._poll)
        self._periodic_job = self.after(250, self._periodic)

    # ================================================================== setup

    def _init_style(self) -> None:
        style = ttk.Style(self)
        for theme in ("vista", "clam"):
            if theme in style.theme_names():
                style.theme_use(theme)
                break
        style.configure("Treeview", rowheight=22)
        style.configure("Status.TLabel", padding=(6, 2))
        style.configure("Big.TLabel", font=("Segoe UI", 18, "bold"))
        style.configure("Hint.TLabel", foreground="#6b6b6b")
        style.configure("Accent.TButton", font=("Segoe UI", 9, "bold"))

    def _init_vars(self) -> None:
        self.mode = tk.StringVar(value="sim")
        self.port = tk.StringVar()
        self.baud = tk.StringVar(value="57600")
        self.timeout = tk.IntVar(value=100)
        self.sim_id = tk.IntVar(value=1)
        self.echo = tk.BooleanVar(value=False)
        self.target_id = tk.IntVar(value=1)
        self.goal = tk.IntVar(value=2048)
        self.auto_poll = tk.BooleanVar(value=False)
        self.auto_table = tk.BooleanVar(value=False)
        self.paused = tk.BooleanVar(value=False)
        self.autoscroll = tk.BooleanVar(value=True)
        self.raw_bytes = tk.BooleanVar(value=False)
        self.hide_poll = tk.BooleanVar(value=False)
        self.filter_text = tk.StringVar()
        self.status_text = tk.StringVar(value="Disconnected")
        self.stats_text = tk.StringVar(value="")
        self.present_text = tk.StringVar(value="-")
        self.drop = tk.IntVar(value=0)
        self.corrupt = tk.IntVar(value=0)
        self.delay = tk.IntVar(value=0)
        self.split = tk.BooleanVar(value=False)
        self.hw_error = tk.StringVar(value="None")

    # ================================================================== layout

    def _build_ui(self) -> None:
        self._build_menu()
        self._build_connection_bar()

        paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=6, pady=(0, 4))

        self.tabs = ttk.Notebook(paned)
        paned.add(self.tabs, weight=2)
        self._build_quick_tab()
        self._build_builder_tab()
        self._build_table_tab()
        self._build_scan_tab()
        self._build_sim_tab()

        paned.add(self._build_debug_panel(paned), weight=3)
        self._on_mode()

        bar = ttk.Frame(self, relief=tk.SUNKEN)
        bar.pack(fill=tk.X, side=tk.BOTTOM)
        ttk.Label(bar, textvariable=self.status_text, style="Status.TLabel").pack(side=tk.LEFT)
        ttk.Label(bar, textvariable=self.stats_text, style="Status.TLabel").pack(side=tk.RIGHT)

    def _build_menu(self) -> None:
        menu = tk.Menu(self)
        file_menu = tk.Menu(menu, tearoff=False)
        file_menu.add_command(label="Export log...", accelerator="Ctrl+E",
                              command=self._export_log)
        file_menu.add_command(label="Clear log", accelerator="Ctrl+L", command=self._clear_log)
        file_menu.add_separator()
        file_menu.add_command(label="Rebuild native library", command=self._build_native)
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self._on_close)
        menu.add_cascade(label="File", menu=file_menu)
        help_menu = tk.Menu(menu, tearoff=False)
        help_menu.add_command(label="About", command=self._about)
        menu.add_cascade(label="Help", menu=help_menu)
        self.config(menu=menu)
        self.bind_all("<Control-e>", lambda _e: self._export_log())
        self.bind_all("<Control-l>", lambda _e: self._clear_log())
        self.bind_all("<F5>", lambda _e: self._toggle_connection())

    def _build_connection_bar(self) -> None:
        box = ttk.LabelFrame(self, text="Connection", padding=(8, 4))
        box.pack(fill=tk.X, padx=6, pady=6)

        mode = ttk.Frame(box)
        mode.pack(side=tk.LEFT)
        self.lockable: List[tk.Widget] = []
        for text, value in (("Simulation (static library)", "sim"),
                            ("Serial port (embedded device)", "serial")):
            button = ttk.Radiobutton(mode, text=text, value=value, variable=self.mode,
                                     command=self._on_mode)
            button.pack(anchor=tk.W)
            self.lockable.append(button)
        ttk.Separator(box, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=10)

        self.sim_options = ttk.Frame(box)
        ttk.Label(self.sim_options, text="Simulated device ID").pack(side=tk.LEFT)
        spin = ttk.Spinbox(self.sim_options, from_=0, to=252, width=5, textvariable=self.sim_id)
        spin.pack(side=tk.LEFT, padx=4)
        self.lockable.append(spin)

        self.serial_options = ttk.Frame(box)
        ttk.Label(self.serial_options, text="Port").pack(side=tk.LEFT)
        self.port_box = ttk.Combobox(self.serial_options, textvariable=self.port, width=28)
        self.port_box.pack(side=tk.LEFT, padx=4)
        refresh = ttk.Button(self.serial_options, text="Refresh", width=8,
                             command=self._refresh_ports)
        refresh.pack(side=tk.LEFT)
        ttk.Label(self.serial_options, text="Baud").pack(side=tk.LEFT, padx=(10, 0))
        baud = ttk.Combobox(self.serial_options, textvariable=self.baud, width=9,
                            values=[str(b) for b in transport.BAUD_RATES])
        baud.pack(side=tk.LEFT, padx=4)
        echo = ttk.Checkbutton(self.serial_options, text="Discard echo (half-duplex)",
                               variable=self.echo)
        echo.pack(side=tk.LEFT, padx=6)
        self.lockable += [self.port_box, refresh, baud, echo]

        right = ttk.Frame(box)
        right.pack(side=tk.RIGHT)
        ttk.Label(right, text="Reply timeout").pack(side=tk.LEFT)
        ttk.Spinbox(right, from_=5, to=5000, increment=10, width=6,
                    textvariable=self.timeout).pack(side=tk.LEFT, padx=4)
        ttk.Label(right, text="ms").pack(side=tk.LEFT, padx=(0, 10))
        self.led = tk.Canvas(right, width=16, height=16, highlightthickness=0)
        self.led_dot = self.led.create_oval(2, 2, 14, 14, fill="#b0b0b0", outline="")
        self.led.pack(side=tk.LEFT, padx=4)
        self.connect_btn = ttk.Button(right, text="Connect (F5)", style="Accent.TButton",
                                      command=self._toggle_connection, width=16)
        self.connect_btn.pack(side=tk.LEFT)

    def _build_quick_tab(self) -> None:
        tab = ttk.Frame(self.tabs, padding=10)
        self.tabs.add(tab, text="Quick control")

        top = ttk.Frame(tab)
        top.pack(fill=tk.X)
        ttk.Label(top, text="Target ID").pack(side=tk.LEFT)
        ttk.Spinbox(top, from_=0, to=254, width=5, textvariable=self.target_id).pack(
            side=tk.LEFT, padx=4)
        ttk.Label(top, text="(254 = broadcast)", style="Hint.TLabel").pack(side=tk.LEFT)

        basic = ttk.LabelFrame(tab, text="Device", padding=8)
        basic.pack(fill=tk.X, pady=8)
        for col, (text, cmd) in enumerate((
                ("Ping", self._ping),
                ("Reboot", lambda: self._simple(0x08)),
                ("Read HW error", self._read_hw_error),
                ("Factory reset...", self._factory_reset))):
            ttk.Button(basic, text=text, command=cmd).grid(row=0, column=col, padx=3, sticky=tk.EW)
        for col, (text, cmd) in enumerate((
                ("Torque ON", lambda: self._write_u(ADDR_TORQUE, 1, 1)),
                ("Torque OFF", lambda: self._write_u(ADDR_TORQUE, 0, 1)),
                ("LED ON", lambda: self._write_u(ADDR_LED, 1, 1)),
                ("LED OFF", lambda: self._write_u(ADDR_LED, 0, 1)))):
            ttk.Button(basic, text=text, command=cmd).grid(row=1, column=col, padx=3, pady=4,
                                                           sticky=tk.EW)
        for col in range(4):
            basic.columnconfigure(col, weight=1)

        motion = ttk.LabelFrame(tab, text="Position (Goal Position 116 / Present Position 132)",
                                padding=8)
        motion.pack(fill=tk.X)
        ttk.Scale(motion, from_=0, to=4095, variable=self.goal, orient=tk.HORIZONTAL,
                  command=lambda v: self.goal.set(int(float(v)))).pack(fill=tk.X)
        row = ttk.Frame(motion)
        row.pack(fill=tk.X, pady=4)
        ttk.Label(row, text="Goal").pack(side=tk.LEFT)
        ttk.Spinbox(row, from_=0, to=4095, width=7, textvariable=self.goal).pack(side=tk.LEFT,
                                                                                padx=4)
        ttk.Button(row, text="Move", command=self._move).pack(side=tk.LEFT, padx=4)
        ttk.Button(row, text="Read present", command=self._read_present).pack(side=tk.LEFT)
        ttk.Checkbutton(row, text="Auto-poll (200 ms)", variable=self.auto_poll).pack(
            side=tk.LEFT, padx=8)
        ttk.Label(motion, textvariable=self.present_text, style="Big.TLabel").pack(anchor=tk.W)
        self.position_canvas = tk.Canvas(motion, height=26, background="#f4f4f4",
                                         highlightthickness=0)
        self.position_canvas.pack(fill=tk.X, pady=(4, 0))

        hint = ("Tip: the Control Table layout follows the X-series (XL430-W250).\n"
                "Torque must be ON for the simulated device to move. EEPROM items\n"
                "(address < 64) can only be written with torque OFF.")
        ttk.Label(tab, text=hint, style="Hint.TLabel", justify=tk.LEFT).pack(anchor=tk.W,
                                                                           pady=10)

    def _build_builder_tab(self) -> None:
        tab = ttk.Frame(self.tabs, padding=10)
        self.tabs.add(tab, text="Packet builder")

        self.b_inst = tk.StringVar(value="READ")
        self.b_id = tk.IntVar(value=1)
        self.b_addr = tk.IntVar(value=ADDR_PRESENT_POSITION)
        self.b_len = tk.IntVar(value=4)
        self.b_value = tk.StringVar(value="0")
        self.b_size = tk.StringVar(value="4")
        self.b_ids = tk.StringVar(value="1 2 3")
        self.b_params = tk.StringVar()
        self.b_raw_mode = tk.BooleanVar(value=False)
        self.b_preview = tk.StringVar()
        self.raw_send = tk.StringVar()

        form = ttk.LabelFrame(tab, text="Instruction Packet (encoded by dxl2_link_send)",
                              padding=8)
        form.pack(fill=tk.X)
        names = [n for n in decode.INSTRUCTIONS.values() if n != "STATUS"]
        rows = (
            ("Instruction", ttk.Combobox(form, textvariable=self.b_inst, values=names,
                                         state="readonly", width=18)),
            ("ID", ttk.Spinbox(form, from_=0, to=254, textvariable=self.b_id, width=8)),
            ("Address", ttk.Spinbox(form, from_=0, to=65535, textvariable=self.b_addr, width=8)),
            ("Length", ttk.Spinbox(form, from_=1, to=255, textvariable=self.b_len, width=8)),
            ("Value", ttk.Entry(form, textvariable=self.b_value, width=20)),
            ("Value size", ttk.Combobox(form, textvariable=self.b_size, values=("1", "2", "4"),
                                        state="readonly", width=6)),
            ("IDs (sync read)", ttk.Entry(form, textvariable=self.b_ids, width=20)),
        )
        self.b_widgets = {}
        for r, (label, widget) in enumerate(rows):
            ttk.Label(form, text=label).grid(row=r, column=0, sticky=tk.W, pady=2)
            widget.grid(row=r, column=1, sticky=tk.W, pady=2, padx=6)
            self.b_widgets[label] = widget
        ttk.Checkbutton(form, text="Raw parameters (hex):", variable=self.b_raw_mode).grid(
            row=7, column=0, sticky=tk.W, pady=(6, 2))
        ttk.Entry(form, textvariable=self.b_params, width=40).grid(row=7, column=1, sticky=tk.EW,
                                                                  padx=6)
        self.b_hint = ttk.Label(form, style="Hint.TLabel", justify=tk.LEFT, wraplength=420)
        self.b_hint.grid(row=8, column=0, columnspan=2, sticky=tk.W, pady=(6, 0))

        preview = ttk.LabelFrame(tab, text="Preview", padding=8)
        preview.pack(fill=tk.X, pady=8)
        ttk.Label(preview, textvariable=self.b_preview, font=MONO, wraplength=520,
                  justify=tk.LEFT).pack(anchor=tk.W)
        buttons = ttk.Frame(preview)
        buttons.pack(fill=tk.X, pady=(6, 0))
        ttk.Button(buttons, text="Send", style="Accent.TButton",
                   command=self._builder_send).pack(side=tk.LEFT)
        ttk.Button(buttons, text="Copy to raw sender",
                   command=self._builder_to_raw).pack(side=tk.LEFT, padx=6)

        raw = ttk.LabelFrame(tab, text="Raw bytes (bypass the encoder - test the receiver)",
                             padding=8)
        raw.pack(fill=tk.X)
        ttk.Entry(raw, textvariable=self.raw_send, font=MONO).pack(fill=tk.X)
        raw_buttons = ttk.Frame(raw)
        raw_buttons.pack(fill=tk.X, pady=(6, 0))
        ttk.Button(raw_buttons, text="Send raw", command=self._send_raw).pack(side=tk.LEFT)
        ttk.Button(raw_buttons, text="Break CRC",
                   command=lambda: self._mutate_raw("crc")).pack(side=tk.LEFT, padx=4)
        ttk.Button(raw_buttons, text="Truncate",
                   command=lambda: self._mutate_raw("cut")).pack(side=tk.LEFT)
        ttk.Button(raw_buttons, text="Add noise",
                   command=lambda: self._mutate_raw("noise")).pack(side=tk.LEFT, padx=4)

        for var in (self.b_inst, self.b_id, self.b_addr, self.b_len, self.b_value, self.b_size,
                    self.b_ids, self.b_params, self.b_raw_mode):
            var.trace_add("write", lambda *_: self._update_preview())

    def _build_table_tab(self) -> None:
        tab = ttk.Frame(self.tabs, padding=10)
        self.tabs.add(tab, text="Control table")
        bar = ttk.Frame(tab)
        bar.pack(fill=tk.X)
        ttk.Button(bar, text="Read all", command=self._read_table).pack(side=tk.LEFT)
        ttk.Checkbutton(bar, text="Auto refresh (500 ms)", variable=self.auto_table).pack(
            side=tk.LEFT, padx=8)
        ttk.Label(bar, text="Double-click a RW item to write it", style="Hint.TLabel").pack(
            side=tk.RIGHT)

        columns = ("addr", "name", "size", "access", "value", "hex")
        self.table = ttk.Treeview(tab, columns=columns, show="headings", selectmode="browse")
        for col, text, width, anchor in (("addr", "Addr", 50, tk.E), ("name", "Item", 190, tk.W),
                                         ("size", "Size", 45, tk.CENTER),
                                         ("access", "Access", 60, tk.CENTER),
                                         ("value", "Value", 100, tk.E),
                                         ("hex", "Hex", 110, tk.W)):
            self.table.heading(col, text=text)
            self.table.column(col, width=width, anchor=anchor, stretch=col == "name")
        self.table.tag_configure("ro", foreground="#555555")
        self.table.tag_configure("changed", background="#fff3c4")
        scroll = ttk.Scrollbar(tab, orient=tk.VERTICAL, command=self.table.yview)
        self.table.configure(yscrollcommand=scroll.set)
        scroll.pack(side=tk.RIGHT, fill=tk.Y, pady=6)
        self.table.pack(fill=tk.BOTH, expand=True, pady=6)
        self.table.bind("<Double-1>", self._edit_table_item)

    def _build_scan_tab(self) -> None:
        tab = ttk.Frame(self.tabs, padding=10)
        self.tabs.add(tab, text="Bus scan")
        self.scan_from = tk.IntVar(value=0)
        self.scan_to = tk.IntVar(value=252)
        bar = ttk.Frame(tab)
        bar.pack(fill=tk.X)
        ttk.Label(bar, text="IDs").pack(side=tk.LEFT)
        ttk.Spinbox(bar, from_=0, to=252, width=5, textvariable=self.scan_from).pack(
            side=tk.LEFT, padx=4)
        ttk.Label(bar, text="to").pack(side=tk.LEFT)
        ttk.Spinbox(bar, from_=0, to=252, width=5, textvariable=self.scan_to).pack(
            side=tk.LEFT, padx=4)
        self.scan_btn = ttk.Button(bar, text="Scan (ping each)", command=self._scan)
        self.scan_btn.pack(side=tk.LEFT, padx=4)
        ttk.Button(bar, text="Broadcast ping", command=self._broadcast_ping).pack(side=tk.LEFT)
        self.scan_progress = ttk.Progressbar(tab, mode="determinate")
        self.scan_progress.pack(fill=tk.X, pady=6)
        self.scan_tree = ttk.Treeview(tab, columns=("id", "model", "fw", "latency"),
                                      show="headings")
        for col, text in (("id", "ID"), ("model", "Model number"), ("fw", "Firmware"),
                          ("latency", "Latency [ms]")):
            self.scan_tree.heading(col, text=text)
            self.scan_tree.column(col, width=90, anchor=tk.CENTER)
        self.scan_tree.pack(fill=tk.BOTH, expand=True)
        self.scan_tree.bind("<Double-1>", self._use_scanned_id)
        ttk.Label(tab, text="Double-click a device to make it the target ID",
                  style="Hint.TLabel").pack(anchor=tk.W, pady=4)

    def _build_sim_tab(self) -> None:
        tab = ttk.Frame(self.tabs, padding=10)
        self.sim_tab = tab
        self.tabs.add(tab, text="Simulator")
        info = ("The simulated device runs the DXL2 static library on its own dxl2_link.\n"
                "Use fault injection to check how the master copes with a bad bus.")
        ttk.Label(tab, text=info, style="Hint.TLabel", justify=tk.LEFT).pack(anchor=tk.W)

        faults = ttk.LabelFrame(tab, text="Fault injection (device -> master)", padding=8)
        faults.pack(fill=tk.X, pady=8)
        for r, (label, var, upper) in enumerate((("Drop replies [%]", self.drop, 100),
                                                 ("Corrupt replies [%]", self.corrupt, 100),
                                                 ("Reply delay [ms]", self.delay, 2000))):
            ttk.Label(faults, text=label).grid(row=r, column=0, sticky=tk.W, pady=2)
            ttk.Scale(faults, from_=0, to=upper, variable=var, orient=tk.HORIZONTAL,
                      length=220, command=lambda v, var=var: var.set(int(float(v)))).grid(
                row=r, column=1, padx=6)
            ttk.Label(faults, textvariable=var, width=5).grid(row=r, column=2)
        ttk.Checkbutton(faults, text="Split replies into two chunks", variable=self.split).grid(
            row=3, column=0, columnspan=3, sticky=tk.W, pady=4)
        for var in (self.drop, self.corrupt, self.delay, self.split):
            var.trace_add("write", lambda *_: self._apply_faults())

        hw = ttk.LabelFrame(tab, text="Hardware Error Status (address 70)", padding=8)
        hw.pack(fill=tk.X)
        values = ["None"] + [f"{name} (0x{bit:02X})" for bit, name in
                             decode.HARDWARE_ERRORS.items()]
        box = ttk.Combobox(hw, textvariable=self.hw_error, values=values, state="readonly")
        box.pack(side=tk.LEFT)
        box.bind("<<ComboboxSelected>>", lambda _e: self._apply_hw_error())
        ttk.Label(hw, text="sets the alert bit (0x80) in every Status Packet",
                  style="Hint.TLabel").pack(side=tk.LEFT, padx=8)

        dev = ttk.LabelFrame(tab, text="Device side", padding=8)
        dev.pack(fill=tk.X, pady=8)
        self.sim_info = tk.StringVar(value="not running")
        ttk.Label(dev, textvariable=self.sim_info, font=MONO, justify=tk.LEFT).pack(anchor=tk.W)
        ttk.Button(dev, text="Power cycle device (factory defaults)",
                   command=self._reset_sim).pack(anchor=tk.W, pady=(6, 0))

    def _build_debug_panel(self, parent) -> ttk.Frame:
        frame = ttk.LabelFrame(parent, text="Communication debugger", padding=6)
        bar = ttk.Frame(frame)
        bar.pack(fill=tk.X)
        ttk.Checkbutton(bar, text="Pause", variable=self.paused).pack(side=tk.LEFT)
        ttk.Checkbutton(bar, text="Autoscroll", variable=self.autoscroll).pack(side=tk.LEFT,
                                                                             padx=6)
        ttk.Checkbutton(bar, text="Raw RX chunks", variable=self.raw_bytes,
                        command=self._apply_raw).pack(side=tk.LEFT)
        ttk.Checkbutton(bar, text="Hide auto-poll", variable=self.hide_poll,
                        command=self._refilter).pack(side=tk.LEFT, padx=6)
        ttk.Button(bar, text="Clear", command=self._clear_log, width=7).pack(side=tk.RIGHT)
        ttk.Button(bar, text="Export...", command=self._export_log, width=9).pack(side=tk.RIGHT,
                                                                                padx=4)
        entry = ttk.Entry(bar, textvariable=self.filter_text, width=16)
        entry.pack(side=tk.RIGHT)
        ttk.Label(bar, text="Filter").pack(side=tk.RIGHT, padx=4)
        self.filter_text.trace_add("write", lambda *_: self._refilter())

        split = ttk.PanedWindow(frame, orient=tk.VERTICAL)
        split.pack(fill=tk.BOTH, expand=True, pady=(6, 0))

        top = ttk.Frame(split)
        columns = ("time", "delta", "dir", "id", "inst", "error", "summary", "bytes")
        self.log = ttk.Treeview(top, columns=columns, show="headings", selectmode="browse")
        for col, text, width, stretch in (("time", "Time", 90, False),
                                          ("delta", "Δt / rt ms", 75, False),
                                          ("dir", "Dir", 60, False), ("id", "ID", 45, False),
                                          ("inst", "Instruction", 100, False),
                                          ("error", "Error", 100, False),
                                          ("summary", "Details", 240, True),
                                          ("bytes", "Bytes", 300, True)):
            self.log.heading(col, text=text)
            self.log.column(col, width=width, stretch=stretch,
                            anchor=tk.W if stretch or col in ("inst", "error") else tk.CENTER)
        for tag, color in COLORS.items():
            self.log.tag_configure(tag, foreground=color)
        scroll = ttk.Scrollbar(top, orient=tk.VERTICAL, command=self.log.yview)
        self.log.configure(yscrollcommand=scroll.set)
        scroll.pack(side=tk.RIGHT, fill=tk.Y)
        self.log.pack(fill=tk.BOTH, expand=True)
        self.log.bind("<<TreeviewSelect>>", self._show_detail)
        split.add(top, weight=3)

        bottom = ttk.Frame(split)
        self.detail = ttk.Treeview(bottom, columns=("field", "bytes", "meaning"),
                                   show="headings", height=7)
        for col, text, width in (("field", "Field", 110), ("bytes", "Bytes", 260),
                                 ("meaning", "Meaning", 300)):
            self.detail.heading(col, text=text)
            self.detail.column(col, width=width, anchor=tk.W)
        self.detail.pack(fill=tk.BOTH, expand=True)
        self.detail_text = tk.Text(bottom, height=3, font=MONO, wrap=tk.CHAR,
                                   background="#fafafa", relief=tk.FLAT)
        self.detail_text.pack(fill=tk.X, pady=(4, 0))
        split.add(bottom, weight=1)
        return frame

    # ================================================================== native library

    def _load_native(self) -> None:
        try:
            self.native = native.Native()
        except native.NativeLibraryError as exc:
            self.native = None
            self._log_info(f"native library: {exc}")
            if messagebox.askyesno(
                    "Native library missing",
                    "The bridge to the DXL2 static library is not built yet.\n\n"
                    f"{exc}\n\nBuild it now with CMake?"):
                self._build_native()
            return
        self._log_info(f"loaded {self.native.path}")
        self._fill_table()
        self._update_preview()

    def _build_native(self) -> None:
        if self.session is not None:
            messagebox.showinfo("Rebuild", "Disconnect first.")
            return
        self.status_text.set("Building native library...")
        self.config(cursor="watch")
        lines: List[str] = []

        def worker() -> None:
            try:
                native.build(lines.append)
                error = None
            except native.NativeLibraryError as exc:
                error = str(exc)
            self.after(0, lambda: done(error))

        def done(error: Optional[str]) -> None:
            self.config(cursor="")
            for line in lines[-25:]:
                self._log_info(line)
            if error:
                self.status_text.set("Native build failed")
                messagebox.showerror("Build failed", f"{error}\n\nSee the debug log for the "
                                                     "compiler output.")
                return
            self.status_text.set("Native library built")
            if self.native is None:
                self._load_native()
            else:
                messagebox.showinfo("Rebuilt", "Restart the application to load the new library.")

        threading.Thread(target=worker, daemon=True).start()

    # ================================================================== connection

    def _on_mode(self) -> None:
        if self.mode.get() == "sim":
            self.serial_options.pack_forget()
            self.sim_options.pack(side=tk.LEFT)
            self.tabs.tab(self.sim_tab, state="normal")
        else:
            self.sim_options.pack_forget()
            self.serial_options.pack(side=tk.LEFT)
            if self.tabs.select() == str(self.sim_tab):
                self.tabs.select(0)
            self.tabs.tab(self.sim_tab, state="disabled")
            self._refresh_ports()

    def _lock_connection(self, locked: bool) -> None:
        for widget in self.lockable:
            widget.configure(state="disabled" if locked else "normal")

    def _refresh_ports(self) -> None:
        if transport.serial is None:
            self.port_box.configure(values=[])
            self.status_text.set("pyserial not installed: python -m pip install pyserial")
            return
        ports = transport.list_ports()
        self.port_box.configure(values=[f"{dev} - {desc}" for dev, desc in ports])
        if ports and not self.port.get():
            self.port.set(f"{ports[0][0]} - {ports[0][1]}")

    def _toggle_connection(self) -> None:
        if self.session is not None:
            self._disconnect()
        else:
            self._connect()

    def _connect(self) -> None:
        if self.native is None:
            self._load_native()
            if self.native is None:
                return
        try:
            if self.mode.get() == "sim":
                link: transport.Transport = transport.SimulationTransport(
                    self.native, self._int(self.sim_id, 0, 252))
                self.sim = link  # type: ignore[assignment]
                self._apply_faults()
                self._apply_hw_error()
            else:
                # "COM3 - description" from the list, or a pyserial URL typed in
                port = self.port.get().split(" - ")[0].strip()
                if not port:
                    messagebox.showwarning("Serial port", "Select a serial port first.")
                    return
                link = transport.SerialTransport(port, int(self.baud.get()), self.echo.get())
                self.sim = None
            link.open()
        except (transport.TransportError, ValueError) as exc:
            messagebox.showerror("Connection failed", str(exc))
            return

        self.session = Session(self.native, link, self._on_log, self._int(self.timeout, 5, 5000))
        self.session.log_raw = self.raw_bytes.get()
        self._log_info(f"connected: {link.name}")
        self.status_text.set(f"Connected - {link.name}")
        self.led.itemconfigure(self.led_dot, fill="#2ea043")
        self.connect_btn.configure(text="Disconnect (F5)")
        self._lock_connection(True)

    def _disconnect(self) -> None:
        if self.session is None:
            return
        self.session.close()
        self._log_info("disconnected")
        self.session = None
        self.sim = None
        self.scan_running = False
        self.scan_btn.configure(text="Scan (ping each)")
        self.status_text.set("Disconnected")
        self.led.itemconfigure(self.led_dot, fill="#b0b0b0")
        self.connect_btn.configure(text="Connect (F5)")
        self.sim_info.set("not running")
        self._lock_connection(False)

    def _require(self) -> Optional[Session]:
        if self.session is None:
            self.status_text.set("Not connected - press Connect (F5)")
            self.bell()
        return self.session

    # ================================================================== requests

    @staticmethod
    def _int(var: tk.Variable, low: int, high: int) -> int:
        try:
            value = int(var.get())
        except (tk.TclError, ValueError):
            value = low
        value = max(low, min(high, value))
        var.set(value)
        return value

    def _target(self) -> int:
        return self._int(self.target_id, 0, 254)

    def _ping(self) -> None:
        if (s := self._require()) is not None:
            s.request(self._target(), 0x01)

    def _simple(self, instruction: int, params: bytes = b"") -> None:
        if (s := self._require()) is not None:
            s.request(self._target(), instruction, params)

    def _factory_reset(self) -> None:
        if self._require() is None:
            return
        if messagebox.askyesno("Factory reset", "Reset all settings except ID of device "
                                                f"{self._target()}?"):
            self._simple(0x06, b"\x01")

    def _write_u(self, address: int, value: int, size: int, tag: str = "") -> None:
        if (s := self._require()) is not None:
            data = address.to_bytes(2, "little") + (value & ((1 << (8 * size)) - 1)).to_bytes(
                size, "little")
            s.request(self._target(), 0x03, data, tag=tag)

    def _read(self, address: int, length: int, callback, tag: str = "") -> None:
        if (s := self._require()) is not None:
            s.request(self._target(), 0x02,
                      address.to_bytes(2, "little") + length.to_bytes(2, "little"),
                      callback=callback, tag=tag)

    def _move(self) -> None:
        self._write_u(ADDR_GOAL_POSITION, self._int(self.goal, 0, 4095), 4)

    def _read_present(self, tag: str = "") -> None:
        self._read(ADDR_PRESENT_POSITION, 4, self._on_present, tag)

    def _on_present(self, req: Request) -> None:
        reply = req.reply
        if reply is None or len(reply.data) < 4:
            self.present_text.set("no reply" if req.timed_out else "-")
            return
        value = decode.signed(int.from_bytes(reply.data[:4], "little"), 4)
        self.present_text.set(f"Present position: {value}")
        self._draw_position(value)

    def _draw_position(self, value: int) -> None:
        c = self.position_canvas
        c.delete("all")
        width = max(c.winfo_width(), 10)
        goal_x = 4 + (width - 8) * self._int(self.goal, 0, 4095) / 4095
        x = 4 + (width - 8) * max(0, min(4095, value)) / 4095
        c.create_line(4, 13, width - 4, 13, fill="#c8c8c8", width=4)
        c.create_line(goal_x, 3, goal_x, 23, fill="#d9730d", width=2)
        c.create_oval(x - 7, 6, x + 7, 20, fill="#1f5fbf", outline="")

    def _read_hw_error(self) -> None:
        def done(req: Request) -> None:
            if req.reply and req.reply.data:
                value = req.reply.data[0]
                names = [n for bit, n in decode.HARDWARE_ERRORS.items() if value & bit]
                messagebox.showinfo("Hardware Error Status",
                                    f"0x{value:02X}: " + (", ".join(names) or "no error"))
        self._read(ADDR_HW_ERROR, 1, done)

    # ---------------------------------------------------------------- builder

    def _builder_packet(self):
        name = self.b_inst.get()
        instruction = next(k for k, v in decode.INSTRUCTIONS.items() if v == name)
        packet_id = int(self.b_id.get())
        if self.b_raw_mode.get():
            return packet_id, instruction, decode.parse_hex(self.b_params.get())
        addr = int(self.b_addr.get()).to_bytes(2, "little")
        length = int(self.b_len.get()).to_bytes(2, "little")
        if name == "READ":
            return packet_id, instruction, addr + length
        if name in ("WRITE", "REG_WRITE"):
            text = self.b_value.get().strip()
            if any(ch in text for ch in " ,"):
                data = decode.parse_hex(text)  # explicit byte list
            else:
                size = int(self.b_size.get())
                data = (int(text, 0) & ((1 << (8 * size)) - 1)).to_bytes(size, "little")
            return packet_id, instruction, addr + data
        if name in ("SYNC_READ", "FAST_SYNC_READ"):
            ids = bytes(int(x) for x in self.b_ids.get().replace(",", " ").split())
            return decode.BROADCAST_ID, instruction, addr + length + ids
        if name == "FACTORY_RESET":
            return packet_id, instruction, b"\x01"
        if name == "CLEAR":
            return packet_id, instruction, bytes((0x01, 0x44, 0x58, 0x4C, 0x22))
        return packet_id, instruction, b""

    HINTS = {
        "PING": "Asks the device for model number and firmware version.",
        "READ": "Reads Length bytes from Address.",
        "WRITE": "Writes Value (as Value size bytes, or hex bytes like '01 02') to Address.",
        "REG_WRITE": "Like WRITE, but executed only after ACTION.",
        "ACTION": "Executes the registered REG_WRITE.",
        "FACTORY_RESET": "Resets everything except the ID (parameter 0x01).",
        "REBOOT": "Restarts the device.",
        "CLEAR": "Clears the multi-turn position.",
        "SYNC_READ": "Broadcast read of Address/Length from each listed ID.",
        "SYNC_WRITE": "Use raw parameters: ADDR_L ADDR_H LEN_L LEN_H then [ID data...] per device.",
        "BULK_READ": "Use raw parameters: [ID ADDR_L ADDR_H LEN_L LEN_H] per device.",
        "BULK_WRITE": "Use raw parameters: [ID ADDR_L ADDR_H LEN_L LEN_H data...] per device.",
    }

    def _update_preview(self) -> None:
        if not hasattr(self, "b_preview"):
            return
        self.b_hint.configure(text=self.HINTS.get(self.b_inst.get(),
                                                  "Use raw parameters for this instruction."))
        if self.native is None:
            self.b_preview.set("native library not loaded")
            return
        try:
            packet_id, instruction, params = self._builder_packet()
            raw = native.Host(self.native).build(packet_id, instruction, params)
        except (ValueError, tk.TclError, StopIteration) as exc:
            self.b_preview.set(f"invalid input: {exc}")
            return
        self.b_preview.set(f"params ({len(params)}): {decode.hexdump(params) or '-'}\n"
                           f"packet ({len(raw)}): {decode.hexdump(raw)}")

    def _builder_send(self) -> None:
        if (s := self._require()) is None:
            return
        try:
            packet_id, instruction, params = self._builder_packet()
        except (ValueError, tk.TclError) as exc:
            messagebox.showerror("Packet builder", f"Invalid input: {exc}")
            return
        s.request(packet_id, instruction, params)

    def _builder_to_raw(self) -> None:
        text = self.b_preview.get()
        if "packet" in text:
            self.raw_send.set(text.split(": ", 2)[-1])

    def _mutate_raw(self, how: str) -> None:
        try:
            data = bytearray(decode.parse_hex(self.raw_send.get()))
        except ValueError:
            return
        if not data:
            return
        if how == "crc":
            data[-1] ^= 0x5A
        elif how == "cut":
            data = data[:max(1, len(data) - 3)]
        else:
            data = bytearray(b"\x00\x13\xFF") + data + bytearray(b"\xFF\xFF")
        self.raw_send.set(decode.hexdump(bytes(data)))

    def _send_raw(self) -> None:
        if (s := self._require()) is None:
            return
        try:
            data = decode.parse_hex(self.raw_send.get())
        except ValueError as exc:
            messagebox.showerror("Raw bytes", f"Invalid hex: {exc}")
            return
        if data:
            s.send_raw(data)

    # ---------------------------------------------------------------- control table

    def _fill_table(self) -> None:
        self.table.delete(*self.table.get_children())
        if self.native is None:
            return
        for item in self.native.table_items:
            self.table.insert("", tk.END, iid=str(item.address),
                              values=(item.address, item.name, item.size,
                                      "RW" if item.writable else "R", "", ""),
                              tags=() if item.writable else ("ro",))

    def _table_length(self) -> int:
        last = self.native.table_items[-1]
        return last.address + last.size

    def _read_table(self, tag: str = "") -> None:
        if self.native is not None:
            self._read(0, self._table_length(), self._on_table, tag)

    def _on_table(self, req: Request) -> None:
        if req.reply is None or not req.reply.data or self.native is None:
            return
        data = req.reply.data
        for item in self.native.table_items:
            chunk = data[item.address:item.address + item.size]
            if len(chunk) < item.size:
                continue
            value = int.from_bytes(chunk, "little")
            if item.size == 4:
                value = decode.signed(value, 4)
            changed = item.address in self.table_values and self.table_values[item.address] != chunk
            self.table_values[item.address] = chunk
            tags = [] if item.writable else ["ro"]
            if changed:
                tags.append("changed")
            self.table.item(str(item.address), values=(item.address, item.name, item.size,
                                                        "RW" if item.writable else "R", value,
                                                        decode.hexdump(chunk)),
                            tags=tags)

    def _edit_table_item(self, _event) -> None:
        selection = self.table.selection()
        if not selection or self.native is None:
            return
        address = int(selection[0])
        item = next(i for i in self.native.table_items if i.address == address)
        if not item.writable:
            messagebox.showinfo("Control table", f"{item.name} is read-only.")
            return
        current = self.table.set(selection[0], "value")
        value = simpledialog.askinteger("Write item", f"{item.name} (address {address}, "
                                                      f"{item.size} byte)",
                                        parent=self, initialvalue=int(current or 0))
        if value is not None:
            self._write_u(address, value, item.size)
            self._read_table()

    # ---------------------------------------------------------------- scan

    def _scan(self) -> None:
        if (s := self._require()) is None:
            return
        if self.scan_running:
            s.cancel_all("scan")
            self.scan_running = False
            self.scan_btn.configure(text="Scan (ping each)")
            return
        low = self._int(self.scan_from, 0, 252)
        high = self._int(self.scan_to, low, 252)
        self.scan_tree.delete(*self.scan_tree.get_children())
        self.scan_progress.configure(maximum=high - low + 1, value=0)
        self.scan_running = True
        self.scan_btn.configure(text="Stop scan")
        for packet_id in range(low, high + 1):
            last = packet_id == high
            s.request(packet_id, 0x01, callback=lambda r, last=last: self._on_scan(r, last),
                      tag="scan")

    def _on_scan(self, req: Request, last: bool) -> None:
        self.scan_progress.step(1)
        self._add_scan_results(req)
        if last:
            self.scan_running = False
            self.scan_btn.configure(text="Scan (ping each)")

    def _broadcast_ping(self) -> None:
        if (s := self._require()) is not None:
            self.scan_tree.delete(*self.scan_tree.get_children())
            s.request(decode.BROADCAST_ID, 0x01, callback=self._add_scan_results)

    def _add_scan_results(self, req: Request) -> None:
        for reply in req.replies:
            if len(reply.data) >= 3 and not self.scan_tree.exists(str(reply.packet_id)):
                self.scan_tree.insert("", tk.END, iid=str(reply.packet_id),
                                      values=(reply.packet_id, decode.u16(reply.data),
                                              reply.data[2], f"{reply.latency_ms:.2f}"))

    def _use_scanned_id(self, _event) -> None:
        selection = self.scan_tree.selection()
        if selection:
            self.target_id.set(int(selection[0]))
            self.b_id.set(int(selection[0]))
            self.tabs.select(0)

    # ---------------------------------------------------------------- simulator

    def _apply_faults(self) -> None:
        if self.sim is not None:
            f = self.sim.faults
            f.drop_percent = self.drop.get()
            f.corrupt_percent = self.corrupt.get()
            f.delay_ms = self.delay.get()
            f.split_reply = self.split.get()

    def _apply_hw_error(self) -> None:
        if self.sim is not None:
            text = self.hw_error.get()
            value = int(text[text.index("0x") + 2:text.index(")")], 16) if "0x" in text else 0
            self.sim.device.set_hardware_error(value)

    def _reset_sim(self) -> None:
        if self.sim is not None:
            self.sim.device.reset(self._int(self.sim_id, 0, 252))
            self.hw_error.set("None")
            self._log_info("simulated device power cycled")

    def _update_sim_info(self) -> None:
        if self.sim is None:
            return
        d = self.sim.device
        table = d.read_table()
        pos = decode.signed(int.from_bytes(table[132:136], "little"), 4)
        goal = decode.signed(int.from_bytes(table[116:120], "little"), 4)
        self.sim_info.set(f"ID {d.device_id:<4} torque {table[ADDR_TORQUE]}  "
                          f"LED {table[ADDR_LED]}  moving {table[ADDR_MOVING]}\n"
                          f"goal {goal:<6} present {pos:<6} hw error 0x{table[ADDR_HW_ERROR]:02X}\n"
                          f"rejected request packets: {d.invalid_packets}")

    # ================================================================== debug log

    def _log_info(self, text: str) -> None:
        self._on_log(LogEntry(time.time(), "INFO", summary=text))

    def _on_log(self, entry: LogEntry) -> None:
        if self.paused.get() and entry.direction in ("TX", "RX", "RAW"):
            return
        self.log_entries.append(entry)
        if len(self.log_entries) > LOG_LIMIT:
            self.log_entries = self.log_entries[-LOG_LIMIT:]
        self._insert_row(entry)

    def _row_values(self, entry: LogEntry):
        stamp = datetime.datetime.fromtimestamp(entry.time).strftime("%H:%M:%S.%f")[:-3]
        delta = "" if self.last_log_time is None else f"{(entry.time - self.last_log_time) * 1000:.1f}"
        if entry.latency_ms is not None:
            delta = f"rt {entry.latency_ms:.2f}"
        inst = decode.instruction_name(entry.instruction) if entry.instruction is not None else ""
        pid = decode.id_text(entry.packet_id) if entry.packet_id is not None else ""
        return (stamp, delta, entry.direction, pid, inst, entry.error, entry.summary,
                decode.hexdump(entry.raw))

    def _visible(self, entry: LogEntry, values) -> bool:
        if self.hide_poll.get() and entry.tag == "poll":
            return False
        needle = self.filter_text.get().strip().lower()
        return not needle or any(needle in str(v).lower() for v in values)

    def _insert_row(self, entry: LogEntry) -> None:
        values = self._row_values(entry)
        self.last_log_time = entry.time
        if not self._visible(entry, values):
            return
        tag = entry.direction
        if entry.direction == "RX" and entry.error and entry.error != "OK":
            tag = "STATUSERR"
        iid = self.log.insert("", tk.END, values=values, tags=(tag,))
        self.log_rows[iid] = entry
        children = self.log.get_children()
        if len(children) > LOG_LIMIT:
            for old in children[:len(children) - LOG_LIMIT]:
                self.log_rows.pop(old, None)
            self.log.delete(*children[:len(children) - LOG_LIMIT])
        if self.autoscroll.get():
            self.log.see(iid)

    def _refilter(self) -> None:
        self.log.delete(*self.log.get_children())
        self.log_rows.clear()
        self.last_log_time = None
        for entry in self.log_entries:
            self._insert_row(entry)

    def _clear_log(self) -> None:
        self.log_entries.clear()
        self.log_rows.clear()
        self.last_log_time = None
        self.log.delete(*self.log.get_children())
        self.detail.delete(*self.detail.get_children())
        self.detail_text.delete("1.0", tk.END)
        if self.session is not None:
            self.session.stats = Statistics()

    def _apply_raw(self) -> None:
        if self.session is not None:
            self.session.log_raw = self.raw_bytes.get()

    def _show_detail(self, _event) -> None:
        selection = self.log.selection()
        self.detail.delete(*self.detail.get_children())
        self.detail_text.delete("1.0", tk.END)
        if not selection:
            return
        entry = self.log_rows.get(selection[0])
        if entry is None:
            return
        if entry.raw and self.native is not None:
            frames = decode.scan_frames(entry.raw) if entry.direction in ("RAW", "ERR", "TX") \
                else [entry.raw]
            for frame in frames or [entry.raw]:
                for field, data, meaning in decode.breakdown(frame, self.native.crc16):
                    self.detail.insert("", tk.END, values=(field, data, meaning))
        self.detail_text.insert(tk.END, f"{entry.direction}  {entry.summary}\n"
                                        f"{len(entry.raw)} bytes: {decode.hexdump(entry.raw)}")

    def _export_log(self) -> None:
        if not self.log_entries:
            messagebox.showinfo("Export", "The log is empty.")
            return
        path = filedialog.asksaveasfilename(
            defaultextension=".csv", initialfile="dxl2_log.csv",
            filetypes=(("CSV", "*.csv"), ("Text", "*.txt")))
        if not path:
            return
        self.last_log_time = None
        with open(path, "w", newline="", encoding="utf-8") as handle:
            if path.lower().endswith(".txt"):
                for entry in self.log_entries:
                    v = self._row_values(entry)
                    self.last_log_time = entry.time
                    handle.write(" | ".join(str(x) for x in v) + "\n")
            else:
                writer = csv.writer(handle)
                writer.writerow(("time", "delta_ms", "dir", "id", "instruction", "error",
                                 "details", "bytes"))
                for entry in self.log_entries:
                    writer.writerow(self._row_values(entry))
                    self.last_log_time = entry.time
        self._log_info(f"log exported to {path}")

    # ================================================================== timers

    def _poll(self) -> None:
        if self.session is not None:
            try:
                self.session.poll()
            except Exception as exc:  # keep the UI alive, report the problem
                self._log_info(f"poll error: {exc}")
            link = self.session.transport if self.session else None
            if isinstance(link, transport.SerialTransport) and not link.alive:
                self._disconnect()
                messagebox.showerror("Serial port", "The serial port stopped responding.")
        self._poll_job = self.after(POLL_MS, self._poll)

    def _periodic(self) -> None:
        s = self.session
        if s is not None:
            if self.auto_poll.get() and s.pending == 0:
                self._read_present(tag="poll")
            if self.auto_table.get() and s.pending == 0 and self._tick % 2 == 0:
                self._read_table(tag="poll")
            st = s.stats
            avg = st.average_latency_ms
            self.stats_text.set(
                f"TX {st.tx_packets} pkts / {st.tx_bytes} B   RX {st.rx_packets} pkts / "
                f"{st.rx_bytes} B   invalid {st.invalid_packets}   timeouts {st.timeouts}   "
                f"status errors {st.status_errors}   latency "
                + (f"{st.last_latency_ms:.2f} ms (avg {avg:.2f})" if avg is not None else "-")
                + f"   queue {s.pending}")
            self._update_sim_info()
        self._tick += 1
        self._periodic_job = self.after(200 if self.auto_poll.get() else 250, self._periodic)

    _tick = 0

    # ================================================================== misc

    def _about(self) -> None:
        path = self.native.path if self.native else "not loaded"
        messagebox.showinfo("About DXL2 Studio",
                            "DXL2 Studio - test and debug tool for the DXL2-C library\n"
                            "(DYNAMIXEL Protocol 2.0).\n\n"
                            "Simulation: master and simulated device both run the static\n"
                            "library through the native bridge.\n"
                            "Serial: the master talks to a real device over a COM port.\n\n"
                            f"Native bridge: {path}")

    def _on_close(self) -> None:
        self._disconnect()
        for job in (self._poll_job, self._periodic_job):
            self.after_cancel(job)
        self.destroy()


def parse_args(argv: Optional[List[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="DXL2 Studio - DYNAMIXEL Protocol 2.0 GUI")
    parser.add_argument("--mode", choices=("sim", "serial"), default="sim",
                        help="start in simulation or serial-port mode (default: sim)")
    parser.add_argument("--port", help="serial port or pyserial URL, e.g. COM3, /dev/ttyUSB0")
    parser.add_argument("--baud", type=int, help="serial baud rate (default: 57600)")
    parser.add_argument("--id", type=int, dest="sim_id", help="simulated device ID (default: 1)")
    parser.add_argument("--timeout", type=int, help="reply timeout in ms (default: 100)")
    parser.add_argument("--connect", action="store_true", help="connect right after start")
    return parser.parse_args(argv)


def main(argv: Optional[List[str]] = None) -> None:
    args = parse_args(argv)
    app = App()
    app.mode.set(args.mode)
    if args.port:
        app.port.set(args.port)
    if args.baud:
        app.baud.set(str(args.baud))
    if args.sim_id is not None:
        app.sim_id.set(args.sim_id)
    if args.timeout:
        app.timeout.set(args.timeout)
    app._on_mode()
    if args.port:
        app.port.set(args.port)  # keep the requested port over the first listed one
    if args.connect:
        app.after(100, app._connect)
    app.mainloop()
