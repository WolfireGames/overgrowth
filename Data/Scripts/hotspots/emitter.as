//-----------------------------------------------------------------------------
//           Name: emitter.as
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

enum ParticleType {
    _smoke = 0,
    _falling_water = 1,
    _foggy = 2,
    _dripping_water = 3,
    _dripping_water_lower = 4
};

int particle_type;
float delay = 0.0f;
float last_game_time = 0.0f;

void Init() {
    hotspot.SetCollisionEnabled(false);
}

void SetParameters() {
    params.AddString("Type", "Smoke");
    string type_string = params.GetString("Type");
    particle_type = GetParticleTypeFromString(type_string);
}

int GetParticleTypeFromString(string type_string) {
    if (type_string == "Smoke") {
        return _smoke;
    } else if (type_string == "Foggy") {
        return _foggy;
    } else if (type_string == "Falling Water") {
        return _falling_water;
    } else if (type_string == "Dripping Water") {
        return _dripping_water;
    } else if (type_string == "Dripping Water Lower") {
        return _dripping_water_lower;
    }
    return _smoke; // Default
}

void PreDraw(float curr_game_time) {
    EnterTelemetryZone("Emitter Update");

    if (!ReadObjectFromID(hotspot.GetID()).GetEnabled()) {
        last_game_time = curr_game_time;
        LeaveTelemetryZone();
        return;
    }

    float delta_time = curr_game_time - last_game_time;
    delay -= delta_time;

    if (delay > 0.0f) {
        last_game_time = curr_game_time;
        LeaveTelemetryZone();
        return;
    }

    Object@ obj = ReadObjectFromID(hotspot.GetID());
    vec3 pos = obj.GetTranslation();
    vec3 scale = obj.GetScale();
    quaternion rotation = obj.GetRotation();

    GenerateParticles(pos, scale, rotation);

    last_game_time = curr_game_time;
    LeaveTelemetryZone();
}

void GenerateParticles(vec3 pos, vec3 scale, quaternion rotation) {
    switch (particle_type) {
        case _smoke:
            EmitParticles("Data/Particles/smoke_ambient.xml", pos, scale, rotation, 1);
            delay += 0.4f;
            break;
        case _foggy:
            EmitParticles("Data/Particles/smoke_foggy.xml", pos, scale, rotation, 1);
            delay += 0.4f;
            break;
        case _falling_water:
            EmitFallingWater(pos, scale, rotation);
            delay += 0.2f;
            break;
        case _dripping_water:
            EmitParticles("Data/Particles/rain.xml", pos, scale, rotation, 1);
            delay += 0.3f;
            break;
        case _dripping_water_lower:
            EmitParticles("Data/Particles/rainsmall.xml", pos, scale, rotation, 1);
            delay += 0.6f;
            break;
    }

    if (delay < -1.0f) {
        delay = -1.0f;
    }
}

void EmitParticles(string particle_path, vec3 pos, vec3 scale, quaternion rotation, int count) {
    for (int i = 0; i < count; ++i) {
        vec3 offset = GetRandomOffset(scale);
        vec3 particle_pos = pos + rotation * offset;
        MakeParticle(particle_path, particle_pos, vec3(0.0f), vec3(1.0f));
    }
}

void EmitFallingWater(vec3 pos, vec3 scale, quaternion rotation) {
    for (int i = 0; i < 1; ++i) {
        vec3 offset = GetRandomOffset(scale);
        vec3 vel = rotation * vec3(1, 0, 0) * 3.0f;
        MakeParticle("Data/Particles/falling_water.xml", pos + rotation * offset, vel, vec3(1.0f));
    }
    for (int i = 0; i < 1; ++i) {
        vec3 offset = GetRandomOffset(scale);
        vec3 vel = rotation * vec3(1, 0, 0) * 2.0f;
        MakeParticle("Data/Particles/falling_water_drops.xml", pos + rotation * offset, vel, vec3(1.0f));
    }
}

vec3 GetRandomOffset(vec3 scale) {
    return vec3(
        RangedRandomFloat(-scale.x * 2.0f, scale.x * 2.0f),
        RangedRandomFloat(-scale.y * 2.0f, scale.y * 2.0f),
        RangedRandomFloat(-scale.z * 2.0f, scale.z * 2.0f)
    );
}
