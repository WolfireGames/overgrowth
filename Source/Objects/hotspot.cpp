//-----------------------------------------------------------------------------
//           Name: hotspot.cpp
//      Developer: Wolfire Games LLC
//         Author: Phillip Isola
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
#include "hotspot.h"

#include <Graphics/graphics.h>
#include <Graphics/shaders.h>
#include <Graphics/pxdebugdraw.h>

#include <Internal/filesystem.h>
#include <Internal/memwrite.h>
#include <Internal/common.h>
#include <Internal/profiler.h>

#include <Scripting/angelscript/asfuncs.h>
#include <Scripting/angelscript/add_on/scripthandle/scripthandle.h>
#include <Scripting/angelscript/add_on/scriptarray/scriptarray.h>

#include <Math/vec3math.h>
#include <Math/vec4math.h>

#include <Game/level.h>
#include <Game/savefile_script.h>

#include <Main/scenegraph.h>
#include <Objects/hotspot.h>
#include <Physics/bulletworld.h>
#include <Game/EntityDescription.h>
#include <Editors/map_editor.h>
#include <Logging/logdata.h>
#include <Asset/Asset/hotspotfile.h>

#include <imgui.h>

extern std::string script_dir_path;
extern std::string preview_script_name;
extern bool show_script;

extern bool g_debug_runtime_disable_hotspot_draw;
extern bool g_debug_runtime_disable_hotspot_pre_draw_frame;

Hotspot::Hotspot() : as_context(NULL),
                     collision_object(NULL),
                     abstract_collision(true) {
    collidable = false;
    box_.dims = vec3(4.0f);
}

Hotspot::~Hotspot() {
    if (as_context) {
        delete as_context;
        as_context = NULL;
    }
    if (collision_object) {
        scenegraph_->abstract_bullet_world_->RemoveObject(&collision_object);
        collision_object = NULL;
    }
}

void Hotspot::UpdateCollisionShape() {
    if (collision_object) {
        scenegraph_->abstract_bullet_world_->RemoveObject(&collision_object);
        collision_object = NULL;
    }
    if (abstract_collision) {
        vec3 scale = GetScale();
        scale[0] = fabs(scale[0]);
        scale[1] = fabs(scale[1]);
        scale[2] = fabs(scale[2]);
        collision_object = scenegraph_->abstract_bullet_world_->CreateBox(
            vec3(0.0f), scale * 4.0f, BW_STATIC);
        // mat4 mat = m_transform.blGetMatrix();
        // mat4 no_scale_mat = mat.GetRotationPart();
        // no_scale_mat.SetTranslationPart(GetCenter());
        // col_sphere->SetTransform(no_scale_mat);
        collision_object->SetTransform(GetTranslation(), Mat4FromQuaternion(GetRotation()), vec3(1.0f));  // GetScale());
        collision_object->body->setInterpolationWorldTransform(collision_object->body->getWorldTransform());
        collision_object->body->setCollisionFlags(collision_object->body->getCollisionFlags() |
                                                  btCollisionObject::CF_NO_CONTACT_RESPONSE);
        collision_object->SetVisibility(true);
        collision_object->FixDiscontinuity();
        collision_object->Sleep();
        scenegraph_->abstract_bullet_world_->dynamics_world_->updateSingleAabb(collision_object->body);
        collision_object->owner_object = this;
    }
}

void Hotspot::Dispose() {
    as_context->CallScriptFunction(as_funcs.dispose);
}

void Hotspot::GetDisplayName(char* buf, int buf_size) {
    if (GetName().empty()) {
        FormatString(buf, buf_size, "%d: Hotspot: %s", GetID(), obj_file.c_str());
    } else {
        FormatString(buf, buf_size, "%s: Hotspot: %s", GetName().c_str(), obj_file.c_str());
    }
}

void Hotspot::DrawImGuiEditor() {
    ImGui::Text("Script: %s", m_script_file.c_str());
    ImGui::SameLine();
    if (ImGui::Button("Preview script")) {
        show_script = preview_script_name == m_script_file ? !show_script : true;
        preview_script_name = m_script_file;
    }
}

bool Hotspot::Initialize() {
    UpdateCollisionShape();

    std::string script_file_path = script_dir_path + m_script_file;
    if (!m_script_file.empty() && FileExists(script_file_path.c_str(), kDataPaths | kModPaths)) {
        ASData as_data;
        as_data.scenegraph = scenegraph_;
        as_data.gui = scenegraph_->map_editor->gui;
        as_context = new ASContext("hotspot", as_data);
        AttachUIQueries(as_context);
        AttachActiveCamera(as_context);
        AttachScreenWidth(as_context);
        AttachPhysics(as_context);
        AttachEngine(as_context);
        AttachScenegraph(as_context, scenegraph_);
        AttachTextCanvasTextureToASContext(as_context);
        AttachLevel(as_context);
        AttachInterlevelData(as_context);
        AttachPlaceholderObject(as_context);
        AttachTokenIterator(as_context);
        AttachStringConvert(as_context);
        AttachMessages(as_context);
        AttachSaveFile(as_context, &Engine::Instance()->save_file_);
        AttachIMGUI(as_context);
        AttachOnline(as_context);
        hud_images.AttachToContext(as_context);

        as_collisions.reset(new ASCollisions(scenegraph_));
        as_collisions->AttachToContext(as_context);

        ScriptParams::RegisterScriptType(as_context);
        sp.RegisterScriptInstance(as_context);

        as_context->RegisterGlobalProperty("Hotspot hotspot", this);

        as_funcs.init = as_context->RegisterExpectedFunction("void Init()", false);
        as_funcs.set_parameters = as_context->RegisterExpectedFunction("void SetParameters()", false);
        as_funcs.receive_message = as_context->RegisterExpectedFunction("void ReceiveMessage(string)", false);
        as_funcs.update = as_context->RegisterExpectedFunction("void Update()", false);
        as_funcs.pre_draw = as_context->RegisterExpectedFunction("void PreDraw(float curr_game_time)", false);
        as_funcs.draw = as_context->RegisterExpectedFunction("void Draw()", false);
        as_funcs.draw_editor = as_context->RegisterExpectedFunction("void DrawEditor()", false);
        as_funcs.reset = as_context->RegisterExpectedFunction("void Reset()", false);
        as_funcs.set_enabled = as_context->RegisterExpectedFunction("void SetEnabled(bool val)", false);
        as_funcs.handle_event = as_context->RegisterExpectedFunction("void HandleEvent(string event, MovementObject @mo)", false);
        as_funcs.handle_event_item = as_context->RegisterExpectedFunction("void HandleEventItem(string event, ItemObject @obj)", false);
        as_funcs.get_type_string = as_context->RegisterExpectedFunction("string GetTypeString()", false);
        as_funcs.dispose = as_context->RegisterExpectedFunction("void Dispose()", false);
        as_funcs.pre_script_reload = as_context->RegisterExpectedFunction("void PreScriptReload()", false);
        as_funcs.post_script_reload = as_context->RegisterExpectedFunction("void PostScriptReload()", false);
        as_funcs.connect_to = as_context->RegisterExpectedFunction("bool ConnectTo(Object @other)", false);
        as_funcs.disconnect = as_context->RegisterExpectedFunction("bool Disconnect(Object @other)", false);
        as_funcs.connected_from = as_context->RegisterExpectedFunction("void ConnectedFrom(Object @other)", false);
        as_funcs.disconnected_from = as_context->RegisterExpectedFunction("void DisconnectedFrom(Object @other)", false);
        as_funcs.accept_connections_from = as_context->RegisterExpectedFunction("bool AcceptConnectionsFrom(ConnectionType type)", false);
        as_funcs.accept_connections_from_obj = as_context->RegisterExpectedFunction("bool AcceptConnectionsFrom(Object @other)", false);
        as_funcs.accept_connections_to_obj = as_context->RegisterExpectedFunction("bool AcceptConnectionsTo(Object @other)", false);
        as_funcs.launch_custom_gui = as_context->RegisterExpectedFunction("void LaunchCustomGUI()", false);
        as_funcs.object_inspector_read_only = as_context->RegisterExpectedFunction("bool ObjectInspectorReadOnly()", false);

        PROFILER_ENTER(g_profiler_ctx, "Exporting docs");
        char path[kPathSize];
        FormatString(path, kPathSize, "%sashotspot_docs.h", GetWritePath(CoreGameModID).c_str());
        as_context->ExportDocs(path);
        PROFILER_LEAVE(g_profiler_ctx);

        Path script_path = FindFilePath(script_file_path, kDataPaths | kModPaths);
        as_context->LoadScript(script_path);
        as_context->CallScriptFunction(as_funcs.init);
        as_context->CallScriptFunction(as_funcs.set_parameters);
        return true;
    } else {
        LOGE << "Hotspot script \"" << script_file_path << "\" not found" << std::endl;
        return false;
    }
}

void Hotspot::ReceiveObjectMessageVAList(OBJECT_MSG::Type type, va_list args) {
    switch (type) {
        case OBJECT_MSG::SCRIPT: {
            const std::string& str = *va_arg(args, std::string*);
            ASArglist args;
            args.AddObject((void*)&str);
            as_context->CallScriptFunction(as_funcs.receive_message, &args);
            break;
        }
        case OBJECT_MSG::QUEUE_SCRIPT: {
            const std::string& str = *va_arg(args, std::string*);
            message_queue.push(str);
            break;
        }
        case OBJECT_MSG::RESET: {
            Reset();
            std::string str = "reset";
            ReceiveObjectMessage(OBJECT_MSG::SCRIPT, &str);
            break;
        }
        default:
            Object::ReceiveObjectMessageVAList(type, args);
            break;
    }
}

void Hotspot::Update(float timestep) {
    if (as_context) {
        while (message_queue.empty() == false) {
            ASArglist args;
            const std::string str = message_queue.front();
            args.AddObject((void*)&str);
            as_context->CallScriptFunction(as_funcs.receive_message, &args);
            message_queue.pop();
        }
        as_context->CallScriptFunction(as_funcs.update);
    }
}

void Hotspot::PreDrawFrame(float curr_game_time) {
    if (g_debug_runtime_disable_hotspot_pre_draw_frame) {
        return;
    }

    PROFILER_ZONE(g_profiler_ctx, "Hotspot::PreDrawFrame");
    ASArglist args;
    args.Add(curr_game_time);
    as_context->CallScriptFunction(as_funcs.pre_draw, &args);
}

void Hotspot::Draw() {
    if (g_debug_runtime_disable_hotspot_draw) {
        return;
    }

    as_context->CallScriptFunction(as_funcs.draw);
    if (scenegraph_->map_editor->state_ != MapEditor::kInGame) {
        as_context->CallScriptFunction(as_funcs.draw_editor);
    }
    if (!Graphics::Instance()->media_mode() && billboard_texture_ref_.valid()) {
        DrawBillboard(billboard_texture_ref_->GetTextureRef(), GetTranslation() + GetScale() * vec3(0.0f, 0.5f, 0.0f), 2.0f, vec4(1.0f), kStraight);
    }
    hud_images.Draw();
}

void Hotspot::Reset() {
    if (as_context) {
        as_context->CallScriptFunction(as_funcs.reset);
    }
}

void Hotspot::SetEnabled(bool val) {
    Object::SetEnabled(val);
    PROFILER_ZONE(g_profiler_ctx, "Hotspot::SetEnabled");
    ASArglist args;
    args.Add(val);
    as_context->CallScriptFunction(as_funcs.set_enabled, &args);
}

int Hotspot::lineCheck(const vec3& start, const vec3& end, vec3* point, vec3* normal) {
    return LineCheckEditorCube(start, end, point, normal);
}

void Hotspot::Moved(Object::MoveType type) {
    Object::Moved(type);
    if (collision_object) {
        UpdateCollisionShape();
    }
}

void Hotspot::GetDesc(EntityDescription& desc) const {
    Object::GetDesc(desc);
    desc.AddString(EDF_FILE_PATH, obj_file);
    desc.AddIntVec(EDF_CONNECTIONS, connected_to);
    desc.AddIntVec(EDF_CONNECTIONS_FROM, connected_from);
}

// Just handle everything in hotspots

void Hotspot::NotifyDeleted(Object* o) {
    std::vector<int>::iterator iter = std::find(connected_to.begin(), connected_to.end(), o->GetID());
    if (iter != connected_to.end()) {
        Disconnect(*scenegraph_->GetObjectFromID(*iter));
    }
    Object::NotifyDeleted(o);
}

void Hotspot::HandleEvent(const std::string& event, MovementObject* mo) {
    if (!as_context) {
        return;
    }
    ASArglist args;
    args.AddObject((void*)&event);
    args.AddObject(mo);
    as_context->CallScriptFunction(as_funcs.handle_event, &args);
}

void Hotspot::HandleEventItem(const std::string& event, ItemObject* obj) {
    if (!as_context) {
        return;
    }
    ASArglist args;
    args.AddObject((void*)&event);
    args.AddObject(obj);
    as_context->CallScriptFunction(as_funcs.handle_event_item, &args);
}

void Hotspot::SetScriptParams(const ScriptParamMap& spm) {
    Object::SetScriptParams(spm);
}

void Hotspot::UpdateScriptParams() {
    if (as_context) {
        as_context->CallScriptFunction(as_funcs.set_parameters);
    }
}

std::string Hotspot::GetTypeString() {
    if (!as_context) {
        return "unknown - no script";
    }
    ASArglist args;
    ASArg ret_val;
    ret_val.type = _as_object;
    bool success = as_context->CallScriptFunction(as_funcs.get_type_string, &args, &ret_val);
    if (success) {
        return *((std::string*)ret_val.data);
    } else {
        return "unknown - hotspot has no 'GetTypeString()' function";
    }
}

bool Hotspot::ASHasVar(const std::string& name) {
    return as_context->GetVarPtr(name.c_str()) != NULL;
}

int Hotspot::ASGetIntVar(const std::string& name) {
    return *((int*)as_context->GetVarPtr(name.c_str()));
}

int Hotspot::ASGetArrayIntVar(const std::string& name, int index) {
    return *((int*)as_context->GetArrayVarPtr(name, index));
}

float Hotspot::ASGetFloatVar(const std::string& name) {
    return *((float*)as_context->GetVarPtr(name.c_str()));
}

bool Hotspot::ASGetBoolVar(const std::string& name) {
    return *((bool*)as_context->GetVarPtr(name.c_str()));
}

void Hotspot::ASSetIntVar(const std::string& name, int value) {
    *(int*)as_context->GetVarPtr(name.c_str()) = value;
}

void Hotspot::ASSetArrayIntVar(const std::string& name, int index, int value) {
    *(int*)as_context->GetArrayVarPtr(name, index) = value;
}

void Hotspot::ASSetFloatVar(const std::string& name, float value) {
    *(float*)as_context->GetVarPtr(name.c_str()) = value;
}

void Hotspot::ASSetBoolVar(const std::string& name, bool value) {
    *(bool*)as_context->GetVarPtr(name.c_str()) = value;
}

bool Hotspot::SetFromDesc(const EntityDescription& desc) {
    bool ret = Object::SetFromDesc(desc);
    if (ret) {
        for (const auto& field : desc.fields) {
            switch (field.type) {
                case EDF_FILE_PATH: {
                    std::string type_file;
                    field.ReadString(&type_file);
                    if (obj_file != type_file) {
                        obj_file = type_file;
                        // HotspotFileRef hfr = HotspotFiles::Instance()->ReturnRef(type_file);
                        HotspotFileRef hfr = Engine::Instance()->GetAssetManager()->LoadSync<HotspotFile>(type_file);
                        if (hfr.valid()) {
                            if (hfr->billboard_color_map != "Data/UI/spawner/thumbs/Hotspot/empty.png") {
                                SetBillboardColorMap(hfr->billboard_color_map);
                            }
                            SetScriptFile(hfr->script);
                        } else {
                            ret = false;
                        }
                    }
                    break;
                }
                case EDF_CONNECTIONS: {
                    field.ReadIntVec(&unfinalized_connected_to);
                    break;
                }
                case EDF_CONNECTIONS_FROM: {
                    field.ReadIntVec(&unfinalized_connected_from);
                    break;
                }
            }
        }
    }
    return ret;
}

void Hotspot::SetBillboardColorMap(const std::string& color_map) {
    billboard_texture_ref_ = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(color_map, PX_SRGB, 0x0);
    if (billboard_texture_ref_.valid() == false) {
        LOGE << "Failed to load Hotspot Billboard texture: " << color_map << " for " << this << std::endl;
    }
}

static vec3 GetWind(vec3 check_where, float curr_game_time, float change_rate) {
    vec3 wind_vel;
    check_where[0] += curr_game_time * 0.7f * change_rate;
    check_where[1] += curr_game_time * 0.3f * change_rate;
    check_where[2] += curr_game_time * 0.5f * change_rate;
    wind_vel[0] = sin(check_where[0]) + cos(check_where[1] * 1.3f) + sin(check_where[2] * 3.0f);
    wind_vel[1] = sin(check_where[0] * 1.2f) + cos(check_where[1] * 1.8f) + sin(check_where[2] * 0.8f);
    wind_vel[2] = sin(check_where[0] * 1.6f) + cos(check_where[1] * 0.5f) + sin(check_where[2] * 1.2f);

    return wind_vel;
}

void Hotspot::Reload() {
    if (as_context) {
        as_context->CallScriptFunction(as_funcs.pre_script_reload);
        as_context->Reload();
        as_context->CallScriptFunction(as_funcs.post_script_reload);
    }
}

static void CFireRibbonUpdate(asIScriptObject* obj, float delta_time, float curr_game_time, const vec3& fire_pos) {
    CScriptArray& particles = *(CScriptArray*)obj->GetAddressOfProperty(0);
    vec3& rel_pos = *(vec3*)obj->GetAddressOfProperty(1);
    vec3& pos = *(vec3*)obj->GetAddressOfProperty(2);
    float& base_rand = *(float*)obj->GetAddressOfProperty(3);
    float& spawn_new_particle_delay = *(float*)obj->GetAddressOfProperty(4);

    spawn_new_particle_delay -= delta_time;
    if (spawn_new_particle_delay <= 0.0f) {
        particles.Resize(particles.GetSize() + 1);
        asIScriptObject* particle = (asIScriptObject*)particles.At(particles.GetSize() - 1);
        *(vec3*)particle->GetAddressOfProperty(0) = pos;
        *(vec3*)particle->GetAddressOfProperty(1) = vec3(0.0, 0.0, 0.0);
        *(float*)particle->GetAddressOfProperty(2) = RangedRandomFloat(0.5, 1.5);
        *(float*)particle->GetAddressOfProperty(3) = curr_game_time;

        while (spawn_new_particle_delay <= 0.0f) {
            spawn_new_particle_delay += 0.1f;
        }
    }

    int max_particles = 5;
    if ((int)particles.GetSize() > max_particles) {
        for (int i = 0; i < max_particles; ++i) {
            asIScriptObject* dst_particle = (asIScriptObject*)particles.At(i);
            asIScriptObject* src_particle = (asIScriptObject*)particles.At(particles.GetSize() - max_particles + i);
            dst_particle->CopyFrom(src_particle);
        }
        particles.Resize(max_particles);
    }
    for (int i = 0, len = (int)particles.GetSize(); i < len; ++i) {
        asIScriptObject* particle = (asIScriptObject*)particles.At(i);
        vec3& particle_pos = *(vec3*)particle->GetAddressOfProperty(0);
        vec3& particle_vel = *(vec3*)particle->GetAddressOfProperty(1);
        float& particle_heat = *(float*)particle->GetAddressOfProperty(2);
        particle_vel *= pow(0.2f, delta_time);
        particle_pos += particle_vel * delta_time;
        particle_vel += GetWind(particle_pos * 5.0f, curr_game_time, 10.0f) * delta_time * 1.0f;
        particle_vel += GetWind(particle_pos * 30.0f, curr_game_time, 10.0f) * delta_time * 2.0f;
        vec3 rel = particle_pos - fire_pos;
        rel[1] = 0.0;
        particle_heat -= delta_time * (2.0f + min(1.0f, powf(dot(rel, rel), 2.0) * 64.0f)) * 2.0f;
        if (dot(rel, rel) > 1.0) {
            rel = normalize(rel);
        }

        particle_vel += rel * delta_time * -3.0f * 6.0f;
        particle_vel[1] += delta_time * 12.0f;
    }
}

void CFireRibbonPreDraw(asIScriptObject* obj, float curr_game_time) {
    CScriptArray& particles = *(CScriptArray*)obj->GetAddressOfProperty(0);
    float& base_rand = *(float*)obj->GetAddressOfProperty(3);

    DebugDraw* debug_draw = DebugDraw::Instance();
    int ribbon_id = debug_draw->AddRibbon(_delete_on_draw);
    DebugDrawElement* element = DebugDraw::Instance()->GetElement(ribbon_id);
    DebugDrawRibbon* ribbon = (DebugDrawRibbon*)element;
    const float flame_width = 0.12f;
    for (int i = 0, len = particles.GetSize(); i < len; ++i) {
        asIScriptObject* particle = (asIScriptObject*)particles.At(i);
        vec3& particle_pos = *(vec3*)particle->GetAddressOfProperty(0);
        vec3& particle_vel = *(vec3*)particle->GetAddressOfProperty(1);
        float& particle_heat = *(float*)particle->GetAddressOfProperty(2);
        float& particle_spawn_time = *(float*)particle->GetAddressOfProperty(3);
        ribbon->AddPoint(particle_pos, vec4(particle_heat, particle_spawn_time + base_rand, curr_game_time + base_rand, 0.0), flame_width);
    }
}

static void ASSetCollisionEnabled(Hotspot* hotspot, bool val) {
    if (val != hotspot->abstract_collision) {
        hotspot->abstract_collision = val;
        hotspot->UpdateCollisionShape();
    }
}

bool Hotspot::AcceptConnectionsFromImplemented() {
    if (!as_context)
        return false;
    return as_context->HasFunction(as_funcs.accept_connections_from_obj) || as_context->HasFunction(as_funcs.accept_connections_from);
}

bool Hotspot::AcceptConnectionsFrom(Object::ConnectionType type, Object& other) {
    bool return_value = false;
    if (as_context) {
        ASArglist args;

        ASArg ret_val;
        ret_val.type = _as_bool;
        ret_val.data = &return_value;
        args.AddObject(&other);
        if (!as_context->CallScriptFunction(as_funcs.accept_connections_from_obj, &args, &ret_val)) {
            args.clear();
            args.Add((int)type);
            as_context->CallScriptFunction(as_funcs.accept_connections_from, &args, &ret_val);
        }
    }

    return return_value;
}

bool Hotspot::AcceptConnectionsTo(Object& other) {
    bool return_value = false;
    if (as_context) {
        ASArglist args;

        ASArg ret_val;
        ret_val.type = _as_bool;
        ret_val.data = &return_value;
        args.AddObject(&other);
        as_context->CallScriptFunction(as_funcs.accept_connections_to_obj, &args, &ret_val);
    }

    return return_value;
}

bool Hotspot::ConnectTo(Object& other, bool checking_other /*= false*/) {
    if (std::find(connected_to.begin(), connected_to.end(), other.GetID()) != connected_to.end())
        return false;

    bool return_value = false;
    if (as_context) {
        ASArglist args;
        args.AddObject(&other);

        ASArg ret_val;
        ret_val.type = _as_bool;
        ret_val.data = &return_value;

        as_context->CallScriptFunction(as_funcs.connect_to, &args, &ret_val);
    }

    if (return_value) {
        connected_to.push_back(other.GetID());
        other.ConnectedFrom(*this);
    }

    return return_value;
}

bool Hotspot::Disconnect(Object& other, bool checking_other /* = false*/) {
    if (std::find(connected_to.begin(), connected_to.end(), other.GetID()) == connected_to.end())
        return false;

    bool return_value = false;
    if (as_context) {
        ASArglist args;
        args.AddObject(&other);

        ASArg ret_val;
        ret_val.type = _as_bool;
        ret_val.data = &return_value;

        as_context->CallScriptFunction(as_funcs.disconnect, &args, &ret_val);
    }

    if (return_value) {
        connected_to.erase(std::remove(connected_to.begin(), connected_to.end(), other.GetID()), connected_to.end());
        other.DisconnectedFrom(*this);
    }

    return return_value;
}

void Hotspot::ConnectedFrom(Object& other) {
    if (as_context) {
        ASArglist args;
        args.AddObject(&other);
        as_context->CallScriptFunction(as_funcs.connected_from, &args);
    }
    connected_from.push_back(other.GetID());
}

void Hotspot::DisconnectedFrom(Object& other) {
    if (as_context) {
        ASArglist args;
        args.AddObject(&other);
        as_context->CallScriptFunction(as_funcs.disconnected_from, &args);
    }
    connected_from.erase(std::remove(connected_from.begin(), connected_from.end(), other.GetID()), connected_from.end());
}

CScriptArray* Hotspot::ASGetConnectedObjects() {
    asIScriptEngine* engine = as_context->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoByDecl("array<int>");
    CScriptArray* array = CScriptArray::Create(arrayType);

    array->Reserve(connected_to.size());
    for (int& i : connected_to) {
        array->InsertLast(&i);
    }
    return array;
}

bool Hotspot::HasCustomGUI() {
    return as_context && as_context->HasFunction(as_funcs.launch_custom_gui);
}

void Hotspot::LaunchCustomGUI() {
    if (as_context) {
        as_context->CallScriptFunction(as_funcs.launch_custom_gui);
    }
}

bool Hotspot::ObjectInspectorReadOnly() {
    bool return_value = false;
    if (as_context) {
        ASArg ret_val;
        ret_val.type = _as_bool;
        ret_val.data = &return_value;
        as_context->CallScriptFunction(as_funcs.object_inspector_read_only, NULL, &ret_val);
    }
    return return_value;
}

bool Hotspot::HasFunction(const std::string& function_definition) {
    bool result = false;

    if (as_context) {
        result = as_context->HasFunction(function_definition);
    }

    return result;
}

int Hotspot::QueryIntFunction(const std::string& function) {
    if (as_context) {
        ASArglist args;
        int val;
        ASArg return_val;
        return_val.type = _as_int;
        return_val.data = &val;
        as_context->CallScriptFunction(function, &args, &return_val);
        return val;
    } else {
        return -1;
    }
}

bool Hotspot::QueryBoolFunction(const std::string& function) {
    if (as_context) {
        ASArglist args;
        bool val;
        ASArg return_val;
        return_val.type = _as_bool;
        return_val.data = &val;
        as_context->CallScriptFunction(function, &args, &return_val);
        return val;
    } else {
        return false;
    }
}

float Hotspot::QueryFloatFunction(const std::string& function) {
    if (as_context) {
        ASArglist args;
        float val;
        ASArg return_val;
        return_val.type = _as_float;
        return_val.data = &val;
        as_context->CallScriptFunction(function, &args, &return_val);
        return val;
    } else {
        return -1.0f;
    }
}

std::string Hotspot::QueryStringFunction(const std::string& function) {
    if (as_context) {
        ASArglist args;
        std::string val;
        ASArg return_val;
        return_val.type = _as_string;
        as_context->CallScriptFunction(function, &args, &return_val);
        return return_val.strData;
    } else {
        return "";
    }
}

static bool ConnectTo(Hotspot* obj, Object* other) {
    if (!other)
        return false;
    return obj->ConnectTo(*other);
}
static bool Disconnect(Hotspot* obj, Object* other) {
    if (!other)
        return false;
    return obj->Disconnect(*other);
}

void DefineHotspotTypePublic(ASContext* as_context) {
    as_context->RegisterObjectType("Hotspot", 0, asOBJ_REF | asOBJ_NOCOUNT);
    as_context->GetEngine()->RegisterInterface("C_ACCEL");
    as_context->RegisterObjectMethod("Hotspot",
                                     "int GetID()",
                                     asMETHOD(Hotspot, GetID), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Hotspot",
                                     "string GetTypeString()",
                                     asMETHOD(Hotspot, GetTypeString), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Hotspot",
                                     "bool HasVar(string& in name)",
                                     asMETHOD(Hotspot, ASHasVar), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Hotspot",
                                     "bool GetBoolVar(string& in name)",
                                     asMETHOD(Hotspot, ASGetBoolVar), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Hotspot",
                                     "int GetIntVar(string& in name)",
                                     asMETHOD(Hotspot, ASGetIntVar), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Hotspot",
                                     "int GetArrayIntVar(const string &in name, int index)",
                                     asMETHOD(Hotspot, ASGetArrayIntVar), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Hotspot",
                                     "float GetFloatVar(string& in name)",
                                     asMETHOD(Hotspot, ASGetFloatVar), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Hotspot",
                                     "void SetIntVar(string& in name, int value)",
                                     asMETHOD(Hotspot, ASSetIntVar), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Hotspot",
                                     "void SetArrayIntVar(const string &in name, int index, int value)",
                                     asMETHOD(Hotspot, ASSetArrayIntVar), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Hotspot",
                                     "void SetFloatVar(string& in name, float value)",
                                     asMETHOD(Hotspot, ASSetFloatVar), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Hotspot",
                                     "void SetBoolVar(string& in name, bool value)",
                                     asMETHOD(Hotspot, ASSetBoolVar), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Hotspot",
                                     "array<int> @GetConnectedObjects()",
                                     asMETHOD(Hotspot, ASGetConnectedObjects), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Hotspot",
                                     "bool HasFunction(const string &in function_definition)",
                                     asMETHOD(Hotspot, HasFunction), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Hotspot",
                                     "int QueryIntFunction(const string &in function)",
                                     asMETHOD(Hotspot, QueryIntFunction), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Hotspot",
                                     "bool QueryBoolFunction(const string &in function)",
                                     asMETHOD(Hotspot, QueryBoolFunction), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Hotspot",
                                     "float QueryFloatFunction(const string &in function)",
                                     asMETHOD(Hotspot, QueryFloatFunction), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Hotspot",
                                     "string QueryStringFunction(const string &in function)",
                                     asMETHOD(Hotspot, QueryStringFunction), asCALL_THISCALL);
    as_context->RegisterObjectMethod("Hotspot",
                                     "void SetCollisionEnabled(bool)",
                                     asFUNCTION(ASSetCollisionEnabled), asCALL_CDECL_OBJFIRST);
    // Inherited from Object
    as_context->RegisterObjectMethod("Hotspot", "bool ConnectTo(Object@)", asFUNCTION(ConnectTo), asCALL_CDECL_OBJFIRST);
    as_context->RegisterObjectMethod("Hotspot", "bool Disconnect(Object@)", asFUNCTION(Disconnect), asCALL_CDECL_OBJFIRST);

    as_context->RegisterGlobalFunction("void CFireRibbonUpdate(C_ACCEL @, float delta_time, float curr_game_time, vec3 &in pos)",
                                       asFUNCTION(CFireRibbonUpdate), asCALL_CDECL);
    as_context->RegisterGlobalFunction("void CFireRibbonPreDraw(C_ACCEL @, float curr_game_time)",
                                       asFUNCTION(CFireRibbonPreDraw), asCALL_CDECL);
    as_context->DocsCloseBrace();
}
