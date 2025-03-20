//-----------------------------------------------------------------------------
//           Name: lugaru_level.as
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
#include "campaign_common.as"

string death_message = "Press $attack$ to try again";
float blackout_amount = 0.0f;
float ko_time = -1.0f;
float win_time = -1.0f;
bool sent_level_complete_message = false;

void SetParameters() {
    params.AddString("death_message", death_message);
    params.AddIntCheckbox("press_attack_to_restart", true);
    params.AddString("music", "lugaru_ambient_grass");
}

void Init() {
    AddMusic("Data/Music/lugaru_new.xml");
}

void Update() {
    HandleMusic();
    HandlePlayerKnockout();
    HandleLevelWinCondition();
}

void HandleMusic() {
    int player_id = GetPlayerCharacterID();
    if (player_id != -1 && ReadCharacter(player_id).QueryIntFunction("int CombatSong()") == 1 &&
        ReadCharacter(player_id).GetIntVar("knocked_out") == _awake) {
        PlaySong("lugaru_combat");
    } else if (params.HasParam("music")) {
        string song = params.GetString("music");
        if (song != "") {
            PlaySong(song);
        }
    }
}

void HandlePlayerKnockout() {
    int player_id = GetPlayerCharacterID();
    blackout_amount = 0.0f;

    if (player_id == -1 || !ObjectExists(player_id)) {
        ko_time = -1.0f;
        return;
    }

    MovementObject@ char = ReadCharacter(player_id);
    if (char.GetIntVar("knocked_out") != _awake) {
        if (ko_time == -1.0f) {
            ko_time = the_time;
        }
        if (ko_time < the_time - 1.0f) {
            if (params.GetInt("press_attack_to_restart") == 1 && (GetInputPressed(0, "attack") || ko_time < the_time - 5.0f)) {
                level.SendMessage("reset");
                level.SendMessage("skip_dialogue");
            }
        }
        blackout_amount = 0.2f + 0.6f * (1.0f - pow(0.5f, (the_time - ko_time)));
        DisplayDeathMessage();
    } else {
        ko_time = -1.0f;
    }
}

void DisplayDeathMessage() {
    bool use_keyboard = (max(last_mouse_event_time, last_keyboard_event_time) > last_controller_event_time);
    string respawn_message = params.GetString("death_message");
    respawn_message = respawn_message.replace("$attack$", GetStringDescriptionForBinding(use_keyboard ? "key" : "gamepad_0", "attack"));
    level.SendMessage("screen_message " + respawn_message);
}

void HandleLevelWinCondition() {
    if (ThreatsRemaining() == 0 && ThreatsPossible() != 0 && ko_time == -1.0f) {
        if (win_time == -1.0f) {
            win_time = the_time;
        }
        if (win_time < the_time - 5.0f && !sent_level_complete_message) {
            SendGlobalMessage("levelwin");
            sent_level_complete_message = true;
        }
    } else {
        win_time = -1.0f;
    }
}

void PreDraw(float curr_game_time) {
    camera.SetTint(camera.GetTint() * (1.0f - blackout_amount));
}

void DrawEditor() {
    Object@ obj = ReadObjectFromID(hotspot.GetID());
    DebugDrawBillboard("Data/Textures/ui/lugaru_icns_256x256.png",
                       obj.GetTranslation(),
                       obj.GetScale().y * 2.0f,
                       vec4(0.5f),
                       _delete_on_draw);
}
