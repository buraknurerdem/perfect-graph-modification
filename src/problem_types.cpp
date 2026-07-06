#include "problem_types.h"
#include "find_odd_holes.h"
#include "utils.h"

//////////////////
// Problem_Edit //
//////////////////

Problem_Edit::Problem_Edit(IloEnv e, IloModel m, IloCplex c, std::string s) : Problem(e, m, c, s)
{

    // Read Input Graph. e_ij : Input graph's adjacency matrix
    input_graph = read_graph_adj_matrix_from_txt_file(input_graph_file_name);

    order = input_graph.size();

    // Parsing input_graph_file_name to get p parameter
    // inputs/inputGraph_order_20_density_0.750000_id_1.txt
    std::size_t pos = input_graph_file_name.find("density_");
    if (pos != std::string::npos)
    {
        std::string p_str = input_graph_file_name.substr(pos + 8); // 8 is the length of "density_"

        std::size_t end_pos = p_str.find_first_not_of("0123456789.");
        if (end_pos != std::string::npos)
        {
            input_graph_p = std::stod(p_str.substr(0, end_pos));
        }
    }
    else
    {
        std::cout << "p_parameter not found in the input_graph_file_name" << std::endl;
    }

    input_edge_count = 0;
    for (int i = 0; i < order; i++)
    {
        for (int j = i + 1; j < order; j++)
        {
            if (input_graph[i][j])
            {
                input_edge_count++;
            }
        }
    }
    n_of_vertex_pairs = ((order * order) - order) / 2; // n choose 2
    input_graph_density = static_cast<double>(input_edge_count) / n_of_vertex_pairs;
    flat_index_matrix.assign(order, std::vector<int>(order, 0));

    return;
}

void Problem_Edit::create_decision_vars()
{
    x_flat = IloNumVarArray(env, n_of_vertex_pairs);
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
            temp_index++;
        }
    }

    return;
}

void Problem_Edit::set_objective()
{

    IloExpr objective_exp(env);
    for (int i = 0; i < order; ++i)
    {
        for (int j = i + 1; j < order; ++j)
        {
            // adding to objective expression according to edge/nonedge existence
            if (input_graph[i][j])
            {
                objective_exp += (1 - x_flat[flat_index_matrix[i][j]]);
            }
            else
            {
                objective_exp += x_flat[flat_index_matrix[i][j]];
            }
        }
    }

    // Adding Objective to the Model
    IloObjective objective = IloMinimize(env, objective_exp);
    model.add(objective);
    objective_exp.end();

    return;
}

void Problem_Edit::add_odd_hole_constraint(IloRangeArray &cuts, const std::vector<int> &hole)
{
    IloExpr hole_constraint(env);
    for (size_t i = 0; i < hole.size(); i++)
    {
        for (size_t j = i + 1; j < hole.size(); j++)
        {
            if ((j == i + 1) || (i == 0 && j == (hole.size() - 1)))
            { // edge on the cycle
                hole_constraint += (1 - x_flat[flat_index_matrix[hole[i]][hole[j]]]);
            }
            else
            {
                hole_constraint += x_flat[flat_index_matrix[hole[i]][hole[j]]];
            }
        }
    }
    cuts.add(hole_constraint >= 1);
    hole_constraint.end();

    return;
}

void Problem_Edit::add_odd_antihole_constraint(IloRangeArray &cuts, const std::vector<int> &hole)
{
    IloExpr hole_constraint(env);
    for (size_t i = 0; i < hole.size(); i++)
    {
        for (size_t j = i + 1; j < hole.size(); j++)
        {
            if ((j == i + 1) || (i == 0 && j == (hole.size() - 1)))
            { // edge on the cycle
                hole_constraint += x_flat[flat_index_matrix[hole[i]][hole[j]]];
            }
            else
            {
                hole_constraint += (1 - x_flat[flat_index_matrix[hole[i]][hole[j]]]);
            }
        }
    }
    cuts.add(hole_constraint >= 1);
    hole_constraint.end();

    return;
}

void Problem_Edit::write_output(std::ofstream &output_file)
{
    // write info related to input graph
    output_file << "========================" << std::endl;
    output_file << "Input Graph:" << std::endl;
    output_file << "Number of Vertices: " << order << std::endl;
    output_file << "Erdos Renyi p Parameter: " << input_graph_p << std::endl;
    output_file << "Number of Edges: " << input_edge_count << std::endl;
    output_file << "Graph Density: " << input_graph_density << std::endl;

    // write input graph adjacency matrix
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
    output_edge_count = 0;
    output_graph.assign(order, std::vector<bool>(order, false));
    int nChangesRemoved = 0;
    int nChangesAdded = 0;
    for (int i = 0; i < order; ++i)
    {
        for (int j = 0; j < order; ++j)
        {
            if (i != j)
            {
                auto value = cplex.getValue(x_flat[flat_index_matrix[i][j]]);
                output_graph[i][j] = value;
                if (value)
                    output_edge_count++;
                if (value != input_graph[i][j] && i < j)
                {
                    if (value == 1)
                    {
                        changes.push_back(
                            "Added Edge: (" + std::to_string(i) + ", " + std::to_string(j) + ")"
                        );
                        nChangesAdded++;
                    }
                    else
                    {
                        changes.push_back(
                            "Removed Edge: (" + std::to_string(i) + ", " + std::to_string(j) + ")"
                        );
                        nChangesRemoved++;
                    }
                }
            }
        }
    }
    output_edge_count = output_edge_count / 2; // Because each edge is counted twice above.
    output_graph_density = static_cast<double>(output_edge_count) / n_of_vertex_pairs;

    // Related to output graph
    output_file << "========================" << std::endl;
    output_file << "Output Graph: " << std::endl;
    output_file << "Number of Vertices: " << order << std::endl;
    output_file << "Number of Edges: " << output_edge_count << std::endl;
    output_file << "Graph Density: " << output_graph_density << std::endl;
    for (int i = 0; i < order; ++i)
    {
        for (int j = 0; j < order; ++j)
        {
            output_file << output_graph[i][j] << ",";
        }
        output_file << std::endl;
    }
    output_file << "Is Output Graph Empty or Complete?: " << is_graph_empty_or_complete(output_graph)
                << std::endl;
    output_file << "Ratio of Added Edges Over Added + Removed Edges: "
                << static_cast<double>(nChangesAdded) / (nChangesAdded + nChangesRemoved + 1e-6) << std::endl;
    output_file << "Changes from Input to Output: " << std::endl;
    for (auto line : changes)
        output_file << line << std::endl;
    output_file << "========================" << std::endl;

    return;
}

void Problem_Edit::write_summary(std::ofstream &summary_file)
{
    summary_file << std::to_string(order) + ",";
    summary_file << std::to_string(input_graph_p) + ",";
    summary_file << std::to_string(input_edge_count) + ",";
    summary_file << std::to_string(input_graph_density) + ",";
    summary_file << std::to_string(output_edge_count) + ",";
    summary_file << std::to_string(output_graph_density) + ",";
    summary_file << std::to_string(is_graph_empty_or_complete(output_graph));
    // CAREFUL, There shouldn't be a comma at last entry

    return;
}

heuristic_stats Problem_Edit::run_heuristic(std::vector<std::vector<bool>> &graph)
{
    auto results = iterative_modification_heuristic(graph);
    return results;
}

////////////////////////
// Problem_Completion //
////////////////////////

Problem_Completion::Problem_Completion(IloEnv e, IloModel m, IloCplex c, std::string s) : Problem(e, m, c, s)
{

    // Read Input Graph. e_ij : Input graph's adjacency matrix
    input_graph = read_graph_adj_matrix_from_txt_file(input_graph_file_name);

    order = input_graph.size();

    // Parsing input_graph_file_name to get p parameter
    // inputs/inputGraph_order_20_density_0.750000_id_1.txt
    std::size_t pos = input_graph_file_name.find("density_");
    if (pos != std::string::npos)
    {
        std::string p_str = input_graph_file_name.substr(pos + 8); // 8 is the length of "density_"

        std::size_t end_pos = p_str.find_first_not_of("0123456789.");
        if (end_pos != std::string::npos)
        {
            input_graph_p = std::stod(p_str.substr(0, end_pos));
        }
    }
    else
    {
        std::cout << "p_parameter not found in the input_graph_file_name" << std::endl;
    }

    input_edge_count = 0;
    for (int i = 0; i < order; i++)
    {
        for (int j = i + 1; j < order; j++)
        {
            if (input_graph[i][j])
            {
                input_edge_count++;
            }
        }
    }
    n_of_vertex_pairs = ((order * order) - order) / 2; // n choose 2
    input_graph_density = static_cast<double>(input_edge_count) / n_of_vertex_pairs;
    flat_index_matrix.assign(order, std::vector<int>(order, 0));
    input_non_edge_count = n_of_vertex_pairs - input_edge_count;

    return;
}

void Problem_Completion::create_decision_vars()
{
    x_flat = IloNumVarArray(env, input_non_edge_count);
    int temp_index = 0;
    for (int i = 0; i < order; ++i)
    {
        for (int j = i + 1; j < order; ++j)
        {
            if (!input_graph[i][j])
            {
                flat_index_matrix[i][j] = temp_index;
                flat_index_matrix[j][i] = temp_index;
                x_flat[temp_index] = IloNumVar(env, 0.0, 1.0, ILOBOOL);
                x_flat[temp_index].setIntProperty("i", i);
                x_flat[temp_index].setIntProperty("j", j);
                model.add(x_flat[temp_index]);
                temp_index++;
            }
        }
    }

    return;
}

void Problem_Completion::set_objective()
{
    IloExpr objective_exp(env);

    int temp_index = 0;
    for (int i = 0; i < order; ++i)
    {
        for (int j = i + 1; j < order; ++j)
        {
            if (!input_graph[i][j])
            {
                objective_exp += x_flat[flat_index_matrix[i][j]];
            }
        }
    }

    // Adding Objective to the Model
    IloObjective objective = IloMinimize(env, objective_exp);
    model.add(objective);
    objective_exp.end();

    return;
}

void Problem_Completion::add_odd_hole_constraint(IloRangeArray &cuts, const std::vector<int> &hole)
{
    IloExpr hole_constraint(env);
    for (size_t i = 0; i < hole.size(); i++)
    {
        for (size_t j = i + 1; j < hole.size(); j++)
        {
            // skip vertex pair if it is an edge in the input graph
            if (input_graph[hole[i]][hole[j]])
                continue;

            if ((j == i + 1) || (i == 0 && j == (hole.size() - 1)))
            { // edge on the cycle
                hole_constraint += (1 - x_flat[flat_index_matrix[hole[i]][hole[j]]]);
            }
            else
            {
                hole_constraint += x_flat[flat_index_matrix[hole[i]][hole[j]]];
            }
        }
    }
    cuts.add(hole_constraint >= 1);
    hole_constraint.end();

    return;
}

void Problem_Completion::add_odd_antihole_constraint(IloRangeArray &cuts, const std::vector<int> &hole)
{
    IloExpr hole_constraint(env);
    for (size_t i = 0; i < hole.size(); i++)
    {
        for (size_t j = i + 1; j < hole.size(); j++)
        {
            // skip vertex pair if it is an edge in the input graph
            if (input_graph[hole[i]][hole[j]])
                continue;

            if ((j == i + 1) || (i == 0 && j == (hole.size() - 1)))
            { // edge on the cycle
                hole_constraint += x_flat[flat_index_matrix[hole[i]][hole[j]]];
            }
            else
            {
                hole_constraint += (1 - x_flat[flat_index_matrix[hole[i]][hole[j]]]);
            }
        }
    }
    cuts.add(hole_constraint >= 1);
    hole_constraint.end();

    return;
}

void Problem_Completion::write_output(std::ofstream &output_file)
{
    // write info related to input graph
    output_file << "========================" << std::endl;
    output_file << "Input Graph:" << std::endl;
    output_file << "Number of Vertices: " << order << std::endl;
    output_file << "Erdos Renyi p Parameter: " << input_graph_p << std::endl;
    output_file << "Number of Edges: " << input_edge_count << std::endl;
    output_file << "Graph Density: " << input_graph_density << std::endl;

    // write input graph adjacency matrix
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
    output_edge_count = 0;
    output_graph = input_graph;
    int nChangesRemoved = 0;
    int nChangesAdded = 0;

    // Get output graph
    for (size_t ind = 0; ind < x_flat.getSize(); ind++)
    {
        int i = x_flat[ind].getIntProperty("i");
        int j = x_flat[ind].getIntProperty("j");

        output_graph[i][j] = cplex.getValue(x_flat[ind]);
        output_graph[j][i] = cplex.getValue(x_flat[ind]);
    }

    // Check changes in the output graph wrt the input graph
    for (int i = 0; i < order; ++i)
    {
        for (int j = i + 1; j < order; ++j)
        {
            if(output_graph[i][j]) output_edge_count++;
            
            if (output_graph[i][j] != input_graph[i][j])
            {
                if (output_graph[i][j] == 1)
                {
                    changes.push_back(
                        "Added Edge: (" + std::to_string(i) + ", " + std::to_string(j) + ")"
                    );
                    nChangesAdded++;
                }
                else
                {
                    std::cerr << "PROBLEM. Removed edge in output graph! input path: " << input_graph_file_name << std::endl;
                    changes.push_back(
                        "Removed Edge: (" + std::to_string(i) + ", " + std::to_string(j) + ")"
                    );
                    nChangesRemoved++;
                }
            }
        }
    }
    output_graph_density = static_cast<double>(output_edge_count) / n_of_vertex_pairs;

    // Related to output graph
    output_file << "========================" << std::endl;
    output_file << "Output Graph: " << std::endl;
    output_file << "Number of Vertices: " << order << std::endl;
    output_file << "Number of Edges: " << output_edge_count << std::endl;
    output_file << "Graph Density: " << output_graph_density << std::endl;
    for (int i = 0; i < order; ++i)
    {
        for (int j = 0; j < order; ++j)
        {
            output_file << output_graph[i][j] << ",";
        }
        output_file << std::endl;
    }
    output_file << "Is Output Graph Empty or Complete?: " << is_graph_empty_or_complete(output_graph)
                << std::endl;
    output_file << "Ratio of Added Edges Over Added + Removed Edges: "
                << static_cast<double>(nChangesAdded) / (nChangesAdded + nChangesRemoved + 1e-6) << std::endl;
    output_file << "Changes from Input to Output: " << std::endl;
    for (auto line : changes)
        output_file << line << std::endl;
    output_file << "========================" << std::endl;

    return;
}

void Problem_Completion::write_summary(std::ofstream &summary_file)
{
    summary_file << std::to_string(order) + ",";
    summary_file << std::to_string(input_graph_p) + ",";
    summary_file << std::to_string(input_edge_count) + ",";
    summary_file << std::to_string(input_graph_density) + ",";
    summary_file << std::to_string(output_edge_count) + ",";
    summary_file << std::to_string(output_graph_density) + ",";
    summary_file << std::to_string(is_graph_empty_or_complete(output_graph));
    // CAREFUL, There shouldn't be a comma at last entry

    return;
}

heuristic_stats Problem_Completion::run_heuristic(std::vector<std::vector<bool>> &graph)
{
    auto results = iterative_modification_heuristic_completion(input_graph, graph);
    return results;
}

