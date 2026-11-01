#!/usr/bin/env python3
"""Convert EDAC 395-020 socket STEP to WRL with silver pins and dark black body.

Default input is EDAC_395-020-520-202.STEP (straight). For J3 use 395-020-558-212
when a 558-212 STEP is available. Pad footprint is unchanged between variants.

Body diffuse: 30/255 = 0.1176 (RGB 30, 30, 30)
Pins diffuse: silver (0.78, 0.78, 0.80), shiny specular (0.85, 0.85, 0.88)
Coordinates are kept in mm (1:1 with STEP and current WRL).
"""

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


def main():
    import cascadio

    main_pcb = Path(__file__).resolve().parents[1]
    step_path = (main_pcb / "library/MakiNeat.3dshapes/EDAC_395-020-520-202.STEP").resolve()
    wrl_path = (main_pcb / "library/MakiNeat.3dshapes/EDAC_395-020-520-202.wrl").resolve()

    with tempfile.TemporaryDirectory() as td:
        obj_path = Path(td) / "model.obj"
        rc = cascadio.step_to_obj(str(step_path), str(obj_path), use_colors=False)
        if rc != 0:
            print(f"step_to_obj failed with {rc}", file=sys.stderr)
            sys.exit(1)
        verts, groups = parse_obj(obj_path)

    body_tris = []
    pins_tris = []
    for name, tris in groups.items():
        if "345-292" in name or "345-293" in name:
            pins_tris.extend(tris)
        else:
            body_tris.extend(tris)

    print(f"Parsed {len(verts)} verts, {len(body_tris)} body tris, {len(pins_tris)} pins tris")

    out = ["#VRML V2.0 utf8", "", "Group {", "  children ["]
    out.extend(build_shape(verts, body_tris, BODY_DIFFUSE, BODY_SPECULAR, BODY_SHININESS))
    out.extend(build_shape(verts, pins_tris, PINS_DIFFUSE, PINS_SPECULAR, PINS_SHININESS))
    out.append("  ]")
    out.append("}")

    wrl_path.write_text("\n".join(out) + "\n")
    print(f"Successfully wrote {wrl_path} ({wrl_path.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
