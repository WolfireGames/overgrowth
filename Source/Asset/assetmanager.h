//-----------------------------------------------------------------------------
//           Name: assetmanager.h
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

#include <Asset/assetref.h>
#include <Asset/assetmanager.h>
#include <Asset/assetloaderbase.h>
#include <Asset/assettypes.h>
#include <Asset/assetmanagerthreadhandler.h>
#include <Asset/assetloaderrors.h>

#include <Internal/common.h>
#include <Internal/datemodified.h>
#include <Internal/error.h>

#include <Logging/logdata.h>
#include <Utility/timing.h>
#include <Threading/sdl_wrapper.h>
#include <Utility/simple_vector.h>
#include <XML/Parsers/assetloadwarningparser.h>

#include <list>
#include <string>
#include <limits>

#define HOLD_LOAD_MASK_PRELOAD (1UL >> 0)

enum AssetLoadingState {
    ASSET_LOADING_UNLOADED,
    ASSET_LOADING_LOADING,
    ASSET_LOADING_LOADED
};

struct AssetInstanceCounter {
    uint32_t id;  
    uint32_t hold_load_mask;
    uint32_t load_flags;
    uint16_t count;  
    int64_t modified;
    uint32_t deallocate_attempts;
    AssetType type;
    const char* asset_type_name;
    Asset* asset;
    AssetLoadingState load_state;
    char rel_name[kPathSize];
    bool load_warning;
};

class AssetLoadedCallbackInterface  {
public:
    virtual void AssetLoadedCallback( uint32_t loadid, AssetRefBase* ref ) = 0;
    virtual void AssetFailedLoadCallback( uint32_t loadid, AssetRefBase* ref ) = 0;
};

struct LoadingInstance {
    uint32_t load_id;
    uint32_t asset_id;
    AssetLoaderBase* loader; 
    AssetLoadedCallbackInterface* caller;
    bool in_queue;
    uint32_t next_step_id;
    int load_step_result;
    AssetRefBase* asset_ref;
};

#ifdef MEASURE_LOADS
struct AssetLoadTime {
    AssetLoadTime(uint64_t time, uint32_t time_ms, const char* asset_type_name, AssetType asset_type );

    uint64_t time_ms;
    uint64_t time;
    const char* asset_type_name;
    AssetType asset_type;
};
#endif

size_t GetNextAssetListSize( size_t current_size );

/* 
 * Manager for all assets, keeps track of all assets.
 */
class AssetManager {
private: 
#ifdef MEASURE_LOADS
    std::vector<AssetLoadTime> asset_load_times;
#endif
    bool load_warning_activated;
    char load_warning_mode_name[32];
    char load_warning_level_name[256];
    AssetLoadWarningParser load_warning_instances;
    
    uint32_t asset_id_counter;

    AssetInstanceCounter* asset_list;
    size_t asset_list_count;
    size_t asset_list_size;

    void ReallocAssetInstanceCounter( size_t new_size );

    uint32_t load_id_counter;

    SimpleVector<LoadingInstance> loading_instances;

    AssetManagerThreadHandler io_thread_manager; 
    AssetManagerThreadHandler loading_thread_manager; 

    static const int MAX_ASSET_TYPE_COUNT = 32;
    int asset_type_count[MAX_ASSET_TYPE_COUNT];

    int asset_warning_count;
    int asset_hold_count;

    template <typename TAsset>
    uint32_t AllocateAsset( const std::string& rel_path, uint32_t load_flags ) {
        for( uint32_t id = 0; id < asset_list_count; id++ ) {
            if(asset_list[id].type == TAsset::GetType() && asset_list[id].load_flags == load_flags && strcmp(asset_list[id].rel_name, rel_path.c_str()) == 0) {
                return id;
            }
        }
        
        if( asset_list_count == asset_list_size ) {
            ReallocAssetInstanceCounter(GetNextAssetListSize(asset_list_size));
        } else if( asset_list_count > asset_list_size ) {
            LOGF << "Asset list count is larger than asset list size, this should never be possible" << std::endl;
        }

        bool has_warning = false;
        if( load_warning_activated && TAsset::AssetWarning() ) {
            has_warning = true;
			//LOGW << "Loading asset: \"" << rel_path << "\" " << TAsset::GetType() << " " << TAsset::GetTypeName() << " while in load warning mode. State: " << load_warning_mode_name << std::endl;
            load_warning_instances.AddAssetWarning(AssetLoadWarningParser::AssetWarning(rel_path, load_flags, TAsset::GetTypeName(), load_warning_level_name));
        }

        uint32_t asset_id = asset_id_counter++;
        TAsset *asset = new TAsset(this,asset_id);

        asset_list[asset_list_count].asset = asset;
        asset_list[asset_list_count].hold_load_mask = 0x0;
        asset_list[asset_list_count].load_flags = load_flags;
        asset_list[asset_list_count].type = TAsset::GetType();
        asset_list[asset_list_count].asset_type_name = TAsset::GetTypeName();
        asset_list[asset_list_count].id = asset_id;
        asset_list[asset_list_count].count = 0;
        asset_list[asset_list_count].load_state = ASSET_LOADING_UNLOADED;
        asset_list[asset_list_count].load_warning = has_warning;
        strncpy(asset_list[asset_list_count].rel_name, rel_path.c_str(), kPathSize);

        asset_type_count[(int)TAsset::GetType()]++;

        if( asset_list[asset_list_count].load_warning ) {
            asset_warning_count++;
        }

        return (uint32_t) asset_list_count++;
    }

    uint32_t GetAsset( uint32_t asset_id );

    void DeallocateAsset( uint32_t asset_id, uint32_t starting_id );

public:
    //Increment reference count to asset, indicating there are more valid references in existance.
    void IncrementAsset( uint32_t asset_id );

    //Decrement reference count to asset, marking it for deletion during update if it reaches zero.
    void DecrementAsset( uint32_t asset_id );

    void Initialize();

    bool UnloadUnreferenced( int limit, unsigned int required_attempts );

    template<typename TAsset>
    void Reload() {
        LOGI << "Reloading " << TAsset::GetTypeName() << std::endl;
        int64_t new_modified;
        for( size_t i = 0; i < asset_list_count; i++ ) {
            if( asset_list[i].type == TAsset::GetType() ) {
                TAsset* asset = static_cast<TAsset*>(asset_list[i].asset);
                Path file_path = FindFilePath(asset->path_, kDataPaths | kModPaths | kAbsPath, false);
                new_modified = GetDateModifiedInt64(file_path.GetFullPath());
                if(asset_list[i].modified != new_modified) {
                    asset_list[i].modified = new_modified;
                    asset->Reload();
                }
            }
        }
    }

    void Dispose();
    AssetManager();
    ~AssetManager();

    /*
     * Request that an asset is to be loaded
     * 
     * returns the globally unique id of the asset loaded.
     * The hold load mask is set to the asset loaded, if this mask is set
     * to anything else than zero the asset manager will not unload the asset until the mask
     * has been release by calling ReleaseHoldLoadMask(uint32_t) including all the bits set with the original call (or more)
     * If the asset exists since before and the mask was set to something, the masks will be bitwise OR'd together.
     */

     template<typename TAsset> 
     uint32_t LoadAssetAsynchronized( const std::string& instr, uint32_t load_flags, AssetLoadedCallbackInterface* callback, uint32_t hold_load_mask ) {
        std::string str = SanitizePath(instr); 

        uint32_t asset_index = AllocateAsset<TAsset>(str, load_flags);
        uint32_t load_index = loading_instances.Allocate();

        loading_instances[load_index].load_id = load_id_counter++;
        loading_instances[load_index].asset_id = asset_list[asset_index].id;

        if(asset_list[asset_index].hold_load_mask == 0x0 && hold_load_mask != 0x0 ) {
            asset_hold_count++;
        }

        asset_list[asset_index].hold_load_mask |= hold_load_mask;

        if( asset_list[asset_index].load_state == ASSET_LOADING_LOADED
            || asset_list[asset_index].load_state == ASSET_LOADING_LOADING ) {
            loading_instances[load_index].in_queue = false;
            loading_instances[load_index].next_step_id = std::numeric_limits<uint32_t>::max(); //If we loaded, we did all the steps.
            loading_instances[load_index].loader = NULL;
            loading_instances[load_index].caller = callback;
            loading_instances[load_index].asset_ref = new AssetRef<TAsset>(asset_list[asset_index].asset);
            loading_instances[load_index].load_step_result = kLoadOk;

        } else if( asset_list[asset_index].load_state == ASSET_LOADING_UNLOADED ) {
            asset_list[asset_index].load_state = ASSET_LOADING_LOADING;
            loading_instances[load_index].in_queue = false;
            loading_instances[load_index].next_step_id = 0;
            loading_instances[load_index].loader = asset_list[asset_index].asset->NewLoader();
            loading_instances[load_index].caller = callback;
            loading_instances[load_index].asset_ref = new AssetRef<TAsset>(asset_list[asset_index].asset);
            loading_instances[load_index].load_step_result = kLoadOk;

            loading_instances[load_index].loader->Initialize(str,load_flags,asset_list[asset_index].asset);
        } else {
            LOGE << "unknown load state" << std::endl; 
        }

        return loading_instances[load_index].load_id;
    }

    template<typename TAsset> 
    uint32_t LoadAssetAsynchronized( const std::string& str, AssetLoadedCallbackInterface* callback ) {
       return LoadAssetAsynchronized<TAsset>(str, callback, 0x0, 0x0);
    }

    //Load flags are specifically used by texture loading.
    template<typename TAsset>
    AssetRef<TAsset> LoadSync( const std::string& instr, uint32_t load_flags, uint32_t hold_load_mask ) {
        std::string str = SanitizePath(instr); 

        uint32_t internal_id = AllocateAsset<TAsset>(str, load_flags);
        bool failed_load = false;

        if(asset_list[internal_id].hold_load_mask == 0x0 && hold_load_mask != 0x0 ) {
            asset_hold_count++;
        }

        asset_list[internal_id].hold_load_mask |= hold_load_mask;
        
        TAsset * asset = static_cast<TAsset*>(asset_list[internal_id].asset);

        if( asset_list[internal_id].load_state == ASSET_LOADING_LOADING ) {
            char buffer[kPathSize];
            strcpy(buffer, asset_list[internal_id].rel_name);
            LOGI << "Trying to synchronously load an asset being asynchronously loaded" 
                    ", will wait until it's finished." << std::endl;
            while( asset_list[internal_id].load_state != ASSET_LOADING_LOADED ) {
                Update();
            }

            // This was discovered when a SyncedAnimationGroup (A) required another SyncedAnimationGroup (B) which in turn required A
            if(strcmp(buffer, asset_list[internal_id].rel_name) != 0) {
                LOGW << buffer << " internal_id changed while loading it" << std::endl;
                LOGW << "There is a possible circular include, make sure " << buffer << " doesn't require something that in turn requires " << buffer << std::endl;
                failed_load = true;
            }
        }

        if( !failed_load && asset_list[internal_id].load_state == ASSET_LOADING_UNLOADED ) {
            asset_list[internal_id].load_state = ASSET_LOADING_LOADING;
#ifdef MEASURE_LOADS
            uint64_t start_load = getCPUTSC();
            uint32_t start_load_ms = SDL_TS_GetTicks();
#endif
            AssetLoaderBase* loader = asset->NewLoader();
            loader->Initialize(str, load_flags, asset);

            ErrorResponse ret = _retry;
            while( ret == _retry ) {
                ret = _continue;
                for( int i = 0; i < loader->GetAssetLoadStepCount(); i++ ) {
                    int err = loader->DoLoadStep(i); 
                    if( err != kLoadOk ) {
                        {
                            LOGE << "Error loading asset type: "
                                << loader->GetTypeName()
                                << " from path: "
                                << asset_list[internal_id].rel_name
                                <<  ". Reason: "
                                << GetLoadErrorString(err)
                                << ", specifically: \""
                                << loader->GetLoadErrorString()
                                << loader->GetLoadErrorStringExtended()
                                << "\""
                                << std::endl;
                        }

                        ret = DisplayFormatError( _ok_cancel_retry, 
                                                    true, 
                                                    "Asset Load Error", 
                                                    "Error loading asset type: %s from path: %s.\n\nReason: %s, specifically: \"%s\", \nSee log output for more details (if available)",
                                                    loader->GetTypeName(), 
                                                    asset_list[internal_id].rel_name,
                                                    GetLoadErrorString(err),
                                                    loader->GetLoadErrorString()
                                                                 
                        );

                        if( ret == _continue ) {
                            failed_load = true;
                        }
                        break;
                    }
                }
            }

            delete loader;
#ifdef MEASURE_LOADS
            asset_load_times.push_back(AssetLoadTime(getCPUTSC()-start_load, SDL_TS_GetTicks()-start_load_ms, TAsset::GetTypeName(), TAsset::GetType() ));
#endif
            asset_list[internal_id].modified = GetDateModifiedInt64(str.c_str());
            asset_list[internal_id].load_state = ASSET_LOADING_LOADED;
        } else if( asset_list[internal_id].load_state == ASSET_LOADING_LOADED ) {

        } else {
            LOGE << "Unknown load state for asset" << std::endl;
        }
        
        if( failed_load ) {
            return AssetRef<TAsset>();
        } else {
            return AssetRef<TAsset>(asset);
        }
    }

    template<typename TAsset>
    AssetRef<TAsset> LoadSync( const std::string& str ) {
        return LoadSync<TAsset>( str, 0x0, 0x0 );
    }

    void LoadSync( AssetType type, const std::string& str, uint32_t load_flags, uint32_t hold_load_mask );

    /*
     * Main thread update step
     */
    void Update();

    void ReleaseAssetHoldLoad( uint32_t hold_load_mask );

    //Set if we want the asset-loading warning outputs.
    //This is a tool for determining if and what assets
    //are being loaded after the loading screen and during gameplay, which shouldn't
    //happen.
    void SetLoadWarningMode(bool val, const char* mode_name, const char* level_name);
    //Clear and dump load warnings to write directory .xml
    void DumpLoadWarningData(const char* destination);  

    size_t GetLoadedAssetCount();
    const char* GetAssetName(uint32_t index);
    AssetType GetAssetType(uint32_t index);
    uint32_t GetAssetHoldFlags(uint32_t index);
    uint16_t GetAssetRefCount(uint32_t index);
    bool GetAssetLoadWarning(uint32_t index);

    int GetAssetTypeCount( AssetType asset );

    int GetAssetWarningCount();
    int GetAssetHoldCount();
#ifdef MEASURE_LOADS
    void DumpTimingData();
#endif
};

