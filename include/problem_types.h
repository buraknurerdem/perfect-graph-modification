#pragma once

#include <ilcplex/ilocplex.h>
#include <iostream>
#include <string>
#include <vector>
#include "iterative_modification_heuristic.h"

class Problem
{
public:
    IloEnv env;
    IloModel model;
    IloCplex cplex;

    std::string input_graph_file_name;
    std::vector<std::vector<bool>> input_graph;
    int order;
    double input_graph_p;
    int input_edge_count;
    double input_graph_density;
    int n_of_vertex_pairs;
    std::vector<std::vector<int>> flat_index_matrix;
    IloNumVarArray x_flat;
    std::vector<std::vector<bool>> output_graph;
    int output_edge_count;
    double output_graph_density;
    

    Problem(IloEnv e, IloModel m, IloCplex c, std::string s) : env(e), model(m), cplex(c), input_graph_file_name(s) {}

    virtual void create_decision_vars() = 0;
    virtual void set_objective() = 0;
    virtual void add_odd_hole_constraint(IloRangeArray &cuts, const std::vector<int> &hole) = 0;
    virtual void add_odd_antihole_constraint(IloRangeArray &cuts, const std::vector<int> &hole) = 0;
    virtual void write_output(std::ofstream& output_file) = 0;
    virtual void write_summary(std::ofstream& summary_file) = 0;
    virtual heuristic_stats run_heuristic(std::vector<std::vector<bool>> &graph) = 0;

    virtual ~Problem(){}
};

class Problem_Edit : public Problem
{
public:
    Problem_Edit(IloEnv p_env, IloModel p_model, IloCplex p_cplex, std::string s);

    void create_decision_vars() override;
    void set_objective() override;
    void add_odd_hole_constraint(IloRangeArray &cuts, const std::vector<int> &hole) override;
    void add_odd_antihole_constraint(IloRangeArray &cuts, const std::vector<int> &hole) override;
    void write_output(std::ofstream& output_file) override;
    void write_summary(std::ofstream& summary_file) override;
    heuristic_stats run_heuristic(std::vector<std::vector<bool>> &graph) override;
};


class Problem_Completion : public Problem
{
public:

    int input_non_edge_count;

    Problem_Completion(IloEnv p_env, IloModel p_model, IloCplex p_cplex, std::string s);

    void create_decision_vars() override;
    void set_objective() override;
    void add_odd_hole_constraint(IloRangeArray &cuts, const std::vector<int> &hole) override;
    void add_odd_antihole_constraint(IloRangeArray &cuts, const std::vector<int> &hole) override;
    void write_output(std::ofstream& output_file) override;
    void write_summary(std::ofstream& summary_file) override;
    heuristic_stats run_heuristic(std::vector<std::vector<bool>> &graph) override;
};