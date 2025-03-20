//-----------------------------------------------------------------------------
//           Name: soak_level.as
//      Developer: Wolfire Games LLC
//    Script Type: Hotspot
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

float elapsed_time_before_reload = 0.0f;
bool level_load_triggered = false;

void SetParameters() {
    params.AddString("next_level", "");
    params.AddFloat("time_to_next_level", 5.0f);
}

void Update() {
    string next_level_path = params.GetString("next_level");
    if (next_level_path == "") {
        return;
    }
    float time_to_next_level = params.GetFloat("time_to_next_level");
    elapsed_time_before_reload += time_step;

    DebugText("soaktest1", "Time til next level: " + (time_to_next_level - elapsed_time_before_reload), 0.5f);

    if (elapsed_time_before_reload >= time_to_next_level && !level_load_triggered) {
        level_load_triggered = true;
        level.SendMessage("loadlevel \"" + next_level_path + "\"");
    }
}
