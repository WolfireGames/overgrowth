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
#include <Images/freeimage_wrapper.h>

bool HeightmapImage::ReadCacheFile(const std::string& path, uint16_t checksum) {
    FILE* file = my_fopen(path.c_str(),"rb");
    if(!file){
        return false;
    }
    CacheFile::ScopedFileCloser file_closer(file);
    unsigned char version;
    fread(&version, sizeof(version), 1, file);
    if(version != kHMCacheVersion){
        return false;
    }
    uint16_t file_checksum;
    fread(&file_checksum, sizeof(file_checksum), 1, file);
    if(checksum != 0 && checksum != file_checksum){
        return false;
    }
    checksum_ = file_checksum;
    fread(&width_, sizeof(width_), 1, file);
    fread(&depth_, sizeof(depth_), 1, file);
    height_data_.resize(width_*depth_);
    fread(&height_data_[0], sizeof(height_data_[0])*width_*depth_, 1, file);
    return true;  
}

void HeightmapImage::WriteCacheFile(const std::string& path) {
    FILE* file = my_fopen(path.c_str(),"wb");
    CacheFile::ScopedFileCloser file_closer(file);
    unsigned char version = kHMCacheVersion;
    fwrite(&version, sizeof(version), 1, file);
    fwrite(&checksum_, sizeof(checksum_), 1, file);
    fwrite(&width_, sizeof(width_), 1, file);
    fwrite(&depth_, sizeof(depth_), 1, file);
    fwrite(&height_data_[0], sizeof(height_data_[0]), width_*depth_, file);
}

bool HeightmapImage::LoadData(const std::string& rel_path, HMScale scaled) {
    height_data_.clear();
    
    rel_path_ = rel_path;
    
    std::string cache_suffix = "_hmcache";
    if(scaled == DOWNSAMPLED){
        cache_suffix += "_scaled";
    }

    std::string load_path;
    ModID load_modsource;
    if(CacheFile::CheckForCache(rel_path_, cache_suffix, &load_path, &load_modsource, &checksum_)){
        if(ReadCacheFile(load_path, checksum_)){
            modsource_ = load_modsource;
            return true;
        }
    }
    
    char abs_path[kPathSize];
    ModID modsource;
    FindFilePath(rel_path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths, true, NULL, &modsource);
    modsource_ = modsource;
    FIBitmapContainer image_container(GenericLoader(abs_path, 0));
    FIBITMAP* image = image_container.get();
    //FreeImage_Save(FIF_TARGA, image, (file_name+".tga").c_str());
    if (image == NULL) {
        FatalError("Error", "Could not load heightmap: %s", rel_path.c_str());
    } else {
		fiTYPE image_type = getImageType(image);
        int bytes_per_pixel = getBPP(image) / 8;
        if (scaled) {
            width_ = kTerrainDimX;//FreeImage_GetWidth(image);
            depth_ = kTerrainDimY;//FreeImage_GetHeight(image);
        } else {
            width_ = getWidth(image);
            depth_ = getHeight(image);
        }

        int heightDataSize = width_ * depth_;
        int imageDataSize = heightDataSize * bytes_per_pixel;
        
        if (0 < heightDataSize && 0 < imageDataSize) {
        
            // ...allocate enough space for the height data...
            height_data_.resize(heightDataSize,0);
            //m_flowData.resize(heightDataSize,vecf(2,0));
            
            float scale_factor = kTerrainHeightScale;
            float x_scale = getWidth(image)/(float)width_;
            float z_scale = getHeight(image)/(float)depth_;
            
            if (image_type == FIWT_UINT16) {        // e.g. 16 bit PNGs
                for(int z = 0; z < depth_; z++) {    // flipped
                    unsigned short *bits = (unsigned short *)getScanLine(image, (int)(z*z_scale));
                    for(int x = 0; x < width_; x++) {
                        height_data_[x + (((depth_-1) - z) * width_)] = ((float)bits[(int)(x*x_scale)])/scale_factor;
                    }
                }
            } else if (image_type == FIWT_BITMAP) {
                for(int z = 0; z < depth_; z++) {
                    uint8_t *bits = (uint8_t *)getScanLine(image, (int)(z*z_scale));
                    for(int x = 0; x < width_; x++) {
                        height_data_[x + (((depth_-1) - z) * width_)] = ((float)bits[(int)(x*x_scale)*bytes_per_pixel])/scale_factor*256.0f;
                    }
                }
            } else {
                FatalError("Error","Heightmaps must be 16-bit grayscale PNGs.");
            }
            char path[kPathSize];
            FormatString(path, kPathSize, "%s%s%s", GetWritePath(modsource).c_str(), rel_path.c_str(), cache_suffix.c_str());
            WriteCacheFile(path);
            return true;
        }
    }
    return false;    
}

float HeightmapImage::GetHeight(int x, int z) const
{
    if (!width_ || !depth_) {
        return (0);
    }
    
    x = (x < 0) ? (0) : (x >= width_) ? (width_ - 1) : (x); 
    z = (z < 0) ? (0) : (z >= depth_) ? (depth_ - 1) : (z);

    const int pos = x + (z * width_);
    return float(height_data_[pos]);
}

float HeightmapImage::GetHeightXZ(float x, float z) const
{
    // ensure the heightmap is valid
    if (!width_ || !depth_)
    {
        return (0);
    }

    // clamp values to the resolution of the heightmap
    x = (x < 0) ? (0) : (x >= width_) ? (width_ - 1) : (x);
    z = (z < 0) ? (0) : (z >= depth_) ? (depth_ - 1) : (z);

    const int ix = int(x);
    const int iz = int(z);
    float xoffset = x - ix;
    float zoffset = z - iz;

    xoffset = 1 - xoffset;
    zoffset = 1 - zoffset;

    float weightTL = xoffset*zoffset;
    float weightTR = (1-xoffset)*zoffset;
    float weightBL = xoffset*(1-zoffset);
    float weightBR = (1-xoffset)*(1-zoffset);
    float total = weightTL+weightTR+weightBL+weightBR;

    weightTL /= total;
    weightTR /= total;
    weightBL /= total;
    weightBR /= total;

    return (
        GetHeight(ix, iz) * weightTL +
        GetHeight(ix+1, iz) * weightTR +
        GetHeight(ix, iz+1) * weightBL +
        GetHeight(ix+1, iz+1) * weightBR);
}
