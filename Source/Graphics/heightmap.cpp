//-----------------------------------------------------------------------------
//           Name: heightmap.cpp
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
#include "heightmap.h"

#include <Internal/checksum.h>
#include <Internal/common.h>
#include <Internal/error.h>
#include <Internal/datemodified.h>
#include <Internal/cachefile.h>
#include <Internal/filesystem.h>

#include <Compat/fileio.h>
#include <Images/stbimage_wrapper.h>

bool HeightmapImage::ReadCacheFile(const std::string& path, uint16_t checksum) {
    FILE* file = my_fopen(path.c_str(), "rb");
    if (!file) {
        return false;
    }
    CacheFile::ScopedFileCloser file_closer(file);
    unsigned char version;
    fread(&version, sizeof(version), 1, file);
    if (version != kHMCacheVersion) {
        return false;
    }
    uint16_t file_checksum;
    fread(&file_checksum, sizeof(file_checksum), 1, file);
    if (checksum != 0 && checksum != file_checksum) {
        return false;
    }
    checksum_ = file_checksum;
    fread(&width_, sizeof(width_), 1, file);
    fread(&depth_, sizeof(depth_), 1, file);
    height_data_.resize(width_ * depth_);
    fread(&height_data_[0], sizeof(height_data_[0]) * width_ * depth_, 1, file);
    return true;
}

void HeightmapImage::WriteCacheFile(const std::string& path) {
    FILE* file = my_fopen(path.c_str(), "wb");
    CacheFile::ScopedFileCloser file_closer(file);
    unsigned char version = kHMCacheVersion;
    fwrite(&version, sizeof(version), 1, file);
    fwrite(&checksum_, sizeof(checksum_), 1, file);
    fwrite(&width_, sizeof(width_), 1, file);
    fwrite(&depth_, sizeof(depth_), 1, file);
    fwrite(&height_data_[0], sizeof(height_data_[0]), width_ * depth_, file);
}

bool HeightmapImage::LoadData(const std::string& rel_path, HMScale scaled) {
    height_data_.clear();

    rel_path_ = rel_path;

    std::string cache_suffix = "_hmcache";
    if (scaled == DOWNSAMPLED) {
        cache_suffix += "_scaled";
    }

    std::string load_path;
    ModID load_modsource;
    if (CacheFile::CheckForCache(rel_path_, cache_suffix, &load_path, &load_modsource, &checksum_)) {
        if (ReadCacheFile(load_path, checksum_)) {
            modsource_ = load_modsource;
            return true;
        }
    }

    char abs_path[kPathSize];
    ModID modsource;
    FindFilePath(rel_path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths, true, NULL, &modsource);
    modsource_ = modsource;

    // Tell stb_image that we want 4 components (RGBA)
    int img_width = 0, img_height = 0, num_comp = 0;
    float* data = stbi_loadf(abs_path, &img_width, &img_height, &num_comp, 0);

    if (data == NULL) {
        FatalError("Error", "Could not load heightmap: %s", rel_path.c_str());
    } else {
        if (scaled) {
            width_ = kTerrainDimX;
            depth_ = kTerrainDimY;
        } else {
            width_ = img_width;
            depth_ = img_height;
        }

        int heightDataSize = width_ * depth_;

        if (0 < heightDataSize) {
            // ...allocate enough space for the height data...
            height_data_.resize(heightDataSize, 0);
            // m_flowData.resize(heightDataSize,vecf(2,0));

            float scale_factor = kTerrainHeightScale;
            float x_scale = img_width / (float)width_;
            float z_scale = img_height / (float)depth_;

            if (num_comp == 1) {                    // monochrome texture
                for (int z = 0; z < depth_; z++) {  // flipped
                    float* bits = &data[((int)(z * z_scale)) * img_height];
                    for (int x = 0; x < width_; x++) {
                        height_data_[x + (((depth_ - 1) - z) * width_)] = (bits[(int)(x * x_scale)] * 65535.0f) / scale_factor;
                    }
                }
            } else {
                FatalError("Error", "Heightmaps must be 16-bit grayscale PNGs.");
            }
            stbi_image_free(data);
            char path[kPathSize];
            FormatString(path, kPathSize, "%s%s%s", GetWritePath(modsource).c_str(), rel_path.c_str(), cache_suffix.c_str());
            WriteCacheFile(path);
            return true;
        }
    }
    return false;
}

float HeightmapImage::GetHeight(int x, int z) const {
    if (!width_ || !depth_) {
        return (0);
    }

    x = (x < 0) ? (0) : (x >= width_) ? (width_ - 1)
                                      : (x);
    z = (z < 0) ? (0) : (z >= depth_) ? (depth_ - 1)
                                      : (z);

    const int pos = x + (z * width_);
    return float(height_data_[pos]);
}

float HeightmapImage::GetHeightXZ(float x, float z) const {
    // ensure the heightmap is valid
    if (!width_ || !depth_) {
        return (0);
    }

    // clamp values to the resolution of the heightmap
    x = (x < 0) ? (0) : (x >= width_) ? (width_ - 1)
                                      : (x);
    z = (z < 0) ? (0) : (z >= depth_) ? (depth_ - 1)
                                      : (z);

    const int ix = int(x);
    const int iz = int(z);
    float xoffset = x - ix;
    float zoffset = z - iz;

    xoffset = 1 - xoffset;
    zoffset = 1 - zoffset;

    float weightTL = xoffset * zoffset;
    float weightTR = (1 - xoffset) * zoffset;
    float weightBL = xoffset * (1 - zoffset);
    float weightBR = (1 - xoffset) * (1 - zoffset);
    float total = weightTL + weightTR + weightBL + weightBR;

    weightTL /= total;
    weightTR /= total;
    weightBL /= total;
    weightBR /= total;

    return (
        GetHeight(ix, iz) * weightTL +
        GetHeight(ix + 1, iz) * weightTR +
        GetHeight(ix, iz + 1) * weightBL +
        GetHeight(ix + 1, iz + 1) * weightBR);
}
