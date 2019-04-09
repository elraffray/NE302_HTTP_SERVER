#!/bin/sh

total=0
success=0
for test in tests/testFile/*; do
    if [ -f "$test" ]; then
        echo "testing $test"
        ./bin/parser method $test > /dev/null 2>&1
        if [ "$?" -eq 1 ]; then
            success=$(($success+1))
        fi
        total=$(($total+1))
    fi
done

echo "Succeded $success/$total tests."
