//-----------------------------------------------------------------------------
//           Name: goal_level.as
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
#include "lugaru_campaign.as"

int progress = 0;
bool queued_goal_check = true;

float blackout_amount = 0.0;
float ko_time = -1.0;
float win_time = -1.0;
int win_target = 0;
bool sent_level_complete_message = false;
int curr_music_layer = 0;

void SetParameters() {
    params.AddString("music", "");
    params.AddString("player_spawn", "");
}

void Init() {
    curr_music_layer = 0;
    level.ReceiveLevelEvents(hotspot.GetID());
}

void Dispose() {
    level.StopReceivingLevelEvents(hotspot.GetID());
}

void TriggerGoalString(const string &in goal_str){
    TokenIterator token_iter;
    token_iter.Init();
    if(token_iter.FindNextToken(goal_str)){
        if(token_iter.GetToken(goal_str) == "dialogue" && !EditorModeActive()){
            if(token_iter.FindNextToken(goal_str)){
                level.SendMessage("start_dialogue \""+token_iter.GetToken(goal_str)+"\"");
            }
        }
    }    
}

void TriggerGoalPre() {
    if(params.HasParam("goal_"+progress+"_pre")){
        TriggerGoalString(params.GetString("goal_"+progress+"_pre"));
    }
}

void TriggerGoalPost() {
    if(params.HasParam("goal_"+progress+"_post")){
        TriggerGoalString(params.GetString("goal_"+progress+"_post"));
    }
}

void IncrementProgress() {
    TriggerGoalPost();
    ++progress;
    win_time = -1.0;

    TriggerGoalPre();

    if(params.HasParam("goal_"+progress)){
        string goal_str = params.GetString("goal_"+progress);
        TokenIterator token_iter;
        token_iter.Init();
        if(token_iter.FindNextToken(goal_str)){
            if(token_iter.GetToken(goal_str) == "spawn_defeat"){
                while(token_iter.FindNextToken(goal_str)){
                    int id = atoi(token_iter.GetToken(goal_str));
                    if(ObjectExists(id) && ReadObjectFromID(id).GetType() == _movement_object){
                        SetEnabledCharacterAndItems(id, true);
                    }
                }
            }
        }
    }


    // Place player character at correct spawn point
    array<int> player_ids = GetControlledCharacterIDs();
    for(uint i = 0; i < player_ids.length(); i++) {
        MovementObject@ mo = ReadCharacter(player_ids[i]);
        mo.ReceiveMessage("restore_health");
    }
}

void SetEnabledCharacterAndItems(int id, bool enabled){
    ReadObjectFromID(id).SetEnabled(enabled);
    MovementObject@ char = ReadCharacterID(id);
    for(int item_index=0; item_index<6; ++item_index){
        int item_id = char.GetArrayIntVar("weapon_slots",item_index);
        if(item_id != -1 && ObjectExists(item_id)){
            ReadObjectFromID(item_id).SetEnabled(enabled);                                    
        }
    }
}

void CheckReset() {
    // Count valid player spawn points, and set their preview viz
    TokenIterator token_iter;
    int num_player_spawn = 0;
    if(params.HasParam("player_spawn")){
        string param_str = params.GetString("player_spawn");
        token_iter.Init();
        while(token_iter.FindNextToken(param_str)){
            int id = atoi(token_iter.GetToken(param_str));
            if(ObjectExists(id)){
                Object@ obj = ReadObjectFromID(id);
                if(obj.GetType() == _placeholder_object){
                    PlaceholderObject@ placeholder_object = cast<PlaceholderObject@>(obj);
                    placeholder_object.SetPreview("Data/Objects/therium/characters/ghostobj.xml");
                    ++num_player_spawn;
                }
            }
        }
    }

    // Cannot respawn at progress points with no spawn point
    if(progress > num_player_spawn){
        progress = num_player_spawn;
    }

    // Re-enable all characters
    int num_characters = GetNumCharacters();
    for(int i=0; i<num_characters; ++i){
        SetEnabledCharacterAndItems(ReadCharacter(i).GetID(), true);
    }

    // Disable all defeated characters
    for(int i=0; i<progress; ++i){
        Print("Iterating through completed goal: "+i+"\n");
        if(params.HasParam("goal_"+i)){
            string goal_str = params.GetString("goal_"+i);
            token_iter.Init();
            if(token_iter.FindNextToken(goal_str)){
                if(token_iter.GetToken(goal_str) == "defeat" || token_iter.GetToken(goal_str) == "spawn_defeat"){
                    while(token_iter.FindNextToken(goal_str) && token_iter.GetToken(goal_str) != ""){
                        int id = atoi(token_iter.GetToken(goal_str));
                        if(ObjectExists(id) && ReadObjectFromID(id).GetType() == _movement_object){
                            SetEnabledCharacterAndItems(id, false);
                        }
                    }
                }
            }
        }
    }

    // Disable all characters that have not been spawned yet
    for(int i=progress+1; params.HasParam("goal_"+i); ++i){
        Print("Iterating through future goals: "+i+"\n");
        if(params.HasParam("goal_"+i)){
            string goal_str = params.GetString("goal_"+i);
            Print("Goal_str: "+goal_str+"\n");
            token_iter.Init();
            if(token_iter.FindNextToken(goal_str)){
                if(token_iter.GetToken(goal_str) == "spawn_defeat"){
                    while(token_iter.FindNextToken(goal_str)){
                        int id = atoi(token_iter.GetToken(goal_str));
                        if(ObjectExists(id) && ReadObjectFromID(id).GetType() == _movement_object){
                            Print("Disabling: "+id+"\n");
                            SetEnabledCharacterAndItems(id, false);
                        }
                    }
                }
            }
        }
    }

    // Place player character at correct spawn point
    array<int> player_ids = GetControlledCharacterIDs();
    for(uint i = 0; i < player_ids.length(); i++) {
        MovementObject@ mo = ReadCharacter(player_ids[i]);
        if(progress == 0){ // Spawn at actual initial player spawn point
            Object@ obj = ReadObjectFromID(mo.GetID());
            mo.position = obj.GetTranslation();
            mo.SetRotationFromFacing(obj.GetRotation() * vec3(0,0,1));
        } else { // Spawn at a custom spawn point
            string param_str = params.GetString("player_spawn");
            token_iter.Init();
            for(int j=0; j<progress; ++j){
                token_iter.FindNextToken(param_str);
            }
            int id = atoi(token_iter.GetToken(param_str));
            if(ObjectExists(id)){
                Object@ obj = ReadObjectFromID(id);
                if(obj.GetType() == _placeholder_object){
                    mo.position = obj.GetTranslation();
                    mo.SetRotationFromFacing(obj.GetRotation() * vec3(0,0,1));
                    mo.Execute("SetCameraFromFacing();");
                }
            }
        }
    }

    // Trigger whatever happens at the start of this goal
    TriggerGoalPre();
}

void PossibleWinEvent(const string &in event, int val, int goal_check){
    if(event == "checkpoint"){
        Print("Player entered checkpoint: "+val+"\n");
        if(params.HasParam("goal_"+goal_check)){
            string goal_str = params.GetString("goal_"+goal_check);
            TokenIterator token_iter;
            token_iter.Init();
            if(token_iter.FindNextToken(goal_str)){
                string goal_type = token_iter.GetToken(goal_str);
                if(goal_type == "reach" || goal_type == "reach_skippable"){
                    if(token_iter.FindNextToken(goal_str)){
                        int id = atoi(token_iter.GetToken(goal_str));
                        if(id == val){
                            win_time = the_time + 1.0;
                            win_target = goal_check+1;
                        } else if(goal_type == "reach_skippable") {
                            PossibleWinEvent(event, val, goal_check+1);
                        }
                    }
                }
            }
        }
    } else if(event == "character_defeated"){
        Print("Character defeated, checking goal\n");
        if(params.HasParam("goal_"+goal_check)){
            string goal_str = params.GetString("goal_"+goal_check);
            Print("Goal_str: "+goal_str+"\n");
            TokenIterator token_iter;
            token_iter.Init();
            if(token_iter.FindNextToken(goal_str)){
                if(token_iter.GetToken(goal_str) == "defeat" || token_iter.GetToken(goal_str) == "spawn_defeat"){
                    Print("Checking defeat conditions\n");
                    bool success = true;
                    while(token_iter.FindNextToken(goal_str) && token_iter.GetToken(goal_str) != ""){
                        Print("Looking at token \""+token_iter.GetToken(goal_str)+"\"\n");
                        int id = atoi(token_iter.GetToken(goal_str));
                        if(ObjectExists(id) && ReadObjectFromID(id).GetType() == _movement_object && ReadCharacterID(id).GetIntVar("knocked_out") == _awake){
                            success = false;
                            Print("Conditions failed, "+id+" is awake\n");
                        }
                    }
                    if(success){
                        win_time = the_time + 3.0;
                        win_target = goal_check+1;
                    }
                }
            }
        }
    }
}

void ReceiveMessage(string msg) {
    //Print("Received message: "+msg+"\n");
    TokenIterator token_iter;
    token_iter.Init();
    if(!token_iter.FindNextToken(msg)){
        return;
    }
    string msg_start = token_iter.GetToken(msg);
    if(msg_start == "reset"){
        queued_goal_check = true;
        ko_time = -1.0;
        win_time = -1.0;
    }

    if(win_time == -1.0){
        if(msg_start == "player_entered_checkpoint"){
            if(token_iter.FindNextToken(msg)){
                int checkpoint_id = atoi(token_iter.GetToken(msg));
                PossibleWinEvent("checkpoint", checkpoint_id, progress);
            }
        }
        if(msg_start == "level_event" &&
           token_iter.FindNextToken(msg) &&
           (token_iter.GetToken(msg) == "character_knocked_out" || token_iter.GetToken(msg) == "character_died"))
        {
            PossibleWinEvent("character_defeated", -1, progress);
        }
    }
}


void Update() {
    if(queued_goal_check){
        CheckReset();
        queued_goal_check = false;
    }

    int player_id = GetPlayerCharacterID();

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
                    level.SendMessage("skip_dialogue");                 
				}
			}
            blackout_amount = 0.2 + 0.6 * (1.0 - pow(0.5, (the_time - ko_time)));
		} else {
			ko_time = -1.0f;
		}
	} else {
        ko_time = -1.0f;
    }

    if(win_time != -1.0 && the_time > win_time && ko_time == -1.0){
        while(progress < win_target){
            IncrementProgress();
        }
    }

    int num_player_spawn = 0;
    if(params.HasParam("player_spawn")){
        string param_str = params.GetString("player_spawn");
        TokenIterator token_iter;
        token_iter.Init();
        while(token_iter.FindNextToken(param_str)){
            int id = atoi(token_iter.GetToken(param_str));
            if(ObjectExists(id)){
                Object@ obj = ReadObjectFromID(id);
                if(obj.GetType() == _placeholder_object){
                    PlaceholderObject@ placeholder_object = cast<PlaceholderObject@>(obj);
                    placeholder_object.SetPreview("Data/Objects/therium/characters/ghostobj.xml");
                    ++num_player_spawn;
                }
            }
        }
    }

    if(GetInputPressed(0, "k")){
        ++progress;
        if(!params.HasParam("goal_"+progress)){
            progress = 0;
        }
    }
    /*
    if(GetInputPressed(0, "i")){
        ++curr_music_layer;
        if(curr_music_layer >= int(music_layer.size())){
            curr_music_layer = 0;
        }
    }*/
    DebugText("Progress", "Progress: "+progress, 2.0f);
}

void PreDraw(float curr_game_time) {
    camera.SetTint(camera.GetTint() * (1.0 - blackout_amount));
}

void Draw(){
    if(EditorModeActive()){
        Object@ obj = ReadObjectFromID(hotspot.GetID());
        DebugDrawBillboard("Data/Textures/ui/ogicon.png",
                           obj.GetTranslation(),
                           obj.GetScale()[1]*2.0,
                           vec4(vec3(0.5), 1.0),
                           _delete_on_draw);
    }
}
