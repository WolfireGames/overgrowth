//-----------------------------------------------------------------------------
//           Name: navmeshregionobject.h
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

#include <Graphics/textureref.h>
#include <Objects/object.h>
#include <Math/vec3.h>

#include <string>
#include <vector>

class NavMesh;

class NavmeshRegionObject : public Object {
   public:
    NavmeshRegionObject();
    ~NavmeshRegionObject() override;

    EntityType GetType() const override { return _navmesh_region_object; }
    bool Initialize() override;
    void Draw() override;
    void GetDesc(EntityDescription& desc) const override;
    bool SetFromDesc(const EntityDescription& desc) override;

    vec3 GetMinBounds();
    vec3 GetMaxBounds();
};
