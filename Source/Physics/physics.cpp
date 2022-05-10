//-----------------------------------------------------------------------------
//           Name: physics.cpp
//      Developer: Wolfire Games LLC
//         Author: David Rosen
//    Description: This class handles wind and gravity
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
#include "physics.h"

#include <cmath>

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

// Draw a particle
void Physics::Update() {
}

vec3 Physics::GetWind(vec3 check_where, float curr_game_time, float change_rate) {
    vec3 wind_vel;
    check_where[0] += curr_game_time * 0.7f * change_rate;
    check_where[1] += curr_game_time * 0.3f * change_rate;
    check_where[2] += curr_game_time * 0.5f * change_rate;
    wind_vel[0] = sinf(check_where[0]) + cosf(check_where[1] * 1.3f) + sinf(check_where[2] * 3.0f);
    wind_vel[1] = sinf(check_where[0] * 1.2f) + cosf(check_where[1] * 1.8f) + sinf(check_where[2] * 0.8f);
    wind_vel[2] = sinf(check_where[0] * 1.6f) + cosf(check_where[1] * 0.5f) + sinf(check_where[2] * 1.2f);

    return wind_vel;
}
