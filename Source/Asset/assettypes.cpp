//-----------------------------------------------------------------------------
//           Name: assettypes.cpp
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
#include "assettypes.h"

#include <Utility/strings.h>

const char* GetAssetTypeString(AssetType assettype) {
    switch (assettype) {
        case UNKNOWN:
            return "UNKNOWN";
        case SKELETON_ASSET:
            return "SKELETON_ASSET";
        case SYNCED_ANIMATION_GROUP_ASSET:
            return "SYNCED_ANIMATION_GROUP_ASSET";
        case ANIMATION_ASSET:
            return "ANIMATION_ASSET";
        case OBJECT_FILE_ASSET:
            return "OBJECT_FILE_ASSET";
        case MATERIAL_ASSET:
            return "MATERIAL_ASSET";
        case ATTACK_ASSET:
            return "ATTACK_ASSET";
        case VOICE_FILE_ASSET:
            return "VOICE_FILE_ASSET";
        case ANIMATION_EFFECT_ASSET:
            return "ANIMATION_EFFECT_ASSET";
        case SOUND_GROUP_INFO_ASSET:
            return "SOUND_GROUP_INFO_ASSET";
        case CHARACTER_ASSET:
            return "CHARACTER_ASSET";
        case REACTION_ASSET:
            return "REACTION_ASSET";
        case DECAL_FILE_ASSET:
            return "DECAL_FILE_ASSET";
        case HOTSPOT_FILE_ASSET:
            return "HOTSPOT_FILE_ASSET";
        case AMBIENT_SOUND_ASSET:
            return "AMBIENT_SOUND_ASSET";
        case ACTOR_FILE_ASSET:
            return "ACTOR_FILE_ASSET";
        case ITEM_ASSET:
            return "ITEM_ASSET";
        case LEVEL_SET_ASSET:
            return "LEVEL_SET_ASSET";
        case IMAGE_SAMPLER_ASSET:
            return "IMAGE_SAMPLER_ASSET";
        case SOUND_GROUP_ASSET:
            return "SOUND_GROUP_ASSET";
        case PARTICLE_TYPE_ASSET:
            return "PARTICLE_TYPE_ASSET";
        case AVERAGE_COLOR_ASSET:
            return "AVERAGE_COLOR_ASSET";
        case FZX_ASSET:
            return "FZX_ASSET";
        case ATTACHMENT_ASSET:
            return "ATTACHMENT_ASSET";
        case LIP_SYNC_FILE_ASSET:
            return "LIP_SYNC_FILE_ASSET";
        case TEXTURE_DUMMY_ASSET:
            return "TEXTURE_DUMMY_ASSET";
        case TEXTURE_ASSET:
            return "TEXTURE_ASSET";
        case LEVEL_INFO_ASSET:
            return "LEVEL_INFO_ASSET";
        case FILE_HASH_ASSET:
            return "FILE_HASH_ASSET";
        case ASSET_TYPE_FINAL:
            return "ASSET_TYPE_FINAL";
    }
    return "UNKNOWN";
}

AssetType GetAssetTypeValue(const char* assettype) {
    if (strmtch("UNKNOWN", assettype))
        return UNKNOWN;
    if (strmtch("SKELETON_ASSET", assettype))
        return SKELETON_ASSET;
    if (strmtch("SYNCED_ANIMATION_GROUP_ASSET", assettype))
        return SYNCED_ANIMATION_GROUP_ASSET;
    if (strmtch("ANIMATION_ASSET", assettype))
        return ANIMATION_ASSET;
    if (strmtch("OBJECT_FILE_ASSET", assettype))
        return OBJECT_FILE_ASSET;
    if (strmtch("MATERIAL_ASSET", assettype))
        return MATERIAL_ASSET;
    if (strmtch("ATTACK_ASSET", assettype))
        return ATTACK_ASSET;
    if (strmtch("VOICE_FILE_ASSET", assettype))
        return VOICE_FILE_ASSET;
    if (strmtch("ANIMATION_EFFECT_ASSET", assettype))
        return ANIMATION_EFFECT_ASSET;
    if (strmtch("SOUND_GROUP_INFO_ASSET", assettype))
        return SOUND_GROUP_INFO_ASSET;
    if (strmtch("CHARACTER_ASSET", assettype))
        return CHARACTER_ASSET;
    if (strmtch("REACTION_ASSET", assettype))
        return REACTION_ASSET;
    if (strmtch("DECAL_FILE_ASSET", assettype))
        return DECAL_FILE_ASSET;
    if (strmtch("HOTSPOT_FILE_ASSET", assettype))
        return HOTSPOT_FILE_ASSET;
    if (strmtch("AMBIENT_SOUND_ASSET", assettype))
        return AMBIENT_SOUND_ASSET;
    if (strmtch("ACTOR_FILE_ASSET", assettype))
        return ACTOR_FILE_ASSET;
    if (strmtch("ITEM_ASSET", assettype))
        return ITEM_ASSET;
    if (strmtch("LEVEL_SET_ASSET", assettype))
        return LEVEL_SET_ASSET;
    if (strmtch("IMAGE_SAMPLER_ASSET", assettype))
        return IMAGE_SAMPLER_ASSET;
    if (strmtch("SOUND_GROUP_ASSET", assettype))
        return SOUND_GROUP_ASSET;
    if (strmtch("PARTICLE_TYPE_ASSET", assettype))
        return PARTICLE_TYPE_ASSET;
    if (strmtch("AVERAGE_COLOR_ASSET", assettype))
        return AVERAGE_COLOR_ASSET;
    if (strmtch("FZX_ASSET", assettype))
        return FZX_ASSET;
    if (strmtch("ATTACHMENT_ASSET", assettype))
        return ATTACHMENT_ASSET;
    if (strmtch("LIP_SYNC_FILE_ASSET", assettype))
        return LIP_SYNC_FILE_ASSET;
    if (strmtch("TEXTURE_DUMMY_ASSET", assettype))
        return TEXTURE_DUMMY_ASSET;
    if (strmtch("TEXTURE_ASSET", assettype))
        return TEXTURE_ASSET;
    if (strmtch("LEVEL_INFO_ASSET", assettype))
        return LEVEL_INFO_ASSET;
    if (strmtch("FILE_HASH_ASSET", assettype))
        return FILE_HASH_ASSET;
    return UNKNOWN;
}
