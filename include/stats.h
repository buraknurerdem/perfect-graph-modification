#pragma once

#include <iostream>
#include <chrono>
#include <ilcplex/ilocplex.h>
#include "callback.h"

struct Stats
{

    std::chrono::high_resolution_clock::time_point time_start;
    long long time_elapsed;
    bool is_optimal;
    double optimality_gap;
    double objective_value;
    int n_feas_sols_found = 0;
    int n_times_heuristic_called = 0;
    int number_of_explored_bc_nodes = 0;
    int number_of_unexplored_bc_nodes = 0;
    size_t n_added_initial_cuts_odd_hole = 0;
    size_t n_added_initial_cuts_odd_antihole = 0;
    size_t n_added_lazy_cuts_odd_hole = 0;
    size_t n_added_lazy_cuts_odd_antihole = 0;

    int n_added_oh_ohbar_cuts = 0;
    int n_find_odd_hole_calls = 0;
    size_t n_found_odd_holes = 0;
    size_t n_found_odd_antiholes = 0;
    long long time_spent_in_find_odd_holes = 0;

    void start_time();
    void collect_info(IloCplex &cplex, Generic_Callback &candidate_callback);
    void write_output(std::ofstream& output_file);
    void write_summary(std::ofstream& summary_file);
    void increment_n_found_odd_holes(size_t x) {n_found_odd_holes += x;};
    void increment_n_found_odd_antiholes(size_t x) {n_found_odd_antiholes += x;};
    void increment_n_added_initial_cuts_odd_hole(size_t x) {n_added_initial_cuts_odd_hole += x;};
    void increment_n_added_initial_cuts_odd_antihole(size_t x) {n_added_initial_cuts_odd_antihole += x;};
    void increment_n_added_lazy_cuts_odd_hole(size_t x) {n_added_lazy_cuts_odd_hole += x;};
    void increment_n_added_lazy_cuts_odd_antihole(size_t x) {n_added_lazy_cuts_odd_antihole += x;};
};