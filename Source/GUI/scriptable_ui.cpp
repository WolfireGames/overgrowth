//-----------------------------------------------------------------------------
//           Name: scriptable_ui.cpp
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
#include "scriptable_ui.h"

#include <GUI/IMUI/imui.h>
#include <GUI/IMUI/imui_script.h>

#include <Scripting/angelscript/asfuncs.h>

#include <Graphics/hudimage.h>
#include <Graphics/text.h>

#include <Game/levelset_script.h>
#include <Game/savefile.h>
#include <Game/savefile_script.h>

#include <Internal/levelxml_script.h>
#include <Internal/filesystem.h>
#include <Internal/common.h>
#include <Internal/profiler.h>
#include <Internal/locale.h>

#include <Utility/assert.h>
#include <Main/engine.h>

#include <cassert>

namespace {
void AttachScriptableUI(ASContext* as_context, ScriptableUI* sui) {
    as_context->RegisterObjectType("ScriptableUI", 0, asOBJ_REF | asOBJ_NOHANDLE);
    as_context->RegisterObjectMethod("ScriptableUI", "void SendCallback(const string &in)", asMETHOD(ScriptableUI, SendCallback), asCALL_THISCALL);
    as_context->DocsCloseBrace();
    as_context->RegisterGlobalProperty("ScriptableUI this_ui", sui);
}
}  // Anonymous namespace

void ScriptableUI::Initialize(const Path& script_path, const ASData& as_data, bool debug_break) {
    mod_activation_change_ = false;
    LOG_ASSERT(!as_context && !hud_images);
    // Construct all the components
    as_context = new ASContext("scriptable_ui", as_data);
    if (debug_break)
        as_context->dbg.Break();
    hud_images = new HUDImages();
    // Attach necessary script functionality
    AttachTextCanvasTextureToASContext(as_context);
    AttachStringConvert(as_context);
    AttachIMUI(as_context);
    AttachScreenWidth(as_context);
    hud_images->AttachToContext(as_context);
    AttachLevelSet(as_context);
    AttachLevelXML(as_context);
    AttachUIQueries(as_context);
    AttachTokenIterator(as_context);
    AttachScriptableUI(as_context, this);
    AttachSimpleFile(as_context);
    AttachLocale(as_context);
    AttachIMGUI(as_context);
    AttachIMGUIModding(as_context);
    AttachInterlevelData(as_context);
    AttachEngine(as_context);
    AttachOnline(as_context);

    AttachSaveFile(as_context, &Engine::Instance()->save_file_);

    as_funcs.initialize = as_context->RegisterExpectedFunction("void Initialize()", true);
    as_funcs.can_go_back = as_context->RegisterExpectedFunction("bool CanGoBack()", true);
    as_funcs.dispose = as_context->RegisterExpectedFunction("void Dispose()", true);
    as_funcs.draw_gui = as_context->RegisterExpectedFunction("void DrawGUI()", true);
    as_funcs.update = as_context->RegisterExpectedFunction("void Update()", true);

    as_funcs.mod_activation_reload = as_context->RegisterExpectedFunction("void ModActivationReload()", false);
    as_funcs.resize = as_context->RegisterExpectedFunction("void Resize()", false);
    as_funcs.script_reloaded = as_context->RegisterExpectedFunction("void ScriptReloaded()", false);

    as_funcs.queue_basic_popup = as_context->RegisterExpectedFunction("void QueueBasicPopup(string title, string body)", false);

    // Get screen dimensions so we can detect when things change
    currentWindowDims[0] = Graphics::Instance()->window_dims[0];
    currentWindowDims[1] = Graphics::Instance()->window_dims[1];

    PROFILER_ENTER(g_profiler_ctx, "Exporting docs");
    char path[kPathSize];
    FormatString(path, kPathSize, "%sasscriptable_ui_docs.h", GetWritePath(CoreGameModID).c_str());
    as_context->ExportDocs(path);
    PROFILER_LEAVE(g_profiler_ctx);

    // Load script and run init function
    as_context->LoadScript(script_path);
    as_context->CallScriptFunction(as_funcs.initialize);

    ModLoading::Instance().RegisterCallback(this);
}

bool ScriptableUI::CanGoBack() {
    ASArg ret;
    asBYTE retvalue;
    ret.type = _as_bool;
    ret.data = &retvalue;
    ASArglist args;

    as_context->CallScriptFunction(as_funcs.can_go_back, &args, &ret);

    return retvalue;
}

void ScriptableUI::Dispose() {
    ModLoading::Instance().DeRegisterCallback(this);

    as_context->CallScriptFunction(as_funcs.dispose);

    delete as_context;
    delete hud_images;
}

void ScriptableUI::Draw() {
    as_context->CallScriptFunction(as_funcs.draw_gui);
    {
        PROFILER_GPU_ZONE(g_profiler_ctx, "hud_images->Draw()");
        hud_images->Draw();
    }
}

void ScriptableUI::Update() {
    if (mod_activation_change_) {
        LOGI << "ModActivation: ModActivationReload() called" << std::endl;
        as_context->CallScriptFunction(as_funcs.mod_activation_reload);
        mod_activation_change_ = false;
    }

    // See if the window size has changed
    if (currentWindowDims[0] != Graphics::Instance()->window_dims[0] ||
        currentWindowDims[1] != Graphics::Instance()->window_dims[1]) {
        currentWindowDims[0] = Graphics::Instance()->window_dims[0];
        currentWindowDims[1] = Graphics::Instance()->window_dims[1];
        as_context->CallScriptFunction(as_funcs.resize);
    }

    as_context->CallScriptFunction(as_funcs.update);
}

void ScriptableUI::QueueBasicPopup(const std::string& title, const std::string& body) {
    if (as_context->HasFunction(as_funcs.queue_basic_popup)) {
        ASArglist args;
        args.AddString(&const_cast<std::string&>(title));
        args.AddString(&const_cast<std::string&>(body));
        as_context->CallScriptFunction(as_funcs.queue_basic_popup, &args);
    } else {
        LOGW << "Attempted to send popup to scriptable UI, but ASContext has not specified void QueueBasicPopup(string title, string body)! This is usually included in menu_common.as" << std::endl;
        LOGW << "Failed to queue popup: \"" << title << "\" \"" << body << "\"" << std::endl;
    }
}

void ScriptableUI::SendCallback(const std::string& message) {
    LOG_ASSERT(notification_callback_ && callback_instance_);
    notification_callback_(callback_instance_, message);
}

void ScriptableUI::Reload(bool force) {
    if (as_context && as_context->Reload()) {
        as_context->CallScriptFunction(as_funcs.script_reloaded);
    } else if (force) {
        as_context->CallScriptFunction(as_funcs.script_reloaded);
    }
}

void ScriptableUI::ScheduleDelete() {
    to_delete_ = true;
}

bool ScriptableUI::IsDeleteScheduled() {
    return to_delete_;
}

void ScriptableUI::ModActivationChange(const ModInstance* mod) {
    mod_activation_change_ = true;
}
