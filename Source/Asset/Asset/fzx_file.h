//-----------------------------------------------------------------------------
//           Name: fzx_file.h
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

#include <Asset/assetbase.h>
#include <Asset/assetinfobase.h>

#include <Math/vec3.h>
#include <Math/quaternions.h>

#include <string>
#include <vector>

struct FZXObject {
    std::string label;
    vec3 location;
    vec3 scale;
    quaternion rotation;
};

class FZXAsset : public Asset {
   public:
    FZXAsset(AssetManager* owner, uint32_t asset_id);
    static AssetType GetType() { return FZX_ASSET; }
    static const char* GetTypeName() { return "FZX_ASSET"; }
    static bool AssetWarning() { return true; }

    std::vector<FZXObject> objects;

    int sub_error;
    int Load(const std::string& path, uint32_t load_flags);
    const char* GetLoadErrorString();
    const char* GetLoadErrorStringExtended() { return ""; }
    void Unload();
    void Reload();

    AssetLoaderBase* NewLoader() override;
};

typedef AssetRef<FZXAsset> FZXAssetRef;
