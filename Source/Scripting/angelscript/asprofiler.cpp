//-----------------------------------------------------------------------------
//           Name: asprofiler.cpp
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
#include "asprofiler.h"

#include <Compat/time.h>
#include <Scripting/angelscript/ascontext.h>
#include <Logging/logdata.h>

#include <algorithm>

std::vector<ASContext*> ASProfiler::active_contexts;

ASProfiler::ASProfiler()
    : enabled(false), last_active_zone(NULL), active_zone(NULL), id_counter(0), window_counter(0) {}

ASProfiler::~ASProfiler() {
    for (auto& zone_time : zone_times)
        DeleteZone(zone_time);
}

void ASProfiler::Update() {
    if (window_counter == kMaxWindowSize) {
        for (auto& zone_time : zone_times) {
            MoveForward(zone_time);
        }
        for (auto& script_function_time : script_function_times) {
            MoveForward(&script_function_time);
        }
        window_counter = 0;
    } else {
        window_counter++;
    }
}

static float TimeGetter(void* data, int idx) {
    return (float)(((uint64_t*)data)[idx]) * 0.001f;
}

static char buffer[256];
void ASProfiler::Draw() {
    if (active_zone != NULL) {
        LOGW << active_zone->name << " isn't closed before drawing the AS profiler, have you forgotten to close it? It will be automatically closed" << std::endl;
        active_zone = NULL;
    }

    uint64_t zone_time = 0;
    for (auto& i : zone_times) {
        zone_time += i->times.back();
    }

    sprintf(buffer, "%s: %f###%p", context->current_script.GetOriginalPath(), zone_time * 0.001f, (void*)context);
    if (ImGui::TreeNode(buffer)) {
        for (auto& script_function_time : script_function_times) {
            ZoneTime* zone = &script_function_time;
            ImGui::Text("%s", zone->name.c_str());
            ImGui::PlotHistogram("", &TimeGetter, zone->times.data(), kTimeHistorySize, 0, NULL, 0.0f, 5000.0f, ImVec2(0, 40));
        }

        for (auto& zone_time : zone_times) {
            DrawZone(zone_time);
        }
        ImGui::TreePop();
    }
}

void ASProfiler::AddContext(ASContext* context) {
    bool add = true;
    for (auto& active_context : active_contexts) {
        if (active_context == context) {
            add = false;
            break;
        }
    }

    if (add)
        active_contexts.push_back(context);
}

void ASProfiler::RemoveContext(ASContext* context) {
    for (size_t i = 0; i < active_contexts.size(); ++i) {
        if (active_contexts[i] == context) {
            active_contexts.erase(active_contexts.begin() + i);
            break;
        }
    }
}

const std::vector<ASContext*>& ASProfiler::GetActiveContexts() {
    return active_contexts;
}

void ASProfiler::SetContext(ASContext* context) {
    this->context = context;
}

void ASProfiler::ASEnterTelemetryZone(const std::string& str) {
    if (!enabled)
        return;

    bool found = false;
    if (active_zone == NULL) {
        for (auto& zone_time : zone_times) {
            if (zone_time->name == str) {
                active_zone = zone_time;
                found = true;
                break;
            }
        }

        if (!found) {
            ZoneTime* newZone = new ZoneTime;
            newZone->name = str;
            newZone->parent = NULL;
            newZone->times.resize(kTimeHistorySize, 0);
            newZone->id = id_counter++;
            zone_times.push_back(newZone);
            active_zone = zone_times.back();
        }
    } else {
        for (auto& i : active_zone->children) {
            if (i->name == str) {
                active_zone = i;
                found = true;
                break;
            }
        }

        if (!found) {
            ZoneTime* newZone = new ZoneTime;
            newZone->name = str;
            newZone->parent = active_zone;
            newZone->times.resize(kTimeHistorySize, 0);
            newZone->id = id_counter++;
            active_zone->children.push_back(newZone);
            active_zone = active_zone->children.back();
        }
    }

    active_zone->temp_time = GetPrecisionTime();
}

void ASProfiler::ASLeaveTelemetryZone() {
    uint64_t end_time = GetPrecisionTime();
    if (!enabled) {
        return;
    } else if (active_zone == NULL) {
        if (last_active_zone == NULL)
            LOGE << "LeaveTelemetryZone called from script file without calling EnterTelemetryZone first, no last active zone" << std::endl;
        else
            LOGE << "LeaveTelemetryZone called from script file without calling EnterTelemetryZone first, last active zone was " << last_active_zone->name << std::endl;

        return;
    }

    uint64_t duration = ToNanoseconds(end_time) - ToNanoseconds(active_zone->temp_time);
    active_zone->times[kTimeHistorySize - 1] = std::max(duration, active_zone->times[kTimeHistorySize - 1]);

    last_active_zone = active_zone;
    active_zone = active_zone->parent;
}

void ASProfiler::CallScriptFunction(const char* str) {
    if (!enabled)
        return;

    ZoneTime* zone = NULL;
    for (auto& script_function_time : script_function_times) {
        if (strcmp(script_function_time.name.c_str(), str) == 0) {
            zone = &script_function_time;
            break;
        }
    }

    if (!zone) {
        ZoneTime newZone;
        newZone.name = str;
        newZone.parent = NULL;
        newZone.id = -1;
        newZone.times.resize(kTimeHistorySize);
        script_function_times.push_back(newZone);
        zone = &script_function_times.back();
    }

    last_script_function_zone = zone;
    zone->temp_time = GetPrecisionTime();
}

void ASProfiler::LeaveScriptFunction() {
    uint64_t end_time = GetPrecisionTime();
    if (!enabled)
        return;

    if (!last_script_function_zone)
        return;  // Shouldn't happen

    uint64_t duration = ToNanoseconds(end_time) - ToNanoseconds(last_script_function_zone->temp_time);
    last_script_function_zone->times[kTimeHistorySize - 1] = std::max(duration, last_script_function_zone->times[kTimeHistorySize - 1]);
    last_script_function_zone = NULL;
}

void ASProfiler::DeleteZone(ZoneTime* zone) {
    for (auto& i : zone->children) {
        DeleteZone(i);
    }

    delete zone;
}

void ASProfiler::MoveForward(ZoneTime* zone) {
    memmove(zone->times.data(), zone->times.data() + 1, sizeof(uint64_t) * (kTimeHistorySize - 1));
    zone->times[kTimeHistorySize - 1] = 0;

    for (auto& i : zone->children) {
        MoveForward(i);
    }
}

void ASProfiler::DrawZone(ZoneTime* zone) {
    uint64_t zone_time = zone->times.back();

    sprintf(buffer, "%s: %f###%d", zone->name.c_str(), zone_time * 0.001f, zone->id);
    if (ImGui::TreeNode(buffer)) {
        ImGui::PlotHistogram("", &TimeGetter, zone->times.data(), kTimeHistorySize, 0, NULL, 0.0f, 5000.0f, ImVec2(0, 40));
        for (auto& i : zone->children) {
            DrawZone(i);
        }
        ImGui::TreePop();
    }
}
