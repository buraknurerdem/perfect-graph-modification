#include <iostream>
#include <fstream>

#include "find_odd_holes.h"
#include "iterative_modification_heuristic.h"
#include "utils.h"


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

    auto h_stats = iterative_modification_heuristic(input_graph);

    auto output_graph = h_stats.output_graph;

    // Write output
    write_graph_to_file_given_filename(output_graph, output_file_name);

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
                        "input_id,runtime_sec,is_heur_successful,is_perfect,"
                        "heuristic_objective,n_inner_loops";
        summary_file << std::endl;
    }

    std::string summary_str;
    summary_str += input_graph_file_name + ",";
    summary_str += output_file_name + ",";
    summary_str += graph_file_info_obj.type + ",";
    summary_str += graph_file_info_obj.order + ",";
    summary_str += graph_file_info_obj.density + ",";
    summary_str += graph_file_info_obj.id + ",";
    summary_str += std::to_string(h_stats.runtime) + ",";
    summary_str += std::to_string(h_stats.is_successful) + ",";
    summary_str += std::to_string(is_perfect(output_graph)) + ",";
    summary_str += std::to_string(calculate_number_of_changes(input_graph, output_graph)) + ",";
    summary_str += std::to_string(h_stats.number_of_inner_loops);

    summary_file << summary_str << std::endl;

    summary_file.close();

    return 0;
}