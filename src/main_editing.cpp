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
#include "utils.h"


// Parameter Definitions
int ODD_HOLE_TERMINATION_BATCH_SIZE;
int ODD_ANTI_HOLE_TERMINATION_BATCH_SIZE;
bool IS_RUNNING_HEURISTIC_ON_CANDIDATE;
bool IS_RUNNING_HEURISTIC_ON_RELAXATION;
bool ADD_ALL_AT_START;

struct OddHoleWorker
{
    std::vector<std::vector<int>> &flat_index_matrix;
    IloEnv &env;

    int n_worker_find_odd_hole_call = 0;
    int n_worker_found_odd_holes = 0;
    int n_worker_found_odd_anti_holes = 0;
    int n_worker_found_feas_sols = 0;
    int n_worker_heuristic_called = 0;
    int n_worker_oh_and_ohbar_cut_size = 0;
    long long n_worker_time_spent_in_find_odd_holes = 0;

    OddHoleWorker(std::vector<std::vector<int>> &flat_index_matrix_in, IloEnv &env_in);
    std::unordered_map<std::string, std::vector<int>>
    call_find_odd_holes(std::vector<std::vector<bool>> &graph,
                        IloRangeArray &cuts,
                        IloNumVarArray &x_flat,
                        bool is_anti_hole_search);
};

OddHoleWorker::OddHoleWorker(std::vector<std::vector<int>> &flat_index_matrix_in, IloEnv &env_in)
    : flat_index_matrix(flat_index_matrix_in), env(env_in)
{
}

// Find odd holes, write to expression array as constraints
std::unordered_map<std::string, std::vector<int>>
OddHoleWorker::call_find_odd_holes(std::vector<std::vector<bool>> &graph,
                                   IloRangeArray &cuts,
                                   IloNumVarArray &x_flat,
                                   bool is_anti_hole_search)
{

    n_worker_find_odd_hole_call++;

    auto time_start = std::chrono::high_resolution_clock::now();

    std::unordered_map<std::string, std::vector<int>> found_holes;
    if (is_anti_hole_search)
    {
        found_holes =
            find_odd_holes(graph, is_anti_hole_search, ODD_ANTI_HOLE_TERMINATION_BATCH_SIZE);
    }
    else
    {
        found_holes = find_odd_holes(graph, is_anti_hole_search, ODD_HOLE_TERMINATION_BATCH_SIZE);
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

            for (const auto &[key, cycle] : found_holes)
            {

                IloExpr oddHoleConstraint(env);
                for (int i = 0; i < cycle.size(); ++i)
                {
                    for (int j = i + 1; j < cycle.size(); ++j)
                    {
                        if ((j == i + 1) || (i == 0 && j == (cycle.size() - 1)))
                        { // edge on the cycle
                            oddHoleConstraint +=
                                (1 - x_flat[flat_index_matrix[cycle[i]][cycle[j]]]);
                        }
                        else
                        {
                            oddHoleConstraint += x_flat[flat_index_matrix[cycle[i]][cycle[j]]];
                        }
                    }
                }
                cuts.add(oddHoleConstraint >= 1);
                oddHoleConstraint.end();
            }

            n_worker_found_odd_holes += found_holes.size();
        }
        else
        { // if working on the complement of the graph, add odd anti hole constraint
            for (const auto &[key, cycle] : found_holes)
            {

                IloExpr oddHoleConstraint(env);
                for (int i = 0; i < cycle.size(); ++i)
                {
                    for (int j = i + 1; j < cycle.size(); ++j)
                    {
                        if ((j == i + 1) || (i == 0 && j == (cycle.size() - 1)))
                        { // edge on the cycle
                            oddHoleConstraint += x_flat[flat_index_matrix[cycle[i]][cycle[j]]];
                        }
                        else
                        {
                            oddHoleConstraint +=
                                (1 - x_flat[flat_index_matrix[cycle[i]][cycle[j]]]);
                        }
                    }
                }
                cuts.add(oddHoleConstraint >= 1);
                oddHoleConstraint.end();
            }
            n_worker_found_odd_anti_holes += found_holes.size();
        }
    }
    return found_holes;
}

struct Generic_callback : public IloCplex::Callback::Function
{
    IloEnv &env;
    std::vector<OddHoleWorker *> workers;
    IloNumVarArray &x_flat;
    std::vector<std::vector<int>> &flat_index_matrix;
    std::vector<std::vector<bool>> &input_graph;
    int order;
    int n_of_vertex_pairs;

    Generic_callback(IloNumVarArray &xIn,
                     int n_threads,
                     int order_in,
                     int m_edges_in,
                     std::vector<std::vector<int>> &flat_index_matrix,
                     std::vector<std::vector<bool>> &input_graph,
                     IloEnv &env);

    void invoke(const IloCplex::Callback::Context &context);
};

Generic_callback::Generic_callback(IloNumVarArray &xIn,
                                   int n_threads,
                                   int order_in,
                                   int m_edges_in,
                                   std::vector<std::vector<int>> &flat_index_matrix_in,
                                   std::vector<std::vector<bool>> &input_graph_in,
                                   IloEnv &env_in)
    : x_flat(xIn), order(order_in), n_of_vertex_pairs(m_edges_in),
      flat_index_matrix(flat_index_matrix_in), input_graph(input_graph_in), env(env_in)
{
    workers.resize(n_threads);
    for (int i = 0; i < n_threads; ++i)
        workers[i] = new OddHoleWorker(flat_index_matrix_in, env);
}

void Generic_callback::invoke(const IloCplex::Callback::Context &context)
{

    // std::cout << "Generic_callback::invoke has been called" << std::endl;

    int const thread_no = context.getIntInfo(IloCplex::Callback::Context::Info::ThreadId);
    IloEnv env = context.getEnv();
    auto context_id = context.getId();

    if (context_id != IloCplex::Callback::Context::Id::Relaxation &&
        context_id != IloCplex::Callback::Context::Id::Candidate)
        std::cout << "Unexpected context type at Callback!" << std::endl;

    if (context_id == IloCplex::Callback::Context::Id::Candidate)
    {
        // Get candidate, current solution
        IloNumArray x_val(env);
        context.getCandidatePoint(x_flat, x_val);

        // create the graph corresponding to the current solution
        std::vector<std::vector<bool>> candidate_graph(order, std::vector<bool>(order, false));
        for (int ind = 0; ind < n_of_vertex_pairs; ++ind)
        {
            if (x_val[ind] == 1)
            {
                int i = x_flat[ind].getIntProperty("i");
                int j = x_flat[ind].getIntProperty("j");
                candidate_graph[i][j] = true;
                candidate_graph[j][i] = true;
            }
        }
        // construct its complement complement
        std::vector<std::vector<bool>> complement_graph = get_complement_of_graph(candidate_graph);

        OddHoleWorker *p_worker = workers[thread_no];

        IloRangeArray cuts(env);
        auto found_odd_holes = p_worker->call_find_odd_holes(candidate_graph, cuts, x_flat, false);
        auto found_odd_anti_holes =
            p_worker->call_find_odd_holes(complement_graph, cuts, x_flat, true);

        if ((found_odd_holes.size() > 0) || (found_odd_anti_holes.size() > 0))
        {
            // add constraints, reject current candidate solution
            p_worker->n_worker_oh_and_ohbar_cut_size += cuts.getSize();
            context.rejectCandidate(cuts);

            // Run heuristic below
            if (IS_RUNNING_HEURISTIC_ON_CANDIDATE)
            {

                p_worker->n_worker_heuristic_called++;

                // heuristic_stats heuristic_stats_obj =
                //     heuristic02_base_graph(input_graph, candidate_graph, HEURISTIC_INPUT_PRIORITY);
                heuristic_stats heuristic_stats_obj = heuristic02(candidate_graph);

                if (heuristic_stats_obj.is_successful)
                {
                    p_worker->n_worker_found_feas_sols++;

                    // Generate necessary cplex objects.
                    IloNumArray heuristic_x_val(env, x_val.getSize());
                    double heuristic_obj_val = 0;
                    for (int i = 0; i < order - 1; i++)
                    {
                        for (int j = i + 1; j < order; j++)
                        {
                            heuristic_x_val[flat_index_matrix[i][j]] =
                                heuristic_stats_obj.output_graph[i][j];
                            if (input_graph[i][j] != heuristic_stats_obj.output_graph[i][j])
                            {
                                heuristic_obj_val += 1;
                            }
                        }
                    }

                    auto node_best_sol_obj_info =
                        context.getDoubleInfo(IloCplex::Callback::Context::Info::BestSolution);

                    // provide heuristic solution to cplex, if it is better tthan current UB
                    if (heuristic_obj_val < node_best_sol_obj_info)
                    {
                        std::cout << "In Candidate Callback. Heuristic solution found with "
                                     "objective value: "
                                  << heuristic_obj_val
                                  << ". Prev. Best Sol: " << node_best_sol_obj_info << std::endl;
                        context.postHeuristicSolution(
                            x_flat, heuristic_x_val, heuristic_obj_val,
                            IloCplex::Callback::Context::SolutionStrategy::NoCheck);
                    }
                }
            }
        }
        else
        {
            p_worker->n_worker_found_feas_sols++;
        }
        cuts.end();
    }

    // Check if at relaxation callback and Heuristic on relaxation is on
    if (IS_RUNNING_HEURISTIC_ON_RELAXATION &&
        context_id == IloCplex::Callback::Context::Id::Relaxation)
    {
        auto node_count_info = context.getIntInfo(IloCplex::Callback::Context::Info::NodeCount);

        // Run heuristic for every tenth node of b&c tree
        if (node_count_info % 10 != 0)
        {
            return;
        }

        // std::cout << "=================================" << std::endl;
        // std::cout << "In relaxation callback. Node ID: " << node_count_info << std::endl;

        // Get current relatation point
        IloNumArray x_val(env);

        context.getRelaxationPoint(x_flat, x_val);

        // find a graph according to the fractional solution
        std::vector<std::vector<bool>> candidate_graph(order, std::vector<bool>(order, false));
        for (int i = 0; i < order - 1; i++)
        {
            for (int j = i + 1; j < order; j++)
            {
                bool edge_value = false;
                if (x_val[flat_index_matrix[i][j]] > 0.50)
                {
                    edge_value = true;
                }
                candidate_graph[i][j] = edge_value;
                candidate_graph[j][i] = edge_value;
            }
        }

        OddHoleWorker *p_worker = workers[thread_no];

        // call heuristic on this graph.
        // heuristic_stats heuristic_stats_obj =
        //     heuristic02_base_graph(input_graph, candidate_graph, HEURISTIC_INPUT_PRIORITY);
        heuristic_stats heuristic_stats_obj = heuristic02(candidate_graph);
        p_worker->n_worker_heuristic_called++;

        if (heuristic_stats_obj.is_successful)
        {
            p_worker->n_worker_found_feas_sols++;

            // Generate necessary cplex objects.
            IloNumArray heuristic_x_val(env, x_val.getSize());
            double heuristic_obj_val = 0;
            for (int i = 0; i < order - 1; i++)
            {
                for (int j = i + 1; j < order; j++)
                {
                    heuristic_x_val[flat_index_matrix[i][j]] =
                        heuristic_stats_obj.output_graph[i][j];
                    if (input_graph[i][j] != heuristic_stats_obj.output_graph[i][j])
                    {
                        heuristic_obj_val += 1;
                    }
                }
            }

            // provide heuristic solution to cplex
            auto node_best_sol_obj_info =
                context.getDoubleInfo(IloCplex::Callback::Context::Info::BestSolution);

            // provide heuristic solution to cplex, if it is better tthan current UB
            if (heuristic_obj_val < node_best_sol_obj_info)
            {
                // std::cout << "================================" << std::endl;
                // std::cout << "Thread No:" << thread_no << std::endl;
                std::cout
                    << "In Relaxation Callback. Heuristic solution found with objective value: "
                    << heuristic_obj_val << ". Prev. Best Sol: " << node_best_sol_obj_info
                    << std::endl;
                context.postHeuristicSolution(
                    x_flat, heuristic_x_val, heuristic_obj_val,
                    IloCplex::Callback::Context::SolutionStrategy::NoCheck);
            }
        }
    }
}

struct Stats
{

    std::chrono::high_resolution_clock::time_point time_start;
    long long time_elapsed;
    bool is_optimal;
    double optimality_gap;
    double objective_value;
    int n_feas_sols_found = 0;
    int n_times_heuristic_called = 0;
    int n_added_oh_ohbar_cuts = 0;

    int n_find_odd_hole_calls = 0;
    int n_odd_holes = 0;
    int n_odd_anti_holes = 0;
    long long time_spent_in_find_odd_holes = 0;

    void start_time();
    void collect_info(IloCplex &cplex, Generic_callback &candidate_callback);
};

void Stats::start_time() { time_start = std::chrono::high_resolution_clock::now(); }

void Stats::collect_info(IloCplex &cplex, Generic_callback &candidate_callback)
{

    std::chrono::high_resolution_clock::time_point time_end =
        std::chrono::high_resolution_clock::now();
    time_elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();

    for (auto &worker : candidate_callback.workers)
    {
        n_find_odd_hole_calls += worker->n_worker_find_odd_hole_call;
        n_odd_holes += worker->n_worker_found_odd_holes;
        n_odd_anti_holes += worker->n_worker_found_odd_anti_holes;
        time_spent_in_find_odd_holes += worker->n_worker_time_spent_in_find_odd_holes;
        n_feas_sols_found += worker->n_worker_found_feas_sols;
        n_times_heuristic_called += worker->n_worker_heuristic_called;
        n_added_oh_ohbar_cuts += worker->n_worker_oh_and_ohbar_cut_size;
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
}

int main(int argc, char *argv[])
{

    Stats program_stats;
    program_stats.start_time();

    IloEnv env;
    IloModel model(env);

    // Parameter Control
    if (argc < 5)
    {
        std::cerr << "Too few arguments for: " << argv[0] << "!" << std::endl;
        return 1;
    }

    // Read Program Parameters
    std::string parameter_file_name = argv[1];
    std::ifstream parameter_file(parameter_file_name);

    if (!parameter_file)
    {
        std::cerr << "Error opening parameter file" << std::endl;
        return 1;
    }

    std::unordered_map<std::string, std::string> parameters;
    std::string param, value;
    while (parameter_file >> param >> value)
    { // Read parameters and values
        parameters[param] = value;
    }

    parameter_file.close();

    // Read Input Graph. e_ij : Input graph's adjacency matrix
    std::string input_graph_file_name = argv[2];
    std::vector<std::vector<bool>> input_graph =
        read_graph_adj_matrix_from_txt_file(input_graph_file_name);

    // Parsing input_graph_file_name to get p parameter
    // inputs/inputGraph_order_20_density_0.750000_id_1.txt
    std::size_t pos = input_graph_file_name.find("density_");
    double inputGraph_p_Param;
    if (pos != std::string::npos)
    {
        std::string p_str = input_graph_file_name.substr(pos + 8); // 8 is the length of "density_"

        std::size_t end_pos = p_str.find_first_not_of("0123456789.");
        if (end_pos != std::string::npos)
        {
            inputGraph_p_Param = std::stod(p_str.substr(0, end_pos));
        }
    }
    else
    {
        std::cout << "p_parameter not found in the input_graph_file_name" << std::endl;
    }

    int order = input_graph.size();
    int m_edges_input = 0;
    int n_of_vertex_pairs = ((order * order) - order) / 2; // n choose 2
    std::vector<std::vector<int>> flat_index_matrix(order, std::vector<int>(order, 0));

    // Integrate Parameters into the model
    int n_threads = std::stoi(parameters["N_THREADS"]);
    // HEURISTIC_INPUT_PRIORITY = std::stod(parameters["HEURISTIC_INPUT_PRIORITY"]);
    if (parameters["IS_RUNNING_HEURISTIC_ON_CANDIDATE"] == "1")
    {
        IS_RUNNING_HEURISTIC_ON_CANDIDATE = 1;
    }
    else
    {
        IS_RUNNING_HEURISTIC_ON_CANDIDATE = 0;
    }
    if (parameters["IS_RUNNING_HEURISTIC_ON_RELAXATION"] == "1")
    {
        IS_RUNNING_HEURISTIC_ON_RELAXATION = 1;
    }
    else
    {
        IS_RUNNING_HEURISTIC_ON_RELAXATION = 0;
    }
    if (parameters["ADD_ALL_AT_START"] == "1")
    {
        ADD_ALL_AT_START = 1;
    }
    else
    {
        ADD_ALL_AT_START = 0;
    }

    // Odd Hole Termination Count Calculation:
    double odd_hole_termination_percentage =
        std::stod(parameters["ODD_HOLE_TERMINATION_PERCENTAGE"]);
    int expected_no_of_odd_holes =
        static_cast<int>(expected_value_of_odd_holes(order, inputGraph_p_Param));
    int expected_no_of_odd_anti_holes =
        static_cast<int>(expected_value_of_odd_anti_holes(order, inputGraph_p_Param));
    if (odd_hole_termination_percentage == 999)
    {
        std::cout << "ODD_HOLE_TERMINATION_PERCENTAGE is set to 999" << std::endl;
        std::cout << "Program searches for ALL odd holes and odd antiholes" << std::endl;
        ODD_HOLE_TERMINATION_BATCH_SIZE =
            0; // Zero means do not terminate based on the found odd hole count
        ODD_ANTI_HOLE_TERMINATION_BATCH_SIZE =
            0; // Zero means do not terminate based on the found odd anti hole count
    }
    else if (odd_hole_termination_percentage == 1)
    {
        std::cout << "ODD_HOLE_TERMINATION_PERCENTAGE is set to 1" << std::endl;
        std::cout << "Program searches for JUST ONE odd hole or odd antihole" << std::endl;
        ODD_HOLE_TERMINATION_BATCH_SIZE = 1;
        ODD_ANTI_HOLE_TERMINATION_BATCH_SIZE = 1;
    }
    else if (odd_hole_termination_percentage == 0)
    {
        std::cout << "ERROR: ODD_HOLE_TERMINATION_PERCENTAGE is set to 0" << std::endl;
        exit(0);
    }
    else
    {
        ODD_HOLE_TERMINATION_BATCH_SIZE =
            static_cast<int>(odd_hole_termination_percentage * expected_no_of_odd_holes / 100);
        ODD_ANTI_HOLE_TERMINATION_BATCH_SIZE =
            static_cast<int>(odd_hole_termination_percentage * expected_no_of_odd_anti_holes / 100);
    }

    std::cout << "==============================================" << std::endl;
    std::cout << "n:                                    " << order << std::endl;
    std::cout << "ODD_HOLE_TERMINATION_PERCENTAGE:      " << odd_hole_termination_percentage
              << std::endl;
    std::cout << "ODD_HOLE_TERMINATION_BATCH_SIZE:      " << ODD_HOLE_TERMINATION_BATCH_SIZE
              << std::endl;
    std::cout << "ODD_ANTI_HOLE_TERMINATION_BATCH_SIZE: " << ODD_ANTI_HOLE_TERMINATION_BATCH_SIZE
              << std::endl;
    std::cout << "==============================================" << std::endl;

    // Objective Expression
    IloExpr objective_exp(env);

    // Decision Var. x_{i,j}
    // Upper Triangular Entries Only: i < j
    IloNumVarArray x_flat(env, n_of_vertex_pairs);
    int temp_index = 0;
    for (int i = 0; i < order; ++i)
    {
        for (int j = i + 1; j < order; ++j)
        {
            flat_index_matrix[i][j] = temp_index;
            flat_index_matrix[j][i] = temp_index;
            x_flat[temp_index] = IloNumVar(env, 0.0, 1.0, ILOBOOL);
            x_flat[temp_index].setIntProperty("i", i);
            x_flat[temp_index].setIntProperty("j", j);
            // x[i][j].setStringProperty("type", "x");
            model.add(x_flat[temp_index]);

            // adding to objective expression according to edge/nonedge existence
            if (input_graph[i][j])
                objective_exp += (1 - x_flat[temp_index]);
            else
                objective_exp += x_flat[temp_index];

            temp_index++;

            if (input_graph[i][j])
                m_edges_input++;
        }
    }

    // Adding Objective to the Model
    IloObjective objective = IloMinimize(env, objective_exp);
    model.add(objective);
    objective_exp.end();

    // adding all odd-holes and odd-anti holes at the start
    if (ADD_ALL_AT_START)
    {
        auto odd_holes = find_odd_holes(input_graph, false, 0);
        auto complement_of_input_graph = get_complement_of_graph(input_graph);
        auto odd_anti_holes = find_odd_holes(complement_of_input_graph, true, 0);
        std::cout << "Odd hole count:" << odd_holes.size() << std::endl;
        std::cout << "Odd anti hole count:" << odd_anti_holes.size() << std::endl;
        // ADD TO STATS THE FOUND ODD CYCLE COUNT
        if (!odd_holes.empty())
        {

            for (const auto &[key, cycle] : odd_holes)
            {

                IloExpr oddHoleConstraint(env);
                for (int i = 0; i < cycle.size(); ++i)
                {
                    for (int j = i + 1; j < cycle.size(); ++j)
                    {
                        if ((j == i + 1) || (i == 0 && j == (cycle.size() - 1)))
                        { // edge on the cycle
                            oddHoleConstraint +=
                                (1 - x_flat[flat_index_matrix[cycle[i]][cycle[j]]]);
                        }
                        else
                        {
                            oddHoleConstraint += x_flat[flat_index_matrix[cycle[i]][cycle[j]]];
                        }
                    }
                }
                model.add(oddHoleConstraint >= 1);
                oddHoleConstraint.end();
            }
        }
        if (!odd_anti_holes.empty())
        {
            for (const auto &[key, cycle] : odd_anti_holes)
            {

                IloExpr oddHoleConstraint(env);
                for (int i = 0; i < cycle.size(); ++i)
                {
                    for (int j = i + 1; j < cycle.size(); ++j)
                    {
                        if ((j == i + 1) || (i == 0 && j == (cycle.size() - 1)))
                        { // edge on the cycle
                            oddHoleConstraint += x_flat[flat_index_matrix[cycle[i]][cycle[j]]];
                        }
                        else
                        {
                            oddHoleConstraint +=
                                (1 - x_flat[flat_index_matrix[cycle[i]][cycle[j]]]);
                        }
                    }
                }
                model.add(oddHoleConstraint >= 1);
                oddHoleConstraint.end();
            }
        }
    }

    IloCplex cplex(model);

    // MIP emphasis:
    // cplex.setParam(IloCplex::MIPEmphasis, CPX_MIPEMPHASIS_HIDDENFEAS);

    cplex.setParam(IloCplex::Threads, n_threads);
    cplex.setParam(IloCplex::Param::TimeLimit, std::stoi(parameters["TIME_LIMIT_SECONDS"]));
    /* if(std::stod(parameters["OPTIMALITY_GAP_PARAMETER"]) > 0){ // Only alter the optimality gap
    threshold when it is not zero. cplex.setParam(IloCplex::EpGap,
    std::stod(parameters["OPTIMALITY_GAP_PARAMETER"]));
    } */

    // cplex verbose
    // cplex.setParam(IloCplex::Param::MIP::Display, 4); // Higher verbosity

    // Callback:
    Generic_callback callback(x_flat, n_threads, order, n_of_vertex_pairs, flat_index_matrix,
                              input_graph, env);
    cplex.use(&callback, IloCplex::Callback::Context::Id::Candidate |
                             IloCplex::Callback::Context::Id::Relaxation);

    IloInt number_of_explored_bc_nodes;
    IloInt number_of_unexplored_bc_nodes;
    try
    {
        cplex.solve();

        std::cout << "STATUS: " << cplex.getStatus() << std::endl;
        std::cout << "OBJ VALUE: " << cplex.getObjValue() << std::endl;
        number_of_explored_bc_nodes = cplex.getNnodes();
        number_of_unexplored_bc_nodes = cplex.getNnodesLeft();
        std::cout << "Explored Number of nodes: " << number_of_explored_bc_nodes << std::endl;
        std::cout << "Unexplored Number of nodes: " << number_of_unexplored_bc_nodes << std::endl;
    }
    catch (IloException &e)
    {
        std::cout << "CPLEX Exception: " << e << std::endl;
    }
    catch (...)
    {
        std::cout << "Unknown Exception" << std::endl;
    }

    /* std::cout << std::endl;
    std::cout << "========================" << std::endl;
    cplex.out() << "Solution status: " << cplex.getStatus() << std::endl;
    cplex.out() << "Best Objective: " << cplex.getBestObjValue() << std::endl;
    cplex.out() << "Ncols: " << cplex.getNcols() << std::endl;
    cplex.out() << "Nrows: " << cplex.getNrows() << std::endl;
    cplex.out() << "Optimal value: " << cplex.getObjValue() << std::endl; */

    // Stats
    program_stats.collect_info(cplex, callback);

    // write output log to a txt
    std::string output_file_name = argv[3];
    std::ofstream output_file(output_file_name);

    // Related to Parameter File
    output_file << "========================" << std::endl;
    output_file << "TIME_LIMIT_SECONDS: " << parameters["TIME_LIMIT_SECONDS"] << std::endl;
    output_file << "ODD_HOLE_TERMINATION_PERCENTAGE: "
                << parameters["ODD_HOLE_TERMINATION_PERCENTAGE"] << std::endl;
    output_file << "N_THREADS: " << parameters["N_THREADS"] << std::endl;
    output_file << "IS_RUNNING_HEURISTIC_ON_CANDIDATE: "
                << parameters["IS_RUNNING_HEURISTIC_ON_CANDIDATE"] << std::endl;
    output_file << "IS_RUNNING_HEURISTIC_ON_RELAXATION: "
                << parameters["IS_RUNNING_HEURISTIC_ON_RELAXATION"] << std::endl;
    output_file << "ADD_ALL_AT_START: " << parameters["ADD_ALL_AT_START"] << std::endl;

    // Related to run
    output_file << "========================" << std::endl;
    output_file << "Wall-Clock Runtime (ms): " << program_stats.time_elapsed << std::endl;
    output_file << "Calculated Odd Hole Termination Batch Size: " << ODD_HOLE_TERMINATION_BATCH_SIZE
                << std::endl;
    output_file << "Total Time of All Workers in OddHoleAlg (nano): "
                << program_stats.time_spent_in_find_odd_holes << std::endl;
    output_file << "Total Time of All Workers in OddHoleAlg (ms): "
                << program_stats.time_spent_in_find_odd_holes / 1000000 << std::endl;
    output_file << "OddHoleAlg calls: " << program_stats.n_find_odd_hole_calls << std::endl;
    output_file << "Number of Found Odd-Holes: " << program_stats.n_odd_holes << std::endl;
    output_file << "Number of Found Odd Anti-Holes: " << program_stats.n_odd_anti_holes
                << std::endl;
    output_file << "Is Optimal? (1e-6): " << program_stats.is_optimal << std::endl;
    output_file << "Optimality Gap: " << program_stats.optimality_gap << std::endl;
    output_file << "Objective Value: " << program_stats.objective_value << std::endl;
    output_file << "Number of Feasible Solutions Found: " << program_stats.n_feas_sols_found
                << std::endl;
    output_file << "Number of Times Heuristic is Called: " << program_stats.n_times_heuristic_called
                << std::endl;
    output_file << "Total number of added OH and OHbar Cuts: "
                << program_stats.n_added_oh_ohbar_cuts << std::endl;
    output_file << "Number of Explored B&C tree nodes: " << number_of_explored_bc_nodes
                << std::endl;
    output_file << "Number of Unexplored B&C tree nodes: " << number_of_unexplored_bc_nodes
                << std::endl;
    output_file << "========================" << std::endl;

    // Related to input graph
    output_file << "Input Graph:" << std::endl;
    output_file << "Number of Vertices: " << order << std::endl;
    output_file << "Erdos Renyi p Parameter: " << inputGraph_p_Param << std::endl;
    output_file << "Number of Edges: " << m_edges_input << std::endl;
    output_file << "Expected No of Odd Holes: " << expected_no_of_odd_holes << std::endl;
    output_file << "Expected No of Odd Anti Holes: " << expected_no_of_odd_anti_holes << std::endl;
    output_file << "ODD_HOLE_TERMINATION_BATCH_SIZE: " << ODD_HOLE_TERMINATION_BATCH_SIZE
                << std::endl;
    output_file << "Graph Density: " << static_cast<double>(m_edges_input) / n_of_vertex_pairs
                << std::endl;
    for (int i = 0; i < order; ++i)
    {
        for (int j = 0; j < order; ++j)
        {
            output_file << input_graph[i][j] << ",";
        }
        output_file << std::endl;
    }

    // Output graph and the changes wrt input graph
    std::vector<std::string> changes;
    int mEdgesOutput = 0;
    std::vector<std::vector<bool>> outputGraph(order, std::vector<bool>(order, false));
    int nChangesRemoved = 0;
    int nChangesAdded = 0;
    for (int i = 0; i < order; ++i)
    {
        for (int j = 0; j < order; ++j)
        {
            if (i != j)
            {
                auto value = cplex.getValue(x_flat[flat_index_matrix[i][j]]);
                // std::cout << value << ",";
                outputGraph[i][j] = value;
                if (value)
                    mEdgesOutput++;
                if (value != input_graph[i][j] && i < j)
                {
                    if (value == 1)
                    {
                        changes.push_back("Added Edge: (" + std::to_string(i) + ", " +
                                          std::to_string(j) + ")");
                        nChangesAdded++;
                    }
                    else
                    {
                        changes.push_back("Removed Edge: (" + std::to_string(i) + ", " +
                                          std::to_string(j) + ")");
                        nChangesRemoved++;
                    }
                }
            }
        }
    }
    mEdgesOutput = mEdgesOutput / 2; // Because each edge is counted twice above.

    // Related to output graph
    output_file << "========================" << std::endl;
    output_file << "Output Graph: " << std::endl;
    output_file << "Number of Vertices: " << order << std::endl;
    output_file << "Number of Edges: " << mEdgesOutput << std::endl;
    output_file << "Graph Density: " << static_cast<double>(mEdgesOutput) / n_of_vertex_pairs
                << std::endl;
    for (int i = 0; i < order; ++i)
    {
        for (int j = 0; j < order; ++j)
        {
            output_file << outputGraph[i][j] << ",";
        }
        output_file << std::endl;
    }
    output_file << "Is Output Graph Empty or Complete?: " << is_graph_empty_or_complete(outputGraph)
                << std::endl;
    output_file << "Ratio of Added Edges Over Added + Removed Edges: "
                << static_cast<double>(nChangesAdded) / (nChangesAdded + nChangesRemoved + 1e-6)
                << std::endl;
    output_file << "Changes from Input to Output: " << std::endl;
    for (auto line : changes)
        output_file << line << std::endl;
    output_file << "========================" << std::endl;

    output_file.close();

    // Summary Output to experiment csv
    std::string summary_file_name = argv[4];
    std::ofstream summary_file;

    summary_file.open(summary_file_name, std::ios::app);
    if (!summary_file)
    {
        std::cerr << "Error opening summary_file" << std::endl;
        return 1;
    }
    std::string summary;
    // Format:
    // Write column names only if the file is empty
    if (summary_file.tellp() == 0)
    {
        summary_file << "input_graph_file_name,parameter_file_name,TIME_LIMIT_SECONDS,ODD_HOLE_TERMINATION_PERCENTAGE,N_THREADS,IS_RUNNING_HEURISTIC_ON_CANDIDATE,IS_RUNNING_HEURISTIC_ON_RELAXATION,ADD_ALL_AT_START,graphOrder,inputGraph_p_Param,ODD_HOLE_TERMINATION_BATCH_SIZE,wallclock(ms),timeInAlgo(ms),AlgoCalls,foundOddHoles,foundOddAntiHoles,is_optimal,optimality_gap,objective_value,n_feas_sols_found,n_times_heuristic_called,n_added_oh_ohbar_cuts,number_of_explored_bc_nodes,number_of_unexplored_bc_nodes,m_edges_input,inputGraphDensity,mEdgesOutput,outputGraphDensity,isOutputEmptyOrComplete,inputExpOHCount,inputExOHbarCount";
        summary_file << std::endl;
    }
    summary += input_graph_file_name + ",";
    summary += parameter_file_name + ",";
    summary += parameters["TIME_LIMIT_SECONDS"] + ",";
    summary += parameters["ODD_HOLE_TERMINATION_PERCENTAGE"] + ",";
    summary += parameters["N_THREADS"] + ",";
    summary += parameters["IS_RUNNING_HEURISTIC_ON_CANDIDATE"] + ",";
    summary += parameters["IS_RUNNING_HEURISTIC_ON_RELAXATION"] + ",";
    summary += parameters["ADD_ALL_AT_START"] + ",";
    // summary += parameters["HEURISTIC_INPUT_PRIORITY"] + ",";
    summary += std::to_string(order) + ",";
    summary += std::to_string(inputGraph_p_Param) + ",";
    summary += std::to_string(ODD_HOLE_TERMINATION_BATCH_SIZE) + ",";
    summary += std::to_string(program_stats.time_elapsed) + ",";
    summary += std::to_string(program_stats.time_spent_in_find_odd_holes / 1000000) + ",";
    summary += std::to_string(program_stats.n_find_odd_hole_calls) + ",";
    summary += std::to_string(program_stats.n_odd_holes) + ",";
    summary += std::to_string(program_stats.n_odd_anti_holes) + ",";
    summary += std::to_string(program_stats.is_optimal) + ",";
    summary += std::to_string(program_stats.optimality_gap) + ",";
    summary += std::to_string(program_stats.objective_value) + ",";
    summary += std::to_string(program_stats.n_feas_sols_found) + ",";
    summary += std::to_string(program_stats.n_times_heuristic_called) + ",";
    summary += std::to_string(program_stats.n_added_oh_ohbar_cuts) + ",";
    summary += std::to_string(number_of_explored_bc_nodes) + ",";
    summary += std::to_string(number_of_unexplored_bc_nodes) + ",";
    summary += std::to_string(m_edges_input) + ",";
    summary += std::to_string(static_cast<double>(m_edges_input) / n_of_vertex_pairs) + ",";
    summary += std::to_string(mEdgesOutput) + ",";
    summary += std::to_string(static_cast<double>(mEdgesOutput) / n_of_vertex_pairs) + ",";
    summary += std::to_string(is_graph_empty_or_complete(outputGraph)) + ",";
    summary += std::to_string(expected_no_of_odd_holes) + ",";
    summary += std::to_string(expected_no_of_odd_anti_holes);

    summary_file << summary << std::endl;

    summary_file.close();

    return 0;
}