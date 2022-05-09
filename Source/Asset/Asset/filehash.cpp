//-----------------------------------------------------------------------------
//           Name: filehash.cpp
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

#include "filehash.h"

#include <Asset/AssetLoader/fallbackassetloader.h>

FileHashAsset::FileHashAsset(AssetManager* owner, uint32_t asset_id) : Asset(owner, asset_id), sub_error(0) {
}

int FileHashAsset::Load(const std::string& path, uint32_t load_flags) {
    Path file = FindFilePath(path, kAnyPath);

    if (file.isValid()) {
        hash = GetFileHash(file.GetAbsPathStr().c_str());
        return kLoadOk;
    } else {
        return kLoadErrorMissingFile;
    }
}

const char* FileHashAsset::GetLoadErrorString() {
    return "no error known";
}

void FileHashAsset::Unload() {
}

void FileHashAsset::Reload() {
}

AssetLoaderBase* FileHashAsset::NewLoader() {
    return new FallbackAssetLoader<FileHashAsset>();
}
