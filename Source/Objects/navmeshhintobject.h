//-----------------------------------------------------------------------------
//           Name: navmeshhintobject.h
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
#include <Graphics/vbocontainer.h>

#include <Objects/object.h>

#include <string>
#include <vector>

class NavmeshHintObject: public Object {
private:
    RC_VBOContainer unit_box_vbo;
    std::vector<vec3> cross_marking;
public:
    NavmeshHintObject();
    ~NavmeshHintObject() override;
    EntityType GetType() const override { return _navmesh_hint_object; }
    bool Initialize() override;
    void Draw() override;
    void GetDesc(EntityDescription &desc) const override;
    bool SetFromDesc( const EntityDescription& desc ) override;
};

