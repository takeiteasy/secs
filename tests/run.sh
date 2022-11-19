#!/usr/bin/env sh
echo "Running $# tests..."

count=0
successes=0
failures=0
for exe in "$@"
do
    ((count++))
    printf " * #$count ($exe)..."
    if [ $? = 0 ]; then
        echo "[SUCCESS]"
        ((successes++))
    else
        echo "[FAILED]"
        ((failures++))
    fi
done
if [ $failures = 0 ]; then
    echo "ALL TESTS COMPLETED SUCCESSFULLY"
else
    echo "#$count tests run, $failures failed"
fi
