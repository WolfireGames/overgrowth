//-----------------------------------------------------------------------------
//           Name: drika_element.as
//      Developer:
//         Author: Gyrth, Fason7
//    Script Type: None
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

enum drika_element_types {
    none,
    drika_wait_level_message,
    drika_wait,
    drika_set_enabled,
    drika_set_character,
    drika_create_particle,
    drika_play_sound
};

class DrikaElement {
    drika_element_types drika_element_type = none;
    bool edit_mode = false;
    bool visible;
    bool has_settings = false;
    vec4 display_color = vec4(1.0);
    int index = -1;

    string GetSaveString() { return ""; }
    string GetDisplayString() { return ""; }
    void Update() {}
    bool Trigger() { return false; }
    void Reset() {}
    void AddSettings() {}
    void EditDone() {}
    void SetCurrent(bool _current) {}
    void Delete() {}
    void ReceiveMessage(string message) {}
    void SetIndex(int _index) {
        index = _index;
    }
}
