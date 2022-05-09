//-----------------------------------------------------------------------------
//           Name: paths.h
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
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

#include <Math/vec3.h>

#include <map>
#include <vector>
#include <string>

struct PathConnection {
    int point_ids[2];
    PathConnection(int a, int b) {
        point_ids[0] = a;
        point_ids[1] = b;
    }
};

class PathPointObject;

class AIPaths {
   private:
    typedef std::map<int, vec3> PointMap;
    PointMap points;
    std::vector<PathConnection> connections;

   public:
    void Draw();

    int GetNearestPoint(const vec3 &pos);
    vec3 GetPointPosition(int point_id);
    int GetConnectedPoint(int point_id);
    int GetOtherConnectedPoint(int point_id, int other_id);
    static AIPaths *Instance() {
        static AIPaths paths;
        return &paths;
    }
    void AddPoint(int id, const vec3 &pos);
    void SetPoint(int id, const vec3 &pos);
    void AddConnection(int a, int b);
    void RemovePoint(int id);
    void RemoveConnection(int a, int b);
};

class ASContext;
class PathScriptReader {
   public:
    int GetNearestPoint(vec3 pos);
    vec3 GetPointPosition(int point_id);
    int GetConnectedPoint(int point_id);
    int GetOtherConnectedPoint(int point_id, int other_id);
    void AttachToScript(ASContext *as_context, const std::string &as_name);
};
