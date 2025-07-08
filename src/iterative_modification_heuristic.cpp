#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include "find_odd_holes.h"
#include "iterative_modification_heuristic.h"
#include "utils.h"

heuristic_stats heuristic02(std::vector<std::vector<bool>> graph)
// At each iteration, changes the vertex pair incident to most oh and oah if
// the change reduces the total count of odd holes and odd antiholes
{

    auto time_start = std::chrono::high_resolution_clock::now();
    heuristic_stats h_stats;
    int n = graph.size();

    // Find all odd-holes and odd anti-holes
    auto complement_graph = get_complement_of_graph(graph);
    auto odd_holes = find_odd_holes(graph, false, 0);
    auto odd_anti_holes = find_odd_holes(complement_graph, true, 0);

    std::unordered_map<uint64_t, int> vp_count;
    vp_count.reserve(n * (n - 1) / 2);
    for (int i = 0; i < n; i++)
    {
        for (int j = i + 1; j < n; j++)
        {
            auto key = pack_pair(i, j);
            vp_count.emplace(key, 0);
        }
    }

    for (const auto &oh : odd_holes)
    {
        for (int i = 0; i < oh.second.size() - 1; i++)
        {
            for (int j = i + 1; j < oh.second.size(); j++)
            {
                auto key = pack_pair(oh.second[i], oh.second[j]);
                vp_count[key] += 1;
            }
        }
    }
    for (const auto &oah : odd_anti_holes)
    {
        for (int i = 0; i < oah.second.size() - 1; i++)
        {
            for (int j = i + 1; j < oah.second.size(); j++)
            {
                auto key = pack_pair(oah.second[i], oah.second[j]);
                vp_count[key] += 1;
            }
        }
    }

    bool heuristic_continue = true;
    while (heuristic_continue)
    {
        h_stats.number_of_inner_loops++;

        // perfectness check
        if (odd_holes.size() == 0 && odd_anti_holes.size() == 0)
        {
            heuristic_continue = false;
            h_stats.is_successful = true;
            break;
        }

        std::unordered_set<uint64_t> failed_vp;
        while (true)
        {
            // heuristic failed if all vp are not valid.
            if (failed_vp.size() == (n * (n - 1) / 2))
            {
                std::cout << "ALL VP NON VALID. HEURISTIC FAILED" << std::endl;
                heuristic_continue = false;
                h_stats.is_successful = false;
                break;
            }

            // find vp of max count not in failed_vp
            int max_count = -1;
            uint64_t vp_to_try;
            for (int i = 0; i < n; i++)
            {
                for (int j = i + 1; j < n; j++)
                {
                    auto key = pack_pair(i, j);
                    if (vp_count[key] > max_count &&
                        std::find(failed_vp.begin(), failed_vp.end(), key) == failed_vp.end())
                    {
                        max_count = vp_count[key];
                        vp_to_try = key;
                    }
                }
            }

            // Check if vp is ok, first, Change vp in graph
            auto [v1, v2] = unpack_pair(vp_to_try);
            graph[v1][v2] = !graph[v1][v2];
            graph[v2][v1] = !graph[v2][v1];
            complement_graph[v1][v2] = !complement_graph[v1][v2];
            complement_graph[v2][v1] = !complement_graph[v2][v1];

            // calculate newly created odd holes and odd anti holes including vp
            std::unordered_map<std::string, std::vector<int>> new_oh;
            std::unordered_map<std::string, std::vector<int>> new_oah;
            find_new_oh_and_oah_for_vp(v1, v2, graph, complement_graph, new_oh, new_oah, 0);

            int newly_added_count = new_oh.size() + new_oah.size();
            if (newly_added_count >= vp_count[vp_to_try])
            {
                // if here than changing the state of vp_to_try is not reducing total oh and oah
                // count, try another vp
                failed_vp.insert(vp_to_try);

                // Revert the changes in the graph
                graph[v1][v2] = !graph[v1][v2];
                graph[v2][v1] = !graph[v2][v1];
                complement_graph[v1][v2] = !complement_graph[v1][v2];
                complement_graph[v2][v1] = !complement_graph[v2][v1];
            }
            else
            {
                // if here than changing the state of vp_to_try is not reducing total oh and oah
                // continue with changes

                // remove odd holes and odd antiholes containg vp in the maps
                remove_entries_containing_vp_from_odd_holes(odd_holes, v1, v2);
                remove_entries_containing_vp_from_odd_holes(odd_anti_holes, v1, v2);

                // add new ones to the maps
                for (auto &[k, v] : new_oh)
                {
                    auto &existing_vec = odd_holes.try_emplace(k).first->second;
                    existing_vec.insert(existing_vec.end(), v.begin(), v.end());
                }
                for (auto &[k, v] : new_oah)
                {
                    auto &existing_vec = odd_anti_holes.try_emplace(k).first->second;
                    existing_vec.insert(existing_vec.end(), v.begin(), v.end());
                }

                // Update vp_count
                for (int i = 0; i < n; i++)
                {
                    for (int j = i + 1; j < n; j++)
                    {
                        auto key = pack_pair(i, j);
                        vp_count[key] = 0;
                    }
                }

                for (const auto &oh : odd_holes)
                {
                    for (int i = 0; i < oh.second.size() - 1; i++)
                    {
                        for (int j = i + 1; j < oh.second.size(); j++)
                        {
                            auto key = pack_pair(oh.second[i], oh.second[j]);
                            vp_count[key] += 1;
                        }
                    }
                }
                for (const auto &oah : odd_anti_holes)
                {
                    for (int i = 0; i < oah.second.size() - 1; i++)
                    {
                        for (int j = i + 1; j < oah.second.size(); j++)
                        {
                            auto key = pack_pair(oah.second[i], oah.second[j]);
                            vp_count[key] += 1;
                        }
                    }
                }

                // break since a valid vp is found and necessary changes are done
                break;
            }
        }
    }

    auto time_end = std::chrono::high_resolution_clock::now();
    h_stats.runtime =
        std::chrono::duration_cast<std::chrono::seconds>(time_end - time_start).count();

    h_stats.output_graph = graph;
    return h_stats;
}

heuristic_stats heuristic02_completion(const std::vector<std::vector<bool>> &base_graph,
                                       std::vector<std::vector<bool>> graph)
// At each iteration, changes the vertex pair incident to most oh and oah if
// the change reduces the total count of odd holes and odd antiholes.
// Cannot remove the edges of input_graph
{

    auto time_start = std::chrono::high_resolution_clock::now();
    heuristic_stats h_stats;
    int n = graph.size();

    // Find all odd-holes and odd anti-holes
    auto complement_graph = get_complement_of_graph(graph);
    auto odd_holes = find_odd_holes(graph, false, 0);
    auto odd_anti_holes = find_odd_holes(complement_graph, true, 0);

    std::unordered_map<uint64_t, int> vp_count;
    vp_count.reserve(n * (n - 1) / 2);
    for (int i = 0; i < n; i++)
    {
        for (int j = i + 1; j < n; j++)
        {
            auto key = pack_pair(i, j);
            vp_count.emplace(key, 0);
        }
    }

    for (const auto &oh : odd_holes)
    {
        for (int i = 0; i < oh.second.size() - 1; i++)
        {
            for (int j = i + 1; j < oh.second.size(); j++)
            {
                auto key = pack_pair(oh.second[i], oh.second[j]);
                vp_count[key] += 1;
            }
        }
    }
    for (const auto &oah : odd_anti_holes)
    {
        for (int i = 0; i < oah.second.size() - 1; i++)
        {
            for (int j = i + 1; j < oah.second.size(); j++)
            {
                auto key = pack_pair(oah.second[i], oah.second[j]);
                vp_count[key] += 1;
            }
        }
    }

    bool heuristic_continue = true;
    while (heuristic_continue)
    {
        h_stats.number_of_inner_loops++;

        // perfectness check
        if (odd_holes.size() == 0 && odd_anti_holes.size() == 0)
        {
            heuristic_continue = false;
            h_stats.is_successful = true;
            break;
        }

        std::unordered_set<uint64_t> failed_vp;
        while (true)
        {
            // heuristic failed if all vp are not valid.
            if (failed_vp.size() == (n * (n - 1) / 2))
            {
                heuristic_continue = false;
                h_stats.is_successful = false;
                break;
            }

            // find vp of max count not in failed_vp
            int max_count = -1;
            uint64_t vp_to_try;
            for (int i = 0; i < n; i++)
            {
                for (int j = i + 1; j < n; j++)
                {
                    auto key = pack_pair(i, j);
                    if (vp_count[key] > max_count &&
                        std::find(failed_vp.begin(), failed_vp.end(), key) == failed_vp.end())
                    {
                        max_count = vp_count[key];
                        vp_to_try = key;
                    }
                }
            }

            auto [v1, v2] = unpack_pair(vp_to_try);

            if (base_graph[v1][v2] == true && graph[v1][v2] == true)
            {
                // cannot remove an edge of input_graph
                failed_vp.insert(vp_to_try);
                continue;
            }
            // TEST
            else if (base_graph[v1][v2] == true && graph[v1][v2] == false)
            {
                std::cerr
                    << "There is a prb. Edge of input does not exist on the graph in heuristic!!!"
                    << std::endl;
            }

            // Check if vp is ok, first, Change vp in graph
            graph[v1][v2] = !graph[v1][v2];
            graph[v2][v1] = !graph[v2][v1];
            complement_graph[v1][v2] = !complement_graph[v1][v2];
            complement_graph[v2][v1] = !complement_graph[v2][v1];

            // calculate newly created odd holes and odd anti holes including vp
            std::unordered_map<std::string, std::vector<int>> new_oh;
            std::unordered_map<std::string, std::vector<int>> new_oah;
            find_new_oh_and_oah_for_vp(v1, v2, graph, complement_graph, new_oh, new_oah, 0);

            int newly_added_count = new_oh.size() + new_oah.size();
            if (newly_added_count >= vp_count[vp_to_try])
            {
                // if here than changing the state of vp_to_try is not reducing total oh and oah
                // count, try another vp
                failed_vp.insert(vp_to_try);

                // Revert the changes in the graph
                graph[v1][v2] = !graph[v1][v2];
                graph[v2][v1] = !graph[v2][v1];
                complement_graph[v1][v2] = !complement_graph[v1][v2];
                complement_graph[v2][v1] = !complement_graph[v2][v1];
            }
            else
            {
                // if here than changing the state of vp_to_try is not reducing total oh and oah
                // continue with changes

                // remove odd holes and odd antiholes containg vp in the maps
                remove_entries_containing_vp_from_odd_holes(odd_holes, v1, v2);
                remove_entries_containing_vp_from_odd_holes(odd_anti_holes, v1, v2);

                // add new ones to the maps
                for (auto &[k, v] : new_oh)
                {
                    auto &existing_vec = odd_holes.try_emplace(k).first->second;
                    existing_vec.insert(existing_vec.end(), v.begin(), v.end());
                }
                for (auto &[k, v] : new_oah)
                {
                    auto &existing_vec = odd_anti_holes.try_emplace(k).first->second;
                    existing_vec.insert(existing_vec.end(), v.begin(), v.end());
                }

                // Update vp_count
                for (int i = 0; i < n; i++)
                {
                    for (int j = i + 1; j < n; j++)
                    {
                        auto key = pack_pair(i, j);
                        vp_count[key] = 0;
                    }
                }

                for (const auto &oh : odd_holes)
                {
                    for (int i = 0; i < oh.second.size() - 1; i++)
                    {
                        for (int j = i + 1; j < oh.second.size(); j++)
                        {
                            auto key = pack_pair(oh.second[i], oh.second[j]);
                            vp_count[key] += 1;
                        }
                    }
                }
                for (const auto &oah : odd_anti_holes)
                {
                    for (int i = 0; i < oah.second.size() - 1; i++)
                    {
                        for (int j = i + 1; j < oah.second.size(); j++)
                        {
                            auto key = pack_pair(oah.second[i], oah.second[j]);
                            vp_count[key] += 1;
                        }
                    }
                }

                // break since a valid vp is found and necessary changes are done
                break;
            }
        }
    }

    auto time_end = std::chrono::high_resolution_clock::now();
    h_stats.runtime =
        std::chrono::duration_cast<std::chrono::seconds>(time_end - time_start).count();

    h_stats.output_graph = graph;
    return h_stats;
}
