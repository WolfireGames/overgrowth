//-----------------------------------------------------------------------------
//           Name: texturepack.h
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

#include <Math/vec4.h>
#include <Math/vec2.h>

#include <Graphics/glstate.h>

#include <Asset/Asset/texture.h>

#include <vector>

class TexturePackNode {
private:
    bool filled;
    vec2 rect_min;
    vec2 rect_max;
    TexturePackNode* children[2];

public:
    TexturePackNode* Insert(const TextureAssetRef &texture);
    void SetRect(const vec2 _rect_min , const vec2 _rect_max);
    const vec2& GetMin() const;
    const vec2& GetMax() const;

    TexturePackNode();
    void Dispose();
    ~TexturePackNode();
};

struct TexturePackParam {
    TextureAssetRef texture;
    vec4 offset; //(vec2 corner, vec2 dimensions)
    bool inserted;
};

class TexturePack {
    int size;
    TexturePackNode root_node;

public:
    TextureRef pack_texture;
    
    void Create(int size, std::vector<TexturePackParam> &textures);
    TexturePack();
    void Dispose();
    ~TexturePack();
};
