#pragma once

#include <fstream>
#include <iostream>
#include <string>

#include "problem_types.h"

struct Cutting_Plane_Parameters
{
    std::string parameter_file_name;

    int time_limit = -1;
    int n_threads = -1;
    int ohtp = -1;
    int heur_on_c = -1;
    int heur_on_r = -1;
    int add_all = -1;

    int oh_threshold = -1;
    int oah_threshold = -1;

    Cutting_Plane_Parameters(std::string s) : parameter_file_name(s) {}

    int read_parameter_file();
    int set_oh_thresholds(int n, double p);
    void write_output(std::ofstream& output_file);
    void write_summary(std::ofstream& summary_file);
};