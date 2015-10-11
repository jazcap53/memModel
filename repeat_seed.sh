#!/bin/bash

# run with -t -l repeatedly, checking for 'byte' in output
# then run with -t -l -s 6200, doing the same

i=0
iTarget=32
seed=6200

while [ $i -lt $iTarget ]; do
    make fresh
    ./memModel -t -l 1>output 2>errout
    grep byte output > /dev/null
    ret=$?
    if [ $ret -ne 0 ]; then
        echo Seed "$seed" not found
    else
        echo "$seed" found on run "$i"
    fi
    let i=$i+1
done

i=0
echo

while [ $i -lt $iTarget ]; do
    make fresh
    ./memModel -t -l -s "$seed" 1>output 2>errout
    grep byte output > /dev/null
    ret=$?
    if [ $ret -ne 0 ]; then
        echo Seed "$seed" not found
    else
        echo "$seed" found on run "$i"
    fi
    let i=$i+1
done
