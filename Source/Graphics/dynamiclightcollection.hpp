//-----------------------------------------------------------------------------
//           Name: dynamiclightcollection.hpp
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

#include <vector>

struct DynamicLight {
    int id;
    vec3 pos;
    vec3 color;
    float radius;
    // TODO: need intensity
    // NOTE: this is just the CPU data structure, see scenegraph.cpp
    // for ShaderLight which is the GPU data struct
};

// when you want to add spot lights, DO NOT change DynamicLight
// add new DynamicSpotLight since they need different data and are processed differently
// also DynamicLight should be renamed to DynamicPointLight to clarify its meaning

class BulletWorld;
class SceneGraph;

class DynamicLightCollection {
   public:
    DynamicLightCollection();
    ~DynamicLightCollection();
    int AddLight(const vec3& pos, const vec3& color, float radius);
    bool MoveLight(int id, const vec3& pos);
    bool DeleteLight(int id);
    void Draw(BulletWorld& bw);
    void Init();
    void Dispose();
    DynamicLight* GetLightFromID(int id);

   private:
    std::vector<DynamicLight> dynamic_lights;
    int next_id;

    friend class SceneGraph;
};
