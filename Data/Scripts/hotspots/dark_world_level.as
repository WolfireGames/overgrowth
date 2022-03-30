//-----------------------------------------------------------------------------
//           Name: dark_world_level.as
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

#include "threatcheck.as"

bool dark_world = true;
bool initialized = false;

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

void Dispose() {
}

void Update() {
    if(!initialized){
        SetDark(false);
        SetDark(true);
        initialized = true;
    }
}

float last_time = 0.0;

void PreDraw(float curr_game_time) {
    if(target_vis_params.size() != 0){
        if(vis_params.size() == 0){
            for(int i=0, len=target_vis_params.size(); i<len; ++i){
                vis_params.push_back(target_vis_params[i]);
            }
        } else {
            float time_step = curr_game_time - last_time;
            for(int i=0, len=target_vis_params.size(); i<len; ++i){
                vis_params[i] = mix(target_vis_params[i], vis_params[i], pow(0.1, time_step));
            }

            SetHDRWhitePoint(vis_params[kWhitePoint]);
            SetHDRBlackPoint(vis_params[kBlackPoint]);
            SetHDRBloomMult(vis_params[kBloomMult]);
            SetSunAmbient(vis_params[kAmbient]);
            SetSkyTint(vec3(vis_params[kSkyTintr],vis_params[kSkyTintg],vis_params[kSkyTintb]));
        }
    }
    last_time = curr_game_time;
}

void SetDark(bool val){
    if(dark_world != val){
        target_vis_params.resize(kNumVisParam);
        dark_world = val;

        for(int dark=0; dark<2; ++dark){
            string obj_list;
            if(dark == 0){
                obj_list = params.GetString("light_world_only");
            } else {
                obj_list = params.GetString("dark_world_only");                
            }

            TokenIterator token_iter;
            token_iter.Init();
            while(token_iter.FindNextToken(obj_list)){
                string token = token_iter.GetToken(obj_list);
                int target_id = atoi(token);
                if(ObjectExists(target_id)){
                    Object @obj = ReadObjectFromID(target_id);
                    if(dark == 0){
                        obj.SetEnabled(!dark_world);
                    } else {
                        obj.SetEnabled(dark_world);
                    }
                }
            }
        }

        if(!dark_world){
            target_vis_params[kWhitePoint] = 0.8;
            target_vis_params[kBlackPoint] = 0.04;
            target_vis_params[kBloomMult] = 2.0;
            target_vis_params[kAmbient] = 2.0;
            target_vis_params[kSkyTintr] = 1.0;
            target_vis_params[kSkyTintg] = 1.0;
            target_vis_params[kSkyTintb] = 1.0;
            PlaySong("light");
            if(GetPlayerCharacterID() != -1){
                MovementObject@ mo = ReadCharacter(GetPlayerCharacterID());
                mo.Execute("SwitchCharacter(\"Data/Characters/pale_turner.xml\");");
            }
        } else {
            target_vis_params[kWhitePoint] = 1.0;
            target_vis_params[kBlackPoint] = 0.3;
            target_vis_params[kBloomMult] = 1.0;
            target_vis_params[kAmbient] = 0.5;
            target_vis_params[kSkyTintr] = 1.0;
            target_vis_params[kSkyTintg] = 1.0;
            target_vis_params[kSkyTintb] = 1.0;
            PlaySong("dark");
            if(GetPlayerCharacterID() != -1){
                MovementObject@ mo = ReadCharacter(GetPlayerCharacterID());
                mo.Execute("SwitchCharacter(\"Data/Characters/rabbot.xml\");");
            }
        }
    }
}

void ReceiveMessage(string msg) {
    if(msg == "trigger_enter"){
        SetDark(!dark_world);
    }
    if(msg == "reset"){
        SetDark(true);
    }
}

void Draw(){
    if(EditorModeActive()){
        Object@ obj = ReadObjectFromID(hotspot.GetID());
        DebugDrawBillboard("Data/Textures/ui/eclipse.tga",
                           obj.GetTranslation(),
                           obj.GetScale()[1]*2.0,
                           vec4(vec3(0.5), 1.0),
                           _delete_on_draw);
    }
}
