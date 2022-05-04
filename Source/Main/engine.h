//-----------------------------------------------------------------------------
//           Name: engine.h
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

#include <Main/scenegraph.h>

#include <UserInput/input.h>

#include <Graphics/Cursor.h>
#include <Graphics/graphics.h>
#include <Graphics/text.h>
#include <Graphics/font_renderer.h>
#include <Graphics/animationeffectsystem.h>
#include <Graphics/lipsyncsystem.h>

#include <Sound/sound.h>
#include <Sound/threaded_sound_wrapper.h>


#include <Game/avatar_control_manager.h>
#include <Game/savefile.h>
#include <Game/scriptablecampaign.h>

#include <GUI/gui.h>
#include <Asset/assetmanager.h>
#include <Network/asnetwork.h>
#include <Internal/modloading.h>

#include <stack>
#include <iostream>
#include <mutex>

//-----------------------------------------------------------------------------
// Class Definition
//-----------------------------------------------------------------------------

struct ShadowUpdate;
class ScriptableUI;
class MovementObject;
struct SDL_Keysym;
class MapEditor;
class Timer;
class ProfilerContext;
class Multiplayer;

extern bool shadow_cache_dirty;
extern bool shadow_cache_dirty_sun_moved;

extern bool g_draw_collision;

static const int kMaxLevelHistory = 10;

enum EngineStateType {
    kEngineNoState = 0,
    kEngineLevelState,
    kEngineEditorLevelState,
    kEngineScriptableUIState,
    kEngineCampaignState //Mid stat stack injection to indicate what campaign we are currently in.
};

const char* CStrEngineStateType( const EngineStateType& state );

void SaveCollisionNormals(const SceneGraph* scenegraph);
void LoadCollisionNormals(SceneGraph* scenegraph);

void PushGPUProfileRange(const char* cstr);
void PopGPUProfileRange();

class EngineState
{
public:
    EngineState();
    EngineState(std::string id, EngineStateType _type);
    EngineState(std::string id, EngineStateType _type, Path _path);
    EngineState(std::string id, ScriptableCampaign* campaign, Path _path);

    EngineStateType type;
    
    bool pop_past;
    std::string id;
    Path path;
    ReferenceCounter<ScriptableCampaign> campaign;   
};

enum EngineStateActionType {
    kEngineStateActionPushState,
    kEngineStateActionPopState,
    kEngineStateActionPopUntilType,
    kEngineStateActionPopUntilID
};
enum ForcedSplitScreenMode {
    kForcedModeNone,
    kForcedModeFull,
    kForcedModeSplit
};

class EngineStateAction
{
public:
    bool allow_game_exit;
    EngineState state;
    EngineStateActionType type;
};

std::ostream& operator<<( std::ostream& out, const EngineState& in );

class Engine : public ModLoadingCallback {
    public:
        enum DrawingViewport { kViewport, kScreen };
        enum PostEffectsType { kStraight, kFinal };

        void Initialize();
        void GetShaderNames(std::map<std::string, int>& preload_shaders);
        void Update();
        void Draw();
        void Dispose();

        static Engine* Instance();
         
        ThreadedSound* GetSound();
        AssetManager* GetAssetManager();
        AnimationEffectSystem* GetAnimationEffectSystem();
        LipSyncSystem* GetLipSyncSystem();
        ASNetwork* GetASNetwork();
        SceneGraph* GetSceneGraph();
    
        bool IsStateQueued();
        void QueueState(const EngineState& state);
        void QueueState( const EngineStateAction& action ) ;

        void QueueErrorMessage(const std::string& title, const std::string& message);

        void AddLevelPathToRecentLevels(const Path& level_path);  // TODO: Expose some Save state to queue instead?

	void GetAvatarIds(std::vector<ObjectID> &avatars);

        /**
        * @brief function called when the "back" button is pushed, 
        * commonly escape on keyboard or b on controller. 
        * Depending on the current state this might be ignored
        * or propagated into the current state's script.
        */
        void PopQueueStateStack( bool allow_game_exit );
private:
        void LoadLevel(Path queued_level);
        void PreloadAssets(const Path &level_path);
        void LoadLevelData(const Path &level_path);

        bool back_to_menu; // Used after cache generation to queue up the state
        std::vector<Path> cache_generation_paths;
        void QueueLevelCacheGeneration(const Path& path);

        ForcedSplitScreenMode forced_split_screen_mode;
public:

        void GenerateLevelCache(ModInstance* mod_instance);

        void HandleRabbotToggleControls();

        void ClearArenaSession();
        
        void ClearLoadedLevel();
        static void StaticScriptableUICallback(void* instance, const std::string &level);
        void ScriptableUICallback(const std::string &level);
		static void NewLevel();

        void SetForcedSplitScreenMode(ForcedSplitScreenMode mode) { forced_split_screen_mode = mode; }
        bool GetSplitScreen() const;

        bool quitting_;
		bool paused;
        bool user_paused;
		bool menu_paused;
        bool slow_motion;
        bool check_save_level_changes_dialog_is_showing;
        bool check_save_level_changes_dialog_quit_if_not_cancelled;
        bool check_save_level_changes_dialog_is_finished;
        bool check_save_level_changes_last_result;
        int current_menu_player;
        std::string current_spawner_thumbnail;
        TextureAssetRef spawner_thumbnail;

		TextureAssetRef loading_screen_logo;
		TextureAssetRef loading_screen_og_logo;

		TextureAssetRef loading_screen_og_logo_casual;
		TextureAssetRef loading_screen_og_logo_hardcore;
		TextureAssetRef loading_screen_og_logo_expert;

		bool level_has_screenshot;
		TextureAssetRef level_screenshot;
		uint32_t first_level_drawn;

        SaveFile save_file_;
        GUI gui;
        EngineState current_engine_state_;

        std::map<std::string, std::string> interlevel_data;

        vec2 active_screen_start;
        vec2 active_screen_end;

        Path GetLatestMenuPath();
        Path GetLatestLevelPath();

        ScriptableCampaign* GetCurrentCampaign();
		std::string GetCurrentLevelID();
		char load_screen_tip[kPathSize];
		bool waiting_for_input_;

        std::deque<EngineState> state_history;

        void CommitPause();

		uint64_t draw_frame;

		bool loading_in_progress_;
    private:
        Path latest_level_path_;
        Path latest_menu_path_;

        void QueueLevelToLoad(const Path& level);

        static Engine* instance_;

        std::deque<EngineStateAction> queued_engine_state_;
        std::deque<std::tuple<std::string, std::string>> popup_pueue;
#ifdef WIN32
		HANDLE data_change_notification;
		HANDLE write_change_notification;
#endif
        GameCursor cursor;
        ThreadedSound sound;
        AssetManager asset_manager;
        AnimationEffectSystem particle_types;
        LipSyncSystem lip_sync_system;
        ASNetwork as_network;
        FontRenderer font_renderer;

		int started_loading_time;
		int last_loading_input_time;
        int level_updated_;
        SceneGraph *scenegraph_;
        static const int kFPSLabelMaxLen = 64;
        char fps_label_str[kFPSLabelMaxLen];
        char frame_time_label_str[kFPSLabelMaxLen];

        // Used for loading thread to tell main thread if it is done
        std::mutex loading_mutex_;     
		float finished_loading_time;

        ScriptableUI *scriptable_menu_;

        Path queued_level_;
        bool level_loaded_;

        // These are just members to avoid mallocs
	public:
        static const int kMaxAvatars = 64;
        int num_avatars;
		int num_npc_avatars;
        int avatar_ids[kMaxAvatars];
		int npc_avatar_ids[kMaxAvatars];

    private:

        //Countdown value used to delay massively frequent resizing requests.
        int resize_event_frame_counter;
        ivec2 resize_value;

        float current_global_scale_mult;

        uint64_t frame_counter;

        bool printed_rendering_error_message;

        AvatarControlManager avatar_control_manager;

    public:
        void DrawScene(DrawingViewport drawing_viewport, PostEffectsType post_effects_type, SceneGraph::SceneDrawType scene_draw_type);        
    private:

        void SetViewportForCamera(int which_cam, int num_screens, Graphics::ScreenType screen_type);

        void LoadScreenLoop(bool loading_in_progress);
        void DrawLoadScreen(bool loading_in_progress);

        void LoadConfigFile();

        void UpdateControls(float timestep, bool loading_screen);

        void DrawCubeMap(TextureRef cube_map, const vec3 &pos, GLuint cube_map_fbo, SceneGraph::SceneDrawType scene_draw_type);

        void ModActivationChange( const ModInstance* mod ) override;
public:
        void InjectWindowResizeEvent(ivec2 size);
        void SetGameSpeed(float val, bool hard);

        bool RequestedInterruptLoading();
};
