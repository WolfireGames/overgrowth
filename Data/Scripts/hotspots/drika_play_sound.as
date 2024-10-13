//-----------------------------------------------------------------------------
//           Name: drika_play_sound.as
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

class DrikaPlaySound : DrikaElement {
    string sound_path;
    int object_id;

    DrikaPlaySound(int _index, int _object_id, string _sound_path) {
        index = _index;
        object_id = _object_id;
        sound_path = _sound_path;
        drika_element_type = drika_play_sound;
        display_color = vec4(145, 99, 66, 255);
        has_settings = true;
    }

    string GetSaveString() {
        return "play_sound " + object_id + " " + sound_path;
    }

    string GetDisplayString() {
        return "PlaySound " + sound_path;
    }

    void AddSettings() {
        ImGui_Text("Sound Path:");
        ImGui_Text(sound_path);
        if (ImGui_Button("Set Sound Path")) {
            string new_path = GetUserPickedReadPath("wav", "Data/Sounds");
            if (new_path != "") {
                sound_path = new_path;
            }
        }
        ImGui_InputInt("Object ID", object_id);
    }

    bool Trigger() {
        if (!ObjectExists(object_id)) {
            Log(info, "Object does not exist.");
            return false;
        }
        Object@ obj = ReadObjectFromID(object_id);
        PlaySound(sound_path, obj.GetTranslation());
        return true;
    }
}
