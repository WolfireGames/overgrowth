//-----------------------------------------------------------------------------
//           Name: movementobject.cpp
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
#include "movementobject.h"

#include <Objects/riggedobject.h>
#include <Objects/itemobject.h>
#include <Objects/pathpointobject.h>
#include <Objects/group.h>
#include <Objects/hotspot.h>

#include <Graphics/pxdebugdraw.h>
#include <Graphics/particles.h>
#include <Graphics/models.h>
#include <Graphics/text.h>

#include <Asset/Asset/material.h>
#include <Asset/Asset/voicefile.h>

#include <Internal/common.h>
#include <Internal/filesystem.h>
#include <Internal/memwrite.h>
#include <Internal/comma_separated_list.h>
#include <Internal/checksum.h>
#include <Internal/profiler.h>

#include <Math/vec2math.h>
#include <Math/vec3math.h>
#include <Math/vec4math.h>

#include <Editors/mem_read_entity_description.h>
#include <Editors/actors_editor.h>
#include <Editors/map_editor.h>

#include <Sound/sound.h>
#include <Sound/threaded_sound_wrapper.h>

#include <Asset/Asset/objectfile.h>
#include <Asset/Asset/actorfile.h>

#include <Online/online.h>
#include <Online/Message/set_object_enabled_message.h>
#include <Online/online_datastructures.h>

#include <Main/scenegraph.h>
#include <Main/engine.h>

#include <Scripting/angelscript/add_on/scriptarray/scriptarray.h>
#include <Scripting/angelscript/asfuncs.h>

#include <Game/level.h>
#include <Logging/logdata.h>
#include <UserInput/input.h>
#include <AI/navmesh.h>

#include <angelscript.h>
#include <SDL.h>

extern AnimationConfig animation_config;
extern std::string script_dir_path;
extern Timer game_timer;

extern bool g_perform_occlusion_query;

extern bool g_debug_runtime_disable_movement_object_draw;
extern bool g_debug_runtime_disable_movement_object_draw_depth_map;
extern bool g_debug_runtime_disable_movement_object_pre_draw_camera;
extern bool g_debug_runtime_disable_movement_object_pre_draw_frame;

const int MovementObject::_awake = 0;
const int MovementObject::_unconscious = 1;
const int MovementObject::_dead = 2;
//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

MovementObject::MovementObject() : controlled(false),
                                   was_controlled(false),
                                   static_char(false),
                                   voice_sound(0),
                                   connected_pathpoint_id(-1),
                                   controller_id(0),
                                   camera_id(0),
                                   visible(true),
                                   velocity(0.0f),
                                   is_player(false),
                                   reset_time(0.0f),
                                   do_connection_finalization_remap(false),
                                   char_sphere(NULL),
                                   shader("3d_color #NO_VELOCITY_BUF"),
                                   no_grab(0),
                                   focused_character(false),
                                   kCullRadius(2.0f),
                                   remote(false) {
    box_.dims = vec3(2.0f);
    created_on_the_fly = false;
}

void MovementObject::GetChildren(std::vector<Object*>* ret_children) {
    if (!env_object_attach_data.empty()) {
        std::vector<AttachedEnvObject> attached_env_objects;
        Deserialize(env_object_attach_data, attached_env_objects);
        ret_children->reserve(attached_env_objects.size());
        for (auto& attached_env_object : attached_env_objects) {
            Object* obj = attached_env_object.direct_ptr;
            if (obj) {
                ret_children->push_back(attached_env_object.direct_ptr);
            }
        }
    }
}

void MovementObject::GetBottomUpCompleteChildren(std::vector<Object*>* ret_children) {
    if (!env_object_attach_data.empty()) {
        std::vector<AttachedEnvObject> attached_env_objects;
        Deserialize(env_object_attach_data, attached_env_objects);
        ret_children->reserve(attached_env_objects.size());
        for (auto& attached_env_object : attached_env_objects) {
            Object* obj = attached_env_object.direct_ptr;
            if (obj) {
                obj->GetBottomUpCompleteChildren(ret_children);
                ret_children->push_back(attached_env_object.direct_ptr);
            }
        }
    }
}

bool MovementObject::IsMultiplayerSupported() {
    ASArglist arglist;

    ASArg return_val;
    bool multiplayer_supported = false;
    return_val.type = _as_bool;
    return_val.data = &multiplayer_supported;

    if (as_context->CallScriptFunction(as_funcs.is_multiplayer_supported, &arglist, &return_val)) {
        return multiplayer_supported;
    } else {
        return false;
    }
}

void MovementObject::UpdatePaused() {
    if (as_context.get()) {
        as_context->CallScriptFunction(as_funcs.update_paused);
    }
}

int MovementObject::AboutToBeHitByItem(int id) {
    asDWORD return_val_int = 0;
    ASArg return_val;
    return_val.type = _as_int;
    return_val.data = &return_val_int;
    ASArglist args;
    args.Add(id);
    if (as_context.get()) {
        as_context->CallScriptFunction(as_funcs.about_to_be_hit_by_item, &args, &return_val);
    }
    return return_val_int;
}

void MovementObject::PreDrawFrame(float curr_game_time) {
    Online* online = Online::Instance();

    if (g_debug_runtime_disable_movement_object_pre_draw_frame) {
        return;
    }

    PROFILER_GPU_ZONE(g_profiler_ctx, "MovementObject::PreDrawFrame");
    needs_predraw_update = true;
    // Hide shadow decals
    {
        PROFILER_ZONE(g_profiler_ctx, "Angelscript PreDrawFrame()");
        ASArglist args;
        args.Add(curr_game_time);
        as_context->CallScriptFunction(as_funcs.pre_draw_frame, &args);
    }

    // Update attached objects

    if (!online->IsClient()) {
        int num_attached_env_objects = rigged_object_->children.size();
        for (int i = 0; i < num_attached_env_objects; ++i) {
            AttachedEnvObject& attached_env_object = rigged_object_->children[i];
            Object* obj = attached_env_object.direct_ptr;
            if (obj) {
                if (IsBeingMoved(scenegraph_->map_editor, obj) && obj->Selected()) {
                    attached_env_object.bone_connection_dirty = true;
                } else {
                    if (attached_env_object.bone_connection_dirty) {
                        for (auto& bone_connect : attached_env_object.bone_connects) {
                            if (bone_connect.bone_id != -1) {
                                mat4 transform_mat = rigged_object_->display_bone_transforms[bone_connect.bone_id].GetMat4();
                                mat4 env_mat = Mat4FromQuaternion(obj->GetRotation());
                                env_mat.SetTranslationPart(obj->GetTranslation());
                                bone_connect.rel_mat = invert(transform_mat) * env_mat;
                            }
                        }
                        attached_env_object.bone_connection_dirty = false;
                    }
                    bool had_moved = obj->online_transform_dirty;
                    obj->SetTranslation(vec3(0.0f, -999999.0f, 0.0f));
                    obj->online_transform_dirty = had_moved;
                    if (obj->IsGroupDerived()) {
                        obj->PropagateTransformsDown(true);
                    }
                }
            }
        }
    }
}

void MovementObject::Reload() {
    as_context->Reload();
}

//const float kCullRadius = 2.0f;

void MovementObject::ActualPreDraw(float curr_game_time) {
    bool dirty_attachment = false;
    for (auto& i : rigged_object_->children) {
        if (i.bone_connection_dirty) {
            dirty_attachment = true;
        }
    }
    rigged_object_->PreDrawFrame(curr_game_time);
    for (auto& child : rigged_object_->children) {
        Object* obj = child.direct_ptr;
        if (obj) {
            obj->PreDrawFrame(curr_game_time);
        }
    }

    if (dirty_attachment) {
        Serialize(rigged_object()->children, env_object_attach_data);  // TODO(david): make this only happen when needed
    }

    DrawNametag();

    {
        PROFILER_ZONE(g_profiler_ctx, "Angelscript PreDrawCamera()");
        ASArglist args;
        args.Add(curr_game_time);
        as_context->CallScriptFunction(as_funcs.pre_draw_camera, &args);
    }
    needs_predraw_update = false;
}

void MovementObject::DrawNametag() {
    RegenerateNametag();  // TODO DEBUG remove this! This should be removed once we can hook this function call into an "OnAvatarChange" event
    if (nametag_string.length() > 1) {
        // Figure out text draw position, apply some smoothing based on the last position
        vec3 base_position = rigged_object_->GetDisplayBonePosition(0);
        nametag_last_position = vec3(
            nametag_last_position.x() * 0.2f + 0.8f * base_position.x(),
            nametag_last_position.y() * 0.8f + 0.2f * (base_position.y() + 1),
            nametag_last_position.z() * 0.2f + 0.8f * base_position.z());

        // Figure out text opacity, fade out when away from camera
        float fadeoutStart = 10;
        float fadeoutEnd = 15;

        float distance_to_camera = length(position - ActiveCameras::Get()->GetPos());
        float opacity = 1.0f - (distance_to_camera - fadeoutStart) / (fadeoutEnd - fadeoutStart);
        opacity = std::min(1.0f, std::max(0.0f, opacity));  // Clamp opacity to [0;1]

        // Get Text color
        vec4 color = vec4(  // Disco lights
            std::cos(game_timer.game_time * 2 + 0.0f) * 0.5f + 0.5f,
            std::cos(game_timer.game_time * 2 + 2.0f) * 0.5f + 0.5f,
            std::cos(game_timer.game_time * 2 + 4.0f) * 0.5f + 0.5f,
            opacity * 0.8f);

        // Draw Text
        DebugDraw::Instance()->AddText(nametag_last_position, nametag_string, 1.0f, LifespanFromInt(_delete_on_draw), _DD_SCREEN_SPACE, color);
    }
}

void MovementObject::RegenerateNametag() {  // TODO This must be called when avatar possession changes, do we have an event for this?
    // nametag_string = "ID: " + std::to_string(GetID()) + " (" + std::to_string(Online::Instance()->GetOriginalID(GetID())) + "), controlled: " + std::to_string(controlled) + ", controller_id: " + std::to_string(controller_id) + ", camera_id: " + std::to_string(camera_id) + ", remote: " + std::to_string(remote) + ", is_player: " + std::to_string(is_player);
    // return;

    nametag_string = "";
    if (controlled && !remote) {
        return;  // Don't show nametag on character controlled by us
    }

    PlayerState player_state;
    if (Online::Instance()->TryGetPlayerState(player_state, GetID())) {
        nametag_last_position = rigged_object_->GetDisplayBonePosition(0) + vec3(0, 1, 0);
        nametag_string = player_state.playername;
    } else {
        nametag_string = "";
    }
}

int GetStateID(std::vector<MovementObject::OcclusionState>& occlusion_states, int cam_id) {
    int state_id = -1;
    for (int state_index = 0, state_len = occlusion_states.size();
         state_index < state_len;
         ++state_index) {
        if (occlusion_states[state_index].cam_id == cam_id) {
            state_id = state_index;
            break;
        }
    }
    if (state_id == -1) {
        state_id = occlusion_states.size();
        occlusion_states.resize(occlusion_states.size() + 1);
        occlusion_states[state_id].cam_id = cam_id;
    }
    return state_id;
}

void MovementObject::DrawDepthMap(const mat4& proj_view_matrix, const vec4* cull_planes, int num_cull_planes, Object::DrawType draw_type) {
    if (g_debug_runtime_disable_movement_object_draw_depth_map) {
        return;
    }

    if (visible) {
        bool culled = false;
        for (int plane = 0; plane < num_cull_planes; ++plane) {
            if (dot(position, cull_planes[plane].xyz()) +
                    cull_planes[plane][3] + kCullRadius <=
                0.0f) {
                culled = true;
                break;
            }
        }
        if (!culled) {
            // Check if character is occluded from pov of active camera
            bool occluded = false;
            for (auto& occlusion_state : occlusion_states) {
                if (occlusion_state.cam_id == ActiveCameras::GetID()) {
                    occluded = occlusion_state.occluded;
                    break;
                }
            }

            if (!occluded) {
                rigged_object_->DrawRiggedObject(proj_view_matrix, RiggedObject::kDrawDepthOnly);
            }
        }
    }
}

void MovementObject::PreDrawCamera(float curr_game_time) {
    Online* online = Online::Instance();

    if (g_debug_runtime_disable_movement_object_pre_draw_camera) {
        return;
    }

    {
        PROFILER_ZONE(g_profiler_ctx, "Angelscript PreDrawCameraNoCull()");
        ASArglist args;
        args.Add(curr_game_time);
        as_context->CallScriptFunction(as_funcs.pre_draw_camera_no_cull, &args);
    }
    if (visible && enabled_) {
        Camera* cam = ActiveCameras::Get();
        if (cam->checkSphereInFrustum(position, kCullRadius) || online->IsActive()) {
            bool occluded = false;
            if (g_perform_occlusion_query) {
                // Check previous occlusion queries to update per-camera occlusion
                {
                    PROFILER_ZONE(g_profiler_ctx, "Process occlusion queries");

                    for (auto& occlusion_querie : occlusion_queries) {
                        OcclusionQuery* query = &occlusion_querie;
                        if (!query->in_progress) {
                            continue;
                        }
                        PROFILER_ZONE(g_profiler_ctx, "Process occlusion query");
                        int count = -1;
                        {
                            PROFILER_ZONE(g_profiler_ctx, "GL_QUERY_RESULT");
                            glGetQueryObjectiv(query->id, GL_QUERY_RESULT_AVAILABLE, &count);
                            if (count == 1) {
                                glGetQueryObjectiv(query->id, GL_QUERY_RESULT, &count);
                            } else {
                                count = -1;
                            }
                        }
                        if (count != -1) {
                            // LOGI << "Query completed with " << count << "samples visible";
                            query->in_progress = false;

                            {
                                PROFILER_ZONE(g_profiler_ctx, "Update occlusion state");
                                int state_id = GetStateID(occlusion_states, query->cam_id);

                                if (count == 0) {
                                    occlusion_states[state_id].occluded = true;
                                } else {
                                    occlusion_states[state_id].occluded = false;
                                }
                            }
                        }
                    }
                }

                if (distance_squared(position, ActiveCameras::Get()->GetPos()) <= kCullRadius) {
                    int state_id = GetStateID(occlusion_states, ActiveCameras::GetID());
                    occlusion_states[state_id].occluded = false;
                }

                // Check if character is occluded from pov of active camera
                for (auto& occlusion_state : occlusion_states) {
                    if (occlusion_state.cam_id == ActiveCameras::GetID()) {
                        occluded = occlusion_state.occluded;
                        break;
                    }
                }

                if (distance_squared(position, cam->GetPos()) <= kCullRadius * kCullRadius) {
                    occluded = false;
                }
            }

            if (!occluded || online->IsActive()) {
                if (needs_predraw_update) {
                    ActualPreDraw(curr_game_time);
                }
                rigged_object_->PreDrawCamera(game_timer.GetRenderTime());
            }
        }
    }
}

// Interpolate the network data.
void MovementObject::ClientBeforeDraw() {
    Online* online = Online::Instance();
    if (online->IsClient()) {
        rigged_object_->ClientBeforeDraw();
    }
}

void MovementObject::Draw() {
    if (g_debug_runtime_disable_movement_object_draw) {
        return;
    }

    ClientBeforeDraw();

    if (visible) {
        PROFILER_ZONE(g_profiler_ctx, "MovementObject::Draw");
        Camera* cam = ActiveCameras::Get();
        if (cam->checkSphereInFrustum(position, kCullRadius)) {
            // Check if character is occluded from pov of active camera
            bool occluded = false;
            if (g_perform_occlusion_query) {
                for (auto& occlusion_state : occlusion_states) {
                    if (occlusion_state.cam_id == ActiveCameras::GetID()) {
                        occluded = occlusion_state.occluded;
                        break;
                    }
                }

                if (distance_squared(position, cam->GetPos()) <= kCullRadius * kCullRadius) {
                    occluded = false;
                } else {
                    glColorMask(false, false, false, false);
                    PROFILER_ZONE(g_profiler_ctx, "Launch occlusion query");
                    int active_cam_id = ActiveCameras::Instance()->GetID();
                    int curr_query_id = -1;
                    for (int i = 0, len = occlusion_queries.size(); i < len; ++i) {
                        if (occlusion_queries[i].in_progress == false) {
                            curr_query_id = i;
                            break;
                        }
                    }
                    if (curr_query_id == -1) {
                        curr_query_id = occlusion_queries.size();
                        occlusion_queries.resize(occlusion_queries.size() + 1);
                        GLuint val;
                        glGenQueries(1, &val);
                        occlusion_queries[curr_query_id].id = val;
                    }
                    occlusion_queries[curr_query_id].cam_id = active_cam_id;
                    occlusion_queries[curr_query_id].in_progress = true;

                    // LOGI << "glBeginQuery on " << occlusion_queries[curr_query_id].id <<  std::endl;
                    glBeginQuery(GL_SAMPLES_PASSED, occlusion_queries[curr_query_id].id);

                    GLState gl_state;
                    gl_state.blend = false;
                    gl_state.cull_face = true;
                    gl_state.depth_test = true;
                    gl_state.depth_write = false;

                    Graphics::Instance()->setGLState(gl_state);

                    int shader_id = Shaders::Instance()->returnProgram(shader);
                    Shaders::Instance()->setProgram(shader_id);

                    mat4 char_mat;
                    char_mat.SetScale(vec3(rigged_object_->GetCharScale()));
                    char_mat.SetTranslationPart(position);

                    mat4 mvp = cam->GetProjMatrix() * cam->GetViewMatrix() * char_mat;

                    Shaders::Instance()->SetUniformMat4("mvp", mvp);
                    int vert_attrib_id = Shaders::Instance()->returnShaderAttrib("vert_attrib", shader_id);
                    Model* probe_model = &Models::Instance()->GetModel(scenegraph_->light_probe_collection.probe_model_id);
                    if (!probe_model->vbo_loaded) {
                        probe_model->createVBO();
                    }
                    probe_model->VBO_vertices.Bind();
                    probe_model->VBO_faces.Bind();
                    Graphics::Instance()->EnableVertexAttribArray(vert_attrib_id);
                    glVertexAttribPointer(vert_attrib_id, 3, GL_FLOAT, false, 3 * sizeof(GL_FLOAT), 0);
                    Graphics::Instance()->DrawElements(GL_TRIANGLES, probe_model->faces.size(), GL_UNSIGNED_INT, 0);
                    Graphics::Instance()->ResetVertexAttribArrays();

                    // LOGI << "glEndQuery on " << occlusion_queries[curr_query_id].id <<  std::endl;
                    glEndQuery(GL_SAMPLES_PASSED);
                    glColorMask(true, true, true, true);
                }
            }

            if (!occluded) {
                rigged_object_->DrawRiggedObject(cam->GetProjMatrix() * cam->GetViewMatrix(), RiggedObject::kFullDraw);
            }
        }
    }

    if (scenegraph_->map_editor->IsTypeEnabled(_movement_object) &&
        !Graphics::Instance()->media_mode() &&
        scenegraph_->map_editor->state_ != MapEditor::kInGame) {
        if (connected_pathpoint_id != -1) {
            Object* obj = scenegraph_->GetObjectFromID(connected_pathpoint_id);
            if (obj) {
                vec3 pos;
                if (obj->GetType() == _path_point_object) {
                    pos = ((PathPointObject*)obj)->GetTranslation();
                } else if (obj->GetType() == _movement_object) {
                    pos = obj->GetTranslation();
                }
                DebugDraw::Instance()->AddLine(GetTranslation(), pos, vec4(vec3(1.0f), 0.5f), vec4(0.0f, 0.5f, 0.5f, 0.5f), _delete_on_draw);
            }
        }
        std::list<ItemObjectScriptReader>& ic = item_connections;
        for (auto& item : ic) {
            if (item.valid()) {
                DebugDraw::Instance()->AddLine(GetTranslation(),
                                               item->GetTranslation(),
                                               vec4(0.0f, 0.5f, 0.5f, 0.5f),
                                               vec4(0.0f, 0.5f, 0.5f, 0.5f),
                                               _delete_on_draw);
            }
        }
        if (is_player) {
            box_color = vec4(0.0f, 1.0f, 0.0f, 1.0f);
        } else {
            box_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }
}

void MovementObject::HandleTransformationOccured() {
    velocity = vec3(0.0f);
    position = GetTranslation();
}

void MovementObject::ASPlaySoundGroupAttached(std::string path, vec3 location) {
    // SoundGroupRef sgr = SoundGroups::Instance()->ReturnRef(path);

    SoundGroupRef sgr = Engine::Instance()->GetAssetManager()->LoadSync<SoundGroup>(path);
    SoundGroupPlayInfo sgpi(*sgr, location);
    sgpi.occlusion_position = position;
    sgpi.created_by_id = GetID();

    unsigned long handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
    Engine::Instance()->GetSound()->PlayGroup(handle, sgpi);
    attached_sounds.push_back(handle);
}

void MovementObject::ASPlaySoundAttached(std::string path, vec3 location) {
    SoundPlayInfo spi;
    spi.path = path;
    spi.position = location;
    spi.occlusion_position = position;
    spi.created_by_id = GetID();

    unsigned long handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
    Engine::Instance()->GetSound()->Play(handle, spi);
    attached_sounds.push_back(handle);
}

void MovementObject::PlaySoundGroupVoice(std::string path, float delay) {
    Online* online = Online::Instance();

    if (path.empty()) {
        return;
    }

    if (online->IsHosting()) {
        online->Send<OnlineMessages::AudioPlaySoundGroupVoiceMessage>(path, GetID(), delay);
    }
    // CharacterRef c_ref = Characters::Instance()->ReturnRef(character_path);
    CharacterRef c_ref = Engine::Instance()->GetAssetManager()->LoadSync<Character>(character_path);
    const std::string& voice_path = c_ref->GetVoicePath();
    if (voice_path.empty()) {
        // printf("No voice file.\n");
        return;
    }
    float pitch = c_ref->GetVoicePitch();
    // VoiceFileRef v_ref = VoiceFiles::Instance()->ReturnRef(voice_path);
    VoiceFileRef v_ref = Engine::Instance()->GetAssetManager()->LoadSync<VoiceFile>(voice_path);
    const std::string voice_file = v_ref->GetVoicePath(path);
    if (voice_file.empty()) {
        // printf("No voice path for %s.\n",path.c_str());
        return;
    }
    voice_queue.push_back(VoiceQueue(voice_file, delay, pitch));
}

void MovementObject::ForceSoundGroupVoice(std::string path, float delay) {
    // CharacterRef c_ref = Characters::Instance()->ReturnRef(character_path);
    CharacterRef c_ref = Engine::Instance()->GetAssetManager()->LoadSync<Character>(character_path);
    float pitch = c_ref->GetVoicePitch();
    voice_queue.push_back(VoiceQueue(path, delay, pitch));
}

std::string MovementObject::GetTeamString() {
    return sp.GetStringVal("Teams");
}

void MovementObject::StartPoseAnimation(std::string path) {
    ASArglist args;
    args.AddObject(&path);

    bool val;
    ASArg return_val;
    return_val.type = _as_bool;
    return_val.data = &val;
    as_context->CallScriptFunction(as_funcs.start_pose, &args, &return_val);
}

void MovementObject::StopVoice() {
    voice_queue.clear();
    Engine::Instance()->GetSound()->Stop(voice_sound);
}

struct GridCellInfo {
    bool solid;
    bool walkable;
    bool boundary;
    vec3 avg_collide;
};

static std::vector<GridCellInfo> grid_cells;
const int _grid_size = 11;
const float cell_size = 0.2f;

int GridIndexFromCoord(int x, int y, int z) {
    return z + y * _grid_size + x * _grid_size * _grid_size;
}

void GridTest(vec3 pos, BulletWorld* bw, float _leg_sphere_size) {
    if (grid_cells.empty()) {
        grid_cells.resize(_grid_size * _grid_size * _grid_size);
    }
    int index = 0;
    for (int i = 0; i < _grid_size; ++i) {
        for (int j = 0; j < _grid_size; ++j) {
            for (int k = 0; k < _grid_size; ++k) {
                vec3 quantized_pos = vec3(floor(pos[0] / cell_size), floor(pos[1] / cell_size), floor(pos[2] / cell_size)) * cell_size;
                quantized_pos[0] += (i - (_grid_size / 2)) * cell_size;
                quantized_pos[1] += (j - (_grid_size / 2)) * cell_size;
                quantized_pos[2] += (k - (_grid_size / 2)) * cell_size;
                ContactSlideCallback cb;
                cb.single_sided = false;
                bw->GetSphereCollisions(quantized_pos, _leg_sphere_size, cb);
                if (cb.collision_info.contacts.size() == 0) {
                    grid_cells[index].solid = false;
                } else {
                    grid_cells[index].solid = true;
                }
                grid_cells[index].walkable = false;
                grid_cells[index].boundary = false;
                ++index;
            }
        }
    }

    for (int i = 0; i < _grid_size; ++i) {
        for (int j = 0; j < _grid_size; ++j) {
            for (int k = 0; k < _grid_size; ++k) {
                if (!grid_cells[GridIndexFromCoord(i, j, k)].solid) {
                    bool next_to_solid = false;
                    if (i != 0 && grid_cells[GridIndexFromCoord(i - 1, j, k)].solid == true) {
                        next_to_solid = true;
                    } else if (i != _grid_size - 1 && grid_cells[GridIndexFromCoord(i + 1, j, k)].solid == true) {
                        next_to_solid = true;
                    } else if (k != 0 && grid_cells[GridIndexFromCoord(i, j, k - 1)].solid == true) {
                        next_to_solid = true;
                    } else if (k != _grid_size - 1 && grid_cells[GridIndexFromCoord(i, j, k + 1)].solid == true) {
                        next_to_solid = true;
                    } else if (j != 0 && grid_cells[GridIndexFromCoord(i, j - 1, k)].solid == true) {
                        next_to_solid = true;
                    } else if (j != _grid_size - 1 && grid_cells[GridIndexFromCoord(i, j + 1, k)].solid == true) {
                        next_to_solid = true;
                    }
                    if (next_to_solid) {
                        grid_cells[GridIndexFromCoord(i, j, k)].boundary = true;
                        vec3 quantized_pos = vec3(floor(pos[0] / cell_size), floor(pos[1] / cell_size), floor(pos[2] / cell_size)) * cell_size;
                        quantized_pos[0] += (i - (_grid_size / 2)) * cell_size;
                        quantized_pos[1] += (j - (_grid_size / 2)) * cell_size;
                        quantized_pos[2] += (k - (_grid_size / 2)) * cell_size;
                        DebugDraw::Instance()->AddWireSphere(quantized_pos, 0.1f, vec4(vec3(0.0f, 1.0f, 0.0f), 0.2f), _delete_on_update);
                    }
                }
            }
        }
    }

    /*
    for(int i=0; i<_grid_size; ++i){
        for(int j=1; j<_grid_size; ++j){
            for(int k=0; k<_grid_size; ++k){
                if(grid_cells[GridIndexFromCoord(i,j-1,k)].solid == true && grid_cells[GridIndexFromCoord(i,j,k)].solid == false){
                    vec3 quantized_pos = vec3(int(pos[0]/cell_size), int(pos[1]/cell_size), int(pos[2]/cell_size))*cell_size;
                    quantized_pos[0] += (i - (_grid_size/2)) * cell_size;
                    quantized_pos[1] += (j - (_grid_size/2)) * cell_size;
                    quantized_pos[2] += (k - (_grid_size/2)) * cell_size;

                    // Check slope of ground
                    vec3 upper_pos = quantized_pos+vec3(0,0.1f,0);
                    vec3 lower_pos = quantized_pos+vec3(0,-0.2f,0);
                    SweptSlideCallback cb;
                    cb.m_collisionFilterMask = btBroadphaseProxy::StaticFilter;
                    bw->GetSweptSphereCollisions(upper_pos, lower_pos, _leg_sphere_size, cb);
                    vec3 sphere_pos = mix(upper_pos,lower_pos,cb.true_closest_hit_fraction);

                    ContactSlideCallback cb2;
                    cb2.m_collisionFilterMask = btBroadphaseProxy::StaticFilter;
                    cb2.single_sided = false;
                    bw->GetSphereCollisions(sphere_pos, _leg_sphere_size * 1.1f, cb2);
                    vec3 adjusted_position = bw->ApplySphereSlide(sphere_pos, _leg_sphere_size * 1.1f, cb2.collision_info);

                    //quantized_pos = sphere_col.position;
                    vec3 norm = normalize(adjusted_position - sphere_pos);
                    grid_cells[GridIndexFromCoord(i,j,k)].avg_collide = vec3(0.0f);
                    for(int l=0, len=cb2.collision_info.points.size(); l<len; ++l){
                        grid_cells[GridIndexFromCoord(i,j,k)].avg_collide += ToVec3(cb2.collision_info.points[l]);
                    }
                    if(cb2.collision_info.points.size() > 0){
                        grid_cells[GridIndexFromCoord(i,j,k)].avg_collide /= (float)cb2.collision_info.points.size();
                    }
                    norm = normalize(norm);
                    if(norm[1] > 0.7f){
                        grid_cells[GridIndexFromCoord(i,j,k)].walkable = true;
                        //DebugDraw::Instance()->AddWireSphere(quantized_pos, 0.1f, vec3(1.0f), _delete_on_update);
                        //DebugDraw::Instance()->AddLine(quantized_pos, quantized_pos + norm, vec3(1.0f,0.0f,0.0f), _delete_on_update);
                    } else {
                        if(cb.true_closest_hit_fraction != 1.0f){
                            //DebugDraw::Instance()->AddWireSphere(sphere_pos, _leg_sphere_size * 1.1f, vec3(0.0f,1.0f,0.0f), _delete_on_update);
                        }
                        //DebugDraw::Instance()->AddWireSphere(quantized_pos, 0.1f, vec3(1.0f,1.0f,0.0f), _delete_on_update);
                        //DebugDraw::Instance()->AddLine(quantized_pos, quantized_pos + norm, vec3(1.0f,0.0f,0.0f), _delete_on_update);
                    }
                }
            }
        }
    }

    for(int i=1; i<_grid_size-1; ++i){
        for(int j=2; j<_grid_size-2; ++j){
            for(int k=1; k<_grid_size-1; ++k){
                if(grid_cells[GridIndexFromCoord(i,j,k)].walkable == true){
                    bool next_to_nonwalkable = false;
                    {
                        bool dir = true;
                        for(int l=j-2; l<=j+2; ++l){
                            if(grid_cells[GridIndexFromCoord(i-1,l,k)].walkable){
                                dir = false;
                            }
                        }
                        if(dir){
                            next_to_nonwalkable = true;
                        }
                    }
                    {
                        bool dir = true;
                        for(int l=j-2; l<=j+2; ++l){
                            if(grid_cells[GridIndexFromCoord(i+1,l,k)].walkable){
                                dir = false;
                            }
                        }
                        if(dir){
                            next_to_nonwalkable = true;
                        }
                    }
                    {
                        bool dir = true;
                        for(int l=j-2; l<=j+2; ++l){
                            if(grid_cells[GridIndexFromCoord(i,l,k-1)].walkable){
                                dir = false;
                            }
                        }
                        if(dir){
                            next_to_nonwalkable = true;
                        }
                    }
                    {
                        bool dir = true;
                        for(int l=j-2; l<=j+2; ++l){
                            if(grid_cells[GridIndexFromCoord(i,l,k+1)].walkable){
                                dir = false;
                            }
                        }
                        if(dir){
                            next_to_nonwalkable = true;
                        }
                    }

                    vec3 quantized_pos = vec3(int(pos[0]/cell_size), int(pos[1]/cell_size), int(pos[2]/cell_size))*cell_size;
                    quantized_pos[0] += (i - (_grid_size/2)) * cell_size;
                    quantized_pos[1] += (j - (_grid_size/2)) * cell_size;
                    quantized_pos[2] += (k - (_grid_size/2)) * cell_size;
                    if(next_to_nonwalkable){
                        DebugDraw::Instance()->AddWireSphere(grid_cells[GridIndexFromCoord(i,j,k)].avg_collide, 0.01f, vec3(1.0f), _delete_on_update);
                        //DebugDraw::Instance()->AddWireSphere(quantized_pos, 0.1f, vec3(1.0f), _delete_on_update);
                    } else {
                        //DebugDraw::Instance()->AddWireSphere(quantized_pos, 0.1f, vec4(vec3(1.0f),0.2f), _delete_on_update);
                    }
                } else if(grid_cells[GridIndexFromCoord(i,j,k)].boundary) {
                    vec3 quantized_pos = vec3(int(pos[0]/cell_size), int(pos[1]/cell_size), int(pos[2]/cell_size))*cell_size;
                    quantized_pos[0] += (i - (_grid_size/2)) * cell_size;
                    quantized_pos[1] += (j - (_grid_size/2)) * cell_size;
                    quantized_pos[2] += (k - (_grid_size/2)) * cell_size;
                    //DebugDraw::Instance()->AddWireSphere(quantized_pos, 0.1f, vec4(vec3(0.0f,1.0f,0.0f),0.2f), _delete_on_update);
                }
            }
        }
    }*/
}

// static btSoftBody* soft_body = NULL;
// static btRigidBody* soft_body_attach = NULL;

void MovementObject::Moved(Object::MoveType type) {
    Object::Moved(type);
    velocity = vec3(0.0f);
    position = GetTranslation();
    if (type & Object::kRotate) {
        SetRotationFromEditorTransform();
    }
}

void MovementObject::Update(float timestep) {
    // GridTest(position, scenegraph_->bullet_world_, 0.45f);
    ThreadedSound* si = Engine::Instance()->GetSound();
    Online* online = Online::Instance();

    while (message_queue.empty() == false) {
        ReceiveMessage(message_queue.front());
        message_queue.pop();
    }

    std::list<VoiceQueue>::iterator iter;
    for (iter = voice_queue.begin(); iter != voice_queue.end();) {
        VoiceQueue& vq = (*iter);
        vq.delay -= timestep;
        if (vq.delay <= 0.0f) {
            si->SetVolume(voice_sound, 0.0f);
            // si->Stop(voice_sound);
            // SoundGroupRef sgr = SoundGroups::Instance()->ReturnRef(vq.path);
            SoundGroupRef sgr = Engine::Instance()->GetAssetManager()->LoadSync<SoundGroup>(vq.path);
            std::string snd_path = sgr->GetSoundPath();
            // printf("%s\n",snd_path.c_str());
            std::string lpsnc_path = snd_path.substr(0, snd_path.size() - 4) + ".txt";
            // printf("%s\n",lpsnc_path.c_str());
            if (FileExists(lpsnc_path.c_str(), kDataPaths | kModPaths)) {
                // LipSyncFileRef lsf_ref = LipSyncFiles::Instance()->ReturnRef(lpsnc_path.c_str());
                LipSyncFileRef lsf_ref = Engine::Instance()->GetAssetManager()->LoadSync<LipSyncFile>(lpsnc_path.c_str());
                rigged_object_->lipsync_reader.AttachTo(lsf_ref);
            }
            SoundGroupPlayInfo sgpi(*sgr, rigged_object_->GetAvgIKChainPos("head"));
            sgpi.specific_path = snd_path;
            sgpi.created_by_id = GetID();
            sgpi.pitch_shift = vq.pitch;
            sgpi.priority = _sound_priority_high;
            if (controlled) {
                sgpi.priority = _sound_priority_very_high;
            }
            voice_sound = si->CreateHandle(__FUNCTION__);
            si->PlayGroup(voice_sound, sgpi);
            attached_sounds.push_back(voice_sound);
            iter = voice_queue.erase(iter);
        } else {
            ++iter;
        }
    }

    for (int i = (int)attached_sounds.size() - 1; i >= 0; --i) {
        if (!si->IsHandleValid(attached_sounds[i])) {
            attached_sounds.erase(attached_sounds.begin() + i);
        }
    }
    if (!rigged_object_->animated) {
        velocity = rigged_object_->GetAvgVelocity();
    }
    for (unsigned long& attached_sound : attached_sounds) {
        si->SetOcclusionPosition(attached_sound, position);
        si->TranslatePosition(attached_sound, velocity * timestep);
        // DebugDraw::Instance()->AddWireSphere(si->GetPosition(attached_sounds[i]), 1.0f, vec4(1.0f), _delete_on_update);
    }

    if (controlled && (Engine::Instance()->GetSplitScreen() || controller_id == 0)) {
        ActiveCameras::Set(controller_id);
        ActiveCameras::Get()->IncrementProgress();
    }

    --update_script_counter;
    if (update_script_counter <= 0) {
        ASArglist args;
        args.Add(update_script_period);
        {
            PROFILER_ZONE(g_profiler_ctx, "Angelscript Update()");

            as_context->CallScriptFunction(as_funcs.update, &args);
            angle_script_ready = true;
        }

        update_script_counter += update_script_period;
    }

    // Get data from host, if we are a client and won't generate it locally.
    if (online->IsClient()) {
        list<OnlineMessageRef>& frames = incoming_movement_object_frames;

        while (frames.size() > 1) {
            network_time_interpolator.timestamps.Clear();
            for (const OnlineMessageRef& frame : frames) {
                // TODO: Optimized this multiple GetData() call into a single grouped GetDatas() to reduce lock count to one
                MovementObjectUpdate* frame_data = static_cast<MovementObjectUpdate*>(frame.GetData());
                network_time_interpolator.timestamps.PushValue(frame_data->timestamp);
            }
            int interpolator_code = network_time_interpolator.Update();

            if (interpolator_code == 1) {
                continue;
            } else if (interpolator_code == 2) {
                last_walltime_diff = network_time_interpolator.timestamps.from_begin(1) - network_time_interpolator.timestamps.from_begin(0);
                frames.pop_front();
                continue;
            } else if (interpolator_code == 3) {
                break;
            }

            MovementObjectUpdate* current_frame = static_cast<MovementObjectUpdate*>(frames.begin()->GetData());
            MovementObjectUpdate* next_frame = static_cast<MovementObjectUpdate*>(std::next(frames.begin())->GetData());

            float interpolation_step = network_time_interpolator.interpolation_step;

            if (disable_network_bone_interpolation) {
                interpolation_step = 0.0f;
            }

            position = lerp(current_frame->position, next_frame->position, interpolation_step);
            velocity = lerp(current_frame->velocity, next_frame->velocity, interpolation_step);
            facing = lerp(current_frame->facing, next_frame->facing, interpolation_step);

            size_t bone_count = rigged_object_->skeleton_.physics_bones.size();
            rigged_object_->network_display_bone_matrices.resize(bone_count);
            for (size_t i = 0; i < bone_count; i++) {
                rigged_object_->network_display_bone_matrices[i] = RiggedObject::InterpolateBetweenTwoBones(current_frame->rigged_body_frame.bones[i], next_frame->rigged_body_frame.bones[i], interpolation_step);
            }

            break;
        }

        for (auto& it : angelscript_update) {
            as_context->CallMPCallBack(it.first, it.second);
        }
        angelscript_update.clear();
    }

    if (as_context->HasFunction(as_funcs.update_multiplayer)) {
        ASArglist args;
        args.Add(update_script_period);
        as_context->CallScriptFunction(as_funcs.update_multiplayer, &args);
    }

    if (online->IsClient()) {
        if (incoming_cut_lines.size() > 0) {
            CutLine* cutline = static_cast<CutLine*>(incoming_cut_lines.begin()->GetData());

            rigged_object_->MPCutPlane(cutline->normal, cutline->pos, cutline->dir, cutline->type, cutline->depth, cutline->hit_list, cutline->points);

            rigged_object()->blood_surface.Update(scenegraph_, timestep);

            incoming_cut_lines.pop_front();
        }

        // TODO: Fix listener update in host.
        // This should be controlled in angelscript, same way as on the host side, to ensure that the level of control is equivalent.
        // This cannot be tied to the character, not only is it wrong, as by default the audio listener is usually on the camera position.
        // But the support of a custom listener location has likely been used in third party mods. /Max
        // ODOT
        if (controlled && !remote) {
            Engine::Instance()->GetSound()->updateListener(position, velocity, facing, vec3(0.0, 1.0, 0.0));
        }

        if (incoming_material_sound_events.size() > 0) {
            MaterialSoundEvent* mse = static_cast<MaterialSoundEvent*>(incoming_material_sound_events.begin()->GetData());
            HandleMovementObjectMaterialEvent(mse->event_name, mse->pos, mse->gain);
            incoming_material_sound_events.pop_front();
        }

        if (game_timer.game_time < rigged_object_->blood_surface.sleep_time) {
            rigged_object_->blood_surface.Update(Engine::Instance()->GetSceneGraph(), timestep);
        }

    } else {
        ActiveCameras::Set(0);

        if (rigged_object_->animated) {
            position += velocity * timestep;
        } else {
            position = rigged_object_->GetAvgPosition();
            velocity = rigged_object_->GetAvgVelocity();
        }

        // if(controlled){
        //     ApplyCameraControls();
        // }
        vec3 ground_pos = position - vec3(0.0f, _leg_sphere_size + rigged_object_->floor_height, 0.0f);
        rigged_object_->SetTranslation(ground_pos);
        rigged_object_->static_char = static_char;
        rigged_object_->Update(timestep);

        position += rigged_object_->FetchCenterOffset();
        float rot = rigged_object_->FetchRotation();
        if (rot) {
            mat4 rot_mat;
            rot_mat.SetRotationY(rot);
            rigged_object_->anim_client.SetRotationMatrix(
                rot_mat * rigged_object_->anim_client.GetRotationMatrix());
        }

        char_sphere->SetPosition(position);
        char_sphere->body->setInterpolationWorldTransform(char_sphere->body->getWorldTransform());
        char_sphere->UpdateTransform();
        char_sphere->Activate();
        /*DebugDraw::Instance()->AddWireSphere(char_sphere->GetPosition(), _leg_sphere_size, vec3(1.0), _delete_on_update);
        btVector3 min_val, max_val;
        {
                char_sphere->body->getAabb(min_val, max_val);
                vec3 a(min_val[0], min_val[1], min_val[2]);
                vec3 b(max_val[0], max_val[1], max_val[2]);
                DebugDraw::Instance()->AddWireBox((b+a)*0.5, b-a, vec3(1.0), _delete_on_update);
        }
        {
                min_val = char_sphere->body->getBroadphaseProxy()->m_aabbMin;
                max_val = char_sphere->body->getBroadphaseProxy()->m_aabbMax;
                vec3 a(min_val[0], min_val[1], min_val[2]);
                vec3 b(max_val[0], max_val[1], max_val[2]);
                DebugDraw::Instance()->AddWireBox((b+a)*0.5, b-a, vec3(1.0), _delete_on_update);
        }*/

        /*if(Input::Instance()->getKeyboard().wasKeyPressed(SDLK_h)){
                soft_body = scenegraph_->bullet_world_->AddCloth(position);
                soft_body_attach = new btRigidBody(0.0f, NULL, NULL);
                btTransform transform;
                transform.setIdentity();
                transform.setOrigin(btVector3(position[0], position[1], position[2]));
                soft_body_attach->setWorldTransform(transform);
                for(int i=0; i<31; ++i){
                        soft_body->appendAnchor(i, soft_body_attach, btVector3(0+i*1.6/32.0f,0,0), false, 1.0f);
                }
        }

        if(soft_body){
                int num_nodes = soft_body->m_nodes.size();
                for(int i=0;i<num_nodes;++i) {
                        const btVector3 &point = soft_body->m_nodes[i].m_x;
                        DebugDraw::Instance()->AddWireSphere(vec3(point[0], point[1], point[2]), 0.01f, vec4(1.0f), _delete_on_update);
                }
                btTransform transform;
                transform.setIdentity();
                transform.setOrigin(btVector3(position[0], position[1]+0.5f, position[2]));
                soft_body_attach->setWorldTransform(transform);
        }*/

        if (was_controlled != controlled && scenegraph_) {
            std::vector<Object*>::iterator hotit;
            for (hotit = scenegraph_->hotspots_.begin();
                 hotit != scenegraph_->hotspots_.end();
                 hotit++) {
                Hotspot* hs = static_cast<Hotspot*>(*hotit);
                if (controlled) {
                    hs->HandleEvent("engaged_player_control", this);
                } else {
                    hs->HandleEvent("disengaged_player_control", this);
                }
            }
            was_controlled = controlled;
        }

        /*
        if (9 == GetID()) {
            static float y = 0.f;
            y += 0.002f * timestep; // timestep
            position.y() = y;
        }
        */
    }

    return;
}

mat4 RotationFromVectors(const vec3& front,
                         const vec3& right,
                         const vec3& up) {
    mat4 the_matrix;
    the_matrix.SetColumn(0, right);
    the_matrix.SetColumn(1, up);
    the_matrix.SetColumn(2, front);
    return the_matrix;
}

void MovementObject::SetRotationFromFacing(vec3 facing) {
    if (length_squared(facing) < 0.00001f) {
        return;
    }
    vec3 front = normalize(facing);
    vec3 straight_up = vec3(0.0f, 1.0f, 0.0f);
    vec3 right = normalize(cross(straight_up, facing));
    vec3 up = normalize(cross(front, right));

    mat4 rotation = RotationFromVectors(front, right, up);
    if (rigged_object_.get()) {
        rigged_object_->anim_client.SetRotationMatrix(rotation);
    }
}

void MovementObject::GetDisplayName(char* buf, int buf_size) {
    if (GetName().empty()) {
        FormatString(buf, buf_size, "%d: Character: %s", GetID(), obj_file.c_str());
    } else {
        FormatString(buf, buf_size, "%s: Character: %s", GetName().c_str(), obj_file.c_str());
    }
}

void MovementObject::RemapReferences(std::map<int, int> id_map) {
    connection_finalization_remap = id_map;
    do_connection_finalization_remap = true;
}

vec3 MovementObject::GetFacing() {
    return rigged_object_->anim_client.GetRotationMatrix() * vec3(0.0f, 0.0f, 1.0f);
}

void MovementObject::Ragdoll() {
    if (!rigged_object_->animated) {
        return;
    }
    rigged_object_->Ragdoll(velocity);
}

void MovementObject::ApplyForce(vec3 force) {
    ASArglist args;
    args.AddObject(&force);
    as_context->CallScriptFunction(as_funcs.force_applied, &args);
}

void MovementObject::UnRagdoll() {
    position = rigged_object_->GetAvgPosition();
    velocity = rigged_object_->GetAvgVelocity();
    rigged_object_->UnRagdoll();
}

void MovementObject::SetAnimation(std::string path, float fade_speed, char flags) {
    rigged_object_->anim_client.SetAnimation(path, fade_speed, flags);
    rigged_object_->SetCharAnim("");
}

void MovementObject::SetAnimation(std::string path, float fade_speed) {
    rigged_object_->anim_client.SetAnimation(path, fade_speed);
    rigged_object_->SetCharAnim("");
}

void MovementObject::SetAnimation(std::string path) {
    rigged_object_->anim_client.SetAnimation(path);
    rigged_object_->SetCharAnim("");
}

void MovementObject::OverrideCharAnim(const std::string& label, const std::string& new_path) {
    character_script_getter.OverrideCharAnim(label, new_path);
}

void MovementObject::SwapAnimation(std::string path) {
    rigged_object_->anim_client.SwapAnimation(path);
    rigged_object_->SetCharAnim("");
}

void MovementObject::SetCharAnimation(const std::string& path, float fade_speed, char flags) {
    char extra_flags = character_script_getter.GetAnimFlags(path);
    if (extra_flags & _ANM_MIRRORED) {
        flags = flags ^ _ANM_MIRRORED;
    }
    rigged_object_->anim_client.SetAnimation(character_script_getter.GetAnimPath(path), fade_speed, flags);
    rigged_object_->SetCharAnim(path, flags);
}

void MovementObject::ASSetCharAnimation(std::string path, float fade_speed, char flags) {
    if (Online::Instance()->IsClient())
        return;

    SetCharAnimation(path, fade_speed, flags);
}

void MovementObject::SetAnimAndCharAnim(std::string path, float fade_speed, char flags, std::string anim_path) {
    rigged_object_->anim_client.SetAnimation(path, fade_speed, flags);
    rigged_object_->SetCharAnim(anim_path, flags, path);
}

void MovementObject::ASSetCharAnimation(std::string path, float fade_speed) {
    SetCharAnimation(path, fade_speed);
}

void MovementObject::ASSetCharAnimation(std::string path) {
    SetCharAnimation(path);
}

vec4 MovementObject::GetAvgRotationVec4() {
    quaternion quat = rigged_object_->GetAvgRotation();
    return vec4(quat.entries[0], quat.entries[1], quat.entries[2], quat.entries[3]);
}

float MovementObject::GetTempHealth() {
    ASArglist args;
    float val;
    ASArg return_val;
    return_val.type = _as_float;
    return_val.data = &val;
    as_context->CallScriptFunction(as_funcs.get_temp_health, &args, &return_val);
    return val;
}

void MovementObject::addAngelScriptUpdate(uint32_t state, std::vector<uint32_t> data) {
    std::pair<uint32_t, std::vector<uint32_t>> val(state, data);
    angelscript_update.push_back(val);
}

bool MovementObject::HasFunction(const std::string& function_definition) {
    bool result = false;
    result = as_context->HasFunction(function_definition);
    return result;
}

int MovementObject::QueryIntFunction(std::string func) {
    ASArglist args;
    int val;
    ASArg return_val;
    return_val.type = _as_int;
    return_val.data = &val;
    as_context->CallScriptFunction(func, &args, &return_val);
    return val;
}

void MovementObject::HandleMovementObjectMaterialEvent(std::string the_event, vec3 event_pos, float gain) {
    Online* online = Online::Instance();

    if (event_pos != event_pos) {
        return;
    }

    if (online->IsHosting()) {
        online->Send<OnlineMessages::MaterialSoundEvent>(GetID(), the_event, event_pos, gain);
    }

    const MaterialEvent* me = scenegraph_->GetMaterialEvent(the_event, event_pos, character_script_getter.GetSoundMod());
    if (me && !me->soundgroup.empty()) {
        // SoundGroupRef sgr = SoundGroups::Instance()->ReturnRef(me->soundgroup);
        SoundGroupRef sgr = Engine::Instance()->GetAssetManager()->LoadSync<SoundGroup>(me->soundgroup);
        touched_surface_refs.insert(sgr);
        SoundGroupPlayInfo sgpi(*sgr, event_pos);
        sgpi.gain = gain;
        sgpi.occlusion_position = position;
        if (me->max_distance != 0.0f) {
            sgpi.max_distance = me->max_distance * gain;
        }
        sgpi.created_by_id = GetID();
        if (controlled) {
            sgpi.priority = _sound_priority_very_high;
        }
        unsigned long sound_handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
        Engine::Instance()->GetSound()->PlayGroup(sound_handle, sgpi);
        if (sound_handle != 0 && me->attached) {
            attached_sounds.push_back(sound_handle);
        }
    }

    const std::string clothing_path = character_script_getter.GetClothingPath();
    if (!clothing_path.empty()) {
        // MaterialRef clothing = Materials::Instance()->ReturnRef(clothing_path);
        MaterialRef clothing = Engine::Instance()->GetAssetManager()->LoadSync<Material>(clothing_path);
        clothing_refs.insert(clothing);
        const MaterialEvent* me = &clothing->GetEvent(the_event);
        if (me && !me->soundgroup.empty()) {
            // SoundGroupRef sgr = SoundGroups::Instance()->ReturnRef(me->soundgroup);
            SoundGroupRef sgr = Engine::Instance()->GetAssetManager()->LoadSync<SoundGroup>(me->soundgroup);
            touched_surface_refs.insert(sgr);
            SoundGroupPlayInfo sgpi(*sgr, event_pos);
            sgpi.gain = gain;
            sgpi.created_by_id = GetID();
            sgpi.occlusion_position = position;
            if (me->max_distance != 0.0f) {
                sgpi.max_distance = me->max_distance * gain;
            }
            if (controlled) {
                sgpi.priority = _sound_priority_very_high;
            }
            unsigned long sound_handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
            Engine::Instance()->GetSound()->PlayGroup(sound_handle, sgpi);
            if (sound_handle != 0 && me->attached) {
                attached_sounds.push_back(sound_handle);
            }
        }
    }
}

void MovementObject::HandleMovementObjectMaterialEventDefault(std::string the_event, vec3 event_pos) {
    HandleMovementObjectMaterialEvent(the_event, event_pos);
}

void MovementObject::MaterialParticleAtBone(std::string type, std::string bone_name) {
    int bone = rigged_object_->skeleton_.simple_ik_bones[bone_name].bone_id;
    vec3 pos = rigged_object_->GetBonePosition(bone);
    vec3 vel = rigged_object_->GetBoneLinearVel(bone);

    const MaterialParticle* mp = scenegraph_->GetMaterialParticle(type, pos);
    if (!mp || mp->particle_path.empty()) {
        return;
    }
    pos += vec3(0.0f, -0.1f, 0.0f);
    vel = vec3(0.0f, 1.0f * min(1.0f, length(vel)), 0.0f);
    vec3 tint = scenegraph_->GetColorAtPoint(pos);
    scenegraph_->particle_system->MakeParticle(
        scenegraph_, mp->particle_path, pos, vel, tint);
}

static void asMaterialParticle(MovementObject* mo, const std::string& type, const vec3& pos, const vec3& vel) {
    const MaterialParticle* mp = mo->scenegraph_->GetMaterialParticle(type, pos);
    if (!mp || mp->particle_path.empty()) {
        return;
    }
    vec3 particle_pos = pos;
    vec3 tint = mo->scenegraph_->GetColorAtPoint(pos);
    mo->scenegraph_->particle_system->MakeParticle(
        mo->scenegraph_, mp->particle_path, particle_pos, vel, tint);
}

int MovementObject::ASWasHit(std::string type, std::string attack_path, vec3 dir, vec3 pos, int attacker_id, float attack_damage_mult, float attack_knockback_mult) {
    ASArglist args;
    args.AddObject(&type);
    args.AddObject(&attack_path);
    args.AddObject(&dir);
    args.AddObject(&pos);
    args.Add(attacker_id);
    args.Add(attack_damage_mult);
    args.Add(attack_knockback_mult);
    int val;
    ASArg return_val;
    return_val.type = _as_int;
    return_val.data = &val;
    as_context->CallScriptFunction(as_funcs.was_hit, &args, &return_val);
    return val;
}

void MovementObject::ASWasBlocked() {
    as_context->CallScriptFunction(as_funcs.was_blocked);
}

void MovementObject::CreateRiggedObject() {
    OGPalette old_palette = palette;
    // for(int i=0; i<100; ++i){
    rigged_object_.reset(new RiggedObject());
    rigged_object_->char_id = GetID();
    palette.clear();
    rigged_object_->SetCharacterScriptGetter(character_script_getter);

    if (GetScriptParams()->HasParam("Character Scale")) {
        float char_scale = GetScriptParams()->ASGetFloat("Character Scale");
        float new_radius = char_scale * 2;
        rigged_object_->SetCharScale(char_scale);
        //if (new_radius <= 2.0f) {
        //    kCullRadius = 2.0f;
        //} else if (new_radius >= 200.0f) {
        //    kCullRadius = 200.0f;
        //} else {
        //    kCullRadius = new_radius;
        //}

        //kCullRadius = min(200.0f, max(2.0f, new_radius));
        kCullRadius = clamp(new_radius, 2.0f, 200.0f);

    } else {
        kCullRadius = 2.0f;
        rigged_object_->SetCharScale(1.0f);
    }

    {
        PROFILER_ZONE(g_profiler_ctx, "rigged_object_->Load");
        rigged_object_->Load(character_path,
                             position,
                             scenegraph_,
                             palette);
    }
    //}
    for (auto& i : palette) {
        for (auto& j : old_palette) {
            if (i.label == j.label) {
                i.color = j.color;
            }
        }
    }
    rigged_object_->SetSkeletonOwner(this);
    rigged_object_->Initialize();
    rigged_object_->animated = true;
    rigged_object_->SetASContext(as_context.get());
    rigged_object_->ApplyPalette(palette);
    if (!env_object_attach_data.empty()) {
        Deserialize(env_object_attach_data, rigged_object_->children);
    }
}

void MovementObject::RecreateRiggedObject(std::string _char_path) {
    character_path = _char_path;
    CreateRiggedObject();
    update_script_period = (current_control_script_path == scenegraph_->level->GetPCScript(this)) ? 1 : 4;
    update_script_counter = rand() % update_script_period - update_script_period;
    rigged_object_->SetAnimUpdatePeriod(max(2, update_script_period));
    SetRotationFromEditorTransform();
}

int MovementObject::GetWaypointTarget() {
    return connected_pathpoint_id;
}

void MovementObject::Reset() {
    as_context->CallScriptFunction(as_funcs.reset);
    position = GetTranslation();
    char_sphere->SetPosition(position);
    char_sphere->body->setInterpolationWorldTransform(char_sphere->body->getWorldTransform());
    char_sphere->UpdateTransform();
    SetRotationFromEditorTransform();
    velocity = vec3(0.0f);
    no_grab = 0;
    as_context->ResetGlobals();
    as_context->CallScriptFunction(as_funcs.set_parameters);
    as_context->CallScriptFunction(as_funcs.post_reset);
    for (auto& item_connection : item_connections) {
        if (item_connection.attachment_type != _at_unspecified) {
            int which = item_connection->GetID();
            AttachmentType type = item_connection.attachment_type;
            bool mirrored = item_connection.attachment_mirror;

            AttachItemToSlotAttachmentRef(which, type, mirrored, &item_connection.attachment_ref);
            ASArglist args;
            args.Add(which);
            args.Add(type);
            args.Add(mirrored);
            as_context->CallScriptFunction(as_funcs.handle_editor_attachment, &args);
        } else {
            ASArglist args;
            args.Add(item_connection->GetID());
            switch (item_connection->item_ref()->GetType()) {
                case _weapon:
                    as_context->CallScriptFunction(as_funcs.attach_weapon, &args);
                    break;
                case _misc:
                    as_context->CallScriptFunction(as_funcs.attach_misc, &args);
                    break;
                default:
                    DisplayError("Error", "Resetting MovementObject with Item of unknown type");
                    break;
            }
        }
    }

    RegisterMPCallbacks();
}

bool MovementObject::ASOnSameTeam(MovementObject* other) {
    if (other) {
        CSLIterator iter(sp.GetStringVal("Teams"));
        std::string team;

        while (iter.GetNext(&team)) {
            if (other->OnTeam(team)) {
                return true;
            }
        }
    } else {
        LOGE << "Received null pointer to movement object" << std::endl;
    }
    return false;
}

bool MovementObject::OnTeam(const std::string& other_team) {
    CSLIterator iter(sp.GetStringVal("Teams"));
    std::string team;

    while (iter.GetNext(&team)) {
        if (team == other_team) {
            return true;
        }
    }
    return false;
}

static void FixDiscontinuity(MovementObject* mo) {
    mo->rigged_object()->static_char = mo->static_char;
    mo->rigged_object()->FixDiscontinuity();
}

static std::string ASGetCurrentControlScript(MovementObject* mo) {
    return mo->GetCurrentControlScript();
}

static std::string AsGetActorScript(MovementObject* mo) {
    return mo->GetActorScript();
}

static std::string AsGetNPCObjectScript(MovementObject* mo) {
    return mo->GetNPCObjectScript();
}

static std::string ASGetPCObjectScript(MovementObject* mo) {
    return mo->GetPCObjectScript();
}

static std::string ASGetMovementObjectNPCScript(MovementObject* mo) {
    return mo->scenegraph_->level->GetNPCScript(mo);
}

static std::string ASGetMovementObjectPCScript(MovementObject* mo) {
    return mo->scenegraph_->level->GetPCScript(mo);
}

void DefineMovementObjectTypePublic(ASContext* as_context) {
    as_context->RegisterGlobalProperty("const int _awake", (void*)&MovementObject::_awake);
    as_context->RegisterGlobalProperty("const int _unconscious", (void*)&MovementObject::_unconscious);
    as_context->RegisterGlobalProperty("const int _dead", (void*)&MovementObject::_dead);

    as_context->RegisterObjectType("MovementObject", 0, asOBJ_REF | asOBJ_NOCOUNT);
    as_context->RegisterObjectProperty("MovementObject", "vec3 position", asOFFSET(MovementObject, position));
    as_context->RegisterObjectProperty("MovementObject", "vec3 velocity", asOFFSET(MovementObject, velocity));
    as_context->RegisterObjectProperty("MovementObject", "bool visible", asOFFSET(MovementObject, visible));
    as_context->RegisterObjectProperty("MovementObject", "int no_grab", asOFFSET(MovementObject, no_grab));
    as_context->RegisterObjectProperty("MovementObject", "string char_path", asOFFSET(MovementObject, character_path));
    as_context->RegisterObjectProperty("MovementObject", "bool controlled", asOFFSET(MovementObject, controlled));
    as_context->RegisterObjectProperty("MovementObject", "bool remote", asOFFSET(MovementObject, remote));
    as_context->RegisterObjectProperty("MovementObject", "bool is_player", asOFFSET(MovementObject, is_player));
    as_context->RegisterObjectProperty("MovementObject", "bool static_char", asOFFSET(MovementObject, static_char));
    as_context->RegisterObjectProperty("MovementObject", "int controller_id", asOFFSET(MovementObject, controller_id));
    as_context->RegisterObjectProperty("MovementObject", "int update_script_counter", asOFFSET(MovementObject, update_script_counter));
    as_context->RegisterObjectProperty("MovementObject", "int update_script_period", asOFFSET(MovementObject, update_script_period));
    as_context->RegisterObjectProperty("MovementObject", "bool focused_character", asOFFSET(MovementObject, focused_character));

    as_context->RegisterObjectMethod("MovementObject", "string GetTeamString()", asMETHOD(MovementObject, GetTeamString), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "RiggedObject@ rigged_object()", asMETHOD(MovementObject, rigged_object), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void SetScriptUpdatePeriod(int steps)", asMETHOD(MovementObject, ASSetScriptUpdatePeriod), asCALL_THISCALL, "Script update() is called every X engine timesteps");
    as_context->RegisterObjectMethod("MovementObject", "bool HasFunction(const string &in function_definition)", asMETHOD(MovementObject, HasFunction), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "int QueryIntFunction(string function)", asMETHOD(MovementObject, QueryIntFunction), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void Execute(string code)", asMETHOD(MovementObject, Execute), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "bool HasVar(string var_name)", asMETHOD(MovementObject, ASHasVar), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "int GetIntVar(string var_name)", asMETHOD(MovementObject, ASGetIntVar), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "int GetArrayIntVar(const string &in var_name, int index)", asMETHOD(MovementObject, ASGetArrayIntVar), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "float GetFloatVar(string var_name)", asMETHOD(MovementObject, ASGetFloatVar), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "bool GetBoolVar(string var_name)", asMETHOD(MovementObject, ASGetBoolVar), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void SetIntVar(string var_name, int value)", asMETHOD(MovementObject, ASSetIntVar), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void SetArrayIntVar(const string &in var_name, int index, int value)", asMETHOD(MovementObject, ASSetArrayIntVar), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void SetFloatVar(string var_name, float value)", asMETHOD(MovementObject, ASSetFloatVar), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void SetBoolVar(string var_name, bool value)", asMETHOD(MovementObject, ASSetBoolVar), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "bool OnSameTeam(MovementObject@ other)", asMETHOD(MovementObject, ASOnSameTeam), asCALL_THISCALL);
    // Keeping ReceiveMessage for compatibility
    as_context->RegisterObjectMethod("MovementObject", "void ReceiveMessage(const string &in)", asMETHOD(MovementObject, ReceiveScriptMessage), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void ReceiveScriptMessage(const string &in)", asMETHOD(Object, ReceiveScriptMessage), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void QueueScriptMessage(const string &in)", asMETHOD(Object, QueueScriptMessage), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "int GetID()", asMETHOD(MovementObject, GetID), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void SetRotationFromFacing(vec3 facing)", asMETHOD(MovementObject, SetRotationFromFacing), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void FixDiscontinuity()", asFUNCTION(FixDiscontinuity), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("MovementObject", "string GetCurrentControlScript()", asFUNCTION(ASGetCurrentControlScript), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("MovementObject", "string GetActorScript()", asFUNCTION(AsGetActorScript), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("MovementObject", "string GetNPCObjectScript()", asFUNCTION(AsGetNPCObjectScript), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("MovementObject", "string GetPCObjectScript()", asFUNCTION(ASGetPCObjectScript), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("MovementObject", "string GetNPCScript()", asFUNCTION(ASGetMovementObjectNPCScript), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("MovementObject", "string GetPCScript()", asFUNCTION(ASGetMovementObjectPCScript), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("MovementObject", "void ChangeControlScript(const string &path)", asMETHOD(MovementObject, ChangeControlScript), asCALL_THISCALL);
}

namespace {
int GetNumBoneChildren(CScriptArray* bone_children_index, int bone) {
    return *(int*)bone_children_index->At(bone + 1) - *(int*)bone_children_index->At(bone);
}

enum ChainPointLabels { kHandPoint,
                        kWristPoint,
                        kElbowPoint,
                        kShoulderPoint,
                        kCollarTipPoint,
                        kCollarPoint,
                        kNumArmPoints };
enum IKLabel { kLeftArmIK,
               kRightArmIK,
               kLeftLegIK,
               kRightLegIK,
               kHeadIK,
               kLeftEarIK,
               kRightEarIK,
               kTorsoIK,
               kTailIK,
               kNumIK };
enum IdleType { _stand,
                _active,
                _combat };

// Key transform enums
const int kHeadKey = 0;
const int kLeftArmKey = 1;
const int kRightArmKey = 2;
const int kLeftLegKey = 3;
const int kRightLegKey = 4;
const int kChestKey = 5;
const int kHipKey = 6;
const int kNumKeys = 7;

enum Species {
    _rabbit = 0,
    _wolf = 1,
    _dog = 2,
    _rat = 3,
    _cat = 4
};

static void CDrawArmsImpl(
    MovementObject& this_mo,
    RiggedObject& rigged_object,
    Skeleton& skeleton,
    float& breath_amount,
    float& max_speed,
    float& true_max_speed,
    int& idle_type,
    bool& on_ground,
    bool& flip_info_flipping,
    float& threat_amount,
    bool& ledge_info_on_ledge,
    CScriptArray* skeleton_bind_transforms,
    CScriptArray* inv_skeleton_bind_transforms,
    CScriptArray* ik_chain_start_index,
    CScriptArray* ik_chain_elements,
    CScriptArray* ik_chain_length,
    CScriptArray* key_transforms,
    CScriptArray* arm_points,
    CScriptArray* old_arm_points,
    CScriptArray* temp_old_arm_points,
    CScriptArray* bone_children,
    CScriptArray* bone_children_index,
    const BoneTransform& chest_transform, const BoneTransform& l_hand_transform, const BoneTransform& r_hand_transform, int num_frames) {
    // Get relative chest transformation
    int chest_bone = ASIKBoneStart(&skeleton, "torso");
    BoneTransform chest_frame_matrix = rigged_object.animation_frame_bone_matrices[chest_bone];
    BoneTransform chest_bind_matrix = *(BoneTransform*)skeleton_bind_transforms->At(chest_bone);
    BoneTransform rel_mat = chest_transform * invert(chest_frame_matrix * chest_bind_matrix);

    // Get points in arm IK chain transformed by chest
    float upper_arm_length[2];
    float upper_arm_weight[2];
    float lower_arm_length[2];
    float lower_arm_weight[2];
    vec3 chain_points[kNumArmPoints * 2];

    for (int right = 0; right < 2; ++right) {
        int chain_start = *(int*)ik_chain_start_index->At(kLeftArmIK + right);
        vec3 hand = GetTransformedBonePoint(&rigged_object, *(int*)ik_chain_elements->At(chain_start + 0), 1);
        vec3 wrist = GetTransformedBonePoint(&rigged_object, *(int*)ik_chain_elements->At(chain_start + 0), 0);
        vec3 elbow = GetTransformedBonePoint(&rigged_object, *(int*)ik_chain_elements->At(chain_start + 2), 0);
        vec3 shoulder = GetTransformedBonePoint(&rigged_object, *(int*)ik_chain_elements->At(chain_start + 4), 0);
        vec3 collar_tip = GetTransformedBonePoint(&rigged_object, *(int*)ik_chain_elements->At(chain_start + 5), 1);
        vec3 collar = GetTransformedBonePoint(&rigged_object, *(int*)ik_chain_elements->At(chain_start + 5), 0);

        // Get more metrics about arm lengths
        upper_arm_length[right] = distance(elbow, shoulder);
        lower_arm_length[right] = distance(wrist, elbow);

        vec3 mid_low_arm = GetTransformedBonePoint(&rigged_object, *(int*)ik_chain_elements->At(chain_start + 1), 0);
        vec3 mid_up_arm = GetTransformedBonePoint(&rigged_object, *(int*)ik_chain_elements->At(chain_start + 3), 0);

        lower_arm_weight[right] = distance(elbow, mid_low_arm) / distance(wrist, elbow);
        upper_arm_weight[right] = distance(shoulder, mid_up_arm) / distance(shoulder, elbow);

        BoneTransform world_chest = *(BoneTransform*)key_transforms->At(kChestKey) * *(BoneTransform*)inv_skeleton_bind_transforms->At(*(int*)ik_chain_start_index->At(kTorsoIK));
        vec3 breathe_dir = world_chest.rotation * normalize(vec3(0.0f, -0.3f, 1.0f));
        float scale = rigged_object.GetCharScale();
        shoulder += breathe_dir * breath_amount * 0.005f * scale;

        BoneTransform old_hand_transform;
        {  // Apply traditional IK
            vec3 old_wrist = wrist;
            vec3 old_shoulder = shoulder;
            BoneTransform hand_transform = right == 1 ? r_hand_transform : l_hand_transform;
            shoulder = rel_mat * shoulder;

            hand = hand_transform * skeleton.points[skeleton.bones[*(int*)ik_chain_elements->At(chain_start + 0)].points[1]];
            wrist = hand_transform * skeleton.points[skeleton.bones[*(int*)ik_chain_elements->At(chain_start + 0)].points[0]];

            // Rotate arm to match new IK pos
            quaternion rotation;
            GetRotationBetweenVectors(old_wrist - old_shoulder, wrist - shoulder, rotation);
            elbow = shoulder + ASMult(rotation, elbow - old_shoulder);

            // Scale to put elbow in approximately the right place
            float old_length = distance(old_wrist, old_shoulder);
            float new_length = distance(wrist, shoulder);
            elbow = shoulder + (elbow - shoulder) * (new_length / old_length);

            // Enforce arm lengths to finalize elbow position
            const int iterations = 2;
            for (int i = 0; i < iterations; ++i) {
                vec3 offset;
                offset += (shoulder + normalize(elbow - shoulder) * upper_arm_length[right]) - elbow;
                offset += (wrist + normalize(elbow - wrist) * lower_arm_length[right]) - elbow;
                elbow += offset;
            }
        }

        collar_tip = rel_mat * collar_tip;
        collar = rel_mat * collar;

        int points_offset = kNumArmPoints * right;
        chain_points[kHandPoint + points_offset] = hand;
        chain_points[kWristPoint + points_offset] = wrist;
        chain_points[kElbowPoint + points_offset] = elbow;
        chain_points[kShoulderPoint + points_offset] = shoulder;
        chain_points[kCollarTipPoint + points_offset] = collar_tip;
        chain_points[kCollarPoint + points_offset] = collar;
    }

    old_arm_points->Resize(6);
    temp_old_arm_points->Resize(6);
    if (arm_points->GetSize() != 6) {  // Initialize arm physics particles
        arm_points->Resize(6);
        *(vec3*)arm_points->At(0) = chain_points[kShoulderPoint];
        *(vec3*)arm_points->At(1) = chain_points[kElbowPoint];
        *(vec3*)arm_points->At(2) = chain_points[kWristPoint];
        *(vec3*)arm_points->At(3) = chain_points[kShoulderPoint + kNumArmPoints];
        *(vec3*)arm_points->At(4) = chain_points[kElbowPoint + kNumArmPoints];
        *(vec3*)arm_points->At(5) = chain_points[kWristPoint + kNumArmPoints];
        for (int i = 0, len = arm_points->GetSize(); i < len; ++i) {
            *(vec3*)old_arm_points->At(i) = *(vec3*)arm_points->At(i);
            *(vec3*)temp_old_arm_points->At(i) = *(vec3*)arm_points->At(i);
        }
    } else {  // Simulate arm physics
        for (int i = 0; i < 6; ++i) {
            *(vec3*)temp_old_arm_points->At(i) = *(vec3*)arm_points->At(i);
        }
        for (int right = 0; right < 2; ++right) {
            int start = right * 3;

            float arm_drag = 0.92f;
            // Determine how loose the arms should be
            if (max_speed == 0.0f) {
                max_speed = true_max_speed;
            }
            float arm_loose = 1.0f - length(this_mo.velocity) / max_speed;
            if (idle_type == _combat) {
                arm_loose = 0.0f;
            }
            if (!on_ground) {
                arm_loose = 0.7f;
            }
            // TODO: fix
            if (flip_info_flipping) {
                arm_loose = 0.0f;
            }
            arm_loose = max(0.0f, arm_loose - max(0.0f, threat_amount));
            float arm_stiffness = mix(0.9f, 0.97f, arm_loose);
            float shoulder_stiffness = arm_stiffness;
            float elbow_stiffness = arm_stiffness;

            vec3 shoulder = chain_points[kShoulderPoint + kNumArmPoints * right];
            vec3 elbow = chain_points[kElbowPoint + kNumArmPoints * right];
            vec3 wrist = chain_points[kWristPoint + kNumArmPoints * right];
            *(vec3*)arm_points->At(start + 0) = shoulder;
            {  // Apply arm velocity
                vec3 full_vel_offset = this_mo.velocity * game_timer.timestep * (float)num_frames;
                vec3 vel_offset = ((*(vec3*)arm_points->At(start + 1) - *(vec3*)old_arm_points->At(start + 1)) - full_vel_offset) * pow(arm_drag, num_frames) + full_vel_offset;
                *(vec3*)arm_points->At(start + 1) += vel_offset;
                vel_offset = ((*(vec3*)arm_points->At(start + 2) - *(vec3*)old_arm_points->At(start + 2)) - full_vel_offset) * pow(arm_drag, num_frames) + full_vel_offset;
                *(vec3*)arm_points->At(start + 2) += vel_offset;
            }
            quaternion rotation;
            {  // Apply linear force towards target positions
                *(vec3*)arm_points->At(start + 1) += (elbow - *(vec3*)arm_points->At(start + 1)) * (1.0f - pow(shoulder_stiffness, num_frames));
                GetRotationBetweenVectors(elbow - shoulder, *(vec3*)arm_points->At(start + 1) - shoulder, rotation);
                vec3 rotated_tip = ASMult(rotation, wrist - elbow) + elbow;
                *(vec3*)arm_points->At(start + 2) += (rotated_tip - *(vec3*)arm_points->At(start + 2)) * (1.0f - pow(elbow_stiffness, num_frames));
            }
            const char* armblends[] = {"leftarm_blend", "rightarm_blend"};
            float softness_override = rigged_object.GetStatusKeyValue(armblends[right]);
            // softness_override = mix(softness_override, 1.0f, min(1.0f, flip_ik_fade));

            // We want to make the arms stiff when the character is climbing with arms,
            //  TODO: fix
            if (ledge_info_on_ledge) {
                softness_override = 1.0f;
            }

            // Check if character has been teleported and prevent affects of arm physics if that's the case.
            if (length(*(vec3*)arm_points->At(start + 2) - *(vec3*)old_arm_points->At(start + 2)) > 10.0f) {
                // Log(warning, "Massive movement in arm positions detected, skipping arm physics for this frame. TODO\n");
                softness_override = 1.0f;
            }

            {  // Blend with original position to override physics
                *(vec3*)arm_points->At(start + 0) = mix(*(vec3*)arm_points->At(start + 0), shoulder, softness_override);
                *(vec3*)arm_points->At(start + 1) = mix(*(vec3*)arm_points->At(start + 1), elbow, softness_override);
                *(vec3*)arm_points->At(start + 2) = mix(*(vec3*)arm_points->At(start + 2), wrist, softness_override);
            }

            {  // Enforce constraints
                // Get hinge joint info
                int chain_start = *(int*)ik_chain_start_index->At(kLeftArmIK + right);
                BoneTransform elbow_mat = rigged_object.animation_frame_bone_matrices[*(int*)ik_chain_elements->At(chain_start + 3)];
                vec3 elbow_axis = ASMult(rotation, elbow_mat.rotation * vec3(1, 0, 0));
                vec3 elbow_front = ASMult(rotation, elbow_mat.rotation * vec3(0, 1, 0));
                vec3 shoulder_offset, elbow_offset, wrist_offset;

                for (int i = 0; i < 1; ++i) {
                    float iter_strength = 0.75f * (1.0f - softness_override);

                    // Distance constraints
                    elbow_offset = (shoulder + normalize(*(vec3*)arm_points->At(start + 1) - shoulder) * upper_arm_length[right]) - *(vec3*)arm_points->At(start + 1);
                    vec3 mid = (*(vec3*)arm_points->At(start + 1) + *(vec3*)arm_points->At(start + 2)) * 0.5f;
                    vec3 dir = normalize(*(vec3*)arm_points->At(start + 1) - *(vec3*)arm_points->At(start + 2));
                    wrist_offset = (mid - dir * lower_arm_length[right] * 0.5f) - *(vec3*)arm_points->At(start + 2);
                    elbow_offset += (mid + dir * lower_arm_length[right] * 0.5f) - *(vec3*)arm_points->At(start + 1);

                    // Hinge constraints
                    vec3 offset;
                    offset += elbow_axis * dot(elbow_axis, *(vec3*)arm_points->At(start + 2) - *(vec3*)arm_points->At(start + 1));
                    float front_amount = dot(elbow_front, *(vec3*)arm_points->At(start + 2) - *(vec3*)arm_points->At(start + 1));
                    if (front_amount < 0.0f) {
                        offset += elbow_front * front_amount;
                    }
                    elbow_offset += offset * 0.5f;
                    wrist_offset -= offset * 0.5f;

                    // Apply scaled correction vectors
                    *(vec3*)arm_points->At(start + 1) += elbow_offset * iter_strength;
                    *(vec3*)arm_points->At(start + 2) += wrist_offset * iter_strength;
                }
            }
        }
        /*{ // Apply arm physics to actual elbow, hand and wrist positions
            int point_offset = kNumArmPoints;
            vec3 hand = chain_points[kHandPoint+point_offset];
            vec3 wrist = chain_points[kWristPoint+point_offset];
            vec3 elbow = chain_points[kElbowPoint+point_offset];
            int start = 3;
            quaternion hand_rotation;
            GetRotationBetweenVectors(elbow-wrist, *(vec3*)arm_points->At(start+1)-*(vec3*)arm_points->At(start+2), hand_rotation);

            vec3 hand_offset = hand-wrist;
            elbow = *(vec3*)arm_points->At(start+1);
            wrist = *(vec3*)arm_points->At(start+2);
            hand = wrist + Mult(hand_rotation, hand_offset);

            BoneTransform temp_r_hand_transform;
            temp_r_hand_transform.origin = (wrist+hand) * 0.5f;
            temp_r_hand_transform.rotation = hand_rotation * right_hand.rotation;
            BoneTransform temp_l_hand_transform = temp_r_hand_transform*invert(right_hand)*left_hand;
            vec3 offset = temp_l_hand_transform.origin - *(vec3*)arm_points->At(2);
            *(vec3*)arm_points->At(2) += offset * 0.5f;
            // *(vec3*)arm_points->At(5) -= offset * 0.5f;
        }*/
        for (int i = 0; i < 6; ++i) {
            *(vec3*)old_arm_points->At(i) = *(vec3*)temp_old_arm_points->At(i);
        }
    }

    for (int right = 0; right < 2; ++right) {
        int chain_start = *(int*)ik_chain_start_index->At(kLeftArmIK + right);
        int point_offset = right * kNumArmPoints;
        vec3 hand = chain_points[kHandPoint + point_offset];
        vec3 wrist = chain_points[kWristPoint + point_offset];
        vec3 elbow = chain_points[kElbowPoint + point_offset];
        vec3 shoulder = chain_points[kShoulderPoint + point_offset];
        vec3 collar_tip = chain_points[kCollarTipPoint + point_offset];
        vec3 collar = chain_points[kCollarPoint + point_offset];

        {  // Apply arm physics to actual elbow, hand and wrist positions
            int start = right * 3;
            quaternion hand_rotation;
            GetRotationBetweenVectors(elbow - wrist, *(vec3*)arm_points->At(start + 1) - *(vec3*)arm_points->At(start + 2), hand_rotation);

            vec3 hand_offset = hand - wrist;
            // Enforce arm length
            elbow = *(vec3*)arm_points->At(start) + normalize(*(vec3*)arm_points->At(start + 1) - *(vec3*)arm_points->At(start)) * upper_arm_length[right];
            wrist = elbow + normalize(*(vec3*)arm_points->At(start + 2) - *(vec3*)arm_points->At(start + 1)) * lower_arm_length[right];
            hand = wrist + ASMult(hand_rotation, hand_offset);
        }

        BoneTransform old_hand_matrix = rigged_object.animation_frame_bone_matrices[*(int*)ik_chain_elements->At(chain_start + 0)];

        for (int i = 4, len = *(int*)ik_chain_length->At(kLeftArmIK + right); i < len; ++i) {
            rigged_object.animation_frame_bone_matrices[*(int*)ik_chain_elements->At(chain_start + i)] = rel_mat * rigged_object.animation_frame_bone_matrices[*(int*)ik_chain_elements->At(chain_start + i)];
        }

        RotateBoneToMatchVec(&rigged_object, collar, collar_tip, *(int*)ik_chain_elements->At(chain_start + 5));
        RotateBoneToMatchVec(&rigged_object, shoulder, mix(shoulder, elbow, upper_arm_weight[right]), *(int*)ik_chain_elements->At(chain_start + 4));
        RotateBoneToMatchVec(&rigged_object, mix(shoulder, elbow, upper_arm_weight[right]), elbow, *(int*)ik_chain_elements->At(chain_start + 3));
        RotateBoneToMatchVec(&rigged_object, elbow, mix(elbow, wrist, lower_arm_weight[right]), *(int*)ik_chain_elements->At(chain_start + 2));
        RotateBoneToMatchVec(&rigged_object, mix(elbow, wrist, lower_arm_weight[right]), wrist, *(int*)ik_chain_elements->At(chain_start + 1));

        {
            BoneTransform hand_transform = right == 1 ? r_hand_transform : l_hand_transform;
            rigged_object.animation_frame_bone_matrices[*(int*)ik_chain_elements->At(chain_start + 0)] = hand_transform * *(BoneTransform*)inv_skeleton_bind_transforms->At(*(int*)ik_chain_elements->At(chain_start + 0));
            RotateBoneToMatchVec(&rigged_object, wrist, hand, *(int*)ik_chain_elements->At(chain_start + 0));
        }

        BoneTransform hand_rel = rigged_object.animation_frame_bone_matrices[*(int*)ik_chain_elements->At(chain_start + 0)] * invert(old_hand_matrix);

        // Apply hand rotation to child bones (like fingers)
        for (int i = 0, len = GetNumBoneChildren(bone_children_index, *(int*)ik_chain_elements->At(chain_start + 0)); i < len; ++i) {
            int child = *(int*)bone_children->At(*(int*)bone_children_index->At(*(int*)ik_chain_elements->At(chain_start + 0)) + i);
            rigged_object.animation_frame_bone_matrices[child] = hand_rel * rigged_object.animation_frame_bone_matrices[child];
        }
    }
}

static void CDrawArms(MovementObject* mo, const BoneTransform& chest_transform, const BoneTransform& l_hand_transform, const BoneTransform& r_hand_transform, int num_frames) {
    PROFILER_ZONE(g_profiler_ctx, "C++ Draw Arms")
    MovementObject& this_mo = *mo;
    RiggedObject& rigged_object = *this_mo.rigged_object();
    Skeleton& skeleton = rigged_object.skeleton();

    PROFILER_ENTER(g_profiler_ctx, "Get AS pointers");
    ASContext* as_context = this_mo.as_context.get();
    float& breath_amount = *(float*)as_context->module.GetVarPtrCache("breath_amount");
    float& max_speed = *(float*)as_context->module.GetVarPtrCache("max_speed");
    float& true_max_speed = *(float*)as_context->module.GetVarPtrCache("true_max_speed");
    int& idle_type = *(int*)as_context->module.GetVarPtrCache("idle_type");
    bool& on_ground = *(bool*)as_context->module.GetVarPtrCache("on_ground");
    bool& flip_info_flipping = *(bool*)((asIScriptObject*)as_context->module.GetVarPtrCache("flip_info"))->GetAddressOfProperty(0);
    float& threat_amount = *(float*)as_context->module.GetVarPtrCache("threat_amount");
    bool& ledge_info_on_ledge = *(bool*)((asIScriptObject*)as_context->module.GetVarPtrCache("ledge_info"))->GetAddressOfProperty(0);

    CScriptArray* skeleton_bind_transforms = (CScriptArray*)as_context->module.GetVarPtrCache("skeleton_bind_transforms");
    CScriptArray* inv_skeleton_bind_transforms = (CScriptArray*)as_context->module.GetVarPtrCache("inv_skeleton_bind_transforms");
    CScriptArray* ik_chain_start_index = (CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_start_index");
    CScriptArray* ik_chain_elements = (CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_elements");
    CScriptArray* ik_chain_length = (CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_length");
    CScriptArray* key_transforms = (CScriptArray*)as_context->module.GetVarPtrCache("key_transforms");
    CScriptArray* arm_points = (CScriptArray*)as_context->module.GetVarPtrCache("arm_points");
    CScriptArray* old_arm_points = (CScriptArray*)as_context->module.GetVarPtrCache("old_arm_points");
    CScriptArray* temp_old_arm_points = (CScriptArray*)as_context->module.GetVarPtrCache("temp_old_arm_points");
    CScriptArray* bone_children = (CScriptArray*)as_context->module.GetVarPtrCache("bone_children");
    CScriptArray* bone_children_index = (CScriptArray*)as_context->module.GetVarPtrCache("bone_children_index");
    PROFILER_LEAVE(g_profiler_ctx);

    CDrawArmsImpl(
        this_mo, rigged_object, skeleton,
        breath_amount,
        max_speed,
        true_max_speed, idle_type, on_ground, flip_info_flipping, threat_amount, ledge_info_on_ledge,
        skeleton_bind_transforms, inv_skeleton_bind_transforms, ik_chain_start_index, ik_chain_elements, ik_chain_length, key_transforms, arm_points, old_arm_points, temp_old_arm_points, bone_children, bone_children_index, chest_transform, l_hand_transform, r_hand_transform, num_frames);
}

template <typename T>
class CScriptArrayWrapper {
   public:
    CScriptArray* c_script_array_;

    CScriptArrayWrapper(CScriptArray* c_script_array) : c_script_array_(c_script_array) {}

    T& operator[](int index) {
        return *(T*)c_script_array_->At(index);
    }

    const T& operator[](int index) const {
        return *(T*)c_script_array_->At(index);
    }

    void resize(int size) {
        c_script_array_->Resize(size);
    }

    int size() {
        return (int)c_script_array_->GetSize();
    }

    void push_back(const T& val) {
        c_script_array_->InsertLast((void*)&val);
    }
};

#define rigged_object_SetFrameMatrix(A, B) (rigged_object.animation_frame_bone_matrices[A] = B)
#define rigged_object_GetFrameMatrix(A) (rigged_object.animation_frame_bone_matrices[A])
#define rigged_object_GetTransformedBonePoint(A, B) (GetTransformedBonePoint(&rigged_object, A, B))
#define rigged_object_RotateBoneToMatchVec(A, B, C) (RotateBoneToMatchVec(&rigged_object, A, B, C))

static void CDisplayMatrixUpdateImpl(
    MovementObject* mo,
    float breath_amount,
    const CScriptArrayWrapper<int>& ik_chain_start_index,
    const CScriptArrayWrapper<int>& ik_chain_elements,
    const CScriptArrayWrapper<BoneTransform>& skeleton_bind_transforms,
    const CScriptArrayWrapper<BoneTransform>& inv_skeleton_bind_transforms) {
    MovementObject& this_mo = *mo;
    RiggedObject& rigged_object = *this_mo.rigged_object();

    float scale = rigged_object.GetCharScale();
    int chest_bone = ik_chain_elements[ik_chain_start_index[kTorsoIK]];
    BoneTransform world_chest = BoneTransform(rigged_object.display_bone_matrices[chest_bone]) * inv_skeleton_bind_transforms[chest_bone];
    vec3 breathe_dir = world_chest.rotation * normalize(vec3(0, 0, 1));
    vec3 breathe_front = world_chest.rotation * normalize(vec3(0, 1, 0));
    vec3 breathe_side = world_chest.rotation * normalize(vec3(1, 0, 0));

    for (int i = 0; i < 2; ++i) {
        int collar_bone = ik_chain_elements[ik_chain_start_index[kLeftArmIK + i] + 5];
        BoneTransform collar = BoneTransform(rigged_object.display_bone_matrices[collar_bone]) * inv_skeleton_bind_transforms[collar_bone];
        collar.origin += breathe_dir * breath_amount * 0.01f * scale;
        collar.origin += breathe_front * breath_amount * 0.01f * scale;
        collar = collar * skeleton_bind_transforms[collar_bone];
        rigged_object.display_bone_matrices[collar_bone] = collar.GetMat4();

        int shoulder_bone = ik_chain_elements[ik_chain_start_index[kLeftArmIK + i] + 4];
        BoneTransform shoulder = BoneTransform(rigged_object.display_bone_matrices[shoulder_bone]) * inv_skeleton_bind_transforms[shoulder_bone];
        shoulder.origin += breathe_dir * breath_amount * 0.002f * scale;
        shoulder = shoulder * skeleton_bind_transforms[shoulder_bone];
        rigged_object.display_bone_matrices[shoulder_bone] = shoulder.GetMat4();
    }

    world_chest.origin += breathe_dir * breath_amount * 0.01f * scale;
    world_chest.origin += breathe_front * breath_amount * 0.01f * scale;
    world_chest.rotation = quaternion(vec4(breathe_side.x(), breathe_side.y(), breathe_side.z(), breath_amount * 0.05f)) * world_chest.rotation;
    world_chest = world_chest * skeleton_bind_transforms[chest_bone];
    rigged_object.display_bone_matrices[chest_bone] = world_chest.GetMat4();

    int abdomen_bone = ik_chain_elements[ik_chain_start_index[kTorsoIK] + 1];
    BoneTransform abdomen = BoneTransform(rigged_object.display_bone_matrices[abdomen_bone]) * inv_skeleton_bind_transforms[abdomen_bone];
    abdomen.origin += breathe_dir * breath_amount * 0.008f * scale;
    abdomen.origin += breathe_front * breath_amount * 0.005f * scale;
    abdomen.rotation = quaternion(vec4(breathe_side.x(), breathe_side.y(), breathe_side.z(), breath_amount * -0.02f)) * abdomen.rotation;
    abdomen = abdomen * skeleton_bind_transforms[abdomen_bone];
    rigged_object.display_bone_matrices[abdomen_bone] = abdomen.GetMat4();

    int neck_bone = ik_chain_elements[ik_chain_start_index[kHeadIK] + 1];
    BoneTransform neck = BoneTransform(rigged_object.display_bone_matrices[neck_bone]) * inv_skeleton_bind_transforms[neck_bone];
    neck.origin += breathe_dir * breath_amount * 0.005f * scale;
    neck = neck * skeleton_bind_transforms[neck_bone];
    rigged_object.display_bone_matrices[neck_bone] = neck.GetMat4();
}

static void CDisplayMatrixUpdate(MovementObject* mo) {
    PROFILER_ZONE(g_profiler_ctx, "C++ Display Matrix Update")
    MovementObject& this_mo = *mo;

    PROFILER_ENTER(g_profiler_ctx, "Get AS pointers");
    ASContext* as_context = this_mo.as_context.get();
    float& breath_amount = *(float*)as_context->module.GetVarPtrCache("breath_amount");

    CScriptArrayWrapper<int> ik_chain_start_index((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_start_index"));
    CScriptArrayWrapper<int> ik_chain_elements((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_elements"));
    CScriptArrayWrapper<BoneTransform> skeleton_bind_transforms((CScriptArray*)as_context->module.GetVarPtrCache("skeleton_bind_transforms"));
    CScriptArrayWrapper<BoneTransform> inv_skeleton_bind_transforms((CScriptArray*)as_context->module.GetVarPtrCache("inv_skeleton_bind_transforms"));
    PROFILER_LEAVE(g_profiler_ctx);

    CDisplayMatrixUpdateImpl(
        mo,
        breath_amount,
        ik_chain_start_index, ik_chain_elements, skeleton_bind_transforms, inv_skeleton_bind_transforms);
}

static vec3 CGetCenterOfMassEstimateImpl(
    MovementObject* mo,
    CScriptArrayWrapper<int>& ik_chain_start_index,
    CScriptArrayWrapper<int>& ik_chain_elements,
    CScriptArrayWrapper<BoneTransform>& key_transforms,
    CScriptArrayWrapper<float>& key_masses,
    CScriptArrayWrapper<int>& root_bone) {
    MovementObject& this_mo = *mo;
    RiggedObject& rigged_object = *this_mo.rigged_object();
    Skeleton& skeleton = rigged_object.skeleton();

    vec3 estimate_com;
    float body_mass = 0.0f;

    for (int j = 0; j < 2; ++j) {
        int bone = ik_chain_elements[ik_chain_start_index[kLeftLegIK + j]];
        vec3 point = skeleton.points[skeleton.bones[bone].points[0]];
        vec3 foot_point = key_transforms[kLeftLegKey + j] * point;
        vec3 hip_point = key_transforms[kHipKey] * skeleton.points[skeleton.bones[root_bone[kLeftLegKey + j]].points[0]];
        estimate_com += (hip_point + foot_point) * 0.5f * key_masses[kLeftLegKey + j];
        body_mass += key_masses[kLeftLegKey + j];
    }

    for (int j = 0; j < 2; ++j) {
        int bone = ik_chain_elements[ik_chain_start_index[kLeftArmIK + j]];
        vec3 foot_point = key_transforms[j == 0 ? kLeftArmKey : kRightArmKey] * skeleton.points[skeleton.bones[bone].points[0]];
        vec3 hip_point = key_transforms[kChestKey] * skeleton.points[skeleton.bones[root_bone[kLeftArmKey + j]].points[0]];
        estimate_com += (hip_point + foot_point) * 0.5f * key_masses[kLeftArmKey + j];
        body_mass += key_masses[kLeftArmKey + j];
    }

    {
        int bone = ik_chain_elements[ik_chain_start_index[kTorsoIK]];
        vec3 foot_point = key_transforms[kChestKey] * skeleton.points[skeleton.bones[bone].points[1]];
        vec3 hip_point = key_transforms[kHipKey] * skeleton.points[skeleton.bones[root_bone[kChestKey]].points[0]];
        estimate_com += (hip_point + foot_point) * 0.5f * key_masses[kChestKey];
        body_mass += key_masses[kChestKey];
    }

    {
        int bone = ik_chain_elements[ik_chain_start_index[kHeadIK]];
        vec3 foot_point = key_transforms[kHeadKey] * skeleton.points[skeleton.bones[bone].points[1]];
        vec3 hip_point = key_transforms[kChestKey] * skeleton.points[skeleton.bones[root_bone[kHeadKey]].points[0]];
        estimate_com += (hip_point + foot_point) * 0.5f * key_masses[kHeadKey];
        body_mass += key_masses[kHeadKey];
    }

    return estimate_com / body_mass;
}

static vec3 CGetCenterOfMassEstimate(MovementObject* mo, CScriptArray& key_transforms, CScriptArray& key_masses, CScriptArray& root_bone) {
    PROFILER_ZONE(g_profiler_ctx, "C++ Do Get Center of Mass Estimate")
    MovementObject& this_mo = *mo;

    PROFILER_ENTER(g_profiler_ctx, "Get AS pointers");
    ASContext* as_context = this_mo.as_context.get();

    CScriptArrayWrapper<int> ik_chain_start_index((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_start_index"));
    CScriptArrayWrapper<int> ik_chain_elements((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_elements"));

    CScriptArrayWrapper<BoneTransform> key_transforms_wrapper(&key_transforms);
    CScriptArrayWrapper<float> key_masses_wrapper(&key_masses);
    CScriptArrayWrapper<int> root_bone_wrapper(&root_bone);
    PROFILER_LEAVE(g_profiler_ctx);

    return CGetCenterOfMassEstimateImpl(
        mo,
        ik_chain_start_index, ik_chain_elements,
        key_transforms_wrapper, key_masses_wrapper, root_bone_wrapper);
}

static void CDoChestIKImpl(
    RiggedObject& rigged_object,
    float& time,
    float& time_step,
    float& idle_stance_amount,
    bool& asleep,
    bool& sitting,
    vec3& old_chest_facing,
    vec2& old_chest_angle_vec,
    vec2& chest_angle,
    vec2& target_chest_angle,
    vec2& chest_angle_vel,
    float& old_chest_angle,
    vec3& torso_look,
    int& queue_reset_secondary_animation,
    bool& dialogue_control,
    float& test_talking_amount,
    CScriptArrayWrapper<BoneTransform>& key_transforms,
    CScriptArrayWrapper<int>& ik_chain_start_index,
    CScriptArrayWrapper<int>& ik_chain_elements,
    CScriptArrayWrapper<BoneTransform>& skeleton_bind_transforms,
    CScriptArrayWrapper<BoneTransform>& inv_skeleton_bind_transforms,
    float chest_tilt_offset, float angle_threshold, float torso_damping, float torso_stiffness, int num_frames) {
    if (torso_look != torso_look) {
        torso_look = vec3(0.0);
    }

    float head_look_amount = length(torso_look);

    vec3 head_dir = normalize(key_transforms[kChestKey].rotation * vec3(0, 0, 1));
    vec3 head_up = normalize(key_transforms[kChestKey].rotation * vec3(0, 1, 0));
    vec3 head_right = cross(head_dir, head_up);

    {
        vec2 head_look_flat;
        head_look_flat.x() = dot(old_chest_facing, head_right);
        head_look_flat.y() = dot(old_chest_facing, head_dir);

        if (!(abs(dot(normalize(old_chest_facing), head_up)) > 0.9f)) {
            old_chest_angle_vec.x() = atan2(-head_look_flat.x(), head_look_flat.y());
        }

        float head_up_val = dot(old_chest_facing, head_up);
        old_chest_angle_vec.y() = 0.0f;
        old_chest_angle_vec.y() = asin(dot(old_chest_facing, head_up));

        if (old_chest_angle_vec.y() != old_chest_angle_vec.y()) {
            old_chest_angle_vec.y() = 0.0f;
        }

        old_chest_facing = head_dir;
    }

    vec2 head_look_flat;
    head_look_flat.x() = dot(torso_look, head_right);
    head_look_flat.y() = dot(torso_look, head_dir);
    float angle = atan2(-head_look_flat.x(), head_look_flat.y());

    if (abs(dot(normalize(torso_look), head_up)) > 0.9f) {
        angle = old_chest_angle;
    }

    float head_up_val = dot(torso_look, head_up);
    float angle2 = 0.0f;
    float preangle_dot = dot(torso_look, head_up) + chest_tilt_offset;

    if (preangle_dot >= -1.0f && preangle_dot <= 1.0f) {
        angle2 = asin(preangle_dot);
    }

    // Avoid head flip-flopping when trying to look straight back
    float head_range = 1.0f;

    if (angle > angle_threshold && old_chest_angle <= -head_range) {
        angle = -head_range;
    } else if (angle < -angle_threshold && old_chest_angle >= head_range) {
        angle = head_range;
    }

    angle = min(head_range, max(-head_range, angle));

    old_chest_angle = angle;

    target_chest_angle.x() = angle * head_look_amount;
    target_chest_angle.y() = idle_stance_amount * (angle2 * head_look_amount);

    float torso_shake_amount = 0.005f;
    target_chest_angle.x() += RangedRandomFloat(-torso_shake_amount, torso_shake_amount);
    target_chest_angle.y() += RangedRandomFloat(-torso_shake_amount, torso_shake_amount);

    chest_angle_vel *= pow(torso_damping, num_frames);
    chest_angle_vel += (target_chest_angle - chest_angle) * torso_stiffness * (float)num_frames;
    chest_angle += chest_angle_vel * time_step * (float)num_frames;

    if (queue_reset_secondary_animation != 0) {
        chest_angle = target_chest_angle;
        chest_angle_vel = 0.0;
    }

    int neck_bone = ik_chain_elements[ik_chain_start_index[kHeadIK] + 1];
    int chest_bone = ik_chain_elements[ik_chain_start_index[kTorsoIK]];
    BoneTransform chest_frame_matrix = rigged_object_GetFrameMatrix(chest_bone);
    BoneTransform chest_bind_matrix = skeleton_bind_transforms[chest_bone];
    BoneTransform rel_mat = key_transforms[kChestKey] * invert(chest_frame_matrix * chest_bind_matrix);
    vec3 neck = rigged_object_GetTransformedBonePoint(neck_bone, 0);

    if (dialogue_control) {
        float head_bob = 0.1f * test_talking_amount + 0.02f;
        head_bob *= 0.1f;
        chest_angle.x() += (sin(time * 5.5f) * 0.1f + sin(time * 9.5f) * 0.1f) * head_bob;
        chest_angle.y() += (sin(time * 4.5f) * 0.1f + sin(time * 7.5f) * 0.1f) * head_bob;
    }

    quaternion rotation(vec4(head_up.x(), head_up.y(), head_up.z(), chest_angle.x()));
    quaternion rotation2(vec4(head_right.x(), head_right.y(), head_right.z(), chest_angle.y()));

    quaternion identity;
    int abdomen_bone = ik_chain_elements[ik_chain_start_index[kTorsoIK] + 1];
    vec3 abdomen_top = rigged_object_GetTransformedBonePoint(abdomen_bone, 1);
    vec3 abdomen_bottom = rigged_object_GetTransformedBonePoint(abdomen_bone, 0);

    BoneTransform old_chest_transform = key_transforms[kChestKey];
    key_transforms[kChestKey] = key_transforms[kChestKey] * inv_skeleton_bind_transforms[ik_chain_elements[ik_chain_start_index[kTorsoIK]]];
    key_transforms[kChestKey].origin -= abdomen_top;
    quaternion chest_rotate = rotation * rotation2;
    key_transforms[kChestKey] = mix(chest_rotate, identity, 0.5f) * key_transforms[kChestKey];
    key_transforms[kChestKey].origin += abdomen_top;
    key_transforms[kChestKey].origin -= abdomen_bottom;
    key_transforms[kChestKey] = mix(chest_rotate, identity, 0.5f) * key_transforms[kChestKey];
    key_transforms[kChestKey].origin += abdomen_bottom;
    key_transforms[kChestKey] = key_transforms[kChestKey] * skeleton_bind_transforms[ik_chain_elements[ik_chain_start_index[kTorsoIK]]];
    BoneTransform offset = key_transforms[kChestKey] * invert(old_chest_transform);

    if (!sitting && !asleep) {
        key_transforms[kLeftArmKey] = offset * key_transforms[kLeftArmKey];
        key_transforms[kRightArmKey] = offset * key_transforms[kRightArmKey];
    }

    key_transforms[kHeadKey] = offset * key_transforms[kHeadKey];
}

static void CDoChestIK(MovementObject* mo, float chest_tilt_offset, float angle_threshold, float torso_damping, float torso_stiffness, int num_frames) {
    PROFILER_ZONE(g_profiler_ctx, "C++ Do Chest IK")
    MovementObject& this_mo = *mo;
    RiggedObject& rigged_object = *this_mo.rigged_object();

    PROFILER_ENTER(g_profiler_ctx, "Get AS pointers");
    ASContext* as_context = this_mo.as_context.get();
    float& time = game_timer.game_time;
    float& time_step = game_timer.timestep;
    float& idle_stance_amount = *(float*)as_context->module.GetVarPtrCache("idle_stance_amount");
    bool& asleep = *(bool*)as_context->module.GetVarPtrCache("asleep");
    bool& sitting = *(bool*)as_context->module.GetVarPtrCache("sitting");
    int& queue_reset_secondary_animation = *(int*)as_context->module.GetVarPtrCache("queue_reset_secondary_animation");
    bool& dialogue_control = *(bool*)as_context->module.GetVarPtrCache("dialogue_control");
    float& test_talking_amount = *(float*)as_context->module.GetVarPtrCache("test_talking_amount");

    CScriptArrayWrapper<BoneTransform> key_transforms((CScriptArray*)as_context->module.GetVarPtrCache("key_transforms"));
    CScriptArrayWrapper<int> ik_chain_start_index((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_start_index"));
    CScriptArrayWrapper<int> ik_chain_elements((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_elements"));
    CScriptArrayWrapper<BoneTransform> skeleton_bind_transforms((CScriptArray*)as_context->module.GetVarPtrCache("skeleton_bind_transforms"));
    CScriptArrayWrapper<BoneTransform> inv_skeleton_bind_transforms((CScriptArray*)as_context->module.GetVarPtrCache("inv_skeleton_bind_transforms"));

    vec3& old_chest_facing = *(vec3*)as_context->module.GetVarPtrCache("old_chest_facing");
    vec2& old_chest_angle_vec = *(vec2*)as_context->module.GetVarPtrCache("old_chest_angle_vec");
    vec2& chest_angle = *(vec2*)as_context->module.GetVarPtrCache("chest_angle");
    vec2& target_chest_angle = *(vec2*)as_context->module.GetVarPtrCache("target_chest_angle");
    vec2& chest_angle_vel = *(vec2*)as_context->module.GetVarPtrCache("chest_angle_vel");
    float& old_chest_angle = *(float*)as_context->module.GetVarPtrCache("old_chest_angle");
    vec3& torso_look = *(vec3*)as_context->module.GetVarPtrCache("torso_look");
    PROFILER_LEAVE(g_profiler_ctx);

    CDoChestIKImpl(
        rigged_object,
        time, time_step, idle_stance_amount, asleep, sitting, old_chest_facing, old_chest_angle_vec, chest_angle, target_chest_angle, chest_angle_vel, old_chest_angle, torso_look, queue_reset_secondary_animation, dialogue_control, test_talking_amount,
        key_transforms, ik_chain_start_index, ik_chain_elements, skeleton_bind_transforms, inv_skeleton_bind_transforms,
        chest_tilt_offset, angle_threshold, torso_damping, torso_stiffness, num_frames);
}

static void CDoHeadIKImpl(
    RiggedObject& rigged_object,
    float& time,
    float& time_step,
    vec3& old_head_facing,
    vec2& old_angle,
    vec2& head_angle,
    vec2& target_head_angle,
    vec2& head_angle_vel,
    vec2& head_angle_accel,
    float& old_head_angle,
    vec3& head_look,
    Species& species,
    float& flip_ik_fade,
    int& queue_reset_secondary_animation,
    bool& dialogue_control,
    float& test_talking_amount,
    CScriptArrayWrapper<BoneTransform>& key_transforms,
    CScriptArrayWrapper<int>& ik_chain_start_index,
    CScriptArrayWrapper<int>& ik_chain_elements,
    CScriptArrayWrapper<BoneTransform>& skeleton_bind_transforms,
    CScriptArrayWrapper<BoneTransform>& inv_skeleton_bind_transforms,
    float head_tilt_offset, float angle_threshold, float head_damping, float head_accel_inertia, float head_accel_damping, float head_stiffness, int num_frames) {
    vec3 head_dir = key_transforms[kHeadKey].rotation * vec3(0, 0, 1);
    vec3 head_up = key_transforms[kHeadKey].rotation * vec3(0, 1, 0);
    vec3 head_right = cross(head_dir, head_up);

    {
        vec2 head_look_flat;
        head_look_flat.x() = dot(old_head_facing, head_right);
        head_look_flat.y() = dot(old_head_facing, head_dir);

        if (!(abs(dot(normalize(old_head_facing), head_up)) > 0.9f)) {
            old_angle.x() = atan2(-head_look_flat.x(), head_look_flat.y());
        }

        float head_up_val = dot(old_head_facing, head_up);
        old_angle.y() = 0.0f;
        float dotp_head = dot(old_head_facing, head_up);

        if (dotp_head >= -1.0f && dotp_head <= 1.0f) {
            old_angle.y() = asin(dotp_head);
        } else {
            old_angle.y() = 0.0f;
        }

        old_head_facing = head_dir;
    }

    if (head_look != head_look) {
        head_look = vec3(0.0);
    }

    float head_look_amount = length(head_look);
    vec2 head_look_flat;
    head_look_flat.x() = dot(head_look, head_right);
    head_look_flat.y() = dot(head_look, head_dir);
    float angle = atan2(-head_look_flat.x(), head_look_flat.y());

    if (abs(dot(normalize(head_look), head_up)) > 0.9f) {
        angle = old_head_angle;
    }

    float head_up_val = dot(head_look, head_up);
    float angle2 = 0.0f;

    if (head_up_val >= -1.0f && head_up_val <= 1.0f) {
        angle2 = (asin(head_up_val) + head_tilt_offset);
    }

    // Avoid head flip-flopping when trying to look straight back
    float head_range = 1.3f;

    if (species != _rabbit) {
        head_range *= 0.7f;
    }

    if (angle > angle_threshold && old_head_angle <= -head_range) {
        angle = -head_range;
    } else if (angle < -angle_threshold && old_head_angle >= head_range) {
        angle = head_range;
    }

    angle = min(head_range, max(-head_range, angle));
    angle2 = min(0.8f, max(-0.8f, angle2));

    old_head_angle = angle;

    target_head_angle.x() = angle * head_look_amount;
    target_head_angle.y() = angle2 * head_look_amount;

    float head_shake_amount = 0.001f;
    target_head_angle.x() += RangedRandomFloat(-head_shake_amount, head_shake_amount);
    target_head_angle.y() += RangedRandomFloat(-head_shake_amount, head_shake_amount);

    head_angle_vel *= pow(head_damping, num_frames);
    head_angle_accel *= pow(head_accel_damping, num_frames);
    head_angle_accel = mix((target_head_angle - head_angle) * head_stiffness, head_angle_accel, pow(head_accel_inertia, num_frames));

    if (head_angle_accel != head_angle_accel) {
        head_angle_accel = vec2(0.0);
    }

    head_angle_vel += head_angle_accel * (float)num_frames;
    head_angle_vel.x() = min(max(head_angle_vel.x(), -15.0f), 15.0f);
    head_angle_vel.y() = min(max(head_angle_vel.y(), -15.0f), 15.0f);
    head_angle += head_angle_vel * time_step * (float)num_frames;

    head_angle.x() = min(max(head_angle.x(), -1.5f), 1.5f);
    head_angle.y() = min(max(head_angle.y(), -1.5f), 1.5f);
    target_head_angle.x() = min(max(target_head_angle.x(), -1.5f), 1.5f);

    if (queue_reset_secondary_animation != 0) {
        head_angle = target_head_angle;
        head_angle_vel = 0.0;
        head_angle_accel = 0.0;
    }

    vec2 old_offset(old_angle * (1.0f - flip_ik_fade));

    if ((head_angle.x() > target_head_angle.x() && old_offset.x() < 0.0f) ||
        (head_angle.x() < target_head_angle.x() && old_offset.x() > 0.0f)) {
        head_angle.x() = MoveTowards(head_angle.x(), target_head_angle.x(), abs(old_offset.x()));
    }

    if ((head_angle.y() > target_head_angle.x() && old_offset.y() < 0.0f) ||
        (head_angle.y() < target_head_angle.x() && old_offset.y() > 0.0f)) {
        head_angle.y() = MoveTowards(head_angle.y(), target_head_angle.y(), abs(old_offset.y()));
    }

    int neck_bone = ik_chain_elements[ik_chain_start_index[kHeadIK] + 1];
    int chest_bone = ik_chain_elements[ik_chain_start_index[kTorsoIK]];
    BoneTransform chest_frame_matrix = rigged_object_GetFrameMatrix(chest_bone);
    BoneTransform chest_bind_matrix = skeleton_bind_transforms[chest_bone];
    BoneTransform rel_mat = key_transforms[kChestKey] * invert(chest_frame_matrix * chest_bind_matrix);
    vec3 neck = rel_mat * rigged_object_GetTransformedBonePoint(neck_bone, 0);

    vec2 temp_head_angle = head_angle;

    if (dialogue_control) {
        float head_bob = 0.3f * test_talking_amount + 0.02f;
        head_bob *= 0.7f;
        temp_head_angle.y() += (sin(time * 5) * 0.1f + sin(time * 12) * 0.1f) * head_bob;
        temp_head_angle.x() += (sin(time * 4) * 0.1f + sin(time * 10) * 0.1f) * head_bob;
    }

    quaternion rotation(vec4(head_up.x(), head_up.y(), head_up.z(), temp_head_angle.x()));
    quaternion rotation2(vec4(head_right.x(), head_right.y(), head_right.z(), temp_head_angle.y()));
    quaternion combined_rotation = rotation * rotation2;

    quaternion identity;
    key_transforms[kHeadKey] = key_transforms[kHeadKey] * inv_skeleton_bind_transforms[ik_chain_elements[ik_chain_start_index[kHeadIK]]];
    BoneTransform neck_mat = rel_mat * rigged_object_GetFrameMatrix(neck_bone);
    neck_mat = mix(combined_rotation, identity, 0.5f) * neck_mat;
    rigged_object_SetFrameMatrix(neck_bone, neck_mat);
    key_transforms[kHeadKey].origin -= neck;
    key_transforms[kHeadKey] = mix(combined_rotation, identity, 0.5f) * key_transforms[kHeadKey];
    key_transforms[kHeadKey].rotation = mix(combined_rotation, identity, 0.5f) * key_transforms[kHeadKey].rotation;
    key_transforms[kHeadKey].origin += neck;
    key_transforms[kHeadKey] = key_transforms[kHeadKey] * skeleton_bind_transforms[ik_chain_elements[ik_chain_start_index[kHeadIK]]];
}

static void CDoHeadIK(MovementObject* mo, float head_tilt_offset, float angle_threshold, float head_damping, float head_accel_inertia, float head_accel_damping, float head_stiffness, int num_frames) {
    PROFILER_ZONE(g_profiler_ctx, "C++ Do Head IK")
    MovementObject& this_mo = *mo;
    RiggedObject& rigged_object = *this_mo.rigged_object();

    PROFILER_ENTER(g_profiler_ctx, "Get AS pointers");
    ASContext* as_context = this_mo.as_context.get();
    float& time = game_timer.game_time;
    float& time_step = game_timer.timestep;
    Species& species = *(Species*)as_context->module.GetVarPtrCache("species");
    float& flip_ik_fade = *(float*)as_context->module.GetVarPtrCache("flip_ik_fade");
    int& queue_reset_secondary_animation = *(int*)as_context->module.GetVarPtrCache("queue_reset_secondary_animation");
    bool& dialogue_control = *(bool*)as_context->module.GetVarPtrCache("dialogue_control");
    float& test_talking_amount = *(float*)as_context->module.GetVarPtrCache("test_talking_amount");

    CScriptArrayWrapper<BoneTransform> key_transforms((CScriptArray*)as_context->module.GetVarPtrCache("key_transforms"));
    CScriptArrayWrapper<int> ik_chain_start_index((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_start_index"));
    CScriptArrayWrapper<int> ik_chain_elements((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_elements"));
    CScriptArrayWrapper<BoneTransform> skeleton_bind_transforms((CScriptArray*)as_context->module.GetVarPtrCache("skeleton_bind_transforms"));
    CScriptArrayWrapper<BoneTransform> inv_skeleton_bind_transforms((CScriptArray*)as_context->module.GetVarPtrCache("inv_skeleton_bind_transforms"));

    vec3& old_head_facing = *(vec3*)as_context->module.GetVarPtrCache("old_head_facing");
    vec2& old_angle = *(vec2*)as_context->module.GetVarPtrCache("old_angle");
    vec2& head_angle = *(vec2*)as_context->module.GetVarPtrCache("head_angle");
    vec2& target_head_angle = *(vec2*)as_context->module.GetVarPtrCache("target_head_angle");
    vec2& head_angle_vel = *(vec2*)as_context->module.GetVarPtrCache("head_angle_vel");
    vec2& head_angle_accel = *(vec2*)as_context->module.GetVarPtrCache("head_angle_accel");
    float& old_head_angle = *(float*)as_context->module.GetVarPtrCache("old_head_angle");
    vec3& head_look = *(vec3*)as_context->module.GetVarPtrCache("head_look");
    PROFILER_LEAVE(g_profiler_ctx);

    CDoHeadIKImpl(
        rigged_object,
        time, time_step, old_head_facing, old_angle, head_angle, target_head_angle, head_angle_vel, head_angle_accel, old_head_angle, head_look, species, flip_ik_fade, queue_reset_secondary_animation, dialogue_control, test_talking_amount,
        key_transforms, ik_chain_start_index, ik_chain_elements, skeleton_bind_transforms, inv_skeleton_bind_transforms,
        head_tilt_offset, angle_threshold, head_damping, head_accel_inertia, head_accel_damping, head_stiffness, num_frames);
}

static void CDoFootIKImpl(
    RiggedObject& rigged_object,
    Skeleton& skeleton,
    ASCollisions& col,
    float& roll_ik_fade,
    bool& balancing,
    CScriptArrayWrapper<vec3>& foot_info_pos,
    CScriptArrayWrapper<float>& foot_info_height,
    vec3& balance_pos,
    CScriptArrayWrapper<float>& old_foot_offset,
    CScriptArrayWrapper<std::string>& legs,
    bool& ik_failed,
    CScriptArrayWrapper<quaternion>& old_foot_rotate,
    CScriptArrayWrapper<BoneTransform>& key_transforms,
    CScriptArrayWrapper<int>& ik_chain_start_index,
    CScriptArrayWrapper<int>& ik_chain_elements,
    CScriptArrayWrapper<BoneTransform>& skeleton_bind_transforms,
    CScriptArrayWrapper<BoneTransform>& inv_skeleton_bind_transforms,
    const BoneTransform& local_to_world, int num_frames) {
    SphereCollision& sphere_col = col.as_col;

    bool ground_collision[2] = {false, false};
    vec3 offset[2];
    vec3 foot_balance_offset;

    for (int j = 0; j < 2; ++j) {
        // Get initial foot information
        std::string& ik_label = legs[j];
        BoneTransform mat = local_to_world * GetIKTransform(&rigged_object, ik_label) * skeleton_bind_transforms[ik_chain_elements[ik_chain_start_index[kLeftLegIK + j]]];
        int bone = ASIKBoneStart(&skeleton, ik_label);
        float weight = rigged_object.GetIKWeight(ik_label);
        vec3 anim_pos = GetUnmodifiedIKTransform(&rigged_object, ik_label).GetTranslationPart();
        weight *= (1.0f - roll_ik_fade);

        if (weight > 0.0f) {
            vec3 foot_center = (skeleton.points[skeleton.bones[bone].points[0]] +
                                skeleton.points[skeleton.bones[bone].points[1]]) *
                               0.5f;

            BoneTransform transform = mix(key_transforms[kLeftLegKey + j], mat, weight);
            vec3 pos = mat * foot_center;
            vec3 check_pos = pos + foot_info_pos[j];

            if (balancing) {
                ground_collision[j] = true;
                float ground_height = balance_pos[1] - 0.01f;

                if (old_foot_offset[j] != 0.0f) {
                    old_foot_offset[j] = min(ground_height + 0.1f, max(ground_height - 0.1f, old_foot_offset[j]));
                    ground_height = mix(ground_height, old_foot_offset[j], (float)pow(0.8, num_frames));
                }

                old_foot_offset[j] = ground_height;
                float ground_offset = ground_height - pos.y();
                vec3 new_pos = pos + vec3(0, ground_offset, 0);

                vec3 vec(1, 0, 0);
                quaternion quat(vec4(0.0f, 1.0f, 0.0f, 3.1417f * 0.25f));

                for (int i = 0; i < 8; ++i) {
                    col.ASGetSweptSphereCollision(balance_pos + vec * 0.4f,
                                                  balance_pos,
                                                  0.05f);

                    if (sphere_col.NumContacts() > 0) {
                        // DebugDrawWireSphere(sphere_col.position, 0.05f, vec3(1.0), _delete_on_draw);

                        if (dot(sphere_col.position, vec) - 0.05f < dot(new_pos, vec)) {
                            new_pos += vec * (dot(sphere_col.position, vec) - 0.05f - dot(new_pos, vec));
                        }
                    }

                    vec = quat * vec;
                }

                new_pos.y() += anim_pos.y() - 0.05f;
                new_pos.y() += foot_info_height[j];
                new_pos.x() += foot_info_pos[j].x();
                new_pos.z() += foot_info_pos[j].z();
                offset[j] = (new_pos - pos) * weight;
                foot_balance_offset += offset[j] * 0.5;
                transform.origin += offset[j];
            } else {
                // Check where ground is under foot
                col.ASGetSweptSphereCollision(check_pos + vec3(0.0f, 0.2f, 0.0f),
                                              check_pos + vec3(0.0f, -0.6f, 0.0f),
                                              0.05f);

                if (sphere_col.NumContacts() > 0) {
                    ground_collision[j] = true;
                    float ground_height = sphere_col.position.y() - 0.01f;

                    if (old_foot_offset[j] != 0.0f) {
                        old_foot_offset[j] = min(ground_height + 0.1f, max(ground_height - 0.1f, old_foot_offset[j]));
                        ground_height = mix(ground_height, old_foot_offset[j], (float)pow(0.8, num_frames));
                    }

                    old_foot_offset[j] = ground_height;
                    float ground_offset = ground_height - pos.y();
                    vec3 new_pos = pos + vec3(0, ground_offset, 0);
                    new_pos.y() += anim_pos.y() - 0.05f;
                    new_pos.y() += foot_info_height[j];
                    new_pos.x() += foot_info_pos[j].x();
                    new_pos.z() += foot_info_pos[j].z();
                    offset[j] = (new_pos - pos) * weight;
                    transform.origin += offset[j];
                } else {
                    ik_failed = true;
                }
            }

            // Check ground normal
            col.ASGetSlidingSphereCollision(sphere_col.position + vec3(0.0f, -0.01f, 0.0f), 0.05f);
            vec3 normal = normalize(sphere_col.adjusted_position - sphere_col.position);

            float rotate_weight = 1.0f;
            rotate_weight -= (anim_pos.y() - 0.05f) * 4.0f;
            rotate_weight = max(0.0f, rotate_weight);

            quaternion rotation;
            GetRotationBetweenVectors(vec3(0.0f, 1.0f, 0.0f), normal, rotation);
            rotation = mix(rotation, old_foot_rotate[j], (float)pow(0.8, num_frames));
            old_foot_rotate[j] = rotation;
            quaternion identity;
            rotation = mix(identity, rotation, weight * rotate_weight);

            transform = transform * inv_skeleton_bind_transforms[bone];

            if (!balancing) {
                transform.rotation = rotation * transform.rotation;
            }

            transform = transform * skeleton_bind_transforms[bone];
            key_transforms[kLeftLegKey + j] = transform;
        }
    }

    for (int i = 0; i < 2; ++i) {
        int other = (i + 1) % 2;

        if (ground_collision[i] && !ground_collision[other]) {
            key_transforms[kLeftLegKey + other].origin += offset[i];
        }
    }

    if (balancing) {
        key_transforms[kHipKey].origin += foot_balance_offset * 0.7f;
        key_transforms[kChestKey].origin += foot_balance_offset * 0.5f;
        key_transforms[kHeadKey].origin += foot_balance_offset * 0.4f;
        key_transforms[kLeftArmKey].origin += foot_balance_offset * 1.0f;
        key_transforms[kRightArmKey].origin += foot_balance_offset * 1.0f;
    }
}

static void CDoFootIK(MovementObject* mo, const BoneTransform& local_to_world, int num_frames) {
    PROFILER_ZONE(g_profiler_ctx, "C++ Do Foot IK")
    MovementObject& this_mo = *mo;
    RiggedObject& rigged_object = *this_mo.rigged_object();
    Skeleton& skeleton = rigged_object.skeleton();
    ASCollisions* col = mo->as_collisions.get();

    PROFILER_ENTER(g_profiler_ctx, "Get AS pointers");
    ASContext* as_context = this_mo.as_context.get();
    float& roll_ik_fade = *(float*)as_context->module.GetVarPtrCache("roll_ik_fade");
    bool& balancing = *(bool*)as_context->module.GetVarPtrCache("balancing");
    vec3& balance_pos = *(vec3*)as_context->module.GetVarPtrCache("balance_pos");
    bool& ik_failed = *(bool*)as_context->module.GetVarPtrCache("ik_failed");

    CScriptArrayWrapper<BoneTransform> key_transforms((CScriptArray*)as_context->module.GetVarPtrCache("key_transforms"));
    CScriptArrayWrapper<int> ik_chain_start_index((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_start_index"));
    CScriptArrayWrapper<int> ik_chain_elements((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_elements"));
    CScriptArrayWrapper<float> ik_chain_bone_lengths((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_bone_lengths"));
    CScriptArrayWrapper<BoneTransform> skeleton_bind_transforms((CScriptArray*)as_context->module.GetVarPtrCache("skeleton_bind_transforms"));
    CScriptArrayWrapper<BoneTransform> inv_skeleton_bind_transforms((CScriptArray*)as_context->module.GetVarPtrCache("inv_skeleton_bind_transforms"));

    CScriptArrayWrapper<vec3> foot_info_pos((CScriptArray*)as_context->module.GetVarPtrCache("foot_info_pos"));         // TODO: Does this work???
    CScriptArrayWrapper<float> foot_info_height((CScriptArray*)as_context->module.GetVarPtrCache("foot_info_height"));  // TODO: Does this work???
    CScriptArrayWrapper<float> old_foot_offset((CScriptArray*)as_context->module.GetVarPtrCache("old_foot_offset"));
    CScriptArrayWrapper<std::string> legs((CScriptArray*)as_context->module.GetVarPtrCache("legs"));
    CScriptArrayWrapper<quaternion> old_foot_rotate((CScriptArray*)as_context->module.GetVarPtrCache("old_foot_rotate"));  // TODO: Does this work???
    PROFILER_LEAVE(g_profiler_ctx);

    CDoFootIKImpl(  // TODO: Verfiy that all the variables are actually getting grabbed from AS correctly
        rigged_object, skeleton, *col,
        roll_ik_fade, balancing, foot_info_pos, foot_info_height, balance_pos, old_foot_offset, legs, ik_failed, old_foot_rotate,
        key_transforms, ik_chain_start_index, ik_chain_elements, skeleton_bind_transforms, inv_skeleton_bind_transforms,
        local_to_world, num_frames);
}

#define skeleton_GetParent(A) (skeleton.parents[A])

static void DoHipIKImpl(
    MovementObject* mo,
    CScriptArrayWrapper<float>& target_leg_length,
    vec3& old_hip_offset,
    CScriptArrayWrapper<BoneTransform>& key_transforms,
    CScriptArrayWrapper<int>& ik_chain_start_index,
    CScriptArrayWrapper<int>& ik_chain_elements,
    CScriptArrayWrapper<int>& ik_chain_length,
    BoneTransform& hip_offset, quaternion& hip_rotate, int num_frames) {
    MovementObject& this_mo = *mo;
    RiggedObject& rigged_object = *this_mo.rigged_object();
    Skeleton& skeleton = rigged_object.skeleton();

    vec3 temp_foot_pos[2];
    vec3 hip_pos[2];
    vec3 orig_hip_pos[2];

    for (int j = 0; j < 2; ++j) {
        int bone = ik_chain_start_index[kLeftLegIK + j];
        int bone_len = ik_chain_length[kLeftLegIK + j];
        ;
        temp_foot_pos[j] = key_transforms[kLeftLegKey + j] * skeleton.points[skeleton.bones[ik_chain_elements[bone]].points[0]];
        hip_pos[j] = key_transforms[kHipKey] * skeleton.points[skeleton.bones[ik_chain_elements[bone + bone_len - 1]].points[0]];
        orig_hip_pos[j] = hip_pos[j];
    }

    vec3 orig_mid = (orig_hip_pos[0] + orig_hip_pos[1]) * 0.5f;

    for (int i = 0; i < 2; ++i) {
        float hip_dist = distance(hip_pos[0], hip_pos[1]);

        for (int j = 0; j < 2; ++j) {
            hip_pos[j] = temp_foot_pos[j] + normalize(hip_pos[j] - temp_foot_pos[j]) * target_leg_length[j];
        }

        vec3 mid = (hip_pos[0] + hip_pos[1]) * 0.5f;
        mid = vec3(orig_mid.x(), mid.y(), orig_mid.z());
        vec3 dir = normalize((hip_pos[1] - hip_pos[0]) + (orig_hip_pos[1] - orig_hip_pos[0]) * 3.0f);
        hip_pos[0] = mid + dir * hip_dist * -0.5f;
        hip_pos[1] = mid + dir * hip_dist * 0.5f;
    }

    quaternion rotation;

    {
        vec3 orig_hip_vec = normalize(orig_hip_pos[1] - orig_hip_pos[0]);
        vec3 new_hip_vec = normalize(hip_pos[1] - hip_pos[0]);
        vec3 rotate_axis = normalize(cross(orig_hip_vec, new_hip_vec));
        vec3 right_vec = cross(orig_hip_vec, rotate_axis);
        float rotate_angle = atan2(-dot(new_hip_vec, right_vec), dot(new_hip_vec, orig_hip_vec));
        float flat_speed = sqrt(this_mo.velocity.x() * this_mo.velocity.x() + this_mo.velocity.z() * this_mo.velocity.z());
        float max_rotate_angle = max(0.0f, 0.2f - flat_speed * 0.2f);
        rotate_angle = max(-max_rotate_angle, min(max_rotate_angle, rotate_angle));
        rotation = quaternion(vec4(rotate_axis, rotate_angle));
    }

    hip_rotate = rotation;
    int hip_bone = ASIKBoneStart(&skeleton, "torso");
    int hip_bone_len = ASIKBoneLength(&skeleton, "torso");

    for (int i = 0; i < hip_bone_len - 1; ++i) {
        hip_bone = skeleton_GetParent(hip_bone);
    }

    vec3 hip_root = key_transforms[kHipKey] * ((skeleton.points[skeleton.bones[hip_bone].points[1]]));
    vec3 orig_hip_offset = (orig_hip_pos[0] + orig_hip_pos[1]) * 0.5f - hip_root;
    hip_offset.origin = (hip_pos[0] + hip_pos[1]) * 0.5f - (orig_hip_pos[0] + orig_hip_pos[1]) * 0.5f + orig_hip_offset - hip_rotate * orig_hip_offset;
    vec3 temp = hip_offset.origin;

    if (old_hip_offset == vec3(0.0f)) {
        old_hip_offset = temp;
    }

    hip_offset.origin = mix(temp, old_hip_offset, pow(0.95f, num_frames));
    old_hip_offset = hip_offset.origin;
}

static void CDoHipIK(MovementObject* mo, BoneTransform& hip_offset, quaternion& hip_rotate, int num_frames) {
    PROFILER_ZONE(g_profiler_ctx, "C++ Do Hip IK")
    MovementObject& this_mo = *mo;

    PROFILER_ENTER(g_profiler_ctx, "Get AS pointers");
    ASContext* as_context = this_mo.as_context.get();

    CScriptArrayWrapper<BoneTransform> key_transforms((CScriptArray*)as_context->module.GetVarPtrCache("key_transforms"));
    CScriptArrayWrapper<int> ik_chain_start_index((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_start_index"));
    CScriptArrayWrapper<int> ik_chain_elements((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_elements"));
    CScriptArrayWrapper<int> ik_chain_length((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_length"));

    CScriptArrayWrapper<float> target_leg_length((CScriptArray*)as_context->module.GetVarPtrCache("target_leg_length"));
    vec3& old_hip_offset = *(vec3*)as_context->module.GetVarPtrCache("old_hip_offset");
    PROFILER_LEAVE(g_profiler_ctx);

    DoHipIKImpl(
        mo,
        target_leg_length, old_hip_offset,
        key_transforms, ik_chain_start_index, ik_chain_elements, ik_chain_length,
        hip_offset, hip_rotate, num_frames);
}

static void CDrawLegImpl(
    RiggedObject& rigged_object,
    Skeleton& skeleton,
    CScriptArrayWrapper<int>& ik_chain_start_index,
    CScriptArrayWrapper<int>& ik_chain_elements,
    CScriptArrayWrapper<float>& ik_chain_bone_lengths,
    CScriptArrayWrapper<BoneTransform>& skeleton_bind_transforms,
    CScriptArrayWrapper<BoneTransform>& inv_skeleton_bind_transforms,
    bool right, const BoneTransform& hip_transform, const BoneTransform& foot_transform) {
    int ik_chain_start = ik_chain_start_index[kLeftLegIK + (right ? 1 : 0)];

    // Get important joint positions
    vec3 foot_tip, foot_base, ankle, knee, hip;
    foot_tip = GetTransformedBonePoint(&rigged_object, ik_chain_elements[ik_chain_start + 0], 1);
    foot_base = GetTransformedBonePoint(&rigged_object, ik_chain_elements[ik_chain_start + 0], 0);
    ankle = GetTransformedBonePoint(&rigged_object, ik_chain_elements[ik_chain_start + 1], 0);
    knee = GetTransformedBonePoint(&rigged_object, ik_chain_elements[ik_chain_start + 3], 0);
    hip = GetTransformedBonePoint(&rigged_object, ik_chain_elements[ik_chain_start + 5], 0);

    vec3 old_foot_dir = foot_tip - foot_base;
    float upper_foot_length = ik_chain_bone_lengths[ik_chain_start + 1];
    float lower_leg_length = (ik_chain_bone_lengths[ik_chain_start + 2] + ik_chain_bone_lengths[ik_chain_start + 3]);
    float upper_leg_length = (ik_chain_bone_lengths[ik_chain_start + 4] + ik_chain_bone_lengths[ik_chain_start + 5]);

    float lower_leg_weight = ik_chain_bone_lengths[ik_chain_start + 2] / lower_leg_length;
    float upper_leg_weight = ik_chain_bone_lengths[ik_chain_start + 4] / upper_leg_length;

    vec3 old_hip = hip;
    float old_length = distance(foot_base, old_hip);

    // New hip position based on key hip transform
    hip = hip_transform * skeleton.points[skeleton.bones[ik_chain_elements[ik_chain_start + 5]].points[0]];

    // New foot_base position based on key foot transform
    vec3 ik_target = foot_transform * skeleton.points[skeleton.bones[ik_chain_elements[ik_chain_start + 0]].points[0]];

    quaternion rotate;
    GetRotationBetweenVectors(foot_base - old_hip, ik_target - hip, rotate);
    mat3 rotate_mat = Mat3FromQuaternion(rotate);
    knee = rotate_mat * (knee - old_hip) + hip;
    ankle = rotate_mat * (ankle - old_hip) + hip;
    foot_base = rotate_mat * (foot_base - old_hip) + hip;
    float new_length = distance(ik_target, hip);
    float weight = new_length / old_length;
    knee = mix(hip, knee, weight);
    ankle = mix(hip, ankle, weight);
    foot_base = mix(hip, foot_base, weight);
    foot_tip = foot_transform * skeleton.points[skeleton.bones[ik_chain_elements[ik_chain_start + 0]].points[1]];

    int num_iterations = 2;

    for (int i = 0; i < num_iterations; ++i) {
        knee = hip + normalize(knee - hip) * upper_leg_length;
        ankle = foot_base + normalize(ankle - foot_base) * upper_foot_length;
        vec3 knee_ankle_vec = normalize(knee - ankle);
        vec3 mid = (knee + ankle) * 0.5f;
        ankle = mid - knee_ankle_vec * 0.5f * lower_leg_length;
        knee = mid + knee_ankle_vec * 0.5f * lower_leg_length;
    }

    rigged_object.animation_frame_bone_matrices[ik_chain_elements[ik_chain_start + 0]] = foot_transform * inv_skeleton_bind_transforms[ik_chain_elements[ik_chain_start + 0]];

    RotateBoneToMatchVec(&rigged_object, foot_base, foot_tip, ik_chain_elements[ik_chain_start + 0]);
    RotateBoneToMatchVec(&rigged_object, ankle, foot_base, ik_chain_elements[ik_chain_start + 1]);
    RotateBonesToMatchVec(&rigged_object, knee, ankle, ik_chain_elements[ik_chain_start + 3], ik_chain_elements[ik_chain_start + 2], lower_leg_weight);
    RotateBonesToMatchVec(&rigged_object, hip, knee, ik_chain_elements[ik_chain_start + 5], ik_chain_elements[ik_chain_start + 4], upper_leg_weight);
}

static void CDrawLeg(MovementObject* mo, bool right, const BoneTransform& hip_transform, const BoneTransform& foot_transform, int num_frames) {
    PROFILER_ZONE(g_profiler_ctx, "C++ Draw Leg")
    MovementObject& this_mo = *mo;
    RiggedObject& rigged_object = *this_mo.rigged_object();
    Skeleton& skeleton = rigged_object.skeleton();

    PROFILER_ENTER(g_profiler_ctx, "Get AS pointers");
    ASContext* as_context = this_mo.as_context.get();
    CScriptArrayWrapper<int> ik_chain_start_index((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_start_index"));
    CScriptArrayWrapper<int> ik_chain_elements((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_elements"));
    CScriptArrayWrapper<float> ik_chain_bone_lengths((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_bone_lengths"));
    CScriptArrayWrapper<BoneTransform> skeleton_bind_transforms((CScriptArray*)as_context->module.GetVarPtrCache("skeleton_bind_transforms"));
    CScriptArrayWrapper<BoneTransform> inv_skeleton_bind_transforms((CScriptArray*)as_context->module.GetVarPtrCache("inv_skeleton_bind_transforms"));
    PROFILER_LEAVE(g_profiler_ctx);

    CDrawLegImpl(
        rigged_object, skeleton,
        ik_chain_start_index, ik_chain_elements, ik_chain_bone_lengths, skeleton_bind_transforms, inv_skeleton_bind_transforms,
        right, hip_transform, foot_transform);
}

static void CDrawHeadImpl(
    RiggedObject& rigged_object,
    Skeleton& skeleton,
    float& breath_amount,
    CScriptArrayWrapper<BoneTransform>& key_transforms,
    CScriptArrayWrapper<BoneTransform>& inv_skeleton_bind_transforms,
    CScriptArrayWrapper<int>& ik_chain_start_index,
    CScriptArrayWrapper<int>& ik_chain_elements,
    CScriptArrayWrapper<int>& bone_children,
    CScriptArrayWrapper<int>& bone_children_index,
    const BoneTransform& chest_transform, const BoneTransform& head_transform) {
    int start = ik_chain_start_index[kHeadIK];
    int head_bone = ik_chain_elements[start + 0];
    int neck_bone = ik_chain_elements[start + 1];

    int chest_bone = ik_chain_elements[ik_chain_start_index[kTorsoIK]];
    BoneTransform chest_frame_matrix = rigged_object.animation_frame_bone_matrices[chest_bone];
    BoneTransform chest_bind_matrix = inv_skeleton_bind_transforms[chest_bone];
    BoneTransform rel_mat = chest_transform * invert(BoneTransform(chest_frame_matrix * chest_bind_matrix));

    BoneTransform old_head_matrix = rigged_object.animation_frame_bone_matrices[head_bone];

    vec3 crown, skull, neck;
    crown = head_transform * skeleton.points[skeleton.bones[head_bone].points[1]];
    skull = head_transform * skeleton.points[skeleton.bones[head_bone].points[0]];
    neck = chest_transform * skeleton.points[skeleton.bones[neck_bone].points[0]];

    BoneTransform world_chest = key_transforms[kChestKey] * inv_skeleton_bind_transforms[ik_chain_start_index[kTorsoIK]];
    vec3 breathe_dir = world_chest.rotation * normalize(vec3(0.0f, -0.3f, 1.0f));
    float scale = rigged_object.GetCharScale();
    skull += breathe_dir * breath_amount * 0.005f * scale;
    crown += breathe_dir * breath_amount * 0.002f * scale;

    rigged_object.animation_frame_bone_matrices[head_bone] = head_transform * inv_skeleton_bind_transforms[head_bone];
    RotateBoneToMatchVec(&rigged_object, neck, skull, neck_bone);
    RotateBoneToMatchVec(&rigged_object, skull, crown, head_bone);

    BoneTransform head_rel = rigged_object.animation_frame_bone_matrices[head_bone] * invert(old_head_matrix);

    for (int i = 0, len = GetNumBoneChildren(bone_children_index.c_script_array_, head_bone); i < len; ++i) {
        int child = bone_children[bone_children_index[head_bone] + i];
        rigged_object.animation_frame_bone_matrices[child] = head_rel * rigged_object.animation_frame_bone_matrices[child];
    }
}

static void CDrawHead(MovementObject* mo, const BoneTransform& chest_transform, const BoneTransform& head_transform, int num_frames) {
    PROFILER_ZONE(g_profiler_ctx, "C++ Draw Head")
    MovementObject& this_mo = *mo;
    RiggedObject& rigged_object = *this_mo.rigged_object();
    Skeleton& skeleton = rigged_object.skeleton();

    PROFILER_ENTER(g_profiler_ctx, "Get AS pointers");
    ASContext* as_context = this_mo.as_context.get();
    float& breath_amount = *(float*)as_context->module.GetVarPtrCache("breath_amount");

    CScriptArrayWrapper<BoneTransform> key_transforms((CScriptArray*)as_context->module.GetVarPtrCache("key_transforms"));
    CScriptArrayWrapper<BoneTransform> inv_skeleton_bind_transforms((CScriptArray*)as_context->module.GetVarPtrCache("inv_skeleton_bind_transforms"));
    CScriptArrayWrapper<int> ik_chain_start_index((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_start_index"));
    CScriptArrayWrapper<int> ik_chain_elements((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_elements"));
    CScriptArrayWrapper<int> bone_children((CScriptArray*)as_context->module.GetVarPtrCache("bone_children"));
    CScriptArrayWrapper<int> bone_children_index((CScriptArray*)as_context->module.GetVarPtrCache("bone_children_index"));
    PROFILER_LEAVE(g_profiler_ctx);

    CDrawHeadImpl(
        rigged_object, skeleton,
        breath_amount,
        key_transforms, inv_skeleton_bind_transforms, ik_chain_start_index, ik_chain_elements, bone_children, bone_children_index,
        chest_transform, head_transform);
}

static void CDrawBodyImpl(
    RiggedObject& rigged_object,
    Skeleton& skeleton,
    CScriptArrayWrapper<BoneTransform>& inv_skeleton_bind_transforms,
    CScriptArrayWrapper<int>& ik_chain_start_index,
    CScriptArrayWrapper<int>& ik_chain_elements,
    CScriptArrayWrapper<int>& ik_chain_length,
    const BoneTransform& hip_transform, const BoneTransform& chest_transform) {
    int start = ik_chain_start_index[kTorsoIK];

    BoneTransform old_hip_matrix = rigged_object.animation_frame_bone_matrices[ik_chain_elements[start + 2]];

    int chest_bone = ik_chain_elements[start + 0];
    int abdomen_bone = ik_chain_elements[start + 1];
    int hip_bone = ik_chain_elements[start + 2];

    vec3 collarbone = chest_transform * skeleton.points[skeleton.bones[chest_bone].points[1]];
    vec3 ribs = chest_transform * skeleton.points[skeleton.bones[chest_bone].points[0]];
    vec3 stomach = hip_transform * skeleton.points[skeleton.bones[abdomen_bone].points[0]];
    vec3 hips = hip_transform * skeleton.points[skeleton.bones[hip_bone].points[0]];

    rigged_object.animation_frame_bone_matrices[chest_bone] = chest_transform * inv_skeleton_bind_transforms[chest_bone];
    BoneTransform temp = mix(chest_transform, hip_transform, 0.5f);
    temp.rotation = mix(hip_transform.rotation, chest_transform.rotation, 0.5f);
    rigged_object.animation_frame_bone_matrices[abdomen_bone] = temp * inv_skeleton_bind_transforms[abdomen_bone];

    RotateBoneToMatchVec(&rigged_object, ribs, collarbone, chest_bone);
    RotateBoneToMatchVec(&rigged_object, stomach, ribs, abdomen_bone);
    RotateBoneToMatchVec(&rigged_object, hips, stomach, hip_bone);

    BoneTransform hip_rel = rigged_object.animation_frame_bone_matrices[hip_bone] * invert(old_hip_matrix);

    start = ik_chain_start_index[kTailIK];
    int len = ik_chain_length[kTailIK];

    for (int i = 0; i < len; ++i) {
        int bone = ik_chain_elements[start + i];
        rigged_object.animation_frame_bone_matrices[bone] = hip_rel * rigged_object.animation_frame_bone_matrices[bone];
    }
}

static void CDrawBody(MovementObject* mo, const BoneTransform& hip_transform, const BoneTransform& chest_transform) {
    PROFILER_ZONE(g_profiler_ctx, "C++ Draw Body")
    MovementObject& this_mo = *mo;
    RiggedObject& rigged_object = *this_mo.rigged_object();
    Skeleton& skeleton = rigged_object.skeleton();

    PROFILER_ENTER(g_profiler_ctx, "Get AS pointers");
    ASContext* as_context = this_mo.as_context.get();
    CScriptArrayWrapper<BoneTransform> inv_skeleton_bind_transforms((CScriptArray*)as_context->module.GetVarPtrCache("inv_skeleton_bind_transforms"));
    CScriptArrayWrapper<int> ik_chain_start_index((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_start_index"));
    CScriptArrayWrapper<int> ik_chain_elements((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_elements"));
    CScriptArrayWrapper<int> ik_chain_length((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_length"));
    PROFILER_LEAVE(g_profiler_ctx);

    CDrawBodyImpl(
        rigged_object, skeleton,
        inv_skeleton_bind_transforms, ik_chain_start_index, ik_chain_elements, ik_chain_length,
        hip_transform, chest_transform);
}

void CDrawEarImpl(
    MovementObject* mo,
    float& time,
    float& time_step,
    int& species,
    int& skip_ear_physics_counter,
    bool& flip_info_flipping,
    bool& on_ground,
    CScriptArrayWrapper<int>& ik_chain_start_index,
    CScriptArrayWrapper<int>& ik_chain_elements,
    CScriptArrayWrapper<float>& ear_rotation,
    CScriptArrayWrapper<float>& ear_rotation_time,
    CScriptArrayWrapper<float>& target_ear_rotation,
    CScriptArrayWrapper<BoneTransform>& inv_skeleton_bind_transforms,
    CScriptArrayWrapper<vec3>& ear_points,
    CScriptArrayWrapper<vec3>& old_ear_points,
    CScriptArrayWrapper<vec3>& temp_old_ear_points,
    bool right, const BoneTransform& head_transform, int num_frames) {
    MovementObject& this_mo = *mo;
    RiggedObject& rigged_object = *this_mo.rigged_object();

    int chain_start = ik_chain_start_index[kLeftEarIK + (right ? 1 : 0)];

    vec3 tip, middle, base;
    tip = rigged_object_GetTransformedBonePoint(ik_chain_elements[chain_start + 0], 1);
    middle = rigged_object_GetTransformedBonePoint(ik_chain_elements[chain_start + 0], 0);
    base = rigged_object_GetTransformedBonePoint(ik_chain_elements[chain_start + 1], 0);

    if (ear_rotation.size() == 0) {
        ear_rotation.resize(2);
        ear_rotation_time.resize(2);
        target_ear_rotation.resize(2);
        for (int i = 0; i < 2; ++i) {
            target_ear_rotation[i] = 0.0f;
            ear_rotation[i] = 0.0f;
            ear_rotation_time[i] = 0.0f;
        }
    } else {
        int which = right ? 1 : 0;
        if (ear_rotation_time[which] < time) {
            target_ear_rotation[which] = RangedRandomFloat(-0.4f, 0.8f);
            ear_rotation_time[which] = time + RangedRandomFloat(0.7f, 4.0f);
        }
        ear_rotation[which] = mix(target_ear_rotation[which], ear_rotation[which], pow(0.9f, num_frames));
    }

    bool ear_rotate_test = true;
    if (ear_rotate_test) {
        // vec3 head_up = head_transform.rotation * inv_skeleton_bind_transforms[ik_chain_elements[ik_chain_start_index[kHeadIK]]].rotation * vec3(0,0,1);
        vec3 head_dir = head_transform.rotation * inv_skeleton_bind_transforms[ik_chain_elements[ik_chain_start_index[kHeadIK]]].rotation * vec3(0, 1, 0);
        // vec3 head_right = cross(head_dir, head_up);
        // float ear_back_amount = 2.5f;
        // float ear_twist_amount = 3.0f * (right?-1.0f:1.0f);
        float ear_back_amount = 0.0f;
        float ear_twist_amount = ear_rotation[right ? 1 : 0];
        if (right) {
            ear_twist_amount *= -1.0f;
        }
        if (species != _rabbit) {
            ear_twist_amount *= 0.3f;
        }
        BoneTransform ear_back_mat;
        vec3 ear_right = normalize(cross(head_dir, middle - base));
        ear_back_mat.rotation = quaternion(vec4(ear_right, ear_back_amount));
        BoneTransform ear_twist_mat;
        ear_twist_mat.rotation = quaternion(vec4(ear_back_mat.rotation * normalize(middle - base), ear_twist_amount));
        BoneTransform base_offset;
        base_offset.origin = base;
        BoneTransform ear_transform = base_offset * ear_twist_mat * ear_back_mat * invert(base_offset);
        BoneTransform tip_ear_transform = base_offset * ear_twist_mat * ear_twist_mat * ear_back_mat * invert(base_offset);
        rigged_object_SetFrameMatrix(ik_chain_elements[chain_start + 0], tip_ear_transform * rigged_object_GetFrameMatrix(ik_chain_elements[chain_start + 0]));
        rigged_object_SetFrameMatrix(ik_chain_elements[chain_start + 1], ear_transform * rigged_object_GetFrameMatrix(ik_chain_elements[chain_start + 1]));

        tip = rigged_object_GetTransformedBonePoint(ik_chain_elements[chain_start + 0], 1);
        middle = rigged_object_GetTransformedBonePoint(ik_chain_elements[chain_start + 0], 0);
    }

    float low_dist = distance(base, middle);
    float high_dist = distance(middle, tip);

    old_ear_points.resize(6);
    temp_old_ear_points.resize(6);
    if (ear_points.size() != 6) {
        ear_points.push_back(base);
        ear_points.push_back(middle);
        ear_points.push_back(tip);
        for (int i = 0, len = ear_points.size(); i < len; ++i) {
            old_ear_points[i] = ear_points[i];
            temp_old_ear_points[i] = ear_points[i];
        }
    } else {
        int start = right ? 3 : 0;

        for (int i = 0; i < 3; ++i) {
            temp_old_ear_points[start + i] = ear_points[start + i];
        }

        // The following contains ear physics, they are acting odd in cutscenes, therefore we disabled them during them.
        // One of the values in this routine goes insane for some reason.

        float ear_damping = 0.95f;
        float low_ear_rotation_damping = 0.9f;
        float up_ear_rotation_damping = 0.92f;

        ear_points[start + 0] = base;
        vec3 vel_offset = this_mo.velocity * time_step * (float)num_frames;

        if (length(ear_points[start + 0] - old_ear_points[start + 0]) > 10.0f) {
            // Log(warning, "Massive movement in ear_position detected, skipping ear physics for a couple of frames until position is gussed to have returned to stable. TODO\n");
            // TODO: Find the source of the massive position change, and fix it, often caused by something
            // when using the dialogue editor
            skip_ear_physics_counter = 5;
        }

        if (skip_ear_physics_counter == 0) {
            ear_points[start + 1] += (((ear_points[start + 1] - old_ear_points[start + 1]) - vel_offset) * pow(ear_damping, num_frames) + vel_offset) * ear_damping * ear_damping;
            ear_points[start + 2] += (((ear_points[start + 2] - old_ear_points[start + 2]) - vel_offset) * pow(ear_damping, num_frames) + vel_offset) * ear_damping * ear_damping;
            quaternion rotation;
            GetRotationBetweenVectors(middle - base, ear_points[start + 1] - base, rotation);
            vec3 rotated_tip = ASMult(rotation, tip - middle) + ear_points[start + 1];
            ear_points[start + 2] += (rotated_tip - ear_points[start + 2]) * (1.0f - pow(up_ear_rotation_damping, num_frames));
            ear_points[start + 1] += (middle - ear_points[start + 1]) * (1.0f - pow(low_ear_rotation_damping, num_frames));

            for (int i = 0; i < 3; ++i) {
                ear_points[start + 1] = base + normalize(ear_points[start + 1] - base) * low_dist;
                vec3 mid = (ear_points[start + 1] + ear_points[start + 2]) * 0.5f;
                vec3 dir = normalize(ear_points[start + 1] - ear_points[start + 2]);
                ear_points[start + 2] = mid - dir * high_dist * 0.5f;
                ear_points[start + 1] = mid + dir * high_dist * 0.5f;
            }

            if (flip_info_flipping && on_ground) {
                ASCollisions* col = mo->as_collisions.get();
                col->GetSweptSphereCollision(ear_points[start + 0], ear_points[start + 1], 0.03f, col->as_col);
                ear_points[start + 1] = col->as_col.adjusted_position;
                col->GetSweptSphereCollision(ear_points[start + 1], ear_points[start + 2], 0.03f, col->as_col);
                ear_points[start + 2] = col->as_col.adjusted_position;
            }

            // debug_lines.push_back(DebugDrawLine(ear_points[start+0], ear_points[start+1], vec3(1.0f), _fade));
            // debug_lines.push_back(DebugDrawLine(ear_points[start+1], ear_points[start+2], vec3(1.0f), _fade));
            rigged_object_RotateBoneToMatchVec(ear_points[start + 0], ear_points[start + 1], ik_chain_elements[chain_start + 1]);
            rigged_object_RotateBoneToMatchVec(ear_points[start + 1], ear_points[start + 2], ik_chain_elements[chain_start + 0]);
            middle = ear_points[start + 1];
            tip = ear_points[start + 2];
        } else {
            skip_ear_physics_counter--;
        }

        for (int i = 0; i < 3; ++i) {
            old_ear_points[start + i] = temp_old_ear_points[start + i];
        }
    }
}

void CDrawEar(MovementObject* mo, bool right, const BoneTransform& head_transform, int num_frames) {
    PROFILER_ZONE(g_profiler_ctx, "C++ DrawEar");
    MovementObject& this_mo = *mo;

    PROFILER_ENTER(g_profiler_ctx, "Get AS pointers");
    ASContext* as_context = this_mo.as_context.get();
    float& time = game_timer.game_time;
    float& time_step = game_timer.timestep;
    int& species = *(int*)as_context->module.GetVarPtrCache("species");
    int& skip_ear_physics_counter = *(int*)as_context->module.GetVarPtrCache("skip_ear_physics_counter");
    bool& flip_info_flipping = *(bool*)((asIScriptObject*)as_context->module.GetVarPtrCache("flip_info"))->GetAddressOfProperty(0);
    bool& on_ground = *(bool*)as_context->module.GetVarPtrCache("on_ground");

    CScriptArrayWrapper<int> ik_chain_start_index((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_start_index"));
    CScriptArrayWrapper<int> ik_chain_elements((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_elements"));
    CScriptArrayWrapper<float> ear_rotation((CScriptArray*)as_context->module.GetVarPtrCache("ear_rotation"));
    CScriptArrayWrapper<float> ear_rotation_time((CScriptArray*)as_context->module.GetVarPtrCache("ear_rotation_time"));
    CScriptArrayWrapper<float> target_ear_rotation((CScriptArray*)as_context->module.GetVarPtrCache("target_ear_rotation"));
    CScriptArrayWrapper<BoneTransform> inv_skeleton_bind_transforms((CScriptArray*)as_context->module.GetVarPtrCache("inv_skeleton_bind_transforms"));
    CScriptArrayWrapper<vec3> ear_points((CScriptArray*)as_context->module.GetVarPtrCache("ear_points"));
    CScriptArrayWrapper<vec3> old_ear_points((CScriptArray*)as_context->module.GetVarPtrCache("old_ear_points"));
    CScriptArrayWrapper<vec3> temp_old_ear_points((CScriptArray*)as_context->module.GetVarPtrCache("temp_old_ear_points"));
    PROFILER_LEAVE(g_profiler_ctx);

    CDrawEarImpl(
        mo,
        time, time_step, species, skip_ear_physics_counter, flip_info_flipping, on_ground,
        ik_chain_start_index, ik_chain_elements, ear_rotation, ear_rotation_time, target_ear_rotation, inv_skeleton_bind_transforms, ear_points, old_ear_points, temp_old_ear_points,
        right, head_transform, num_frames);
}

void CDrawTailImpl(
    MovementObject* mo,
    float& time,
    float& time_step,
    CScriptArrayWrapper<int>& ik_chain_elements,
    CScriptArrayWrapper<int>& ik_chain_start_index,
    CScriptArrayWrapper<int>& ik_chain_length,
    CScriptArrayWrapper<vec3>& tail_points,
    CScriptArrayWrapper<vec3>& old_tail_points,
    CScriptArrayWrapper<vec3>& temp_old_tail_points,
    CScriptArrayWrapper<vec3>& tail_correction,
    CScriptArrayWrapper<float>& tail_section_length,
    int num_frames) {
    MovementObject& this_mo = *mo;
    RiggedObject& rigged_object = *this_mo.rigged_object();
    Skeleton& skeleton = rigged_object.skeleton();

    int chain_start = ik_chain_start_index[kTailIK];
    int chain_length = ik_chain_length[kTailIK];

    // Tail wag behavior
    bool wag_tail = false;
    if (wag_tail) {
        float wag_freq = 5.0f;
        vec3 tail_root = rigged_object_GetTransformedBonePoint(ik_chain_elements[chain_start + chain_length - 1], 0);
        int hip_bone = skeleton_GetParent(ik_chain_elements[chain_start + chain_length - 1]);
        vec3 axis = rigged_object_GetFrameMatrix(hip_bone).rotation * vec3(0, 0, 1);
        quaternion rotation(vec4(axis, sin(time * wag_freq)));
        for (int i = 0, len = chain_length; i < len; ++i) {
            BoneTransform mat = rigged_object_GetFrameMatrix(ik_chain_elements[chain_start + i]);
            mat.origin -= tail_root;
            mat = rotation * mat;
            mat.origin += tail_root;
            rigged_object_SetFrameMatrix(ik_chain_elements[chain_start + i], mat);
        }
    }

    bool ambient_tail = false;
    if (ambient_tail) {
        vec3 tail_root = rigged_object_GetTransformedBonePoint(ik_chain_elements[chain_start + chain_length - 1], 0);
        int hip_bone = skeleton_GetParent(ik_chain_elements[chain_start + chain_length - 1]);
        vec3 axis = rigged_object_GetFrameMatrix(hip_bone).rotation * vec3(0, 0, 1);
        quaternion rotation(vec4(axis, (sin(time) + sin(time * 1.3f)) * 0.2f));
        for (int i = 0, len = chain_length; i < len; ++i) {
            BoneTransform mat = rigged_object_GetFrameMatrix(ik_chain_elements[chain_start + i]);
            mat.origin -= tail_root;
            mat = rotation * mat;
            mat.origin += tail_root;
            rigged_object_SetFrameMatrix(ik_chain_elements[chain_start + i], mat);
        }
    }

    bool twitch_tail_tip = false;
    if (twitch_tail_tip) {
        float wag_freq = 5.0f;
        vec3 tail_root = rigged_object_GetTransformedBonePoint(ik_chain_elements[chain_start + 0], 0);
        int hip_bone = skeleton_GetParent(ik_chain_elements[chain_start + chain_length - 1]);
        vec3 axis = rigged_object_GetFrameMatrix(hip_bone).rotation * vec3(0, 0, 1);
        quaternion rotation(vec4(axis, sin(time * wag_freq)));
        for (int i = 0; i < 1; ++i) {
            BoneTransform mat = rigged_object_GetFrameMatrix(ik_chain_elements[chain_start + i]);
            mat.origin -= tail_root;
            mat = rotation * mat;
            mat.origin += tail_root;
            rigged_object_SetFrameMatrix(ik_chain_elements[chain_start + i], mat);
        }
    }

    tail_section_length.resize(chain_length);
    tail_correction.resize(chain_length + 1);
    old_tail_points.resize(chain_length + 1);
    temp_old_tail_points.resize(chain_length + 1);
    if (tail_points.size() == 0) {
        tail_points.resize(chain_length + 1);
        for (int i = 0; i < chain_length; ++i) {
            tail_points[i] = rigged_object_GetTransformedBonePoint(ik_chain_elements[chain_start + i], 1);
            old_tail_points[i] = tail_points[i];
            temp_old_tail_points[i] = tail_points[i];
        }
        tail_points[chain_length] = rigged_object_GetTransformedBonePoint(ik_chain_elements[chain_start + chain_length - 1], 0);
        for (int i = 0; i < chain_length; ++i) {
            tail_section_length[i] = distance(tail_points[i], tail_points[i + 1]);
        }
    }
    {
        tail_points[chain_length] = rigged_object_GetTransformedBonePoint(ik_chain_elements[chain_start + chain_length - 1], 0);
        for (int i = 0; i < chain_length + 1; ++i) {
            temp_old_tail_points[i] = tail_points[i];
        }
        // Damping
        for (int i = 0; i < chain_length; ++i) {
            tail_points[i] += (tail_points[i] - old_tail_points[i]) * 0.95f;
        }
        // Gravity
        for (int i = 0; i < chain_length; ++i) {
            tail_points[i].y() -= time_step * num_frames * 0.1f;
        }
        tail_correction[chain_length - 1] += (rigged_object_GetTransformedBonePoint(ik_chain_elements[chain_start + chain_length - 1], 1) - tail_points[chain_length - 1]) * (1.0f - pow(0.9f, num_frames));
        for (int i = chain_length - 2; i >= 0; --i) {
            vec3 offset = rigged_object_GetTransformedBonePoint(ik_chain_elements[chain_start + i], 1) - rigged_object_GetTransformedBonePoint(ik_chain_elements[chain_start + i + 1], 1);
            quaternion rotation;
            GetRotationBetweenVectors(rigged_object_GetTransformedBonePoint(ik_chain_elements[chain_start + i + 1], 1) - rigged_object_GetTransformedBonePoint(ik_chain_elements[chain_start + i + 1], 0), tail_points[i + 1] - tail_points[i + 2], rotation);
            tail_correction[i] = ((tail_points[i + 1] + ASMult(rotation, offset)) - tail_points[i]) * (0.2f) * 0.5f;
            tail_correction[i + 1] -= tail_correction[i];
        }
        for (int i = chain_length - 1; i >= 0; --i) {
            tail_points[i] += tail_correction[i];
        }

        for (int j = 0, len = max(5, int(num_frames * 1.5)); j < len; ++j) {
            for (int i = 0; i < chain_length + 1; ++i) {
                tail_correction[i] = vec3(0.0f);
            }
            for (int i = 0; i < chain_length; ++i) {
                vec3 mid = (tail_points[i] + tail_points[i + 1]) * 0.5f;
                vec3 dir = normalize(tail_points[i] - tail_points[i + 1]);
                tail_correction[i] += (mid + dir * tail_section_length[i] * 0.5f - tail_points[i]) * 0.75f;
                tail_correction[i + 1] += (mid - dir * tail_section_length[i] * 0.5f - tail_points[i + 1]) * 0.25f;
            }
            for (int i = chain_length - 1; i >= 0; --i) {
                tail_points[i] += tail_correction[i] * min(1.0f, (0.5f + j * 0.125f));
            }
        }

        for (int i = chain_length - 1; i >= 0; --i) {
            ASCollisions* col = mo->as_collisions.get();
            col->GetSweptSphereCollision(tail_points[i + 1], tail_points[i], 0.03f, col->as_col);
            tail_points[i] = col->as_col.adjusted_position;
        }

        for (int i = 0; i < chain_length + 1; ++i) {
            old_tail_points[i] = temp_old_tail_points[i];
        }
    }

    // Enforce tail lengths for drawing
    vec3 root = rigged_object_GetTransformedBonePoint(ik_chain_elements[chain_start + chain_length - 1], 0);
    vec3 root_tail = rigged_object_GetTransformedBonePoint(ik_chain_elements[chain_start + chain_length - 1], 1);
    float root_len = distance(root, root_tail);
    temp_old_tail_points[chain_length - 1] = root + normalize(tail_points[chain_length - 1] - root) * root_len;
    for (int i = chain_length - 1; i > 0; --i) {
        temp_old_tail_points[i - 1] = temp_old_tail_points[i] + normalize(tail_points[i - 1] - tail_points[i]) * tail_section_length[i - 1];
    }

    for (int i = 0; i < chain_length; ++i) {
        rigged_object_RotateBoneToMatchVec(temp_old_tail_points[i + 1], temp_old_tail_points[i], ik_chain_elements[chain_start + i]);
    }
}

void CDrawTail(MovementObject* mo, int num_frames) {
    PROFILER_ZONE(g_profiler_ctx, "C++ DrawTail");
    MovementObject& this_mo = *mo;

    PROFILER_ENTER(g_profiler_ctx, "Get AS pointers");
    ASContext* as_context = this_mo.as_context.get();
    float& time = game_timer.game_time;
    float& time_step = game_timer.timestep;

    CScriptArrayWrapper<int> ik_chain_elements((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_elements"));
    CScriptArrayWrapper<int> ik_chain_start_index((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_start_index"));
    CScriptArrayWrapper<int> ik_chain_length((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_length"));
    CScriptArrayWrapper<vec3> tail_points((CScriptArray*)as_context->module.GetVarPtrCache("tail_points"));
    CScriptArrayWrapper<vec3> old_tail_points((CScriptArray*)as_context->module.GetVarPtrCache("old_tail_points"));
    CScriptArrayWrapper<vec3> temp_old_tail_points((CScriptArray*)as_context->module.GetVarPtrCache("temp_old_tail_points"));
    CScriptArrayWrapper<vec3> tail_correction((CScriptArray*)as_context->module.GetVarPtrCache("tail_correction"));
    CScriptArrayWrapper<float> tail_section_length((CScriptArray*)as_context->module.GetVarPtrCache("tail_section_length"));
    PROFILER_LEAVE(g_profiler_ctx);

    CDrawTailImpl(
        mo,
        time, time_step,
        ik_chain_elements, ik_chain_start_index, ik_chain_length, tail_points, old_tail_points, temp_old_tail_points, tail_correction, tail_section_length,
        num_frames);
}

static void CDrawFinalBoneIK(MovementObject* mo, int num_frames) {
    PROFILER_ZONE(g_profiler_ctx, "C++ Draw Final Bone IK");
    MovementObject& this_mo = *mo;
    RiggedObject& rigged_object = *this_mo.rigged_object();
    Skeleton& skeleton = rigged_object.skeleton();

    PROFILER_ENTER(g_profiler_ctx, "Get AS pointers");
    ASContext* as_context = this_mo.as_context.get();
    float& breath_amount = *(float*)as_context->module.GetVarPtrCache("breath_amount");
    float& max_speed = *(float*)as_context->module.GetVarPtrCache("max_speed");
    float& true_max_speed = *(float*)as_context->module.GetVarPtrCache("true_max_speed");
    int& idle_type = *(int*)as_context->module.GetVarPtrCache("idle_type");
    bool& on_ground = *(bool*)as_context->module.GetVarPtrCache("on_ground");
    bool& flip_info_flipping = *(bool*)((asIScriptObject*)as_context->module.GetVarPtrCache("flip_info"))->GetAddressOfProperty(0);
    float& threat_amount = *(float*)as_context->module.GetVarPtrCache("threat_amount");
    bool& ledge_info_on_ledge = *(bool*)((asIScriptObject*)as_context->module.GetVarPtrCache("ledge_info"))->GetAddressOfProperty(0);
    float& time = game_timer.game_time;
    float& time_step = game_timer.timestep;
    int& species = *(int*)as_context->module.GetVarPtrCache("species");
    int& skip_ear_physics_counter = *(int*)as_context->module.GetVarPtrCache("skip_ear_physics_counter");

    CScriptArrayWrapper<BoneTransform> key_transforms((CScriptArray*)as_context->module.GetVarPtrCache("key_transforms"));
    CScriptArrayWrapper<int> ik_chain_start_index((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_start_index"));
    CScriptArrayWrapper<int> ik_chain_elements((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_elements"));
    CScriptArrayWrapper<int> ik_chain_length((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_length"));
    CScriptArrayWrapper<float> ik_chain_bone_lengths((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_bone_lengths"));
    CScriptArrayWrapper<int> bone_children((CScriptArray*)as_context->module.GetVarPtrCache("bone_children"));
    CScriptArrayWrapper<int> bone_children_index((CScriptArray*)as_context->module.GetVarPtrCache("bone_children_index"));
    CScriptArrayWrapper<BoneTransform> skeleton_bind_transforms((CScriptArray*)as_context->module.GetVarPtrCache("skeleton_bind_transforms"));
    CScriptArrayWrapper<BoneTransform> inv_skeleton_bind_transforms((CScriptArray*)as_context->module.GetVarPtrCache("inv_skeleton_bind_transforms"));

    CScriptArrayWrapper<vec3> arm_points((CScriptArray*)as_context->module.GetVarPtrCache("arm_points"));
    CScriptArrayWrapper<vec3> old_arm_points((CScriptArray*)as_context->module.GetVarPtrCache("old_arm_points"));
    CScriptArrayWrapper<vec3> temp_old_arm_points((CScriptArray*)as_context->module.GetVarPtrCache("temp_old_arm_points"));
    CScriptArrayWrapper<float> ear_rotation((CScriptArray*)as_context->module.GetVarPtrCache("ear_rotation"));
    CScriptArrayWrapper<float> ear_rotation_time((CScriptArray*)as_context->module.GetVarPtrCache("ear_rotation_time"));
    CScriptArrayWrapper<float> target_ear_rotation((CScriptArray*)as_context->module.GetVarPtrCache("target_ear_rotation"));
    CScriptArrayWrapper<vec3> ear_points((CScriptArray*)as_context->module.GetVarPtrCache("ear_points"));
    CScriptArrayWrapper<vec3> old_ear_points((CScriptArray*)as_context->module.GetVarPtrCache("old_ear_points"));
    CScriptArrayWrapper<vec3> temp_old_ear_points((CScriptArray*)as_context->module.GetVarPtrCache("temp_old_ear_points"));
    PROFILER_LEAVE(g_profiler_ctx);

    CDrawLegImpl(
        rigged_object, skeleton,
        ik_chain_start_index, ik_chain_elements, ik_chain_bone_lengths, skeleton_bind_transforms, inv_skeleton_bind_transforms,
        false, key_transforms[kHipKey], key_transforms[kLeftLegKey]);
    CDrawLegImpl(
        rigged_object, skeleton,
        ik_chain_start_index, ik_chain_elements, ik_chain_bone_lengths, skeleton_bind_transforms, inv_skeleton_bind_transforms,
        true, key_transforms[kHipKey], key_transforms[kRightLegKey]);

    CDrawArmsImpl(
        this_mo, rigged_object, skeleton,
        breath_amount, max_speed, true_max_speed, idle_type, on_ground, flip_info_flipping, threat_amount, ledge_info_on_ledge,
        skeleton_bind_transforms.c_script_array_, inv_skeleton_bind_transforms.c_script_array_, ik_chain_start_index.c_script_array_, ik_chain_elements.c_script_array_, ik_chain_length.c_script_array_, key_transforms.c_script_array_, arm_points.c_script_array_, old_arm_points.c_script_array_, temp_old_arm_points.c_script_array_, bone_children.c_script_array_, bone_children_index.c_script_array_,
        key_transforms[kChestKey], key_transforms[kLeftArmKey], key_transforms[kRightArmKey], num_frames);

    CDrawHeadImpl(
        rigged_object, skeleton,
        breath_amount,
        key_transforms, inv_skeleton_bind_transforms, ik_chain_start_index, ik_chain_elements, bone_children, bone_children_index,
        key_transforms[kChestKey], key_transforms[kHeadKey]);

    CDrawBodyImpl(
        rigged_object, skeleton,
        inv_skeleton_bind_transforms, ik_chain_start_index, ik_chain_elements, ik_chain_length,
        key_transforms[kHipKey], key_transforms[kChestKey]);

    CDrawEarImpl(
        mo,
        time, time_step, species, skip_ear_physics_counter, flip_info_flipping, on_ground,
        ik_chain_start_index, ik_chain_elements, ear_rotation, ear_rotation_time, target_ear_rotation, inv_skeleton_bind_transforms, ear_points, old_ear_points, temp_old_ear_points,
        false, key_transforms[kHeadKey], num_frames);
    CDrawEarImpl(
        mo,
        time, time_step, species, skip_ear_physics_counter, flip_info_flipping, on_ground,
        ik_chain_start_index, ik_chain_elements, ear_rotation, ear_rotation_time, target_ear_rotation, inv_skeleton_bind_transforms, ear_points, old_ear_points, temp_old_ear_points,
        true, key_transforms[kHeadKey], num_frames);

    if (ik_chain_length[kTailIK] != 0) {
        PROFILER_ENTER(g_profiler_ctx, "Get AS pointers");
        CScriptArrayWrapper<vec3> tail_points((CScriptArray*)as_context->module.GetVarPtrCache("tail_points"));
        CScriptArrayWrapper<vec3> old_tail_points((CScriptArray*)as_context->module.GetVarPtrCache("old_tail_points"));
        CScriptArrayWrapper<vec3> temp_old_tail_points((CScriptArray*)as_context->module.GetVarPtrCache("temp_old_tail_points"));
        CScriptArrayWrapper<vec3> tail_correction((CScriptArray*)as_context->module.GetVarPtrCache("tail_correction"));
        CScriptArrayWrapper<float> tail_section_length((CScriptArray*)as_context->module.GetVarPtrCache("tail_section_length"));
        PROFILER_LEAVE(g_profiler_ctx);

        CDrawTailImpl(
            mo,
            time, time_step,
            ik_chain_elements, ik_chain_start_index, ik_chain_length, tail_points, old_tail_points, temp_old_tail_points, tail_correction, tail_section_length,
            num_frames);
    }
}

static void CSetEyeLookDirImpl(RiggedObject& rigged_object, vec3 eye_dir) {
    // Set weights for carnivore
    rigged_object.SetMTTargetWeight("look_r", max(0.0f, eye_dir.x()), 1.0f);
    rigged_object.SetMTTargetWeight("look_l", max(0.0f, -eye_dir.x()), 1.0f);
    rigged_object.SetMTTargetWeight("look_u", max(0.0f, eye_dir.y()), 1.0f);
    rigged_object.SetMTTargetWeight("look_d", max(0.0f, -eye_dir.y()), 1.0f);

    // Set weights for herbivore
    rigged_object.SetMTTargetWeight("look_u", max(0.0f, eye_dir.y()), 1.0f);
    rigged_object.SetMTTargetWeight("look_d", max(0.0f, -eye_dir.y()), 1.0f);
    rigged_object.SetMTTargetWeight("look_f", max(0.0f, eye_dir.z()), 1.0f);
    rigged_object.SetMTTargetWeight("look_b", max(0.0f, -eye_dir.z()), 1.0f);

    // Set weights for independent-eye herbivore
    rigged_object.SetMTTargetWeight("look_u_l", max(0.0f, eye_dir.y()), 1.0f);
    rigged_object.SetMTTargetWeight("look_u_r", max(0.0f, eye_dir.y()), 1.0f);
    rigged_object.SetMTTargetWeight("look_d_l", max(0.0f, -eye_dir.y()), 1.0f);
    rigged_object.SetMTTargetWeight("look_d_r", max(0.0f, -eye_dir.y()), 1.0f);

    float right_front = eye_dir.z();
    float left_front = eye_dir.z();

    rigged_object.SetMTTargetWeight("look_f_r", max(0.0f, right_front), 1.0f);
    rigged_object.SetMTTargetWeight("look_b_r", max(0.0f, -right_front), 1.0f);
    rigged_object.SetMTTargetWeight("look_f_l", max(0.0f, left_front), 1.0f);
    rigged_object.SetMTTargetWeight("look_b_l", max(0.0f, -left_front), 1.0f);
}

typedef enum {
    kLeftEye,
    kRightEye
} WhichEye;

static bool CGetEyeDirImpl(
    MovementObject& this_mo,
    const CScriptArrayWrapper<BoneTransform>& skeleton_bind_transforms,
    WhichEye which_eye, const std::string& morph_label, vec3& start, vec3& end) {
    if (this_mo.character_script_getter.GetMorphMetaPoints(morph_label, start, end)) {
        if (which_eye == kLeftEye) {
            start.x() *= -1.0f;
            end.x() *= -1.0f;
        }

        RiggedObject& rigged_object = *this_mo.rigged_object();

        Skeleton& skeleton = rigged_object.skeleton();
        BoneTransform test = skeleton_bind_transforms[ASIKBoneStart(&skeleton, "head")];
        float temp;
        vec3 model_center = ASGetModelCenter(&rigged_object);
        temp = start.z();
        start.z() = -start.y();
        start.y() = temp;
        temp = end.z();
        end.z() = -end.y();
        end.y() = temp;
        start -= model_center;
        end -= model_center;
        start = test * start;
        end = test * end;

        return true;
    } else {
        return false;
    }
}

static void CUpdateShadowImpl(
    MovementObject* mo,
    int& shadow_id,
    int& lf_shadow_id,
    int& rf_shadow_id,
    const CScriptArrayWrapper<BoneTransform>& skeleton_bind_transforms) {
    MovementObject& this_mo = *mo;
    RiggedObject& rigged_object = *this_mo.rigged_object();
    Skeleton& skeleton = rigged_object.skeleton();

    int bone = ASIKBoneStart(&skeleton, "torso");
    BoneTransform transform = rigged_object.display_bone_matrices[bone];
    BoneTransform bind_matrix = invert(skeleton_bind_transforms[bone]);
    transform = transform;
    vec3 torso = transform * skeleton.points[skeleton.bones[bone].points[0]];

    bone = ASIKBoneStart(&skeleton, "head");
    transform = rigged_object.display_bone_matrices[bone];
    bind_matrix = invert(skeleton_bind_transforms[bone]);
    transform = transform;
    vec3 head = transform * skeleton.points[skeleton.bones[bone].points[0]];

    bone = ASIKBoneStart(&skeleton, "left_leg");
    transform = rigged_object.display_bone_matrices[bone];
    bind_matrix = invert(skeleton_bind_transforms[bone]);
    transform = transform;
    vec3 left_foot = transform * skeleton.points[skeleton.bones[bone].points[0]];

    bone = ASIKBoneStart(&skeleton, "right_leg");
    transform = rigged_object.display_bone_matrices[bone];
    bind_matrix = invert(skeleton_bind_transforms[bone]);
    transform = transform;
    vec3 right_foot = transform * skeleton.points[skeleton.bones[bone].points[0]];

    {
        if (shadow_id == -1) {
            shadow_id = ASCreateObject("Data/Objects/Decals/blob_shadow.xml", true);
        }

        Object* shadow_obj = ReadObjectFromID(shadow_id);
        vec3 scale = vec3(1.5f, max(1.5f, distance((left_foot + right_foot) * 0.5f, head) * 2.0f), 1.5f);
        shadow_obj->SetScale(scale);

        shadow_obj->SetTranslation(torso + vec3(0.0f, -0.3f, 0.0f));
    }

    {
        if (lf_shadow_id == -1) {
            lf_shadow_id = ASCreateObject("Data/Objects/Decals/blob_shadow.xml", true);
        }

        Object* shadow_obj = ReadObjectFromID(lf_shadow_id);

        shadow_obj->SetTranslation(left_foot + vec3(0.0f));
        shadow_obj->SetScale(vec3(0.4f));
    }

    {
        if (rf_shadow_id == -1) {
            rf_shadow_id = ASCreateObject("Data/Objects/Decals/blob_shadow.xml", true);
        }

        Object* shadow_obj = ReadObjectFromID(rf_shadow_id);

        shadow_obj->SetTranslation(right_foot + vec3(0.0f, 0.0f, 0.0f));
        shadow_obj->SetScale(vec3(0.4f));
    }
}

static void CUpdateShadow(MovementObject* mo) {
    PROFILER_ZONE(g_profiler_ctx, "C++ Update Shadow")
    MovementObject& this_mo = *mo;

    PROFILER_ENTER(g_profiler_ctx, "Get AS pointers");
    ASContext* as_context = this_mo.as_context.get();
    int& shadow_id = *(int*)as_context->module.GetVarPtrCache("shadow_id");
    int& lf_shadow_id = *(int*)as_context->module.GetVarPtrCache("lf_shadow_id");
    int& rf_shadow_id = *(int*)as_context->module.GetVarPtrCache("rf_shadow_id");

    CScriptArrayWrapper<BoneTransform> skeleton_bind_transforms((CScriptArray*)as_context->module.GetVarPtrCache("skeleton_bind_transforms"));
    PROFILER_LEAVE(g_profiler_ctx);

    CUpdateShadowImpl(
        mo,
        shadow_id, lf_shadow_id, rf_shadow_id,
        skeleton_bind_transforms);
}

static void CUpdateEyeLookImpl(
    MovementObject* mo,
    float& time,
    vec3& eye_dir,
    vec3 eye_look_target,
    vec3& eye_offset,
    float& eye_offset_time,
    Species species,
    int knocked_out,
    const CScriptArrayWrapper<int>& ik_chain_elements,
    const CScriptArrayWrapper<int>& ik_chain_start_index,
    const CScriptArrayWrapper<BoneTransform>& skeleton_bind_transforms) {
    MovementObject& this_mo = *mo;

    if (knocked_out != this_mo._awake) {
        return;
    }

    RiggedObject& rigged_object = *this_mo.rigged_object();

    vec3 target_pos = eye_look_target;
    BoneTransform head_mat = rigged_object_GetFrameMatrix(ik_chain_elements[ik_chain_start_index[kHeadIK]]);
    vec3 temp_target_pos = normalize(invert(head_mat) * target_pos);

    vec3 base_start, base_end;
    WhichEye which_eye = kRightEye;
    bool valid = CGetEyeDirImpl(this_mo, skeleton_bind_transforms, kRightEye, "look_c", base_start, base_end);

    if (valid) {
        eye_dir = 0;

        if (species == _rabbit || species == _rat) {
            if ((invert(head_mat) * ActiveCameras::GetCamera(this_mo.camera_id)->GetPos()).x() < 0.0f) {
                which_eye = kLeftEye;
            }

            CGetEyeDirImpl(this_mo, skeleton_bind_transforms, which_eye, "look_c", base_start, base_end);
        }

        vec3 start1, end1, start2, end2;
        {
            CGetEyeDirImpl(this_mo, skeleton_bind_transforms, which_eye, "look_u", start1, end1);
            CGetEyeDirImpl(this_mo, skeleton_bind_transforms, which_eye, "look_d", start2, end2);
            vec3 normal = normalize(cross(end2 - base_start, end1 - base_start));
            vec3 test_targ = temp_target_pos - normal * dot(normal, temp_target_pos);
            vec3 neutral_dir = normalize(base_end - base_start);
            vec3 normal2 = normalize(cross(normal, neutral_dir));
            float up_angle = asin(dot(normalize(end1 - start1), normal2));
            float down_angle = asin(dot(normalize(end2 - start2), normal2));
            float targ_angle = asin(dot(normalize(test_targ - base_start), normal2));

            if (targ_angle > 0.0f) {
                eye_dir.y() = min(1.0f, targ_angle / up_angle);
            } else {
                eye_dir.y() = -min(1.0f, targ_angle / down_angle);
            }
        }

        if (species == _rabbit || species == _rat) {
            CGetEyeDirImpl(this_mo, skeleton_bind_transforms, which_eye, "look_f", start1, end1);
            CGetEyeDirImpl(this_mo, skeleton_bind_transforms, which_eye, "look_b", start2, end2);
            vec3 normal = normalize(cross(end2 - base_start, end1 - base_start));
            vec3 test_targ = temp_target_pos - normal * dot(normal, temp_target_pos);
            vec3 neutral_dir = normalize(base_end - base_start);
            vec3 normal2 = normalize(cross(normal, neutral_dir));
            float front_angle = asin(dot(normalize(end1 - start1), normal2));
            float back_angle = asin(dot(normalize(end2 - start2), normal2));
            float targ_angle = asin(dot(normalize(test_targ - base_start), normal2));

            if (targ_angle > 0.0f) {
                eye_dir.z() = min(1.0f, targ_angle / front_angle);
            } else {
                eye_dir.z() = -min(1.0f, targ_angle / back_angle);
            }
        } else {
            CGetEyeDirImpl(this_mo, skeleton_bind_transforms, which_eye, "look_l", start1, end1);
            CGetEyeDirImpl(this_mo, skeleton_bind_transforms, which_eye, "look_r", start2, end2);
            vec3 normal = normalize(cross(end2 - base_start, end1 - base_start));
            vec3 test_targ = temp_target_pos - normal * dot(normal, temp_target_pos);
            vec3 neutral_dir = normalize(base_end - base_start);
            vec3 normal2 = normalize(cross(normal, neutral_dir));
            float front_angle = asin(dot(normalize(end1 - start1), normal2));
            float back_angle = asin(dot(normalize(end2 - start2), normal2));
            float targ_angle = asin(dot(normalize(test_targ - base_start), normal2));

            if (targ_angle > 0.0f) {
                eye_dir.x() = -min(1.0f, targ_angle / front_angle);
            } else {
                eye_dir.x() = min(1.0f, targ_angle / back_angle);
            }
        }
    }

    if (eye_offset_time < time) {
        eye_offset = vec3(RangedRandomFloat(-0.2f, 0.2f),
                          RangedRandomFloat(-0.2f, 0.2f),
                          RangedRandomFloat(-0.2f, 0.2f));
        eye_offset_time = time + RangedRandomFloat(0.2f, 1.0f);
    }

    CSetEyeLookDirImpl(rigged_object, eye_dir + eye_offset);
}

static void CUpdateEyeLook(MovementObject* mo) {
    PROFILER_ZONE(g_profiler_ctx, "C++ Update Eye Look")
    MovementObject& this_mo = *mo;

    PROFILER_ENTER(g_profiler_ctx, "Get AS pointers");
    ASContext* as_context = this_mo.as_context.get();
    float& time = game_timer.game_time;
    vec3& eye_dir = *(vec3*)as_context->module.GetVarPtrCache("eye_dir");
    vec3 eye_look_target = *(vec3*)as_context->module.GetVarPtrCache("eye_look_target");
    vec3& eye_offset = *(vec3*)as_context->module.GetVarPtrCache("eye_offset");
    float& eye_offset_time = *(float*)as_context->module.GetVarPtrCache("eye_offset_time");
    Species species = *(Species*)as_context->module.GetVarPtrCache("species");
    int knocked_out = *(int*)as_context->module.GetVarPtrCache("knocked_out");

    CScriptArrayWrapper<int> ik_chain_elements((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_elements"));
    CScriptArrayWrapper<int> ik_chain_start_index((CScriptArray*)as_context->module.GetVarPtrCache("ik_chain_start_index"));
    CScriptArrayWrapper<BoneTransform> skeleton_bind_transforms((CScriptArray*)as_context->module.GetVarPtrCache("skeleton_bind_transforms"));
    PROFILER_LEAVE(g_profiler_ctx);

    CUpdateEyeLookImpl(
        mo,
        time, eye_dir, eye_look_target, eye_offset, eye_offset_time, species, knocked_out,
        ik_chain_elements, ik_chain_start_index, skeleton_bind_transforms);
}

vec3 GetWind(vec3 check_where, float curr_game_time, float change_rate) {
    vec3 wind_vel;
    check_where[0] += curr_game_time * 0.7f * change_rate;
    check_where[1] += curr_game_time * 0.3f * change_rate;
    check_where[2] += curr_game_time * 0.5f * change_rate;
    wind_vel[0] = sin(check_where[0]) + cos(check_where[1] * 1.3f) + sin(check_where[2] * 3.0f);
    wind_vel[1] = sin(check_where[0] * 1.2f) + cos(check_where[1] * 1.8f) + sin(check_where[2] * 0.8f);
    wind_vel[2] = sin(check_where[0] * 1.6f) + cos(check_where[1] * 0.5f) + sin(check_where[2] * 1.2f);

    return wind_vel;
}

void CFireRibbonUpdate(MovementObject* mo, asIScriptObject* obj, float delta_time, float curr_game_time) {
    ASContext* as_context = mo->as_context.get();
    bool& on_fire = *(bool*)as_context->module.GetVarPtrCache("on_fire");
    vec3& pos = *(vec3*)obj->GetAddressOfProperty(2);
    vec3& vel = *(vec3*)obj->GetAddressOfProperty(3);
    float& spawn_new_particle_delay = *(float*)obj->GetAddressOfProperty(5);

    CScriptArrayWrapper<asIScriptObject*> particles((CScriptArray*)obj->GetAddressOfProperty(0));

    bool non_zero_particles = false;
    for (int i = 0, len = particles.size(); i < len; ++i) {
        asIScriptObject* particle = (asIScriptObject*)(particles.c_script_array_->At(i));
        // asIScriptObject*& particle = particles[i];
        float& heat = *(float*)particle->GetAddressOfProperty(3);
        if (heat > 0.0f) {
            non_zero_particles = true;
        }
    }
    if (non_zero_particles || on_fire) {
        spawn_new_particle_delay -= delta_time;
        if (spawn_new_particle_delay <= 0.0f) {
            particles.resize(particles.size() + 1);
            asIScriptObject* particle = (asIScriptObject*)(particles.c_script_array_->At(particles.size() - 1));
            // asIScriptObject*& particle = particles[particles.size()-1];
            vec3& particle_pos = *(vec3*)particle->GetAddressOfProperty(0);
            vec3& particle_vel = *(vec3*)particle->GetAddressOfProperty(1);
            float& particle_width = *(float*)particle->GetAddressOfProperty(2);
            float& particle_heat = *(float*)particle->GetAddressOfProperty(3);
            float& particle_spawn_time = *(float*)particle->GetAddressOfProperty(4);
            particle_pos = pos;
            particle_vel = vel * 0.1f;
            particle_width = 0.12f * min(2.0f, (length(vel) * 0.1f + 1.0f));
            particle_heat = RangedRandomFloat(0.0, 1.5);
            if (!on_fire || particles.size() == 0) {
                particle_heat = 0.0f;
            }
            particle_spawn_time = curr_game_time;

            while (spawn_new_particle_delay <= 0.0f) {
                spawn_new_particle_delay += 0.1f;
            }
        }
        int max_particles = 5;
        if (int(particles.size()) > max_particles) {
            for (int i = 0; i < max_particles; ++i) {
                asIScriptObject* dst_particle = (asIScriptObject*)(particles.c_script_array_->At(i));
                asIScriptObject* src_particle = (asIScriptObject*)(particles.c_script_array_->At(particles.size() - max_particles + i));
                // asIScriptObject*& dst_particle = particles[i];
                // asIScriptObject*& src_particle = particles[particles.size()-max_particles+i];
                dst_particle->CopyFrom(src_particle);
            }
            particles.resize(max_particles);
        }
        if (particles.size() > 0) {
            asIScriptObject* particle = (asIScriptObject*)(particles.c_script_array_->At(particles.size() - 1));
            // asIScriptObject*& particle = particles[particles.size()-1];
            vec3& particle_pos = *(vec3*)particle->GetAddressOfProperty(0);
            particle_pos = pos;
        }
        for (int i = 0, len = particles.size(); i < len; ++i) {
            asIScriptObject* particle = (asIScriptObject*)(particles.c_script_array_->At(i));
            // asIScriptObject*& particle = particles[i];
            vec3& particle_pos = *(vec3*)particle->GetAddressOfProperty(0);
            vec3& particle_vel = *(vec3*)particle->GetAddressOfProperty(1);
            float& particle_width = *(float*)particle->GetAddressOfProperty(2);
            float& particle_heat = *(float*)particle->GetAddressOfProperty(3);
            float& particle_spawn_time = *(float*)particle->GetAddressOfProperty(4);
            particle_vel *= pow(0.2f, delta_time);
            particle_pos += particle_vel * delta_time;
            particle_vel += GetWind(particle_pos * 5.0f, curr_game_time, 10.0f) * delta_time * 1.0f;
            particle_vel += GetWind(particle_pos * 30.0f, curr_game_time, 10.0f) * delta_time * 2.0f;
            float max_dist = 0.2f;
            if (i != len - 1) {
                asIScriptObject* next_particle = (asIScriptObject*)(particles.c_script_array_->At(i + 1));
                // asIScriptObject*& next_particle = particles[i+1];
                vec3& next_particle_pos = *(vec3*)next_particle->GetAddressOfProperty(0);
                if (distance(next_particle_pos, particle_pos) > max_dist) {
                    // particles[i].pos = normalize(particles[i].pos - particles[i+1].pos) * max_dist + particles[i+1].pos;
                    particle_vel += normalize(next_particle_pos - particle_pos) * (distance(next_particle_pos, particle_pos) - max_dist) * 100.0f * delta_time;
                }
                particle_heat -= length(next_particle_pos - particle_pos) * 10.0f * delta_time;
            }
            particle_heat -= delta_time * 3.0f;
            particle_vel[1] += delta_time * 12.0f;

            vec3 rel = particle_pos - mo->position;
            rel[1] = 0.0;
            // particles[i].heat -= delta_time * (2.0f + min(1.0f, pow(dot(rel,rel), 2.0)*64.0f)) * 2.0f;
            if (dot(rel, rel) > 1.0) {
                rel = normalize(rel);
            }

            particle_vel += rel * delta_time * -3.0f * 6.0f;
        }
        /*for(int i=0, len=particles.size()-1; i<len; ++i){
        //DebugDrawLine(particles[i].pos, particles[i+1].pos, ColorFromHeat(particles[i].heat), ColorFromHeat(particles[i+1].heat), _delete_on_update);
        DebugDrawRibbon(particles[i].pos, particles[i+1].pos, ColorFromHeat(particles[i].heat), ColorFromHeat(particles[i+1].heat), flame_width * max(particles[i].heat, 0.0), flame_width * max(particles[i+1].heat, 0.0), _delete_on_update);
        }*/
    } else {
        particles.resize(0);
    }
}

#undef rigged_object_SetFrameMatrix
#undef rigged_object_GetFrameMatrix
#undef rigged_object_GetTransformedBonePoint
#undef rigged_object_RotateBoneToMatchVec
#undef skeleton_GetParent

}  // namespace

void MovementObject::SetEnabled(bool val) {
    if (val != enabled_) {
        if (Online::Instance()->IsHosting()) {
            Online::Instance()->Send<OnlineMessages::SetObjectEnabledMessage>(GetID(), val);
        }
        Object::SetEnabled(val);
        ASArglist args;
        args.Add(val);
        as_context->CallScriptFunction(as_funcs.set_enabled, &args);

        if (!enabled_) {
            scenegraph_->UnlinkUpdateObject(this, update_list_entry);
            update_list_entry = -1;
            scenegraph_->abstract_bullet_world_->UnlinkObject(char_sphere);
        } else {
            update_list_entry = scenegraph_->LinkUpdateObject(this);
            scenegraph_->abstract_bullet_world_->LinkObject(char_sphere);
        }
    }
}

void MovementObject::ReceiveMessage(std::string msg) {
    ASArglist args;
    args.AddObject((void*)&msg);
    as_context->CallScriptFunction(as_funcs.receive_message, &args);
}

bool MovementObject::Initialize() {
    SDL_assert(update_list_entry == -1);
    update_list_entry = scenegraph_->LinkUpdateObject(this);

    PROFILER_ENTER(g_profiler_ctx, "Preparing movementobject angelscript context");
    ASData as_data;
    as_data.scenegraph = scenegraph_;
    as_data.gui = scenegraph_->map_editor->gui;
    ASContext* ctx = new ASContext("movement_object", as_data);
    as_context.reset(ctx);
    AttachUIQueries(ctx);
    AttachMovementObjectCamera(ctx, this);
    AttachScreenWidth(ctx);
    AttachPhysics(ctx);
    AttachTextCanvasTextureToASContext(ctx);
    ScriptParams::RegisterScriptType(ctx);
    AttachLevel(ctx);
    AttachInterlevelData(ctx);
    AttachNavMesh(ctx);
    AttachMessages(ctx);
    AttachTokenIterator(ctx);
    AttachAttackScriptGetter(as_context.get());
    sp.RegisterScriptInstance(ctx);
    AttachStringConvert(ctx);
    AttachOnline(ctx);

    as_collisions.reset(new ASCollisions(scenegraph_));
    as_collisions->AttachToContext(ctx);

    DefineRiggedObjectTypePublic(ctx);

    DefineMovementObjectTypePublic(ctx);
    as_context->GetEngine()->RegisterInterface("C_ACCEL");
    as_context->RegisterObjectMethod("MovementObject", "void AddToAttackHistory(const string &in attack_path)", asMETHOD(MovementObject, AddToAttackHistory), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "float CheckAttackHistory(const string &in attack_path)", asMETHOD(MovementObject, CheckAttackHistory), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void ClearAttackHistory()", asMETHOD(MovementObject, ClearAttackHistory), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "int WasHit(string type, string attack_path, vec3 dir, vec3 pos, int attacker_id, float attack_damage_mult, float attack_knockback_mult)", asMETHOD(MovementObject, ASWasHit), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void WasBlocked()", asMETHOD(MovementObject, ASWasBlocked), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "float GetTempHealth()", asMETHOD(MovementObject, GetTempHealth), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void Ragdoll()", asMETHOD(MovementObject, Ragdoll), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void ApplyForce(vec3 force)", asMETHOD(MovementObject, ApplyForce), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "vec3 GetFacing()", asMETHOD(MovementObject, GetFacing), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void UnRagdoll()", asMETHOD(MovementObject, UnRagdoll), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void SetAnimation(string anim_path)", asMETHODPR(MovementObject, SetAnimation, (std::string), void), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void SetAnimAndCharAnim(string anim_path, float transition_speed, int8 flags, string char_anim)", asMETHOD(MovementObject, SetAnimAndCharAnim), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void SwapAnimation(string anim_path)", asMETHODPR(MovementObject, SwapAnimation, (std::string), void), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void SetAnimation(string anim_path, float transition_speed)", asMETHODPR(MovementObject, SetAnimation, (std::string, float), void), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void SetAnimation(string anim_path, float transition_speed, int8 flags)", asMETHODPR(MovementObject, SetAnimation, (std::string, float, char), void), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void OverrideCharAnim(const string &in char_anim, const string &in anim_path)", asMETHOD(MovementObject, OverrideCharAnim), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void SetCharAnimation(string char_anim, float transition_speed, int8 flags)", asMETHODPR(MovementObject, ASSetCharAnimation, (std::string, float, char), void), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void SetCharAnimation(string char_anim, float transition_speed)", asMETHODPR(MovementObject, ASSetCharAnimation, (std::string, float), void), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void SetCharAnimation(string char_anim)", asMETHODPR(MovementObject, ASSetCharAnimation, (std::string), void), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void MaterialEvent(string event, vec3 position)", asMETHOD(MovementObject, HandleMovementObjectMaterialEventDefault), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void MaterialEvent(string event, vec3 position, float audio_gain)", asMETHOD(MovementObject, HandleMovementObjectMaterialEvent), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void PlaySoundGroupAttached(string path, vec3 position)", asMETHOD(MovementObject, ASPlaySoundGroupAttached), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void PlaySoundAttached(string path, vec3 position)", asMETHOD(MovementObject, ASPlaySoundAttached), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void PlaySoundGroupVoice(string voice_key, float delay)", asMETHOD(MovementObject, PlaySoundGroupVoice), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void ForceSoundGroupVoice(string voice_key, float delay)", asMETHOD(MovementObject, ForceSoundGroupVoice), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void StopVoice()", asMETHOD(MovementObject, StopVoice), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "vec4 GetAvgRotationVec4()", asMETHOD(MovementObject, GetAvgRotationVec4), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "int getID()", asMETHOD(MovementObject, GetID), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void AttachItemToSlot(int item_id, int attachment_type, bool mirrored)", asMETHOD(MovementObject, AttachItemToSlot), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void DetachItem(int item_id)", asMETHOD(MovementObject, ASDetachItem), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void DetachAllItems()", asMETHOD(MovementObject, ASDetachAllItems), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void MaterialParticleAtBone(string type, string IK_label)", asMETHOD(MovementObject, MaterialParticleAtBone), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void MaterialParticle(const string &in type, const vec3 &in pos, const vec3 &in vel)", asFUNCTION(asMaterialParticle), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("MovementObject", "void RecreateRiggedObject(string character_path)", asMETHOD(MovementObject, RecreateRiggedObject), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "int GetWaypointTarget()", asMETHOD(MovementObject, GetWaypointTarget), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void UpdateWeapons()", asMETHOD(MovementObject, UpdateWeapons), asCALL_THISCALL);
    as_context->RegisterObjectMethod("MovementObject", "void CDisplayMatrixUpdate()", asFUNCTION(CDisplayMatrixUpdate), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("MovementObject", "vec3 CGetCenterOfMassEstimate(array<BoneTransform> &in key_transforms, const array<float> &in key_masses, const array<int> &in root_bone)", asFUNCTION(CGetCenterOfMassEstimate), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("MovementObject", "void CDoChestIK(float chest_tilt_offset, float angle_threshold, float torso_damping, float torso_stiffness, int num_frames)", asFUNCTION(CDoChestIK), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("MovementObject", "void CDoHeadIK(float head_tilt_offset, float angle_threshold, float head_damping, float head_accel_inertia, float head_accel_damping, float head_stiffness, int num_frames)", asFUNCTION(CDoHeadIK), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("MovementObject", "void CDoFootIK(const BoneTransform &in local_to_world, int num_frames)", asFUNCTION(CDoFootIK), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("MovementObject", "void CDoHipIK(BoneTransform &inout hip_offset, quaternion &inout hip_rotate, int num_frames)", asFUNCTION(CDoHipIK), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("MovementObject", "void CDrawLeg(bool right, const BoneTransform &in hip_transform, const BoneTransform &in foot_transform, int num_frames)", asFUNCTION(CDrawLeg), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("MovementObject", "void CDrawArms(const BoneTransform &in chest_transform, const BoneTransform &in l_hand_transform, const BoneTransform &in r_hand_transform, int num_frames)", asFUNCTION(CDrawArms), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("MovementObject", "void CDrawHead(const BoneTransform &in chest_transform, const BoneTransform &in head_transform, int num_frames)", asFUNCTION(CDrawHead), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("MovementObject", "void CDrawBody(const BoneTransform &in hip_transform, const BoneTransform &in chest_transform)", asFUNCTION(CDrawBody), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("MovementObject", "void CDrawEar(bool right, const BoneTransform &in head_transform, int num_frames)", asFUNCTION(CDrawEar), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("MovementObject", "void CDrawTail(int num_frames)", asFUNCTION(CDrawTail), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("MovementObject", "void CDrawFinalBoneIK(int num_frames)", asFUNCTION(CDrawFinalBoneIK), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("MovementObject", "void CUpdateShadow()", asFUNCTION(CUpdateShadow), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("MovementObject", "void CUpdateEyeLook()", asFUNCTION(CUpdateEyeLook), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("MovementObject", "void CFireRibbonUpdate(C_ACCEL @, float delta_time, float curr_game_time)", asFUNCTION(CFireRibbonUpdate), asCALL_CDECL_OBJFIRST);

    as_context->DocsCloseBrace();

    as_funcs.init = as_context->RegisterExpectedFunction("bool Init(string)", true);
    as_funcs.init_multiplayer = as_context->RegisterExpectedFunction("void InitMultiplayer()", false);
    as_funcs.is_multiplayer_supported = as_context->RegisterExpectedFunction("void IsMultiplayerSupported()", false);
    as_funcs.set_parameters = as_context->RegisterExpectedFunction("void SetParameters()", true);
    as_funcs.notify_item_detach = as_context->RegisterExpectedFunction("void NotifyItemDetach(int)", true);
    as_funcs.handle_editor_attachment = as_context->RegisterExpectedFunction("void HandleEditorAttachment(int, int, bool)", true);
    as_funcs.contact = as_context->RegisterExpectedFunction("void Contact()", true);
    as_funcs.collided = as_context->RegisterExpectedFunction("void Collided(float, float, float, float, float)", true);
    as_funcs.movement_object_deleted = as_context->RegisterExpectedFunction("void MovementObjectDeleted(int id)", true);
    as_funcs.script_swap = as_context->RegisterExpectedFunction("void ScriptSwap()", true);
    as_funcs.handle_collisions_btc = as_context->RegisterExpectedFunction("void HandleCollisionsBetweenTwoCharacters(MovementObject @other)", true);
    as_funcs.hit_by_item = as_context->RegisterExpectedFunction("void HitByItem(string material, vec3 point, int id, int type)", true);
    as_funcs.update = as_context->RegisterExpectedFunction("void Update(int)", true);
    as_funcs.update_multiplayer = as_context->RegisterExpectedFunction("void UpdateMultiplayer(int frames)", false);
    as_funcs.force_applied = as_context->RegisterExpectedFunction("void ForceApplied(vec3 force)", true);
    as_funcs.get_temp_health = as_context->RegisterExpectedFunction("float GetTempHealth()", true);
    as_funcs.was_hit = as_context->RegisterExpectedFunction("int WasHit(string type, string attack_path, vec3 dir, vec3 pos, int attacker_id, float attack_damage_mult, float attack_knockback_mult)", true);
    as_funcs.reset = as_context->RegisterExpectedFunction("void Reset()", true);
    as_funcs.post_reset = as_context->RegisterExpectedFunction("void PostReset()", true);
    as_funcs.attach_weapon = as_context->RegisterExpectedFunction("void AttachWeapon(int)", true);
    as_funcs.set_enabled = as_context->RegisterExpectedFunction("void SetEnabled(bool)", true);
    as_funcs.receive_message = as_context->RegisterExpectedFunction("void ReceiveMessage(string)", true);
    as_funcs.pre_draw_camera = as_context->RegisterExpectedFunction("void PreDrawCamera(float curr_game_time)", false);
    as_funcs.pre_draw_frame = as_context->RegisterExpectedFunction("void PreDrawFrame(float curr_game_time)", false);
    as_funcs.pre_draw_camera_no_cull = as_context->RegisterExpectedFunction("void PreDrawCameraNoCull(float curr_game_time)", false);
    as_funcs.update_paused = as_context->RegisterExpectedFunction("void UpdatePaused()", true);
    as_funcs.about_to_be_hit_by_item = as_context->RegisterExpectedFunction("int AboutToBeHitByItem(int id)", true);
    as_funcs.InputToEngine = as_context->RegisterExpectedFunction("uint InputToEngine(int input", false);
    as_funcs.attach_misc = as_context->RegisterExpectedFunction("void AttachMisc(int)", false);
    as_funcs.was_blocked = as_context->RegisterExpectedFunction("void WasBlocked()", false);
    as_funcs.reset_waypoint_target = as_context->RegisterExpectedFunction("void ResetWaypointTarget()", false);
    as_funcs.dispose = as_context->RegisterExpectedFunction("void Dispose()", false);
    as_funcs.apply_host_input = as_context->RegisterExpectedFunction("void ApplyHostInput(uint inputs)", false);
    as_funcs.register_mp_callbacks = as_context->RegisterExpectedFunction("void RegisterMPCallBacks()", false);
    as_funcs.set_damage_time_from_socket = as_context->RegisterExpectedFunction("void SetDamageTimeFromSocket()", false);
    as_funcs.set_damage_blood_time_from_socket = as_context->RegisterExpectedFunction("void SetDamageBloodTimeFromSocket()", false);
    as_funcs.start_pose = as_context->RegisterExpectedFunction("bool StartPose(string animation_path)", false);

    as_funcs.apply_host_camera_flat_facing = as_context->RegisterExpectedFunction("void ApplyHostCameraFlatFacing(vec3 direction)", false);

    AttachEngine(ctx);
    AttachScenegraph(ctx, scenegraph_);

    as_context->RegisterGlobalFunction("float GetAnimationEventTime( string &in anim_path, string &in event_label )", asFUNCTION(GetAnimationEventTime), asCALL_CDECL);
    as_context->RegisterGlobalProperty("MovementObject this_mo", this);

    static const int _as_at_grip = _at_grip;
    static const int _as_at_sheathe = _at_sheathe;

    as_context->RegisterGlobalProperty("const int _at_grip", (void*)&_as_at_grip);
    as_context->RegisterGlobalProperty("const int _at_sheathe", (void*)&_as_at_sheathe);

    static const unsigned char __ANM_MIRRORED = _ANM_MIRRORED;
    static const unsigned char __ANM_MOBILE = _ANM_MOBILE;
    static const unsigned char __ANM_SUPER_MOBILE = _ANM_SUPER_MOBILE;
    static const unsigned char __ANM_SWAP = _ANM_SWAP;
    static const unsigned char __ANM_FROM_START = _ANM_FROM_START;

    as_context->RegisterGlobalProperty("const uint8 _ANM_MIRRORED", (void*)&__ANM_MIRRORED);
    as_context->RegisterGlobalProperty("const uint8 _ANM_MOBILE", (void*)&__ANM_MOBILE);
    as_context->RegisterGlobalProperty("const uint8 _ANM_SUPER_MOBILE", (void*)&__ANM_SUPER_MOBILE);
    as_context->RegisterGlobalProperty("const uint8 _ANM_SWAP", (void*)&__ANM_SWAP);
    as_context->RegisterGlobalProperty("const uint8 _ANM_FROM_START", (void*)&__ANM_FROM_START);

    character_script_getter.AttachToScript(as_context.get(), "character_getter");
    reaction_script_getter.AttachToScript(as_context.get(), "reaction_getter");
    PROFILER_LEAVE(g_profiler_ctx);

    PROFILER_ENTER(g_profiler_ctx, "Loading script");
    /*if(!object_script_path.empty()) {
        current_control_script_path = object_script_path;
    } else {*/
    current_control_script_path = scenegraph_->level->GetNPCScript(this);
    //}
    Path script_path = FindFilePath(AssemblePath(script_dir_path, current_control_script_path), kDataPaths | kModPaths);
    while (as_context->LoadScript(script_path) == false) {
    };
    PROFILER_LEAVE(g_profiler_ctx);

    {
        PROFILER_ZONE(g_profiler_ctx, "Calling script Init()");
        ASArglist args;
        args.AddObject(&character_path);
        ASArg ret;
        asBYTE v;
        ret.type = _as_bool;
        ret.data = &v;
        if (as_context->CallScriptFunction(as_funcs.init, &args, &ret)) {
            if (*(asBYTE*)ret.data) {
                // All good;
            } else {
                LOGE << "Failed in Init on MovementObject script" << std::endl;
                return false;
            }
        } else {
            LOGE << "Failed at calling Init on MovementObject script" << std::endl;
            return false;
        }
    }
    as_context->CallScriptFunction(as_funcs.set_parameters);
    SetRotationFromEditorTransform();

    char_sphere = scenegraph_->abstract_bullet_world_->CreateSphere(
        position, _leg_sphere_size, 0);
    char_sphere->SetVisibility(true);
    char_sphere->owner_object = this;
    char_sphere->body->setCollisionFlags(char_sphere->body->getCollisionFlags() |
                                         btCollisionObject::CF_NO_CONTACT_RESPONSE);

    PROFILER_ENTER(g_profiler_ctx, "Exporting docs");
    char path[kPathSize];
    FormatString(path, kPathSize, "%saschar_docs.h", GetWritePath(CoreGameModID).c_str());
    as_context->ExportDocs(path);

    RegisterMPCallbacks();
    PROFILER_LEAVE(g_profiler_ctx);
    return true;
}

bool MovementObject::InitializeMultiplayer() {
    if (!as_context->CallScriptFunction(as_funcs.init_multiplayer)) {
        LOGE << "Failed to initialize mutliplayer script for client " << std::endl;
        return false;
    }
    return true;
}

void MovementObject::GetShaderNames(std::map<std::string, int>& shaders) {
    shaders[shader] = 0;
    if (rigged_object_.get() != NULL) {
        rigged_object_.get()->GetShaderNames(shaders);
    }
}

extern std::stack<ASContext*> active_context_stack;
static void ItemObjectError(int which) __attribute__((noreturn));
static void ItemObjectError(int which) {
    std::string callstack = active_context_stack.top()->GetCallstack();
    FatalError("Error", "There is no item object %d.\n Called from: %s\n", callstack.c_str(), which);
}

void MovementObject::AttachItemToSlotAttachmentRef(int which, AttachmentType type, bool mirrored, const AttachmentRef* ref, bool from_socket) {
    Online* online = Online::Instance();

    Object* obj = scenegraph_->GetObjectFromID(which);
    if (!obj || obj->GetType() != _item_object) {
        ItemObjectError(which);
    }

    if (online->IsActive() && !from_socket) {
        online->Send<OnlineMessages::AttachToMessage>(GetID(), which, type, true, mirrored);
    }
    ItemObject* io = (ItemObject*)obj;
    io->InvalidateHeldReaders();
    io->SetHolderID(GetID());
    rigged_object_->AttachItemToSlot(io, type, mirrored, ref);
    character_script_getter.ItemsChanged(rigged_object_->GetWieldedItemRefs());
    reaction_script_getter.ItemsChanged(rigged_object_->GetWieldedItemRefs());
}

void MovementObject::UpdateWeapons() {
    character_script_getter.ItemsChanged(rigged_object_->GetWieldedItemRefs());
    reaction_script_getter.ItemsChanged(rigged_object_->GetWieldedItemRefs());
}

void MovementObject::AttachItemToSlot(int which, AttachmentType type, bool mirrored) {
    AttachItemToSlotAttachmentRef(which, type, mirrored, NULL, true);
}

void MovementObject::AttachItemToSlotEditor(
    int which,
    AttachmentType type,
    bool mirrored,
    const AttachmentRef& attachment_ref,
    bool from_socket) {
    Object* object = scenegraph_->GetObjectFromID(which);
    if (object != nullptr && object->GetType() == EntityType::_item_object) {
        ItemObject* item_object = (ItemObject*)object;
        item_object->InvalidateReaders();

        ASArglist args2;
        args2.Add(which);
        as_context->CallScriptFunction(as_funcs.notify_item_detach, &args2);
        if (type == _at_unspecified) {
            type = _at_grip;
            mirrored = false;
        }
        AttachItemToSlotAttachmentRef(which, type, mirrored, &attachment_ref, from_socket);
        ASArglist args;
        args.Add(which);
        args.Add(type);
        args.Add(mirrored);
        as_context->CallScriptFunction(as_funcs.handle_editor_attachment, &args);

        for (std::list<ItemObjectScriptReader>::iterator iter = item_connections.begin();
             iter != item_connections.end();) {
            if ((*iter)->GetID() == which) {
                iter = item_connections.erase(iter);
            } else {
                ++iter;
            }
        }

        item_connections.resize(item_connections.size() + 1);
        ItemObjectScriptReader& item_connection = item_connections.back();
        item_connection.AttachToItemObject((ItemObject*)scenegraph_->GetObjectFromID(which));
        item_connection.attachment_type = type;
        item_connection.attachment_mirror = mirrored;
        item_connection.attachment_ref = attachment_ref;
        item_connection.SetInvalidateCallback(&MovementObject::InvalidatedItemCallback, this);
    } else {
        LOGE << "Tried to attach item id -1 to movement object " << *this << std::endl;
    }
}

void MovementObject::ASDetachItem(int which) {
    Object* obj = scenegraph_->GetObjectFromID(which);
    if (!obj || obj->GetType() != _item_object) {
        ItemObjectError(which);
    }
    rigged_object_->DetachItem((ItemObject*)obj);
    character_script_getter.ItemsChanged(rigged_object_->GetWieldedItemRefs());
    reaction_script_getter.ItemsChanged(rigged_object_->GetWieldedItemRefs());
}

void MovementObject::ASDetachAllItems() {
    rigged_object_->DetachAllItems();
    character_script_getter.ItemsChanged(rigged_object_->GetWieldedItemRefs());
    reaction_script_getter.ItemsChanged(rigged_object_->GetWieldedItemRefs());
}

void MovementObject::RegisterMPCallbacks() const {
    if (as_context->HasFunction(as_funcs.register_mp_callbacks)) {
        as_context->CallScriptFunction(as_funcs.register_mp_callbacks);
    }
}

void MovementObject::Collided(const vec3& pos, float impulse, const CollideInfo& collide_info, BulletObject* object) {
    as_context->CallScriptFunction(as_funcs.contact);
    if (!collide_info.true_impact) {
        return;
    }
    float hardness = scenegraph_->GetMaterialHardness(pos);
    // printf("Hardness: %f\n", hardness);

    if (rigged_object_->InHeadChain(object)) {
        hardness *= 5.0f;
        // printf("HEAD HIT %f\n", impulse * hardness);
    }

    ASArglist args;
    args.Add(pos[0]);
    args.Add(pos[1]);
    args.Add(pos[2]);  // For some reason was getting asCONTEXT_NOT_PREPARED when trying to send whole vec3 at once
    args.Add(impulse);
    args.Add(hardness);
    as_context->CallScriptFunction(as_funcs.collided, &args);
}

void MovementObject::InvalidatedItem(ItemObjectScriptReader* invalidated) {
    int weap_id = -1;
    for (std::list<ItemObjectScriptReader>::iterator iter = item_connections.begin();
         iter != item_connections.end();) {
        if (&(*iter) == invalidated) {
            weap_id = (*iter)->GetID();
            iter = item_connections.erase(iter);
        } else {
            ++iter;
        }
    }
    if (as_context.get()) {
        ASArglist args;
        args.Add(weap_id);
        as_context->CallScriptFunction(as_funcs.notify_item_detach, &args);
    }
}

void MovementObject::InvalidatedItemCallback(ItemObjectScriptReader* invalidated, void* this_ptr) {
    ((MovementObject*)this_ptr)->InvalidatedItem(invalidated);
}

bool MovementObject::ConnectTo(Object& other, bool checking_other /*= false*/) {
    EntityType other_type = other.GetType();
    if (other.GetType() == _hotspot_object) {
        return Object::ConnectTo(other, checking_other);
    } else if (other_type == _path_point_object || other_type == _movement_object) {
        connected_pathpoint_id = other.GetID();
        as_context->CallScriptFunction(as_funcs.reset_waypoint_target);
        return true;
    } else {
        if (checking_other) {
            return false;
        } else {
            return other.ConnectTo(*this, true);
        }
    }
}

bool MovementObject::AcceptConnectionsFrom(Object::ConnectionType type, Object& object) {
    return type == kCTPathPoints || type == kCTMovementObjects || type == kCTEnvObjectsAndGroups || type == kCTItemObjects || type == kCTPlaceholderObjects;
}

bool MovementObject::Disconnect(Object& other, bool from_socket /* = false */, bool checking_other /*= false*/) {
    Online* online = Online::Instance();
    if (online->IsActive() && !from_socket) {
        online->Send<OnlineMessages::AttachToMessage>(GetID(), other.GetID(), 0, false, false);
    }

    if (connected_pathpoint_id == other.GetID() && (other.GetType() == _path_point_object || other.GetType() == _movement_object)) {
        connected_pathpoint_id = -1;
        as_context->CallScriptFunction(as_funcs.reset_waypoint_target);
        return true;
    } else if (other.GetType() == _item_object) {
        for (std::list<ItemObjectScriptReader>::iterator iter = item_connections.begin();
             iter != item_connections.end();) {
            if ((*iter)->GetID() == other.GetID()) {
                iter = item_connections.erase(iter);
                if (as_context.get()) {
                    ASArglist args;
                    args.Add(other.GetID());
                    as_context->CallScriptFunction(as_funcs.notify_item_detach, &args);
                }
                rigged_object_->DetachItem((ItemObject*)&other);
                ((ItemObject*)(&other))->Reset();
            } else {
                ++iter;
            }
        }
        return true;
    } else if (other.GetType() == _hotspot_object) {
        return Object::Disconnect(other, checking_other);
    } else {
        if (checking_other) {
            return false;
        } else {
            return other.Disconnect(*this, true);
        }
    }
}

void MovementObject::GetConnectionIDs(std::vector<int>* cons) {
    if (connected_pathpoint_id != -1) {
        cons->push_back(connected_pathpoint_id);
    }
    if (rigged_object()) {
        AttachedItemList::iterator attached_it = rigged_object()->attached_items.items.begin();
        for (; attached_it != rigged_object()->attached_items.items.end(); attached_it++) {
            ItemObject* item = attached_it->item.GetAttached();
            if (item) {
                cons->push_back(item->GetID());
            }
        }

        AttachedItemList::iterator stuck_it = rigged_object()->stuck_items.items.begin();
        for (; stuck_it != rigged_object()->stuck_items.items.end(); stuck_it++) {
            ItemObject* item = stuck_it->item.GetAttached();
            if (item) {
                cons->push_back(item->GetID());
            }
        }
    }
}

void MovementObject::NotifyDeleted(Object* other) {
    Object::NotifyDeleted(other);
    if (other->GetID() == connected_pathpoint_id) {
        connected_pathpoint_id = -1;
        as_context->CallScriptFunction(as_funcs.reset_waypoint_target);
    }
    for (std::list<ItemObjectScriptReader>::iterator iter = item_connections.begin();
         iter != item_connections.end();) {
        if (other->GetID() == (*iter)->GetID()) {
            iter = item_connections.erase(iter);
        } else {
            ++iter;
        }
    }
    if (other->GetType() == _movement_object) {
        ASArglist args;
        args.Add(other->GetID());
        as_context->CallScriptFunction(as_funcs.movement_object_deleted, &args);
    }
}

void MovementObject::FinalizeLoadedConnections() {
    Object::FinalizeLoadedConnections();
    if (do_connection_finalization_remap) {
        if (connection_finalization_remap.find(connected_pathpoint_id) != connection_finalization_remap.end()) {
            connected_pathpoint_id = connection_finalization_remap[connected_pathpoint_id];
        } else {
            connected_pathpoint_id = -1;
        }
    }

    for (auto& item_connection : item_connection_vec) {
        if (do_connection_finalization_remap) {
            if (connection_finalization_remap.find(item_connection.id) != connection_finalization_remap.end()) {
                item_connection.id = connection_finalization_remap[item_connection.id];
            } else {
                item_connection.id = -1;
            }
        }

        int which = item_connection.id;
        AttachmentType type = item_connection.attachment_type;
        bool mirrored = item_connection.mirrored;
        AttachItemToSlotEditor(which, type, mirrored, item_connection.attachment_ref);
    }
    item_connection_vec.clear();

    if (!env_object_attach_data.empty()) {
        std::vector<AttachedEnvObject> attached_env_objects;
        Deserialize(env_object_attach_data, attached_env_objects);
        for (int i = 0; i < (int)attached_env_objects.size();) {
            if (!attached_env_objects[i].direct_ptr) {
                if (do_connection_finalization_remap) {
                    if (connection_finalization_remap.find(attached_env_objects[i].legacy_obj_id) != connection_finalization_remap.end()) {
                        attached_env_objects[i].legacy_obj_id = connection_finalization_remap[attached_env_objects[i].legacy_obj_id];
                    } else {
                        attached_env_objects[i].legacy_obj_id = -1;
                    }
                }

                Object* obj = scenegraph_->GetObjectFromID(attached_env_objects[i].legacy_obj_id);
                if (obj) {
                    attached_env_objects[i].direct_ptr = obj;
                    obj->SetParent(this);
                    ++i;
                } else {
                    attached_env_objects[i] = attached_env_objects.back();
                    attached_env_objects.resize(attached_env_objects.size() - 1);
                }
            } else {
                ++i;
            }
        }
        if (rigged_object_.get()) {
            rigged_object_->children = attached_env_objects;
        }
        Serialize(attached_env_objects, env_object_attach_data);
    }
    connection_finalization_remap.clear();
    do_connection_finalization_remap = false;
}

void MovementObject::GetDesc(EntityDescription& desc) const {
    Object::GetDesc(desc);
    desc.AddString(EDF_FILE_PATH, obj_file);
    if (!object_npc_script_path.empty())
        desc.AddString(EDF_NPC_SCRIPT_PATH, object_npc_script_path);
    if (!object_pc_script_path.empty())
        desc.AddString(EDF_PC_SCRIPT_PATH, object_pc_script_path);
    desc.AddInt(EDF_IS_PLAYER, (int)is_player);

    std::vector<int> connections;
    connections.push_back(connected_pathpoint_id);
    desc.AddIntVec(EDF_CONNECTIONS, connections);

    std::vector<ItemConnectionData> item_connections_vec;
    for (const auto& script_reader : item_connections) {
        ItemConnectionData item_connection;
        item_connection.id = script_reader->GetID();
        item_connection.attachment_type = script_reader.attachment_type;
        item_connection.mirrored = script_reader.attachment_mirror;
        if (script_reader.attachment_ref.valid()) {
            item_connection.attachment_str = script_reader.attachment_ref->path_;
        }
        item_connections_vec.push_back(item_connection);
    }
    desc.AddItemConnectionVec(EDF_ITEM_CONNECTIONS, item_connections_vec);
    desc.AddPalette(EDF_PALETTE, palette);

    if (!rigged_object_->children.empty()) {
        std::vector<char> temp_env_object_attach_data;
        Serialize(rigged_object_->children, temp_env_object_attach_data);
        desc.AddData(EDF_ENV_OBJECT_ATTACH, temp_env_object_attach_data);
    }
    for (auto& i : rigged_object_->children) {
        Object* env_obj = i.direct_ptr;
        if (env_obj) {
            desc.children.resize(desc.children.size() + 1);
            env_obj->GetDesc(desc.children.back());
        }
    }
}

void MovementObject::ASSetScriptUpdatePeriod(int val) {
    update_script_period = val;
}

void MovementObject::ChangeControlScript(const std::string& script_path) {
    PROFILER_ZONE(g_profiler_ctx, "ChangeControlScript");
    if (current_control_script_path != script_path) {
        Path found_path = FindFilePath(AssemblePath(script_dir_path, script_path.c_str()), kDataPaths | kModPaths);
        if (found_path.isValid()) {
            current_control_script_path = script_path;
            if (as_context.get()) {
                as_context->SaveGlobalVars();
                as_context->LoadScript(found_path);
                as_context->LoadGlobalVars();
                update_script_period = (script_path == scenegraph_->level->GetPCScript(this)) ? 1 : 4;
                update_script_counter = rand() % update_script_period - update_script_period;
                rigged_object_->SetAnimUpdatePeriod(max(2, update_script_period));
                as_context->CallScriptFunction(as_funcs.script_swap);
            }
        } else {
            DisplayFormatMessage("Cannot find file", "Cannot change to script file \"%s\" because it couldn't be found", script_path.c_str());
        }
        if (script_path == "playermpcontrol.as" || script_path == "remotecontrol.as") {
            InitializeMultiplayer();
        }
    }
}

void MovementObject::SetRotationFromEditorTransform() {
    vec3 dir = (transform_ * vec4(0.0f, 0.0f, 1.0f, 0.0f)).xyz();
    dir[1] = 0.0f;
    dir = normalize(dir);
    if (length_squared(dir) == 0.0f) {
        dir = vec3(1.0f, 0.0f, 0.0f);
    }
    SetRotationFromFacing(dir);
}

void MovementObject::CollideWith(MovementObject* other) {
    ASArglist args;
    args.AddAddress(other);
    as_context->CallScriptFunction(as_funcs.handle_collisions_btc, &args);
}

void MovementObject::SetScriptParams(const ScriptParamMap& spm) {
    Object::SetScriptParams(spm);
}

void MovementObject::UpdateScriptParams() {
    if (as_context.get()) {
        as_context->CallScriptFunction(as_funcs.set_parameters);
    }
}

void MovementObject::ApplyPalette(const OGPalette& _palette, bool from_socket) {
    Online* online = Online::Instance();
    palette = _palette;

    if (online->IsActive() && !from_socket) {
        online->SendAvatarPaletteChange(_palette, GetID());  // TODO Is this something clients should send to the host?
    }

    if (rigged_object_.get() != nullptr) {
        rigged_object_->ApplyPalette(palette);
    }
}

OGPalette* MovementObject::GetPalette() {
    return &palette;
}

void MovementObject::ReceiveObjectMessageVAList(OBJECT_MSG::Type type, va_list args) {
    switch (type) {
        case OBJECT_MSG::QUEUE_SCRIPT: {
            const std::string& str = *va_arg(args, std::string*);
            message_queue.push(str);
            break;
        }
        case OBJECT_MSG::SCRIPT: {
            const std::string& str = *va_arg(args, std::string*);
            ReceiveMessage(str);
            break;
        }
        case OBJECT_MSG::UPDATE_GPU_SKINNING:
            rigged_object_->UpdateGPUSkinning();
            break;
        default:
            Object::ReceiveObjectMessageVAList(type, args);
            break;
    }
}

void MovementObject::HitByItem(int id, const vec3& point, const std::string& material, int type) {
    ASArglist args;
    args.AddObject((void*)&material);
    args.AddObject((void*)&point);
    args.Add(id);
    args.Add(type);
    as_context->CallScriptFunction(as_funcs.hit_by_item, &args);
}

void MovementObject::Execute(std::string string) {
    as_context->Execute(string);
}

bool MovementObject::ASHasVar(std::string name) {
    return as_context->GetVarPtr(name.c_str()) != NULL;
}

int MovementObject::ASGetIntVar(std::string name) {
    return *((int*)as_context->GetVarPtr(name.c_str()));
}

int MovementObject::ASGetArrayIntVar(std::string name, int index) {
    return *((int*)as_context->GetArrayVarPtr(name, index));
}

float MovementObject::ASGetFloatVar(std::string name) {
    return *((float*)as_context->GetVarPtr(name.c_str()));
}

bool MovementObject::ASGetBoolVar(std::string name) {
    return *((bool*)as_context->GetVarPtr(name.c_str()));
}

void MovementObject::ASSetIntVar(std::string name, int value) {
    *(int*)as_context->GetVarPtr(name.c_str()) = value;
}

void MovementObject::ASSetArrayIntVar(std::string name, int index, int value) {
    *(int*)as_context->GetArrayVarPtr(name, index) = value;
}

void MovementObject::ASSetFloatVar(std::string name, float value) {
    *(float*)as_context->GetVarPtr(name.c_str()) = value;
}

void MovementObject::ASSetBoolVar(std::string name, bool value) {
    *(bool*)as_context->GetVarPtr(name.c_str()) = value;
}

void MovementObject::Dispose() {
    as_context->CallScriptFunction(as_funcs.dispose);
    as_context.reset(NULL);
    rigged_object_->as_context = NULL;

    for (auto& occlusion_querie : occlusion_queries) {
        const GLuint val = occlusion_querie.id;
        glDeleteQueries(1, &val);
    }
    occlusion_queries.clear();
}

void MovementObject::AddToAttackHistory(const std::string& str) {
    attack_history.Add(str);
}

float MovementObject::CheckAttackHistory(const std::string& str) {
    return attack_history.Check(str);
}

void MovementObject::ClearAttackHistory() {
    attack_history.Clear();
}

void MovementObject::RemovePhysicsShapes() {
    if (char_sphere) {
        scenegraph_->abstract_bullet_world_->RemoveObject(&char_sphere);
        char_sphere = NULL;
    }
}

bool MovementObject::SetFromDesc(const EntityDescription& desc) {
    bool ret = Object::SetFromDesc(desc);
    if (ret) {
        for (const auto& field : desc.fields) {
            switch (field.type) {
                case EDF_FILE_PATH: {
                    std::string type_file;
                    field.ReadString(&type_file);
                    if (obj_file != type_file) {
                        // ActorFileRef afr = ActorFiles::Instance()->ReturnRef(type_file);
                        ActorFileRef afr = Engine::Instance()->GetAssetManager()->LoadSync<ActorFile>(type_file);
                        if (afr.valid()) {
                            actor_file_ref = afr;
                            actor_script_path = afr->script;
                            character_path = afr->character;
                            obj_file = type_file;
                        } else {
                            LOGE << "Failed at loading ActorFile \"" << type_file << "\" for MovementObject." << std::endl;
                            ret = false;
                        }
                    }
                    break;
                }
                case EDF_IS_PLAYER: {
                    int read_is_player;
                    memread(&read_is_player, sizeof(int), 1, field.data);
                    is_player = (read_is_player != 0);
                    break;
                }
                case EDF_CONNECTIONS: {
                    std::vector<int> connections;
                    field.ReadIntVec(&connections);
                    if (!connections.empty()) {
                        connected_pathpoint_id = connections[0];
                    } else {
                        connected_pathpoint_id = -1;
                    }
                    break;
                }
                case EDF_ITEM_CONNECTIONS: {
                    std::vector<ItemConnectionData> item_connections;
                    field.ReadItemConnectionDataVec(&item_connections);
                    for (auto& i : item_connections) {
                        if (i.id != -1) {
                            ItemConnection item_connection;
                            item_connection.id = i.id;
                            item_connection.mirrored = i.mirrored != 0;
                            item_connection.attachment_type = (AttachmentType)i.attachment_type;
                            if (!i.attachment_str.empty()) {
                                item_connection.attachment_ref = Engine::Instance()->GetAssetManager()->LoadSync<Attachment>(i.attachment_str);
                                if (item_connection.attachment_ref.valid() == false) {
                                    LOGE << "Unable to load attachment " << i.attachment_str << std::endl;
                                }
                            }
                            item_connection_vec.push_back(item_connection);
                        } else {
                            LOGE << "movementobject " << *this << " has item connection with id -1, skipping." << std::endl;
                        }
                    }
                    break;
                }
                case EDF_PALETTE: {
                    OGPalette palette;
                    ReadPaletteFromRAM(palette, field.data);
                    ApplyPalette(palette);
                    break;
                }
                case EDF_ENV_OBJECT_ATTACH: {
                    env_object_attach_data = field.data;
                    break;
                }
                case EDF_NPC_SCRIPT_PATH: {
                    field.ReadString(&object_npc_script_path);
                    break;
                }
                case EDF_PC_SCRIPT_PATH: {
                    field.ReadString(&object_pc_script_path);
                    break;
                }
            }
        }

        if (!env_object_attach_data.empty()) {
            std::vector<AttachedEnvObject> attached_env_objects;
            Deserialize(env_object_attach_data, attached_env_objects);
            for (auto& attached_env_object : attached_env_objects) {
                attached_env_object.direct_ptr = NULL;
            }
            for (int i = 0, len = desc.children.size(); i < len; ++i) {
                EntityDescription new_desc = desc.children[i];
                Object* obj = CreateObjectFromDesc(new_desc);
                if (obj) {
                    attached_env_objects[i].direct_ptr = obj;
                    obj->SetParent(this);
                } else {
                    attached_env_objects[i].direct_ptr = NULL;
                    LOGE << "Failed at creating object for movementobject attachment" << std::endl;
                }
            }
            Serialize(attached_env_objects, env_object_attach_data);
        }
    }
    return ret;
}

void MovementObject::ChildLost(Object* obj) {
    Online* online = Online::Instance();

    if (rigged_object_.get()) {
        for (int i = 0, len = rigged_object_->children.size(); i < len; ++i) {
            if (rigged_object_->children[i].direct_ptr == obj) {
                rigged_object_->children.erase(rigged_object_->children.begin() + i);
                if (online->IsActive()) {
                    online->Send<OnlineMessages::AttachToMessage>(GetID(), obj->GetID(), 0, false, false);
                }

                break;
            }
        }
        Serialize(rigged_object_->children, env_object_attach_data);
    }
}

RiggedObject* MovementObject::rigged_object() {
    return rigged_object_.get();
}

MovementObject::~MovementObject() {
    RemovePhysicsShapes();
    while (!item_connections.empty()) {
        ItemObject* obj = item_connections.front().obj;
        Disconnect(*obj);
        obj->ActivatePhysics();
    }
}

const float kAttackTransitionTime = 2.0f;

void AttackHistory::Add(const std::string& str) {
    AttackHistoryEntry entry;
    entry.attack_script_getter.Load(str);
    entry.time = game_timer.game_time;
    if (!entries.empty() && entries.back().time > entry.time - kAttackTransitionTime) {
        const float kTransitionDecaySubtract = 0.05f;
        const float kTransitionDecayMultiply = 0.9f;
        OuterTransitionMap::iterator iter = transitions.begin();
        while (iter != transitions.end()) {
            InnerTransitionMap& itm = iter->second;
            InnerTransitionMap::iterator iter2 = itm.begin();
            while (iter2 != itm.end()) {
                iter2->second *= kTransitionDecayMultiply;
                iter2->second -= kTransitionDecaySubtract;
                if (iter2->second <= 0.0f) {
                    itm.erase(iter2++);
                } else {
                    ++iter2;
                }
            }
            if (itm.empty()) {
                transitions.erase(iter++);
            } else {
                ++iter;
            }
        }

        const AttackHistoryEntry& last_entry = entries.back();
        unsigned long hash = djb2_string_hash(last_entry.attack_script_getter.attack_ref()->path_.c_str());
        transitions[hash][entry.attack_script_getter.attack_ref()] += 1.0f;
    }
    entries.clear();
    entries.push_back(entry);
}

void GetAttackInfo(const AttackScriptGetter& asg, AttackHeight* ah, AttackDirection* ad, std::string* name) {
    const AttackRef& attack_ref = asg.attack_ref();
    bool mirror = asg.mirror();
    *ah = attack_ref->height;
    *ad = attack_ref->direction;
    *name = attack_ref->path_;
    if (mirror) {
        switch (*ad) {
            case _right:
                *ad = _left;
                break;
            case _left:
                *ad = _right;
                break;
            default:
                break;
        }
    }
}

float AttackHistory::Check(const std::string& str) {
    if (entries.empty()) {
        return 0.5f;
    }
    float cur_time = game_timer.game_time;
    AttackScriptGetter attack_script_getter;
    attack_script_getter.Load(str);
    AttackHeight attack_height;
    AttackDirection attack_direction;
    std::string name;
    GetAttackInfo(attack_script_getter, &attack_height, &attack_direction, &name);

    const AttackHistoryEntry& entry = entries.back();
    const AttackScriptGetter& old_attack_script_getter = entry.attack_script_getter;
    AttackHeight old_attack_height;
    AttackDirection old_attack_direction;
    std::string old_name;
    GetAttackInfo(old_attack_script_getter, &old_attack_height, &old_attack_direction, &old_name);

    float similarity = 0.5f;
    if (old_attack_height == attack_height) {
        similarity += 0.15f;
    } else if ((old_attack_height == _low && attack_height == _high) ||
               (old_attack_height == _high && attack_height == _low)) {
        similarity -= 0.15f;
    }
    if (old_attack_direction == attack_direction) {
        similarity += 0.15f;
    } else if ((old_attack_direction == _left && attack_direction == _right) ||
               (old_attack_direction == _right && attack_direction == _left)) {
        similarity -= 0.15f;
    }
    if (old_name == name) {
        similarity += 0.15f;
    } else {
        similarity -= 0.15f;
    }

    unsigned long hash = djb2_string_hash(old_name.c_str());
    const InnerTransitionMap& itm = transitions[hash];
    InnerTransitionMap::const_iterator iter = itm.find(attack_script_getter.attack_ref());
    float transition_probability = 0.0f;
    if (entry.time > cur_time - kAttackTransitionTime) {
        float num = 0.0f;
        if (iter != itm.end()) {
            num = iter->second;
        }
        if (num != 0) {
            float total = 0;
            for (const auto& iter : itm) {
                total += iter.second;
            }
            transition_probability = num / total;
        }
    }
    similarity += (1.0f - similarity) * transition_probability;
    return similarity;
}

void AttackHistory::Clear() {
    entries.clear();
    transitions.clear();
}
