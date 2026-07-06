#pragma once

#include <ilcplex/ilocplex.h>
#include <problem_types.h>
#include <parameters.h>
#include <unordered_map>

struct Odd_Hole_Worker
{
    Problem *problem;
    Cutting_Plane_Parameters *parameters;

    int n_worker_find_odd_hole_call = 0;
    size_t n_worker_found_odd_holes = 0;
    size_t n_worker_found_odd_anti_holes = 0;
    int n_worker_found_feas_sols = 0;
    int n_worker_heuristic_called = 0;
    size_t n_worker_added_lazy_cuts_odd_hole = 0;
    size_t n_worker_added_lazy_cuts_odd_antihole = 0;
    long long n_worker_time_spent_in_find_odd_holes = 0;

    Odd_Hole_Worker(Problem *prb, Cutting_Plane_Parameters *params) : problem(prb), parameters(params) {};

    std::unordered_map<std::string, std::vector<int>> call_find_odd_holes(
        std::vector<std::vector<bool>> &graph,
        IloRangeArray &cuts,
        bool is_anti_hole_search
    );

    inline void increment_n_worker_added_lazy_cuts_odd_hole(size_t x) { n_worker_added_lazy_cuts_odd_hole += x; }
    inline void increment_n_worker_added_lazy_cuts_odd_antihole(size_t x) { n_worker_added_lazy_cuts_odd_antihole += x; }
};