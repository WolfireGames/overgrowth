//-----------------------------------------------------------------------------
//           Name: fire_test_cheap.as
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

        SpawnNewParticles(delta_time, curr_game_time);
        LimitParticles();
        UpdateParticles(delta_time, curr_game_time);

        LeaveTelemetryZone();
    }

    void SpawnNewParticles(float delta_time, float curr_game_time) {
        spawn_new_particle_delay -= delta_time;
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

    void LimitParticles() {
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
            particle.vel += GetWind(particle.pos * 5.0f, curr_game_time, 10.0f) * delta_time * 1.0f;
            particle.vel += GetWind(particle.pos * 30.0f, curr_game_time, 10.0f) * delta_time * 2.0f;

            vec3 rel = particle.pos - fire_pos;
            rel.y = 0.0f;
            particle.heat -= delta_time * (2.0f + min(1.0f, pow(dot(rel, rel), 2.0f) * 64.0f)) * 2.0f;

            if (dot(rel, rel) > 1.0f) {
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
            AddDebugDrawRibbonPoint(ribbon_id, particle.pos, vec4(particle.heat, particle.spawn_time + base_rand, curr_game_time + base_rand, 0.0f), flame_width);
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
    bool ignite = true;
    if (params.HasParam("Ignite Characters") && params.GetInt("Ignite Characters") == 0) {
        ignite = false;
    }
    if (ignite) {
        mo.ReceiveScriptMessage("entered_fire");
    }
}

void OnExit(MovementObject@ mo) {
    // Handle exit if needed
}

void SetEnabled(bool val) {
    if (!val) {
        Dispose();
    }
}

void PreDraw(float curr_game_time) {
    int obj_id = hotspot.GetID();
    if (!ObjectExists(obj_id) || !ReadObjectFromID(obj_id).GetEnabled()) {
        return;
    }

    EnterTelemetryZone("Fire Hotspot Predraw");

    float delta_time = curr_game_time - last_game_time;
    Object@ obj = ReadObjectFromID(obj_id);
    vec3 pos = obj.GetTranslation();
    vec3 scale = obj.GetScale();
    quaternion rot = obj.GetRotation();

    InitializeRibbons();
    UpdateRibbons(delta_time, curr_game_time, pos, scale, rot);
    SpawnParticles(delta_time);
    ManageSound(pos, curr_game_time);

    last_game_time = curr_game_time;

    LeaveTelemetryZone();
}

void InitializeRibbons() {
    if (int(ribbons.size()) == num_ribbons) {
        return;
    }

    ribbons.resize(num_ribbons);
    for (int i = 0; i < num_ribbons; ++i) {
        ribbons[i].rel_pos = vec3(RangedRandomFloat(-1.0f, 1.0f), 0.0f, RangedRandomFloat(-1.0f, 1.0f));
        ribbons[i].base_rand = RangedRandomFloat(0.0f, 100.0f);
        ribbons[i].spawn_new_particle_delay = RangedRandomFloat(0.0f, 0.1f);
    }
}

void UpdateRibbons(float delta_time, float curr_game_time, vec3 pos, vec3 scale, quaternion rot) {
    --count;
    for (int i = 0; i < num_ribbons; ++i) {
        ribbons[i].pos = pos + rot * vec3(ribbons[i].rel_pos.x * scale.x, scale.y * 2.0f, ribbons[i].rel_pos.z * scale.z);
    }

    if (count <= 0) {
        count = 10;
    }

    for (int i = 0; i < num_ribbons; ++i) {
        ribbons[i].Update(delta_time, curr_game_time);
        ribbons[i].PreDraw(curr_game_time);
    }
}

void SpawnParticles(float delta_time) {
    delay -= delta_time;
    if (delay > 0.0f) {
        return;
    }

    delay = RangedRandomFloat(0.0f, 0.6f);
}

void ManageSound(vec3 pos, float curr_game_time) {
    if (sound_handle == -1 && !level.WaitingForInput()) {
        // Uncomment and set the correct sound file path to enable sound
        // sound_handle = PlaySoundLoopAtLocation("Data/Sounds/fire/campfire_loop.wav", pos, 0.0f);
        // sound_start_time = curr_game_time;
    } else if (sound_handle != -1) {
        // SetSoundPosition(sound_handle, pos);
        // SetSoundGain(sound_handle, min(1.0f, curr_game_time - sound_start_time));
    }
}

vec4 ColorFromHeat(float heat) {
    if (heat < 0.0f) {
        return vec4(0.0f);
    }
    if (heat > 0.5f) {
        return mix(vec4(2.0f, 1.0f, 0.0f, 1.0f), vec4(4.0f, 4.0f, 0.0f, 1.0f), (heat - 0.5f) / 0.5f);
    } else {
        return mix(vec4(0.0f), vec4(2.0f, 1.0f, 0.0f, 1.0f), heat / 0.5f);
    }
}

void SetParameters() {
    params.AddInt("Fire Ribbons", 4);
    params.AddIntCheckbox("Ignite Characters", true);
    num_ribbons = max(0, min(500, params.GetInt("Fire Ribbons")));
    hotspot.SetCollisionEnabled(params.GetInt("Ignite Characters") == 1);
}
