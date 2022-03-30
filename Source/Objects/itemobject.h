//-----------------------------------------------------------------------------
//           Name: itemobject.h
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

#include <Online/time_interpolator.h>
#include <Online/online_datastructures.h>

#include <Graphics/drawbatch.h>
#include <Graphics/bloodsurface.h>

#include <Asset/Asset/item.h>
#include <Asset/Asset/objectfile.h>

#include <Objects/object.h>
#include <XML/level_loader.h>
#include <Physics/bulletworld.h>
#include <Game/color_tint_component.h>

#include <vector>
#include <string>
#include <list>

//-----------------------------------------------------------------------------
// Class Definition
//-----------------------------------------------------------------------------

class BulletObject;
class ItemObjectScriptReader;
class ASContext;
struct Flare;
class ItemObject;

class ItemObjectBloodSurfaceTransformedVertexGetter : public BloodSurface::TransformedVertexGetter {
public:
    ItemObjectBloodSurfaceTransformedVertexGetter(ItemObject* p_item_object):
        item_object(p_item_object) {}
    virtual vec3 GetTransformedVertex(int val);
private:
    ItemObject* item_object;
};

class ItemObject: public Object {
public:
    virtual EntityType GetType() const { return _item_object; }
    enum ItemState {
        kWielded, kSheathed, kFree
    };
    
    ItemObject();
    virtual ~ItemObject();
    
    void Load(const std::string &item_path);
    void GetShaderNames(std::map<std::string, int>& preload_shaders);

    virtual bool Initialize();
    virtual void Update(float timestep);
    virtual void Draw();
    virtual void PreDrawFrame(float curr_game_time);
    virtual bool ConnectTo(Object& other, bool checking_other = false);
    virtual bool Disconnect(Object& other, bool checking_other = false);
    virtual void Moved(Object::MoveType type);

    int model_id() const;
    const ItemRef& item_ref() const;
    const vec3& phys_offset() const;
    ItemState state() const;
    
    void Copied();
    
    void AddReader(ItemObjectScriptReader* reader);
    void RemoveReader(ItemObjectScriptReader* reader);
    void InvalidateReaders();
    void InvalidateHeldReaders();
    bool IsHeld();
    int StuckInWhom();
    
	void SetPosition(const vec3& pos);
    vec3 GetPhysicsPosition();
    void SetPhysicsTransform( const mat4 &transform );
    void SetVelocities( const vec3 &linear_vel, const vec3 &angular_vel );
    void SetInterpolation( int count, int period );
    void ActivatePhysics( );
    vec3 GetAngularVelocity();
    void SetAngularVelocity(vec3 vel);
    mat4 GetPhysicsTransform();
    mat4 GetPhysicsTransformIncludeOffset();
    void GetPhysicsVel( vec3 & linear_vel, vec3 & angular_vel );
    void SetLinearVelocity(vec3 vel);
    vec3 GetLinearVelocity();
    void WakeUpPhysics();
    void SleepPhysics();
    btTypedConstraint* AddConstraint( BulletObject* _bullet_object );
    void RemoveConstraint( btTypedConstraint** constraint );
    
	mat4 GetTransform() const; // Returns final transform matix, used in multiplayer

    void Collided( const vec3& pos, float impulse, const CollideInfo &collide_info, BulletObject* object );
    void GetDesc(EntityDescription &desc) const;
    void AddBloodDecal( vec3 pos, vec3 dir, float size );
    void CleanBlood();
    void Reset();
    virtual void Dispose();
    void SetHolderID( int char_id );
    void SetThrown();
    void SetThrownStraight();
    void SetState(ItemState state);
    ScriptParams* ASGetScriptParams();
    int HeldByWhom();
    virtual void ReceiveObjectMessageVAList(OBJECT_MSG::Type type, va_list args);
    virtual bool SetFromDesc( const EntityDescription& desc );
    const vec3 & GetColorTint();
    const float& GetOverbright();
    bool CheckThrownSafe() const;
    void SetSafe();
    vec3 GetTransformedVertex(int which);
    int last_held_char_id_;

    std::list<ItemObjectFrame> incoming_online_item_frames;

private:
    enum ThrownState {
        kSafe, kThrown, kThrownStraight, kBounced
    };
    enum WeightClass {
        kLightWeight, kMediumWeight, kHeavyWeight
    };

	uint64_t last_frame = 0;

	// final model matrix. Stored as member variable to use for network interpolation
	mat4 transform; 
    bool use_tangent_;
    ColorTintComponent color_tint_component_;
    ItemState state_;
    ThrownState thrown_;
    float active_time_;
    
    float sun_ray_clear_;
    float new_clear_;
    vec3 last_sun_checked_;
    
    ItemObjectBloodSurfaceTransformedVertexGetter blood_surface_getter_;
    BloodSurface blood_surface_;
    int shadow_group_id_;
    std::vector<TextureAssetRef> textures_;
    DrawBatch batch_;
    DrawBatch depth_batch_;
    int shader_id_;
    int depth_shader_id_;
    int shadow_catch_shader_id_;
    int model_id_;
    ObjectFileRef ofc_;
    ItemRef item_ref_;
    
    unsigned long whoosh_sound_handle_;
    std::vector<int> attached_sounds_;
    float impact_sound_delay_;
    float whoosh_sound_delay_;
    float whoosh_volume_;
    float whoosh_pitch_;

    bool using_physics_;
    BulletObject* bullet_object_;
    BulletObject* abstract_bullet_object_;
    vec3 phys_offset_;
    vec3 display_phys_offset_;
    int interp_count_;
    int interp_period_;
    mat4 old_transform_;
    vec3 old_lin_vel_;
    vec3 old_ang_vel_;

    bool stuck_in_environment_;
    bool sticking_collision_occured_;
    vec3 stuck_pos_;
    int stuck_vert_;
    mat4 stuck_transform_;
    CollideInfo stuck_collide_info_;

    Flare* flare_;
    bool flashing_;
    float flash_progress_;
    float flash_delay_;

    std::list<ItemObjectScriptReader*> readers_;
    float char_impact_delay_;

    TextureAssetRef texture_spatterdecal_;
    std::string shader;

    TimeInterpolator network_time_interpolator;
    
    virtual void DrawDepthMap(const mat4& proj_view_matrix, const vec4* cull_planes, int num_cull_planes, Object::DrawType draw_type);
    virtual void HandleMaterialEvent(const std::string &the_event, const vec3& normal, const vec3 &event_pos, float gain = 1.0f, float pitch_shift = 1.0f);
    
    void MakeBulletObject();
    void StickingCollisionOccured(const vec3 &pos, int vert, const CollideInfo &collide_info);
    void StickToEnvironment(const vec3 &pos, int vert, const CollideInfo &collide_info);
    vec3 WorldToModelPos(const vec3 &pos);
    vec3 ModelToWorldPos(const vec3 &pos);
    float GetVertSharpness( int vert );
    int GetClosestVert(const vec3& pos);
    vec3 GetVertPos(int vert);
    WeightClass GetWeightClass();
    bool IsStuckInCharacter();
    bool IsConstrained();
    void HandleThrownImpact(const vec3 &start, const vec3 &end, int char_id, int line_id, const mat4 &cur_transform, const mat4 &transform);
    void CheckThrownCollisionLine( int line_id, const mat4 &cur_transform, const mat4 &transform );
    void DrawItem(const mat4& proj_view_matrix, DrawType type);

public:
    void PlayImpactSound(const vec3& pos, const vec3& normal, float impulse);
};

void DefineItemObjectTypePublic(ASContext* as_context);
