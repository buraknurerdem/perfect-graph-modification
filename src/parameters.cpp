#include "parameters.h"
#include "utils.h"

int Cutting_Plane_Parameters::read_parameter_file()
{
    std::ifstream parameter_file(parameter_file_name);

    if (!parameter_file)
    {
        std::cerr << "Error while reading parameters: Failed to parameter file." << std::endl;
        return 1;
    }

    std::string key, value;
    while (parameter_file >> key >> value)
    { // Read parameters and values
        if (key == "TIME_LIMIT_SECONDS")
        {
            time_limit = std::stoi(value);
        }
        else if (key == "N_THREADS")
        {
            n_threads = std::stoi(value);
        }
        else if (key == "ODD_HOLE_TERMINATION_PERCENTAGE")
        {
            ohtp = std::stoi(value);
        }
        else if (key == "IS_RUNNING_HEURISTIC_ON_CANDIDATE")
        {
            heur_on_c = std::stoi(value);
        }
        else if (key == "IS_RUNNING_HEURISTIC_ON_RELAXATION")
        {
            heur_on_r = std::stoi(value);
        }
        else if (key == "ADD_ALL_AT_START")
        {
            add_all = std::stoi(value);
        }
    }

    if (time_limit == -1 || n_threads == -1 || ohtp == -1 || heur_on_c == -1 || heur_on_r == -1 ||
        add_all == -1)
    {
        std::cerr << "Error while reading parameters: Failed to set at least one parameter." << std::endl;
        return 1;
    }

    parameter_file.close();

    return 0;
}

int Cutting_Plane_Parameters::set_oh_thresholds(int n, double p)
{

    // Odd Hole Termination Count Calculation:
    if (ohtp == 999)
    {
        std::cout << "ODD_HOLE_TERMINATION_PERCENTAGE is set to 999" << std::endl;
        std::cout << "Program searches for ALL odd holes and odd antiholes" << std::endl << std::endl;
        oh_threshold = 0;  // Zero means do not terminate based on the found odd hole count
        oah_threshold = 0; // Zero means do not terminate based on the found odd anti hole count
    }
    else if (ohtp == 1)
    {
        std::cout << "ODD_HOLE_TERMINATION_PERCENTAGE is set to 1" << std::endl;
        std::cout << "Program searches for JUST ONE odd hole or odd antihole" << std::endl << std::endl;
        oh_threshold = 1;
        oah_threshold = 1;
    }
    else if (ohtp <= 0)
    {
        std::cout << "ERROR: ODD_HOLE_TERMINATION_PERCENTAGE is set to 0" << std::endl;
    }
    else if (ohtp != -1 && ohtp != 0)
    {
        int expected_no_of_oh = static_cast<int>(expected_value_of_odd_holes(n, p));
        int expected_no_of_oah = static_cast<int>(expected_value_of_odd_anti_holes(n, p));

        oh_threshold = static_cast<int>(ohtp * expected_no_of_oh / 100);
        oah_threshold = static_cast<int>(ohtp * expected_no_of_oah / 100);

        std::cout << "ODD_HOLE_TERMINATION_PERCENTAGE is set to " << ohtp << std::endl;
        std::cout << "oh_threshold: " << oh_threshold << std::endl;
        std::cout << "oah_threshold is set to " << oah_threshold << std::endl << std::endl;
    }
    else
    {
        std::cerr << "Error: ohtp 0 or -1.";
        return 1;
    }

    // Check after
    if (oh_threshold == -1 || oah_threshold == -1)
    {
        std::cerr << "Error while setting odd hole termination thresholds." << std::endl;
        return 1;
    }

    return 0;
}

void Cutting_Plane_Parameters::write_output(std::ofstream &output_file)
{
    output_file << "========================" << std::endl;
    output_file << "TIME_LIMIT_SECONDS: " << time_limit << std::endl;
    output_file << "N_THREADS: " << n_threads << std::endl;
    output_file << "IS_RUNNING_HEURISTIC_ON_CANDIDATE: " << heur_on_c << std::endl;
    output_file << "IS_RUNNING_HEURISTIC_ON_RELAXATION: " << heur_on_r << std::endl;
    output_file << "ADD_ALL_AT_START: " << add_all << std::endl;
    output_file << "ODD_HOLE_TERMINATION_PERCENTAGE: " << ohtp << std::endl;
    output_file << "Calculated Odd Hole Termination Batch Size: " << oh_threshold << std::endl;
    output_file << "Calculated Odd Antihole Termination Batch Size: " << oah_threshold << std::endl;

    return;
}

void Cutting_Plane_Parameters::write_summary(std::ofstream &summary_file)
{
    summary_file << time_limit << ",";
    summary_file << n_threads << ",";
    summary_file << ohtp << ",";
    summary_file << oh_threshold << ",";
    summary_file << oah_threshold << ",";
    summary_file << heur_on_c << ",";
    summary_file << heur_on_r << ",";
    summary_file << add_all << ",";

    return;
}