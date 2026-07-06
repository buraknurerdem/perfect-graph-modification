#!/bin/bash

timestamp=$(date +"%Y-%m-%d_%H-%M-%S")

PROBLEM_TYPE="completion" # edit, completion, sandwich
PARAMS_DIR="parameters/dimac_comp"
INPUTS_DIR="inputs/dimacs_nonperfect_filtered"

OUTPUTS_DIR="outputs/experiment_"$PROBLEM_TYPE"_"$timestamp

CONSOLE_LOG_DIR=$OUTPUTS_DIR"/console_logs"
summary_file=$OUTPUTS_DIR"/summary.csv"

# Ensure the outputs directory exists
mkdir -p "$OUTPUTS_DIR"
mkdir -p "$CONSOLE_LOG_DIR"

for input_file in "$INPUTS_DIR"/*; do
    for param_file in "$PARAMS_DIR"/*; do
        output_file="$OUTPUTS_DIR/output_"$PROBLEM_TYPE"_$(basename "$param_file" .txt)_$(basename "$input_file" .txt).txt"
        console_log_file="$CONSOLE_LOG_DIR/console_log_"$PROBLEM_TYPE"_$(basename "$param_file" .txt)_$(basename "$input_file" .txt).txt"

        ./bin/main "$PROBLEM_TYPE" "$param_file" "$input_file" "$output_file" "$summary_file" > "$console_log_file"
    done
done