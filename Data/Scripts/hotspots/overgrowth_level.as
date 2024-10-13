//-----------------------------------------------------------------------------
//           Name: overgrowth_level.as
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

int progress = 0;
bool queued_goal_check = true;

float blackout_amount = 0.0f;
float ko_time = -1.0f;
float win_time = -1.0f;
int win_target = 0;
bool sent_level_complete_message = false;
int current_music_layer = 0;
float music_sting_end = 0.0f;
const bool kDebugText = true;
int sting_handle = -1;

bool music_settings_valid = false;
string custom_music_xml_filename;
string custom_music_song_name;

string music_prefix;
string success_sting = "Data/Music/slaver_loop/the_slavers_success.wav";
string defeat_sting = "Data/Music/slaver_loop/the_slavers_defeat.wav";
string death_message = "Press $attack$ to try again";

// Audience info
float audience_excitement = 0.0f;
float total_excitement = 0.0f;
int audience_sound_handle = -1;
float crowd_cheer_amount = 0.0f;
float crowd_cheer_velocity = 0.0f;
float boo_amount = 0.0f;

// Duplicated from aschar to maintain backwards compatibility; can't add methods since
// they might not exist in some mod's old aschar
enum DeathHint {
    _hint_none,
    _hint_deflect_thrown,
    _hint_cant_swim,
    _hint_extinguish,
    _hint_escape_throw,
    _hint_no_dog_choke,
    _hint_roll_stand,
    _hint_vary_attacks,
    _hint_one_at_a_time,
    _hint_avoid_spikes,
    _hint_stealth
};

void SetParameters() {
    params.AddString("music", "slaver");
    params.AddString("player_spawn", "");
    params.AddString("death_message", death_message);
    params.AddIntCheckbox("press_attack_to_restart", true);

    string music_param_value = params.GetString("music");

    if (music_param_value == "slaver") {
        SetMusicParams("Data/Music/slaver_loop/layers.xml", "slavers1", "slavers_", "Data/Music/slaver_loop/the_slavers_success.wav", "Data/Music/slaver_loop/the_slavers_defeat.wav");
    } else if (music_param_value == "swamp") {
        SetMusicParams("Data/Music/swamp_loop/swamp_layer.xml", "swamp1", "swamp_", "Data/Music/swamp_loop/swamp_success.wav", "Data/Music/swamp_loop/swamp_defeat.wav");
    } else if (music_param_value == "cats") {
        SetMusicParams("Data/Music/cats_loop/layers.xml", "cats1", "cats_", "Data/Music/cats_loop/cats_success.wav", "Data/Music/cats_loop/cats_defeat.wav");
    } else if (music_param_value == "crete") {
        SetMusicParams("Data/Music/crete_loop/layers.xml", "crete1", "crete_", "Data/Music/crete_loop/crete_success.wav", "Data/Music/crete_loop/crete_defeat.wav");
    } else if (music_param_value == "rescue") {
        SetMusicParams("Data/Music/rescue_loop/layers.xml", "rescue1", "rescue_", "Data/Music/rescue_loop/rescue_success.wav", "Data/Music/rescue_loop/rescue_defeat.wav");
    } else if (music_param_value == "arena") {
        SetMusicParams("Data/Music/SubArena/layers.xml", "sub_arena", "arena_", "Data/Sounds/versus/fight_win1_1.wav", "Data/Sounds/versus/fight_lose1_1.wav");
    } else if (music_param_value == "custom_og_1_5") {
        AddCustomMusicParams();
        if (FileExists(custom_music_xml_filename)) {
            AddMusic(custom_music_xml_filename);
            PlaySong(custom_music_song_name);
            music_settings_valid = true;
        } else {
            Log(warning, "Custom music XML cannot be found. Filename: " + custom_music_xml_filename);
            SetSilentMusic();
        }
    } else if (music_param_value == "") {
        SetSilentMusic();
    } else {
        params.SetString("music", "slaver");
        SetParameters();
    }
}

void SetMusicParams(const string& in xml_file, const string& in song_name, const string& in prefix, const string& in success_sting_path, const string& in defeat_sting_path) {
    AddMusic(xml_file);
    PlaySong(song_name);
    music_settings_valid = true;
    music_prefix = prefix;
    success_sting = success_sting_path;
    defeat_sting = defeat_sting_path;
}

void SetSilentMusic() {
    PlaySong("overgrowth_silence");
    music_prefix = "silence_";
    music_settings_valid = true;
    success_sting = "";
    defeat_sting = "";
}

void AddCustomMusicParams() {
    params.AddIntCheckbox("music_custom_is_layered", true);
    params.AddString("music_custom_xml_filename", "Data/Music/slaver_loop/layers.xml");
    params.AddString("music_custom_song_name", "slavers1");
    params.AddString("music_custom_layer_prefix", "slavers_");
    params.AddString("music_custom_success_sting_wav_filename", "Data/Music/slaver_loop/the_slavers_success.wav");
    params.AddString("music_custom_defeat_sting_wav_filename", "Data/Music/slaver_loop/the_slavers_defeat.wav");

    custom_music_xml_filename = params.GetString("music_custom_xml_filename");
    custom_music_song_name = params.GetString("music_custom_song_name");
    music_prefix = params.GetString("music_custom_layer_prefix");
    success_sting = params.GetString("music_custom_success_sting_wav_filename");
    defeat_sting = params.GetString("music_custom_defeat_sting_wav_filename");
}

void Init() {
    save_file.WriteInPlace();
    current_music_layer = 0;
    level.ReceiveLevelEvents(hotspot.GetID());
    hotspot.SetCollisionEnabled(false);
    audience_sound_handle = -1;
}

void Dispose() {
    level.StopReceivingLevelEvents(hotspot.GetID());
}

void HandleEvent(string event, MovementObject@ mo) {
    // No event handling needed
}

void Update() {
    if (queued_goal_check) {
        CheckReset();
        queued_goal_check = false;
    }
    UpdateMusicLayer();
    UpdateAudience();
    HandlePlayerKnockout();
    HandleWinCondition();
}

void UpdateMusicLayer() {
    if (!music_settings_valid) {
        return;
    }

    int player_id = GetPlayerCharacterID();
    if (ko_time == -1.0f) {
        if (music_layer_override == -1) {
            if (player_id == -1) {
                SetMusicLayer(-1);
            } else if (ReadCharacter(player_id).GetIntVar("knocked_out") != _awake) {
                SetMusicLayer(0);
            } else if (ReadCharacter(player_id).QueryIntFunction("int CombatSong()") == 1) {
                SetMusicLayer(3);
            } else if (params.HasParam("music")) {
                SetMusicLayer(1);
            }
        } else {
            SetMusicLayer(music_layer_override);
        }
    } else {
        SetMusicLayer(-2);
    }

    if (the_time >= music_sting_end) {
        if (music_sting_end != 0.0f && ko_time == -1.0f) {
            music_sting_end = 0.0f;
            SetLayerGain(music_prefix + "layer_" + current_music_layer, 1.0f);
        }
    } else {
        SetLayerGain(music_prefix + "layer_" + current_music_layer, 0.0f);
    }
}

void SetMusicLayer(int layer) {
    if (kDebugText) {
        DebugText("music_layer", "music_layer: " + layer, 0.5f);
        DebugText("music_layer_override", "music_layer_override: " + music_layer_override, 0.5f);
        DebugText("music_prefix", "music_prefix: " + music_prefix, 0.5f);
    }
    if (layer != current_music_layer) {
        for (int i = 0; i < 5; ++i) {
            SetLayerGain(music_prefix + "layer_" + i, 0.0f);
        }
        SetLayerGain(music_prefix + "layer_" + layer, 1.0f);
        current_music_layer = layer;
    }
}

void UpdateAudience() {
    if (audience_sound_handle == -1 && music_prefix == "arena_") {
        audience_sound_handle = PlaySoundLoop("Data/Sounds/crowd/crowd_arena_general_1.wav", 0.0f);
    } else if (audience_sound_handle != -1 && music_prefix != "arena_") {
        StopSound(audience_sound_handle);
        audience_sound_handle = -1;
    }

    float total_char_speed = 0.0f;
    int num_chars = GetNumCharacters();
    for (int i = 0; i < num_chars; ++i) {
        MovementObject@ char = ReadCharacter(i);
        if (char.GetIntVar("knocked_out") == _awake) {
            total_char_speed += length(char.velocity);
        }
    }

    float excitement_decay_rate = 1.0f / (1.0f + total_char_speed / 14.0f);
    excitement_decay_rate *= 3.0f;
    audience_excitement *= pow(0.05f, 0.001f * excitement_decay_rate);
    total_excitement += audience_excitement * time_step;

    float target_crowd_cheer_amount = audience_excitement * 0.1f + 0.15f;
    float target_boo_amount = 0.0f;
    if (crowd_override) {
        target_crowd_cheer_amount = crowd_gain_override;
        target_boo_amount = crowd_pitch_override;
    }
    boo_amount = mix(target_boo_amount, boo_amount, 0.98f);
    crowd_cheer_velocity += (target_crowd_cheer_amount - crowd_cheer_amount) * time_step * 10.0f;
    crowd_cheer_velocity *= crowd_cheer_velocity > 0.0f ? 0.99f : 0.95f;
    crowd_cheer_amount += crowd_cheer_velocity * time_step;
    crowd_cheer_amount = max(crowd_cheer_amount, 0.1f);

    SetSoundGain(audience_sound_handle, crowd_cheer_amount * 2.0f);
    SetSoundPitch(audience_sound_handle, mix(min(0.8f + crowd_cheer_amount * 0.5f, 1.2f), 0.7f, boo_amount));
}

void HandlePlayerKnockout() {
    int player_id = GetPlayerCharacterID();
    blackout_amount = 0.0f;
    if (player_id != -1 && ObjectExists(player_id)) {
        MovementObject@ char = ReadCharacter(player_id);
        if (char.GetIntVar("knocked_out") != _awake) {
            if (ko_time == -1.0f) {
                ko_time = the_time;
                PlayDeathSting();
                can_press_attack = false;
            }
            if (ko_time < the_time - 1.0f) {
                if (!GetInputDown(0, "attack")) {
                    can_press_attack = true;
                }
                if (params.GetInt("press_attack_to_restart") == 1 &&
                    ((GetInputDown(0, "attack") && can_press_attack) || GetInputDown(0, "skip_dialogue") || GetInputDown(0, "keypadenter"))) {
                    if (sting_handle != -1) {
                        music_sting_end = the_time;
                        StopSound(sting_handle);
                        sting_handle = -1;
                    }
                    level.SendMessage("reset");
                    level.SendMessage("skip_dialogue");
                }
            }
            blackout_amount = 0.2f + 0.6f * (1.0f - pow(0.5f, (the_time - ko_time)));
        } else {
            ko_time = -1.0f;
        }
        char.SetFloatVar("level_blackout", blackout_amount);
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
    if (kDebugText) {
        DebugText("progress", "progress: " + progress, 0.5f);
    }
    camera.SetTint(camera.GetTint() * (1.0f - blackout_amount));
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
        music_layer_override = -1;
    }

    if (msg_start == "level_event" && token_iter.FindNextToken(msg)) {
        string sub_msg = token_iter.GetToken(msg);
        if (sub_msg == "music_layer_override") {
            if (token_iter.FindNextToken(msg)) {
                music_layer_override = atoi(token_iter.GetToken(msg));
            }
        } else if (sub_msg == "crowd_override") {
            crowd_override = false;
            if (token_iter.FindNextToken(msg)) {
                crowd_gain_override = atof(token_iter.GetToken(msg));
                if (token_iter.FindNextToken(msg)) {
                    crowd_override = true;
                    crowd_pitch_override = atof(token_iter.GetToken(msg));
                }
            }
        } else if (sub_msg == "character_knocked_out" || sub_msg == "character_died") {
            if (win_time == -1.0f) {
                PossibleWinEvent("character_defeated", -1, progress);
            }
        }
    }

    if (msg_start == "player_entered_checkpoint_fall_death") {
        if (token_iter.FindNextToken(msg)) {
            int checkpoint_id = atoi(token_iter.GetToken(msg));
            if (progress >= checkpoint_id) {
                int player_id = GetPlayerCharacterID();
                if (player_id != -1 && ObjectExists(player_id)) {
                    MovementObject@ char = ReadCharacter(player_id);
                    char.ReceiveMessage("fall_death");
                }
            }
        }
    }

    if (win_time == -1.0f && msg_start == "player_entered_checkpoint") {
        if (token_iter.FindNextToken(msg)) {
            int checkpoint_id = atoi(token_iter.GetToken(msg));
            PossibleWinEvent("checkpoint", checkpoint_id, progress);
        }
    }
}

void PossibleWinEvent(const string& in event, int val, int goal_check, int recursion = 0) {
    if (ko_time != -1.0f || recursion > 5000) {
        return;
    }
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

    if (event == "checkpoint") {
        if (goal_type == "reach" || goal_type == "reach_skippable") {
            if (token_iter.FindNextToken(goal_str)) {
                int id = atoi(token_iter.GetToken(goal_str));
                if (id == val) {
                    win_time = the_time + 1.0f;
                    TriggerGoalPost(_sting_only);
                    win_target = goal_check + 1;
                } else if (goal_type == "reach_skippable") {
                    PossibleWinEvent(event, val, goal_check + 1, recursion + 1);
                }
            }
        }
        if (goal_type == "defeat_optional") {
            PossibleWinEvent(event, val, goal_check + 1, recursion + 1);
        }
    } else if (event == "character_defeated") {
        if (goal_type == "defeat" || goal_type == "spawn_defeat" || goal_type == "defeat_optional") {
            bool success = true;
            bool no_delay = false;
            while (token_iter.FindNextToken(goal_str) && token_iter.GetToken(goal_str) != "") {
                string token = token_iter.GetToken(goal_str);
                if (token == "no_delay") {
                    no_delay = true;
                } else {
                    int id = atoi(token);
                    if (ObjectExists(id) && ReadObjectFromID(id).GetType() == _movement_object &&
                        ReadCharacterID(id).GetIntVar("knocked_out") == _awake) {
                        success = false;
                    }
                }
            }
            if (success) {
                int player_id = GetPlayerCharacterID();
                if (player_id != -1) {
                    MovementObject@ mo = ReadCharacter(player_id);
                    mo.QueueScriptMessage("restore_health");
                }
                win_time = the_time + (no_delay ? 2.0f : 5.0f);
                TriggerGoalPost(_sting_only);
                win_target = goal_check + 1;
            } else if (goal_type == "defeat_optional") {
                PossibleWinEvent(event, val, goal_check + 1, recursion + 1);
            }
        }
        if (goal_type == "reach_skippable") {
            PossibleWinEvent(event, val, goal_check + 1, recursion + 1);
        }
    }
}

void IncrementProgress() {
    TriggerGoalPost(_all_but_sting);
    ++progress;
    win_time = -1.0f;
    TriggerGoalPre();
    CheckSpawnDefeat();
    RestorePlayerHealth();
    if (win_time == -1.0f) {
        PossibleWinEvent("character_defeated", -1, progress);
    }
}

void CheckSpawnDefeat() {
    if (params.HasParam("goal_" + progress)) {
        string goal_str = params.GetString("goal_" + progress);
        TokenIterator token_iter;
        token_iter.Init();
        if (token_iter.FindNextToken(goal_str)) {
            if (token_iter.GetToken(goal_str) == "spawn_defeat") {
                while (token_iter.FindNextToken(goal_str)) {
                    string token = token_iter.GetToken(goal_str);
                    if (token == "no_delay") {
                        continue;
                    }
                    int id = atoi(token);
                    if (ObjectExists(id) && ReadObjectFromID(id).GetType() == _movement_object) {
                        SetEnabledCharacterAndItems(id, true);
                    }
                }
            }
        }
    }
}

void RestorePlayerHealth() {
    array<int> player_ids = GetControlledCharacterIDs();
    for (uint i = 0; i < player_ids.length(); ++i) {
        MovementObject@ mo = ReadCharacter(player_ids[i]);
        mo.ReceiveScriptMessage("restore_health");
    }
}

void SetEnabledCharacterAndItems(int id, bool enabled) {
    ReadObjectFromID(id).SetEnabled(enabled);
    MovementObject@ char = ReadCharacterID(id);
    for (int item_index = 0; item_index < 6; ++item_index) {
        int item_id = char.GetArrayIntVar("weapon_slots", item_index);
        if (item_id != -1 && ObjectExists(item_id)) {
            ReadObjectFromID(item_id).SetEnabled(enabled);
        }
    }
}

void CheckReset() {
    int num_player_spawn = CountValidPlayerSpawns();
    if (progress > num_player_spawn) {
        progress = num_player_spawn;
    }
    EnableAllCharacters();
    DisableDefeatedCharacters();
    DisableFutureSpawnDefeatCharacters();
    PlacePlayerAtSpawnPoint();
    TriggerGoalPre();
}

int CountValidPlayerSpawns() {
    int num_player_spawn = 0;
    if (params.HasParam("player_spawn")) {
        string param_str = params.GetString("player_spawn");
        TokenIterator token_iter;
        token_iter.Init();
        while (token_iter.FindNextToken(param_str)) {
            int id = atoi(token_iter.GetToken(param_str));
            if (ObjectExists(id) && ReadObjectFromID(id).GetType() == _placeholder_object) {
                PlaceholderObject@ placeholder = cast<PlaceholderObject@>(ReadObjectFromID(id));
                placeholder.SetPreview("Data/Objects/IGF_Characters/pale_turner.xml");
                ++num_player_spawn;
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
                string goal_type = token_iter.GetToken(goal_str);
                if (goal_type == "defeat" || goal_type == "spawn_defeat" || goal_type == "defeat_optional") {
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
    token_iter.FindNextToken(goal_str); // Skip goal type
    while (token_iter.FindNextToken(goal_str)) {
        string token = token_iter.GetToken(goal_str);
        if (token == "no_delay") {
            continue;
        }
        int id = atoi(token);
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
    if (ObjectExists(id) && ReadObjectFromID(id).GetType() == _placeholder_object) {
        Object@ obj = ReadObjectFromID(id);
        mo.position = obj.GetTranslation();
        mo.SetRotationFromFacing(obj.GetRotation() * vec3(0, 0, 1));
        mo.Execute("SetCameraFromFacing(); SetOnGround(true); FixDiscontinuity();");
    }
}

enum GoalTriggerType {
    _sting_only,
    _all_but_sting
}

void TriggerGoalPre() {
    if (params.HasParam("goal_" + progress + "_pre")) {
        TriggerGoalString(params.GetString("goal_" + progress + "_pre"), _all_but_sting);
    }
}

void TriggerGoalPost(GoalTriggerType type) {
    if (params.HasParam("goal_" + progress + "_post")) {
        TriggerGoalString(params.GetString("goal_" + progress + "_post"), type);
    }
}

void TriggerGoalString(const string& in goal_str, GoalTriggerType type) {
    TokenIterator token_iter;
    token_iter.Init();
    while (token_iter.FindNextToken(goal_str)) {
        string token = token_iter.GetToken(goal_str);
        if (token == "dialogue" && !EditorModeActive() && !Online_IsClient() && type == _all_but_sting) {
            if (token_iter.FindNextToken(goal_str)) {
                level.SendMessage("start_dialogue \"" + token_iter.GetToken(goal_str) + "\"");
            }
        } else if (token == "dialogue_fade" && !EditorModeActive() && type == _all_but_sting) {
            if (token_iter.FindNextToken(goal_str)) {
                level.SendMessage("start_dialogue_fade \"" + token_iter.GetToken(goal_str) + "\"");
            }
        } else if (token == "dialogue_fade_if_not_hostile" && !EditorModeActive() && type == _all_but_sting) {
            if (token_iter.FindNextToken(goal_str)) {
                int player_id = GetPlayerCharacterID();
                if (player_id != -1 && ReadCharacter(player_id).QueryIntFunction("int CombatSong()") != 1) {
                    level.SendMessage("start_dialogue_fade \"" + token_iter.GetToken(goal_str) + "\"");
                }
            }
        } else if (token == "activate" && !EditorModeActive() && type == _all_but_sting) {
            if (token_iter.FindNextToken(goal_str)) {
                int id = atoi(token_iter.GetToken(goal_str));
                if (ObjectExists(id)) {
                    Object@ obj = ReadObjectFromID(id);
                    if (obj.GetType() == _movement_object) {
                        ReadCharacterID(id).Execute("this_mo.static_char = false;");
                    } else if (obj.GetType() == _hotspot_object) {
                        obj.ReceiveScriptMessage("activate");
                    }
                }
            }
        } else if (token == "disable" && !EditorModeActive() && type == _all_but_sting) {
            if (token_iter.FindNextToken(goal_str)) {
                int id = atoi(token_iter.GetToken(goal_str));
                if (ObjectExists(id)) {
                    ReadObjectFromID(id).SetEnabled(false);
                }
            }
        } else if (token == "enable" && !EditorModeActive() && type == _all_but_sting) {
            if (token_iter.FindNextToken(goal_str)) {
                int id = atoi(token_iter.GetToken(goal_str));
                if (ObjectExists(id)) {
                    ReadObjectFromID(id).SetEnabled(true);
                }
            }
        } else if (token == "music_layer_override" && !EditorModeActive() && type == _all_but_sting) {
            if (token_iter.FindNextToken(goal_str)) {
                music_layer_override = atoi(token_iter.GetToken(goal_str));
            }
        } else if (token == "play_success_sting" && !EditorModeActive() && type == _sting_only) {
            PlaySuccessSting();
        }
    }
}

void PlaySuccessSting() {
    if (sting_handle != -1) {
        StopSound(sting_handle);
        sting_handle = -1;
    }
    sting_handle = PlaySound(success_sting);
    SetSoundGain(sting_handle, GetConfigValueFloat("music_volume"));
    music_sting_end = the_time + 5.0f;
    SetLayerGain(music_prefix + "layer_" + current_music_layer, 0.0f);
}

void PlayDeathSting() {
    if (sting_handle != -1) {
        StopSound(sting_handle);
        sting_handle = -1;
    }
    sting_handle = PlaySound(defeat_sting);
    SetSoundGain(sting_handle, GetConfigValueFloat("music_volume"));
    music_sting_end = the_time + 5.0f;
    SetLayerGain(music_prefix + "layer_" + current_music_layer, 0.0f);
}

bool can_press_attack = false;
int music_layer_override = -1;
bool crowd_override = false;
float crowd_gain_override;
float crowd_pitch_override;
array<float> dof_params;
