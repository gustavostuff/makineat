# MakiNeat - Status Digits

Status: In the SKiDL netlist (`tools/kicad/main-pcb/skidl/makineat.py`)  
Scope: Add a fifth MAX7219 (U7) by extending the display chain. This chip drives up to eight 7-segment digits for credits, lives, score, level, timer, or other numeric information. All eight digits route to two 2x4 pin headers (J7 for segments, J9 for digits) for clean copper tracing and flexible physical placement.

## 1. Goal

A fifth MAX7219 cascaded after the four matrix drivers provides eight 7-segment digits.

The visual style is designed to evoke the retro control panel digits from the Back to the Future movie, using similar display colors (such as red, green, and amber or yellow) across grouped digits, but without fixed printed labels. Omitting permanent functional labels on the cabinet faceplate allows each game cartridge to assign its own meaning to each digit cluster dynamically.

<img src="../img/references/time_circuits.png" alt="Time circuits reference" />

Typical allocations:

* Two digits for Credits (00 to 99)
* Two digits for Lives (00 to 99)
* Remaining four digits for score, level, timer, or high score

The hardware and low-level driver treat every digit as generic numeric display positions. Individual games assign specific meaning.


## 2. Hardware

While most components across the MakiNeat system are through-hole parts for straightforward hand assembly, the 7-segment status display subsystem uses surface-mount passive display modules. Driver IC U7, its passives, and the two 2x4 connectors J7 and J9 are defined in the SKiDL netlist.

### 2.1 Driver IC (U7) and Main Board Placement

* Part: MAX7219ENG+ or MAX7219CNG+ in DIP-24 through-hole package
* Placement: Both U7 and the male header connectors (J7 and J9) will live in the currently free space on the left side of the main system PCB.
* Power: +5V and GND from the main system rail, decoupled with a 100 nF ceramic capacitor (C7) and a 10 uF electrolytic bulk capacitor (C17)
* ISET resistor: R7 (28 kohm) set to match current requirements of the selected 7-segment modules

### 2.2 Chain Connection

The display chain extends through U7:

`Cart AVR -> U1 -> U2 -> U3 -> U4 -> U7`

* Signals DIN, CLK, and LOAD originate from cartridge pins PA4, PA6, and PA7.
* One board trace (`DOUT_U4`) connects DOUT of U4 to DIN of U7.
* CLK and LOAD distribute in parallel across all five drivers.
* No additional edge connector pins are required on the cartridge interface.

### 2.3 Display Daughterboard and Connectors

The two 4-digit SMD white display units will be soldered to another PCB (a display daughterboard). This PCB connects to the main system board via two 2x4 pin headers (J7 for segments, J9 for digits, `Connector_PinHeader_2.54mm:PinHeader_2x04_P2.54mm_Vertical`) on the left side.

No designs exist yet for the new PCB holding the digits boards, but it will probably be L-shaped:

```
 --------
|   --   |
|  |  |  |   <---- pin headers (J7, J9) that go into the main PCB
|  |  |  |
|   --   |________________________________________________________
|                                                                 |
|                                                                 |
|                   7-segment displays go here                    |
|                                                                 |
|_________________________________________________________________|
```

The vertical stem positions the headers to mate directly into the left side of the main system PCB, while the horizontal section extends underneath the 16x16 LED matrix array to hold the two display modules.

The status display subsystem settles on two passive 4-digit SMD modules (eight digits total), both emitting white light:

* Two 4-digit white SMD modules
* All color styling achieved through overlay filtering

These units are passive common-cathode SMD modules (29.2 mm x 12 mm x 3.75 mm) featuring castellated edge pads. Emitting pure white light allows the front overlay to tint each digit into any desired color scheme. Being passive, they contain no onboard controller and are wired directly to U7 (no +5V or GND rails required on the passive display module).

<img src="../img/references/7-segment-displays.png" alt="Passive 4-digit SMD 7-segment display modules" />

The 16 display lines are split into two 2x4 headers on the main board to allow direct copper tracing along both sides of U7 without crossing traces:

#### J7: Segment Anode Header (U7 Pins 13-24)

| J7 Pin | U7 Pin | Signal | Net Name |
|---|---|---|---|
| 1 | 14 | SEGA | DS5_A |
| 2 | 15 | SEGF | DS5_1 |
| 3 | 16 | SEGB | DS5_B |
| 4 | 17 | SEGG | DS5_0 |
| 5 | 20 | SEGC | DS5_6 |
| 6 | 21 | SEGE | DS5_5 |
| 7 | 22 | SEGDP | DS5_3 |
| 8 | 23 | SEGD | DS5_D |

#### J9: Digit Cathode Header (U7 Pins 1-12)

| J9 Pin | U7 Pin | Signal | Net Name |
|---|---|---|---|
| 1 | 2 | DIG0 | DS5_7 |
| 2 | 3 | DIG4 | DS5_8 |
| 3 | 5 | DIG6 | DS5_9 |
| 4 | 6 | DIG2 | DS5_F |
| 5 | 7 | DIG3 | DS5_4 |
| 6 | 8 | DIG7 | DS5_C |
| 7 | 10 | DIG5 | DS5_E |
| 8 | 11 | DIG1 | DS5_2 |

### 2.4 Auxiliary Expansion Header

A 2x6 pin male header provides general-purpose expansion signals:

* +5V and GND
* Unused GPIO lines from the audio microcontroller (U5)
* Optional I2C or SPI interface lines for external peripherals (such as EEPROM, real-time clocks, or auxiliary controls)

## 3. Firmware Architecture

Driver changes are located in cartridge firmware (`tools/fw/`).

### 3.1 Cascade Chain Configuration

In `flush.c`, the cascade chain count increases:

```c
#define CHAIN 5
```

Each transmission burst sends five 16-bit words. The fifth word targets U7.

### 3.2 Register Initialization for U7

During `drivers_init()`, U7 receives configuration appropriate for 7-segment numeric decoding:

| Register | Value | Function |
|---|---|---|
| Decode Mode (0x09) | 0x0F or 0xFF | Code-B decode enabled across active digits |
| Intensity (0x0A) | 0x0A to 0x0F | Brightness set independently from matrix panels |
| Scan Limit (0x0B) | 0x03 or 0x07 | Four digits (0x03) or eight digits (0x07) |
| Shutdown (0x0C) | 0x01 | Normal operation |
| Display Test (0x0F) | 0x00 | Normal display mode |

Matrix drivers U1 through U4 maintain raw segment bitmask mode (Decode Mode 0x00, Scan Limit 0x07).

### 3.3 Status Display API

```c
void status_credits(uint8_t value);
void status_lives(uint8_t value);
void status_set_digit(uint8_t digit, uint8_t code);
void status_set_pair(uint8_t pair, uint8_t value);
```

These functions update an internal RAM buffer. During the regular frame flush, `drivers_flush()` packs the status buffer into Code-B data bytes for transmission in the fifth word slot.

Games that omit status calls leave digits blank.

### 3.4 Frame Timing

Transmitting five words instead of four adds negligible overhead (around 128 us at a 1 MHz SPI clock rate), fitting well within the 16.6 ms (60 Hz) frame interval.

## 4. Mechanical and Visual Layout

* Driver U7 and the mating headers (J7 and J9) live in the available open space along the left side of the main system PCB.
* The separate L-shaped display daughterboard plugs into the left-side headers on the main system PCB, positioning the two 4-digit display modules directly below the 16x16 matrix array on the front panel.
* The full-front overlay assembly (detailed in [overlays.md](overlays.md)) extends across both the 16x16 playfield and the eight status digits. A single rigid plastic frame holds a clear carrier acetate sheet with colored gel cutout pieces affixed on top, defining custom color zones across the four digit pairs (for example, blue, red, green, and yellow).
* Game labels (such as Lives, Pts, Jade, or Time) are applied as printed stickers on the plastic frame directly below the digits, providing high-contrast legends tailored to each title.
* Board mounting holes remain unobstructed.

## 5. Bill of Materials

Most parts are standard through-hole components, while the display modules are compact surface-mount units:

| Quantity | Item | Notes |
|---|---|---|
| 1 | MAX7219 DIP-24 | Through-hole display driver IC U7 (lives on left side of main PCB) |
| 2 | 4-digit SMD 7-segment display (White) | Passive common-cathode SMD module (soldered to L-shaped daughterboard) |
| 1 | ISET resistor (R7, 28 kΩ) | Through-hole horizontal axial resistor |
| 1 | 100 nF ceramic capacitor (C7) | Through-hole decoupling capacitor |
| 1 | 10 uF electrolytic capacitor (C17) | Through-hole bulk power rail capacitor |
| 2 | 2x4 male pin headers (J7, J9) | Display daughterboard interface to main PCB (`PinHeader_2x04_P2.54mm_Vertical`) |
| 1 | 2x6 male pin header | Auxiliary expansion interface |

## 6. Implementation Steps

1. Add U7 footprint and decoupling capacitors to the main board schematic. *(Done in SKiDL)*
2. Connect DOUT of U4 to DIN of U7 (`DOUT_U4`), sharing CLK and LOAD. *(Done in SKiDL)*
3. Route U7 segment pins to J7 and digit pins to J9. *(Done in SKiDL)*
4. Add the 2x6 auxiliary expansion header.
5. Update `CHAIN` to 5 in `tools/fw/flush.c` and expand `drivers_init()` and `drivers_flush()`.
6. Implement the status buffer API in `tools/fw/`.
7. Add simulation support in `tools/sim/` for fifth-driver telemetry.
