//-----------------------------------------------------------------------------
//           Name: movementobject.h
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

#include <Objects/object.h>
#include <Objects/itemobjectscriptreader.h>
#include <Objects/riggedobject.h>

#include <Graphics/camera.h>
#include <Graphics/palette.h>

#include <Game/attackscript.h>
#include <Game/reactionscript.h>
#include <Game/characterscript.h>

#include <Online/time_interpolator.h>
#include <Online/online_datastructures.h>

#include <Asset/Asset/material.h>
#include <Asset/Asset/actorfile.h>
#include <Asset/Asset/soundgroup.h>
#include <Asset/Asset/attacks.h>

#include <Scripting/angelscript/ascontext.h>
#include <Scripting/angelscript/ascollisions.h>

#include <Internal/timer.h>
#include <Math/quaternions.h>
#include <AI/pathfind.h>
#include <Sound/soundplayinfo.h>

#include <queue>
//-----------------------------------------------------------------------------
// Class Definition
//-----------------------------------------------------------------------------

const float _cam_follow_distance = 2.0f;
const float _leg_sphere_size = 0.5f;

class Decal;
class ItemObject;
class SweptSlideCallback;
class ContactSlideCallback;
class PathPointObject;
class IMUIContext;

using std::list;

struct VoiceQueue {
    std::string path;
    float delay;
    float pitch;
    VoiceQueue(const std::string& _path, float _delay, float _pitch) : path(_path),
                                                                       delay(_delay),
                                                                       pitch(_pitch) {}
};

class AttackHistory {
   public:
    void Add(const std::string& str);
    float Check(const std::string& str);
    void Clear();

   private:
    struct AttackHistoryEntry {
        AttackScriptGetter attack_script_getter;
        float time;
    };
    typedef std::list<AttackHistoryEntry> AttackHistoryEntries;
    AttackHistoryEntries entries;
    typedef std::map<AttackRef, float> InnerTransitionMap;
    typedef std::map<unsigned long, InnerTransitionMap> OuterTransitionMap;
    OuterTransitionMap transitions;
};

class MovementObject : public Object {
   public:
    float mp_time = 0.0;
    float tickRate = 0.0;
    bool angle_script_ready;
    const static int _awake;
    const static int _unconscious;
    const static int _dead;

    struct OcclusionQuery {
        int id;
        bool in_progress;
        int cam_id;
    };
    struct OcclusionState {
        int cam_id;
        bool occluded;
    };

    std::queue<std::string> message_queue;
    std::vector<OcclusionState> occlusion_states;
    std::vector<OcclusionQuery> occlusion_queries;

    EntityType GetType() const override { return _movement_object; }
    bool controlled;
    bool remote;
    bool was_controlled;
    bool static_char;

    unsigned long voice_sound;
    std::list<VoiceQueue> voice_queue;
    std::vector<unsigned long> attached_sounds;
    std::string character_path;
    ReactionScriptGetter reaction_script_getter;
    CharacterScriptGetter character_script_getter;
    std::unique_ptr<ASContext> as_context;
    OGPalette palette;
    int connected_pathpoint_id;
    std::vector<ItemConnection> item_connection_vec;
    std::map<int, int> connection_finalization_remap;
    bool do_connection_finalization_remap;
    std::list<ItemObjectScriptReader> item_connections;
    int update_script_counter;
    int update_script_period;
    int controller_id;
    int camera_id;
    bool visible;
    int no_grab;
    bool focused_character;
    float kCullRadius;
    std::string object_npc_script_path;
    std::string object_pc_script_path;

    std::string nametag_string;
    vec3 nametag_last_position;

    std::unique_ptr<ASCollisions> as_collisions;

    ActorFileRef actor_file_ref;
    std::set<SoundGroupRef> touched_surface_refs;
    std::set<MaterialRef> clothing_refs;

    std::vector<char> env_object_attach_data;
    bool is_player;

    vec3 position;
    vec3 velocity;

    double delta = 0.0;
    bool needs_predraw_update;
    float reset_time;

    list<OnlineMessageRef> incoming_material_sound_events;
    list<OnlineMessageRef> incoming_movement_object_frames;
    list<OnlineMessageRef> incoming_cut_lines;
    bool disable_network_bone_interpolation = false;
    TimeInterpolator network_time_interpolator;
    float last_walltime_diff = 0.0f;

    MovementObject();
    ~MovementObject() override;

    bool Initialize() override;
    bool InitializeMultiplayer();
    void GetShaderNames(std::map<std::string, int>& shaders) override;

    void Update(float timestep) override;
    void Draw() override;
    void ClientBeforeDraw();
    virtual void HandleTransformationOccured();
    void PreDrawFrame(float curr_game_time) override;

    void Reload() override;
    void ActualPreDraw(float curr_game_time);
    void NotifyDeleted(Object* other) override;
    bool ConnectTo(Object& other, bool checking_other = false) override;
    bool AcceptConnectionsFrom(ConnectionType type, Object& object) override;
    bool Disconnect(Object& other, bool from_socket = false, bool checking_other = false) override;
    void FinalizeLoadedConnections() override;
    void GetDesc(EntityDescription& desc) const override;
    bool SetFromDesc(const EntityDescription& desc) override;
    void ChangeControlScript(const std::string& script_path);
    void CollideWith(MovementObject* other);
    void ASSetScriptUpdatePeriod(int val);
    bool HasFunction(const std::string& function_definition);
    int QueryIntFunction(std::string func);
    void SetScriptParams(const ScriptParamMap& spm) override;
    void ApplyPalette(const OGPalette& palette, bool from_socket = false) override;
    OGPalette* GetPalette() override;
    void HitByItem(int id, const vec3& point, const std::string& material, int type);
    void Execute(std::string);
    float ASGetFloatVar(std::string name);
    bool ASGetBoolVar(std::string name);
    bool ASHasVar(std::string name);
    int ASGetIntVar(std::string name);
    int ASGetArrayIntVar(std::string name, int index);
    void ASSetIntVar(std::string name, int value);
    void ASSetArrayIntVar(std::string name, int index, int value);
    void ASSetFloatVar(std::string name, float value);
    void ASSetBoolVar(std::string name, bool value);
    void Dispose() override;
    bool ASOnSameTeam(MovementObject* other);
    void AttachItemToSlot(int which, AttachmentType type, bool mirrored);
    void AttachItemToSlotEditor(int which, AttachmentType type, bool mirrored, const AttachmentRef& attachment_ref, bool from_socket = false);
    void RemovePhysicsShapes();
    void ReceiveMessage(std::string msg);
    void UpdateScriptParams() override;
    void SetRotationFromFacing(vec3 facing);
    void GetDisplayName(char* buf, int buf_size) override;
    RiggedObject* rigged_object();

    void RemapReferences(std::map<int, int> id_map) override;

    void GetConnectionIDs(std::vector<int>* cons) override;

    void UpdatePaused();
    int AboutToBeHitByItem(int id);

    int InputFromAngelScript();

    const std::string& GetCurrentControlScript() const { return current_control_script_path; }
    const std::string& GetActorScript() const { return actor_script_path; }
    const std::string& GetNPCObjectScript() const { return object_npc_script_path; }
    const std::string& GetPCObjectScript() const { return object_pc_script_path; }
    vec3 facing;
    vec3 GetFacing();
    float GetTempHealth();
    void addAngelScriptUpdate(uint32_t state, std::vector<uint32_t> data);
    std::vector<std::pair<uint32_t, std::vector<uint32_t>>> angelscript_update;
    void SetCharAnimation(const std::string& path, float fade_speed = _default_fade_speed, char flags = 0);
    void SetAnimAndCharAnim(std::string path, float fade_speed, char flags, std::string anim_path);
    std::string GetTeamString();
    void StartPoseAnimation(std::string path);

   private:
    std::unique_ptr<RiggedObject> rigged_object_;
    std::string current_control_script_path;
    std::string actor_script_path;
    BulletObject* char_sphere;
    AttackHistory attack_history;

    uint64_t last_timestamp = 0;

    struct {
        ASFunctionHandle init;
        ASFunctionHandle init_multiplayer;
        ASFunctionHandle is_multiplayer_supported;
        ASFunctionHandle set_parameters;
        ASFunctionHandle handle_editor_attachment;
        ASFunctionHandle contact;
        ASFunctionHandle collided;
        ASFunctionHandle notify_item_detach;
        ASFunctionHandle movement_object_deleted;
        ASFunctionHandle script_swap;
        ASFunctionHandle handle_collisions_btc;
        ASFunctionHandle hit_by_item;
        ASFunctionHandle pre_draw_frame;
        ASFunctionHandle pre_draw_camera;
        ASFunctionHandle update;
        ASFunctionHandle update_multiplayer;
        ASFunctionHandle force_applied;
        ASFunctionHandle get_temp_health;
        ASFunctionHandle was_hit;
        ASFunctionHandle was_blocked;
        ASFunctionHandle reset;
        ASFunctionHandle post_reset;
        ASFunctionHandle attach_weapon;
        ASFunctionHandle attach_misc;
        ASFunctionHandle set_enabled;
        ASFunctionHandle receive_message;
        ASFunctionHandle update_paused;
        ASFunctionHandle about_to_be_hit_by_item;
        ASFunctionHandle InputToEngine;
        ASFunctionHandle apply_host_input;
        ASFunctionHandle apply_host_camera_flat_facing;
        ASFunctionHandle set_damage_time_from_socket;
        ASFunctionHandle set_damage_blood_time_from_socket;
        ASFunctionHandle reset_waypoint_target;
        ASFunctionHandle dispose;
        ASFunctionHandle pre_draw_camera_no_cull;
        ASFunctionHandle register_mp_callbacks;
        ASFunctionHandle start_pose;
    } as_funcs;

    const char* shader;

    void ReInitializeASFunctions();

    void RegisterMPCallbacks() const;
    void Collided(const vec3& pos, float impulse, const CollideInfo& collide_info, BulletObject* object) override;
    void DrawDepthMap(const mat4& proj_view_matrix, const vec4* cull_planes, int num_cull_planes, Object::DrawType draw_type) override;
    void PreDrawCamera(float curr_game_time) override;
    void GoLimp();
    void ApplyForce(vec3 force);
    void Ragdoll();
    void UnRagdoll();
    void ASWasBlocked();
    void SetAnimation(std::string path, float fade_speed);
    void SetAnimation(std::string path);
    void SetAnimation(std::string path, float fade_speed, char flags);
    void SwapAnimation(std::string path);
    void HandleMovementObjectMaterialEvent(std::string the_event, vec3 event_pos, float gain = 1.0f);

    vec4 GetAvgRotationVec4();
    int ASWasHit(std::string type, std::string attack_path, vec3 dir, vec3 pos, int attacker_id, float attack_damage_mult, float attack_knockback_mult);
    void ASPlaySoundGroupAttached(std::string path, vec3 location);
    void ASPlaySoundAttached(std::string path, vec3 location);
    void MaterialParticleAtBone(std::string type, std::string bone_name);
    void CreateRiggedObject();
    void RecreateRiggedObject(std::string _char_path);
    void ASDetachItem(int which);
    void HandleMovementObjectMaterialEventDefault(std::string the_event, vec3 event_pos);
    void ForceSoundGroupVoice(std::string path, float delay);
    int GetWaypointTarget();
    void Reset() override;
    void SetRotationFromEditorTransform();
    void ASSetCharAnimation(std::string path, float fade_speed, char flags);
    void ASSetCharAnimation(std::string path, float fade_speed);
    void ASSetCharAnimation(std::string path);
    void StopVoice();
    void InvalidatedItem(ItemObjectScriptReader* invalidated);
    static void InvalidatedItemCallback(ItemObjectScriptReader* invalidated, void* this_ptr);
    void ASDetachAllItems();
    bool OnTeam(const std::string& other_team);
    void SetEnabled(bool val) override;
    void AttachItemToSlotAttachmentRef(int which, AttachmentType type, bool mirrored, const AttachmentRef* ref, bool from_socket = false);
    void AddToAttackHistory(const std::string& str);
    float CheckAttackHistory(const std::string& str);
    void ClearAttackHistory();
    void ReceiveObjectMessageVAList(OBJECT_MSG::Type type, va_list args) override;
    void OverrideCharAnim(const std::string& label, const std::string& new_path);
    void UpdateWeapons();
    void Moved(Object::MoveType type) override;
    void ChildLost(Object* obj) override;
    void GetChildren(std::vector<Object*>* ret_children) override;
    void GetBottomUpCompleteChildren(std::vector<Object*>* ret_children) override;
    bool IsMultiplayerSupported() override;
    void RegenerateNametag();
    void DrawNametag();

   public:
    void PlaySoundGroupVoice(std::string path, float delay);
};

mat4 RotationFromVectors(const vec3& front, const vec3& right, const vec3& up);
void DefineMovementObjectTypePublic(ASContext* as_context);
