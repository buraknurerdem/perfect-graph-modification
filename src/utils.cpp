#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <random>

#include "utils.h"

void print_graph(const std::vector<std::vector<bool>> &graph)
{
    for (const auto &row : graph)
    {
        for (const auto &entry : row)
        {
            std::cout << entry;
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
    return;
}

double expected_value_of_odd_holes(int n, double p)
{

    double expected_value = 0;

    for (int cycle_length = 5; cycle_length <= n; cycle_length += 2)
    {

        long double power_of_term_2 = (cycle_length * (((cycle_length - 1) / 2) - 1));
        // The code below is designed to do the term calculations without ending up with very big or
        // very small numbers
        long double term_total = 1;
        int t1 = n - cycle_length + 1; // partial factorial
        int t2 = 1;                    // power 1
        int t3 = 1;                    // power 2
        while (true)
        { // multiplying of the terms individually
            // term1: partial_factorial
            bool break_cond = true;
            if (t1 <= n)
            {
                term_total *= t1;
                t1++;
                break_cond = false;
            }
            if (t2 <= cycle_length)
            {
                term_total *= p;
                t2++;
                break_cond = false;
            }
            if (t3 <= power_of_term_2)
            {
                term_total *= (1 - p);
                t3++;
                break_cond = false;
            }
            if (break_cond)
            {
                break;
            }
        }
        expected_value += ((term_total / 2) / cycle_length);
    }
    return expected_value;
}

double expected_value_of_odd_anti_holes(int n, double p)
{

    double expected_value = 0;

    for (int cycle_length = 7; cycle_length <= n; cycle_length += 2)
    {

        long double power_of_term_2 = (cycle_length * (((cycle_length - 1) / 2) - 1));
        // The code below is designed to do the term calculations without ending up with very big or
        // very small numbers
        long double term_total = 1;
        int t1 = n - cycle_length + 1; // partial factorial
        int t2 = 1;                    // power 1
        int t3 = 1;                    // power 2
        while (true)
        { // multiplying of the terms individually
            // term1: partial_factorial
            bool break_cond = true;
            if (t1 <= n)
            {
                term_total *= t1;
                t1++;
                break_cond = false;
            }
            if (t2 <= cycle_length)
            {
                term_total *= (1 - p);
                t2++;
                break_cond = false;
            }
            if (t3 <= power_of_term_2)
            {
                term_total *= p;
                t3++;
                break_cond = false;
            }
            if (break_cond)
            {
                break;
            }
        }
        expected_value += ((term_total / 2) / cycle_length);
    }
    return expected_value;
}

bool is_graph_empty_or_complete(std::vector<std::vector<bool>> &graph)
{
    int graph_size = graph.size();

    bool is_complete = true;
    bool is_empty = true;
    // Checking only the upper triangular of adj. matrix
    for (int i = 0; i < graph_size - 1; i++)
    {
        for (int j = i + 1; j < graph_size; j++)
        {
            if (is_complete && !graph[i][j])
            {
                is_complete = false;
            }
            if (is_empty && graph[i][j])
            {
                is_empty = false;
            }
        }
    }
    if (is_complete || is_empty)
    {
        return true;
    }
    else
    {
        return false;
    }
}

std::string cycle_vector_to_string(std::vector<int> vec)
{

    std::sort(vec.begin(), vec.end());

    std::string cycle_string;

    for (const auto &v : vec)
    {
        cycle_string.append(std::to_string(v) + "_");
    }

    return cycle_string;
}

std::string get_string_rep_of_vp(int v1, int v2)
{
    std::string vp_str;
    if (v1 < v2)
    {
        vp_str = std::to_string(v1) + "_" + std::to_string(v2);
    }
    else
    {
        vp_str = std::to_string(v2) + "_" + std::to_string(v1);
    }
    return vp_str;
}

std::pair<int, int> get_two_integers_in_vp_string(std::string input)
{

    size_t underscorePos = input.find("_");

    std::string s1 = input.substr(0, underscorePos);
    std::string s2 = input.substr(underscorePos + 1);

    int v1 = std::stoi(s1);
    int v2 = std::stoi(s2);

    if (v1 < v2)
    {
        return std::pair<int, int>(v1, v2);
    }
    else
    {
        return std::pair<int, int>(v2, v1);
    }
}

std::vector<std::vector<bool>> get_complement_of_graph(const std::vector<std::vector<bool>> &graph)
{

    int n = graph.size();
    std::vector<std::vector<bool>> complement_graph(n, std::vector<bool>(n, false));

    for (int i = 0; i < n - 1; i++)
    {
        for (int j = 0; j < n; j++)
        {
            if (!graph[i][j] && i != j)
            {
                complement_graph[i][j] = 1;
                complement_graph[j][i] = 1;
            }
        }
    }

    return complement_graph;
}

std::vector<std::vector<bool>> read_graph_adj_matrix_from_txt_file(std::string graph_file_name)
{
    std::ifstream graph_file(graph_file_name);
    std::vector<std::vector<bool>> graph;

    if (!graph_file)
    {
        std::cerr << "Unable to open graph file!" << std::endl;
        return graph;
    }

    std::string line;
    while (std::getline(graph_file, line))
    {
        std::vector<bool> row;
        for (const char &c : line)
        {
            if (c == '0' || c == '1')
            {
                row.push_back(c == '1');
            }
        }
        graph.push_back(row);
    }

    graph_file.close();

    return graph;
}

double calculate_density(const std::vector<std::vector<bool>> &graph)
{

    if (graph.empty())
    {
        std::cerr << "Graph empty in calculate_density()" << std::endl;
    }

    int edge_count = 0;
    int n = graph.size();
    int vp = n * (n - 1) / 2;

    for (int i = 0; i < n; i++)
    {
        for (int j = i + 1; j < n; j++)
        {
            if (graph[i][j])
            {
                edge_count++;
            }
        }
    }

    double density = static_cast<double>(edge_count) / vp;
    return density;
}

std::vector<std::vector<int>>
get_adj_list_from_adj_matrix(const std::vector<std::vector<bool>> &graph)
{
    int n = graph.size();
    std::vector<std::vector<int>> graph_adj_list(n);

    for (int i = 0; i < n; ++i)
    {
        for (int j = i + 1; j < n; ++j)
        {
            if (graph[i][j])
            {
                graph_adj_list[i].push_back(j);
                graph_adj_list[j].push_back(i);
            }
        }
    }
    return graph_adj_list;
}

void print_odd_holes(const std::unordered_map<std::string, std::vector<int>> &odd_holes)
{
    std::cout << "Number of odd holes: " << odd_holes.size() << std::endl;

    if (!odd_holes.empty())
    {
        std::cout << "Printing odd-holes: " << std::endl;
        for (const auto &[key, cycle] : odd_holes)
        {
            for (const auto &v : cycle)
            {
                std::cout << v << ", ";
            }
            std::cout << std::endl;
        }
    }

    return;
}

int calculate_number_of_changes(const std::vector<std::vector<bool>> &g1,
                                const std::vector<std::vector<bool>> &g2)
{
    int n1 = g1.size();
    int n2 = g2.size();
    if (n1 != n2)
    {
        std::cerr << "Two graphs are not the same order in calculate_number_of_changes()!"
                  << std::endl;
    }

    int changes = 0;
    for (int i = 0; i < n1; i++)
    {
        for (int j = i + 1; j < n1; j++)
        {
            if (g1[i][j] != g2[i][j])
            {
                changes++;
            }
        }
    }
    return changes;
}

void remove_entries_containing_vp_from_odd_holes(
    std::unordered_map<std::string, std::vector<int>> &odd_holes, int v1, int v2)
{
    for (auto it = odd_holes.begin(); it != odd_holes.end();)
    {
        const std::vector<int> &values = it->second;
        if (std::find(values.begin(), values.end(), v1) != values.end() &&
            std::find(values.begin(), values.end(), v2) != values.end())
        {
            it = odd_holes.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

bool parse_graph_filename(const std::string &filepath, graph_file_info &graph_file_info_obj)
{

    size_t last_slash = filepath.find_last_of("/\\");
    std::string filename =
        (last_slash == std::string::npos) ? filepath : filepath.substr(last_slash + 1);

    if (filename.size() < 4 || filename.substr(filename.size() - 4) != ".txt")
        return false;
    std::string base = filename.substr(0, filename.size() - 4);

    size_t last_underscore = base.rfind('_');
    if (last_underscore == std::string::npos)
        return false;
    graph_file_info_obj.id = base.substr(last_underscore + 1);

    base = base.substr(0, last_underscore);
    last_underscore = base.rfind('_');
    if (last_underscore == std::string::npos)
        return false;
    graph_file_info_obj.density = base.substr(last_underscore + 1);

    base = base.substr(0, last_underscore);
    last_underscore = base.rfind('_');
    if (last_underscore == std::string::npos)
        return false;
    graph_file_info_obj.order = base.substr(last_underscore + 1);

    base = base.substr(0, last_underscore);
    last_underscore = base.rfind('_');
    if (last_underscore == std::string::npos)
        return false;
    graph_file_info_obj.type = base.substr(last_underscore + 1);

    return true;
}

bool is_matrix_symmetric(const std::vector<std::vector<bool>> &mat)
{
    int matrix_len = mat.size();
    for (const auto &row : mat)
    {
        int row_len = row.size();
        if (row_len != matrix_len)
        {
            return false;
        }
    }

    for (int i = 0; i < matrix_len; i++)
    {
        for (int j = 0; j < matrix_len; j++)
        {
            if (mat[i][j] != mat[j][i])
            {
                return false;
            }
        }
    }

    return true;
}

std::vector<int>
find_a_maximal_clique_including_a_given_vertex(const std::vector<std::vector<bool>> &graph, int v)
{
    std::vector<int> clique = {v};
    int n = graph.size();

    bool added = true;
    while (added)
    {
        added = false;
        for (int i = 0; i < n; i++)
        {
            // if i in clique, pass i
            if (std::find(clique.begin(), clique.end(), i) != clique.end())
            {
                continue;
            }
            // check if i adjacent to all vertices in the clique
            bool adjacent_to_all = true;
            for (int u : clique)
            {
                if (!graph[i][u])
                {
                    adjacent_to_all = false;
                    break;
                }
            }
            // if i adjacent to all and i not in clique, add i
            if (adjacent_to_all && std::find(clique.begin(), clique.end(), i) == clique.end())
            {
                clique.push_back(i);
                added = true;
                break;
            }
        }
    }

    return clique;
}

void write_graph_to_file(std::vector<std::vector<bool>> &graph,
                         std::string graph_folder,
                         std::string type,
                         int order,
                         std::string density_dec_str,
                         int id)
{
    int target_length = 5;

    std::ostringstream oss_n, oss_id;
    oss_n << std::setw(5) << std::setfill('0') << order;
    oss_id << std::setw(5) << std::setfill('0') << id;

    std::string n_str = oss_n.str();
    std::string id_str = oss_id.str();
    if (density_dec_str.length() < target_length)
    {
        density_dec_str.append(target_length - density_dec_str.length(), '0');
    }

    std::string graph_file_name = graph_folder + "/graph_" + type + "_" + n_str + "_" +
                                  density_dec_str + "_" + id_str + ".txt";

    // writing to txt
    std::ofstream graph_file(graph_file_name);
    for (int i = 0; i < order; ++i)
    {
        for (int j = 0; j < order; ++j)
        {
            graph_file << graph[i][j];
            if (j == order - 1)
            {
                graph_file << std::endl;
            }
        }
    }
    graph_file.close();
    return;
}

void write_graph_to_file_given_filename(std::vector<std::vector<bool>> &graph,
                                        std::string output_file_name)
{
    int order = graph.size();
    std::ofstream graph_file(output_file_name);
    for (int i = 0; i < order; ++i)
    {
        for (int j = 0; j < order; ++j)
        {
            graph_file << graph[i][j];
            if (j == order - 1)
            {
                graph_file << std::endl;
            }
        }
    }
    graph_file.close();
    return;
}

void remove_vertex_from_graph(std::vector<std::vector<bool>> &graph, int v)
{
    int n = graph.size();
    for (int i = 0; i < n; i++)
    {
        graph[i].erase(graph[i].begin() + v);
    }
    graph.erase(graph.begin() + v);
    return;
}

std::vector<int> select_k_without_replacement(int n, int k){

    if (k > n) {
        throw std::invalid_argument("k cannot be greater than the array size");
    }

    // Create a vector of integers from 0 to n
    std::vector<int> indices(n, 0);
    for(int i = 0; i < n; i++){
        indices[i] = i;
    }

    // Shuffle the vector
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(indices.begin(), indices.end(), gen);

    // Return the first k elements
    return std::vector<int>(indices.begin(), indices.begin() + k);

}