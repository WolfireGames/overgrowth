//-----------------------------------------------------------------------------
//           Name: assetloaderbase.h
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

#include <Internal/filesystem.h>
#include <Internal/integer.h>

class AssetManager;
class Asset;

enum AssetLoaderStepType
{
    ASSET_LOADER_DISK_IO, //Perform job in IO thread
    ASSET_LOADER_ASYNC_JOB, //Perform job in asynchronous work thread
    ASSET_LOADER_SYNC_JOB //Perform job in main thread (like OpenGL calls)
};

/*
 * An asset loader is responsible for loading, interpreting and potentially moving data for an asset.
 */
class AssetLoaderBase
{
public:
    AssetLoaderBase();
    virtual ~AssetLoaderBase();

    virtual void Initialize(const std::string& path, uint32_t load_flags, Asset* asset) = 0;

    virtual const AssetLoaderStepType* GetAssetLoadSteps() const = 0;
    virtual const int GetAssetLoadStepCount() const = 0;

    virtual int DoLoadStep( const int step_id ) = 0;

    virtual const AssetLoaderStepType* GetAssetUnloadSteps() const = 0;
    virtual const unsigned GetAssetUnloadStepCount() const = 0;

    virtual bool DoUnloadStep( const int step_id ) = 0;

    virtual const char* GetTypeName() = 0;
    virtual const char* GetLoadErrorString() = 0;
    virtual const char* GetLoadErrorStringExtended() = 0;
};
