//-----------------------------------------------------------------------------
//           Name: goal_level.as
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
#include "lugaru_campaign.as"

int progress = 0;
bool queued_goal_check = true;
float blackout_amount = 0.0f;
float ko_time = -1.0f;
float win_time = -1.0f;
int win_target = 0;
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

void TriggerGoalString(const string& in goal_str) {
    TokenIterator token_iter;
    token_iter.Init();
    if (!token_iter.FindNextToken(goal_str)) {
        return;
    }
    if (token_iter.GetToken(goal_str) == "dialogue" && !EditorModeActive()) {
        if (token_iter.FindNextToken(goal_str)) {
            level.SendMessage("start_dialogue \"" + token_iter.GetToken(goal_str) + "\"");
        }
    }
}

void TriggerGoalPre() {
    if (params.HasParam("goal_" + progress + "_pre")) {
        TriggerGoalString(params.GetString("goal_" + progress + "_pre"));
    }
}

void TriggerGoalPost() {
    if (params.HasParam("goal_" + progress + "_post")) {
        TriggerGoalString(params.GetString("goal_" + progress + "_post"));
    }
}

void IncrementProgress() {
    TriggerGoalPost();
    ++progress;
    win_time = -1.0f;
    TriggerGoalPre();

    if (params.HasParam("goal_" + progress)) {
        string goal_str = params.GetString("goal_" + progress);
        TokenIterator token_iter;
        token_iter.Init();
        if (token_iter.FindNextToken(goal_str)) {
            if (token_iter.GetToken(goal_str) == "spawn_defeat") {
                EnableSpawnDefeatCharacters(goal_str);
            }
        }
    }

    RestorePlayerHealth();
}

void EnableSpawnDefeatCharacters(const string& in goal_str) {
    TokenIterator token_iter;
    token_iter.Init();
    token_iter.FindNextToken(goal_str); // Skip 'spawn_defeat' token
    while (token_iter.FindNextToken(goal_str)) {
        int id = atoi(token_iter.GetToken(goal_str));
        if (ObjectExists(id) && ReadObjectFromID(id).GetType() == _movement_object) {
            SetEnabledCharacterAndItems(id, true);
        }
    }
}

void SetEnabledCharacterAndItems(int id, bool enabled) {
    ReadObjectFromID(id).SetEnabled(enabled);
    MovementObject@ character = ReadCharacterID(id);
    for (int i = 0; i < 6; ++i) {
        int item_id = character.GetArrayIntVar("weapon_slots", i);
        if (item_id != -1 && ObjectExists(item_id)) {
            ReadObjectFromID(item_id).SetEnabled(enabled);
        }
    }
}

void RestorePlayerHealth() {
    array<int> player_ids = GetControlledCharacterIDs();
    for (uint i = 0; i < player_ids.length(); ++i) {
        MovementObject@ mo = ReadCharacter(player_ids[i]);
        mo.ReceiveMessage("restore_health");
    }
}

void CheckReset() {
    int num_player_spawn = SetupPlayerSpawns();
    progress = min(progress, num_player_spawn);
    EnableAllCharacters();
    DisableDefeatedCharacters();
    DisableFutureSpawnDefeatCharacters();
    PlacePlayerAtSpawnPoint();
    TriggerGoalPre();
}

int SetupPlayerSpawns() {
    int num_player_spawn = 0;
    if (params.HasParam("player_spawn")) {
        string param_str = params.GetString("player_spawn");
        TokenIterator token_iter;
        token_iter.Init();
        while (token_iter.FindNextToken(param_str)) {
            int id = atoi(token_iter.GetToken(param_str));
            if (ObjectExists(id)) {
                Object@ obj = ReadObjectFromID(id);
                if (obj.GetType() == _placeholder_object) {
                    PlaceholderObject@ placeholder = cast<PlaceholderObject@>(obj);
                    placeholder.SetPreview("Data/Objects/therium/characters/ghostobj.xml");
                    ++num_player_spawn;
                }
            }
        }
    }
    return num_player_spawn;
}

void EnableAllCharacters() {
    int num_characters = GetNumCharacters();
    for (int i = 0; i < num_characters; ++i) {
        SetEnabledCharacterAndItems(ReadCharacter(i).GetID(), true);
    }
}

void DisableDefeatedCharacters() {
    for (int i = 0; i < progress; ++i) {
        if (params.HasParam("goal_" + i)) {
            string goal_str = params.GetString("goal_" + i);
            TokenIterator token_iter;
            token_iter.Init();
            if (token_iter.FindNextToken(goal_str)) {
                if (token_iter.GetToken(goal_str) == "defeat" || token_iter.GetToken(goal_str) == "spawn_defeat") {
                    DisableCharactersInGoal(goal_str);
                }
            }
        }
    }
}

void DisableFutureSpawnDefeatCharacters() {
    for (int i = progress + 1; params.HasParam("goal_" + i); ++i) {
        if (params.HasParam("goal_" + i)) {
            string goal_str = params.GetString("goal_" + i);
            TokenIterator token_iter;
            token_iter.Init();
            if (token_iter.FindNextToken(goal_str)) {
                if (token_iter.GetToken(goal_str) == "spawn_defeat") {
                    DisableCharactersInGoal(goal_str);
                }
            }
        }
    }
}

void DisableCharactersInGoal(const string& in goal_str) {
    TokenIterator token_iter;
    token_iter.Init();
    token_iter.FindNextToken(goal_str); // Skip goal type token
    while (token_iter.FindNextToken(goal_str)) {
        int id = atoi(token_iter.GetToken(goal_str));
        if (ObjectExists(id) && ReadObjectFromID(id).GetType() == _movement_object) {
            SetEnabledCharacterAndItems(id, false);
        }
    }
}

void PlacePlayerAtSpawnPoint() {
    array<int> player_ids = GetControlledCharacterIDs();
    for (uint i = 0; i < player_ids.length(); ++i) {
        MovementObject@ mo = ReadCharacter(player_ids[i]);
        if (progress == 0) {
            Object@ obj = ReadObjectFromID(mo.GetID());
            mo.position = obj.GetTranslation();
            mo.SetRotationFromFacing(obj.GetRotation() * vec3(0, 0, 1));
        } else {
            PlacePlayerAtCustomSpawn(mo);
        }
    }
}

void PlacePlayerAtCustomSpawn(MovementObject@ mo) {
    if (!params.HasParam("player_spawn")) {
        return;
    }
    string param_str = params.GetString("player_spawn");
    TokenIterator token_iter;
    token_iter.Init();
    for (int j = 0; j < progress; ++j) {
        token_iter.FindNextToken(param_str);
    }
    int id = atoi(token_iter.GetToken(param_str));
    if (ObjectExists(id)) {
        Object@ obj = ReadObjectFromID(id);
        if (obj.GetType() == _placeholder_object) {
            mo.position = obj.GetTranslation();
            mo.SetRotationFromFacing(obj.GetRotation() * vec3(0, 0, 1));
            mo.Execute("SetCameraFromFacing();");
        }
    }
}

void PossibleWinEvent(const string& in event, int val, int goal_check) {
    if (!params.HasParam("goal_" + goal_check)) {
        return;
    }
    string goal_str = params.GetString("goal_" + goal_check);
    TokenIterator token_iter;
    token_iter.Init();
    if (!token_iter.FindNextToken(goal_str)) {
        return;
    }
    string goal_type = token_iter.GetToken(goal_str);

    if (event == "checkpoint" && (goal_type == "reach" || goal_type == "reach_skippable")) {
        HandleCheckpointEvent(goal_str, val, goal_check, goal_type);
    } else if (event == "character_defeated" && (goal_type == "defeat" || goal_type == "spawn_defeat")) {
        HandleCharacterDefeatedEvent(goal_str, goal_check);
    }
}

void HandleCheckpointEvent(const string& in goal_str, int val, int goal_check, const string& in goal_type) {
    TokenIterator token_iter;
    token_iter.Init();
    token_iter.FindNextToken(goal_str); // Skip goal type token
    if (token_iter.FindNextToken(goal_str)) {
        int id = atoi(token_iter.GetToken(goal_str));
        if (id == val) {
            win_time = the_time + 1.0f;
            win_target = goal_check + 1;
        } else if (goal_type == "reach_skippable") {
            PossibleWinEvent("checkpoint", val, goal_check + 1);
        }
    }
}

void HandleCharacterDefeatedEvent(const string& in goal_str, int goal_check) {
    TokenIterator token_iter;
    token_iter.Init();
    token_iter.FindNextToken(goal_str); // Skip goal type token
    bool success = true;
    while (token_iter.FindNextToken(goal_str)) {
        int id = atoi(token_iter.GetToken(goal_str));
        if (ObjectExists(id) && ReadObjectFromID(id).GetType() == _movement_object &&
            ReadCharacterID(id).GetIntVar("knocked_out") == _awake) {
            success = false;
            break;
        }
    }
    if (success) {
        win_time = the_time + 3.0f;
        win_target = goal_check + 1;
    }
}

void ReceiveMessage(string msg) {
    TokenIterator token_iter;
    token_iter.Init();
    if (!token_iter.FindNextToken(msg)) {
        return;
    }
    string msg_start = token_iter.GetToken(msg);

    if (msg_start == "reset") {
        queued_goal_check = true;
        ko_time = -1.0f;
        win_time = -1.0f;
    }

    if (win_time == -1.0f && msg_start == "player_entered_checkpoint") {
        if (token_iter.FindNextToken(msg)) {
            int checkpoint_id = atoi(token_iter.GetToken(msg));
            PossibleWinEvent("checkpoint", checkpoint_id, progress);
        }
    } else if (win_time == -1.0f && msg_start == "level_event" &&
               token_iter.FindNextToken(msg) &&
               (token_iter.GetToken(msg) == "character_knocked_out" || token_iter.GetToken(msg) == "character_died")) {
        PossibleWinEvent("character_defeated", -1, progress);
    }
}

void Update() {
    if (queued_goal_check) {
        CheckReset();
        queued_goal_check = false;
    }
    HandlePlayerKnockout();
    HandleWinCondition();
}

void HandlePlayerKnockout() {
    int player_id = GetPlayerCharacterID();
    blackout_amount = 0.0f;

    if (player_id != -1 && ObjectExists(player_id)) {
        MovementObject@ char = ReadCharacter(player_id);
        if (char.GetIntVar("knocked_out") != _awake) {
            if (ko_time == -1.0f) {
                ko_time = the_time;
            }
            if (ko_time < the_time - 1.0f) {
                if (GetInputPressed(0, "attack") || ko_time < the_time - 5.0f) {
                    level.SendMessage("reset");
                    level.SendMessage("skip_dialogue");
                }
            }
            blackout_amount = 0.2f + 0.6f * (1.0f - pow(0.5f, (the_time - ko_time)));
        } else {
            ko_time = -1.0f;
        }
    } else {
        ko_time = -1.0f;
    }
}

void HandleWinCondition() {
    if (win_time != -1.0f && the_time > win_time && ko_time == -1.0f) {
        while (progress < win_target) {
            IncrementProgress();
        }
    }
}

void PreDraw(float curr_game_time) {
    camera.SetTint(camera.GetTint() * (1.0f - blackout_amount));
}

void Draw() {
    if (EditorModeActive()) {
        Object@ obj = ReadObjectFromID(hotspot.GetID());
        DebugDrawBillboard("Data/Textures/ui/ogicon.png",
                           obj.GetTranslation(),
                           obj.GetScale().y * 2.0f,
                           vec4(0.5f),
                           _delete_on_draw);
    }
}
