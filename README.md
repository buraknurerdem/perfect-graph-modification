# Perfect Graph Modification

This repository contains the data instances and C++ source code used in our paper:
**"Perfect Graph Modification Problems: An Integer Programming Approach"** by Erdem, B. N., Ekim T., and Taşkın, Z. C.

## Repository Structure

* `src/`: Source code for the minimum perfect editing and completion problems.
* `src_sandwich/`: Source code for the perfect graph sandwich problem.
* `heuristic_experiment/`: Files and scripts for the standalone performance evaluation of the heuristic `IMH`.
* `inputs/`: Graphs used in the computational experiments.

## Prerequisites

To compile and run the programs, you will need:
* A C++20 compatible compiler (e.g., `clang++`)
* IBM ILOG CPLEX Optimization Studio (Version 22.1.1)

## Compilation

The programs under `src/` can be compiled using the following command. Note that the library paths target a Linux x86-64 environment:

```bash
clang++ -std=c++20 -O3 -o $EXECUTABLE \
    src/* \
    -Iinclude \
    -I$CPLEX_DIR/cplex/include \
    -I$CPLEX_DIR/concert/include \
    -L$CPLEX_DIR/cplex/lib/x86-64_linux/static_pic \
    -L$CPLEX_DIR/concert/lib/x86-64_linux/static_pic \
    -lilocplex -lcplex -lconcert -lm -lpthread -ldl
```

You must set the environment variable `$CPLEX_DIR` to your CPLEX installation directory and the path of the output file `$EXECUTABLE` before compiling.

## Usage

Once compiled, you can reproduce the experiments with `run_exp.sh` shell script.

For any additional information, feel free to contact me.