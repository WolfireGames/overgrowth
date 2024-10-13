//-----------------------------------------------------------------------------
//           Name: drika_wait.as
//      Developer:
//         Author: Gyrth, Fason7
//    Script Type: Drika Element
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

class DrikaWait : DrikaElement {
    float timer;
    int duration;

    DrikaWait(int _index, int _duration) {
        index = _index;
        duration = _duration;
        timer = duration / 1000.0;
        drika_element_type = drika_wait;
        display_color = vec4(152, 113, 80, 255);
        has_settings = true;
    }

    string GetSaveString() {
        return "wait " + duration;
    }

    string GetDisplayString() {
        return "Wait " + duration;
    }

    void AddSettings() {
        ImGui_Text("Wait in ms:");
        ImGui_DragInt("Duration", duration, 1.0, 1, 10000);
    }

    bool Trigger() {
        if (timer <= 0.0) {
            Log(info, "Timer done");
            return true;
        }
        timer -= time_step;
        return false;
    }

    void Reset() {
        timer = duration / 1000.0;
    }
}
