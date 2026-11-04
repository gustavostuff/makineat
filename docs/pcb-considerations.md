Note: This doc is a routing reference during PCB layout. It will be removed once the boards are fabricated, tested, and these rules are no longer needed for day-to-day edits.

# PCB routing considerations

Layout and routing rules for the main system PCB (`makineat_v_05.kicad_pcb`) and the cartridge PCB focus on noise isolation between high-current pulsed matrix lines, fast digital clocks, sensitive high-impedance analog audio paths, and timing-critical pulse trains.

## Ground pour via stitching (main PCB)

The active main layout ties **F.Cu** and **B.Cu** ground pours with a stitched via grid (ViaStitcher-style fill: about **5 mm** horizontal and vertical pitch, stagger pattern, clearance **0** to the zone). Goals:

- Lower plane impedance between the matrix driver cluster and the audio corner so digit return pulses see a short path across layers.
- Reduce slotting in the pour from signal tracks that cut one side only, especially under SPI and `EVENT` corridors.
- Give probe and clip grounds (future test points in [test-points-and-status-leds.md](test-points-and-status-leds.md)) a nearby via to the opposite pour.

Rules of thumb when editing pours after stitching:

- Do not remove stitching vias to make room for test pads without re-running fill on both layers.
- Keep the audio edge (U5, U6) over continuous pour with stitching at least as dense as the matrix half of the board.
- Matrix `RCOL`/`LCOL` and SPI still follow the isolation rules below; stitching supplements routing, it does not replace ground separation from analog inputs.

## Main PCB

### Audio analog versus display SPI and matrix lines

- Signals: `DAC`, `AUDIN` (LM386 pin 3), `BYPASS` (LM386 pin 7), `AMP`, `SPK` versus `CLK`, `DIN`, `LOAD`, and matrix `RCOL`/`LCOL` nets (`DS1_*` through `DS4_*`).
- Why critical:
  - The LM386 amplifier operates with high voltage gain (up to 46 dB). At its high-impedance non-inverting input, millivolt-level coupled noise is amplified directly into the speaker output.
  - The four MAX7219 matrix drivers scan at approximately 800 Hz, with digit cathode pins sinking up to 160 mA of pulse current. Fast switching rise and fall times generate strong electromagnetic and capacitive fields.
  - When digital clock or matrix tracks run parallel to audio inputs, or overlap them on adjacent copper layers (broadside coupling), parasitic capacitance between layers injects switching transients and the 800 Hz scan frequency into the audio path. Power rail filtering and bypass capacitors cannot eliminate noise that couples directly into high-impedance signal nodes.
- Routing rules:
  - Audio components and tracks stay strictly confined to the right edge around U5 and U6, isolated over an unbroken ground pour.
  - Digital control signals and matrix lines must not run parallel to or overlap analog audio traces on opposing layers.
  - Where crossing is unavoidable, tracks cross strictly at 90-degree angles to minimize mutual capacitive and inductive coupling.

### MAX7219 ISET current references versus matrix and clock lines

- Signals: `U1_ISET`, `U2_ISET`, `U3_ISET`, `U4_ISET` versus matrix segment and digit tracks and `CLK`.
- Why critical:
  - The ISET pin sets the reference current for the internal current mirrors that govern LED segment brightness. The pin operates as a high-impedance node pulled up to +5 V through a 28 kohm resistor.
  - Capacitive noise injected into the ISET trace directly modulates the reference current mirror, translating into visible LED brightness flicker, display shimmer, or ghosting across scanned digits.
- Routing rules:
  - Resistors R1 through R4 sit directly beside their respective MAX7219 packages to keep ISET traces as short as possible.
  - Matrix lines and SPI clock lines must never run under or parallel to ISET tracks.

### EVENT audio trigger versus display SPI and matrix lines

- Signals: `EVENT` versus `CLK`, `DIN`, `LOAD`, and matrix tracks.
- Why critical:
  - `EVENT` conveys sound command bytes from the cartridge AVR to the system audio AVR using a single-wire, active-low pulse protocol where bit values depend on pulse width (100 us versus 200 us).
  - The audio AVR samples this line at 40 kHz. Spurious voltage spikes or ringing induced by SPI clock edges or matrix switching can trigger false input interrupts or distort measured pulse widths, resulting in corrupt audio commands or missed sound playback.
- Routing rules:
  - `EVENT` routes cleanly from cart socket contact B8 to U5 pin 23, shielded by surrounding ground plane and kept away from high-speed SPI bus routing.

### Display SPI versus +5V and layer crossings

- Signals: `CLK`, `DIN`, `LOAD` versus `+5V`, and versus each other on F.Cu and B.Cu.
- Context:
  - Display SPI from cart contact B1 through B3 into U1 runs at about 1 MHz. The +5V rail is low impedance with bulk and local bypass capacitors, so short parallel runs or perpendicular crossings against +5V are lower risk than coupling into `EVENT`, `RESET`, `UPDI`, or matrix `RCOL`/`LCOL` returns.
  - Broadside overlap of SPI traces on adjacent layers in the same direction couples more than a short perpendicular crossing with solid ground reference underneath.
- Routing checklist:
  - Group `CLK`, `DIN`, and `LOAD` from J3 toward U1 through U4. Where matrix strip nets share the same area, leave ground copper between the SPI group and `RCOL`/`LCOL` tracks when the layout allows it.
  - Cross `+5V` at 90 degrees. Avoid long same-layer parallel runs of SPI and +5V when a path on the far side of a ground strip is available. The goal is a clean return path, not fear of the supply rail itself.
  - At 1 MHz on a two-layer board with ground pour, differential pairs are unnecessary. SPI net class clearance and continuous pour are sufficient unless bring-up shows SPI or display errors on unusually long segments.

### Reset and programming lines versus matrix returns

- Signals: `RESET`, `UPDI`, and `AUDIO_UPDI` versus matrix `RCOL`/`LCOL` returns and digit-heavy segments of the `DS1_*` through `DS4_*` nets.
- Why critical:
  - Digit and segment return tracks carry up to 160 mA in rapid switching pulses. High-impedance reset and UPDI lines running beside or over these high-current tracks risk false resets or flashing interruptions from induced inductive transients.
- Routing rules:
  - Reset and UPDI lines route away from matrix scan lines and remain surrounded by the ground pour.
  - Display SPI (`CLK`, `DIN`, `LOAD`) may run parallel together for long distances. Those three nets stay separated from `EVENT`, `RESET`, and `UPDI`. `EVENT` and `RESET` may share a routing corridor. Perpendicular crossings with ground reference are acceptable where nets must meet.

## Cartridge PCB

### EVENT trigger versus display SPI clock

- Signals: `EVENT` (PA0) versus `CLK` (SPI clock).
- Why critical:
  - On the compact 44x37 mm cartridge board, the game AVR outputs both high-frequency SPI data to refresh the display field and the time-sensitive `EVENT` sound pulses.
  - Running `CLK` in parallel with `EVENT`, or overlapping them on opposing layers, capacitive-couples fast clock edge transients onto `EVENT`. Because the system audio AVR detects edges on this line, clock noise can register as false pulse starts, corrupting sound indexes.
- Routing rules:
  - `EVENT` and `CLK` follow distinct routing corridors from the microcontroller pins to their respective edge connector fingers. They must not run parallel or overlap on top and bottom copper layers.

### Programming and reset versus display SPI lines

- Signals: `UPDI`, `RESET` versus `CLK`, `DIN`.
- Why critical:
  - Fast digital switching edges adjacent to high-impedance reset or UPDI tracks can inject noise that causes unexpected CPU resets during gameplay or disrupts flash verification during cartridge programming.
- Routing rules:
  - Programming lines stay isolated from the SPI bus and stay shielded by the ground pour.

### Controller switch inputs versus high-speed digital

- Signals: `P1_*` and `P2_*` switch lines versus `CLK`, `DIN`.
- Why critical:
  - Controller traces connect directly to external wiring harnesses leading to arcade sticks and buttons. Coupling high-frequency SPI switching noise onto these lines turns the controller cables into accidental antennas, increasing electromagnetic radiation.
- Routing rules:
  - Maintain ground separation between SPI digital lines and the controller input traces leading to the edge fingers.

## Critical short traces

Beyond power rail decoupling capacitors, three high-sensitivity paths must stay strictly constrained in physical length:

### 1. MAX7219 ISET resistors (U1 to U4, pin 18)
- Target length: Under 5 to 10 mm.
- Rationale: Pin 18 is a high-impedance current reference node pulled to 5 V through 28 kohm. Long traces act as antennas that pick up local matrix switching transients, modulating the internal current mirrors and causing visible display brightness flicker.
- Rule: Position R1 through R4 directly beside pin 18 of each MAX7219 package.

### 2. LM386 audio input and bypass (U6, pins 2, 3, and 7)
- Target length: Under 5 to 10 mm.
- Rationale: The LM386 has up to 46 dB of voltage gain. Long traces between the DAC divider resistors (R6, R15, R16) and pin 3, or between pin 7 and bypass capacitor C6, readily pick up matrix scan pulses and inject a loud buzz into the speaker output.
- Rule: Cluster R6, R15, R16, and C6 immediately beside U6.

### 3. LM386 Zobel snubber network (U6, pin 5 to R26 and C46)
- Target length: Under 10 to 15 mm.
- Rationale: The series 10 ohm resistor (R26) and 47 nF capacitor (C46) suppress high-frequency parasitic RF oscillation in the amplifier power output stage. Excess trace inductance between the output pin and the snubber degrades amplifier stability.
- Rule: Place R26 and C46 directly at pin 5 with an immediate return to the ground pour.

## Primary layout priority

The single most critical layout constraint across both boards is isolating the analog audio input path from the MAX7219 matrix scan lines and SPI lines on the main PCB.

Digital logic lines operate with 1.5 V to 2 V of noise margin, but analog audio has zero noise margin. The LM386 provides up to 46 dB of voltage gain, meaning microvolt-level capacitive coupling into the high-impedance DAC divider or amplifier inputs gets amplified directly into the speaker. Because the four display drivers sink up to 160 mA per digit at an 800 Hz scan frequency, any parallel or broadside trace overlap will inject an audible 800 Hz buzz that cannot be removed by power rail bypass capacitors or ferrite beads after fabrication.
