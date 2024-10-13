//-----------------------------------------------------------------------------
//           Name: rain.as
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

float lightning_time = -1.0f;
float next_lightning_time = -1.0f;
float thunder_time = -1.0f;
float lightning_distance = 0.0f;

vec3 original_sun_position;
vec3 original_sun_color;
float original_sun_ambient;

void Init() {
    original_sun_position = GetSunPosition();
    original_sun_color = GetSunColor();
    original_sun_ambient = GetSunAmbient();
}

void Dispose() {
    RestoreSunSettings();
}

void Update() {
    if (next_lightning_time < the_time) {
        ScheduleNextLightning();
    }
    if (thunder_time != -1.0f && thunder_time < the_time) {
        PlayThunderSound();
        thunder_time = -1.0f;
    }
    if (lightning_time <= the_time) {
        UpdateLightningEffects();
    }
}

void ScheduleNextLightning() {
    next_lightning_time = the_time + RangedRandomFloat(6.0f, 12.0f);
    lightning_distance = RangedRandomFloat(0.0f, 1.0f);
    thunder_time = the_time + lightning_distance * 3.0f;
    lightning_time = the_time;
    SetSunPosition(RandomLightningPosition());
}

vec3 RandomLightningPosition() {
    float x = RangedRandomFloat(-1.0f, 1.0f);
    float y = RangedRandomFloat(0.5f, 1.0f);
    float z = RangedRandomFloat(-1.0f, 1.0f);
    return normalize(vec3(x, y, z));
}

void PlayThunderSound() {
    if (lightning_distance < 0.3f) {
        PlaySoundGroup("Data/Sounds/weather/thunder_strike_mike_koenig.xml");
    } else {
        PlaySoundGroup("Data/Sounds/weather/tapio/thunder.xml");
    }
}

void UpdateLightningEffects() {
    float flash_amount = Clamp(1.0f + (lightning_time - the_time) * 0.1f, 0.0f, 1.0f);
    SetSunAmbient(1.5f);

    flash_amount = Clamp(1.0f + (lightning_time - the_time) * 2.0f, 0.0f, 1.0f);
    flash_amount *= RangedRandomFloat(0.8f, 1.2f) * 3.0f;

    vec3 sky_tint = mix(GetBaseSkyTint() * 0.7f, vec3(3.0f), flash_amount);
    SetSkyTint(sky_tint);

    SetSunColor(vec3(flash_amount) * 4.0f);
    SetFlareDiffuse(4.0f);
}

void RestoreSunSettings() {
    SetSunAmbient(original_sun_ambient);
    SetSkyTint(GetBaseSkyTint());
    SetSunColor(original_sun_color);
    SetSunPosition(original_sun_position);
}

float Clamp(float value, float min_value, float max_value) {
    return min(max(value, min_value), max_value);
}
