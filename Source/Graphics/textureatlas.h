//-----------------------------------------------------------------------------
//           Name: textureatlas.h
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

#include <Graphics/textures.h>
#include <Graphics/textureref.h>
#include <Graphics/atlasnodetree.h>
#include <Graphics/vbocontainer.h>

#include <Images/texture_data.h>

#include <queue>

class TextureAtlas;

struct TextureAtlasRef {
   public:
    ivec2 pos;
    ivec2 dim;
    vec2 uv_start;
    vec2 uv_size;
    TextureAtlas* source;
};

class TextureAtlas {
   private:
    std::queue<std::pair<std::string, AtlasNodeTree::AtlasNode*> > dirtyNodes;

    void DrawToAtlasNode(const std::string& rel_path, const AtlasNodeTree::AtlasNode& node);

   public:
    GLuint framebuffer;
    TextureRef atlas_texture;
    ivec2 dim;
    TextureAtlas(AtlasNodeTree& atlasNodeTree, TextureData::ColorSpace colorSpace);

    // AtlasTextureAssetRef

    TextureAtlasRef SetTextureToNode(const std::string& rel_path, AtlasNodeTree::AtlasNodeRef node);
    void Draw();
};
