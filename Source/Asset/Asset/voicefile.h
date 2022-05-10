//-----------------------------------------------------------------------------
//           Name: voicefile.h
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

#include <map>
#include <string>

class AssetManager;

class VoiceFile : public AssetInfo {
    std::map<std::string, std::string> voice_paths;
    std::string null_string;

   public:
    VoiceFile(AssetManager* owner, uint32_t asset_id);
    const std::string& GetVoicePath(const std::string voice);
    int Load(const std::string& path, uint32_t load_flags);
    const char* GetLoadErrorString();
    const char* GetLoadErrorStringExtended() { return ""; }
    void Unload();
    void Reload();
    void ReportLoad() override;
    void ReturnPaths(PathSet& path_set) override;
    static AssetType GetType() { return VOICE_FILE_ASSET; }
    static const char* GetTypeName() { return "VOICE_FILE_ASSET"; }
    static bool AssetWarning() { return true; }

    AssetLoaderBase* NewLoader() override;
};

typedef AssetRef<VoiceFile> VoiceFileRef;
