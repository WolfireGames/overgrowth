//-----------------------------------------------------------------------------
//           Name: soundgroupinfo.h
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

#include <vector>

class SoundGroupInfo : public AssetInfo {
    int num_variants;
    float delay;
    float volume;
    float max_distance;
    
public:
    SoundGroupInfo( AssetManager* owner, uint32_t asset_id );

    int Load( const std::string &path, uint32_t load_flags );
    const char* GetLoadErrorString();
    const char* GetLoadErrorStringExtended() { return ""; }
    void Unload();
    void Reload( );
    void ReportLoad() override;

    void ReturnPaths(PathSet &path_set) override;
    inline int GetNumVariants() const {return num_variants;}
    inline float GetDelay() const {return delay;}
    inline float GetVolume() const {return volume;}
    inline float GetMaxDistance() const {return max_distance;}
    std::string GetSoundPath( int choice ) const;
    static AssetType GetType() { return SOUND_GROUP_INFO_ASSET; }
    static const char* GetTypeName() { return "SOUND_GROUP_INFO_ASSET"; }
    static bool AssetWarning() { return true; }

    AssetLoaderBase* NewLoader() override;
private:
    bool ParseXML( const char* data );
};

typedef AssetRef<SoundGroupInfo> SoundGroupInfoRef;
