#include "find_odd_holes.h"
#include "utils.h"

void odd_hole_recursive(
    const std::vector<std::vector<bool>> &graph_adj_mat,
    const std::vector<std::vector<int>> &graph_adj_list,
    std::vector<int> &path_vector,
    std::unordered_map<std::string, std::vector<int>> &odd_holes,
    bool is_anti_hole_search,
    int termination_batch_size
)
{

    if (termination_batch_size != 0 && (odd_holes.size() >= termination_batch_size))
    {
        return;
    }

    int last_added_v = path_vector.back();
    int path_length = path_vector.size();

    int wanted_min_cycle_length;
    // If searcing for odd anti-holes, wanted_min_cycle_length should be 7, to avoid double counting
    // C_5's
    if (is_anti_hole_search)
    {
        wanted_min_cycle_length = 7;
    }
    // If searching for odd holes wanted_min_cycle_length should be 5
    else
    {
        wanted_min_cycle_length = 5;
    }

    // for neighbors of last vertex
    for (auto i : graph_adj_list[last_added_v])
    {

        // Smaller indices of the start vertex is not considered. So an odd-hole is only identified
        // when path start with its smallest indexed vertex smallest index vertex
        if (i < path_vector[0])
            continue;

        // It skips the vertex if it the one added before last_added_v.
        if (path_length > 1 && path_vector[path_length - 2] == i)
            continue;

        // i should be skipped if it is an internal vertex in the path. there is no explicit check.
        // the chord check also manages to skip i if it is already an internal vertex

        // chord check
        bool chord_exist = false;
        for (int j = 1; (path_length > 2) && (j < (path_length - 1)); ++j)
        {
            // if chord exist, no odd hole, no need to continue the path with this vertex.
            if (graph_adj_mat[i][path_vector[j]])
            {
                chord_exist = true;
                break;
            }
        }

        // if there is no chord, then check for cycle (hole), and if cycle exists, check if it is
        // an oddhole. If it is an even-hole, pass vertex i.
        if ((!chord_exist) && (path_length > 1) && (graph_adj_mat[path_vector[0]][i]))
        {

            int cycle_length = path_length + 1;

            if (cycle_length % 2 == 1 && cycle_length >= wanted_min_cycle_length)
            { // if true, then odd hole is found

                // add found odd hole to the data structure. CAN THIS PART BE IMPROVED FOR
                // EFFICIENCY???
                auto odd_hole = path_vector;
                odd_hole.push_back(i);

                auto cycle_string = cycle_vector_to_string(odd_hole);
                odd_holes.try_emplace(cycle_string, std::move(odd_hole));
            }
        }

        // if no chord, no odd-hole, then add i to the path and continue recursion.
        else if (!chord_exist)
        {
            path_vector.push_back(i);
            odd_hole_recursive(
                graph_adj_mat, graph_adj_list, path_vector, odd_holes, is_anti_hole_search,
                termination_batch_size
            );
        }
    }

    path_vector.pop_back();
    return;
}

std::unordered_map<std::string, std::vector<int>> find_odd_holes(
    const std::vector<std::vector<bool>> &graph_adj_mat,
    bool is_anti_hole_search,
    int termination_batch_size
)
{

    auto graph_adj_list = get_adj_list_from_adj_matrix(graph_adj_mat);
    std::unordered_map<std::string, std::vector<int>> odd_holes;

    for (int i = 0; i < graph_adj_mat.size(); ++i)
    {
        std::vector<int> path_vector = {i};
        path_vector.reserve(graph_adj_mat.size());
        odd_hole_recursive(
            graph_adj_mat, graph_adj_list, path_vector, odd_holes, is_anti_hole_search,
            termination_batch_size
        );
    }

    return odd_holes;
}

bool is_perfect(const std::vector<std::vector<bool>> &graph_adj_mat)
{

    auto odd_holes = find_odd_holes(graph_adj_mat, false, 1);
    if (odd_holes.size() > 0)
    {
        return false;
    }

    auto complement_graph = get_complement_of_graph(graph_adj_mat);
    auto odd_anti_holes = find_odd_holes(complement_graph, true, 1);
    if (odd_anti_holes.size() > 0)
    {
        return false;
    }
    return true;
}

void odd_hole_recursive_for_vp(
    const std::vector<std::vector<bool>> &graph_adj_mat,
    const std::vector<std::vector<int>> &graph_adj_list,
    std::vector<int> &path_vector,
    std::unordered_map<std::string, std::vector<int>> &odd_holes,
    bool is_anti_hole_search,
    int v2,
    int termination_batch_size
)
{

    if (termination_batch_size != 0 && odd_holes.size() >= termination_batch_size)
    {
        return;
    }

    int last_added_v = path_vector.back();
    int path_length = path_vector.size();

    int wanted_min_cycle_length;
    // If searcing for odd anti-holes, wanted_min_cycle_length should be 7, to avoid double counting
    // C_5's
    if (is_anti_hole_search)
    {
        wanted_min_cycle_length = 7;
    }
    // If searching for odd holes wanted_min_cycle_length should be 5
    else
    {
        wanted_min_cycle_length = 5;
    }

    // for neighbors of last vertex
    for (auto i : graph_adj_list[last_added_v])
    {

        // this part is commented since we start path with a specific vertex in this version
        // if (i < path_vector[0])
        //     continue;

        // It skips the vertex if it the one added before last_added_v.
        if (path_length > 1 && path_vector[path_length - 2] == i)
            continue;

        // i should be skipped if it is an internal vertex in the path. there is no explicit check.
        // the chord check also manages to skip i if it is already an internal vertex

        // chord check
        bool chord_exist = false;
        for (int j = 1; (path_length > 2) && (j < (path_length - 1)); ++j)
        {
            // if chord exist, no odd hole, no need to continue the path with this vertex.
            if (graph_adj_mat[i][path_vector[j]])
            {
                chord_exist = true;
                break;
            }
        }

        // if there is no chord, then check for cycle (hole), and if cycle exists, check if it is
        // an oddhole. If it is an even-hole, pass vertex i.
        if ((!chord_exist) && (path_length > 1) && (graph_adj_mat[path_vector[0]][i]))
        {

            int cycle_length = path_length + 1;

            if (cycle_length % 2 == 1 && cycle_length >= wanted_min_cycle_length)
            { // if true, then odd hole is found

                auto odd_hole = path_vector;
                odd_hole.push_back(i);

                if (std::find(odd_hole.begin(), odd_hole.end(), v2) != odd_hole.end())
                {
                    auto cycle_string = cycle_vector_to_string(odd_hole);
                    odd_holes.try_emplace(cycle_string, std::move(odd_hole));
                }
            }
        }

        // if no chord, no odd-hole, then add i to the path and continue recursion.
        else if (!chord_exist)
        {
            path_vector.push_back(i);
            odd_hole_recursive_for_vp(
                graph_adj_mat, graph_adj_list, path_vector, odd_holes, is_anti_hole_search, v2,
                termination_batch_size
            );
        }
    }

    path_vector.pop_back();
    return;
}

void find_new_oh_and_oah_for_vp(
    int v1,
    int v2,
    const std::vector<std::vector<bool>> &graph,
    const std::vector<std::vector<bool>> &complement_graph,
    std::unordered_map<std::string, std::vector<int>> &odd_holes,
    std::unordered_map<std::string, std::vector<int>> &odd_anti_holes,
    int termination_batch_size
)
{
    auto graph_adj_list = get_adj_list_from_adj_matrix(graph);

    std::vector<int> path_vector = {v1};
    path_vector.reserve(graph.size());
    odd_hole_recursive_for_vp(
        graph, graph_adj_list, path_vector, odd_holes, false, v2, termination_batch_size
    );

    int new_oh_size = odd_holes.size();
    if (termination_batch_size != 0 && new_oh_size >= termination_batch_size)
    {
        return;
    }
    int oah_term_size = termination_batch_size - new_oh_size;

    // odd anti hole search
    auto comp_adj_list = get_adj_list_from_adj_matrix(complement_graph);

    path_vector = {v1};
    path_vector.reserve(graph.size());
    odd_hole_recursive_for_vp(
        complement_graph, comp_adj_list, path_vector, odd_anti_holes, true, v2, (oah_term_size)
    );
}

void find_and_list_odd_holes_and_odd_antiholes(const std::vector<std::vector<bool>> &graph)
{
    auto oh = find_odd_holes(graph, false, 0);
    auto complement = get_complement_of_graph(graph);
    auto oah = find_odd_holes(complement, true, 0);

    std::cout << "Odd Holes: " << std::endl;
    print_odd_holes(oh);
    std::cout << "Odd Antiholes: " << std::endl;
    print_odd_holes(oah);

    return;
};

