//-----------------------------------------------------------------------------
//           Name: animation.cpp
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
#include "animation.h"

#include <Asset/Asset/skeletonasset.h>
#include <Asset/AssetLoader/fallbackassetloader.h>
#include <Asset/Asset/syncedanimation.h>

#include <Compat/fileio.h>
#include <Compat/filepath.h>

#include <Internal/filesystem.h>
#include <Internal/memwrite.h>
#include <Internal/profiler.h>
#include <Internal/timer.h>
#include <Internal/error.h>
#include <Internal/checksum.h>

#include <Math/enginemath.h>
#include <Math/vec3math.h>
#include <Math/vec4math.h>

#include <Graphics/retargetfile.h>
#include <Graphics/models.h>

#include <Logging/logdata.h>
#include <Main/engine.h>
#include <Utility/assert.h>

#include <algorithm>
#include <cmath>
#include <cassert>
#include <sstream>

using std::endl;
using std::map;
using std::ostringstream;
using std::sort;
using std::string;
using std::swap;
using std::vector;

static const int _anm_cache_version = 17;

AnimationConfig animation_config;

Animation::Animation(AssetManager* owner, uint32_t asset_id) : AnimationAsset(owner, asset_id), sub_error(0) {
    clear();
}

struct DepthSorter {
    int depth;
    int bone_id;
};

class DepthSorterCompare {
   public:
    bool operator()(const DepthSorter& a, const DepthSorter& b) {
        return a.depth < b.depth;
    }
};

static void SwitchStringRightLeft(string& the_string) {
    size_t index = 0;
    while (1) {
        size_t left_index;
        size_t right_index;
        right_index = the_string.find("right", index);
        left_index = the_string.find("left", index);
        if (left_index == string::npos && right_index == string::npos) {
            break;
        }
        bool choose_left;
        if (left_index == string::npos) {
            choose_left = false;
        } else if (right_index == string::npos) {
            choose_left = true;
        } else {
            choose_left = left_index < right_index;
        }
        if (choose_left) {
            the_string.replace(left_index, 4, "right");
            index = left_index + 5;
        } else {
            the_string.replace(right_index, 5, "left");
            index = right_index + 4;
        }
    }
}

static void SwitchStringRL(string& the_string) {
    LOGD << "Old string: " << the_string << endl;
    /*for(unsigned i=0; i<10; ++i){
        ostringstream rfs;
        ostringstream lfs;
        rfs << "_r" << i;
        lfs << "_l" << i;
        string rfindstring = rfs.str();
        string lfindstring = lfs.str();
        if(the_string.find(rfindstring) != string::npos){
            the_string.replace(the_string.find(rfindstring), 3, lfindstring);
        } else if(the_string.find(lfindstring) != string::npos) {
            the_string.replace(the_string.find(lfindstring), 3, rfindstring);
        }
    }*/
    size_t end = the_string.size();
    if (the_string[end - 1] == 'l' && the_string[end - 2] == '_') {
        the_string[end - 1] = 'r';
    } else if (the_string[end - 1] == 'r' && the_string[end - 2] == '_') {
        the_string[end - 1] = 'l';
    }
    LOGD << "New string: " << the_string << endl;
}

void Animation::CalcInvertBoneMats() {
    size_t num_keyframes = keyframes.size();
    for (size_t i = 0; i < num_keyframes; i++) {
        Keyframe& k = keyframes[i];
        vector<BoneTransform>& bm = k.bone_mats;
        vector<BoneTransform>& ibm = k.invert_bone_mats;
        size_t num_bones = bm.size();
        ibm.resize(num_bones);
        for (int j = 0; j < num_bones; j++) {
            ibm[j] = invert(bm[j]);
        }
    }
}

void Animation::UpdateIKBones() {
    for (auto& k : keyframes) {
        // Initialize uses_ik vector
        size_t num_bones = k.bone_mats.size();
        k.uses_ik.resize(num_bones);
        for (size_t i = 0; i < num_bones; i++) {
            k.uses_ik[i] = false;
        }
        // Travel down each ik chain to specify that each bone uses ik
        size_t num_ik_bones = k.ik_bones.size();
        for (size_t i = 0; i < num_ik_bones; i++) {
            const BonePath& bone_path = k.ik_bones[i].bone_path;
            size_t bone_path_len = bone_path.size();
            for (int j = 0; j < bone_path_len; j++) {
                k.uses_ik[bone_path[j]] = true;
            }
        }
    }
}

void Animation::WriteToFile(FILE* file) {
    int version = _animation_version;
    fwrite(&version, sizeof(int), 1, file);
    bool centered = true;
    fwrite(&centered, sizeof(bool), 1, file);
    fwrite(&looping, sizeof(bool), 1, file);

    fwrite(&length, sizeof(int), 1, file);

    int num_keyframes = (int)keyframes.size();
    ;
    fwrite(&num_keyframes, sizeof(int), 1, file);

    for (auto& keyframe : keyframes) {
        fwrite(&keyframe.time, sizeof(int), 1, file);

        size_t num_weights = keyframe.weights.size();
        fwrite(&num_weights, sizeof(int), 1, file);
        if (num_weights) {
            fwrite(&keyframe.weights[0], sizeof(float), num_weights, file);
        }

        int num_bone_mats = (int)keyframe.bone_mats.size();
        fwrite(&num_bone_mats, sizeof(int), 1, file);
        mat4 temp;
        for (unsigned j = 0; j < keyframe.bone_mats.size(); j++) {
            temp = keyframe.bone_mats[j].GetMat4();
            fwrite(&temp.entries, sizeof(float), 16, file);
        }

        int num_weapon_mats = (int)keyframe.weapon_mats.size();
        fwrite(&num_weapon_mats, sizeof(int), 1, file);
        for (unsigned j = 0; j < keyframe.weapon_mats.size(); j++) {
            temp = keyframe.weapon_mats[j].GetMat4();
            fwrite(&temp.entries, sizeof(float), 16, file);
            fwrite(&keyframe.weapon_relative_id[j], sizeof(int), 1, file);
            fwrite(&keyframe.weapon_relative_weight[j], sizeof(float), 1, file);
        }

        fwrite(&keyframe.use_mobility, sizeof(bool), 1, file);
        if (keyframe.use_mobility) {
            mat4 temp_mat;
            temp_mat.SetColumn(0, keyframe.mobility_mat.GetColumn(0));
            temp_mat.SetColumn(1, keyframe.mobility_mat.GetColumn(2) * -1.0f);
            temp_mat.SetColumn(2, keyframe.mobility_mat.GetColumn(1));
            temp_mat.SetColumn(3, keyframe.mobility_mat.GetColumn(3));
            fwrite(&temp_mat.entries, sizeof(float), 16, file);
        }

        int num_events = (int)keyframe.events.size();
        fwrite(&num_events, sizeof(int), 1, file);

        for (size_t j = 0; j < keyframe.events.size(); j++) {
            fwrite(&keyframe.events[j].which_bone, sizeof(int), 1, file);
            const string& event_string = keyframe.events[j].event_string;
            size_t string_size = event_string.size();
            fwrite(&string_size, sizeof(int), 1, file);
            fwrite(event_string.c_str(), sizeof(char), string_size, file);
        }

        int num_ik_bones = (int)keyframe.ik_bones.size();
        fwrite(&num_ik_bones, sizeof(int), 1, file);

        for (size_t j = 0; j < keyframe.ik_bones.size(); j++) {
            // Used to be ik_bones.start and end
            vec3 filler;
            fwrite(&filler, sizeof(vec3), 1, file);
            fwrite(&filler, sizeof(vec3), 1, file);
            size_t path_length = keyframe.ik_bones[j].bone_path.size();
            fwrite(&path_length, sizeof(int), 1, file);
            fwrite(&keyframe.ik_bones[j].bone_path[0], sizeof(int), path_length, file);
            const string& label_string = keyframe.ik_bones[j].label;
            size_t string_size = label_string.size();
            fwrite(&string_size, sizeof(int), 1, file);
            fwrite(label_string.c_str(), sizeof(char), string_size, file);
        }

        int num_shape_keys = (int)keyframe.shape_keys.size();
        fwrite(&num_shape_keys, sizeof(int), 1, file);

        for (size_t j = 0; j < keyframe.shape_keys.size(); j++) {
            fwrite(&keyframe.shape_keys[j].weight, sizeof(float), 1, file);
            const string& label_string = keyframe.shape_keys[j].label;
            size_t string_size = label_string.size();
            fwrite(&string_size, sizeof(int), 1, file);
            fwrite(label_string.c_str(), sizeof(char), string_size, file);
        }

        int num_status_keys = (int)keyframe.status_keys.size();
        fwrite(&num_status_keys, sizeof(int), 1, file);

        for (size_t j = 0; j < keyframe.status_keys.size(); j++) {
            fwrite(&keyframe.status_keys[j].weight, sizeof(float), 1, file);
            const string& label_string = keyframe.status_keys[j].label;
            size_t string_size = label_string.size();
            fwrite(&string_size, sizeof(int), 1, file);
            fwrite(label_string.c_str(), sizeof(char), string_size, file);
        }

        if (centered) {
            fwrite(&keyframe.rotation, sizeof(float), 1, file);
            fwrite(&keyframe.center_offset, sizeof(float), 3, file);
        }
    }
}

void Animation::Center() {
    size_t num_keyframes = keyframes.size();
    // Find the center offset for each keyframe, and subtract it from
    // the translation of each bone
    for (size_t i = 0; i < num_keyframes; i++) {
        Keyframe& k = keyframes[i];
        vec3 center(0.0f);
        if (k.use_mobility) {
            // Get rotation and xz translation from the mobility bone
            center = k.mobility_mat.GetTranslationPart();
            center[0] *= -1.0f;
            center[2] *= -1.0f;

            vec3 dir = k.mobility_mat.GetRotatedvec3(vec3(0.0f, 0.0f, 1.0f));
            k.rotation = atan2f(dir[0], dir[2]);
            if (i != 0) {
                Keyframe& prev_key = keyframes[i - 1];
                if (k.rotation > prev_key.rotation + (float)PI) {
                    k.rotation -= (float)PI * 2.0f;
                } else if (k.rotation < prev_key.rotation - (float)PI) {
                    k.rotation += (float)PI * 2.0f;
                }
            }
        } else {
            // If not using mobility bone, determine center of mass
            float total_mass = 0.0f;
            for (auto& bone_mat : k.bone_mats) {
                center += bone_mat.origin;
                total_mass += 1.0f;
            }
            LOG_ASSERT(total_mass != 0.0f);
            center /= total_mass;
        }
        // Subtract the center from the position of each bone
        for (auto& bone_mat : k.bone_mats) {
            bone_mat.origin -= center;
        }
        for (auto& weapon_mat : k.weapon_mats) {
            weapon_mat.origin -= center;
        }
        k.center_offset = center;
    }
    // Get the center offset for the first keyframe
    vec3 base_offset;
    if (!keyframes[0].use_mobility) {
        base_offset = keyframes[0].center_offset;
    }
    // Add the first keyframe's offset to each frame
    vec3 temp_offset;
    for (int i = 0; i < num_keyframes; i++) {
        Keyframe& k = keyframes[i];
        temp_offset = base_offset;
        for (auto& bone_mat : k.bone_mats) {
            bone_mat.origin += temp_offset;
        }
        for (auto& weapon_mat : k.weapon_mats) {
            weapon_mat.origin += temp_offset;
        }
        k.center_offset -= temp_offset;
    }
    // Apply the inverse of each keyframe's rotation to each bone
    for (int i = 0; i < num_keyframes; i++) {
        Keyframe& k = keyframes[i];
        mat4 rotation_mat;
        rotation_mat.SetRotationY(-k.rotation);
        for (auto& bone_mat : k.bone_mats) {
            bone_mat = rotation_mat * bone_mat;
        }
        for (auto& weapon_mat : k.weapon_mats) {
            weapon_mat = rotation_mat * weapon_mat;
        }
    }
    // Apply the inverse rotation to center offset changes
    vec3 last_offset = keyframes[0].center_offset;
    for (int i = 1; i < num_keyframes; i++) {
        Keyframe& k = keyframes[i];
        mat4 rotation_mat;
        rotation_mat.SetRotationY(-k.rotation);
        vec3 vec = k.center_offset - last_offset;
        vec = rotation_mat * vec;
        vec3 new_co = keyframes[i - 1].center_offset + vec;
        last_offset = k.center_offset;
        k.center_offset = new_co;
    }
    RecalcCaches();
}

void Retarget(const AnimInput& anim_input, AnimOutput& anim_output, const string& old_path) {
    PROFILER_ZONE(g_profiler_ctx, "Retargeting");
    PROFILER_ENTER(g_profiler_ctx, "Get SkeletonAssetRefs");
    SkeletonAssetRef old_sar = Engine::Instance()->GetAssetManager()->LoadSync<SkeletonAsset>(old_path);
    SkeletonAssetRef new_sar = Engine::Instance()->GetAssetManager()->LoadSync<SkeletonAsset>(anim_input.retarget_new);
    PROFILER_LEAVE(g_profiler_ctx);
    PROFILER_ENTER(g_profiler_ctx, "Get SkeletonFileData");
    const SkeletonFileData& old_data = old_sar->GetData();
    const SkeletonFileData& new_data = new_sar->GetData();
    PROFILER_LEAVE(g_profiler_ctx);
    // Add bones that don't exist in original skeleton
    anim_output.matrices.resize(new_data.bone_mats.size());
    anim_output.physics_weights.resize(new_data.bone_mats.size());
    for (size_t i = old_data.bone_mats.size(), len = anim_output.matrices.size(); i < len; ++i) {
        anim_output.physics_weights[i] = 0.0f;
    }

    // Find new and old leg length
    float old_leg_length = 0.0f;
    {
        PROFILER_ZONE(g_profiler_ctx, "Find old leg length");
        SkeletonFileData::IKBoneMap::const_iterator iter = old_data.simple_ik_bones.find("left_leg");
        if (iter != old_data.simple_ik_bones.end()) {
            const SimpleIKBone& leg_bone = iter->second;
            int bone_id = leg_bone.bone_id;
            for (int i = 0; i < leg_bone.chain_length - 1; ++i) {
                old_leg_length += distance(old_data.points[old_data.bone_ends[bone_id * 2 + 0]],
                                           old_data.points[old_data.bone_ends[bone_id * 2 + 1]]);
                bone_id = old_data.hier_parents[bone_id];
            }
        } else {
            FatalError("Error", "\"left_leg\" not found in simple ik bones");
        }
    }

    float new_leg_length = 0.0f;
    {
        PROFILER_ZONE(g_profiler_ctx, "Find new leg length");
        SkeletonFileData::IKBoneMap::const_iterator iter = new_data.simple_ik_bones.find("left_leg");
        if (iter != new_data.simple_ik_bones.end()) {
            const SimpleIKBone& leg_bone = iter->second;
            int bone_id = leg_bone.bone_id;
            for (int i = 0; i < leg_bone.chain_length - 1; ++i) {
                new_leg_length += distance(new_data.points[new_data.bone_ends[bone_id * 2 + 0]],
                                           new_data.points[new_data.bone_ends[bone_id * 2 + 1]]);
                bone_id = new_data.hier_parents[bone_id];
            }
        } else {
            FatalError("Error", "\"left_leg\" not found in simple ik bones");
        }
    }

    float ratio = new_leg_length / old_leg_length;

    // Shift root bone based on hip point
    vec3 old_root_offset, new_root_offset;
    {
        SkeletonFileData::IKBoneMap::const_iterator iter = old_data.simple_ik_bones.find("torso");
        if (iter != old_data.simple_ik_bones.end()) {
            const SimpleIKBone& torso_ik_bone = iter->second;
            int hip_bone = old_data.bone_parents[old_data.bone_parents[torso_ik_bone.bone_id]];
            int point_id = old_data.bone_ends[hip_bone * 2 + 0];
            old_root_offset = old_data.points[point_id];
            old_root_offset += old_data.old_model_center;
        }
    }

    // Shift root bone based on leg length ratio
    for (size_t i = 0, len = new_data.bone_mats.size(); i < len; ++i) {
        if (new_data.hier_parents[i] == -1) {
            anim_output.matrices[i].origin += old_root_offset;
            anim_output.matrices[i].origin *= ratio;
        }
    }
    for (auto& bbp : anim_output.ik_bones) {
        bbp.transform.origin += old_root_offset;
        bbp.transform.origin *= ratio;
    }
    anim_output.center_offset *= ratio;

    // Calculate new bone lengths
    {
        PROFILER_ZONE(g_profiler_ctx, "Calculate new_bone_lengths");
        vector<float> new_bone_length(new_data.bone_mats.size());
        for (size_t i = 0, len = new_data.bone_mats.size(); i < len; ++i) {
            new_bone_length[i] =
                distance(new_data.points[new_data.bone_ends[i * 2 + 0]],
                         new_data.points[new_data.bone_ends[i * 2 + 1]]);
        }
    }

    // Calculate each bone's depth in hierarchy (root = 0)
    vector<DepthSorter> depth_sorter(new_data.bone_mats.size());
    {
        PROFILER_ZONE(g_profiler_ctx, "Calculate bone depths in hierarchy");
        for (size_t i = 0, len = new_data.bone_mats.size(); i < len; ++i) {
            int parent = new_data.hier_parents[i];
            int depth = 0;
            while (parent != -1) {
                ++depth;
                parent = new_data.hier_parents[parent];
            }
            depth_sorter[i].depth = depth;
            depth_sorter[i].bone_id = (int)i;
        }
    }

    // Create bone list sorted by depth
    sort(depth_sorter.begin(), depth_sorter.end(), DepthSorterCompare());

    {
        PROFILER_ZONE(g_profiler_ctx, "Calculate new bone positions using sorted bone list");
        for (auto& i : depth_sorter) {
            int bone_id = i.bone_id;
            int parent = new_data.hier_parents[bone_id];

            // Set rotation for new bones
            if (bone_id > (int)old_data.bone_mats.size()) {
                if (anim_input.parents->at(bone_id) < (int)old_data.bone_mats.size()) {
                    anim_output.matrices[bone_id].rotation = invert(QuaternionFromMat4(old_data.bone_mats[anim_input.parents->at(bone_id)])) * QuaternionFromMat4(new_data.bone_mats[bone_id]);
                } else {
                    anim_output.matrices[bone_id].rotation = invert(QuaternionFromMat4(new_data.bone_mats[anim_input.parents->at(bone_id)])) * QuaternionFromMat4(new_data.bone_mats[bone_id]);
                }
            }

            // Enforce bone length
            if (bone_id < (int)anim_input.parents->size() && anim_input.parents->at(bone_id) != -1) {
                vec3 parent_bone = invert(QuaternionFromMat4(new_data.bone_mats[anim_input.parents->at(bone_id)])) * ((new_data.points[new_data.bone_ends[anim_input.parents->at(bone_id) * 2 + 1]] - new_data.points[new_data.bone_ends[anim_input.parents->at(bone_id) * 2 + 0]]) * 0.5f +
                                                                                                                      (new_data.points[new_data.bone_ends[bone_id * 2 + 0]] - new_data.points[new_data.bone_ends[anim_input.parents->at(bone_id) * 2 + 1]]));
                vec3 child_bone = anim_output.matrices[bone_id].rotation * invert(QuaternionFromMat4(new_data.bone_mats[bone_id])) * (new_data.points[new_data.bone_ends[bone_id * 2 + 1]] - new_data.points[new_data.bone_ends[bone_id * 2 + 0]]);
                anim_output.matrices[bone_id].origin = parent_bone + child_bone * 0.5f;
            } else if (parent != -1) {
                vec3 parent_bone = invert(QuaternionFromMat4(new_data.bone_mats[parent])) * (new_data.points[new_data.bone_ends[parent * 2 + 1]] - new_data.points[new_data.bone_ends[parent * 2 + 0]]) * 0.5f;
                vec3 child_bone = anim_output.matrices[bone_id].rotation * invert(QuaternionFromMat4(new_data.bone_mats[bone_id])) * (new_data.points[new_data.bone_ends[bone_id * 2 + 1]] - new_data.points[new_data.bone_ends[bone_id * 2 + 0]]);
                anim_output.matrices[bone_id].origin = parent_bone + child_bone * 0.5f;
            }
        }
    }

    if (anim_input.mirrored) {
        PROFILER_ZONE(g_profiler_ctx, "Mirror animation");
        SkeletonAssetRef sar = Engine::Instance()->GetAssetManager()->LoadSync<SkeletonAsset>(AnimationRetargeter::Instance()->GetSkeletonFile(anim_input.retarget_new));
        // SkeletonAssets::Instance()->ReturnRef(AnimationRetargeter::Instance()->GetSkeletonFile(anim_input.retarget_new));

        const SkeletonFileData& skeleton_file_data = sar->GetData();
        const vector<int>& symmetry = skeleton_file_data.symmetry;

        // Mirror all rotations
        vector<BoneTransform>& matrices = anim_output.matrices;

        for (size_t i = 0, len = new_data.bone_mats.size(); i < len; ++i) {
            int bone_id = depth_sorter[i].bone_id;
            // int parent = new_data.hier_parents[bone_id];

            if (bone_id < (int)anim_input.parents->size() && anim_input.parents->at(bone_id) != -1) {
                matrices[bone_id] = matrices[anim_input.parents->at(bone_id)] * matrices[bone_id];
            }
        }

        for (size_t i = 0; i < skeleton_file_data.bone_mats.size(); ++i) {
            matrices[i] = matrices[i] * invert(skeleton_file_data.bone_mats[i]);
        }

        for (auto& matrix : matrices) {
            matrix.rotation[0] *= -1.0f;
            matrix.rotation[3] *= -1.0f;
            matrix.origin[0] *= -1.0f;
        }

        vector<BoneTransform>& weapon_matrices = anim_output.weapon_matrices;
        for (auto& weapon_matrix : weapon_matrices) {
            MirrorBT(weapon_matrix, true);
        }

        // Swap weapon relative ids (so knife in right hand is now in left hand)
        for (int& weapon_relative_id : anim_output.weapon_relative_ids) {
            if (weapon_relative_id != -1) {
                weapon_relative_id = symmetry[weapon_relative_id];
            }
        }
        for (unsigned j = 0; j < symmetry.size(); j++) {
            if (symmetry[j] > (int)j) {
                swap(matrices[j], matrices[symmetry[j]]);
            }
        }

        for (unsigned i = 0; i < skeleton_file_data.bone_mats.size(); ++i) {
            matrices[i] = matrices[i] * skeleton_file_data.bone_mats[i];
        }

        for (int i = (int)new_data.bone_mats.size() - 1; i >= 0; --i) {
            int bone_id = depth_sorter[i].bone_id;
            // int parent = new_data.hier_parents[bone_id];

            if (bone_id < (int)anim_input.parents->size() && anim_input.parents->at(bone_id) != -1) {
                matrices[bone_id] = invert(matrices[anim_input.parents->at(bone_id)]) * matrices[bone_id];
            }
        }

        for (unsigned j = 0; j < symmetry.size(); j++) {
            if (symmetry[j] > (int)j && symmetry[j] < (int)anim_output.physics_weights.size()) {
                swap(anim_output.physics_weights[j], anim_output.physics_weights[symmetry[j]]);
            }
        }

        anim_output.center_offset[0] *= -1.0f;
        anim_output.delta_rotation *= -1.0f;
        anim_output.rotation *= -1.0f;

        for (auto& skey : anim_output.status_keys) {
            SwitchStringRightLeft(skey.label);
        }
        for (auto& skey : anim_output.shape_keys) {
            SwitchStringRL(skey.label);
        }

        for (auto& ik_bone : anim_output.ik_bones) {
            string& label = ik_bone.ik_bone.label;
            SwitchStringRightLeft(label);
            BonePath& bone_path = ik_bone.ik_bone.bone_path;
            ik_bone.transform = ik_bone.transform * skeleton_file_data.bone_mats[bone_path[0]];
            ik_bone.transform.origin[0] *= -1.0f;
            ik_bone.transform.rotation[0] *= -1.0f;
            ik_bone.transform.rotation[3] *= -1.0f;
            for (int& k : bone_path) {
                if (k < (int)symmetry.size() && symmetry[k] != -1) {
                    k = symmetry[k];
                }
            }
            ik_bone.transform = ik_bone.transform * invert(skeleton_file_data.bone_mats[bone_path[0]]);
            if (label == "left_leg" || label == "right_leg") {
                ik_bone.transform.rotation = ik_bone.transform.rotation * quaternion(vec4(0.0f, 0.0f, 1.0f, 3.14f));
            }
        }
    }

    PROFILER_ZONE(g_profiler_ctx, "Check for flipped toe");
    {
        // Sometimes the left toe bone would be flipped upside down, especially on Josh/Akazi's models
        // This checks for that condition and flips it back
        for (int side = 0; side < 2; ++side) {
            const SimpleIKBone& ik_bone = new_data.simple_ik_bones.find(side ? "left_leg" : "right_leg")->second;
            vec3 test = QuaternionFromMat4(old_data.bone_mats[ik_bone.bone_id]) * invert(QuaternionFromMat4(new_data.bone_mats[ik_bone.bone_id])) * vec3(0.0f, 1.0f, 0.0f);
            if (test[1] < 0.0f) {
                anim_output.matrices[ik_bone.bone_id].rotation = anim_output.matrices[ik_bone.bone_id].rotation * quaternion(vec4(0.0f, 0.0f, 1.0f, 3.14f));
                for (auto& bbp : anim_output.ik_bones) {
                    if (bbp.ik_bone.label == "left_leg") {
                        bbp.transform.rotation = bbp.transform.rotation * quaternion(vec4(0.0f, 0.0f, 1.0f, 3.14f));
                    }
                }
            }
        }
    }

    if ((anim_input.mirrored) != (anim_input.retarget_new == old_path)) {
        // Fix twisted tail
        {
            SkeletonFileData::IKBoneMap::const_iterator iter = new_data.simple_ik_bones.find("tail");
            if (iter != new_data.simple_ik_bones.end()) {
                const SimpleIKBone& ik_bone = iter->second;
                int bone_id = ik_bone.bone_id;
                for (int i = 0; i < ik_bone.chain_length; ++i) {
                    anim_output.matrices[bone_id].rotation[0] *= -1.0f;
                    anim_output.matrices[bone_id].rotation[3] *= -1.0f;
                    bone_id = new_data.hier_parents[bone_id];
                }
            }
        }
    }
}

int Animation::ReadFromFile(FILE* file) {
    PROFILER_ZONE(g_profiler_ctx, "Animation::ReadFromFile");
    clear();

    int version;
    fread(&version, sizeof(int), 1, file);

    int start = 0;
    if (version > _animation_version || version < 0) {
        start = version;
        version = 0;
        return kLoadErrorIncompatibleFileVersion;
    }

    bool centered;

    if (version >= 10) {
        fread(&centered, sizeof(bool), 1, file);
    } else {
        centered = false;
    }

    if (version >= 1) {
        fread(&looping, sizeof(bool), 1, file);
    } else {
        looping = true;
    }

    if (version > 0 && version < 11) {
        fread(&start, sizeof(int), 1, file);
    }
    fread(&length, sizeof(int), 1, file);
    length -= start;

    int num_keyframes;
    fread(&num_keyframes, sizeof(int), 1, file);

    if (num_keyframes <= 0) {
        // FatalError("Error", "Animation has %d keyframes.", num_keyframes);
        sub_error = 1;
        return kLoadErrorCorruptFile;
    }

    for (int i = 0; i < num_keyframes; i++) {
        int time;
        fread(&time, sizeof(int), 1, file);
        time -= start;

        keyframes.resize(keyframes.size() + 1);
        keyframes.back().time = time;
        Keyframe* keyframe = &keyframes.back();

        keyframe->rotation = 0.0f;

        if (version >= 4) {
            int num_weights;
            fread(&num_weights, sizeof(int), 1, file);
            keyframe->weights.resize(num_weights);
            if (num_weights) {
                fread(&keyframe->weights[0], sizeof(float), num_weights, file);
            }
        }

        int num_bone_mats;
        fread(&num_bone_mats, sizeof(int), 1, file);
        keyframe->bone_mats.resize(num_bone_mats);

        mat4 temp;
        for (auto& bone_mat : keyframe->bone_mats) {
            fread(&temp.entries, sizeof(float), 16, file);
            bone_mat = temp;
        }

        if (version >= 7) {
            int num_weapon_mats;
            fread(&num_weapon_mats, sizeof(int), 1, file);
            keyframe->weapon_mats.resize(num_weapon_mats);
            keyframe->weapon_relative_id.resize(num_weapon_mats);
            keyframe->weapon_relative_weight.resize(num_weapon_mats);

            mat4 temp;
            for (unsigned j = 0; j < keyframe->weapon_mats.size(); j++) {
                fread(&temp.entries, sizeof(float), 16, file);
                keyframe->weapon_mats[j] = temp;
                if (version >= 8) {
                    fread(&keyframe->weapon_relative_id[j], sizeof(int), 1, file);
                    fread(&keyframe->weapon_relative_weight[j], sizeof(float), 1, file);
                    LOGD << "Relative ID and Weight: " << keyframe->weapon_relative_id[j] << " " << keyframe->weapon_relative_weight[j] << endl;
                } else {
                    keyframe->weapon_relative_id[j] = -1;
                    keyframe->weapon_relative_weight[j] = 0.0f;
                }
            }
        }

        if (version >= 9) {
            fread(&keyframe->use_mobility, sizeof(bool), 1, file);
            if (keyframe->use_mobility) {
                mat4 temp_mat;
                fread(&temp_mat.entries, sizeof(float), 16, file);
                keyframe->mobility_mat.SetColumn(0, temp_mat.GetColumn(0));
                keyframe->mobility_mat.SetColumn(1, temp_mat.GetColumn(2));
                keyframe->mobility_mat.SetColumn(2, temp_mat.GetColumn(1) * -1.0f);
                keyframe->mobility_mat.SetColumn(3, temp_mat.GetColumn(3));
            }
        } else {
            keyframe->use_mobility = false;
        }

        if (version >= 2) {
            int num_events;
            fread(&num_events, sizeof(int), 1, file);
            keyframe->events.resize(num_events);

            for (auto& event : keyframe->events) {
                fread(&event.which_bone, sizeof(int), 1, file);
                int string_size;
                fread(&string_size, sizeof(int), 1, file);
                vector<char> string_buffer(string_size + 1, '\0');
                fread(&string_buffer[0], sizeof(char), string_size, file);
                event.event_string = &string_buffer[0];
            }
        }

        if (version >= 3) {
            int num_ik_bones;
            fread(&num_ik_bones, sizeof(int), 1, file);
            keyframe->ik_bones.resize(num_ik_bones);

            for (auto& ik_bone : keyframe->ik_bones) {
                // used to be ik_bones start and end
                vec3 filler;
                fread(&filler, sizeof(vec3), 1, file);
                fread(&filler, sizeof(vec3), 1, file);
                int path_length;
                fread(&path_length, sizeof(int), 1, file);
                ik_bone.bone_path.resize(path_length);
                fread(&ik_bone.bone_path[0], sizeof(int), path_length, file);
                int string_size;
                fread(&string_size, sizeof(int), 1, file);
                vector<char> string_buffer(string_size + 1, '\0');
                fread(&string_buffer[0], sizeof(char), string_size, file);
                ik_bone.label = &string_buffer[0];
            }
        }

        if (version >= 5) {
            int num_shape_keys;
            fread(&num_shape_keys, sizeof(int), 1, file);
            keyframe->shape_keys.resize(num_shape_keys);

            for (auto& shape_key : keyframe->shape_keys) {
                fread(&shape_key.weight, sizeof(float), 1, file);
                int string_size;
                fread(&string_size, sizeof(int), 1, file);
                vector<char> string_buffer(string_size + 1, '\0');
                fread(&string_buffer[0], sizeof(char), string_size, file);
                shape_key.label = &string_buffer[0];
            }
        }
        if (version >= 6) {
            int num_status_keys;
            fread(&num_status_keys, sizeof(int), 1, file);
            keyframe->status_keys.resize(num_status_keys);

            for (auto& status_key : keyframe->status_keys) {
                fread(&status_key.weight, sizeof(float), 1, file);
                int string_size;
                fread(&string_size, sizeof(int), 1, file);
                vector<char> string_buffer(string_size + 1, '\0');
                fread(&string_buffer[0], sizeof(char), string_size, file);
                status_key.label = &string_buffer[0];
            }
        }
        if (centered) {
            fread(&keyframe->rotation, sizeof(float), 1, file);
            fread(&keyframe->center_offset, sizeof(float), 3, file);
        }
    }

    if (!centered) {
        Center();
    } else {
        RecalcCaches();
    }

    return kLoadOk;
}

float Animation::LocalFromNormalized(float normalized_time) const {
    return length * normalized_time;
}

void GetCubicSplineWeights(float interp, float* weights) {
    float interp_squared = interp * interp;
    float interp_cubed = interp_squared * interp;
    weights[0] = 0.5f * (-interp_cubed + 2.0f * interp_squared - interp);
    weights[1] = 0.5f * (3.0f * interp_cubed - 5.0f * interp_squared + 2.0f);
    weights[2] = 0.5f * (-3.0f * interp_cubed + 4.0f * interp_squared + interp);
    weights[3] = 0.5f * (interp_cubed - interp_squared);
}

BoneTransform BlendFourBones(BoneTransform* transforms, float* weights) {
    BoneTransform transform;
    float total_weight = weights[0] + weights[1];
    if (total_weight > 0.0f) {
        transform = mix(transforms[0], transforms[1], weights[1] / total_weight);
    }
    total_weight += weights[2];
    if (total_weight > 0.0f) {
        transform = mix(transform, transforms[2], weights[2] / total_weight);
    }
    total_weight += weights[3];
    if (total_weight > 0.0f) {
        transform = mix(transform, transforms[3], weights[3] / total_weight);
    }
    return transform;
}

AnimOutput mix(const AnimOutput& a, const AnimOutput& b, float alpha) {
    LOG_ASSERT(a.matrices.size() == b.matrices.size());

    AnimOutput result;
    size_t num_bones = a.matrices.size();
    result.matrices.resize(num_bones);
    for (unsigned i = 0; i < num_bones; i++) {
        result.matrices[i] = mix(a.matrices[i], b.matrices[i], alpha);
    }
    result.center_offset = mix(a.center_offset, b.center_offset, alpha);
    result.rotation = mix(a.rotation, b.rotation, alpha);
    result.delta_offset = mix(a.delta_offset, b.delta_offset, alpha);
    result.delta_rotation = mix(a.delta_rotation, b.delta_rotation, alpha);

    size_t num_weapon_bones = max(a.weapon_matrices.size(),
                                  b.weapon_matrices.size());
    result.weapon_matrices.resize(num_weapon_bones);
    result.weapon_weights.resize(num_weapon_bones);
    result.weapon_weight_weights.resize(num_weapon_bones);
    result.weapon_relative_ids.resize(num_weapon_bones);
    result.weapon_relative_weights.resize(num_weapon_bones);
    for (size_t i = 0; i < num_weapon_bones; i++) {
        if (i >= b.weapon_matrices.size()) {
            result.weapon_matrices[i] = a.weapon_matrices[i];
            result.weapon_weights[i] = a.weapon_weights[i] * (1.0f - alpha);
            result.weapon_weight_weights[i] = a.weapon_weight_weights[i] * (1.0f - alpha);
            result.weapon_relative_ids[i] = a.weapon_relative_ids[i];
            result.weapon_relative_weights[i] = a.weapon_relative_weights[i];
        } else if (i >= a.weapon_matrices.size()) {
            result.weapon_matrices[i] = b.weapon_matrices[i];
            result.weapon_weights[i] = b.weapon_weights[i] * alpha;
            result.weapon_weight_weights[i] = b.weapon_weight_weights[i] * alpha;
            result.weapon_relative_ids[i] = b.weapon_relative_ids[i];
            result.weapon_relative_weights[i] = b.weapon_relative_weights[i];
        } else {
            result.weapon_matrices[i] =
                mix(a.weapon_matrices[i], b.weapon_matrices[i], alpha);
            result.weapon_weights[i] =
                mix(a.weapon_weights[i], b.weapon_weights[i], alpha);
            result.weapon_weight_weights[i] =
                mix(a.weapon_weight_weights[i], b.weapon_weight_weights[i], alpha);
            result.weapon_relative_ids[i] = max(a.weapon_relative_ids[i], b.weapon_relative_ids[i]);
            result.weapon_relative_weights[i] = a.weapon_relative_weights[i];
        }
    }

    // Add ik bone paths from both sources, and adjust their weights
    {
        map<string, BlendedBonePath> ik_bones;
        map<string, BlendedBonePath>::iterator iter;

        for (const auto& ik_bone : a.ik_bones) {
            const string& label = ik_bone.ik_bone.label;
            ik_bones[label] = ik_bone;
            ik_bones[label].weight *= 1.0f - alpha;
        }

        for (const auto& ik_bone : b.ik_bones) {
            const string& label = ik_bone.ik_bone.label;
            iter = ik_bones.find(label);
            if (iter == ik_bones.end()) {
                ik_bones[label] = ik_bone;
                ik_bones[label].weight *= alpha;
            } else {
                ik_bones[label].weight = ik_bones[label].weight + ik_bone.weight * alpha;
                ik_bones[label].transform = mix(ik_bones[label].transform, ik_bone.transform, alpha);
            }
        }

        result.ik_bones.resize(ik_bones.size());
        iter = ik_bones.begin();
        int index = 0;
        for (; iter != ik_bones.end(); ++iter) {
            result.ik_bones[index++] = (*iter).second;
        }
    }

    size_t num_joints = max(a.physics_weights.size(), b.physics_weights.size());
    size_t min_num_joints = min(a.physics_weights.size(), b.physics_weights.size());
    result.physics_weights.resize(num_joints, 1.0f);

    if (a.physics_weights.empty()) {
        for (unsigned i = 0; i < num_joints; i++) {
            result.physics_weights[i] = mix(1.0f,
                                            b.physics_weights[i],
                                            alpha);
        }
    } else if (b.physics_weights.empty()) {
        for (unsigned i = 0; i < num_joints; i++) {
            result.physics_weights[i] = mix(a.physics_weights[i],
                                            1.0f,
                                            alpha);
        }
    } else {
        for (unsigned i = 0; i < min_num_joints; i++) {
            result.physics_weights[i] = mix(a.physics_weights[i],
                                            b.physics_weights[i],
                                            alpha);
        }
    }

    // Add shape keys from both sources, and adjust their weights
    {
        map<string, ShapeKeyBlend> shape_keys;
        map<string, ShapeKeyBlend>::iterator iter;

        // Add shape keys from A
        for (const auto& shape_key : a.shape_keys) {
            const string& label = shape_key.label;
            shape_keys[label] = shape_key;
            shape_keys[label].weight_weight *= 1.0f - alpha;
        }

        // Merge shape keys from B
        for (const auto& shape_key : b.shape_keys) {
            const string& label = shape_key.label;
            iter = shape_keys.find(label);
            if (iter == shape_keys.end()) {
                shape_keys[label] = shape_key;
                shape_keys[label].weight_weight *= alpha;
            } else {
                shape_keys[label].weight = mix(shape_keys[label].weight, shape_key.weight, alpha);
                shape_keys[label].weight_weight = shape_keys[label].weight_weight + shape_key.weight_weight * alpha;
            }
        }

        // Add shape keys from merging map into result array
        result.shape_keys.resize(shape_keys.size());
        iter = shape_keys.begin();
        int index = 0;
        for (; iter != shape_keys.end(); ++iter) {
            result.shape_keys[index++] = (*iter).second;
        }
    }

    // Add status keys from both sources, and adjust their weights
    {
        map<string, StatusKeyBlend> status_keys;
        map<string, StatusKeyBlend>::iterator iter;

        for (const auto& status_key : a.status_keys) {
            const string& label = status_key.label;
            status_keys[label] = status_key;
            status_keys[label].weight_weight *= 1.0f - alpha;
        }

        for (const auto& status_key : b.status_keys) {
            const string& label = status_key.label;
            iter = status_keys.find(label);
            if (iter == status_keys.end()) {
                status_keys[label] = status_key;
                status_keys[label].weight_weight *= alpha;
            } else {
                status_keys[label].weight = mix(status_keys[label].weight, status_key.weight, alpha);
                status_keys[label].weight_weight = status_keys[label].weight_weight + status_key.weight_weight * alpha;
            }
        }

        result.status_keys.resize(status_keys.size());
        iter = status_keys.begin();
        int index = 0;
        for (; iter != status_keys.end(); ++iter) {
            result.status_keys[index++] = (*iter).second;
        }

        if (result.status_keys.empty() && (!a.status_keys.empty() || !b.status_keys.empty())) {
            LOGD << "Lost status keys." << endl;
        }
    }

    map<int, WeapAnimInfo>::const_iterator iter_a = a.weap_anim_info_map.begin();
    map<int, WeapAnimInfo>::const_iterator iter_b = b.weap_anim_info_map.begin();

    while (iter_a != a.weap_anim_info_map.end() ||
           iter_b != b.weap_anim_info_map.end()) {
        int iter_a_id = -1;
        int iter_b_id = -1;
        if (iter_a != a.weap_anim_info_map.end()) {
            iter_a_id = iter_a->first;
        }
        if (iter_b != b.weap_anim_info_map.end()) {
            iter_b_id = iter_b->first;
        }
        if (iter_b_id != -1 && (iter_a_id == -1 || iter_a_id > iter_b_id)) {
            result.weap_anim_info_map[iter_b_id] = iter_b->second;
            result.weap_anim_info_map[iter_b_id].weight *= alpha;
            ++iter_b;
        } else if (iter_b_id == -1 || iter_b_id > iter_a_id) {
            result.weap_anim_info_map[iter_a_id] = iter_a->second;
            result.weap_anim_info_map[iter_a_id].weight *= (1.0f - alpha);
            ++iter_a;
        } else {
            result.weap_anim_info_map[iter_a_id] = mix(iter_a->second, iter_b->second, alpha);
            ++iter_a;
            ++iter_b;
        }
    }

    return result;
}

WeapAnimInfo mix(const WeapAnimInfo& a, const WeapAnimInfo& b, float b_weight) {
    WeapAnimInfo result;
    result.bone_transform = mix(a.bone_transform, b.bone_transform, b_weight);
    result.weight = mix(a.weight, b.weight, b_weight);
    result.relative_id = max(a.relative_id, b.relative_id);
    result.relative_weight = a.relative_weight;
    return result;
}

AnimOutput add_mix(const AnimOutput& a, const AnimOutput& b, float alpha) {
    LOG_ASSERT(a.matrices.size() == b.matrices.size());

    AnimOutput result = a;
    size_t num_bones = a.matrices.size();

    vector<float> bone_weights = b.physics_weights;
    bone_weights.resize(num_bones, 1.0f);

    float clamped_alpha = clamp(alpha, 0.0f, 1.0f);

    result.matrices.resize(num_bones);
    for (size_t i = 0; i < num_bones; i++) {
        result.matrices[i] = mix(a.matrices[i], b.matrices[i], alpha * (bone_weights[i]));
    }

    // Add shape keys from both sources, and adjust their weights
    {
        map<string, ShapeKeyBlend> shape_keys;
        map<string, ShapeKeyBlend>::iterator iter;

        // Add shape keys from A
        for (const auto& shape_key : a.shape_keys) {
            const string& label = shape_key.label;
            shape_keys[label] = shape_key;
        }

        // Merge shape keys from B
        for (const auto& shape_key : b.shape_keys) {
            const string& label = shape_key.label;
            iter = shape_keys.find(label);
            if (iter == shape_keys.end()) {
                shape_keys[label] = shape_key;
                shape_keys[label].weight_weight *= clamped_alpha;
            } else {
                ShapeKeyBlend& a_key = shape_keys[label];
                const ShapeKeyBlend& b_key = shape_key;
                a_key.weight = mix(a_key.weight, b_key.weight, b_key.weight_weight * clamped_alpha);
                a_key.weight_weight = max(a_key.weight_weight, b_key.weight_weight * clamped_alpha);
            }
        }

        // Add shape keys from merging map into result array
        result.shape_keys.resize(shape_keys.size());
        iter = shape_keys.begin();
        int index = 0;
        for (; iter != shape_keys.end(); ++iter) {
            result.shape_keys[index++] = (*iter).second;
        }
    }

    // Add status keys from both sources, and adjust their weights
    {
        map<string, StatusKeyBlend> status_keys;
        map<string, StatusKeyBlend>::iterator iter;

        for (const auto& status_key : a.status_keys) {
            const string& label = status_key.label;
            status_keys[label] = status_key;
        }

        for (const auto& status_key : b.status_keys) {
            const string& label = status_key.label;
            iter = status_keys.find(label);
            if (iter == status_keys.end()) {
                status_keys[label] = status_key;
                status_keys[label].weight_weight *= clamped_alpha;
            } else {
                StatusKeyBlend& a_key = status_keys[label];
                const StatusKeyBlend& b_key = status_key;
                a_key.weight = mix(a_key.weight, b_key.weight, b_key.weight_weight * clamped_alpha);
                a_key.weight_weight = max(a_key.weight_weight, b_key.weight_weight * clamped_alpha);
            }
        }

        result.status_keys.resize(status_keys.size());
        iter = status_keys.begin();
        int index = 0;
        for (; iter != status_keys.end(); ++iter) {
            result.status_keys[index++] = (*iter).second;
        }
    }

    size_t num_weapon_bones = max(a.weapon_matrices.size(),
                                  b.weapon_matrices.size());
    result.weapon_matrices.resize(num_weapon_bones);
    result.weapon_weights.resize(num_weapon_bones);
    result.weapon_weight_weights.resize(num_weapon_bones);
    result.weapon_relative_ids.resize(num_weapon_bones);
    result.weapon_relative_weights.resize(num_weapon_bones);
    for (unsigned i = 0; i < num_weapon_bones; i++) {
        if (i >= b.weapon_matrices.size()) {
            result.weapon_matrices[i] = a.weapon_matrices[i];
            result.weapon_weights[i] = a.weapon_weights[i];
            result.weapon_weight_weights[i] = a.weapon_weight_weights[i];
            result.weapon_relative_ids[i] = a.weapon_relative_ids[i];
            result.weapon_relative_weights[i] = a.weapon_relative_weights[i];
        } else if (i >= a.weapon_matrices.size()) {
            result.weapon_matrices[i] = b.weapon_matrices[i];
            result.weapon_weights[i] = b.weapon_weights[i] * clamped_alpha;
            result.weapon_weight_weights[i] = b.weapon_weight_weights[i] * clamped_alpha;
            result.weapon_relative_ids[i] = b.weapon_relative_ids[i];
            result.weapon_relative_weights[i] = b.weapon_relative_weights[i];
        } else {
            result.weapon_matrices[i] =
                mix(a.weapon_matrices[i], b.weapon_matrices[i], alpha * b.weapon_weight_weights[i]);
            result.weapon_weights[i] =
                mix(a.weapon_weights[i], b.weapon_weights[i], clamped_alpha * b.weapon_weight_weights[i]);
            result.weapon_weight_weights[i] = max(a.weapon_weight_weights[i], b.weapon_weight_weights[i] * clamped_alpha);
            result.weapon_relative_ids[i] = max(a.weapon_relative_ids[i], b.weapon_relative_ids[i]);
            result.weapon_relative_weights[i] = a.weapon_relative_weights[i];
        }
    }

    size_t num_joints = max(a.physics_weights.size(),
                            b.physics_weights.size());
    size_t min_num_joints = min(a.physics_weights.size(),
                                b.physics_weights.size());
    result.physics_weights.resize(num_joints, 1.0f);

    if (a.physics_weights.empty()) {
        for (size_t i = 0; i < num_joints; i++) {
            result.physics_weights[i] = mix(0.0f,
                                            b.physics_weights[i],
                                            clamped_alpha);
        }
    } else if (b.physics_weights.empty()) {
        for (size_t i = 0; i < num_joints; i++) {
            result.physics_weights[i] = mix(a.physics_weights[i],
                                            0.0f,
                                            clamped_alpha);
        }
    } else {
        for (size_t i = 0; i < min_num_joints; i++) {
            result.physics_weights[i] = max(a.physics_weights[i],
                                            b.physics_weights[i] * clamped_alpha);
        }
    }

    map<int, WeapAnimInfo>::const_iterator iter_a = a.weap_anim_info_map.begin();
    map<int, WeapAnimInfo>::const_iterator iter_b = b.weap_anim_info_map.begin();

    while (iter_a != a.weap_anim_info_map.end() ||
           iter_b != b.weap_anim_info_map.end()) {
        int iter_a_id = -1;
        int iter_b_id = -1;
        if (iter_a != a.weap_anim_info_map.end()) {
            iter_a_id = iter_a->first;
        }
        if (iter_b != b.weap_anim_info_map.end()) {
            iter_b_id = iter_b->first;
        }
        if (iter_b_id != -1 && (iter_a_id == -1 || iter_a_id > iter_b_id)) {
            result.weap_anim_info_map[iter_b_id] = iter_b->second;
            result.weap_anim_info_map[iter_b_id].weight *= clamped_alpha;
            ++iter_b;
        } else if (iter_b_id == -1 || iter_b_id > iter_a_id) {
            result.weap_anim_info_map[iter_a_id] = iter_a->second;
            ++iter_a;
        } else {
            result.weap_anim_info_map[iter_a_id] = mix(iter_a->second, iter_b->second, clamped_alpha);
            ++iter_a;
            ++iter_b;
        }
    }

    return result;
}

/*
void Animation::SaveCache(unsigned short checksum) {
    FILE *file = my_fopen((GetWritePath(CoreGameModID)+path_+".anmcache2").c_str(), "wb");
    fwrite(&checksum, sizeof(unsigned short), 1, file);
    fwrite(&_anm_cache_version, sizeof(int), 1, file);
    WriteToFile(file);
    fclose(file);
}
bool Animation::LoadCache(unsigned short checksum) {

    PROFILER_ZONE(g_profiler_ctx, "Loading cache");
    FILE *file = my_fopen((GetWritePath(CoreGameModID)+path_+".anmcache2").c_str(), "rb");
    if(!file){
        return false;
    }
    //
    //fclose(file);
    //return false;
    //
    unsigned short file_checksum;
    fread(&file_checksum, sizeof(unsigned short), 1, file);
    if(file_checksum != checksum){
        fclose(file);
        return false;
    }
    int cache_version;
    fread(&cache_version, sizeof(int), 1, file);
    if(cache_version != _anm_cache_version){
        fclose(file);
        return false;
    }
    ReadFromFile(file);
    fclose(file);
    return true;
}
*/

void Animation::GetMatrices(float normalized_time, AnimOutput& anim_output, const AnimInput& anim_input) const {
    PROFILER_ZONE(g_profiler_ctx, "Animation::GetMatrices");
    float local_time = LocalFromNormalized(normalized_time);

    anim_output.weap_anim_info_map.clear();

    // Get the id of the previous frame
    int frame_id = -1;
    int num_keyframes = (int)keyframes.size();
    if (num_keyframes > 0) {
        for (int i = 0; i < num_keyframes; i++) {
            if (keyframes[i].time <= local_time) {
                frame_id = i;
            }
        }

        // Get the 4 frames for interpolation:
        // two frames ahead of the current time and two frames behind
        const Keyframe* frames[4];
        if (looping) {
            for (int i = 0; i < 4; ++i) {
                frames[i] = &keyframes[(frame_id + num_keyframes + i - 1) % num_keyframes];
            }
        } else {
            int last_frame = num_keyframes - 1;
            for (int i = 0; i < 4; ++i) {
                frames[i] = &keyframes[min(last_frame, max(0, frame_id + i - 1))];
            }
        }

        // Get bilinear time weight between the last frame and the next frame
        float interp = 0.0f;
        if (frames[1] != frames[2]) {
            float frame_time = (float)frames[1]->time;
            if (frame_time > local_time) {
                frame_time -= length;
            }
            float next_frame_time = (float)frames[2]->time;
            if (next_frame_time < local_time) {
                next_frame_time += length;
            }
            interp = (local_time - frame_time) / (float)(next_frame_time - frame_time);
        }
        interp = max(0.0f, min(1.0f, interp));

        // Get all four cubic spline weights from bilinear time weight
        float weights[4];
        GetCubicSplineWeights(interp, weights);
        const bool kForceBilinear = false;
        if (kForceBilinear) {
            weights[0] = 0.0f;
            weights[1] = 1.0f - interp;
            weights[2] = interp;
            weights[3] = 0.0f;
        }
        if (animation_config.kDisableInterpolation) {
            weights[0] = 0.0f;
            if (interp < 0.5f) {
                weights[1] = 1.0f;
                weights[2] = 0.0f;
                interp = 0.0f;
            } else {
                weights[1] = 0.0f;
                weights[2] = 1.0f;
                interp = 1.0f;
            }
            weights[3] = 0.0f;
        }

        // Get per-bone info
        size_t num_matrices = keyframes[0].bone_mats.size();
        anim_output.matrices.resize(num_matrices);
        {
            PROFILER_ZONE(g_profiler_ctx, "Getting bone transforms");
            for (uint32_t i = 0; i < num_matrices; i++) {
                // Get bone transforms (linear or angular)
                if (anim_input.parents && i < anim_input.parents->size() && anim_input.parents->at(i) != -1) {
                    BoneTransform bone_transforms[4];
                    for (int j = 0; j < 4; ++j) {
                        bone_transforms[j] = frames[j]->invert_bone_mats[anim_input.parents->at(i)] * frames[j]->bone_mats[i];
                    }
                    BoneTransform transform = BlendFourBones(bone_transforms, weights);
                    anim_output.matrices[i] = transform;
                } else {
                    if (anim_input.parents && i >= anim_input.parents->size())
                        LOGW_ONCE("Incompatible animation");

                    BoneTransform bone_transforms[4];
                    for (int j = 0; j < 4; ++j) {
                        bone_transforms[j] = frames[j]->bone_mats[i];
                    }
                    BoneTransform transform = BlendFourBones(bone_transforms, weights);
                    anim_output.matrices[i] = transform;
                }
            }
        }

        // Get per-weapon info
        size_t num_weapon_matrices = keyframes[0].weapon_mats.size();
        anim_output.weapon_weights.resize(num_weapon_matrices);
        anim_output.weapon_weight_weights.resize(num_weapon_matrices);
        for (int i = 0; i < num_weapon_matrices; ++i) {
            anim_output.weapon_weights[i] = 1.0f;
            anim_output.weapon_weight_weights[i] = 1.0f;
        }
        anim_output.weapon_relative_ids = keyframes[0].weapon_relative_id;
        anim_output.weapon_relative_weights = keyframes[0].weapon_relative_weight;
        anim_output.weapon_matrices.resize(num_weapon_matrices);
        for (size_t i = 0; i < num_weapon_matrices; i++) {
            BoneTransform bone_transforms[4];
            for (int j = 0; j < 4; ++j) {
                bone_transforms[j] = frames[j]->weapon_mats[i];
                if (frames[j]->weapon_relative_weight[i] > 0.0f) {
                    bone_transforms[j] = invert(frames[j]->bone_mats[frames[j]->weapon_relative_id[i]]) * bone_transforms[j];
                }
            }
            BoneTransform transform = BlendFourBones(bone_transforms, weights);
            anim_output.weapon_matrices[i] = transform;
        }

        // Get physics weights
        size_t num_weights = keyframes[0].weights.size();
        anim_output.physics_weights.resize(num_weights);
        for (size_t i = 0; i < num_weights; i++) {
            anim_output.physics_weights[i] = 0.0f;
            for (int j = 0; j < 4; ++j) {
                anim_output.physics_weights[i] += frames[j]->weights[i] * weights[j];
            }
        }

        anim_output.center_offset = frames[1]->center_offset * (1.0f - interp) + frames[2]->center_offset * interp;
        anim_output.rotation = frames[1]->rotation * (1.0f - interp) + frames[2]->rotation * interp;

        typedef map<string, BlendedBonePath> IKBoneMap;
        IKBoneMap ik_bone_weights;
        for (int j = 0; j < 4; ++j) {
            for (unsigned i = 0; i < frames[j]->ik_bones.size(); ++i) {
                BlendedBonePath& weighted_ik_bone = ik_bone_weights[frames[j]->ik_bones[i].label];
                weighted_ik_bone.weight += weights[j];
                weighted_ik_bone.ik_bone = frames[j]->ik_bones[i];
            }
        }

        for (auto& ik_bone_weight : ik_bone_weights) {
            BlendedBonePath& blended_bone_path = ik_bone_weight.second;
            int bone = blended_bone_path.ik_bone.bone_path.back();
            BoneTransform bone_transforms[4];
            for (int j = 0; j < 4; ++j) {
                bone_transforms[j] = frames[j]->bone_mats[bone];
            }
            BoneTransform transform = mix(bone_transforms[1], bone_transforms[2], interp);  // BlendFourBones(bone_transforms, weights);
            blended_bone_path.transform = transform;
        }

        anim_output.ik_bones.resize(ik_bone_weights.size());
        int index = 0;
        for (auto& ik_bone_weight : ik_bone_weights) {
            anim_output.ik_bones[index] = ik_bone_weight.second;
            ++index;
        }

        anim_output.shape_keys.resize(frames[0]->shape_keys.size());
        for (unsigned i = 0; i < anim_output.shape_keys.size(); i++) {
            anim_output.shape_keys[i].weight = 0.0f;
            anim_output.shape_keys[i].weight_weight = 1.0f;
            for (int j = 0; j < 4; ++j) {
                anim_output.shape_keys[i].weight += frames[j]->shape_keys[i].weight * weights[j];
            }
            anim_output.shape_keys[i].label = frames[0]->shape_keys[i].label;
        }

        anim_output.status_keys.resize(frames[1]->status_keys.size());
        for (unsigned i = 0; i < anim_output.status_keys.size(); i++) {
            anim_output.status_keys[i].weight = frames[1]->status_keys[i].weight * (1.0f - interp) +
                                                frames[2]->status_keys[i].weight * interp;
            anim_output.status_keys[i].weight_weight = 1.0f;
            anim_output.status_keys[i].label = frames[1]->status_keys[i].label;
        }

        anim_output.old_path = AnimationRetargeter::Instance()->GetSkeletonFile(path_);
        // Retarget if needed
        /*if(!anim_input.retarget_new.empty())
        {
            Retarget(anim_input, anim_output, anim_output.old_path);
            anim_output.old_path = "retargeted";
        }*/
    } else {
        LOGE << "There are no keyframes in this animation" << endl;
    }
}

int Animation::Load(const string& rel_path, uint32_t load_flags) {
    sub_error = 0;
    char abs_path[kPathSize];
    ModID modsource;
    if (FindFilePath(rel_path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths, true, NULL, &modsource) == -1) {
        return kLoadErrorMissingFile;
    }
    FILE* file = my_fopen(abs_path, "rb");
    if (file == NULL) {
        return kLoadErrorCouldNotOpen;
    }

    LOGD << "Loading animation " << rel_path << endl;
    modsource_ = modsource;
    int error = ReadFromFile(file);
    fclose(file);
    return error;
}

void Animation::Unload() {
}

const char* Animation::GetLoadErrorString() {
    switch (sub_error) {
        case 0:
            return "";
        case 1:
            return "Animation data has a negative number of keyframes.";
        default:
            return "Undefined error";
    }
}

void Animation::clear() {
    looping = false;
    keyframes.clear();
}

float Animation::GetFrequency(const BlendMap& blendmap) const {
    float frequency = 1.0f / GetPeriod(blendmap);
    return frequency;
}

float Animation::GetPeriod(const BlendMap& blendmap) const {
    float period = length / 1000.0f;
    return period;
}

float Animation::NormalizedFromLocal(float local_time) const {
    if (length == 0) {
        return 0.0f;
    }
    return local_time / length;
}

bool Animation::IsLooping() const {
    return looping;
}

float Animation::GetGroundSpeed(const BlendMap& blendmap) const {
    return 1.0f;
}

vector<NormalizedAnimationEvent> Animation::GetEvents(int& anim_id, bool mirror) const {
    vector<NormalizedAnimationEvent> normalized_events;
    for (const auto& keyframe : keyframes) {
        for (unsigned j = 0; j < keyframe.events.size(); j++) {
            const AnimationEvent& the_event = keyframe.events[j];

            normalized_events.resize(normalized_events.size() + 1);
            NormalizedAnimationEvent& normalized_event =
                normalized_events.back();

            normalized_event.event = the_event;
            normalized_event.anim_id = anim_id;
            normalized_event.time = NormalizedFromLocal((float)keyframe.time);
        }
    }

    if (mirror) {
        const string& skeleton_file_path = AnimationRetargeter::Instance()->GetSkeletonFile(path_);
        // SkeletonAssets::Instance()->ReturnRef(skeleton_file_path);
        SkeletonAssetRef sar = Engine::Instance()->GetAssetManager()->LoadSync<SkeletonAsset>(skeleton_file_path);
        const SkeletonFileData& skeleton_file_data = sar->GetData();
        const vector<int>& symmetry = skeleton_file_data.symmetry;
        for (auto& normalized_event : normalized_events) {
            AnimationEvent& event = normalized_event.event;
            SwitchStringRightLeft(event.event_string);
            if (symmetry[event.which_bone] != -1) {
                event.which_bone = symmetry[event.which_bone];
            }
        }
    }

    ++anim_id;
    return normalized_events;
}

void Animation::RecalcCaches() {
    // Cache which bones use IK in each frame
    for (auto& k : keyframes) {
        // Initialize uses_ik vector
        size_t num_bones = k.bone_mats.size();
        k.uses_ik.resize(num_bones);
        for (int i = 0; i < num_bones; i++) {
            k.uses_ik[i] = false;
        }
        // Travel down each ik chain to specify that each bone uses ik
        size_t num_ik_bones = k.ik_bones.size();
        for (int i = 0; i < num_ik_bones; i++) {
            const BonePath& bone_path = k.ik_bones[i].bone_path;
            size_t bone_path_len = bone_path.size();
            for (size_t j = 0; j < bone_path_len; j++) {
                k.uses_ik[bone_path[j]] = true;
            }
        }
    }
    CalcInvertBoneMats();
}

void Animation::Reload() {
    Load(path_, 0x0);
}

void Animation::ReportLoad() {
}

void MirrorBT(BoneTransform& bt, bool xy_flip) {
    bt.origin[0] *= -1.0f;

    bt.rotation[1] *= -1.0f;
    bt.rotation[2] *= -1.0f;

    for (int i = 0; i < 4; ++i) {
        LOG_ASSERT(bt.rotation[i] == bt.rotation[i]);
    }

    if (xy_flip) {
        mat4 mat = Mat4FromQuaternion(bt.rotation);
        mat.SetColumn(0, mat.GetColumn(0) * -1.0f);
        mat.SetColumn(1, mat.GetColumn(1) * -1.0f);
        bt.rotation = QuaternionFromMat4(mat);
    }
}

string filename(const string& path) {
    size_t last_slash_pos = path.rfind('/');
    return path.substr(last_slash_pos + 1);
}

string extension(const string& path) {
    size_t last_slash_pos = path.rfind('.');
    return path.substr(last_slash_pos + 1);
}

AnimationAsset::AnimationAsset(AssetManager* owner, uint32_t asset_id) : AssetInfo(owner, asset_id) {
}

AnimationAssetRef ReturnAnimationAssetRef(const string& path) {
    AnimationAssetRef new_ref;
    if (extension(path) == "xml") {
        SyncedAnimationGroupRef ref = Engine::Instance()->GetAssetManager()->LoadSync<SyncedAnimationGroup>(path);
        new_ref = AnimationAssetRef(&(*ref));
    } else if (extension(path) == "anm") {
        AnimationRef ref = Engine::Instance()->GetAssetManager()->LoadSync<Animation>(path);
        new_ref = AnimationAssetRef(&(*ref));
    }
    return new_ref;
}

float GetAnimationEventTime(const string& anim_path, const string& event_str) {
    AnimationAssetRef aar = ReturnAnimationAssetRef(anim_path);
    int anim_id = 0;
    vector<NormalizedAnimationEvent> events = aar->GetEvents(anim_id, false);
    for (const auto& event : events) {
        if (event_str == event.event.event_string) {
            return aar->AbsoluteTimeFromNormalized(event.time) * 0.001f;
        }
    }
    return -1.0f;
}

void Animation::ReturnPaths(PathSet& path_set) {
    path_set.insert("animation " + path_);
}

Animation::~Animation() {
}

int Animation::GetActiveID(const BlendMap& blendmap, int& anim_id) const {
    int curr_id = anim_id;
    ++anim_id;
    return curr_id;
}

float Animation::AbsoluteTimeFromNormalized(float normalized_time) const {
    return normalized_time * length;
}

AnimationConfig::AnimationConfig() : kDisableIK(false),
                                     kDisableModifiers(false),
                                     kDisableSoftAnimation(false),
                                     kDisableInterpolation(false),
                                     kDisableAnimationLayers(false),
                                     kDisableAnimationMix(false),
                                     kDisableAnimationTransition(false),
                                     kDisablePhysicsInterpolation(false),
                                     kForceIdleAnim(false) {
}

AssetLoaderBase* Animation::NewLoader() {
    return new FallbackAssetLoader<Animation>();
}
