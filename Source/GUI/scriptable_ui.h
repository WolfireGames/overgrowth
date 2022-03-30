//-----------------------------------------------------------------------------
//           Name: scriptable_ui.h
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

#include <Internal/modloading.h>
#include <Internal/path.h>

#include <Utility/disallow_copy_and_assign.h>
#include <Scripting/angelscript/ascontext.h>

#include <string>
#include <memory>

class ASContext;
struct HUDImages;
struct ASData;
class SaveFile;

class ScriptableUI : public ModLoadingCallback {
public:
    typedef void (*NotificationCallback)(void*, const std::string&);
private:
    // These are pointers to minimize header includes
    HUDImages *hud_images;
    void Dispose();
    void* callback_instance_;
    bool to_delete_;
    bool mod_activation_change_;
    NotificationCallback notification_callback_;
    
    int currentWindowDims[2];

    struct {
        ASFunctionHandle initialize;
        ASFunctionHandle can_go_back;
        ASFunctionHandle dispose;
        ASFunctionHandle draw_gui;
        ASFunctionHandle update;

        ASFunctionHandle mod_activation_reload;
        ASFunctionHandle resize;
        ASFunctionHandle script_reloaded;

        ASFunctionHandle queue_basic_popup;
    } as_funcs;
public:
	ASContext *as_context;
    ScriptableUI(void *callback_instance, NotificationCallback notification_callback)
        :as_context(NULL), hud_images(NULL),
        callback_instance_(callback_instance), to_delete_(false),
        notification_callback_(notification_callback) 
    {}
    ~ScriptableUI() {
        Dispose();
    };
    
    bool CanGoBack();
    void ScheduleDelete();
    bool IsDeleteScheduled();
    void Initialize(const Path& script_path, const ASData& data, bool debug_break = false);
    void Update();
    void Draw();
    void QueueBasicPopup(const std::string& title, const std::string& body);
    void SendCallback(const std::string& message);

    void Reload(bool force=false);

    virtual void ModActivationChange( const ModInstance* mod );
    
    DISALLOW_COPY_AND_ASSIGN(ScriptableUI);
};
