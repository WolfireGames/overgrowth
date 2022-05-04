//-----------------------------------------------------------------------------
//           Name: levelset.h
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

class LevelSet : public AssetInfo {
public:
    typedef std::list<std::string> LevelPaths;
private:
    LevelPaths level_paths_;
public:
    LevelSet(AssetManager* owner, uint32_t asset_id);
    static AssetType GetType() { return LEVEL_SET_ASSET; }
    static const char* GetTypeName() { return "LEVEL_SET_ASSET"; }
    static bool AssetWarning() { return true; }

    LevelPaths& level_paths(){return level_paths_;}
    
    void ReturnPaths(PathSet &path_set) override;

    int sub_error;
    int Load(const std::string &path, uint32_t load_flags);
    const char* GetLoadErrorString();
    const char* GetLoadErrorStringExtended() { return ""; }
    void Unload();
    void Reload();
    void ReportLoad() override;

    void clear();

    AssetLoaderBase* NewLoader() override;
};

typedef AssetRef<LevelSet> LevelSetRef;

