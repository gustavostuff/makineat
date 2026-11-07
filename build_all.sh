#!/bin/sh
set -eu
cd "$(dirname "$0")"
make -C tools/sim
if command -v avr-gcc >/dev/null 2>&1; then
    make -C tools/fw/avr
else
    echo "avr-gcc not found, skipping tools/fw/avr/square.hex"
fi
