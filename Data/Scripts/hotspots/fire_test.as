//-----------------------------------------------------------------------------
//           Name: fire_test.as
//      Developer: Wolfire Games LLC
//    Script Type: Hotspot
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
//

void Init() {
    // No initialization needed
}

float delay = 0.0f;

class Particle {
    vec3 pos;
    vec3 vel;
    float heat;
    float spawn_time;
}

int count = 0;
int num_ribbons;
float sound_start_time;

class Ribbon {
    array<Particle> particles;
    vec3 rel_pos;
    vec3 pos;
    float base_rand;
    float spawn_new_particle_delay;

    void Update(float delta_time, float curr_game_time) {
        EnterTelemetryZone("Ribbon Update");

        UpdateSpawnDelay(delta_time);
        SpawnNewParticle(curr_game_time);
        LimitParticleCount();
        UpdateParticles(delta_time, curr_game_time);

        LeaveTelemetryZone();
    }

    void UpdateSpawnDelay(float delta_time) {
        spawn_new_particle_delay -= delta_time;
    }

    void SpawnNewParticle(float curr_game_time) {
        if (spawn_new_particle_delay > 0.0f) {
            return;
        }

        Particle particle;
        particle.pos = pos;
        particle.vel = vec3(0.0f);
        particle.heat = RangedRandomFloat(0.5f, 1.5f);
        particle.spawn_time = curr_game_time;
        particles.push_back(particle);

        while (spawn_new_particle_delay <= 0.0f) {
            spawn_new_particle_delay += 0.1f;
        }
    }

    void LimitParticleCount() {
        const int max_particles = 5;
        if (int(particles.size()) <= max_particles) {
            return;
        }

        for (int i = 0; i < max_particles; ++i) {
            particles[i] = particles[particles.size() - max_particles + i];
        }
        particles.resize(max_particles);
    }

    void UpdateParticles(float delta_time, float curr_game_time) {
        Object@ obj = ReadObjectFromID(hotspot.GetID());
        vec3 fire_pos = obj.GetTranslation();

        for (uint i = 0; i < particles.size(); ++i) {
            Particle@ particle = particles[i];

            particle.vel *= pow(0.2f, delta_time);
            particle.pos += particle.vel * delta_time;
            particle.vel += GetWind(particle.pos * 5.0f, curr_game_time, 10.0f) * delta_time;
            particle.vel += GetWind(particle.pos * 30.0f, curr_game_time, 10.0f) * delta_time * 2.0f;

            vec3 rel = particle.pos - fire_pos;
            rel.y = 0.0f;
            float distance_squared = dot(rel, rel);
            particle.heat -= delta_time * (2.0f + min(1.0f, pow(distance_squared, 2.0f) * 64.0f)) * 2.0f;

            if (distance_squared > 1.0f) {
                rel = normalize(rel);
            }

            particle.vel += rel * delta_time * -18.0f;
            particle.vel.y += delta_time * 12.0f;
        }
    }

    void PreDraw(float curr_game_time) {
        EnterTelemetryZone("Ribbon PreDraw");

        int ribbon_id = DebugDrawRibbon(_delete_on_draw);
        const float flame_width = 0.12f;

        for (uint i = 0; i < particles.size(); ++i) {
            Particle@ particle = particles[i];
            AddDebugDrawRibbonPoint(
                ribbon_id,
                particle.pos,
                vec4(particle.heat, particle.spawn_time + base_rand, curr_game_time + base_rand, 0.0f),
                flame_width
            );
        }

        LeaveTelemetryZone();
    }
}

array<Ribbon> ribbons;

vec3 GetWind(vec3 check_where, float curr_game_time, float change_rate) {
    vec3 wind_vel;

    check_where.x += curr_game_time * 0.7f * change_rate;
    check_where.y += curr_game_time * 0.3f * change_rate;
    check_where.z += curr_game_time * 0.5f * change_rate;

    wind_vel.x = sin(check_where.x) + cos(check_where.y * 1.3f) + sin(check_where.z * 3.0f);
    wind_vel.y = sin(check_where.x * 1.2f) + cos(check_where.y * 1.8f) + sin(check_where.z * 0.8f);
    wind_vel.z = sin(check_where.x * 1.6f) + cos(check_where.y * 0.5f) + sin(check_where.z * 1.2f);

    return wind_vel;
}

int fire_object_id = -1;
int sound_handle = -1;
float last_game_time = 0.0f;

void Dispose() {
    if (fire_object_id != -1) {
        QueueDeleteObjectID(fire_object_id);
        fire_object_id = -1;
    }
    if (sound_handle != -1) {
        StopSound(sound_handle);
        sound_handle = -1;
    }
}

void HandleEvent(string event, MovementObject@ mo) {
    if (event == "enter") {
        OnEnter(mo);
    } else if (event == "exit") {
        OnExit(mo);
    }
}

void OnEnter(MovementObject@ mo) {
    if (!ShouldIgniteCharacters()) {
        return;
    }
    mo.ReceiveScriptMessage("entered_fire");
}

bool ShouldIgniteCharacters() {
    return !(params.HasParam("Ignite Characters") && params.GetInt("Ignite Characters") == 0);
}

void SetEnabled(bool val) {
    if (!val) {
        Dispose();
    }
}

void OnExit(MovementObject@ mo) {
    // Handle exit event if needed
}

void PreDraw(float curr_game_time) {
    int obj_id = hotspot.GetID();
    if (!ObjectExists(obj_id) || !ReadObjectFromID(obj_id).GetEnabled()) {
        return;
    }

    EnterTelemetryZone("Fire Hotspot Predraw");

    float delta_time = curr_game_time - last_game_time;
    UpdateRibbons(delta_time, curr_game_time);

    SpawnParticles(delta_time);
    UpdateFireObject(curr_game_time);
    UpdateSound(curr_game_time);

    last_game_time = curr_game_time;

    LeaveTelemetryZone();
}

void UpdateRibbons(float delta_time, float curr_game_time) {
    Object@ obj = ReadObjectFromID(hotspot.GetID());
    vec3 pos = obj.GetTranslation();
    vec3 scale = obj.GetScale();
    quaternion rot = obj.GetRotation();

    InitializeRibbons();

    for (int i = 0; i < num_ribbons; ++i) {
        ribbons[i].pos = pos + rot * vec3(
            ribbons[i].rel_pos.x * scale.x,
            scale.y * 2.0f,
            ribbons[i].rel_pos.z * scale.z
        );
    }

    for (int i = 0; i < num_ribbons; ++i) {
        ribbons[i].Update(delta_time, curr_game_time);
        ribbons[i].PreDraw(curr_game_time);
    }
}

void InitializeRibbons() {
    if (int(ribbons.size()) == num_ribbons) {
        return;
    }

    ribbons.resize(num_ribbons);
    for (int i = 0; i < num_ribbons; ++i) {
        ribbons[i].rel_pos = vec3(
            RangedRandomFloat(-1.0f, 1.0f),
            0.0f,
            RangedRandomFloat(-1.0f, 1.0f)
        );
        ribbons[i].base_rand = RangedRandomFloat(0.0f, 100.0f);
        ribbons[i].spawn_new_particle_delay = RangedRandomFloat(0.0f, 0.1f);
    }
}

void SpawnParticles(float delta_time) {
    delay -= delta_time;

    if (delay > 0.0f) {
        return;
    }

    int random_ribbon_index = int(RangedRandomFloat(0, num_ribbons - 0.01f));
    vec3 position = ribbons[random_ribbon_index].pos;
    vec3 velocity = vec3(
        RangedRandomFloat(-2.0f, 2.0f),
        RangedRandomFloat(5.0f, 10.0f),
        RangedRandomFloat(-2.0f, 2.0f)
    );
    MakeParticle("Data/Particles/firespark.xml", position, velocity, vec3(1.0f));

    delay = RangedRandomFloat(0.0f, 0.6f);
}

void UpdateFireObject(float curr_game_time) {
    if (fire_object_id == -1) {
        fire_object_id = CreateObject("Data/Objects/default_light.xml", true);
    }

    float amplify = 1.0f;
    if (params.HasParam("Light Amplify")) {
        amplify *= params.GetFloat("Light Amplify");
    }

    float distance = 10.0f;
    if (params.HasParam("Light Distance")) {
        distance *= params.GetFloat("Light Distance");
    }

    if (ribbons[0].particles.size() > 3) {
        Object@ fire_obj = ReadObjectFromID(fire_object_id);
        Particle@ p3 = ribbons[0].particles[3];
        Particle@ p2 = ribbons[0].particles[2];
        float t = ribbons[0].spawn_new_particle_delay / 0.1f;
        vec3 position = mix(p3.pos, p2.pos, t);
        float heat = mix(p3.heat, p2.heat, t);
        fire_obj.SetTranslation(position);
        fire_obj.SetTint(amplify * 0.2f * vec3(2.0f, 1.0f, 0.0f) * (2.0f + heat));
        fire_obj.SetScale(vec3(distance));
    }
}

void UpdateSound(float curr_game_time) {
    vec3 pos = ReadObjectFromID(hotspot.GetID()).GetTranslation();
    if (sound_handle == -1) {
        if (!level.WaitingForInput()) {
            sound_handle = PlaySoundLoopAtLocation("Data/Sounds/fire/campfire_loop.wav", pos, 0.0f);
            sound_start_time = curr_game_time;
        }
    } else {
        SetSoundPosition(sound_handle, pos);
        SetSoundGain(sound_handle, min(1.0f, curr_game_time - sound_start_time));
    }
}

vec4 ColorFromHeat(float heat) {
    if (heat < 0.0f) {
        return vec4(0.0f);
    } else if (heat > 0.5f) {
        return mix(
            vec4(2.0f, 1.0f, 0.0f, 1.0f),
            vec4(4.0f, 4.0f, 0.0f, 1.0f),
            (heat - 0.5f) / 0.5f
        );
    } else {
        return mix(
            vec4(0.0f),
            vec4(2.0f, 1.0f, 0.0f, 1.0f),
            heat / 0.5f
        );
    }
}

void SetParameters() {
    params.AddFloatSlider("Light Amplify", 1.0f, "min:0,max:20,step:0.1");
    params.AddFloatSlider("Light Distance", 2.0f, "min:0,max:100,step:0.1");
    params.AddInt("Fire Ribbons", 4);
    params.AddIntCheckbox("Ignite Characters", true);
    num_ribbons = max(0, min(500, params.GetInt("Fire Ribbons")));
    hotspot.SetCollisionEnabled(params.GetInt("Ignite Characters") == 1);
}
