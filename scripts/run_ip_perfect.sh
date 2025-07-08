#!/bin/bash

timestamp=$(date +"%Y-%m-%d_%H-%M-%S")

INPUTS_DIR="inputs/random_for_ip"
# PARAMS_DIR="parameters/heuristic"
OUTPUTS_DIR="outputs/ip_$timestamp"
CONSOLE_LOG_DIR="console_logs/ip_$timestamp"
summary_file="outputs/ip_summary_$timestamp.csv"

mkdir -p "outputs"
mkdir -p "$OUTPUTS_DIR"
mkdir -p "$CONSOLE_LOG_DIR"

# Loop over each input file in the inputs directory
for input_file in "$INPUTS_DIR"/*; do
    # Define the output file name and experiment summary file name
    output_file="$OUTPUTS_DIR/output_$(basename "$param_file" .txt)_$(basename "$input_file" .txt).txt"
    console_log_file="$CONSOLE_LOG_DIR/console_log_$(basename "$param_file" .txt)_$(basename "$input_file" .txt).txt"

    # Run the executable with the current parameter and input file
    echo "Running IP with input: $input_file"
    ./bin/ip_perfect "$input_file" "$output_file" "$summary_file" > "$console_log_file"
done