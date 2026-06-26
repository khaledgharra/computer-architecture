#!/bin/bash

cd "$(dirname "$0")"

echo "=== Compiling ==="
make || { echo "Compilation failed!"; exit 1; }

echo ""
echo "=== Copying binary to test folder ==="
cp cacheSim tests/FINAL_tests/cacheSim
chmod +x tests/FINAL_tests/cacheSim

echo ""
echo "=== Running Tests ==="
cd tests/FINAL_tests
bash run.sh

echo ""
echo "=== Done ==="
