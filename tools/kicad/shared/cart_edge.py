"""Shared 20-contact cart edge.

The console socket is an EDAC 395-020-558-212 (90° motherboard part), ten contacts a side.
Side A sits at the bottom of the connector on the main PCB, carrying
player inputs. Side B sits at the top of the connector on the main PCB,
carrying power, audio events, reset, programming, and display signals.
"""

from __future__ import annotations

# (contact, A net, A avr pin, B net, B avr pin)
_ROWS = (
    (1, "P2_BTN", 11, "CLK", 28),
    (2, "P2_DOWN", 8, "LOAD", 27),
    (3, "P2_UP", 7, "DIN", 26),
    (4, "P2_RIGHT", 10, "GND", None),
    (5, "P2_LEFT", 9, "GND", None),
    (6, "P1_BTN", 5, "UPDI", 19),
    (7, "P1_DOWN", 2, "RESET", 18),
    (8, "P1_UP", 1, "EVENT", 22),
    (9, "P1_RIGHT", 4, "GND", None),
    (10, "P1_LEFT", 3, "+5V", None),
)


def contacts():
    rows = []
    for number, a_net, a_avr, b_net, b_avr in _ROWS:
        rows.append((number, "A", a_net, a_avr))
        rows.append((number, "B", b_net, b_avr))
    return rows


def socket_pad(number: int, side: str) -> int:
    if side == "A":
        return 2 * number - 1
    return 2 * number


def socket_pins():
    pins = []
    for number, side, _net, _avr in contacts():
        pins.append((socket_pad(number, side), f"{side}{number}"))
    return pins


def finger_pins():
    pins = []
    for number, side, _net, _avr in contacts():
        name = f"{side}{number}"
        pins.append((name, name))
    return pins
