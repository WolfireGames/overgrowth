//-----------------------------------------------------------------------------
//           Name: dark_world_level.as
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

#include "threatcheck.as"

bool dark_world = true;
bool initialized = false;
float last_time = 0.0f;

enum VisParam {
    kWhitePoint,
    kBlackPoint,
    kBloomMult,
    kAmbient,
    kSkyTintr,
    kSkyTintg,
    kSkyTintb,
    kNumVisParam
};

array<float> vis_params;
array<float> target_vis_params;

void SetParameters() {
    params.AddInt("light_world_only", -1);
    params.AddInt("dark_world_only", -1);
}

void Init() {
    AddMusic("Data/Music/dark_world.xml");
    initialized = false;
}

void Update() {
    if (initialized) {
        return;
    }
    InitializeDarkWorld();
    initialized = true;
}

void InitializeDarkWorld() {
    SetDark(false);
    SetDark(true);
}

void PreDraw(float curr_game_time) {
    if (target_vis_params.size() == 0) {
        return;
    }
    if (vis_params.size() == 0) {
        vis_params = target_vis_params;
    } else {
        UpdateVisualParameters(curr_game_time);
    }
    last_time = curr_game_time;
}

void UpdateVisualParameters(float curr_game_time) {
    float time_step = curr_game_time - last_time;
    float mix_factor = pow(0.1, time_step);
    for (int i = 0, len = target_vis_params.size(); i < len; ++i) {
        vis_params[i] = mix(target_vis_params[i], vis_params[i], mix_factor);
    }
    ApplyVisualSettings();
}

void ApplyVisualSettings() {
    SetHDRWhitePoint(vis_params[kWhitePoint]);
    SetHDRBlackPoint(vis_params[kBlackPoint]);
    SetHDRBloomMult(vis_params[kBloomMult]);
    SetSunAmbient(vis_params[kAmbient]);
    SetSkyTint(vec3(vis_params[kSkyTintr], vis_params[kSkyTintg], vis_params[kSkyTintb]));
}

void SetDark(bool val) {
    if (dark_world == val) {
        return;
    }
    dark_world = val;
    target_vis_params.resize(kNumVisParam);
    ToggleObjectVisibility();
    UpdateVisualSettings();
    UpdatePlayerCharacter();
}

void ToggleObjectVisibility() {
    for (int dark = 0; dark < 2; ++dark) {
        string obj_list = (dark == 0) ? params.GetString("light_world_only") : params.GetString("dark_world_only");
        TokenIterator token_iter;
        token_iter.Init();
        while (token_iter.FindNextToken(obj_list)) {
            int target_id = atoi(token_iter.GetToken(obj_list));
            if (ObjectExists(target_id)) {
                Object@ obj = ReadObjectFromID(target_id);
                obj.SetEnabled(dark == 1 ? dark_world : !dark_world);
            }
        }
    }
}

void UpdateVisualSettings() {
    if (!dark_world) {
        SetLightWorldVisuals();
        PlaySong("light");
    } else {
        SetDarkWorldVisuals();
        PlaySong("dark");
    }
}

void SetLightWorldVisuals() {
    target_vis_params[kWhitePoint] = 0.8;
    target_vis_params[kBlackPoint] = 0.04;
    target_vis_params[kBloomMult] = 2.0;
    target_vis_params[kAmbient] = 2.0;
    target_vis_params[kSkyTintr] = 1.0;
    target_vis_params[kSkyTintg] = 1.0;
    target_vis_params[kSkyTintb] = 1.0;
}

void SetDarkWorldVisuals() {
    target_vis_params[kWhitePoint] = 1.0;
    target_vis_params[kBlackPoint] = 0.3;
    target_vis_params[kBloomMult] = 1.0;
    target_vis_params[kAmbient] = 0.5;
    target_vis_params[kSkyTintr] = 1.0;
    target_vis_params[kSkyTintg] = 1.0;
    target_vis_params[kSkyTintb] = 1.0;
}

void UpdatePlayerCharacter() {
    int player_id = GetPlayerCharacterID();
    if (player_id == -1) {
        return;
    }
    MovementObject@ mo = ReadCharacter(player_id);
    if (!dark_world) {
        mo.Execute("SwitchCharacter(\"Data/Characters/pale_turner.xml\");");
    } else {
        mo.Execute("SwitchCharacter(\"Data/Characters/rabbot.xml\");");
    }
}

void ReceiveMessage(string msg) {
    if (msg == "trigger_enter") {
        SetDark(!dark_world);
    } else if (msg == "reset") {
        SetDark(true);
    }
}

void Draw() {
    if (!EditorModeActive()) {
        return;
    }
    Object@ obj = ReadObjectFromID(hotspot.GetID());
    DebugDrawBillboard("Data/Textures/ui/eclipse.tga",
                       obj.GetTranslation(),
                       obj.GetScale()[1] * 2.0,
                       vec4(vec3(0.5), 1.0),
                       _delete_on_draw);
}
