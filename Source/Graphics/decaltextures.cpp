//-----------------------------------------------------------------------------
//           Name: decaltextures.cpp
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
#include "decaltextures.h"

#include <Internal/config.h>
#include <Internal/hardware_specs.h>
#include <Internal/profiler.h>

#include <Main/engine.h>
#include <Math/ivec2math.h>
#include <Logging/logdata.h>

#include <cmath>

bool kUseDecalNormals = false;

// We define this because windows is missing log2
double Log2(double n) {
    // log(n)/log(2) is log2.
    return log(n) / log(2);
}

DecalTexture::DecalTexture() {
}

DecalTexture::~DecalTexture() {
    DecalTextures* dt = DecalTextures::Instance();

    if (dt->initialized) {
        if (colornode_ref.valid())
            dt->colornodetree->FreeNode(colornode_ref);
        if (normalnode_ref.valid())
            dt->normalnodetree->FreeNode(normalnode_ref);
    }
}

DecalTextures::DecalTextures() : colornodetree(NULL),
                                 normalnodetree(NULL),
                                 coloratlas(NULL),
                                 normalatlas(NULL),
                                 initialized(false) {
}

DecalTextures::~DecalTextures() {
    delete colornodetree;
    colornodetree = NULL;
    delete normalnodetree;
    normalnodetree = NULL;
    delete coloratlas;
    coloratlas = NULL;
    delete normalatlas;
    normalatlas = NULL;
    initialized = false;
}

void DecalTextures::Init() {
    kUseDecalNormals = config["decal_normals"].toNumber<bool>();
    int s = GetGLMaxTextureSize();
    s = (int)std::pow(2.0, (int)std::min((int)Log2(s), 13));

    s = s / Graphics::Instance()->config_.texture_reduction_factor();

    ivec2 dim(s, s);
    PROFILER_ENTER(g_profiler_ctx, "Create AtlasNodeTrees");
    colornodetree = new AtlasNodeTree(dim, 32);
    normalnodetree = new AtlasNodeTree(dim, 32);
    PROFILER_LEAVE(g_profiler_ctx);
    PROFILER_ENTER(g_profiler_ctx, "Create Texture Atlases");
    coloratlas = new TextureAtlas(*colornodetree, TextureData::sRGB);
    Textures::Instance()->SetTextureName(coloratlas->atlas_texture, "Decal Color Atlas");
    normalatlas = new TextureAtlas(*normalnodetree, TextureData::Linear);
    Textures::Instance()->SetTextureName(normalatlas->atlas_texture, "Decal Normal Atlas");
    PROFILER_LEAVE(g_profiler_ctx);
    initialized = true;
}

void DecalTextures::Draw() {
    coloratlas->Draw();
    if (kUseDecalNormals) {
        normalatlas->Draw();
    }
}

static DecalTextures* instance = NULL;

DecalTextures* DecalTextures::Instance() {
    if (instance == NULL) {
        instance = new DecalTextures();
    }

    return instance;
}

void DecalTextures::Dispose() {
    delete instance;
    instance = NULL;
}

void DecalTextures::Clear() {
    colornodetree->Clear();
    normalnodetree->Clear();
}

RC_DecalTexture DecalTextures::allocateTexture(std::string color, std::string norm) {
    // This will, thankfully, not go straight into VRAM.
    RC_DecalTexture rc_dt;

    TextureAssetRef color_tex = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(color);
    TextureAssetRef normal_tex = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(norm);

    // we put these in a compressed atlas so they must be compressed
    if (!Textures::Instance()->IsCompressed(color_tex)) {
        LOGW << "Reloading a texture as compressed, this is unexpected" << std::endl;
        if (!Textures::Instance()->ReloadAsCompressed(color_tex)) {
            LOGE << "Failed to re-save color texture " << color << " as compressed" << std::endl;
        }
    }
    if (!Textures::Instance()->IsCompressed(normal_tex)) {
        LOGW << "Reloading a texture as compressed, this is unexpected" << std::endl;
        if (!Textures::Instance()->ReloadAsCompressed(normal_tex)) {
            LOGE << "Failed to re-save normal texture " << norm << " as compressed" << std::endl;
        }
    }

    int reduction_factor = Graphics::Instance()->config_.texture_reduction_factor();
    ivec2 color_tex_dim = ivec2(Textures::Instance()->getWidth(color_tex), Textures::Instance()->getWidth(color_tex)) / reduction_factor;
    ivec2 normal_tex_dim = ivec2(Textures::Instance()->getHeight(normal_tex), Textures::Instance()->getHeight(normal_tex)) / reduction_factor;

    {
        rc_dt->colornode_ref = colornodetree->RetrieveNode(color_tex_dim);

        if (rc_dt->colornode_ref.valid()) {
            rc_dt->color_texture_ref = coloratlas->SetTextureToNode(color, rc_dt->colornode_ref);
        } else {
            LOGE << "Unable to allocate atlas space for decal color." << std::endl;
        }
    }

    {
        rc_dt->normalnode_ref = normalnodetree->RetrieveNode(normal_tex_dim);

        if (rc_dt->normalnode_ref.valid()) {
            rc_dt->normal_texture_ref = normalatlas->SetTextureToNode(norm, rc_dt->normalnode_ref);
        } else {
            LOGE << "Unable to allocate atlas space for decal normals." << std::endl;
        }
    }

    return rc_dt;
}
