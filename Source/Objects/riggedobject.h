//-----------------------------------------------------------------------------
//           Name: riggedobject.h
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
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

#include <Graphics/skeleton.h>
#include <Graphics/vbocontainer.h>
#include <Graphics/modelsurfacewalker.h>
#include <Graphics/bloodsurface.h>
#include <Graphics/model.h>
#include <Graphics/palette.h>
#include <Graphics/textureref.h>
#include <Graphics/animationclient.h>
#include <Graphics/bonetransform.h>
#include <Graphics/vboringcontainer.h>

#include <Editors/editor_types.h>
#include <Editors/editor_utilities.h>

#include <Math/quaternions.h>
#include <Math/overgrowth_geometry.h>

#include <Objects/softinfo.h>
#include <Objects/itemobjectscriptreader.h>
#include <Objects/object.h>
#include <Objects/envobjectattach.h>

#include <Asset/Asset/animation.h>
#include <Asset/Asset/lipsyncfile.h>
#include <Asset/Asset/character.h>
#include <Asset/Asset/item.h>
#include <Asset/Asset/attachmentasset.h>
#include <Asset/Asset/objectfile.h>

#include <Online/time_interpolator.h>
#include <Online/online_datastructures.h>

#include <Game/characterscript.h>
#include <Scripting/angelscript/ascontext.h>

#include <string>

#define USE_SSE
#ifdef USE_SSE
#include <Math/simd_mat4.h>
#endif
//-----------------------------------------------------------------------------
// Class Definition
//-----------------------------------------------------------------------------

using std::string;
using std::unordered_map;

struct BlendableAnimation {
    AnimationRef animation;
    float weight;
    float curr_time;
};

struct MorphTarget {
    std::string name;
    int model_id[4];
    bool model_copy[4];
    float default_val;
    float anim_weight;
    float script_weight;
    float script_weight_weight;
    float disp_weight;
    float weight;
    float old_weight;
    float mod_weight;  // used for multiplayer sync
    unsigned short checksum;
    std::vector<int> modified_verts[4];
    std::vector<char> verts_modified[4];
    std::vector<int> modified_tc[4];
    std::vector<char> tc_modified[4];
    std::vector<MorphTarget> morph_targets;

    void PreDrawCamera(int num, int progress, int lod_level, uint32_t char_id = 0);
    void UpdateForInterpolation();
    void Load(const std::string &base_model_name,
              int base_model_id,
              const std::string &_name,
              int parts = 1,
              bool absolute_path = false);
    float GetScriptWeight();
    void SetScriptWeight(float weight, float weight_weight);
    float GetWeight();
    void CalcVertsModified(int base_model_id, int lod_level);
    void Dispose();
};

typedef std::list<BlendableAnimation> BlendableAnimationList;

class BulletWorld;
class ASContext;
struct SphereCollision;

struct SoftIKBone {
    std::string bone_label;
    vec3 position;
    vec3 velocity;
    bool wind;
    float joint_strength;
    float blend;
    float default_blend;
    float angular_damping;
    std::vector<int> extra;
    SoftIKBone() : wind(false), joint_strength(0.0f), blend(0.0f), default_blend(0.0f), angular_damping(0.0f) {}
};

struct EdgeCollapse {
    int vert_id[2];
    vec3 offset;
    std::vector<int> modified_faces;
    std::vector<int> deleted_faces;
};

class Decal;

class btPoint2PointConstraint;
class btTypedConstraint;
class IMUIContext;

struct AttachedItem {
    btTypedConstraint *constraint;
    ItemObjectScriptReader item;
    int bone_id;
    mat4 rel_mat;
};

typedef std::list<AttachedItem> AttachedItemList;

struct AttachedItems {
    AttachedItemList items;
    void Add(ItemObject *io, int bone, mat4 rel_mat, int char_id, BulletWorld *bw, Skeleton &skeleton);
    void Remove(int id);
    void InvalidatedItem(ItemObjectScriptReader *invalidated);
    static void InvalidatedItemCallback(ItemObjectScriptReader *invalidated, void *this_ptr);
    void RemoveAll();
};

struct AttachmentSlot {
    vec3 pos;
    AttachmentType type;
    AttachmentRef attachment_ref;
    bool mirrored;
};

struct CachedSkeletonInfo {
    std::vector<BoneTransform> bind_matrices;
};

class RiggedObject;

class RiggedTransformedVertexGetter : public BloodSurface::TransformedVertexGetter {
   public:
    RiggedTransformedVertexGetter(RiggedObject *p_rigged_object) : rigged_object(p_rigged_object) {}
    vec3 GetTransformedVertex(int val) override;

   private:
    RiggedObject *rigged_object;
};

typedef std::list<AttachmentSlot> AttachmentSlotList;
typedef std::map<int, WeapAnimInfo> WeapAnimInfoMap;
struct NetworkBone;
struct MorphTargetStateStorage;
class RiggedObject : public Object {
    struct {
        ASFunctionHandle handle_animation_event;
        ASFunctionHandle display_matrix_update;
        ASFunctionHandle final_animation_matrix_update;
        ASFunctionHandle final_attached_item_update;
        ASFunctionHandle notify_item_detach;
    } as_funcs;

    static const int kLodLevels = 4;

   public:
    void CalcRootBoneVelocity();
    float GetRootBoneVelocity();
    mat4 old_root_bone;
    float velocity;
    float old_time = .0f;
    const std::vector<mat4> &GetViewBones() const;
    std::vector<NetworkBone> network_bones;
    std::vector<MorphTargetStateStorage> network_morphs;
    unordered_map<string, MorphTargetStateStorage> incoming_network_morphs;
    float network_bones_host_walltime = 0.0f;

    bool static_char;
    float last_draw_time;
    vec3 ambient_cube_color[6];
    CachedSkeletonInfo cached_skeleton_info_;
    EntityType GetType() const override { return _rigged_object; }
    // Textures
    TextureAssetRef stab_texture;
    TextureAssetRef texture_ref;
    TextureAssetRef normal_texture_ref;
    TextureAssetRef palette_texture_ref;
    TextureAssetRef fur_tex;
    TextureRef cube_map;
    // Model and VBO data
    int model_id[4];
    std::vector<int> fur_base_vertex_ids;
    Model fur_model;
    bool need_vbo_update[kLodLevels];
    VBORingContainer vel_vbo;
    VBORingContainer *transform_vec_vbo0[kLodLevels];
    VBORingContainer *transform_vec_vbo1[kLodLevels];
    VBORingContainer *transform_vec_vbo2[kLodLevels];
    VBORingContainer *tex_transform_vbo[kLodLevels];
    VBORingContainer fur_transform_vec_vbo0;
    VBORingContainer fur_transform_vec_vbo1;
    VBORingContainer fur_transform_vec_vbo2;
    VBORingContainer fur_tex_transform_vbo;
    vec3 current_pos, next_pos;
    vec3 last_pos;
    bool dont = false;
    float last_model_scale;

    bool update_camera_pos;
    int last_draw_frame = 0;

    std::vector<mat4> network_display_bone_matrices;  // Final interpolated bone matrices for rendering, from server when client.
    std::vector<mat4> display_bone_matrices;          // Final interpolated bone matrices for rendering
    std::vector<BoneTransform> display_bone_transforms;
#ifdef USE_SSE
    simd_mat4 *simd_bone_mats;
#endif
    // Do we need to redo skinning?
    bool needs_matrix_update;
    // Item data
    AttachedItems attached_items;
    AttachedItems stuck_items;
    WeapAnimInfoMap weap_anim_info_map;  // Storing weapon info from current animation frame
    std::map<int, mat4> weapon_offset;
    std::map<int, mat4> weapon_offset_retarget;
    int primary_weapon_id;
    std::vector<SoftIKBone> soft_ik_bones;
    // Ragdoll transition data
    bool first_ragdoll_frame;
    // Physics data
    float ragdoll_strength;
    Skeleton skeleton_;
    // Animation data
    AnimationClient anim_client;
    bool animated;
    std::vector<BoneTransform> animation_frame_bone_matrices;
    std::vector<BoneTransform> cached_animation_frame_bone_matrices;
    std::vector<BoneTransform> unmodified_transforms;
    std::set<BulletObject *> head_chain;
    std::vector<float> ik_offset;
    float total_ik_offset;
    bool ik_enabled;
    vec3 total_center_offset;
    float total_rotation;
    std::vector<BlendedBonePath> blended_bone_paths;
    std::vector<MorphTarget> morph_targets;
    std::vector<AttachedEnvObject> children;
    std::map<std::string, float> status_keys;
    // Animation update data
    int anim_update_period;
    int max_time_until_next_anim_update;
    int time_until_next_anim_update;
    int curr_anim_update_time;
    int prev_anim_update_time;
    // Script data
    CharacterScriptGetter *character_script_getter;
    ASContext *as_context;
    // Character animation info
    std::string char_anim_override;
    std::string char_anim;
    char char_anim_flags;
    // Misc
    LipSyncFileReader lipsync_reader;
    std::vector<vec3> palette_colors;
    std::vector<vec3> palette_colors_srgb;
    int char_id;
    std::map<std::string, std::vector<Decal *> > last_decal;
    int lod_level;
    int shadow_group_id;
    BloodSurface blood_surface;
    float floor_height;
    vec3 camera_translation;

    RiggedObject();
    ~RiggedObject() override;

    bool Initialize() override;
    void Update(float timestep) override;

    // Drawing
    void Draw(const mat4 &proj_view_matrix, DrawType type);
    void ClientBeforeDraw();
    void GetInterpolationWithLeftOver();
    void InterpolateBetweenNetworkBones();
    static void GetCubicSplineWeights(float interp, float *weights);
    static const mat4 InterpolateBetweenFourBonesQuadratic(const NetworkBone *bones, float inter_step);
    static const mat4 InterpolateBetweenTwoBones(const NetworkBone &current, const NetworkBone &next, float interp_step);
    bool CheckIfNextStepLeadsToStop();
    bool CheckIfOverShot();
    void InterpolateNoNewData();
    void DebugCalculateInterpolationVelocity();
    void CalcNoDataInterpStep();
    void StoreNetworkBones();
    void StoreNetworkMorphTargets();
    void PreDrawFrame(float curr_game_time) override;
    void PreDrawCamera(float curr_game_time) override;
    void DrawModel(Model *model, int lod_level);
    void SetASContext(ASContext *_as_context);

    std::vector<vec3> *GetPaletteColors();
    void ApplyPalette(const OGPalette &palette) override;
    float GetStatusKeyValue(const std::string &label);
    void Ragdoll(const vec3 &velocity);
    void AddAnimation(std::string path, float weight);
    void ApplyBoneMatricesToModel(bool old, int lod_level);
    void FixDiscontinuity();
    vec3 GetAvgPosition();
    vec3 GetAvgVelocity();
    void UnRagdoll();
    vec3 GetAvgAngularVelocity();
    quaternion GetAvgRotation();
    void SetMTTargetWeight(const std::string &target_name, float weight, float weight_weight);
    vec3 GetIKTargetPosition(const std::string &target_name);
    vec3 GetIKTargetAnimPosition(const std::string &target_name);
    vec3 FetchCenterOffset();
    float GetIKWeight(const std::string &target_name);
    void ApplyForceToRagdoll(const vec3 &force, const vec3 &position);
    void ApplyForceToBone(const vec3 &force, const int &bone);
    void ApplyForceLineToRagdoll(const vec3 &force, const vec3 &position, const vec3 &line_dir);
    vec3 GetDisplayBonePosition(int bone);
    vec3 GetBonePosition(int bone);
    mat4 GetBoneRotation(int bone);
    vec3 GetBoneLinearVel(int bone);
    void SetSkeletonOwner(Object *owner);
    void Load(const std::string &character_path, vec3 pos, SceneGraph *_scenegraph, OGPalette &palette);
    void SetAnimUpdatePeriod(int update_script_period);
    float FetchRotation();
    void SetDamping(float amount);
    mat4 GetIKTargetTransform(const std::string &target_name);
    void DetachItem(ItemObject *item_object);
    void HandleAnimationEvents(AnimationClient &anim_client);
    void SetRagdollStrength(float amount);
    mat4 GetAvgIKChainTransform(const std::string &target_name);
    void EnableSleep();
    void DisableSleep();
    vec3 GetAvgIKChainPos(const std::string &target_name);
    void CheckForNAN();
    void HandleLipSyncMorphTargets(float timestep);
    bool InHeadChain(BulletObject *obj);
    vec3 GetTransformedVertex(int vert_id);
    void CreateBloodDrip(const std::string &ik_name, int ik_link, const vec3 &direction);
    void CreateBloodDripAtBone(int bone_id, int ik_link, const vec3 &direction);
    void CleanBlood();
    void GetTransformedTri(int id, vec3 *points);
    void RemoveLayer(int which, float fade_speed);
    void SetCharAnim(const std::string &path, char flags = 0, const std::string &override = "");
    void CheckItemAnimBlends(const std::string &path, char flags = 0);
    void CutPlane(const vec3 &normal, const vec3 &pos, const vec3 &dir, int type, int depth);
    void MPCutPlane(const vec3 &normal, const vec3 &pos, const vec3 &dir, int type, int depth, std::vector<int> mp_hit_list, vec3 points[3]);
    Skeleton &skeleton();
    void UpdateCollisionObjects();
    void AddBloodAtPoint(const vec3 &point);
    void Stab(const vec3 &pos, const vec3 &dir, int type, int depth);
    void AddWaterCube(const mat4 &transform);
    void DetachAllItems();
    static void InvalidatedItemCallback(ItemObjectScriptReader *invalidated, void *this_ptr);
    void InvalidatedItem(ItemObjectScriptReader *invalidated);
    int GetTimeBetweenLastTwoAnimUpdates();
    int GetTimeSinceLastAnimUpdate();
    void SaveSimplificationCache(const std::string &path);
    bool LoadSimplificationCache(const std::string &path);
    void CreateFurModel(int model_id, int fur_model_id);
    void MoveRagdollPart(const std::string &label, const vec3 &position, float strength_mult) const;
    void FixedRagdollPart(int boneID, const vec3 &position) const;
    void SpikeRagdollPart(int boneID, const vec3 &start, const vec3 &end, const vec3 &pos) const;
    void ClearBoneConstraints();
    void RefreshRagdoll();
    vec3 GetIKChainPos(const std::string &target_name, int which);
    mat4 GetIKChainTransform(const std::string &target_name, int which);
    void StickItem(int id, const vec3 &start, const vec3 &end, const mat4 &transform);
    void SetWeaponOffset(AttachedItem &stuck_item, AttachmentType type, bool mirror);
    void SheatheItem(int id, bool on_right);
    void UnSheatheItem(int id, bool right_hand);
    void SetCharAnimOverride(std::string path) const;
    void UnStickItem(int id);
    void SetItemAttachment(AttachedItem &stuck_item, AttachmentRef attachment_ref, bool mirror);
    std::vector<ItemRef> GetWieldedItemRefs() const;
    void SetCharacterScriptGetter(CharacterScriptGetter &csg);
    void SetPrimaryWeaponID(int id);
    void AvailableItemSlots(const ItemRef &item_ref, AttachmentSlotList *list) const;
    void AttachItemToSlot(ItemObject *item_object, AttachmentType type, bool mirrored, const AttachmentRef *attachment_ref);
    float GetCharScale();
    AnimationClient &GetAnimClient();
    void SetCharScale(float val);
    float GetRelativeCharScale();
    bool DrawBoneConnectUI(Object *objects[], int num_obj_ids, IMUIContext &imui_context, EditorTypes::Tool tool, int id);
    void AddToDesc(EntityDescription &desc);
    void UpdateGPUSkinning();
    void GetShaderNames(std::map<std::string, int> &shaders) override;

    bool GetOnlineIncomingMorphTargetState(MorphTargetStateStorage &dest, const char *name);

   private:
    ObjectFileRef ofc;
    RiggedTransformedVertexGetter transformed_vertex_getter_;
    float char_scale;
    float model_char_scale;
    void UpdateAttachedItems();

    RiggedObject(const RiggedObject &other);
    RiggedObject &operator=(const RiggedObject &other);

    const char *shader;
    const char *water_cube;
    const char *water_cube_expand;
};

mat4 ASGetBindMatrix(Skeleton *skeleton, int which_bone);
int ASIKBoneStart(Skeleton *skeleton, const std::string &name);
int ASIKBoneLength(Skeleton *skeleton, const std::string &name);

void RotateBoneToMatchVec(RiggedObject *rigged_object, const vec3 &a, const vec3 &b, int bone);
void RotateBonesToMatchVec(RiggedObject *rigged_object, const vec3 &a, const vec3 &c, int bone, int bone2, float weight);
vec3 GetTransformedBonePoint(RiggedObject *rigged_object, int bone, int point);
void DefineRiggedObjectTypePublic(ASContext *as_context);
BoneTransform GetIKTransform(RiggedObject *rigged_object, const std::string &target_name);
mat4 GetUnmodifiedIKTransform(RiggedObject *rigged_object, const std::string &target_name);

vec3 ASGetModelCenter(RiggedObject *rigged_object);
