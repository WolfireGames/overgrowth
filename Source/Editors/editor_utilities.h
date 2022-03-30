//-----------------------------------------------------------------------------
//           Name: editor_utilities.h
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

#pragma once

#include <Math/vec3.h>

class EditorEventTrigger {
public:
    EditorEventTrigger(int delay_time = 0);
    
    void Start();
    void Stop();
    bool IsActive();
    void ForceATrigger();
    bool Check();
    
    inline int GetDelay() const { return m_delay_time; }
    inline void SetDelay(int delay_time) { m_delay_time = delay_time; }

private:
    int m_last_triggered_time;    // in milliseconds
    int m_delay_time;            // 
    bool m_force_a_trigger, m_active;    // triggers can only be forced if the trigger is active
};

inline int GetSign(float n) { return n<0?-1:1; }

vec3 Interpolate(const vec3& v1, const vec3& v2, float amount);    // returns a vec3 that is amount percent v1 and 1-amount percent v2

float ConvertToFirstCycle(float n, float cycle_size);
