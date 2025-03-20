//-----------------------------------------------------------------------------
//           Name: general_campaign_level.as
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

string campaign_name = "my_simple_campaign";
string current_level_path;
array<string> level_list;

float blackout_amount = 0.0;
float ko_time = -1.0;
float win_time = -1.0;
bool sent_level_complete_message = false;

void SetParameters() {
    params.AddString("next_level", "");
}

void Init() {
    campaign_name = GetCurrCampaignID();
    current_level_path = GetCurrLevelRelPath();

    if (campaign_name == "") {
        Log(error, "Did not find an active campaign!");
        return;
    }

    LoadModCampaign();
    Log(info, "Found " + level_list.size() + " levels!");

    SavedLevel@ level = save_file.GetSavedLevel(campaign_name);
    level.SetValue("current_level", current_level_path);
    save_file.WriteInPlace();
}

void LoadModCampaign() {
    level_list.removeRange(0, level_list.length());
    array<ModID>@ active_sids = GetActiveModSids();

    for (uint i = 0; i < active_sids.length(); i++) {
        if (ModGetID(active_sids[i]) == campaign_name) {
            array<ModLevel>@ campaign_levels = ModGetCampaignLevels(active_sids[i]);
            for (uint k = 0; k < campaign_levels.length(); k++) {
                level_list.insertLast(campaign_levels[k].GetPath());
            }
            break;
        }
    }
}

string GetNextLevel() {
    int current_index = -1;
    for (uint i = 0; i < level_list.size(); i++) {
        if ("Data/Levels/" + level_list[i] == current_level_path) {
            current_index = i;
            break;
        }
    }

    if (current_index == -1 || (current_index + 1) >= int(level_list.size())) {
        return "";
    } else {
        return "Data/Levels/" + level_list[current_index + 1];
    }
}

void ReceiveMessage(string msg) {
    Log(info, "Getting msg: " + msg);
    TokenIterator token_iter;
    token_iter.Init();

    if (!token_iter.FindNextToken(msg)) {
        return;
    }

    string token = token_iter.GetToken(msg);
    if (token != "levelwin") {
        return;
    }

    if (EditorModeActive()) {
        Log(info, "Ignoring levelwin command, game is in editor mode");
        return;
    }

    string path = GetNextLevel();
    if (path != "") {
        FinishedGeneralCampaignLevel(current_level_path);
        level.SendMessage("loadlevel \"" + path + "\"");
    } else {
        level.SendMessage("go_to_main_menu");
    }
}

void FinishedGeneralCampaignLevel(string level_name) {
    Log(info, "Finished Level \"" + level_name + "\"");
    SavedLevel@ level = save_file.GetSavedLevel(campaign_name);

    string current_highest_level = level.GetValue("highest_level");
    int id_current_highest_level = (current_highest_level != "") ? atoi(current_highest_level) : -1;
    int id_new_level = -1;

    for (uint i = 0; i < level_list.length(); i++) {
        string full_level_path = "Data/Levels/" + level_list[i];
        if (level_name == full_level_path) {
            id_new_level = i;
            break;
        }
    }

    if (id_new_level + 1 > id_current_highest_level) {
        Log(info, "Setting new highest level id to: " + (id_new_level + 1));
        level.SetValue("highest_level", "" + (id_new_level + 1));
        save_file.WriteInPlace();
    }
}

void Update() {
    blackout_amount = 0.0;

    UpdatePlayerState();
    UpdateLevelWinCondition();
}

void UpdatePlayerState() {
    int player_id = GetPlayerCharacterID();

    if (player_id == -1 || !ObjectExists(player_id)) {
        ko_time = -1.0f;
        return;
    }

    MovementObject@ character = ReadCharacter(player_id);
    if (character.GetIntVar("knocked_out") != _awake) {
        HandlePlayerKnockedOut(character);
    } else {
        ko_time = -1.0f;
    }
}

void HandlePlayerKnockedOut(MovementObject@ character) {
    if (ko_time == -1.0f) {
        ko_time = the_time;
    }

    if (ko_time < the_time - 1.0) {
        if (GetInputPressed(0, "attack") || ko_time < the_time - 5.0) {
            level.SendMessage("reset");
            level.SendMessage("skip_dialogue");
        }
    }

    blackout_amount = 0.2 + 0.6 * (1.0 - pow(0.5, (the_time - ko_time)));
}

void UpdateLevelWinCondition() {
    if (ThreatsRemaining() != 0 || ThreatsPossible() == 0 || ko_time != -1.0) {
        win_time = -1.0;
        return;
    }

    if (win_time == -1.0f) {
        win_time = the_time;
    }

    if (win_time < the_time - 5.0 && !sent_level_complete_message) {
        SendGlobalMessage("levelwin");
        Log(info, "Sending level win");
        sent_level_complete_message = true;
    }
}

void PreDraw(float curr_game_time) {
    camera.SetTint(camera.GetTint() * (1.0 - blackout_amount));
}

void DrawEditor() {
    Object@ obj = ReadObjectFromID(hotspot.GetID());
    DebugDrawBillboard(
        "Data/Textures/ui/lugaru_icns_256x256.png",
        obj.GetTranslation(),
        obj.GetScale()[1] * 2.0,
        vec4(vec3(0.5), 1.0),
        _delete_on_draw
    );
}
