#!/usr/bin/env python3
"""SKiDL netlist for the MakiNeat console.

J1 is Player 1 controls. J2 is Player 2 controls. J3 is the cart
socket. The game AVR lives on the cartridge. U5 is the
audio AVR and U6 is the LM386. U1 to U4 are the MAX7219 chain, left to
right, top to bottom. DS1 to DS4 are the matrices those drivers scan.
The sim keeps the picture inside each driver. It models the audio AVR
on EVENT, and it models J5. Here each segment and digit pin lands on
one matrix pad.

A part that sits on Un uses n. A second part on that IC adds 10, a
third adds 20. C1 and R1 sit on U1. C5 and R5 sit on U5.

Each matrix footprint is two PinSocket_1x08 strips composed in
Matrix8x8_2x8_P2.52mm_R25mm.
Top row has pads 0-7 (left to right) and bottom row has pads 8-F (left to right),
with the notch and 1088AW label at the bottom edge.
Display Row 0 is DIG0, Row 7 is DIG7. Display Col 0 is SEGDP, Col 7 is SEGG.

Run from the repo:

    python3 tools/kicad/main-pcb/skidl/makineat.py
"""

from __future__ import annotations

import builtins
import os
import sys
from pathlib import Path

_SYMBOLS = "/usr/share/kicad/symbols"
for _key in (
    "KICAD10_SYMBOL_DIR",
    "KICAD9_SYMBOL_DIR",
    "KICAD8_SYMBOL_DIR",
    "KICAD7_SYMBOL_DIR",
    "KICAD6_SYMBOL_DIR",
):
    os.environ.setdefault(_key, _SYMBOLS)

from skidl import KICAD10, Net, Part, Pin, SKIDL, generate_netlist
from skidl.pin import pin_types

HERE = Path(__file__).resolve().parent
MAIN_PCB_DIR = HERE.parent
REPO = Path(__file__).resolve().parents[4]

sys.path.insert(0, str(REPO))
sys.path.insert(0, str(HERE))
from tools.kicad.shared.cart_edge import contacts, socket_pad, socket_pins
from net_classes import (
    annotate_netlist_classes,
    apply_netclasses,
    assignments_for,
    patch_kicad_pro,
)

NET_PATH = HERE / "makineat.net"
PRO_PATHS = (
    MAIN_PCB_DIR / "makineat_v_05.kicad_pro",
    MAIN_PCB_DIR / "makineat_v_04.kicad_pro",
    MAIN_PCB_DIR / "makineat_v_03.kicad_pro",
    MAIN_PCB_DIR / "makineat_v_02.kicad_pro",
    MAIN_PCB_DIR / "makineat.kicad_pro",
)


SOCKET_FP = "MakiNeat:EDAC_395_MoboSocket_2x10_2.54x5.08mm"
HDR_FP = "Connector_PinHeader_2.54mm:PinHeader_1x06_P2.54mm_Vertical"
PWR_FP = "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical"
SPK_FP = "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical"
UPDI_FP = "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical"
DISP_HDR_FP = "Connector_PinHeader_2.54mm:PinHeader_2x04_P2.54mm_Vertical"
DRV_FP = "Package_DIP:DIP-24_W7.62mm"
AUDIO_FP = "Package_DIP:DIP-28_W7.62mm"
AMP_FP = "Package_DIP:DIP-8_W7.62mm"
MATRIX_FP = "MakiNeat:Matrix8x8_2x8_P2.52mm_R25mm"
R_HORIZ_FP = "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal"
R_VERT_FP = "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P2.54mm_Vertical"
R_FP = R_VERT_FP
C_ELEC_FP = "Capacitor_THT:CP_Radial_D5.0mm_P2.50mm"
C_DISC_FP = "Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P5.00mm"

# 1x6 player header (GND, four directions, button).
# Pin 1 is GND.
# Pins: 1=GND, 2=LEFT, 3=RIGHT, 4=UP, 5=DOWN, 6=BUTTON.
HDR_PINS = (
    (1, "GND"),
    (2, "LEFT"),
    (3, "RIGHT"),
    (4, "UP"),
    (5, "DOWN"),
    (6, "BUTTON"),
)

# 1x2 JST XH power header (Pin 1 is GND, Pin 2 is +5V).
PWR_PINS = (
    (1, "GND"),
    (2, "VCC"),
)

# 1x2 speaker header (Pin 1 is GND, Pin 2 is SPK).
SPK_PINS = (
    (1, "GND"),
    (2, "SPK"),
)

MATRIX_PIN_NAMES = ("0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F")
# The footprint has pads 8-F on the left strip (notch side) and 0-7 on the right strip.
# MAX7219 driver pin connected to each matrix pin (0-F).
MATRIX_DRV_PINS = {
    "0": 17,  # COL 8 -> SEGG (pin 17)
    "1": 15,  # COL 7 -> SEGF (pin 15)
    "2": 11,  # ROW 2 -> DIG1 (pin 11)
    "3": 22,  # COL 1 -> SEGDP (pin 22)
    "4": 7,   # ROW 4 -> DIG3 (pin 7)
    "5": 21,  # COL 6 -> SEGE (pin 21)
    "6": 20,  # COL 4 -> SEGC (pin 20)
    "7": 2,   # ROW 1 -> DIG0 (pin 2)
    "8": 3,   # ROW 5 -> DIG4 (pin 3)
    "9": 5,   # ROW 7 -> DIG6 (pin 5)
    "A": 14,  # COL 2 -> SEGA (pin 14)
    "B": 16,  # COL 3 -> SEGB (pin 16)
    "C": 8,   # ROW 8 -> DIG7 (pin 8)
    "D": 23,  # COL 5 -> SEGD (pin 23)
    "E": 10,  # ROW 6 -> DIG5 (pin 10)
    "F": 6,   # ROW 3 -> DIG2 (pin 6)
}

# 2x4 header pin to matrix pin map for 8-digit status display
# J7 connects to U7 segment pins (range 13-24)
J7_PIN_MAP = {
    1: "A",  # U7 pin 14 (SEGA)
    2: "1",  # U7 pin 15 (SEGF)
    3: "B",  # U7 pin 16 (SEGB)
    4: "0",  # U7 pin 17 (SEGG)
    5: "6",  # U7 pin 20 (SEGC)
    6: "5",  # U7 pin 21 (SEGE)
    7: "3",  # U7 pin 22 (SEGDP)
    8: "D",  # U7 pin 23 (SEGD)
}

# J9 connects to U7 digit cathode pins (range 1-12)
J9_PIN_MAP = {
    1: "7",  # U7 pin 2 (DIG0)
    2: "8",  # U7 pin 3 (DIG4)
    3: "9",  # U7 pin 5 (DIG6)
    4: "F",  # U7 pin 6 (DIG2)
    5: "4",  # U7 pin 7 (DIG3)
    6: "C",  # U7 pin 8 (DIG7)
    7: "E",  # U7 pin 10 (DIG5)
    8: "2",  # U7 pin 11 (DIG1)
}

DRV_PINS = (
    (1, "DIN"),
    (2, "DIG0"),
    (3, "DIG4"),
    (4, "GND"),
    (5, "DIG6"),
    (6, "DIG2"),
    (7, "DIG3"),
    (8, "DIG7"),
    (9, "GND2"),
    (10, "DIG5"),
    (11, "DIG1"),
    (12, "LOAD"),
    (13, "CLK"),
    (14, "SEGA"),
    (15, "SEGF"),
    (16, "SEGB"),
    (17, "SEGG"),
    (18, "ISET"),
    (19, "V+"),
    (20, "SEGC"),
    (21, "SEGE"),
    (22, "SEGDP"),
    (23, "SEGD"),
    (24, "DOUT"),
)

# Same 28-pin map as the cartridge. PD6 is the DAC. PA1 listens to EVENT.
AUDIO_PINS = (
    (1, "PA7"),
    (2, "PC0"),
    (3, "PC1"),
    (4, "PC2"),
    (5, "PC3"),
    (6, "VDDIO2"),
    (7, "PD1"),
    (8, "PD2"),
    (9, "PD3"),
    (10, "PD4"),
    (11, "PD5"),
    (12, "PD6"),
    (13, "PD7"),
    (14, "AVDD"),
    (15, "GND"),
    (16, "PF0"),
    (17, "PF1"),
    (18, "PF6"),
    (19, "UPDI"),
    (20, "VDD"),
    (21, "GND2"),
    (22, "PA0"),
    (23, "PA1"),
    (24, "PA2"),
    (25, "PA3"),
    (26, "PA4"),
    (27, "PA5"),
    (28, "PA6"),
)

AMP_PINS = (
    (1, "GAIN"),
    (2, "INN"),
    (3, "INP"),
    (4, "GND"),
    (5, "VOUT"),
    (6, "VS"),
    (7, "BYPASS"),
    (8, "GAINB"),
)

TWO_PINS = ((1, "1"), (2, "2"))

POWER_NAMES = {"VDD", "VDDIO2", "AVDD", "V+", "VS", "GND", "GND2"}


def _pin(num: int | str, name: str) -> Pin:
    func = pin_types.PWRIN if name in POWER_NAMES else pin_types.PASSIVE
    return Pin(num=num, name=name, func=func)


def _part(mpn: str, ref: str, footprint: str, pins, value: str, description: str = "") -> Part:
    part = Part(
        tool=SKIDL,
        name=mpn,
        ref=ref,
        tag=ref,
        footprint=footprint,
        pins=[_pin(num, name) for num, name in pins],
        value=value,
        description=description or value,
    )
    return part


def _tie(part: Part, num: int | str, net: Net) -> None:
    part[num] += net


def _stub(part: Part, num: int) -> None:
    _tie(part, num, Net(f"NC_{part.ref}_{num}"))


def _stub_unused(part: Part, pins) -> None:
    for num, _name in pins:
        if part[num].net is None:
            _stub(part, num)


def _passive(ref: str, footprint: str, value: str, description: str = "") -> Part:
    return _part("Passive", ref, footprint, TWO_PINS, value, description)


def _ic_ref(prefix: str, ic: int, extra: int = 0) -> str:
    return f"{prefix}{ic + 10 * extra}"


TP_FP = "TestPoint:TestPoint_Pad_D1.5mm"
LED_PINS = ((1, "K"), (2, "A"))


def _place_text(part: Part, text: str) -> None:
    part.fields["Text"] = text


def _test_point(ref: str, net: Net, text: str, description: str) -> None:
    pad = _part("TestPoint", ref, TP_FP, ((1, "1"),), net.name, description)
    _place_text(pad, text)
    _tie(pad, 1, net)


def _status_led(
    ref: str,
    resistor: str,
    vcc: Net,
    cathode: Net,
    footprint: str,
    value: str,
    description: str,
    resistor_value: str,
    resistor_description: str,
    text: str,
) -> None:
    """1 kohm from +5V to the anode. The cathode net is GND or a U5 pin."""
    anode = Net(f"{ref}_A")
    led = _part("LED", ref, footprint, LED_PINS, value, description)
    _place_text(led, text)
    series = _passive(resistor, R_VERT_FP, resistor_value, resistor_description)
    _tie(series, 1, vcc)
    _tie(series, 2, anode)
    _tie(led, 2, anode)
    _tie(led, 1, cathode)


def _status_leds(cpu: Part, vcc: Net, gnd: Net) -> None:
    """D1 is the power lamp. D2 to D4 sink on PA4, PA5, and PA6."""
    ok = Net("LED_OK")
    link = Net("LED_LINK")
    fault = Net("LED_FAULT")
    _tie(cpu, 26, ok)
    _tie(cpu, 27, link)
    _tie(cpu, 28, fault)
    _status_led(
        "D1",
        "R35",
        vcc,
        gnd,
        "MakiNeat:LED_D3.0mm_Blue",
        "Blue Power",
        "Blue power indicator. On while +5V is present.",
        "1k PWR",
        "1 kohm series resistor for the power indicator.",
        "Rear or top edge near J6 and J8",
    )
    _status_led(
        "D2",
        "R45",
        vcc,
        ok,
        "MakiNeat:LED_D3.0mm_Green",
        "Green OK",
        "Green system OK indicator. On when U5 drives PA4 low.",
        "1k OK",
        "1 kohm series resistor for the system OK indicator.",
        "With D1, near J6 and J8",
    )
    _status_led(
        "D3",
        "R55",
        vcc,
        link,
        "MakiNeat:LED_D3.0mm_Yellow",
        "Amber Link",
        "Amber cart link indicator. On when U5 drives PA5 low.",
        "1k LINK",
        "1 kohm series resistor for the cart link indicator.",
        "With D1, near J6 and J8",
    )
    _status_led(
        "D4",
        "R65",
        vcc,
        fault,
        "MakiNeat:LED_D3.0mm_Red",
        "Red Fault",
        "Red fault indicator. On when U5 drives PA6 low.",
        "1k FAULT",
        "1 kohm series resistor for the fault indicator.",
        "With D1, near J6 and J8",
    )


def _test_points(
    vcc: Net,
    gnd: Net,
    din: Net,
    clk: Net,
    load: Net,
    reset: Net,
    event: Net,
    aud: Net,
) -> None:
    probes = (
        ("TP1", gnd, "Rear edge at J4, beside C10", "Ground at the power entry"),
        ("TP2", gnd, "Beside the U1 to U4 matrix drivers", "Ground by the matrix drivers"),
        ("TP3", vcc, "Rear edge at J4, with TP1", "Rail at the power entry"),
        ("TP4", vcc, "Beside the MAX7219 cluster, with TP2", "Rail by the display drivers"),
        ("TP5", clk, "Row by J3, with TP6 to TP9", "Display SPI clock"),
        ("TP6", din, "Row by J3, with TP5", "Display SPI data"),
        ("TP7", load, "Row by J3, with TP5", "Display SPI latch"),
        ("TP8", event, "Row by J3, with TP5", "Cart to audio trigger"),
        ("TP9", reset, "Row by J3, with TP5", "System and cart reset"),
        (
            "TP10",
            aud,
            "U5 and U6 corner, clear of matrix returns and SPI",
            "Audio after the DAC divider",
        ),
    )
    for ref, net, text, description in probes:
        _test_point(ref, net, text, description)


def _audio(vcc: Net, gnd: Net, event: Net, reset: Net) -> Net:
    """Console synth. PD6 feeds the LM386 through the divider in the audio doc."""
    cpu = _part(
        "AVR128DB28-I/SP",
        "U5",
        AUDIO_FP,
        AUDIO_PINS,
        "Audio AVR",
        "AVR128DB28-I/SP",
    )
    for num in (20, 14, 6):
        _tie(cpu, num, vcc)
    for num in (15, 21):
        _tie(cpu, num, gnd)
    _tie(cpu, 18, reset)
    _tie(cpu, 23, event)

    updi = Net("AUDIO_UPDI")
    _tie(cpu, 19, updi)
    prog = _part(
        "PinHeader_1x03",
        "J6",
        UPDI_FP,
        ((1, "UPDI"), (2, "VCC"), (3, "GND")),
        "UPDI",
        "Audio programming",
    )
    _tie(prog, 1, updi)
    _tie(prog, 2, vcc)
    _tie(prog, 3, gnd)

    dac = Net("DAC")
    _tie(cpu, 12, dac)
    pull = _passive(_ic_ref("R", 5), R_FP, "10k EVENT", "10 kohm EVENT pull-up")
    _tie(pull, 1, event)
    _tie(pull, 2, vcc)
    reset_pull = _passive(_ic_ref("R", 5, 2), R_HORIZ_FP, "10k RESET", "10 kohm RESET pull-up")
    _tie(reset_pull, 1, reset)
    _tie(reset_pull, 2, vcc)
    sink = _passive(_ic_ref("R", 5, 1), R_FP, "10k DAC", "10 kohm DAC load")
    _tie(sink, 1, dac)
    _tie(sink, 2, gnd)

    aud = Net("AUD")
    couple = _passive(_ic_ref("C", 5, 3), C_ELEC_FP, "1u DAC", "1 uF DAC coupling")
    _tie(couple, 1, dac)
    _tie(couple, 2, aud)
    audin = Net("AUDIN")
    series = _passive(_ic_ref("R", 6), R_FP, "22k IN", "22 kohm into the LM386")
    _tie(series, 1, aud)
    _tie(series, 2, audin)
    shunt = _passive(_ic_ref("R", 6, 1), R_FP, "1k IN", "1 kohm LM386 input shunt")
    _tie(shunt, 1, audin)
    _tie(shunt, 2, gnd)

    amp = _part("LM386N-1", "U6", AMP_FP, AMP_PINS, "LM386", "LM386N-1 speaker amplifier")
    _tie(amp, 2, gnd)
    _tie(amp, 3, audin)
    _tie(amp, 4, gnd)
    _tie(amp, 6, vcc)
    bypass = Net("BYPASS")
    _tie(amp, 7, bypass)
    amp_out = Net("AMP")
    _tie(amp, 5, amp_out)

    spk = Net("SPK")
    output = _passive(_ic_ref("C", 6, 3), C_ELEC_FP, "220u SPK", "220 uF speaker coupling")
    _tie(output, 1, amp_out)
    _tie(output, 2, spk)
    speaker = _part(
        "PinHeader_1x02",
        "J5",
        SPK_FP,
        SPK_PINS,
        "Speaker",
        "Speaker header",
    )
    _tie(speaker, 1, gnd)
    _tie(speaker, 2, spk)

    zobel = Net("ZOBEL")
    zobel_r = _passive(_ic_ref("R", 6, 2), R_FP, "10R ZOB", "10 ohm Zobel")
    _tie(zobel_r, 1, amp_out)
    _tie(zobel_r, 2, zobel)
    zobel_c = _passive(_ic_ref("C", 6, 4), C_DISC_FP, "47n ZOB", "47 nF Zobel")
    _tie(zobel_c, 1, zobel)
    _tie(zobel_c, 2, gnd)

    c5 = _passive(_ic_ref("C", 5), C_DISC_FP, "U5 VDD", "100 nF on U5 pins 20 and 21")
    _tie(c5, 1, vcc)
    _tie(c5, 2, gnd)
    c15 = _passive(_ic_ref("C", 5, 1), C_DISC_FP, "U5 AVDD", "100 nF on U5 pins 14 and 15")
    _tie(c15, 1, vcc)
    _tie(c15, 2, gnd)
    c25 = _passive(_ic_ref("C", 5, 2), C_DISC_FP, "U5 IO", "100 nF on U5 pins 6 and 21")
    _tie(c25, 1, vcc)
    _tie(c25, 2, gnd)
    c6 = _passive(_ic_ref("C", 6), C_DISC_FP, "U6 VS", "100 nF on U6 pins 6 and 4")
    _tie(c6, 1, vcc)
    _tie(c6, 2, gnd)
    c16 = _passive(_ic_ref("C", 6, 1), C_ELEC_FP, "U6 VS", "100 uF on U6 pins 6 and 4")
    _tie(c16, 1, vcc)
    _tie(c16, 2, gnd)
    c26 = _passive(_ic_ref("C", 6, 2), C_ELEC_FP, "U6 BYP", "10 uF on U6 pins 7 and 4")
    _tie(c26, 1, bypass)
    _tie(c26, 2, gnd)
    _status_leds(cpu, vcc, gnd)
    # Unused AVR pins and LM386 gain pins 1 and 8 stay open. Stub them
    # so every footprint pad has a pin in the netlist.
    _stub_unused(cpu, AUDIO_PINS)
    _stub_unused(amp, AMP_PINS)
    return aud


def build() -> None:
    # Two global power nets. Every rail pin and every decoupling cap lands here.
    vcc = Net("+5V")
    gnd = Net("GND")
    din = Net("DIN")
    clk = Net("CLK")
    load = Net("LOAD")
    reset = Net("RESET")
    updi = Net("UPDI")
    event = Net("EVENT")
    p1_switches = {
        "P1_UP": Net("P1_UP"),
        "P1_DOWN": Net("P1_DOWN"),
        "P1_LEFT": Net("P1_LEFT"),
        "P1_RIGHT": Net("P1_RIGHT"),
        "P1_BTN": Net("P1_BTN"),
    }
    p2_switches = {
        "P2_UP": Net("P2_UP"),
        "P2_DOWN": Net("P2_DOWN"),
        "P2_LEFT": Net("P2_LEFT"),
        "P2_RIGHT": Net("P2_RIGHT"),
        "P2_BTN": Net("P2_BTN"),
    }

    named = {
        "DIN": din,
        "CLK": clk,
        "LOAD": load,
        "RESET": reset,
        "UPDI": updi,
        "EVENT": event,
        **p1_switches,
        **p2_switches,
    }
    socket = _part(
        "CartEdge_2x10_P2.54mm",
        "J3",
        SOCKET_FP,
        socket_pins(),
        "Cart",
        "20-contact card edge socket, 2.54 mm pitch (e.g. EDAC 395-020-520-802)",
    )
    for number, side, net_name, _avr in contacts():
        pad = socket_pad(number, side)
        if net_name == "GND":
            _tie(socket, pad, gnd)
        elif net_name == "+5V":
            _tie(socket, pad, vcc)
        elif net_name is None:
            _stub(socket, pad)
        elif net_name in named:
            _tie(socket, pad, named[net_name])
        else:
            _tie(socket, pad, Net(net_name))

    cart_prog = _part(
        "PinHeader_1x03",
        "J8",
        UPDI_FP,
        ((1, "UPDI"), (2, "VCC"), (3, "GND")),
        "Cart UPDI",
        "Cartridge programming",
    )
    _tie(cart_prog, 1, updi)
    _tie(cart_prog, 2, vcc)
    _tie(cart_prog, 3, gnd)

    p1_hdr = _part("PinHeader_1x06", "J1", HDR_FP, HDR_PINS, "P1 Controls", "Player 1 controls")
    _tie(p1_hdr, 1, gnd)
    _tie(p1_hdr, 2, p1_switches["P1_LEFT"])
    _tie(p1_hdr, 3, p1_switches["P1_RIGHT"])
    _tie(p1_hdr, 4, p1_switches["P1_UP"])
    _tie(p1_hdr, 5, p1_switches["P1_DOWN"])
    _tie(p1_hdr, 6, p1_switches["P1_BTN"])

    p2_hdr = _part("PinHeader_1x06", "J2", HDR_FP, HDR_PINS, "P2 Controls", "Player 2 controls")
    _tie(p2_hdr, 1, gnd)
    _tie(p2_hdr, 2, p2_switches["P2_LEFT"])
    _tie(p2_hdr, 3, p2_switches["P2_RIGHT"])
    _tie(p2_hdr, 4, p2_switches["P2_UP"])
    _tie(p2_hdr, 5, p2_switches["P2_DOWN"])
    _tie(p2_hdr, 6, p2_switches["P2_BTN"])

    power = _part("PinHeader_1x02", "J4", PWR_FP, PWR_PINS, "Power", "5 V power")
    _tie(power, 1, gnd)
    _tie(power, 2, vcc)
    c10 = _passive("C10", C_ELEC_FP, "470u VCC", "470 uF power entry bulk reservoir")
    _tie(c10, 1, vcc)
    _tie(c10, 2, gnd)
    aud = _audio(vcc, gnd, event, reset)

    drivers = []
    matrices = []
    for index in range(4):
        ref = f"U{index + 1}"
        drv = _part("MAX7219", ref, DRV_FP, DRV_PINS, "MAX7219")
        matrix_pins = tuple((pin_name, pin_name) for pin_name in MATRIX_PIN_NAMES)
        matrix = _part(
            "Matrix8x8",
            f"DS{index + 1}",
            MATRIX_FP,
            matrix_pins,
            "Matrix8x8",
        )
        _tie(drv, 19, vcc)
        _tie(drv, 4, gnd)
        _tie(drv, 9, gnd)
        _tie(drv, 13, clk)
        _tie(drv, 12, load)
        n = index + 1
        iset = Net(f"{ref}_ISET")
        _tie(drv, 18, iset)
        iset_r = _passive(_ic_ref("R", n), R_HORIZ_FP, f"{ref} ISET", "28 kohm ISET")
        _tie(iset_r, 1, vcc)
        _tie(iset_r, 2, iset)
        bypass_n = _passive(_ic_ref("C", n), C_DISC_FP, f"{ref} V+", f"100 nF on {ref} V+")
        _tie(bypass_n, 1, vcc)
        _tie(bypass_n, 2, gnd)
        bulk_n = _passive(_ic_ref("C", n, 1), C_ELEC_FP, f"{ref} 10u", f"10 uF on {ref} V+")
        _tie(bulk_n, 1, vcc)
        _tie(bulk_n, 2, gnd)
        if index == 0:
            _tie(drv, 1, din)
        for pin_name in MATRIX_PIN_NAMES:
            net = Net(f"{matrix.ref}_{pin_name}")
            _tie(drv, MATRIX_DRV_PINS[pin_name], net)
            _tie(matrix, pin_name, net)
        drivers.append(drv)
        matrices.append(matrix)

    for index in range(3):
        link = Net(f"DOUT_{drivers[index].ref}")
        _tie(drivers[index], 24, link)
        _tie(drivers[index + 1], 1, link)

    # 5th MAX7219 driver (U7) for the 8-digit status display
    u7 = _part("MAX7219", "U7", DRV_FP, DRV_PINS, "MAX7219")
    _tie(u7, 19, vcc)
    _tie(u7, 4, gnd)
    _tie(u7, 9, gnd)
    _tie(u7, 13, clk)
    _tie(u7, 12, load)
    iset7 = Net("U7_ISET")
    _tie(u7, 18, iset7)
    iset7_r = _passive(_ic_ref("R", 7), R_HORIZ_FP, "U7 ISET", "28 kohm ISET")
    _tie(iset7_r, 1, vcc)
    _tie(iset7_r, 2, iset7)
    bypass_7 = _passive(_ic_ref("C", 7), C_DISC_FP, "U7 V+", "100 nF on U7 V+")
    _tie(bypass_7, 1, vcc)
    _tie(bypass_7, 2, gnd)
    bulk_7 = _passive(_ic_ref("C", 7, 1), C_ELEC_FP, "U7 10u", "10 uF on U7 V+")
    _tie(bulk_7, 1, vcc)
    _tie(bulk_7, 2, gnd)

    # Chain U4 -> U7
    link_u4 = Net("DOUT_U4")
    _tie(drivers[3], 24, link_u4)
    _tie(u7, 1, link_u4)
    _tie(u7, 24, Net("DOUT"))

    # 2x4 connectors for status digits daughterboard
    # J7 connects to segment pins on U7 (pins 13-24)
    # J9 connects to digit cathode pins on U7 (pins 1-12)
    disp_pins = tuple((i, str(i)) for i in range(1, 9))
    j7 = _part(
        "PinHeader_2x04",
        "J7",
        DISP_HDR_FP,
        disp_pins,
        "Status Segments",
        "8-digit status display segment header (U7 pins 13-24)",
    )
    for hdr_pin, matrix_pin in J7_PIN_MAP.items():
        net = Net(f"DS5_{matrix_pin}")
        _tie(u7, MATRIX_DRV_PINS[matrix_pin], net)
        _tie(j7, hdr_pin, net)

    j9 = _part(
        "PinHeader_2x04",
        "J9",
        DISP_HDR_FP,
        disp_pins,
        "Status Digits",
        "8-digit status display digit cathode header (U7 pins 1-12)",
    )
    for hdr_pin, matrix_pin in J9_PIN_MAP.items():
        net = Net(f"DS5_{matrix_pin}")
        _tie(u7, MATRIX_DRV_PINS[matrix_pin], net)
        _tie(j9, hdr_pin, net)

    _test_points(vcc, gnd, din, clk, load, reset, event, aud)


def _check(text: str) -> None:
    import re

    from net_classes import class_for_name

    def nodes(name: str) -> set[tuple[str, str]]:
        match = re.search(
            rf'\(name "{re.escape(name)}"\) \(class "([^"]+)"\)',
            text,
        )
        if match is None:
            raise SystemExit(f"missing net {name}")
        if match.group(1) != class_for_name(name):
            raise SystemExit(f"{name} class is {match.group(1)}")
        start = match.end()
        nxt = re.search(r"\n\s*\(net\b", text[start:])
        chunk = text[start : start + (nxt.start() if nxt else len(text))]
        return set(re.findall(r'\(ref "([^"]+)"\)\s*\(pin "([^"]+)"\)', chunk))

    if '(value "Cart")' not in text or "2.54 mm pitch" not in text:
        raise SystemExit("cart socket is not the 20-contact 2.54 mm edge")
    if text.count(f'(footprint "{SOCKET_FP}")') != 1:
        raise SystemExit("cart socket footprint missing")
    if text.count(f'(footprint "{HDR_FP}")') != 2:
        raise SystemExit("expected 2 player control header footprints")
    if text.count(f'(footprint "{PWR_FP}")') != 2:
        raise SystemExit("expected 2 1x02 vertical header footprints (power and speaker)")
    if text.count(f'(footprint "{DISP_HDR_FP}")') != 2:
        raise SystemExit("expected 2 status digits display header footprints (J7 and J9)")
    if text.count('(footprint "Package_DIP:DIP-24_W7.62mm")') != 5:
        raise SystemExit("expected 5 MAX7219 footprints")
    if text.count(f'(footprint "{MATRIX_FP}")') != 4:
        raise SystemExit("expected 4 matrix footprints")
    if text.count('(name "+5V")') != 1:
        raise SystemExit("expected one global +5V net")
    if text.count('(name "GND")') != 1:
        raise SystemExit("expected one global GND net")
    vcc = nodes("+5V")
    gnd = nodes("GND")
    if ("J3", "20") not in vcc:
        raise SystemExit("+5V does not reach cart socket contact B10")
    if ("J4", "2") not in vcc:
        raise SystemExit("+5V does not reach the power header")
    drivers_on_vcc = {ref for ref, pin in vcc if pin == "19"}
    if drivers_on_vcc != {"U1", "U2", "U3", "U4", "U7"}:
        raise SystemExit(f"+5V drivers {sorted(drivers_on_vcc)}")
    for pad in ("8", "10", "18"):
        if ("J3", pad) not in gnd:
            raise SystemExit(f"GND does not reach cart socket pad {pad}")
    if ("J3", "16") in gnd:
        raise SystemExit("cart socket pad 16 is EVENT")
    if ("J1", "1") not in gnd or ("J2", "1") not in gnd:
        raise SystemExit("GND does not reach player header ground pins")
    if ("J4", "1") not in gnd:
        raise SystemExit("GND does not reach power header ground pin")
    for u in ("U1", "U2", "U3", "U4", "U7"):
        if (u, "4") not in gnd or (u, "9") not in gnd:
            raise SystemExit(f"GND does not reach both {u} ground pins")
    din = nodes("DIN")
    if din != {("J3", "6"), ("U1", "1"), ("TP6", "1")}:
        raise SystemExit(f"DIN nodes {din}")
    clk = nodes("CLK")
    if ("J3", "2") not in clk or ("U1", "13") not in clk or ("TP5", "1") not in clk:
        raise SystemExit(f"CLK nodes {clk}")
    load = nodes("LOAD")
    if ("J3", "4") not in load or ("U1", "12") not in load or ("TP7", "1") not in load:
        raise SystemExit(f"LOAD nodes {load}")
    p1_left = nodes("P1_LEFT")
    if p1_left != {("J3", "19"), ("J1", "2")}:
        raise SystemExit(f"P1_LEFT nodes {p1_left}")
    p1_right = nodes("P1_RIGHT")
    if p1_right != {("J3", "17"), ("J1", "3")}:
        raise SystemExit(f"P1_RIGHT nodes {p1_right}")
    p1_up = nodes("P1_UP")
    if p1_up != {("J3", "15"), ("J1", "4")}:
        raise SystemExit(f"P1_UP nodes {p1_up}")
    p1_down = nodes("P1_DOWN")
    if p1_down != {("J3", "13"), ("J1", "5")}:
        raise SystemExit(f"P1_DOWN nodes {p1_down}")
    p1_btn = nodes("P1_BTN")
    if p1_btn != {("J3", "11"), ("J1", "6")}:
        raise SystemExit(f"P1_BTN nodes {p1_btn}")
    p2_left = nodes("P2_LEFT")
    if p2_left != {("J3", "9"), ("J2", "2")}:
        raise SystemExit(f"P2_LEFT nodes {p2_left}")
    p2_right = nodes("P2_RIGHT")
    if p2_right != {("J3", "7"), ("J2", "3")}:
        raise SystemExit(f"P2_RIGHT nodes {p2_right}")
    p2_up = nodes("P2_UP")
    if p2_up != {("J3", "5"), ("J2", "4")}:
        raise SystemExit(f"P2_UP nodes {p2_up}")
    p2_down = nodes("P2_DOWN")
    if p2_down != {("J3", "3"), ("J2", "5")}:
        raise SystemExit(f"P2_DOWN nodes {p2_down}")
    p2_btn = nodes("P2_BTN")
    if p2_btn != {("J3", "1"), ("J2", "6")}:
        raise SystemExit(f"P2_BTN nodes {p2_btn}")
    reset_net = nodes("RESET")
    if reset_net != {("J3", "14"), ("U5", "18"), ("R25", "1"), ("TP9", "1")}:
        raise SystemExit(f"RESET nodes {reset_net}")
    if ("R25", "2") not in vcc:
        raise SystemExit("RESET pull-up does not reach +5V")
    if text.count('(value "Audio AVR")') != 1 or text.count('(description "AVR128DB28-I/SP")') != 1:
        raise SystemExit("audio AVR missing")
    if text.count('(value "LM386")') != 1 or text.count('(description "LM386N-1 speaker amplifier")') != 1:
        raise SystemExit("LM386 missing")
    if text.count(f'(footprint "{AUDIO_FP}")') != 1:
        raise SystemExit("audio AVR footprint missing")
    if text.count(f'(footprint "{AMP_FP}")') != 1:
        raise SystemExit("LM386 footprint missing")
    if text.count(f'(footprint "{UPDI_FP}")') != 2:
        raise SystemExit("expected 2 UPDI headers (audio and cart)")
    if text.count('(description "Cartridge programming")') != 1:
        raise SystemExit("cart UPDI header missing")
    if text.count(f'(footprint "{R_HORIZ_FP}")') != 6:
        raise SystemExit("expected 6 horizontal resistor footprints")
    if text.count(f'(footprint "{R_VERT_FP}")') != 9:
        raise SystemExit("expected 9 vertical resistor footprints")
    if text.count(f'(footprint "{TP_FP}")') != 10:
        raise SystemExit("expected 10 test point footprints")
    for footprint in (
        "MakiNeat:LED_D3.0mm_Blue",
        "MakiNeat:LED_D3.0mm_Green",
        "MakiNeat:LED_D3.0mm_Yellow",
        "MakiNeat:LED_D3.0mm_Red",
    ):
        if text.count(f'(footprint "{footprint}")') != 1:
            raise SystemExit(f"missing status LED footprint {footprint}")
    if text.count('(value "10k EVENT")') != 1 or text.count('(value "10k DAC")') != 1 or text.count('(value "10k RESET")') != 1:
        raise SystemExit("expected the EVENT pull-up, DAC load, and RESET pull-up")
    if text.count('(value "22k IN")') != 1 or text.count('(value "1k IN")') != 1:
        raise SystemExit("input divider missing")
    if text.count('(value "10R ZOB")') != 1 or text.count('(value "47n ZOB")') != 1:
        raise SystemExit("zobel missing")
    if text.count('(value "1u DAC")') != 1 or text.count('(value "220u SPK")') != 1:
        raise SystemExit("coupling caps missing")
    if text.count('(value "U5 VDD")') != 1 or text.count('(value "U5 AVDD")') != 1:
        raise SystemExit("U5 bypass caps missing")
    if text.count('(value "U5 IO")') != 1:
        raise SystemExit("U5 IO bypass missing")
    if text.count('(value "U6 VS")') != 2 or text.count('(value "U6 BYP")') != 1:
        raise SystemExit("U6 supply caps missing")
    if text.count('(value "470u VCC")') != 1:
        raise SystemExit("470 uF power entry bulk reservoir missing")
    for n in (1, 2, 3, 4, 7):
        if text.count(f'(value "U{n} ISET")') != 1 or text.count(f'(value "U{n} V+")') != 1:
            raise SystemExit(f"U{n} ISET or 100 nF missing")
        if text.count(f'(value "U{n} 10u")') != 1:
            raise SystemExit(f"U{n} 10 uF missing")
        iset = nodes(f"U{n}_ISET")
        if iset != {(f"U{n}", "18"), (f"R{n}", "2")}:
            raise SystemExit(f"U{n}_ISET nodes {iset}")
        if (f"R{n}", "1") not in vcc:
            raise SystemExit(f"R{n} does not reach +5V")
        for ref in (f"C{n}", f"C{n + 10}"):
            if (ref, "1") not in vcc or (ref, "2") not in gnd:
                raise SystemExit(f"{ref} is not across +5V and GND")
    event = nodes("EVENT")
    if event != {("J3", "16"), ("U5", "23"), ("R5", "1"), ("TP8", "1")}:
        raise SystemExit(f"EVENT nodes {event}")
    if ("R5", "2") not in vcc:
        raise SystemExit("EVENT pull-up does not reach +5V")
    dac = nodes("DAC")
    if dac != {("U5", "12"), ("R15", "1"), ("C35", "1")}:
        raise SystemExit(f"DAC nodes {dac}")
    if ("R15", "2") not in gnd:
        raise SystemExit("DAC load does not reach GND")
    aud = nodes("AUD")
    if aud != {("C35", "2"), ("R6", "1"), ("TP10", "1")}:
        raise SystemExit(f"AUD nodes {aud}")
    audin = nodes("AUDIN")
    if audin != {("R6", "2"), ("R16", "1"), ("U6", "3")}:
        raise SystemExit(f"AUDIN nodes {audin}")
    if ("R16", "2") not in gnd or ("U6", "2") not in gnd or ("U6", "4") not in gnd:
        raise SystemExit("LM386 grounds missing")
    if ("U6", "6") not in vcc:
        raise SystemExit("LM386 supply missing")
    for num in ("20", "14", "6"):
        if ("U5", num) not in vcc:
            raise SystemExit(f"audio AVR pin {num} is not on +5V")
    for num in ("15", "21"):
        if ("U5", num) not in gnd:
            raise SystemExit(f"audio AVR pin {num} is not ground")
    for ref in ("C5", "C15", "C25", "C6", "C16", "C10"):
        if (ref, "1") not in vcc or (ref, "2") not in gnd:
            raise SystemExit(f"{ref} is not across +5V and GND")
    bypass_net = nodes("BYPASS")
    if bypass_net != {("U6", "7"), ("C26", "1")}:
        raise SystemExit(f"BYPASS nodes {bypass_net}")
    if ("C26", "2") not in gnd:
        raise SystemExit("LM386 bypass return missing")
    if "NetTie" in text or '(ref "NT' in text:
        raise SystemExit("net ties must stay out of the netlist")
    amp = nodes("AMP")
    if amp != {("U6", "5"), ("C36", "1"), ("R26", "1")}:
        raise SystemExit(f"AMP nodes {amp}")
    zobel = nodes("ZOBEL")
    if zobel != {("R26", "2"), ("C46", "1")}:
        raise SystemExit(f"ZOBEL nodes {zobel}")
    if ("C46", "2") not in gnd:
        raise SystemExit("zobel cap does not reach GND")
    spk = nodes("SPK")
    if spk != {("C36", "2"), ("J5", "2")}:
        raise SystemExit(f"SPK nodes {spk}")
    if ("J5", "1") not in gnd:
        raise SystemExit("speaker return missing")
    audio_updi = nodes("AUDIO_UPDI")
    if audio_updi != {("U5", "19"), ("J6", "1")}:
        raise SystemExit(f"AUDIO_UPDI nodes {audio_updi}")
    if ("J6", "2") not in vcc or ("J6", "3") not in gnd:
        raise SystemExit("audio UPDI header power missing")
    cart_updi = nodes("UPDI")
    if cart_updi != {("J3", "12"), ("J8", "1")}:
        raise SystemExit(f"UPDI nodes {cart_updi}")
    if ("J8", "2") not in vcc or ("J8", "3") not in gnd:
        raise SystemExit("cart UPDI header power missing")
    used_u5 = {6, 12, 14, 15, 18, 19, 20, 21, 23, 26, 27, 28}
    for num, _name in AUDIO_PINS:
        if num in used_u5:
            continue
        nc = nodes(f"NC_U5_{num}")
        if nc != {("U5", str(num))}:
            raise SystemExit(f"NC_U5_{num} nodes {nc}")
    for num in (1, 8):
        nc = nodes(f"NC_U6_{num}")
        if nc != {("U6", str(num))}:
            raise SystemExit(f"NC_U6_{num} nodes {nc}")
    for ref in ("TP1", "TP2"):
        if (ref, "1") not in gnd:
            raise SystemExit(f"{ref} is not on GND")
    for ref in ("TP3", "TP4", "R35", "R45", "R55", "R65"):
        if (ref, "1") not in vcc:
            raise SystemExit(f"{ref} is not on +5V")
    if ("D1", "1") not in gnd:
        raise SystemExit("D1 cathode is not on GND")
    led_cathodes = (
        ("LED_OK", "D2", "26"),
        ("LED_LINK", "D3", "27"),
        ("LED_FAULT", "D4", "28"),
    )
    for net_name, led, pin in led_cathodes:
        actual = nodes(net_name)
        if actual != {("U5", pin), (led, "1")}:
            raise SystemExit(f"{net_name} nodes {actual}")
    for led, resistor in (("D1", "R35"), ("D2", "R45"), ("D3", "R55"), ("D4", "R65")):
        actual = nodes(f"{led}_A")
        if actual != {(resistor, "2"), (led, "2")}:
            raise SystemExit(f"{led}_A nodes {actual}")
    notes = {
        "TP1": "Rear edge at J4, beside C10",
        "TP2": "Beside the U1 to U4 matrix drivers",
        "TP3": "Rear edge at J4, with TP1",
        "TP4": "Beside the MAX7219 cluster, with TP2",
        "TP5": "Row by J3, with TP6 to TP9",
        "TP6": "Row by J3, with TP5",
        "TP7": "Row by J3, with TP5",
        "TP8": "Row by J3, with TP5",
        "TP9": "Row by J3, with TP5",
        "TP10": "U5 and U6 corner, clear of matrix returns and SPI",
        "D1": "Rear or top edge near J6 and J8",
        "D2": "With D1, near J6 and J8",
        "D3": "With D1, near J6 and J8",
        "D4": "With D1, near J6 and J8",
    }
    for ref, note in notes.items():
        start = text.find(f'(ref "{ref}")')
        if start < 0:
            raise SystemExit(f"missing {ref}")
        nxt = text.find("\n    (comp", start)
        block = text[start:nxt if nxt > 0 else None]
        if f'(name "Text") "{note}"' not in block:
            raise SystemExit(f"{ref} Text field is not the placement note")
    for link_name, from_node, to_node in (
        ("DOUT_U1", ("U1", "24"), ("U2", "1")),
        ("DOUT_U2", ("U2", "24"), ("U3", "1")),
        ("DOUT_U3", ("U3", "24"), ("U4", "1")),
        ("DOUT_U4", ("U4", "24"), ("U7", "1")),
    ):
        link = nodes(link_name)
        if link != {from_node, to_node}:
            raise SystemExit(f"{link_name} nodes {link}")
    dout = nodes("DOUT")
    if dout != {("U7", "24")}:
        raise SystemExit(f"DOUT nodes {dout}")
    for n in range(1, 5):
        for pin_name in MATRIX_PIN_NAMES:
            net_name = f"DS{n}_{pin_name}"
            actual = nodes(net_name)
            expected = {(f"U{n}", str(MATRIX_DRV_PINS[pin_name])), (f"DS{n}", pin_name)}
            if actual != expected:
                raise SystemExit(f"{net_name} nodes {actual} != {expected}")
    for hdr_pin, matrix_pin in J7_PIN_MAP.items():
        net_name = f"DS5_{matrix_pin}"
        actual = nodes(net_name)
        expected = {("U7", str(MATRIX_DRV_PINS[matrix_pin])), ("J7", str(hdr_pin))}
        if actual != expected:
            raise SystemExit(f"{net_name} nodes {actual} != {expected}")
    for hdr_pin, matrix_pin in J9_PIN_MAP.items():
        net_name = f"DS5_{matrix_pin}"
        actual = nodes(net_name)
        expected = {("U7", str(MATRIX_DRV_PINS[matrix_pin])), ("J9", str(hdr_pin))}
        if actual != expected:
            raise SystemExit(f"{net_name} nodes {actual} != {expected}")


def _clean_strays() -> None:
    for folder in (REPO, HERE, Path.cwd()):
        for name in ("skidl_REPL.log", "skidl_REPL.erc", "makineat.log", "makineat.erc", "maki_nita.log", "maki_nita.erc"):
            path = folder / name
            if path.is_file():
                path.unlink()


def main() -> None:
    build()

    nets = []
    for part in builtins.default_circuit.parts:
        for pin in part.pins:
            if pin.net is not None:
                nets.append(pin.net)
    apply_netclasses(set(nets))
    generate_netlist(file_=str(NET_PATH), do_backup=False, tool=KICAD10)
    text = annotate_netlist_classes(NET_PATH.read_text(encoding="utf-8"))
    NET_PATH.write_text(text, encoding="utf-8")
    _check(text)
    targets = [Path(arg) for arg in sys.argv[1:] if arg.endswith(".kicad_pro")]
    pro_paths = targets if targets else PRO_PATHS
    names = {getattr(net, "name", "") or "" for net in nets}
    for pro_path in pro_paths:
        if pro_path.is_file():
            old = pro_path.read_text(encoding="utf-8")
            new = patch_kicad_pro(old, assignments_for(names))
            if new != old:
                pro_path.write_text(new, encoding="utf-8")
                print(f"updated net classes in {pro_path}")
    print(f"wrote {NET_PATH}")
    _clean_strays()


if __name__ == "__main__":
    main()
