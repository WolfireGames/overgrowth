//-----------------------------------------------------------------------------
//           Name: snow.as
//      Developer: Wolfire Games LLC
//    Script Type: Hotspot
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

const float k_base_spawn_rate = 120.0f;
bool g_is_spawn_always_on = true;
float g_time_per_spawn = 1.0f / k_base_spawn_rate;

int g_player_in_hotspot_count = 0;
float g_elapsed_time = 0;
vec3 g_pos_previous = vec3(0.0);

void SetParameters() {
    params.AddIntCheckbox("Spawns Only When Player Inside", false);
    g_is_spawn_always_on = params.GetInt("Spawns Only When Player Inside") == 0;

    params.AddFloatSlider("Spawn Rate", 1.0, "min:0.01,max:2.0,step:0.01,text_mult:100");
    float spawn_frequency = min(10.0f, max(0.01f, params.GetFloat("Spawn Rate")));
    g_time_per_spawn = 1.0f / (k_base_spawn_rate * spawn_frequency);
}

void Update() {
    if(g_player_in_hotspot_count > 0 || g_is_spawn_always_on) {
        vec3 pos = camera.GetPos();
        float domain_size = 5.0f;
        vec3 scale = vec3(domain_size);
        vec3 movement_vec = pos - g_pos_previous;

        g_elapsed_time += time_step;

        while(g_elapsed_time >= g_time_per_spawn) {
            vec3 offset;
            offset.x = RangedRandomFloat(-scale.x * 2.0f, scale.x * 2.0f);
            offset.y = RangedRandomFloat(-scale.y * 2.0f, scale.y * 2.0f);
            offset.z = RangedRandomFloat(-scale.z * 2.0f, scale.z * 2.0f);

            vec3 initial_position = pos + offset + movement_vec * 150;
            uint32 id = MakeParticle("Data/Particles/snow.xml", initial_position, vec3(0.0f, 0.0f, 0.0f));

            g_elapsed_time -= g_time_per_spawn;
        }

        g_pos_previous = pos;
    }
}

void HandleEvent(string event, MovementObject @mo) {
    Object@ mo_obj = ReadObjectFromID(mo.GetID());

    if(event == "enter") {
        if(mo_obj.GetPlayer()) {
            ++g_player_in_hotspot_count;
        }
    } else if(event == "exit") {
        if(mo_obj.GetPlayer()) {
            --g_player_in_hotspot_count;
        }
    }
}
