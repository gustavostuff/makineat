# MakiNeat - Future Plans and Roadmap

Status: Roadmap and technical specifications  
Scope: System enhancements planned for post-base-design hardware and firmware revisions.

## 1. Overview

Once the base 16x16 LED matrix arcade hardware is proven and verified, several hardware and firmware enhancements build upon the existing architecture. These features expand audio capability, power flexibility, cabinet illumination, status display, and visual resolution while maintaining core compatibility.

## 2. 7-Segment Status Display Subsystem

A fifth MAX7219 driver (U7) cascaded from matrix driver U4 provides up to eight generic 7-segment numeric digits. Driver U7, its passives, and the two 2x4 connectors (J7 for segments and J9 for digits) are defined in the SKiDL netlist (`tools/kicad/main-pcb/skidl/makineat.py`). While most components are through-hole, the status display uses two passive 4-digit white SMD modules.

* **Daughterboard PCB**: The two 4-digit SMD modules solder to a dedicated display daughterboard PCB. While formal PCB designs are not created yet, the daughterboard will likely adopt an L-shaped outline, positioning two 2x4 male headers on its left side while extending the display section beneath the main 16x16 matrix.
* **Main Board Placement**: Both U7 and the two mating display headers (J7 for segments, J9 for digits) reside in the currently free space on the left side of the main system PCB.
* **Interface**: All eight segment lines (SEGA to SEGG, DP) route through J7 (pins 13-24 of U7) and all eight digit sinks (DIG0 to DIG7) route through J9 (pins 1-12 of U7), enabling direct, non-crossing copper tracing on both sides of U7.
* **Protocol**: Driven via the existing SPI chain using Code-B decoding mode, requiring no additional cartridge edge pins.
* **Visual Style**: White LED digits filtered into vibrant custom colors by a full-front overlay assembly that covers both the 16x16 matrix and the status digits, keeping display styling dynamic and customizable.
* **Framed Overlay Assembly**: A rigid plastic frame holds a clear carrier sheet with pasted color cutout gels and printed label stickers beneath the digits, defining game-specific numeric fields without permanent cabinet legends.
* **Use Cases**: Credits, player lives, high score, game timer, or level indicators.

Complete hardware, pinout, register, and API details are documented in [status-digits.md](status-digits.md).

## 3. Translucent Marquee Artwork with Backlight Illumination

Classic arcade cabinets feature illuminated upper marquee signage. This enhancement provides dedicated backlighting integrated into the cabinet power and control distribution.

### 3.1 Hardware Integration

* **Lighting Elements**: Diffused warm-white or neutral-white 5V LED strip arrays mounted inside the cabinet upper housing behind an acrylic diffuser panel and translucent graphic film.
* **Power Connection**: A dedicated 2-pin polarized header on the main PCB supplies regulated +5V directly from the main power rail, protected by an onboard resettable PTC fuse.
* **Current Budget**: Designed for an operating current of 150 mA to 300 mA, accommodating standard high-density COB (chip-on-board) LED strips without overloading the main DC supply.

### 3.2 Brightness Control

* **Dimming Mechanism**: Low-side N-channel MOSFET switch (such as 2N7002 or BSS138) driven by a PWM output from the system microcontroller.
* **Control Integration**: Marquee brightness levels link directly to the system global display brightness setting, dimming and brightening in tandem with the playfield matrices.

## 4. AKWF Wavetable Audio Synthesis

The current audio system uses procedural wave generators (triangle, square, noise) in `tools/fw/synth.c`. The audio subsystem roadmap incorporates single-cycle digital wavetables from the AKWF (Adventure Kid Waveforms) sound library.

### 4.1 Storage and Memory Architecture

* **Wavetable Format**: Single-cycle waveforms stored as 256-sample 8-bit signed lookup tables.
* **Flash Budget**: The system audio microcontroller (AVR128DB28) provides 128 KB of program flash memory. A bank of 64 distinct single-cycle waveforms occupies only 16 KB of flash, leaving ample room for synth firmware and lookup tables.
* **Sampling Rate**: The DAC output runs at a steady 20 kHz interrupt rate, matching the base audio engine timing.

### 4.2 Synthesis Engine

* **Phase Accumulator**: 16-bit phase accumulators provide high-precision pitch control across musical notes with sub-Hertz tuning accuracy.
* **Envelope Generator**: Software ADSR (attack, decay, sustain, release) amplitude envelopes shaped per voice.
* **Control Protocol**: The existing single-wire EVENT protocol carries 8-bit sound trigger indices from the game cartridge to the audio MCU. Specific index ranges map to instrument presets and melodic sequences without requiring hardware changes to the cart edge connector.

## 5. Dual-Input Power Architecture (USB-C and Barrel Jack)

The base design uses a center-positive 2.1 mm DC barrel jack supplying regulated +5V. A dual-input power circuit introduces a modern USB-C receptacle while preserving the barrel connector.

### 5.1 USB-C Port Configuration

* **Connector**: Standard 16-pin USB Type-C receptacle wired for 5V power sink operation.
* **Configuration Channel**: Two 5.1k ohm pull-down resistors placed on pins CC1 and CC2 signal a legacy 5V power sink to USB-PD and USB-C source devices, enabling up to 3.0A current delivery without active controller negotiation.

### 5.2 Automatic Power Path Selection

* **OR-ing Protection**: Schottky power diodes or a dual P-channel MOSFET ideal-diode controller circuit (such as LTC4412 or discrete P-FET switches) bridge both the barrel input and USB-C input.
* **Backfeed Prevention**: The circuit prevents voltage from backfeeding from one power input into the other when both cables are simultaneously attached.
* **Voltage Drop Minimization**: Ideal-diode MOSFET switches reduce forward voltage drop to under 30 mV, preserving a full 5.0V supply rail to the LED drivers and logic chips.

## 6. Dedicated Headphone Output with Automatic Speaker Mute

The cabinet speaker driven by the LM386 audio amplifier (U6) provides loud playfield audio. A dedicated 3.5 mm TRS stereo headphone jack provides private listening.

### 6.1 Mechanical Switching

* **Jack Type**: 3.5 mm TRS phone jack equipped with isolated normally-closed mechanical shunt switches.
* **Speaker Isolation**: Inserting a headphone plug physically disconnects the speaker return line, muting the cabinet speaker completely without software intervention.

### 6.2 Level Attenuation and Filtering

* **Attenuation Network**: A resistive voltage divider network scales down the 1W speaker-level amplifier output to safe headphone voltage levels (under 100 mV RMS into 32-ohm loads).
* **DC Blocking**: Series 220 uF electrolytic capacitors block DC bias from reaching the headphone drivers.
* **Hearing Protection**: Series limiting resistors prevent sudden high-amplitude pops from exceeding comfortable listening limits.

## 7. High-Resolution Display Variant (32x32 Matrix Grid)

A future high-resolution MakiNeat system revision doubles display resolution from 16x16 to 32x32 pixels, expanding the visual canvas from 256 LEDs to 1024 LEDs.

### 7.1 Display Architecture

* **Matrix Array**: 16 standard 8x8 LED matrix units (such as 1088AS) arranged in a 4x4 panel layout.
* **Driver Network**: 16 MAX7219 display drivers arranged in four parallel 4-chip SPI chains, or two 8-chip cascade chains.
* **Refresh Budget**: At 12 MHz SPI clock rates, updating 16 driver registers requires under 30 us per row scan, maintaining standard 60 Hz frame refresh rates.

### 7.2 Backward Compatibility

* **Pixel Scaling**: 16x16 cartridges remain compatible through automatic 2x2 pixel doubling. Each logical pixel from an older cartridge maps to a 2x2 LED block on the 32x32 field.
* **Cartridge Detection**: An expansion pin on the cartridge edge connector or a firmware header identification byte signals native 32x32 mode versus legacy 16x16 mode.
