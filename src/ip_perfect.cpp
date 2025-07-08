#include <iostream>
#include <vector>
#include <chrono>
#include <bitset>
#include <string>
#include <string>
#include <ilcplex/ilocplex.h>
#include "find_odd_holes.h"
#include "utils.h"

struct ip_run_info
{

    std::vector<std::vector<bool>> output_graph;
    std::string n_constraints;
    std::string n_oh_constraints;
    std::string n_oah_constraints;
    std::string objective;
    std::string status;
};


ip_run_info ip_perfect_min_edit(const std::vector<std::vector<bool>> &input_graph)
{

    int n = input_graph.size();
    IloEnv env;
    IloModel model(env);
    IloCplex cplex(model);
    int n_oh_constraints = 0;
    int n_oah_constraints = 0;

    std::unordered_map<uint64_t, IloBoolVar> x;

    for (int i = 0; i < (n - 1); i++)
    {
        for (int j = i + 1; j < n; j++)
        {
            auto key = pack_pair(i, j);
            x[key] = IloBoolVar(env);
            model.add(x[key]);
        }
    }

    // objective func:
    IloExpr obj_func(env);
    for (int i = 0; i < (n - 1); i++)
    {
        for (int j = i + 1; j < n; j++)
        {
            auto key = pack_pair(i, j);
            if (input_graph[i][j])
            {
                obj_func += (1 - x[key]);
            }
            else
            {
                obj_func += x[key];
            }
        }
    }
    model.add(IloMinimize(env, obj_func));
    obj_func.end();

    // Constraints
    // Loop over subsets:
    // Assumes graph order is maximum 64
    if (n > 64)
    {
        std::cerr << "Graph order is higher than 64!" << std::endl;
    }
    int counter = 0;
    for (uint64_t subset = 1; subset < (1ULL << n); subset++)
    {

        int subset_size = __builtin_popcountll(subset);
        if (subset_size % 2 == 0 || subset_size < 5)
        {
            continue;
        }

        // Get vertices in the subset to vec
        std::bitset<64> bits(subset);
        std::vector<int> subset_vec;
        subset_vec.reserve(n);
        for (int i = 0; i < 64; ++i)
        {
            if (bits[i])
            {
                subset_vec.push_back(i);
            }
        }

        // Get configurations according to the subset.
        // Configurations are basically permutations with two constraints:
        // 1. first element is the vertex with smallest index in the subset
        // 2. second element's index is smaller than last element's index

        auto min_it = std::min_element(subset_vec.begin(), subset_vec.end());
        int min_val = *min_it;
        // Sort to use next_permutation
        std::sort(subset_vec.begin(), subset_vec.end()); // to use next_permutation
        // Remove first element from the vector to permute the rest
        int min_vertex = subset_vec.front();
        subset_vec.erase(subset_vec.begin());

        do
        {
            // Check second < last
            if (subset_vec[0] > subset_vec.back())
            {
                continue;
            }
            std::vector<int> config;
            config.reserve(subset_vec.size() + 1);
            config.push_back(min_val);
            config.insert(config.end(), subset_vec.begin(), subset_vec.end());

            // We have the config at this point.
            // for(const auto &i: config)
            // {
            //     std::cout << i << " ";
            // }
            // std::cout << std::endl;
            // counter++;

            // Add odd hole constraint
            int config_length = config.size();
            IloExpr odd_hole_constr(env);
            for (int i = 0; i < config_length; i++)
            {
                for (int j = i + 1; j < config_length; j++)
                {
                    auto key = pack_pair(config[i], config[j]);

                    // consecutive
                    if (j - i == 1 || (i == 0 && j == (config_length - 1)))
                    {
                        odd_hole_constr += (1 - x[key]);
                    }
                    // non-consecutive
                    else
                    {
                        odd_hole_constr += x[key];
                    }
                }
            }
            model.add(odd_hole_constr >= 1);
            odd_hole_constr.end();
            n_oh_constraints++;

            // Add odd antihole constraint if length >= 7
            if (config_length > 5)
            {
                IloExpr odd_antihole_constr(env);
                for (int i = 0; i < config_length; i++)
                {
                    for (int j = i + 1; j < config_length; j++)
                    {
                        auto key = pack_pair(config[i], config[j]);

                        // consecutive
                        if (j - i == 1 || (i == 0 && j == (config_length - 1)))
                        {
                            odd_antihole_constr += x[key];
                        }
                        // non-consecutive
                        else
                        {
                            odd_antihole_constr += (1 - x[key]);
                        }
                    }
                }
                model.add(odd_antihole_constr >= 1);
                odd_antihole_constr.end();
                n_oah_constraints++;
            }

        } while (std::next_permutation(subset_vec.begin(), subset_vec.end()));
    }

    cplex.solve();

    ip_run_info info_object;

    info_object.status = std::to_string(cplex.getStatus());
    info_object.objective = std::to_string(cplex.getObjValue());
    info_object.n_constraints = std::to_string(n_oh_constraints + n_oah_constraints);
    info_object.n_oh_constraints = std::to_string(n_oh_constraints);
    info_object.n_oah_constraints = std::to_string(n_oah_constraints);

    // std::cout << "STATUS: " << cplex.getStatus() << std::endl;
    // std::cout << "OBJ VALUE: " << cplex.getObjValue() << std::endl;

    // Create output graph from decision variables.
    std::vector<std::vector<bool>> output_graph(n, std::vector<bool>(n, false));
    for (int i = 0; i < (n - 1); i++)
    {
        for (int j = i + 1; j < n; j++)
        {
            auto key = pack_pair(i, j);
            double val = cplex.getValue(x[key]);
            if(val > 0.5)
            {
                output_graph[i][j] = true;
                output_graph[j][i] = true;
            }
        }
    }

    info_object.output_graph = std::move(output_graph);

    return info_object;
}

int main(int argc, char *argv[])
{
    std::string input_graph_file_name = argv[1];
    std::string output_file_name = argv[2];
    std::string summary_file_name = argv[3];

    auto input_graph = read_graph_adj_matrix_from_txt_file(input_graph_file_name);

    graph_file_info graph_file_info_obj;
    bool parsed_input = parse_graph_filename(input_graph_file_name, graph_file_info_obj);
    if (!parsed_input)
    {
        std::cerr << "Couldn't parse input graph filename!" << std::endl;
    }

    auto time_start = std::chrono::high_resolution_clock::now();
    auto info_obj = ip_perfect_min_edit(input_graph);
    auto time_end = std::chrono::high_resolution_clock::now();
    long long time_elapsed =
        std::chrono::duration_cast<std::chrono::seconds>(time_end - time_start).count();

    // // Write output
    write_graph_to_file_given_filename(info_obj.output_graph, output_file_name);

    // Write summary
    std::ofstream summary_file;
    summary_file.open(summary_file_name, std::ios::app);
    if (!summary_file)
    {
        std::cerr << "Error opening summary_file" << std::endl;
        return 1;
    }
    // Write column names only if the file is empty
    if (summary_file.tellp() == 0)
    {
        summary_file << "input_file,output_file,input_type,input_order,input_density,"
                        "input_id,runtime_sec,is_perfect,objective_from_changes,objective_from_cplex,n_constraints,n_oh_constraints,n_oah_constraints,cplex_status";
        summary_file << std::endl;
    }//./run_ip_perfect.sh

    std::string summary_str;
    summary_str += input_graph_file_name + ",";
    summary_str += output_file_name + ",";
    summary_str += graph_file_info_obj.type + ",";
    summary_str += graph_file_info_obj.order + ",";
    summary_str += graph_file_info_obj.density + ",";
    summary_str += graph_file_info_obj.id + ",";
    summary_str += std::to_string(time_elapsed) + ",";
    summary_str += std::to_string(is_perfect(info_obj.output_graph)) + ",";
    summary_str += std::to_string(calculate_number_of_changes(input_graph, info_obj.output_graph)) + ",";
    summary_str += info_obj.objective + ",";
    summary_str += info_obj.n_constraints + ",";
    summary_str += info_obj.n_oh_constraints + ",";
    summary_str += info_obj.n_oah_constraints + ",";
    summary_str += info_obj.status;

    summary_file << summary_str << std::endl;

    summary_file.close();

    return 0;
}