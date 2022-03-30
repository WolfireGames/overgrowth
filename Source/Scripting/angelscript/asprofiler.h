//-----------------------------------------------------------------------------
//           Name: asprofiler.h
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

#include <Internal/integer.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <vector>
#include <string>
#include <map>

class ASContext;

class ASProfiler
{
public:
    ASProfiler();
    ~ASProfiler();

    bool enabled;

    void Update();
    void Draw();

    static void AddContext(ASContext* context);
    static void RemoveContext(ASContext* context);
    static const std::vector<ASContext*>& GetActiveContexts();

    void SetContext(ASContext* context);

    void ASEnterTelemetryZone(const std::string& str);
    void ASLeaveTelemetryZone();

    void CallScriptFunction(const char* str);
    void LeaveScriptFunction();

    static int GetMaxWindowSize() { return kMaxWindowSize; }
    static int GetMaxTimeHistorySize() { return kTimeHistorySize; }
private:
    const static int kMaxWindowSize = 60;
    const static int kTimeHistorySize = 10;
    struct ZoneTime {
        std::string name;
        std::vector<uint64_t> times;
        uint64_t temp_time;
        ZoneTime* parent;
        std::vector<ZoneTime*> children;
        int id;
    };
    static std::vector<ASContext*> active_contexts;

    std::vector<ZoneTime> script_function_times;
    ZoneTime* last_script_function_zone;

    int id_counter;
    int window_counter;

    ASContext* context;

    ZoneTime* last_active_zone;
    ZoneTime* active_zone;
    std::vector<ZoneTime*> zone_times;

    void DeleteZone(ZoneTime* zone);
    void MoveForward(ZoneTime* zone);
    void DrawZone(ZoneTime* zone);
};
