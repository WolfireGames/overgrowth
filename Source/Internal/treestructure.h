//-----------------------------------------------------------------------------
//           Name: treestructure.h
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
#pragma once

#include <vector>

namespace TreeStructure {
    void GetBalancedTree(std::vector<int> &temp_parent);
    int GetTotalNumChildren(int parent, const std::vector<int> &parents);
    int GetDepth(int which, const std::vector<int> &parents);
    int GetNumChildren(int parent, const std::vector<int> &parents);
    std::vector<int> GetChildren(int parent, const std::vector<int> &parents);
    int GetNumConnections(int parent, const std::vector<int> &parents);
    std::vector<int> FindTreePath( int a, int b, const std::vector<int> &parents );
    std::vector<int> GetChildrenRecursive( int parent, const std::vector<int> &parents );
}
