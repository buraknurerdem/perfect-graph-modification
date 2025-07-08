#include "find_odd_holes.h"
#include "utils.h"

int main(int argc, char *argv[])
{
    std::string input_graph_file_name = argv[1];
    std::string is_verbose = argv[2];
    bool verbose = (is_verbose == "1") ? true : false;
    auto input_graph = read_graph_adj_matrix_from_txt_file(input_graph_file_name);
    bool result = is_perfect(input_graph);

    if (verbose && result)
        std::cout << "Perfect : " << input_graph_file_name << std::endl;
    else if (verbose && !result)
        std::cout << "Not Perfect : " << input_graph_file_name << std::endl;

    if (result)
        return 0;
    else
        return 1;
}