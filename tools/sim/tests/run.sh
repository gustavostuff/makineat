#!/bin/sh
# Run each test binary. On a terminal, one line counts individual checks and
# is rewritten as they pass. A failing suite prints its own report.
if [ "$#" -eq 0 ]; then
    echo "no tests" >&2
    exit 1
fi

tty=0
if [ -t 1 ]; then
    tty=1
    export REPORT_COLOR=1
fi

if [ "$tty" -eq 1 ]; then
    total=0
    for bin in "$@"; do
        counted=$(REPORT_COUNT=1 "$bin" 2>&1)
        status=$?
        if [ "$status" -ne 0 ]; then
            printf '%s\n' "$counted"
            exit "$status"
        fi
        total=$((total + counted))
    done
    n=0
    for bin in "$@"; do
        err=$(mktemp)
        added=$(REPORT_PROGRESS=1 REPORT_BASE="$n" REPORT_TOTAL="$total" "$bin" 2>"$err")
        status=$?
        if [ "$status" -ne 0 ]; then
            printf '\n'
            cat "$err"
            rm -f "$err"
            exit "$status"
        fi
        if [ -s "$err" ]; then
            printf '\n'
            cat "$err"
        fi
        rm -f "$err"
        if [ -z "$added" ]; then
            echo "missing check count from $bin" >&2
            exit 1
        fi
        n=$((n + added))
    done
    printf '\r\033[K\033[32mtests %d/%d ok\033[0m\n' "$n" "$total"
    exit 0
fi

for bin in "$@"; do
    out=$("$bin" 2>&1)
    status=$?
    if [ "$status" -ne 0 ]; then
        printf '%s\n' "$out"
        exit "$status"
    fi
    printf '%s\n' "$out"
done
