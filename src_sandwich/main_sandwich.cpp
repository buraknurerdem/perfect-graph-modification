#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <string>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <chrono>
#include <ilcplex/ilocplex.h>


// Parameter Definitions
int ODD_HOLE_TERMINATION_BATCH_SIZE;
int ODD_ANTI_HOLE_TERMINATION_BATCH_SIZE;
bool IS_RUNNING_HEURISTIC_ON_CANDIDATE;
bool IS_RUNNING_HEURISTIC_ON_RELAXATION;
bool ADD_ALL_AT_START;

bool is_graph_empty_or_complete(std::vector<std::vector<bool>>& graph){
    int graph_size = graph.size();

    bool is_complete = true;
    bool is_empty = true;
    // Checking only the upper triangular of adj. matrix
    for(int i = 0; i < graph_size - 1; i++){
        for(int j = i + 1; j < graph_size; j++){
            if(is_complete && !graph[i][j]){
                is_complete = false;
            }
            if(is_empty && graph[i][j]){
                is_empty = false;
            }
        }
    }
    if(is_complete || is_empty){
        return true;
    }
    else{ return false; }
}

std::string cycle_vector_to_string(std::vector<int> vec){
    
    std::sort(vec.begin(), vec.end());

    std::string cycle_string;

    for(const auto& v: vec){
        cycle_string.append(std::to_string(v) + "_");
    }

    return cycle_string;
}

std::string canonical_constraint_naming(std::vector<int> cycle, bool is_anti_hole){
    
    std::string canon_key;

    if(is_anti_hole){
        canon_key.append("OAH_");
    }
    else{
        canon_key.append("OH_");
    }

    for(const auto& v: cycle){
        canon_key.append(std::to_string(v) + "_");
    }

    return canon_key;
}

void ip_abort_check(std::vector<std::vector<bool>>& inputGraph, std::vector<std::vector<bool>>& optionalEdgesGraph, std::vector<int>& odd_hole, const bool& is_anti_hole_search, bool& abortIP){
    
    bool local_abort = true;
    if(!is_anti_hole_search){
        for(int k = 0; k < odd_hole.size() - 1; k++){
            for(int l = k + 1; l < odd_hole.size(); l++){
                if((l - k == 1) || (l - k == (odd_hole.size() - 1) )){ // if edge
                    if(!inputGraph[odd_hole[k]][odd_hole[l]]){
                        local_abort = false;
                        break;
                    }
                }
                else{ // if chord
                    if(optionalEdgesGraph[odd_hole[k]][odd_hole[l]]){
                        local_abort = false;
                        break;
                    }
                }
            }
            if(!local_abort){
                break;
            }
        }
    }
    else{ // if working on an odd anti hole
        for(int k = 0; k < odd_hole.size() - 1; k++){
            for(int l = k + 1; l < odd_hole.size(); l++){
                if((l - k == 1) || (l - k == (odd_hole.size() - 1) )){
                    if(optionalEdgesGraph[odd_hole[k]][odd_hole[l]]){
                        local_abort = false;
                        break;
                    }
                }
                else{
                    if(!inputGraph[odd_hole[k]][odd_hole[l]]){
                        local_abort = false;
                        break;
                    }
                }
            }
            if(!local_abort){
                break;
            }
        }
    }
    // log the problematic odd-hole or odd anti-hole
    if(local_abort){
        std::cout << "Program is infeasible due to the ";
        if(is_anti_hole_search){ std::cout << "odd anti-hole: "; }
        else {std::cout << "odd hole: "; }
        for(const auto& vertex: odd_hole){
            std::cout << vertex << ",";
        }
        std::cout << std::endl;
        abortIP = true;
        // std::cout << "    Time: " << std::to_string(std::chrono::high_resolution_clock::now()) << std::endl;
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::cout << "    Time: " << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S") << std::endl;
    }
    return;
}

void oddHoleRecursive(std::vector<std::vector<bool>>& graph_adj_mat, std::vector<std::vector<int>>& graph_adj_list, std::vector<int>& path_vector, std::map<std::string, std::vector<int>>& odd_holes, const bool &is_anti_hole_search, const bool &is_for_heuristic, std::vector<std::vector<bool>>& inputGraph, std::vector<std::vector<bool>>& optionalEdgesGraph, bool& precheck, bool& abortIP){

    if(abortIP){
        return;
    }

    if(!is_for_heuristic){
        if((!is_anti_hole_search) && (ODD_HOLE_TERMINATION_BATCH_SIZE != 0) && (odd_holes.size() >= ODD_HOLE_TERMINATION_BATCH_SIZE)){
            return;
        }
        if((is_anti_hole_search) && (ODD_ANTI_HOLE_TERMINATION_BATCH_SIZE != 0) && (odd_holes.size() >= ODD_ANTI_HOLE_TERMINATION_BATCH_SIZE)){
            return;
        }
    }

    int last_added_v = path_vector.back();
    int path_length = path_vector.size();

    int wanted_min_cycle_length;
    // If searcing for odd anti-holes, it should be 7, to avoid double counting C_5's
    if(is_anti_hole_search){
        wanted_min_cycle_length = 7;
    }
    // If searching for odd holes it should be 5
    else{
        wanted_min_cycle_length = 5;
    }

    // for neighbors of last vertex
    for(auto i: graph_adj_list[last_added_v]){

        // smaller indices of the start vertex is not considered. Finding odd holes only from their smallest index vertex
        if(i < path_vector[0]) continue;

        // It skips the vertex if it the one added before last_added_v.
        // DONT KNOW IF THIS LINE CAN BE SKIPPED SOMEHOW OR IS IT GOOD AS IT IS NOW
        if(path_length > 1 && path_vector[path_length - 2] == i) continue;
        // SHOULD WE ALSO CHECK IF i is in the path. NOT NEEDED FOR NOW BUT FOR EFFICIENCY?

        // Printing
        //std::cout << std::endl; for(auto v: path_vector) std::cout << v << ","; std::cout << " : " << i; std::cout << std::endl;

        // chord search
        bool chord_exist = false;
        for(int j = 1; (path_length > 2) && (j < (path_length - 1)); ++j){
            // if chord exist, no odd hole, no need to continue the path with this vertex.
            if(graph_adj_mat[i][path_vector[j]]){
                chord_exist = true;
                break;
            }
        }

        // if no chord, then check for cycle, and if cycle, add if it is oddhole, if not pass i.
        if((!chord_exist) && (path_length > 1) && (graph_adj_mat[path_vector[0]][i])){

            int cycle_length = path_length + 1;

            if(cycle_length % 2 == 1 && cycle_length >= wanted_min_cycle_length){// if true, then odd hole
                
                // add found odd hole to the data structure. CAN THIS PART BE IMPROVED FOR EFFICIENCY???
                auto odd_hole = path_vector;
                odd_hole.push_back(i);

                /* std::cout << "cycle found! cycle len: " << cycle_len << std::endl;
                for(auto v: odd_hole) std::cout << v << ", "; std::cout << std::endl; */

                // IP abort check
                if(precheck){
                    ip_abort_check(inputGraph, optionalEdgesGraph, odd_hole, is_anti_hole_search, abortIP);
                    if(abortIP){
                        return;
                    }
                }
                auto cycle_string = cycle_vector_to_string(odd_hole);
                odd_holes.try_emplace(cycle_string, std::move(odd_hole));

            }
        }

        // if no chord, no odd-hole, then add i to the path and continue recursion.
        else if(!chord_exist){
            path_vector.push_back(i);
            oddHoleRecursive(graph_adj_mat, graph_adj_list, path_vector, odd_holes, is_anti_hole_search, is_for_heuristic, inputGraph, optionalEdgesGraph, precheck, abortIP);
        }
    }

    path_vector.pop_back();
    return;
}

std::map<std::string, std::vector<int>> findOddHoles(std::vector<std::vector<bool>>& graph_adj_mat, bool is_anti_hole_search, const bool& is_for_heuristic, std::vector<std::vector<bool>>& inputGraph, std::vector<std::vector<bool>>& optionalEdgesGraph,  bool& precheck, bool& abortIP){

    // std::cout << "findOddHolesCalled. is_anti_hole_search: " << is_anti_hole_search << std::endl;

    // Create Adjacency List for the graph.
    std::vector<std::vector<int>> graph_adj_list (graph_adj_mat.size());
    for(int i = 0; i < graph_adj_mat.size(); ++i){
        for(int j = i + 1; j < graph_adj_mat.size(); ++j){
            if(graph_adj_mat[i][j]){
                graph_adj_list[i].push_back(j);
                graph_adj_list[j].push_back(i);
            }
        }
    }

    std::map<std::string, std::vector<int>> odd_holes;

    for(int i = 0; i < graph_adj_mat.size(); ++i){
        std::vector<int> path_vector = {i};
        path_vector.reserve(graph_adj_mat.size());
        oddHoleRecursive(graph_adj_mat, graph_adj_list, path_vector, odd_holes, is_anti_hole_search, is_for_heuristic, inputGraph, optionalEdgesGraph, precheck, abortIP);
    }

    // Printing Odd Holes
    // if(!odd_holes.empty()){
    //     std::cout << "printing odd cycles: " << std::endl;
    //     for(const auto& [key, cycle]: odd_holes){
    //         for(const auto& v: cycle){
    //             std::cout << v << ", ";
    //         }
    //         std::cout << std::endl;
    //     }
    // }

    // Printing number of odd holes
    // std::cout << "Number of odd holes: " << odd_holes.size() << std::endl;

    return odd_holes;
}

std::string get_string_rep_of_vp(int v1, int v2){
    std::string vp_str;
    if(v1 < v2){
        vp_str = std::to_string(v1) + "_" + std::to_string(v2);
    }
    else{
        vp_str = std::to_string(v2) + "_" + std::to_string(v1);
    }
    return vp_str;
}

std::pair<int, int> get_two_integers_in_vp_string(std::string input){

    size_t underscorePos = input.find("_");
    
    std::string s1 = input.substr(0, underscorePos);
    std::string s2 = input.substr(underscorePos + 1);

    int v1 = std::stoi(s1);
    int v2 = std::stoi(s2);

    if(v1 < v2){
        return std::pair<int, int>(v1, v2);
    }
    else{
        return std::pair<int, int>(v2, v1);
    }
}

std::vector<std::vector<bool>> get_complement_of_graph(const std::vector<std::vector<bool>> &graph){

    int n = graph.size();

    std::vector<std::vector<bool>> complement_graph(n, std::vector<bool>(n, false));

    for(int i = 0; i < n - 1; i++){
        for(int j = 0; j < n; j++){
            if(!graph[i][j] && i != j){
                complement_graph[i][j] = 1;
                complement_graph[j][i] = 1;
            }
        }
    }

    return complement_graph;
}

struct OddHoleWorker{

    //GraphParamMatrix& x;
    std::vector<std::vector<int>>& flattenIndexMatrix;
    std::vector<std::vector<bool>>& inputGraph;
    std::vector<std::vector<bool>>& optionalEdgesGraph;
    IloEnv& env;

    int nWorkerOddHoleAlgCall = 0;
    int nWorkerFoundOddHoles = 0;
    int nWorkerFoundOddAntiHoles = 0;
    int nWorkerFoundFeasSols = 0;
    int n_worker_heuristic_called = 0;
    int n_worker_oh_and_ohbar_cut_size = 0;

    long long timeSpentInOddHoleAlgWorker = 0;

    OddHoleWorker(std::vector<std::vector<int>>& flattenIndexMatrixIn, std::vector<std::vector<bool>>& inputGraphIn, std::vector<std::vector<bool>>& optionalEdgesGraphIn, IloEnv& envIn);
    std::map<std::string, std::vector<int>> callFindOddHoles(std::vector<std::vector<bool>>& graph, IloRangeArray& oddHoleCuts, IloNumVarArray& x_flat, bool isAntiHoleSearch, const IloCplex::Callback::Context& context, std::vector<std::vector<bool>>& inputGraph, std::vector<std::vector<bool>>& optionalEdgesGraph);
};

OddHoleWorker::OddHoleWorker(std::vector<std::vector<int>>& flattenIndexMatrixIn, std::vector<std::vector<bool>>& inputGraphIn, std::vector<std::vector<bool>>& optionalEdgesGraphIn, IloEnv& envIn) : flattenIndexMatrix(flattenIndexMatrixIn), inputGraph(inputGraphIn), optionalEdgesGraph(optionalEdgesGraphIn), env(envIn)
{}

// Find odd holes, write to expression array as constraints
std::map<std::string, std::vector<int>> OddHoleWorker::callFindOddHoles(std::vector<std::vector<bool>>& graph, IloRangeArray& oddHoleCuts, IloNumVarArray& x_flat, bool isAntiHoleSearch, const IloCplex::Callback::Context& context, std::vector<std::vector<bool>>& inputGraph, std::vector<std::vector<bool>>& optionalEdgesGraph){

    // Printing Callback Input
    // std::cout << "Odd Hole Alg. called with graph: " << std::endl;
	/* for(int i = 0; i < graph.size(); ++i){
	    for(int j = 0; j < graph.size(); ++j){
            std::cout << graph[i][j] << ",";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl; */

    nWorkerOddHoleAlgCall++;

    auto timeOddHoleStart = std::chrono::high_resolution_clock::now();
    bool precheck = false;
    bool dummy_bool = false;
    std::map<std::string, std::vector<int>> foundOddCycles = findOddHoles(graph, isAntiHoleSearch, false, inputGraph, optionalEdgesGraph, precheck, dummy_bool);

    auto timeOddHoleEnd = std::chrono::high_resolution_clock::now();
    long long timeElapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(timeOddHoleEnd - timeOddHoleStart).count();
    timeSpentInOddHoleAlgWorker += timeElapsed;

    bool returnValue = false;

    // add found oddcycles as constraints
    if(!foundOddCycles.empty()){

        returnValue = true;

        if(!isAntiHoleSearch){ // if working on the graph itself, add odd hole constraint

            for(const auto& [key, cycle]: foundOddCycles){


                IloExpr oddHoleConstraint(env);
                for(int i = 0; i < cycle.size(); ++i){
                    for(int j = i + 1; j < cycle.size(); ++j){
                        if(!inputGraph[cycle[i]][cycle[j]] && optionalEdgesGraph[cycle[i]][cycle[j]]){ // Add a term if pair is a non-edge in the input graph, also option

                            if( (j == i + 1) || (i==0 && j == (cycle.size() - 1)) ){ // edge on the cycle
                                oddHoleConstraint += ( 1 - x_flat[flattenIndexMatrix[cycle[i]][cycle[j]]] );
                            }
                            else{
                                oddHoleConstraint += x_flat[flattenIndexMatrix[cycle[i]][cycle[j]]];
                            }
                        }
                    }
                }

                oddHoleCuts.add(oddHoleConstraint >= 1);
                oddHoleConstraint.end();
            }

            nWorkerFoundOddHoles += foundOddCycles.size();
        }
        else{ // if working on the complement of the graph, add odd anti hole constraint
            for(const auto& [key, cycle]: foundOddCycles){


                IloExpr oddHoleConstraint(env);
                for(int i = 0; i < cycle.size(); ++i){
                    for(int j = i + 1; j < cycle.size(); ++j){
                        if(!inputGraph[cycle[i]][cycle[j]] && optionalEdgesGraph[cycle[i]][cycle[j]]){ // Add a term if pair is a non-edge in the input graph, also option

                            if( (j == i + 1) || (i==0 && j == (cycle.size() - 1)) ){ // edge on the cycle
                                oddHoleConstraint += x_flat[flattenIndexMatrix[cycle[i]][cycle[j]]];
                            }
                            else{
                                oddHoleConstraint += ( 1 - x_flat[flattenIndexMatrix[cycle[i]][cycle[j]]] );
                            }
                        }
                    }
                }

                oddHoleCuts.add(oddHoleConstraint >= 1);
                oddHoleConstraint.end();

            }
            nWorkerFoundOddAntiHoles += foundOddCycles.size();
        }
    }
    return foundOddCycles;
}

struct Generic_callback : public IloCplex::Callback::Function
{
    IloEnv& env;
    std::vector<OddHoleWorker*> workers;
    IloNumVarArray& x_flat;
    std::vector<std::vector<int>>& flattenIndexMatrix;
    std::vector<std::vector<bool>>& inputGraph;
    std::vector<std::vector<bool>>& optionalEdgesGraph;

    int nVertices;
    // int nOfVertexPairs;
    int nVariables;

    Generic_callback(IloNumVarArray& xIn, int nThreads, int nVerticesIn, int nVariablesIn, std::vector<std::vector<int>>& flattenIndexMatrix, std::vector<std::vector<bool>>& inputGraph, std::vector<std::vector<bool>>& optionalEdgesGraph, IloEnv& env);

    void invoke(const IloCplex::Callback::Context& context);
};

Generic_callback::Generic_callback(IloNumVarArray& xIn, int nThreads, int nVerticesIn, int nVariablesIn, std::vector<std::vector<int>>& flattenIndexMatrixIn, std::vector<std::vector<bool>>& inputGraphIn, std::vector<std::vector<bool>>& optionalEdgesGraphIn, IloEnv& envIn) : x_flat(xIn),
 nVertices(nVerticesIn), nVariables(nVariablesIn), flattenIndexMatrix(flattenIndexMatrixIn), inputGraph(inputGraphIn), optionalEdgesGraph(optionalEdgesGraphIn), env(envIn)
{
	workers.resize(nThreads);
	for (int i = 0; i < nThreads; ++i)
		workers[i] = new OddHoleWorker(flattenIndexMatrixIn, inputGraph, optionalEdgesGraph, env);
}

void Generic_callback::invoke(const IloCplex::Callback::Context& context){

	//std::cout << "Generic_callback::invoke has been called" << std::endl;

    int const threadNo = context.getIntInfo(IloCplex::Callback::Context::Info::ThreadId);
    IloEnv env = context.getEnv();
    auto context_id = context.getId();

    if (context_id != IloCplex::Callback::Context::Id::Relaxation && context_id != IloCplex::Callback::Context::Id::Candidate)
		std::cout << "Unexpected context type at Callback!" << std::endl;

    if(context_id == IloCplex::Callback::Context::Id::Candidate){

        //std::cout << "In candidate callback" << std::endl;

        // Get candidate, current solution
        IloNumArray xVal(env);
        context.getCandidatePoint(x_flat, xVal);
        
        
        // create the graph corresponding to the current solution
        std::vector<std::vector<bool>> candidate_graph = inputGraph;
        for(int ind = 0; ind < nVariables; ++ind){

            if(xVal[ind] == 1){
                int i = x_flat[ind].getIntProperty("i");
                int j = x_flat[ind].getIntProperty("j");

                candidate_graph[i][j] = true;
                candidate_graph[j][i] = true;
            }
        }
        // construct its complement complement
        std::vector<std::vector<bool>> complementGraph = get_complement_of_graph(candidate_graph);
        // std::vector<std::vector<bool>> complementGraph(nVertices, std::vector<bool>(nVertices, false));
        // for(int i = 0; i < nVertices; i++){
        //     for(int j = i + 1; j < nVertices; j++){
        //         if(!candidate_graph[i][j]){
        //             complementGraph[i][j] = true;
        //             complementGraph[j][i] = true;
        //         }
        //     }
        // }

        // Call worker here
        OddHoleWorker* pWorker = workers[threadNo];

        IloRangeArray cuts(env);

        std::map<std::string, std::vector<int>> oddHolesFound = pWorker->callFindOddHoles(candidate_graph, cuts, x_flat, false, context, inputGraph, optionalEdgesGraph);
        std::map<std::string, std::vector<int>> oddAntiHolesFound = pWorker->callFindOddHoles(complementGraph, cuts, x_flat, true, context, inputGraph, optionalEdgesGraph);


        if((oddHolesFound.size() > 0) || (oddAntiHolesFound.size() > 0)){
            // add constraints, reject current candidate solution
            pWorker->n_worker_oh_and_ohbar_cut_size += cuts.getSize();
            // std::cout << "==================" << std::endl;
            // std::cout << "Cuts Size      : " << cuts.getSize() << std::endl;
            // std::cout << "oh + ohbar size: " << oddHolesFound.size() + oddAntiHolesFound.size() << std::endl;
            context.rejectCandidate(cuts);
        }
        else{
            pWorker->nWorkerFoundFeasSols++;
        }
        cuts.end();
    }
}

struct Stats{

    std::chrono::high_resolution_clock::time_point timeStarted;
    long long time_elapsed;
    long long time_elapsed_in_precheck;
    bool isFeasible;
    bool isUnknown;
    bool isInfeasibleByPrecheck;
    bool isInfeasibleByCplex;
    bool problematic_case = false;
    // double optimalityGap;
    // double objectiveValue;
    int nFeasibleSolutionsFound = 0;
    int n_times_heuristic_called = 0;
    int n_added_oh_ohbar_cuts = 0;
    
    int nOddHoleAlgCalls = 0;
    int nOddHoles = 0;
    int nOddAntiHoles = 0;
    long long timeSpentInOddHoleAlg = 0;

    void start_time();
    void collect_info(IloCplex& cplex, Generic_callback& candidate_callback, bool& precheck_found_infeasible);
    void calculate_time_in_precheck();
};

void Stats::start_time(){
    timeStarted = std::chrono::high_resolution_clock::now();
}

void Stats::calculate_time_in_precheck(){
    std::chrono::high_resolution_clock::time_point time_after_precheck = std::chrono::high_resolution_clock::now();
    time_elapsed_in_precheck = std::chrono::duration_cast<std::chrono::milliseconds>(time_after_precheck - timeStarted).count();
}

void Stats::collect_info(IloCplex& cplex, Generic_callback& candidate_callback, bool& precheck_found_infeasible){
    

    std::chrono::high_resolution_clock::time_point timeEnded = std::chrono::high_resolution_clock::now();
    time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(timeEnded - timeStarted).count();
    if(!precheck_found_infeasible){
        for (auto& worker : candidate_callback.workers){
            nOddHoleAlgCalls += worker -> nWorkerOddHoleAlgCall;
            nOddHoles += worker -> nWorkerFoundOddHoles;
            nOddAntiHoles += worker -> nWorkerFoundOddAntiHoles;
            timeSpentInOddHoleAlg += worker -> timeSpentInOddHoleAlgWorker;
            nFeasibleSolutionsFound += worker -> nWorkerFoundFeasSols;
            n_times_heuristic_called += worker -> n_worker_heuristic_called;
            n_added_oh_ohbar_cuts += worker -> n_worker_oh_and_ohbar_cut_size;
        }
    }
    /* IloAlgorithm::Status status = cplex.getStatus();
    if(status == IloAlgorithm::Optimal){
        isOptimal = true;
    }
    else{isOptimal = false;} */
    
    isFeasible = false;
    isUnknown = false;
    isInfeasibleByPrecheck = false;
    isInfeasibleByCplex = false;
    //  Infeasible by CPLEX
    if(precheck_found_infeasible){
        isInfeasibleByPrecheck = true;
    }
    else if(cplex.getStatus() == IloAlgorithm::Infeasible){
        isInfeasibleByCplex = true;
    }
    // Feasible or Optimal
    else if((cplex.getStatus() == IloAlgorithm::Feasible) || (cplex.getStatus() == IloAlgorithm::Optimal)){
        isFeasible = true;
    }
    // Unknown
    else if(cplex.getStatus() == IloAlgorithm::Unknown){
        isUnknown = true;
    }
    else{
        std::cout << "UNCONTROLLED OUTPUT!" << std::endl;

        std::cerr << "UNCONTROLLED OUTPUT!" << std::endl;
        std::cerr << cplex.getStatus() << std::endl;
        problematic_case = true;
    }
}

int main(int argc, char* argv[]){

    Stats programStats;
    programStats.start_time();

    IloEnv env;
    IloModel model(env);

    // Parameter Control
    if (argc < 5) {
        std::cerr << "Too few arguments for: " << argv[0] << "!" << std::endl;
        return 1;
    }

    // Read Program Parameters
    std::string parameterFileName = argv[1];
    std::ifstream parameterFile(parameterFileName);

    if (!parameterFile) {
        std::cerr << "Error opening parameter file" << std::endl;
        return 1;
    }

    std::unordered_map<std::string, std::string> parameters;
    std::string param, value;
    while (parameterFile >> param >> value) { // Read parameters and values
        parameters[param] = value;
    }

    parameterFile.close();


    // Read Input Graph. e_ij : Input graph's adjacency matrix
    std::string inputGraphFileName = argv[2];
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

    // Read Optional Edges Graph
    std::string optionalEdgesGraphFileName = argv[5];
    std::ifstream optionalEdgesGraphFile(optionalEdgesGraphFileName);
    std::vector<std::vector<bool>> optionalEdgesGraph;
    if (!optionalEdgesGraphFile) {
        std::cerr << "Unable to open optionalEdgesGraphFile" << std::endl;
        return 1;
    }
    while (std::getline(optionalEdgesGraphFile, line)) {
        std::vector<bool> row;
        std::istringstream stream(line);
        std::string value;
        
        while (std::getline(stream, value, ',')) {
            row.push_back(value == "1");
        }

        optionalEdgesGraph.push_back(row);
    }
    optionalEdgesGraphFile.close();

    // Parsing inputGraphFileName to get p parameter
    std::size_t pos = inputGraphFileName.find("density_");
    double inputGraph_p_Param;
    if (pos != std::string::npos) {
        std::string p_str = inputGraphFileName.substr(pos + 8); // 8 is the length of "density_"
        
        std::size_t end_pos = p_str.find_first_not_of("0123456789.");
        if (end_pos != std::string::npos) {
            inputGraph_p_Param = std::stod(p_str.substr(0, end_pos));
        }
        
    } 
    else {
        std::cout << "p_parameter not found in the inputGraphFileName" << std::endl;
    }

    int nVertices = inputGraph.size();
    int mEdgesInput = 0;
    int nOfVertexPairs = ((nVertices * nVertices) - nVertices ) / 2; // n choose 2
    std::vector<std::vector<int>> flattenIndexMatrix(nVertices, std::vector<int>(nVertices, 0));

    // Integrate Parameters into the model
	int nThreads = std::stoi(parameters["N_THREADS"]);

    // Odd Hole Termination Count Calculation:
    double odd_hole_termination_percentage = std::stod(parameters["ODD_HOLE_TERMINATION_PERCENTAGE"]);
    int expected_no_of_odd_holes = static_cast<int>(expected_value_of_odd_holes(nVertices, inputGraph_p_Param));
    int expected_no_of_odd_anti_holes = static_cast<int>(expected_value_of_odd_anti_holes(nVertices, inputGraph_p_Param));
    if(odd_hole_termination_percentage == 999){
        std::cout << "ODD_HOLE_TERMINATION_PERCENTAGE is set to 999" << std::endl;
        std::cout << "Program searches for ALL odd holes and odd antiholes" << std::endl;
        ODD_HOLE_TERMINATION_BATCH_SIZE = 0; // Zero means do not terminate based on the found odd hole count
        ODD_ANTI_HOLE_TERMINATION_BATCH_SIZE = 0; // Zero means do not terminate based on the found odd anti hole count
    }
    else if(odd_hole_termination_percentage == 1){
        std::cout << "ODD_HOLE_TERMINATION_PERCENTAGE is set to 1" << std::endl;
        std::cout << "Program searches for JUST ONE odd hole or odd antihole" << std::endl;
        ODD_HOLE_TERMINATION_BATCH_SIZE = 1;
        ODD_ANTI_HOLE_TERMINATION_BATCH_SIZE = 1;
    }
    else if(odd_hole_termination_percentage == 0){
        std::cout << "ERROR: ODD_HOLE_TERMINATION_PERCENTAGE is set to 0" << std::endl;
        exit(0);
    }
    else{
        ODD_HOLE_TERMINATION_BATCH_SIZE = static_cast<int>(odd_hole_termination_percentage * expected_no_of_odd_holes / 100);
        ODD_ANTI_HOLE_TERMINATION_BATCH_SIZE = static_cast<int>(odd_hole_termination_percentage * expected_no_of_odd_anti_holes / 100);
    }

    std::cout << "==============================================" << std::endl;
    std::cout << "n:                                    " << nVertices << std::endl;
    std::cout << "ODD_HOLE_TERMINATION_PERCENTAGE:      " << odd_hole_termination_percentage << std::endl;
    std::cout << "ODD_HOLE_TERMINATION_BATCH_SIZE:      " << ODD_HOLE_TERMINATION_BATCH_SIZE << std::endl;
    std::cout << "ODD_ANTI_HOLE_TERMINATION_BATCH_SIZE: " << ODD_ANTI_HOLE_TERMINATION_BATCH_SIZE << std::endl;
    std::cout << "==============================================" << std::endl;

    // Objective Expression
    IloExpr objectiveExp(env);

    // Decision Var. x_{i,j}
    // Upper Triangular Entries Only: i < j
    int nVariables = 0;
    for(int i = 0; i < nVertices; ++i){
        for(int j = i + 1; j < nVertices; ++j){
            if(!inputGraph[i][j] && optionalEdgesGraph[i][j]){
                nVariables++;
            }
        }
    }
    IloNumVarArray x_flat(env, nVariables);
    int tempIndex = 0;
    for(int i = 0; i < nVertices; ++i){
        for(int j = i + 1; j < nVertices; ++j){

            if(!inputGraph[i][j] && optionalEdgesGraph[i][j]){
                flattenIndexMatrix[i][j] = tempIndex;
                flattenIndexMatrix[j][i] = tempIndex;
                x_flat[tempIndex] = IloNumVar(env, 0.0, 1.0, ILOBOOL);
                x_flat[tempIndex].setIntProperty("i", i);
                x_flat[tempIndex].setIntProperty("j", j);
                //x[i][j].setStringProperty("type", "x");
                model.add(x_flat[tempIndex]);

                // adding non-edges to objective expression
                objectiveExp += x_flat[tempIndex];
                tempIndex++;
            }

            if(inputGraph[i][j]) mEdgesInput++;
        }
    }

    print_graph(inputGraph);
    print_graph(optionalEdgesGraph);

    std::cerr << "Running for input graph: " << inputGraphFileName << std::endl;
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::cout << "    Time: " << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S") << std::endl;

    // Adding Objective to the Model
    IloObjective objective = IloMinimize(env, objectiveExp);
	model.add(objective);
    objectiveExp.end();

    bool precheck = true;
    bool precheck_found_infeasible = false;

    auto odd_holes = findOddHoles(inputGraph, false, true, inputGraph, optionalEdgesGraph, precheck, precheck_found_infeasible);

    if(precheck_found_infeasible){
        std::cerr << "PRECHECK FOUND INFEASIBLE because of Odd-hole" << std::endl;
    }
    // std::cout << "Odd hole count:" << odd_holes.size() << std::endl;

    auto complement_of_input_graph = get_complement_of_graph(inputGraph);
    std::map<std::string, std::vector<int>> odd_anti_holes;

    if(!precheck_found_infeasible){
        odd_anti_holes = findOddHoles(complement_of_input_graph, true, true, inputGraph, optionalEdgesGraph, precheck, precheck_found_infeasible);
        if(precheck_found_infeasible){
            std::cerr << "PRECHECK FOUND INFEASIBLE because of Odd anti-hole" << std::endl;
        }
        // std::cout << "Odd anti hole count:" << odd_anti_holes.size() << std::endl;
    }

    // Calculate time in precheck
    programStats.calculate_time_in_precheck();

    // Adding constraints for odd holes
    if(!precheck_found_infeasible && !odd_holes.empty()){

        for(const auto& [key, cycle]: odd_holes){

            IloExpr oddHoleConstraint(env);
            for(int i = 0; i < cycle.size(); ++i){
                for(int j = i + 1; j < cycle.size(); ++j){
                    if(!inputGraph[cycle[i]][cycle[j]] && optionalEdgesGraph[cycle[i]][cycle[j]]){ // Add a term if pair is a non-edge in the input graph, also option

                        if( (j == i + 1) || (i==0 && j == (cycle.size() - 1)) ){ // edge on the cycle
                            oddHoleConstraint += ( 1 - x_flat[flattenIndexMatrix[cycle[i]][cycle[j]]] );
                        }
                        else{
                            oddHoleConstraint += x_flat[flattenIndexMatrix[cycle[i]][cycle[j]]];
                        }
                    }
                }
            }
            model.add(oddHoleConstraint >= 1);
            oddHoleConstraint.end();
        }
    }
    // Adding constraints for odd anti holes
    if(!precheck_found_infeasible && !odd_anti_holes.empty()){
        for(const auto& [key, cycle]: odd_anti_holes){

            IloExpr oddHoleConstraint(env);
            for(int i = 0; i < cycle.size(); ++i){
                for(int j = i + 1; j < cycle.size(); ++j){
                    if(!inputGraph[cycle[i]][cycle[j]] && optionalEdgesGraph[cycle[i]][cycle[j]]){ // Add a term if pair is a non-edge in the input graph, also option

                        if( (j == i + 1) || (i==0 && j == (cycle.size() - 1)) ){ // edge on the cycle
                            oddHoleConstraint += x_flat[flattenIndexMatrix[cycle[i]][cycle[j]]];
                        }
                        else{
                            oddHoleConstraint += ( 1 - x_flat[flattenIndexMatrix[cycle[i]][cycle[j]]] );
                        }
                    }
                }
            }
            model.add(oddHoleConstraint >= 1);
            oddHoleConstraint.end();
        }
    }

    IloCplex cplex(model);

    cplex.setParam(IloCplex::Threads, nThreads);
    cplex.setParam(IloCplex::Param::TimeLimit, std::stoi(parameters["TIME_LIMIT_SECONDS"]));

    // Stop when a feasible solution is found
    cplex.setParam(IloCplex::Param::MIP::Limits::Solutions, 1);

    // cplex verbose
    //cplex.setParam(IloCplex::Param::MIP::Display, 4); // Higher verbosity

    // Callback:
    Generic_callback callback(x_flat, nThreads, nVertices, nVariables, flattenIndexMatrix, inputGraph, optionalEdgesGraph, env);
    // cplex.use(&callback, IloCplex::Callback::Context::Id::Candidate | IloCplex::Callback::Context::Id::Relaxation);
    cplex.use(&callback, IloCplex::Callback::Context::Id::Candidate);

    IloInt number_of_explored_bc_nodes;
    IloInt number_of_unexplored_bc_nodes;


    if(!precheck_found_infeasible){
        try {
            cplex.solve();

            std::cout << "STATUS: "<< cplex.getStatus() << std::endl;

            if(cplex.getStatus() != IloAlgorithm::Infeasible && cplex.getStatus() != IloAlgorithm::Unknown){
                std::cout << "OBJ VALUE: "<< cplex.getObjValue() << std::endl;
                number_of_explored_bc_nodes = cplex.getNnodes();
                number_of_unexplored_bc_nodes = cplex.getNnodesLeft();
                std::cout << "Explored Number of nodes: "<< number_of_explored_bc_nodes << std::endl;
                std::cout << "Unexplored Number of nodes: "<< number_of_unexplored_bc_nodes << std::endl;
            }
        }
        catch (IloException& e) {
            std::cout << "CPLEX Exception: " << e << std::endl;
        }
        catch (...) {
            std::cout << "Unknown Exception" << std::endl;
        }
    }

    /* std::cout << std::endl;
    std::cout << "========================" << std::endl;
    cplex.out() << "Solution status: " << cplex.getStatus() << std::endl;
    cplex.out() << "Best Objective: " << cplex.getBestObjValue() << std::endl;
    cplex.out() << "Ncols: " << cplex.getNcols() << std::endl;""
    cplex.out() << "Nrows: " << cplex.getNrows() << std::endl;
    cplex.out() << "Optimal value: " << cplex.getObjValue() << std::endl; */

    std::cout << "\n\n\nStatus:" << cplex.getStatus() << "\n";
    
    // Stats
    programStats.collect_info(cplex, callback, precheck_found_infeasible);
    
    if(programStats.isFeasible){


        // write output log to a txt
        std::string outputFileName = argv[3];
        std::ofstream outputFile(outputFileName);
    
        // Related to Parameter File
        outputFile << "========================" << std::endl;
        outputFile << "TIME_LIMIT_SECONDS: " << parameters["TIME_LIMIT_SECONDS"] << std::endl;
        outputFile << "ODD_HOLE_TERMINATION_PERCENTAGE: " << parameters["ODD_HOLE_TERMINATION_PERCENTAGE"] << std::endl;
        outputFile << "N_THREADS: " << parameters["N_THREADS"] << std::endl;
        outputFile << "IS_RUNNING_HEURISTIC_ON_CANDIDATE: " << parameters["IS_RUNNING_HEURISTIC_ON_CANDIDATE"] << std::endl;
        outputFile << "IS_RUNNING_HEURISTIC_ON_RELAXATION: " << parameters["IS_RUNNING_HEURISTIC_ON_RELAXATION"] << std::endl;
        outputFile << "ADD_ALL_AT_START: " << parameters["ADD_ALL_AT_START"] << std::endl;
        // outputFile << "SOLVE_FOR_SUBGRAPHS_FOR_VI: " << parameters["SOLVE_FOR_SUBGRAPHS_FOR_VI"] << std::endl;

        // Related to run
        outputFile << "========================" << std::endl;
        outputFile << "Wall-Clock Runtime (ms): " << programStats.time_elapsed << std::endl;
        outputFile << "Calculated Odd Hole Termination Batch Size: " << ODD_HOLE_TERMINATION_BATCH_SIZE << std::endl;
        outputFile << "Total Time of All Workers in OddHoleAlg (nano): " << programStats.timeSpentInOddHoleAlg << std::endl;
        outputFile << "Total Time of All Workers in OddHoleAlg (ms): " << programStats.timeSpentInOddHoleAlg / 1000000 << std::endl;
        outputFile << "OddHoleAlg calls: " << programStats.nOddHoleAlgCalls << std::endl;
        outputFile << "Number of Found Odd-Holes: " << programStats.nOddHoles << std::endl;
        outputFile << "Number of Found Odd Anti-Holes: " << programStats.nOddAntiHoles << std::endl;
        // outputFile << "Is Optimal? (1e-6): " << programStats.isOptimal << std::endl;
        // outputFile << "Optimality Gap: " << programStats.optimalityGap << std::endl;
        // outputFile << "Objective Value: " << programStats.objectiveValue << std::endl;
        outputFile << "Number of Feasible Solutions Found: " << programStats.nFeasibleSolutionsFound << std::endl;
        outputFile << "Number of Times Heuristic is Called: " << programStats.n_times_heuristic_called << std::endl;
        outputFile << "Total number of added OH and OHbar Cuts: " << programStats.n_added_oh_ohbar_cuts << std::endl;
        outputFile << "Number of Explored B&C tree nodes: " << number_of_explored_bc_nodes << std::endl;
        outputFile << "Number of Unexplored B&C tree nodes: " << number_of_unexplored_bc_nodes << std::endl;
        outputFile << "========================" << std::endl;

        // Related to input graph
        outputFile << "Input Graph:" << std::endl;
        outputFile << "Number of Vertices: " << nVertices << std::endl;
        outputFile << "Erdos Renyi p Parameter: " << inputGraph_p_Param << std::endl;
        outputFile << "Number of Edges: " << mEdgesInput << std::endl;
        outputFile << "Expected No of Odd Holes: " << expected_no_of_odd_holes << std::endl;
        outputFile << "Expected No of Odd Anti Holes: " << expected_no_of_odd_anti_holes << std::endl;
        outputFile << "ODD_HOLE_TERMINATION_BATCH_SIZE: " << ODD_HOLE_TERMINATION_BATCH_SIZE << std::endl;
        outputFile << "Graph Density: " << static_cast<double>(mEdgesInput) / nOfVertexPairs << std::endl;
        for(int i = 0; i < nVertices; ++i){
            for(int j = 0; j < nVertices; ++j){
                outputFile << inputGraph[i][j] << ",";
            }
            outputFile << std::endl;
        }

        // Output graph and the changes wrt input graph
        std::vector<std::string> changes;
        std::vector<std::vector<bool>> outputGraph = inputGraph;
        int nChangesRemoved = 0;
        int nChangesAdded = 0;
        for(int ind = 0; ind < nVariables; ++ind){
            auto value = cplex.getValue(x_flat[ind]);
            int i = x_flat[ind].getIntProperty("i");
            int j = x_flat[ind].getIntProperty("j");

            outputGraph[i][j] = value;
            outputGraph[j][i] = value;
            if(value != inputGraph[i][j]){
                if(value == 1){
                    changes.push_back("Added Edge: (" + std::to_string(i) + ", " + std::to_string(j) + ")");
                    nChangesAdded+=2;
                }
                else{
                    changes.push_back("Removed Edge: (" + std::to_string(i) + ", " + std::to_string(j) + ")");
                    nChangesRemoved+=2;
                }
            }
        }

        int mEdgesOutput = 0;
        for(int i = 0; i < nVertices; i++){
            for(int j = i+1; j < nVertices; j++){
                if(outputGraph[i][j]){
                    mEdgesOutput++;
                }
            }
        }

        // Related to output graph
        outputFile << "========================" << std::endl;
        outputFile << "Output Graph: " << std::endl;
        outputFile << "Number of Vertices: " << nVertices << std::endl;
        outputFile << "Number of Edges: " << mEdgesOutput << std::endl;
        outputFile << "Graph Density: " << static_cast<double>(mEdgesOutput) / nOfVertexPairs << std::endl;
        for(int i = 0; i < nVertices; ++i){
            for(int j = 0; j < nVertices; ++j){
                outputFile << outputGraph[i][j] << ",";
            }
            outputFile << std::endl;
        }
        outputFile << "Is Output Graph Empty or Complete?: " << is_graph_empty_or_complete(outputGraph) <<std::endl;
        outputFile << "Ratio of Added Edges Over Added + Removed Edges: " << static_cast<double>(nChangesAdded) / (nChangesAdded + nChangesRemoved + 1e-6) << std::endl;
        outputFile << "Changes from Input to Output: " << std::endl;
        for(auto line: changes) outputFile << line << std::endl;
        outputFile << "========================" << std::endl;

        outputFile.close();

    }
    // Summary Output to experiment csv
    std::string experimentCsvFileName = argv[4];
    std::ofstream experimentCsvFile;

    experimentCsvFile.open(experimentCsvFileName, std::ios::app);
    if(!experimentCsvFile){
        std::cerr << "Error opening experimentCsvFile" << std::endl;
        return 1;
    }
    std::string summary;
    // Format:
    summary += inputGraphFileName + ",";
    summary += parameterFileName + ",";
    summary += parameters["TIME_LIMIT_SECONDS"] + ",";
    summary += parameters["ODD_HOLE_TERMINATION_PERCENTAGE"] + ",";
    summary += parameters["N_THREADS"] + ",";
    summary += std::to_string(nVertices) + ",";
    summary += std::to_string(inputGraph_p_Param) + ",";
    summary += std::to_string(ODD_HOLE_TERMINATION_BATCH_SIZE) + ",";
    summary += std::to_string(programStats.time_elapsed) + ",";
    summary += std::to_string(programStats.time_elapsed_in_precheck) + ",";
    summary += std::to_string(programStats.isFeasible) + ",";;
    summary += std::to_string(programStats.isInfeasibleByPrecheck) + ",";;
    summary += std::to_string(programStats.isInfeasibleByCplex) + ",";;
    summary += std::to_string(programStats.isUnknown);

    if(programStats.problematic_case){
        std::cerr << inputGraphFileName << std::endl;
    }

    experimentCsvFile << summary << std::endl;
    experimentCsvFile.close();

    std::cout << "Program finished OK!" << std::endl;
    // std::cerr << "Program finished OK!" << std::endl;
    now = std::chrono::system_clock::now();
    now_time = std::chrono::system_clock::to_time_t(now);
    std::cout << "    Time: " << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S") << std::endl;

    return 0;
}