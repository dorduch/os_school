#!/bin/bash
gcc -o dispatcher dispatcher.c
gcc -o counter counter.c
./dispatcher a ./a.txt