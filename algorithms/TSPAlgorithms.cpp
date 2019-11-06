#include "TSPAlgorithms.h"

int TSPAlgorithms::bruteForce(const IGraph *tspInstance) {
    // Get size of the ATSP instance
    int permutationSize = tspInstance->getVertexCount();

    // (permutationSize - 1) is an index of the fixed start vertex
    --permutationSize;

    // Initialize natural permutation
    std::vector<int> permutation(permutationSize);
    for (int position = 0; position < permutationSize; ++position) {
        permutation[position] = position;
    }

    // Holds currently processed (sub)permutation last element's index
    int stackSlotIndex = 1;
    // Holds number of swap operation to be performed on (sub)permutations
    std::vector<int> stackCounters(permutationSize, 1);
    // Holds index of permutation's element to swap with last element in the permutation
    int swapIndex;
    // Current permutation's target function value
    int currentPathTargetFunctionValue;

    // Check target function value for natural permutation and take as best for now
    int bestPathTargetFunctionValue = TSPUtils::calculateTargetFunctionValue(tspInstance, permutationSize,
                                                                             permutation);
    do {
        // Test if there are still available swaps in (sub)permutation to be performed
        // (stackSlotIndex + 1) is a position (not index) of stack's slot
        if (stackCounters[stackSlotIndex] <= stackSlotIndex) {
            // Choose swap position with Heap's rule
            if ((stackSlotIndex + 1) % 2 == 1) {
                swapIndex = 0;
            } else {
                swapIndex = stackCounters[stackSlotIndex] - 1;
            }
            // Do the swap to generate next permutation
            std::swap(permutation[stackSlotIndex], permutation[swapIndex]);
            // Update number of swap of the permutation to be performed
            ++stackCounters[stackSlotIndex];
            // Generate sub-permutations
            stackSlotIndex = 1;

            // Compare with current best permutation and update if better solution was found
            currentPathTargetFunctionValue = TSPUtils::calculateTargetFunctionValue(tspInstance, permutationSize,
                                                                                    permutation);
            if (currentPathTargetFunctionValue < bestPathTargetFunctionValue) {
                bestPathTargetFunctionValue = currentPathTargetFunctionValue;
            }
        } else {
            // Process permutations with bigger size
            stackCounters[stackSlotIndex] = 1;
            ++stackSlotIndex;
        }

        // Repeat until all permutations are processed
    } while ((stackSlotIndex + 1) <= permutationSize);

    return bestPathTargetFunctionValue;
}

int TSPAlgorithms::dynamicProgrammingHeldKarp(const IGraph *tspInstance) {
    // Get size of the ATSP instance
    // (nVertex - 1) is an index of the fixed start vertex
    const int nVertex = tspInstance->getVertexCount();

    // Number of subsets of sets with size (nVertex - 1)
    const unsigned int pathSetCount = 1u << (nVertex - 1);

    // Initialize 2D std::vector storing computed costs of partial paths
    // First dimension denote an end-on-path vertex
    // Second dimension denote set of go-trough vertices:
    //      The set is stored as an unsigned int where "1" bit denote that vertex is in set and
    //      "0" bit denote that set is not in the set
    std::vector<std::vector<int>> partialPathCosts(nVertex, std::vector<int>(pathSetCount));

    // Go trough all [0, 1, ..., nVertex - 2] vertices
    for (int vertexIdx = 0; vertexIdx < nVertex - 1; ++vertexIdx) {
        // Go trough all possible sets of vertices
        for (unsigned int pathSet = 0; pathSet < pathSetCount; ++pathSet) {
            partialPathCosts[vertexIdx][pathSet] = -1;
        }
        // opt({q}, q) = dist(x, q)
        partialPathCosts[vertexIdx][1u << vertexIdx] = tspInstance->getEdgeParameter(nVertex - 1, vertexIdx);
    }

    // Find best path cost
    int pathCost, bestPathCost = std::numeric_limits<int>::max();
    // A set witch contains all vertices without the starting one
    unsigned int fullPathSet = (1u << (nVertex - 1)) - 1;
    for (int endVertexIdx = 0; endVertexIdx < nVertex - 1; ++endVertexIdx) {
        // v∗ = min(opt(N, t) + dist(t, x) : t ∈ N)
        pathCost = dpGetPartialPathCost(fullPathSet, endVertexIdx, partialPathCosts, tspInstance) +
                   tspInstance->getEdgeParameter(endVertexIdx, nVertex - 1);
        // Update best path cost
        if (pathCost < bestPathCost) {
            bestPathCost = pathCost;
        }
    }
    return bestPathCost;
}

int TSPAlgorithms::dpGetPartialPathCost(unsigned int partialPathSet, int endVertexIdx,
                                        std::vector<std::vector<int>> &partialPathCostTable,
                                        const IGraph *tspInstance) {
    const int nVertex = tspInstance->getVertexCount();
    int partialPathCost;
    int bestPartialPathCost = std::numeric_limits<int>::max();

    // Subset is the set without the end vertex
    unsigned int partialPathSubset;
    // Compute partial path cost if not available, otherwise return the cost
    if (partialPathCostTable[endVertexIdx][partialPathSet] == -1) {
        // Exclude end vertex from the set
        partialPathSubset = partialPathSet & ~(1u << endVertexIdx);
        for (int vertexIdx = 0; vertexIdx < nVertex - 1; ++vertexIdx) {
            // Continue if current vertex is not in the set
            if (!(partialPathSubset & (1u << vertexIdx))) {
                continue;
            }
            // opt(S, t) = min(opt(S \ {t}, q) + dist(q, t) : q ∈ S \ {t})
            partialPathCost = dpGetPartialPathCost(partialPathSubset, vertexIdx, partialPathCostTable, tspInstance) +
                              tspInstance->getEdgeParameter(vertexIdx, endVertexIdx);
            // Update best partial path cost
            if (partialPathCost < bestPartialPathCost) {
                bestPartialPathCost = partialPathCost;
            }
        }
        // Save computed partial path cost for further reuse
        partialPathCostTable[endVertexIdx][partialPathSet] = bestPartialPathCost;
    }
    return partialPathCostTable[endVertexIdx][partialPathSet];
}

int TSPAlgorithms::branchAndBound(const IGraph *tspInstance) {
    const int instanceSize = tspInstance->getVertexCount();

    BBNodeData initNode(instanceSize, 0);
    for (int i = 0; i != instanceSize; ++i) {
        for (int j = 0; j != instanceSize; ++j) {
            if (i == j) {
                initNode.distances[i][j] = std::numeric_limits<int>::max();
            }
            initNode.distances[i][j] = tspInstance->getEdgeParameter(i, j);
        }
    }

    auto bbNodeComparator =
            [](const BBNodeData &lhs, const BBNodeData &rhs) -> bool {
                if (lhs.lowerBound == rhs.lowerBound) {
                    return lhs.edgesOnPath < rhs.edgesOnPath;
                }
                return lhs.lowerBound > rhs.lowerBound;
            };
    std::priority_queue<BBNodeData, std::vector<BBNodeData>, decltype(bbNodeComparator)> leafs(bbNodeComparator);

    std::vector<int> naturalPermutation(instanceSize);
    for (int i = 0; i != naturalPermutation.size(); ++i) {
        naturalPermutation[i] = i;
    }
    int upperBound = TSPUtils::calculateTargetFunctionValue(tspInstance, naturalPermutation);

    bbCalculateLowerBoundAndDesignateHighestZeroPenalty(initNode);
    leafs.push(initNode);
    BBNodeData leftNode, rightNode;
    while (leafs.top().lowerBound < upperBound) {
        leftNode = leafs.top();
        rightNode = leafs.top();
        leafs.pop();

        bbCreateLeftNodeData(leftNode);
        if (leftNode.lowerBound < upperBound) {
            leafs.push(leftNode);
        }

        bbCreateRightNodeData(rightNode);
        if (rightNode.lowerBound < upperBound) {
            leafs.push(rightNode);
        }

        if (rightNode.edgesOnPath == instanceSize - 2) {
            upperBound = bbGetCycleCost(rightNode);
        }
    }
    return upperBound;
}

void TSPAlgorithms::bbCalculateLowerBoundAndDesignateHighestZeroPenalty(BBNodeData &nodeData) {
    // pair: <Indexes of zero, penalties>
    std::list<std::pair<EdgeCities, Penalties>> matrixZeroes;

    int rowMinimum;
    for (int i = 0; i != nodeData.distances.size(); ++i) {
        rowMinimum = *std::min_element(nodeData.distances[i].begin(), nodeData.distances[i].end());
        if (rowMinimum == std::numeric_limits<int>::max()) {
            continue;
        }
        for (int j = 0; j != nodeData.distances.size(); ++j) {
            if (nodeData.distances[i][j] == std::numeric_limits<int>::max()) {
                continue;
            }
            nodeData.distances[i][j] -= rowMinimum;
            if (nodeData.distances[i][j] == 0) {
                matrixZeroes.emplace_back(std::pair<EdgeCities, Penalties>(EdgeCities(i, j), Penalties()));
            }
        }
        nodeData.lowerBound += rowMinimum;
    }

    int columnMinimum;
    for (int j = 0; j != nodeData.distances.size(); ++j) {
        columnMinimum = nodeData.distances[0][j];
        for (int i = 1; i != nodeData.distances.size(); ++i) {
            if (nodeData.distances[i][j] < columnMinimum) {
                columnMinimum = nodeData.distances[i][j];
            }
        }
        if (columnMinimum == std::numeric_limits<int>::max() || columnMinimum == 0) {
            continue;
        }
        for (int i = 0; i != nodeData.distances.size(); ++i) {
            if (nodeData.distances[i][j] == std::numeric_limits<int>::max()) {
                continue;
            }
            nodeData.distances[i][j] -= columnMinimum;
            if (nodeData.distances[i][j] == 0) {
                matrixZeroes.emplace_back(std::pair<EdgeCities, Penalties>(EdgeCities(i, j), Penalties()));
            }
        }
        nodeData.lowerBound += columnMinimum;
    }


    for (auto &matrixZero : matrixZeroes) {
        rowMinimum = std::numeric_limits<int>::max();
        columnMinimum = std::numeric_limits<int>::max();
        for (int j = 0; j != nodeData.distances.size(); ++j) {
            if (nodeData.distances[matrixZero.first.i][j] < rowMinimum && j != matrixZero.first.j) {
                rowMinimum = nodeData.distances[matrixZero.first.i][j];
            }
        }
        for (int i = 0; i != nodeData.distances.size(); ++i) {
            if (nodeData.distances[i][matrixZero.first.j] < columnMinimum && i != matrixZero.first.i) {
                columnMinimum = nodeData.distances[i][matrixZero.first.j];
            }
        }
        matrixZero.second.row = rowMinimum;
        matrixZero.second.column = columnMinimum;
    }

    auto highestPenaltyZeroData = std::max_element(matrixZeroes.begin(), matrixZeroes.end(),
                                                   [](const std::pair<EdgeCities, Penalties> &lhs,
                                                      const std::pair<EdgeCities, Penalties> &rhs) -> bool {
                                                       return lhs.second.row + lhs.second.column <
                                                              rhs.second.row + rhs.second.column;
                                                   });
    nodeData.highestZeroPenaltiesIndexes = highestPenaltyZeroData->first;
    nodeData.highestZeroPenalties = highestPenaltyZeroData->second;
}

void TSPAlgorithms::bbCreateLeftNodeData(BBNodeData &nodeData) {

}

void TSPAlgorithms::bbCreateRightNodeData(BBNodeData &nodeData) {

}

int TSPAlgorithms::bbGetCycleCost(BBNodeData &nodeData) {
    return 0;
}

