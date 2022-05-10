//-----------------------------------------------------------------------------
//           Name: image_sampler.cpp
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
#include "image_sampler.h"

#include <Asset/AssetLoader/fallbackassetloader.h>
#include <Asset/assetloaderrors.h>

#include <Internal/cachefile.h>
#include <Internal/common.h>
#include <Internal/filesystem.h>
#include <Internal/error.h>

#include <Compat/fileio.h>

#include <cerrno>
#include <sstream>
#include <cstring>
#include <algorithm>

using std::max;
using std::min;

namespace {
const char* suffix = "_image_sample_cache";
}

ImageSampler::ImageSampler(AssetManager* owner, uint32_t asset_id) : Asset(owner, asset_id) {
}

vec4 ImageSampler::GetInterpolatedColorUV(float x, float y) const {
    x = max(0.01f, min(0.99f, x));
    y = max(0.01f, min(0.99f, y));
    return GetInterpolatedColor(x * width_ + 0.5f, y * height_ + 1.0f);
}

vec4 ImageSampler::GetInterpolatedColor(float x, float y) const {
    byte4 top_left = GetPixel((int)x, (int)y);
    byte4 top_right = GetPixel((int)(x + 1), (int)y);
    byte4 bottom_left = GetPixel((int)x, (int)(y + 1));
    byte4 bottom_right = GetPixel((int)(x + 1), (int)(y + 1));
    ;

    float x_weight = x - (int)x;
    float y_weight = y - (int)y;

    vec3 left;
    left[0] = (uint8_t)(top_left[0] * (1 - y_weight) + bottom_left[0] * (y_weight));
    left[1] = (uint8_t)(top_left[1] * (1 - y_weight) + bottom_left[1] * (y_weight));
    left[2] = (uint8_t)(top_left[2] * (1 - y_weight) + bottom_left[2] * (y_weight));
    vec3 right;
    right[0] = (uint8_t)(top_right[0] * (1 - y_weight) + bottom_right[0] * (y_weight));
    right[1] = (uint8_t)(top_right[1] * (1 - y_weight) + bottom_right[1] * (y_weight));
    right[2] = (uint8_t)(top_right[2] * (1 - y_weight) + bottom_right[2] * (y_weight));

    vec3 value(0.0f);
    value.r() = ((uint8_t)(left[0] * (1 - x_weight) + right[0] * (x_weight))) / 255.0f;
    value.g() = ((uint8_t)(left[1] * (1 - x_weight) + right[1] * (x_weight))) / 255.0f;
    value.b() = ((uint8_t)(left[2] * (1 - x_weight) + right[2] * (x_weight))) / 255.0f;

    return value;
}

ImageSampler::byte4 ImageSampler::GetPixel(int x, int y) const {
    x = max(0, min(width_ - 1, x));
    y = max(0, min(height_ - 1, y));
    return pixels_[x * height_ + y];
}

int ImageSampler::Load(const std::string& path, uint32_t load_flags) {
    std::string load_path;
    ModID cache_modsource;
    bool loaded_cache = false;

    if (CacheFile::CheckForCache(path_, suffix, &load_path, &cache_modsource, &checksum_)) {
        if (ReadCacheFile(load_path, checksum_)) {
            loaded_cache = true;
            LOGI << "Loaded cache file " << load_path << std::endl;
            modsource_ = cache_modsource;
        }
    }

    if (loaded_cache == false) {
        char abs_path[kPathSize];
        ModID modsource;
        if (FindImagePath(path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths | kWriteDir | kModWriteDirs, true, NULL, false, false, &modsource) == -1) {
            return kLoadErrorMissingFile;
        }
        LOGI << "Loading " << abs_path << std::endl;
        /*
        if(FindFilePath(path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths | kWriteDir) == -1){
            FatalError("Error", "Could not find image sampler file \"%s\"", path_.c_str());
        }
        */

        int n = 0;
        // Tell stb_image that we want 4 components (RGBA)
        stbi_uc* data = stbi_load(abs_path, &width_, &width_, &n, 4);

        if (data == nullptr || n != 4) {
            return kLoadErrorGeneralFileError;
        }

        pixels_.resize(width_ * height_);
        std::memcpy(pixels_.data(), data, width_ * height_ * 4);

        stbi_image_free(data);

        char write_path[kPathSize];
        FormatString(write_path, kPathSize, "%s%s%s", GetWritePath(modsource).c_str(), path.c_str(), suffix);
        modsource_ = modsource;
        WriteCacheFile(write_path);
    }
    return kLoadOk;
}

const char* ImageSampler::GetLoadErrorString() {
    switch (sub_error) {
        case 0:
            return "";
        default:
            return "Unknown error";
    }
}

void ImageSampler::Unload() {
}

void ImageSampler::Reload() {
}

void ImageSampler::ReportLoad() {
}

bool ImageSampler::GetCachePath(std::string* dst) {
    if (!CacheFile::CheckForCache(path_, suffix, dst, &checksum_)) {
        DisplayError("Error", ("Could not find cache file for " + path_).c_str());
        return false;
    }
    return true;
}

bool ImageSampler::ReadCacheFile(const std::string& path, uint16_t checksum) {
    FILE* file = my_fopen(path.c_str(), "rb");
    if (!file) {
        return false;
    }
    CacheFile::ScopedFileCloser file_closer(file);
    unsigned char version;
    fread(&version, sizeof(version), 1, file);
    if (version != kImageSamplerCacheVersion) {
        return false;
    }
    uint16_t file_checksum;
    fread(&file_checksum, sizeof(file_checksum), 1, file);
    if (checksum != 0 && checksum != file_checksum) {
        return false;
    }

    fread(&width_, sizeof(width_), 1, file);
    fread(&height_, sizeof(height_), 1, file);
    if (width_ <= 0 || height_ <= 0) {
        return false;
    }

    pixels_.resize(width_ * height_);
    fread(&pixels_[0], sizeof(pixels_[0]) * width_ * height_, 1, file);
    return true;
}

void ImageSampler::WriteCacheFile(const std::string& path) {
    FILE* file = my_fopen(path.c_str(), "wb");
    if (!file) {
        FatalError("Error", "Could not write file \"%s\" because of error \"%s\"", path.c_str(), strerror(errno));
    }
    CacheFile::ScopedFileCloser file_closer(file);
    unsigned char version = kImageSamplerCacheVersion;
    fwrite(&version, sizeof(version), 1, file);
    fwrite(&checksum_, sizeof(checksum_), 1, file);
    fwrite(&width_, sizeof(width_), 1, file);
    fwrite(&height_, sizeof(height_), 1, file);
    fwrite(&pixels_[0], sizeof(pixels_[0]), width_ * height_, file);
}

AssetLoaderBase* ImageSampler::NewLoader() {
    return new FallbackAssetLoader<ImageSampler>();
}
