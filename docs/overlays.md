# Screen overlays and game packaging

The display field is 16x16 and 1-bit, augmented by eight 7-segment status digits. Color and visual tone come from a physical translucent sheet placed in front of white LEDs, drawing direct inspiration from early tabletop games and the Magnavox Odyssey.

## The physical game kit

Each game is conceived as a complete, tactile physical release rather than just a software binary. Opening a game box reveals a self-contained kit:

- A cartridge in its custom plastic shell, containing the game AVR and ROM.
- A framed full-front overlay assembly covering both the 16x16 matrix playfield and the eight status digits, complete with frame labels.
- A snap-on magnetic marquee panel featuring game title artwork.
- A printed mini-manual illustrating controls, scoring, and lore.

Preparing to play involves an enjoyable physical ritual. Sliding the cartridge into the edge slot, snapping the matching magnetic marquee onto the cabinet top, and snapping the overlay into the faceplate turns the tabletop system into a dedicated arcade cabinet for that specific title.

## Visual mechanics

Both the playfield matrix LEDs and the 7-segment status digits emit white light. A single overlay assembly covers the entire front visual area, filtering the white emitters into custom color zones:

- The 16x16 playfield section can be colored in multiple ways (such as green playing areas, amber ground zones, blue skies) or solid tinting across the board.
- The lower section covers the eight 7-segment displays, dividing them into colored digits, it can be a large 8 digit score number, two 4-digit scores (player 1 and 2), 4 pairs of counters for items, lives, etc. Anything the game needs.

Because all color filtering happens physically, game firmware remains fast, light, and 1-bit. Sprites and digits change perceived color solely based on the section of the sheet they illuminate.

## Materials and fabrication

The practical construction of the full-front overlay uses a layered approach:

* **Carrier Sheet**: A base sheet of optically clear, fully transparent acetate provides structural backing.
* **Color Filters**: Custom cut-out shapes of colored lighting gel or tinted acetate film are affixed onto the clear carrier to define color zones for both the playfield and the individual digit pairs.
* **Protective Outer Frame**: A single rigid plastic frame holds the gel and acetate sandwich securely, allowing easy insertion into the cabinet faceplate.
* **Frame Labels**: Adhesive vinyl stickers or printed decals on the plastic frame below the digits provide crisp, high-contrast labels (such as Lives, Score, Items, etc.) tailored to each cartridge.
