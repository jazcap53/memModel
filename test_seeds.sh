#!/bin/bash

# run checking for 'byte' in output

i=0
iTarget=128
j=0
jTarget=1
seed=0

make fresh > /dev/null
while [ $i -lt $iTarget ]; do
    ./memModel -t -l -s "$seed" 1>output 2>errout
    grep Byte output > /dev/null
    ret=$?
    if [ $ret -ne 0 ]; then
        echo Seed "$seed" not found
    else
#    if [ $ret -eq 0 ]; then
        echo "$seed"
    fi
    make fresh > /dev/null
    let i=$i+1
    let seed=$seed+100
done
