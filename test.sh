#!/usr/bin/env sh
echo "Running $# tests..."

count=0
successes=0
failures=0
for exe in "$@"
do
    ((count++))
    echo " * Running test #$count ($exe)..."
    eval "./$exe"
    if [ $? = 0 ]; then
        printf " * #$count \e[0;32m[SUCCESS]\e[m\n"
        ((successes++))
    else
        printf " * #$count \e[0;31m[FAILED]\e[m\n"
        ((failures++))
    fi
done
if [ $failures = 0 ]; then
    echo "ALL TESTS COMPLETED SUCCESSFULLY"
    exit 0
else
    echo "#$count tests run, $failures failed"
    exit 1
fi
