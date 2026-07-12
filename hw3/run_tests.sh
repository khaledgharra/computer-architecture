#!/usr/bin/env bash
# Run the full N&R + Segel test suite for hw3.
# Usage: ./run_tests.sh
# Must be run from the hw3/ directory.

set -e

HW3="$(cd "$(dirname "$0")" && pwd)"
TESTS="$HW3/../CompArchTests/HW3/N&R_INCLUDE_SEGEL"

if [ ! -d "$TESTS" ]; then
    echo "ERROR: test directory not found: $TESTS"
    exit 1
fi

# Build
echo "[*] Building..."
make -C "$HW3" clean
make -C "$HW3"

# Put binary where run.sh expects it
cp "$HW3/dflow_calc" "$TESTS/dflow_calc"

# Run
echo "[*] Running tests..."
cd "$TESTS"
bash run.sh
