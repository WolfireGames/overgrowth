//-----------------------------------------------------------------------------
//           Name: folticliff_level.as
//      Developer: Wolfire Games LLC
//    Script Type: Level
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

void SetParameters() {
    params.AddString("music", "folticliff");
}

string music_prefix;
void Init() {
    if(params.GetString("music") == "folticliff"){
        AddMusic("Data/Music/folticliff.xml");
	}
    music_prefix = params.GetString("music") + "_";

	bool postinit = false;
    level.ReceiveLevelEvents(hotspot.GetID());
}
bool postinit = false;
void PostInit(){
    AddMusic(params.GetString("music"));
    postinit = true;
}

void Dispose() {
    level.StopReceivingLevelEvents(hotspot.GetID());
}


float blackout_amount = 0.0;
float ko_time = -1.0;

void Update() {
    int player_id = GetPlayerCharacterID();
    PlaySong(music_prefix + "ambient-tense");

    if(!postinit){
        PostInit();
    }
	
	blackout_amount = 0.0;
	if(player_id != -1 && ObjectExists(player_id)){
		MovementObject@ char = ReadCharacter(player_id);
		if(char.GetIntVar("knocked_out") != _awake){
			if(ko_time == -1.0f){
				ko_time = the_time;
			}
			if(ko_time < the_time - 1.0){
				if(GetInputPressed(0, "attack") || ko_time < the_time - 5.0){
	            	level.SendMessage("reset"); 				               
				}
			}
            blackout_amount = 0.2 + 0.6 * (1.0 - pow(0.5, (the_time - ko_time)));
		} else {
			ko_time = -1.0f;
		}
	} else {
        ko_time = -1.0f;
    }
}

array<float> dof_params;

void ReceiveMessage(string msg) {
    TokenIterator token_iter;
    token_iter.Init();

    if(!token_iter.FindNextToken(msg)) {
        return;
    }

    string token = token_iter.GetToken(msg);

    if(token == "therium2_enable_object") {
        if(!token_iter.FindNextToken(msg)) {
            Log(warning, "therium2_enable_object - missing required parameters. Syntax is: \"therium2_enable_object object_id\"");
            return;
        }

        int object_id = atoi(token_iter.GetToken(msg));

        HandleSetEnabledMessage("therium2_enable_object", object_id, true);
    } else if(token == "therium2_disable_object") {
        if(!token_iter.FindNextToken(msg)) {
            Log(warning, "therium2_disable_object missing required parameters. Syntax is: \"therium2_disable_object object_id\"");
            return;
        }

        int object_id = atoi(token_iter.GetToken(msg));

        HandleSetEnabledMessage("therium2_disable_object", object_id, false);
    } else if(token == "level_event" &&
       token_iter.FindNextToken(msg))
    {
        string sub_msg = token_iter.GetToken(msg);
        if(sub_msg == "set_camera_dof"){
            dof_params.resize(0);
            while(token_iter.FindNextToken(msg)){
                dof_params.push_back(atof(token_iter.GetToken(msg)));
            }
            if(dof_params.size() == 6){
                camera.SetDOF(dof_params[0], dof_params[1], dof_params[2], dof_params[3], dof_params[4], dof_params[5]);
            }
        }
        if(sub_msg == "therium2_enable_object") {
            if(!token_iter.FindNextToken(msg)) {
                Log(warning, "therium2_enable_object - missing required parameters. Syntax is: \"therium2_enable_object object_id\"");
                return;
            }

            int object_id = atoi(token_iter.GetToken(msg));

            HandleSetEnabledMessage("therium2_enable_object", object_id, true);
        } else if(sub_msg == "therium2_disable_object") {
            if(!token_iter.FindNextToken(msg)) {
                Log(warning, "therium2_disable_object missing required parameters. Syntax is: \"therium2_disable_object object_id\"");
                return;
            }

            int object_id = atoi(token_iter.GetToken(msg));

            HandleSetEnabledMessage("therium2_disable_object", object_id, false);
        }
    }
}

void HandleSetEnabledMessage(string message_name, int object_id, bool is_enabled) {
    if(object_id == -1 || !ObjectExists(object_id)) {
        Log(warning, message_name + " - unable to find object with id: " + object_id);
        return;
    }

    Object@ obj = ReadObjectFromID(object_id);

    if(obj.GetEnabled() == is_enabled) {
        Log(warning, message_name + " - object id: " + object_id + " - was already in desired state is_enabled: " + is_enabled);
    }

    obj.SetEnabled(is_enabled);
    Log(info, message_name + " - set object id: " + object_id + " - is_enabled: " + is_enabled);
}

void PreDraw(float curr_game_time) {
    camera.SetTint(camera.GetTint() * (1.0 - blackout_amount));
}

void Draw(){
    if(EditorModeActive()){
        Object@ obj = ReadObjectFromID(hotspot.GetID());
        DebugDrawBillboard("Data/Textures/therium/logo256.png",
                           obj.GetTranslation(),
                           obj.GetScale()[1]*2.0,
                           vec4(vec3(0.5), 1.0),
                           _delete_on_draw);
    }
}
