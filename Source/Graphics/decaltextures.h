//-----------------------------------------------------------------------------
//           Name: decaltextures.h
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

#include <Graphics/textureatlas.h>
#include <Graphics/atlasnodetree.h>

#include <Internal/referencecounter.h>

class EnvObject;
/*
 Combined instance creating a reference to a texture
*/
class DecalTexture
{
    AtlasNodeTree::AtlasNodeRef colornode_ref;
    AtlasNodeTree::AtlasNodeRef normalnode_ref;
    friend class DecalTextures;
public:
    TextureAtlasRef color_texture_ref;
    TextureAtlasRef normal_texture_ref;

    DecalTexture();
    ~DecalTexture();
};

typedef ReferenceCounter<DecalTexture> RC_DecalTexture;

class DecalTextures
{
private:
    AtlasNodeTree* colornodetree;
    AtlasNodeTree* normalnodetree;
    TextureAtlas* coloratlas;
    TextureAtlas* normalatlas;
    bool initialized;
    friend class DecalTexture;
    friend class EnvObject;
	friend class SceneGraph;

    DecalTextures();
    ~DecalTextures();
public:
    
    void Init();
    void Draw();
    void Clear();

    RC_DecalTexture allocateTexture( std::string color_tex, std::string norm_tex );

    static DecalTextures* Instance();
    static void Dispose();
};
