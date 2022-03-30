//-----------------------------------------------------------------------------
//           Name: assetbase.h
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
#include <Internal/filesystem.h>

#include <Asset/assettypes.h>
#include <Asset/assetloaderbase.h>

#include <typeinfo>
#include <string>

template<class T>
class AssetRef;

class AssetManager;

class Asset {
private:
    friend class AssetManager;

public:
    int32_t asset_id;
    std::string path_;
    AssetManager* owner;

    virtual void ReportLoad() {}
    virtual bool Valid() { return true; }
    virtual const char* GetName() { return typeid(*this).name(); }

    Asset(AssetManager* source, uint32_t asset_id);
    virtual ~Asset();

    void IncrementRefCount();
    void DecrementRefCount();
    
    //virtual static AssetType GetType() = 0;
    //static AssetType GetAssetType() = 0 ;
    virtual AssetLoaderBase* NewLoader() = 0;
};

///*
// * An asset is an object which is read from disk or other data source and can be used within the game, like textures or models. This is what a datatype would implement.
// */
//class NAssetBase
//{
//     
//private:
//    friend class NAssetManager;
//
//    /*
//     * Id for asset instance
//     */
//    uint32_t asset_id;
//    AssetType asset_type;
//    NAssetManager* owner;
//
//    NAssetBase(NAssetManager* source, uint32_t asset_id, AssetType asset_type);
//    virtual ~NAssetBase();
//    
//    inline AssetType GetAssetType() {return asset_type;};
//    /* Returns a reference to the class which handles the loading and unloading of this particular asset */
//    /* Needs to be implemented on every variant of an asset, non virtual */
//};
