//-----------------------------------------------------------------------------
//           Name: scriptablecampaign.h
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
#include <Math/ivec2.h>

#include <string>
#include <memory>

struct ASData;
class SaveFile;
class ASContext;

class ScriptableCampaign {
private:
    std::string campaign_id;
	ASContext *as_context;

    struct {
        ASFunctionHandle init;

        ASFunctionHandle update;
        ASFunctionHandle dispose;
        ASFunctionHandle enter_campaign;
        ASFunctionHandle enter_level;
        ASFunctionHandle leave_campaign;
        ASFunctionHandle leave_level;
        ASFunctionHandle receive_message;
        ASFunctionHandle set_window_dimensions;
    } as_funcs;

public:

    ScriptableCampaign();
    ~ScriptableCampaign();

    void Initialize(Path campaign_script, std::string _campaign_id);
    void Dispose();

    void Update();
    
    void EnterCampaign();
    void LeaveCampaign();
    void EnterLevel();
    void LeaveLevel();

    void ReceiveMessage(std::string message);
    void WindowResized(ivec2 value);

    std::string GetCampaignID();

    DISALLOW_COPY_AND_ASSIGN(ScriptableCampaign);
};
