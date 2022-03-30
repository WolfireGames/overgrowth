//-----------------------------------------------------------------------------
//           Name: skeletonasset.h
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
#include <Math/mat4.h>

#include <Asset/assetbase.h>
#include <Asset/assetinfobase.h>
#include <Asset/assettypes.h>

#include <Internal/integer.h>
#include <Utility/flat_hash_map.hpp>

#include <vector>
#include <map>

class AssetManager;

enum {_hinge_joint = 0,
      _amotor_joint = 1,
      _fixed_joint = 2};

struct JointData {
    int type;
    int bone_id[2];
    float stop_angle[6];
    vec3 axis;
};

struct SimpleIKBone {
    int bone_id;
    int chain_length;
};

enum RiggingStage { _nothing = -1,
_create_bones = 0, 
_control_joints = 1,
_animate = 2,
_pose_weights = 3};

struct SkeletonFileData {
    std::vector<vec3> points; // Endpoints of bones
    std::vector<int> bone_ends; // Indices into points array (2 per bone)
    std::vector<int> hier_parents;  // The parent bone id of each bone (including connector bones)
    std::vector<int> point_parents; // The parent point id of each point
    std::vector<int> bone_parents; // Same as hier_parents? (why?)
    std::vector<float> bone_mass; // Mass of each bone
    std::vector<vec3> bone_com; // The center of mass of each bone
    std::vector<mat4> bone_mats; // The initial matrix for each bone
    std::vector<JointData> joints; // Physics info for each joint
    typedef ska::flat_hash_map<std::string, SimpleIKBone> IKBoneMap;
    IKBoneMap simple_ik_bones; // labeled IK bone chains
    std::vector<vec4> model_bone_weights; // For each vertex, the weight of the four attached bones
    std::vector<vec4> model_bone_ids; // For each vertex, the id of the four attached bones
    std::vector<int> symmetry;
    vec3 old_model_center;
    RiggingStage rigging_stage; // At which rigging stage was this skeleton saved (obsolete)
};

class SkeletonAsset : public Asset {
    SkeletonFileData data;
    unsigned short checksum_;
    std::string error_string;
public:
    SkeletonAsset(AssetManager* owner, uint32_t asset_id);
    ~SkeletonAsset();

    const SkeletonFileData& GetData();
    unsigned short checksum() {return checksum_;}

    int sub_error;
    int Load(const std::string &path, uint32_t load_flags);
    const char* GetLoadErrorString();
    const char* GetLoadErrorStringExtended();
    void Unload();

    void Reload( );
    virtual void ReportLoad();
    static AssetType GetType() { return SKELETON_ASSET; };
    static const char* GetTypeName() { return "SKELETON_ASSET"; }
    static bool AssetWarning() { return true; }

    virtual AssetLoaderBase* NewLoader();
};

typedef AssetRef<SkeletonAsset> SkeletonAssetRef;

