#!/bin/bash

# Possible Types: random, p4free, perfect_operations, relative_to_base, heuristic02, sandwich_outer
# Check the script below for additional parameters for specific generation types

# Change as you will
INPUTS_DIR="showg/perfect_mckay"
OUTPUTS_DIR="output/operations"
TYPE="perfect_operations"
ORDERS=(50)
DENSITIES=(25 50 75)
REPETITION=100

mkdir -p "$OUTPUTS_DIR"

if [ "$TYPE" == "perfect_operations" ] || [ "$TYPE" == "random" ] || [ "$TYPE" == "p4free" ] ; then
    for n in "${ORDERS[@]}"; do
        for d in "${DENSITIES[@]}"; do
            for ((id=1; id<=REPETITION; id++ )); do
                if [ "$TYPE" == "perfect_operations" ]; then
                    epsilon=0.01
                    input_pool_path="/Users/burakerdem/showg/perfect_mckay"
                    ./bin/gen_graph "$TYPE" "$n" "$d" "$id" "$epsilon" "$input_pool_path" "$OUTPUTS_DIR"
                elif [ "$TYPE" == "random" ]; then
                    ./bin/gen_graph "$TYPE" "$n" "$d" "$id" "$OUTPUTS_DIR"
                elif [ "$TYPE" == "p4free" ]; then
                    ./bin/gen_graph "$TYPE" "$n" "$d" "$id" "$OUTPUTS_DIR"
                else
                    echo "Error: Unknown graph type specified."
                    exit 1
                fi
            done
        done
    done
elif [ "$TYPE" == "relative_to_base" ]; then
    for input_file in "$INPUTS_DIR"/*; do
        # output file name is defined here. can be changed
        graph_type="relative"
        filename="${input_file##*/}" 
        parts=(${filename//_/ })
        output_filename="${parts[0]}_${graph_type}_${parts[2]}_${parts[3]}_${parts[4]}"
        output_file="$OUTPUTS_DIR/$output_filename"
        different_edge_ratio=0.1
        ./bin/gen_graph "$TYPE" "$input_file" "$different_edge_ratio" "$output_file"
    done
elif [ "$TYPE" == "heuristic02" ]; then
    for input_file in "$INPUTS_DIR"/*; do
        # output file name is defined here. can be changed
        graph_type="heuristic02"
        filename="${input_file##*/}" 
        parts=(${filename//_/ })
        output_filename="${parts[0]}_${graph_type}_${parts[2]}_${parts[3]}_${parts[4]}"
        output_file="$OUTPUTS_DIR/$output_filename"
        ./bin/gen_graph "$TYPE" "$input_file" "$output_file"
    done
elif [ "$TYPE" == "sandwich_outer" ]; then
    for input_file in "$INPUTS_DIR"/*; do
        # output file name is defined here. can be changed
        graph_type="sandwich"
        filename="${input_file##*/}" 
        parts=(${filename//_/ })
        output_filename="${parts[0]}_${graph_type}_${parts[2]}_${parts[3]}_${parts[4]}"
        output_file="$OUTPUTS_DIR/$output_filename"
        # output_file="$OUTPUTS_DIR/${input_file##*/}"
        optional_edge_density=1
        ./bin/gen_graph "$TYPE" "$input_file" "$optional_edge_density" "$output_file"
    done
else
    echo "Error: Unknown graph type specified."
    exit 1
fi