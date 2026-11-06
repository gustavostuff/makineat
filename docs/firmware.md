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

Games call `sound(index)` with a catalog index 0 through 255. Index 0 is silent. Sound definitions live in `tools/fw/synth.c`. The cartridge transmits index bytes across the EVENT line as eight low pulses (100 us or 200 us wide). The system audio AVR samples EVENT at 40 kHz, decodes the index, and mixes square wave, triangle wave, and pseudo-random noise channels at 20 kHz into the 10-bit DAC output on PD6. A global mix scale defaults to 20% (`SYNTH_VOLUME_DEFAULT`); see [design.md](design.md) for planned volume hardware.

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

### Stage (`stage.c`)

Wide or tall platform levels live outside the 16×16 `map`. Import art with `tools/fw/gen_stage.py`. A stage folder holds **`fg.png`** and optional **`bg.png`** (same height as **`fg.png`**). Pure RGB on **`fg.png`** only:

- **`(0,0,0)`** empty (**not** drawn, **not** solid)
- **`(255,255,255)`** exactly one spawn pixel (**not** drawn, **not** solid)
- **`(0,255,0)`** decorative ON (drawn on the foreground pass, **not** solid)
- **`(255,0,0)`** solid ON (drawn, solid for collision)
- **`(128,128,128)`** exactly **`#808080`**, a **one-row one-way platform**. Any width on that row. Stored in **`fg_semisolid`** and drawn. The player stands in the cell **above** the gray row, the same way they stand above red ground. A downward step into the gray cell is blocked. An upward step into it is not, so a jump from below passes through. Sideways moves do not collide with it. A tall player’s head ignores semisolid on the way down (`stage_h_blocks_vert_hard`) so the head does not catch the ledge before the feet do. Pickups still treat the cell as solid through **`stage_h_pickup_blocked`**.
- **`(255,255,0)`** coin block. Every yellow pixel belongs to exactly one **2×2** block (`STAGE_ITEM_KIND_COIN`). The block is solid and is not stored in `fg_lit` or `fg_solid`. Until it is used the block shines through four patterns. The first pattern lights every cell and holds for 1 second. Each of the other three holds for 0.1 second. The second pattern leaves the top-left cell off. The third pattern lights the top-left and bottom-right cells. The fourth pattern leaves the bottom-right cell off. Cells that are off in the current pattern stay off, including over a lit background LED. The block stays solid the whole time. A jump that tries to enter the block from below shifts that **2×2** up one cell for `STAGE_ITEM_BOUNCE_TICKS` frames, then it returns. That bump uses the container. `stage_h_item_bump` returns `STAGE_BUMP_COIN` for that first use and `STAGE_BUMP_NONE` after. Afterward the block stays fully lit, stays solid, and does not bounce or shine again. The stage does not draw a coin pickup sprite.
- **`(0,100,100)`** and **`(0,255,255)`** growth block. Every such pixel belongs to exactly one **2×2** (`STAGE_ITEM_KIND_GROWTH`). The block is solid and is not stored in `fg_lit` or `fg_solid`. Unused growth blocks use the same four-pattern shine schedule as coin blocks. The same bump animation applies. `stage_h_item_bump` returns `STAGE_BUMP_GROWTH` on the first use. When the bounce timer ends, a one-cell pickup appears one row above the block top edge if that cell is free. The pickup moves horizontally at **4** cells per second. It reverses when a horizontal step is blocked. Fall uses the same `plat_phys_fall_waits` curve as a player ledge drop. Pickups do not block the player for `stage_h_solid`. They block each other for movement probes through `stage_h_pickup_blocked`. Touching the pickup with the feet cell or the head cell consumes it and starts a growth cutscene on **super-led-bros**. During the cutscene `platform_player_growth_freeze` is true, `stage_h_items_tick` does not run, and coin pickups from `platform_player_tick` are not applied. The cutscene lasts **0.8** seconds on **60** Hz (**48** ticks). Every **0.1** second (**6** ticks) the drawn height alternates between **1** cell and **2** cells for **8** segments (**4** full cycles), then the player stays **2** cells tall with feet at **`py`** and head at **`py - 1`**. `stage_h_paint_ex` takes **`hide_player_h`** (**1** or **2**) so both body cells get the decor dimming pass when tall.

On **`bg.png`**, any pixel that is **not** black is drawn in a background pass. The background has **no** collision. Empty foreground cells show that background. A foreground LED that is on covers it. A foreground LED that is off because its art says so, such as a dark cell of an item-block shine pattern, stays off and hides the background behind that cell.

When **`bg.png`** is present, its width may differ from **`fg.png`**. Each layer can scroll by its width minus **16**. The background camera follows **`bg_camera = fg_camera * (bg_width - 16) / (fg_width - 16)`** with integer division, then stays inside that background span. Both cameras are **0** at the left edge and both sit on their last legal column at the right edge. A **64** wide background behind a **256** wide foreground therefore keeps moving until the foreground camera reaches **240**.

`StageMap` stores **`fg_lit`**, **`fg_solid`**, and **`fg_semisolid`**. **`stage_map_get`** / **`stage_map_lit`** return the lit mask (decor, red solid, semisolid, and item art as lit). **`stage_map_solid`** returns the red solid mask only; **`stage_map_semisolid`** returns the gray one-row mask. **`stage_h_solid`** is red solid, semisolid, or any item **2×2** (full collision for pickups and generic probes). **`stage_h_blocks_horiz`** / **`stage_h_blocks_vert`** implement platformer body rules for semisolid. Coin blocks contribute to **`stage_item_covers`**. Growth blocks use **`stage_growth_covers`**. Block origins and **`kind`** flags live in **`items`**.

- **Horizontal (implemented):** `stage_bind_h`, `stage_h_scroll_tick` (held stick at **30 FPS**), `stage_h_paint`, `stage_h_solid`, `stage_h_camera_follow_dead_zone`, `stage_h_bg_camera_get`. Defaults are `STAGE_H_DEAD_ZONE_X` **5** and `STAGE_H_DEAD_ZONE_W` **6** in `stage.h`, so the dead zone is screen columns **5** through **10** inclusive with height **FIELD_SIZE** rows. `stage_h_camera_set` clips the foreground camera and refreshes the background camera so each **FIELD_SIZE** window stays inside its layer bounds.
- **Vertical (stub):** `stage_bind_v` returns `STAGE_ERR_UNSUPPORTED`. The `stage_v_*` symbols are placeholders.

### Platform (`platform.c`)

The player stands on one or two playfield rows over a horizontal `StageView`. Entry points are `platform_player_bind`, `platform_player_tick`, and `platform_h_paint`. `platform_player_tick` returns how many fresh **coin** blocks were used on that tick (growth blocks do not add to that count). While grown and not crouching, horizontal and vertical moves test both body cells against **`stage_h_blocks_horiz`** / **`stage_h_blocks_vert`** (semisolid-aware). Holding **down** while grown crouches to **1** cell (feet stay at **`py`**). A small player ignores down. Releasing down stands back up when the head cell is not hard terrain; a low red ceiling or item block keeps the crouch until that cell is clear. The camera follows the player with `stage_h_camera_follow_dead_zone` so the player world column stays inside the default dead zone unless the camera is clipped at the stage edge. A body cell above the stage is above the screen and is open air: the player keeps jumping, falling, and walking there and is not drawn until a cell comes back on screen. The left and right edges of the stage still block. A solid cell on the top row still blocks. The bottom edge is not a floor. With no solid, semisolid, or item cell under the feet, the player falls through and is not drawn below the stage.

Shared fall stepping for the player and for pickups lives in **`plat_physics.c`** (`plat_phys_fall_step`, `plat_phys_move_vertical`, `plat_phys_horiz_bounce`).

- **Walk:** horizontal motion uses **15** playfield cell steps per second (15 FPS). `platform_player_tick` accumulates against `STAGE_TICK_HZ` (60 Hz). The pattern matches how `stage_h_scroll_tick` uses `STAGE_SCROLL_FPS` for camera scroll, but the player rate is 15 cells per second. Walk has no acceleration.
- **Jump (scripted):** a debounced **button** press while grounded starts a jump. Motion is **not** velocity-based. It follows a fixed schedule on **60 Hz** game ticks (same tick interval as **Game loop and display refresh**). Playfield rows count as **positions 1 through 6** from the ground row before the first step. Position 1 is that ground row. **Rise** counts how many cells move upward from position 1. A full jump has rise **5** and ends at position 6 apex. If the **button** is not held while rise is still below **2** cells, ascent keeps going until rise reaches **2** (minimum jump height is **3** rows counting position 1). After that, releasing the **button** ends ascent and starts descent. The same mirrored waits apply on the way down from the height reached on the way up.
  - **1 to 2:** one cell up on the same tick as the jump input.
  - **At position 2:** **1** idle tick, then step to position 3.
  - **At 3:** **2** idle ticks, then step to 4.
  - **At 4:** **3** idle ticks, then step to 5.
  - **At 5:** **4** idle ticks, then step to 6 apex on a full jump.
  - **Descent** always uses the same mirrored wait list as a full jump, but it **starts at the step that matches apex height**. The list of waits between downward steps stays **4, 3, 2, 1, 1** ticks. The fastest wait is **1** idle tick, one cell every two ticks (**30** cells per second). If that descent ends in the air, later cells keep that fastest wait.
- **Fall (not scripted):** a ledge drop uses the jump descent waits in `plat_phys_fall_waits`: **4**, **3**, **2**, **1**, then **1** idle tick between downward cells. That last wait repeats until the body lands, so the fall never exceeds one cell every two ticks. The tick that leaves the ground only arms the clock. Growth pickups use the same waits through `plat_phys_move_vertical`. A jump that finishes its descent still in the air continues at that fastest wait instead of starting over.

Stage art for **super-led-bros** uses folders `games/super-led-bros/img/<name>/` with **`fg.png`** and optional **`bg.png`**. The script `games/super-led-bros/regen_stages.sh` calls `tools/fw/gen_stage.py` on each folder and writes **`games/super-led-bros/<name>.c`** and **`.h`** (for example **`stage_01`** from **`img/stage_01/`**).

**super-led-bros** writes the status digits from left to right as four score digits, two coin digits, and two life digits. Score stays at 0. Lives start at **4** and count down through **0**. Each fresh **coin** block adds one coin while the growth cutscene is not running. The coin count wraps from 99 to 0. Falling through the bottom leaves the stage on screen for **3** seconds (180 ticks at 60 Hz) so a death sting can play later. The life digits stay put during that wait. When a life remains, the stage then restarts at spawn and the counter drops by one. Coins are kept. Losing the life shown as **0** ends that same wait on **game over** in the large font, with the default one-pixel character gap and **4** pixels of padding before the first letter and after the last. The line scrolls and stays up for **3** seconds (180 ticks). After that, the action button starts a new game with 4 lives and 0 coins.

Link `../stage.c`, `../plat_physics.c`, `../platform.c`, and generated stage sources in `tools/fw/avr/Makefile` for platform carts.

## Carts

Each cart implements `void game_tick(const Input *in)`. Carts recognized in `cart_placed.json` include `space-shooter`, `pong`, `arkaled`, `text-demo`, `snake`, and `super-led-bros`.
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
