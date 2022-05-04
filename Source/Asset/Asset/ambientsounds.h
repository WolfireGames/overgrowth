//-----------------------------------------------------------------------------
//           Name: ambientsounds.h
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

#include <string>

using std::string;
 
enum AmbientSoundType {
    _continuous = 0,
    _occasional = 1
};

class AmbientSound : public AssetInfo {
    string sound_path;
    AmbientSoundType type;
    float delay_min;
    float delay_max;

public:
    AmbientSound( AssetManager* owner, uint32_t asset_id );
    static AssetType GetType() { return AMBIENT_SOUND_ASSET; }
    static const char* GetTypeName() { return "AMBIENT_SOUND_ASSET"; }

    float GetDelay();
    const string &GetPath();
    AmbientSoundType GetSoundType();

    int sub_error;
    int Load(const string &path, uint32_t load_flags);
    const char* GetLoadErrorString();
    const char* GetLoadErrorStringExtended() { return ""; }
    void Unload();
    
    void Reload();
    void ReportLoad() override;

    float GetDelayNoLower();
    void ReturnPaths(PathSet &path_set) override;

    AssetLoaderBase* NewLoader() override;
    static bool AssetWarning() { return true; }
};

typedef AssetRef<AmbientSound> AmbientSoundRef;
