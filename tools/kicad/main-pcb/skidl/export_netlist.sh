#!/usr/bin/env bash
# Write the console netlist from the SKiDL circuit and update net classes.
set -euo pipefail

DIR="$(cd "$(dirname "$0")" && pwd)"
NET="$DIR/makineat.net"

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    echo "Usage: $0 [PROJECT.kicad_pro]"
    echo "  Writes $NET and updates net classes in main-board *.kicad_pro files"
    echo "  The cartridge netlist is tools/kicad/cart-pcb/skidl/export_netlist.sh"
    exit 0
fi

if ! command -v python3 >/dev/null 2>&1; then
    echo "python3 not found" >&2
    exit 1
fi
if ! python3 -c "import importlib.util, sys; sys.exit(0 if importlib.util.find_spec('skidl') else 1)"; then
    echo "skidl not installed" >&2
    exit 1
fi

python3 "$DIR/../scripts/build_matrix8x8_wrl.py"
python3 "$DIR/../scripts/compose_matrix_footprint.py"
python3 "$DIR/makineat.py" "$@"
