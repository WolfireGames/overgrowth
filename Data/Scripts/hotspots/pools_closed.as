//-----------------------------------------------------------------------------
//           Name: pools_closed.as
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

void Init() {
}

void SetParameters() {
    params.AddFloatSlider("Time",10.0f,"min:0.1,max:50.0,step:1.0,text_mult:1");
    params.AddString("Text", "You're not too smart...");
}

void HandleEvent(string event, MovementObject @mo){
    if(event == "enter"){
        OnEnter(mo);
    } else if(event == "exit"){
        OnExit(mo);
    }
}

float drown_timer = 0.0f;
bool triggered = false;
int victim_id = -1;
int font_size = 70;

void Update(){
    if(victim_id != -1){
        if(GetInputDown(ReadCharacterID(victim_id).controller_id, "crouch")){
            drown_timer += time_step;
            if(drown_timer > params.GetFloat("Time")){
                victim_id = -1;
                drown_timer = 0.0f;
                triggered = true;
                level.SendMessage("displaytext \""+params.GetString("Text")+"\"");
            }
        }else{
            drown_timer = 0.0f;
        }
    }
}

void Reset(){
    if(triggered){
        level.SendMessage("cleartext");
        triggered = false;
    }
}

void OnExit(MovementObject @mo) {
    if(mo.GetID() == victim_id){
        drown_timer = 0.0f;
        victim_id = -1;
    }
}

void OnEnter(MovementObject @mo) {
    victim_id = mo.GetID();
}
