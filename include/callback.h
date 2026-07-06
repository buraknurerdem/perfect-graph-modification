#pragma once

#include <ilcplex/ilocplex.h>
#include <problem_types.h>
#include <parameters.h>
#include <odd_hole_worker.h>

struct Generic_Callback : public IloCplex::Callback::Function
{
    std::vector<Odd_Hole_Worker*> workers;
    Problem *problem;
    Cutting_Plane_Parameters *parameters;
    // IloNumVarArray &x_flat;
    // std::vector<std::vector<int>> &flat_index_matrix;
    // std::vector<std::vector<bool>> &input_graph;
    // int order;
    // int n_of_vertex_pairs;

    Generic_Callback(Problem *prb, Cutting_Plane_Parameters *params);
    ~Generic_Callback();

    void invoke(const IloCplex::Callback::Context &context);
};
