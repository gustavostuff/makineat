#!/usr/bin/env python3
"""Convert EDAC 395-020 socket STEP to WRL with silver pins and dark black body.

Uses the cascadio package (OpenCASCADE) for STEP to mesh conversion:

    pip install --break-system-packages cascadio

Body diffuse: 30/255 (RGB 30, 30, 30)
Pins diffuse: silver (0.78, 0.78, 0.80), shiny specular (0.85, 0.85, 0.88)
Coordinates are kept in mm (1:1 with STEP and KiCad WRL).

Examples:

    # J3 straight socket (520-202 STEP; order 395-020-520-802):
    python3 tools/kicad/main-pcb/scripts/convert_edac_20pin.py

    # Legacy right-angle 558-212 (not used on main PCB):
    python3 tools/kicad/main-pcb/scripts/convert_edac_20pin.py \\
        --step "tools/kicad/main-pcb/library/MakiNeat.3dshapes/395-020-558-212 - EDAC Card Edge Connector.STEP" \\
        --wrl tools/kicad/main-pcb/library/MakiNeat.3dshapes/EDAC_395-020-558-212.wrl
"""

from __future__ import annotations

import argparse
import sys
import tempfile
from pathlib import Path

# Colors
BODY_DIFFUSE = (30 / 255.0, 30 / 255.0, 30 / 255.0)
BODY_SPECULAR = (0.15, 0.15, 0.15)
BODY_SHININESS = 0.12

PINS_DIFFUSE = (0.78, 0.78, 0.80)
PINS_SPECULAR = (0.85, 0.85, 0.88)
PINS_SHININESS = 0.75

PIN_MARKERS = ("345-292", "345-293")


def parse_obj(obj_path: Path):
    verts = []
    groups = {}
    current = "__default__"
    with obj_path.open() as f:
        for line in f:
            if line.startswith("v "):
                p = line.split()
                verts.append((float(p[1]), float(p[2]), float(p[3])))
            elif line.startswith("g "):
                current = line[2:].strip()
                groups.setdefault(current, [])
            elif line.startswith("f "):
                idx = [int(t.split("/")[0]) - 1 for t in line.split()[1:]]
                groups.setdefault(current, [])
                for j in range(1, len(idx) - 1):
                    groups[current].append((idx[0], idx[j], idx[j + 1]))
    return verts, groups


def build_shape(verts_mm, tris, diffuse, specular, shininess):
    if not tris:
        return []
    used = {}
    points = []
    for a, b, c in tris:
        for i in (a, b, c):
            if i not in used:
                used[i] = len(points)
                points.append(verts_mm[i])
    lines = [
        "Shape {",
        "  appearance Appearance {",
        "    material Material {",
        "      ambientIntensity 0.2",
        f"      diffuseColor {diffuse[0]:.4f} {diffuse[1]:.4f} {diffuse[2]:.4f}",
        f"      specularColor {specular[0]:.4f} {specular[1]:.4f} {specular[2]:.4f}",
        f"      shininess {shininess:.2f}",
        "    }",
        "  }",
        "  geometry IndexedFaceSet {",
        "    coord Coordinate {",
        "      point [",
    ]
    lines.extend(f"        {x:.6f} {y:.6f} {z:.6f}," for x, y, z in points)
    lines.append("      ]")
    lines.append("    }")
    lines.append("    coordIndex [")
    for a, b, c in tris:
        lines.append(f"      {used[a]}, {used[b]}, {used[c]}, -1,")
    lines.append("    ]")
    lines.append("    creaseAngle 0.5")
    lines.append("  }")
    lines.append("}")
    return lines


def step_to_wrl(step_path: Path, wrl_path: Path) -> None:
    import cascadio

    step_path = step_path.resolve()
    wrl_path = wrl_path.resolve()
    if not step_path.is_file():
        raise SystemExit(f"STEP not found: {step_path}")

    with tempfile.TemporaryDirectory() as td:
        obj_path = Path(td) / "model.obj"
        rc = cascadio.step_to_obj(str(step_path), str(obj_path), use_colors=False)
        if rc != 0:
            raise SystemExit(f"step_to_obj failed with {rc}")
        verts, groups = parse_obj(obj_path)

    body_tris = []
    pins_tris = []
    for name, tris in groups.items():
        if any(marker in name for marker in PIN_MARKERS):
            pins_tris.extend(tris)
        else:
            body_tris.extend(tris)

    print(f"Parsed {len(verts)} verts, {len(body_tris)} body tris, {len(pins_tris)} pin tris")

    out = ["#VRML V2.0 utf8", "", "Group {", "  children ["]
    out.extend(build_shape(verts, body_tris, BODY_DIFFUSE, BODY_SPECULAR, BODY_SHININESS))
    out.extend(build_shape(verts, pins_tris, PINS_DIFFUSE, PINS_SPECULAR, PINS_SHININESS))
    out.append("  ]")
    out.append("}")

    wrl_path.parent.mkdir(parents=True, exist_ok=True)
    wrl_path.write_text("\n".join(out) + "\n")
    print(f"Wrote {wrl_path} ({wrl_path.stat().st_size} bytes)")


def main() -> None:
    main_pcb = Path(__file__).resolve().parents[1]
    shapes = main_pcb / "library" / "MakiNeat.3dshapes"
    default_step = shapes / "EDAC_395-020-520-202.STEP"
    default_wrl = shapes / "EDAC_395-020-520-202.wrl"
    default_558_step = shapes / "395-020-558-212 - EDAC Card Edge Connector.STEP"
    default_558_wrl = shapes / "EDAC_395-020-558-212.wrl"

    parser = argparse.ArgumentParser(description="Convert EDAC 395-020 STEP to KiCad WRL")
    parser.add_argument(
        "--step",
        type=Path,
        default=None,
        help=f"Input STEP (default: {default_step.name}, or 558 file if present and --variant 558)",
    )
    parser.add_argument("--wrl", type=Path, default=None, help="Output WRL path")
    parser.add_argument(
        "--variant",
        choices=("520", "558", "auto"),
        default="auto",
        help="520=straight 520-202 (default), 558=right-angle 558-212, auto=pick 520 STEP if --step omitted",
    )
    args = parser.parse_args()

    if args.step is not None:
        step_path = args.step
        wrl_path = args.wrl or step_path.with_suffix(".wrl")
    elif args.variant == "558" or (args.variant == "auto" and default_558_step.is_file()):
        step_path = default_558_step
        wrl_path = args.wrl or default_558_wrl
    else:
        step_path = default_step
        wrl_path = args.wrl or default_wrl

    step_to_wrl(step_path, wrl_path)


if __name__ == "__main__":
    main()
