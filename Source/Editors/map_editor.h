//-----------------------------------------------------------------------------
//           Name: map_editor.h
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

#include <Editors/editor_types.h>
#include <Editors/editor_utilities.h>
#include <Editors/editor_tools.h>
#include <Editors/save_state.h>

#include <Internal/collisiondetection.h>
#include <Internal/levelxml.h>

#include <Game/EntityDescription.h>
#include <Online/online_datastructures.h>
#include <GUI/IMUI/imui.h>
#include <Objects/object.h>
#include <XML/level_loader.h>
#include <Math/overgrowth_geometry.h>
#include <UserInput/keyboard.h>

#include <queue>
#include <set>

extern bool draw_group_and_prefab_boxes;
extern bool always_draw_hotspot_connections;

namespace PHOENIX_KEY_CONSTANTS {
#ifdef PLATFORM_MACOSX
const int command_key = Keyboard::GUI;
const int command_SDLK_key = SDLK_GUI;
#else
const int command_key = Keyboard::CTRL;
const int command_SDLK_key = SDLK_CTRL;
#endif
}  // namespace PHOENIX_KEY_CONSTANTS

class SceneGraph;
class EnvObject;
class TiXmlDocument;
class TiXmlElement;
class Group;
class ActorsEditor;
class SkyEditor;
class Hotspot;
class DecalObject;
class GUI;
class GameCursor;
class TiXmlNode;
class TerrainObject;

class TypeEnable {
   public:
    TypeEnable(const char* config_postfix);
    bool IsTypeEnabled(EntityType type) const;
    void SetTypeEnabled(EntityType type, bool enabled);
    void SetAll(bool enabled);
    void SetFromConfig();
    void WriteToConfig();

   private:
    typedef std::map<EntityType, bool> TypeEnabledMap;
    TypeEnabledMap type_enabled_;
    bool unknown_types_enabled_;
    const char* config_postfix;
    bool ReadTypeString(EntityType type);
    void WriteTypeString(EntityType type) const;
};

struct FaceDisplay {
    bool facing, plane;
};

enum ToolMode {
    SCALE_WHOLE,
    SCALE_PLANE,
    SCALE_NORMAL,
    TRANSLATE_CAMERA_PLANE,
    TRANSLATE_FACE_PLANE,
    TRANSLATE_FACE_NORMAL,
    ROTATE_SPHERE,
    ROTATE_CIRCLE
};

inline const char* GetToolModeString(ToolMode m) {
    switch (m) {
        case SCALE_WHOLE:
            return "SCALE_WHOLE";
        case SCALE_PLANE:
            return "SCALE_PLANE";
        case SCALE_NORMAL:
            return "SCALE_NORMAL";
        case TRANSLATE_CAMERA_PLANE:
            return "TRANSLATE_CAMERA_PLANE";
        case TRANSLATE_FACE_PLANE:
            return "TRANSLATE_FACE_PLANE";
        case TRANSLATE_FACE_NORMAL:
            return "TRANSLATE_FACE_NORMAL";
        case ROTATE_SPHERE:
            return "ROTATE_SPHERE";
        case ROTATE_CIRCLE:
            return "ROTATE_CIRCLE";
        default:
            return "(unknown tool mode string value)";
    }
}

struct Tool {
    ToolMode mode;

    int control_vertex;

    quaternion trackball_accumulate;
    vec3 center;
    vec3 around;
    int old_mousex;
    int old_mousey;
};

struct Basis {
    vec3 up;
    vec3 facing;
    vec3 right;
};

struct ControlEditorInfo {
    vec3 start_cam_pos;
    LineSegment start_mouseray_;
    vec3 clicked_point_;
    Basis basis_;
    int face_selected_;
    FaceDisplay face_display_;

    Tool tool_;
};

// The MapEditor class contains all of the editors.
class MapEditor {
   public:
    MapEditor();
    ~MapEditor();

    enum State {
        kInGame,
        kIdle,
        kTransformDrag,
        kBoxSelectDrag,
        kSkyDrag
    };

    State state_;
    GUI* gui;

    void Initialize(SceneGraph* s);
    void UpdateEnabledObjects();

    void Draw();
    void Update(GameCursor* cursor);

    void ShowEditorMessage(int type, const std::string& message);

    void Undo();
    void Redo();
    void CopySelected();
    void Paste(const LineSegment& mouseray);
    void DeleteSelected();
    void DeleteID(int val);
    void CutSelected();
    void SavePrefab(bool do_resave);
    void SaveSelected();
    void GroupSelected();
    bool ContainsPrefabsRecursively(std::vector<Object*> objects);
    int PrefabSelected();
    void UngroupSelected();
    bool IsSomethingSelected();
    bool IsOneObjectSelected();

    void SendInRabbot();
    void StopRabbot(bool handle_gui = true);

    void SaveEntities(TiXmlNode* root);

    void QueueSaveHistoryState();

    bool CanUndo();
    bool CanRedo();
    void SetViewNavMesh(bool enabled);
    void SetViewCollisionNavMesh(bool enabled);
    bool IsViewingNavMesh();
    bool IsViewingCollisionNavMesh();
    void ToggleImposter();
    void SetImposter(bool set);
    void CPSetColor(const vec3& color);
    static Object* AddEntityFromDesc(SceneGraph* scenegraph, const EntityDescription& desc, bool level_loading);
    void ApplyScriptParams(const ScriptParamMap& spm, int id);
    void ApplyPalette(const OGPalette& palette, int edited_id);
    OGPalette* GetPalette(int id);
    void ReceiveMessage(const std::string& msg);
    void SetUpSky(const SkyInfo& si);
    void RemoveObject(Object* o, SceneGraph* scenegraph, bool removed_by_socket = false);

    bool WasLastSaveOnCurrentUndoChunk();
    void SetLastSaveOnCurrentUndoChunk();
    void SaveLevel(LevelLoader::SaveLevelType type);

    int CreateObject(const std::string& path);
    int DuplicateObject(const Object* obj);
    int ReplaceObjects(const std::vector<Object*>& objects, const std::string& replacement_path);
    bool IsTypeEnabled(EntityType type);
    void SetTypeEnabled(EntityType type, bool enabled);
    void SaveViewRibbonTypeEnabled(EntityType type, bool enabled);
    bool IsTypeVisible(EntityType type);
    void SetTypeVisible(EntityType type, bool enabled);
    Object* GetSelectedCameraObject();
    static void DeselectAll(SceneGraph* scenegraph);
    void ClearUndoHistory();
    void LoadDialogueFile(const std::string& path);
    void UpdateGPUSkinning();
    void CarveAgainstTerrain();
    void ExecuteSaveLevelChanges();
    void AddLightProbes();
    void BakeLightProbes(int pass);
    int LoadEntitiesFromFile(const std::string& filename);
    void ReturnSelected(std::vector<Object*>* selected_objects);
    void RibbonItemClicked(const std::string& item, bool param);
    void SelectAll();
    const StateHistory& state_history() { return state_history_; }

    void ReloadAllPrefabs();
    void ReloadPrefabs(const Path& path);
    bool ReloadPrefab(Object* obj, SceneGraph* scenegraph);

    SceneGraph* GetSceneGraph() { return scenegraph_; }

    Object* control_obj;  // editor for curr object controlling the transformations
    ControlEditorInfo control_editor_info;
    EditorTypes::Tool active_tool_;
    SkyEditor* sky_editor_;

    vec4 GetColorHistoryIndex(int index) { return color_history_[index]; }

    static const int kColorHistoryLen = 20;
    void AddColorToHistory(vec4 color);

    void PreviewTerrain(const char* heightmap_path, const char* colormap_path, const char* weightmap_path);
    void PreviewTerrainDetailmap(int index, const char* color_path, const char* normal_path, const char* material_path);
    void PreviewTerrainHeightmap(const char* path);
    void PreviewTerrainColormap(const char* path);
    void PreviewTerrainWeightmap(const char* path);

    bool GameplayObjectsEnabled() const;

    const TerrainInfo* GetPreviewTerrainInfo() const;
    bool GetTerrainPreviewMode() const { return terrain_preview_mode; }
    bool CreateObjectFromHost(const std::string& path, const vec3& pos, CommonObjectID host_id);

   private:
    void UpdateTransformTool(SceneGraph* scenegraph, EditorTypes::Tool type, const LineSegment& mouseray, const Collision& c, GameCursor* cursor);
    TypeEnable type_enable_;
    TypeEnable type_visible_;
    vec4 color_history_candidate_;

    bool gameplay_objects_enabled_;  // The setting is called "gameplay objects" in-game, but the enum value is actually hotspots...
    bool nav_hints_enabled_;
    bool nav_region_enabled_;
    bool jump_nodes_enabled_;

    StateHistory state_history_;
    EntityDescriptionList add_desc_list_;
    Path add_desc_list_source_;
    bool create_as_prefab_;
    SeparatedTransform curr_tool_transform;

    enum {
        LEVEL_PARAM_ID = -1
    };

    void InitializeColorHistory();
    void SaveColorToHistory();
    int color_history_countdown_;
    vec4 color_history_[kColorHistoryLen];

    void UpdateCursor(const LineSegment& mouseray, GameCursor* cursor);
    // internal helper functions
    void DeleteEditors();

    void SetTool(EditorTypes::Tool tool);

    // Control and tool handlers
    void HandleShortcuts(const LineSegment& mouseray);
    void UpdateTools(const LineSegment& mouseray, GameCursor* cursor);
    bool CheckForSelections();
    bool CheckForSelections(const LineSegment& mouseray);
    IMUIContext imui_context;

    void BringAllToCurrentState(int old_state);
    bool HandleScrollSelect(const vec3& start, const vec3& end);
    int save_countdown_;

    SceneGraph* scenegraph_;

    EntityDescriptionList copy_desc_list_;

    // Tool structures
    BoxSelector box_selector_;
    EditorTypes::Tool omni_tool_tool_;

    std::vector<Object*> selected;  // This is only a member to avoid per-frame memory allocs
    std::vector<Object*> box_objects;

    // Value indicating what point in the undo stack we last saved.
    unsigned last_saved_chunk_global_id;

    void EnterTerrainPreviewMode();
    void ExitTerrainPreviewMode();

    bool terrain_preview_mode;
    TerrainInfo real_terrain_info;
    TerrainObject* terrain_preview;
};

void LoadLevel(bool local);
bool IsBeingMoved(const MapEditor* map_editor, const Object* object);
// Don't move whole code chunk from engine.cpp yet
extern void PlaceLightProbes(SceneGraph* scenegraph, vec3 translation, quaternion rotation, vec3 scale);
