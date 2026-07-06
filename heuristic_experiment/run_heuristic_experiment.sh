#!/bin/bash

# Compile Command:
# clang++ heuristic_experiment/main_experiment_heuristic.cpp -std=c++20 -O3 -o bin/main_experiment_heuristic src/find_odd_holes.cpp src/iterative_modification_heuristic.cpp src/utils.cpp -Iinclude
# Run Script Command:
# ./heuristic_experiment/run_heuristic_experiment.sh

timestamp=$(date +"%Y-%m-%d_%H-%M-%S")

INPUTS_DIR="inputs/erdos-renyi"
OUTPUTS_DIR="outputs/heuristic_$timestamp"
summary_file="$OUTPUTS_DIR/heuristic_summary.csv"

mkdir -p "$OUTPUTS_DIR"

for input_file in "$INPUTS_DIR"/*; do
    output_file="$OUTPUTS_DIR/output_$(basename "$input_file" .txt).txt"
    ./bin/main_experiment_heuristic "$input_file" "$output_file" "$summary_file"
done

