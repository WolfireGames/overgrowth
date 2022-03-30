//-----------------------------------------------------------------------------
//           Name: heightmap.h
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

#include <Internal/integer.h>
#include <Internal/modid.h>

#include <string>
#include <vector>

class HeightmapImage
{
public:
    enum HMScale {
        DOWNSAMPLED = true,
        ORIGINAL_RES = false
    };
    
    HeightmapImage():width_(0),depth_(0),checksum_(0){}
    
    bool LoadData(const std::string& fileName, HMScale scaled);

    float GetHeight(int x, int z) const;
    float GetHeightXZ(float x, float z) const;

    inline int32_t width() const {return width_;}
    inline int32_t depth() const {return depth_;}
    inline const std::string& path() const {return rel_path_;} 
    inline uint16_t checksum() const {return checksum_;}

    ModID modsource_;
    
private:
    static const int kTerrainDimX = 1024;
    static const int kTerrainDimY = 1024;
    static const int kTerrainHeightScale = 32;
    static const int kHMCacheVersion = 1;

    bool ReadCacheFile(const std::string& path, uint16_t checksum);
    void WriteCacheFile(const std::string& path);
    bool LoadFromCacheIfAvailable(HMScale scaled);

    int32_t width_;
    int32_t depth_;
    uint16_t checksum_;
    std::vector<float> height_data_;
    std::string rel_path_;
};
