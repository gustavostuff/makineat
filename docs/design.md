# System design

The electrical source of truth is `tools/kicad/main-pcb/skidl/makineat.py`. `tools/kicad/main-pcb/skidl/export_netlist.sh` writes `tools/kicad/main-pcb/skidl/makineat.net` and patches net classes in the main-board `*.kicad_pro` files (including `makineat_v_05.kicad_pro`). The schematic and PCB live under `tools/kicad/main-pcb/`. The cartridge netlist is `tools/kicad/cart-pcb/skidl/`. Cart edge finger names match `tools/kicad/shared/cart_edge.py`.

## PCB and layout status

| Board | KiCad tree | Layout state |
| --- | --- | --- |
| Main system | `tools/kicad/main-pcb/` (`makineat_v_05.kicad_pcb` is the active layout) | Copper routing is in place but not final. EMI margins, test points, noise review, and similar polish remain. Not fabricated or tested in hardware. |
| Cartridge | `tools/kicad/cart-pcb/` | Layout matches the SKiDL netlist and appears ready for fabrication review. Not fabricated or tested in hardware. |
| Status digit daughterboard | [status-digits.md](status-digits.md) | No KiCad project or PCB file yet. |

## Main system board

The main system board is a 100x100 mm 2-layer board with ground pours on both sides. Parts are through-hole on a 2.54 mm grid. The game AVR is on the cartridge PCB. Six M3 (3.2 mm) mounting holes (H1 to H6) secure the corners and top edge.

Cables for rear-mounted connectors leave the board from behind the cabinet face. Front-mounted connectors sit along the top edge of the main PCB toward the player. The planned cabinet accepts cartridges perpendicular to the main PCB through a front or side card-edge slot. J3 is a straight card-edge socket on that mating plane. J7 and J9 mate the status digit daughterboard on the front. The 16x16 display field occupies the central and lower area of the front.

| Ref | PCB side |
| --- | --- |
| J1 | Rear |
| J2 | Rear |
| J3 | Front |
| J4 | Rear |
| J5 | Rear |
| J6 | Rear |
| J7 | Front |
| J8 | Rear |
| J9 | Front |

| Ref | Role |
| --- | --- |
| J1 | 1x6 header, Player 1 controls (GND, LEFT, RIGHT, UP, DOWN, button) |
| J2 | 1x6 header, Player 2 controls (GND, LEFT, RIGHT, UP, DOWN, button) |
| J3 | 20-contact card-edge socket, 2.54 mm pitch (10 contacts per side, 5.08 mm between rows) |
| J4 | 1x2 vertical header, 5 V power (JST XH: Pin 1 GND, Pin 2 +5V) |
| J5 | 1x2 vertical header, speaker (Pin 1 GND, Pin 2 SPK) |
| J6 | 1x3 header, audio AVR UPDI (Pin 1 UPDI, Pin 2 +5V, Pin 3 GND) |
| J7 | 2x4 header, status display segments (U7 pins 13-24) |
| J8 | 1x3 header, cartridge AVR UPDI (in-system flashing: Pin 1 UPDI, Pin 2 +5V, Pin 3 GND) |
| J9 | 2x4 header, status display digits (U7 pins 1-12) |
| U1 to U4 | MAX7219, one per 8x8 matrix block |
| U5 | AVR128DB28, audio and planned brightness control |
| U6 | LM386N-1 amplifier |
| U7 | MAX7219, 8-digit status display driver |
| DS1 to DS4 | 8x8 common-cathode matrices |

The planned cabinet routes J2 to a side-panel port for an external arcade stick and button harness (1x6, same pinout as the table above). J1 remains on the rear harness for the primary in-cabinet controls. Both headers feed the same cart edge input nets.

A part on "UN" uses "N" as part of its silkscreen label. A second part on that IC adds 10, a third adds 20. For instance:

- On U5: C5 is the first bypass, C15 the second, C25 the third. R5 is EVENT pull-up, R15 is DAC load, R25 is RESET pull-up.
- On U1: R1 is ISET, C1 is the 100 nF bypass, C11 is the 10 uF bulk.
- On U7: R7 is ISET, C7 is the 100 nF bypass, C17 is the 10 uF bulk.

R1 to R4 and R7 are horizontal axial resistors on a 10.16 mm pitch. Audio, event, and reset resistors stand vertical on a 2.54 mm pitch.

Power enters on J4 (Pin 1 GND, Pin 2 +5V). A 470 uF electrolytic bulk reservoir capacitor (C10) sits directly across +5V and GND near J4 to buffer matrix scan transients and audio amplifier peaks. The barrel jack, polyfuse, Schottky, and panel switch are off the board (design TBD).

## Display

Four white matrices form a 16x16 field. DS1 is top left, DS2 top right, DS3 bottom left, DS4 bottom right. 

Each matrix sits in two elevated 1x8 female pin socket strips. Its MAX7219 driver (U1 to U4) and 100 nF ceramic bypass capacitor (C1 to C4) sit nested directly underneath the matrix body between the socket rows. Bulk 10 uF electrolytic capacitors (C11 to C14) sit slightly outside the matrix area.

Vertical clearance under the matrix body depends on socket height and needs physical verification during assembly. If a DIP-24 socket plus the MAX7219 package is too tall to clear the seated matrix, the drivers can be soldered directly to the board without IC sockets. Because the MAX7219 is fixed-function silicon without firmware or flashing needs, sockets are primarily valuable for the AVR microcontrollers.

DIN, CLK, and LOAD come from the cart through J3 into U1. DOUT chains U1 to U2 to U3 to U4 to U7.

The cart AVR runs at 12 MHz and shifts the frame over DIN, CLK, and LOAD at 1 MHz (0.64 ms full frame update, under 4% of the 60 Hz frame interval). Each MAX7219 scans its block. Segment pins source anodes. Digit pins sink cathodes.

The MAX7219 multiplexes its eight rows using an autonomous internal oscillator at ~800 Hz (~1.25 ms per full 8-row cycle). This scan frequency is fixed in hardware and cannot be reconfigured in software. During each 16.67 ms (60 Hz) game frame, the display completes roughly 13 full refresh cycles. Because driver registers update atomically on the rising edge of LOAD, frame flushes remain clean without tearing or flicker.

| Block | Screen columns | Screen rows | Chain order |
| --- | --- | --- | --- |
| DS1 | 0 to 7 | 0 to 7 | First |
| DS2 | 8 to 15 | 0 to 7 | Second |
| DS3 | 0 to 7 | 8 to 15 | Third |
| DS4 | 8 to 15 | 8 to 15 | Fourth |

## Cart edge (J3)

The electrical and mechanical interface is defined by 2.54 mm pitch on both the main-board through-hole pattern and the cartridge gold fingers (ten contacts per edge, 5.08 mm between the two rows). Any socket and cart edge that match that geometry can mate. The design is not tied to a single manufacturer part number.

J3 is a straight 2x10 card-edge motherboard socket. EDAC 395-020-520-802 is a recommended default for price and distributor availability (DigiKey, Mouser, and similar).

The KiCad 3D view of the main PCB (currently `makineat_v_05.kicad_pcb`) assigns the 395-020-520-802 model to J3.

Side A pins sit at the bottom part of the connector on the main PCB, carrying player inputs. Side B pins sit at the top part of the connector on the main PCB, carrying power, audio events, reset, programming, and display signals.

| Contact | Side A | Side B |
| --- | --- | --- |
| 1 | P2_BTN | CLK |
| 2 | P2_DOWN | LOAD |
| 3 | P2_UP | DIN |
| 4 | P2_RIGHT | GND |
| 5 | P2_LEFT | GND |
| 6 | P1_BTN | UPDI |
| 7 | P1_DOWN | RESET |
| 8 | P1_UP | EVENT |
| 9 | P1_RIGHT | GND |
| 10 | P1_LEFT | +5V |

Player 2 cabinet stick and button wires land on J2 (a 1x6 header with pins 1 to 6 carrying GND, LEFT, RIGHT, UP, DOWN, BUTTON) and route to cart Side A contacts 1 through 5. Player 1 stick and button wires land on J1 (the same pin arrangement) and route to cart Side A contacts 6 through 10. +5V reaches the cart on Side B contact 10. RESET on cart contact B7 ties to U5 reset with a 10 kohm pull-up to +5V (R25).

## Audio (U5, U6, J5)

U5 sits along the left edge of the board. U6 and the speaker header J5 (a 1x2 vertical 2.54 mm header, Pin 1 GND, Pin 2 SPK) sit along the right edge.

The cart drives EVENT on contact B8 from PA0. U5 listens on PA1 and plays rows from `tools/fw/synth.c`. The mix goes to the DAC, through the divider, into U6, and out to J5. U5 runs at 20 MHz. The byte is eight low pulses on EVENT, 100 us or 200 us wide, sampled at 40 kHz on the system board. The mix updates at 20 kHz.

## Net classes

From `tools/kicad/main-pcb/skidl/net_classes.py`: 5V is 0.8 mm track width. **RCOL** and **LCOL** are the two pin strips on each 8x8 matrix (nets `DSn_0` through `DSn_7` and `DSn_8` through `DSn_F` for `n` = 1 to 4). **MATRIX** is the U7 status-digit nets (`DS5_*`). SPI is CLK, DIN, LOAD, DOUT, and the cascade links `DOUT_U1` through `DOUT_U4`. RCOL, LCOL, and MATRIX use 0.4 mm width instead of the default 0.25 mm. Each digit pin sinks up to eight LEDs simultaneously, carrying peak return currents around 160 mA. The 0.4 mm width lowers line resistance and voltage drop across the scan cycle, keeping LED brightness uniform between digits and reducing switching noise into adjacent lines. AUDIO is DAC, AUD, AUDIN, AMP, SPK, ZOBEL, and BYPASS. Default is 0.25 mm. GND is the pour.

## Not in the SKiDL netlist yet

* Global brightness (Bright button with tap-to-increment and hold-plus-stick adjustment, system AVR writing MAX7219 intensity) and extra system wiring for that feature.
* Status display daughterboard PCB layout (see [status-digits.md](status-digits.md)). Driver U7 and mating headers J7 and J9 are on the main PCB netlist.
* The playfield is the 256 LEDs from the four matrices only.

## Future plans

See [future-plans.md](future-plans.md) for roadmap items and technical specifications beyond the current main and cart PCBs, including the status digit daughterboard layout, translucent marquee backlighting, AKWF wavetable audio synthesis, dual-input USB-C power, headphone output, and the 32x32 display expansion.

