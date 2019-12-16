#!/bin/bash

string=`uname` 
if [[ $string == *"beaglebone"* ]]; then
    gcc lab4b_src/lab4b.c lab4b_src/main_b.c $@ -lmraa -lm -o lab4b
else
    gcc -DDUMMY $@ lab4b_src/debug.c lab4b_src/lab4b.c lab4b_src/main_b.c -lm -o lab4b
fi 

