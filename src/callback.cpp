#include "utils.h"
#include "callback.h"
#include "iterative_modification_heuristic.h"

Generic_Callback::Generic_Callback(Problem *prb, Cutting_Plane_Parameters *params) : problem(prb), parameters(params)
{
    workers.resize(parameters->n_threads);
    for (int i = 0; i < parameters->n_threads; ++i)
    {
        workers[i] = new Odd_Hole_Worker(problem, parameters);
    }
    return;
}

Generic_Callback::~Generic_Callback()
{
    for(Odd_Hole_Worker* w: workers)
    {
        delete w;
    }
    workers.clear();
    return;
}

void Generic_Callback::invoke(const IloCplex::Callback::Context &context)
{

    // std::cout << "Generic_Callback::invoke has been called" << std::endl;

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
        context.getCandidatePoint(problem->x_flat, x_val);

        // create the graph corresponding to the current solution
        std::vector<std::vector<bool>> candidate_graph = problem->input_graph;
        for (int ind = 0; ind < (problem->x_flat.getSize()); ++ind)
        {
            int i = problem->x_flat[ind].getIntProperty("i");
            int j = problem->x_flat[ind].getIntProperty("j");
            if (x_val[ind] == 1)
            {
                candidate_graph[i][j] = true;
                candidate_graph[j][i] = true;
            }
            else
            {
                candidate_graph[i][j] = false;
                candidate_graph[j][i] = false;
            }
        }
        // construct its complement complement
        std::vector<std::vector<bool>> complement_graph = get_complement_of_graph(candidate_graph);

        Odd_Hole_Worker *p_worker = workers[thread_no];

        IloRangeArray cuts(env);

        auto found_odd_holes = p_worker->call_find_odd_holes(candidate_graph, cuts, false);
        auto found_odd_anti_holes = p_worker->call_find_odd_holes(complement_graph, cuts, true);

        if ((found_odd_holes.size() > 0) || (found_odd_anti_holes.size() > 0))
        {
            // add constraints, reject current candidate solution
            context.rejectCandidate(cuts);

            // Run heuristic below
            if (parameters->heur_on_c)
            {

                p_worker->n_worker_heuristic_called++;

                // heuristic_stats heuristic_stats_obj =
                //     heuristic02_base_graph(input_graph, candidate_graph, HEURISTIC_INPUT_PRIORITY);
                heuristic_stats heuristic_stats_obj = problem->run_heuristic(candidate_graph);


                if (heuristic_stats_obj.is_successful)
                {
                    p_worker->n_worker_found_feas_sols++;

                    // Generate necessary cplex objects.
                    IloNumArray heuristic_x_val(env, x_val.getSize());
                    double heuristic_obj_val = 0;
                    for (int i = 0; i < problem->order - 1; i++)
                    {
                        for (int j = i + 1; j < problem->order; j++)
                        {
                            heuristic_x_val[problem->flat_index_matrix[i][j]] = heuristic_stats_obj.output_graph[i][j];
                            if (problem->input_graph[i][j] != heuristic_stats_obj.output_graph[i][j])
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
                                  << heuristic_obj_val << ". Prev. Best Sol: " << node_best_sol_obj_info
                                  << std::endl;
                        context.postHeuristicSolution(
                            problem->x_flat, heuristic_x_val, heuristic_obj_val,
                            IloCplex::Callback::Context::SolutionStrategy::NoCheck
                        );
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
    if (parameters->heur_on_r && context_id == IloCplex::Callback::Context::Id::Relaxation)
    {
        auto node_count_info = context.getIntInfo(IloCplex::Callback::Context::Info::NodeCount);

        // Run heuristic for every tenth node of b&c tree
        if (node_count_info % 10 != 0)
        {
            return;
        }

        // std::cout << "=================================" << std::endl;
        // std::cout << "In relaxation callback. Node ID: " << node_count_info << std::endl;

        // Get the current fractional solution
        IloNumArray x_val(env);

        context.getRelaxationPoint(problem->x_flat, x_val);

        // find a graph according to the fractional solution
        std::vector<std::vector<bool>> candidate_graph = problem->input_graph;
        for (int ind = 0; ind < (problem->x_flat.getSize()); ++ind)
        {
            int i = problem->x_flat[ind].getIntProperty("i");
            int j = problem->x_flat[ind].getIntProperty("j");
            if (x_val[ind] > 0.5)
            {
                candidate_graph[i][j] = true;
                candidate_graph[j][i] = true;
            }
            else
            {
                candidate_graph[i][j] = false;
                candidate_graph[j][i] = false;
            }
        }

        Odd_Hole_Worker *p_worker = workers[thread_no];

        // call heuristic on this graph.
        // heuristic_stats heuristic_stats_obj =
        //     heuristic02_base_graph(input_graph, candidate_graph, HEURISTIC_INPUT_PRIORITY);
        heuristic_stats heuristic_stats_obj = problem->run_heuristic(candidate_graph);

        p_worker->n_worker_heuristic_called++;

        if (heuristic_stats_obj.is_successful)
        {
            p_worker->n_worker_found_feas_sols++;

            // Generate necessary cplex objects.
            IloNumArray heuristic_x_val(env, x_val.getSize());
            double heuristic_obj_val = 0;
            for (int i = 0; i < problem->order - 1; i++)
            {
                for (int j = i + 1; j < problem->order; j++)
                {
                    heuristic_x_val[problem->flat_index_matrix[i][j]] = heuristic_stats_obj.output_graph[i][j];
                    if (problem->input_graph[i][j] != heuristic_stats_obj.output_graph[i][j])
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
                std::cout << "In Relaxation Callback. Heuristic solution found with objective value: "
                          << heuristic_obj_val << ". Prev. Best Sol: " << node_best_sol_obj_info << std::endl;
                context.postHeuristicSolution(
                    problem->x_flat, heuristic_x_val, heuristic_obj_val,
                    IloCplex::Callback::Context::SolutionStrategy::NoCheck
                );
            }
        }
    }
}

