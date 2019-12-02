#ifndef PEA_P1_TSPALGORITHMSTEST_H
#define PEA_P1_TSPALGORITHMSTEST_H

#include <vector>

#include "../utilities/TSPUtils.h"
#include "../algorithms/TSPExactAlgorithms.h"
#include "../algorithms/TSPGreedyAlgorithms.h"


class TSPAlgorithmsTest {
public:
    void run() const;

private:
    void bruteForceTest() const;
    void bruteForceTreeTest() const;
    void dynamicProgrammingHeldKarpTest() const;
    void branchAndBoundTest() const;
    void nearestNeighbourTest() const;
    void greedyTest() const;

    // instanceFiles: map with paths to the instances in form {<directory of instances>, <vector with instance file names>}
    // first file name in the vector is a name of a solution file for instances in the directory
    void testAlgorithm(const std::map<std::string, std::vector<std::string>> &instanceFiles,
                       int (*tspAlgorithm)(const IGraph *, std::vector<int> &),
                       bool isSolutionApproximated, const std::string &testName) const;

    bool isSolutionValid(IGraph *tspInstance, const std::vector<int> &solutionPermutation,
                         int solutionPathCost) const;
};


#endif //PEA_P1_TSPALGORITHMSTEST_H
