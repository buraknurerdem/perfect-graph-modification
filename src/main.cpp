#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
// #include <map>
#include <ilcplex/ilocplex.h>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "find_odd_holes.h"
#include "iterative_modification_heuristic.h"
#include "odd_hole_worker.h"
#include "parameters.h"
#include "problem_types.h"
#include "stats.h"
#include "utils.h"


int main(int argc, char *argv[])
{

    // Create Stats object
    Stats program_stats;
    program_stats.start_time();

    IloEnv env;
    IloModel model(env);
    IloCplex cplex(model);

    // Parameter Control CHECK
    if (argc < 6)
    {
        std::cerr << "Too few arguments for: " << argv[0] << "!" << std::endl;
        return 1;
    }

    // Create Parameters Object
    std::string parameter_file_name = argv[2];
    Cutting_Plane_Parameters parameters(parameter_file_name);
    if (parameters.read_parameter_file())
    {
        return 1;
    }

    // Init Problem
    std::string problem_type = argv[1]; // edit, completion, sandwich
    std::string input_graph_file_name = argv[3];
    Problem *problem = nullptr;
    if (problem_type == "edit")
    {
        problem = new Problem_Edit(env, model, cplex, input_graph_file_name);
    }
    else if (problem_type == "completion")
    {
        problem = new Problem_Completion(env, model, cplex, input_graph_file_name);
    }
    else if (problem_type == "sandwich")
    {
        ;
    }
    else
    {
        std::cerr << "Error: problem_type unexpected value." << std::endl;
        return 1;
    }

    
    problem->create_decision_vars();
    problem->set_objective();
    
    // Calculate oh termiation thresholds
    if (parameters.set_oh_thresholds(problem->order, problem->input_graph_p))
    {
        return 1;
    }

    // adding all odd holes and odd antiholes at the start
    if (parameters.add_all)
    {
        auto odd_holes = find_odd_holes(problem->input_graph, false, 0);
        auto complement_of_input_graph = get_complement_of_graph(problem->input_graph);
        auto odd_anti_holes = find_odd_holes(complement_of_input_graph, true, 0);

        IloRangeArray cuts(env);

        if (!odd_holes.empty())
        {
            for (const auto &[key, hole] : odd_holes)
            {
                problem->add_odd_hole_constraint(cuts, hole);
            }
        }
        if (!odd_anti_holes.empty())
        {
            for (const auto &[key, antihole] : odd_anti_holes)
            {
                problem->add_odd_antihole_constraint(cuts, antihole);
            }
        }

        model.add(cuts);

        // Stats update
        program_stats.increment_n_added_initial_cuts_odd_hole(odd_holes.size());
        program_stats.increment_n_found_odd_holes(odd_holes.size());
        program_stats.increment_n_added_initial_cuts_odd_antihole(odd_anti_holes.size());
        program_stats.increment_n_found_odd_antiholes(odd_anti_holes.size());
    }

    cplex.setParam(IloCplex::Threads, parameters.n_threads);
    cplex.setParam(IloCplex::Param::TimeLimit, parameters.time_limit);

    // cplex verbose
    // cplex.setParam(IloCplex::Param::MIP::Display, 4); // Higher verbosity

    // Callback:
    Generic_Callback callback(problem, &parameters);
    cplex.use(
        &callback, IloCplex::Callback::Context::Id::Candidate | IloCplex::Callback::Context::Id::Relaxation
    );

    try
    {
        cplex.solve();
    }
    catch (IloException &e)
    {
        std::cerr << "CPLEX Exception: " << e << std::endl;
    }
    catch (...)
    {
        std::cerr << "Unknown Exception" << std::endl;
    }

    /* std::cout << std::endl;
    std::cout << "========================" << std::endl;
    cplex.out() << "Solution status: " << cplex.getStatus() << std::endl;
    cplex.out() << "Best Objective: " << cplex.getBestObjValue() << std::endl;
    cplex.out() << "Ncols: " << cplex.getNcols() << std::endl;
    cplex.out() << "Nrows: " << cplex.getNrows() << std::endl;
    cplex.out() << "Optimal value: " << cplex.getObjValue() << std::endl; */

    // Collect Stats
    program_stats.collect_info(cplex, callback);

    ////////////////////////////////
    //////////// OUTPUT ////////////
    ////////////////////////////////
    std::string output_file_name = argv[4];
    std::ofstream output_file;
    output_file.open(output_file_name, std::ios::app);
    if (!output_file)
    {
        std::cerr << "Error opening output_file" << std::endl;
        return 1;
    }
    // Write Parameters
    parameters.write_output(output_file);
    // Write Stats
    program_stats.write_output(output_file);
    // Write Problem Output
    problem->write_output(output_file);
    output_file.close();

    /////////////////////////////////
    //////////// SUMMARY ////////////
    /////////////////////////////////
    std::string summary_file_name = argv[5];
    std::ofstream summary_file;

    summary_file.open(summary_file_name, std::ios::app);
    if (!summary_file)
    {
        std::cerr << "Error opening summary_file" << std::endl;
        return 1;
    }
    // Format:
    // Write column names only if the file is empty
    if (summary_file.tellp() == 0)
    {
        summary_file
            << "input_graph_file_name,parameter_file_name,TIME_LIMIT_SECONDS,"
               "N_THREADS,ODD_HOLE_TERMINATION_PERCENTAGE,OH_TERM_THRESHOLD,OAH_TERM_THRESHOLD,"
               "IS_RUNNING_HEURISTIC_ON_CANDIDATE,IS_RUNNING_HEURISTIC_ON_RELAXATION,ADD_ALL_AT_START,";
        summary_file << "wallclock(ms),timeInAlgo(ms),n_foh_calls,n_found_oh,n_found_oah,n_added_initial_"
                        "cuts_odd_hole,"
                        "n_added_initial_cuts_odd_antihole,n_added_lazy_cuts_odd_hole,n_added_lazy_cuts_odd_"
                        "antihole,is_optimal,"
                        "optimality_gap,objective_value,n_feas_sols_found,n_times_heuristic_called,"
                        "number_of_explored_bc_nodes,number_of_unexplored_bc_nodes,";
        summary_file << "graph_order,input_p,m_edges_input,input_graph_density,output_edge_count,"
                        "output_graph_density,is_output_empty_or_complete";
        summary_file << std::endl;
    }

    // Write filenames
    summary_file << input_graph_file_name + ",";
    summary_file << parameter_file_name + ",";
    // Write parameter summary
    parameters.write_summary(summary_file);
    // Write program stats summary
    program_stats.write_summary(summary_file);
    // Write output summary
    problem->write_summary(summary_file);

    // New line at the end:
    summary_file << "\n";

    summary_file.close();

    // Clean up
    env.end();
    delete problem;

    return 0;
}