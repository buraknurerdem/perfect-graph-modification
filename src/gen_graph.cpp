#include <format>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "utils.h"
#include "iterative_modification_heuristic.h"


void complete_join(std::vector<std::vector<bool>> &g1, const std::vector<std::vector<bool>> &g2)
{
    int n1 = g1.size();
    int n2 = g2.size();

    for (int i = 0; i < n1; i++)
    {
        for (int j = 0; j < n2; j++)
        {
            g1[i].push_back(true);
        }
    }

    for (int j = 0; j < n2; j++)
    {
        std::vector<bool> temp_vect(n1 + n2, true);
        for (int i = 0; i < n2; i++)
        {
            if (!g2[i][j])
            {
                temp_vect[n1 + i] = false;
            }
        }
        g1.push_back(temp_vect);
    }

    return;
}

void disjoint_union(std::vector<std::vector<bool>> &g1, const std::vector<std::vector<bool>> &g2)
{
    int n1 = g1.size();
    int n2 = g2.size();

    for (int i = 0; i < n1; i++)
    {
        for (int j = 0; j < n2; j++)
        {
            g1[i].push_back(false);
        }
    }

    for (int j = 0; j < n2; j++)
    {
        std::vector<bool> temp_vect(n1 + n2, false);
        for (int i = 0; i < n2; i++)
        {
            if (g2[i][j])
            {
                temp_vect[n1 + i] = true;
            }
        }
        g1.push_back(temp_vect);
    }

    return;
}

void operation_substitution(std::vector<std::vector<bool>> &g1,
                            const std::vector<std::vector<bool>> &g2,
                            std::mt19937 &gen)
{
    int n1 = g1.size();
    int n2 = g2.size();

    std::uniform_int_distribution<> distrib(0, n1 - 1);

    // random vertex of v
    int v = distrib(gen);

    // neighbors of v
    std::vector<int> neighbors_v;
    for (int i = 0; i < n1; i++)
    {
        if (g1[v][i])
        {
            neighbors_v.push_back(i);
        }
    }

    // firstly, use disjoint union to merge adjacency matrices:
    disjoint_union(g1, g2);

    // create adjacencies between vertices of g2 and neighbors_v
    for (int i = n1; i < (n1 + n2); i++)
    {
        for (int u : neighbors_v)
        {
            g1[i][u] = true;
            g1[u][i] = true;
        }
    }

    // remove v from g1
    for (int i = 0; i < (n1 + n2); i++)
    {
        g1[i].erase(g1[i].begin() + v);
    }
    g1.erase(g1.begin() + v);

    return;
}

void operation_composition(std::vector<std::vector<bool>> &g1,
                           const std::vector<std::vector<bool>> &g2,
                           std::mt19937 &gen)
{
    int n1 = g1.size();
    int n2 = g2.size();

    if (n1 < 3 || n2 < 3)
    {
        return;
    }

    std::uniform_int_distribution<> distrib1(0, n1 - 1);
    std::uniform_int_distribution<> distrib2(0, n2 - 1);

    // random vertices from g1 and g2
    int v1 = distrib1(gen);
    int v2 = distrib2(gen);

    // neighbors of v1 and v2
    std::vector<int> neighbors_v1;
    std::vector<int> neighbors_v2;
    for (int i = 0; i < n1; i++)
    {
        if (g1[v1][i])
        {
            neighbors_v1.push_back(i);
        }
    }
    for (int i = 0; i < n2; i++)
    {
        if (g2[v2][i])
        {
            // here we add n1 to the vertex label since the two graphs will be
            // merged later and vertex labels (indices) of g2 will increase as much as n1
            neighbors_v2.push_back(i + n1);
        }
    }

    // first use disjoint union to merge adjacency matrices:
    disjoint_union(g1, g2);

    // create adjacencies between vertices of neighbors_v1 and neighbors_v2
    for (int u1 : neighbors_v1)
    {
        for (int u2 : neighbors_v2)
        {
            g1[u1][u2] = true;
            g1[u2][u1] = true;
        }
    }

    // remove v1 and v2 from g2. new label of v2 in g1 is (v2+n1)
    //  first remove v2 since (v2+n1) > v1
    for (int i = 0; i < (n1 + n2); i++)
    {
        g1[i].erase(g1[i].begin() + v2 + n1);
    }
    g1.erase(g1.begin() + v2 + n1);
    for (int i = 0; i < (n1 + n2 - 1); i++)
    {
        g1[i].erase(g1[i].begin() + v1);
    }
    g1.erase(g1.begin() + v1);
}

void operation_clique_identification(std::vector<std::vector<bool>> &g1,
                                     const std::vector<std::vector<bool>> &g2,
                                     std::mt19937 &gen)
{
    int n1 = g1.size();
    int n2 = g2.size();

    std::uniform_int_distribution<> distrib1(0, n1 - 1);
    std::uniform_int_distribution<> distrib2(0, n2 - 1);

    // random vertices from g1 and g2
    int v1 = distrib1(gen);
    int v2 = distrib2(gen);

    // find maximal cliques including the selected vertices
    auto clique1 = find_a_maximal_clique_including_a_given_vertex(g1, v1);
    auto clique2 = find_a_maximal_clique_including_a_given_vertex(g2, v2);

    // find the bigger clique, shuffle it, downsize it to the clique of the lesser size
    int c1_size = clique1.size();
    int c2_size = clique2.size();

    if (c1_size == 0 || c2_size == 0)
    {
        std::cerr << "Clique size 0!" << std::endl;
    }

    int new_clique_size;
    if (c1_size >= c2_size)
    {
        std::shuffle(clique1.begin(), clique1.end(), gen);
        clique1.resize(c2_size);
        new_clique_size = c2_size;
    }
    else
    {
        std::shuffle(clique2.begin(), clique2.end(), gen);
        clique2.resize(c1_size);
        new_clique_size = c1_size;
    }

    // disjoint union to merge adjacency matrices:
    disjoint_union(g1, g2);

    // readjust the labels of clique2
    for (int i = 0; i < new_clique_size; i++)
    {
        clique2[i] += n1;
    }

    // a one to one mapping is carried out on indices.
    // since a clique is shuffled, there is no bias towards smaller indices
    // connect a clique1's vertex to neighbors of corresponding vertex in clique2
    for (int i = 0; i < new_clique_size; i++)
    {
        int u1 = clique1[i];
        int u2 = clique2[i];

        // find neighbors of u2, connect them to u1
        for (int j = n1; j < (n1 + n2); j++)
        {
            if (g1[j][u2])
            {
                g1[j][u1] = true;
                g1[u1][j] = true;
            }
        }
    }

    // remove clique2 from g1. first order decreasing
    // then iteratively remove the last vertex in clique2
    std::sort(clique2.begin(), clique2.end());
    for (int j = new_clique_size - 1; j >= 0; j--)
    {
        int vertex_to_remove = clique2[j];
        clique2.pop_back();

        // remove vertex_to_remove from g1
        for (int i = 0; i < (n1 + n2 - new_clique_size + j + 1); i++)
        {
            g1[i].erase(g1[i].begin() + vertex_to_remove);
        }
        g1.erase(g1.begin() + vertex_to_remove);
    }

    return;
}

enum operation_type
{
    COMPLETE_JOIN,
    DISJOINT_UNION,
    OPERATION_SUBSTITUTION,
    OPERATION_COMPOSITION,
    OPERATION_CLIQUE_ID,
    COMPLEMENT
};

std::vector<std::vector<bool>>
gen_perfect_via_operations(int n, double d, double eps, std::string folder_path)
{

    std::random_device rd;
    std::mt19937 gen(rd());

    // pick a random operation.
    // IF NEW OPERATION IS ADDED, UPDATE THIS DISTRIBUTION
    std::uniform_int_distribution<> dist_operations(0, 5);

    // iterate through the folder and collect graph files
    std::vector<std::filesystem::directory_entry> files;
    for (const auto &entry : std::filesystem::directory_iterator(folder_path))
    {
        if (entry.is_regular_file())
        {
            files.push_back(entry);
        }
    }

    std::uniform_int_distribution<> dist_files(0, files.size() - 1);

    // read a external perfect graph
    const auto &random_file = files[dist_files(gen)];
    auto graph = read_graph_adj_matrix_from_txt_file(random_file.path());

    int counter = 0;
    while (true)
    {
        operation_type random_operation = static_cast<operation_type>(dist_operations(gen));

        switch (random_operation)
        {
        case COMPLETE_JOIN:
        {
            auto new_random_file = files[dist_files(gen)];
            auto graph_ext = read_graph_adj_matrix_from_txt_file(new_random_file.path());
            complete_join(graph, graph_ext);
            break;
        }
        case DISJOINT_UNION:
        {
            auto new_random_file = files[dist_files(gen)];
            auto graph_ext = read_graph_adj_matrix_from_txt_file(new_random_file.path());
            disjoint_union(graph, graph_ext);
            break;
        }
        case OPERATION_SUBSTITUTION:
        {
            auto new_random_file = files[dist_files(gen)];
            auto graph_ext = read_graph_adj_matrix_from_txt_file(new_random_file.path());
            operation_substitution(graph, graph_ext, gen);
            break;
        }
        case OPERATION_COMPOSITION:
        {
            auto new_random_file = files[dist_files(gen)];
            auto graph_ext = read_graph_adj_matrix_from_txt_file(new_random_file.path());
            operation_composition(graph, graph_ext, gen);
            break;
        }
        case OPERATION_CLIQUE_ID:
        {
            auto new_random_file = files[dist_files(gen)];
            auto graph_ext = read_graph_adj_matrix_from_txt_file(new_random_file.path());
            operation_clique_identification(graph, graph_ext, gen);
            break;
        }
        case COMPLEMENT:
        {
            graph = get_complement_of_graph(graph);
            break;
        }
        }

        if (graph.size() >= n)
        {

            // It is safe to remove any vertex, since perfectness is hereditary
            // Remove a random vertex until graph size is n
            while (graph.size() > n)
            {
                int n_current = graph.size();
                std::uniform_int_distribution<> dist_remove(0, n_current - 1);

                int vertex_to_removed = dist_remove(gen);

                for (int i = 0; i < n_current; i++)
                {
                    graph[i].erase(graph[i].begin() + vertex_to_removed);
                }
                graph.erase(graph.begin() + vertex_to_removed);
            }

            // Check if the result is close to wanted density
            double result_density = calculate_density(graph);
            double complement_density = 1 - result_density;
            if ((d - eps <= result_density) && (result_density <= d + eps))
            {
                break;
            }
            else if ((d - eps <= complement_density) && (complement_density <= d + eps))
            {
                graph = get_complement_of_graph(graph);
                break;
            }
            else
            {
                // if density is different, start again
                const auto &random_file2 = files[dist_files(gen)];
                graph = read_graph_adj_matrix_from_txt_file(random_file2.path());
            }
        }
        counter++;
    }

    return graph;
}

std::vector<std::vector<bool>>
gen_graph_related_to_base_graph(const std::vector<std::vector<bool>> &base_graph,
                                double different_edge_ratio)
{
    int n = base_graph.size();
    std::vector<uint64_t> vertex_pairs;
    for (int i = 0; i < (n - 1); i++)
    {
        for (int j = i + 1; j < n; j++)
        {
            auto key = pack_pair(i, j);
            vertex_pairs.push_back(key);
        }
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(vertex_pairs.begin(), vertex_pairs.end(), g);

    int different_vp_count = static_cast<int>(different_edge_ratio * n * (n - 1) / 2);

    std::vector<uint64_t> picked(vertex_pairs.begin(), vertex_pairs.begin() + different_vp_count);

    auto output_graph = base_graph;
    for (const auto &vp : picked)
    {
        auto [i, j] = unpack_pair(vp);
        output_graph[i][j] = !output_graph[i][j];
        output_graph[j][i] = !output_graph[j][i];
    }

    return output_graph;
}

std::vector<std::vector<bool>> gen_p4_free_graph(int n, double p)
// n is graph order, p is the probability that a vertex is universal to the vertices before
// amazingly expected density of the graph is p.
{
    std::vector<std::vector<bool>> adjacency_matrix(n, std::vector<bool>(n, false));
    std::random_device rd;
    std::default_random_engine generator(rd());
    std::uniform_real_distribution<double> distribution(0.0, 1.0);

    // A vector to hold if the added vertex is universal or not, at its addition.
    // A vertex is either universal or isolated to vertices with smaller index
    for (int i = 1; i < n; i++)
    {
        double random_value = distribution(generator);
        if (random_value <= p)
        {
            // vertex is universal
            for (int j = 0; j < i; j++)
            {
                adjacency_matrix[i][j] = true;
                adjacency_matrix[j][i] = true;
            }
        }
    }

    return adjacency_matrix;
}

std::vector<std::vector<bool>> erdos_renyi_gen(int n, double p)
{
    std::vector<std::vector<bool>> adjacency_matrix(n, std::vector<bool>(n, false));

    std::random_device rd;
    std::default_random_engine generator(rd());
    std::uniform_real_distribution<double> distribution(0.0, 1.0);
    for (int i = 0; i < n; i++)
    {
        for (int j = i + 1; j < n; j++)
        {
            double random_value = distribution(generator);
            if (random_value <= p)
            {
                adjacency_matrix[i][j] = true;
                adjacency_matrix[j][i] = true;
            }
        }
    }

    return adjacency_matrix;
}

std::vector<std::vector<bool>>
gen_graph_outer_sandwich(const std::vector<std::vector<bool>> &base_graph,
                                double optional_edge_density)
{
    int order = base_graph.size();
    auto output_graph = base_graph;

    std::vector<uint64_t> non_edge_indices;
    // count nonedges in the input graph, also store their indices
    int n_non_edges = 0;
    for(int i = 0; i < order; i++){
        for(int j = i + 1; j < order; j++){
            if(!base_graph[i][j]){
                n_non_edges++;
                auto vp = pack_pair(i,j);
                non_edge_indices.push_back(vp);
            }
        }
    }
    // calculate number of edges to be added in the optional edges according to the completion dens.
    int n_edges_to_be_added = static_cast<int>(n_non_edges * optional_edge_density);

    // Select randomly n_edges_to_be_added many pairs among n_non_edges
    std::vector<int> indices_to_be_edges = select_k_without_replacement(n_non_edges, n_edges_to_be_added);

    // modify output graph
    for(const auto& pair_index: indices_to_be_edges){
        auto vp = non_edge_indices[pair_index];
        auto [u, v] = unpack_pair(vp);
        output_graph[v][u] = true;
        output_graph[u][v] = true;
    }

    return output_graph;
}

int main(int argc, char *argv[])
{
    std::string type = argv[1];

    if (type == "random")
    {
        int order = std::stoi(argv[2]);
        double density = std::stod("0." + std::string(argv[3]));
        std::string density_dec_str = argv[3];
        int id = std::stoi(argv[4]);
        std::string graph_folder = argv[5];
        auto graph = erdos_renyi_gen(order, density);
        write_graph_to_file(graph, graph_folder, type, order, density_dec_str, id);
    }
    else if (type == "p4free")
    {
        int order = std::stoi(argv[2]);
        double density = std::stod("0." + std::string(argv[3]));
        std::string density_dec_str = argv[3];
        int id = std::stoi(argv[4]);
        std::string graph_folder = argv[5];
        auto graph = erdos_renyi_gen(order, density);
        write_graph_to_file(graph, graph_folder, type, order, density_dec_str, id);
    }
    else if (type == "perfect_operations")
    {
        std::string type = argv[1];
        int order = std::stoi(argv[2]);
        double density = std::stod("0." + std::string(argv[3]));
        std::string density_dec_str = argv[3];
        int id = std::stoi(argv[4]);
        double epsilon = std::stod(argv[5]);
        std::string pool_folder = argv[6];
        std::string output_folder = argv[7];
        auto graph = gen_perfect_via_operations(order, density, epsilon, pool_folder);
        write_graph_to_file(graph, output_folder, "perfectOperations", order, density_dec_str, id);
    }
    else if (type == "relative_to_base")
    {
        auto input_graph = read_graph_adj_matrix_from_txt_file(argv[2]);
        double different_edge_ratio = std::stod(argv[3]);
        auto output_graph = gen_graph_related_to_base_graph(input_graph, different_edge_ratio);
        write_graph_to_file_given_filename(output_graph, argv[4]);
    }
    else if (type == "heuristic02")
    {
        auto input_graph = read_graph_adj_matrix_from_txt_file(argv[2]);
        heuristic_stats heuristic_stats_obj = heuristic02(input_graph);
        write_graph_to_file_given_filename(heuristic_stats_obj.output_graph, argv[3]);
    }
    else if (type == "sandwich_outer")
    {
        auto input_graph = read_graph_adj_matrix_from_txt_file(argv[2]);
        double optional_edge_density = std::stod(argv[3]);
        auto output_graph = gen_graph_outer_sandwich(input_graph, optional_edge_density);
        write_graph_to_file_given_filename(output_graph, argv[4]);
    }

    return 0;
}