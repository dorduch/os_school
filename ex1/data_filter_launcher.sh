#!/bin/bash
export DATA_SIZE="10K"
export INPUT_DATA="/dev/urandom"
export OUTPUT_DATA="./out.txt"
./data_filter ${DATA_SIZE} ${INPUT_DATA} ${OUTPUT_DATA}
