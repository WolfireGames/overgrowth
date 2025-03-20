//-----------------------------------------------------------------------------
//           Name: decalobject.h
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

#include <Game/EntityDescription.h>
#include <Game/color_tint_component.h>

#include <Internal/hardware_specs.h>
#include <Graphics/decaltextures.h>

#include <Asset/Asset/decalfile.h>
#include <Objects/object.h>

#include <cmath>

class EnvObject;
class DecalEditor;

class DecalObject : public Object {
   public:

    EntityType GetType() const override;
    DecalObject();
    bool Initialize() override;
    void Dispose() override;
    void GetDisplayName(char* buf, int buf_size) override;
    void Draw() override;
    bool SetFromDesc(const EntityDescription& desc) override;
    void GetDesc(EntityDescription& desc) const override;
    void Load(const std::string& type_file);
    void ReceiveObjectMessageVAList(OBJECT_MSG::Type type, va_list args) override;
    void PreDrawFrame(float curr_game_time) override;
    ColorTintComponent color_tint_component_;
    float spawn_time_;

    RC_DecalTexture texture;
    DecalFileRef decal_file_ref;

   protected:
    static std::map<std::string, RC_DecalTexture> textureCache;
};
