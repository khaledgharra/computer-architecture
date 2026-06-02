#!/bin/bash

PASSED=0
FAILED=0

for trace in tests/*.trc
do
    base=$(basename "$trace" .trc)

    expected="tests/${base}.out"

    ./bp_main "$trace" > temp_output.txt

    if diff -q temp_output.txt "$expected" > /dev/null
    then
        echo "[PASS] $base"
        PASSED=$((PASSED + 1))
    else
        echo "[FAIL] $base"
        FAILED=$((FAILED + 1))
    fi
done

rm -f temp_output.txt

echo ""
echo "===================="
echo "Passed: $PASSED"
echo "Failed: $FAILED"
echo "===================="