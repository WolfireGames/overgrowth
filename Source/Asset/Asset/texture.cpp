//-----------------------------------------------------------------------------
//           Name: texture.cpp
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
#include "texture.h"

#include <Asset/AssetLoader/fallbackassetloader.h>

#include <Graphics/textureref.h>
#include <Graphics/textures.h>

TextureRef TextureAsset::GetTextureRef() {
    return TextureRef(id);
}

TextureAsset::TextureAsset(AssetManager* owner, uint32_t asset_id) : Asset(owner, asset_id) {
    id = PhoenixTextures::INVALID_ID;
}

TextureAsset::~TextureAsset() {
    Textures::Instance()->DecrementRefCount(id);
}

int TextureAsset::Load(const std::string& path, uint32_t load_flags) {
    id = Textures::Instance()->AllocateFreeSlot();
    int load_texture_value = Textures::Instance()->loadTexture(path, id, load_flags);

    if (load_texture_value != kLoadOk) {
        Textures::Instance()->DecrementRefCount(id);
        id = PhoenixTextures::INVALID_ID;
    }
    return load_texture_value;
}

const char* TextureAsset::GetLoadErrorString() {
    return "";
}

void TextureAsset::Unload() {
}

AssetLoaderBase* TextureAsset::NewLoader() {
    return new FallbackAssetLoader<TextureAsset>();
}

unsigned int TextureAsset::GetTexID() {
    return id;
}
