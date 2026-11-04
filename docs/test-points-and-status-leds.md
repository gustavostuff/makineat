# Test points and status LEDs

Scope: main system PCB (`tools/kicad/main-pcb/`, active layout `makineat_v_05.kicad_pcb`). Neither test points nor status LEDs appear in the SKiDL netlist (`tools/kicad/main-pcb/skidl/makineat.py`) or on the fabricated layout yet. This document is the electrical and placement target for a future schematic and PCB update.

Bring-up signals, net names, and connector roles are defined in [design.md](design.md). Analog and digital isolation rules that govern probe placement sit in [pcb-considerations.md](pcb-considerations.md).

## Test points

Footprint: KiCad `TestPoint_Pad_D1.5mm` (1.5 mm round exposed pad, no wire loop). Silkscreen labels `TP1` through `TP10` on the Fab layer with the net name beside each pad where space allows.

Ten pads balance bench access against board area on a 100x100 mm layout. Existing headers (J4 power, J6/J8 UPDI, J3 cart edge) already expose several nets. Test points exist for scope, DMM, and logic-analyzer hooks without jumper wires.

| Label | Net | Role |
| --- | --- | --- |
| TP1 | GND | Ground reference at the power entry (J4 / bulk C10 region). |
| TP2 | GND | Ground reference near the U1 to U4 matrix driver cluster. |
| TP3 | +5V | Input rail at J4 before long distribution runs. |
| TP4 | +5V | Same rail near the MAX7219 display cluster (load sag check). |
| TP5 | CLK | Display SPI clock from cart contact B1 into U1. |
| TP6 | DIN | Display SPI data in. |
| TP7 | LOAD | Display SPI latch. |
| TP8 | EVENT | Cart to system audio trigger (J3 B8, U5 PA1). |
| TP9 | RESET | System and cart reset (J3 B7, U5 reset network). |
| TP10 | AUD | Audio path after the DAC divider, before LM386 U6 (high-impedance analog: probe briefly, avoid long clip leads). |

### Placement notes

- Group TP1 and TP3 on the rear edge with J4. Group TP2 and TP4 on the front or side near the matrix drivers.
- Place TP5 through TP9 in one short row reachable with a cart seated in J3 (SPI and control debug).
- Keep TP10 adjacent to the U5/U6 audio corner; do not route the test pad under matrix `RCOL`/`LCOL` returns or parallel to SPI at length.
- Omit dedicated pads on ISET nets, matrix segment strips, and the speaker output unless a later revision adds shielded coax test fixtures.

Optional pads (not counted in the ten above): `DOUT` after U4 (chain into U7), or `AUDIO_UPDI` (largely redundant with J6 pin 1).

## Status LEDs

Footprint: 3 mm through-hole LED (package per KiCad library choice, 2.54 mm grid friendly). One series resistor per LED from the +5 V rail or from a GPIO, sized for roughly 2 mA to 5 mA forward current (typical 470 ohm to 1 kohm at 5 V, exact value follows the LED datasheet).

Four indicators cover rail presence and firmware-visible health on the system AVR (U5). The game AVR on the cartridge does not drive these LEDs. Status GPIO assignments are not fixed in hardware yet; the table below is the intended meaning once U5 firmware defines the pins.

| Color | Name | Source | Indication |
| --- | --- | --- | --- |
| Blue (or cool white) | Power | Hardwired from +5 V through resistor to LED to GND | Rail present whenever the supply is connected, independent of MCU execution. |
| Green | System OK | U5 GPIO, active-high or active-low per firmware | Steady on after U5 init; optional slow blink during normal audio operation. |
| Amber | Cart link | U5 GPIO, driven from EVENT and/or SPI activity detection | On or short blink when cart-side traffic is observed; off when the cart is missing or not shifting the display chain. |
| Red | Fault | U5 GPIO | On for defined fault conditions (held reset, missing SPI after power-up timeout, future watchdog). Off in healthy operation. |

### Placement notes

- Mount LEDs on the rear or top edge near J6 and J8 so they remain visible during bench bring-up with the cabinet open.
- Route GPIO LED returns through the same ground pour as U5; keep LED currents off sensitive analog (`AUD`, `BYPASS`) and ISET nets.
- Firmware for GPIO patterns is not implemented in `tools/fw/` yet; behavior in the table is a design target for a later U5 module.

## PCB and netlist status

| Item | SKiDL / schematic | `makineat_v_05.kicad_pcb` |
| --- | --- | --- |
| Test points TP1 to TP10 | Not present | Not present |
| Status LEDs and resistors | Not present | Not present |
| Ground via stitching (5 mm stagger grid on pour) | N/A (layout-only) | Present (see [pcb-considerations.md](pcb-considerations.md)) |
