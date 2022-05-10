//-----------------------------------------------------------------------------
//           Name: editor_utilities.cpp
//      Developer: Wolfire Games LLC
//         Author: Phillip Isola
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
#include "editor_utilities.h"

#include <Utility/assert.h>

#include <SDL.h>

#include <string>

EditorEventTrigger::EditorEventTrigger(int delay_time) : m_delay_time(delay_time) {
    m_active = false;
    m_force_a_trigger = false;
}

void EditorEventTrigger::Start() {
    m_active = true;
    m_last_triggered_time = SDL_GetTicks();
}

void EditorEventTrigger::Stop() {
    m_active = false;
}

bool EditorEventTrigger::IsActive() {
    return m_active;
}

void EditorEventTrigger::ForceATrigger() {
    m_force_a_trigger = true;
}

bool EditorEventTrigger::Check() {
    if (!m_active) return false;
    if (m_force_a_trigger) {
        m_force_a_trigger = false;
        return true;
    }

    int curr_time = SDL_GetTicks();
    if (curr_time - m_last_triggered_time > m_delay_time) {
        m_last_triggered_time = curr_time;
        return true;
    }
    return false;
}

// returns a vec3 that is amount percent v2 and 1-amount percent v1
vec3 Interpolate(const vec3& v1, const vec3& v2, float amount) {
#ifdef _DEBUG
    LOG_ASSERT(amount >= 0.0f && amount <= 1.0f);
#endif

    float amount_complement = 1.0f - amount;
    return vec3(v2[0] * amount + v1[0] * amount_complement,
                v2[1] * amount + v1[1] * amount_complement,
                v2[2] * amount + v1[2] * amount_complement);
}

float ConvertToFirstCycle(float n, float cycle_size) {
    n -= ((int)(n / cycle_size)) * cycle_size;
    if (n < 0) n = cycle_size + n;
    return n;
}
