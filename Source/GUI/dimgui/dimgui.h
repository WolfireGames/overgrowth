//-----------------------------------------------------------------------------
//           Name: dimgui.h
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

#include <Main/engine.h>
#include <Main/scenegraph.h>

#include <SDL.h>

class ImGuiTextEditCallbackData;

extern bool show_test_window;
extern bool show_debug_text;
extern bool show_scenegraph;
extern bool select_scenegraph_search;
extern bool show_selected;
extern bool show_color_picker;
extern bool show_mod_menu;
extern bool show_sound;
extern bool show_performance;
extern bool show_log;
extern bool show_state;
extern bool show_save;
extern bool show_warnings;
extern bool show_asdebugger_contexts;
extern bool show_asprofiler;
extern bool show_mp_debug;
extern bool show_mp_info;
extern bool show_mp_settings;
extern bool show_input_debug;
extern bool break_on_script_change;
extern bool show_load_item_search;

void InitImGui();
void ReloadImGui();
void DisposeImGui();

void UpdateImGui();
void ProcessEventImGui(SDL_Event* event);
void DrawImGuiCameraPreview(Engine* engine, SceneGraph* scenegraph, Graphics* graphics);
void DrawImGui(Graphics* graphics, SceneGraph* scenegraph, GUI* gui, AssetManager* assetmanager, Engine* engine, bool cursor_visible);
bool WantMouseImGui();
bool WantKeyboardImGui();
void RenderImGui();
