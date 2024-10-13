//-----------------------------------------------------------------------------
//           Name: snow.as
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

const float kBaseSpawnRate = 120.0f;
bool spawn_always_on = true;
float time_per_spawn = 1.0f / kBaseSpawnRate;

int player_in_hotspot_count = 0;
float elapsed_time = 0.0f;
vec3 previous_position = vec3(0.0f);

void SetParameters() {
    params.AddIntCheckbox("SpawnsOnlyWhenPlayerInside", false);
    spawn_always_on = params.GetInt("SpawnsOnlyWhenPlayerInside") == 0;

    params.AddFloatSlider("SpawnRate", 1.0f, "min:0.01,max:2.0,step:0.01,text_mult:100");
    float spawn_frequency = Clamp(params.GetFloat("SpawnRate"), 0.01f, 10.0f);
    time_per_spawn = 1.0f / (kBaseSpawnRate * spawn_frequency);
}

void Update() {
    if (player_in_hotspot_count <= 0 && !spawn_always_on) {
        return;
    }
    SpawnSnowParticles();
}

void SpawnSnowParticles() {
    vec3 current_position = camera.GetPos();
    vec3 movement_vector = current_position - previous_position;
    elapsed_time += time_step;
    float domain_size = 5.0f;
    vec3 scale = vec3(domain_size);

    while (elapsed_time >= time_per_spawn) {
        vec3 offset;
        offset.x = RangedRandomFloat(-scale.x * 2.0f, scale.x * 2.0f);
        offset.y = RangedRandomFloat(-scale.y * 2.0f, scale.y * 2.0f);
        offset.z = RangedRandomFloat(-scale.z * 2.0f, scale.z * 2.0f);

        vec3 initial_position = current_position + offset + movement_vector * 150.0f;
        MakeParticle("Data/Particles/snow.xml", initial_position, vec3(0.0f));
        elapsed_time -= time_per_spawn;
    }

    previous_position = current_position;
}

void HandleEvent(string event, MovementObject@ mo) {
    Object@ mo_obj = ReadObjectFromID(mo.GetID());
    if (!mo_obj.GetPlayer()) {
        return;
    }
    if (event == "enter") {
        ++player_in_hotspot_count;
    } else if (event == "exit") {
        --player_in_hotspot_count;
    }
}

float Clamp(float value, float min_value, float max_value) {
    return min(max(value, min_value), max_value);
}
