//-----------------------------------------------------------------------------
//           Name: lugaru_level.as
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
#include "campaign_common.as"

string death_message = "Press $attack$ to try again";

void SetParameters() {
	//params.AddString("next_level", "");
    params.AddString("death_message", death_message);
    params.AddIntCheckbox("press_attack_to_restart", true);
    params.AddString("music", "lugaru_ambient_grass");
}

void Init() {
    AddMusic("Data/Music/lugaru_new.xml");
}

void Dispose() {
}

void ReceiveMessage(string msg) {
    Log(info, "Getting msg in lugaru: " + msg );
    TokenIterator token_iter;
    token_iter.Init();
    if(!token_iter.FindNextToken(msg)){
        return;
    }
    string token = token_iter.GetToken(msg);

    if(token == "levelwin" ) {
    }
}

float blackout_amount = 0.0;
float ko_time = -1.0;
float win_time = -1.0;
bool sent_level_complete_message = false;

void Update() {
    int player_id = GetPlayerCharacterID();
    if(player_id != -1 && ReadCharacter(player_id).QueryIntFunction("int CombatSong()") == 1 && ReadCharacter(player_id).GetIntVar("knocked_out") == _awake){
        PlaySong("lugaru_combat");
    } else if(params.HasParam("music")){
        string song= params.GetString("music");
        if( song != "" ) {
            PlaySong(song);
        }
    }

	blackout_amount = 0.0;
	if(player_id != -1 && ObjectExists(player_id)){
		MovementObject@ char = ReadCharacter(player_id);
		if(char.GetIntVar("knocked_out") != _awake){
			if(ko_time == -1.0f){
				ko_time = the_time;
			}
			if(ko_time < the_time - 1.0){
				if(params.GetInt("press_attack_to_restart") == 1 && (GetInputPressed(0, "attack") || ko_time < the_time - 5.0)){
	            	level.SendMessage("reset"); 				
                    level.SendMessage("skip_dialogue");                 
				}
			}
            blackout_amount = 0.2 + 0.6 * (1.0 - pow(0.5, (the_time - ko_time)));
		} else {
			ko_time = -1.0f;
		}

        bool use_keyboard = (max(last_mouse_event_time, last_keyboard_event_time) > last_controller_event_time);
        string respawn = params.GetString("death_message");
        int index = respawn.findFirst("$attack$");
        while(index != -1) {
            respawn.erase(index, 8);
            respawn.insert(index, GetStringDescriptionForBinding(use_keyboard?"key":"gamepad_0", "attack"));

            index = respawn.findFirst("$attack$", index + 8);
        }
        level.SendMessage("screen_message "+""+respawn);
	} else {
        ko_time = -1.0f;
    }
	if(ThreatsRemaining() == 0 && ThreatsPossible() != 0 && ko_time == -1.0){
		if(win_time == -1.0f){
			win_time = the_time;
		}
		if(win_time < the_time - 5.0 && !sent_level_complete_message){
            SendGlobalMessage("levelwin"); 
            Log(info, "Sending level win");
            sent_level_complete_message = true; 
	    }
	} else {
        win_time = -1.0;
    }
}

void PreDraw(float curr_game_time) {
    camera.SetTint(camera.GetTint() * (1.0 - blackout_amount));
}

void DrawEditor(){
    Object@ obj = ReadObjectFromID(hotspot.GetID());
    DebugDrawBillboard("Data/Textures/ui/lugaru_icns_256x256.png",
                       obj.GetTranslation(),
                       obj.GetScale()[1]*2.0,
                       vec4(vec3(0.5), 1.0),
                       _delete_on_draw);
}
