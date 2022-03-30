//-----------------------------------------------------------------------------
//           Name: general_campaign_level.as
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

string campaign_name = "my_simple_campaign";
string current_level_path;
array<string> level_list;

void SetParameters() {
	params.AddString("next_level", "");
}

void Init() {
    campaign_name = GetCurrCampaignID();
    current_level_path = GetCurrLevelRelPath();
    if(campaign_name == ""){
        Log(error, "Did not find an active campaign!");
    }else{
        LoadModCampaign();
        Log(info, "Found " + level_list.size() + " levels!");
    }
    SavedLevel @level = save_file.GetSavedLevel(campaign_name);
    level.SetValue("current_level",current_level_path);
    save_file.WriteInPlace();
}

void LoadModCampaign() {
    level_list.removeRange(0, level_list.length());
    array<ModID>@ active_sids = GetActiveModSids();
    for( uint i = 0; i < active_sids.length(); i++ ) {
        if( ModGetID(active_sids[i]) == campaign_name ) {
            array<ModLevel>@ campaign_levels = ModGetCampaignLevels(active_sids[i]);
			Campaign c = ModGetCampaign(active_sids[i]);
            for( uint k = 0; k < campaign_levels.length(); k++ ) {
                level_list.insertLast(campaign_levels[k].GetPath());
            }
            break;
        }
    }
}

string GetNextLevel(){
    int current_index = -1;
    for(uint i = 0; i < level_list.size(); i++){
        if("Data/Levels/" + level_list[i] == current_level_path){
            current_index = i;
            break;
        }
    }
    if((current_index + 1) >= int(level_list.size()) || current_index == -1){
        return "";
    }else{
        return "Data/Levels/" + level_list[current_index + 1];
    }
}

void Dispose() {
}

void ReceiveMessage(string msg) {
    Log(info, "Getting msg : " + msg );
    TokenIterator token_iter;
    token_iter.Init();
    if(!token_iter.FindNextToken(msg)){
        return;
    }
    string token = token_iter.GetToken(msg);

    if(token == "levelwin" ) {
        if(!EditorModeActive()){
            string path = GetNextLevel();
            if(path != ""){
                FinishedGeneralCampaignLevel(current_level_path);
                level.SendMessage("loadlevel \""+path+"\"");
            } else {
                level.SendMessage("go_to_main_menu");		
            }
        } else {
            Log(info, "Ignoring levelwin command, game is in editor mode");
        }
    }
}

void FinishedGeneralCampaignLevel( string level_name ) {
    Log(info, "Finished Level \"" + level_name + "\"" );
    SavedLevel @level = save_file.GetSavedLevel(campaign_name);

    string current_highest_level = level.GetValue("highest_level");

    Log(info, "Current Level id \"" + current_highest_level + "\"" );
    int id_current_highest_level = -1;

    if( current_highest_level != "" ) {
        id_current_highest_level = atoi(current_highest_level);
    }

    int id_new_level = -1;
    Log( info, "level_name: " + level_name );
    for( uint i = 0; i < level_list.length(); i++ ) {
        string full_lugaru_level = "Data/Levels/" + level_list[i];
        if( level_name == full_lugaru_level ) {
            Log( info, "Matched: " + i );
            id_new_level = i;
        }
    }

    if( id_new_level + 1 > id_current_highest_level ) {
        Log( info, "Setting new highest level id to: " + (id_new_level + 1));
        level.SetValue("highest_level", "" + (id_new_level + 1));  
        save_file.WriteInPlace();
    }
}

float blackout_amount = 0.0;
float ko_time = -1.0;
float win_time = -1.0;
bool sent_level_complete_message = false;

void Update() {
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
