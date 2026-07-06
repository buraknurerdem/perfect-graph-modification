#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

void odd_hole_recursive(
    const std::vector<std::vector<bool>> &graph_adj_mat,
    const std::vector<std::vector<int>> &graph_adj_list,
    std::vector<int> &path_vector,
    std::unordered_map<std::string, std::vector<int>> &odd_holes,
    bool is_anti_hole_search,
    int termination_batch_size
);

std::unordered_map<std::string, std::vector<int>> find_odd_holes(
    const std::vector<std::vector<bool>> &graph_adj_mat,
    bool is_anti_hole_search,
    int termination_batch_size
);

bool is_perfect(const std::vector<std::vector<bool>> &graph_adj_mat);

void find_new_oh_and_oah_for_vp(
    int v1,
    int v2,
    const std::vector<std::vector<bool>> &graph,
    const std::vector<std::vector<bool>> &complement_graph,
    std::unordered_map<std::string, std::vector<int>> &odd_holes,
    std::unordered_map<std::string, std::vector<int>> &odd_anti_holes,
    int termination_batch_size
);

void find_and_list_odd_holes_and_odd_antiholes(const std::vector<std::vector<bool>> &graph);
