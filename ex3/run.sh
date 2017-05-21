#!/bin/bash
gcc -o dispatcher dispatcher.c -lm
gcc -o counter counter.c
./dispatcher t ./test.txt
#./dispatcher a ./a.txt
