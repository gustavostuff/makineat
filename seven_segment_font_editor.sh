#!/bin/sh
set -eu
cd "$(dirname "$0")"
make -C tools/seven-seg-font
exec ./tools/seven-seg-font/seven-seg-font
