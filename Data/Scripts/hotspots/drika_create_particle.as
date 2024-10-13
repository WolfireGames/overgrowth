//-----------------------------------------------------------------------------
//           Name: drika_create_particle.as
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

class DrikaCreateParticle : DrikaElement {
    string particle_path;
    int object_id;
    int amount;

    DrikaCreateParticle(int _index, int _object_id, int _amount, string _particle_path) {
        index = _index;
        object_id = _object_id;
        amount = _amount;
        particle_path = _particle_path;
        drika_element_type = drika_create_particle;
        display_color = vec4(85, 131, 102, 255);
        has_settings = true;
    }

    string GetSaveString() {
        return "create_particle " + object_id + " " + amount + " " + particle_path;
    }

    string GetDisplayString() {
        return "CreateParticle " + particle_path;
    }

    void AddSettings() {
        ImGui_Text("Particle Path: ");
        ImGui_Text(particle_path);
        if (ImGui_Button("Set Particle Path")) {
            string new_path = GetUserPickedReadPath("xml", "Data/Particles");
            if (new_path != "") {
                particle_path = new_path;
            }
        }
        ImGui_InputInt("Object ID", object_id);
        ImGui_InputInt("Amount", amount);
    }

    bool Trigger() {
        if (!ObjectExists(object_id)) {
            Log(info, "Object does not exist.");
            return false;
        }
        Object@ obj = ReadObjectFromID(object_id);
        for (int i = 0; i < amount; i++) {
            MakeParticle(particle_path, obj.GetTranslation(), vec3(0), GetBloodTint());
        }
        return true;
    }

    void Reset() {
        // No reset needed
    }
}
