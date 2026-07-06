#include <chrono>

#include "odd_hole_worker.h"
#include "find_odd_holes.h"

std::unordered_map<std::string, std::vector<int>> Odd_Hole_Worker::call_find_odd_holes(
    std::vector<std::vector<bool>> &graph,
    IloRangeArray &cuts,
    bool is_anti_hole_search
)
{
    n_worker_find_odd_hole_call++;

    auto time_start = std::chrono::high_resolution_clock::now();

    std::unordered_map<std::string, std::vector<int>> found_holes;
    if (is_anti_hole_search)
    {
        found_holes = find_odd_holes(graph, is_anti_hole_search, parameters->oh_threshold);
    }
    else
    {
        found_holes = find_odd_holes(graph, is_anti_hole_search, parameters->oah_threshold);
    }

    auto time_end = std::chrono::high_resolution_clock::now();
    long long time_elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(time_end - time_start).count();
    n_worker_time_spent_in_find_odd_holes += time_elapsed;

    // add found oddcycles as constraints
    if (!found_holes.empty())
    {
        if (!is_anti_hole_search)
        { // if working on the graph itself, add odd hole constraint

            for (const auto &[key, hole] : found_holes)
            {
                problem->add_odd_hole_constraint(cuts, hole);
                increment_n_worker_added_lazy_cuts_odd_hole(1);
            }

            n_worker_found_odd_holes += found_holes.size();
        }
        else
        { // if working on the complement of the graph, add odd anti hole constraint
            for (const auto &[key, antihole] : found_holes)
            {
                problem->add_odd_antihole_constraint(cuts, antihole);
                increment_n_worker_added_lazy_cuts_odd_antihole(1);
            }
            n_worker_found_odd_anti_holes += found_holes.size();
        }
    }
    return found_holes;
}

