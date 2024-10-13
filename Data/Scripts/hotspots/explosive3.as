//-----------------------------------------------------------------------------
//           Name: explosive3.as
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

void Init() {
    // No initialization needed
}

void SetParameters() {
    params.AddString("Smoke particle amount", "5.0");
}

void HandleEvent(string event, MovementObject@ mo) {
    if (event == "enter") {
        OnEnter(mo);
    }
}

void OnEnter(MovementObject@ mo) {
    vec3 explosion_point = hotspot.GetTranslation();
    MakeMetalSparks(explosion_point);
    CreateSmokeParticles(mo.position, params.GetFloat("Smoke particle amount"));
    PlaySound("Data/Sounds/explosives/explosion3.wav");
}

void CreateSmokeParticles(vec3 position, float amount) {
    float speed = 5.0f;
    for (int i = 0; i < int(amount); ++i) {
        vec3 velocity = RandomVector(-speed, speed);
        MakeParticle("Data/Particles/explosion_smoke.xml", position, velocity);
    }
}

void MakeMetalSparks(vec3 pos) {
    int num_sparks = 60;
    float speed = 20.0f;
    for (int i = 0; i < num_sparks; ++i) {
        vec3 velocity = RandomVector(-speed, speed);
        MakeParticle("Data/Particles/explosion_fire.xml", pos, velocity);
    }
}

vec3 RandomVector(float min, float max) {
    return vec3(
        RangedRandomFloat(min, max),
        RangedRandomFloat(min, max),
        RangedRandomFloat(min, max)
    );
}
