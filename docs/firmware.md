# Firmware and simulator

Sources live under `tools/fw/`. Cartridge game firmware runs on the cartridge AVR128DB28 at 12 MHz, while system audio synthesis runs on the AVR128DB28 at 20 MHz.

## Game loop and display refresh

The runtime loops at 60 Hz (~16.67 ms per frame). Each pass debounces inputs, executes `game_tick`, and flushes the 16x16 framebuffer across the MAX7219 chain.

The framebuffer is sixteen rows of sixteen bits in `frame[16]`. `pixel`, `pixel_get`, and `clear` form the drawing API. There is no per-pixel brightness in RAM.

DIN is PA4, CLK is PA6, LOAD is PA7. One flush transmits eight digit rows across all four drivers via SPI. During init, firmware configures:
- Decode mode register (0x09) to 0x00 (raw matrix drive, no BCD decode).
- Scan limit register (0x0B) to 0x07 (scan all 8 digits).
- Shutdown register (0x0C) to 0x01 (normal operation).
- Intensity register (0x0A) defaults to 0x0F (full intensity).

### Refresh rate and timing

The MAX7219 uses an autonomous internal RC oscillator with a nominal multiplex scan rate of ~800 Hz (~1.25 ms per full 8-row cycle). This scan frequency is fixed in hardware by internal silicon timing and cannot be reconfigured via register writes.

In relation to the 60 Hz game loop:
- During each 16.67 ms game frame, the MAX7219 hardware autonomously multiplexes the LED rows approximately 13.3 times.
- Because data is double-buffered within the driver registers and latched atomically on the rising edge of LOAD, frame updates never tear or glitch across row scan cycles.
- Operating the hardware multiplexer at ~800 Hz keeps scanning well above human flicker fusion thresholds while completely decoupling LED scanning from CPU frame execution.

## Inputs and debouncing

Player 1 and Player 2 stick and action button nets reach dedicated input pins on the cartridge AVR (ten switch inputs in total). Each switch is active low with an internal pull-up.

Debouncing in `tools/fw/input.c` samples inputs once per 60 Hz tick. Two consecutive disagreeing ticks are required before a state change registers, providing approximately 33 ms of noise immunity.

The planned system brightness control uses debounced input on the Bright button and stick. Tapping the Bright button advances brightness to the next level (wrapping from 15 to 1). Holding the Bright button while pressing stick Up or Down steps the level up or down. Levels are 1, 3, 5, 7, 9, 11, 13, and 15 across all four MAX7219 parts. Zero is not used. Bright and intensity writes are not in the SKiDL netlist yet. The simulator does not model Brightness.

## Sound

Games call `sound(index)` with a catalog index 0 through 255. Index 0 is silent. Sound definitions live in `tools/fw/synth.c`. The cartridge transmits index bytes across the EVENT line as eight low pulses (100 us or 200 us wide). The system audio AVR samples EVENT at 40 kHz, decodes the index, and mixes square wave, triangle wave, and pseudo-random noise channels at 20 kHz into the 10-bit DAC output on PD6.

## Game SDK

Shared drawing and collision code lives under `tools/fw/`. Pick the tier that fits the cart.

### Frame (`frame.c`)

The scan-out buffer is sixteen rows of sixteen bits in `frame[16]`. `pixel`, `pixel_get`, and `clear` are the basic API. `frame_rect`, `frame_hline`, and `frame_vline` fill clipped rectangles and lines on the screen (columns and rows 0..15).

### Map (`map.c`)

A separate 16 by 16 occupancy bitmap uses the same bit layout as `frame`. Use it for walls, long bodies, and static art without spending entity slots. Entities stay limited to 4 by 4 shapes; a Snake-style body belongs on the map, not as one entity per segment.

- `map_clear`, `map_get`, `map_set`, `map_toggle`, `map_rect`
- `map_stamp_frame` ORs lit map cells into `frame` without clearing
- `map_solid` is the same as `map_get` (handy for `collide_step` callbacks)
- `scene_blocked(x, y, mask)` with `SCENE_MAP` and/or `SCENE_ENTITIES`

Grid games probe the next cell with `map_get` or `scene_blocked` before moving. Bouncing movers (Arkaled) keep using `collide_step` in `collide.c`.

**Draw recipes**

- Entities only: `world_draw();`
- Map only: `clear(); map_stamp_frame();`
- Map then entities: `clear(); map_stamp_frame(); world_paint();`
- Entities then manual tweaks (Arkaled-style blinks): `world_draw();` then `pixel()` overrides

When a cart uses the map on hardware, add `../map.c` to `GAME_SRCS` in `tools/fw/avr/Makefile` (same pattern as `collide.c` for Arkaled).

### Entity (`entity.c`)

Up to 64 sprites in world space -8..23 with an 8-pixel margin around the screen. Shapes are at most 4 by 4, packed with `SHAPE1` through `SHAPE4`. `world_tick` steps every live entity; `world_draw` clears the frame and calls `world_paint`, which stamps the visible window into `frame`. `world_paint` alone composes on top of an existing frame (for example after `map_stamp_frame`).

Pairwise overlap uses `world_pairs`, `entity_covers`, and `entity_overlaps`. Query helpers: `entity_at(x, y)` on screen cells 0..15, `entity_set_pos`, `entity_set_motion`, `entity_set_flags`, and `entity_clear_flags`.

### Collide (`collide.c`)

`collide_step` moves one cell at a time with axis bounce against a `solid` callback. Link this module only on carts that need it.

## Carts

Each cart implements `void game_tick(const Input *in)`. Carts recognized in `cart_placed.json` include `space-shooter`, `pong`, `arkaled`, `text-demo`, and `snake`.
- Build cartridge firmware with `make -C tools/fw/avr` or `make -C tools/fw/avr CART=pong`.
- Build system audio firmware with `make -C tools/fw/audio`.

## Flashing hardware (UPDI)

Both microcontrollers (cartridge AVR on J8 and audio AVR on J6) use identical 3-pin headers:
- Pin 1: UPDI
- Pin 2: +5V
- Pin 3: GND

Common low-cost programmers:
1. **Adafruit UPDI Friend**: Compact USB-C or micro-USB to UPDI programmer with built-in bidirectional resistor and diode network and selectable 3.3V or 5V target power. Uses a 3-wire cable to J6 or J8.
2. **HiLetgo CH340 USB-UART adapter (SerialUPDI)**: Low-cost USB-to-serial adapter. SerialUPDI wiring joins TX to RX through a 4.7 kohm resistor for the bidirectional UPDI line on Pin 1, with 5V on Pin 2 and GND on Pin 3.

Flash commands using `pymcuprog`:
```bash
# Flash cartridge game (with cart seated in J3, programmer plugged into J8):
pymcuprog write -d avr128db28 -t uart -u /dev/ttyACM0 --verify -f tools/fw/avr/game.hex

# Flash console audio synthesis (programmer plugged into J6):
pymcuprog write -d avr128db28 -t uart -u /dev/ttyACM0 --verify -f tools/fw/audio/audio.hex
```

## Simulator

`./sim.sh` builds and launches `tools/sim/makineat-ui`. The netlist model connects the cartridge to matrix drivers U1 through U4, status driver U7, EVENT, and J5. Status digits render from the simulated U7 chain. Host simulation runs at 60 Hz with 12 MHz and 20 MHz timing constants in `tools/sim/clocks.h`. Per-cart acetate overlay tinting is described in [overlays.md](overlays.md).
