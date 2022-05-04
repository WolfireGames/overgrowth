//-----------------------------------------------------------------------------
//           Name: image_sampler.h
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

#include <Asset/assetbase.h>
#include <Asset/assetinfobase.h>
#include <Asset/assets.h>

#include <Math/vec4.h>
#include <Images/freeimage_wrapper.h>

#include <vector>

class ImageSampler : public Asset {    
    struct byte4 {
        unsigned char entries[4];
        inline unsigned char& operator[](int which){return entries[which];}
    };
public:
    ImageSampler( AssetManager* owner, uint32_t asset_id );    

    static AssetType GetType() { return IMAGE_SAMPLER_ASSET; }
    static const char* GetTypeName() { return "IMAGE_SAMPLER_ASSET"; }
    static bool AssetWarning() { return true; }

    int sub_error;
    int Load(const std::string& path, uint32_t load_flags);
    const char* GetLoadErrorString();
    const char* GetLoadErrorStringExtended() { return ""; }
    void Unload();
    void Reload();
    void ReportLoad() override;

    vec4 GetInterpolatedColorUV(float x, float y) const;
    bool GetCachePath(std::string* dst);

    AssetLoaderBase* NewLoader() override;
    
    ModID modsource_;

private:
    static const int kImageSamplerCacheVersion = 2;

    byte4 GetPixel( int x, int y ) const;
    void LoadDataFromFIBitmap(FIBITMAP* image);
    vec4 GetInterpolatedColor(float x, float y) const;
    bool ReadCacheFile(const std::string& path, uint16_t checksum);
    void WriteCacheFile(const std::string& path);

    std::vector<byte4> pixels_;
    int width_, height_; 
    uint16_t checksum_;

};

typedef AssetRef<ImageSampler> ImageSamplerRef;
