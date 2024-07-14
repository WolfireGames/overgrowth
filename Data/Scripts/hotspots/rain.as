//-----------------------------------------------------------------------------
//           Name: rain.as
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

float lightning_time = -1.0;
float next_lightning_time = -1.0;
float thunder_time = -1.0;
float lightning_distance; // in miles

vec3 old_sun_position;
vec3 old_sun_color;
float old_sun_ambient;

void Init() {
    old_sun_position = GetSunPosition();
    old_sun_color = GetSunColor();
    old_sun_ambient = GetSunAmbient();
}

void Dispose() {
    SetSunAmbient(old_sun_ambient);// + 1.5*flash_amount);
    SetSkyTint(GetBaseSkyTint());
    SetSunColor(old_sun_color);
    SetSunPosition(old_sun_position);
}

void SetParameters() {
    params.AddFloatSlider("Thunder Time Multiplier",1.0f,"min:0.5,max:10,step:0.1");
}

void Update() {
    if(next_lightning_time < the_time){
        next_lightning_time = the_time + RangedRandomFloat(6.0, 12.0) * params.GetFloat("Thunder Time Multiplier");//RangedRandomFloat(3.0, 6.0);
        lightning_distance = RangedRandomFloat(0.0, 1.0);
        thunder_time = the_time + lightning_distance * 3.0;
        lightning_time = the_time;
        SetSunPosition(normalize(vec3(RangedRandomFloat(-1.0, 1.0), RangedRandomFloat(0.5, 1.0), RangedRandomFloat(-1.0, 1.0))));
    }

    if(thunder_time < the_time && thunder_time != -1.0){
        if(lightning_distance < 0.3){
            PlaySoundGroup("Data/Sounds/weather/thunder_strike_mike_koenig.xml", _sound_priority_high);
        } else {
            PlaySoundGroup("Data/Sounds/weather/tapio/thunder.xml", _sound_priority_high);
        }
        thunder_time = -1.0;
    }

    if(lightning_time <= the_time){
        float flash_amount = min(1.0, max(0.0, 1.0 + (lightning_time - the_time) * 0.1));
        SetSunAmbient(1.5);// + 1.5*flash_amount);
        flash_amount = min(1.0, max(0.0, 1.0 + (lightning_time - the_time) * 2.0));
        flash_amount *= RangedRandomFloat(0.8,1.2);
        flash_amount *= 3.0;
        SetSkyTint(mix(GetBaseSkyTint() * 0.7, vec3(3.0), flash_amount));
        SetSunColor(vec3(flash_amount) * 4.0);
        SetFlareDiffuse(4.0);
    }

}