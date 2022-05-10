//-----------------------------------------------------------------------------
//           Name: levelinfo.cpp
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
#include "levelinfo.h"

#include <Asset/AssetLoader/fallbackassetloader.h>
#include <Asset/Asset/filehash.h>

#include <Main/engine.h>
#include <Logging/logdata.h>

LevelInfoAsset::LevelInfoAsset(AssetManager* owner, uint32_t asset_id) : Asset(owner, asset_id) {
}

int LevelInfoAsset::Load(const std::string& _path, uint32_t load_flags) {
    path = _path;
    Path p = FindFilePath(path);
    if (p.isValid()) {
        std::string level_info_cache_path_string = GenerateParallelPath("Data/Levels", "Data/LevelInfo", "_linfo.xml", p);

        Path level_info_cache_path = FindFilePath(level_info_cache_path_string);

        if (level_info_cache_path.isValid()) {
            FileHashAssetRef level_hash = Engine::Instance()->GetAssetManager()->LoadSync<FileHashAsset>(path);
            if (levelparser.Load(level_info_cache_path.GetAbsPathStr())) {
                if (levelparser.hash == level_hash->hash.ToString()) {
                    return kLoadOk;
                }
            }
        }

        if (levelparser.Load(p.GetAbsPathStr())) {
            std::string savepath = AssemblePath(GetWritePath(p.GetModsource()), level_info_cache_path_string);
            LOGI << "Saving a cached version of LevelInfoAsset to " << savepath << std::endl;
            levelparser.Save(savepath);

            return kLoadOk;
        } else {
            return kLoadErrorGeneralFileError;
        }
    } else {
        return kLoadErrorMissingFile;
    }
}

void LevelInfoAsset::ReportLoad() {
}

AssetLoaderBase* LevelInfoAsset::NewLoader() {
    return new FallbackAssetLoader<LevelInfoAsset>();
}

const std::string LevelInfoAsset::GetLevelName() {
    if (levelparser.name.empty() == true) {
        return SplitPathFileName(path).second;
    } else {
        return levelparser.name;
    }
}

const std::string& LevelInfoAsset::GetLoadingScreenImage() {
    return levelparser.loading_screen.image;
}

void LevelInfoAsset::Unload() {
    levelparser.Clear();
}
