//-----------------------------------------------------------------------------
//           Name: drika_set_enabled.as
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

class DrikaSetEnabled : DrikaElement {
    bool enabled;
    int object_id;

    DrikaSetEnabled(int _index, int _object_id, bool _enabled) {
        index = _index;
        object_id = _object_id;
        enabled = _enabled;
        drika_element_type = drika_set_enabled;
        display_color = vec4(88, 122, 147, 255);
        has_settings = true;
        Reset();
    }

    string GetSaveString() {
        return "set_enabled " + object_id + " " + enabled;
    }

    string GetDisplayString() {
        return "SetEnabled " + object_id + " " + enabled;
    }

    void AddSettings() {
        ImGui_Text("Set To:");
        ImGui_SameLine();
        ImGui_Checkbox("", enabled);
        ImGui_InputInt("Object ID", object_id);
    }

    bool Trigger() {
        if (!ObjectExists(object_id)) {
            Log(info, "Object does not exist with id " + object_id);
            return false;
        }
        Object@ obj = ReadObjectFromID(object_id);
        obj.SetEnabled(enabled);
        return true;
    }

    void Reset() {
        if (!ObjectExists(object_id)) {
            return;
        }
        Object@ obj = ReadObjectFromID(object_id);
        obj.SetEnabled(!enabled);
    }
}
