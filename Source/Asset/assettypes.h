//-----------------------------------------------------------------------------
//           Name: assettypes.h
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

enum AssetType {
    UNKNOWN,
    SKELETON_ASSET,
    SYNCED_ANIMATION_GROUP_ASSET,
    ANIMATION_ASSET,
    OBJECT_FILE_ASSET,
    MATERIAL_ASSET,
    ATTACK_ASSET,
    VOICE_FILE_ASSET,
    ANIMATION_EFFECT_ASSET,
    SOUND_GROUP_INFO_ASSET,
    CHARACTER_ASSET,
    REACTION_ASSET,
    DECAL_FILE_ASSET,
    HOTSPOT_FILE_ASSET,
    AMBIENT_SOUND_ASSET,
    ACTOR_FILE_ASSET,
    ITEM_ASSET,
    LEVEL_SET_ASSET,
    IMAGE_SAMPLER_ASSET,
    SOUND_GROUP_ASSET,
    PARTICLE_TYPE_ASSET,
    AVERAGE_COLOR_ASSET,
    FZX_ASSET,
    ATTACHMENT_ASSET,
    LIP_SYNC_FILE_ASSET,
    TEXTURE_DUMMY_ASSET,
    TEXTURE_ASSET,
    LEVEL_INFO_ASSET,
    FILE_HASH_ASSET,
    ASSET_TYPE_FINAL
};

const char* GetAssetTypeString( AssetType assettype );
AssetType GetAssetTypeValue( const char* assettype );
