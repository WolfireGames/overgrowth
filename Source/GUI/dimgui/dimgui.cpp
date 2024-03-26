//-----------------------------------------------------------------------------
//           Name: dimgui.cpp
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
#include "dimgui.h"

#include <Internal/spawneritem.h>
#include <Internal/timer.h>
#include <Internal/config.h>
#include <Internal/dialogues.h>
#include <Internal/profiler.h>
#include <Internal/stopwatch.h>

#include <Objects/object.h>
#include <Objects/itemobject.h>
#include <Objects/prefab.h>
#include <Objects/envobject.h>
#include <Objects/movementobject.h>
#include <Objects/cameraobject.h>
#include <Objects/decalobject.h>
#include <Objects/dynamiclightobject.h>
#include <Objects/placeholderobject.h>
#include <Objects/terrainobject.h>
#include <Objects/hotspot.h>

#include <Math/enginemath.h>
#include <Math/vec3.h>
#include <Math/vec3math.h>
#include <Math/vec4.h>
#include <Math/vec4math.h>

#include <Editors/map_editor.h>
#include <Editors/sky_editor.h>

#include <Utility/imgui_macros.h>
#include <Utility/stacktrace.h>

#include <Graphics/sky.h>
#include <Graphics/pxdebugdraw.h>
#include <Graphics/model.h>
#include <Graphics/models.h>
#include <Graphics/simplify.hpp>
#include <Graphics/halfedge.h>

#include <GUI/gui.h>
#include <GUI/dimgui/dimgui_graph.h>
#include <GUI/dimgui/imgui_impl_sdl_gl3.h>
#include <GUI/dimgui/settings_screen.h>
#include <GUI/dimgui/modmenu.h>

#include <Logging/ramhandler.h>
#include <Steam/steamworks.h>
#include <Version/version.h>
#include <Scripting/angelscript/asdebugger.h>
#include <Online/online.h>
#include <Main/engine.h>
#include <UserInput/keyTranslator.h>
#include <Game/level.h>
#include <Asset/Asset/levelinfo.h>
#include <UserInput/keycommands.h>

// disable Windows.h defining min and max as macros. Windows.h is pulled in by imgui.cpp
#define NOMINMAX
// #define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include <imgui_internal.h>
#include <imgui.h>
#include <imgui_color_picker.h>
#include <implot.h>

#include <SDL.h>

#include <array>
#include <climits>

#define IMGUI_RED ImVec4(0.9f, 0.1f, 0.1f, 1.0f)
#define IMGUI_ORANGE ImVec4(0.7f, 0.4f, 0.1f, 1.0f)
#define IMGUI_GREEN ImVec4(0.1f, 0.9f, 0.1f, 1.0f)
#define IMGUI_WHITE ImVec4(1.0f, 1.0f, 1.0f, 1.0f)

static inline ImVec2 operator*(const ImVec2& lhs, const float rhs) { return ImVec2(lhs.x * rhs, lhs.y * rhs); }
static inline ImVec2 operator/(const ImVec2& lhs, const float rhs) { return ImVec2(lhs.x / rhs, lhs.y / rhs); }
static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }
static inline ImVec2 operator*(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x * rhs.x, lhs.y * rhs.y); }
static inline ImVec2 operator/(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x / rhs.x, lhs.y / rhs.y); }
static inline ImVec2& operator*=(ImVec2& lhs, const float rhs) {
    lhs.x *= rhs;
    lhs.y *= rhs;
    return lhs;
}
static inline ImVec2& operator/=(ImVec2& lhs, const float rhs) {
    lhs.x /= rhs;
    lhs.y /= rhs;
    return lhs;
}
static inline ImVec2& operator+=(ImVec2& lhs, const ImVec2& rhs) {
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    return lhs;
}
static inline ImVec2& operator-=(ImVec2& lhs, const ImVec2& rhs) {
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    return lhs;
}
static inline ImVec2& operator*=(ImVec2& lhs, const ImVec2& rhs) {
    lhs.x *= rhs.x;
    lhs.y *= rhs.y;
    return lhs;
}
static inline ImVec2& operator/=(ImVec2& lhs, const ImVec2& rhs) {
    lhs.x /= rhs.x;
    lhs.y /= rhs.y;
    return lhs;
}
static inline ImVec4 operator+(const ImVec4& lhs, const ImVec4& rhs) { return ImVec4(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w); }
static inline ImVec4 operator-(const ImVec4& lhs, const ImVec4& rhs) { return ImVec4(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w); }
static inline ImVec4 operator*(const ImVec4& lhs, const ImVec4& rhs) { return ImVec4(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w); }

char imgui_ini_path[kPathSize];

typedef std::vector<SpawnerItem> SpawnerTab;
typedef std::map<std::string, SpawnerTab> SpawnerTabMap;
SpawnerTabMap spawner_tabs;

ImGuiTextFilter scenegraph_filter;
ImGuiTextFilter spawner_global_filter;
typedef std::map<std::string, ImGuiTextFilter> SpawnerTabFilterMap;
SpawnerTabFilterMap spawner_tab_filters;

static AssetType asset_detail_list_type;
static std::vector<std::string> asset_detail_list;
static std::vector<uint32_t> asset_detail_list_load_flags;
static std::vector<uint16_t> asset_detail_list_ref_count;
static std::vector<bool> asset_detail_list_load_warning;
static const char* asset_detail_title = "";

bool show_test_window = false;
bool show_scenegraph = false;
bool select_scenegraph_search = false;
bool select_scenegraph_level_params = false;
bool search_children_scenegraph = false;
bool show_flat_scenegraph = false;
bool show_named_only_scenegraph = false;
bool show_selected = false;
bool select_object_script_params = false;
bool show_paintbrush = false;
bool show_color_picker = false;
bool show_mod_menu = false;
bool show_mod_menu_previous = false;
bool show_performance = false;
bool show_mp_debug = false;
bool show_mp_settings = false;
bool show_log = false;
bool show_warnings = false;
bool show_state = false;
bool show_events = false;
bool show_save = false;
bool show_sound = false;
bool show_mp_info = true;
bool show_input_debug = false;
bool show_chat = true;
bool show_build_watermark = true;
bool show_pose = false;
bool show_script = false;
bool show_select_script = false;

std::string preview_script_name = "";

float imgui_scale = 1.0f;

std::map<std::string, ImGUIGraph<256>> graphs;

// ImGUIGraph<256> host_walltime_offset_shift;
// ImGUIGraph<128> interpolation_step;
// ImGUIGraph<128> rigged_interpolation_time;
// ImGUIGraph<1000> interpolation_speed;
// ImGUIGraph<1000> playerVelocity;
// ImGUIGraph<1000> delta_time;
// ImGUIGraph<256> pendingBonesHistory;
// ImGUIGraph<256> pendingPosHistory;
// ImGUIGraph<256> frametimeHistory;
// ImGUIGraph<256> deltabetweenframes;
// ImGUIGraph<256> root_bone_x;
// ImGUIGraph<256> root_bone_y;
// ImGUIGraph<256> root_bone_z;
// ImGUIGraph<256> last_frame_delta;

extern std::string script_dir_path;

extern bool asdebugger_enabled;
extern bool asprofiler_enabled;
extern bool g_make_invisible_visible;
extern bool g_level_shadows;
extern bool g_simple_shadows;
bool show_asdebugger_contexts = false;
bool show_asprofiler = false;
bool break_on_script_change = false;

bool show_load_item_search = false;
static bool run_parse_spawner_flag;

extern Timer game_timer;
float GraphTime = 0.f;

extern RamHandler ram_handler;

static int select_start_index = -1;

static bool show_graphics_debug_disable_menu = false;

extern bool g_debug_runtime_disable_blood_surface_pre_draw;
extern bool g_debug_runtime_disable_debug_draw;
extern bool g_debug_runtime_disable_debug_ribbon_draw;
extern bool g_debug_runtime_disable_decal_object_draw;
extern bool g_debug_runtime_disable_decal_object_pre_draw_frame;
extern bool g_debug_runtime_disable_detail_object_surface_draw;
extern bool g_debug_runtime_disable_detail_object_surface_pre_draw;
extern bool g_debug_runtime_disable_dynamic_light_object_draw;
extern bool g_debug_runtime_disable_env_object_draw;
extern bool g_debug_runtime_disable_env_object_draw_depth_map;
extern bool g_debug_runtime_disable_env_object_draw_detail_object_instances;
extern bool g_debug_runtime_disable_env_object_draw_instances;
extern bool g_debug_runtime_disable_env_object_draw_instances_transparent;
extern bool g_debug_runtime_disable_env_object_pre_draw_camera;
extern bool g_debug_runtime_disable_flares_draw;
extern bool g_debug_runtime_disable_gpu_particle_field_draw;
extern bool g_debug_runtime_disable_group_pre_draw_camera;
extern bool g_debug_runtime_disable_group_pre_draw_frame;
extern bool g_debug_runtime_disable_hotspot_draw;
extern bool g_debug_runtime_disable_hotspot_pre_draw_frame;
extern bool g_debug_runtime_disable_item_object_draw;
extern bool g_debug_runtime_disable_item_object_draw_depth_map;
extern bool g_debug_runtime_disable_item_object_pre_draw_frame;
extern bool g_debug_runtime_disable_morph_target_pre_draw_camera;
extern bool g_debug_runtime_disable_movement_object_draw;
extern bool g_debug_runtime_disable_movement_object_draw_depth_map;
extern bool g_debug_runtime_disable_movement_object_pre_draw_camera;
extern bool g_debug_runtime_disable_movement_object_pre_draw_frame;
extern bool g_debug_runtime_disable_navmesh_connection_object_draw;
extern bool g_debug_runtime_disable_navmesh_hint_object_draw;
extern bool g_debug_runtime_disable_particle_draw;
extern bool g_debug_runtime_disable_particle_system_draw;
extern bool g_debug_runtime_disable_pathpoint_object_draw;
extern bool g_debug_runtime_disable_placeholder_object_draw;
extern bool g_debug_runtime_disable_reflection_capture_object_draw;
extern bool g_debug_runtime_disable_rigged_object_draw;
extern bool g_debug_runtime_disable_rigged_object_pre_draw_camera;
extern bool g_debug_runtime_disable_rigged_object_pre_draw_frame;
extern bool g_debug_runtime_disable_scene_graph_draw;
extern bool g_debug_runtime_disable_scene_graph_draw_depth_map;
extern bool g_debug_runtime_disable_scene_graph_prepare_lights_and_decals;
extern bool g_debug_runtime_disable_sky_draw;
extern bool g_debug_runtime_disable_terrain_object_draw_depth_map;
extern bool g_debug_runtime_disable_terrain_object_draw_terrain;
extern bool g_debug_runtime_disable_terrain_object_pre_draw_camera;

#ifdef _WIN32
#include <shellapi.h>
static void open_url(const char* url) {
#if ENABLE_STEAMWORKS
    if (Steamworks::Instance()->IsConnected()) {
        Steamworks::Instance()->OpenWebPage(url);
        return;
    }
#endif

    ShellExecute(GetActiveWindow(),
                 "open", url, NULL, NULL, SW_SHOWNORMAL);
}
#elif defined(PLATFORM_MACOSX)
#include <ApplicationServices/ApplicationServices.h>
static void open_url(const char* url) {
#if ENABLE_STEAMWORKS
    if (Steamworks::Instance()->IsConnected()) {
        Steamworks::Instance()->OpenWebPage(url);
        return;
    }
#endif
    CFURLRef url_ref = CFURLCreateWithBytes(NULL, (UInt8*)url, strlen(url), kCFStringEncodingASCII, NULL);
    LSOpenCFURLRef(url_ref, NULL);
    CFRelease(url_ref);
}
#else
static void open_url(const char* url) {
#if ENABLE_STEAMWORKS
    if (Steamworks::Instance()->IsConnected()) {
        Steamworks::Instance()->OpenWebPage(url);
        return;
    }
#endif
    DisplayError("Error", "Browser open is not yet implemented on this OS");
}
#endif

static vec4 GetObjColor(Object* obj) {
    vec4 color(1.0f);
    switch (obj->GetType()) {
        case _env_object:
            color = vec4(0.8f, 0.8f, 1.0f, 1.0f);
            break;
        case _movement_object:
            color = vec4(1.0f, 0.8f, 0.8f, 1.0f);
            break;
        case _hotspot_object:
            color = vec4(1.0f, 1.0f, 0.8f, 1.0f);
            break;
        case _decal_object:
            color = vec4(0.8f, 1.0f, 0.8f, 1.0f);
            break;
        default:
            color = vec4(0.9f, 0.9f, 0.9f, 1.0f);
            break;
    }
    if (obj->exclude_from_save) {
        color[3] *= 0.5f;
    }
    return color;
}

static int ParseDetails(const char* str, float* results) {
    // example str: "min:0,max:2,step:0.001,text_mult:100"
    // parse that into results = {0.0f, 2.0f, 0.001f, 100.0f}
    // return 0 on success, -1 on failure
    int last_break = -1;
    int last_colon = -1;
    for (int index = 0;; ++index) {
        if (str[index] == ':') {
            last_colon = index;
        }
        if (str[index] == ' ' && index == last_break + 1) {
            last_break = index;
        }
        if (str[index] == ',' || str[index] == '\0') {
            if (last_colon <= last_break) {
                return -1;
            }
            const char* token = &str[last_break + 1];
            int token_len = last_colon - last_break - 1;
            float val = (float)atof(&str[last_colon + 1]);
            switch (token_len) {
                case 3:
                    if (strncmp(token, "min", 3) == 0) {
                        results[0] = val;
                    } else if (strncmp(token, "max", 3) == 0) {
                        results[1] = val;
                    } else {
                        return -1;
                    }
                    break;
                case 4:
                    if (strncmp(token, "step", 4) == 0) {
                        results[2] = val;
                    } else {
                        return -1;
                    }
                    break;
                case 9:
                    if (strncmp(token, "text_mult", 9) == 0) {
                        results[3] = val;
                    } else {
                        return -1;
                    }
                    break;
                default:
                    return -1;
            }
            last_break = index;
        }
        if (str[index] == '\0') {
            break;
        }
    }
    for (int i = 0; i < 4; ++i) {
        if (results[i] == FLT_MAX) {
            return -1;
        }
    }
    return 0;
}

static void DrawColorPicker(Object** selected, unsigned selected_count, SceneGraph* scenegraph) {
    MapEditor* me = scenegraph->map_editor;
    bool color_changed = false;
    vec4 color;

    ImGui::Text("Color history");
    ImGui::BeginGroup();
    for (int i = 0; i < MapEditor::kColorHistoryLen; ++i) {
        vec4 col = me->GetColorHistoryIndex(i);
        ImVec4 imCol(col.x(), col.y(), col.z(), col.w());
        ImGui::PushID(i);
        if (ImGui::ColorButton("##histcolbutt", imCol, false)) {
            color.x() = imCol.x;
            color.y() = imCol.y;
            color.z() = imCol.z;
            color_changed = true;
        }
        ImGui::PopID();

        if (i == 0 || (i + 1) % 10 != 0)
            ImGui::SameLine(0, -1);
    }
    ImGui::EndGroup();

    // Show color picker if no objects are selected
    bool only_movement_objects_selected = (selected_count != 0);
    for (unsigned selected_i = 0; selected_i < selected_count; ++selected_i) {
        if (selected[selected_i]->GetType() != _movement_object) {
            only_movement_objects_selected = false;
            break;
        }
    }

    const size_t MAX_PALETTES = 24;
    const char* labeled_colors[MAX_PALETTES];
    unsigned labeled_color_count = 0;

    if (only_movement_objects_selected) {
        static int palette_index = 0;
        int max_palette_index = 0;
        for (unsigned selected_i = 0; selected_i < selected_count; ++selected_i) {
            MovementObject* mo = (MovementObject*)selected[selected_i];
            OGPalette* palette = mo->GetPalette();
            for (auto& i : *palette) {
                LabeledColor* labeled_color = &i;
                bool found = false;
                for (unsigned palette_i = 0; palette_i < labeled_color_count; palette_i++) {
                    if (strcmp(labeled_colors[palette_i], labeled_color->label.c_str()) == 0) {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    if (labeled_color_count < MAX_PALETTES) {
                        labeled_colors[labeled_color_count] = labeled_color->label.c_str();
                        labeled_color_count++;
                    }
                }
            }
        }

        if (labeled_color_count != 0) {
            palette_index = min(palette_index, (int)labeled_color_count - 1);
            char dropdown_text[512];
            size_t dropdown_text_length = 0;
            for (unsigned i = 0; i < labeled_color_count; ++i) {
                strcpy(&dropdown_text[dropdown_text_length], labeled_colors[i]);
                dropdown_text_length += strlen(labeled_colors[i]) + 1;
            }
            dropdown_text[dropdown_text_length] = '\0';
            ImGui::Combo("Palette", &palette_index, dropdown_text);

            if (!color_changed) {
                for (unsigned selected_i = 0; selected_i < selected_count; ++selected_i) {
                    MovementObject* mo = (MovementObject*)selected[selected_i];
                    OGPalette* palette = mo->GetPalette();
                    for (auto& i : *palette) {
                        LabeledColor* labeled_color = &i;
                        bool found = false;
                        if (strcmp(labeled_colors[palette_index], labeled_color->label.c_str()) == 0) {
                            color = labeled_color->color;
                            goto breakselectedloop;
                        }
                    }
                }
            }

        breakselectedloop:
            if (ColorPicker(&color[0], NULL, false, false) || color_changed) {
                if (!color_changed)
                    me->AddColorToHistory(color);

                for (unsigned selected_i = 0; selected_i < selected_count; ++selected_i) {
                    MovementObject* mo = (MovementObject*)selected[selected_i];
                    OGPalette* palette = mo->GetPalette();
                    bool found = false;
                    for (size_t i = 0, len = palette->size(); i < len; ++i) {
                        LabeledColor* labeled_color = &palette->at(i);
                        if (strcmp(labeled_colors[palette_index], labeled_color->label.c_str()) == 0) {
                            labeled_color->color = color.xyz();
                            mo->ApplyPalette(*palette);
                            break;
                        }
                    }
                }
            }
        }
    } else {
        float overbright(0.0f);

        if (!color_changed) {
            for (unsigned i = 0; i < selected_count; ++i) {
                if (selected[i]->GetType() == _env_object) {
                    color = ((EnvObject*)selected[i])->GetColorTint();
                    overbright = ((EnvObject*)selected[i])->GetOverbright();
                    break;
                } else if (selected[i]->GetType() == _item_object) {
                    color = ((ItemObject*)selected[i])->GetColorTint();
                    overbright = ((ItemObject*)selected[i])->GetOverbright();
                    break;
                } else if (selected[i]->GetType() == _decal_object) {
                    color = ((DecalObject*)selected[i])->color_tint_component_.tint_;
                    overbright = ((DecalObject*)selected[i])->color_tint_component_.overbright_;
                    break;
                } else if (selected[i]->GetType() == _dynamic_light_object) {
                    color = ((DynamicLightObject*)selected[i])->GetTint();
                    overbright = ((DynamicLightObject*)selected[i])->GetOverbright();
                }
            }
        }

        if (ColorPicker(&color[0], &overbright, false, true) || color_changed) {
            if (!color_changed)
                me->AddColorToHistory(color);

            for (auto obj : scenegraph->objects_) {
                if (obj->Selected()) {
                    if (obj->GetType() == _env_object) {
                        EnvObject* eo = (EnvObject*)obj;
                        eo->ReceiveObjectMessage(OBJECT_MSG::SET_COLOR, &color);
                        eo->ReceiveObjectMessage(OBJECT_MSG::SET_OVERBRIGHT, &overbright);
                    }
                    if (obj->GetType() == _item_object) {
                        ItemObject* io = (ItemObject*)obj;
                        io->ReceiveObjectMessage(OBJECT_MSG::SET_COLOR, &color);
                        io->ReceiveObjectMessage(OBJECT_MSG::SET_OVERBRIGHT, &overbright);
                    }
                    if (obj->GetType() == _decal_object) {
                        DecalObject* decalo = (DecalObject*)obj;
                        decalo->ReceiveObjectMessage(OBJECT_MSG::SET_COLOR, &color);
                        decalo->ReceiveObjectMessage(OBJECT_MSG::SET_OVERBRIGHT, &overbright);
                    }
                    if (obj->GetType() == _dynamic_light_object) {
                        DynamicLightObject* dlo = (DynamicLightObject*)obj;
                        dlo->ReceiveObjectMessage(OBJECT_MSG::SET_COLOR, &color);
                        dlo->ReceiveObjectMessage(OBJECT_MSG::SET_OVERBRIGHT, &overbright);
                    }
                }
            }
            scenegraph->map_editor->QueueSaveHistoryState();
        }
    }
}

static void DrawAudioDebug() {
    std::vector<SoundSourceInfo> sound_sources = Engine::Instance()->GetSound()->GetCurrentSoundSources();
    if (ImGui::TreeNode("Sounds", "Sounds: %d", (int)sound_sources.size())) {
        for (auto& ss : sound_sources) {
            ImGui::Text("%s :%f %f %f", ss.name, ss.pos[0], ss.pos[1], ss.pos[2]);
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Handle Count", "Handle Count: %d", (int)Engine::Instance()->GetSound()->GetSoundHandleCount())) {
        std::map<wrapper_sound_handle, real_sound_handle> handles = Engine::Instance()->GetSound()->GetAllHandles();
        std::map<wrapper_sound_handle, real_sound_handle>::iterator hit = handles.begin();

        for (; hit != handles.end(); hit++) {
            ImGui::Text("%s %s: %s", Engine::Instance()->GetSound()->GetID(hit->first).c_str(), Engine::Instance()->GetSound()->GetName(hit->first).c_str(), Engine::Instance()->GetSound()->GetType(hit->first).c_str());
        }
        ImGui::TreePop();
    }

    ImGui::Text("Sound Instances: %d", (int)Engine::Instance()->GetSound()->GetSoundInstanceCount());

    ImGui::Text("Current Song: %s", Engine::Instance()->GetSound()->GetSongName().c_str());
    ImGui::Text("Current Song Type: %s", Engine::Instance()->GetSound()->GetSongType().c_str());

    if (strmtch(Engine::Instance()->GetSound()->GetSongType().c_str(), "layered")) {
        std::vector<std::string> layers = Engine::Instance()->GetSound()->GetLayerNames();
        if (ImGui::TreeNode("Layers", "Layers: %d", (int)layers.size())) {
            static float v = 0.0f;
            static std::string selected_layer_name = "";

            for (auto& layer : layers) {
                float layer_gain = Engine::Instance()->GetSound()->GetLayerGain(layer);
                ImGui::Text("%s %f", layer.c_str(), layer_gain);

                if (ImGui::IsItemClicked()) {
                    if (selected_layer_name == layer) {
                        selected_layer_name = "";
                    } else {
                        selected_layer_name = layer;
                        v = layer_gain;
                    }
                }

                if (selected_layer_name == layer) {
                    if (ImGui::DragFloat(layer.c_str(), &v, 0.01f, 0.0f, 1.0f)) {
                        LOGI << "Settings layer gain" << std::endl;
                        Engine::Instance()->GetSound()->SetLayerGain(layer, v);
                    }
                }
            }

            ImGui::TreePop();
        }
    }

    ImGui::Text("Next Song: %s", Engine::Instance()->GetSound()->GetNextSongName().c_str());
    ImGui::Text("Next Song Type: %s", Engine::Instance()->GetSound()->GetNextSongType().c_str());
}

static void DrawDebugText(GUI* gui) {
    uint32_t curr_ticks = SDL_TS_GetTicks();
    for (GUI::DebugTextMap::iterator iter = gui->debug_text.begin();
         iter != gui->debug_text.end();) {
        GUI::DebugTextEntry* debug_text_entry = &(iter->second);
        if (debug_text_entry->delete_time >= curr_ticks) {
            ImGui::Text(debug_text_entry->text.c_str());
            ++iter;
        } else {
            gui->debug_text.erase(iter++);
        }
    }
}

static void DrawDebugText(GUI* gui, const std::string& text) {
    GUI::DebugTextMap::iterator iter = gui->debug_text.find(text);
    if (iter != gui->debug_text.end()) {
        ImGui::Text(iter->second.text.c_str());
    }
}

// Return true if anything was changed
static bool DrawScriptParamsEditor(ScriptParams* params) {
    Online* online = Online::Instance();
    const int kBufSize = 256;
    char buf[kBufSize];
    bool changed = false;
    bool rename = false;
    bool to_delete = false;
    char rename_buf[2][kBufSize];
    const ScriptParamMap& spm = params->GetParameterMap();
    ImGui::Columns(3);
    int id = 0;
    for (const auto& iter : spm) {
        const ScriptParam& sp = iter.second;
        const ScriptParamParts::Editor& editor = sp.editor();
        FormatString(buf, kBufSize, "%s", iter.first.c_str());
        ImGui::PushItemWidth(-1);  // Don't make space for invisible labels
        ImGui::PushID(id++);
        if (ImGui::InputText("###inputnamething", buf, kBufSize, ImGuiInputTextFlags_EnterReturnsTrue)) {
            FormatString(rename_buf[0], kBufSize, "%s", iter.first.c_str());
            FormatString(rename_buf[1], kBufSize, "%s", buf);
            rename = true;
        }
        ImGui::PopID();
        ImGui::PopItemWidth();
        ImGui::NextColumn();
        /*
        UNDEFINED,
        TEXTFIELD,
        DISPLAY_SLIDER,
        COLOR_PICKER,
        CHECKBOX,
        DROPDOWN_MENU,
        MULTI_SELECT,
        CUSTOM_WINDOW_LAUNCHER*/

        ImGui::PushItemWidth(-1);  // Don't make space for invisible labels
        ImGui::PushID(id++);
        switch (editor.type()) {
            case ScriptParamEditorType::CHECKBOX: {
                bool checked = (sp.GetInt() == 1);
                if (ImGui::Checkbox("", &checked)) {
                    params->ASSetInt(iter.first, checked ? 1 : 0);
                    changed = true;
                }
                break;
            }
            case ScriptParamEditorType::COLOR_PICKER: {
                vec3 color = ColorFromString(sp.GetString().c_str());
                if (ImGui::ColorEdit3("", &color[0])) {
                    FormatString(buf, kBufSize, "%d, %d, %d", (int)(color[0] * 255), (int)(color[1] * 255), (int)(color[2] * 255));
                    params->ASSetString(iter.first, buf);
                    changed = true;
                }
                break;
            }
            case ScriptParamEditorType::DISPLAY_SLIDER: {
                const char* details = editor.GetDetails().c_str();
                float details_flt[4] = {FLT_MAX, FLT_MAX, 1.0f, 1.0f};
                if (ParseDetails(details, details_flt) == 0) {
                    bool is_float = false;
                    if (sp.GetFloat() != (int)sp.GetFloat() ||
                        details_flt[0] != (int)details_flt[0] ||
                        details_flt[1] != (int)details_flt[1] ||
                        details_flt[2] != (int)details_flt[2] ||
                        details_flt[3] != (int)details_flt[3]) {
                        is_float = true;
                    }
                    if (is_float) {
                        float val = sp.GetFloat() * details_flt[3];
                        if (ImGui::SliderFloat("", &val, details_flt[0] * details_flt[3], details_flt[1] * details_flt[3])) {
                            params->ASSetFloat(iter.first, val / details_flt[3]);
                            changed = true;
                        }
                    } else {
                        int val = (int)(sp.GetInt() * details_flt[3]);
                        if (ImGui::SliderInt("", &val, (int)(details_flt[0] * details_flt[3]), (int)(details_flt[1] * details_flt[3]))) {
                            params->ASSetInt(iter.first, (int)(val / details_flt[3]));
                            changed = true;
                        }
                    }
                }
                break;
            }
            case ScriptParamEditorType::TEXTFIELD: {
                if (sp.IsString()) {
                    // Due to some problem in ImGUI this value can't grow during runtime. Limits input.
                    const size_t kLargerBufSize = 1024 * 16;
                    if (sp.GetString().size() < (kBufSize - 1)) {
                        FormatString(buf, kBufSize, "%s", sp.GetString().c_str());
                        if (ImGui::InputText("##smallertextfield", buf, kBufSize)) {
                            params->ASSetString(iter.first, buf);
                            changed = true;
                        }
                    } else if (sp.GetString().size() < kLargerBufSize) {
                        char* larger_buf = (char*)alloc.stack.Alloc(kLargerBufSize);
                        FormatString(larger_buf, kLargerBufSize, "%s", sp.GetString().c_str());
                        if (ImGui::InputTextMultiline("##largertextfield", larger_buf, kLargerBufSize)) {
                            params->ASSetString(iter.first, larger_buf);
                            changed = true;
                        }
                        alloc.stack.Free(larger_buf);
                    } else {
                        ImGui::TextWrapped("String is too large to display");
                    }
                } else if (sp.IsInt()) {
                    int val = sp.GetInt();
                    if (ImGui::InputInt("", &val)) {
                        params->ASSetInt(iter.first, val);
                        changed = true;
                    }
                } else if (sp.IsFloat()) {
                    int val = (int)sp.GetFloat();
                    if (ImGui::InputInt("", &val)) {
                        params->ASSetFloat(iter.first, (float)val);
                        changed = true;
                    }
                }
                break;
            }
            default:
                ImGui::LabelText("", sp.AsString().c_str());
        }
        ImGui::PopID();
        ImGui::PopItemWidth();
        ImGui::NextColumn();
        ImGui::PushID(id++);
        if (ImGui::Button("x")) {
            to_delete = true;
            FormatString(rename_buf[0], kBufSize, "%s", iter.first.c_str());
        }
        ImGui::PopID();
        ImGui::NextColumn();
    }
    ImGui::Columns(1);
    if (ImGui::Button("New parameter")) {
        ScriptParamMap spm = params->GetParameterMap();
        ScriptParam sp;
        sp.SetString("");
        params->InsertNewScriptParam("Untitled parameter", sp);
        changed = true;
    }
    if (rename) {
        ScriptParamMap spm = params->GetParameterMap();
        ScriptParamMap::iterator iter = spm.find(std::string(rename_buf[0]));

        if (iter != spm.end()) {
            ScriptParam sp = iter->second;
            spm.erase(iter);
            spm[rename_buf[1]] = sp;
            params->SetParameterMap(spm);
            changed = true;
            if (online->IsActive()) {
                online->Send<OnlineMessages::SPRenameMessage>(params->GetObjectID(), std::string(rename_buf[1]), std::string(rename_buf[0]));
            }
        }
    }
    if (to_delete) {
        ScriptParamMap spm = params->GetParameterMap();
        ScriptParamMap::iterator iter = spm.find(std::string(rename_buf[0]));
        if (iter != spm.end()) {
            spm.erase(iter);
            params->SetParameterMap(spm);
            changed = true;
            if (online->IsActive()) {
                online->Send<OnlineMessages::SPRemoveMessage>(params->GetObjectID(), std::string(rename_buf[0]));
            }
        }
    }
    return changed;
}

#include <Objects/itemobjectscriptreader.h>
#include <Objects/pathpointobject.h>
#include <Objects/navmeshconnectionobject.h>

void GetDisplayName(Object* obj, std::vector<char>& buffer) {
    char temp[512];
    temp[0] = '\0';
    if (!obj->GetName().empty()) {
        sprintf(temp, "%d, ", obj->GetID());
    }
    obj->GetDisplayName(temp + strlen(temp), 512);
    size_t length = strlen(temp) + 1;
    size_t old_size = buffer.size();
    buffer.resize(old_size + length);
    memcpy(buffer.data() + old_size, temp, length);
}

static void PrintConnections(MovementObject* mov,
                             const char* name,
                             const std::list<ItemObjectScriptReader>& list,
                             AttachmentType type,
                             bool mirrored,
                             bool show_connected,
                             bool only_named) {
    if (ImGui::TreeNodeEx(name)) {
        if (ImGui::BeginListBox("Connected")) {
            for (const auto& it : list) {
                if (it.attachment_type == type && it.attachment_mirror == mirrored) {
                    Object* obj = it.obj;
                    char buffer[512];
                    obj->GetDisplayName(buffer, 512);
                    if (ImGui::Selectable(buffer, false)) {
                        mov->Disconnect(*obj);
                    }
                }
            }
            ImGui::EndListBox();
        }

        if (ImGui::BeginListBox("Available")) {
            for (size_t i = 0; i < mov->scenegraph_->item_objects_.size(); ++i) {
                ItemObject* obj = (ItemObject*)mov->scenegraph_->item_objects_[i];
                if ((!show_connected && obj->HeldByWhom() != -1) || (only_named && obj->GetName().empty())) {
                    continue;
                }

                bool found = false;
                for (const auto& it : list) {
                    if (obj->GetID() == it->GetID() && it.attachment_type == type && it.attachment_mirror == mirrored) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    const ItemRef& ref = obj->item_ref();
                    if (ref->HasAttachment(type)) {
                        int held_by_id = obj->HeldByWhom();
                        Object* held_by = NULL;
                        if (held_by_id != -1)
                            held_by = obj->scenegraph_->GetObjectFromID(held_by_id);

                        char buffer[512];
                        obj->GetDisplayName(buffer, 512);

                        size_t length = strlen(buffer);
                        if (held_by != NULL) {
                            if (held_by->GetName().empty()) {
                                length += sprintf(buffer + length, " held by %d", held_by->GetID());
                            } else {
                                length += sprintf(buffer + length, " held by %d (%s)", held_by->GetID(), held_by->GetName().c_str());
                            }
                        }

                        if (ImGui::Selectable(buffer, false)) {
                            AttachmentRef attachment_ref;
                            mov->AttachItemToSlotEditor(obj->GetID(), type, mirrored, attachment_ref);
                        }
                    }
                }
            }
            ImGui::EndListBox();
        }
        ImGui::TreePop();
    }
}

static void DrawLaunchCustomGuiButton(Object* obj, bool& are_script_params_read_only, int button_instance_id = -1) {
    if (button_instance_id != -1) {
        ImGui::PushID(button_instance_id);
    }

    if (obj->GetType() == _hotspot_object) {
        Hotspot* hotspot = (Hotspot*)obj;

        if (hotspot->HasCustomGUI() && ImGui::Button("Open custom editor")) {
            hotspot->LaunchCustomGUI();
        }

        are_script_params_read_only = hotspot->ObjectInspectorReadOnly();
    } else if (obj->GetType() == _placeholder_object) {
        PlaceholderObject* placeholder = (PlaceholderObject*)obj;

        if (placeholder->GetScriptParams()->HasParam("Dialogue")) {
            if (ImGui::Button("Launch dialogue editor")) {
                char buffer[64];
                sprintf(buffer, "edit_dialogue_id%i", placeholder->GetID());
                placeholder->scenegraph_->level->Message(buffer);
            }

            are_script_params_read_only = true;
        }
    }

    if (button_instance_id != -1) {
        ImGui::PopID();
    }
}

static void DrawObjectInfoFlat(Object* obj) {
    ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
    obj->DrawImGuiEditor();
    ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());

    bool are_script_params_read_only = false;
    DrawLaunchCustomGuiButton(obj, are_script_params_read_only);

    bool temp_enabled = obj->enabled_;
    if (ImGui::Checkbox("Enabled", &temp_enabled)) {
        obj->SetEnabled(temp_enabled);
    }

    const size_t name_len = 64;
    char name[name_len];
    strscpy(name, obj->GetName().c_str(), name_len);
    if (ImGui::InputText("Name", name, name_len)) {
        obj->SetName(std::string(name));
        obj->scenegraph_->map_editor->QueueSaveHistoryState();
    }

    if (obj->GetType() == _prefab) {
        Prefab* prefab = static_cast<Prefab*>(obj);

        bool prefab_locked;
        prefab_locked = prefab->prefab_locked;

        if (ImGui::Checkbox("Prefab Locked", &prefab_locked)) {
            prefab->prefab_locked = prefab_locked;
            prefab->scenegraph_->map_editor->QueueSaveHistoryState();
        }

        ImGui::Text("Prefab Source: %s", prefab->prefab_path.GetOriginalPath());
    }

    bool changed = false;

    vec3 translation = obj->GetTranslation();
    if (ImGui::DragFloat3("Translation", &translation[0], 0.01f) || ImGui::IsItemActive()) {
        obj->SetTranslation(translation);
        changed = true;
    }

    vec3 scale = obj->GetScale();
    if (ImGui::DragFloat3("Scale", &scale[0], 0.01f) || ImGui::IsItemActive()) {
        obj->SetScale(scale);
        changed = true;
    }

    vec3 euler_angles = obj->GetRotationEuler();
    euler_angles *= 180.0f / (float)M_PI;

    if (ImGui::DragFloat3("Rotation", &euler_angles[0], 1.0f, 0.0f, 0.0f, "%.0f") || ImGui::IsItemActive()) {
        obj->SetRotationEuler(euler_angles * (float)M_PI / 180.0f);
        changed = true;
    }

    if (ImGui::Button("Reset Rotation")) {
        obj->SetRotation(quaternion());
        changed = true;
    }

    if (changed) {
        obj->scenegraph_->UpdatePhysicsTransforms();
        obj->scenegraph_->map_editor->QueueSaveHistoryState();
    }

    SceneGraph* scenegraph = obj->scenegraph_;
    ImGui::Separator();
    bool show_connections = false;
    switch (obj->GetType()) {
        case _movement_object: {
            static bool only_named = false;
            ImGui::Text("Connected objects");
            ImGui::Checkbox("Show only named items in available list", &only_named);
            static bool show_connected = true;
            ImGui::Checkbox("Show items with connections in available list", &show_connected);
            MovementObject* mov = (MovementObject*)obj;
            std::vector<char> text;
            text.reserve(1024);
            text.resize(strlen("Nothing") + 1, '\0');
            strcpy(text.data(), "Nothing");
            int item_index = 0;
            int path_point_count = 0;
            for (auto obj : scenegraph->path_points_) {
                if (!(only_named && obj->GetName().empty())) {
                    GetDisplayName(obj, text);
                    if (obj->GetID() == mov->connected_pathpoint_id) {
                        item_index = path_point_count + 1;
                    }
                    path_point_count++;
                }
            }
            int movement_object_count = 0;
            for (auto obj : scenegraph->movement_objects_) {
                if (!(only_named && obj->GetName().empty()) && mov->GetID() != obj->GetID()) {
                    GetDisplayName(obj, text);
                    if (obj->GetID() == mov->connected_pathpoint_id) {
                        item_index = path_point_count + movement_object_count + 1;
                    }
                    movement_object_count++;
                }
            }
            text.push_back('\0');
            if (ImGui::Combo("Path object", &item_index, text.data())) {
                if (item_index == 0 && mov->connected_pathpoint_id != -1) {
                    mov->Disconnect(*scenegraph->GetObjectFromID(mov->connected_pathpoint_id));
                } else {
                    size_t offset = 0;
                    for (int i = 0; i < item_index; ++i) {
                        offset += strlen(text.data() + offset) + 1;
                    }
                    int id = atoi(text.data() + offset);
                    mov->ConnectTo(*scenegraph->GetObjectFromID(id));
                }
            }

            std::list<ItemObjectScriptReader> list = mov->item_connections;
            PrintConnections(mov, "Grip", list, _at_grip, false, show_connected, only_named);
            PrintConnections(mov, "Mirrored grip", list, _at_grip, true, show_connected, only_named);
            PrintConnections(mov, "Sheathe", list, _at_sheathe, false, show_connected, only_named);
            PrintConnections(mov, "Mirrored sheathe", list, _at_sheathe, true, show_connected, only_named);

            if (ImGui::TreeNode("Connected objects")) {
                show_connections = true;
                RiggedObject* rig = mov->rigged_object();
                if (rig && !rig->children.empty()) {
                    if (ImGui::BeginListBox("Attached objects")) {
                        for (auto& i : rig->children) {
                            Object* ptr = i.direct_ptr;
                            char buffer[512];
                            ptr->GetDisplayName(buffer, 512);
                            if (ImGui::Selectable(buffer, false)) {
                                ptr->SetParent(NULL);
                            }
                        }
                        ImGui::EndListBox();
                    }
                }
            }
            break;
        }
        case _item_object: {
            ItemObject* item = (ItemObject*)obj;
            Object* held_by = NULL;
            if (item->HeldByWhom() != -1) {
                held_by = scenegraph->GetObjectFromID(item->HeldByWhom());
                if (ImGui::Button("X")) {
                    item->Disconnect(*held_by);
                } else {
                    ImGui::SameLine();
                    if (held_by->GetName().empty()) {
                        ImGui::Text("Held by %d", held_by->GetID());
                    } else {
                        ImGui::Text("Held by %d (%s)", held_by->GetID(), held_by->GetName().c_str());
                    }
                }
            } else {
                ImGui::Text("Not held by anyone");
            }
            if (ImGui::TreeNode("Hotspots connected to this")) {
                show_connections = true;
            }
            break;
        }
        case _path_point_object: {
            PathPointObject* path_obj = (PathPointObject*)obj;
            if (ImGui::BeginListBox("Connected")) {
                for (auto& movement_object : scenegraph->movement_objects_) {
                    MovementObject* mov_obj = (MovementObject*)movement_object;
                    if (mov_obj->connected_pathpoint_id == path_obj->GetID()) {
                        char buffer[512];
                        mov_obj->GetDisplayName(buffer, 512);
                        if (ImGui::Selectable(buffer, false)) {
                            mov_obj->Disconnect(*path_obj);
                        }
                    }
                }
                for (size_t i = 0; i < path_obj->connection_ids.size(); i++) {
                    Object* obj = scenegraph->GetObjectFromID(path_obj->connection_ids[i]);
                    char buffer[512];
                    obj->GetDisplayName(buffer, 512);
                    if (ImGui::Selectable(buffer, false)) {
                        path_obj->Disconnect(*obj);
                    }
                }
                ImGui::EndListBox();
            }
            if (ImGui::BeginListBox("Available characters")) {
                for (auto& movement_object : scenegraph->movement_objects_) {
                    MovementObject* obj = (MovementObject*)movement_object;
                    if (obj->connected_pathpoint_id != path_obj->GetID()) {
                        char buffer[512];
                        obj->GetDisplayName(buffer, 512);
                        if (ImGui::Selectable(buffer, false)) {
                            obj->ConnectTo(*path_obj);
                        }
                    }
                }
                ImGui::EndListBox();
            }
            if (ImGui::TreeNode("Hotspots connected to this")) {
                show_connections = true;
            }
            break;
        }
        case _navmesh_connection_object: {
            static bool only_named = false;
            ImGui::Text("Connected objects");
            ImGui::Checkbox("Show only named items in available list", &only_named);
            static bool show_connected = true;
            ImGui::Checkbox("Show items with connections in available list", &show_connected);
            NavmeshConnectionObject* nav_con = (NavmeshConnectionObject*)obj;
            if (ImGui::BeginListBox("Connected")) {
                for (size_t i = 0; i < nav_con->connections.size(); i++) {
                    Object* obj = scenegraph->GetObjectFromID(nav_con->connections[i].other_object_id);
                    char buffer[512];
                    obj->GetDisplayName(buffer, 512);
                    if (ImGui::Selectable(buffer, false)) {
                        nav_con->Disconnect(*obj);
                    }
                }
                ImGui::EndListBox();
            }
            if (ImGui::BeginListBox("Available")) {
                for (auto& navmesh_connection : scenegraph->navmesh_connections_) {
                    NavmeshConnectionObject* obj = (NavmeshConnectionObject*)navmesh_connection;
                    if (obj->GetID() == nav_con->GetID() || (!show_connected && !obj->connections.empty())) {
                        continue;
                    }
                    bool connected = false;
                    for (auto& connection : nav_con->connections) {
                        if (obj->GetID() == connection.other_object_id) {
                            connected = true;
                            break;
                        }
                    }
                    if (!connected && !(only_named && obj->GetName().empty())) {
                        char buffer[512];
                        obj->GetDisplayName(buffer, 512);
                        if (ImGui::Selectable(buffer, false)) {
                            nav_con->ConnectTo(*obj);
                        }
                    }
                }
                ImGui::EndListBox();
            }
            if (ImGui::TreeNode("Hotspots connected to this")) {
                show_connections = true;
            }
            break;
        }
        case _placeholder_object: {
            PlaceholderObject* place_obj = (PlaceholderObject*)obj;
            int item_index = 0;
            std::vector<char> text;
            text.reserve(1024);
            text.resize(strlen("Nothing") + 1, '\0');
            strcpy(text.data(), "Nothing");
            for (size_t i = 0; i < scenegraph->movement_objects_.size(); ++i) {
                MovementObject* mov_obj = (MovementObject*)scenegraph->movement_objects_[i];
                GetDisplayName(mov_obj, text);
                if (mov_obj->GetID() == place_obj->GetConnectID()) {
                    item_index = (int)i + 1;
                }
            }
            text.push_back('\0');
            if (ImGui::Combo("##setcon", &item_index, text.data())) {
                if (item_index == 0 && place_obj->GetConnectID() != -1) {
                    place_obj->Disconnect(*scenegraph->GetObjectFromID(place_obj->GetConnectID()));
                } else {
                    size_t offset = 0;
                    for (int i = 0; i < item_index; ++i) {
                        offset += strlen(text.data() + offset) + 1;
                    }
                    int id = atoi(text.data() + offset);
                    place_obj->ConnectTo(*scenegraph->GetObjectFromID(id));
                }
            }
            if (ImGui::TreeNode("Hotspots connected to this")) {
                show_connections = true;
            }
            break;
        }
        case _hotspot_object: {
            if (ImGui::TreeNode("Connected objects")) {
                show_connections = true;
            }
            break;
        }
        default:
            if (ImGui::TreeNode("Hotspots connected to this")) {
                show_connections = true;
            }
            break;
    }

    if (show_connections) {
        if (!obj->connected_to.empty()) {
            if (ImGui::BeginListBox("Hotspots")) {
                for (size_t i = 0; i < obj->connected_to.size(); ++i) {
                    Object* ptr = scenegraph->GetObjectFromID(obj->connected_to[i]);
                    char buffer[512];
                    ptr->GetDisplayName(buffer, 512);
                    if (ImGui::Selectable(buffer)) {
                        obj->Disconnect(*ptr);
                    }
                }
                ImGui::EndListBox();
            }
        }
        ImGui::TreePop();
    }

    ImGui::Separator();
    DrawColorPicker(&obj, 1, obj->scenegraph_);
    if (obj->GetType() == _env_object) {
        EnvObject* eo = (EnvObject*)obj;
        ImGui::Separator();
        ImGui::Checkbox("no_navmesh", &eo->no_navmesh);
    }

    ImGui::Separator();
    if (obj->GetType() == _movement_object) {
        MovementObject* mov_obj = (MovementObject*)obj;
        ImGui::Text("NPC control script: %s", mov_obj->GetCurrentControlScript().c_str());
        ImGui::SameLine();
        if (ImGui::Button("Preview script")) {
            show_script = preview_script_name == mov_obj->GetCurrentControlScript() ? !show_script : true;
            preview_script_name = mov_obj->GetCurrentControlScript();
        }

        if (ImGui::Button("Set NPC script...")) {
            const int BUF_SIZE = 512;
            char buffer[BUF_SIZE];
            Dialog::DialogErr err = Dialog::readFile("as", 1, "Data/Scripts", buffer, BUF_SIZE);
            if (err != Dialog::NO_ERR) {
                LOGE << Dialog::DialogErrString(err) << std::endl;
            } else {
                std::string script_name = SplitPathFileName(buffer).second;
                mov_obj->object_npc_script_path = script_name;
                mov_obj->ChangeControlScript(script_name);
            }
        }
        if (!mov_obj->object_npc_script_path.empty()) {
            ImGui::SameLine();
            if (ImGui::Button("Remove control script")) {
                mov_obj->object_npc_script_path.clear();
                mov_obj->ChangeControlScript(mov_obj->scenegraph_->level->GetNPCScript(mov_obj));
            }
        }
        if (mov_obj->is_player) {
            ImGui::Text("PC control script: %s", mov_obj->scenegraph_->level->GetPCScript(mov_obj).c_str());
            ImGui::SameLine();
            if (ImGui::Button("Preview script##1")) {
                show_script = preview_script_name == mov_obj->scenegraph_->level->GetPCScript(mov_obj) ? !show_script : true;
                preview_script_name = mov_obj->scenegraph_->level->GetPCScript(mov_obj);
            }

            if (ImGui::Button("Set PC script...")) {
                const int BUF_SIZE = 512;
                char buffer[BUF_SIZE];
                Dialog::DialogErr err = Dialog::readFile("as", 1, "Data/Scripts", buffer, BUF_SIZE);
                if (err != Dialog::NO_ERR) {
                    LOGE << Dialog::DialogErrString(err) << std::endl;
                } else {
                    std::string script_name = SplitPathFileName(buffer).second;
                    mov_obj->object_pc_script_path = script_name;
                }
            }
            if (!mov_obj->object_pc_script_path.empty()) {
                ImGui::SameLine();
                if (ImGui::Button("Remove control script")) {
                    mov_obj->object_pc_script_path.clear();
                }
            }
        }
    }
    ImGui::Text("Script Params");

    const int kSecondLaunchCustomGuiInstanceId = 1;
    DrawLaunchCustomGuiButton(obj, are_script_params_read_only, kSecondLaunchCustomGuiInstanceId);

    ImGui::BeginDisabled(are_script_params_read_only);
    if (DrawScriptParamsEditor(obj->GetScriptParams())) {
        obj->scenegraph_->map_editor->QueueSaveHistoryState();
        obj->UpdateScriptParams();
    }
    ImGui::EndDisabled();

    ImGui::Separator();
    ImGui::Text("Debug");
    ImGui::Text("Selectable: %s", obj->selectable_ ? "true" : "false");
    if (obj->parent) {
        ImGui::Text("Parent: %d", obj->parent->GetID());
    } else {
        ImGui::Text("No Parent");
    }
}

static void DrawObjectInfo(Object* obj, bool force_expand_script_params) {
    ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
    obj->DrawImGuiEditor();
    ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());

    bool are_script_params_read_only = false;
    DrawLaunchCustomGuiButton(obj, are_script_params_read_only);

    bool temp_enabled = obj->enabled_;
    if (ImGui::Checkbox("Enabled", &temp_enabled)) {
        obj->SetEnabled(temp_enabled);
    }

    if (ImGui::TreeNode("Name")) {
        const size_t name_len = 64;
        char name[name_len];
        strscpy(name, obj->GetName().c_str(), name_len);
        if (ImGui::InputText("Name", name, name_len)) {
            obj->SetName(std::string(name));
            obj->scenegraph_->map_editor->QueueSaveHistoryState();
        }

        if (obj->GetType() == _prefab) {
            Prefab* prefab = static_cast<Prefab*>(obj);

            bool prefab_locked;
            prefab_locked = prefab->prefab_locked;

            if (ImGui::Checkbox("Prefab Locked", &prefab_locked)) {
                prefab->prefab_locked = prefab_locked;
                prefab->scenegraph_->map_editor->QueueSaveHistoryState();
            }

            ImGui::Text("Prefab Source: %s", prefab->prefab_path.GetOriginalPath());
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Transform")) {
        bool changed = false;

        vec3 translation = obj->GetTranslation();
        if (ImGui::DragFloat3("Translation", &translation[0], 0.01f) || ImGui::IsItemActive()) {
            obj->SetTranslation(translation);
            changed = true;
        }

        vec3 scale = obj->GetScale();
        if (ImGui::DragFloat3("Scale", &scale[0], 0.01f) || ImGui::IsItemActive()) {
            obj->SetScale(scale);
            changed = true;
        }

        vec3 euler_angles = obj->GetRotationEuler();
        euler_angles *= 180.0f / (float)M_PI;

        if (ImGui::DragFloat3("Rotation", &euler_angles[0], 1.0f, 0.0f, 0.0f, "%.0f") || ImGui::IsItemActive()) {
            obj->SetRotationEuler(euler_angles * (float)M_PI / 180.0f);
            changed = true;
        }

        if (ImGui::Button("Reset Rotation")) {
            obj->SetRotation(quaternion());
            changed = true;
        }

        if (changed) {
            obj->scenegraph_->UpdatePhysicsTransforms();
            obj->scenegraph_->map_editor->QueueSaveHistoryState();
        }

        ImGui::TreePop();
    }  // Transform

    if (obj->GetType() == _env_object) {
        EnvObject* eo = (EnvObject*)obj;
        vec3 color = eo->GetColorTint();
        if (ImGui::TreeNode("Tint")) {
            if (ColorPicker(&color[0], NULL, false, false)) {
                eo->ReceiveObjectMessage(OBJECT_MSG::SET_COLOR, &color);
                obj->scenegraph_->map_editor->QueueSaveHistoryState();
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Flags")) {
            ImGui::Checkbox("no_navmesh", &eo->no_navmesh);
            ImGui::TreePop();
        }
    }

    if (obj->GetType() == _movement_object) {
        MovementObject* mo = (MovementObject*)obj;
        if (ImGui::TreeNode("Color Palette")) {
            DrawColorPicker(&obj, 1, obj->scenegraph_);
            ImGui::TreePop();
        }
    }

    if (force_expand_script_params) {
        ImGui::SetNextItemOpen(true);
    }

    if (ImGui::TreeNode("Script Params")) {
        DrawLaunchCustomGuiButton(obj, are_script_params_read_only);

        ImGui::BeginDisabled(are_script_params_read_only);
        if (DrawScriptParamsEditor(obj->GetScriptParams())) {
            obj->scenegraph_->map_editor->QueueSaveHistoryState();
            obj->UpdateScriptParams();
        }
        ImGui::EndDisabled();

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Debug")) {
        ImGui::Text("Selectable: %s", obj->selectable_ ? "true" : "false");
        if (obj->parent) {
            ImGui::Text("Parent: %d", obj->parent->GetID());
        } else {
            ImGui::Text("No Parent");
        }

        ImGui::TreePop();
    }
}

static bool IsCharacterSelceted(SpawnerItem* item) {
    bool return_value = false;
    if (ImGui::MenuItem(item->display_name.c_str())) {
        return_value = true;
    }

    if (ImGui::IsItemHovered()) {
        std::string thumbnail_full_path = item->thumbnail_path;

        if (thumbnail_full_path != Engine::Instance()->current_spawner_thumbnail) {
            if (FileExists(thumbnail_full_path, kDataPaths | kModPaths)) {
                Engine::Instance()->spawner_thumbnail = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(thumbnail_full_path, PX_NOMIPMAP | PX_NOREDUCE | PX_NOCONVERT, 0x0);
            } else {
                Engine::Instance()->spawner_thumbnail.clear();
            }
            Engine::Instance()->current_spawner_thumbnail = thumbnail_full_path;
        }

        if (Engine::Instance()->spawner_thumbnail.valid()) {
            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.5, 0.5, 1.0, 0.5));
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(450.0f);

            Textures::Instance()->EnsureInVRAM(Engine::Instance()->spawner_thumbnail);
            ImGui::Image((ImTextureID)Textures::Instance()->returnTexture(Engine::Instance()->spawner_thumbnail), ImVec2(128, 128), ImVec2(0, 0), ImVec2(1, 1));

            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
            ImGui::PopStyleColor();
        }
    }
    return return_value;
}

static bool AddSpawnerItem(SpawnerItem* item, SceneGraph* scenegraph) {
    bool return_value = false;
    if (ImGui::MenuItem(item->display_name.c_str())) {
        if (scenegraph->map_editor->state_ == MapEditor::kIdle && scenegraph->map_editor->LoadEntitiesFromFile(item->path) == 0) {
            scenegraph->level->PushSpawnerItemRecent(*item);
            scenegraph->map_editor->active_tool_ = EditorTypes::ADD_ONCE;
            return_value = true;
        }
    }
    if (ImGui::IsItemHovered()) {
        std::string thumbnail_full_path = item->thumbnail_path;

        if (thumbnail_full_path != Engine::Instance()->current_spawner_thumbnail) {
            if (FileExists(thumbnail_full_path, kDataPaths | kModPaths)) {
                Engine::Instance()->spawner_thumbnail = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(thumbnail_full_path, PX_NOMIPMAP | PX_NOREDUCE | PX_NOCONVERT, 0x0);
            } else {
                Engine::Instance()->spawner_thumbnail.clear();
            }
            Engine::Instance()->current_spawner_thumbnail = thumbnail_full_path;
        }

        if (Engine::Instance()->spawner_thumbnail.valid()) {
            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.5, 0.5, 1.0, 0.5));
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(450.0f);

            Textures::Instance()->EnsureInVRAM(Engine::Instance()->spawner_thumbnail);
            ImGui::Image((ImTextureID)Textures::Instance()->returnTexture(Engine::Instance()->spawner_thumbnail), ImVec2(128, 128), ImVec2(0, 0), ImVec2(1, 1));

            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
            ImGui::PopStyleColor();
        }
    }
    return return_value;
}

static void LevelMenuItem(ModInstance::Campaign& campaign, ModInstance::Level& level) {
    const char* level_name = level.title;
    const char* level_path = level.path;
    const char* level_id = level.id;

    if (ImGui::MenuItem(level_name, NULL, false, level_name[0] != '*')) {
        Engine::Instance()->ScriptableUICallback("set_campaign " + campaign.id.str());
        Engine::Instance()->ScriptableUICallback("load_campaign_level " + std::string(level_id));
    }
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(450.0f);
        ImGui::Text("Path: %s", level_path);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

static void OpenLevelMenu() {
    if (ImGui::BeginMenu("Open")) {
        ModLoading& mod_loading = ModLoading::Instance();

        std::vector<ModInstance::Campaign> campaigns = mod_loading.GetCampaigns();

        for (ModInstance::Campaign& campaign : campaigns) {
            if (ImGui::BeginMenu(campaign.id.c_str())) {
                for (ModInstance::Level& level : campaign.levels) {
                    LevelMenuItem(campaign, level);
                }

                ImGui::EndMenu();
            }
        }

        ImGui::EndMenu();
    }
}

static void OpenRecentMenu() {
    if (ImGui::BeginMenu("Open Recent")) {
        for (int i = 0; i < kMaxLevelHistory; ++i) {
            const int kBufSize = 128;
            char buf[kBufSize];
            FormatString(buf, kBufSize, "level_history%d", i + 1);
            if (config.HasKey(buf) && config[buf].str() != "") {
                if (ImGui::MenuItem(config[buf].str().c_str())) {
                    if (FileExists(config[buf].str(), kAnyPath)) {
                        LevelInfoAssetRef levelinfo = Engine::Instance()->GetAssetManager()->LoadSync<LevelInfoAsset>(config[buf].str());
                        Path p = FindFilePath(config[buf].str(), kAnyPath);
                        Engine::Instance()->QueueState(EngineState(levelinfo->GetName(), kEngineEditorLevelState, p));
                    } else {
                        LOGE << "Level missing: " << config[buf].str() << std::endl;
                    }
                }
            }
        }
        ImGui::EndMenu();
    }
}

static bool TreeScenegraphElementVisible(Object* obj, Object* root, char* buf) {
    const int kBufSize = 512;
    if ((show_named_only_scenegraph == false || obj->GetName().empty() == false) && obj->selectable_) {
        if (obj->parent == root) {
            obj->GetDisplayName(buf, kBufSize);
            if (scenegraph_filter.IsActive() && !scenegraph_filter.PassFilter(buf)) {
                if (search_children_scenegraph && obj->IsGroupDerived()) {
                    char child_buf[kBufSize];
                    Group* group = static_cast<Group*>(obj);
                    for (auto& i : group->children) {
                        i.direct_ptr->GetDisplayName(child_buf, kBufSize);
                        if (scenegraph_filter.PassFilter(child_buf)) {
                            return true;
                        }
                    }
                }

                return false;
            }

            return true;
        }
    }

    return false;
}
static void DrawTreeScenegraphFor(SceneGraph* scenegraph, GUI* gui, Object* root);

static void DrawTreeScenegraphElement(SceneGraph* scenegraph, GUI* gui, Object* parent, Object* obj, int index, bool force) {
    static bool select_elements;

    const int kBufSize = 512;
    char buf[kBufSize];
    if (!force) {
        if (!TreeScenegraphElementVisible(obj, parent, buf))
            return;
    } else {
        obj->GetDisplayName(buf, kBufSize);
    }

    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (obj->Selected()) {
        node_flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (obj->GetType() != _group && obj->GetType() != _prefab) {
        node_flags |= ImGuiTreeNodeFlags_Leaf;
    }
    ImGui::PushID(obj->GetID());
    vec4 color = GetObjColor(obj);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(color[0], color[1], color[2], color[3]));
    bool node_open = ImGui::TreeNodeEx("", node_flags, "%s", buf);
    ImGui::PopStyleColor();
    ImGui::PopID();
    if (ImGui::IsItemClicked()) {
        obj->Select(!obj->Selected());

        const Keyboard& keyboard = Input::Instance()->getKeyboard();

        if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_LShift)) || ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_RShift))) {
            if (select_start_index == -1) {
                select_start_index = index;
                select_elements = obj->Selected();
            } else {
                char temp_buf[kBufSize];

                int low_index = std::min(select_start_index, index);
                int high_index = std::max(select_start_index, index);

                for (int j = 0, len = (int)scenegraph->objects_.size(); j < len; ++j) {
                    Object* temp_obj = scenegraph->objects_[j];
                    if (TreeScenegraphElementVisible(temp_obj, parent, temp_buf)) {
                        if (j >= low_index && j <= high_index) {
                            if (temp_obj->Selected() != select_elements)
                                temp_obj->Select(select_elements);
                        }
                    }
                }
            }
        } else {
            select_start_index = index;
            select_elements = obj->Selected();
        }
    }
    if (node_open) {
        if ((node_flags & ImGuiTreeNodeFlags_Leaf) == 0) {
            DrawTreeScenegraphFor(scenegraph, gui, obj);
        }
        ImGui::TreePop();
    }
}

static void DrawTreeScenegraphFor(SceneGraph* scenegraph, GUI* gui, Object* root) {
    if (root && root->IsGroupDerived()) {
        Group* group = static_cast<Group*>(root);
        for (int i = 0, len = (int)group->children.size(); i < len; ++i) {
            DrawTreeScenegraphElement(scenegraph, gui, root, group->children[i].direct_ptr, i, true);
        }
    } else {
        for (int i = 0, len = (int)scenegraph->objects_.size(); i < len; ++i) {
            DrawTreeScenegraphElement(scenegraph, gui, root, scenegraph->objects_[i], i, false);
        }
    }
}

static void FindFaces(const Model& model, const WOLFIRE_SIMPLIFY::SimplifyModel& processed_model, const std::vector<HalfEdge>& half_edges, std::vector<int>& half_edge_face, std::vector<int>& face_vert_equivalent) {
    std::vector<VertSortable> face_vert_sortable;
    face_vert_sortable.resize(model.vertices.size() / 3);
    for (int vert_index = 0, vert_id = 0, len = (int)model.vertices.size(); vert_index < len; ++vert_id, vert_index += 3) {
        face_vert_sortable[vert_id].id = vert_id;
        memcpy(&face_vert_sortable[vert_id].vert, &model.vertices[vert_index], sizeof(vec3));
    }
    std::vector<VertSortable> processed_vert_sortable;
    processed_vert_sortable.resize(processed_model.vertices.size() / 3);
    for (int vert_index = 0, vert_id = 0, len = (int)processed_model.vertices.size(); vert_index < len; ++vert_id, vert_index += 3) {
        processed_vert_sortable[vert_id].id = vert_id;
        memcpy(&processed_vert_sortable[vert_id].vert, &processed_model.vertices[vert_index], sizeof(vec3));
    }
    std::sort(face_vert_sortable.begin(), face_vert_sortable.end(), SortVertSortable);
    std::sort(processed_vert_sortable.begin(), processed_vert_sortable.end(), SortVertSortable);
    face_vert_equivalent.resize(face_vert_sortable.size(), -1);
    unsigned processed_vert_index = 0;
    for (size_t i = 0, len = face_vert_equivalent.size(); i < len;) {
        if (face_vert_sortable[i].vert == processed_vert_sortable[processed_vert_index].vert) {
            face_vert_equivalent[face_vert_sortable[i].id] = processed_vert_sortable[processed_vert_index].id;
            ++i;
        } else {
            ++processed_vert_index;
        }
        if (processed_vert_index >= processed_vert_sortable.size()) {
            break;
        }
    }

    std::vector<EdgeSortable> face_edge_sortable;
    face_edge_sortable.resize(model.faces.size());
    for (int face_index = 0, face_id = 0, len = (int)model.faces.size(); face_index < len; ++face_id, face_index += 3) {
        for (int i = 0; i < 3; ++i) {
            face_edge_sortable[face_index + i].id = face_id;
            face_edge_sortable[face_index + i].verts[0] = face_vert_equivalent[model.faces[face_index + i]];
            face_edge_sortable[face_index + i].verts[1] = face_vert_equivalent[model.faces[face_index + ((i + 1) % 3)]];
        }
    }
    std::vector<EdgeSortable> half_edge_sortable;
    half_edge_sortable.resize(half_edges.size());
    for (int half_edge_id = 0, len = (int)half_edges.size(); half_edge_id < len; ++half_edge_id) {
        half_edge_sortable[half_edge_id].id = half_edge_id;
        for (int i = 0; i < 2; ++i) {
            half_edge_sortable[half_edge_id].verts[i] = half_edges[half_edge_id].vert[i];
        }
    }
    std::sort(face_edge_sortable.begin(), face_edge_sortable.end(), SortEdgeSortable);
    std::sort(half_edge_sortable.begin(), half_edge_sortable.end(), SortEdgeSortable);
    half_edge_face.resize(half_edges.size(), -1);
    unsigned face_index = 0;
    for (size_t i = 0, len = half_edges.size(); i < len;) {
        if (half_edge_sortable[i].verts[0] == face_edge_sortable[face_index].verts[0]) {
            if (half_edge_sortable[i].verts[1] == face_edge_sortable[face_index].verts[1]) {
                half_edge_face[half_edge_sortable[i].id] = face_edge_sortable[face_index].id;
                ++i;
            } else {
                ++face_index;
            }
        } else {
            ++face_index;
        }
        if (face_index >= face_edge_sortable.size()) {
            break;
        }
    }
}

static int LogWindowCallback(ImGuiInputTextCallbackData* data) {
    uint32_t* selection_out = static_cast<uint32_t*>(data->UserData);

    selection_out[0] = data->SelectionStart;
    selection_out[1] = data->SelectionEnd;
    return 0;
}

static void ParseSpawner() {
    spawner_tabs.clear();

    const std::vector<ModInstance*>& mods = ModLoading::Instance().GetMods();

    for (auto mod : mods) {
        if (mod->IsActive()) {
            for (unsigned u = 0; u < mod->items.size(); u++) {
                const ModInstance::Item& item = mod->items[u];

                SpawnerTab* spawner_tab_ptr = &spawner_tabs[std::string(item.category)];
                spawner_tab_ptr->resize(spawner_tab_ptr->size() + 1);

                SpawnerItem* new_item = &spawner_tab_ptr->back();

                new_item->mod_source_title = mod->name;
                new_item->display_name = item.title;
                new_item->path = item.path;
                new_item->thumbnail_path = item.thumbnail;

                spawner_tab_filters.insert(make_pair(std::string(item.category), ImGuiTextFilter()));
            }
        }
    }
}

void InitImGui() {
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui_ImplSdlGL3_Init(Graphics::Instance()->sdl_window_);
    FormatString(imgui_ini_path, kPathSize, "%simgui.ini", GetWritePath(CoreGameModID).c_str());

    ImGui::GetIO().IniFilename = imgui_ini_path;

    run_parse_spawner_flag = true;
    InitializeModMenu();
    imgui_scale = 1.0f + config.GetRef("imgui_scale").toNumber<float>() / 4.0f;
}

void UpdateImGui() {
    if (run_parse_spawner_flag) {
        ParseSpawner();
        run_parse_spawner_flag = false;
    }
}

void ReloadImGui() {
    run_parse_spawner_flag = true;
}

void DisposeImGui() {
    ImGui_ImplSdlGL3_Shutdown();
}

void ProcessEventImGui(SDL_Event* event) {
    ImGui_ImplSdlGL3_ProcessEvent(event);
}

void DrawImGuiCameraPreview(Engine* engine, SceneGraph* scenegraph_, Graphics* graphics) {
    Object* cam_object = scenegraph_->map_editor->GetSelectedCameraObject();
    if (cam_object && (!graphics->queued_screenshot || cam_object->Selected()) && ActiveCameras::Instance()->Get()->GetFlags() != Camera::kPreviewCamera) {
        ImGui::SetNextWindowSizeConstraints(ImVec2(320.0f, 240.0f), ImVec2(FLT_MAX, FLT_MAX));
        if (ImGui::Begin("Camera Preview", NULL, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoFocusOnAppearing)) {
            PushGPUProfileRange("Draw in-game camera");
            bool media_mode = graphics->media_mode();
            graphics->SetMediaMode(true);

            ((PlaceholderObject*)cam_object)->SetVisible(false);
            cam_object->editor_visible = false;
            float top_left[2], bottom_right[2];
            {
                ImVec2 pos = ImGui::GetWindowPos();
                ImVec2 window_content_min = ImGui::GetWindowContentRegionMin();
                ImVec2 window_content_max = ImGui::GetWindowContentRegionMax();
                top_left[0] = pos.x + window_content_min.x;
                top_left[1] = pos.y + window_content_min.y;
                bottom_right[0] = pos.x + window_content_max.x;
                bottom_right[1] = pos.y + window_content_max.y;
            }
            if (bottom_right[0] > top_left[0] && bottom_right[1] > top_left[1]) {
                {
                    ImVec2 size(bottom_right[0] - top_left[0], bottom_right[1] - top_left[1]);
                    ImVec2 uv0(top_left[0] / graphics->render_dims[0], 1.0f - top_left[1] / graphics->render_dims[1]);
                    ImVec2 uv1(bottom_right[0] / graphics->render_dims[0], 1.0f - bottom_right[1] / graphics->render_dims[1]);
                    ImGui::Image((ImTextureID)Textures::Instance()->returnTexture(Graphics::Instance()->post_effects.temp_screen_tex), size, uv0, uv1);
                }

                top_left[0] /= graphics->window_dims[0];
                top_left[1] /= graphics->window_dims[1];
                top_left[1] = 1.0f - top_left[1];
                bottom_right[0] /= graphics->window_dims[0];
                bottom_right[1] /= graphics->window_dims[1];
                bottom_right[1] = 1.0f - bottom_right[1];
                std::swap(top_left[1], bottom_right[1]);

                if (graphics->queued_screenshot && cam_object->Selected()) {
                    top_left[0] = 0.0f;
                    top_left[1] = 0.0f;
                    bottom_right[0] = 1.0f;
                    bottom_right[1] = 1.0f;
                }

                engine->active_screen_start[0] = clamp(top_left[0], 0.0f, 1.0f);
                engine->active_screen_start[1] = clamp(top_left[1], 0.0f, 1.0f);
                engine->active_screen_end[0] = clamp(bottom_right[0], 0.0f, 1.0f);
                engine->active_screen_end[1] = clamp(bottom_right[1], 0.0f, 1.0f);

                graphics->startDraw(vec2(top_left[0], top_left[1]),
                                    vec2(bottom_right[0], bottom_right[1]),
                                    Graphics::kWindow);
                ActiveCameras::Set(2);
                ActiveCameras::Get()->SetFlags(Camera::kPreviewCamera);

                // Apply camera object settings to camera
                Camera* camera = ActiveCameras::Instance()->Get();

                // static Camera prev_camera = *camera;
                // LOGI << "First camera diff" << std::endl;
                // camera->PrintDifferences(prev_camera);
                // prev_camera = *camera;

                // Set camera position
                camera->SetPos(cam_object->GetTranslation());
                // Set camera euler angles from rotation matrix
                const mat4& rot = Mat4FromQuaternion(cam_object->GetRotation());
                vec3 front = rot * vec3(0, 0, 1);
                camera->SetYRotation(atan2f(front[0], front[2]) * 180.0f / PI_f);
                camera->SetXRotation(asinf(front[1]) * -180.0f / PI_f);
                vec3 up = rot * vec3(0, 1, 0);
                vec3 expected_right = normalize(cross(front, vec3(0, 1, 0)));
                vec3 expected_up = normalize(cross(expected_right, front));
                camera->SetZRotation(atan2f(dot(up, expected_right), dot(up, expected_up)) * 180.0f / PI_f);
                // Set camera zoom from scale
                const float zoom_sensitivity = 3.5f;
                camera->SetFOV(min(150.0f, 90.0f / max(0.001f, (1.0f + (cam_object->GetScale()[0] - 1.0f) * zoom_sensitivity))));
                camera->SetDistance(0.0f);

                scenegraph_->level->Message("request_preview_dof");

                // Draw view
                engine->DrawScene(Engine::kViewport, Engine::kFinal, SceneGraph::kStaticAndDynamic);

                if (!graphics->queued_screenshot) {
                    camera->DrawSafeZone();
                }

                graphics->bindFramebuffer(graphics->post_effects.post_framebuffer);
                graphics->framebufferColorTexture2D(Graphics::Instance()->post_effects.temp_screen_tex);
                graphics->bindFramebuffer(0);
                CHECK_FBO_ERROR();
                CHECK_GL_ERROR();
                glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, graphics->post_effects.post_framebuffer);
                glReadBuffer(GL_BACK);
                glDrawBuffer(GL_COLOR_ATTACHMENT0);
                glBlitFramebuffer(0, 0, graphics->window_dims[0], graphics->window_dims[1], 0, 0, graphics->render_dims[0], graphics->render_dims[1], GL_COLOR_BUFFER_BIT, GL_NEAREST);
                glBindFramebuffer(GL_FRAMEBUFFER, graphics->curr_framebuffer);
                CHECK_FBO_ERROR();
                CHECK_GL_ERROR();

                ((PlaceholderObject*)cam_object)->SetVisible(true);
                cam_object->editor_visible = true;
                graphics->SetMediaMode(media_mode);
                PopGPUProfileRange();
            }
        }
        ImGui::End();
    }
}

bool DrawBrowsePath(const char* label, const char* current_path, const char* extensions, int extension_count, char* out_buffer, int out_buffer_size, const char* start_dir = NULL) {
    bool change = false;
    ImGui::PushID(label);
    char path_buffer[kPathSize];
    if (ImGui::Button("Browse...")) {
        Dialog::DialogErr err = Dialog::readFile(extensions, extension_count, start_dir ? start_dir : "Data", path_buffer, kPathSize);
        if (err != Dialog::NO_ERR) {
            LOGE << Dialog::DialogErrString(err) << std::endl;
        } else {
            change = true;
        }
    } else {
        int copy = strscpy(path_buffer, current_path, kPathSize);
        if (copy != 0 && copy != SOURCE_IS_NULL) {
            LOGE << "Path is too long for buffer. Contact developers and attach log file\n"
                 << GenerateStacktrace() << std::endl;
            LOGE << "Path is " << current_path << std::endl;
        }
        assert(copy == 0 || copy == SOURCE_IS_NULL);
    }
    ImGui::SameLine();
    if (ImGui::InputText(label, path_buffer, kPathSize, ImGuiInputTextFlags_EnterReturnsTrue)) {
        change = true;
    }
    if (change) {
        const char* data = strstr(path_buffer, "Data/");
        if (!data) {
            DisplayError("Error", "Path must include 'Data' folder");
            change = false;
        } else {
            if (!FindFilePath(data, kDataPaths | kModPaths, false).isValid()) {
                DisplayError("Error", "Path must be inside Overgrowth data directory");
                change = false;
            } else {
                int copy = strscpy(out_buffer, data, out_buffer_size);
                if (copy != 0 && copy != SOURCE_IS_NULL) {
                    LOGE << "Path is too long for buffer. Contact developers and attach log file\n"
                         << GenerateStacktrace() << std::endl;
                    LOGE << "Path is " << current_path << std::endl;
                }
                assert(copy == 0 || copy == SOURCE_IS_NULL);
            }
        }
    }
    ImGui::PopID();
    return change;
}

static void ResizeCallback(ImGuiSizeCallbackData* data) {
    static float last_x = 0.0f;
    static float last_y = 0.0f;

    float max_height = std::min(data->CurrentSize.y, Graphics::Instance()->window_dims[1] - 29.0f);

    float increase_x = (data->CurrentSize.x - last_x) * 0.1f;
    float increase_y = (max_height - last_y) * 0.1f;

    if (increase_x < 1.0f && increase_x > -1.0f) {
        increase_x = data->CurrentSize.x - last_x;
    }
    if (increase_y < 1.0f && increase_y > -1.0f) {
        increase_y = max_height - last_y;
    }

    data->DesiredSize = ImVec2(last_x + increase_x, last_y + increase_y);

    last_x = data->DesiredSize.x;
    last_y = data->DesiredSize.y;
}

void DrawImGui(Graphics* graphics, SceneGraph* scenegraph, GUI* gui, AssetManager* assetmanager, Engine* engine, bool cursor_visible) {
    Online* online = Online::Instance();

    ImGui::GetIO().FontGlobalScale = imgui_scale;
    ImGui_ImplSdlGL3_NewFrame(graphics->sdl_window_, Input::Instance()->GetGrabMouse());
    ImGui::GetIO().MouseDrawCursor = cursor_visible;

    if (engine->check_save_level_changes_dialog_is_showing) {
        bool dialog_finished = false;
        bool execute_save = false;
        bool continue_action = false;

        if (false == scenegraph->map_editor->WasLastSaveOnCurrentUndoChunk() || scenegraph->level->isMetaDataDirty()) {
            ImGui::OpenPopup("Check Save Changes");

            ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

            if (ImGui::BeginPopupModal("Check Save Changes", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Save changes to the level before closing it?");
                ImGui::Separator();

                if (ImGui::Button("Yes", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                    continue_action = true;
                    execute_save = true;
                    dialog_finished = true;
                }
                ImGui::SetItemDefaultFocus();

                ImGui::SameLine();
                if (ImGui::Button("No", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                    continue_action = true;
                    execute_save = false;
                    dialog_finished = true;
                }

                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                    dialog_finished = true;
                }

                if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape))) {
                    ImGui::CloseCurrentPopup();
                    dialog_finished = true;
                }

                ImGui::EndPopup();
            } else {
                dialog_finished = true;
            }

        } else {
            // Continue on immediately without showing dialog
            continue_action = true;
            dialog_finished = true;
        }

        if (dialog_finished) {
            if (execute_save) {
                scenegraph->map_editor->ExecuteSaveLevelChanges();
            }

            engine->check_save_level_changes_dialog_is_finished = true;
            engine->check_save_level_changes_dialog_is_showing = false;
            engine->check_save_level_changes_last_result = continue_action;
        }

        return;
    }

    const bool kDisplayKeys = false;
    if (kDisplayKeys) {
        {
            const int kBufSize = 256;
            char buf[kBufSize] = {'\0'};
            FormatString(buf, kBufSize, "Keys(ImGui): ");
            char temp[kBufSize];
            for (int i = 0; i < 512; ++i) {
                if (ImGui::IsKeyDown(i)) {
                    int val = i;
                    if (val > SDLK_z) {
                        val = val + SDLK_CAPSLOCK - 1 - SDLK_z;
                    }
                    FormatString(temp, kBufSize, "%s%s ", buf, SDLKeycodeToString(val));
                    FormatString(buf, kBufSize, "%s", temp);
                }
            }
            gui->AddDebugText("keys_imgui", buf, 0.5f);
        }

        {
            const int kBufSize = 256;
            char buf[kBufSize] = {'\0'};
            char temp[kBufSize];
            FormatString(buf, kBufSize, "Keys(game): ");
            Keyboard::KeyStatusMap* keys = &Input::Instance()->getKeyboard().keys;
            for (auto& key : *keys) {
                if (key.second.down) {
                    FormatString(temp, kBufSize, "%s%s ", buf, SDLKeycodeToString(key.first));
                    FormatString(buf, kBufSize, "%s", temp);
                }
            }
            gui->AddDebugText("keys_game", buf, 0.5f);
        }
    }

    if (!scenegraph || scenegraph->map_editor->state_ != MapEditor::kInGame) {
        if (ImGui::BeginMainMenuBar()) {
            if (!scenegraph) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("New Level", KeyCommand::GetDisplayText(KeyCommand::kNewLevel))) {
                        Engine::NewLevel();
                    }
                    if (ImGui::MenuItem("Open Level...", KeyCommand::GetDisplayText(KeyCommand::kOpenLevel))) {
                        LoadLevel(false);
                    }
                    if (ImGui::MenuItem("Open Local Level...", "")) {
                        LoadLevel(true);
                    }
                    OpenLevelMenu();
                    OpenRecentMenu();
                    ImGui::Separator();
                    if (ImGui::MenuItem("Quit", KeyCommand::GetDisplayText(KeyCommand::kQuit))) {
                        Input::Instance()->RequestQuit();
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Settings")) {
                    DrawSettingsImGui(scenegraph, IMGST_MENU);
                    ImGui::EndMenu();
                }
            }
            if (scenegraph) {
                MapEditor* me = scenegraph->map_editor;

                // TODO: only enable items that are relevant to what we can do now
                /*
                if(gui->ribbon_gui_){
                    // Update ribbon items
                    uint32_t* ribbon_flags = &gui->ribbon_gui_->buttons_enabled[0];
                    SetBit(ribbon_flags, RibbonGUI::decal_editor_active, IsTypeEnabled(_decal_object));
                    SetBit(ribbon_flags, RibbonGUI::hotspots_editor_active, IsTypeEnabled(_hotspot_object));
                    SetBit(ribbon_flags, RibbonGUI::objects_editor_active, IsTypeEnabled(_env_object));
                    SetBit(ribbon_flags, RibbonGUI::lights_editor_active, IsTypeEnabled(_dynamic_light_object));
                    SetBit(ribbon_flags, RibbonGUI::something_selected, IsSomethingSelected());
                    SetBit(ribbon_flags, RibbonGUI::one_object_selected, IsOneObjectSelected());
                    SetBit(ribbon_flags, RibbonGUI::can_undo, CanUndo());
                    SetBit(ribbon_flags, RibbonGUI::can_redo, CanRedo());
                    SetBit(ribbon_flags, RibbonGUI::show_probes, scenegraph_->light_probe_collection.show_probes);
                    SetBit(ribbon_flags, RibbonGUI::show_probes_through_walls, scenegraph_->light_probe_collection.show_probes_through_walls);
                    SetBit(ribbon_flags, RibbonGUI::probe_lighting_enabled, scenegraph_->light_probe_collection.probe_lighting_enabled);
                    SetBit(ribbon_flags, RibbonGUI::show_tet_mesh, scenegraph_->light_probe_collection.tet_mesh_viz_enabled);

                    uint32_t* ribbon_toggle = &gui->ribbon_gui_->buttons_toggled[0];
                    SetBit(ribbon_toggle, RibbonGUI::view_nav_mesh, IsViewingNavMesh());
                    SetBit(ribbon_toggle, RibbonGUI::view_collision_nav_mesh, IsViewingCollisionNavMesh());
                    SetBit(ribbon_toggle, RibbonGUI::view_nav_mesh_hints, type_enable_.IsTypeEnabled( _navmesh_hint_object ));
                    SetBit(ribbon_toggle, RibbonGUI::view_nav_mesh_region, type_enable_.IsTypeEnabled( _navmesh_region_object ));
                    SetBit(ribbon_toggle, RibbonGUI::view_nav_mesh_jump_nodes, type_enable_.IsTypeEnabled( _navmesh_connection_object ));
                }
                */
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("New Level", KeyCommand::GetDisplayText(KeyCommand::kNewLevel))) {
                        Engine::NewLevel();
                    }
                    if (ImGui::MenuItem("Open Level...", KeyCommand::GetDisplayText(KeyCommand::kOpenLevel))) {
                        LoadLevel(false);
                    }
                    if (ImGui::MenuItem("Open Local Level...")) {
                        LoadLevel(true);
                    }
                    OpenLevelMenu();
                    OpenRecentMenu();
                    if (ImGui::MenuItem("Save", KeyCommand::GetDisplayText(KeyCommand::kSaveLevel), false, !me->GetTerrainPreviewMode())) {
                        me->SaveLevel(LevelLoader::kSaveInPlace);
                    }
                    if (ImGui::MenuItem("Save As..", KeyCommand::GetDisplayText(KeyCommand::kSaveLevelAs), false, !me->GetTerrainPreviewMode())) {
                        me->SaveLevel(LevelLoader::kSaveAs);
                    }

                    ImGui::Separator();
                    if (ImGui::MenuItem("Back to main menu")) {
                        if (scenegraph) {
                            Engine::Instance()->ScriptableUICallback("back_to_main_menu");
                        }
                    }
                    if (ImGui::MenuItem("Quit", KeyCommand::GetDisplayText(KeyCommand::kQuit))) {
                        Input::Instance()->RequestQuit();
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Edit")) {
                    if (ImGui::MenuItem("Undo", KeyCommand::GetDisplayText(KeyCommand::kUndo), false, !me->GetTerrainPreviewMode())) {
                        me->Undo();
                    }
                    if (ImGui::MenuItem("Redo", KeyCommand::GetDisplayText(KeyCommand::kRedo), false, !me->GetTerrainPreviewMode())) {
                        me->Redo();
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Cut", KeyCommand::GetDisplayText(KeyCommand::kCut))) {
                        me->CutSelected();
                    }
                    if (ImGui::MenuItem("Copy", KeyCommand::GetDisplayText(KeyCommand::kCopy))) {
                        me->CopySelected();
                    }
                    if (ImGui::MenuItem("Paste", KeyCommand::GetDisplayText(KeyCommand::kPaste))) {
                        me->RibbonItemClicked("paste", true);
                    }
                    ImGui::Separator();

                    if (ImGui::MenuItem("Select All", KeyCommand::GetDisplayText(KeyCommand::kSelectAll))) {
                        me->SelectAll();
                    }
                    if (ImGui_TooltipMenuItem("Deselect All", KeyCommand::GetDisplayText(KeyCommand::kDeselectAll), "Deselect everything in the scenegraph")) {
                        me->DeselectAll(scenegraph);
                    }
                    ImGui::Separator();

                    if (ImGui_TooltipMenuItem("Reload All Prefabs", NULL, "Reload all prefabs from their source file, assuming they are locked")) {
                        me->ReloadAllPrefabs();
                    }
                    ImGui::Separator();

                    if (ImGui::MenuItem("Set Level Params...")) {
                        show_scenegraph = true;
                        select_scenegraph_level_params = true;
                    }
                    if (ImGui::MenuItem("Set Level Script...")) {
                        me->RibbonItemClicked("level_script", true);
                    }
                    if (ImGui::MenuItem("Set Sky Texture...")) {
                        Input::Instance()->ignore_mouse_frame = true;
                        const int BUF_SIZE = 512;
                        char buffer[BUF_SIZE];

                        // Display a file dialog to the user
                        Dialog::DialogErr err = Dialog::readFile("tga\0png\0jpg\0dds", 4, "Data/Textures/skies/", buffer, BUF_SIZE);
                        if (err != Dialog::NO_ERR) {
                            LOGE << Dialog::DialogErrString(err) << std::endl;
                        } else {
                            const char* find_str[2] = {"data/", "DATA\\"};
                            int found_match = -1;
                            for (size_t i = 0, len = strlen(buffer); i < len - 5; ++i) {
                                bool match = true;
                                for (size_t j = 0; j < 5; ++j) {
                                    if (buffer[i + j] != find_str[0][j] &&
                                        buffer[i + j] != find_str[1][j]) {
                                        match = false;
                                        break;
                                    }
                                }
                                if (match) {
                                    found_match = (int)i;
                                    break;
                                }
                            }
                            if (found_match != -1) {
                                scenegraph->sky->dome_texture_name = &buffer[found_match];
                                scenegraph->sky->LightingChanged(scenegraph->terrain_object_ != NULL);
                            } else {
                                DisplayError("Error", "Path must include 'Data' folder");
                            }
                        }
                    }
                    if (ImGui::MenuItem("Set Player Control Script...")) {
                        const int BUF_SIZE = 512;
                        char buffer[BUF_SIZE];
                        Dialog::DialogErr err = Dialog::readFile("as", 1, "Data/Scripts", buffer, BUF_SIZE);
                        if (err != Dialog::NO_ERR) {
                            LOGE << Dialog::DialogErrString(err) << std::endl;
                        } else {
                            std::string scriptName = SplitPathFileName(buffer).second;
                            scenegraph->level->SetPCScript(scriptName);
                        }
                    }
                    if (ImGui::BeginMenu("Player control script")) {
                        std::string pc_script = scenegraph->level->GetPCScript(NULL);
                        if (pc_script == Level::DEFAULT_PLAYER_SCRIPT) {
                            ImGui::Text("%s (default)", pc_script.c_str());
                        } else {
                            ImGui::Text("%s (custom)", pc_script.c_str());
                        }
                        if (ImGui::MenuItem("Change script...")) {
                            const int BUF_SIZE = 512;
                            char buffer[BUF_SIZE];
                            Dialog::DialogErr err = Dialog::readFile("as", 1, "Data/Scripts", buffer, BUF_SIZE);
                            if (err != Dialog::NO_ERR) {
                                LOGE << Dialog::DialogErrString(err) << std::endl;
                            } else {
                                std::string script_name = SplitPathFileName(buffer).second;
                                for (size_t i = 0; i < scenegraph->movement_objects_.size(); ++i) {
                                    MovementObject* obj = (MovementObject*)scenegraph->movement_objects_[i];
                                    std::string old_script = scenegraph->level->GetPCScript(obj);
                                    if (obj->object_pc_script_path.empty() && obj->GetCurrentControlScript() == old_script) {
                                        obj->ChangeControlScript(script_name);
                                    }
                                }
                                if (script_name != Level::DEFAULT_PLAYER_SCRIPT)
                                    scenegraph->level->SetPCScript(script_name);
                                else
                                    scenegraph->level->ClearPCScript();
                            }
                        }
                        if (ImGui::MenuItem("Clear script")) {
                            for (size_t i = 0; i < scenegraph->movement_objects_.size(); ++i) {
                                MovementObject* obj = (MovementObject*)scenegraph->movement_objects_[i];
                                std::string old_script = scenegraph->level->GetPCScript(obj);
                                if (obj->object_pc_script_path.empty() && obj->GetCurrentControlScript() == old_script) {
                                    obj->ChangeControlScript(obj->GetActorScript().empty() ? Level::DEFAULT_PLAYER_SCRIPT : obj->GetActorScript());
                                }
                            }
                            scenegraph->level->ClearPCScript();
                        }
                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu("Enemy control script")) {
                        std::string npc_script = scenegraph->level->GetNPCScript(NULL);
                        if (npc_script == Level::DEFAULT_ENEMY_SCRIPT) {
                            ImGui::Text("%s (default)", npc_script.c_str());
                        } else {
                            ImGui::Text("%s (custom)", npc_script.c_str());
                        }
                        if (ImGui::MenuItem("Change script...")) {
                            const int BUF_SIZE = 512;
                            char buffer[BUF_SIZE];
                            Dialog::DialogErr err = Dialog::readFile("as", 1, "Data/Scripts", buffer, BUF_SIZE);
                            if (err != Dialog::NO_ERR) {
                                LOGE << Dialog::DialogErrString(err) << std::endl;
                            } else {
                                std::string script_name = SplitPathFileName(buffer).second;
                                for (size_t i = 0; i < scenegraph->movement_objects_.size(); ++i) {
                                    MovementObject* obj = (MovementObject*)scenegraph->movement_objects_[i];
                                    std::string old_script = scenegraph->level->GetNPCScript(obj);
                                    if (obj->object_npc_script_path.empty() && obj->GetCurrentControlScript() == old_script) {
                                        obj->ChangeControlScript(script_name);
                                    }
                                }
                                if (script_name != Level::DEFAULT_ENEMY_SCRIPT)
                                    scenegraph->level->SetNPCScript(script_name);
                                else
                                    scenegraph->level->ClearNPCScript();
                            }
                        }
                        if (ImGui::MenuItem("Clear script")) {
                            for (size_t i = 0; i < scenegraph->movement_objects_.size(); ++i) {
                                MovementObject* obj = (MovementObject*)scenegraph->movement_objects_[i];
                                std::string old_script = scenegraph->level->GetNPCScript(obj);
                                if (obj->object_npc_script_path.empty() && obj->GetCurrentControlScript() == old_script) {
                                    obj->ChangeControlScript(obj->GetActorScript().empty() ? Level::DEFAULT_ENEMY_SCRIPT : obj->GetActorScript());
                                }
                            }
                            scenegraph->level->ClearNPCScript();
                        }
                        ImGui::EndMenu();
                    }
                    ImGui::Separator();
                    bool temp = me->IsTypeEnabled(_env_object);
                    if (ImGui::Checkbox("Edit static meshes", &temp)) {
                        me->RibbonItemClicked("objecteditoractive", temp);
                    }
                    temp = me->IsTypeEnabled(_decal_object);
                    if (ImGui::Checkbox("Edit decals", &temp)) {
                        me->RibbonItemClicked("decaleditoractive", temp);
                    }
                    temp = me->IsTypeEnabled(_hotspot_object);
                    if (ImGui::Checkbox("Edit gameplay objects", &temp)) {
                        me->RibbonItemClicked("hotspoteditoractive", temp);
                    }
                    temp = me->IsTypeEnabled(_dynamic_light_object);
                    if (ImGui::Checkbox("Edit lighting", &temp)) {
                        me->RibbonItemClicked("lighteditoractive", temp);
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Play level", "8")) {
                        me->RibbonItemClicked("sendinrabbot", true);
                    }
                    if (ImGui::MenuItem("Media mode")) {
                        Graphics::Instance()->SetMediaMode(true);
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("View")) {
                    bool temp = scenegraph->IsNavMeshVisible();
                    if (ImGui::Checkbox("Nav mesh", &temp)) {
                        scenegraph->SetNavMeshVisible(temp);
                    }
                    temp = scenegraph->IsCollisionNavMeshVisible();
                    if (ImGui::Checkbox("Nav mesh collision", &temp)) {
                        scenegraph->SetCollisionNavMeshVisible(temp);
                    }
                    ImGui::Checkbox("Collision paint visualization", &g_draw_collision);

                    ImGui::Separator();

                    ImGui::BeginDisabled(!me->GameplayObjectsEnabled());
                    temp = me->IsTypeEnabled(_navmesh_hint_object);
                    if (ImGui::Checkbox("Nav hints", &temp)) {
                        me->SetTypeEnabled(_navmesh_hint_object, temp);
                        me->SetTypeVisible(_navmesh_hint_object, temp);
                    }

                    temp = me->IsTypeEnabled(_navmesh_region_object);
                    if (ImGui::Checkbox("Nav regions", &temp)) {
                        me->SetTypeEnabled(_navmesh_region_object, temp);
                        me->SetTypeVisible(_navmesh_region_object, temp);
                    }

                    temp = me->IsTypeEnabled(_navmesh_connection_object);
                    if (ImGui::Checkbox("Jump nodes", &temp)) {
                        me->SetTypeEnabled(_navmesh_connection_object, temp);
                        me->SetTypeVisible(_navmesh_connection_object, temp);
                    }
                    ImGui::EndDisabled();

                    ImGui::Separator();
                    ImGui::Checkbox("Invisible objects", &g_make_invisible_visible);
                    ImGui::Checkbox("Draw boxes around groups and prefabs", &draw_group_and_prefab_boxes);
                    ImGui::Checkbox("Always show hotspot connections", &always_draw_hotspot_connections);

                    ImGui::EndMenu();
                }

                std::vector<Object*> selected;
                scenegraph->ReturnSelected(&selected);

                if (ImGui::BeginMenu("Selected", selected.size() > 0)) {
                    std::vector<Object*> selected;
                    scenegraph->ReturnSelected(&selected);
                    bool any_selected_object_has_custom_gui = false;
                    bool selected_items_changed = false;

                    for (auto& selected_i : selected) {
                        if (selected_i->GetType() == _hotspot_object) {
                            Hotspot* hotspot = (Hotspot*)selected_i;

                            if (hotspot->HasCustomGUI()) {
                                any_selected_object_has_custom_gui = true;
                                break;
                            }
                        } else if (selected_i->GetType() == _placeholder_object) {
                            PlaceholderObject* placeholder = (PlaceholderObject*)selected_i;

                            if (placeholder->GetScriptParams()->HasParam("Dialogue")) {
                                any_selected_object_has_custom_gui = true;
                                break;
                            }
                        }
                    }

                    if (!any_selected_object_has_custom_gui) {
                        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                    }

                    if (ImGui::MenuItem("Open Custom Editor", KeyCommand::GetDisplayText(KeyCommand::kOpenCustomEditor))) {
                        LOGI << "Launch custom editor(s)" << std::endl;
                        scenegraph->level->Message("edit_selected_dialogue");

                        for (auto& selected_i : selected) {
                            if (selected_i->GetType() == _hotspot_object) {
                                Hotspot* hotspot = (Hotspot*)selected_i;

                                if (hotspot->HasCustomGUI()) {
                                    hotspot->LaunchCustomGUI();
                                }
                            }
                        }
                    }

                    if (!any_selected_object_has_custom_gui) {
                        ImGui::PopItemFlag();
                        ImGui::PopStyleVar();
                    }

                    if (ImGui::MenuItem("Go To Selected", KeyCommand::GetDisplayText(KeyCommand::kFrameSelected))) {
                        CameraObject* co = ActiveCameras::Instance()->Get()->getCameraObject();
                        if (co) {
                            co->FrameSelection(true);
                        }
                    }

                    ImGui::Separator();
                    if (ImGui::MenuItem("Delete", "Backspace")) {
                        me->DeleteSelected();
                        selected_items_changed = true;
                    }
                    if (ImGui::MenuItem("Replace mesh...")) {
                        Input::Instance()->ignore_mouse_frame = true;
                        const int BUF_SIZE = 512;
                        char buf[BUF_SIZE];
                        Dialog::DialogErr err = Dialog::readFile("xml", 1, "Data/Objects", buf, BUF_SIZE);
                        if (!err) {
                            // Un-absolute the file and make it relative to the "best" candidate for relative reference.
                            std::string shortened = FindShortestPath(std::string(buf));
                            scenegraph->map_editor->ReplaceObjects(selected, shortened);
                        }
                        selected_items_changed = true;
                    }

                    ImGui::Separator();
                    if (ImGui::MenuItem("Group", KeyCommand::GetDisplayText(KeyCommand::kGroup))) {
                        me->GroupSelected();
                        selected_items_changed = true;
                    }

                    if (ImGui::MenuItem("Ungroup", KeyCommand::GetDisplayText(KeyCommand::kUngroup))) {
                        me->RibbonItemClicked("ungroup", true);
                        selected_items_changed = true;
                    }

                    // TODO: Reenable?
                    /*ImGui::Separator();
                    if(ImGui::MenuItem("Set Selection Script Params...")) {
                        show_selected = true;
                        select_object_script_params = true;
                    }*/

                    ImGui::Separator();
                    if (ImGui_TooltipMenuItem("Save Selection...", KeyCommand::GetDisplayText(KeyCommand::kSaveSelectedItems), "Save selection in to loadable file, not a prefab")) {
                        me->SaveSelected();
                    }

                    if (ImGui_TooltipMenuItem("Prefab Save", NULL, "Save selection as a prefab object")) {
                        me->SavePrefab(true);
                        selected_items_changed = true;
                    }

                    if (ImGui_TooltipMenuItem("Prefab Save As...", NULL, "Save selection as a prefab object in an explicit path")) {
                        me->SavePrefab(false);
                        selected_items_changed = true;
                    }

                    ImGui::Separator();

                    if (selected_items_changed) {
                        selected.clear();
                        scenegraph->ReturnSelected(&selected);
                    }

                    bool any_movement_objects_selected = false;
                    for (auto& selected_i : selected) {
                        if (selected_i->GetType() == _movement_object) {
                            any_movement_objects_selected = true;
                            break;
                        }
                    }

                    if (!any_movement_objects_selected) {
                        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                    }

                    if (ImGui_TooltipMenuItem("Make Character(s) Corpse", KeyCommand::GetDisplayText(KeyCommand::kMakeSelectedCharacterSavedCorpse), "Save selected characters' current corpse positions (and knock them out, if awake)")) {
                        scenegraph->level->Message("make_selected_character_saved_corpse");
                    }

                    if (ImGui_TooltipMenuItem("Revive Character Corpse(s)", KeyCommand::GetDisplayText(KeyCommand::kReviveSelectedCharacterAndUnsaveCorpse), "Wipe selected characters' saved corpse positions (and revive them, if knocked out)")) {
                        scenegraph->level->Message("revive_selected_character_and_unsave_corpse");
                    }

                    if (!any_movement_objects_selected) {
                        ImGui::PopItemFlag();
                        ImGui::PopStyleVar();
                    }

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Load", "")) {
                    if (ImGui::MenuItem("Load item...", "")) {
                        Input::Instance()->ignore_mouse_frame = true;
                        const int BUF_SIZE = 512;
                        char buf[BUF_SIZE];
                        Dialog::DialogErr err = Dialog::readFile("xml", 1, "Data/Objects", buf, BUF_SIZE);
                        if (!err) {
                            // Un-absolute the file and make it relative to the "best" candidate for relative reference.
                            std::string shortened = FindShortestPath(std::string(buf));
                            if (scenegraph->map_editor->state_ == MapEditor::kIdle && scenegraph->map_editor->LoadEntitiesFromFile(shortened) == 0) {
                                scenegraph->level->PushSpawnerItemRecent(SpawnerItem("Load Item", SplitPathFileName(shortened).second, shortened, "empty_placeholder.png"));
                                scenegraph->map_editor->active_tool_ = EditorTypes::ADD_ONCE;
                            }
                        }
                    }
                    if (ImGui::BeginMenu("Open Recent...")) {
                        std::vector<SpawnerItem> spawner_items = scenegraph->level->GetRecentlyCreatedItems();

                        for (auto it = spawner_items.rbegin(); it != spawner_items.rend(); it++) {
                            AddSpawnerItem(&(*it), scenegraph);
                        }

                        ImGui::EndMenu();
                    }
                    ImGui::Separator();

                    /*if (ImGui::IsWindowHovered()) {
                        ImGui::SetKeyboardFocusHere();
                    }*/

                    // Not calling filter.Draw() so we can pass InputText flags
                    if (ImGui::InputText("", spawner_global_filter.InputBuf, IM_ARRAYSIZE(spawner_global_filter.InputBuf), ImGuiInputTextFlags_AutoSelectAll)) {
                        spawner_global_filter.Build();
                    }

                    if (spawner_global_filter.IsActive()) {
                        for (auto& spawner_tab : spawner_tabs) {
                            SpawnerTab* tab = &spawner_tab.second;

                            for (auto& item_iter : *tab) {
                                std::string item_display_name = spawner_tab.first + " -> " + item_iter.display_name;

                                if (spawner_global_filter.PassFilter(item_iter.mod_source_title.c_str()) || spawner_global_filter.PassFilter(item_display_name.c_str())) {
                                    AddSpawnerItem(&item_iter, scenegraph);
                                }
                            }
                        }
                    } else {
                        for (auto& spawner_tab : spawner_tabs) {
                            if (ImGui::BeginMenu(spawner_tab.first.c_str())) {
                                SpawnerTab* tab = &spawner_tab.second;
                                ImGuiTextFilter& current_tab_filter = spawner_tab_filters[spawner_tab.first];

                                // if (ImGui::IsWindowHovered()) {
                                //     ImGui::SetKeyboardFocusHere();
                                // }

                                // Not calling filter.Draw() so we can pass InputText flags
                                if (ImGui::InputText("", current_tab_filter.InputBuf, IM_ARRAYSIZE(current_tab_filter.InputBuf), ImGuiInputTextFlags_AutoSelectAll)) {
                                    current_tab_filter.Build();
                                }

                                for (auto& item_iter : *tab) {
                                    if (current_tab_filter.PassFilter(item_iter.mod_source_title.c_str()) || current_tab_filter.PassFilter(item_iter.display_name.c_str())) {
                                        AddSpawnerItem(&item_iter, scenegraph);
                                    }
                                }
                                ImGui::EndMenu();
                            }
                        }
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Settings")) {
                    DrawSettingsImGui(scenegraph, IMGST_MENU);
                    ImGui::EndMenu();
                }

                /*if (ImGui::BeginMenu("Lighting")) {
                    if(ImGui::MenuItem("Calculate GI first pass")){
                        me->RibbonItemClicked("rebake_light_probes", true);
                    }
                    if(ImGui::MenuItem("Calculate GI second pass")){
                        me->RibbonItemClicked("calculate2pass", true);
                    }
                    LightProbeCollection* lpc = &scenegraph->light_probe_collection;
                    if(ImGui::Checkbox("Probe lighting enabled", &lpc->probe_lighting_enabled)){
                    }
                    if(ImGui::Checkbox("Show probes through walls", &lpc->show_probes_through_walls)){
                    }
                    if(ImGui::Checkbox("Show tet mesh", &lpc->tet_mesh_viz_enabled)){
                        me->RibbonItemClicked("show_tet_mesh", lpc->tet_mesh_viz_enabled);
                    }
                    if(ImGui::Checkbox("Show probes", &lpc->show_probes)){
                        me->RibbonItemClicked("show_probes", lpc->show_probes);
                    }
                    ImGui::EndMenu();
                }*/
                if (ImGui::BeginMenu("Nav Mesh", !scenegraph->map_editor->GetTerrainPreviewMode())) {
                    NavMeshParameters* nmp = &scenegraph->level->nav_mesh_parameters_;
                    ImGui::Checkbox("Automatically generate navmesh for this level", &nmp->generate);

                    ImGui::Separator();

                    if (ImGui::MenuItem("Create")) {
                        scenegraph->CreateNavMesh();
                    }
                    if (ImGui::MenuItem("Save")) {
                        scenegraph->SaveNavMesh();
                    }
                    if (ImGui::MenuItem("Load")) {
                        scenegraph->LoadNavMesh();
                    }

                    ImGui::Separator();
                    ImGui::Text("%s", "Nav Mesh Parameters");

                    if (ImGui::IsItemHovered()) {
                        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.5, 0.5, 1.0, 0.5));
                        ImGui::BeginTooltip();
                        ImGui::PushTextWrapPos(450.0f);
                        ImGui::Text("%s", "Don't change these parameters if the defaults work well for you. Changing these values will have an impact on how long generation take, and potential navigational ability of characters. If the initial coverage of the navmesh isn't good for you, i suggest you start off with changing the cell size and height to a lower value.");
                        ImGui::PopTextWrapPos();
                        ImGui::EndTooltip();
                        ImGui::PopStyleColor(1);
                    }

                    if (ImGui::SliderFloat("Cell Size", &nmp->m_cellSize, 0.1f, 0.5f, "%.2f")) {
                        Graphics::Instance()->nav_mesh_out_of_date = true;
                    }
                    if (ImGui::SliderFloat("Cell Height", &nmp->m_cellHeight, 0.1f, 0.5f, "%.2f")) {
                        Graphics::Instance()->nav_mesh_out_of_date = true;
                    }
                    if (ImGui::SliderFloat("Agent Height", &nmp->m_agentHeight, 1.5f, 2.0f, "%.2f")) {
                        Graphics::Instance()->nav_mesh_out_of_date = true;
                    }
                    if (ImGui::SliderFloat("Agent Radius", &nmp->m_agentRadius, 0.2f, 1.0f, "%.2f")) {
                        Graphics::Instance()->nav_mesh_out_of_date = true;
                    }
                    if (ImGui::SliderFloat("Agent Max Climb", &nmp->m_agentMaxClimb, 1.0f, 2.0f, "%.2f")) {
                        Graphics::Instance()->nav_mesh_out_of_date = true;
                    }
                    if (ImGui::SliderFloat("Agent Max Slope", &nmp->m_agentMaxSlope, 45.0f, 70.0f, "%.2f")) {
                        Graphics::Instance()->nav_mesh_out_of_date = true;
                    }

                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Dialogue")) {
                    if (ImGui::MenuItem("Edit selected")) {
                        me->RibbonItemClicked("edit_selected_dialogue", true);
                    }
                    /*
                    if(ImGui::MenuItem("Load pose")){
                        me->RibbonItemClicked("load_dialogue_pose", true);
                    }
                    */
                    if (ImGui::MenuItem("Load dialogue")) {
                        me->RibbonItemClicked("load_dialogue", true);
                    }
                    if (ImGui::MenuItem("New dialogue")) {
                        me->RibbonItemClicked("create_empty_dialogue", true);
                    }
                    ImGui::EndMenu();
                }
            }
            if (ImGui::BeginMenu("Windows")) {
                if (scenegraph) {
                    if (ImGui::MenuItem("Scenegraph", KeyCommand::GetDisplayText(KeyCommand::kScenegraph), &show_scenegraph)) {
                    }
                    if (ImGui::MenuItem("Selected", KeyCommand::GetDisplayText(KeyCommand::kEditScriptParams), &show_selected)) {
                    }
                    if (ImGui::MenuItem("ColorPicker", KeyCommand::GetDisplayText(KeyCommand::kEditColor), &show_color_picker)) {
                    }
                    if (ImGui::MenuItem("Collision Paint", "", &show_paintbrush)) {
                    }
                } else {
                    if (ImGui::MenuItem("Mods", NULL, &show_mod_menu)) {
                    }
                }
                if (ImGui::MenuItem("Multiplayer debug", "", &show_mp_debug)) {
                }
                if (ImGui::MenuItem("Multiplayer settings", "", &show_mp_settings)) {
                }
                if (ImGui::MenuItem("Performance", "", &show_performance)) {
                }
                if (ImGui::MenuItem("Log", "", &show_log)) {
                }
                if (ImGui::MenuItem("Warnings", "", &show_warnings)) {
                }
                if (ImGui::MenuItem("Save", "", &show_save)) {
                }
                if (ImGui::MenuItem("State", "", &show_state)) {
                }
                if (ImGui::MenuItem("Sound", "", &show_sound)) {
                }
                if (ImGui::MenuItem("Input", "", &show_input_debug)) {
                }
                bool val = config["debug_draw_window"].toNumber<bool>();
                if (ImGui::MenuItem("Debug Window", "", &val)) {
                    config.GetRef("debug_draw_window") = val;
                }
                if (ImGui::MenuItem("View scripts", "", &show_select_script)) {
                }
                if (ImGui::MenuItem("Test pose", "", &show_pose)) {
                }
                if (ImGui::MenuItem("ImGUI Demo", "", &show_test_window)) {
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Debug")) {
                if (ImGui::MenuItem("Events", "", &show_events)) {
                }
                if (asdebugger_enabled && ImGui::MenuItem("AS debugger contexts", "", &show_asdebugger_contexts)) {
                }
                if (asprofiler_enabled && ImGui::MenuItem("AS profiler", "", &show_asprofiler)) {
                }
                if (ImGui::MenuItem("Graphics Debug Disable", "", &show_graphics_debug_disable_menu)) {
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help")) {
                if (ImGui::BeginMenu("Web Links")) {
                    if (ImGui::MenuItem("Contact us...")) {
                        open_url("http://www.wolfire.com/contact");
                    }
                    if (ImGui::MenuItem("Overgrowth wiki...")) {
                        open_url("http://wiki.wolfire.com/index.php/Portal:Overgrowth");
                    }
                    if (ImGui::MenuItem("Wolfire discussion board...")) {
                        open_url("http://forums.wolfire.com/");
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Hardware Info")) {
                    static bool first = true;
                    static std::string output;
                    if (first) {
                        PrintGPU(output, true);
                        first = false;
                    }

                    ImGui::Text(output.c_str());

                    ImGui::EndMenu();
                }
                if (ImGui::MenuItem("Open Write Directory...", NULL, false, false)) {
                }
                ImGui::Separator();
                if (ImGui::BeginMenu("Credits...")) {
                    ImGui::Text(
                        "Kylie Allen\nMicah J Best\nJimmy Chi\n\
Max Danielsson\nJosh Goheen\nJohn Graham\nPhillip Isola\nTuro Lamminen\n\
Tapio Liukkonen\nRyan Mapa\nBrendan Mauro\nGyrth McMulin\nMerlyn Morgan-Graham\n\
Tuomas Narvainen\nJillian Ogle\n\
Lukas Orsvarn\nConstance Paige\nWes Platt\nTim Pratt\nAnton Riehl\nDavid Rosen\nJeffrey Rosen\nAubrey Serr\nCarl Söyseth\n\
Mark Stockton\nMikko Tarmia\nDavid Lyhed Danielsson");
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("About...")) {
                    ImGui::Text("%s", GetPlatform());
                    ImGui::Text("%s", GetArch());
                    ImGui::Text("%s", GetBuildVersion());
                    ImGui::Text("%s", GetBuildTimestamp());
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if (scenegraph && scenegraph->level) {
                bool draw_menu = false;
                const std::vector<Level::HookedASContext>& contexts = scenegraph->level->as_contexts_;
                for (auto& as_context : scenegraph->level->as_contexts_) {
                    if (as_context.context_name == "mod_level_hook") {
                        if (as_context.ctx->HasFunction(as_context.as_funcs.menu)) {
                            draw_menu = true;
                            break;
                        }
                    }
                }
                if (draw_menu && ImGui::BeginMenu("Mods")) {
                    const std::vector<Level::HookedASContext>& contexts = scenegraph->level->as_contexts_;
                    for (auto& as_context : scenegraph->level->as_contexts_) {
                        if (as_context.context_name == "mod_level_hook") {
                            as_context.ctx->CallScriptFunction(as_context.as_funcs.menu);
                        }
                    }
                    ImGui::EndMenu();
                }
            }
            ImGui::EndMainMenuBar();
        }

        if (show_load_item_search) {
            ImGui::OpenPopup("loaditemsearch");
            show_load_item_search = false;
        }

        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, 19.0f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
        ImGui::SetNextWindowSizeConstraints(ImVec2(-1.0f, -1.0f), ImVec2(-1.0f, -1.0f), ResizeCallback);
        // ImGui::SetNextWindowSizeConstraints(ImVec2(-1.0f, -1.0f), ImVec2(-1.0f, -1.0f), ResizeCallback);
        //  This stupid counter has to be here, because otherwise the SetKeyboardFocusHere
        //  call does nothing the first time the popup is opened if it isn't called
        //  like 30 times
        static int counter = 0;
        if (ImGui::BeginPopup("loaditemsearch")) {
            static int available_numbers = 0;

            int pressed_number = -1;
            for (int i = SDLK_1; i < SDLK_9; ++i) {
                Keyboard& keyboard = Input::Instance()->getKeyboard();
                if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(i)) {
                    pressed_number = i - SDLK_1 + 1;
                    break;
                }
            }
            if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter))) {
                pressed_number = 1;
            }

            if (pressed_number >= available_numbers) {
                pressed_number = -1;
            }

            if (pressed_number == -1) {
                ImGui::Text("Press CTRL + number to spawn items");
                if (counter < 60) {
                    ImGui::SetKeyboardFocusHere(0);
                }
                if (ImGui::InputText("Search for item", spawner_global_filter.InputBuf, IM_ARRAYSIZE(spawner_global_filter.InputBuf), ImGuiInputTextFlags_AutoSelectAll)) {
                    spawner_global_filter.Build();
                }
            }
            bool item_clicked = false;
            if (pressed_number != -1) {
                int items = 0;
                for (SpawnerTabMap::iterator tab_iter = spawner_tabs.begin(); !item_clicked && tab_iter != spawner_tabs.end(); ++tab_iter) {
                    SpawnerTab* tab = &tab_iter->second;

                    for (auto& item_iter : *tab) {
                        std::string item_display_name = tab_iter->first + " -> " + item_iter.display_name;

                        if (spawner_global_filter.PassFilter(item_iter.mod_source_title.c_str()) || spawner_global_filter.PassFilter(item_display_name.c_str())) {
                            if (pressed_number == items + 1 && scenegraph->map_editor->state_ == MapEditor::kIdle && scenegraph->map_editor->LoadEntitiesFromFile(item_iter.path) == 0) {
                                scenegraph->level->PushSpawnerItemRecent(item_iter);
                                scenegraph->map_editor->active_tool_ = EditorTypes::ADD_ONCE;
                                item_clicked = true;
                                break;
                            }
                            items++;
                        }
                    }
                }
            } else {
                int items = 0;
                if (spawner_global_filter.IsActive()) {
                    for (auto& spawner_tab : spawner_tabs) {
                        SpawnerTab* tab = &spawner_tab.second;

                        for (auto& item_iter : *tab) {
                            std::string item_display_name = spawner_tab.first + " -> " + item_iter.display_name;

                            if (spawner_global_filter.PassFilter(item_iter.mod_source_title.c_str()) || spawner_global_filter.PassFilter(item_display_name.c_str())) {
                                if (items < 9) {
                                    char buffer[32];
                                    sprintf(buffer, "%i", items + 1);
                                    ImGui::Text("%s", buffer);
                                    ImGui::SameLine(0.0f, 16.0f);
                                    if (AddSpawnerItem(&item_iter, scenegraph)) {
                                        item_clicked = true;
                                    }
                                } else {
                                    if (AddSpawnerItem(&item_iter, scenegraph)) {
                                        item_clicked = true;
                                    }
                                }
                                items++;
                            }
                        }
                    }
                }
                available_numbers = items + 1;
            }
            if (item_clicked) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
            counter++;
        } else {
            counter = 0;
        }

        if (scenegraph && show_scenegraph) {
            ImGui::SetNextWindowSize(ImVec2(640.0f, 480.0f), ImGuiCond_FirstUseEver);
            ImGui::Begin("Scenegraph", &show_scenegraph);
            if (select_scenegraph_level_params) {
                ImGui::SetNextItemOpen(true);
                // Flag cleared farther down, after Script Params is expanded as well
            }
            if (ImGui::TreeNode("Level")) {
                char input_text_buf[kPathSize];
                strscpy(input_text_buf, scenegraph->level->loading_screen_.image.c_str(), kPathSize);
                static char last_checked_screenshot_path[kPathSize];
                static int last_checked_screenshot_path_color = 0;

                if (strmtch(last_checked_screenshot_path, input_text_buf) == false) {
                    if (FileExists(input_text_buf, kModPaths | kDataPaths)) {
                        last_checked_screenshot_path_color = 1;
                    } else {
                        last_checked_screenshot_path_color = 2;
                    }
                    strscpy(last_checked_screenshot_path, input_text_buf, kPathSize);
                }

                if (last_checked_screenshot_path_color == 1) {
                    ImGui::PushStyleColor(ImGuiCol_Text, IMGUI_GREEN);
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Text, IMGUI_RED);
                }

                if (ImGui::InputText("Loading Screen Image", input_text_buf, kPathSize)) {
                    scenegraph->level->loading_screen_.image = input_text_buf;
                }

                ImGui::PopStyleColor();

                vec3 pos = scenegraph->primary_light.pos;
                if (ImGui::DragFloat3("Sun Position", &pos[0], 0.1f, -1.0f, 1.0f)) {
                    scenegraph->map_editor->sky_editor_->PlaceSun(normalize(pos));
                    shadow_cache_dirty = true;
                    shadow_cache_dirty_sun_moved = true;
                }
                float color = scenegraph->map_editor->sky_editor_->m_sun_color_angle;
                if (ImGui::DragFloat("Sun Color", &color, 1.0f)) {
                    scenegraph->map_editor->sky_editor_->m_sun_color_angle = 0.0f;
                    scenegraph->map_editor->sky_editor_->RotateSun(color);
                }
                float sun_intensity = scenegraph->map_editor->sky_editor_->m_sun_angular_rad;
                if (ImGui::DragFloat("Sun Intensity", &sun_intensity, 0.001f, 0.0f, 0.314f)) {
                    scenegraph->map_editor->sky_editor_->m_sun_angular_rad = sun_intensity;
                    scenegraph->map_editor->sky_editor_->ScaleSun(1.0f);
                }
                if (select_scenegraph_level_params) {
                    ImGui::SetScrollHereY();
                    ImGui::SetNextItemOpen(true);
                    select_scenegraph_level_params = false;
                }

                if (ImGui::Checkbox("Enable dynamic shadows for this level", &g_level_shadows)) {
                    Graphics::Instance()->setSimpleShadows(g_simple_shadows);
                    scenegraph->level->setMetaDataDirty();
                }
                if (g_simple_shadows) {
                    ImGui::Text("Note: disable \"Simple shadows\" in the settings menu to see dynamic shadows");
                }

                /*ImGui::Separator();
                ImGui::Text("%s", "Terrain");
                const TerrainInfo* terrain_info = NULL;
                if(scenegraph->map_editor->GetPreviewTerrainInfo()) {
                    terrain_info = scenegraph->map_editor->GetPreviewTerrainInfo();
                } else {
                    for(size_t i = 0; i < scenegraph->terrain_objects_.size(); ++i) {
                        TerrainObject* terrain_object = scenegraph->terrain_objects_[i];
                        terrain_info = &(terrain_object->terrain_info());
                        break;
                    }
                }

                char buffer[kPathSize];
                const char* path = "";
                if(terrain_info) {
                    path = terrain_info->heightmap.c_str();
                    if(!terrain_info->heightmap.empty()) {
                        strcpy(buffer, terrain_info->heightmap.c_str());
                        *strrchr(buffer, '/') = '\0';
                    } else {
                        strcpy(buffer, "Data/Textures/Terrain");
                    }
                    if(DrawBrowsePath("Heightmap", path, "png\0dds\0tga", 3, buffer, kPathSize, buffer)) {
                        scenegraph->map_editor->PreviewTerrainHeightmap(buffer);
                    }
                    path = terrain_info->colormap.c_str();
                    if(!terrain_info->colormap.empty()) {
                        strcpy(buffer, terrain_info->colormap.c_str());
                        *strrchr(buffer, '/') = '\0';
                    } else {
                        strcpy(buffer, "Data/Textures/Terrain");
                    }
                    if(DrawBrowsePath("Colormap", path, "png\0dds\0tga", 3, buffer, kPathSize, buffer)) {
                        scenegraph->map_editor->PreviewTerrainColormap(buffer);
                    }
                    path = terrain_info->weightmap.c_str();
                    if(!terrain_info->weightmap.empty()) {
                        strcpy(buffer, terrain_info->weightmap.c_str());
                        *strrchr(buffer, '/') = '\0';
                    } else {
                        strcpy(buffer, "Data/Textures/Terrain");
                    }
                    if(DrawBrowsePath("Weightmap", path, "png\0dds\0tga", 3, buffer, kPathSize, buffer)) {
                        scenegraph->map_editor->PreviewTerrainWeightmap(buffer);
                    }
                    for(int i = 0; i < (int)terrain_info->detail_map_info.size(); ++i) {
                        if(ImGui::TreeNode(&terrain_info->detail_map_info[i], "Detail map %d", i)) {
                            const DetailMapInfo& info = terrain_info->detail_map_info[i];
                            path = info.colorpath.c_str();
                            if(!info.colorpath.empty()) {
                                strcpy(buffer, info.colorpath.c_str());
                                *strrchr(buffer, '/') = '\0';
                            } else {
                                strcpy(buffer, "Data/Textures/Terrain/DetailTextures/");
                            }
                            if(DrawBrowsePath("Colormap", path, "png\0dds\0tga", 3, buffer, kPathSize, buffer)) {
                                scenegraph->map_editor->PreviewTerrainDetailmap(i, buffer, info.normalpath.c_str(), info.materialpath.c_str());
                            }
                            path = info.normalpath.c_str();
                            if(!info.normalpath.empty()) {
                                strcpy(buffer, info.normalpath.c_str());
                                *strrchr(buffer, '/') = '\0';
                            } else {
                                strcpy(buffer, "Data/Textures/Terrain/DetailTextures/");
                            }
                            if(DrawBrowsePath("Normal", path, "png\0dds\0tga", 3, buffer, kPathSize, buffer)) {
                                scenegraph->map_editor->PreviewTerrainDetailmap(i, info.colorpath.c_str(), buffer, info.materialpath.c_str());
                            }
                            path = info.materialpath.c_str();
                            if(!info.materialpath.empty()) {
                                strcpy(buffer, info.materialpath.c_str());
                                *strrchr(buffer, '/') = '\0';
                            } else {
                                strcpy(buffer, "Data/Objects/");
                            }
                            if(DrawBrowsePath("Material", path, "png\0dds\0tga", 3, buffer, kPathSize, buffer)) {
                                scenegraph->map_editor->PreviewTerrainDetailmap(i, info.colorpath.c_str(), info.materialpath.c_str(), buffer);
                            }
                            ImGui::TreePop();
                        }
                    }
                } else {
                    if(DrawBrowsePath("Heightmap", path, "png\0dds\0tga", 3, buffer, kPathSize, "Data/Textures/Terrain")) {
                        scenegraph->map_editor->PreviewTerrainHeightmap(buffer);
                    }
                }*/

                ImGui::Separator();
                if (ImGui::TreeNode("Script Params")) {
                    ScriptParamMap spm = scenegraph->level->script_params().GetParameterMap();
                    {
                        ScriptParam& sp = spm["Sky Rotation"];
                        sp.SetFloat(scenegraph->sky->sky_rotation);
                        sp.editor().SetDisplaySlider("min:-360,max:360,step:1,text_mult:1");
                    }
                    {
                        ScriptParam& sp = spm["Level Boundaries"];
                        sp.SetInt(scenegraph->level->script_params().ASGetInt("Level Boundaries"));
                        sp.editor().SetCheckbox();
                    }
                    {
                        ScriptParam& sp = spm["Shared Camera"];
                        sp.SetInt(scenegraph->level->script_params().ASGetInt("Shared Camera"));
                        sp.editor().SetCheckbox();
                    }
                    {
                        ScriptParam& sp = spm["HDR White point"];
                        sp.SetFloat(Graphics::Instance()->hdr_white_point);
                        sp.editor().SetDisplaySlider("min:0,max:2,step:0.001,text_mult:100");
                    }
                    {
                        ScriptParam& sp = spm["HDR Black point"];
                        sp.SetFloat(Graphics::Instance()->hdr_black_point);
                        sp.editor().SetDisplaySlider("min:0,max:2,step:0.001,text_mult:100");
                    }
                    {
                        ScriptParam& sp = spm["HDR Bloom multiplier"];
                        sp.SetFloat(Graphics::Instance()->hdr_bloom_mult);
                        sp.editor().SetDisplaySlider("min:0,max:5,step:0.001,text_mult:100");
                    }
                    {
                        ScriptParam& sp = spm["Saturation"];
                        sp.SetFloat(scenegraph->level->script_params().ASGetFloat("Saturation"));
                        sp.editor().SetDisplaySlider("min:0,max:2,step:0.001,text_mult:100");
                    }
                    {
                        ScriptParam& sp = spm["Fog amount"];
                        sp.SetFloat(scenegraph->fog_amount);
                        sp.editor().SetDisplaySlider("min:0,max:5s,step:0.1,text_mult:10");
                    }
                    {
                        scenegraph->level->script_params().ASAddString("Sky Tint", "255, 255, 255");
                        spm["Sky Tint"].SetString(scenegraph->level->script_params().GetStringVal("Sky Tint"));
                        spm["Sky Tint"].editor().SetColorPicker();
                    }
                    {
                        scenegraph->level->script_params().ASAddFloat("Sky Brightness", 1.0f);
                        spm["Sky Brightness"].SetFloat(scenegraph->level->script_params().ASGetFloat("Sky Brightness"));
                        spm["Sky Brightness"].editor().SetDisplaySlider("min:0,max:5,step:0.01,text_mult:100");
                    }
                    scenegraph->level->script_params().SetParameterMap(spm);
                    if (DrawScriptParamsEditor(&scenegraph->level->script_params())) {
                        SceneGraph::ApplyScriptParams(scenegraph, scenegraph->level->script_params().GetParameterMap());
                        scenegraph->map_editor->QueueSaveHistoryState();
                    }
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }
            ImGui::Separator();
            if (select_scenegraph_search) {
                ImGui::SetKeyboardFocusHere();
                select_scenegraph_search = false;
            }
            if (ImGui::InputText("", scenegraph_filter.InputBuf, IM_ARRAYSIZE(scenegraph_filter.InputBuf), ImGuiInputTextFlags_AutoSelectAll)) {
                scenegraph_filter.Build();
                select_start_index = -1;
            }
            ImGui::Checkbox("Flat", &show_flat_scenegraph);
            ImGui::Checkbox("Search children", &search_children_scenegraph);
            ImGui::Checkbox("Named Only", &show_named_only_scenegraph);
            if (show_flat_scenegraph) {
                for (auto obj : scenegraph->objects_) {
                    const int kBufSize = 512;
                    char buf[kBufSize];
                    if ((show_named_only_scenegraph == false || obj->GetName().empty() == false) && obj->selectable_) {
                        obj->GetDisplayName(buf, kBufSize);
                        if (scenegraph_filter.IsActive() && !scenegraph_filter.PassFilter(buf)) {
                            bool pass = false;
                            if (search_children_scenegraph) {
                                if (obj->IsGroupDerived()) {
                                    char child_buf[kBufSize];
                                    Group* group = static_cast<Group*>(obj);
                                    for (auto& i : group->children) {
                                        i.direct_ptr->GetDisplayName(child_buf, kBufSize);
                                        if (scenegraph_filter.PassFilter(child_buf)) {
                                            pass = true;
                                            break;
                                        }
                                    }
                                }
                            }

                            if (!pass)
                                continue;
                        }

                        ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

                        if (obj->Selected()) {
                            node_flags |= ImGuiTreeNodeFlags_Selected;
                        }
                        ImGui::PushID(obj->GetID());
                        vec4 color = GetObjColor(obj);
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(color[0], color[1], color[2], color[3]));
                        bool node_open = ImGui::TreeNodeEx("", node_flags, "%s", buf);
                        ImGui::PopStyleColor(1);
                        ImGui::PopID();
                        if (ImGui::IsItemClicked()) {
                            obj->Select(!obj->Selected());
                        }
                        if (node_open) {
                            DrawObjectInfo(obj, false);
                            ImGui::TreePop();
                        }
                    }
                }
            } else {
                DrawTreeScenegraphFor(scenegraph, gui, NULL);
            }
            ImGui::End();
        }
        if (scenegraph && show_selected) {
            ImGui::SetNextWindowSize(ImVec2(640.0f, 480.0f), ImGuiCond_FirstUseEver);
            ImGui::Begin("Selected", &show_selected);

            std::vector<Object*> selected;
            scenegraph->ReturnSelected(&selected);

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
            ImGui::Text(selected.size() == 1 ? "%u object selected" : "%u objects selected", selected.size());
            ImGui::PopStyleColor(1);

            for (size_t i = 0, len = selected.size(); i < len; ++i) {
                const int kBufSize = 512;
                char buf[kBufSize];
                Object* obj = selected[i];
                obj->GetDisplayName(buf, kBufSize);
                if (len > 1) {
                    ImGui::PushID(obj->GetID());

                    vec4 color = GetObjColor(obj);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(color[0], color[1], color[2], color[3]));
                    bool node_open = ImGui::TreeNode("", "%s", buf);
                    ImGui::PopStyleColor(1);

                    ImGui::SameLine();
                    ImGui::SmallButton("copy");
                    if (ImGui::BeginPopupContextItem("copy object info", 0)) {
                        ImGui::Text("Copy to clipboard");
                        ImGui::Separator();
                        if (ImGui::Button("copy object id")) {
                            char copy_buf[kBufSize];
                            FormatString(copy_buf, kBufSize, "%i", obj->GetID());
                            ImGui::SetClipboardText(copy_buf);
                            ImGui::CloseCurrentPopup();
                        }
                        if (ImGui::Button("copy object path")) {
                            char copy_buf[kBufSize];
                            FormatString(copy_buf, kBufSize, "%s", obj->obj_file.c_str());
                            ImGui::SetClipboardText(copy_buf);
                            ImGui::CloseCurrentPopup();
                        }
                        if (ImGui::Button("copy all")) {
                            ImGui::SetClipboardText(buf);
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }

                    ImGui::PopID();

                    if (node_open) {
                        DrawObjectInfoFlat(obj);
                        ImGui::TreePop();
                    }
                } else {
                    vec4 color = GetObjColor(obj);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(color[0], color[1], color[2], color[3]));
                    ImGui::Text("%s", buf);
                    ImGui::PopStyleColor(1);

                    ImGui::SameLine();
                    ImGui::SmallButton("copy");
                    if (ImGui::BeginPopupContextItem("copy object info", 0)) {
                        ImGui::Text("Copy to clipboard");
                        ImGui::Separator();
                        if (ImGui::Button("copy object id")) {
                            char copy_buf[kBufSize];
                            FormatString(copy_buf, kBufSize, "%i", obj->GetID());
                            ImGui::SetClipboardText(copy_buf);
                            ImGui::CloseCurrentPopup();
                        }
                        if (ImGui::Button("copy object path")) {
                            char copy_buf[kBufSize];
                            FormatString(copy_buf, kBufSize, "%s", obj->obj_file.c_str());
                            ImGui::SetClipboardText(copy_buf);
                            ImGui::CloseCurrentPopup();
                        }
                        if (ImGui::Button("copy all")) {
                            ImGui::SetClipboardText(buf);
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }

                    DrawObjectInfoFlat(obj);
                }
            }
            select_object_script_params = false;
            ImGui::End();
        }

        if (scenegraph && show_paintbrush) {
            ImGui::Begin("Collision", &show_paintbrush, ImGuiWindowFlags_AlwaysAutoResize);
            static int paint_type = 0;
            ImGui::RadioButton("Per-object", &paint_type, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Per-triangle", &paint_type, 1);

            static int spread = 1;
            static float spread_threshold = 0.0;
            if (paint_type == 1) {
                ImGui::RadioButton("Don't spread", &spread, 0);
                ImGui::RadioButton("Spread across flat surfaces", &spread, 1);
                ImGui::RadioButton("Spread across similar normals", &spread, 2);

                if (spread == 2) {
                    ImGui::DragFloat("Range", &spread_threshold, 0.01f, -1.0f, 1.0f);
                }
            }

            if (ImGui::Button("Save")) {
                SaveCollisionNormals(scenegraph);
            }
            ImGui::SameLine();
            if (ImGui::Button("Load")) {
                LoadCollisionNormals(scenegraph);
            }

            ImGui::Separator();
            static int paintbrush_type = 0;
            ImGui::Text("Floor:");
            ImGui::RadioButton("Walkable", &paintbrush_type, 1);
            ImGui::SameLine();
            ImGui::RadioButton("Balance", &paintbrush_type, 2);
            ImGui::SameLine();
            ImGui::RadioButton("Slide", &paintbrush_type, 3);
            ImGui::Text("Wall:");
            ImGui::RadioButton("Wallrun", &paintbrush_type, 4);
            ImGui::SameLine();
            ImGui::RadioButton("No wallrun", &paintbrush_type, 5);
            ImGui::SameLine();
            ImGui::RadioButton("Ledge", &paintbrush_type, 6);
            ImGui::SameLine();
            ImGui::RadioButton("Ragdoll", &paintbrush_type, 10);
            ImGui::SameLine();
            ImGui::RadioButton("Ragdoll-death", &paintbrush_type, 11);
            ImGui::Text("Ceiling:");
            ImGui::RadioButton("Ceiling", &paintbrush_type, 7);
            ImGui::Text("Unlabeled:");
            ImGui::RadioButton("Clear", &paintbrush_type, 8);
            ImGui::SameLine();
            ImGui::RadioButton("No climb", &paintbrush_type, 9);
            if (paintbrush_type == 9) {
                if (ImGui::Button("Apply 'no climb' to all unlabeled surfaces")) {
                    for (auto obj : scenegraph->collide_objects_) {
                        if (obj->GetType() == _env_object) {
                            EnvObject* eo = (EnvObject*)obj;
                            for (auto& i : eo->normal_override_custom) {
                                if (i == vec4(0.0f)) {
                                    i = vec4(0.0f, 9.5f, 0.0f, 1.0f);
                                }
                            }
                            eo->normal_override_buffer_dirty = true;
                        }
                    }
                }
            }
            const bool kEnableBrushSize = false;
            static float brush_size = 10.0f;
            if (kEnableBrushSize) {
                ImGui::DragFloat("Size", &brush_size);
                if (brush_size < 1.0f) {
                    brush_size = 1.0f;
                }
                Camera* cam = ActiveCameras::Get();
                vec3 cam_facing = cam->GetFacing();
                vec3 cam_up = cam->GetUpVector();
                vec3 cam_right = cross(cam_facing, cam_up);
                mat4 old_scale;
                old_scale.SetUniformScale(brush_size);
                mat4 circle_transform;
                circle_transform.SetColumn(0, cam_right);
                circle_transform.SetColumn(1, cam_up);
                circle_transform.SetColumn(2, cam_facing);
                circle_transform.SetTranslationPart(cam->GetPos() + normalize(cam->GetMouseRay()) * 100.0f);
                DebugDraw::Instance()->AddCircle(circle_transform * old_scale, vec4(0.0f, 0.0f, 0.0f, 0.5f), _delete_on_draw, _DD_XRAY);
                // Paintbrush tag (wall, floor, ceiling)
                // Paintbrush toggle by-asset or by-triangle
                // Paintbrush size
            }

            ImGui::End();

            PROFILER_ZONE(g_profiler_ctx, "Normal override paint");
            Collision mouseray_collision_selected;
            /*
            std::vector<Collision> collisions;
            GetEditorLineCollisions(scenegraph_, mouseray.start, mouseray.end, collisions, type_enable_);
            for(int i=0, len=collisions.size(); i<len; ++i){
                if(collisions[i].hit_what && collisions[i].hit_what->GetType()==_env_object){
                    EnvObject* eo = (EnvObject*)collisions[i].hit_what;
                    if(eo->GetCollisionModelID() != -1 && !eo->ofr->no_collision){
                        mouseray_collision_selected = collisions[i];
                        break;
                    }
                }
            }*/

            bool actually_painting = (Input::Instance()->getKeyboard().isScancodeDown(SDL_SCANCODE_B, KIMF_LEVEL_EDITOR_GENERAL));
            bool actually_edge_painting = (Input::Instance()->getKeyboard().isScancodeDown(SDL_SCANCODE_V, KIMF_LEVEL_EDITOR_GENERAL));

            SimpleRayTriResultCallback cb;
            int num_samples = 1;
            if (paint_type == 1 && kEnableBrushSize) {
                num_samples = 20;
            }
            for (int i = 0; i < num_samples; ++i) {
                float rand_amount = 0.0f;
                if (paint_type == 1 && kEnableBrushSize) {
                    rand_amount = brush_size * 0.01f;
                }
                vec3 jitter = vec3(RangedRandomFloat(-rand_amount, rand_amount), RangedRandomFloat(-rand_amount, rand_amount), RangedRandomFloat(-rand_amount, rand_amount));
                vec3 dir = ActiveCameras::Get()->GetMouseRay();
                cb.m_collisionObject = NULL;
                float dist = 10.0f;
                while (!cb.m_collisionObject && dist < 1000.0f) {
                    scenegraph->bullet_world_->CheckRayTriCollisionInfo(ActiveCameras::Get()->GetPos(), ActiveCameras::Get()->GetPos() + normalize(dir + jitter) * dist, cb, true);
                    dist *= 2.0f;
                }
                if (cb.hasHit() && cb.m_object) {
                    mouseray_collision_selected.hit_what = cb.m_object->owner_object;
                    mouseray_collision_selected.hit_where = cb.m_hit_pos;
                    mouseray_collision_selected.hit_how = cb.m_tri;
                } else {
                    mouseray_collision_selected.hit_what = NULL;
                }

                if (mouseray_collision_selected.hit_what) {
                    if (mouseray_collision_selected.hit_what->GetType() == _env_object) {
                        EnvObject* eo = (EnvObject*)mouseray_collision_selected.hit_what;
                        vec4 norm;
                        if (paintbrush_type != 8) {
                            norm = vec4(0, paintbrush_type + 0.5f, 0, 1.0f);
                        } else {
                            norm = vec4(0.0f);
                        }
                        if (paint_type == 0) {
                            if (actually_painting) {
                                for (auto& i : eo->normal_override_custom) {
                                    i = norm;
                                }
                                eo->normal_override_buffer_dirty = true;
                            }
                        } else {
                            int model_id = eo->GetCollisionModelID();
                            if (model_id != -1) {
                                Model* model = &Models::Instance()->GetModel(model_id);
                                WOLFIRE_SIMPLIFY::SimplifyModel processed_model;
                                std::vector<HalfEdge> half_edges;
                                WOLFIRE_SIMPLIFY::Process(*model, processed_model, half_edges, true);
                                std::vector<int> half_edge_faces;
                                std::vector<int> face_vert_equivalent;
                                FindFaces(*model, processed_model, half_edges, half_edge_faces, face_vert_equivalent);
                                vec3 verts[3];
                                int face_index = mouseray_collision_selected.hit_how * 3;
                                int vert_translate[3] = {-1};
                                for (size_t index = 0, len = processed_model.vertices.size(); index < len; index += 3) {
                                    for (int vert_id = 0; vert_id < 3; ++vert_id) {
                                        int vert_index = model->faces[face_index + vert_id] * 3;
                                        if (processed_model.vertices[index + 0] == model->vertices[vert_index + 0] &&
                                            processed_model.vertices[index + 1] == model->vertices[vert_index + 1] &&
                                            processed_model.vertices[index + 2] == model->vertices[vert_index + 2]) {
                                            vert_translate[vert_id] = (int)index / 3;
                                        }
                                    }
                                }
                                for (int vert_id = 0; vert_id < 3; ++vert_id) {
                                    int vert_index = model->faces[face_index + vert_id] * 3;
                                    memcpy(&verts[vert_id], &model->vertices[vert_index], sizeof(vec3));
                                    verts[vert_id] = eo->GetTransform() * verts[vert_id];
                                }
                                std::vector<int> checked;
                                checked.resize(half_edges.size(), 0);
                                std::queue<HalfEdge*> to_check;
                                for (auto& half_edge : half_edges) {
                                    if (half_edge.vert[0] == vert_translate[0] &&
                                        half_edge.vert[1] == vert_translate[1]) {
                                        to_check.push(&half_edge);
                                    }
                                }
                                vec3 start_norm;
                                bool norm_set = false;
                                while (!to_check.empty()) {
                                    HalfEdge* curr = to_check.front();
                                    to_check.pop();
                                    if (checked[curr->id] == 1) {
                                        continue;
                                    }

                                    vec3 verts[3];
                                    for (auto& vert : verts) {
                                        int vert_index = processed_model.old_vert_id[curr->vert[0]] * 3;
                                        memcpy(&vert, &model->vertices[vert_index], sizeof(vec3));
                                        curr = curr->next;
                                    }

                                    vec3 curr_norm = normalize(cross(verts[1] - verts[0], verts[2] - verts[0]));
                                    if (!norm_set) {
                                        norm_set = true;
                                        start_norm = curr_norm;

                                        float shortest_dist = FLT_MAX;
                                        int closest_edge = 0;
                                        for (int edge = 0; edge < 3; ++edge) {
                                            vec3 perp_dir = normalize(cross(dir, eo->GetTransform() * verts[(edge + 1) % 3] - eo->GetTransform() * verts[edge]));
                                            float dist = fabs(dot(perp_dir, eo->GetTransform() * verts[edge]) - dot(perp_dir, mouseray_collision_selected.hit_where));
                                            if (dist < shortest_dist) {
                                                shortest_dist = dist;
                                                closest_edge = edge;
                                            }
                                        }
                                        for (int edge = 0; edge < 3; ++edge) {
                                            vec4 color = vec4(1.0);
                                            if (edge == closest_edge) {
                                                color = vec4(0, 1, 0, 1);
                                                if (actually_edge_painting) {
                                                    DebugDraw::Instance()->AddLine(eo->GetTransform() * verts[edge], eo->GetTransform() * verts[(edge + 1) % 3], color, _fade, _DD_XRAY);
                                                }
                                            }
                                            DebugDraw::Instance()->AddLine(eo->GetTransform() * verts[edge], eo->GetTransform() * verts[(edge + 1) % 3], color, _delete_on_draw, _DD_XRAY);
                                        }
                                    }
                                    bool can_spread = false;
                                    if (spread == 1) {
                                        can_spread = distance(start_norm, curr_norm) < 0.1f;
                                    } else if (spread == 2) {
                                        can_spread = dot(start_norm, curr_norm) > spread_threshold;
                                    }
                                    if (actually_painting && half_edge_faces[curr->id] != -1 && (spread == 0 || can_spread)) {
                                        if ((int)eo->normal_override_custom.size() > half_edge_faces[curr->id]) {
                                            eo->normal_override_custom[half_edge_faces[curr->id]] = norm;
                                        }
                                        eo->normal_override_buffer_dirty = true;
                                    }
                                    if (can_spread) {
                                        for (int i = 0; i < 3; ++i) {
                                            // DebugDraw::Instance()->AddLine(eo->GetTransform()*verts[i], eo->GetTransform()*verts[(i+1)%3], vec4(1.0), _delete_on_draw, _DD_XRAY);
                                            checked[curr->id] = 1;
                                            if (curr->twin && checked[curr->twin->id] == 0) {
                                                to_check.push(curr->twin);
                                            }
                                            curr = curr->next;
                                        }
                                    }
                                }
                                eo->ledge_lines.clear();
                                for (auto& half_edge : half_edges) {
                                    HalfEdge* curr = &half_edge;
                                    if (curr->twin) {
                                        if ((int)eo->normal_override_custom.size() > half_edge_faces[curr->id]) {
                                            int curr_color = (int)(eo->normal_override_custom[half_edge_faces[curr->id]][1]);
                                            int neighbor_color = (int)(eo->normal_override_custom[half_edge_faces[curr->twin->id]][1]);
                                            if (curr_color == 6 && neighbor_color < 4) {
                                                for (int j : curr->vert) {
                                                    int vert_index = processed_model.old_vert_id[j] * 3;
                                                    eo->ledge_lines.push_back(vert_index);
                                                }
                                            }
                                        }
                                    }
                                }
                                for (size_t index = 1, len = eo->ledge_lines.size(); index < len; index += 2) {
                                    DebugDraw::Instance()->AddLine(eo->GetTransform() * *((vec3*)&model->vertices[eo->ledge_lines[index - 1]]),
                                                                   eo->GetTransform() * *((vec3*)&model->vertices[eo->ledge_lines[index]]),
                                                                   vec4(1.0),
                                                                   vec4(1.0),
                                                                   _fade,
                                                                   _DD_XRAY);
                                }
                            }
                        }
                    }
                }
            }
        }

        if (show_mod_menu) {
            show_mod_menu_previous = true;
            DrawModMenu(engine);
        }

        if (show_mod_menu_previous && !show_mod_menu) {
            show_mod_menu_previous = false;
            CleanupModMenu();
        }

        if (scenegraph && show_color_picker) {
            std::vector<Object*> selected;
            scenegraph->ReturnSelected(&selected);

            ImGui::SetNextWindowSize(ImVec2(0.0f, 0.0f), ImGuiCond_FirstUseEver);
            ImGui::Begin("Color picker", &show_color_picker, ImGuiWindowFlags_NoResize);

            DrawColorPicker(selected.data(), (int)selected.size(), scenegraph);

            ImGui::End();
        }
        if (!gui->debug_text.empty()) {
            if (config["debug_draw_window"].toBool()) {
                ImGui::SetNextWindowPos(ImVec2(10, 30));
                ImGui::Begin("##debug_text_overlay", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs);
                DrawDebugText(gui);
                ImGui::End();
            } else if (config["fps_label"].toBool()) {
                ImGui::SetNextWindowPos(ImVec2(10, 30));
                ImGui::Begin("##debug_text_overlay", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs);
                DrawDebugText(gui, "_framerate");
                DrawDebugText(gui, "_frametime");
                ImGui::End();
            }
        }
        if (show_test_window) {
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
            ImGui::ShowDemoWindow(&show_test_window);
        }
    } else if (scenegraph && scenegraph->map_editor->state_ == MapEditor::kInGame && !gui->debug_text.empty()) {
        if (engine->current_engine_state_.type == kEngineEditorLevelState && config["debug_draw_window"].toBool()) {
            ImGui::SetNextWindowPos(ImVec2(10, 10));

            ImGui::Begin("##debug_text_overlay", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs);
            DrawDebugText(gui);
            ImGui::End();
        } else if (config["fps_label"].toBool()) {
            ImGui::SetNextWindowPos(ImVec2(10, 30));
            ImGui::Begin("##debug_text_overlay", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs);
            DrawDebugText(gui, "_framerate");
            DrawDebugText(gui, "_frametime");
            ImGui::End();
        }
    }

    if (show_mp_settings) {
        ImGui::SetNextWindowPos(ImVec2(0.f, 200.f));
        ImGui::Begin("Multiplayer Settings", &show_mp_info,
                     ImGuiWindowFlags_NoTitleBar |
                         ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_AlwaysAutoResize);

        bool host_allow_editor = online->GetHostAllowsClientEditor();
        ImGui::Checkbox("Allow clients using the editor", &host_allow_editor);

        online->SetIfHostAllowsClientEditor(host_allow_editor);

        if (spawner_tabs.find("Character") != spawner_tabs.end()) {
            SpawnerTab* character_tab = &spawner_tabs["Character"];
            std::string current_hot_join_char = online->GetDefaultHotJoinCharacter();
            ImGui::Text(("Hot join character: " + current_hot_join_char).c_str());
            if (ImGui::BeginMenu("Select hot join character")) {
                for (auto& i : *character_tab) {
                    SpawnerItem* item = &i;  // no way that the asm is pretty here

                    if (IsCharacterSelceted(item)) {
                        online->SetDefaultHotJoinCharacter(item->path);
                    }
                }
                ImGui::EndMenu();
            }
        }

        ImGui::End();
    }

    if (show_build_watermark && GetBuildID() != -1) {
        const ImVec4 transparent = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, transparent);
        ImGui::PushStyleColor(ImGuiCol_Border, transparent);
        ImGui::SetNextWindowPos(ImVec2(0, Graphics::Instance()->render_dims[1] - ImGui::GetTextLineHeight() * 2), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(400, ImGui::GetTextLineHeight()), ImGuiCond_Always);
        ImGui::Begin("VersionWatermark", (bool*)1, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), std::string("BUILD ID: " + std::to_string(GetBuildID()) + ", IN_DEV").c_str());
        ImGui::End();
        ImGui::PopStyleColor(2);
    }

    static bool write_to_chat = false;
    if (online->IsActive()) {
        Input* input = Input::Instance();
        const Keyboard& keyboard = input->getKeyboard();
        int window_x = Graphics::Instance()->render_dims[0];
        int window_y = Graphics::Instance()->render_dims[1];

        ImGuiWindowFlags w_flags = 0;

        static bool enter_is_down_this_frame = false;
        static bool enter_was_down_last_frame = false;

        enter_was_down_last_frame = enter_is_down_this_frame;
        enter_is_down_this_frame = keyboard.isKeycodeDown(SDLK_RETURN, KIMF_ANY);

        static bool wait_for_enter_release = false;
        static float time_from_host_start = 0;
        static bool host_started_level_last_frame = false;

        if (host_started_level_last_frame != online->host_started_level) {  // check if host started the level this frame
            wait_for_enter_release = true;
            host_started_level_last_frame = online->host_started_level;
            time_from_host_start = game_timer.game_time;
            write_to_chat = false;  // close the chat when moving from lobby into game
        }

        if (wait_for_enter_release) {
            // can not open chat by pressing enter for the first half second when going from loading screen to game, except from if enter is released
            if (game_timer.game_time - time_from_host_start > 0.5f) {
                wait_for_enter_release = false;
            } else if (enter_was_down_last_frame && !enter_is_down_this_frame) {
                wait_for_enter_release = false;
            }
        }

        bool player_pressed_enter = enter_is_down_this_frame && !enter_was_down_last_frame && !wait_for_enter_release;

        bool open_chat = false;
        if (!write_to_chat && player_pressed_enter) {
            write_to_chat = true;
            open_chat = true;
        } else if (write_to_chat && player_pressed_enter) {
            write_to_chat = false;
        }

        if (show_chat) {
            SceneGraph* scene_graph = Engine::Instance()->GetSceneGraph();
            if (scene_graph) {
                // This will prevent the chat from open when i a dialogue.
                ObjectID player_object_id = online->GetOwnPlayerState().object_id;
                if (player_object_id != -1) {
                    Object* player_object = scene_graph->GetObjectFromID(player_object_id);
                    if (player_object && player_object->GetType() == _movement_object) {
                        MovementObject* player_mo = static_cast<MovementObject*>(player_object);
                        // if player is in dialogue set write_to_chat to false;
                        write_to_chat &= !*reinterpret_cast<bool*>(player_mo->as_context->module.GetVarPtrCache("dialogue_control"));
                    }
                }
            }

            const auto& chat_messages = online->GetChatMessages();
            constexpr size_t line_count = 5;
            constexpr float chat_width = 600;
            constexpr float chat_alpha = 0.1f;
            ImVec2 chat_write_pos = ImVec2(10, window_y - ImGui::GetTextLineHeight() * 2 - 10);
            ImVec2 chat_text_pos = chat_write_pos - ImVec2(0, ImGui::GetTextLineHeight() * line_count * 2);

            if (chat_messages.size() > 0) {
                ImGui::SetNextWindowBgAlpha(chat_alpha);
                ImGui::SetNextWindowPos(chat_text_pos, ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(chat_width, ImGui::GetTextLineHeight() * line_count * 2), ImGuiCond_Always);
                ImGui::Begin("Online Chat", &show_chat, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs);

                // Pad lines so messages are always bottom aligned
                for (size_t i = 0; i < line_count + 1; i++) {
                    ImGui::Text("");
                }

                // Display actual chat messages
                ImGui::PushTextWrapPos();
                for (const auto& chat_message : chat_messages) {
                    ImGui::Text(chat_message.message.c_str());
                }
                ImGui::PopTextWrapPos();

                // Smooth scroll to the bottom of the chat window
                if (ImGui::GetScrollY() < ImGui::GetScrollMaxY()) {
                    ImGui::SetScrollY(ImGui::GetScrollY() + (ImGui::GetScrollMaxY() - ImGui::GetScrollY() + 10.0f) * ImGui::GetIO().DeltaTime * 10);
                }

                ImGui::End();

                online->RemoveOldChatMessages(10.0f);  // todo: should be moved to mp.update once thats a thing
            }
            if (write_to_chat) {
                ImGui::SetNextWindowBgAlpha(chat_alpha);
                ImGui::SetNextWindowPos(chat_write_pos, ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(chat_width, ImGui::GetTextLineHeight()), ImGuiCond_Always);
                ImGui::Begin("Online Chat Write", &show_chat, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs);

                static char input_buffer[512];  // player will be able to write a message with 511 chars
                if (open_chat) {
                    ImGui::SetKeyboardFocusHere();
                    memset(input_buffer, '\0', sizeof(input_buffer));
                }
                if (write_to_chat) {
                    if (ImGui::InputText("", input_buffer, sizeof(input_buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
                        write_to_chat = false;

                        if (input_buffer[0] != '\0') {
                            online->BroadcastChatMessage(input_buffer);
                        }
                    }
                }
                ImGui::End();
            }
        }
    } else {
        write_to_chat = false;
    }

    if (show_mp_debug && online->IsActive()) {
        ImGui::SetNextWindowSize(ImVec2(640.0f, 480.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Multiplayer status", &show_mp_debug, ImGuiWindowFlags_NoSavedSettings);
        std::vector<ObjectID> myAvatarID = online->GetLocalAvatarIDs();
        float step = online->GetNoDataInterpStepOverRide();
        ImGui::SliderFloat("Interp no data override", &step, 0, 1.0);
        online->SetNoDataInterpStepOverRide(step);

        if (ImGui::Button("Send Test Message")) {
            online->Send<OnlineMessages::TestMessage>("Test message from debug menu");
        }

        if (online->IsClient()) {
            ImGui::SliderFloat("Time Shift Coefficient", &TimeInterpolator::offset_shift_coefficient_factor, 0.0f, 20.0f, "%.3f", 0);
            ImGui::SliderInt("Target Window Size", &TimeInterpolator::target_window_size, 3, 16);
        }

#if ENABLE_STEAMWORKS || ENABLE_GAMENETWORKINGSOCKETS
        ISteamNetworkingUtils* utils = SteamNetworkingUtils();
        ISteamNetworkingSockets* isns = SteamNetworkingSockets();
        if (utils && isns) {
            SteamNetworkingConfigValue_t config[4];

            static int32_t outPing = 0;
            static int32_t inPing = 0;

            static float packetLossIn = 0;
            static float packetOutIn = 0;

            ImGui::Text(std::string("Outgoing Messages: " + std::to_string(online->OutgoingMessageCount())).c_str());
            ImGui::Text(std::string("Incoming Messages: " + std::to_string(online->IncomingMessageCount())).c_str());

            if (ImGui::TreeNode("Link Quality Testing")) {
                ImGui::SliderInt("Ping out in ms", &outPing, 0, 1000);
                ImGui::SliderInt("Ping in ms", &inPing, 0, 1000);

                ImGui::SliderFloat("Packet loss percentage OUT", &packetOutIn, 0, 100);
                ImGui::SliderFloat("Packet loss percentage IN", &packetLossIn, 0, 100);
                ImGui::TreePop();
            }

            config[0].SetInt32(ESteamNetworkingConfigValue::k_ESteamNetworkingConfig_FakePacketLag_Send, outPing);
            config[1].SetInt32(ESteamNetworkingConfigValue::k_ESteamNetworkingConfig_FakePacketLag_Recv, inPing);
            config[2].SetFloat(ESteamNetworkingConfigValue::k_ESteamNetworkingConfig_FakePacketLoss_Recv, packetLossIn);
            config[3].SetFloat(ESteamNetworkingConfigValue::k_ESteamNetworkingConfig_FakePacketLoss_Send, packetOutIn);

            utils->SetConfigValueStruct(config[0], ESteamNetworkingConfigScope::k_ESteamNetworkingConfig_Global, 0);
            utils->SetConfigValueStruct(config[1], ESteamNetworkingConfigScope::k_ESteamNetworkingConfig_Global, 0);
            utils->SetConfigValueStruct(config[2], ESteamNetworkingConfigScope::k_ESteamNetworkingConfig_Global, 0);
            utils->SetConfigValueStruct(config[3], ESteamNetworkingConfigScope::k_ESteamNetworkingConfig_Global, 0);
            intptr_t packageSize = 0;
            ESteamNetworkingConfigDataType type;
            int mtuDatasize = 0;
            int mtuPacketsize = 0;
            size_t size = 4;
            int outgoingbuffersize = 0;
            int sentRateMax = -1;
            int sentRateMin = -1;

            utils->GetConfigValue(k_ESteamNetworkingConfig_MTU_PacketSize, ESteamNetworkingConfigScope::k_ESteamNetworkingConfig_Global, 0, &type, &mtuDatasize, &size);
            utils->GetConfigValue(k_ESteamNetworkingConfig_MTU_DataSize, ESteamNetworkingConfigScope::k_ESteamNetworkingConfig_Global, 0, &type, &mtuPacketsize, &size);
            utils->GetConfigValue(k_ESteamNetworkingConfig_SendRateMax, ESteamNetworkingConfigScope::k_ESteamNetworkingConfig_Global, 0, &type, &sentRateMax, &size);
            utils->GetConfigValue(k_ESteamNetworkingConfig_SendRateMin, ESteamNetworkingConfigScope::k_ESteamNetworkingConfig_Global, 0, &type, &sentRateMin, &size);
            utils->GetConfigValue(k_ESteamNetworkingConfig_SendBufferSize, ESteamNetworkingConfigScope::k_ESteamNetworkingConfig_Global, 0, &type, &outgoingbuffersize, &size);

            if (ImGui::TreeNode("Steam Network Config")) {
                ImGui::SliderInt("MTU packet size", &mtuPacketsize, 1, 5000);

                ImGui::SliderInt("Internal buffer size", &outgoingbuffersize, 1, 4096 * 60 * 10);
                ImGui::TreePop();
            }

            SteamNetworkingConfigValue_t config_1[4];

            config_1[0].SetInt32(ESteamNetworkingConfigValue::k_ESteamNetworkingConfig_MTU_PacketSize, mtuPacketsize + 100);

            config_1[1].SetInt32(ESteamNetworkingConfigValue::k_ESteamNetworkingConfig_SendBufferSize, outgoingbuffersize);

            config_1[2].SetInt32(ESteamNetworkingConfigValue::k_ESteamNetworkingConfig_SendRateMin, 1024 * 1024 * 9);

            config_1[3].SetInt32(ESteamNetworkingConfigValue::k_ESteamNetworkingConfig_SendRateMax, 1024 * 1024 * 10);

            utils->SetConfigValueStruct(config_1[0], ESteamNetworkingConfigScope::k_ESteamNetworkingConfig_Global, 0);
            utils->SetConfigValueStruct(config_1[1], ESteamNetworkingConfigScope::k_ESteamNetworkingConfig_Global, 0);

            utils->SetConfigValueStruct(config_1[2], ESteamNetworkingConfigScope::k_ESteamNetworkingConfig_Global, 0);
            utils->SetConfigValueStruct(config_1[3], ESteamNetworkingConfigScope::k_ESteamNetworkingConfig_Global, 0);
        }
#endif
        if (ImGui::TreeNode("Data config")) {
            static int tickperiod = 30;
            ImGui::SliderInt("Compression level", &online->compression_level, 0, 100);
            ImGui::SliderInt("Tickperiod in ms", &tickperiod, 1, 200);
            online->SetTickperiod(tickperiod);
            ImGui::TreePop();
        }

        static uint8_t current_frame = 0;

        std::vector<Peer> peers = online->GetPeers();
        static ImGuiTableFlags flags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
                                       ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable;

        uint32_t count = 0;
        float bytesOut = 0.f;
        float bytesIn = 0.f;
        if (ImGui::BeginTable("##table", 6, flags, ImVec2(-1, 0))) {
            ImGui::TableSetupColumn("Player", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Message Ping", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Ping", ImGuiTableColumnFlags_WidthFixed, 75.0f);

            ImGui::TableSetupColumn("Bytes out", ImGuiTableColumnFlags_WidthFixed, 75.0f);
            ImGui::TableSetupColumn("Bytes in", ImGuiTableColumnFlags_WidthFixed, 75.0f);
            ImGui::TableSetupColumn("Packet loss", ImGuiTableColumnFlags_WidthFixed, 75.0f);
            ImGui::TableHeadersRow();
            ImPlot::PushColormap(ImPlotColormap_Cool);
            for (auto& it : peers) {
                ConnectionStatus c_status;
                online->GetConnectionStatus(it, &c_status);
                ImGui::TableNextRow();
                static ImGuiTableFlags flags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
                                               ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable;
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%d", count++);

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%f", it.current_ping_delta);

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%d", c_status.ms_ping);

                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%f", c_status.in_bytes_per_sec);

                ImGui::TableSetColumnIndex(4);
                ImGui::Text(" %f", c_status.out_bytes_per_sec);

                ImGui::TableSetColumnIndex(5);
                ImGui::Text("%f", 1.0f - c_status.connection_quality_remote);
                bytesIn += c_status.in_bytes_per_sec;
                bytesOut += c_status.out_bytes_per_sec;
            }
            ImPlot::PopColormap();
            ImGui::EndTable();
        }

#ifdef NETWORK_DEBUG_DATA
        static ScrollingBuffer<ImVec2> sdata1, sdata2, sdata3, sdata4, sdata5;

        GraphTime += ImGui::GetIO().DeltaTime;

        static float history = 10.0f;
        static ImPlotAxisFlags flags1 = 0;
        ImPlotAxisFlags_NoTickLabels;
        if (ImGui::TreeNode("Data graphs")) {
            ImGui::SliderFloat("History", &history, 1, 30, "%.1f s");

            sdata1.AddPoint(ImVec2(GraphTime, (bytesOut) / 1000.f));
            sdata2.AddPoint(ImVec2(GraphTime, (bytesIn) / 1000.f));
            sdata3.AddPoint(ImVec2(GraphTime, (bytesIn + bytesOut) / 1000.f));

            if (ImPlot::BeginPlot("##BytesRealTime", ImVec2(-1, 150))) {
                ImPlot::SetupAxes(NULL, NULL, flags1, flags1);
                ImPlot::SetupAxisLimits(ImAxis_X1, GraphTime - history, GraphTime, ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1000);
                ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);

                ImPlot::PlotLine("Total traffic bytes", &sdata3.points[0].x, &sdata3.points[0].y, (int)sdata3.points.size(), sdata3.offset, 2 * sizeof(float));
                ImPlot::PlotLine("Total traffic bytes out", &sdata1.points[0].x, &sdata1.points[0].y, (int)sdata1.points.size(), sdata1.offset, 2 * sizeof(float));
                ImPlot::PlotLine("Total traffic bytes in", &sdata2.points[0].x, &sdata2.points[0].y, (int)sdata2.points.size(), sdata2.offset, 2 * sizeof(float));

                ImPlot::EndPlot();
            }

            ImGui::TreePop();
        }
#endif

        if (ImGui::TreeNode("Player debug info")) {
            for (const auto& player_state : online->GetPlayerStates()) {
                ImGui::Text("Playername: %d", player_state.playername.c_str());
                ImGui::Text("Object ID: %d (%d)", player_state.object_id, online->GetOriginalID(player_state.object_id));
                ImGui::Text("Ping: %d", player_state.ping);
                ImGui::Text("Controller ID: %d", player_state.controller_id);
                ImGui::Text("Camera ID: %d", player_state.camera_id);
                ImGui::Separator();
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Peer debug info")) {
            auto peers = online->GetPeers();
            for (const auto& peer : peers) {
                ImGui::Text("Connection ID: %d", peer.conn_id);
                ImGui::Text("Peer ID: %d", peer.peer_id);
                ImGui::Separator();
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Player Characters")) {
            if (scenegraph != nullptr) {
                int num_avatars;
                int avatar_ids[256];

                scenegraph->GetPlayerCharacterIDs(&num_avatars, avatar_ids, 256);

                for (int i = 0; i < num_avatars; i++) {
                    MovementObject* avatar = static_cast<MovementObject*>(scenegraph->GetObjectFromID(engine->avatar_ids[i]));

                    ImGui::Text("ID %d", avatar->GetID());
                    ImGui::Text("Name %s", avatar->GetName().c_str());
                    ImGui::Text("Controller %d", avatar->controller_id);
                    ImGui::Text("Controlled %s", avatar->controlled ? "true" : "false");
                    ImGui::Text("Camera ID %d", avatar->camera_id);
                    ImGui::Separator();
                }
                ImGui::TreePop();
            }
        }

        if (ImGui::TreeNode("NPC Characters")) {
            if (scenegraph != nullptr) {
                int num_avatars;
                int avatar_ids[256];

                scenegraph->GetNPCCharacterIDs(&num_avatars, avatar_ids, 256);

                for (int i = 0; i < num_avatars; i++) {
                    MovementObject* avatar = static_cast<MovementObject*>(scenegraph->GetObjectFromID(engine->avatar_ids[i]));

                    ImGui::Text("ID %d", avatar->GetID());
                    ImGui::Text("Name %s", avatar->GetName().c_str());
                    ImGui::Text("Controller %d", avatar->controller_id);
                    ImGui::Text("Controlled %s", avatar->controlled ? "true" : "false");
                    ImGui::Separator();
                }
                ImGui::TreePop();
            }
        }

        if (ImGui::TreeNode("Character Graphs")) {
            if (scenegraph != nullptr) {
                int num_avatars;
                int avatar_ids[256];

                scenegraph->GetCharacterIDs(&num_avatars, avatar_ids, 256);

                for (int i = 0; i < num_avatars; i++) {
                    float pending_bones = 0.0f;

                    std::stringstream ss;
                    ss << avatar_ids[i];
                    std::string id_prefix = ss.str();

                    MovementObject* movobj = (MovementObject*)(scenegraph->GetObjectFromID(avatar_ids[i]));
                    RiggedObject* rigged = movobj->rigged_object();

                    ImGui::PushID(avatar_ids[i]);
                    if (ImGui::TreeNode("MO #%s", id_prefix.c_str())) {
                        if (movobj != nullptr) {
                            pending_bones = movobj->network_time_interpolator.full_interpolation;

                            graphs[id_prefix + "_host_walltime_diff"].Push(movobj->last_walltime_diff);
                            graphs[id_prefix + "_host_walltime_offset_shift"].Push(movobj->network_time_interpolator.host_walltime_offset_shift_vel);

                            graphs[id_prefix + "_root_bone_x"].Push(rigged->GetDisplayBonePosition(0).x());
                            graphs[id_prefix + "_root_bone_y"].Push(rigged->GetDisplayBonePosition(0).y());
                            graphs[id_prefix + "_root_bone_z"].Push(rigged->GetDisplayBonePosition(0).z());

                            graphs[id_prefix + "_rigged_interpolation_time"].Push(0);
                            graphs[id_prefix + "_mov_interpolation_step"].Push(movobj->network_time_interpolator.interpolation_step);

                            graphs[id_prefix + "_player_velocity"].Push(length(movobj->velocity));
                            graphs[id_prefix + "_delta_between_frames"].Push(0);

                            graphs[id_prefix + "_pending_bones_history"].Push(movobj->network_time_interpolator.full_interpolation);

                            ImGui::Checkbox("Update Camera Pos", &rigged->update_camera_pos);
                            ImGui::Checkbox("Disable Linear Interpolation", &movobj->disable_network_bone_interpolation);
                        }

                        graphs[id_prefix + "_host_walltime_diff"].Draw("Host Walltime Diff");
                        graphs[id_prefix + "_host_walltime_offset_shift"].Draw("Host Walltime Offset Shift");
                        graphs[id_prefix + "_rig_interpolation_step"].Draw("Rig Interpolation Step");
                        graphs[id_prefix + "_mov_interpolation_step"].Draw("MO Interpolation Step");

                        graphs[id_prefix + "_root_bone_x"].Draw("Root Bone X");
                        graphs[id_prefix + "_root_bone_y"].Draw("Root Bone Y");
                        graphs[id_prefix + "_root_bone_z"].Draw("Root Bone Z");

                        graphs[id_prefix + "_rigged_interpolation_time"].Draw("Rigged Object Interpolation Time");

                        graphs[id_prefix + "_player_velocity"].Draw("Player Velocity History");
                        graphs[id_prefix + "_delta_between_frames"].Draw("Delta Between Frame History");

                        graphs[id_prefix + "_pending_bones_history"].Draw("Nr Pending Bones History");

                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
            }
            ImGui::TreePop();
        }

        ImGui::Separator();
        DrawDebugText(gui, "_framerate");
        DrawDebugText(gui, "_frametime");

        ImGui::End();
    }

    if (show_state) {
        ImGui::SetNextWindowSize(ImVec2(500.0f, 200.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("State", &show_state);

        ImGui::Text("Engine State Stack");

        ImGui::Text("0 %s: %s (%s)",
                    CStrEngineStateType(engine->current_engine_state_.type),
                    engine->current_engine_state_.path.GetOriginalPath(),
                    engine->current_engine_state_.id.c_str());

        for (unsigned i = 0; i < engine->state_history.size(); i++) {
            ImGui::Text("%u %s: %s (%s)",
                        i + 1,
                        CStrEngineStateType(engine->state_history[i].type),
                        engine->state_history[i].path.GetOriginalPath(),
                        engine->state_history[i].id.c_str());
        }

        ImGui::Separator();

        ScriptableCampaign* sc = engine->GetCurrentCampaign();
        if (sc) {
            ImGui::Text("Current Campaign ID: %s", sc->GetCampaignID().c_str());
        } else {
            ImGui::Text("No Active Campaign");
        }

        std::string current_level_id = engine->GetCurrentLevelID();
        if (current_level_id.empty() == false) {
            ImGui::Text("Currently Loaded Level: %s", current_level_id.c_str());
        } else {
            ImGui::Text("No Level Currently Loaded");
        }

        ImGui::End();
    }

    if (show_save) {
        ImGui::SetNextWindowSize(ImVec2(350.0f, 150.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Save", &show_save);
        std::set<std::string> campaign_ids;
        for (size_t i = 0; i < engine->save_file_.GetSaveCount(); i++) {
            SavedLevel& level = engine->save_file_.GetSaveIndex(i);

            campaign_ids.insert(level.campaign_id_);
        }

        std::set<std::string>::iterator campaign_id_it = campaign_ids.begin();
        for (; campaign_id_it != campaign_ids.end(); campaign_id_it++) {
            const char* node_title = "{empty}";
            if (*campaign_id_it != "") {
                node_title = (*campaign_id_it).c_str();
            }

            char save_t_id[256];
            FormatString(save_t_id, 256, "save_cat_%s", node_title);

            if (ImGui::TreeNode(save_t_id, "%s", node_title)) {
                for (size_t i = 0; i < engine->save_file_.GetSaveCount(); i++) {
                    SavedLevel& level = engine->save_file_.GetSaveIndex(i);
                    if (*campaign_id_it == level.campaign_id_) {
                        FormatString(save_t_id, 256, "save_inst_%s_%s_%s", level.campaign_id_.c_str(), level.save_category_.c_str(), level.save_name_.c_str());
                        if (ImGui::TreeNode(save_t_id, "%s %s", level.save_category_.c_str(), level.save_name_.c_str())) {
                            const DataMap& data_map = level.GetDataMap();

                            if (data_map.size() > 0) {
                                if (ImGui::TreeNode("DataMap")) {
                                    DataMap::const_iterator data_map_it = data_map.begin();

                                    for (; data_map_it != data_map.end(); data_map_it++) {
                                        ImGui::Text("[%s]:%s", data_map_it->first.c_str(), data_map_it->second.c_str());
                                    }
                                    ImGui::TreePop();
                                }
                            } else {
                                ImGui::Text("%s", "No DataMap Data");
                            }

                            const ArrayDataMap& array_data_map = level.GetArrayDataMap();

                            if (array_data_map.size() > 0) {
                                if (ImGui::TreeNode("ArrayDataMap")) {
                                    ArrayDataMap::const_iterator array_data_map_it = array_data_map.begin();
                                    for (; array_data_map_it != array_data_map.end(); array_data_map_it++) {
                                        if (ImGui::TreeNode(array_data_map_it->first.c_str())) {
                                            std::vector<std::string>::const_iterator array_data_it = array_data_map_it->second.begin();
                                            int index = 0;
                                            for (; array_data_it != array_data_map_it->second.end(); array_data_it++) {
                                                ImGui::Text("%d: %s", index, array_data_it->c_str());
                                                index++;
                                            }

                                            ImGui::TreePop();
                                        }
                                    }

                                    ImGui::TreePop();
                                }
                            } else {
                                ImGui::Text("%s", "No ArrayDataMap Data");
                            }

                            ImGui::TreePop();
                        }
                    }
                }

                ImGui::TreePop();
            }
        }

        ImGui::End();
    }

    if (show_events) {
        ImGui::SetNextWindowSize(ImVec2(300.0f, 146.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Events", &show_events);
        static fixed_string<256> event_buf;
        ImGui::InputText("Message", event_buf.ptr(), event_buf.max_size());

        if (ImGui::Button("Send As Global Event")) {
            std::string msg(event_buf.str());
            if (scenegraph) {
                scenegraph->SendScriptMessageToAllObjects(msg);
                scenegraph->level->Message(msg);
            }
            ScriptableCampaign* sc = Engine::Instance()->GetCurrentCampaign();
            if (sc) {
                sc->ReceiveMessage(msg);
            }
        }
        if (ImGui::Button("Send As Campaign Event")) {
            std::string msg(event_buf.str());
            ScriptableCampaign* sc = Engine::Instance()->GetCurrentCampaign();
            if (sc) {
                sc->ReceiveMessage(msg);
            }
        }
        if (ImGui::Button("Send As Level Event")) {
            std::string msg(event_buf.str());
            if (scenegraph) {
                scenegraph->level->Message(msg);
            }
        }

        if (ImGui::Button("Send as ScriptableUICallback")) {
            std::string msg(event_buf.str());
            Engine::Instance()->ScriptableUICallback(msg);
        }

        ImGui::End();
    }

    if (show_performance) {
        ImGui::SetNextWindowSize(ImVec2(640.0f, 500.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Performance", &show_performance);
        static PrecisionStopwatch frame_time_stopwatch;
        const int kNumElements = 40;
        static float frame_times[kNumElements] = {0.0f};
        static bool paused = true;
        ImGui::Checkbox("Pause", &paused);
        static float max_time = 0.0f;
        if (!paused) {
            max_time = 0.0f;
            for (int i = 0, len = kNumElements - 1; i < len; ++i) {
                frame_times[i] = frame_times[i + 1];
            }
            frame_times[kNumElements - 1] = (float)(frame_time_stopwatch.StopAndReportNanoseconds() / 1000000.0);
            for (int i = 0, len = kNumElements; i < len; ++i) {
                max_time = max(max_time, frame_times[i]);
            }
        } else {
            frame_time_stopwatch.StopAndReportNanoseconds();
        }
        ImGui::PlotHistogram("Frame Times", frame_times, kNumElements, 0, NULL, 0.0f, 100.0f, ImVec2(0, 80));
        ImGui::Text("Max time: %3.2f ms (%3.0f fps)", max_time, 1000.0f / (max_time));

        ImGui::Separator();

#if MONITOR_MEMORY
        ImGui::Text("malloc Calls This Frame: %d", GetAndResetMallocAllocationCount());
        ImGui::Text("malloc Current Allocations: %d", GetCurrentNumberOfMallocAllocations());

        for (int i = 0; i < OG_MALLOC_TYPE_COUNT; i++) {
            uint64_t mem_cb = GetCurrentTotalMallocAllocation(i);
            if (mem_cb >= 1024) {
                ImGui::Text("malloc Memory Use (%s): %u KiB\n", OgMallocTypeString(i), (unsigned)(mem_cb / 1024));
            } else {
                ImGui::Text("malloc Memory Use (%s): %u B\n", OgMallocTypeString(i), (unsigned)mem_cb);
            }
        }
#else
        ImGui::TextWrapped("Compile project with -DMONITOR_MEMORY to have detailed memory use information on malloc and new");
#endif
        ImGui::Separator();

        ImGui::Text("Stack Frame Allocs: %d", alloc.stack.GetAndResetAllocationCount());
        ImGui::Text("Stack Current Allocs: %d", alloc.stack.GetCurrentAllocations());
        if (alloc.stack.AllocatedAmountMemory() >= 1024) {
            ImGui::Text("Stack Alloc Use: %u KiB", (unsigned)(alloc.stack.AllocatedAmountMemory() / 1024));
        } else {
            ImGui::Text("Stack Alloc Use: %u B", (unsigned)(alloc.stack.AllocatedAmountMemory()));
        }

        ImGui::Separator();
        ImGui::Text("Total Active Assets: %d", assetmanager->GetLoadedAssetCount());

        if (ImGui::IsItemClicked()) {
            asset_detail_list.clear();
            asset_detail_list_load_flags.clear();
            asset_detail_list_ref_count.clear();
            asset_detail_list_load_warning.clear();

            asset_detail_list_type = (AssetType)0;
            asset_detail_title = "Complete Asset List";
            for (size_t k = 0; k < assetmanager->GetLoadedAssetCount(); k++) {
                asset_detail_list.push_back(assetmanager->GetAssetName(k));
                asset_detail_list_load_flags.push_back(assetmanager->GetAssetHoldFlags(k));
                asset_detail_list_ref_count.push_back(assetmanager->GetAssetRefCount(k));
                asset_detail_list_load_warning.push_back(assetmanager->GetAssetLoadWarning(k));
            }
        }

        ImGui::Text("Asset Load Warnings: %d", assetmanager->GetAssetWarningCount());

        if (ImGui::IsItemClicked()) {
            asset_detail_list.clear();
            asset_detail_list_load_flags.clear();
            asset_detail_list_ref_count.clear();
            asset_detail_list_load_warning.clear();

            asset_detail_list_type = (AssetType)0;
            asset_detail_title = "Asset Load Warning List";
            for (size_t k = 0; k < assetmanager->GetLoadedAssetCount(); k++) {
                if (assetmanager->GetAssetLoadWarning(k)) {
                    asset_detail_list.push_back(assetmanager->GetAssetName(k));
                    asset_detail_list_load_flags.push_back(assetmanager->GetAssetHoldFlags(k));
                    asset_detail_list_ref_count.push_back(assetmanager->GetAssetRefCount(k));
                    asset_detail_list_load_warning.push_back(assetmanager->GetAssetLoadWarning(k));
                }
            }
        }

        ImGui::Text("Assets held (Likely Preloaded): %d", assetmanager->GetAssetHoldCount());
        if (ImGui::IsItemClicked()) {
            asset_detail_list.clear();
            asset_detail_list_load_flags.clear();
            asset_detail_list_ref_count.clear();
            asset_detail_list_load_warning.clear();

            asset_detail_list_type = (AssetType)0;
            asset_detail_title = "Assets held";
            for (size_t k = 0; k < assetmanager->GetLoadedAssetCount(); k++) {
                if (assetmanager->GetAssetHoldFlags(k)) {
                    asset_detail_list.push_back(assetmanager->GetAssetName(k));
                    asset_detail_list_load_flags.push_back(assetmanager->GetAssetHoldFlags(k));
                    asset_detail_list_ref_count.push_back(assetmanager->GetAssetRefCount(k));
                    asset_detail_list_load_warning.push_back(assetmanager->GetAssetLoadWarning(k));
                }
            }
        }

        int tot = 0;
        for (int i = 1; i < (int)ASSET_TYPE_FINAL; i++) {
            int c = assetmanager->GetAssetTypeCount((AssetType)i);
            ImGui::Text("Active %s: %d", GetAssetTypeString((AssetType)i), c);
            tot += c;

            if (ImGui::IsItemClicked()) {
                asset_detail_list.clear();
                asset_detail_list_load_flags.clear();
                asset_detail_list_ref_count.clear();
                asset_detail_list_load_warning.clear();
                asset_detail_list_type = (AssetType)i;
                asset_detail_title = "Dump insight to loaded AssetType";
                for (size_t k = 0; k < assetmanager->GetLoadedAssetCount(); k++) {
                    if (assetmanager->GetAssetType(k) == (AssetType)i) {
                        asset_detail_list.push_back(assetmanager->GetAssetName(k));
                        asset_detail_list_load_flags.push_back(assetmanager->GetAssetHoldFlags(k));
                        asset_detail_list_ref_count.push_back(assetmanager->GetAssetRefCount(k));
                        asset_detail_list_load_warning.push_back(assetmanager->GetAssetLoadWarning(k));
                    }
                }
            }
        }

        ImGui::Text("Total Sanity Check: %d", tot);

        if (asset_detail_list.size() > 0) {
            ImGui::Separator();
            ImGui::Text("%s: %s", asset_detail_title, GetAssetTypeString(asset_detail_list_type));
            if (ImGui::IsItemClicked()) {
                asset_detail_list.clear();
                asset_detail_list_load_flags.clear();
                asset_detail_list_ref_count.clear();
                asset_detail_list_load_warning.clear();
            }
            for (unsigned int i = 0; i < asset_detail_list.size(); i++) {
                const char* second = "";
                if (asset_detail_list_load_flags[i] & HOLD_LOAD_MASK_PRELOAD) {
                    ImGui::PushStyleColor(ImGuiCol_Text, IMGUI_GREEN);
                    second = " (Held by preloader)";
                } else if (asset_detail_list_ref_count[i] == 0) {
                    ImGui::PushStyleColor(ImGuiCol_Text, IMGUI_RED);
                } else if (asset_detail_list_load_warning[i]) {
                    ImGui::PushStyleColor(ImGuiCol_Text, IMGUI_ORANGE);
                    second = " (Post Loadscreen Warning)";
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Text, IMGUI_WHITE);
                }
                ImGui::Text("(refc: %d) %s%s", (int)asset_detail_list_ref_count[i], asset_detail_list[i].c_str(), second);
                ImGui::PopStyleColor();
            }
        }

        ImGui::Separator();

        Textures::Instance()->DrawImGuiDebug();

        ImGui::Separator();

        DrawAudioDebug();

        ImGui::End();
    }

    if (show_log) {
        static bool show_log_scrollbar = false;
        static bool show_log_spam = false;
        static bool show_log_debug = false;
        static bool show_log_info = false;
        static bool show_log_warning = true;
        static bool show_log_error = true;
        static bool do_lock = false;
        static bool do_pause = false;
        static uint8_t* edit_log_field = NULL;
        static uint32_t edit_log_field_size;
        static uint32_t edit_log_selection[2] = {0, 0};

        bool do_pop_style = false;

        int window_x = Graphics::Instance()->render_dims[0];
        int window_y = Graphics::Instance()->render_dims[1];

        ImGuiWindowFlags w_flags = 0;

        if (do_lock) {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 1.0f, 1.0f, 0.05f));
            do_pop_style = true;
            ImGui::SetNextWindowPos(ImVec2(10, window_y - ImGui::GetTextLineHeight() * 20 - 10), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(window_x * 0.8f, ImGui::GetTextLineHeight() * 20), ImGuiCond_Always);
            w_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs;
        } else {
            ImGui::SetNextWindowSize(ImVec2(700, 500), ImGuiCond_FirstUseEver);
        }

        ImGui::Begin("Log", &show_log, w_flags);

        ImGui::BeginChild("settings", ImVec2(0, 20));
        if (do_pause == false) {
            ImGui::Checkbox("Show Scrollbar", &show_log_scrollbar);
            ImGui::SameLine();
            ImGui::Checkbox("Info", &show_log_info);
            ImGui::SameLine();
            ImGui::Checkbox("Warnings", &show_log_warning);
            ImGui::SameLine();
            ImGui::Checkbox("Errors", &show_log_error);
            ImGui::SameLine();
            ImGui::Checkbox("Debug", &show_log_debug);
            ImGui::SameLine();
            ImGui::Checkbox("Spam", &show_log_spam);
            ImGui::SameLine();
            ImGui::Checkbox("Lock", &do_lock);
            ImGui::SameLine();
        }
        if (ImGui::Checkbox("Pause", &do_pause)) {
            if (do_pause) {
                ram_handler.Lock();
                uint32_t count = ram_handler.GetCount();
                uint32_t current_pos = 0;
                edit_log_field_size = 0;
                if (edit_log_field) {
                    OG_FREE(edit_log_field);
                    edit_log_field = NULL;
                }
                for (uint32_t stage = 0; stage < 2; stage++) {
                    if (stage == 1) {
                        edit_log_field = (uint8_t*)OG_MALLOC(edit_log_field_size + 1);
                        edit_log_field[edit_log_field_size] = '\0';
                    }
                    for (uint32_t i = 0; i < count; i++) {
                        RamHandlerLogRow rhlr = ram_handler.GetLog(i);

                        const char* level_string = "";
                        bool skip = false;
                        bool pushed_style = false;

                        switch (rhlr.type) {
                            case LogSystem::spam:
                                level_string = "s";
                                skip = !show_log_spam;
                                break;
                            case LogSystem::debug:
                                level_string = "d";
                                skip = !show_log_debug;
                                break;
                            case LogSystem::info:
                                level_string = "i";
                                skip = !show_log_info;
                                break;
                            case LogSystem::warning:
                                level_string = "w";
                                skip = !show_log_warning;
                                break;
                            case LogSystem::error:
                                level_string = "e";
                                skip = !show_log_error;
                                break;
                            case LogSystem::fatal:
                                level_string = "f";
                                break;
                            default:
                                level_string = "u";
                                break;
                        }

                        if (skip == false) {
                            if (stage == 0) {
                                edit_log_field_size += snprintf(NULL, 0, "[%s][%2s][%-17s]:%-4u: %s", level_string, rhlr.prefix, rhlr.filename, rhlr.row, rhlr.data);
                            } else {
                                current_pos += snprintf((char*)(edit_log_field + current_pos), edit_log_field_size - current_pos, "[%s][%2s][%-17s]:%-4u: %s", level_string, rhlr.prefix, rhlr.filename, rhlr.row, rhlr.data);
                                assert(current_pos < edit_log_field_size);
                            }
                        }
                    }
                }
                ram_handler.Unlock();
            } else {
                if (edit_log_field) {
                    OG_FREE(edit_log_field);
                    edit_log_field = NULL;
                }
            }
        }

        size_t selection_begin;
        size_t selection_length;
        if (edit_log_selection[1] > edit_log_selection[0]) {
            selection_length = edit_log_selection[1] - edit_log_selection[0];
            selection_begin = edit_log_selection[0];
        } else {
            selection_length = edit_log_selection[0] - edit_log_selection[1];
            selection_begin = edit_log_selection[1];
        }

        if (do_pause) {
            ImGui::SameLine();
            if (ImGui::Button("Copy Selection")) {
                if (selection_length > 0) {
                    uint8_t* selection = (uint8_t*)OG_MALLOC(selection_length + 1);
                    if (selection_begin + selection_length < edit_log_field_size) {
                        memcpy(selection, edit_log_field + selection_begin, selection_length);
                        selection[selection_length] = '\0';
                        SDL_SetClipboardText((char*)selection);
                    }
                    OG_FREE(selection);
                } else {
                    uint8_t* selection = (uint8_t*)OG_MALLOC(edit_log_field_size);
                    memcpy(selection, edit_log_field, edit_log_field_size);
                    selection[edit_log_field_size - 1] = '\0';
                    SDL_SetClipboardText((char*)selection);
                    OG_FREE(selection);
                }
            }
        }
        ImGui::EndChild();

        w_flags = 0;

        if (show_log_scrollbar == false) {
            w_flags |= ImGuiWindowFlags_NoScrollbar;
        }

        ImGui::BeginChild("Log body", ImVec2(0, 0), !do_lock, w_flags);
        if (do_pause) {
            ImGui::InputTextMultiline("log_edit", (char*)edit_log_field, edit_log_field_size, ImVec2(-1, -1), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CallbackAlways, LogWindowCallback, &edit_log_selection);
        } else {
            ram_handler.Lock();
            uint32_t count = ram_handler.GetCount();
            for (uint32_t i = 0; i < count; i++) {
                RamHandlerLogRow rhlr = ram_handler.GetLog(i);

                const char* level_string = "";
                bool skip = false;
                bool pushed_style = false;

                ImVec4 text_color = ImGui::GetStyle().Colors[ImGuiCol_Text];
                switch (rhlr.type) {
                    case LogSystem::spam:
                        level_string = "s";
                        skip = !show_log_spam;
                        break;
                    case LogSystem::debug:
                        level_string = "d";
                        skip = !show_log_debug;
                        break;
                    case LogSystem::info:
                        level_string = "i";
                        skip = !show_log_info;
                        text_color = ImVec4(144.0f / 255.0f, 192.0f / 255.0f, 34.0f / 255.0f, 1.0f);
                        break;
                    case LogSystem::warning:
                        level_string = "w";
                        skip = !show_log_warning;
                        text_color = ImVec4(229.0f / 255.0f, 220.0f / 255.0f, 89.0f / 255.0f, 1.0f);
                        break;
                    case LogSystem::error:
                        level_string = "e";
                        skip = !show_log_error;
                        text_color = ImVec4(1.0f, 107.0f / 255.0f, 104.0f / 255.0f, 1.0f);
                        break;
                    case LogSystem::fatal:
                        level_string = "f";
                        text_color = ImVec4(1.0f, 107.0f / 255.0f, 104.0f / 255.0f, 1.0f);
                        break;
                    default:
                        level_string = "u";
                        break;
                }

                if (skip == false) {
                    ImGui::PushStyleColor(ImGuiCol_Text, text_color);
                    ImGui::Text("[%s][%2s][%-17s]:%-4u: %s", level_string, rhlr.prefix, rhlr.filename, rhlr.row, rhlr.data);
                    ImGui::PopStyleColor();
                }
            }
            ram_handler.Unlock();
        }

        if (show_log_scrollbar == false) {
            ImGui::SetScrollY(ImGui::GetScrollMaxY());
        }

        ImGui::EndChild();

        ImGui::End();

        if (do_pop_style) {
            ImGui::PopStyleColor();
        }
    }

    if (show_warnings) {
        ImGuiWindowFlags w_flags = 0;
        ImGui::Begin("Warnings", &show_warnings, w_flags);

        if (scenegraph) {
            const int kBufSize = 1024;
            char* buf_sanityerror = (char*)alloc.stack.Alloc(kBufSize);
            char* buf_dispname = (char*)alloc.stack.Alloc(kBufSize);
            for (auto& sanity : scenegraph->sanity_list) {
                if (sanity.Valid() && sanity.Ok() == false) {
                    Object* obj = scenegraph->GetObjectFromID(sanity.GetID());
                    if (obj) {
                        obj->GetDisplayName(buf_dispname, kBufSize);

                        ImGuiTreeNodeFlags node_flags = 0UL;  // ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
                        if (obj->Selected()) {
                            node_flags |= ImGuiTreeNodeFlags_Selected;
                        }

                        node_flags |= ImGuiTreeNodeFlags_Leaf;

                        ImGui::PushID(obj->GetID());
                        vec4 color = GetObjColor(obj);
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(color[0], color[1], color[2], color[3]));

                        for (unsigned k = 0; k < sanity.GetErrorCount(); k++) {
                            sanity.GetError(k, buf_sanityerror, kBufSize);
                            bool node_open = ImGui::TreeNodeEx("", node_flags, "%d (%s): %s", sanity.GetID(), buf_dispname, buf_sanityerror);

                            if (node_open) {
                                ImGui::TreePop();
                            }
                        }

                        ImGui::PopStyleColor(1);
                        ImGui::PopID();
                        if (ImGui::IsItemClicked()) {
                            scenegraph->UnselectAll();
                            obj->Select(true);
                        }
                    }
                }
            }
            alloc.stack.Free(buf_dispname);
            alloc.stack.Free(buf_sanityerror);
        }
        ImGui::End();
    }

    static ASContext* active_asdebugger_context = NULL;
    if (show_asdebugger_contexts) {
        ImGui::SetNextWindowSize(ImVec2(640.0f, 480.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("AS debugger contexts", &show_asdebugger_contexts);

        static char debug_buffer[256] = "somefile.as:123";
        ImGui::InputText("##Breakpoint", debug_buffer, 256);
        ImGui::SameLine();
        if (ImGui::Button("Set breakpoint")) {
            for (size_t i = 0; i < strlen(debug_buffer); ++i) {
                if (debug_buffer[i] == ':') {
                    debug_buffer[i] = '\0';
                    int line = atoi(&debug_buffer[i + 1]);
                    ASDebugger::AddGlobalBreakpoint(debug_buffer, line);
                }
            }
        }

        ImGui::Text("File breakpoints");
        const std::vector<std::pair<std::string, int>> global_breakpoints = ASDebugger::GetGlobalBreakpoints();
        for (const auto& global_breakpoint : global_breakpoints) {
            char buffer[256];
            sprintf(buffer, "%s:%d", global_breakpoint.first.c_str(), global_breakpoint.second);
            if (ImGui::Button(buffer)) {
                ASDebugger::RemoveGlobalBreakpoint(global_breakpoint.first.c_str(), global_breakpoint.second);
            }
        }

        ImGui::Separator();
        ImGui::Text("Active contexts");
        const std::vector<ASContext*>& active_contexts = ASDebugger::GetActiveContexts();

        for (size_t i = 0; i < active_contexts.size(); ++i) {
            ImGui::PushID((int)i);
            if (ImGui::Button(active_contexts[i]->current_script.GetOriginalPath())) {
                active_asdebugger_context = active_contexts[i];
            }
            ImGui::PopID();
        }

        ImGui::End();
    }

    if (active_asdebugger_context != NULL) {
        const std::vector<ASContext*>& active_contexts = ASDebugger::GetActiveContexts();
        // Contexts may disappear whenever
        if (std::find(active_contexts.begin(), active_contexts.end(), active_asdebugger_context) == active_contexts.end()) {
            active_asdebugger_context = NULL;
        } else {
            bool is_open = true;
            ImGui::SetNextWindowSize(ImVec2(1024.0f, 768.0f), ImGuiCond_FirstUseEver);
            ImGui::Begin("AS debugger preview", &is_open);

            const ScriptFile* script = ScriptFileUtil::GetScriptFile(active_asdebugger_context->current_script);
            const char* file_name = script->file_path.GetFullPath();
            size_t last_slash = 0;
            for (size_t i = 0; i < std::strlen(file_name); ++i) {
                if (file_name[i] == '\\' || file_name[i] == '/')
                    last_slash = i;
            }
            file_name += last_slash + 1;

            ImGui::Text("%s", file_name);

            const std::string& script_source = script->unexpanded_contents;
            int start_index = 0;
            int end_index = 0;

            ImGui::BeginChild("ASSourceMain", ImVec2(0, 0), false, 0);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::Columns(2);

            int line_nr = 1;

            const int MAX_LINE_LENGTH = 255;
            char line[MAX_LINE_LENGTH + 1];

            const std::vector<int>* breakpoints = active_asdebugger_context->dbg.GetBreakpoints(file_name);

            ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
            while (end_index != (int)script_source.size()) {
                end_index = (int)std::min(script_source.find('\n', start_index), script_source.size());

                int copy_length = std::min(MAX_LINE_LENGTH, end_index - start_index);
                std::memcpy(line, script_source.c_str() + start_index, copy_length);
                line[copy_length] = '\0';
                start_index = end_index + 1;

                bool is_breakpoint = false;
                if (breakpoints != NULL) {
                    for (int breakpoint : *breakpoints) {
                        if (breakpoint == line_nr) {
                            is_breakpoint = true;
                            break;
                        }
                    }
                }

                if (is_breakpoint)
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.0f, 0.0f, 0.5f));

                ImGui::Text("%d", line_nr);
                ImGui::NextColumn();
                ImGui::PushID(line_nr);
                ImGui::SetColumnOffset(-1, 36);
                if (ImGui::ButtonEx(line, ImVec2(ImGui::GetWindowWidth() - 36, 0), ImGuiButtonFlags_AlignTextBaseLine)) {
                    active_asdebugger_context->dbg.ToggleBreakpoint(file_name, line_nr);
                }
                ImGui::NextColumn();
                ImGui::PopID();
                if (is_breakpoint)
                    ImGui::PopStyleColor();

                ++line_nr;
            }
            ImGui::PopStyleVar(2);

            ImGui::PopStyleColor();
            ImGui::EndChild();
            ImGui::End();

            if (!is_open)
                active_asdebugger_context = NULL;
        }
    }

    if (show_asprofiler) {
        ImGui::SetNextWindowSize(ImVec2(640.0f, 480.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("AS profiler", &show_asprofiler);

        ImGui::Separator();
        ImGui::Text("Active contexts");
        const std::vector<ASContext*>& active_contexts = ASProfiler::GetActiveContexts();

        for (size_t i = 0; i < active_contexts.size(); ++i) {
            ImGui::PushID((int)i);
            if (ImGui::Checkbox(active_contexts[i]->current_script.GetOriginalPath(), &active_contexts[i]->profiler.enabled)) {
            }
            ImGui::PopID();
        }

        ImGui::Separator();
        ImGui::Text("Times in microseconds");
        ImGui::Text("Showing history over %.2f seconds", ASProfiler::GetMaxWindowSize() * ASProfiler::GetMaxTimeHistorySize() / 120.0f);
        ImGui::Text("Each time point is calculated by taking the max time over %d frames", ASProfiler::GetMaxWindowSize());
        ImGui::Text("Times may appear to be lagging because not all functions are called each frame");

        for (auto active_context : active_contexts) {
            if (active_context->profiler.enabled)
                active_context->profiler.Draw();
        }

        ImGui::End();
    }

    if (show_graphics_debug_disable_menu) {
        ImGui::SetNextWindowSize(ImVec2(640.0f, 480.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Graphics Debug Disable", &show_graphics_debug_disable_menu);

        ImGui::Text("Warning: Messing with these will likely crash your game.");
        ImGui::Text("These settings are not saved across runs.");

        if (ImGui::Button("Enable All")) {
            g_debug_runtime_disable_blood_surface_pre_draw = false;
            g_debug_runtime_disable_debug_draw = false;
            g_debug_runtime_disable_debug_ribbon_draw = false;
            g_debug_runtime_disable_decal_object_draw = false;
            g_debug_runtime_disable_decal_object_pre_draw_frame = false;
            g_debug_runtime_disable_detail_object_surface_draw = false;
            g_debug_runtime_disable_detail_object_surface_pre_draw = false;
            g_debug_runtime_disable_dynamic_light_object_draw = false;
            g_debug_runtime_disable_env_object_draw = false;
            g_debug_runtime_disable_env_object_draw_depth_map = false;
            g_debug_runtime_disable_env_object_draw_detail_object_instances = false;
            g_debug_runtime_disable_env_object_draw_instances = false;
            g_debug_runtime_disable_env_object_draw_instances_transparent = false;
            g_debug_runtime_disable_env_object_pre_draw_camera = false;
            g_debug_runtime_disable_flares_draw = false;
            g_debug_runtime_disable_gpu_particle_field_draw = false;
            g_debug_runtime_disable_group_pre_draw_camera = false;
            g_debug_runtime_disable_group_pre_draw_frame = false;
            g_debug_runtime_disable_hotspot_draw = false;
            g_debug_runtime_disable_hotspot_pre_draw_frame = false;
            g_debug_runtime_disable_item_object_draw = false;
            g_debug_runtime_disable_item_object_draw_depth_map = false;
            g_debug_runtime_disable_item_object_pre_draw_frame = false;
            g_debug_runtime_disable_morph_target_pre_draw_camera = false;
            g_debug_runtime_disable_movement_object_draw = false;
            g_debug_runtime_disable_movement_object_draw_depth_map = false;
            g_debug_runtime_disable_movement_object_pre_draw_camera = false;
            g_debug_runtime_disable_movement_object_pre_draw_frame = false;
            g_debug_runtime_disable_navmesh_connection_object_draw = false;
            g_debug_runtime_disable_navmesh_hint_object_draw = false;
            g_debug_runtime_disable_particle_draw = false;
            g_debug_runtime_disable_particle_system_draw = false;
            g_debug_runtime_disable_pathpoint_object_draw = false;
            g_debug_runtime_disable_placeholder_object_draw = false;
            g_debug_runtime_disable_reflection_capture_object_draw = false;
            g_debug_runtime_disable_rigged_object_draw = false;
            g_debug_runtime_disable_rigged_object_pre_draw_camera = false;
            g_debug_runtime_disable_rigged_object_pre_draw_frame = false;
            g_debug_runtime_disable_scene_graph_draw = false;
            g_debug_runtime_disable_scene_graph_draw_depth_map = false;
            g_debug_runtime_disable_scene_graph_prepare_lights_and_decals = false;
            g_debug_runtime_disable_sky_draw = false;
            g_debug_runtime_disable_terrain_object_draw_depth_map = false;
            g_debug_runtime_disable_terrain_object_draw_terrain = false;
            g_debug_runtime_disable_terrain_object_pre_draw_camera = false;
        }

        if (ImGui::Button("Disable Drawing Objects")) {
            g_debug_runtime_disable_debug_draw = true;
            g_debug_runtime_disable_debug_ribbon_draw = true;
            g_debug_runtime_disable_decal_object_draw = true;
            g_debug_runtime_disable_detail_object_surface_draw = true;
            g_debug_runtime_disable_dynamic_light_object_draw = true;
            g_debug_runtime_disable_env_object_draw = true;
            g_debug_runtime_disable_env_object_draw_depth_map = true;
            g_debug_runtime_disable_env_object_draw_detail_object_instances = true;
            g_debug_runtime_disable_env_object_draw_instances = true;
            g_debug_runtime_disable_env_object_draw_instances_transparent = true;
            g_debug_runtime_disable_flares_draw = true;
            g_debug_runtime_disable_gpu_particle_field_draw = true;
            g_debug_runtime_disable_hotspot_draw = true;
            g_debug_runtime_disable_item_object_draw = true;
            g_debug_runtime_disable_item_object_draw_depth_map = true;
            g_debug_runtime_disable_movement_object_draw = true;
            g_debug_runtime_disable_movement_object_draw_depth_map = true;
            g_debug_runtime_disable_navmesh_connection_object_draw = true;
            g_debug_runtime_disable_navmesh_hint_object_draw = true;
            g_debug_runtime_disable_particle_draw = true;
            g_debug_runtime_disable_particle_system_draw = true;
            g_debug_runtime_disable_pathpoint_object_draw = true;
            g_debug_runtime_disable_placeholder_object_draw = true;
            g_debug_runtime_disable_reflection_capture_object_draw = true;
            g_debug_runtime_disable_rigged_object_draw = true;
            g_debug_runtime_disable_sky_draw = true;
            g_debug_runtime_disable_terrain_object_draw_depth_map = true;
            g_debug_runtime_disable_terrain_object_draw_terrain = true;
        }

        if (ImGui::Button("Disable All")) {
            g_debug_runtime_disable_blood_surface_pre_draw = true;
            g_debug_runtime_disable_debug_draw = true;
            g_debug_runtime_disable_debug_ribbon_draw = true;
            g_debug_runtime_disable_decal_object_draw = true;
            g_debug_runtime_disable_decal_object_pre_draw_frame = true;
            g_debug_runtime_disable_detail_object_surface_draw = true;
            g_debug_runtime_disable_detail_object_surface_pre_draw = true;
            g_debug_runtime_disable_dynamic_light_object_draw = true;
            g_debug_runtime_disable_env_object_draw = true;
            g_debug_runtime_disable_env_object_draw_depth_map = true;
            g_debug_runtime_disable_env_object_draw_detail_object_instances = true;
            g_debug_runtime_disable_env_object_draw_instances = true;
            g_debug_runtime_disable_env_object_draw_instances_transparent = true;
            g_debug_runtime_disable_env_object_pre_draw_camera = true;
            g_debug_runtime_disable_flares_draw = true;
            g_debug_runtime_disable_gpu_particle_field_draw = true;
            g_debug_runtime_disable_group_pre_draw_camera = true;
            g_debug_runtime_disable_group_pre_draw_frame = true;
            g_debug_runtime_disable_hotspot_draw = true;
            g_debug_runtime_disable_hotspot_pre_draw_frame = true;
            g_debug_runtime_disable_item_object_draw = true;
            g_debug_runtime_disable_item_object_draw_depth_map = true;
            g_debug_runtime_disable_item_object_pre_draw_frame = true;
            g_debug_runtime_disable_morph_target_pre_draw_camera = true;
            g_debug_runtime_disable_movement_object_draw = true;
            g_debug_runtime_disable_movement_object_draw_depth_map = true;
            g_debug_runtime_disable_movement_object_pre_draw_camera = true;
            g_debug_runtime_disable_movement_object_pre_draw_frame = true;
            g_debug_runtime_disable_navmesh_connection_object_draw = true;
            g_debug_runtime_disable_navmesh_hint_object_draw = true;
            g_debug_runtime_disable_particle_draw = true;
            g_debug_runtime_disable_particle_system_draw = true;
            g_debug_runtime_disable_pathpoint_object_draw = true;
            g_debug_runtime_disable_placeholder_object_draw = true;
            g_debug_runtime_disable_reflection_capture_object_draw = true;
            g_debug_runtime_disable_rigged_object_draw = true;
            g_debug_runtime_disable_rigged_object_pre_draw_camera = true;
            g_debug_runtime_disable_rigged_object_pre_draw_frame = true;
            g_debug_runtime_disable_scene_graph_draw = true;
            g_debug_runtime_disable_scene_graph_draw_depth_map = true;
            g_debug_runtime_disable_scene_graph_prepare_lights_and_decals = true;
            g_debug_runtime_disable_sky_draw = true;
            g_debug_runtime_disable_terrain_object_draw_depth_map = true;
            g_debug_runtime_disable_terrain_object_draw_terrain = true;
            g_debug_runtime_disable_terrain_object_pre_draw_camera = true;
        }

        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Individual options");
        ImGui::BeginChild("Scrolling");

        ImGui::Checkbox("Disable Scene Graph Draw", &g_debug_runtime_disable_scene_graph_draw);
        ImGui::Checkbox("Disable Scene Graph Draw Depth Map", &g_debug_runtime_disable_scene_graph_draw_depth_map);
        ImGui::Checkbox("Disable Scene Graph Prepare Lights And Decals", &g_debug_runtime_disable_scene_graph_prepare_lights_and_decals);

        ImGui::Separator();

        ImGui::Checkbox("Disable Debug Draw", &g_debug_runtime_disable_debug_draw);
        ImGui::Checkbox("Disable Debug Ribbon Draw", &g_debug_runtime_disable_debug_ribbon_draw);
        ImGui::Checkbox("Disable Decal Object Draw", &g_debug_runtime_disable_decal_object_draw);
        ImGui::Checkbox("Disable Detail Object Surface Draw", &g_debug_runtime_disable_detail_object_surface_draw);
        ImGui::Checkbox("Disable Dynamic Light Object Draw", &g_debug_runtime_disable_dynamic_light_object_draw);
        ImGui::Checkbox("Disable Env Object Draw", &g_debug_runtime_disable_env_object_draw);
        ImGui::Checkbox("Disable Env Object Draw Depth Map", &g_debug_runtime_disable_env_object_draw_depth_map);
        ImGui::Checkbox("Disable Env Object Draw Detail Object Instances", &g_debug_runtime_disable_env_object_draw_detail_object_instances);
        ImGui::Checkbox("Disable Env Object Draw Instances", &g_debug_runtime_disable_env_object_draw_instances);
        ImGui::Checkbox("Disable Env Object Draw Instances Transparent", &g_debug_runtime_disable_env_object_draw_instances_transparent);
        ImGui::Checkbox("Disable Flares Draw", &g_debug_runtime_disable_flares_draw);
        ImGui::Checkbox("Disable Gpu Particle Field Draw", &g_debug_runtime_disable_gpu_particle_field_draw);
        ImGui::Checkbox("Disable Hotspot Draw", &g_debug_runtime_disable_hotspot_draw);
        ImGui::Checkbox("Disable Item Object Draw", &g_debug_runtime_disable_item_object_draw);
        ImGui::Checkbox("Disable Item Object Draw Depth Map", &g_debug_runtime_disable_item_object_draw_depth_map);
        ImGui::Checkbox("Disable Movement Object Draw", &g_debug_runtime_disable_movement_object_draw);
        ImGui::Checkbox("Disable Movement Object Draw Depth Map", &g_debug_runtime_disable_movement_object_draw_depth_map);
        ImGui::Checkbox("Disable Navmesh Connection Object Draw", &g_debug_runtime_disable_navmesh_connection_object_draw);
        ImGui::Checkbox("Disable Navmesh Hint Object Draw", &g_debug_runtime_disable_navmesh_hint_object_draw);
        ImGui::Checkbox("Disable Particle Draw", &g_debug_runtime_disable_particle_draw);
        ImGui::Checkbox("Disable Particle System Draw", &g_debug_runtime_disable_particle_system_draw);
        ImGui::Checkbox("Disable Pathpoint Object Draw", &g_debug_runtime_disable_pathpoint_object_draw);
        ImGui::Checkbox("Disable Placeholder Object Draw", &g_debug_runtime_disable_placeholder_object_draw);
        ImGui::Checkbox("Disable Reflection Capture Object Draw", &g_debug_runtime_disable_reflection_capture_object_draw);
        ImGui::Checkbox("Disable Rigged Object Draw", &g_debug_runtime_disable_rigged_object_draw);
        ImGui::Checkbox("Disable Sky Draw", &g_debug_runtime_disable_sky_draw);
        ImGui::Checkbox("Disable Terrain Object Draw Depth Map", &g_debug_runtime_disable_terrain_object_draw_depth_map);
        ImGui::Checkbox("Disable Terrain Object Draw Terrain", &g_debug_runtime_disable_terrain_object_draw_terrain);

        ImGui::Separator();

        ImGui::Checkbox("Disable Blood Surface Pre-Draw", &g_debug_runtime_disable_blood_surface_pre_draw);
        ImGui::Checkbox("Disable Decal Object Pre-Draw Frame", &g_debug_runtime_disable_decal_object_pre_draw_frame);
        ImGui::Checkbox("Disable Detail Object Surface Pre-Draw", &g_debug_runtime_disable_detail_object_surface_pre_draw);
        ImGui::Checkbox("Disable Env Object Pre-Draw Camera", &g_debug_runtime_disable_env_object_pre_draw_camera);
        ImGui::Checkbox("Disable Group Pre-Draw Camera", &g_debug_runtime_disable_group_pre_draw_camera);
        ImGui::Checkbox("Disable Group Pre-Draw Frame", &g_debug_runtime_disable_group_pre_draw_frame);
        ImGui::Checkbox("Disable Hotspot Pre-Draw Frame", &g_debug_runtime_disable_hotspot_pre_draw_frame);
        ImGui::Checkbox("Disable Item Object Pre-Draw Frame", &g_debug_runtime_disable_item_object_pre_draw_frame);
        ImGui::Checkbox("Disable Morph Target Pre-Draw Camera", &g_debug_runtime_disable_morph_target_pre_draw_camera);
        ImGui::Checkbox("Disable Movement Object Pre-Draw Camera", &g_debug_runtime_disable_movement_object_pre_draw_camera);
        ImGui::Checkbox("Disable Movement Object Pre-Draw Frame", &g_debug_runtime_disable_movement_object_pre_draw_frame);
        ImGui::Checkbox("Disable Rigged Object Pre-Draw Camera", &g_debug_runtime_disable_rigged_object_pre_draw_camera);
        ImGui::Checkbox("Disable Rigged Object Pre-Draw Frame", &g_debug_runtime_disable_rigged_object_pre_draw_frame);
        ImGui::Checkbox("Disable Terrain Object Pre-Draw Camera", &g_debug_runtime_disable_terrain_object_pre_draw_camera);

        ImGui::EndChild();

        ImGui::End();
    }

    if (show_sound) {
        ImGui::SetNextWindowSize(ImVec2(400.0f, 300.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Sound", &show_sound);

        ImGui::End();
    }

    if (show_input_debug) {
        ImGui::SetNextWindowSize(ImVec2(400.0f, 300.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Input Debug", &show_input_debug);

        Input::JoystickMap& open_joysticks = Input::Instance()->GetOpenJoysticks();

        std::vector<MovementObject*> controlled_movementobjects;
        std::vector<MovementObject*> controllable_movement_objects;

        if (scenegraph != nullptr) {
            controlled_movementobjects = scenegraph->GetControlledMovementObjects();
            controllable_movement_objects = scenegraph->GetControllableMovementObjects();
        }

        auto RenderMovementObject = [&](MovementObject* mo) {
            ImGui::Text("ID %d", mo->GetID());
            ImGui::Text("Name: %s", mo->name.c_str());
            ImGui::Text("Obj File: %s", mo->obj_file.c_str());
            ImGui::Text("Actor File: %s", mo->actor_file_ref->character.c_str());
            ImGui::Text("Controlled: %s", mo->controlled ? "true" : "false");
        };

        auto RenderControlledMovementObject = [&](int controller_id) -> void {
            bool found = false;

            ImGui::PushID(controller_id);
            ImGui::BeginGroup();
            ImGui::Text("Avatar");
            ImGui::Indent(20.0f);
            for (MovementObject* mo : controlled_movementobjects) {
                if (mo->controller_id == controller_id) {
                    RenderMovementObject(mo);
                    found = true;
                }
            }

            if (found == false) {
                ImGui::Text("No Avatar Assigned");
            }

            ImGui::Unindent(20.0f);
            ImGui::EndGroup();
            ImGui::PopID();
        };

        ImGui::Text("Input Sources");
        ImGui::Separator();

        ImGui::Indent(20.0f);
        ImGui::Text("Keyboard");
        ImGui::Text("player_input: 0");
        RenderControlledMovementObject(0);
        ImGui::Separator();

        int joystick_subid = 0;
        ImGui::PushID("joystick_inputs");
        for (auto joystick : open_joysticks) {
            ImGui::PushID(joystick_subid);
            SDL_Joystick* sdl_joystick = joystick.second->sdl_joystick;
            ImGui::Text("Gamepad #%d: \"%s\"", joystick.first, SDL_JoystickName(sdl_joystick));
            ImGui::Text("player_input: %d", joystick.second->player_input);
            // ImGui::BeginCombo("Change Avatar");
            // for(MovementObject* mo : controllable_movement_objects) {
            //     //ImGui::Selectable
            // }
            // ImGui::EndCombo();
            RenderControlledMovementObject(joystick.second->player_input);
            ImGui::Separator();
            ImGui::PopID();
        }
        ImGui::PopID();
        ImGui::Unindent(20.0f);

        if (online->IsActive()) {
            ImGui::PushID("virtual_sources");
            ImGui::Text("Virtual Sources");

            ImGui::Indent(20.0f);

            int mp_subid = 0;
            for (auto& it : online->online_session->player_states) {
                if (it.second.controller_id != -1) {
                    ImGui::PushID(mp_subid++);
                    ImGui::Text("player_state: %d", it.first);
                    ImGui::Text("player_input: %d", it.second.controller_id);

                    RenderControlledMovementObject(it.second.controller_id);
                    ImGui::Separator();
                    ImGui::PopID();
                }
            }
            ImGui::PopID();
            ImGui::Unindent(20.0f);
        }

        ImGui::Text("Player Movement Objects");

        ImGui::Indent(20.0f);
        for (MovementObject* mo : controllable_movement_objects) {
            RenderMovementObject(mo);
            ImGui::Separator();
        }

        ImGui::Unindent(20.0f);
        ImGui::End();
    }

    if (show_pose) {
        std::string animationPath = "";
        std::vector<ModInstance*> all_mods = ModLoading::Instance().GetAllMods();

        ImGui::Begin("Test pose", &show_pose);
        for (const auto& m : all_mods) {
            for (const auto& p : m->poses) {
                if (ImGui::Button(p.name)) {
                    animationPath = p.path;
                }
            }
        }
        ImGui::End();
        SceneGraph* scene_graph = Engine::Instance()->GetSceneGraph();
        if (scene_graph) {
            std::vector<MovementObject*> movement_objects = scene_graph->GetControllableMovementObjects();
            if (!movement_objects.empty() && !animationPath.empty()) {
                MovementObject* player1 = movement_objects.front();
                for (int i = 1; i < movement_objects.size(); i++) {
                    if (movement_objects[i]->controller_id < player1->controller_id) {
                        player1 = movement_objects[i];
                    }
                }
                player1->StartPoseAnimation(animationPath);
            }
        }
    }

    if (show_select_script) {
        ImGui::Begin("Select script", &show_select_script);

        std::vector<std::string> script_paths;
        std::string script_path;
        for (int i = 0; i < vanilla_data_paths.num_paths; i++) {
            script_path = vanilla_data_paths.paths[i] + script_dir_path;
            if (FindFilePath(script_path, kAnyPath, false).isValid()) {
                script_paths.push_back(std::move(script_path));
            }
        }
        for (int i = 0; i < mod_paths.num_paths; i++) {
            script_path = mod_paths.paths[i] + script_dir_path;
            if (FindFilePath(script_path, kAnyPath, false).isValid()) {
                script_paths.push_back(std::move(script_path));
            }
        }

        for (const auto& p : script_paths) {
            std::vector<std::string> script_names;
            GenerateManifest(p.c_str(), script_names);
            for (const auto& n : script_names) {
                if (ImGui::MenuItem(n.c_str())) {
                    show_script = true;
                    preview_script_name = n;
                }
            }
        }
        ImGui::End();
    }

    if (show_script && !preview_script_name.empty()) {
        static std::string prev_script_path = "";
        static std::vector<unsigned char> content;

        Path path = FindFilePath(script_dir_path + preview_script_name, kDataPaths | kModPaths, false);
        if (path.isValid()) {
            if (prev_script_path != path.GetFullPathStr()) {
                prev_script_path = path.GetFullPathStr();
                content = readFile((prev_script_path.c_str()));
                content.push_back('\0');
            }
            ImGui::Begin(preview_script_name.c_str(), &show_script, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);
            ImGui::TextEx(reinterpret_cast<const char*>(content.data()));
            ImGui::End();
        } else {
            show_script = false;
        }
    }
}

void RenderImGui() {
    ImGui::Render();
}

bool WantMouseImGui() {
    return ImGui::GetIO().WantCaptureMouse;
}

bool WantKeyboardImGui() {
    return ImGui::GetIO().WantCaptureKeyboard;
}
