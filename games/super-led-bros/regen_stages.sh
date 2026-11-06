#!/usr/bin/env bash
# Regenerate StageMap .c/.h from every PNG in img/.
# Rules (tools/fw/gen_stage.py): black = empty, pure red = spawn, else solid.
set -euo pipefail

game_dir=$(cd "$(dirname "$0")" && pwd)
repo_root=$(cd "$game_dir/../.." && pwd)
gen="$repo_root/tools/fw/gen_stage.py"
img_dir="$game_dir/img"

if [[ ! -f "$gen" ]]; then
    echo "regen_stages: missing $gen" >&2
    exit 1
fi

shopt -s nullglob
pngs=("$img_dir"/*.png)
if ((${#pngs[@]} == 0)); then
    echo "regen_stages: no PNGs in $img_dir" >&2
    exit 1
fi

for png in "${pngs[@]}"; do
    base=$(basename "$png" .png)
    out="$game_dir/$base"
    echo "regen_stages: $png -> $out.{c,h}"
    python3 "$gen" "$png" "$out"
done

echo "regen_stages: done (${#pngs[@]} stage(s))"
