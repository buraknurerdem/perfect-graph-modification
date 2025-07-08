#pragma once

#include <vector>

struct heuristic_stats
{
    bool is_successful;
    int number_of_inner_loops = 0;
    std::vector<std::vector<bool>> output_graph;
    long long runtime;
};

heuristic_stats heuristic02(std::vector<std::vector<bool>> graph);

heuristic_stats heuristic02_base_graph(const std::vector<std::vector<bool>> &base_graph,
                                       std::vector<std::vector<bool>> graph,
                                       double m);

heuristic_stats heuristic02_completion(const std::vector<std::vector<bool>> &base_graph,
                                       std::vector<std::vector<bool>> graph);