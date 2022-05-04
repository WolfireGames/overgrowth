//-----------------------------------------------------------------------------
//           Name: averagecolorasset.h
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

#include <Math/vec3.h>
#include <Math/vec4.h>

#include <Asset/assetbase.h>
#include <Asset/assetinfobase.h>

#include <Internal/modid.h>

#include <string>

using std::string;

class AverageColor : public Asset {
    public:
        AverageColor(AssetManager* owner, uint32_t asset_id);
        static AssetType GetType() { return AVERAGE_COLOR_ASSET; }
        static const char* GetTypeName() { return "AVERAGE_COLOR_ASSET"; }
        static bool AssetWarning() { return true; }

        int Load( const string &path, uint32_t load_flags );
        const char* GetLoadErrorString();
        const char* GetLoadErrorStringExtended() { return ""; }
        void Unload();
        void Reload();
        void ReportLoad() override;

        inline const vec4& color() const {return color_;}

        AssetLoaderBase* NewLoader() override;

        ModID modsource_;
    private:
        static const int kAverageColorCacheVersion = 2;
        bool ReadCacheFile(const string& path, uint16_t checksum);
        void WriteCacheFile(const string& path, uint16_t checksum);        

    
        vec4 color_;
};

typedef AssetRef<AverageColor> AverageColorRef;
