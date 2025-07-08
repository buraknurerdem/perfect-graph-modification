#!/bin/bash

timestamp=$(date +"%Y-%m-%d_%H-%M-%S")

PARAMS_DIR="debug_parameters"
INPUTS_DIR="debug_inputs"

OUTPUTS_DIR="outputs/cp_$timestamp"
CONSOLE_LOG_DIR="console_logs/cp_$timestamp"
summary_file="outputs/cp_summary_$timestamp.csv"

# Ensure the outputs directory exists
mkdir -p "$OUTPUTS_DIR"
mkdir -p "$CONSOLE_LOG_DIR"

for input_file in "$INPUTS_DIR"/*; do
    for param_file in "$PARAMS_DIR"/*; do
        output_file="$OUTPUTS_DIR/output_$(basename "$param_file" .txt)_$(basename "$input_file" .txt).txt"
        console_log_file="$CONSOLE_LOG_DIR/console_log_$(basename "$param_file" .txt)_$(basename "$input_file" .txt).txt"

        ./output_executable "$param_file" "$input_file" "$output_file" "$summary_file" > "$console_log_file"
    done
done