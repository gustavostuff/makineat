#!/usr/bin/env python3
"""Build NFM-12881xx-style 8x8 matrix WRL for KiCad.

Geometry is defined in millimeters, then scaled to KiCad legacy VRML units
(0.1 inch per coordinate unit) so the 3D viewer shows true mm sizes.
"""

from __future__ import annotations

import math
from pathlib import Path

LIB_WRL = (
    Path(__file__).resolve().parents[1]
    / "library/MakiNeat.3dshapes/NFM-12881xx.wrl"
)

BODY = (12 / 255, 12 / 255, 12 / 255)  # face the LEDs sit in
SIDE = (1.0, 1.0, 1.0)  # faces that do not show LEDs
LED = (1.0, 0.96, 0.62)  # pale yellow

SIZE = 32.0
BODY_H = 5.5
LED_H = 3.0
LED_PROUD = 0.2  # mm above body top; rest of cylinder is inside the block
LED_R = 1.5  # 3 mm dia per NFM-12881xx drawing
PITCH = 4.0
GRID_SPAN = 28.0
ORIGIN = (SIZE - GRID_SPAN) / 2  # 2 mm

# Empirical alignment for Matrix8x8 footprint (F.Fab 0..32 mm, sockets at y=7).
# Same as KiCad model offset (0, -32, 8.5) mm with zero offset on the footprint.
FOOTPRINT_OFFSET_MM = (0.0, -32.0, 8.5)

# KiCad multiplies VRML coords by 2.54 unless a top-level scale is present.
KICAD_WRL_PER_MM = 1.0 / 2.54


def wrl(v: float) -> float:
    return v * KICAD_WRL_PER_MM


def shift_pts(
    pts: list[tuple[float, float, float]],
    dx: float,
    dy: float,
    dz: float,
) -> list[tuple[float, float, float]]:
    return [(x + dx, y + dy, z + dz) for x, y, z in pts]


def box_faces(x0: float, y0: float, z0: float, x1: float, y1: float, z1: float) -> tuple[list, list]:
    pts = [
        (x0, y0, z0),
        (x1, y0, z0),
        (x1, y1, z0),
        (x0, y1, z0),
        (x0, y0, z1),
        (x1, y0, z1),
        (x1, y1, z1),
        (x0, y1, z1),
    ]
    # CCW faces with outward normals (Z-up, +X right, +Y down-board in footprint space).
    faces = [
        (0, 3, 2, 1),  # bottom, -Z
        (4, 5, 6, 7),  # top, +Z
        (0, 1, 5, 4),  # y0 side, -Y
        (1, 2, 6, 5),  # x1 side, +X
        (2, 3, 7, 6),  # y1 side, +Y
        (3, 0, 4, 7),  # x0 side, -X
    ]
    return pts, faces


def cylinder_z(cx: float, cy: float, z0: float, z1: float, radius: float, segments: int = 24) -> tuple[list, list]:
    """Frustum-less cylinder along Z from z0 (bottom) to z1 (top)."""
    pts: list[tuple[float, float, float]] = []
    for i in range(segments):
        a = 2 * math.pi * i / segments
        pts.append((cx + radius * math.cos(a), cy + radius * math.sin(a), z0))
    n0 = len(pts)
    for i in range(segments):
        a = 2 * math.pi * i / segments
        pts.append((cx + radius * math.cos(a), cy + radius * math.sin(a), z1))
    center_bot = len(pts)
    pts.append((cx, cy, z0))
    center_top = len(pts)
    pts.append((cx, cy, z1))
    faces: list[tuple[int, ...]] = []
    for i in range(segments):
        j = (i + 1) % segments
        faces.append((center_bot, j, i))
        faces.append((center_top, n0 + i, n0 + j))
        faces.append((i, j, n0 + j, n0 + i))
    return pts, faces


def merge(
    parts: list[tuple[list, list, tuple[float, float, float]]],
) -> tuple[list, list, list[tuple[float, float, float]]]:
    all_pts: list[tuple[float, float, float]] = []
    all_faces: list[tuple[int, ...]] = []
    colors: list[tuple[float, float, float]] = []
    for pts, faces, color in parts:
        base = len(all_pts)
        all_pts.extend(pts)
        for f in faces:
            all_faces.append(tuple(base + i for i in f))
        colors.append(color)
    return all_pts, all_faces, colors


def shape_block(pts, faces, color) -> str:
    coord = ",\n".join(f"{wrl(p[0]):.6f} {wrl(p[1]):.6f} {wrl(p[2]):.6f}" for p in pts)
    idx = ",\n".join(" ".join(str(i) for i in f) + " -1" for f in faces)
    r, g, b = color
    return f"""Shape {{
appearance Appearance {{
material Material {{
ambientIntensity 0.25
diffuseColor {r:.4f} {g:.4f} {b:.4f}
specularColor 0.2 0.2 0.2
shininess 0.15
}}
}}
geometry IndexedFaceSet {{
solid TRUE
coord Coordinate {{
point [
{coord}
]
}}
coordIndex [
{idx}
]
}}
}}"""


def main() -> None:
    ox, oy, oz = FOOTPRINT_OFFSET_MM
    parts: list[tuple[list, list, tuple[float, float, float]]] = []
    body_pts, body_faces = box_faces(0, 0, 0, SIZE, SIZE, BODY_H)
    parts.append((body_pts, [body_faces[1]], BODY))
    parts.append((body_pts, [body_faces[0], *body_faces[2:]], SIDE))

    z_top = BODY_H + LED_PROUD
    z_bot = z_top - LED_H
    for row in range(8):
        for col in range(8):
            cx = ORIGIN + col * PITCH
            cy = ORIGIN + row * PITCH
            parts.append((*cylinder_z(cx, cy, z_bot, z_top, LED_R), LED))

    parts = [
        (shift_pts(pts, ox, oy, oz), faces, color) for pts, faces, color in parts
    ]

    # One mesh per color group for KiCad material clarity
    by_color: dict[tuple[float, float, float], list] = {}
    for pts, faces, color in parts:
        by_color.setdefault(color, ([], []))
        base = len(by_color[color][0])
        by_color[color][0].extend(pts)
        for f in faces:
            by_color[color][1].append(tuple(base + i for i in f))

    children = [shape_block(pts, faces, color) for color, (pts, faces) in by_color.items()]
    body = "\n".join(children)

    wrl = f"""#VRML V2.0 utf8
# NFM-12881xx 8x8 LED matrix (body + LED dots, no pins)
# Coords in KiCad legacy VRML units (mm/2.54). Aligned to Matrix8x8 F.Fab; +Z off PCB.

Group {{
children [
{body}
]
}}
"""
    LIB_WRL.parent.mkdir(parents=True, exist_ok=True)
    LIB_WRL.write_text(wrl, encoding="utf-8")
    print(f"Wrote {LIB_WRL} ({LIB_WRL.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
