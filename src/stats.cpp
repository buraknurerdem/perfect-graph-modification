#include "stats.h"

void Stats::start_time() { time_start = std::chrono::high_resolution_clock::now(); }

void Stats::collect_info(IloCplex &cplex, Generic_Callback &candidate_callback)
{

    std::chrono::high_resolution_clock::time_point time_end = std::chrono::high_resolution_clock::now();
    time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();

    for (auto &worker : candidate_callback.workers)
    {
        n_find_odd_hole_calls += worker->n_worker_find_odd_hole_call;
        n_found_odd_holes += worker->n_worker_found_odd_holes;
        n_found_odd_antiholes += worker->n_worker_found_odd_anti_holes;
        time_spent_in_find_odd_holes += worker->n_worker_time_spent_in_find_odd_holes;
        n_feas_sols_found += worker->n_worker_found_feas_sols;
        n_times_heuristic_called += worker->n_worker_heuristic_called;
        increment_n_added_lazy_cuts_odd_hole(worker->n_worker_added_lazy_cuts_odd_hole);
        increment_n_added_lazy_cuts_odd_antihole(worker->n_worker_added_lazy_cuts_odd_antihole);
    }

    // opt gap
    optimality_gap = cplex.getMIPRelativeGap();

    // obj value
    objective_value = cplex.getObjValue();

    // is optimal?
    if (optimality_gap < 1e-6)
    {
        is_optimal = true;
    }
    else
    {
        is_optimal = false;
    }

    // bc tree nodes:
    number_of_explored_bc_nodes = int(cplex.getNnodes());
    number_of_unexplored_bc_nodes = int(cplex.getNnodesLeft());

    return;
}

void Stats::write_output(std::ofstream &output_file)
{
    output_file << "========================" << std::endl;
    output_file << "Wall-Clock Runtime (ms): " << time_elapsed << std::endl;
    output_file << "Total Time of All Workers in OddHoleAlg (nano): " << time_spent_in_find_odd_holes
                << std::endl;
    output_file << "Total Time of All Workers in OddHoleAlg (ms): " << time_spent_in_find_odd_holes / 1000000
                << std::endl;
    output_file << "OddHoleAlg calls: " << n_find_odd_hole_calls << std::endl;
    output_file << "Number of Found Odd-Holes: " << n_found_odd_holes << std::endl;
    output_file << "Number of Found Odd Anti-Holes: " << n_found_odd_antiholes << std::endl;
    output_file << "n_added_initial_cuts_odd_hole: " << n_added_initial_cuts_odd_hole << std::endl;
    output_file << "n_added_initial_cuts_odd_antihole: " << n_added_initial_cuts_odd_antihole << std::endl;
    output_file << "n_added_lazy_cuts_odd_hole: " << n_added_lazy_cuts_odd_hole << std::endl;
    output_file << "n_added_lazy_cuts_odd_antihole: " << n_added_lazy_cuts_odd_antihole << std::endl;
    output_file << "Is Optimal? (1e-6): " << is_optimal << std::endl;
    output_file << "Optimality Gap: " << optimality_gap << std::endl;
    output_file << "Objective Value: " << objective_value << std::endl;
    output_file << "Number of Feasible Solutions Found: " << n_feas_sols_found << std::endl;
    output_file << "Number of Times Heuristic is Called: " << n_times_heuristic_called << std::endl;
    output_file << "Number of Explored B&C tree nodes: " << number_of_explored_bc_nodes << std::endl;
    output_file << "Number of Unexplored B&C tree nodes: " << number_of_unexplored_bc_nodes << std::endl;

    return;
}

void Stats::write_summary(std::ofstream &summary_file)
{
    summary_file << std::to_string(time_elapsed) + ",";
    summary_file << std::to_string(time_spent_in_find_odd_holes / 1000000) + ",";
    summary_file << std::to_string(n_find_odd_hole_calls) + ",";
    summary_file << std::to_string(n_found_odd_holes) + ",";
    summary_file << std::to_string(n_found_odd_antiholes) + ",";
    summary_file << std::to_string(n_added_initial_cuts_odd_hole) + ",";
    summary_file << std::to_string(n_added_initial_cuts_odd_antihole) + ",";
    summary_file << std::to_string(n_added_lazy_cuts_odd_hole) + ",";
    summary_file << std::to_string(n_added_lazy_cuts_odd_antihole) + ",";
    summary_file << std::to_string(is_optimal) + ",";
    summary_file << std::to_string(optimality_gap) + ",";
    summary_file << std::to_string(objective_value) + ",";
    summary_file << std::to_string(n_feas_sols_found) + ",";
    summary_file << std::to_string(n_times_heuristic_called) + ",";
    summary_file << std::to_string(number_of_explored_bc_nodes) + ",";
    summary_file << std::to_string(number_of_unexplored_bc_nodes) + ",";
}