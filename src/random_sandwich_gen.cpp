#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <random>

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

void create_optional_edges(std::vector<std::vector<bool>>& graph, double completion_density){

    int order = graph.size();
    std::vector<std::pair<int, int>> non_edge_indices;
    // count nonedges in the input graph, also store their indices
    int n_non_edges = 0;
    for(int i = 0; i < order; i++){
        for(int j = i + 1; j < order; j++){
            if(!graph[i][j]){
                n_non_edges++;
                non_edge_indices.push_back(std::make_pair(i, j));
            }
        }
    }
    // calculate number of edges to be added in the optional edges according to the completion dens.
    int n_edges_to_be_added = static_cast<int>(n_non_edges * completion_density);

    // std::cout << n_edges_to_be_added << ", " << n_non_edges << std::endl;

    // select randomly
    std::vector<int> indices_to_be_edges = select_k_without_replacement(n_non_edges, n_edges_to_be_added);

    // modify input graph
    for(const auto& pair_index: indices_to_be_edges){
        int u = non_edge_indices[pair_index].first;
        int v = non_edge_indices[pair_index].second;
        graph[v][u] = true;
        graph[u][v] = true;
    }

    return;
}

int main(int argc, char* argv[]){

    // Parameter Control
    if (argc < 3) {
        std::cerr << "Too few arguments for: " << argv[0] << "!" << std::endl;
        return 1;
    }

    // Read Input Graph
    std::string inputGraphFileName = argv[1];
    std::ifstream inputGraphFile(inputGraphFileName);
    std::vector<std::vector<bool>> inputGraph;

    if (!inputGraphFile) {
        std::cerr << "Unable to open inputGraphFile" << std::endl;
        return 1;
    }

    std::string line;
    while (std::getline(inputGraphFile, line)) {
        std::vector<bool> row;
        std::istringstream stream(line);
        std::string value;
        
        while (std::getline(stream, value, ',')) {
            row.push_back(value == "1");
        }

        inputGraph.push_back(row);
    }
    inputGraphFile.close();

    // Read the other parameters
    double completion_density = std::stod(argv[2]);
    std::string outputGraphFileName = argv[3];


    int order = inputGraph.size();
    // Create optional edge graph
    create_optional_edges(inputGraph, completion_density);

    // Write the optional edge graph
    std::ofstream outputGraphFile(outputGraphFileName);
    for(int i = 0; i < order; ++i){
        for(int j = 0; j < order; ++j){

            outputGraphFile << inputGraph[i][j];
            if(j == order - 1){
                outputGraphFile << std::endl;
            }
            else{
                outputGraphFile << ",";
            }
        }
    }

    outputGraphFile.close();

    return 0;
}