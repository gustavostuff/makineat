#!/usr/bin/env bash
# Regenerate StageMap .c/.h from img/<name>/fg.png (optional bg.png).
# Rules: tools/fw/gen_stage.py (pure colors on fg, see docs/firmware.md).
set -euo pipefail

game_dir=$(cd "$(dirname "$0")" && pwd)
repo_root=$(cd "$game_dir/../.." && pwd)
gen="$repo_root/tools/fw/gen_stage.py"
img_root="$game_dir/img"

if [[ ! -f "$gen" ]]; then
    echo "regen_stages: missing $gen" >&2
    exit 1
fi

count=0
for d in "$img_root"/*/; do
    [[ -d "$d" ]] || continue
    if [[ ! -f "${d}fg.png" ]]; then
        continue
    fi
    base=$(basename "$d")
    out="$game_dir/$base"
    echo "regen_stages: $d -> $out.{c,h}"
    python3 "$gen" "$d" "$out"
    count=$((count + 1))
done

if ((count == 0)); then
    echo "regen_stages: no img/*/fg.png under $img_root" >&2
    exit 1
fi

echo "regen_stages: done ($count stage(s))"
