//-----------------------------------------------------------------------------
//           Name: scriptablecampaign.cpp
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
#include "scriptablecampaign.h"

#include <Scripting/angelscript/ascontext.h>
#include <Scripting/angelscript/asfuncs.h>

#include <Game/levelset_script.h>
#include <Game/savefile.h>
#include <Game/savefile_script.h>
#include <Game/level.h>

#include <Internal/levelxml_script.h>
#include <Internal/filesystem.h>
#include <Internal/modloading.h>
#include <Internal/common.h>
#include <Internal/profiler.h>

#include <Graphics/text.h>
#include <Utility/assert.h>
#include <Main/engine.h>

ScriptableCampaign::ScriptableCampaign() {

}

ScriptableCampaign::~ScriptableCampaign() {

}

static void ASSendLevelMessage( const std::string& msg ) {
    SceneGraph* s = Engine::Instance()->GetSceneGraph();

    if(s) {
        s->level->Message(msg);
    } else {
        LOGW << "Sending message from campaign to level, but there is no scenegraph, going to the void, it was: " << msg << std::endl;
    }
}

void ScriptableCampaign::Initialize(Path script_path, std::string _campaign_id) {
    campaign_id = _campaign_id;

    ASData as_data;

    as_context = new ASContext("scriptable_campaign", as_data);

    as_context->RegisterGlobalFunction("void SendLevelMessage(const string& in msg)",                asFUNCTION(ASSendLevelMessage),                  asCALL_CDECL);

    AttachLevelSet(as_context);
    AttachLevelXML(as_context);
    AttachUIQueries(as_context);
    AttachTokenIterator(as_context);
    AttachSimpleFile(as_context);
    AttachInterlevelData(as_context);
    AttachEngine(as_context);
    AttachOnline(as_context);

    AttachSaveFile( as_context, &Engine::Instance()->save_file_ );

    as_funcs.init                   = as_context->RegisterExpectedFunction("void Init()", false);
    as_funcs.receive_message        = as_context->RegisterExpectedFunction("void ReceiveMessage(string)", false);
    as_funcs.set_window_dimensions  = as_context->RegisterExpectedFunction("void SetWindowDimensions(int width, int height)", false);

    as_funcs.update                 = as_context->RegisterExpectedFunction("void Update()", true);
    as_funcs.dispose                = as_context->RegisterExpectedFunction("void Dispose()", true);
    as_funcs.enter_campaign         = as_context->RegisterExpectedFunction("void EnterCampaign()", true);
    as_funcs.enter_level            = as_context->RegisterExpectedFunction("void EnterLevel()", true);
    as_funcs.leave_campaign         = as_context->RegisterExpectedFunction("void LeaveCampaign()", true);
    as_funcs.leave_level            = as_context->RegisterExpectedFunction("void LeaveLevel()", true);

    char path[kPathSize];
    FormatString(path, kPathSize, "%sasscriptable_campaign_docs.h", GetWritePath(CoreGameModID).c_str());
    as_context->ExportDocs(path);
    
    // Load script and run init function
    as_context->LoadScript(script_path);

    as_context->CallScriptFunction(as_funcs.init);
}

void ScriptableCampaign::Dispose() {
    as_context->CallScriptFunction(as_funcs.dispose);
    delete as_context;
}

void ScriptableCampaign::Update() {
    if(as_context) {
        as_context->CallScriptFunction(as_funcs.update);
    } 
}

void ScriptableCampaign::EnterCampaign() {
    if(as_context) {
        as_context->CallScriptFunction(as_funcs.enter_campaign);
    } 
}

void ScriptableCampaign::EnterLevel() {
    if(as_context) {
        as_context->CallScriptFunction(as_funcs.enter_level);
    } 
}

void ScriptableCampaign::LeaveCampaign() {
    if(as_context) {
        as_context->CallScriptFunction(as_funcs.leave_campaign);
    } 
}

void ScriptableCampaign::LeaveLevel() {
    if(as_context) {
        as_context->CallScriptFunction(as_funcs.leave_level);
    } 
}

void ScriptableCampaign::ReceiveMessage(std::string message) {
    if(as_context) {
        ASArglist args;
        args.AddObject(&message);
        as_context->CallScriptFunction(as_funcs.receive_message, &args);
    }
}

void ScriptableCampaign::WindowResized(ivec2 value ) {
    ASArglist args;
    args.Add(value[0]);
    args.Add(value[1]);
    as_context->CallScriptFunction(as_funcs.set_window_dimensions, &args);
}

std::string ScriptableCampaign::GetCampaignID() {
    return campaign_id;
}
