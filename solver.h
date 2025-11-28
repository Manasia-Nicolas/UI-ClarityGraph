#pragma once
#include <vector>
#include <utility>

class Solver {
public:
    ///struct Result {
        ///std::vector<std::pair<double,double>> coords;   // P_i.x , P_i.y
        ///std::vector<int> assignment;                    // node v → point index P_i
        ///int r;                                          // grid size
    ///};

    // Main function you will call from UI
    static std::pair<int, std::vector<std::pair<double, double>>> computeLayout(
        int V,
        int E,
        const std::vector<std::vector<int>>& adj,
        int heuristicIndex
        );
};
