#!/usr/bin/env python3
"""Emit StageMap C sources from foreground and optional background PNGs.

Pure RGB rules on the foreground image (fg.png):
  (0,0,0)        empty
  (255,255,255)  player spawn (exactly one pixel, not drawn, not solid)
  (0,255,0)      decorative ON (drawn, not solid)
  (255,0,0)      solid ON (drawn, collidable)
  (255,255,0)    item container (exactly a 2x2, solid, drawn by the runtime)
  (0,255,255)    growth mark (not drawn, not solid)
  (0,100,100)    growth mark (not drawn, not solid)

Background (bg.png): any non-black pixel is drawn for parallax only (no collision).
Width may differ from fg. Height must match fg. Runtime parallax maps scroll
spans so both layers finish together:
  bg_camera = fg_camera * (bg_width - 16) // (fg_width - 16)

Usage:
  python3 gen_stage.py <stage_dir> <output_basename>
    Expects stage_dir/fg.png and optional stage_dir/bg.png
  python3 gen_stage.py <fg.png> <output_basename>
    Single PNG, no background layer
"""

from __future__ import annotations

import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError as exc:
    raise SystemExit("Pillow required: pip install pillow") from exc

EMPTY = (0, 0, 0)
SPAWN = (255, 255, 255)
DECOR = (0, 255, 0)
SOLID = (255, 0, 0)
ITEM = (255, 255, 0)
# Reserved for a later growth pickup. Ignored so a block in the art does not break import.
GROWTH = ((0, 255, 255), (0, 100, 100))
ITEM_MAX = 32  # STAGE_ITEM_MAX in tools/fw/stage.h


def classify_rgb(r: int, g: int, b: int) -> str:
    t = (r, g, b)
    if t == EMPTY:
        return "empty"
    if t == SPAWN:
        return "spawn"
    if t == DECOR:
        return "decor"
    if t == SOLID:
        return "solid"
    if t == ITEM:
        return "item"
    if t in GROWTH:
        return "growth"
    raise SystemExit(
        f"unknown pure color RGB({r},{g},{b}); "
        "allowed: black, white, green, red, yellow, cyan"
    )


def image_to_bitplanes(path: Path, h_expect: int | None, what: str):
    im = Image.open(path).convert("RGB")
    w, h = im.size
    if h_expect is not None and h != h_expect:
        raise SystemExit(f"{path.name}: height {h} != foreground height {h_expect}")
    px = im.load()
    row_words = (w + 15) // 16
    lit_rows: list[int] = []
    solid_rows: list[int] = []
    spawns: list[tuple[int, int]] = []
    yellow = [[False] * w for _ in range(h)]
    for y in range(h):
        lit_words = [0] * row_words
        solid_words = [0] * row_words
        for x in range(w):
            r, g, b = px[x, y]
            kind = classify_rgb(r, g, b)
            wi = x // 16
            bit = x % 16
            if kind == "empty":
                continue
            if kind == "spawn":
                spawns.append((x, y))
                continue
            if kind == "item":
                yellow[y][x] = True
                continue
            if kind == "growth":
                continue
            if kind == "decor":
                lit_words[wi] |= 1 << bit
            elif kind == "solid":
                lit_words[wi] |= 1 << bit
                solid_words[wi] |= 1 << bit
        lit_rows.extend(lit_words)
        solid_rows.extend(solid_words)
    if what == "foreground" and len(spawns) != 1:
        raise SystemExit(
            f"{path.name}: expected exactly one pure white spawn pixel, found {len(spawns)}"
        )
    spawn_x, spawn_y = spawns[0]
    items = item_blocks(path, yellow)
    return w, h, row_words, lit_rows, solid_rows, spawn_x, spawn_y, items


def item_blocks(path: Path, yellow: list[list[bool]]) -> list[tuple[int, int]]:
    """Each yellow pixel must belong to exactly one 2x2, scanned from the top left."""
    h = len(yellow)
    w = len(yellow[0]) if h else 0
    used = [[False] * w for _ in range(h)]
    blocks: list[tuple[int, int]] = []
    for y in range(h):
        for x in range(w):
            if not yellow[y][x] or used[y][x]:
                continue
            cells = ((x, y), (x + 1, y), (x, y + 1), (x + 1, y + 1))
            for cx, cy in cells:
                if cx >= w or cy >= h or not yellow[cy][cx] or used[cy][cx]:
                    raise SystemExit(f"{path.name}: yellow at ({x},{y}) is not a lone 2x2 item block")
            for cx, cy in cells:
                used[cy][cx] = True
            blocks.append((x, y))
    if len(blocks) > ITEM_MAX:
        raise SystemExit(f"{path.name}: {len(blocks)} item blocks exceeds {ITEM_MAX}")
    return blocks


def bg_to_lit(path: Path, h_expect: int) -> tuple[int, int, list[int]]:
    im = Image.open(path).convert("RGB")
    w, h = im.size
    if h != h_expect:
        raise SystemExit(f"{path.name}: height {h} != foreground height {h_expect}")
    px = im.load()
    row_words = (w + 15) // 16
    rows: list[int] = []
    for y in range(h):
        words = [0] * row_words
        for x in range(w):
            r, g, b = px[x, y]
            if r == 0 and g == 0 and b == 0:
                continue
            wi = x // 16
            bit = x % 16
            words[wi] |= 1 << bit
        rows.extend(words)
    return w, row_words, rows


def emit(
    stem: str,
    fg_png: Path,
    bg_png: Path | None,
    w: int,
    h: int,
    row_words: int,
    fg_lit: list[int],
    fg_solid: list[int],
    spawn_x: int,
    spawn_y: int,
    bg_w: int,
    bg_row_words: int,
    bg_lit: list[int] | None,
    items: list[tuple[int, int]],
) -> str:
    sym = stem.upper().replace("-", "_")
    head = f"/* Generated by tools/fw/gen_stage.py from {fg_png.name}"
    if bg_png is not None:
        head += f", {bg_png.name}"
    head += " */"
    lines = [
        head,
        "",
        '#include "stage.h"',
        "",
        f"#define {sym}_W {w}",
        f"#define {sym}_H {h}",
        f"#define {sym}_ROW_WORDS {row_words}",
        f"#define {sym}_SPAWN_X {spawn_x}",
        f"#define {sym}_SPAWN_Y {spawn_y}",
        f"#define {sym}_BG_W {bg_w}",
        f"#define {sym}_BG_ROW_WORDS {bg_row_words}",
        "",
        f"static const uint16_t {stem}_fg_lit[{h} * {sym}_ROW_WORDS] = {{",
    ]
    for y in range(h):
        chunk = fg_lit[y * row_words : (y + 1) * row_words]
        lines.append("    " + ", ".join(f"0x{v:04x}" for v in chunk) + ",")
    lines.extend(["};", "", f"static const uint16_t {stem}_fg_solid[{h} * {sym}_ROW_WORDS] = {{"])
    for y in range(h):
        chunk = fg_solid[y * row_words : (y + 1) * row_words]
        lines.append("    " + ", ".join(f"0x{v:04x}" for v in chunk) + ",")
    lines.append("};")

    if bg_lit is not None:
        lines.extend(["", f"static const uint16_t {stem}_bg_lit[{h} * {sym}_BG_ROW_WORDS] = {{"])
        for y in range(h):
            chunk = bg_lit[y * bg_row_words : (y + 1) * bg_row_words]
            lines.append("    " + ", ".join(f"0x{v:04x}" for v in chunk) + ",")
        lines.append("};")

    bg_ptr = f"{stem}_bg_lit" if bg_lit is not None else "0"
    if items:
        lines.extend(["", f"static const StageItem {stem}_items[{len(items)}] = {{"])
        for x, y in items:
            lines.append(f"    {{ {x}, {y} }},")
        lines.append("};")
        item_count = str(len(items))
        item_ptr = f"{stem}_items"
    else:
        item_count = "0"
        item_ptr = "0"
    lines.extend(
        [
            "",
            f"const StageMap {stem} = {{",
            f"    {sym}_W,",
            f"    {sym}_H,",
            f"    {sym}_ROW_WORDS,",
            f"    {sym}_SPAWN_X,",
            f"    {sym}_SPAWN_Y,",
            f"    {stem}_fg_lit,",
            f"    {stem}_fg_solid,",
            f"    {sym}_BG_W,",
            f"    {sym}_BG_ROW_WORDS,",
            f"    {bg_ptr},",
            f"    {item_count},",
            f"    {item_ptr},",
            "};",
            "",
        ]
    )
    return "\n".join(lines)


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit(
            "usage: gen_stage.py <stage_dir|fg.png> <output_basename>\n"
            "  Directory must contain fg.png and may contain bg.png"
        )
    inp = Path(sys.argv[1]).resolve()
    out = Path(sys.argv[2]).resolve()
    stem = out.name

    if inp.is_dir():
        fg_path = inp / "fg.png"
        bg_path = inp / "bg.png"
        if not fg_path.is_file():
            raise SystemExit(f"missing {fg_path}")
        w, h, row_words, fg_lit, fg_solid, spawn_x, spawn_y, items = image_to_bitplanes(
            fg_path, None, "foreground"
        )
        if w > 65535 or h > 65535 or row_words > 65535 or spawn_x > 65535 or spawn_y > 65535:
            raise SystemExit(f"{fg_path}: width/height/spawn too large for uint16_t StageMap fields")
        bg_png: Path | None = None
        bg_lit = None
        bg_w = 0
        bg_row_words = 0
        if bg_path.is_file():
            bg_png = bg_path
            bg_w, bg_row_words, bg_lit = bg_to_lit(bg_path, h)
            if bg_w > 65535 or bg_row_words > 65535:
                raise SystemExit(f"{bg_path}: bg width too large for uint16_t StageMap fields")
    else:
        if inp.suffix.lower() != ".png":
            raise SystemExit("single file mode expects .png")
        fg_path = inp
        bg_png = None
        w, h, row_words, fg_lit, fg_solid, spawn_x, spawn_y, items = image_to_bitplanes(
            fg_path, None, "foreground"
        )
        if w > 65535 or h > 65535 or row_words > 65535 or spawn_x > 65535 or spawn_y > 65535:
            raise SystemExit(f"{fg_path}: width/height/spawn too large for uint16_t StageMap fields")
        bg_w = 0
        bg_row_words = 0
        bg_lit = None

    body = emit(
        stem,
        fg_path,
        bg_png,
        w,
        h,
        row_words,
        fg_lit,
        fg_solid,
        spawn_x,
        spawn_y,
        bg_w,
        bg_row_words,
        bg_lit,
        items,
    )
    out.with_suffix(".c").write_text(body)
    guard = f"STAGE_{stem.upper().replace('-', '_')}_H"
    header = f"""#ifndef {guard}
#define {guard}

#include "stage.h"

extern const StageMap {stem};

#endif
"""
    out.with_suffix(".h").write_text(header)
    print("wrote", out.with_suffix(".c"), "and", out.with_suffix(".h"))


if __name__ == "__main__":
    main()
