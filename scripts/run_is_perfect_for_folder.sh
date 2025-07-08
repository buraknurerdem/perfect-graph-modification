#!/bin/bash

# Input directory. Loops over all files
INPUTS_DIR="/Users/burakerdem/graphs/heuristic"

# 1 if a console log is wanted for each graph. Otherwise prints non-perfects
VERBOSE=1

for input_file in "$INPUTS_DIR"/*; do
    if ! ./bin/is_perfect "$input_file" "$VERBOSE"; then
        echo "$input_file"
    fi
done
