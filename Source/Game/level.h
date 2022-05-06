//-----------------------------------------------------------------------------
//           Name: level.h
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

#include <Graphics/hudimage.h>
#include <Graphics/text.h>

#include <Scripting/angelscript/asarglist.h>
#include <Scripting/scriptparams.h>

#include <Game/loadingscreeninfo.h>
#include <Game/characterscript.h>

#include <Editors/save_state.h>
#include <Main/engine.h>
#include <Network/asnetwork.h>
#include <Internal/spawneritem.h>
#include <AI/navmeshparameters.h>

#include <set>
#include <map>
#include <vector>
#include <string>
#include <memory>

class ASContext;
class SceneGraph;
class Hotspot;
class MovementObject;
class SaveFile;
struct LevelInfo;
class GUI;
class CScriptArray;
class ASCollisions;

const int kMaxTextElements = 40;
struct TextElement {
    bool in_use;
    TextCanvasTexture text_canvas_texture;  
};

class Level : 
    public ASNetworkCallback
{
private:
    SceneGraph* scenegraph;
public:
    const static char* DEFAULT_ENEMY_SCRIPT;// = "enemycontrol.as";
    const static char* DEFAULT_PLAYER_SCRIPT;// = "playercontrol.as";

    struct HookedASContext {
        struct {
            ASFunctionHandle init;
            ASFunctionHandle update;
            ASFunctionHandle update_deprecated;
            ASFunctionHandle hotspot_exit;
            ASFunctionHandle hotspot_enter;
            ASFunctionHandle receive_message;
            ASFunctionHandle draw_gui;
            ASFunctionHandle draw_gui2;
            ASFunctionHandle draw_gui3;
            ASFunctionHandle has_focus;
            ASFunctionHandle dialogue_camera_control;
            ASFunctionHandle save_history_state;
            ASFunctionHandle read_chunk;
            ASFunctionHandle set_window_dimensions;
            ASFunctionHandle incoming_tcp_data;
            ASFunctionHandle pre_script_reload;
            ASFunctionHandle post_script_reload;
            ASFunctionHandle menu;
			ASFunctionHandle register_mp_callbacks;
            ASFunctionHandle start_dialogue;
        } as_funcs;

        Path path;
        std::string context_name;
        ASContext* ctx;
    };

    std::vector<HookedASContext> as_contexts_;

    typedef std::set<void*> CollisionPtrSet;
    typedef std::map<void*, CollisionPtrSet> CollisionPtrMap;
    
    Level();
    virtual ~Level();

    void IncomingTCPData(SocketID socket, uint8_t* data, size_t len) override;

    void StartDialogue(const std::string& dialogue) const;
	void RegisterMPCallbacks() const;
    void Initialize(SceneGraph *scenegraph, GUI* gui);
    static void DefineLevelTypePublic(ASContext *as_context);
    void GetCollidingObjects( int id, CScriptArray *array );
    void Message( const std::string &msg );
    void Update(bool paused);
	void LiveUpdateCheck();
	void Dispose();
    void HotspotTriggered( Hotspot* hotspot, MovementObject* mo );
    void HandleCollisions( CollisionPtrMap &col_map, SceneGraph &scenegraph );
    bool HasFunction(const std::string& function_definition);
    int QueryIntFunction( const std::string &func );
    void Execute( std::string code );
    void Draw();
    void SetFromLevelInfo( const LevelInfo &li );
    void SetLevelSpecificScript( std::string& script_name );
    void SetPCScript( std::string& script_name );
    void SetNPCScript( std::string& script_name );
    void ClearNPCScript();
    void ClearPCScript();
    std::string GetLevelSpecificScript();
	std::string GetMPPCScript() const;
    std::string GetPCScript(const MovementObject* movementObject);
    std::string GetNPCScript(const MovementObject* movementObject);
	std::string GetNPCMPScript() const;
    int GetNumObjectives();
    int GetNumAchievements();
    std::string GetObjective(int which);
    std::string GetAchievement(int which);
    std::string GetAchievementsString() const;
    const std::string &GetPath(const std::string& key) const; 
    bool HasFocus();
    bool DialogueCameraControl();
    void Reset();
    
    int CreateTextElement();
    void DeleteTextElement(int which);
    TextCanvasTexture* GetTextElement(int which);
    ScriptParams& script_params();
    void SetScriptParams(const ScriptParamMap& spm);
    void SaveHistoryState( std::list<SavedChunk> & chunks, int state_id );
    void ReadChunk( const SavedChunk &the_chunk );
    bool isMetaDataDirty();
    void setMetaDataClean();
    void setMetaDataDirty();
    void ReceiveLevelEvents(int id);
    void StopReceivingLevelEvents(int id);
    void WindowResized(ivec2 value );
    void PushSpawnerItemRecent(const SpawnerItem& item);

    std::vector<SpawnerItem> GetRecentlyCreatedItems();
    NavMeshParameters nav_mesh_parameters_;
    LoadingScreen loading_screen_;
private:
	// These two are just persistent for performance
	std::vector<std::pair<Hotspot*,MovementObject*> > exit_call_queue;
	std::vector<std::pair<Hotspot*,MovementObject*> > enter_call_queue;

    bool metaDataDirty; // Has something changed that's not covered by the history system?
    ScriptParams sp_;
    std::string level_specific_script_;
    std::string pc_script_;
    std::string npc_script_;

    std::vector<SpawnerItem> recently_created_items_;

    std::vector<int> level_event_receivers;
    TextElement text_elements[kMaxTextElements];
    CharacterScriptGetter character_script_getter_;
    typedef std::set<int> CollisionSet;
    typedef std::map<int, CollisionSet> CollisionMap;
    CollisionMap old_col_map_;
    HUDImages hud_images;
    typedef std::map<std::string, std::string> StringMap;
    StringMap level_script_paths;
    // vvv These are just here to avoid per-frame mallocs vvv
    CollisionPtrMap old_col_ptr_map;

    std::unique_ptr<ASCollisions> as_collisions;

    Path FindScript(const std::string& path);
};
