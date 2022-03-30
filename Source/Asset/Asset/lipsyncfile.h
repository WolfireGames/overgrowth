//-----------------------------------------------------------------------------
//           Name: lipsyncfile.h
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

#include <Math/quaternions.h>

#include <vector>
#include <string>
#include <map>


struct KeyWeight {
    int id;
    float weight;
};

struct LipSyncKey {
    std::vector<KeyWeight> keys;
    float time;
};

class LipSyncFile : public Asset {
public:
    LipSyncFile(AssetManager* owner, uint32_t asset_id);    
    static AssetType GetType() { return LIP_SYNC_FILE_ASSET; }
    static const char* GetTypeName() { return "LIP_SYNC_FILE_ASSET"; }
    static bool AssetWarning() { return true; }

    std::vector<LipSyncKey> keys;
    float time_bound[2];

    int sub_error;
    int Load(const std::string &path, uint32_t load_flags);
    const char* GetLoadErrorString();
    const char* GetLoadErrorStringExtended() { return ""; }
    void Unload();
    void Reload();
    void GetWeights(float time, int &marker, std::vector<KeyWeight> &weights );

    virtual AssetLoaderBase* NewLoader();
};

typedef AssetRef<LipSyncFile> LipSyncFileRef;

class ASContext;

class LipSyncFileReader {
    float time;
    LipSyncFileRef ls_ref;
    int marker;
    std::vector<KeyWeight> vis_weights;
    std::map<int, std::string> id2morph;
    std::map<std::string, float> morph_weights;
public:
    void Update(float timestep);
    bool valid();
    void AttachTo(LipSyncFileRef& _ls_ref);
    void UpdateWeights();
    void SetVisemeMorphs( const std::map<std::string, std::string> &vis2morph );
    std::map<std::string, float>& GetMorphWeights();
};
