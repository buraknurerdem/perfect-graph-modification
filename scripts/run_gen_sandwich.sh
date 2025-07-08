#!/bin/bash

# Directory paths
INPUTS_DIR="inputs"
COMPLETION_DENSITY=0.25
OUTPUTS_DIR="sandwich"_"$COMPLETION_DENSITY"

# Ensure the outputs directory exists
mkdir -p "$OUTPUTS_DIR"

# Loop over each input file in the inputs directory
for input_file in "$INPUTS_DIR"/*; do
    # Define the output file name
    output_file="$OUTPUTS_DIR/optional_edges_"$COMPLETION_DENSITY"_$(basename "$input_file" .txt).txt"

    # Run the executable
    ./random_sandwich_gen "$input_file" "$COMPLETION_DENSITY" "$output_file"
done