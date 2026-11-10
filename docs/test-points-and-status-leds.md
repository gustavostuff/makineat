# Test points and status LEDs

Scope: main system PCB. Test points and status LEDs are in the SKiDL netlist (`tools/kicad/main-pcb/skidl/makineat.py`). The layout file for these parts is `tools/kicad/main-pcb/makineat_v_07.kicad_pcb`. Positions of the new footprints on that board are still open.

Bring-up signals, net names, and connector roles are defined in [design.md](design.md). Analog and digital isolation rules that govern probe placement sit in [pcb-considerations.md](pcb-considerations.md).

## Test points

Footprint: KiCad `TestPoint:TestPoint_Pad_D1.5mm` (1.5 mm round exposed pad, no wire loop). The reference is `TP1` through `TP10`. The value is the net name. A Text field on each pad carries a short placement note.

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

`DOUT` after U4 and `AUDIO_UPDI` are not separate test points. J6 pin 1 already presents `AUDIO_UPDI`.

## Status LEDs

Footprint: 3 mm through-hole LED, 2.54 mm pad pitch, in `MakiNeat.pretty`. Pin 1 is the cathode. Pin 2 is the anode. D1 uses `LED_D3.0mm_Blue.step`, D2 uses `LED_D3.0mm_Green.step`, D3 uses `LED_D3.0mm_Yellow.step`, and D4 uses `LED_D3.0mm.step` (the red body). Each LED has a 1 kohm series resistor from +5V to the anode (`Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P2.54mm_Vertical`). Forward current is about 2 mA to 3 mA. A Text field on each LED carries a short placement note.

Four indicators cover rail presence and firmware-visible health on the system AVR (U5). The game AVR on the cartridge does not drive these LEDs. D1 is hardwired. D2, D3, and D4 light when U5 drives the cathode low.

| Color | Ref | Name | Source | Indication |
| --- | --- | --- | --- | --- |
| Blue | D1 | Power | +5V through R35 to the anode, cathode to GND | Rail present whenever the supply is connected, independent of MCU execution. |
| Green | D2 | System OK | Cathode on U5 PA4 (pin 26), anode from +5V through R45 | Steady on after U5 init. A slow blink during normal audio operation is optional. |
| Amber | D3 | Cart link | Cathode on U5 PA5 (pin 27), anode from +5V through R55 | On or a short blink when cart-side traffic is observed. Off when the cart is missing or not shifting the display chain. |
| Red | D4 | Fault | Cathode on U5 PA6 (pin 28), anode from +5V through R65 | On for a held reset, missing SPI after the power-up timeout, or a later watchdog. Off in healthy operation. |

### Placement notes

- Mount LEDs on the rear or top edge near J6 and J8 so they remain visible during bench bring-up with the cabinet open.
- Route GPIO LED returns through the same ground pour as U5. Keep LED currents off sensitive analog (`AUD`, `BYPASS`) and ISET nets.
- Firmware for the GPIO patterns is not in `tools/fw/` yet. The indication column is the target for that U5 module.

## PCB and netlist status

| Item | SKiDL | `makineat_v_07.kicad_pcb` |
| --- | --- | --- |
| Test points TP1 to TP10 | Present | Positions open |
| Status LEDs and resistors | Present | Positions open |
| Ground via stitching (5 mm stagger grid on pour) | N/A (layout-only) | Present (see [pcb-considerations.md](pcb-considerations.md)) |
