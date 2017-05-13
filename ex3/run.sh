#!/bin/bash
gcc -o dispatcher dispatcher.c -lm
gcc -o counter counter.c
./dispatcher a ./a.txt
# ./dispatcher a ./54710-0.txt
