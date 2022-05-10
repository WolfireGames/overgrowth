//-----------------------------------------------------------------------------
//           Name: averagecolorasset.cpp
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
#include "averagecolorasset.h"

#include <Internal/integer.h>
#include <Internal/cachefile.h>
#include <Internal/error.h>

#include <Asset/AssetLoader/fallbackassetloader.h>
#include <Compat/fileio.h>
#include <Graphics/bytecolor.h>

#include <string>
#include <cstdio>
#include <cstdlib>

using std::string;

AverageColor::AverageColor(AssetManager* owner, uint32_t asset_id) : Asset(owner, asset_id) {
}

int AverageColor::Load(const string& rel_path, uint32_t load_flags) {
    string load_path;
    string suffix = "_avg_color_cache";
    uint16_t checksum;
    ModID cached_modsource;
    if (CacheFile::CheckForCache(path_, suffix, &load_path, &cached_modsource, &checksum)) {
        if (ReadCacheFile(load_path, checksum)) {
            modsource_ = cached_modsource;
            return kLoadOk;
        }
    }

    char abs_path[kPathSize];
    ModID modsource;
    if (FindImagePath(rel_path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths, true, NULL, true, true, &modsource) == -1) {
        // FatalError("Error", "Could not get average color of %s", rel_path.c_str());
        return kLoadErrorMissingFile;
    }
    modsource_ = modsource;

    vec4 byte_color = GetAverageColor(abs_path);
    color_ = vec4(byte_color[2] / 255.0f,
                  byte_color[1] / 255.0f,
                  byte_color[0] / 255.0f,
                  byte_color[3] / 255.0f);

    // TODO: this checksum is never set, calculate it!
    WriteCacheFile(GetWritePath(modsource) + rel_path + suffix, checksum);

    return kLoadOk;
}

const char* AverageColor::GetLoadErrorString() {
    return "";
}

void AverageColor::Unload() {
}

void AverageColor::Reload() {
}

void AverageColor::ReportLoad() {
}

bool AverageColor::ReadCacheFile(const string& path, uint16_t checksum) {
    FILE* file = my_fopen(path.c_str(), "rb");
    if (!file) {
        return false;
    }
    CacheFile::ScopedFileCloser file_closer(file);
    unsigned char version;
    fread(&version, sizeof(version), 1, file);
    if (version != kAverageColorCacheVersion) {
        return false;
    }
    uint16_t file_checksum;
    fread(&file_checksum, sizeof(file_checksum), 1, file);
    if (checksum != 0 && checksum != file_checksum) {
        return false;
    }
    fread(&color_, sizeof(color_), 1, file);
    return true;
}

void AverageColor::WriteCacheFile(const string& path, uint16_t checksum) {
    FILE* file = my_fopen(path.c_str(), "wb");
    CacheFile::ScopedFileCloser file_closer(file);
    unsigned char version = kAverageColorCacheVersion;
    fwrite(&version, sizeof(version), 1, file);
    fwrite(&checksum, sizeof(checksum), 1, file);
    fwrite(&color_, sizeof(color_), 1, file);
}

AssetLoaderBase* AverageColor::NewLoader() {
    return new FallbackAssetLoader<AverageColor>();
}
