#!/bin/bash
gcc -o dispatcher dispatcher.c -lm
gcc -o counter counter.c
# ./dispatcher a ./a.txt
./dispatcher a ./b.txt
