//-----------------------------------------------------------------------------
//           Name: fallbackassetloader.h
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

#include <Asset/assetloaderbase.h>
#include <Asset/assetloaderrors.h>

#include <Utility/assert.h>

template <typename TAsset>
class FallbackAssetLoader : public AssetLoaderBase {
private:
    std::string path;
    uint32_t load_flags;

    TAsset* asset;
    std::string error_message;
    std::string error_message_extended;

public:
    FallbackAssetLoader() {

    }

    ~FallbackAssetLoader() override {
        
    }

    void Initialize(const std::string& path, uint32_t load_flags, Asset* asset) override {
        this->load_flags = load_flags;
        this->path = path; 
        this->asset = static_cast<TAsset*>(asset);
    }

    const AssetLoaderStepType* GetAssetLoadSteps() const override {
        static const AssetLoaderStepType tl[] = { ASSET_LOADER_SYNC_JOB };
        return tl;
    }

    const int GetAssetLoadStepCount() const override {
        return 1;
    }

    int DoLoadStep( const int step_id ) override {
        switch( step_id ) {
            case 0:
                asset->path_ = path;
                int error_code = asset->Load(path,load_flags);
                if( error_code == kLoadOk ) {
                    asset->ReportLoad();
                } else {
                    error_message = asset->GetLoadErrorString();
                    error_message_extended = asset->GetLoadErrorStringExtended();
                }
                return error_code;
                break;
        }
        return false;
    }
     
    const char* GetLoadErrorString() override {
        return error_message.c_str();
    }

    const char* GetLoadErrorStringExtended() override {
        return error_message_extended.c_str();
    }

    const AssetLoaderStepType* GetAssetUnloadSteps() const override {
        static const AssetLoaderStepType tl[] = { ASSET_LOADER_SYNC_JOB };
        return tl;
    }

    const unsigned GetAssetUnloadStepCount() const override {
        return 1;
    }

    bool DoUnloadStep( const int step_id ) override {
        switch( step_id )
        {
            case 0:
                asset->path_ = path;
                asset->Unload();
                return true;
                break;
        }
        return false;
    }

    const char* GetTypeName() override {
        return TAsset::GetTypeName(); 
    }
};
