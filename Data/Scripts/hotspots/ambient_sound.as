//-----------------------------------------------------------------------------
//           Name: ambient_sound.as
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

int sound_handle = -1;
string playing_sound_path;
bool one_shot = false;
float sound_play_time = 0.0f;
float wind_gain = 0.0;

void Init() {
    hotspot.SetCollisionEnabled(false);
}

void SetParameters() {
    params.AddString("Sound Path", "Data/Sounds/filename.wav");
    params.AddIntCheckbox("Global", true);
    params.AddIntCheckbox("Wind Scale", false);
    params.AddFloat("Fade Distance", 1.0f);
    if (params.GetFloat("Fade Distance") < 0.01f) {
        params.SetFloat("Fade Distance", 0.01f);
    }
    params.AddFloatSlider("Gain", 1.0f, "min:0,max:1,step:0.01");
    params.AddFloat("Delay Min", 3.0f);
    params.AddFloat("Delay Max", 10.0f);

    string path = params.GetString("Sound Path");
    if (IsWavFile(path) && FileExists(path)) {
        if (playing_sound_path != path) {
            Stop();
        }
        if (sound_handle == -1) {
            sound_handle = PlaySoundLoop(path, 0.0);
            playing_sound_path = path;
        }
    } else if (IsXmlFile(path) && FileExists(path)) {
        Stop();
        playing_sound_path = path;
        one_shot = true;
    } else {
        Stop();
    }
}

bool IsWavFile(string path) {
    return path.substr(path.length() - 4).toLower() == ".wav";
}

bool IsXmlFile(string path) {
    return path.substr(path.length() - 4).toLower() == ".xml";
}

void Reset() {
    // No implementation needed
}

void Dispose() {
    Stop();
}

void Stop() {
    if (sound_handle != -1) {
        StopSound(sound_handle);
        sound_handle = -1;
    }
}

void PreDraw(float curr_game_time) {
    EnterTelemetryZone("Ambient Sound Update");

    if (sound_handle != -1 && !level.WaitingForInput()) {
        UpdateSoundGain();
    }

    if (one_shot && sound_play_time < the_time) {
        PlaySoundGroup(playing_sound_path, ReadObjectFromID(hotspot.GetID()).GetTranslation());
        sound_play_time = the_time + RangedRandomFloat(params.GetFloat("Delay Min"), params.GetFloat("Delay Max"));
    }

    LeaveTelemetryZone();
}

void UpdateSoundGain() {
    if (params.GetInt("Wind Scale") == 1) {
        UpdateWindScaledGain();
    } else {
        SetSoundGain(sound_handle, GetGain());
    }
}

void UpdateWindScaledGain() {
    float avg_wind_shake = CalculateAverageWindShake();
    wind_gain = mix(avg_wind_shake, wind_gain, 0.99);
    float gain = GetGain() * max(0.4, min(1.0, wind_gain * 3.0));
    SetSoundGain(sound_handle, gain);
}

float CalculateAverageWindShake() {
    float avg_wind_shake = 0.0;
    for (int x = -1; x <= 1; ++x) {
        for (int z = -1; z <= 1; ++z) {
            vec3 test_pos = camera.GetPos() + normalize(vec3(x, 0, z)) * 3.0;
            float amount = WindShakeAmount(test_pos, the_time);
            avg_wind_shake += (x == 0 && z == 0) ? amount * 3.0f : amount;
        }
    }
    return avg_wind_shake / 12.0;
}

float WindShakeAmount(vec3 world_pos, float time) {
    float amount = sin((world_pos.x + world_pos.y) + time * 0.3);
    amount *= sin((world_pos.x * 0.1 + world_pos.z) * 0.3 + time * 0.6) + 1.0;
    return max(0.0f, amount);
}

float GetGain() {
    if (params.GetInt("Global") == 1) {
        return params.GetFloat("Gain");
    } else {
        mat4 transform = ReadObjectFromID(hotspot.GetID()).GetTransform();
        vec3 pos = invert(transform) * camera.GetPos();
        float distance = 0.0f;
        for (int i = 0; i < 3; ++i) {
            distance = max(distance, (pos[i] - 2.0) * ReadObjectFromID(hotspot.GetID()).GetScale()[i]);
            distance = max(distance, -(pos[i] + 2.0) * ReadObjectFromID(hotspot.GetID()).GetScale()[i]);
        }
        return max(0.0, 1.0f - distance / params.GetFloat("Fade Distance")) * params.GetFloat("Gain");
    }
}

void DrawEditor() {
    Object@ obj = ReadObjectFromID(hotspot.GetID());
    DebugDrawBillboard("Data/Textures/ui/speaker.png",
                       obj.GetTranslation(),
                       obj.GetScale()[1] * 2.0,
                       vec4(vec3(0.5), 1.0),
                       _delete_on_draw);
}
