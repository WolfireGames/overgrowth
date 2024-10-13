//-----------------------------------------------------------------------------
//           Name: explosive_generic.as
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

int fire_object_id = -1;
float explode_time = -1.0f;

void Init() {
    // No initialization needed
}

void SetParameters() {
    params.AddIntSlider("Smoke particle amount", 5, "min:0,max:15");
    params.AddIntSlider("Sound", 1, "min:1,max:3");
}

void HandleEvent(string event, MovementObject@ mo) {
    if (event == "enter") {
        OnEnter(mo);
    }
}

void ReceiveMessage(string msg) {
    TokenIterator token_iter;
    token_iter.Init();

    if (!token_iter.FindNextToken(msg)) {
        return;
    }
    string token = token_iter.GetToken(msg);
    if (token == "reset") {
        Dispose();
    }
}

void Dispose() {
    if (fire_object_id != -1) {
        QueueDeleteObjectID(fire_object_id);
        fire_object_id = -1;
    }
}

void PreDraw(float curr_game_time) {
    if (fire_object_id == -1) {
        return;
    }

    vec3 explosion_point = hotspot.GetTranslation();
    Object@ fire_obj = ReadObjectFromID(fire_object_id);
    fire_obj.SetTranslation(explosion_point);

    float intensity_fade = pow(clamp((explode_time - curr_game_time) * 0.5f + 1.0f, 0.0f, 1.0f), 2.0f);
    float intensity = mix(0.02f * (sin(curr_game_time * 2.4f) + sin(curr_game_time * 1.5f) + 3.0f), 1.0f, intensity_fade);
    fire_obj.SetTint(vec3(2.0f, 1.0f, 0.0f) * 1000.0f * intensity);
    fire_obj.SetScale(30.0f);
}

void OnEnter(MovementObject@ mo) {
    vec3 explosion_point = hotspot.GetTranslation();
    explode_time = the_time;

    if (fire_object_id == -1) {
        fire_object_id = CreateObject("Data/Objects/default_light.xml", true);
    }

    MakeMetalSparks(explosion_point);
    CreateSmokeParticles(mo.position, params.GetFloat("Smoke particle amount"));
    PlaySound("Data/Sounds/explosives/explosion" + params.GetInt("Sound") + ".wav");
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
