#!/bin/sh

total=0
success=0
for test in tests/testFile/*; do
    if [ -f "$test" ]; then
        ./bin/parser method $test > /dev/null 2>&1
        if [ "$?" -eq 1 ]; then
            success=$(($success+1))
        else
            echo "$test    Not Valid"
        fi
        total=$(($total+1))
    fi
done

echo "Succeded $success/$total tests."
