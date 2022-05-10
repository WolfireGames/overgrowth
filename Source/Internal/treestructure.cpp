//-----------------------------------------------------------------------------
//           Name: treestructure.cpp
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
//   Copyright 2022 Wolfire Games LLC
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//-----------------------------------------------------------------------------

#include <queue>
#include <vector>
#include <algorithm>

int GetDepth(int which, const std::vector<int> &parents) {
    int depth = 0;
    int temp = which;
    while (parents[temp] != -1) {
        temp = parents[temp];
        ++depth;
    }
    return depth;
}

int GetTotalNumChildren(int parent, const std::vector<int> &parents) {
    int num_children = 0;
    for (unsigned i = 0; i < parents.size(); ++i) {
        if (parents[i] == parent) {
            ++num_children;
            num_children += GetTotalNumChildren(i, parents);
        }
    }
    return num_children;
}

int GetNumChildren(int parent, const std::vector<int> &parents) {
    int num_children = 0;
    for (int i : parents) {
        if (i == parent) {
            ++num_children;
        }
    }
    return num_children;
}

int GetNumConnections(int parent, const std::vector<int> &parents) {
    int num_connections = 0;
    if (parents[parent] != -1) {
        ++num_connections;
    }
    for (int i : parents) {
        if (i == parent) {
            ++num_connections;
        }
    }
    return num_connections;
}

void GetBalancedTree(std::vector<int> &temp_parent) {
    // Get root node id
    int root = 0;
    for (unsigned i = 0; i < temp_parent.size(); ++i) {
        if (temp_parent[i] == -1) {
            root = i;
        }
    }

    // int old_imbalance = -1;
    // int imbalance = -1;

    while (true) {
        // Get a list of all the children of the root node
        std::vector<int> root_children;
        for (unsigned i = 0; i < temp_parent.size(); ++i) {
            if (temp_parent[i] == root) {
                root_children.push_back(i);
            }
        }

        // Get weight of each child branch
        std::vector<int> root_weight(root_children.size());
        for (unsigned i = 0; i < root_children.size(); ++i) {
            root_weight[i] = GetTotalNumChildren(root_children[i], temp_parent);
        }

        // Find heaviest and lightest child branch
        int heaviest_child = 0;
        int lightest_child = 0;
        for (unsigned i = 1; i < root_children.size(); ++i) {
            if (root_weight[i] > root_weight[heaviest_child]) {
                heaviest_child = i;
            }
            if (root_weight[i] < root_weight[lightest_child]) {
                lightest_child = i;
            }
        }

        // Make heaviest branch the new root
        // old_imbalance = imbalance;
        /*
        imbalance = root_weight[heaviest_child] -
                    root_weight[lightest_child];
        */

        if (root_weight[heaviest_child] + 1 <=
            (int)temp_parent.size() - root_weight[heaviest_child]) {
            return;
        }

        temp_parent[root_children[heaviest_child]] = -1;
        temp_parent[root] = root_children[heaviest_child];
        root = root_children[heaviest_child];
    }
}

std::vector<int> GetChildren(int parent, const std::vector<int> &parents) {
    std::vector<int> children;
    for (unsigned i = 0; i < parents.size(); ++i) {
        if (parents[i] == parent) {
            children.push_back(i);
        }
    }
    return children;
}

std::vector<int> GetChildrenRecursive(int parent, const std::vector<int> &parents) {
    std::vector<int> children;
    for (unsigned i = 0; i < parents.size(); ++i) {
        if (parents[i] == parent) {
            children.push_back(i);
            std::vector<int> sub_children = GetChildrenRecursive(i, parents);
            for (int &j : sub_children) {
                children.push_back(j);
            }
        }
    }
    return children;
}

std::vector<int> GetConnections(int parent, const std::vector<int> &parents) {
    std::vector<int> connections;
    if (parents[parent] != -1) {
        connections.push_back(parents[parent]);
    }
    for (unsigned i = 0; i < parents.size(); ++i) {
        if (parents[i] == parent) {
            connections.push_back(i);
        }
    }
    return connections;
}

std::vector<int> FindTreePath(int a, int b, const std::vector<int> &parents) {
    std::queue<std::vector<int> > queue;

    std::vector<int> start_path;
    start_path.push_back(a);
    queue.push(start_path);

    while (!queue.empty()) {
        std::vector<int> path = queue.front();
        queue.pop();

        std::vector<int> connections = GetConnections(path.back(), parents);
        for (int &connection : connections) {
            if (find(path.begin(), path.end(), connection) == path.end()) {
                queue.push(path);
                queue.back().push_back(connection);
                if (connection == b) {
                    return queue.back();
                }
            }
        }
    }

    std::vector<int> failure;
    return failure;
}
