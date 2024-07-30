//-----------------------------------------------------------------------------
//           Name: itemobject.cpp
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
#include "itemobject.h"

#include <Graphics/shaders.h>
#include <Graphics/models.h>
#include <Graphics/sky.h>
#include <Graphics/particles.h>
#include <Graphics/camera.h>
#include <Graphics/pxdebugdraw.h>
#include <Graphics/flares.h>

#include <Objects/itemobjectscriptreader.h>
#include <Objects/movementobject.h>
#include <Objects/lightvolume.h>
#include <Objects/group.h>

#include <Physics/bulletworld.h>
#include <Physics/bulletobject.h>

#include <Internal/timer.h>
#include <Internal/profiler.h>
#include <Internal/common.h>

#include <Math/vec4math.h>
#include <Math/vec3math.h>

#include <Scripting/angelscript/ascontext.h>
#include <Main/scenegraph.h>
#include <Main/engine.h>
#include <XML/level_loader.h>
#include <Asset/Asset/material.h>
#include <Sound/sound.h>
#include <Utility/compiler_macros.h>
#include <Logging/logdata.h>
#include <Threading/sdl_wrapper.h>
#include <Online/online.h>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif

#include <btBulletDynamicsCommon.h>

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

extern bool g_simple_shadows;
extern bool g_level_shadows;
extern char* global_shader_suffix;
extern bool g_no_decals;
extern Timer game_timer;
extern bool g_no_reflection_capture;

extern bool g_debug_runtime_disable_item_object_draw;
extern bool g_debug_runtime_disable_item_object_draw_depth_map;
extern bool g_debug_runtime_disable_item_object_pre_draw_frame;

namespace {
const float kHeavyThreshold = 3.0f;
const float kMediumThreshold = 1.0f;

/*
static bool ShouldUseBlur(const BulletObject* bullet_object_, float time_scale){
    const float _blur_threshold = 1.5f * square(time_scale);
    bool use_blur = true;
    if(use_blur &&
        length_squared(bullet_object_->GetAngularVelocity()) + min(
        length_squared(bullet_object_->GetLinearVelocity()-ActiveCameras::Get()->GetVelocity()),
        length_squared(bullet_object_->GetLinearVelocity())) < _blur_threshold)
    {
        use_blur = false;
    }
    if(bullet_object_->transform_history.empty()){
        use_blur = false;
    }
    return use_blur;
}
*/
}  // namespace

void ItemObject::WakeUpPhysics() {
    bullet_object_->Activate();
}

const ItemRef& ItemObject::item_ref() const {
    return item_ref_;
}

void ItemObject::Collided(const vec3& pos, float impulse, const CollideInfo& collide_info, BulletObject* object) {
    if (stuck_in_environment_) {
        return;
    }
    if (thrown_ == kThrown || thrown_ == kThrownStraight) {
        thrown_ = kBounced;
    }
    if (scenegraph_->GetMaterialSharpPenetration(pos, this) > 0.0f && !IsConstrained()) {
        int vert = GetClosestVert(pos);
        float sharpness = GetVertSharpness(vert);
        vec3 local_pos = GetVertPos(vert) - display_phys_offset_;
        // vec3 local_vel = old_lin_vel_ + cross(old_ang_vel_, local_pos);
        vec3 world_vert = bullet_object_->GetTransform() * local_pos;
        vec3 vert_dir = normalize(world_vert - bullet_object_->GetPosition());
        // DebugDraw::Instance()->AddLine(pos, pos+collide_info.normal, vec4(0.0f,1.0f,0.5f,1.0f), _persistent);
        // DebugDraw::Instance()->AddLine(pos, pos-collide_info.normal, vec4(1.0f,0.0f,0.0f,1.0f), _persistent);
        if (sharpness > 0.3f && dot(vert_dir, normalize(old_lin_vel_)) > 0.7f &&
            dot(vert_dir, collide_info.normal) < -0.5f) {
            StickingCollisionOccured(pos, vert, collide_info);
            PlayImpactSound(pos, collide_info.normal, bullet_object_->GetMass());
            return;
        }
    }
    if (!collide_info.true_impact) {
        return;
    }
    if (length_squared(object->GetLinearVelocity()) < 0.1f) {
        return;
    }
    if (impact_sound_delay_ <= 0.0f && impulse > 0.1f) {
        PlayImpactSound(pos, collide_info.normal, impulse);
    }
    const MaterialParticle* mp = scenegraph_->GetMaterialParticle("skid", pos);
    if (!mp || mp->particle_path.empty()) {
        return;
    }

    vec3 particle_pos = pos;
    vec3 vel = vec3(0.0f, 1.0f, 0.0f);
    vec3 tint = scenegraph_->GetColorAtPoint(pos);
    scenegraph_->particle_system->MakeParticle(
        scenegraph_, mp->particle_path, particle_pos, vel, tint);
}

vec3 ItemObject::GetLinearVelocity() {
    return bullet_object_->GetLinearVelocity();
}

vec3 ItemObject::GetAngularVelocity() {
    return bullet_object_->GetAngularVelocity();
}

void ItemObject::SetAngularVelocity(vec3 vel) {
    bullet_object_->SetDamping(0.0f, 0.0f);
    bullet_object_->SetAngularVelocity(vel);
}

void ItemObject::HandleItemObjectMaterialEvent(const std::string& the_event, const vec3& normal, const vec3& event_pos, float gain, float pitch_shift) {
    const MaterialEvent* me = scenegraph_->GetMaterialEvent(the_event, event_pos, item_ref_->GetSoundModifier(), this);
    if (me && !me->soundgroup.empty()) {
        float dist = distance(event_pos, ActiveCameras::Get()->GetPos());
        float dist_filter = dist * 0.1f - 0.4f;
        // printf("Dist filter: %f\n", dist_filter);
        const float _speed_of_sound = 340.29f;  // meters per second

        dist_filter = min(1.0f, max(0.0f, dist_filter));
        // SoundGroupRef sgr = SoundGroups::Instance()->ReturnRef(me->soundgroup);
        SoundGroupRef sgr = Engine::Instance()->GetAssetManager()->LoadSync<SoundGroup>(me->soundgroup);
        SoundGroupPlayInfo sgpi(*sgr, event_pos);
        sgpi.gain = gain;
        sgpi.pitch_shift = pitch_shift;
        sgpi.play_past_tick = SDL_TS_GetTicks() + 1000 * (unsigned int)(dist / _speed_of_sound);
        sgpi.occlusion_position = event_pos + normal * 0.3f;
        if (dist_filter < 1.0f) {
            sgpi.max_gain = 1.0f - dist_filter;
            unsigned long sound_handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
            Engine::Instance()->GetSound()->PlayGroup(sound_handle, sgpi);
            if (me->attached) {
                attached_sounds_.push_back(sound_handle);
            }
        }

        if (dist_filter > 0.0f) {
            sgpi.filter_info.path = "Data/Sounds/filters/dist_lowpass.wav";
            sgpi.filter_info.type = _simple_filter;
            sgpi.max_gain = dist_filter;
            unsigned long sound_handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
            Engine::Instance()->GetSound()->PlayGroup(sound_handle, sgpi);
            if (me->attached) {
                attached_sounds_.push_back(sound_handle);
            }
        }
    }
}

void ItemObject::MakeBulletObject() {
    if (!bullet_object_ && scenegraph_) {
        bullet_object_ = scenegraph_->bullet_world_->CreateConvexModel(
            vec3(0.0f),
            ofc_->model_name,
            &display_phys_offset_);
        bullet_object_->keep_history = true;
        Model& model = Models::Instance()->GetModel(model_id_);
        phys_offset_ = model.old_center + display_phys_offset_;
        bullet_object_->owner_object = this;
        vec3 new_pos = GetTranslation() +
                       GetRotation() * display_phys_offset_;
        bullet_object_->SetTransform(new_pos, Mat4FromQuaternion(GetRotation()), vec4(1.0f));
        bullet_object_->SetMass(item_ref_->GetMass());
        bullet_object_->body->setCcdMotionThreshold(0.000001f);
        bullet_object_->body->setCcdSweptSphereRadius(0.05f);

        abstract_bullet_object_ = scenegraph_->abstract_bullet_world_->CreateConvexModel(
            vec3(0.0f),
            ofc_->model_name,
            &display_phys_offset_,
            BW_ABSTRACT_ITEM);
        abstract_bullet_object_->body->setCollisionFlags(abstract_bullet_object_->body->getCollisionFlags() |
                                                         btCollisionObject::CF_NO_CONTACT_RESPONSE);
        abstract_bullet_object_->keep_history = true;
        abstract_bullet_object_->owner_object = this;
        abstract_bullet_object_->SetTransform(new_pos, Mat4FromQuaternion(GetRotation()), vec4(1.0f));
        abstract_bullet_object_->body->setInterpolationWorldTransform(abstract_bullet_object_->body->getWorldTransform());
        abstract_bullet_object_->Activate();
    }
}

bool ItemObject::Initialize() {
    update_list_entry = scenegraph_->LinkUpdateObject(this);
    MakeBulletObject();
    flare_ = scenegraph_->flares.MakeFlare(GetTranslation(), 1.0f, true);
    Moved(Object::kAll);
    bullet_object_->UpdateTransform();

    const Model& item_model = Models::Instance()->GetModel(model_id_);
    vec3 min = item_model.min_coords;
    vec3 max = item_model.max_coords;
    box_.center = (max + min) * 0.5f;
    box_.dims = max - min;

    return true;
}

vec3 ItemObject::GetTransformedVertex(int vert_id) {
    Model* model = &Models::Instance()->GetModel(model_id_);
    const mat4& transform = bullet_object_->GetTransform();
    vec3 vec = transform * vec3(model->vertices[vert_id * 3 + 0],
                                model->vertices[vert_id * 3 + 1],
                                model->vertices[vert_id * 3 + 2]);
    return vec;
}

ItemObject::ItemObject() : thrown_(kSafe),
                           active_time_(0.0f),
                           sun_ray_clear_(-1.0f),
                           new_clear_(0.0f),
                           shadow_group_id_(-1),
                           whoosh_sound_handle_(0),
                           impact_sound_delay_(0.0f),
                           whoosh_sound_delay_(0.0f),
                           whoosh_volume_(0.0f),
                           whoosh_pitch_(0.0f),
                           using_physics_(true),
                           bullet_object_(NULL),
                           interp_count_(1),
                           interp_period_(1),
                           stuck_in_environment_(false),
                           sticking_collision_occured_(false),
                           flare_(NULL),
                           flashing_(false),
                           flash_delay_(0.0f),
                           last_held_char_id_(-1),
                           char_impact_delay_(0.0f),
                           blood_surface_getter_(this),
                           blood_surface_(&blood_surface_getter_),
                           abstract_bullet_object_(NULL) {
    permission_flags &= ~Object::CAN_SCALE;

    batch_.gl_state.depth_test = true;
    batch_.gl_state.cull_face = true;
    batch_.gl_state.depth_write = true;
    batch_.gl_state.blend = false;

    depth_batch_.gl_state = batch_.gl_state;

    batch_.AddUniformFloat("fade", 0.0f);

    batch_.use_cam_pos = true;
    batch_.use_light = true;

    Textures* ti = Textures::Instance();

    batch_.texture_ref[4] = ti->GetBlankTextureRef();
    batch_.texture_ref[5] = ti->GetBlankTextureRef();

    texture_spatterdecal_ = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/spatterdecal.tga");
}

ItemObject::~ItemObject() {
    InvalidateReaders();
    scenegraph_->bullet_world_->RemoveObject(&bullet_object_);
    if (abstract_bullet_object_) {
        scenegraph_->abstract_bullet_world_->RemoveObject(&abstract_bullet_object_);
        abstract_bullet_object_ = NULL;
    }
    if (flare_) {
        scenegraph_->flares.DeleteFlare(flare_);
    }

    if (Engine::Instance()->GetSound()->IsHandleValid(whoosh_sound_handle_)) {
        Engine::Instance()->GetSound()->Stop(whoosh_sound_handle_);
    }
    blood_surface_.Dispose();
}

void ItemObject::Moved(Object::MoveType type) {
    Object::Moved(type);
    if (bullet_object_) {
        vec3 new_pos = GetTranslation() +
                       GetRotation() * display_phys_offset_;
        bullet_object_->SetTransform(new_pos, Mat4FromQuaternion(GetRotation()), vec4(1.0f));
        bullet_object_->SetLinearVelocity(vec3(0.0f));
        bullet_object_->SetAngularVelocity(vec3(0.0f));
        bullet_object_->UpdateTransform();
        bullet_object_->Activate();
        thrown_ = kThrown;
        if (stuck_in_environment_) {
            scenegraph_->bullet_world_->LinkObject(bullet_object_);
        }
        stuck_in_environment_ = false;
        sticking_collision_occured_ = false;
    }
}

void ItemObject::Reset() {
    Moved(Object::kAll);
    blood_surface_.CleanBlood();
}

void ItemObject::Dispose() {
    Object::Dispose();
    InvalidateReaders();
}

void ItemObject::CleanBlood() {
    blood_surface_.CleanBlood();
}

void ItemObject::SetLinearVelocity(vec3 vel) {
    bullet_object_->SetLinearVelocity(vel);
}

void ItemObject::Update(float timestep) {
    Online* online = Online::Instance();
    bool fetchedData = false;
    if (online->IsClient()) {
        std::list<ItemObjectFrame>& frames = incoming_online_item_frames;
        while (frames.size() > 1) {
            network_time_interpolator.timestamps.Clear();
            for (const ItemObjectFrame& frame : frames) {
                network_time_interpolator.timestamps.PushValue(frame.host_walltime);
            }

            int interpolator_result = network_time_interpolator.Update();

            if (interpolator_result == 1) {
                continue;
            } else if (interpolator_result == 2) {
                frames.pop_front();
                continue;
            } else if (interpolator_result == 3) {
                break;
            }

            mat4& current_transform = frames.begin()->transform;
            mat4& next_transform = std::next(frames.begin())->transform;

            vec3 trans_current = current_transform.GetTranslationPart();
            vec3 trans_next = next_transform.GetTranslationPart();

            quaternion quat_from_current = QuaternionFromMat4(current_transform.GetRotationPart());
            quaternion quat_from_next = QuaternionFromMat4(next_transform.GetRotationPart());

            vec3 translation = lerp(trans_current, trans_next, network_time_interpolator.interpolation_step);
            quaternion rotation = Slerp(quat_from_current, quat_from_next, network_time_interpolator.interpolation_step);

            transform.SetTranslationPart(translation);
            transform.SetRotationPart(Mat4FromQuaternion(rotation));

            break;
        }

        // Temp: Disable all updates for clients in regards to items, as we are not really sure what it means to run all of this code here.
        return;
    }

    impact_sound_delay_ = max(0.0f, impact_sound_delay_ - timestep);
    whoosh_sound_delay_ = max(0.0f, whoosh_sound_delay_ - timestep);

    if (bullet_object_->IsActive()) {
        float val = length(bullet_object_->GetLinearVelocity()) * 0.02f - 0.02f;
        if (StuckInWhom() != -1) {
            val = 0.0f;
        }
        bullet_object_->SetMargin(max(0.0f, min(0.05f, val)));
        vec3 ang_vel = bullet_object_->GetAngularVelocity();
        const float max_ang_vel = 15.0f;
        if ((thrown_ != kThrown && thrown_ != kThrownStraight) && length_squared(ang_vel) > max_ang_vel * max_ang_vel) {
            bullet_object_->SetAngularVelocity(normalize(ang_vel) * max_ang_vel);
        }
        active_time_ += timestep;
        if (!Engine::Instance()->GetSound()->IsHandleValid(whoosh_sound_handle_)) {
            SoundPlayInfo spi;
            spi.looping = true;
            spi.path = "Data/Sounds/windmono.wav";
            spi.position = bullet_object_->GetPosition();
            spi.occlusion_position = spi.position;

            // Disable whoosh handle
            // whoosh_sound_handle_ = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
            // Engine::Instance()->GetSound()->Play(whoosh_sound_handle_, spi);
        }
    } else {
        active_time_ = 0.0f;
        if (Engine::Instance()->GetSound()->IsHandleValid(whoosh_sound_handle_)) {
            Engine::Instance()->GetSound()->Stop(whoosh_sound_handle_);
        }
    }

    // printf("Active time: %f\n Owner: %d\n", active_time, last_held_char_id);

    // Orient spear to face in the direction that it is flying
    vec3 target_dir = normalize(bullet_object_->GetLinearVelocity());
    if (length_squared(target_dir) != 0.0f && thrown_ == kThrownStraight && bullet_object_->IsActive()) {
        mat4 rot = bullet_object_->GetRotation();                                           // Get the current rotation of the object
        vec3 dir = rot * normalize(item_ref_->GetLineEnd(0) - item_ref_->GetLineStart(0));  // Get the direction it is currently pointing
        vec3 perp = cross(dir, target_dir);                                                 // Find a vector perpendicular to both the current direction and the target direction
        float angle = acosf(dot(dir, target_dir));                                          // Get angle needed to rotate current direction to target
        quaternion quat(vec4(perp, angle));                                                 // Create angle-axis rotation
        mat4 quat_mat = Mat4FromQuaternion(quat);
        bullet_object_->SetRotation(quat_mat * rot);
        bullet_object_->SetAngularVelocity(vec3(0.0f));
    }

    const Model& model = Models::Instance()->GetModel(model_id_);
    vec3 tip_vec;
    vec3 dims = model.max_coords - model.min_coords;
    if (dims[0] > dims[1] && dims[0] > dims[2]) {
        tip_vec = vec3(dims[0] * 0.5f, 0.0f, 0.0f);
        if (display_phys_offset_[0] > 0.0f) {
            tip_vec *= -1.0f;
        }
    } else if (dims[1] > dims[2]) {
        tip_vec = vec3(0.0f, dims[1] * 0.5f, 0.0f);
        if (display_phys_offset_[1] > 0.0f) {
            tip_vec *= -1.0f;
        }
    } else {
        tip_vec = vec3(0.0f, 0.0f, dims[2] * 0.5f);
        if (display_phys_offset_[2] > 0.0f) {
            tip_vec *= -1.0f;
        }
    }

    tip_vec -= display_phys_offset_;
    // vec3 local_tip_vec = tip_vec;
    vec3 rotated_tip_pos = bullet_object_->GetRotation() * tip_vec;

    vec3 tip_pos = bullet_object_->GetTransform() * tip_vec;
    /*DebugDraw::Instance()->AddLine(bullet_object->GetPosition(),
                                   tip_pos,
                                   vec4(1.0f),
                                   _delete_on_update,
                                   _DD_XRAY);*/
    Engine::Instance()->GetSound()->SetPosition(whoosh_sound_handle_, tip_pos);
    Engine::Instance()->GetSound()->SetOcclusionPosition(whoosh_sound_handle_, tip_pos);
    vec3 ang_lin_vel = cross(bullet_object_->GetAngularVelocity(), rotated_tip_pos);
    vec3 tip_vel = ang_lin_vel + bullet_object_->GetLinearVelocity();
    /*DebugDraw::Instance()->AddLine(tip_pos,
        tip_pos + tip_vel,
        vec4(1.0f),
        _delete_on_update,
        _DD_XRAY);*/
    Engine::Instance()->GetSound()->SetVelocity(whoosh_sound_handle_, tip_vel * 2.0f);
    float speed = min(length(tip_vel),
                      length(tip_vel - ActiveCameras::Get()->GetVelocity()));
    float pitch = 0.1f / (bullet_object_->GetMass()) * (1.0f + speed * 5.0f);
    const float _whoosh_volume_inertia = 0.9f;
    const float _whoosh_pitch_inertia = 0.9f;
    whoosh_pitch_ = mix(pitch, whoosh_pitch_, _whoosh_pitch_inertia);
    whoosh_pitch_ = min(20.0f, whoosh_pitch_);
    Engine::Instance()->GetSound()->SetPitch(whoosh_sound_handle_, whoosh_pitch_);
    float volume = speed * 0.05f;
    if (!bullet_object_->IsActive()) {
        volume *= 0.12f;
    }
    whoosh_volume_ = mix(volume, whoosh_volume_, _whoosh_volume_inertia);
    Engine::Instance()->GetSound()->SetVolume(whoosh_sound_handle_, whoosh_volume_);

    /*if(whoosh_sound_delay <= 0.0f){
        vec3 ang_vel = bullet_object->GetAngularVelocity();
        const float ang_vel_threshold = 10.0f;
        if(length_squared(ang_vel)>ang_vel_threshold*ang_vel_threshold){
            scenegraph_->sound->PlayGroup(
                "Data/Sounds/sword/light_weapon_swoosh.xml",
                bullet_object->GetPosition(),
                length(ang_vel)/ang_vel_threshold,
                length(ang_vel)/ang_vel_threshold);
            whoosh_sound_delay = 0.2f;
        }
    }*/

    /*mat4 transform = bullet_object->GetInterpRotationX(interp_period,interp_period-interp_count);
    vec3 pos = bullet_object->GetInterpPositionX(interp_period,interp_period-interp_count);
    transform.SetTranslationPart(pos);
    vec3 temp = transform * vec3(0.0f);
    DebugDraw::Instance()->AddLine(temp-vec3(0.0f,0.1f,0.0f),
                  temp+vec3(0.0f,0.1f,0.0f),
                  vec4(1.0f),
                  _delete_on_update);
    DebugDraw::Instance()->AddLine(temp-vec3(0.1f,0.0f,0.0f),
                  temp+vec3(0.1f,0.0f,0.0f),
                  vec4(1.0f),
                  _delete_on_update);
    DebugDraw::Instance()->AddLine(temp-vec3(0.0f,0.0f,0.1f),
                  temp+vec3(0.0f,0.0f,0.1f),
                  vec4(1.0f),
                  _delete_on_update);*/
    old_lin_vel_ = bullet_object_->GetLinearVelocity();
    old_ang_vel_ = bullet_object_->GetAngularVelocity();

    if (sticking_collision_occured_) {
        StickToEnvironment(stuck_pos_, stuck_vert_, stuck_collide_info_);
    }
    if (!enabled_ && flare_) {
        // TODO: This is a hack.
        //       This Update should not be called if the item object is not enabled.
        //       Need to implement a SetEnabled override to make that work.
        //       Would also disable the physics object, etc, inside that function. Maybe the flare too?
        flare_->visible_mult = 0.0f;
    } else if (flare_) {
        const float _flash_threshold = 0.0f;  // 4.0f;
        if (!flashing_) {
            flare_->visible_mult = 0.0f;
            flash_delay_ -= timestep;
            if (!IsHeld() &&
                flash_delay_ <= 0.0f &&
                rand() % 200 == 0 &&
                distance_squared(ActiveCameras::Get()->GetPos(), bullet_object_->GetPosition()) > square(_flash_threshold)) {
                /*bool movement_flash = false;
                if(length_squared(bullet_object_->GetLinearVelocity())>1.0f ||
                   length_squared(bullet_object_->GetAngularVelocity())>1.0f ||
                   length_squared(ActiveCameras::Get()->GetVelocity())>1.0f){
                       movement_flash =true;
                }
                if(movement_flash){*/
                flashing_ = true;
                flash_progress_ = 0.0f;
                //}
            }
        } else {
            flash_progress_ += timestep * 8.0f;
            if (flash_progress_ > 1.0f) {
                flashing_ = false;
                flash_delay_ = 4.0f;
                flare_->visible_mult = 0.0f;
            }
        }

        if (flashing_) {
            const Model& model = Models::Instance()->GetModel(model_id_);
            int longest_axis = -1;
            float longest_length = 0.0f;
            for (unsigned i = 0; i < 3; ++i) {
                float ax_len = model.max_coords[i] - model.min_coords[i];
                if (longest_axis == -1 || ax_len > longest_length) {
                    longest_length = ax_len;
                    longest_axis = i;
                }
            }
            vec3 axis_vec;
            axis_vec[longest_axis] = 1.0f;
            flare_->position = bullet_object_->GetTransform() *
                               (axis_vec * mix(model.min_coords[longest_axis], model.max_coords[longest_axis], flash_progress_) - display_phys_offset_);
            flare_->position += normalize(ActiveCameras::Get()->GetPos() - flare_->position) * 0.05f;
            // flare->size_mult = 1.0f/max(1.0f,distance(ActiveCameras::Get()->GetPos(), flare->position));
            // flare->size_mult = sqrtf(flare->size_mult);
            flare_->size_mult = 0.1f;
            flare_->visible_mult = sinf(flash_progress_ * 3.1415f);

            flare_->distant = false;
            flare_->color = vec3(0.8f, 0.8f, 1.0f);
        }
    }
    if (stuck_in_environment_) {
        // bullet_object->SetTransform(stuck_transform);
        bullet_object_->UpdateTransform();
    }
    // blood_surface_.Update(scenegraph_, timestep);

    mat4 cur_transform = bullet_object_->GetTransform();
    if (bullet_object_->IsActive()) {
        // Check thrown collisions
        mat4 cur_transform = bullet_object_->GetTransform();
        char_impact_delay_ = max(0.0f, char_impact_delay_ - timestep);
        for (unsigned j = 0; j <= 8; ++j) {
            if (char_impact_delay_ == 0.0f && length_squared(bullet_object_->GetLinearVelocity()) > 1.0f) {
                float offset = j * 0.125f;
                mat4 transform = mix(old_transform_, cur_transform, offset);

                size_t num_lines = item_ref_->GetNumLines();
                for (size_t i = 0; i < num_lines; ++i) {
                    CheckThrownCollisionLine((int)i, cur_transform, transform);
                }
            }
        }
    } else {
        char_impact_delay_ = 0.0f;
    }
    old_transform_ = cur_transform;

    if (abstract_bullet_object_) {
        abstract_bullet_object_->SetTransform(bullet_object_->GetTransform());
        abstract_bullet_object_->body->setInterpolationWorldTransform(abstract_bullet_object_->body->getWorldTransform());
        abstract_bullet_object_->UpdateTransform();
    }
}

void ItemObject::SetThrown() {
    thrown_ = kThrown;
}

void ItemObject::SetThrownStraight() {
    thrown_ = kThrownStraight;
}

static void DrawItemModel(Model& model, Graphics* graphics, Shaders* shaders, int shader) {
    PROFILER_GPU_ZONE(g_profiler_ctx, "DrawItemModel");
    if (!model.vbo_loaded) {
        model.createVBO();
    }
    model.VBO_faces.Bind();
    static const int kNumAttribs = 3;
    int attrib_ids[kNumAttribs];
    for (int i = 0; i < kNumAttribs; ++i) {
        const char* attrib_str;
        int num_el;
        VBOContainer* vbo;
        switch (i) {
            case 0:
                attrib_str = "vertex_attrib";
                num_el = 3;
                vbo = &model.VBO_vertices;
                break;
            case 1:
                attrib_str = "tex_coord_attrib";
                num_el = 2;
                vbo = &model.VBO_tex_coords;
                break;
            case 2:
                attrib_str = "normal_attrib";
                num_el = 3;
                vbo = &model.VBO_normals;
                break;
            default:
                __builtin_unreachable();
                break;
        }
        CHECK_GL_ERROR();
        attrib_ids[i] = shaders->returnShaderAttrib(attrib_str, shader);
        CHECK_GL_ERROR();
        if (attrib_ids[i] != -1) {
            vbo->Bind();
            graphics->EnableVertexAttribArray(attrib_ids[i]);
            CHECK_GL_ERROR();
            glVertexAttribPointer(attrib_ids[i], num_el, GL_FLOAT, false, num_el * sizeof(GLfloat), 0);
            CHECK_GL_ERROR();
        }
    }
    graphics->DrawElements(GL_TRIANGLES, model.faces.size(), GL_UNSIGNED_INT, 0);
    graphics->ResetVertexAttribArrays();
    graphics->BindArrayVBO(0);
    graphics->BindElementVBO(0);
}

void ItemObject::Draw() {
    if (g_debug_runtime_disable_item_object_draw) {
        return;
    }

    Camera* cam = ActiveCameras::Get();
    mat4 proj_view_matrix = cam->GetProjMatrix() * cam->GetViewMatrix();
    DrawItem(proj_view_matrix, Object::kFullDraw);
}

void ItemObject::PreDrawFrame(float curr_game_time) {
    if (g_debug_runtime_disable_item_object_pre_draw_frame) {
        return;
    }

    PROFILER_GPU_ZONE(g_profiler_ctx, "ItemObject::PreDrawFrame");
    batch_.texture_ref[2] = scenegraph_->sky->GetSpecularCubeMapTexture();

    shadow_group_id_ = -1;
    blood_surface_.PreDrawFrame(Textures::Instance()->getWidth(batch_.texture_ref[0]) / 4,
                                Textures::Instance()->getHeight(batch_.texture_ref[0]) / 4);

    vec3 avg_pos = GetPhysicsPosition();
    static const float _sun_check_threshold = 0.5f;
    if (distance_squared(avg_pos, last_sun_checked_) > _sun_check_threshold) {
        new_clear_ = (scenegraph_->bullet_world_->CheckRayCollision(
                          avg_pos, avg_pos + scenegraph_->primary_light.pos * 1000) == NULL)
                         ? 1.0f
                         : 0.0f;
        last_sun_checked_ = avg_pos;
    }
    if (sun_ray_clear_ >= 0.0f) {
        sun_ray_clear_ = mix(new_clear_, sun_ray_clear_, pow(0.95f, game_timer.updates_since_last_frame));
    } else {
        sun_ray_clear_ = new_clear_;
    }
}

void ItemObject::DrawDepthMap(const mat4& proj_view_matrix, const vec4* cull_planes, int num_cull_planes, Object::DrawType draw_type) {
    if (g_debug_runtime_disable_item_object_draw_depth_map) {
        return;
    }

    DrawItem(proj_view_matrix, draw_type);
}

int ItemObject::model_id() const {
    return model_id_;
}

void ItemObject::Copied() {
    bullet_object_ = NULL;
    readers_.clear();
    shadow_group_id_ = -1;
}

void ItemObject::AddReader(ItemObjectScriptReader* reader) {
    if (reader->holding) {
        std::list<ItemObjectScriptReader*> to_invalidate;
        for (auto& reader : readers_) {
            if (reader->stuck || reader->holding) {
                to_invalidate.push_back(reader);
            }
        }
        for (auto& iter : to_invalidate) {
            iter->Invalidate();
        }
    }
    readers_.push_back(reader);
}

void ItemObject::RemoveReader(ItemObjectScriptReader* reader) {
    std::list<ItemObjectScriptReader*>::iterator iter =
        std::find(readers_.begin(), readers_.end(), reader);
    if (iter != readers_.end()) {
        readers_.erase(iter);
    }
}

void ItemObject::SetPosition(const vec3& pos) {
    if (bullet_object_) {
        bullet_object_->SetPosition(pos);
    }
}

vec3 ItemObject::GetPhysicsPosition() {
    if (bullet_object_) {
        return bullet_object_->GetPosition();
    }
    return vec3(0.0f);
}

void ItemObject::SetPhysicsTransform(const mat4& transform) {
    using_physics_ = false;
    if (bullet_object_) {
        vec3 new_pos = transform.GetTranslationPart() +
                       transform.GetRotatedvec3(phys_offset_);
        bullet_object_->SetRotationAndVel(transform, interp_period_);
        bullet_object_->SetPositionAndVel(new_pos, interp_period_);
        bullet_object_->SetRotation(transform);
        bullet_object_->SetPosition(new_pos);
        bullet_object_->Sleep();
        bullet_object_->UpdateTransform();
        scenegraph_->bullet_world_->UnlinkObject(bullet_object_);
        if (stuck_in_environment_) {
            scenegraph_->bullet_world_->LinkObject(bullet_object_);
        }
        stuck_in_environment_ = false;
        sticking_collision_occured_ = false;
    }
}

void ItemObject::SetVelocities(const vec3& linear_vel, const vec3& angular_vel) {
    if (bullet_object_) {
        bullet_object_->SetLinearVelocity(linear_vel);
        bullet_object_->SetAngularVelocity(angular_vel);
    }
}

void ItemObject::ActivatePhysics() {
    if (using_physics_) {
        return;
    }
    using_physics_ = true;
    if (bullet_object_) {
        thrown_ = kSafe;
        bullet_object_->Activate();
        interp_period_ = 1;
        interp_count_ = 1;
        scenegraph_->bullet_world_->LinkObject(bullet_object_);
        old_transform_ = bullet_object_->GetTransform();
    }
}

void ItemObject::SetInterpolation(int count, int period) {
    interp_count_ = count;
    interp_period_ = period;
}

void ItemObject::GetDesc(EntityDescription& desc) const {
    Object::GetDesc(desc);
    desc.AddString(EDF_FILE_PATH, obj_file);
    color_tint_component_.AddToDescription(desc);
}

void ItemObject::StickingCollisionOccured(const vec3& pos, int vert, const CollideInfo& collide_info) {
    sticking_collision_occured_ = true;
    stuck_pos_ = pos;
    stuck_vert_ = vert;
    stuck_collide_info_ = collide_info;
}

void ItemObject::StickToEnvironment(const vec3& pos, int vert, const CollideInfo& collide_info) {
    sticking_collision_occured_ = false;
    stuck_in_environment_ = true;
    vec3 vert_pos = ModelToWorldPos(GetVertPos(vert)) - bullet_object_->GetPosition();
    bullet_object_->SetPosition(pos - vert_pos - collide_info.normal * 0.15f * pow(item_ref_->GetMass(), 0.5f));
    bullet_object_->SetLinearVelocity(vec3(0.0f));
    bullet_object_->SetAngularVelocity(vec3(0.0f));
    bullet_object_->SetMargin(0.0f);
    scenegraph_->bullet_world_->UnlinkObject(bullet_object_);
    stuck_transform_ = bullet_object_->GetTransform();
}

vec3 ItemObject::WorldToModelPos(const vec3& pos) {
    return invert(bullet_object_->GetTransform()) * pos + display_phys_offset_;
}

vec3 ItemObject::ModelToWorldPos(const vec3& pos) {
    return bullet_object_->GetTransform() * (pos - display_phys_offset_);
}

int ItemObject::GetClosestVert(const vec3& pos) {
    const Model& model = Models::Instance()->GetModel(model_id_);
    vec3 local_pos = WorldToModelPos(pos);
    int closest_vert = -1;
    float closest_dist;
    unsigned index = 0;
    for (int i = 0, len = model.vertices.size() / 3; i < len; ++i) {
        float dist_squared =
            square(model.vertices[index + 0] - local_pos[0]) +
            square(model.vertices[index + 1] - local_pos[1]) +
            square(model.vertices[index + 2] - local_pos[2]);
        if (closest_vert == -1 || dist_squared < closest_dist) {
            closest_dist = dist_squared;
            closest_vert = i;
        }
        index += 3;
    }
    return closest_vert;
}

vec3 ItemObject::GetVertPos(int vert) {
    const Model& model = Models::Instance()->GetModel(model_id_);
    return vec3(model.vertices[vert * 3 + 0],
                model.vertices[vert * 3 + 1],
                model.vertices[vert * 3 + 2]);
}

float ItemObject::GetVertSharpness(int vert) {
    const Model& model = Models::Instance()->GetModel(model_id_);
    if (model.aux.empty()) {
        return 0.0f;
    }
    return model.aux[vert * 3 + 0];
}

ItemObject::WeightClass ItemObject::GetWeightClass() {
    WeightClass weight_class = kLightWeight;
    float mass = bullet_object_->GetMass();
    if (mass >= kHeavyThreshold) {
        weight_class = kHeavyWeight;
    } else if (mass >= kMediumThreshold) {
        weight_class = kMediumWeight;
    }
    return weight_class;
}

void ItemObject::PlayImpactSound(const vec3& pos, const vec3& normal, float impulse) {
    WeightClass weight_class = GetWeightClass();

    Online* online = Online::Instance();

    if (online->IsHosting()) {
        online->Send<OnlineMessages::AudioPlaySoundImpactItemMessage>(GetID(), pos, normal, impulse);
    }

    switch (weight_class) {
        case kLightWeight:
            HandleItemObjectMaterialEvent("weapon_drop_light", normal, pos, impulse);
            break;
        case kMediumWeight:
            HandleItemObjectMaterialEvent("weapon_drop_medium", normal, pos, impulse);
            break;
        case kHeavyWeight:
            HandleItemObjectMaterialEvent("weapon_drop_heavy", normal, pos, impulse);
            break;
    }
    {  // Notify nearby characters
        const int kBufSize = 512;
        char msg[kBufSize];
        FormatString(msg, kBufSize, "nearby_sound %f %f %f %f %d %d", pos[0], pos[1], pos[2], 10.0f, last_held_char_id_, -1);

        ContactInfoCallback cb;
        scenegraph_->abstract_bullet_world_->GetSphereCollisions(pos, 10.0f, cb);
        for (int i = 0; i < cb.contact_info.size(); ++i) {
            BulletObject* bo = cb.contact_info[i].object;
            if (bo && bo->owner_object && scenegraph_->IsObjectSane(bo->owner_object) && bo->owner_object->GetType() == _movement_object) {
                int id = bo->owner_object->GetID();
                ((MovementObject*)bo->owner_object)->ReceiveScriptMessage(msg);
            }
        }
    }

    impact_sound_delay_ = 0.2f;
}

mat4 ItemObject::GetPhysicsTransform() {
    mat4 transform = bullet_object_->GetTransform();
    // transform.AddTranslation(transform.GetRotatedvec3(phys_offset)*-1.0f);
    return transform;
}

mat4 ItemObject::GetPhysicsTransformIncludeOffset() {
    mat4 transform = bullet_object_->GetTransform();
    transform.AddTranslation(transform.GetRotatedvec3(phys_offset_) * -1.0f);
    return transform;
}

void ItemObject::GetPhysicsVel(vec3& linear_vel, vec3& angular_vel) {
    linear_vel = bullet_object_->GetLinearVelocity();
    angular_vel = bullet_object_->GetAngularVelocity();
}

bool ItemObject::IsHeld() {
    bool held = false;
    for (auto& reader : readers_) {
        if (reader->holding) {
            held = true;
        }
    }
    return held;
}

bool ItemObject::IsConstrained() {
    bool constrained = false;
    for (auto& reader : readers_) {
        if (reader->constraint) {
            constrained = true;
        }
    }
    return constrained;
}

bool ItemObject::IsStuckInCharacter() {
    bool stuck = false;
    for (auto& reader : readers_) {
        if (reader->stuck) {
            stuck = true;
        }
    }
    return stuck;
}

int ItemObject::StuckInWhom() {
    int stuck_id = -1;
    for (auto& reader : readers_) {
        if (reader->stuck && reader->char_id != -1) {
            stuck_id = reader->char_id;
        }
    }
    return stuck_id;
}

int ItemObject::HeldByWhom() {
    int held_id = -1;
    for (auto& reader : readers_) {
        if (reader->holding && reader->char_id != -1) {
            held_id = reader->char_id;
        }
    }
    return held_id;
}

void ItemObject::InvalidateReaders() {
    std::list<ItemObjectScriptReader*> temp_readers = readers_;
    for (std::list<ItemObjectScriptReader*>::iterator iter = temp_readers.begin();
         iter != temp_readers.end();) {
        ItemObjectScriptReader* reader = (*iter);
        reader->Invalidate();
        ++iter;
    }
    readers_.clear();
}

void ItemObject::AddBloodDecal(vec3 pos, vec3 dir, float size) {
    if (Graphics::Instance()->config_.blood() == BloodLevel::kNone) {
        return;
    }
    pos -= bullet_object_->GetRotation() * phys_offset_;
    Model& model = Models::Instance()->GetModel(model_id_);
    std::vector<int> hit_list;
    hit_list.resize(model.faces.size() / 3);
    for (unsigned i = 0; i < hit_list.size(); ++i) {
        hit_list[i] = i;
    }
    blood_surface_.AddDecalToTriangles(hit_list, pos, dir, size, texture_spatterdecal_);
}

bool ItemObject::ConnectTo(Object& other, bool checking_other /*= false*/) {
    if (other.GetType() == _movement_object) {
        return other.ConnectTo(*this, true);
    } else if (other.GetType() == _hotspot_object) {
        return Object::ConnectTo(other, checking_other);
    } else {
        return other.GetType() == _hotspot_object;
    }
}

bool ItemObject::Disconnect(Object& other, bool from_socket /*= false*/, bool checking_other /*= false*/) {
    if (other.GetType() == _movement_object) {
        return other.Disconnect(*this, from_socket, true);
    } else {
        return Object::Disconnect(other, from_socket, checking_other);
    }
}

void ItemObject::SetHolderID(int char_id) {
    last_held_char_id_ = char_id;
    // printf("Item %d now held by %d\n", GetID(), char_id);
}

ItemObject::ItemState ItemObject::state() const {
    return state_;
}

void ItemObject::SetState(ItemState _state) {
    state_ = _state;
}

const bool _debug_draw_collision = false;

bool ItemObject::CheckThrownSafe() const {
    return thrown_ == kSafe;
}

void ItemObject::CheckThrownCollisionLine(int line_id, const mat4& cur_transform, const mat4& transform) {
    vec3 point, normal, start, end;
    start = transform * item_ref_->GetLineStart(line_id);
    end = transform * item_ref_->GetLineEnd(line_id);
    int bone = -1;
    int char_id = scenegraph_->CheckRayCollisionCharacters(start,
                                                           end,
                                                           &point,
                                                           &normal,
                                                           &bone);
    if (char_id == last_held_char_id_) {  // && active_time < 0.1f){
        return;
    }
    vec4 color(1.0f);
    if (item_ref_->GetLineMaterial(line_id) == "wood") {
        color = vec4(105 / 255.0f, 77 / 255.0f, 50 / 255.0f, 1.0f);
    }
    if (char_id != -1 && !IsHeld() && !IsStuckInCharacter()) {
        if (_debug_draw_collision) {
            DebugDraw::Instance()->AddWireSphere(point, 0.2f, color, _fade);
        }
        Object* obj = scenegraph_->GetObjectFromID(char_id);
        MovementObject* mo = (MovementObject*)obj;

        if (thrown_ != kSafe) {
            int accept_hit = 1;
            {
                accept_hit = mo->AboutToBeHitByItem(GetID());
            }
            if (accept_hit == 1) {
                bool will_cut = false;
                int vert = GetClosestVert(point + normalize(GetLinearVelocity()));
                float sharpness = GetVertSharpness(vert);
                if (sharpness > 0.3f && item_ref_->GetLineMaterial(line_id) == "metal") {
                    will_cut = true;
                }
                bool will_stick = false;
                if (will_cut) {
                    mo->rigged_object()->Stab(point, normalize(end - start), _heavy, 0);
                    AddBloodDecal(cur_transform * invert(transform) * point, normalize(end - start), 0.2f);

                    if (item_ref_->GetLabel() == "spear") {
                        will_stick = true;
                    }
                    if (fabs(dot(normalize(end - start), normal)) > 0.4f) {
                        will_stick = true;
                    }
                    if (will_stick) {
                        mat4 stick_transform = transform;
                        stick_transform.AddTranslation(point - end);
                        stick_transform.AddTranslation(transform.GetRotatedvec3(phys_offset_ * -1.0f));
                        float stick_depth = 0.4f;
                        stick_transform.AddTranslation(normalize(end - start) * stick_depth);

                        if (bone != -1) {
                            // stick_transform = mo->rigged_object()->skeleton_.physics_bones[bone].bullet_object->GetTransform();
                            mo->rigged_object()->StickItem(GetID(), start, end, stick_transform);
                        }

                        std::string path = "Data/Sounds/hit/hit_";
                        WeightClass weight_class = GetWeightClass();
                        switch (weight_class) {
                            // case _light_weight: path += "light"; break;
                            case kLightWeight:
                                path = "Data/Sounds/weapon_foley/impact/weapon_knife_hit";
                                break;
                            case kMediumWeight:
                                path += "medium";
                                break;
                            case kHeavyWeight:
                                path += "hard";
                                break;
                        }
                        path += ".xml";
                        if (item_ref_->GetLabel() == "spear") {
                            path = "Data/Sounds/hit/spear_hit_flesh.xml";
                        }
                        // SoundGroupRef sgr = SoundGroups::Instance()->ReturnRef(path);
                        SoundGroupRef sgr = Engine::Instance()->GetAssetManager()->LoadSync<SoundGroup>(path);
                        SoundGroupPlayInfo sgpi(*sgr, point);
                        unsigned long handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
                        Engine::Instance()->GetSound()->PlayGroup(handle, sgpi);
                    } else {
                        std::string path = "Data/Sounds/hit/hit_";
                        WeightClass weight_class = GetWeightClass();
                        switch (weight_class) {
                            // case _light_weight: path += "light"; break;
                            case kLightWeight:
                                path = "Data/Sounds/weapon_foley/impact/weapon_knife_hit_neck";
                                break;
                            case kMediumWeight:
                                path += "medium";
                                break;
                            case kHeavyWeight:
                                path += "hard";
                                break;
                        }
                        path += ".xml";
                        // SoundGroupRef sgr = SoundGroups::Instance()->ReturnRef(path);
                        SoundGroupRef sgr = Engine::Instance()->GetAssetManager()->LoadSync<SoundGroup>(path);
                        SoundGroupPlayInfo sgpi(*sgr, point);

                        unsigned long handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
                        Engine::Instance()->GetSound()->PlayGroup(handle, sgpi);
                    }
                } else {
                    // printf("Will not stick!\n");
                }
                int type = 0;
                if (will_stick) {
                    type = 2;
                } else if (will_cut) {
                    type = 1;
                }
                mo->HitByItem(GetID(), point, item_ref_->GetLineMaterial(line_id), type);
            }
        }
        thrown_ = kSafe;
        vec3 rand_vec;
        do {
            rand_vec = vec3(RangedRandomFloat(-1.0f, 1.0f),
                            RangedRandomFloat(-1.0f, 1.0f),
                            RangedRandomFloat(-1.0f, 1.0f));
        } while (length_squared(rand_vec) > 1.0f);
        rand_vec = normalize(rand_vec);
        vec3 vel = bullet_object_->GetLinearVelocity();
        vec3 reflect_norm = normalize(vel) * -1.0f;
        reflect_norm[1] *= 0.2f;
        reflect_norm = normalize(mix(rand_vec, reflect_norm, 0.6f));
        vel = reflect(vel, reflect_norm);
        vel *= RangedRandomFloat(0.1f, 0.25f);
        bullet_object_->SetLinearVelocity(vel);
        // bullet_object->SetAngularVelocity(bullet_object->GetAngularVelocity()*-0.5f);
        char_impact_delay_ = 0.4f;
        std::string path = "Data/Sounds/weapon_foley/impact/weapon_drop_";
        WeightClass weight_class = GetWeightClass();
        switch (weight_class) {
            // case _light_weight: path += "light_dirt.xml"; break;
            case kLightWeight:
                path = "Data/Sounds/weapon_foley/impact/weapon_knife_hilt.xml";
                break;
            case kMediumWeight:
                path += "medium_dirt.xml";
                break;
            case kHeavyWeight:
                path += "heavy_dirt.xml";
                break;
        }
        // SoundGroupRef sgr = SoundGroups::Instance()->ReturnRef(path);
        SoundGroupRef sgr = Engine::Instance()->GetAssetManager()->LoadSync<SoundGroup>(path);
        SoundGroupPlayInfo sgpi(*sgr, point);

        unsigned long handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
        Engine::Instance()->GetSound()->PlayGroup(handle, sgpi);
        if (_debug_draw_collision) {
            DebugDraw::Instance()->AddLine(obj->GetTranslation(), point, color, _fade);
        }
    }
    if (_debug_draw_collision) {
        DebugDraw::Instance()->AddLine(start, end, color, _fade);
    }
}

void ItemObject::DrawItem(const mat4& proj_view_matrix, Object::DrawType type) {
    PROFILER_ZONE(g_profiler_ctx, "ItemObject::DrawItem()");
    Shaders* shaders = Shaders::Instance();
    Graphics* graphics = Graphics::Instance();
    Camera* cam = ActiveCameras::Get();
    Model* model = &Models::Instance()->GetModel(model_id_);
    Textures* textures = Textures::Instance();
    Online* online = Online::Instance();
    // float time_scale = Timer::Instance()->time_scale;
    // float timestep = Timer::Instance()->timestep;

    CHECK_GL_ERROR();
    batch_.texture_ref[2] = scenegraph_->sky->GetSpecularCubeMapTexture();

    const int kShaderStrSize = 1024;
    char buf[2][kShaderStrSize];
    char* shader_str[2] = {buf[0], buf[1]};
    if (use_tangent_) {
        FormatString(shader_str[0], kShaderStrSize, "envobject #ITEM #TANGENT");
    } else {
        FormatString(shader_str[0], kShaderStrSize, "envobject #ITEM");
    }
    FormatString(shader_str[1], kShaderStrSize, "%s %s", shader_str[0], global_shader_suffix);
    std::swap(shader_str[0], shader_str[1]);
    if (g_simple_shadows || !g_level_shadows) {
        batch_.texture_ref[4] = graphics->static_shadow_depth_ref;
    } else {
        batch_.texture_ref[4] = graphics->cascade_shadow_depth_ref;
    }

    int the_shader = shaders->returnProgram(shader_str[0]);
    batch_.shader_id = the_shader;

    batch_.SetStartState();

    if (type == Object::kFullDraw) {
        TextureRef blood_tex = blood_surface_.blood_tex;
        if (blood_tex.valid()) {
            Textures::Instance()->bindTexture(blood_tex, 6);
        }
        scenegraph_->BindLights(the_shader);
        shaders->SetUniformFloat("in_light", sun_ray_clear_);
        // vec3 display_tint = color_tint_component_.temp_tint();
        shaders->SetUniformVec3("color_tint", color_tint_component_.tint_);
        shaders->SetUniformVec3("blood_tint", graphics->config_.blood_color());
        shaders->SetUniformFloat("haze_mult", scenegraph_->haze_mult);

        std::vector<mat4> shadow_matrix;
        shadow_matrix.resize(4);
        for (int i = 0; i < 4; ++i) {
            shadow_matrix[i] = cam->biasMatrix * graphics->cascade_shadow_mat[i];
        }
        if (g_simple_shadows || !g_level_shadows) {
            shadow_matrix[3] = cam->biasMatrix * graphics->simple_shadow_mat;
        }
        shaders->SetUniformMat4Array("shadow_matrix", shadow_matrix);

        if (scenegraph_->light_probe_collection.light_probe_buffer_object_id != -1) {
            shaders->SetUniformInt("num_tetrahedra", scenegraph_->light_probe_collection.ShaderNumTetrahedra());
            shaders->SetUniformInt("num_light_probes", scenegraph_->light_probe_collection.ShaderNumLightProbes());
            shaders->SetUniformVec3("grid_bounds_min", scenegraph_->light_probe_collection.grid_lookup.bounds[0]);
            shaders->SetUniformVec3("grid_bounds_max", scenegraph_->light_probe_collection.grid_lookup.bounds[1]);
            shaders->SetUniformInt("subdivisions_x", scenegraph_->light_probe_collection.grid_lookup.subdivisions[0]);
            shaders->SetUniformInt("subdivisions_y", scenegraph_->light_probe_collection.grid_lookup.subdivisions[1]);
            shaders->SetUniformInt("subdivisions_z", scenegraph_->light_probe_collection.grid_lookup.subdivisions[2]);
            shaders->SetUniformInt(shaders->GetTexUniform(TEX_AMBIENT_COLOR_BUFFER), TEX_AMBIENT_COLOR_BUFFER);
            shaders->SetUniformInt(shaders->GetTexUniform(TEX_AMBIENT_GRID_DATA), TEX_AMBIENT_GRID_DATA);
            glBindBuffer(GL_TEXTURE_BUFFER, scenegraph_->light_probe_collection.light_probe_buffer_object_id);
        }
        shaders->SetUniformInt("reflection_capture_num", scenegraph_->ref_cap_matrix.size());
        if (!scenegraph_->ref_cap_matrix.empty()) {
            assert(!scenegraph_->ref_cap_matrix_inverse.empty());
            shaders->SetUniformMat4Array("reflection_capture_matrix", scenegraph_->ref_cap_matrix);
            shaders->SetUniformMat4Array("reflection_capture_matrix_inverse", scenegraph_->ref_cap_matrix_inverse);
        }

        std::vector<mat4> light_volume_matrix;
        std::vector<mat4> light_volume_matrix_inverse;
        for (auto obj : scenegraph_->light_volume_objects_) {
            const mat4& mat = obj->GetTransform();
            light_volume_matrix.push_back(mat);
            light_volume_matrix_inverse.push_back(invert(mat));
        }
        shaders->SetUniformInt("light_volume_num", light_volume_matrix.size());
        if (!light_volume_matrix.empty()) {
            assert(!light_volume_matrix_inverse.empty());
            shaders->SetUniformMat4Array("light_volume_matrix", light_volume_matrix);
            shaders->SetUniformMat4Array("light_volume_matrix_inverse", light_volume_matrix_inverse);
        }

        if (scenegraph_->light_probe_collection.light_volume_enabled && type == Object::kFullDraw && scenegraph_->light_probe_collection.ambient_3d_tex.valid()) {
            textures->bindTexture(scenegraph_->light_probe_collection.ambient_3d_tex, 16);
        }

        if (g_no_reflection_capture == false) {
            textures->bindTexture(scenegraph_->cubemaps, 19);
        }
    }

    if (!online->IsClient()) {
        transform = bullet_object_->GetInterpRotationX(interp_period_, interp_period_ - interp_count_);
        vec3 pos = bullet_object_->GetInterpPositionX(interp_period_, interp_period_ - interp_count_);
        transform.SetTranslationPart(pos - transform * display_phys_offset_);
    }

    shaders->SetUniformMat4("projection_view_mat", proj_view_matrix);
    shaders->SetUniformMat4("model_mat", transform);
    shaders->SetUniformMat4("new_vel_mat", bullet_object_->transform);
    shaders->SetUniformMat4("old_vel_mat", bullet_object_->old_transform);
    shaders->SetUniformMat3("model_rotation_mat", mat3(transform));
    shaders->SetUniformFloat("time", game_timer.game_time);
    DrawItemModel(*model, graphics, shaders, the_shader);
    CHECK_GL_ERROR();

    batch_.SetEndState();
}

btTypedConstraint* ItemObject::AddConstraint(BulletObject* _bullet_object) {
    return scenegraph_->bullet_world_->AddFixedJoint(bullet_object_, _bullet_object, (bullet_object_->GetPosition() + _bullet_object->GetPosition()) * 0.5f);
}

void ItemObject::RemoveConstraint(btTypedConstraint** constraint) {
    scenegraph_->bullet_world_->RemoveJoint(constraint);
}

mat4 ItemObject::GetTransform() const {
    return transform;
}

void ItemObject::SleepPhysics() {
    bullet_object_->Sleep();
}

ScriptParams* ItemObject::ASGetScriptParams() {
    return &sp;
}

const vec3& ItemObject::phys_offset() const {
    return phys_offset_;
}

void ItemObject::InvalidateHeldReaders() {
    std::list<ItemObjectScriptReader*> temp_readers = readers_;  // Use this copy because Invalidate() will mess with readers_ through the callback
    for (std::list<ItemObjectScriptReader*>::iterator iter = temp_readers.begin();
         iter != temp_readers.end();) {
        ItemObjectScriptReader* reader = (*iter);
        if (reader->holding) {
            reader->Invalidate();
        }
        ++iter;
    }
}

bool ItemObject::SetFromDesc(const EntityDescription& desc) {
    bool ret = Object::SetFromDesc(desc);
    if (ret) {
        for (const auto& field : desc.fields) {
            switch (field.type) {
                case EDF_FILE_PATH: {
                    std::string type_file;
                    field.ReadString(&type_file);
                    if (!ofc_.valid() || ofc_->path_ != type_file) {
                        Load(type_file);
                    }
                    break;
                }
            }
        }

        color_tint_component_.SetFromDescription(desc);
    }

    return ret;
}

const vec3& ItemObject::GetColorTint() {
    return color_tint_component_.tint_;
}

const float& ItemObject::GetOverbright() {
    return color_tint_component_.overbright_;
}

void ItemObject::Load(const std::string& item_path) {
    Textures* ti = Textures::Instance();

    textures_.clear();

    obj_file = item_path;
    // item_ref_ = Items::Instance()->ReturnRef(item_path);
    item_ref_ = Engine::Instance()->GetAssetManager()->LoadSync<Item>(item_path);
    // ofc_ = ObjectFiles::Instance()->ReturnRef(item_ref_->GetObjPath());
    ofc_ = Engine::Instance()->GetAssetManager()->LoadSync<ObjectFile>(item_ref_->GetObjPath());

    if (ofc_->shader_name == "cubemap") {
        use_tangent_ = true;
    } else {
        use_tangent_ = false;
    }

    char flags = _MDL_CENTER;
    if (use_tangent_) {
        flags |= _MDL_USE_TANGENT;
    }
    model_id_ = Models::Instance()->loadModel(ofc_->model_name.c_str(), flags);
    blood_surface_.AttachToModel(&Models::Instance()->GetModel(model_id_));

    ti->setWrap(GL_REPEAT);
    if (!ofc_->color_map.empty()) {
        TextureAssetRef tex = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(ofc_->color_map, PX_SRGB, 0x0);
        batch_.texture_ref[0] = tex->GetTextureRef();
        textures_.push_back(tex);
    }
    if (!ofc_->normal_map.empty()) {
        TextureAssetRef tex = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(ofc_->normal_map);
        batch_.texture_ref[1] = tex->GetTextureRef();
        textures_.push_back(tex);
    }
    if (!ofc_->sharpness_map.empty()) {
        Models::Instance()->GetModel(model_id_).LoadAuxFromImage(ofc_->sharpness_map);
    }
    batch_.texture_ref[5] = ti->GetBlankTextureRef();
}

void ItemObject::GetShaderNames(std::map<std::string, int>& preload_shaders) {
    if (use_tangent_)
        shader = "envobject #ITEM #TANGENT";
    else
        shader = "envobject #ITEM";

    preload_shaders[shader] = SceneGraph::kPreloadTypeAll;
}

void ItemObject::ReceiveObjectMessageVAList(OBJECT_MSG::Type type, va_list args) {
    switch (type) {
        case OBJECT_MSG::SET_COLOR:
        case OBJECT_MSG::SET_OVERBRIGHT:
            color_tint_component_.ReceiveObjectMessageVAList(type, args);
            break;
        default:
            Object::ReceiveObjectMessageVAList(type, args);
            break;
    }
}

void ItemObject::SetSafe() {
    thrown_ = kSafe;
}

namespace {
static const int _AS_collectable = _collectable;
static const int _AS_misc = _misc;
static const int _AS_weapon = _weapon;
static const int _AS_item_no_type = _item_no_type;

int GetNumHands(ItemObject* io) {
    return io->item_ref()->GetNumHands();
}

int GetType(ItemObject* io) {
    return io->item_ref()->GetItemType();
}

bool HasSheatheAttachment(ItemObject* io) {
    return io->item_ref()->HasAttachment(_at_sheathe);
}

vec3 GetPoint(ItemObject* io, std::string name) {
    return io->item_ref()->GetPoint(name);
}

/*
const std::string & GetPath(ItemObject* io) {
    return io->item_ref()->path_;
}
*/

const std::string& GetSoundModifier(ItemObject* io) {
    return io->item_ref()->GetSoundModifier();
}

/*
bool HasAnimBlend(ItemObject* io, const std::string &anim) {
    return io->item_ref()->HasAnimBlend(anim);
}

const std::string &GetAnimBlend(ItemObject* io, const std::string &anim)
{
    return io->item_ref()->GetAnimBlend(anim);
}
*/

float GetMass(ItemObject* io) {
    return io->item_ref()->GetMass();
}

float GetRangeExtender(ItemObject* io) {
    return io->item_ref()->GetRangeExtender();
}

float GetRangeMultiplier(ItemObject* io) {
    return io->item_ref()->GetRangeMultiplier();
}

int GetNumLines(ItemObject* io) {
    return (int)io->item_ref()->GetNumLines();
}

vec3 GetLineStart(ItemObject* io, int which) {
    return io->item_ref()->GetLineStart(which) - io->phys_offset();
}

vec3 GetLineEnd(ItemObject* io, int which) {
    return io->item_ref()->GetLineEnd(which) - io->phys_offset();
}

std::string GetLineMaterial(ItemObject* io, int which) {
    return io->item_ref()->GetLineMaterial(which);
}

const std::string& GetLabel(ItemObject* io) {
    return io->item_ref()->GetLabel();
}
}  // namespace

void DefineItemObjectTypePublic(ASContext* as_context) {
    ScriptParams::RegisterScriptType(as_context);
    as_context->RegisterObjectType("ItemObject", 0, asOBJ_REF | asOBJ_NOCOUNT);

    as_context->RegisterObjectMethod("ItemObject",
                                     "vec3 GetPhysicsPosition()",
                                     asMETHOD(ItemObject, GetPhysicsPosition), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ItemObject",
                                     "vec3 GetLinearVelocity()",
                                     asMETHOD(ItemObject, GetLinearVelocity), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ItemObject",
                                     "vec3 GetAngularVelocity()",
                                     asMETHOD(ItemObject, GetAngularVelocity), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ItemObject",
                                     "void SetAngularVelocity(vec3)",
                                     asMETHOD(ItemObject, SetAngularVelocity), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ItemObject",
                                     "void AddBloodDecal(vec3 position, vec3 direction, float size)",
                                     asMETHOD(ItemObject, AddBloodDecal), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ItemObject",
                                     "void CleanBlood()",
                                     asMETHOD(ItemObject, CleanBlood), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ItemObject",
                                     "mat4 GetPhysicsTransform()",
                                     asMETHOD(ItemObject, GetPhysicsTransform), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ItemObject",
                                     "mat4 GetPhysicsTransformIncludeOffset()",
                                     asMETHOD(ItemObject, GetPhysicsTransformIncludeOffset), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ItemObject",
                                     "void SetPhysicsTransform(const mat4 &in)",
                                     asMETHOD(ItemObject, SetPhysicsTransform), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ItemObject",
                                     "void ActivatePhysics()",
                                     asMETHOD(ItemObject, ActivatePhysics), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ItemObject",
                                     "bool IsHeld()",
                                     asMETHOD(ItemObject, IsHeld), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ItemObject",
                                     "int StuckInWhom()",
                                     asMETHOD(ItemObject, StuckInWhom), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ItemObject",
                                     "int HeldByWhom()",
                                     asMETHOD(ItemObject, HeldByWhom), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ItemObject",
                                     "int GetID()",
                                     asMETHOD(ItemObject, GetID), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ItemObject",
                                     "void SetLinearVelocity(vec3)",
                                     asMETHOD(ItemObject, SetLinearVelocity), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ItemObject",
                                     "const string& GetLabel()",
                                     asFUNCTION(GetLabel), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("ItemObject",
                                     "void SetThrown()",
                                     asMETHOD(ItemObject, SetThrown), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ItemObject",
                                     "void SetThrownStraight()",
                                     asMETHOD(ItemObject, SetThrownStraight), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ItemObject",
                                     "float GetMass()",
                                     asFUNCTION(GetMass), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("ItemObject",
                                     "bool HasSheatheAttachment()",
                                     asFUNCTION(HasSheatheAttachment), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("ItemObject",
                                     "int GetNumHands()",
                                     asFUNCTION(GetNumHands), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("ItemObject",
                                     "int GetType()",
                                     asFUNCTION(GetType), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("ItemObject",
                                     "vec3 GetPoint(string label)",
                                     asFUNCTION(GetPoint), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("ItemObject",
                                     "int GetNumLines()",
                                     asFUNCTION(GetNumLines), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("ItemObject",
                                     "vec3 GetLineStart(int line_index)",
                                     asFUNCTION(GetLineStart), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("ItemObject",
                                     "vec3 GetLineEnd(int line_index)",
                                     asFUNCTION(GetLineEnd), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("ItemObject",
                                     "string GetLineMaterial(int line_index)",
                                     asFUNCTION(GetLineMaterial), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("ItemObject",
                                     "const string &GetSoundModifier()",
                                     asFUNCTION(GetSoundModifier), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("ItemObject",
                                     "float GetRangeExtender()",
                                     asFUNCTION(GetRangeExtender), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("ItemObject",
                                     "float GetRangeMultiplier()",
                                     asFUNCTION(GetRangeMultiplier), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("ItemObject",
                                     "bool CheckThrownSafe()",
                                     asMETHOD(ItemObject, CheckThrownSafe), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ItemObject",
                                     "void SetSafe()",
                                     asMETHOD(ItemObject, SetSafe), asCALL_THISCALL);
    as_context->RegisterObjectMethod("ItemObject", "ScriptParams@ GetScriptParams()", asMETHOD(ItemObject, ASGetScriptParams), asCALL_THISCALL);
    as_context->RegisterObjectProperty("ItemObject", "int last_held_char_id_", asOFFSET(ItemObject, last_held_char_id_));
    as_context->DocsCloseBrace();
    as_context->RegisterGlobalProperty("const int _collectable",
                                       (void*)&_AS_collectable);
    as_context->RegisterGlobalProperty("const int _misc",
                                       (void*)&_AS_misc);
    as_context->RegisterGlobalProperty("const int _weapon",
                                       (void*)&_AS_weapon);
    as_context->RegisterGlobalProperty("const int _item_no_type",
                                       (void*)&_AS_item_no_type);
}

vec3 ItemObjectBloodSurfaceTransformedVertexGetter::GetTransformedVertex(int val) {
    return item_object->GetTransformedVertex(val);
}
