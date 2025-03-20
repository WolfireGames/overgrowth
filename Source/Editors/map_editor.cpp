//-----------------------------------------------------------------------------
//           Name: map_editor.cpp
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
#include "map_editor.h"

#include <Editors/sky_editor.h>
#include <Editors/actors_editor.h>

#include <Graphics/shaders.h>
#include <Graphics/sky.h>
#include <Graphics/camera.h>
#include <Graphics/csg.h>
#include <Graphics/graphics.h>
#include <Graphics/Cursor.h>
#include <Graphics/skeleton.h>
#include <Graphics/models.h>
#include <Graphics/pxdebugdraw.h>

#include <Objects/group.h>
#include <Objects/terrainobject.h>
#include <Objects/riggedobject.h>
#include <Objects/pathpointobject.h>
#include <Objects/hotspot.h>
#include <Objects/itemobject.h>
#include <Objects/envobject.h>
#include <Objects/decalobject.h>
#include <Objects/lightvolume.h>
#include <Objects/placeholderobject.h>
#include <Objects/dynamiclightobject.h>
#include <Objects/prefab.h>
#include <Objects/terrainobject.h>
#include <Objects/cameraobject.h>
#include <Objects/movementobject.h>
#include <Objects/envobject.h>
#include <Objects/lightprobeobject.h>
#include <Objects/reflectioncaptureobject.h>

#include <Internal/dialogues.h>
#include <Internal/datemodified.h>
#include <Internal/memwrite.h>

#include <Internal/levelxml.h>
#include <Internal/comma_separated_list.h>
#include <Internal/common.h>
#include <Internal/config.h>
#include <Internal/filesystem.h>
#include <Internal/profiler.h>

extern "C" {
#include <Internal/snprintf.h>
}

#include <Physics/bulletcollision.h>
#include <Physics/bulletworld.h>
#include <Physics/bulletobject.h>

#include <Math/vec3math.h>
#include <Math/vec4math.h>

#include <UserInput/input.h>
#include <UserInput/keycommands.h>

#include <GUI/gui.h>
#include <GUI/dimgui/dimgui.h>

#include <Online/online.h>
#include <Online/online_datastructures.h>

#include <Timing/timingevent.h>
#include <Timing/intel_gl_perf.h>

#include <XML/level_loader.h>
#include <Game/level.h>
#include <Main/scenegraph.h>
#include <Math/enginemath.h>
#include <Utility/assert.h>
#include <Asset/Asset/levelinfo.h>
#include <Logging/logdata.h>

#include <tinyxml.h>
#include <SDL.h>

#include <cmath>

extern bool shadow_cache_dirty;
extern bool g_no_reflection_capture;
bool draw_group_and_prefab_boxes = true;
bool always_draw_hotspot_connections = false;

const char* new_empty_level_path = "Data/Levels/nothing.xml";

using namespace PHOENIX_KEY_CONSTANTS;

bool kLightProbe2pass = false;

static const float kMapEditorMouseRayLength = 1000.0f;  // From how far away can you select something?
static const int kNumUpdatesBeforeSave = 5;
static const int kNumUpdatesBeforeColorSave = 5;

static void GetMouseRay(LineSegment* mouseray) {
    Camera* camera = ActiveCameras::Get();
    mouseray->start = camera->GetPos();
    mouseray->end = camera->GetPos() + normalize(camera->GetMouseRay()) * kMapEditorMouseRayLength;
}

class CollisionCompare {
   public:
    CollisionCompare(const vec3& point) {
        point_ = point;
    }
    bool operator()(const Collision& a, const Collision& b) {
        return distance_squared(a.hit_where, point_) < distance_squared(b.hit_where, point_);
    }

   private:
    vec3 point_;
};

static void GetEditorLineCollisions(SceneGraph* scenegraph, const vec3& start, const vec3& end, std::vector<Collision>& collisions, const TypeEnable& type_enable) {
    PROFILER_ZONE(g_profiler_ctx, "GetEditorLineCollisions");
    scenegraph->LineCheckAll(start, end, &collisions);
    if (collisions.empty()) {
        return;
    }
    std::sort(collisions.begin(), collisions.end(), CollisionCompare(start));

    for (int i = 0; i < (int)collisions.size();) {
        if (collisions[i].hit_what->GetType() == _group ||
            collisions[i].hit_what->GetType() == _prefab ||
            !(collisions[i].hit_what->permission_flags & Object::CAN_SELECT) ||
            !type_enable.IsTypeEnabled(collisions[i].hit_what->GetType())) {
            collisions.erase(collisions.begin() + i);
        } else if (collisions[i].hit_what->parent &&
                   (collisions[i].hit_what->parent->GetType() == _group ||
                    collisions[i].hit_what->parent->GetType() == _prefab)) {
            int depth = 1;
            while (collisions[i].hit_what->parent &&
                   (collisions[i].hit_what->parent->GetType() == _group ||
                    collisions[i].hit_what->parent->GetType() == _prefab)

            ) {
                bool is_duplicate = false;
                for (int j = 0; j < i; ++j) {  // Don't allow same group to be in list twice
                    if (collisions[i].hit_what->parent == collisions[j].hit_what) {
                        is_duplicate = true;
                        break;
                    }
                }
                if (!is_duplicate) {
                    ++depth;
                    collisions.insert(collisions.begin() + i, collisions[i]);
                    collisions[i].hit_what = collisions[i].hit_what->parent;
                } else {
                    break;
                }
            }
            i += depth;
        } else {
            ++i;
        }
    }
}

// Given a set of selected objects, what kind of connections are possible
static Object::ConnectionType GetConnectionType(const std::vector<Object*>& selected) {
    if (selected.empty()) {
        return Object::kCTNone;
    }
    bool movement_objects_only = true;
    bool single_item_object = (selected.size() == 1);
    bool env_objects_only = true;
    bool path_points_only = true;
    bool navmesh_connection_objects_only = true;
    bool single_placeholder_object = (selected.size() == 1);
    bool hotspots_only = true;
    for (auto obj : selected) {
        EntityType type = obj->GetType();
        if (type != _movement_object) {
            movement_objects_only = false;
        }
        if (type != _item_object) {
            single_item_object = false;
        }
        if (type != _env_object && type != _group && type != _prefab) {
            env_objects_only = false;
        }
        if (type != _path_point_object) {
            path_points_only = false;
        }
        if (type != _placeholder_object || !((PlaceholderObject*)obj)->connectable()) {
            single_placeholder_object = false;
        }
        if (type != _navmesh_connection_object) {
            navmesh_connection_objects_only = false;
        }
        if (type != _hotspot_object) {
            hotspots_only = false;
        }
    }
    if (movement_objects_only) {
        return Object::kCTMovementObjects;
    } else if (single_item_object) {
        return Object::kCTItemObjects;
    } else if (env_objects_only) {
        return Object::kCTEnvObjectsAndGroups;
    } else if (path_points_only) {
        return Object::kCTPathPoints;
    } else if (single_placeholder_object) {
        return Object::kCTPlaceholderObjects;
    } else if (navmesh_connection_objects_only) {
        return Object::kCTNavmeshConnections;
    } else if (hotspots_only) {
        return Object::kCTHotspots;
    } else {
        return Object::kCTNone;
    }
}

static void DrawObjInfo(Object* obj) {
    Engine::Instance()->gui.AddDebugText("selecteda", obj->obj_file, 10.0f);
    const int kBufSize = 512;
    char str[kBufSize];
    FormatString(str, kBufSize, "Object ID: %d", obj->GetID());
    Engine::Instance()->gui.AddDebugText("selectedb", str, 10.0f);
    FormatString(str, kBufSize, "Object Type: %s", CStringFromEntityType(obj->GetType()));
    Engine::Instance()->gui.AddDebugText("selectedc", str, 10.0f);
}

void MapEditor::RemoveObject(Object* o, SceneGraph* scenegraph, bool removed_by_socket) {
    Online* online = Online::Instance();

    if (online->IsActive()) {
        if (online->IsHosting()) {
            online->RemoveObject(o, o->GetID());
        } else if (removed_by_socket == false) {
            // This case is for the editor
            // If we removed something, and it was not by the socket
            // we want to inform the host.
            // we will not remove this object until the host tells us to
            online->RemoveObject(o, o->GetID());
            return;
        }
    }

    if (o->GetType() == _env_object && !((EnvObject*)o)->ofr->dynamic) {
        shadow_cache_dirty = true;
    }
    if (o->IsGroupDerived()) {
        Group* group = static_cast<Group*>(o);
        while (!group->children.empty()) {
            RemoveObject(group->children.back().direct_ptr, scenegraph, true);
        }
    }
    if (o->GetType() == _movement_object) {
        RiggedObject* ro = static_cast<MovementObject*>(o)->rigged_object();
        /*while(!ro->attached_items.items.empty()) {
            RemoveObject(ro->attached_items.items.front().item.obj, scenegraph);
        }*/
        while (!ro->children.empty()) {
            RemoveObject(ro->children.back().direct_ptr, scenegraph, true);
        }
    }
    o->SetParent(NULL);  // Disconnects from any parent objects
    // Notify everyone that object is deleted
    const int BUF_SIZE = 255;
    char buf[BUF_SIZE];
    snprintf(buf, BUF_SIZE, "notify_deleted %d", o->GetID());
    scenegraph->level->Message(buf);
    for (auto obj : scenegraph->objects_) {
        obj->NotifyDeleted(o);
    }
    scenegraph->UnlinkObject(o);
    o->Dispose();
    delete (o);
}

void LoadLevel(bool local) {
    const int BUF_SIZE = kPathSize;
    char buffer[BUF_SIZE];
    char input_buffer[BUF_SIZE];

    const char* start_dir = "Data/Levels";

    if (local) {
        FormatString(input_buffer, BUF_SIZE, "%s/Data/Levels", GetWritePath(CoreGameModID).c_str());
        start_dir = input_buffer;
    }

    Dialog::DialogErr err = Dialog::readFile("xml", 1, start_dir, buffer, BUF_SIZE);

    if (err != Dialog::NO_ERR) {
        LOGE << Dialog::DialogErrString(err) << std::endl;
    } else {
        ApplicationPathSeparators(buffer);
        Path level = FindFilePath(buffer, kAnyPath);
        if (level.isValid()) {
            level = FindShortestPath2(level.GetFullPath());
            LevelInfoAssetRef levelinfo = Engine::Instance()->GetAssetManager()->LoadSync<LevelInfoAsset>(buffer);
            if (levelinfo.valid()) {
                Engine::Instance()->QueueState(EngineState(levelinfo->GetName(), kEngineEditorLevelState, level));
            } else {
                LOGE << "Unable to load level info, will not queue level for load." << std::endl;
            }
        }
    }
}

void MapEditor::RibbonItemClicked(const std::string& item, bool param) {
    if (item == "prefab") {
        int err = PrefabSelected();

        if (err == 0) {
        } else if (err == 1) {
            ShowEditorMessage(3, "Selected object is already a prefab.");
        } else if (err == 2) {
            ShowEditorMessage(3, "Selected objects contain a prefab, prefabs can't contain prefabs.");
        } else if (err == 3) {
            ShowEditorMessage(3, "Selected group object has a parent, orphan it first.");
        } else if (err == 4) {
            ShowEditorMessage(3, "All selected objects already have a parent.");
        } else if (err == 5) {
            ShowEditorMessage(3, "Unknown error");
        } else if (err == 6) {
            ShowEditorMessage(3, "Error adding prefab to scenegraph");
        }
    } else if (item == "ungroup") {
        UngroupSelected();
    } else if (item == "sendinrabbot") {
        SendInRabbot();
    } else if (item == "objecteditoractive") {
        SetTypeEnabled(_env_object, param);
        SetTypeEnabled(_group, param);
        SetTypeEnabled(_prefab, param);
    } else if (item == "decaleditoractive") {
        SetTypeEnabled(_decal_object, param);
    } else if (item == "hotspoteditoractive") {
        gameplay_objects_enabled_ = param;

        SetTypeEnabled(_hotspot_object, param);
        SetTypeEnabled(_movement_object, param);
        SetTypeEnabled(_path_point_object, param);
        SetTypeEnabled(_item_object, param);
        SetTypeEnabled(_navmesh_region_object, param);
        SetTypeEnabled(_navmesh_hint_object, param);
        SetTypeEnabled(_navmesh_connection_object, param);
        SetTypeEnabled(_placeholder_object, param);
        SetTypeVisible(_navmesh_region_object, param);
        SetTypeVisible(_navmesh_hint_object, param);
        SetTypeVisible(_navmesh_connection_object, param);
    } else if (item == "lighteditoractive") {
        SetTypeEnabled(_dynamic_light_object, param);
        SetTypeEnabled(_reflection_capture_object, param);
        SetTypeEnabled(_light_volume_object, param);
    } else if (item == "isolate") {
        // ToggleSelectedDecalIsolation();
    } else if (item == "exit") {
        Input::Instance()->RequestQuit();
    } else if (item == "addprobes") {
        AddLightProbes();
    } else if (item == "calculateao") {
        BakeLightProbes(0);
    } else if (item == "calculate2pass") {
        BakeLightProbes(1);
    } else if (item == "toggleviewnavmesh") {
        SetViewNavMesh(param);
    } else if (item == "edit_selected_dialogue") {
        scenegraph_->level->Message("edit_selected_dialogue");
    } else if (item == "load_dialogue") {
        Input::Instance()->ignore_mouse_frame = true;
        const int BUF_SIZE = 512;
        char buffer[BUF_SIZE];
        Dialog::DialogErr err = Dialog::readFile("txt", 1, "Data/Dialogues", buffer, BUF_SIZE);

        if (err != Dialog::NO_ERR) {
            LOGE << Dialog::DialogErrString(err) << std::endl;
        } else {
            std::string shortened = FindShortestPath(std::string(buffer));
            LoadDialogueFile(shortened.c_str());
        }
    } else if (item == "create_empty_dialogue") {
        LoadDialogueFile("empty");
    } else if (item == "level_script") {
        Input::Instance()->ignore_mouse_frame = true;
        const int BUF_SIZE = 512;
        char buffer[BUF_SIZE];

        // Display a file dialog to the user
        Dialog::DialogErr err = Dialog::readFile("as", 1, "Data/Scripts", buffer, BUF_SIZE);
        if (err != Dialog::NO_ERR) {
            LOGE << Dialog::DialogErrString(err) << std::endl;
        }
        std::string scriptName = SplitPathFileName(buffer).second;
        scenegraph_->level->SetLevelSpecificScript(scriptName);
    } else if (item == "carve_against_terrain") {
        CarveAgainstTerrain();
    } else if (item == "rebake_light_probes") {
        LightProbeCollection& lpc = scenegraph_->light_probe_collection;
        for (size_t light_probe_index = 0, len = lpc.light_probes.size(); light_probe_index < len; ++light_probe_index) {
            LightProbe& light_probe = lpc.light_probes[light_probe_index];
            for (auto& cube_face : light_probe.ambient_cube_color) {
                cube_face = vec3(0.0f);
            }
            lpc.MoveProbe(light_probe.id, light_probe.pos);  // Hackish way to force refresh of this probe
        }
    } else if (item == "show_tet_mesh") {
        LightProbeCollection& lpc = scenegraph_->light_probe_collection;
        lpc.tet_mesh_viz_enabled = param;
        if (!lpc.light_probes.empty()) {  // Move probe to force tet mesh update
            lpc.MoveProbe(lpc.light_probes[0].id, lpc.light_probes[0].pos);
        }
    } else if (item == "show_probes") {
        LightProbeCollection& lpc = scenegraph_->light_probe_collection;
        lpc.show_probes = param;
        SetTypeEnabled(_light_probe_object, param);
    } else if (item == "show_probes_through_walls") {
        LightProbeCollection& lpc = scenegraph_->light_probe_collection;
        lpc.show_probes_through_walls = param;
    } else if (item == "probe_lighting_enabled") {
        LightProbeCollection& lpc = scenegraph_->light_probe_collection;
        lpc.probe_lighting_enabled = param;
    }
}

static void SendMessageToSelectedObjectsVAList(SceneGraph* scenegraph, OBJECT_MSG::Type type, va_list args) {
    for (auto obj : scenegraph->objects_) {
        if (obj->Selected()) {
            obj->ReceiveObjectMessageVAList(type, args);
        }
    }
}

static void SendMessageToSelectedObjects(SceneGraph* scenegraph, OBJECT_MSG::Type type, ...) {
    va_list args;
    va_start(args, type);
    SendMessageToSelectedObjectsVAList(scenegraph, type, args);
    va_end(args);
}

static void BoxSelectEntities(SceneGraph* scenegraph,
                              const TypeEnable& type_enable,
                              const LineSegment& mouseray,
                              const vec4& p1, const vec4& p3, const vec4& r1, const vec4& r2,
                              const vec4& n1, const vec4& n2, const vec4& n3, const vec4& n4, bool holding_shift) {
    Keyboard* keyboard = &(Input::Instance()->getKeyboard());

    vec4 test_p, test_d;
    Collision c;
    for (size_t i = 0, len = scenegraph->objects_.size(); i < len; ++i) {
        Object* obj = scenegraph->objects_[i];
        if (!obj->parent && obj->permission_flags & Object::CAN_SELECT && type_enable.IsTypeEnabled(obj->GetType())) {
            test_p = obj->GetTranslation();

            test_d = test_p - p1;
            if (dot(test_d, n1) < 0) continue;
            if (dot(test_d, n4) < 0) continue;

            test_d = test_p - p3;
            if (dot(test_d, n2) < 0) continue;
            if (dot(test_d, n3) < 0) continue;

            if (keyboard->isKeycodeDown(SDLK_CTRL, KIMF_LEVEL_EDITOR_GENERAL)) {
                // lineCheck against center and all eight corners of entity's bounding box; only miss if none of these points are hit
                bool hit = false;
                // vec3 rad_ws = obj->GetTransform() * (obj->box_.dims*0.5f);
                c = scenegraph->lineCheckCollidable(mouseray.start, vec3(test_p[0], test_p[1], test_p[2]), NULL);
                if (c.hit_what == obj) hit = true;
                if (!hit) continue;
            }

            obj->Select(true);
        }
    }
}

MapEditor::MapEditor() : state_(MapEditor::kIdle),
                         control_obj(NULL),
                         active_tool_(EditorTypes::OMNI),
                         save_countdown_(0),
                         sky_editor_(NULL),
                         scenegraph_(NULL),
                         last_saved_chunk_global_id(0),
                         create_as_prefab_(false),
                         type_enable_("edit"),
                         type_visible_("visible"),
                         gameplay_objects_enabled_(IsTypeEnabled(_hotspot_object)),
                         color_history_countdown_(-1),
                         terrain_preview_mode(false),
                         terrain_preview(NULL) {
}

MapEditor::~MapEditor() {
    DeleteEditors();
}

void RibbonFlash(GUI* gui, std::string which) {
}

void MapEditor::SendInRabbot() {
    LOG_ASSERT(state_ != MapEditor::kInGame);
    LOGI << "Requesting player-controlled mode..." << std::endl;
    scenegraph_->UpdatePhysicsTransforms();
    state_ = MapEditor::kInGame;
    LOGI << "Request successful..." << std::endl;
}

void MapEditor::StopRabbot(bool handle_gui) {
    LOG_ASSERT(state_ == MapEditor::kInGame);
    LOGI << "Requesting editor-controlled mode..." << std::endl;
    state_ = MapEditor::kIdle;
    LOGI << "Request successful..." << std::endl;
}

void MapEditor::DeleteEditors() {
    delete sky_editor_;
    sky_editor_ = NULL;
    scenegraph_ = NULL;
}

void MapEditor::SetTool(EditorTypes::Tool tool) {
    active_tool_ = tool;
}

void MapEditor::Initialize(SceneGraph* s) {
    ClearUndoHistory();
    DeleteEditors();
    InitializeColorHistory();
    scenegraph_ = s;
    sky_editor_ = new SkyEditor(s);
    sky_editor_->flare = scenegraph_->flares.MakeFlare(vec3(0.0f), 1.0f, true);
    imui_context.Init();
    control_editor_info.face_selected_ = -1;
    control_editor_info.face_display_.facing = false;
    control_editor_info.face_display_.plane = false;
}

void MapEditor::UpdateEnabledObjects() {
    for (auto obj : scenegraph_->objects_) {
        obj->editor_visible = type_visible_.IsTypeEnabled(obj->GetType());
        if (!type_enable_.IsTypeEnabled(obj->GetType())) {
            obj->Select(false);
        }
    }
}

void MapEditor::InitializeColorHistory() {
    for (auto& i : color_history_) {
        i = vec4(1.0f);
    }
}

struct WeapConnectionResult {
    AttachmentSlot slot;
    MovementObject* mo;
};

void DrawWeaponConnectionUI(const SceneGraph* scenegraph, IMUIContext& imui_context, const ItemRef& item_ref, WeapConnectionResult* result) {
    result->mo = NULL;
    vec3 cam_facing = ActiveCameras::Get()->GetFacing();
    vec3 cam_up = ActiveCameras::Get()->GetUpVector();
    vec3 cam_right = cross(cam_facing, cam_up);
    int ui_index = 0;
    for (auto movement_object : scenegraph->movement_objects_) {
        MovementObject* mo = (MovementObject*)movement_object;
        std::list<AttachmentSlot> list;
        mo->rigged_object()->AvailableItemSlots(item_ref, &list);
        for (auto& slot : list) {
            mat4 circle_transform;
            circle_transform.SetColumn(0, cam_right * 0.1f);
            circle_transform.SetColumn(1, cam_up * 0.1f);
            circle_transform.SetColumn(2, cam_facing * 0.1f);
            circle_transform.SetTranslationPart(slot.pos);
            vec4 color = vec4(0.5f, 0.5f, 0.5f, 0.5f);
            vec3 screen_pos_tl = ActiveCameras::Get()->worldToScreen(circle_transform * vec3(-1, -1, 0));
            vec3 screen_pos_br = ActiveCameras::Get()->worldToScreen(circle_transform * vec3(1, 1, 0));
            if (screen_pos_tl[2] < 1.0 && screen_pos_br[2] < 1.0) {
                vec2 top_left(screen_pos_tl[0], screen_pos_tl[1]);
                vec2 bottom_right(screen_pos_br[0], screen_pos_br[1]);
                IMUIContext::UIState ui_state;
                if (imui_context.DoButton(ui_index++, top_left, bottom_right, ui_state)) {
                    result->mo = mo;
                    result->slot = slot;
                }
                if (ui_state == IMUIContext::kActive) {
                    color = vec4(0.0f, 0.0f, 0.0f, 0.5f);
                }
                if (ui_state == IMUIContext::kHot) {
                    color = vec4(1.0f, 1.0f, 1.0f, 0.5f);
                }
            }
            DebugDraw::Instance()->AddCircle(circle_transform, color, _delete_on_draw);
        }
    }
}

static vec3 TransformVec3(const mat4& transform, const vec3& vector) {
    return {
        transform.entries[0 + 0 * 4] * vector[0] + transform.entries[0 + 1 * 4] * vector[1] + transform.entries[0 + 2 * 4] * vector[2] + transform.entries[0 + 3 * 4],
        transform.entries[1 + 0 * 4] * vector[0] + transform.entries[1 + 1 * 4] * vector[1] + transform.entries[1 + 2 * 4] * vector[2] + transform.entries[1 + 3 * 4],
        transform.entries[2 + 0 * 4] * vector[0] + transform.entries[2 + 1 * 4] * vector[1] + transform.entries[2 + 2 * 4] * vector[2] + transform.entries[2 + 3 * 4],
    };
}

static void DrawBox(const Box& box_, const Object* object_, bool highlight, const vec4& box_color) {
    mat4 transform = object_->GetTransform();
    // Translate
    transform[0 + 3 * 4] += box_.center[0];
    transform[1 + 3 * 4] += box_.center[1];
    transform[2 + 3 * 4] += box_.center[2];
    // Scale
    transform[0 + 0 * 4] *= box_.dims[0];
    transform[1 + 0 * 4] *= box_.dims[0];
    transform[2 + 0 * 4] *= box_.dims[0];
    transform[0 + 1 * 4] *= box_.dims[1];
    transform[1 + 1 * 4] *= box_.dims[1];
    transform[2 + 1 * 4] *= box_.dims[1];
    transform[0 + 2 * 4] *= box_.dims[2];
    transform[1 + 2 * 4] *= box_.dims[2];
    transform[2 + 2 * 4] *= box_.dims[2];

    vec4 draw_color = box_color;
    draw_color[3] *= highlight ? 1.0f : 0.2f;

    static const GLfloat vertices[] = {
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f,      // front (in coord system: +x to the right, +y up, -z into distance)
        -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f,  // back
    };
    static const GLubyte edges[] = {
        0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6,
        6, 7, 7, 4, 4, 0, 7, 3, 5, 1, 6, 2};

    GLfloat transformed_vertices[3 * 8];
    for (int i = 0; i < 8; ++i) {
        vec3 temp = TransformVec3(transform, *(vec3*)&vertices[i * 3]);
        memcpy(&transformed_vertices[i * 3], &temp, sizeof(vec3));
    }

    // Draw edges of box
    for (int i = 0; i < 12; i++) {
        int start_index = edges[i * 2 + 0] * 3;
        int end_index = edges[i * 2 + 1] * 3;
        vec3 start = *(vec3*)&transformed_vertices[start_index];
        vec3 end = *(vec3*)&transformed_vertices[end_index];
        DebugDraw::Instance()->AddLine(start, end, draw_color, _delete_on_draw);
    }
}

static void DrawControlledFace(const Box& box_, const Object* object_, int face_selected_, FaceDisplay face_display_) {
    PROFILER_GPU_ZONE(g_profiler_ctx, "DrawControlledFace");

    mat4 translate_mat;
    translate_mat.SetTranslation(box_.center);
    mat4 model_mat = object_->GetTransform() * translate_mat;

    static const GLfloat vertices[] = {
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f,      // front (in coord system: +x to the right, +y up, -z into distance)
        -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f,  // back
        -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f,  // left
        0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f,      // right
        -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f,  // bottom
        -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f       // top
    };
    static const GLuint faces[] = {
        0, 1, 2, 0, 2, 3, 5, 4, 7, 5, 7, 6, 8, 9, 10, 8, 10, 11,
        12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23};

    static VBOContainer vertices_vbo;
    static VBOContainer faces_vbo;
    if (!vertices_vbo.valid()) {
        vertices_vbo.Fill(kVBOStatic | kVBOFloat, sizeof(vertices), (void*)vertices);
        faces_vbo.Fill(kVBOStatic | kVBOElement, sizeof(faces), (void*)faces);
    }

    Graphics* graphics = Graphics::Instance();

    GLState gl_state;
    gl_state.depth_test = true;
    gl_state.blend = true;
    gl_state.cull_face = false;
    gl_state.depth_write = false;
    graphics->setGLState(gl_state);
    graphics->setDepthFunc(GL_LEQUAL);

    mat4 scale_mat;
    scale_mat.SetScale(box_.dims);
    mat4 new_model_mat = model_mat * scale_mat;

    // Draw selected face
    if (face_selected_ != -1) {
        Shaders* shaders = Shaders::Instance();
        Camera* cam = ActiveCameras::Get();
        int shader_id = shaders->returnProgram("face_stipple");
        shaders->setProgram(shader_id);
        mat4 mvp_mat = cam->GetProjMatrix() * cam->GetViewMatrix() * new_model_mat;
        shaders->SetUniformMat4("mvp_mat", mvp_mat);
        shaders->SetUniformVec4("color", vec4(0.1f, 1.0f, 0.4f, 1.0f));
        int vert_coord_id = shaders->returnShaderAttrib("vert_coord", shader_id);
        vertices_vbo.Bind();
        faces_vbo.Bind();
        graphics->EnableVertexAttribArray(vert_coord_id);
        glVertexAttribPointer(vert_coord_id, 3, GL_FLOAT, false, 3 * sizeof(GLfloat), 0);
        graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)(sizeof(GLuint) * (face_selected_ * 6)));
        graphics->ResetVertexAttribArrays();
        // Draw direction line for selected face
        if (face_display_.facing || face_display_.plane) {
            mat4 transform;
            switch (face_selected_) {
                case 0:
                    transform = Mat4FromQuaternion(quaternion(vec4(0.0f, 1.0f, 0.0f, PI_f * -0.5f)));
                    break;
                case 1:
                    transform = Mat4FromQuaternion(quaternion(vec4(0.0f, 1.0f, 0.0f, PI_f * 0.5f)));
                    break;
                case 2:
                    transform = Mat4FromQuaternion(quaternion(vec4(0.0f, 1.0f, 0.0f, PI_f)));
                    break;
                case 3:
                    break;
                case 4:
                    transform = Mat4FromQuaternion(quaternion(vec4(0.0f, 0.0f, 1.0f, PI_f * -0.5f)));
                    break;
                case 5:
                    transform = Mat4FromQuaternion(quaternion(vec4(0.0f, 0.0f, 1.0f, PI_f * 0.5f)));
                    break;
                default:
                    break;
            }

            // Draw line if translating, scaling, or rotating on the plane normal
            vec3 color(0.5f, 0.0f, 0.0f);
            if (face_display_.facing) {
                vec3 points[2];
                points[0] = new_model_mat * transform * vec3(0.5f, 0.0f, 0.0f);
                points[1] = new_model_mat * transform * vec3(1.0f, 0.0f, 0.0f);
                DebugDraw::Instance()->AddLine(points[0], points[1], vec4(color, 1.0f), vec4(color, 0.0f), _delete_on_draw);
            }

            // Draw grid if translating/scaling on a plane
            if (face_display_.plane) {
                static const mat4 transform_test = Mat4FromQuaternion(quaternion(vec4(1.0f, 0.0f, 0.0f, PI_f * -0.5f)));
                vec3 points[2];
                mat4 mat[2];
                mat[0] = new_model_mat * transform;
                mat[1] = mat[0] * transform_test;
                for (int i = 0; i < 3; ++i) {
                    for (int j = 0; j < 2; ++j) {
                        for (const auto& k : mat) {
                            points[0] = k * vec3(0.5f, 0.0f, -0.5f + 0.5f * (float)i);
                            points[1] = k * vec3(0.5f, 1.0f - (float)j * 2.0f, -0.5f + 0.5f * (float)i);
                            DebugDraw::Instance()->AddLine(points[0], points[1], vec4(color, 1.0f), vec4(color, 0.0f), _delete_on_draw);
                        }
                    }
                }
            }
        }
    }
}

static void GetClosest(LineSegment mouseray, Object* obj, Object*& highlit_obj, float& highlit_obj_dist) {
    vec3 point;
    vec3 normal;
    if (obj->lineCheck(mouseray.start, mouseray.end, &point, &normal) != -1) {
        float dist_sq = dot(point - mouseray.start, point - mouseray.start);
        if (dist_sq < highlit_obj_dist) {
            highlit_obj = obj;
            highlit_obj_dist = dist_sq;
        }
    }
}

void MapEditor::Draw() {
    LineSegment mouseray;
    GetMouseRay(&mouseray);

    imui_context.UpdateControls();

    if (!Graphics::Instance()->media_mode()) {
        if (active_tool_ != EditorTypes::CONNECT && active_tool_ != EditorTypes::DISCONNECT) {
            for (auto obj : scenegraph_->objects_) {
                if (obj->editor_visible) {
                    const EntityType& type = obj->GetType();

                    if (IsTypeEnabled(type) && ((
                                                    ((type != _group && type != _prefab) || draw_group_and_prefab_boxes) &&
                                                    type != _env_object &&
                                                    type != _decal_object &&
                                                    type != _light_probe_object &&
                                                    type != _dynamic_light_object &&
                                                    type != _navmesh_hint_object) ||
                                                obj->Selected())) {
                        // Draw box for selected objects or objects that always have a visible box.
                        DrawBox(obj->box_, obj, obj->Selected(), obj->box_color);
                    }
                }
                if (always_draw_hotspot_connections) {
                    vec3 start = obj->GetTranslation();
                    for (int id : obj->connected_to) {
                        vec3 end = scenegraph_->GetObjectFromID(id)->GetTranslation();
                        DebugDraw::Instance()->AddLine(start, end, vec4(0.0f, 1.0f, 0.0f, 0.8f), _delete_on_draw);
                    }
                }
            }
        }
        if (state_ == MapEditor::kTransformDrag && control_obj) {
            DrawControlledFace(control_obj->box_, control_obj, control_editor_info.face_selected_, control_editor_info.face_display_);
        }
    }
    sky_editor_->Draw();
    if (sky_editor_->m_lighting_changed) {
        scenegraph_->SendMessageToAllObjects(OBJECT_MSG::LIGHTING_CHANGED);
        sky_editor_->m_lighting_changed = false;
    }

    if (box_selector_.acting) {
        box_selector_.Draw();
    }

    // Handle attachments
    ReturnSelected(&selected);
    if (selected.size() == 1 && selected[0]->GetType() == _item_object && (active_tool_ == EditorTypes::CONNECT || active_tool_ == EditorTypes::DISCONNECT)) {
        // Attach weapons to character attachment points
        DebugDraw::Instance()->AddLine(selected[0]->GetTranslation(), mouseray.end, vec4(vec3(0.5f), 0.5f), _delete_on_draw, _DD_XRAY);
        WeapConnectionResult wcr;
        Object* weap_obj = selected[0];
        DrawWeaponConnectionUI(scenegraph_, imui_context, ((ItemObject*)weap_obj)->item_ref(), &wcr);
        if (wcr.mo) {
            if (active_tool_ == EditorTypes::CONNECT) {
                wcr.mo->AttachItemToSlotEditor(weap_obj->GetID(), wcr.slot.type, wcr.slot.mirrored, wcr.slot.attachment_ref);
            } else {
                weap_obj->Disconnect(*wcr.mo);
            }
            QueueSaveHistoryState();
        }
    }
    box_objects.clear();
    box_objects.reserve(64);
    if (active_tool_ == EditorTypes::CONNECT || active_tool_ == EditorTypes::DISCONNECT) {
        Object* highlit_obj = NULL;
        Object::ConnectionType connection_type = GetConnectionType(selected);

        if (active_tool_ == EditorTypes::CONNECT) {
            float highlit_obj_dist = FLT_MAX;

            if (connection_type != Object::kCTNone) {
                for (auto obj : scenegraph_->objects_) {
                    if (IsTypeEnabled(obj->GetType())) {
                        bool add_object = obj->Selected();
                        if (!obj->Selected()) {
                            for (auto& j : selected) {
                                if (j->GetType() == _hotspot_object) {
                                    Hotspot* selected_obj = (Hotspot*)j;
                                    if (selected_obj->AcceptConnectionsTo(*obj)) {
                                        if (obj->GetType() == _hotspot_object) {
                                            if (((Hotspot*)obj)->AcceptConnectionsFromImplemented()) {
                                                add_object = obj->AcceptConnectionsFrom(connection_type, *selected_obj);
                                                if (!add_object) {
                                                    add_object = false;
                                                    break;
                                                }
                                            } else {
                                                add_object = true;
                                            }
                                        } else {
                                            add_object = true;
                                        }
                                    } else {
                                        add_object = false;
                                        break;
                                    }
                                } else {
                                    add_object = obj->AcceptConnectionsFrom(connection_type, *j);
                                    if (!add_object) {
                                        add_object = false;
                                        break;
                                    }
                                }
                            }
                        }

                        if (add_object) {
                            vec3 point;
                            vec3 normal;
                            if (obj->lineCheck(mouseray.start, mouseray.end, &point, &normal) != -1) {
                                float dist_sq = dot(point - mouseray.start, point - mouseray.start);
                                if (dist_sq < highlit_obj_dist) {
                                    highlit_obj = obj;
                                    highlit_obj_dist = dist_sq;
                                }
                            }
                            box_objects.push_back(obj);
                        }
                    }
                }
            }
        } else {
            float highlit_obj_dist = FLT_MAX;
            std::vector<int> connected_ids;
            connected_ids.reserve(64);
            for (Object* selected_obj : selected) {
                box_objects.push_back(selected_obj);
                connected_ids.clear();
                selected_obj->GetConnectionIDs(&connected_ids);
                for (int connected_id : connected_ids) {
                    Object* obj = scenegraph_->GetObjectFromID(connected_id);
                    GetClosest(mouseray, obj, highlit_obj, highlit_obj_dist);
                    box_objects.push_back(obj);
                }
                for (int id : selected_obj->connected_to) {
                    Object* obj = scenegraph_->GetObjectFromID(id);
                    GetClosest(mouseray, obj, highlit_obj, highlit_obj_dist);
                    box_objects.push_back(obj);
                }
                for (int id : selected_obj->connected_from) {
                    Object* obj = scenegraph_->GetObjectFromID(id);
                    GetClosest(mouseray, obj, highlit_obj, highlit_obj_dist);
                    box_objects.push_back(obj);
                }
            }
        }

        if (connection_type == Object::kCTEnvObjectsAndGroups) {
            // Attach static objects to character bones
            static const uint32_t MAX_SELECTED_OBJECTS = 32;
            if (selected.size() <= MAX_SELECTED_OBJECTS) {
                Object* selected_objects[MAX_SELECTED_OBJECTS];
                for (size_t i = 0, len = selected.size(); i < len; ++i) {
                    DebugDraw::Instance()->AddLine(selected[i]->GetTranslation(), mouseray.end, vec4(vec3(0.5f), 0.5f), _delete_on_draw, _DD_XRAY);
                    selected_objects[i] = selected[i];
                }
                std::vector<Object*>& movement_objects = scenegraph_->movement_objects_;
                for (auto& movement_object : movement_objects) {
                    MovementObject* mo = (MovementObject*)movement_object;
                    if (mo->rigged_object()->DrawBoneConnectUI(selected_objects, (int)selected.size(), imui_context, active_tool_, mo->GetID())) {
                        QueueSaveHistoryState();
                    }
                }
            }
        }

        for (auto& i : selected) {
            vec3 start = i->GetTranslation();
            for (size_t j = 0; j < i->connected_to.size(); ++j) {
                vec3 end = scenegraph_->GetObjectFromID(i->connected_to[j])->GetTranslation();
                DebugDraw::Instance()->AddLine(start, end, vec4(0.0f, 1.0f, 0.0f, 0.8f), _delete_on_draw);
                DebugDraw::Instance()->AddLine(start, end, vec4(0.0f, 1.0f, 0.0f, 0.3f), _delete_on_draw, _DD_XRAY);
            }
            for (size_t j = 0; j < i->connected_from.size(); ++j) {
                vec3 end = scenegraph_->GetObjectFromID(i->connected_from[j])->GetTranslation();
                DebugDraw::Instance()->AddLine(start, end, vec4(1.0f, 0.0f, 0.0f, 0.8f), _delete_on_draw);
                DebugDraw::Instance()->AddLine(start, end, vec4(1.0f, 0.0f, 0.0f, 0.3f), _delete_on_draw, _DD_XRAY);
            }
        }

        for (auto& box_object : box_objects) {
            DrawBox(box_object->box_, box_object, box_object == highlit_obj || box_object->Selected(), box_object->box_color);
        }
    } else {
        imui_context.ClearHot();
    }

    if (terrain_preview_mode) {
        int width = Graphics::Instance()->window_dims[0];
        int height = Graphics::Instance()->window_dims[1];
        ImGui::SetNextWindowSize(ImVec2(256.0f, 54.0f));
        ImGui::SetNextWindowPos(ImVec2(width * 0.5f - 128.0f, height - 64.0f));
        ImGui::Begin("##terrainpreview", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        ImGui::Text("%s", "Terrain preview mode");
        if (ImGui::Button("Use new terrain")) {
            if (terrain_preview) {
                TerrainInfo terrain_info = terrain_preview->terrain_info();
                terrain_info.minimal = false;

                Object* terrain_object_ptr = scenegraph_->terrain_object_;
                if (terrain_object_ptr) {
                    scenegraph_->UnlinkObject(terrain_object_ptr);
                    delete terrain_object_ptr;
                }
                scenegraph_->UnlinkObject(terrain_preview);
                delete terrain_preview;
                terrain_preview = NULL;

                TerrainObject* new_terrain = new TerrainObject(terrain_info);
                new_terrain->SetID(0);
                scenegraph_->addObject(new_terrain);
                scenegraph_->terrain_object_ = new_terrain;
                new_terrain->PreparePhysicsMesh();
                scenegraph_->sky->QueueLoadResources();
                scenegraph_->sky->BakeFirstPass();
            }
            // Nothing to do if heightmap hasn't changed

            ExitTerrainPreviewMode();
        }
        ImGui::SameLine();
        if (ImGui::Button("Keep old terrain")) {
            if (terrain_preview) {
                scenegraph_->UnlinkObject(terrain_preview);
                delete terrain_preview;
                terrain_preview = NULL;
            } else {
                TerrainObject* terrain = scenegraph_->terrain_object_;
                terrain->SetTerrainColorTexture(real_terrain_info.colormap.c_str());
                terrain->SetTerrainWeightTexture(real_terrain_info.weightmap.c_str());
                terrain->SetTerrainDetailTextures(real_terrain_info.detail_map_info);
            }

            ExitTerrainPreviewMode();
        }
        ImGui::End();
    }
}

static Collision lineCheckSelected(SceneGraph::object_list& objects, const vec3& start, const vec3& end, vec3* point) {
    Collision new_collision;
    new_collision.hit = false;
    new_collision.hit_what = NULL;
    vec3 new_end = end;
    vec3 normal;
    int collided = -1;
    int collide = -1;
    for (auto obj : objects) {
        if (obj->Selected()) {
            collide = obj->lineCheck(start, new_end, point, &normal);
            if (collide != -1) {
                new_end = *point;
                collided = collide;
                new_collision.hit = true;
                new_collision.hit_normal = normal;
                new_collision.hit_what = obj;
                new_collision.hit_how = collided;
                new_collision.hit_where = new_end;
            }
        }
    }
    return new_collision;
}

static Collision lineCheckActiveSelected(SceneGraph* scenegraph, const vec3& start, const vec3& end, vec3* point, vec3* normal) {
    vec3 new_end = end;
    Collision tmp, ret;
    vec3 tmp_point;
    vec3* point_ptr;
    if (point) {
        point_ptr = point;
    } else {
        point_ptr = &tmp_point;
    }
    // lineCheck aginst the terrain
    if (scenegraph != NULL && scenegraph->terrain_object_ != NULL) {
        vec3 normal;
        if (scenegraph->terrain_object_->lineCheck(start, end, point_ptr, &normal) != -1) {
            new_end = *point_ptr;
            new_end += 0.01f * normalize(end - start);  // so that decals on surface of terrain are hit (To do: Why is this necessary? Isn't it also done in DecalEditor::lineCheckDisplay2D?)
        }
    }
    tmp = lineCheckSelected(scenegraph->objects_, start, new_end, point_ptr);
    if (tmp.hit) {
        new_end = tmp.hit_where;
        ret = tmp;
    }
    return ret;
}

static EditorTypes::Tool OmniGetTool(const Collision& c, int permission_flags, const Object* object_, const Box& box_) {
    const Keyboard& keyboard = Input::Instance()->getKeyboard();
    if (KeyCommand::CheckDown(keyboard, KeyCommand::kForceTranslate, KIMF_LEVEL_EDITOR_GENERAL) && permission_flags & Object::CAN_TRANSLATE) {
        return EditorTypes::TRANSLATE;
    } else if (KeyCommand::CheckDown(keyboard, KeyCommand::kForceScale, KIMF_LEVEL_EDITOR_GENERAL) && permission_flags & Object::CAN_SCALE) {
        return EditorTypes::SCALE;
    } else if (KeyCommand::CheckDown(keyboard, KeyCommand::kForceRotate, KIMF_LEVEL_EDITOR_GENERAL) && permission_flags & Object::CAN_ROTATE) {
        return EditorTypes::ROTATE;
    }

    vec3 point = c.hit_where;
    vec3 normal = c.hit_normal;

    // to object space
    mat4 inv = invert(object_->GetTransform());
    point = inv * point;
    normal = normalize(inv.GetRotatedvec3(normal));

    int which_face = box_.GetHitFaceIndex(normal, point);
    if (which_face == -1) {
        return EditorTypes::NO_TOOL;
    }
    float point_dist = 0;
    int closest_point = box_.GetNearestPointIndex(point, point_dist);
    point_dist = length((point - box_.GetPoint(closest_point)) / box_.dims);

    static const float dimensions_shrink = 0.75f;
    if (permission_flags & Object::CAN_TRANSLATE &&
        ((!(permission_flags & Object::CAN_ROTATE) && !(permission_flags & Object::CAN_SCALE)) ||
         box_.IsInFace(point, which_face, dimensions_shrink))) {
        return EditorTypes::TRANSLATE;
    } else if (permission_flags & Object::CAN_SCALE &&
               (!(permission_flags & Object::CAN_ROTATE) || point_dist < 0.25f)) {
        return EditorTypes::SCALE;
    } else if (permission_flags & Object::CAN_ROTATE) {
        return EditorTypes::ROTATE;
    }
    return EditorTypes::NO_TOOL;
}

static void SetBit(uint32_t* bitfield, int mask, bool val) {
    if (val) {
        *bitfield |= mask;
    } else {
        *bitfield &= ~mask;
    }
}

void MapEditor::Update(GameCursor* cursor) {
    if (!Engine::Instance()->menu_paused) {
        // Update editor mouseray
        LineSegment mouseray;
        GetMouseRay(&mouseray);

        if (state_ == MapEditor::kIdle) {  // Update active tool if no action is occuring
            Collision mouseray_collision_selected = lineCheckActiveSelected(scenegraph_, mouseray.start, mouseray.end, NULL, NULL);
            omni_tool_tool_ = EditorTypes::NO_TOOL;
            bool hit_something = mouseray_collision_selected.hit;
            if (hit_something) {
                ;
                Object* hit_what = mouseray_collision_selected.hit_what;
                if (hit_what) {
                    omni_tool_tool_ = OmniGetTool(mouseray_collision_selected, hit_what->permission_flags, hit_what, hit_what->box_);
                }
            }
        }
        // Handle sky editor
        if (state_ == MapEditor::kSkyDrag || !lineCheckActiveSelected(scenegraph_, mouseray.start, mouseray.end, NULL, NULL).hit) {
            vec3 mouseray_dir = normalize(mouseray.end - mouseray.start);
            float dst_from_sun = dot(mouseray_dir, sky_editor_->sun_dir_);
            if (dst_from_sun <= 1.0f && dst_from_sun >= 0.0f) {
                float angle_from_sun = acosf(dst_from_sun);

                // Check if we've double-clicked on the sun
                if (!sky_editor_->HandleSelect(angle_from_sun)) {
                    // Get relevant tool based on mouse position
                    if (!sky_editor_->m_sun_translating && !sky_editor_->m_sun_scaling && !sky_editor_->m_sun_rotating) {
                        sky_editor_->m_tool = sky_editor_->OmniGetTool(angle_from_sun, mouseray);
                    }
                    // Handle the activated tool
                    if (!sky_editor_->m_sun_scaling && !sky_editor_->m_sun_rotating && sky_editor_->m_tool == EditorTypes::TRANSLATE) sky_editor_->HandleSunTranslate(angle_from_sun);
                    if (!sky_editor_->m_sun_translating && !sky_editor_->m_sun_rotating && sky_editor_->m_tool == EditorTypes::SCALE) sky_editor_->HandleSunScale(angle_from_sun);
                    if (!sky_editor_->m_sun_translating && !sky_editor_->m_sun_scaling && sky_editor_->m_tool == EditorTypes::ROTATE) sky_editor_->HandleSunRotate(angle_from_sun, mouseray_dir, cursor);
                }
            }
        }
        if (!CheckForSelections(mouseray)) {
            if (state_ == MapEditor::kIdle) {
                HandleShortcuts(mouseray);
            }
            UpdateTools(mouseray, cursor);
        }
        // Update cursor
        if (state_ == MapEditor::kIdle) {
            UpdateCursor(mouseray, cursor);
        }
    }

    if (color_history_countdown_) {
        color_history_countdown_--;
        if (color_history_countdown_ == 0) {
            int found_index = -1;
            for (int i = 0; i < kColorHistoryLen; ++i) {
                bool closeEnough = true;

                for (int j = 0; j < 4; ++j) {
                    if (std::fabs(color_history_candidate_[j] - color_history_[i][j]) > 0.001f) {
                        closeEnough = false;
                        break;
                    }
                }

                if (closeEnough) {
                    found_index = i;
                    break;
                }
            }

            if (found_index == -1) {
                for (int i = kColorHistoryLen - 1; i > 0; --i)
                    color_history_[i] = color_history_[i - 1];
            } else {
                for (int i = found_index; i > 0; --i) {
                    color_history_[i] = color_history_[i - 1];
                }
            }

            color_history_[0] = color_history_candidate_;
        }
    }

    if (save_countdown_) {
        save_countdown_--;
        if (save_countdown_ == 0) {
            // Add a new state to the state history
            ++state_history_.current_state;
            state_history_.num_states = state_history_.current_state + 1;
            int state_id = state_history_.current_state;

            {  // Erase all chunks past current state
                std::list<SavedChunk>& chunks = state_history_.chunks;
                for (std::list<SavedChunk>::iterator iter = chunks.begin();
                     iter != chunks.end();) {
                    SavedChunk& chunk = (*iter);
                    if (chunk.state_id >= state_history_.current_state) {
                        iter = chunks.erase(iter);
                    } else {
                        ++iter;
                    }
                }
            }
            Graphics* graphics = Graphics::Instance();
            if (!graphics->nav_mesh_out_of_date) {
                graphics->nav_mesh_out_of_date_chunk = state_id;
            }
            LOGI << "Saving history state " << state_id << "..." << std::endl;
            LOGI << "-----------------------" << std::endl;
            // Get iterator to last saved chunk before current state save
            std::list<SavedChunk>& chunks = state_history_.chunks;
            std::list<SavedChunk>::iterator last_chunk = chunks.end();
            if (last_chunk != chunks.begin()) {
                last_chunk--;
            }
            scenegraph_->level->SaveHistoryState(chunks, state_id);
            std::vector<Object*> entities;
            for (auto obj : scenegraph_->objects_) {
                if (!obj->parent && !obj->exclude_from_undo) {
                    entities.push_back(obj);
                }
            }
            for (auto& entity : entities) {
                entity->SaveHistoryState(chunks, state_id);
            }
            sky_editor_->SaveHistoryState(chunks, state_id);
            // Find out how many chunks have been changed
            int num_chunks_changed = 0;
            int num_chunks_checked = 0;
            std::list<SavedChunk>::iterator chunk_iter = last_chunk;
            if (chunk_iter != chunks.end()) {
                chunk_iter++;
            }
            for (; chunk_iter != chunks.end(); ++chunk_iter) {
                num_chunks_changed++;
            }
            // No chunks changed? Don't bother saving this revision
            if (num_chunks_changed == 0 && state_history_.num_states > 2) {
                Undo();
                state_history_.num_states--;
                LOGI << "None of " << num_chunks_checked << " chunks changed... undoing." << std::endl;
            } else {
                LOGI << "Some of " << num_chunks_checked << " chunks changed." << std::endl;
            }
            save_countdown_ = 0;

            // If the last saved chunk global is invalid we try and fix it,
            // This is mainly due to startup, where we don't have a state to point to until the first
            // state is saved.
            if (last_saved_chunk_global_id == 0) {
                SetLastSaveOnCurrentUndoChunk();
            }
        }
    }
}

void MapEditor::ShowEditorMessage(int type, const std::string& message) {
    DisplayError("Editor Error", message.c_str(), _ok);
}

void MapEditor::CPSetColor(const vec3& color) {
    SendMessageToSelectedObjects(scenegraph_, OBJECT_MSG::SET_COLOR, &color);
}

void MapEditor::UpdateCursor(const LineSegment& mouseray, GameCursor* cursor) {
    Collision mouseray_collision_selected =
        lineCheckActiveSelected(scenegraph_, mouseray.start, mouseray.end, NULL, NULL);
    EditorTypes::Tool tool_cursor;
    switch (active_tool_) {
        case EditorTypes::OMNI:
            tool_cursor = omni_tool_tool_;
            break;
        case EditorTypes::CONNECT:
            tool_cursor = EditorTypes::CONNECT;
            break;
        case EditorTypes::DISCONNECT:
            tool_cursor = EditorTypes::DISCONNECT;
            break;
        case EditorTypes::ADD_ONCE:
            tool_cursor = EditorTypes::ADD_ONCE;
            break;
        default:
            if (mouseray_collision_selected.hit) {
                tool_cursor = active_tool_;
            } else {
                if (!sky_editor_->m_sun_translating && !sky_editor_->m_sun_scaling && !sky_editor_->m_sun_rotating) {
                    sky_editor_->UpdateCursor(cursor);
                }
                return;
                // tool_cursor = EditorTypes::NO_TOOL;
            }
    }
    switch (tool_cursor) {
        case EditorTypes::CONNECT:
            cursor->SetCursor(LINK_CURSOR);
            break;
        case EditorTypes::DISCONNECT:
            cursor->SetCursor(UNLINK_CURSOR);
            break;
        case EditorTypes::TRANSLATE:
            cursor->SetCursor(TRANSLATE_CURSOR);
            break;
        case EditorTypes::SCALE:
            cursor->SetCursor(SCALE_CURSOR);
            break;
        case EditorTypes::ROTATE:
            cursor->SetCursor(ROTATE_CURSOR);
            break;
        case EditorTypes::ADD_ONCE:
            cursor->SetCursor(ADD_CURSOR);
            break;
        default:
            cursor->SetCursor(DEFAULT_CURSOR);
            break;
    }
}

void MapEditor::CopySelected() {
    RibbonFlash(gui, "copy");
    ActorsEditor_CopySelectedEntities(scenegraph_, &copy_desc_list_);
}

void MapEditor::Paste(const LineSegment& mouseray) {
    RibbonFlash(gui, "paste");
    EntityDescriptionList fixed_copy_list = copy_desc_list_;
    ActorsEditor_UnlocalizeIDs(&fixed_copy_list, scenegraph_);
    ActorsEditor_AddEntitiesAtMouse(Path(), scenegraph_, fixed_copy_list, mouseray, false);
    QueueSaveHistoryState();
}

static bool HasSelectedDeletableParent(Object* obj) {
    if (obj->parent) {
        if (obj->parent->Selected()) {
            return (obj->parent->permission_flags & Object::CAN_DELETE) != 0;
        } else {
            return HasSelectedDeletableParent(obj->parent);
        }
    } else {
        return false;
    }
}

void MapEditor::DeleteSelected() {
    RibbonFlash(gui, "delete");
    std::vector<Object*> objects_to_delete;
    for (int i = (int)scenegraph_->objects_.size() - 1; i >= 0; --i) {
        Object* obj = scenegraph_->objects_[i];
        if (!HasSelectedDeletableParent(obj)) {
            if (obj->Selected() && obj->permission_flags & Object::CAN_DELETE)
                objects_to_delete.push_back(obj);
        }
    }
    for (auto& i : objects_to_delete) {
        RemoveObject(i, scenegraph_);
    }
    QueueSaveHistoryState();
}

void MapEditor::CutSelected() {
    RibbonFlash(gui, "cut");
    CopySelected();
    DeleteSelected();
}

void MapEditor::GroupSelected() {
    RibbonFlash(gui, "group");
    ReturnSelected(&selected);
    if (selected.size() > 1) {
        std::vector<int> child_ids;
        for (auto& i : selected) {
            if (!i->parent) {
                child_ids.push_back(i->GetID());
            }
        }
        if (child_ids.size() > 1) {
            Group* group = new Group();
            if (ActorsEditor_AddEntity(scenegraph_, group)) {
                group->children.resize(child_ids.size());
                for (size_t i = 0, len = child_ids.size(); i < len; ++i) {
                    group->children[i].direct_ptr = scenegraph_->GetObjectFromID(child_ids[i]);
                }
                for (size_t i = 0, len = child_ids.size(); i < len; ++i) {
                    group->children[i].direct_ptr->SetParent(group);
                }
                group->InitShape();
                group->InitRelMats();
                DeselectAll(scenegraph_);
                group->Select(true);
                QueueSaveHistoryState();
            } else {
                LOGE << "Failed at adding new Group to scenegraph" << std::endl;
                delete group;
            }
        }
    }
}

bool MapEditor::ContainsPrefabsRecursively(std::vector<Object*> objects) {
    bool res = false;
    for (auto& object : objects) {
        if (object->IsGroupDerived()) {
            Group* g = static_cast<Group*>(object);
            std::vector<Object*> kids;
            for (auto& k : g->children) {
                kids.push_back(k.direct_ptr);
            }
            res = res || ContainsPrefabsRecursively(kids);

            if (object->GetType() == _prefab) {
                res = true;
            }
        }
    }
    return res;
}

int MapEditor::PrefabSelected() {
    RibbonFlash(gui, "prefab");
    ReturnSelected(&selected);

    if (selected.size() == 1 && selected[0]->GetType() == _prefab) {
        return 1;
    } else if (ContainsPrefabsRecursively(selected)) {
        return 2;
    } else if (selected.size() == 1 && selected[0]->GetType() == _group) {
        Group* f_group = static_cast<Group*>(selected[0]);
        if (f_group->HasParent()) {
            return 3;
        } else {
            Prefab* prefab = new Prefab();
            if (ActorsEditor_AddEntity(scenegraph_, prefab)) {
                DeselectAll(scenegraph_);

                size_t c_count = f_group->children.size();
                prefab->children.resize(c_count);
                for (unsigned i = 0; i < c_count; i++) {
                    prefab->children[i].direct_ptr = f_group->children[i].direct_ptr;
                }
                for (unsigned i = 0; i < c_count; i++) {
                    prefab->children[i].direct_ptr->SetParent(prefab);
                }

                f_group->children.clear();
                scenegraph_->UnlinkObject(f_group);
                delete f_group;

                prefab->InitShape();
                prefab->InitRelMats();
                prefab->Select(true);
                QueueSaveHistoryState();
                return 0;
            } else {
                LOGE << "Failed at adding prefab to scenegraph" << std::endl;
                delete prefab;
                return 6;
            }
        }
    } else if (selected.size() > 0) {
        std::vector<int> child_ids;
        for (auto& i : selected) {
            if (!i->parent) {
                child_ids.push_back(i->GetID());
            }
        }
        if (child_ids.size() > 0) {
            Prefab* prefab = new Prefab();
            if (ActorsEditor_AddEntity(scenegraph_, prefab)) {
                prefab->children.resize(child_ids.size());
                for (size_t i = 0, len = child_ids.size(); i < len; ++i) {
                    prefab->children[i].direct_ptr = scenegraph_->GetObjectFromID(child_ids[i]);
                }
                for (size_t i = 0, len = child_ids.size(); i < len; ++i) {
                    prefab->children[i].direct_ptr->SetParent(prefab);
                }
                prefab->InitShape();
                prefab->InitRelMats();
                DeselectAll(scenegraph_);
                prefab->Select(true);
                QueueSaveHistoryState();
                return 0;
            } else {
                LOGE << "Failed at adding prefab to scenegraph" << std::endl;
                delete prefab;
                return 6;
            }
        } else {
            return 4;
        }
    } else {
        return 5;
    }
}

void MapEditor::UngroupSelected() {
    RibbonFlash(gui, "ungroup");
    ReturnSelected(&selected);
    for (auto& i : selected) {
        if (i->GetType() == _group || i->GetType() == _prefab) {
            i->SetParent(NULL);  // Disconnects from any parent objects
            // Notify everyone that object is deleted
            const int BUF_SIZE = 255;
            char buf[BUF_SIZE];
            snprintf(buf, BUF_SIZE, "notify_deleted %d", i->GetID());
            scenegraph_->level->Message(buf);
            for (auto obj : scenegraph_->objects_) {
                obj->NotifyDeleted(i);
            }
            scenegraph_->UnlinkObject(i);
            i->Dispose();
            delete i;
        }
    }
    QueueSaveHistoryState();
}

bool MapEditor::IsSomethingSelected() {
    for (auto& object : scenegraph_->objects_) {
        if (object->Selected()) {
            return true;
        }
    }
    return false;
}

bool MapEditor::IsOneObjectSelected() {
    ReturnSelected(&selected);
    if (selected.size() != 1) {
        return false;
    }
    if (selected[0]->GetType() != _env_object) {
        return false;
    }
    return true;
}

void MapEditor::QueueSaveHistoryState() {
    save_countdown_ = kNumUpdatesBeforeSave;
}

void MapEditor::ApplyScriptParams(const ScriptParamMap& spm, int id) {
    if (id == LEVEL_PARAM_ID) {
        if (!testScriptParamsEqual(spm, scenegraph_->level->script_params().GetParameterMap())) {
            SceneGraph::ApplyScriptParams(scenegraph_, spm);
            QueueSaveHistoryState();
        }
    } else {
        Object* obj = scenegraph_->GetObjectFromID(id);
        if (obj) {
            const ScriptParamMap& new_spm = spm;
            ScriptParamMap::iterator iter;

            if (!testScriptParamsEqual(new_spm, obj->GetScriptParamMap())) {
                obj->SetScriptParams(new_spm);
                QueueSaveHistoryState();
            }
        }
    }
}

OGPalette* MapEditor::GetPalette(int id) {
    Object* obj = scenegraph_->GetObjectFromID(id);
    if (!obj) {
        DisplayError("Error", "Could not get palette of object because it doesn't exist.");
        return NULL;
    }
    return obj->GetPalette();
}

struct CollisionSortable {
    int id;
    float dist;
};

bool CollisionSortableCompare(const CollisionSortable& a, const CollisionSortable& b) {
    return a.dist < b.dist;
}

void MapEditor::HandleShortcuts(const LineSegment& mouseray) {
    Online* online = Online::Instance();

    const Keyboard& keyboard = Input::Instance()->getKeyboard();
    if (KeyCommand::CheckPressed(keyboard, KeyCommand::kEditScriptParams, KIMF_LEVEL_EDITOR_GENERAL)) {
        show_selected = !show_selected;
    }

    if (KeyCommand::CheckPressed(keyboard, KeyCommand::kSearchScenegraph, KIMF_LEVEL_EDITOR_GENERAL)) {
        select_scenegraph_search = true;
        show_scenegraph = true;
    }

    if (KeyCommand::CheckPressed(keyboard, KeyCommand::kScenegraph, KIMF_LEVEL_EDITOR_GENERAL)) {
        show_scenegraph = !show_scenegraph;
    }

    if (KeyCommand::CheckPressed(keyboard, KeyCommand::kEditColor, KIMF_LEVEL_EDITOR_GENERAL)) {
        // Color picking.
        show_color_picker = !show_color_picker;
    }

    if (KeyCommand::CheckPressed(keyboard, KeyCommand::kTogglePlayer, KIMF_LEVEL_EDITOR_GENERAL)) {
        ReturnSelected(&selected);
        bool one_character_selected = false;
        if (selected.size() == 1 && selected[0]->GetType() == _movement_object) {
            one_character_selected = true;
        }
        if (one_character_selected) {
            MovementObject* mo = (MovementObject*)(selected[0]);
            mo->is_player = !mo->is_player;
            QueueSaveHistoryState();

            if (online->IsHosting()) {
                if (!mo->is_player) {
                    online->RemoveFreeAvatarId(mo->GetID());
                } else {
                    online->AddFreeAvatarId(mo->GetID());
                }
            }
        }
    }
    // Handle switching editor type
    if (KeyCommand::CheckPressed(keyboard, KeyCommand::kToggleObjectEditing, KIMF_LEVEL_EDITOR_GENERAL)) {
        SetTypeEnabled(_env_object, !IsTypeEnabled(_env_object));
    }
    if (KeyCommand::CheckPressed(keyboard, KeyCommand::kToggleDecalEditing, KIMF_LEVEL_EDITOR_GENERAL)) {
        SetTypeEnabled(_decal_object, !IsTypeEnabled(_decal_object));
    }
    if (KeyCommand::CheckPressed(keyboard, KeyCommand::kToggleHotspotEditing, KIMF_LEVEL_EDITOR_GENERAL)) {
        SetTypeEnabled(_hotspot_object, !IsTypeEnabled(_hotspot_object));
    }
    if (KeyCommand::CheckPressed(keyboard, KeyCommand::kToggleLightEditing, KIMF_LEVEL_EDITOR_GENERAL)) {
        bool lights_enabled = !IsTypeEnabled(_dynamic_light_object);
        SetTypeEnabled(_dynamic_light_object, lights_enabled);
        SetTypeEnabled(_reflection_capture_object, lights_enabled);
        SetTypeEnabled(_light_volume_object, lights_enabled);
    }
    // Handle saves
    // This bool is needed because the new default keybind for "save level as"
    // is the old "save selected items", so to keep backward/forward compatibility
    // it needs to be able to have the same keybind, but override the old behaviour
    bool save_triggered = false;
    if (KeyCommand::CheckPressed(keyboard, KeyCommand::kSaveLevelAs, KIMF_LEVEL_EDITOR_GENERAL)) {
        SaveLevel(LevelLoader::kSaveAs);
        save_triggered = true;
    }
    if (KeyCommand::CheckPressed(keyboard, KeyCommand::kSaveLevel, KIMF_LEVEL_EDITOR_GENERAL)) {
        SaveLevel(LevelLoader::kSaveInPlace);
    }
    if (!save_triggered && KeyCommand::CheckPressed(keyboard, KeyCommand::kSaveSelectedItems, KIMF_LEVEL_EDITOR_GENERAL)) {
        KeyCommand::CheckPressed(keyboard, KeyCommand::kSaveLevelAs, KIMF_LEVEL_EDITOR_GENERAL);
        SaveSelected();
    }
    // handle undo/redo
    if (KeyCommand::CheckPressed(keyboard, KeyCommand::kRedo, KIMF_LEVEL_EDITOR_GENERAL)) {
        Redo();
    }
    if (KeyCommand::CheckPressed(keyboard, KeyCommand::kUndo, KIMF_LEVEL_EDITOR_GENERAL)) {
        Undo();
    }
    // handle copy/paste
    if (KeyCommand::CheckPressed(keyboard, KeyCommand::kCopy, KIMF_LEVEL_EDITOR_GENERAL)) {
        CopySelected();
    }
    if (KeyCommand::CheckPressed(keyboard, KeyCommand::kCut, KIMF_LEVEL_EDITOR_GENERAL)) {
        CutSelected();
    }
    if (KeyCommand::CheckPressed(keyboard, KeyCommand::kPaste, KIMF_LEVEL_EDITOR_GENERAL)) {
        Paste(mouseray);
    }
    // handle delete
    if (keyboard.wasScancodePressed(SDL_SCANCODE_DELETE, KIMF_LEVEL_EDITOR_GENERAL) || keyboard.wasScancodePressed(SDL_SCANCODE_BACKSPACE, KIMF_LEVEL_EDITOR_GENERAL)) {
        DeleteSelected();
    }
    if (KeyCommand::CheckPressed(keyboard, KeyCommand::kEnableImposter, KIMF_LEVEL_EDITOR_GENERAL)) {
        SetImposter(true);
    }
    if (KeyCommand::CheckPressed(keyboard, KeyCommand::kDisableImposter, KIMF_LEVEL_EDITOR_GENERAL)) {
        SetImposter(false);
    }
    const Mouse& mouse = Input::Instance()->getMouse();
    const bool kEnableControlClickAdd = false;
    if (kEnableControlClickAdd) {
        if (mouse.mouse_down_[Mouse::LEFT] == 1 && keyboard.isKeycodeDown(command_SDLK_key, KIMF_LEVEL_EDITOR_GENERAL) && !IsViewingNavMesh()) {
            Camera* ci = ActiveCameras::Get();
            vec3 start = ci->GetPos();
            vec3 end = ci->GetPos() + ci->GetMouseRay() * kMapEditorMouseRayLength;
            Collision c = scenegraph_->lineCheckCollidable(start, end);
            if (c.hit) {
                // Object* ppo = new ReflectionCaptureObject();
                Object* ppo = new LightVolumeObject();
                if (ActorsEditor_AddEntity(scenegraph_, ppo)) {
                    ppo->SetTranslation(c.hit_where + c.hit_normal * 0.7f);
                    QueueSaveHistoryState();
                } else {
                    LOGE << "Failed at adding LightVolumeObject to scenegraph" << std::endl;
                    delete ppo;
                }
            }
        }
    }
    // When holding ALT, see if should activate Connect/Disconnect tool
    bool connect = KeyCommand::CheckDown(keyboard, KeyCommand::kConnect, KIMF_LEVEL_EDITOR_GENERAL);
    bool disconnect = KeyCommand::CheckDown(keyboard, KeyCommand::kDisconnect, KIMF_LEVEL_EDITOR_GENERAL);
    if (active_tool_ != EditorTypes::CONNECT && (connect || disconnect) && !selected.empty()) {
        ReturnSelected(&selected);
        Object::ConnectionType connection_type = GetConnectionType(selected);
        if (connection_type != Object::kCTNone) {  // Disable connection if mouse is over object, so we can still alt-drag clone
            Collision mouseray_collision_selected = lineCheckActiveSelected(scenegraph_, mouseray.start, mouseray.end, NULL, NULL);
            for (auto& i : selected) {
                if (mouseray_collision_selected.hit_what == i) {
                    connection_type = Object::kCTNone;
                }
            }
        }
        if (connection_type != Object::kCTNone) {
            if (connect) {
                SetTool(EditorTypes::CONNECT);
            } else if (disconnect) {
                SetTool(EditorTypes::DISCONNECT);
            }
        }
    }
    // End connect/disconnect if alt is released
    if ((active_tool_ == EditorTypes::CONNECT || active_tool_ == EditorTypes::DISCONNECT) && !connect && !disconnect) {
        SetTool(EditorTypes::OMNI);
    }
    if (active_tool_ == EditorTypes::CONNECT && disconnect) {
        SetTool(EditorTypes::DISCONNECT);
    }
    if (active_tool_ == EditorTypes::DISCONNECT && !disconnect) {
        SetTool(EditorTypes::CONNECT);
    }
    if ((active_tool_ == EditorTypes::CONNECT || active_tool_ == EditorTypes::DISCONNECT) && mouse.mouse_down_[Mouse::LEFT] == 1) {
        Camera* ci = ActiveCameras::Get();
        vec3 start = ci->GetPos();
        vec3 end = ci->GetPos() + ci->GetMouseRay() * kMapEditorMouseRayLength;

        std::vector<Collision> collisions;
        scenegraph_->LineCheckAll(start, end, &collisions);
        std::vector<CollisionSortable> collision_sortable;
        collision_sortable.resize(collisions.size());
        for (int i = 0, len = (int)collisions.size(); i < len; ++i) {
            collision_sortable[i].dist = distance_squared(start, collisions[i].hit_where);
            collision_sortable[i].id = i;
        }
        std::sort(collision_sortable.begin(), collision_sortable.end(), CollisionSortableCompare);

        bool something_happened = false;
        for (size_t i = 0, len = collisions.size(); i < len && !something_happened; ++i) {
            Collision c = collisions[collision_sortable[i].id];
            if (c.hit) {
                Object* hit_obj = c.hit_what;
                if (std::find(box_objects.begin(), box_objects.end(), hit_obj) != box_objects.end()) {
                    ReturnSelected(&selected);
                    for (auto& i : selected) {
                        if (active_tool_ == EditorTypes::CONNECT) {
                            Object* obj = i;
                            if (obj->ConnectTo(*hit_obj)) {
                                something_happened = true;
                            }
                        } else if (active_tool_ == EditorTypes::DISCONNECT) {
                            Object* obj = i;
                            Object* hit_obj = c.hit_what;
                            if (obj->Disconnect(*hit_obj)) {
                                something_happened = true;
                            }
                        }
                    }
                    if (something_happened) {
                        DeselectAll(scenegraph_);
                        if (c.hit_what->permission_flags & Object::CAN_SELECT) {
                            c.hit_what->Select(true);
                        }
                        QueueSaveHistoryState();
                    } else if (c.hit_what->GetType() == _env_object || c.hit_what->GetType() == _terrain_type) {
                        something_happened = true;
                    }
                }
            }
        }
    }
    // Handle grouping
    if (KeyCommand::CheckPressed(keyboard, KeyCommand::kUngroup, KIMF_LEVEL_EDITOR_GENERAL)) {
        UngroupSelected();
    } else if (KeyCommand::CheckPressed(keyboard, KeyCommand::kGroup, KIMF_LEVEL_EDITOR_GENERAL)) {
        GroupSelected();
    }
    if (KeyCommand::CheckPressed(keyboard, KeyCommand::kKillSelected, KIMF_LEVEL_EDITOR_GENERAL)) {
        /*ReturnSelected(&selected);
        bool one_character_selected = false;
        if (selected.size() == 1 && selected[0]->GetType() == _movement_object){
            one_character_selected = true;
        }
        if (one_character_selected){
            MovementObject* mo = (MovementObject*)(selected[0]);
            mo->ToggleDead();
            QueueSaveHistoryState();
        }*/
    }

    if (KeyCommand::CheckPressed(keyboard, KeyCommand::kFrameSelectedForce, KIMF_LEVEL_EDITOR_GENERAL)) {
        CameraObject* co = ActiveCameras::Instance()->Get()->getCameraObject();
        if (co) {
            co->FrameSelection(false);
        }
    } else if (KeyCommand::CheckPressed(keyboard, KeyCommand::kFrameSelected, KIMF_LEVEL_EDITOR_GENERAL)) {
        CameraObject* co = ActiveCameras::Instance()->Get()->getCameraObject();
        if (co) {
            co->FrameSelection(true);
        }
    }
}

static bool MouseWasClickedThisTimestep(Mouse* mouse) {
    for (int i : mouse->mouse_down_) {
        if (i == Mouse::CLICKED) return true;
    }
    return false;
}

int MapEditor::ReplaceObjects(const std::vector<Object*>& objects, const std::string& replacement_path) {
    if (LoadEntitiesFromFile(replacement_path) != 0)
        return -1;

    ActorsEditor_UnlocalizeIDs(&add_desc_list_, scenegraph_);

    for (auto object : objects) {
        std::vector<Object*> new_objects = ActorsEditor_AddEntitiesAtPosition(add_desc_list_source_, scenegraph_, add_desc_list_, object->GetTranslation(), create_as_prefab_);

        if (!new_objects.empty()) {
            vec3 scale = object->GetScale();
            vec3 dims = object->box_.dims;
            vec3 target_dims = dims * scale;
            if (create_as_prefab_) {
                Prefab* prefab = static_cast<Prefab*>(new_objects[0]);
                vec3 prefab_scale = target_dims / prefab->original_scale_;

                prefab->SetScale(prefab->original_scale_ * prefab_scale);
                prefab->SetRotation(object->GetRotation());
                prefab->SetTranslation(object->GetTranslation());
                prefab->PropagateTransformsDown(true);
            } else {
                for (auto& new_object : new_objects) {
                    vec3 new_dims = new_object->box_.dims;
                    vec3 target_scale = target_dims / new_dims;

                    new_object->SetScale(target_scale);
                    new_object->SetRotation(object->GetRotation());
                    new_object->SetTranslation(object->GetTranslation());
                }
            }
        }

        RemoveObject(object, scenegraph_, true);
    }

    QueueSaveHistoryState();
    return 0;
}

void MapEditor::UpdateTools(const LineSegment& mouseray, GameCursor* cursor) {
    EditorTypes::Tool t;
    if (active_tool_ == EditorTypes::OMNI) {
        t = omni_tool_tool_;
    } else {
        t = active_tool_;
    }
    Collision mouseray_collision_selected = lineCheckActiveSelected(scenegraph_, mouseray.start, mouseray.end, NULL, NULL);
    switch (t) {
        case EditorTypes::ADD_ONCE: {
            Mouse* mouse = &(Input::Instance()->getMouse());
            if (mouse->mouse_down_[Mouse::LEFT] == Mouse::CLICKED) {
                ActorsEditor_UnlocalizeIDs(&add_desc_list_, scenegraph_);
                ActorsEditor_AddEntitiesAtMouse(add_desc_list_source_, scenegraph_, add_desc_list_, mouseray, create_as_prefab_);
                QueueSaveHistoryState();
                SetTool(EditorTypes::OMNI);
            }
            break;
        }
        case EditorTypes::TRANSLATE:
        case EditorTypes::SCALE:
        case EditorTypes::ROTATE:
            UpdateTransformTool(scenegraph_, t, mouseray, mouseray_collision_selected, cursor);
            break;
        default:
            // mjh - what is a reasonable thing to do here?
            break;
    }
}

bool MapEditor::HandleScrollSelect(const vec3& start, const vec3& end) {
    bool something_happened = false;

    // Check scroll direction
    int scrolled = 0;
    if (Input::Instance()->getMouse().wheel_delta_y_ < 0) {
        scrolled = -1;
    } else if (Input::Instance()->getMouse().wheel_delta_y_ > 0) {
        scrolled = 1;
    }
    if (scrolled == 0) {
        return false;
    }

    // Get sorted list of collisions with mouse ray
    std::vector<Collision> collisions;
    GetEditorLineCollisions(scenegraph_, start, end, collisions, type_enable_);
    if (collisions.empty()) {
        return false;
    }
    std::sort(collisions.begin(), collisions.end(), CollisionCompare(start));

    for (int i = 0; i < (int)collisions.size();) {
        if (collisions[i].hit_what->GetType() == _group || collisions[i].hit_what->GetType() == _prefab) {
            collisions.erase(collisions.begin() + i);
        } else if (collisions[i].hit_what->parent &&
                   (collisions[i].hit_what->parent->GetType() == _group ||
                    collisions[i].hit_what->parent->GetType() == _prefab)) {
            int depth = 1;
            while (collisions[i].hit_what->parent &&
                   (collisions[i].hit_what->parent->GetType() == _group ||
                    collisions[i].hit_what->parent->GetType() == _prefab)

            ) {
                bool is_duplicate = false;
                for (int j = 0; j < i; ++j) {  // Don't allow same group to be in list twice
                    if (collisions[i].hit_what->parent == collisions[j].hit_what) {
                        is_duplicate = true;
                        break;
                    }
                }
                if (!is_duplicate) {
                    ++depth;
                    collisions.insert(collisions.begin() + i, collisions[i]);
                    collisions[i].hit_what = collisions[i].hit_what->parent;
                } else {
                    break;
                }
            }
            i += depth;
        } else {
            ++i;
        }
    }

    // Get selected item in collision list
    int selected_item = -1;
    for (int i = 0, len = (int)collisions.size(); i < len; ++i) {
        Object* obj = collisions[i].hit_what;
        if (obj->Selected()) {
            selected_item = i;
            break;
        }
    }

    // Move selection to next selectable item in list according to scroll direction
    if (selected_item != -1) {
        int next_selectable = -1;
        for (int i = selected_item + scrolled, len = (int)collisions.size(); i >= 0 && i < len; i += scrolled) {
            Object* obj = collisions[i].hit_what;
            if (obj->permission_flags & Object::CAN_SELECT) {
                next_selectable = i;
                break;
            }
        }
        if (next_selectable != -1) {
            DeselectAll(scenegraph_);
            collisions[next_selectable].hit_what->Select(true);
            DrawObjInfo(collisions[next_selectable].hit_what);
        }
    }

    return something_happened;
}

static Collision GetSelectableInLineSegment(SceneGraph* scenegraph, const LineSegment& l, const TypeEnable& type_enable) {
    std::vector<Collision> collisions;
    GetEditorLineCollisions(scenegraph, l.start, l.end, collisions, type_enable);
    if (!collisions.empty()) {
        return collisions[0];
    } else {
        return Collision();
    }
}

void CalculateGroupString(Object* obj, std::string& str) {
    if (obj->GetType() == _group || obj->GetType() == _prefab) {
        Group* group = (Group*)obj;
        for (auto& i : group->children) {
            CalculateGroupString(i.direct_ptr, str);
        }
    } else {
        str = str + obj->obj_file;
    }
}

void MapEditor::SelectAll() {
    for (auto obj : scenegraph_->objects_) {
        EntityType type = obj->GetType();
        if (type_enable_.IsTypeEnabled(type) && obj->permission_flags & Object::CAN_SELECT && !obj->parent) {
            obj->Select(true);
        }
    }
}

void MapEditor::ReloadAllPrefabs() {
    std::vector<Object*> prefabs;
    int current_depth = 1;
    bool active_depth = true;

    while (active_depth) {
        active_depth = false;
        prefabs.clear();
        for (auto obj : scenegraph_->objects_) {
            if (obj->GetGroupDepth() == current_depth) {
                active_depth = true;
                if (obj->GetType() == _prefab) {
                    prefabs.push_back(obj);
                }
            }
        }

        for (auto& prefab : prefabs) {
            ReloadPrefab(prefab, GetSceneGraph());
        }
        current_depth++;
    }
}

void MapEditor::ReloadPrefabs(const Path& path) {
    std::vector<Object*> prefabs;
    for (auto obj : scenegraph_->objects_) {
        if (obj->GetType() == _prefab) {
            Prefab* prefab = static_cast<Prefab*>(obj);
            if (strmtch(prefab->prefab_path.GetFullPath(), path.GetFullPath())) {
                prefabs.push_back(obj);
            }
        }
    }

    for (auto& prefab : prefabs) {
        ReloadPrefab(prefab, GetSceneGraph());
    }
}

bool MapEditor::ReloadPrefab(Object* obj, SceneGraph* scenegraph) {
    if (obj->GetType() != _prefab) {
        return false;
    }

    Prefab* prefab = static_cast<Prefab*>(obj);

    if (prefab->prefab_locked == false) {
        return false;
    }

    if (prefab->prefab_path.isValid() == false) {
        return false;
    }

    if (FileExists(prefab->prefab_path) == false) {
        return false;
    }
    std::vector<Object*> ret_children;

    prefab->GetBottomUpCompleteChildren(&ret_children);

    for (auto& i : ret_children) {
        RemoveObject(i, scenegraph, true);
    }

    EntityDescriptionList desc_list;
    std::string file_type;
    ActorsEditor_LoadEntitiesFromFile(prefab->prefab_path, desc_list, &file_type);

    if (file_type != "prefab") {
        return false;
    }

    ActorsEditor_AddEntitiesIntoPrefab(prefab, scenegraph, desc_list);

    return true;
}

bool MapEditor::CheckForSelections(const LineSegment& mouseray) {
    bool something_happened = false;
    const Keyboard& keyboard = Input::Instance()->getKeyboard();
    Mouse* mouse = &(Input::Instance()->getMouse());
    if (!box_selector_.acting && state_ == MapEditor::kIdle) {
        if (mouse->mouse_down_[Mouse::LEFT] && (mouse->mouse_down_[Mouse::RIGHT] || KeyCommand::CheckDown(keyboard, KeyCommand::kBoxSelect, KIMF_LEVEL_EDITOR_GENERAL))) {
            // Start box select
            box_selector_.acting = true;
            state_ = MapEditor::kBoxSelectDrag;
            if (ActiveCameras::Get()->m_camera_object != NULL) {
                ActiveCameras::Get()->m_camera_object->IgnoreMouseInput(true);
            }
            Mouse* mouse = &(Input::Instance()->getMouse());
            box_selector_.points[0][0] = (float)mouse->pos_[0];
            box_selector_.points[0][1] = (float)(Graphics::Instance()->window_dims[1] - mouse->pos_[1]);
            box_selector_.points[1][0] = box_selector_.points[0][0];
            box_selector_.points[1][1] = box_selector_.points[0][1];
            something_happened = true;
        }
    } else if (box_selector_.acting) {
        if (mouse->mouse_down_[Mouse::LEFT] && (mouse->mouse_down_[Mouse::RIGHT] || KeyCommand::CheckDown(keyboard, KeyCommand::kBoxSelect, KIMF_LEVEL_EDITOR_GENERAL))) {
            // Update box select
            Mouse* mouse = &(Input::Instance()->getMouse());
            box_selector_.points[1][0] = (float)mouse->pos_[0];
            box_selector_.points[1][1] = (float)(Graphics::Instance()->window_dims[1] - mouse->pos_[1]);
            something_happened = true;
        } else {
            // End box select
            const Keyboard& keyboard = Input::Instance()->getKeyboard();
            bool holding_shift = KeyCommand::CheckDown(keyboard, KeyCommand::kAddToSelection, KIMF_LEVEL_EDITOR_GENERAL);
            if (!holding_shift) {
                DeselectAll(scenegraph_);
            }
            // Make the selections
            Camera* cam = ActiveCameras::Get();
            if (box_selector_.points[0][0] > box_selector_.points[1][0]) {
                std::swap(box_selector_.points[0][0], box_selector_.points[1][0]);
            }
            if (box_selector_.points[0][1] < box_selector_.points[1][1]) {
                std::swap(box_selector_.points[0][1], box_selector_.points[1][1]);
            }
            vec4 p1 = cam->UnProjectPixel((int)box_selector_.points[0][0], (int)box_selector_.points[0][1]);
            vec4 p2 = cam->UnProjectPixel((int)box_selector_.points[1][0], (int)box_selector_.points[0][1]);
            vec4 p3 = cam->UnProjectPixel((int)box_selector_.points[1][0], (int)box_selector_.points[1][1]);
            vec4 p4 = cam->UnProjectPixel((int)box_selector_.points[0][0], (int)box_selector_.points[1][1]);
            vec4 d1 = p2 - p1;
            vec4 d2 = p3 - p2;
            vec4 d3 = p4 - p3;
            vec4 d4 = p1 - p4;
            if (length(d1) != 0 && length(d2) != 0 && length(d3) != 0 && length(d4) != 0) {
                vec4 r1 = cam->GetRayThroughPixel((int)box_selector_.points[0][0], (int)box_selector_.points[0][1]);
                vec4 r2 = cam->GetRayThroughPixel((int)box_selector_.points[1][0], (int)box_selector_.points[1][1]);
                vec4 n1 = cross(r1, d1);
                vec4 n2 = cross(r2, d2);
                vec4 n3 = cross(r2, d3);
                vec4 n4 = cross(r1, d4);
                BoxSelectEntities(scenegraph_, type_enable_, mouseray, p1, p3, r1, r2, n1, n2, n3, n4, holding_shift);
            }
            if (ActiveCameras::Get()->m_camera_object != NULL) {
                ActiveCameras::Get()->m_camera_object->IgnoreMouseInput(false);
            }
            box_selector_.acting = false;
            state_ = MapEditor::kIdle;
            something_happened = true;
        }
    }
    if (KeyCommand::CheckPressed(keyboard, KeyCommand::kDeselectAll, KIMF_LEVEL_EDITOR_GENERAL)) {
        DeselectAll(scenegraph_);
        something_happened = true;
    }
    if (KeyCommand::CheckPressed(keyboard, KeyCommand::kSelectSimilar, KIMF_LEVEL_EDITOR_GENERAL)) {
        std::vector<std::string> selected_string;
        for (auto obj : scenegraph_->objects_) {
            if (obj->GetType() == _group || obj->GetType() == _prefab) {
                CalculateGroupString(obj, obj->obj_file);
            }
        }
        for (auto obj : scenegraph_->objects_) {
            if (obj->Selected()) {
                selected_string.push_back(obj->obj_file);
            }
        }
        for (auto obj : scenegraph_->objects_) {
            if (!obj->Selected() && obj->permission_flags & Object::CAN_SELECT) {
                const std::string& to_string = obj->obj_file;
                for (auto& j : selected_string) {
                    if (to_string == j) {
                        obj->Select(true);
                        break;
                    }
                }
            }
        }
        something_happened = true;
    }
    if (KeyCommand::CheckPressed(keyboard, KeyCommand::kSelectAll, KIMF_LEVEL_EDITOR_GENERAL)) {
        SelectAll();
        something_happened = true;
    }
    // handle double-click select
    if (mouse->mouse_double_click_[Mouse::LEFT] && KeyCommand::CheckDown(keyboard, KeyCommand::kAddToSelection, KIMF_LEVEL_EDITOR_GENERAL)) {
        Collision c = GetSelectableInLineSegment(scenegraph_, mouseray, type_enable_);
        if (c.hit) {
            c.hit_what->Select(!c.hit_what->Selected());
            if (c.hit_what->Selected()) {
                DrawObjInfo(c.hit_what);
            }
        }
        something_happened = true;
    } else if (mouse->mouse_double_click_[Mouse::LEFT]) {
        DeselectAll(scenegraph_);
        Collision c = GetSelectableInLineSegment(scenegraph_, mouseray, type_enable_);
        if (c.hit) {
            c.hit_what->Select(true);
            DrawObjInfo(c.hit_what);
        }
        something_happened = true;
    }
    // handle group scroll select
    something_happened = something_happened || HandleScrollSelect(mouseray.start, mouseray.end);
    return something_happened;
}

void MapEditor::DeselectAll(SceneGraph* scenegraph) {
    for (auto& object : scenegraph->objects_) {
        object->Select(false);
    }
}

static float matrix_distance_squared(const mat4& a, const mat4& b) {
    float total = 0.0f;
    for (unsigned i = 0; i < 16; i++) {
        total += square(a.entries[i] - b.entries[i]);
    }
    return total;
}

static float ClosestMatch(const mat4& matrix, const std::vector<mat4>& matrices) {
    float closest_distance = 99999.0f;
    int closest = -1;
    float test_distance;
    for (unsigned i = 0; i < matrices.size(); ++i) {
        test_distance = matrix_distance_squared(matrix, matrices[i]);
        if (closest == -1 || test_distance < closest_distance) {
            closest_distance = test_distance;
            closest = i;
        }
    }
    return closest_distance;
}

bool IDCompare(const Object* a, const Object* b) {
    return a->GetID() < b->GetID();
}

void MapEditor::SaveEntities(TiXmlNode* root) {
    sky_editor_->SaveSky(root);

    TiXmlElement* objects = new TiXmlElement("ActorObjects");
    root->LinkEndChild(objects);
    std::vector<mat4> matrices;
    const float _difference_threshold = 0.000001f;
    SceneGraph::object_list ordered_objects = scenegraph_->objects_;
    std::sort(ordered_objects.begin(), ordered_objects.end(), IDCompare);
    for (auto obj : ordered_objects) {
        // Only save object if it has no parent and is not an exact duplicate of another object at same transform
        if (!obj->parent && !obj->exclude_from_save) {
            if (ClosestMatch(obj->GetTransform(), matrices) <= _difference_threshold) {
                LOGW << "The saved " << obj << " is placed exactly on top of another object, is this intentional? I'm still saving the object." << std::endl;
            }

            obj->SaveToXML(objects);
            matrices.push_back(obj->GetTransform());
            LOGD << "Saved object " << obj << " to level xml file" << std::endl;
        } else {
            if (obj->exclude_from_save) {
                LOGW << "Skipping save of object " << obj << " because it's explicitly excluded from saving." << std::endl;
            } else {
                LOGW << "Skipping save of objects " << obj << " into level because it has a parent object." << std::endl;
            }
        }

        if (obj->GetType() == _reflection_capture_object) {
            char save_path[kPathSize];
            if (scenegraph_->level_path_.source != kAbsPath) {
                if (config["allow_game_dir_save"].toBool()) {
                    FormatString(save_path, kPathSize, "%s_refl_cap_%d", scenegraph_->level_path_.GetOriginalPath(), obj->GetID());
                } else {
                    FormatString(save_path, kPathSize, "%s%s_refl_cap_%d", GetWritePath(scenegraph_->level_path_.mod_source).c_str(), scenegraph_->level_path_.GetOriginalPath(), obj->GetID());
                }
            } else {
                FormatString(save_path, kPathSize, "%s_refl_cap_%d", scenegraph_->level_path_.GetOriginalPath(), obj->GetID());
            }
            CreateParentDirs(save_path);
            ReflectionCaptureObject* rco = (ReflectionCaptureObject*)obj;
            if (rco->cube_map_ref.valid()) {
                Textures::SaveCubeMapMipmapsHDR(rco->cube_map_ref, save_path, Textures::Instance()->getWidth(rco->cube_map_ref));
            }
        }
    }
}

void MapEditor::SetUpSky(const SkyInfo& si) {
    sky_editor_->ApplySkyInfo(si);
}

int MapEditor::LoadEntitiesFromFile(const std::string& filename) {
    add_desc_list_.clear();
    add_desc_list_source_ = Path();

    std::string file_type;
    Path source;
    if (ActorsEditor_LoadEntitiesFromFile(filename, add_desc_list_, &file_type, &source) == -1) {
        return -1;
    }
    add_desc_list_source_ = source;

    if (file_type == "prefab") {
        create_as_prefab_ = true;
    } else {
        create_as_prefab_ = false;
    }

    // Files saved with new LocalizeIDs function will have IDs in the range [0, <itemcount>)
    // But old files are [X, Y], so this is needed
    LocalizeIDs(&add_desc_list_, false);
    // for(EntityDescriptionList::iterator iter=add_desc_list_.begin(); iter!=add_desc_list_.end(); ++iter){
    //     EntityDescription* desc = &(*iter);
    //     ClearDescID(desc);
    // }
    return 0;
}

void MapEditor::SavePrefab(bool do_overwrite) {
    char buf[kPathSize];
    std::vector<Object*> selected;

    scenegraph_->ReturnSelected(&selected);
    if (selected.size() > 1) {
        int err = PrefabSelected();
        if (err == 0) {
            SavePrefab(false);
        } else if (err == 1) {
            ShowEditorMessage(3, "Selected object is already a prefab.");
        } else if (err == 2) {
            ShowEditorMessage(3, "Selected objects contain a prefab, prefabs can't contain prefabs.");
        } else if (err == 3) {
            ShowEditorMessage(3, "Selected group object has a parent, orphan it first.");
        } else if (err == 4) {
            ShowEditorMessage(3, "All selected objects already have a parent.");
        } else if (err == 5) {
            ShowEditorMessage(3, "Unknown error");
        } else if (err == 6) {
            ShowEditorMessage(3, "Error adding prefab to scenegraph");
        }
        // Always return, because we re-call if we do ok_
        return;
    }

    if (selected.size() == 0) {
        ShowEditorMessage(3, "Nothing selected");
        return;
    }

    Object* object = selected[0];

    if (object->GetType() != _prefab) {
        int err = PrefabSelected();
        if (err == 0) {
            SavePrefab(false);
        } else if (err == 1) {
            ShowEditorMessage(3, "Selected object is already a prefab.");  // will not happen given context
        } else if (err == 2) {
            ShowEditorMessage(3, "Selected objects contain a prefab, prefabs can't contain prefabs.");
        } else if (err == 3) {
            ShowEditorMessage(3, "Selected group object has a parent, orphan it first.");
        } else if (err == 4) {
            ShowEditorMessage(3, "All selected objects already have a valid parent.");
        } else if (err == 5) {
            ShowEditorMessage(3, "Unknown error");
        } else if (err == 6) {
            ShowEditorMessage(3, "Error adding prefab to scenegraph");
        }
        return;
    }
    Prefab* group = static_cast<Prefab*>(object);

    if (group->GetRotation() != quaternion()) {
        ShowEditorMessage(3, "Can't save prefab that's rotated, this is a current limitation to prevent confusion. Go into Selected window and reset rotation to enable saving.");
        return;
    }

    Dialog::DialogErr err = Dialog::NO_ERR;
    if (FileExists(group->prefab_path) && do_overwrite) {
        strscpy(buf, group->prefab_path.GetFullPath(), kPathSize);
    } else {
        err = Dialog::writeFile("xml", 1, "Data/Objects", buf, kPathSize);
    }
    if (!err) {
        TiXmlDocument doc(buf);
        // write xml declaration header
        TiXmlDeclaration* decl = new TiXmlDeclaration("2.0", "", "");
        doc.LinkEndChild(decl);
        // write file type
        TiXmlElement* version = new TiXmlElement("Type");
        doc.LinkEndChild(version);
        version->LinkEndChild(new TiXmlText("prefab"));
        // write out entities
        TiXmlElement* parent = new TiXmlElement("Prefab");
        doc.LinkEndChild(parent);

        EntityDescriptionList descriptions;
        for (auto& i : group->children) {
            const ScriptParamMap& script_param_map = i.direct_ptr->GetScriptParams()->GetParameterMap();
            const ScriptParamMap::const_iterator it = script_param_map.find("No Save");
            if (it != script_param_map.end()) {
                const ScriptParam& param = it->second;
                if (param.GetInt() == 1) {
                    continue;
                }
            }

            EntityDescription desc;
            i.direct_ptr->GetDesc(desc);
            descriptions.push_back(desc);
        }
        LocalizeIDs(&descriptions, false);

        for (auto& description : descriptions) {
            description.SaveToXML(parent);
            // group->children[i].direct_ptr->SaveToXML(parent);
        }
        doc.SaveFile(buf);

        group->prefab_path = FindShortestPath2(buf);
        group->original_scale_ = group->GetScale();  // Reset original_scale_, as the original_scale_ is used to figure out relative scale to prefab link.
        group->prefab_locked = true;

        ReloadPrefabs(group->prefab_path);
    }
}

void MapEditor::SaveSelected() {
    const int BUF_SIZE = 512;
    char buf[BUF_SIZE];
    Dialog::DialogErr err = Dialog::writeFile("xml", 1, "Data/Objects", buf, BUF_SIZE);
    if (!err) {
        TiXmlDocument doc(buf);
        // write xml declaration header
        TiXmlDeclaration* decl = new TiXmlDeclaration("2.0", "", "");
        doc.LinkEndChild(decl);
        // write file type
        TiXmlElement* version = new TiXmlElement("Type");
        doc.LinkEndChild(version);
        version->LinkEndChild(new TiXmlText("saved"));
        // write out entities
        TiXmlElement* parent = new TiXmlElement("ActorObjects");
        doc.LinkEndChild(parent);
        for (auto obj : scenegraph_->objects_) {
            if (obj->Selected()) {
                obj->SaveToXML(parent);
            }
        }
        doc.SaveFile(buf);
    }
}

bool MapEditor::CanUndo() {
    if (state_ != MapEditor::kIdle || state_history_.current_state <= state_history_.start_state || Online::Instance()->IsActive())
        return false;
    return true;
}

namespace {
typedef std::map<int, SavedChunk*> Time_ChunkMap;
typedef std::map<int, Time_ChunkMap> ID_Time_ChunkMap;
typedef std::map<ChunkType::ChunkType, ID_Time_ChunkMap> Type_ID_Time_ChunkMap;

/* Disabled because unused.
*
void GetObjectsInState(const Type_ID_Time_ChunkMap &save_states, int state, std::vector<SavedChunk*> &vec){
    for(Type_ID_Time_ChunkMap::const_iterator iter = save_states.begin();
        iter != save_states.end(); ++iter)
    {
        const ID_Time_ChunkMap &id_time = iter->second;
        for(ID_Time_ChunkMap::const_iterator iter = id_time.begin();
            iter != id_time.end(); ++iter)
        {
            const Time_ChunkMap &time_chunk = iter->second;
            Time_ChunkMap::const_iterator iter2 = time_chunk.find(state);
            if(iter2 != time_chunk.end()){
                vec.push_back(iter2->second);
            }
        }
    }
}
*/
}  // namespace

void MapEditor::BringAllToCurrentState(int old_state) {
    // Create map for easy saved-chunk lookup
    Type_ID_Time_ChunkMap save_states;
    // Loop through saved chunks to populate lookup map
    for (auto& chunk : state_history_.chunks) {
        save_states[chunk.type][chunk.obj_id][chunk.state_id] = &chunk;
    }

    // Remove all objects with editors
    std::vector<int> selected_ids;
    int index = 0;
    while (index < (int)scenegraph_->objects_.size()) {
        Object* obj = scenegraph_->objects_[index];
        if (obj->Selected()) {
            selected_ids.push_back(obj->GetID());
        }
        if (!obj->exclude_from_undo) {
            RemoveObject(scenegraph_->objects_[index], scenegraph_, true);
        } else {
            ++index;
        }
    }
    // Loop through each chunk and set each object to the most appropriate saved chunk
    for (auto& save_state : save_states) {
        const ChunkType::ChunkType& type = save_state.first;
        ID_Time_ChunkMap& submap = save_state.second;
        for (auto& iter : submap) {
            SavedChunk* the_chunk = NULL;
            Time_ChunkMap& timeline = iter.second;
            // Find the last entry that is before the current time
            for (auto& iter2 : timeline) {
                const int& time = iter2.first;
                SavedChunk* chunk = iter2.second;
                if (time == state_history_.current_state) {
                    the_chunk = chunk;
                }
            }
            // Apply that entry to the appropriate object or editor
            if (the_chunk) {
                switch (type) {
                    case ChunkType::SKY_EDITOR:
                        if (sky_editor_->ReadChunk(*the_chunk)) {
                            scenegraph_->sky->LightingChanged(scenegraph_->terrain_object_ != NULL);
                        }
                        break;
                    case ChunkType::LEVEL:
                        scenegraph_->level->ReadChunk(*the_chunk);
                        break;
                    default:
                        Object* o = CreateObjectFromDesc(the_chunk->desc);
                        if (o) {
                            if (ActorsEditor_AddEntity(scenegraph_, o, NULL, false, true)) {
                            } else {
                                LOGE << "Failed at adding object to scenegraph" << std::endl;
                            }
                        } else {
                            LOGE << "Failed at constructing object" << std::endl;
                        }
                        break;
                }
            }
        }
    }

    for (auto obj : scenegraph_->objects_) {
        static const int kBufSize = 256;
        char msg[kBufSize];
        FormatString(msg, kBufSize, "added_object %d", obj->GetID());
        scenegraph_->level->Message(msg);
    }

    if (g_no_reflection_capture == false) {
        scenegraph_->LoadReflectionCaptureCubemaps();
    }

    scenegraph_->SendMessageToAllObjects(OBJECT_MSG::FINALIZE_LOADED_CONNECTIONS);

    // Restore selection
    for (int selected_id : selected_ids) {
        Object* obj = scenegraph_->GetObjectFromID(selected_id);
        if (obj) {
            obj->Select(true);
        }
    }
}

void MapEditor::Undo() {
    if (terrain_preview_mode) {
        DisplayMessage("Cannot undo", "It is not possible to undo while previewing terrain");
        return;
    }
    RibbonFlash(gui, "undo");
    if (!CanUndo()) {
        return;
    }

    int old_state = state_history_.current_state;
    state_history_.current_state--;
    BringAllToCurrentState(old_state);
    scenegraph_->UpdatePhysicsTransforms();
    Graphics* graphics = Graphics::Instance();
    if (state_history_.current_state <= graphics->nav_mesh_out_of_date_chunk) {
        graphics->nav_mesh_out_of_date = false;
    }
}

bool MapEditor::CanRedo() {
    if (state_ != MapEditor::kIdle || state_history_.current_state >= state_history_.num_states - 1 || Online::Instance()->IsActive())
        return false;
    return true;
}

void MapEditor::Redo() {
    if (terrain_preview_mode) {
        DisplayMessage("Cannot redo", "It is not possible to redo while previewing terrain");
        return;
    }
    RibbonFlash(gui, "redo");
    if (CanRedo()) {
        int old_state = state_history_.current_state;
        state_history_.current_state++;
        BringAllToCurrentState(old_state);
        scenegraph_->UpdatePhysicsTransforms();
    }
}

void MapEditor::SetViewNavMesh(bool enabled) {
    scenegraph_->SetNavMeshVisible(enabled);
}

void MapEditor::SetViewCollisionNavMesh(bool enabled) {
    scenegraph_->SetCollisionNavMeshVisible(enabled);
}

bool MapEditor::IsViewingNavMesh() {
    return scenegraph_->IsNavMeshVisible();
}

bool MapEditor::IsViewingCollisionNavMesh() {
    return scenegraph_->IsCollisionNavMeshVisible();
}

void MapEditor::AddLightProbes() {
    std::vector<Object*> selected;
    ReturnSelected(&selected);
    if (selected.size() == 1 && selected[0]->GetType() == _placeholder_object) {
        // Add probes to selected area
        vec3 scale = selected[0]->box_.dims * selected[0]->GetScale() * 0.5f;
        vec3 translation = selected[0]->GetTranslation();
        quaternion rotation = selected[0]->GetRotation();
        PlaceLightProbes(scenegraph_, translation, rotation, scale);
    } else {
        // TODO: Remove existing probes
        /*
        if (!scenegraph_->light_probe_collection.light_probes.empty()) {
            for (int i = 0, len = scenegraph_->objects_.size(); i < len; ++i) {
                if (dynamic_cast<LightProbeObject*>(scenegraph_->objects_[i]) != NULL) {
                    RemoveObject(scenegraph_->objects_[i], scenegraph_);
                }
            }
        }
        */
        // Cover entire level with probes
        vec3 minval = vec3(FLT_MAX, FLT_MAX, FLT_MAX);
        vec3 maxval = vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        for (auto& visible_object : scenegraph_->visible_objects_) {
            vec3 objtrans = visible_object->GetTranslation();
            maxval.x() = max(maxval.x(), objtrans.x());
            minval.x() = min(minval.x(), objtrans.x());
            maxval.y() = max(maxval.y(), objtrans.y());
            minval.y() = min(minval.y(), objtrans.y());
            maxval.z() = max(maxval.z(), objtrans.z());
            minval.z() = min(minval.z(), objtrans.z());
        }

        vec3 translation = (maxval + minval) / 2.0f;
        quaternion rotation = quaternion();
        vec3 scale = 0.6f * (maxval - minval);  // Some margin on edges
        PlaceLightProbes(scenegraph_, translation, rotation, scale);
    }
}

void MapEditor::BakeLightProbes(int pass) {
    if (!scenegraph_->light_probe_collection.light_probes.empty()) {
        LightProbeCollection& light_probes = scenegraph_->light_probe_collection;
        for (size_t i = 0, len = light_probes.light_probes.size(); i < len; ++i) {
            LightProbeUpdateEntry entry;
            entry.id = light_probes.light_probes[i].id;
            entry.pass = pass;
            light_probes.to_process.push(entry);
        }
        if (pass == 1) {
            kLightProbe2pass = true;
        }
    }
}

void MapEditor::ToggleImposter() {
    SendMessageToSelectedObjects(scenegraph_, OBJECT_MSG::TOGGLE_IMPOSTER);
}

void MapEditor::SetImposter(bool set) {
    ReturnSelected(&selected);
    for (auto& i : selected) {
        i->SetImposter(set);
    }
}

Object* MapEditor::AddEntityFromDesc(SceneGraph* scenegraph, const EntityDescription& desc, bool level_loading) {
    if (desc.fields[0].type != EDF_ENTITY_TYPE) {
        DisplayError("Error", "No type info given to MapEditor::AddEntityFromDesc");
        return NULL;
    }
    Object* o = CreateObjectFromDesc(desc);
    if (o) {
        if (ActorsEditor_AddEntity(scenegraph, o, NULL, false, level_loading)) {
            return o;
        } else {
            LOGE << "Failed adding entity from desc to scenegraph" << std::endl;
            delete o;
            return NULL;
        }
    } else {
        return NULL;
    }
}

void MapEditor::ApplyPalette(const OGPalette& palette, int edited_id) {
    Object* obj = scenegraph_->GetObjectFromID(edited_id);
    if (!obj || obj->GetType() != _movement_object) {
        return;
    }
    obj->ApplyPalette(palette);
}

void MapEditor::ReceiveMessage(const std::string& msg) {
    if (msg == "DisplaySettings") {
    } else if (msg == "carve_against_terrain") {
    }
}

void MapEditor::LoadDialogueFile(const std::string& path) {
    if (state_ == MapEditor::kIdle && LoadEntitiesFromFile("Data/Objects/placeholder/empty_placeholder.xml") == 0) {
        ScriptParams sp;
        sp.ASAddString("Dialogue", path);  // "Dialogue" is used in dimgui.cpp, change it there as well if changed here
        add_desc_list_[0].AddScriptParams(EDF_SCRIPT_PARAMS, sp.GetParameterMap());
        SetTool(EditorTypes::ADD_ONCE);
    }
}

// Check if we've performed an action that warrants a new save (meaning action that effects the undo stack).
bool MapEditor::WasLastSaveOnCurrentUndoChunk() {
    std::list<SavedChunk>::iterator chunkit = state_history_.GetCurrentChunk();

    if (chunkit != state_history_.chunks.end()) {
        return chunkit->GetGlobalID() == last_saved_chunk_global_id;
    } else {
        // We assume that no chunks means we haven't opened the editor because that will create a base undo object.
        return true;
    }
}

void MapEditor::SetLastSaveOnCurrentUndoChunk() {
    LOGI << "Reseting undo block value" << std::endl;
    // Remember what undo position we last saved in
    std::list<SavedChunk>::iterator chunkit = state_history_.GetCurrentChunk();
    if (chunkit != state_history_.chunks.end()) {
        last_saved_chunk_global_id = chunkit->GetGlobalID();
        LOGI << "Reseting undo block value to " << chunkit->GetGlobalID() << std::endl;
    } else {
        last_saved_chunk_global_id = 0;  // Counter starts as 1, so 0 will always mismatch.
    }
}

void MapEditor::SaveLevel(LevelLoader::SaveLevelType type) {
    if (terrain_preview_mode) {
        DisplayMessage("Cannot save level", "It is not possible to save while previewing terrain");
        return;
    }

    scenegraph_->level->Message("save_selected_dialogue");

    LevelLoader::SaveLevel(*scenegraph_, type);

    SetLastSaveOnCurrentUndoChunk();

    scenegraph_->level->setMetaDataClean();

    if (scenegraph_->level_path_.isValid() && scenegraph_->level_has_been_previously_saved_) {
        Engine::Instance()->AddLevelPathToRecentLevels(scenegraph_->level_path_);
    }
}

int MapEditor::CreateObject(const std::string& path) {
    int id = -1;
    EntityDescriptionList desc_list;
    std::string file_type;
    Path source;
    ActorsEditor_LoadEntitiesFromFile(path, desc_list, &file_type, &source);
    for (auto& i : desc_list) {
        Object* obj = AddEntityFromDesc(scenegraph_, i, false);
        if (obj) {
            id = obj->GetID();
        } else {
            LOGE << "Failed at creating part of object " << path << std::endl;
        }
    }
    QueueSaveHistoryState();
    return id;
}

void MapEditor::ReturnSelected(std::vector<Object*>* selected_objects) {
    selected_objects->clear();
    for (auto& object : scenegraph_->objects_) {
        if (object->Selected()) {
            selected_objects->push_back(object);
        }
    }
}

void MapEditor::DeleteID(int val) {
    LOGD << "Deleting id: " << val << std::endl;
    Object* obj = scenegraph_->GetObjectFromID(val);
    Camera* active_camera = ActiveCameras::Get();
    if (obj && obj->GetType() == _camera_type) {
        LOGE << "Tried deleting camera object" << std::endl;
    } else if (obj) {
        RemoveObject(obj, scenegraph_, true);
    }
}

bool MapEditor::IsTypeEnabled(EntityType type) {
    return type_enable_.IsTypeEnabled(type);
}

void MapEditor::SetTypeEnabled(EntityType type, bool enabled) {
    type_enable_.SetTypeEnabled(type, enabled);
    for (auto obj : scenegraph_->objects_) {
        if (!type_enable_.IsTypeEnabled(obj->GetType())) {
            obj->Select(false);
        }
    }
}

bool MapEditor::IsTypeVisible(EntityType type) {
    return type_visible_.IsTypeEnabled(type);
}

void MapEditor::SetTypeVisible(EntityType type, bool visible) {
    type_visible_.SetTypeEnabled(type, visible);
    for (auto obj : scenegraph_->objects_) {
        obj->editor_visible = type_visible_.IsTypeEnabled(obj->GetType());
    }
}

int MapEditor::DuplicateObject(const Object* obj) {
    EntityDescription desc;
    obj->GetDesc(desc);
    ClearDescID(&desc);
    Object* new_obj = AddEntityFromDesc(scenegraph_, desc, false);
    int id = -1;
    if (new_obj) {
        id = new_obj->GetID();
    } else {
        LOGE << "Failed at creating a duplicate" << std::endl;
    }
    QueueSaveHistoryState();
    return id;
}

Object* MapEditor::GetSelectedCameraObject() {
    Object* selected = NULL;
    bool multiple = false;
    // Check for any camera preview objects
    for (auto obj : scenegraph_->objects_) {
        if (obj->GetType() == _placeholder_object) {
            PlaceholderObject* po = (PlaceholderObject*)obj;
            if (po->GetSpecialType() == PlaceholderObject::kCamPreview) {
                if (selected == NULL) {
                    selected = (Object*)po;
                } else {
                    multiple = true;
                }
            }
        }
    }
    // If multiple camera preview objects are found, then check for any that are selected
    if (multiple) {
        for (auto obj : scenegraph_->objects_) {
            if (obj->GetType() == _placeholder_object) {
                PlaceholderObject* po = (PlaceholderObject*)obj;
                if (po->GetSpecialType() == PlaceholderObject::kCamPreview && po->Selected()) {
                    selected = (Object*)po;
                }
            }
        }
    }
    return selected;
}

void MapEditor::ClearUndoHistory() {
    state_history_.clear();
    QueueSaveHistoryState();  // Save state to provide a base state to undo to
}

void MapEditor::UpdateGPUSkinning() {
    scenegraph_->SendMessageToAllObjects(OBJECT_MSG::UPDATE_GPU_SKINNING);
}

void MapEditor::CarveAgainstTerrain() {
    for (auto eo : scenegraph_->visible_static_meshes_) {
        if (eo->Selected()) {
            CSGResultCombined results;
            if (CollideObjects(*scenegraph_->bullet_world_,
                               *scenegraph_->terrain_object_->GetModel(),
                               scenegraph_->terrain_object_->GetTransform(),
                               *eo->GetModel(),
                               eo->GetTransform(),
                               &results)) {
                Model* model = &Models::Instance()->GetModel(eo->model_id_);
                float old_model_texel_density = model->texel_density;
                CSGModelInfo model_info;
                AddCSGResult(results.result[1][1], &model_info, *model, false);
                AddCSGResult(results.result[0][0], &model_info, *scenegraph_->terrain_object_->GetModel(), true);
                // Transform from world space back to model space
                mat4 inv_transform = invert(eo->GetTransform());
                for (size_t i = 0, len = model_info.verts.size(); i < len; i += 3) {
                    MultiplyMat4Vec3D(inv_transform, &model_info.verts[i]);
                }
                if (!CheckShapeValid(model_info.faces, model_info.verts)) {
                    LOGE << "Model is invalid" << std::endl;
                } else {
                    eo->model_id_ = ModelFromCSGModelInfo(model_info);
                    model = &Models::Instance()->GetModel(eo->model_id_);
                    model->texel_density = old_model_texel_density;
                    eo->SetCSGModified();
                }
            }
        }
    }
}

void MapEditor::ExecuteSaveLevelChanges() {
    if (terrain_preview_mode) {
        DisplayMessage("Mesh preview will not be saved", "Terrain preview mode is enabled, the previewed terrain will not be saved and the old one will be restored");
        if (terrain_preview) {
            scenegraph_->UnlinkObject(terrain_preview);
            delete terrain_preview;
            terrain_preview = NULL;
        } else {
            TerrainObject* terrain = scenegraph_->terrain_object_;
            terrain->SetTerrainColorTexture(real_terrain_info.colormap.c_str());
            terrain->SetTerrainWeightTexture(real_terrain_info.weightmap.c_str());
            terrain->SetTerrainDetailTextures(real_terrain_info.detail_map_info);
        }
        ExitTerrainPreviewMode();
    }

    SaveLevel(LevelLoader::kSaveInPlace);
}

static void GetCamBasis(Basis* basis) {
    Camera* cam = ActiveCameras::Get();
    basis->up = cam->GetUpVector();
    basis->facing = cam->GetFacing();
    basis->right = normalize(cross(basis->facing, basis->up));
}

static ToolMode DetermineToolMode(EditorTypes::Tool type) {
    const Mouse& mouse = Input::Instance()->getMouse();
    const Keyboard& keyboard = Input::Instance()->getKeyboard();

    switch (type) {
        case EditorTypes::ROTATE:
            if (mouse.mouse_down_[Mouse::LEFT]) {
                return ROTATE_SPHERE;
            } else {
                return ROTATE_CIRCLE;
            }
        case EditorTypes::TRANSLATE:
            if (mouse.mouse_down_[Mouse::LEFT]) {
                return TRANSLATE_CAMERA_PLANE;
            } else if (KeyCommand::CheckDown(keyboard, KeyCommand::kNormalTransform, KIMF_LEVEL_EDITOR_GENERAL)) {
                return TRANSLATE_FACE_NORMAL;
            } else {
                return TRANSLATE_FACE_PLANE;
            }
        case EditorTypes::SCALE:
            if (mouse.mouse_down_[Mouse::LEFT]) {
                return SCALE_WHOLE;
            } else if (KeyCommand::CheckDown(keyboard, KeyCommand::kNormalTransform, KIMF_LEVEL_EDITOR_GENERAL)) {
                return SCALE_NORMAL;
            } else {
                return SCALE_PLANE;
            }
        default:
            LOG_ASSERT(false);
            return TRANSLATE_CAMERA_PLANE;
    }
}

static bool GetTranslationBasis(vec3 clicked_point, vec3 clicked_normal,
                                Basis* basis, const Object* object_, const Box& box_, ToolMode tool_mode) {
    const mat4& obj_transform = object_->GetTransform();
    const quaternion& obj_rot = object_->GetRotation();
    int box_face_index = box_.GetHitFaceIndex(invert(obj_rot) * clicked_normal,
                                              invert(obj_transform) * clicked_point);
    if (box_face_index != -1) {
        switch (tool_mode) {
            case TRANSLATE_FACE_PLANE:
                basis->up = obj_rot * Box::GetPlaneTangent(box_face_index);
                basis->facing = obj_rot * Box::GetPlaneNormal(box_face_index);
                break;
            case TRANSLATE_FACE_NORMAL:
                basis->up = obj_rot * Box::GetPlaneTangent(box_face_index);
                basis->facing = obj_rot * Box::GetPlaneNormal(box_face_index);
                break;
            default:
                return false;  // Tool is incorrect
        }
        basis->right = obj_rot * Box::GetPlaneBitangent(box_face_index);
        return true;
    } else {
        return false;  // Didn't hit any box faces
    }
}

// Returns false if no basis selected
static bool GetScaleBasis(vec3 clicked_point, vec3 clicked_normal,
                          Basis* basis, vec3* p, const Object* object_, const Box& box_, ToolMode tool_mode) {
    const mat4& obj_transform = object_->GetTransform();
    const quaternion& obj_rot = object_->GetRotation();

    int i = box_.GetHitFaceIndex(invert(obj_rot) * clicked_normal, invert(obj_transform) * clicked_point);
    if (i == -1) {
        return false;
    }
    basis->up = obj_rot * Box::GetPlaneTangent(i);
    basis->facing = obj_rot * Box::GetPlaneNormal(i);
    basis->right = obj_rot * Box::GetPlaneBitangent(i);
    if (tool_mode == SCALE_NORMAL) {
        *p = clicked_point;
    } else {
        *p = obj_transform * box_.GetPlanePoint(i);
    }
    return true;
}

static bool GetRotationBasis(vec3 clicked_point, vec3 clicked_normal,
                             Basis* basis, vec3* p, vec3* around,
                             const Object* object_, const Box& box_) {
    const mat4& obj_transform = object_->GetTransform();
    const quaternion& obj_rot = object_->GetRotation();
    int i = box_.GetHitFaceIndex(invert(obj_rot) * clicked_normal, invert(obj_transform) * clicked_point);
    if (i == -1) {
        return false;
    }
    basis->up = obj_rot * Box::GetPlaneTangent(i);
    basis->facing = obj_rot * Box::GetPlaneNormal(i);
    basis->right = obj_rot * Box::GetPlaneBitangent(i);
    *around = Box::GetPlaneNormal(i);
    *p = obj_transform * box_.GetPlanePoint(i);
    return true;
}

static vec3 GetTranslation(const LineSegment& mouseray, bool snapping_enabled, const vec3& cam_moved, const ControlEditorInfo* control_editor_info, Tool* tool_, Object* object_) {
    static const float TRANSLATION_SNAP_INCR = 0.50f;  // percent of dimension in direction of movement
    vec3 new_point;
    vec3 old_point;

    switch (tool_->mode) {
        case TRANSLATE_FACE_NORMAL:
            RayLineClosestPoint(mouseray.start,
                                normalize(mouseray.end - mouseray.start),
                                control_editor_info->clicked_point_,
                                control_editor_info->basis_.facing,
                                &new_point);
            RayLineClosestPoint(control_editor_info->start_mouseray_.start,
                                normalize(control_editor_info->start_mouseray_.end - control_editor_info->start_mouseray_.start),
                                control_editor_info->clicked_point_,
                                control_editor_info->basis_.facing,
                                &old_point);
            break;
        case TRANSLATE_FACE_PLANE:
        case TRANSLATE_CAMERA_PLANE: {
            vec3 dir = normalize(mouseray.end - mouseray.start);
            float to_dist = RayPlaneIntersection(mouseray.start, dir,
                                                 control_editor_info->clicked_point_,
                                                 control_editor_info->basis_.facing);
            if (to_dist > 0.0f) {
                new_point = to_dist * dir;
                new_point += mouseray.start;
            } else {
                return vec3(0.0f);
            }
            dir = normalize(control_editor_info->start_mouseray_.end -
                            control_editor_info->start_mouseray_.start);
            to_dist = RayPlaneIntersection(control_editor_info->start_mouseray_.start,
                                           dir,
                                           control_editor_info->clicked_point_,
                                           control_editor_info->basis_.facing);
            if (to_dist > 0.0f) {
                old_point = to_dist * dir;
                old_point += control_editor_info->start_mouseray_.start;
            } else {
                return vec3(0.0f);
            }
        } break;
        default:
            LOGW << "Unhandled tool_->mode value: " << GetToolModeString(tool_->mode) << std::endl;
            break;
    }

    vec3 delta_translation = new_point - old_point;
    if (snapping_enabled) {
        const quaternion& obj_rotate = object_->GetRotation();
        delta_translation = invert(object_->GetRotation()) * delta_translation;
        for (int i = 0; i < 3; ++i) {
            delta_translation[i] = floorf(delta_translation[i] / TRANSLATION_SNAP_INCR + 0.5f) * TRANSLATION_SNAP_INCR;
        }
        delta_translation = obj_rotate * delta_translation;
    }
    return delta_translation;
}

static vec3 GetScale(const LineSegment& mouseray, vec3* trans, bool snapping_enabled, const vec3& cam_moved,
                     const ControlEditorInfo* control_editor_info, Tool* tool_, Object* object_, const Box& box_) {
    static const float SCALE_SNAP_INCR = 1.50f;  // incr whenever new size = 1.25 * old size
    if (trans) {
        *trans = vec3(0.0f);
    }
    vec3 new_clicked_point;
    vec3 old_clicked_point;
    vec3 original_click_dir = normalize(control_editor_info->start_mouseray_.end - control_editor_info->start_mouseray_.start);
    if (tool_->mode == SCALE_NORMAL) {
        vec3 mouseray_dir = normalize(mouseray.end - mouseray.start);
        RayLineClosestPoint(mouseray.start, mouseray_dir, tool_->center, -control_editor_info->basis_.facing, &new_clicked_point);
        RayLineClosestPoint(control_editor_info->start_cam_pos, original_click_dir, tool_->center, -control_editor_info->basis_.facing, &old_clicked_point);
    } else if (tool_->mode == SCALE_PLANE) {
        vec3 mouseray_dir = normalize(mouseray.end - mouseray.start);
        float to_dist = RayPlaneIntersection(mouseray.start, mouseray_dir, tool_->center, -control_editor_info->basis_.facing);
        if (to_dist > 0.0f) {
            new_clicked_point = mouseray.start + to_dist * mouseray_dir;
        } else {
            LOG_ASSERT(false);
            return vec3(1.0f);
        }
        to_dist = RayPlaneIntersection(control_editor_info->start_cam_pos, original_click_dir, tool_->center, -control_editor_info->basis_.facing);
        if (to_dist > 0.0f) {
            old_clicked_point = control_editor_info->start_cam_pos + to_dist * original_click_dir;
        } else {
            LOG_ASSERT(false);
            return vec3(1.0f);
        }
    }

    vec3 initial_rel_point;
    vec3 scale;
    if (tool_->mode == SCALE_WHOLE) {
        Camera* cam = ActiveCameras::Get();
        vec3 cam_facing = cam->GetFacing();
        vec3 center = tool_->center;
        vec3 mouseray_dir = normalize(mouseray.end - mouseray.start);
        float t = RayPlaneIntersection(mouseray.start, mouseray_dir, center, cam_facing);
        vec3 curr_mouse_on_plane = mouseray.start + mouseray_dir * t;
        float t3 = RayPlaneIntersection(control_editor_info->start_cam_pos, original_click_dir, center, cam_facing);
        vec3 orig_mouse_on_plane = control_editor_info->start_cam_pos + original_click_dir * t3;
        float length_real_old = length(orig_mouse_on_plane - center);
        float length_new = length(curr_mouse_on_plane - center);

        vec3 cam_up = cam->GetUpVector();
        vec3 cam_right = cross(cam_facing, cam_up);
        mat4 old_scale;
        old_scale.SetUniformScale(length_real_old);
        mat4 new_scale;
        new_scale.SetUniformScale(length_new);
        mat4 circle_transform;
        circle_transform.SetColumn(0, cam_right);
        circle_transform.SetColumn(1, cam_up);
        circle_transform.SetColumn(2, cam_facing);
        circle_transform.SetTranslationPart(center + cam_moved);
        DebugDraw::Instance()->AddCircle(circle_transform * old_scale, vec4(0.0f, 0.0f, 0.0f, 0.5f), _delete_on_update, _DD_XRAY);
        DebugDraw::Instance()->AddCircle(circle_transform * new_scale, vec4(1.0f, 1.0f, 1.0f, 0.5f), _delete_on_update, _DD_XRAY);
        if (length_real_old > 0 && length_new > 0) {
            scale = length_new / length_real_old;
        }
    } else {
        initial_rel_point = object_->GetRotation() *
                            (-1.0f * object_->start_transform.scale *
                             box_.GetPoint(tool_->control_vertex));
        initial_rel_point += object_->start_transform.translation;
        vec3 new_clicked_point_local = invert(object_->GetRotation()) * (new_clicked_point - initial_rel_point);
        vec3 old_clicked_point_local = invert(object_->GetRotation()) * (old_clicked_point - initial_rel_point);

        for (int i = 0; i < 3; ++i) {
            if (old_clicked_point_local[i] == 0.0f) {
                LOG_ASSERT(false);
                return vec3(1.0f);
            }
            scale[i] = new_clicked_point_local[i] / old_clicked_point_local[i];
        }
    }

    if (snapping_enabled) {
        float log_SCALE_SNAP_INCR = logf(SCALE_SNAP_INCR);
        for (int i = 0; i < 3; ++i) {
            bool neg = (scale[i] < 0.0f);
            scale[i] = floorf((logf(fabsf(scale[i])) / log_SCALE_SNAP_INCR) + 0.5f);
            scale[i] = (float)pow(SCALE_SNAP_INCR, scale[i]);
            if (neg) {
                scale[i] *= -1.0f;
            }
        }
    }

    if (trans && tool_->mode != SCALE_WHOLE) {
        vec3 new_initial_rel_point = object_->GetRotation() *
                                     (-1.0f * (object_->start_transform.scale * scale) * box_.GetPoint(tool_->control_vertex));
        new_initial_rel_point += object_->start_transform.translation;
        *trans = initial_rel_point - new_initial_rel_point;
    }

    return scale;
}

static quaternion GetRotation(const LineSegment& mouseray, bool snapping_enabled, const vec3& cam_moved, const ControlEditorInfo* control_editor_info, Tool* tool_, Object* object_, const Box& box_, GameCursor* cursor) {
    static const float ANGLE_SNAP_INCR = 30;

    vec3 around = tool_->around;
    float angle = 0.0f;

    vec3 dir = normalize(mouseray.end - mouseray.start);

    vec3 a, b, p, n;
    vec3 new_point;

    quaternion rotation;

    if (tool_->mode == ROTATE_SPHERE) {
        int* mouse_pos = Input::Instance()->getMouse().pos_;
        Camera* cam = ActiveCameras::Get();

        float sensitivity = 0.1f;
        vec3 temp_radius = box_.dims * 0.5f;
        temp_radius *= object_->GetScale();
        sensitivity *= distance(tool_->center, cam->GetPos()) / length(temp_radius);

        angle = (float)(mouse_pos[0] - tool_->old_mousex);
        angle *= sensitivity;
        tool_->old_mousex = mouse_pos[0];
        vec4 x_rot(cam->GetUpVector(), angle / 180.0f * PI_f);
        mat4 mat;
        quaternion x_rot_quat(x_rot);

        angle = (float)(mouse_pos[1] - tool_->old_mousey);
        angle *= sensitivity;
        angle *= -1.0f;
        tool_->old_mousey = mouse_pos[1];
        vec4 y_rot(cross(cam->GetUpVector(), cam->GetFacing()), angle / 180.0f * PI_f);
        quaternion y_rot_quat(y_rot);
        tool_->trackball_accumulate = x_rot_quat * y_rot_quat * tool_->trackball_accumulate;
        rotation = tool_->trackball_accumulate;
    }
    if (tool_->mode == ROTATE_CIRCLE) {
        n = -control_editor_info->basis_.facing;
        p = tool_->center;

        float to_dist = RayPlaneIntersection(mouseray.start, dir, p, n);
        if (to_dist < 0) {
            return quaternion();
        }
        new_point = mouseray.start + to_dist * dir;

        to_dist = RayPlaneIntersection(control_editor_info->start_mouseray_.start, normalize(control_editor_info->start_mouseray_.end - control_editor_info->start_mouseray_.start), p, n);
        if (to_dist < 0) {
            return quaternion();
        }
        vec3 old_point = control_editor_info->start_mouseray_.start + to_dist * normalize(control_editor_info->start_mouseray_.end - control_editor_info->start_mouseray_.start);

        a = normalize(new_point - p);
        b = normalize(old_point - p);

        float a_dot_b = dot(a, b);
        if (a_dot_b < -1 || a_dot_b > 1 || a_dot_b == 0) {
            return quaternion();
        }
        vec3 a_cross_b = cross(a, b);
        angle = atan2f(dot(n, a_cross_b), a_dot_b);

        around = control_editor_info->basis_.facing;

        if (snapping_enabled) {
            const float ANGLE_SNAP_INCR_rad = ANGLE_SNAP_INCR * deg2radf;
            angle = floorf((angle / ANGLE_SNAP_INCR_rad) + 0.5f) * ANGLE_SNAP_INCR_rad;
        }

        // handle rotation cursor
        if (tool_->mode == ROTATE_CIRCLE) {
            cursor->SetCursor(ROTATE_CIRCLE_CURSOR);
            vec3 screen_p1 = ActiveCameras::Get()->ProjectPoint(tool_->center[0], tool_->center[1], tool_->center[2]);
            vec3 screen_p2 = ActiveCameras::Get()->ProjectPoint(new_point[0], new_point[1], new_point[2]);
            vec3 screen_l = screen_p2 - screen_p1;
            float cursor_rotation;
            if (screen_l[1] == 0)
                if (screen_l[0] < 0)
                    cursor_rotation = -90.0f;
                else
                    cursor_rotation = 90.0f;
            else {
                float slope = screen_l[1] / screen_l[0];
                if (screen_l[0] < 0.0f) {
                    cursor_rotation = atanf(slope) * (float)(rad2deg) + 90.0f;
                } else {
                    cursor_rotation = atanf(slope) * (float)(rad2deg)-90.0f;
                }
            }
            cursor->SetRotation(cursor_rotation);
        }
        DebugDraw::Instance()->AddLine(p + cam_moved, mouseray.end + cam_moved, vec4(0, 0, 0, 0.5f), _delete_on_update, _DD_XRAY);
        rotation = quaternion(vec4(around, angle));
    }

    // Handle snaps
    if (snapping_enabled) {
        if (fabsf(angle) >= ANGLE_SNAP_INCR) {
            angle = GetSign(angle) * ANGLE_SNAP_INCR;
        } else {
            angle = 0;
        }
    }
    return rotation;
}

static void StartMouseTransformation(EditorTypes::Tool type, Object* obj, ControlEditorInfo* control_editor_info, const Collision& c, const LineSegment& mouseray, GameCursor* cursor) {
    control_editor_info->start_mouseray_ = mouseray;
    control_editor_info->start_cam_pos = ActiveCameras::Get()->GetPos();
    control_editor_info->clicked_point_ = c.hit_where;

    // turn off camera controls
    if (ActiveCameras::Get()->m_camera_object != NULL) {
        ActiveCameras::Get()->m_camera_object->IgnoreMouseInput(true);
    }
    ToolMode mode = DetermineToolMode(type);

    mat4 obj_transform = obj->GetTransform();
    quaternion obj_rot = obj->GetRotation();
    int box_face_index = obj->box_.GetHitFaceIndex(invert(obj_rot) * c.hit_normal,
                                                   invert(obj_transform) * control_editor_info->clicked_point_);
    int which_face_clicked = box_face_index;

    switch (mode) {
        case TRANSLATE_FACE_PLANE:
        case SCALE_PLANE:
            control_editor_info->face_display_.plane = true;
            control_editor_info->face_selected_ = which_face_clicked;
            break;
        case TRANSLATE_FACE_NORMAL:
        case SCALE_NORMAL:
        case ROTATE_CIRCLE:
            control_editor_info->face_display_.facing = true;
            control_editor_info->face_selected_ = which_face_clicked;
            break;
        default:
            LOGW << "Unhandled mode: " << GetToolModeString(mode) << std::endl;
            break;
    }
    control_editor_info->tool_.mode = mode;
    switch (type) {
        case EditorTypes::ROTATE: {
            if (control_editor_info->tool_.mode == ROTATE_CIRCLE) {
                vec3 rt_center = control_editor_info->tool_.center;
                vec3 rt_around = control_editor_info->tool_.around;
                GetRotationBasis(control_editor_info->clicked_point_,
                                 c.hit_normal, &control_editor_info->basis_, &rt_center, &rt_around,
                                 obj, obj->box_);
                control_editor_info->tool_.center = rt_center;
                control_editor_info->tool_.around = rt_around;
            } else {
                control_editor_info->tool_.center = obj->GetTranslation();
            }

            control_editor_info->tool_.old_mousex = Input::Instance()->getMouse().pos_[0];
            control_editor_info->tool_.old_mousey = Input::Instance()->getMouse().pos_[1];
            control_editor_info->tool_.trackball_accumulate = quaternion();

            GetRotation(mouseray, false, vec3(0.0f), control_editor_info, &control_editor_info->tool_, obj, obj->box_, cursor);
        } break;
        case EditorTypes::TRANSLATE: {
            if (control_editor_info->tool_.mode == TRANSLATE_CAMERA_PLANE) {
                GetCamBasis(&control_editor_info->basis_);
            } else {
                if (!GetTranslationBasis(
                        control_editor_info->clicked_point_,
                        c.hit_normal,
                        &control_editor_info->basis_,
                        obj, obj->box_, control_editor_info->tool_.mode)) {
                    FatalError("Error", "Bad translation mode.");
                }
            }
            GetTranslation(mouseray, false, vec3(0.0f), control_editor_info, &control_editor_info->tool_, obj);
        } break;
        case EditorTypes::SCALE: {
            vec3 scale_tool_center = control_editor_info->tool_.center;
            if (mode == SCALE_WHOLE ||
                !GetScaleBasis(control_editor_info->clicked_point_,
                               c.hit_normal, &control_editor_info->basis_, &scale_tool_center,
                               obj, obj->box_,
                               control_editor_info->tool_.mode)) {
                control_editor_info->tool_.center = obj->GetTranslation();
            } else {
                control_editor_info->tool_.center = scale_tool_center;
            }

            // Back to object space
            mat4 inv = invert(obj->GetTransform());
            vec3 point = inv * control_editor_info->clicked_point_;
            float point_dist = 0;
            control_editor_info->tool_.control_vertex = obj->box_.GetNearestPointIndex(point, point_dist);

            GetScale(mouseray, NULL, false, vec3(0.0f), control_editor_info, &control_editor_info->tool_, obj, obj->box_);
        } break;
        default:
            LOG_ASSERT(false);
    }
}

static bool GetMouseTransformation(
    EditorTypes::Tool type, Object* obj,
    ControlEditorInfo* control_editor_info, bool snapping_enabled,
    const LineSegment& mouseray, SeparatedTransform* curr_transform,
    GameCursor* cursor) {
    vec3 cam_moved = ActiveCameras::Get()->GetPos() - control_editor_info->start_cam_pos;
    LineSegment cam_corrected_mouseray;
    cam_corrected_mouseray.start = mouseray.start - cam_moved;
    cam_corrected_mouseray.end = mouseray.end - cam_moved;
    curr_transform->translation = vec3(0.0f);
    curr_transform->scale = vec3(1.0f);
    curr_transform->rotation = quaternion();
    switch (type) {
        case EditorTypes::TRANSLATE:
            curr_transform->translation = GetTranslation(cam_corrected_mouseray, snapping_enabled, cam_moved, control_editor_info, &control_editor_info->tool_, obj);
            curr_transform->translation += cam_moved;
            return (curr_transform->translation != obj->GetTranslation());
        case EditorTypes::ROTATE:
            curr_transform->rotation = GetRotation(cam_corrected_mouseray, snapping_enabled, cam_moved, control_editor_info, &control_editor_info->tool_, obj, obj->box_, cursor);
            curr_transform->translation += cam_moved;
            return (curr_transform->rotation != obj->GetRotation());
        case EditorTypes::SCALE:
            curr_transform->scale = GetScale(cam_corrected_mouseray, &curr_transform->translation, snapping_enabled, cam_moved, control_editor_info, &control_editor_info->tool_, obj, obj->box_);
            curr_transform->translation += cam_moved;
            return (curr_transform->scale != obj->GetScale());
        default:
            LOGW << "Unknown EditorTypes::Tool " << EditorTypes::GetToolString(type) << std::endl;
            break;
    }
    return false;
}

static void EndTransformation(SceneGraph* scenegraph, EditorTypes::Tool type, Object* control_obj, ControlEditorInfo* control_editor_info, GameCursor* cursor) {
    for (auto obj : scenegraph->objects_) {
        if (obj->Selected()) {
            obj->HandleTransformationOccurred();
        }
    }
    if (control_obj != NULL) {
        // turn on camera controls
        if (ActiveCameras::Get()->m_camera_object != NULL) {
            ActiveCameras::Get()->m_camera_object->IgnoreMouseInput(false);
        }
        if (control_editor_info->tool_.mode == ROTATE_CIRCLE) {
            cursor->SetRotation(0);
        }
        control_obj = NULL;
    }
}

void MapEditor::UpdateTransformTool(SceneGraph* scenegraph, EditorTypes::Tool type, const LineSegment& mouseray, const Collision& c, GameCursor* cursor) {
    PROFILER_ZONE(g_profiler_ctx, "UpdateTransformTool()");

    Mouse* mouse = &(Input::Instance()->getMouse());
    const Keyboard& keyboard = Input::Instance()->getKeyboard();

    bool input_happened = false;
    bool transformation_happened = false;

    if ((mouse->mouse_down_[Mouse::LEFT] || mouse->mouse_down_[Mouse::RIGHT]) && !mouse->mouse_double_click_[Mouse::LEFT]) {
        if (state_ == MapEditor::kIdle) {
            if (MouseWasClickedThisTimestep(mouse) && c.hit && c.hit_what && c.hit_what->Selected()) {
                // We are just starting to drag
                control_obj = c.hit_what;
                input_happened = true;
                StartMouseTransformation(type, control_obj, &control_editor_info, c, mouseray, cursor);
            }
        } else {
            // We are continuing to drag
            input_happened = true;
            bool snapping_enabled = KeyCommand::CheckDown(keyboard, KeyCommand::kSnapTransform, KIMF_LEVEL_EDITOR_GENERAL);
            if (control_obj) {
                transformation_happened = GetMouseTransformation(type, control_obj, &control_editor_info, snapping_enabled, mouseray, &curr_tool_transform, cursor);
            }
        }
    }

    // Apply the transformation
    if (input_happened && state_ == MapEditor::kIdle) {
        PROFILER_ZONE(g_profiler_ctx, "Starting transform");
        if (KeyCommand::CheckDown(keyboard, KeyCommand::kCloneTransform, KIMF_LEVEL_EDITOR_GENERAL)) {
            EntityDescriptionList copy_desc_list;
            ActorsEditor_CopySelectedEntities(scenegraph, &copy_desc_list);
            ActorsEditor_UnlocalizeIDs(&copy_desc_list, scenegraph);
            MapEditor::DeselectAll(scenegraph);
            std::vector<Object*> new_objects;
            for (auto& i : copy_desc_list) {
                Object* new_entity = CreateObjectFromDesc(i);
                if (new_entity) {
                    new_objects.push_back(new_entity);
                    if (new_entity->permission_flags & Object::CAN_SELECT) {
                        new_entity->Select(true);
                    }
                    if (ActorsEditor_AddEntity(scenegraph, new_entity, NULL, false)) {
                    } else {
                        LOGE << "Failed to add entity to scenegraph while cloning" << std::endl;
                        delete new_entity;
                    }
                } else {
                    LOGE << "Failed to entity" << std::endl;
                }
            }
            for (auto& new_object : new_objects) {
                new_object->ReceiveObjectMessage(OBJECT_MSG::FINALIZE_LOADED_CONNECTIONS);
            }
        }
        state_ = MapEditor::kTransformDrag;
        for (auto obj : scenegraph->objects_) {
            obj->start_transform.translation = obj->GetTranslation();
            obj->start_transform.rotation = obj->GetRotation();
            obj->start_transform.scale = obj->GetScale();
        }
    } else if (state_ == MapEditor::kTransformDrag && !input_happened) {  // end
        EndTransformation(scenegraph, type, control_obj, &control_editor_info, cursor);
        control_editor_info.face_selected_ = -1;
        control_editor_info.face_display_.facing = false;
        control_editor_info.face_display_.plane = false;
        state_ = MapEditor::kIdle;
        QueueSaveHistoryState();
        scenegraph_->UpdatePhysicsTransforms();
    } else if (state_ == MapEditor::kTransformDrag && transformation_happened) {  // continue
        std::vector<int> moved_objects;
        PROFILER_ZONE(g_profiler_ctx, "Updating transform");
        for (auto obj : scenegraph->objects_) {
            if (obj->Selected()) {
                obj->SetTranslation(obj->start_transform.translation + curr_tool_transform.translation);
                obj->SetRotation(curr_tool_transform.rotation * obj->start_transform.rotation);
                obj->SetScale(curr_tool_transform.scale * obj->start_transform.scale);
                obj->HandleTransformationOccurred();

                moved_objects.push_back(obj->GetID());
            }
        }
        if (!moved_objects.empty()) {
            PROFILER_ZONE(g_profiler_ctx, "Send moved_objects message");

            std::ostringstream oss;
            oss << "moved_objects";

            for (int moved_object : moved_objects) {
                oss << " " << moved_object;
            }

            scenegraph_->level->Message(oss.str());
        }
    }
    return;
}

bool IsBeingMoved(const MapEditor* map_editor, const Object* object) {
    if (object->parent && IsBeingMoved(map_editor, object->parent)) {
        return true;
    } else {
        if (map_editor->state_ == MapEditor::kTransformDrag && map_editor->control_obj == object) {
            return true;
        } else {
            return false;
        }
    }
}

void MapEditor::AddColorToHistory(vec4 color) {
    color_history_candidate_ = color;
    color_history_countdown_ = kNumUpdatesBeforeColorSave;
}

void MapEditor::PreviewTerrainHeightmap(const char* path) {
    PreviewTerrain(path, NULL, NULL);
}

void MapEditor::PreviewTerrainColormap(const char* path) {
    PreviewTerrain(NULL, path, NULL);
}

void MapEditor::PreviewTerrainWeightmap(const char* path) {
    PreviewTerrain(NULL, NULL, path);
}

void MapEditor::PreviewTerrainDetailmap(int index, const char* color_path, const char* normal_path, const char* material_path) {
    EnterTerrainPreviewMode();

    assert(index >= 0 && index < 4);
    if (terrain_preview) {
        std::vector<DetailMapInfo> detail_map_info = terrain_preview->terrain_info().detail_map_info;
        detail_map_info[index].colorpath = color_path;
        detail_map_info[index].normalpath = normal_path;
        detail_map_info[index].materialpath = material_path;

        terrain_preview->SetTerrainDetailTextures(detail_map_info);
    } else if (scenegraph_->terrain_object_) {
        std::vector<DetailMapInfo> detail_map_info = scenegraph_->terrain_object_->terrain_info().detail_map_info;
        detail_map_info[index].colorpath = color_path;
        detail_map_info[index].normalpath = normal_path;
        detail_map_info[index].materialpath = material_path;

        scenegraph_->terrain_object_->SetTerrainDetailTextures(detail_map_info);
    }
}

void MapEditor::PreviewTerrain(const char* heightmap_path, const char* colormap_path, const char* weightmap_path) {
    EnterTerrainPreviewMode();

    if (heightmap_path) {
        const char* blank_normal_path = Textures::Instance()->GetBlankNormalTextureAssetRef()->path_.c_str();
        TerrainInfo terrain_info;
        bool keep_data = true;
        TerrainObject* current_terrain = scenegraph_->terrain_object_;
        if (terrain_preview) {
            // Copy data from old and delete since a new one will be created
            terrain_info = terrain_preview->terrain_info();
            scenegraph_->UnlinkObject(terrain_preview);
            delete terrain_preview;
        } else if (current_terrain) {
            // Copy data from "real" terrain
            terrain_info = current_terrain->terrain_info();
            current_terrain->preview_mode = true;
        } else {
            keep_data = false;
            terrain_info.SetDefaults();
            DetailMapInfo blank_detail;
            blank_detail.colorpath = "Data/Textures/Terrain/default_d.png";
            blank_detail.normalpath = blank_normal_path;
            blank_detail.materialpath = "Data/Materials/default.xml";
            for (int i = 0; i < 4; ++i)
                terrain_info.detail_map_info.push_back(blank_detail);
        }
        terrain_info.minimal = true;

        if (heightmap_path && strlen(heightmap_path)) {
            terrain_info.heightmap = heightmap_path;
        } else {
            terrain_info.heightmap = "Data/Textures/Terrain/default_hm.png";
        }
        if (colormap_path && strlen(colormap_path)) {
            terrain_info.colormap = colormap_path;
        } else if (!terrain_preview && !keep_data) {
            terrain_info.colormap = "Data/Textures/Terrain/default_c.png";
        }
        if (weightmap_path && strlen(weightmap_path)) {
            terrain_info.weightmap = weightmap_path;
        } else if (!terrain_preview && !keep_data) {
            terrain_info.weightmap = "Data/Textures/Terrain/default_w.png";
        }

        terrain_preview = new TerrainObject(terrain_info);
        scenegraph_->addObject(terrain_preview);
    } else {
        TerrainObject* terrain = scenegraph_->terrain_object_;
        if (terrain_preview)
            terrain = terrain_preview;

        if (colormap_path) {
            terrain->SetTerrainColorTexture(colormap_path);
        }
        if (weightmap_path) {
            terrain->SetTerrainWeightTexture(weightmap_path);
        }
    }
}

bool MapEditor::GameplayObjectsEnabled() const {
    return gameplay_objects_enabled_;
}

const TerrainInfo* MapEditor::GetPreviewTerrainInfo() const {
    return (terrain_preview ? &terrain_preview->terrain_info() : NULL);
}

bool MapEditor::CreateObjectFromHost(const std::string& path, const vec3& pos, CommonObjectID host_id) {
    int retVal = LoadEntitiesFromFile(path);
    ActorsEditor_AddEntitiesAtPosition(add_desc_list_source_, scenegraph_, add_desc_list_, pos, create_as_prefab_, host_id);

    return retVal == 0;
}

void MapEditor::EnterTerrainPreviewMode() {
    if (!terrain_preview_mode) {
        TerrainObject* current_terrain = scenegraph_->terrain_object_;
        if (current_terrain) {
            // Keep these so they can be restored
            real_terrain_info = current_terrain->terrain_info();
        }

        terrain_preview_mode = true;
    }
}

void MapEditor::ExitTerrainPreviewMode() {
    if (terrain_preview_mode) {
        if (scenegraph_->terrain_object_)
            scenegraph_->terrain_object_->preview_mode = false;
        terrain_preview_mode = false;
    }
}
