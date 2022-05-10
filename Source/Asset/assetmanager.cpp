//-----------------------------------------------------------------------------
//           Name: assetmanager.cpp
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
#include "assetmanager.h"

#include <Asset/Asset/actorfile.h>
#include <Asset/Asset/ambientsounds.h>
#include <Asset/Asset/animation.h>
#include <Asset/Asset/attachmentasset.h>
#include <Asset/Asset/attacks.h>
#include <Asset/Asset/averagecolorasset.h>
#include <Asset/Asset/character.h>
#include <Asset/Asset/decalfile.h>
#include <Asset/Asset/fzx_file.h>
#include <Asset/Asset/hotspotfile.h>
#include <Asset/Asset/image_sampler.h>
#include <Asset/Asset/item.h>
#include <Asset/Asset/levelset.h>
#include <Asset/Asset/lipsyncfile.h>
#include <Asset/Asset/material.h>
#include <Asset/Asset/objectfile.h>
#include <Asset/Asset/particletype.h>
#include <Asset/Asset/reactions.h>
#include <Asset/Asset/skeletonasset.h>
#include <Asset/Asset/songlistfile.h>
#include <Asset/Asset/soundgroup.h>
#include <Asset/Asset/soundgroupinfo.h>
#include <Asset/Asset/spawnpointinfo.h>
#include <Asset/Asset/syncedanimation.h>
#include <Asset/Asset/texturedummy.h>
#include <Asset/Asset/voicefile.h>
#include <Asset/Asset/levelinfo.h>
#include <Asset/Asset/filehash.h>
#include <Asset/assetloaderrors.h>

#include <Logging/logdata.h>
#include <Utility/strings.h>

#include <cstdlib>
#include <stdint.h>
#include <stdio.h>
#include <set>
#include <limits>

#ifdef MEASURE_LOADS
AssetLoadTime::AssetLoadTime(uint64_t time, uint32_t time_ms, const char* asset_type_name, AssetType asset_type) : time(time),
                                                                                                                   time_ms(time_ms),
                                                                                                                   asset_type_name(asset_type_name),
                                                                                                                   asset_type(asset_type) {
}
#endif

AssetManager::AssetManager() : asset_id_counter(1),
                               asset_list(NULL),
                               asset_list_count(0),
                               asset_list_size(0),
                               load_id_counter(1),
                               loading_instances(256, 128),
                               io_thread_manager(1),
                               loading_thread_manager(1),
                               load_warning_activated(false),
                               asset_warning_count(0),
                               asset_hold_count(0) {
    LOG_ASSERT((int)ASSET_TYPE_FINAL < MAX_ASSET_TYPE_COUNT);

    memset(asset_type_count, 0, sizeof(int) * MAX_ASSET_TYPE_COUNT);
}

AssetManager::~AssetManager() {
    free(asset_list);
    asset_list = NULL;
    asset_list_count = 0;
    asset_list_size = 0;
}

void AssetManager::Initialize() {
}

bool AssetManager::UnloadUnreferenced(int limit, unsigned int required_attempts) {
    int removes = 0;
    for (int i = (int)asset_list_count - 1; i >= 0; i--) {
        if (asset_list[i].count <= 0) {
            if (asset_list[i].hold_load_mask == 0x0) {
                if (asset_list[i].deallocate_attempts >= required_attempts) {
                    // LOGI << "Unloading unreferenced " << asset_list[i].asset_type_name << std::endl;
                    Asset* asset = asset_list[i].asset;
                    AssetLoaderBase* loader = asset->NewLoader();
                    loader->Initialize(asset_list[i].rel_name, asset_list[i].load_flags, asset);
                    for (unsigned k = 0; k < loader->GetAssetUnloadStepCount(); k++) {
                        loader->DoUnloadStep(k);
                    }
                    delete loader;
                    DeallocateAsset(asset_list[i].id, i);
                    removes++;
                } else {
                    asset_list[i].deallocate_attempts++;
                }
            }
        }

        if (removes >= limit && limit > 0) {
            break;
        }
    }

    return removes > 0;
}

uint32_t AssetManager::GetAsset(uint32_t asset_id) {
    for (unsigned i = 0; i < asset_list_count; i++) {
        if (asset_list[i].id == asset_id) {
            return i;
        }
    }
    return std::numeric_limits<uint32_t>::max();
}

void AssetManager::DeallocateAsset(uint32_t asset_id, uint32_t starting_id) {
    bool shifting = false;
    for (unsigned int i = starting_id; i < asset_list_count; i++) {
        if (shifting) {  // Moving all other assets one step
            asset_list[i - 1] = asset_list[i];
        } else {  // Searching
            if (asset_list[i].id == asset_id) {
                if (asset_list[i].load_warning) {
                    asset_warning_count--;
                }
                if (asset_list[i].hold_load_mask != 0x0) {
                    asset_hold_count--;
                }

                delete asset_list[i].asset;
                asset_type_count[(int)asset_list[i].type]--;
                shifting = true;
            }
        }
    }

    if (shifting) {  // We removed something
        asset_list_count--;
    }
}

void AssetManager::Dispose() {
    DumpLoadWarningData("asset_manager_warnings.xml");
#ifdef MEASURE_LOADS
    DumpTimingData();
#endif
    LOGI << "Releasing hold on all preloaded assets" << std::endl;
    ReleaseAssetHoldLoad(HOLD_LOAD_MASK_PRELOAD);
    LOGI << "Disposing asset manager" << std::endl;
    while (UnloadUnreferenced(0, 0)) {
    };

    if (asset_list_count > 0) {
        LOGW << "Following assets are still loaded after assetmanager is called for disposal" << std::endl;

        for (unsigned int i = 0; i < asset_list_count; i++) {
            LOGW << "References: " << asset_list[i].count << " path: " << asset_list[i].rel_name << std::endl;
            ;
        }

        LOGW << "Marking them as orphans" << std::endl;
        for (unsigned int i = 0; i < asset_list_count; i++) {
            asset_list[i].asset->owner = NULL;
        }
    }
}

size_t GetNextAssetListSize(size_t current_size) {
    return current_size + 1024;
}

void AssetManager::ReallocAssetInstanceCounter(size_t new_size) {
    asset_list_size = new_size;
    asset_list = static_cast<AssetInstanceCounter*>(realloc(asset_list, asset_list_size * sizeof(AssetInstanceCounter)));
}

void AssetManager::IncrementAsset(uint32_t asset_id) {
    for (size_t i = 0; i < asset_list_count; i++) {
        if (asset_list[i].id == asset_id) {
            asset_list[i].deallocate_attempts = 0;
            asset_list[i].count++;
            return;
        }
    }
    LOGW << "Unable to find asset of id for increment: " << asset_id << std::endl;
}

void AssetManager::DecrementAsset(uint32_t asset_id) {
    for (size_t i = 0; i < asset_list_count; i++) {
        if (asset_list[i].id == asset_id) {
            asset_list[i].count--;
            if (asset_list[i].count <= 0) {
                // LOGI << "Asset count is " << asset_list[i].count << " " <<  asset_list[i].asset_type_name << " " << asset_list[i].asset->path_ << std::endl;
            }
            return;
        }
    }
    LOGW << "Unable to find asset of id for decrement: " << asset_id << std::endl;
}

void AssetManager::LoadSync(AssetType type, const std::string& str, uint32_t load_flags, uint32_t hold_load_mask) {
    switch (type) {
#ifndef OGDA
        case SKELETON_ASSET:
            LoadSync<SkeletonAsset>(str, load_flags, hold_load_mask);
            break;
        case SYNCED_ANIMATION_GROUP_ASSET:
            LoadSync<SyncedAnimationGroup>(str, load_flags, hold_load_mask);
            break;
        case ANIMATION_ASSET:
            LoadSync<Animation>(str, load_flags, hold_load_mask);
            break;
        case OBJECT_FILE_ASSET:
            LoadSync<ObjectFile>(str, load_flags, hold_load_mask);
            break;
        case MATERIAL_ASSET:
            LoadSync<Material>(str, load_flags, hold_load_mask);
            break;
        case ATTACK_ASSET:
            LoadSync<Attack>(str, load_flags, hold_load_mask);
            break;
        case VOICE_FILE_ASSET:
            LoadSync<VoiceFile>(str, load_flags, hold_load_mask);
            break;
        // TODO: readd and implement properly
        // case ANIMATION_EFFECT_ASSET:
        //     LoadSync<AnimationEffect>(str, load_flags, hold_load_mask);
        //     break;
        case SOUND_GROUP_INFO_ASSET:
            LoadSync<SoundGroupInfo>(str, load_flags, hold_load_mask);
            break;
        case CHARACTER_ASSET:
            LoadSync<Character>(str, load_flags, hold_load_mask);
            break;
        case REACTION_ASSET:
            LoadSync<Reaction>(str, load_flags, hold_load_mask);
            break;
        case DECAL_FILE_ASSET:
            LoadSync<DecalFile>(str, load_flags, hold_load_mask);
            break;
        case HOTSPOT_FILE_ASSET:
            LoadSync<HotspotFile>(str, load_flags, hold_load_mask);
            break;
        case AMBIENT_SOUND_ASSET:
            LoadSync<AmbientSound>(str, load_flags, hold_load_mask);
            break;
        case ACTOR_FILE_ASSET:
            LoadSync<ActorFile>(str, load_flags, hold_load_mask);
            break;
        case ITEM_ASSET:
            LoadSync<Item>(str, load_flags, hold_load_mask);
            break;
        case LEVEL_SET_ASSET:
            LoadSync<LevelSet>(str, load_flags, hold_load_mask);
            break;
        case IMAGE_SAMPLER_ASSET:
            LoadSync<ImageSampler>(str, load_flags, hold_load_mask);
            break;
        case SOUND_GROUP_ASSET:
            LoadSync<SoundGroup>(str, load_flags, hold_load_mask);
            break;
        case PARTICLE_TYPE_ASSET:
            LoadSync<ParticleType>(str, load_flags, hold_load_mask);
            break;
        case AVERAGE_COLOR_ASSET:
            LoadSync<AverageColor>(str, load_flags, hold_load_mask);
            break;
        case FZX_ASSET:
            LoadSync<FZXAsset>(str, load_flags, hold_load_mask);
            break;
        case ATTACHMENT_ASSET:
            LoadSync<Attachment>(str, load_flags, hold_load_mask);
            break;
        case LIP_SYNC_FILE_ASSET:
            LoadSync<LipSyncFile>(str, load_flags, hold_load_mask);
            break;
        case TEXTURE_DUMMY_ASSET:
            LoadSync<TextureDummy>(str, load_flags, hold_load_mask);
            break;
        case TEXTURE_ASSET:
            LoadSync<TextureAsset>(str, load_flags, hold_load_mask);
            break;
        case LEVEL_INFO_ASSET:
            LoadSync<LevelInfoAsset>(str, load_flags, hold_load_mask);
            break;
        case FILE_HASH_ASSET:
            LoadSync<FileHashAsset>(str, load_flags, hold_load_mask);
            break;
        case UNKNOWN:
            LOGE << "Cannot load UNKNOWN asset type" << std::endl;
            break;
        case ASSET_TYPE_FINAL:
            LOGE << "Cannot load ASSET_TYPE_FINAL, it's not an actual asset type, just a marker in the enum" << std::endl;
            break;
#else
        default:
            // LOGE << "Unahandled asset type " << type << " in LoadSync using AssetType value" << std::endl;
            break;
#endif
    }
}

void AssetManager::Update() {
    UnloadUnreferenced(1, 60 * 20);

    std::vector<uint32_t> remove_loading_instance_indexes;

    // Pop all finished async jobs.
    while (io_thread_manager.queue_out.Count() > 0) {
        AssetLoaderThreadedInstance alti = io_thread_manager.queue_out.Pop();
        for (unsigned i = 0; i < loading_instances.Count(); i++) {
            if (alti.load_id == loading_instances[i].load_id) {
                loading_instances[i].in_queue = false;
                loading_instances[i].load_step_result = alti.return_state;
            }
        }
    }
    while (loading_thread_manager.queue_out.Count() > 0) {
        AssetLoaderThreadedInstance alti = loading_thread_manager.queue_out.Pop();
        for (unsigned i = 0; i < loading_instances.Count(); i++) {
            if (alti.load_id == loading_instances[i].load_id) {
                loading_instances[i].in_queue = false;
                loading_instances[i].load_step_result = alti.return_state;
            }
        }
    }

    // Check for errors and update all loading instances
    for (unsigned i = 0; i < loading_instances.Count(); i++) {
        LoadingInstance& loading_instance = loading_instances[i];
        if (loading_instance.in_queue == false) {
            if (loading_instance.load_step_result != kLoadOk) {
                int ret = DisplayFormatError(_ok_cancel_retry,
                                             true,
                                             "Asset Load Error",
                                             "Error loading asset type: %s from path: %s.\n\nReason: %s, specifically: \"%s\"",
                                             loading_instance.loader->GetTypeName(),
                                             asset_list[GetAsset(loading_instance.asset_id)].rel_name,
                                             GetLoadErrorString(loading_instance.load_step_result),
                                             loading_instance.loader->GetLoadErrorString());

                if (ret == _retry) {
                    loading_instance.next_step_id = 0;
                    loading_instance.load_step_result = kLoadOk;
                } else {
                    loading_instance.next_step_id = loading_instance.loader->GetAssetLoadStepCount();
                }
            }

            if ((int)loading_instance.next_step_id < loading_instance.loader->GetAssetLoadStepCount()) {
                AssetLoaderStepType asset_type = loading_instance.loader->GetAssetLoadSteps()[loading_instance.next_step_id];

                if (asset_type == ASSET_LOADER_DISK_IO) {
                    loading_instance.in_queue = true;
                    io_thread_manager.queue_in.Push(
                        AssetLoaderThreadedInstance(
                            loading_instance.loader,
                            loading_instance.load_id,
                            loading_instance.next_step_id));
                }
                if (asset_type == ASSET_LOADER_ASYNC_JOB) {
                    loading_instance.in_queue = true;
                    loading_thread_manager.queue_in.Push(
                        AssetLoaderThreadedInstance(
                            loading_instance.loader,
                            loading_instance.load_id,
                            loading_instance.next_step_id));
                }
                if (asset_type == ASSET_LOADER_SYNC_JOB) {
                    loading_instance.load_step_result = loading_instance.loader->DoLoadStep(loading_instance.next_step_id);
                }
                loading_instance.next_step_id++;
            } else if ((int)loading_instance.next_step_id >= loading_instance.loader->GetAssetLoadStepCount()) {
                uint32_t asset_index = GetAsset(loading_instance.asset_id);
                asset_list[asset_index].load_state = ASSET_LOADING_LOADED;

                if (loading_instance.load_step_result == kLoadOk) {
                    loading_instance.caller->AssetLoadedCallback(loading_instance.load_id, loading_instance.asset_ref);
                } else {
                    loading_instance.caller->AssetFailedLoadCallback(loading_instance.load_id, loading_instance.asset_ref);
                }

                delete loading_instance.asset_ref;
                loading_instance.asset_ref = NULL;

                remove_loading_instance_indexes.push_back(i);
            }
        }
    }

    loading_instances.Deallocate(remove_loading_instance_indexes);
}

void AssetManager::ReleaseAssetHoldLoad(uint32_t hold_load_mask) {
    for (unsigned i = 0; i < asset_list_count; i++) {
        if (asset_list[i].hold_load_mask != 0x0 && (asset_list[i].hold_load_mask & ~hold_load_mask) == 0x0) {
            asset_hold_count--;
        }
        asset_list[i].hold_load_mask &= ~hold_load_mask;
    }
}

void AssetManager::SetLoadWarningMode(bool val, const char* mode_name, const char* level_name) {
    load_warning_activated = val;
    strscpy(load_warning_mode_name, mode_name, 32);
    strscpy(load_warning_level_name, level_name, 256);
}

void AssetManager::DumpLoadWarningData(const char* destination) {
    LOGI << "Dumping asset manager warning data to " << destination << std::endl;
    if (load_warning_instances.asset_warnings.size() > 0) {
        std::string path = AssemblePath(GetWritePath(CoreGameModID), destination);
        if (CheckFileAccess(path.c_str())) {
            load_warning_instances.Load(path);
        }
        load_warning_instances.Save(path);
        load_warning_instances.Clear();
    }
}

size_t AssetManager::GetLoadedAssetCount() {
    return asset_list_count;
}

int AssetManager::GetAssetTypeCount(AssetType asset) {
    return asset_type_count[(int)asset];
}

int AssetManager::GetAssetWarningCount() {
    return asset_warning_count;
}

int AssetManager::GetAssetHoldCount() {
    return asset_hold_count;
}

const char* AssetManager::GetAssetName(uint32_t index) {
    if (index >= 0 && index < asset_list_count) {
        return asset_list[index].rel_name;
    }
    return "";
}

AssetType AssetManager::GetAssetType(uint32_t index) {
    if (index >= 0 && index < asset_list_count) {
        return (AssetType)asset_list[index].type;
    }
    return (AssetType)0;
}

uint32_t AssetManager::GetAssetHoldFlags(uint32_t index) {
    if (index >= 0 && index < asset_list_count) {
        return asset_list[index].hold_load_mask;
    }
    return (uint32_t)0;
}

uint16_t AssetManager::GetAssetRefCount(uint32_t index) {
    if (index >= 0 && index < asset_list_count) {
        return asset_list[index].count;
    }
    return (uint16_t)0;
}

bool AssetManager::GetAssetLoadWarning(uint32_t index) {
    if (index >= 0 && index < asset_list_count) {
        return asset_list[index].load_warning;
    }
    return false;
}

#ifdef MEASURE_LOADS
void AssetManager::DumpTimingData() {
    std::set<AssetType> types;

    for (unsigned int i = 0; i < asset_load_times.size(); i++) {
        std::cout << asset_load_times[i].time << " " << asset_load_times[i].time_ms << "ms " << asset_load_times[i].asset_type_name << std::endl;
        types.insert(asset_load_times[i].asset_type);
    }

    std::cout << "Average data" << std::endl;

    std::set<AssetType>::iterator typeit = types.begin();
    for (; typeit != types.end(); typeit++) {
        int count = 0;
        uint64_t sum = 0;
        uint32_t sum_ms = 0;
        const char* name;

        for (unsigned int i = 0; i < asset_load_times.size(); i++) {
            if (asset_load_times[i].asset_type == *typeit) {
                sum += asset_load_times[i].time;
                sum_ms += asset_load_times[i].time_ms;
                count++;
                name = asset_load_times[i].asset_type_name;
            }
        }
        std::cout << sum / count << " " << (float)sum_ms / (float)count << "ms " << name << std::endl;
    }
}
#endif
