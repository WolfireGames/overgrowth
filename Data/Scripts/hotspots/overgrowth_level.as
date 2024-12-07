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

float blackout_amount = 0.0;
float ko_time = -1.0;
float win_time = -1.0;
int win_target = 0;
bool sent_level_complete_message = false;
int curr_music_layer = 0;
float music_sting_end = 0.0;
const bool kDebugText = true;
int sting_handle = -1;

bool g_music_settings_are_valid;
string g_music_custom_xml_filename;
string g_music_custom_song_name;

string music_prefix;
string success_sting = "Data/Music/slaver_loop/the_slavers_success.wav";
string defeat_sting = "Data/Music/slaver_loop/the_slavers_defeat.wav";
string death_message = "Press $attack$ to try again";

// Audience info
float audience_excitement;
float total_excitement;
int audience_size;
int audience_sound_handle;
float crowd_cheer_amount;
float crowd_cheer_vel;
float boo_amount = 0.0;

// Duplicated from aschar to maintain backwards compat; can't add methods since
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

void AddCustomMusicParams_() {
    params.AddIntCheckbox("music_custom_is_layered", true);

    params.AddString("music_custom_xml_filename", "Data/Music/slaver_loop/layers.xml");
    params.AddString("music_custom_song_name", "slavers1");
    params.AddString("music_custom_layer_prefix", "slavers_");
    params.AddString("music_custom_success_sting_wav_filename", "Data/Music/slaver_loop/the_slavers_success.wav");
    params.AddString("music_custom_defeat_sting_wav_filename", "Data/Music/slaver_loop/the_slavers_defeat.wav");

    g_music_custom_xml_filename = params.GetString("music_custom_xml_filename");
    g_music_custom_song_name = params.GetString("music_custom_song_name");
    music_prefix = params.GetString("music_custom_layer_prefix");
    success_sting = params.GetString("music_custom_success_sting_wav_filename");
    defeat_sting = params.GetString("music_custom_defeat_sting_wav_filename");
}

void SetParameters() {
    params.AddString("music", "slaver");
    params.AddString("player_spawn", "");
    params.AddString("death_message", death_message);
    params.AddIntCheckbox("press_attack_to_restart", true);

    string music_param_value = params.GetString("music");

    if (music_param_value == "slaver") {
        AddMusic("Data/Music/slaver_loop/layers.xml");
        PlaySong("slavers1");
        g_music_settings_are_valid = true;
        music_prefix = "slavers_";
        success_sting = "Data/Music/slaver_loop/the_slavers_success.wav";
        defeat_sting = "Data/Music/slaver_loop/the_slavers_defeat.wav";
    } else if (music_param_value == "swamp") {
        AddMusic("Data/Music/swamp_loop/swamp_layer.xml");
        PlaySong("swamp1");
        g_music_settings_are_valid = true;
        music_prefix = "swamp_";
        success_sting = "Data/Music/swamp_loop/swamp_success.wav";
        defeat_sting = "Data/Music/swamp_loop/swamp_defeat.wav";
    } else if (music_param_value == "cats") {
        AddMusic("Data/Music/cats_loop/layers.xml");
        PlaySong("cats1");
        g_music_settings_are_valid = true;
        music_prefix = "cats_";
        success_sting = "Data/Music/cats_loop/cats_success.wav";
        defeat_sting = "Data/Music/cats_loop/cats_defeat.wav";
    } else if (music_param_value == "crete") {
        AddMusic("Data/Music/crete_loop/layers.xml");
        PlaySong("crete1");
        g_music_settings_are_valid = true;
        music_prefix = "crete_";
        success_sting = "Data/Music/crete_loop/crete_success.wav";
        defeat_sting = "Data/Music/crete_loop/crete_defeat.wav";
    } else if (music_param_value == "rescue") {
        AddMusic("Data/Music/rescue_loop/layers.xml");
        PlaySong("rescue1");
        g_music_settings_are_valid = true;
        music_prefix = "rescue_";
        success_sting = "Data/Music/rescue_loop/rescue_success.wav";
        defeat_sting = "Data/Music/rescue_loop/rescue_defeat.wav";
    } else if (music_param_value == "arena") {
        AddMusic("Data/Music/SubArena/layers.xml");
        PlaySong("sub_arena");
        g_music_settings_are_valid = true;
        music_prefix = "arena_";
        success_sting = "Data/Sounds/versus/fight_win1_1.wav";
        defeat_sting = "Data/Sounds/versus/fight_lose1_1.wav";
    } else if (music_param_value == "custom_og_1_5") {
        g_music_settings_are_valid = false;

        AddCustomMusicParams_();

        if (FileExists(g_music_custom_xml_filename)) {
            AddMusic(g_music_custom_xml_filename);
            PlaySong(g_music_custom_song_name);
            g_music_settings_are_valid = true;
        } else {
            Log(warning, "Custom music XML cannot be found. Filename: " + g_music_custom_xml_filename);
        }

        if (!g_music_settings_are_valid) {
            Log(warning, "Switching to silence since custom music is not set up correctly");
            PlaySong("overgrowth_silence");
            music_prefix = "silence_";
        }
    } else if (music_param_value == "") {
        PlaySong("overgrowth_silence");
        music_prefix = "silence_";
        g_music_settings_are_valid = true;
        success_sting = "";
        defeat_sting = "";
    } else {
        params.SetString("music", "slaver");
        SetParameters();
    }
}

void Init() {
    save_file.WriteInPlace();
    curr_music_layer = 0;
    level.ReceiveLevelEvents(hotspot.GetID());
    hotspot.SetCollisionEnabled(false);
    audience_sound_handle = -1;
}

void Dispose() {
    level.StopReceivingLevelEvents(hotspot.GetID());
}

enum GoalTriggerType { _sting_only, _all_but_sting };

void TriggerGoalString(const string &in goal_str, GoalTriggerType type) {
    Log(info, "Triggering goal string: " + goal_str);
    TokenIterator token_iter;
    token_iter.Init();
    while (token_iter.FindNextToken(goal_str)) {
        string token = token_iter.GetToken(goal_str);
        if (token == "dialogue" && !EditorModeActive() && !Online_IsClient()) {
            if (token_iter.FindNextToken(goal_str) && type == _all_but_sting) {
                level.SendMessage("start_dialogue \"" + token_iter.GetToken(goal_str) + "\"");
            }
        }
        if (token == "dialogue_fade" && !EditorModeActive()) {
            if (token_iter.FindNextToken(goal_str) && type == _all_but_sting) {
                level.SendMessage("start_dialogue_fade \"" + token_iter.GetToken(goal_str) + "\"");
            }
        }
        if (token == "dialogue_fade_if_not_hostile" && !EditorModeActive()) {
            if (token_iter.FindNextToken(goal_str) && type == _all_but_sting) {
                int player_id = GetPlayerCharacterID();
                if (player_id != -1 && ReadCharacter(player_id).QueryIntFunction("int CombatSong()") != 1) {
                    level.SendMessage("start_dialogue_fade \"" + token_iter.GetToken(goal_str) + "\"");
                }
            }
        }
        if (token == "activate" && !EditorModeActive()) {
            if (token_iter.FindNextToken(goal_str) && type == _all_but_sting) {
                int id = atoi(token_iter.GetToken(goal_str));
                if (ObjectExists(id) && ReadObjectFromID(id).GetType() == _movement_object) {
                    ReadCharacterID(id).Execute("this_mo.static_char = false;");
                }
                if (ObjectExists(id) && ReadObjectFromID(id).GetType() == _hotspot_object) {
                    ReadObjectFromID(id).ReceiveScriptMessage("activate");
                }
            }
        }
        if (token == "disable" && !EditorModeActive()) {
            if (token_iter.FindNextToken(goal_str) && type == _all_but_sting) {
                int id = atoi(token_iter.GetToken(goal_str));
                if (ObjectExists(id)) {
                    Print("Disabling object " + id + "\n");
                    ReadObjectFromID(id).SetEnabled(false);
                }
            }
        }
        if (token == "enable" && !EditorModeActive()) {
            if (token_iter.FindNextToken(goal_str) && type == _all_but_sting) {
                int id = atoi(token_iter.GetToken(goal_str));
                if (ObjectExists(id)) {
                    ReadObjectFromID(id).SetEnabled(true);
                }
            }
        }
        if (token == "music_layer_override" && !EditorModeActive()) {
            if (token_iter.FindNextToken(goal_str) && type == _all_but_sting) {
                int id = atoi(token_iter.GetToken(goal_str));
                music_layer_override = id;
            }
        }
        if (token == "play_success_sting" && !EditorModeActive()) {
            if (type == _sting_only) {
                PlaySuccessSting();
            }
        }
    }
}

void TriggerGoalPre() {
    if (params.HasParam("goal_" + progress + "_pre")) {
        Log(info, "Triggering " + "goal_" + progress + "_pre");
        TriggerGoalString(params.GetString("goal_" + progress + "_pre"), _all_but_sting);
    }
}

void TriggerGoalPost(GoalTriggerType type) {
    if (params.HasParam("goal_" + progress + "_post")) {
        Log(info, "Triggering " + "goal_" + progress + "_post");
        TriggerGoalString(params.GetString("goal_" + progress + "_post"), type);
    }
}

void PlaySuccessSting() {
    if (sting_handle != -1) {
        StopSound(sting_handle);
        sting_handle = -1;
    }
    sting_handle = PlaySound(success_sting);
    SetSoundGain(sting_handle, GetConfigValueFloat("music_volume"));
    music_sting_end = the_time + 5.0;
    SetLayerGain(music_prefix + "layer_" + curr_music_layer, 0.0);
}

void PlayDeathSting() {
    if (sting_handle != -1) {
        StopSound(sting_handle);
        sting_handle = -1;
    }
    sting_handle = PlaySound(defeat_sting);
    SetSoundGain(sting_handle, GetConfigValueFloat("music_volume"));
    music_sting_end = the_time + 5.0;
    SetLayerGain(music_prefix + "layer_" + curr_music_layer, 0.0);
}

void IncrementProgress() {
    EnterTelemetryZone("IncrementProgress()");
    Log(info, "IncrementProgress: " + progress + " to " + (progress + 1));
    TriggerGoalPost(_all_but_sting);
    ++progress;
    win_time = -1.0;

    TriggerGoalPre();

    if (params.HasParam("goal_" + progress)) {
        string goal_str = params.GetString("goal_" + progress);
        TokenIterator token_iter;
        token_iter.Init();
        if (token_iter.FindNextToken(goal_str)) {
            if (token_iter.GetToken(goal_str) == "spawn_defeat") {
                while (token_iter.FindNextToken(goal_str)) {
                    string token = token_iter.GetToken(goal_str);
                    if (token == "no_delay") {
                        // do nothing
                    } else {
                        int id = atoi(token_iter.GetToken(goal_str));
                        if (ObjectExists(id) && ReadObjectFromID(id).GetType() == _movement_object) {
                            SetEnabledCharacterAndItems(id, true);
                        }
                    }
                }
            }
        }
    }

    int player_id = GetPlayerCharacterID();
    array<int> player_ids = GetControlledCharacterIDs();
    for (uint i = 0; i < player_ids.length(); i++) {
        EnterTelemetryZone("Restore player health");
        MovementObject@ mo = ReadCharacter(player_ids[i]);
        mo.ReceiveScriptMessage("restore_health");
        LeaveTelemetryZone();
    }
    LeaveTelemetryZone();

    if (win_time == -1.0) {
        PossibleWinEvent("character_defeated", -1, progress);
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
    EnterTelemetryZone("CheckReset()");
    TokenIterator token_iter;
    int num_player_spawn = 0;
    if (params.HasParam("player_spawn")) {
        string param_str = params.GetString("player_spawn");
        token_iter.Init();
        while (token_iter.FindNextToken(param_str)) {
            int id = atoi(token_iter.GetToken(param_str));
            if (ObjectExists(id)) {
                Object@ obj = ReadObjectFromID(id);
                if (obj.GetType() == _placeholder_object) {
                    PlaceholderObject@ placeholder_object = cast<PlaceholderObject@>(obj);
                    placeholder_object.SetPreview("Data/Objects/IGF_Characters/pale_turner.xml");
                    ++num_player_spawn;
                }
            }
        }
    }

    if (progress > num_player_spawn) {
        progress = num_player_spawn;
    }

    int num_characters = GetNumCharacters();
    for (int i = 0; i < num_characters; ++i) {
        SetEnabledCharacterAndItems(ReadCharacter(i).GetID(), true);
    }

    for (int i = 0; i < progress; ++i) {
        Log(info, "Iterating through completed goal: " + i);
        if (params.HasParam("goal_" + i)) {
            string goal_str = params.GetString("goal_" + i);
            token_iter.Init();
            if (token_iter.FindNextToken(goal_str)) {
                string goal_type = token_iter.GetToken(goal_str);
                if (goal_type == "defeat" || goal_type == "spawn_defeat" || goal_type == "defeat_optional") {
                    while (token_iter.FindNextToken(goal_str) && token_iter.GetToken(goal_str) != "") {
                        string token = token_iter.GetToken(goal_str);
                        if (token == "no_delay") {
                            // do nothing
                        } else {
                            int id = atoi(token_iter.GetToken(goal_str));
                            if (ObjectExists(id) && ReadObjectFromID(id).GetType() == _movement_object) {
                                SetEnabledCharacterAndItems(id, false);
                            }
                        }
                    }
                }
            }
        }
    }

    for (int i = progress + 1; params.HasParam("goal_" + i); ++i) {
        Log(info, "Iterating through future goals: " + i);
        if (params.HasParam("goal_" + i)) {
            string goal_str = params.GetString("goal_" + i);
            Log(info, "Goal_str: " + goal_str);
            token_iter.Init();
            if (token_iter.FindNextToken(goal_str)) {
                if (token_iter.GetToken(goal_str) == "spawn_defeat") {
                    while (token_iter.FindNextToken(goal_str)) {
                        string token = token_iter.GetToken(goal_str);
                        if (token == "no_delay") {
                            // do nothing
                        } else {
                            int id = atoi(token_iter.GetToken(goal_str));
                            if (ObjectExists(id) && ReadObjectFromID(id).GetType() == _movement_object) {
                                Log(info, "Disabling: " + id);
                                SetEnabledCharacterAndItems(id, false);
                            }
                        }
                    }
                }
            }
        }
    }

    array<int> player_ids = GetControlledCharacterIDs();
    for (uint i = 0; i < player_ids.length(); ++i) {
        MovementObject@ mo = ReadCharacter(player_ids[i]);
        if (progress == 0) {
            Object@ obj = ReadObjectFromID(mo.GetID());
            mo.position = obj.GetTranslation();
            mo.SetRotationFromFacing(obj.GetRotation() * vec3(0, 0, 1));
        } else {
            string param_str = params.GetString("player_spawn");
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
                    mo.Execute("SetCameraFromFacing(); SetOnGround(true); FixDiscontinuity();");
                }
            }
        }
    }

    TriggerGoalPre();
    LeaveTelemetryZone();
}

void PossibleWinEvent(const string &in event, int val, int goal_check, int recursion = 0) {
    if (ko_time != -1.0) {
        return;
    }
    Log(info, "PossibleWinEvent(" + event + ", " + val + ", " + goal_check + ", " + recursion + ")");
    if (recursion > 5000) {
        Log(error, "we have recursed over 5000 times, will break");
        return;
    }

    if (event == "checkpoint") {
        Log(info, "Player entered checkpoint: " + val);
        if (params.HasParam("goal_" + goal_check)) {
            string goal_str = params.GetString("goal_" + goal_check);
            Log(info, "Looking at goal: " + goal_str);
            TokenIterator token_iter;
            token_iter.Init();
            if (token_iter.FindNextToken(goal_str)) {
                string goal_type = token_iter.GetToken(goal_str);
                Log(info, "goal_type: " + goal_type);
                if (goal_type == "reach" || goal_type == "reach_skippable") {
                    if (token_iter.FindNextToken(goal_str)) {
                        int id = atoi(token_iter.GetToken(goal_str));
                        Log(info, "id: " + id);
                        if (id == val) {
                            win_time = the_time + 1.0;
                            TriggerGoalPost(_sting_only);
                            win_target = goal_check + 1;
                        } else if (goal_type == "reach_skippable") {
                            Log(info, "Checking next");
                            PossibleWinEvent(event, val, goal_check + 1, recursion + 1);
                        }
                    }
                }
                if (goal_type == "defeat_optional") {
                    PossibleWinEvent(event, val, goal_check + 1, recursion + 1);
                }
            }
        }
    } else if (event == "character_defeated") {
        Log(info, "Character defeated, checking goal");
        if (params.HasParam("goal_" + goal_check)) {
            string goal_str = params.GetString("goal_" + goal_check);
            Log(info, "Goal_str: " + goal_str);
            TokenIterator token_iter;
            token_iter.Init();
            if (token_iter.FindNextToken(goal_str)) {
                string goal_type = token_iter.GetToken(goal_str);
                if (goal_type == "defeat" || goal_type == "spawn_defeat" || goal_type == "defeat_optional") {
                    Log(info, "Checking defeat conditions");
                    bool success = true;
                    bool no_delay = false;
                    while (token_iter.FindNextToken(goal_str) && token_iter.GetToken(goal_str) != "") {
                        string token = token_iter.GetToken(goal_str);
                        if (token == "no_delay") {
                            no_delay = true;
                        } else {
                            Log(info, "Looking at token \"" + token_iter.GetToken(goal_str) + "\"");
                            int id = atoi(token_iter.GetToken(goal_str));
                            if (ObjectExists(id) && ReadObjectFromID(id).GetType() == _movement_object && ReadCharacterID(id).GetIntVar("knocked_out") == _awake) {
                                success = false;
                                Log(info, "Conditions failed, " + id + " is awake");
                            }
                        }
                    }
                    if (success) {
                        int player_id = GetPlayerCharacterID();
                        if (player_id != -1) {
                            EnterTelemetryZone("Restore player health");
                            MovementObject@ mo = ReadCharacter(player_id);
                            mo.QueueScriptMessage("restore_health");
                            LeaveTelemetryZone();
                        }
                        if (no_delay) {
                            win_time = the_time + 2.0;
                        } else {
                            win_time = the_time + 5.0;
                        }
                        TriggerGoalPost(_sting_only);
                        win_target = goal_check + 1;
                    } else if (goal_type == "defeat_optional") {
                        Log(info, "Checking next");
                        PossibleWinEvent(event, val, goal_check + 1, recursion + 1);
                    }
                }
                if (goal_type == "reach_skippable") {
                    PossibleWinEvent(event, val, goal_check + 1, recursion + 1);
                }
            }
        }
    }
}

enum MessageParseType {
    kSimple = 0,
    kOneInt = 1,
    kTwoInt = 2
}

int music_layer_override = -1;
bool crowd_override = false;
float crowd_gain_override;
float crowd_pitch_override;
array<float> dof_params;

void ReceiveMessage(string msg) {
    TokenIterator token_iter;
    token_iter.Init();
    if (!token_iter.FindNextToken(msg)) {
        return;
    }

    string msg_start = token_iter.GetToken(msg);
    if (msg_start == "reset") {
        queued_goal_check = true;
        ko_time = -1.0;
        win_time = -1.0;
        music_layer_override = -1;
    }

    if (msg_start == "level_event" &&
        token_iter.FindNextToken(msg)) {
        string sub_msg = token_iter.GetToken(msg);
        if (sub_msg == "music_layer_override") {
            if (token_iter.FindNextToken(msg)) {
                Log(info, "Set music_layer_override to " + atoi(token_iter.GetToken(msg)));
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
        } else if (sub_msg == "set_camera_dof") {
            dof_params.resize(0);
            while (token_iter.FindNextToken(msg)) {
                dof_params.push_back(atof(token_iter.GetToken(msg)));
            }
            if (dof_params.size() == 6) {
                camera.SetDOF(dof_params[0], dof_params[1], dof_params[2], dof_params[3], dof_params[4], dof_params[5]);
            }
        } else if (sub_msg == "character_knocked_out" || sub_msg == "character_died") {
            if (win_time == -1.0) {
                PossibleWinEvent("character_defeated", -1, progress);
            }
        }
    }

    MessageParseType type = kSimple;
    string token = token_iter.GetToken(msg);
    if (token == "knocked_over" ||
        token == "passive_blocked" ||
        token == "active_blocked" ||
        token == "dodged" ||
        token == "character_attack_feint" ||
        token == "character_attack_missed" ||
        token == "character_throw_escape" ||
        token == "character_thrown" ||
        token == "cut") {
        type = kTwoInt;
    } else if (token == "character_died" ||
               token == "character_knocked_out" ||
               token == "character_start_flip" ||
               token == "character_start_roll" ||
               token == "character_failed_flip" ||
               token == "item_hit") {
        type = kOneInt;
    }

    if (type == kOneInt) {
        token_iter.FindNextToken(msg);
        int char_a = atoi(token_iter.GetToken(msg));
        if (token == "character_died") {
            Log(info, "Player " + char_a + " was killed");
            audience_excitement += 4.0;
        } else if (token == "character_knocked_out") {
            Log(info, "Player " + char_a + " was knocked out");
            audience_excitement += 3.0;
        } else if (token == "character_start_flip") {
            Log(info, "Player " + char_a + " started a flip");
            audience_excitement += 0.4;
        } else if (token == "character_start_roll") {
            Log(info, "Player " + char_a + " started a roll");
            audience_excitement += 0.4;
        } else if (token == "character_failed_flip") {
            Log(info, "Player " + char_a + " failed a flip");
            audience_excitement += 1.0;
        } else if (token == "item_hit") {
            Log(info, "Player " + char_a + " was hit by an item");
            audience_excitement += 1.5;
        }
    } else if (type == kTwoInt) {
        token_iter.FindNextToken(msg);
        int char_a = atoi(token_iter.GetToken(msg));
        token_iter.FindNextToken(msg);
        int char_b = atoi(token_iter.GetToken(msg));
        if (token == "knocked_over") {
            Log(info, "Player " + char_a + " was knocked over by player " + char_b);
            audience_excitement += 1.5;
        } else if (token == "passive_blocked") {
            Log(info, "Player " + char_a + " passive-blocked an attack by player " + char_b);
            audience_excitement += 0.5;
        } else if (token == "active_blocked") {
            Log(info, "Player " + char_a + " active-blocked an attack by player " + char_b);
            audience_excitement += 0.7;
        } else if (token == "dodged") {
            Log(info, "Player " + char_a + " dodged an attack by player " + char_b);
            audience_excitement += 0.7;
        } else if (token == "character_attack_feint") {
            Log(info, "Player " + char_a + " feinted an attack aimed at " + char_b);
            audience_excitement += 0.4;
        } else if (token == "character_attack_missed") {
            Log(info, "Player " + char_a + " missed an attack aimed at " + char_b);
            audience_excitement += 0.4;
        } else if (token == "character_throw_escape") {
            Log(info, "Player " + char_a + " escaped a throw attempt by " + char_b);
            audience_excitement += 0.7;
        } else if (token == "character_thrown") {
            Log(info, "Player " + char_a + " was thrown by " + char_b);
            audience_excitement += 1.5;
        } else if (token == "cut") {
            Log(info, "Player " + char_a + " was cut by " + char_b);
            audience_excitement += 2.0;
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

    if (win_time == -1.0) {
        if (msg_start == "player_entered_checkpoint") {
            if (token_iter.FindNextToken(msg)) {
                int checkpoint_id = atoi(token_iter.GetToken(msg));
                PossibleWinEvent("checkpoint", checkpoint_id, progress);
            }
        }
    }
}

void SetMusicLayer(int layer) {
    if (kDebugText) {
        DebugText("music_layer", "music_layer: " + layer, 0.5);
        DebugText("music_layer_override", "music_layer_override: " + music_layer_override, 0.5);
        DebugText("music_prefix", "music_prefix: " + music_prefix, 0.5);
    }
    if (layer != curr_music_layer) {
        for (int i = 0; i < 5; ++i) {
            SetLayerGain(music_prefix + "layer_" + i, 0.0);
        }
        SetLayerGain(music_prefix + "layer_" + layer, 1.0);
        curr_music_layer = layer;
    }
}

void Update() {
    EnterTelemetryZone("Overgrowth Level Update");

    if (GetInputPressed(0, "k")) {
        ++progress;
        if (!params.HasParam("goal_" + progress)) {
            progress = 0;
        }
    }
    LeaveTelemetryZone();

    if (audience_sound_handle == -1 && music_prefix == "arena_") {
        audience_sound_handle = PlaySoundLoop("Data/Sounds/crowd/crowd_arena_general_1.wav", 0.0);
    } else if (audience_sound_handle != -1 && music_prefix != "arena_") {
        StopSound(audience_sound_handle);
        audience_sound_handle = -1;
    }

    float total_char_speed = 0.0;
    int num = GetNumCharacters();
    for (int i = 0; i < num; ++i) {
        MovementObject@ char = ReadCharacter(i);
        if (char.GetIntVar("knocked_out") == _awake) {
            total_char_speed += length(char.velocity);
        }
    }

    float excitement_decay_rate = 1.0 / (1.0 + total_char_speed / 14.0);
    excitement_decay_rate *= 3.0;
    audience_excitement *= pow(0.05, 0.001 * excitement_decay_rate);
    total_excitement += audience_excitement * time_step;

    float target_crowd_cheer_amount = audience_excitement * 0.1 + 0.15;
    float target_boo_amount = 0.0;
    if (crowd_override) {
        target_crowd_cheer_amount = crowd_gain_override;
        target_boo_amount = crowd_pitch_override;
    }
    boo_amount = mix(target_boo_amount, boo_amount, 0.98);
    crowd_cheer_vel += (target_crowd_cheer_amount - crowd_cheer_amount) * time_step * 10.0;
    if (crowd_cheer_vel > 0.0) {
        crowd_cheer_vel *= 0.99;
    } else {
        crowd_cheer_vel *= 0.95;
    }
    crowd_cheer_amount += crowd_cheer_vel * time_step;
    crowd_cheer_amount = max(crowd_cheer_amount, 0.1);

    SetSoundGain(audience_sound_handle, crowd_cheer_amount * 2.0);
    SetSoundPitch(audience_sound_handle, mix(min(0.8 + crowd_cheer_amount * 0.5, 1.2), 0.7, boo_amount));

    int player_id = GetPlayerCharacterID();
    if (player_id != -1 && ObjectExists(player_id)) {
        MovementObject@ char = ReadCharacter(player_id);
        bool use_keyboard = (max(last_mouse_event_time, last_keyboard_event_time) > last_controller_event_time);
        if (char.GetIntVar("knocked_out") != _awake) {
            string respawn = params.GetString("death_message");
            int index = respawn.findFirst("$attack$");
            while (index != -1) {
                respawn.erase(index, 8);
                respawn.insert(index, GetStringDescriptionForBinding(use_keyboard ? "key" : "gamepad_0", "attack"));
                index = respawn.findFirst("$attack$", index + 8);
            }

            if (GetConfigValueBool("tutorials")) {
                switch (DeathHint(char.GetIntVar("death_hint"))) {
                    case _hint_deflect_thrown:
                        level.SendMessage("tutorial " + "Press " + GetStringDescriptionForBinding(use_keyboard ? "key" : "gamepad_0", "grab") + " just before impact to catch or deflect projectiles." + respawn);
                        break;
                    case _hint_escape_throw:
                        level.SendMessage("tutorial " + "Press " + GetStringDescriptionForBinding(use_keyboard ? "key" : "gamepad_0", "grab") + " to escape from throws." + respawn);
                        break;
                    case _hint_cant_swim:
                        level.SendMessage("tutorial " + "Rabbits don't know how to swim." + respawn);
                        break;
                    case _hint_extinguish:
                        level.SendMessage("tutorial " + "If you catch on fire, you can roll to put yourself out." + respawn);
                        break;
                    case _hint_roll_stand:
                        level.SendMessage("tutorial " + "When knocked down, press " + GetStringDescriptionForBinding(use_keyboard ? "key" : "gamepad_0", "crouch") + " to roll to your feet" + respawn);
                        break;
                    case _hint_vary_attacks:
                        level.SendMessage("tutorial " + "Mix up your attacks to get through enemy defenses" + respawn);
                        break;
                    case _hint_stealth:
                        level.SendMessage("tutorial " + "Sometimes sneak attacks are much easier than direct combat." + respawn);
                        break;
                    default:
                        level.SendMessage("tutorial " + "" + respawn);
                }
            } else {
                level.SendMessage("screen_message " + "" + respawn);
            }
        }
    }
}

bool can_press_attack = false;

void PreDraw(float curr_game_time) {
    EnterTelemetryZone("Overgrowth Level PreDraw");

    if (kDebugText) {
        DebugText("progress", "progress: " + progress, 0.5);
    }
    if (queued_goal_check) {
        CheckReset();
        queued_goal_check = false;
    }

    int player_id = GetPlayerCharacterID();

    if (g_music_settings_are_valid) {
        if (ko_time == -1.0) {
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
            if (music_sting_end != 0.0 && ko_time == -1.0) {
                music_sting_end = 0.0;
                SetLayerGain(music_prefix + "layer_" + curr_music_layer, 1.0);
            }
        } else {
            SetLayerGain(music_prefix + "layer_" + curr_music_layer, 0.0);
        }
    }

    blackout_amount = 0.0;
    if (player_id != -1 && ObjectExists(player_id)) {
        MovementObject@ char = ReadCharacter(player_id);
        if (char.GetIntVar("knocked_out") != _awake) {
            if (ko_time == -1.0) {
                ko_time = the_time;
                PlayDeathSting();
                can_press_attack = false;
            }
            if (ko_time < the_time - 1.0) {
                if (!GetInputDown(0, "attack")) {
                    can_press_attack = true;
                }
                if (params.GetInt("press_attack_to_restart") == 1 && ((GetInputDown(0, "attack") && can_press_attack) || GetInputDown(0, "skip_dialogue") || GetInputDown(0, "keypadenter"))) {
                    if (sting_handle != -1) {
                        music_sting_end = the_time;
                        StopSound(sting_handle);
                        sting_handle = -1;
                    }
                    level.SendMessage("reset");
                    level.SendMessage("skip_dialogue");
                }
            }
            blackout_amount = 0.2 + 0.6 * (1.0 - pow(0.5, (the_time - ko_time)));
        } else {
            ko_time = -1.0;
        }
        ReadCharacter(player_id).SetFloatVar("level_blackout", blackout_amount);
    } else {
        ko_time = -1.0;
    }

    if (win_time != -1.0 && the_time > win_time && ko_time == -1.0) {
        while (progress < win_target) {
            IncrementProgress();
        }
    }

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
                    PlaceholderObject@ placeholder_object = cast<PlaceholderObject@>(obj);
                    placeholder_object.SetPreview("Data/Objects/IGF_Characters/pale_turner.xml");
                    ++num_player_spawn;
                }
            }
        }
    }
    LeaveTelemetryZone();
}

bool g_is_editor_enabled = false;

void LaunchCustomGUI() {
    g_is_editor_enabled = true;
}

const array<string> g_music_track_param_options = {
    "",
    "slaver",
    "swamp",
    "cats",
    "crete",
    "rescue",
    "arena",
    "custom_og_1_5"
};
const int kCustomMusicTrackParamIndex = int(g_music_track_param_options.length()) - 1;
const array<string> g_music_track_names = {
    "No Music",
    "Slaver",
    "Swamp",
    "Cats",
    "Crete",
    "Rescue",
    "Arena",
    "Custom"
};

const TextureAssetRef g_load_item_icon = LoadTexture(
    "Data/UI/ribbon/images/icons/color/Load-Item.png",
    TextureLoadFlags_NoMipmap | TextureLoadFlags_NoConvert |TextureLoadFlags_NoReduce);

string HexFromDigit_(int value) {
    switch (value) {
        case 0: return "0";
        case 1: return "1";
        case 2: return "2";
        case 3: return "3";
        case 4: return "4";
        case 5: return "5";
        case 6: return "6";
        case 7: return "7";
        case 8: return "8";
        case 9: return "9";
        case 10: return "A";
        case 11: return "B";
        case 12: return "C";
        case 13: return "D";
        case 14: return "E";
        case 15: return "F";
    }
    return "0";
}

string HexFromByte_(int value) {
    return HexFromDigit_(value / 16) + HexFromDigit_(value % 16);
}

string HexFromColor_(vec4 color) {
    return "\x1B" +
        HexFromByte_(int(color.x * 255)) +
        HexFromByte_(int(color.y * 255)) +
        HexFromByte_(int(color.z * 255)) +
        HexFromByte_(int(color.a * 255));
}

bool g_music_params_are_dirty = true;
bool g_current_custom_music_xml_filename_exists = false;
bool g_current_custom_success_sting_filename_exists = false;
bool g_current_custom_defeat_sting_filename_exists = false;

void DrawEditor() {
    Object@ obj = ReadObjectFromID(hotspot.GetID());
    DebugDrawBillboard("Data/Textures/ui/ogicon.png",
                       obj.GetTranslation(),
                       obj.GetScale()[1]*2.0,
                       vec4(vec3(0.5), 1.0),
                       _delete_on_draw);

    if (g_is_editor_enabled) {
        bool parameters_changed = false;

        if (g_music_params_are_dirty) {
            g_current_custom_music_xml_filename_exists = FileExists(g_music_custom_xml_filename);
            g_current_custom_success_sting_filename_exists = FileExists(success_sting);
            g_current_custom_defeat_sting_filename_exists = FileExists(defeat_sting);
            g_music_params_are_dirty = false;
        }

        ImGui_PushStyleVar(ImGuiStyleVar_WindowMinSize, vec2(625, 353));
        ImGui_Begin("Overgrowth Level", g_is_editor_enabled);

        bool p_press_attack_to_restart = params.GetInt("press_attack_to_restart") == 1;

        if (ImGui_Checkbox("Prompt to restart on death", p_press_attack_to_restart)) {
            params.SetInt("press_attack_to_restart", p_press_attack_to_restart ? 1 : 0);
            parameters_changed = true;
        }

        string p_music_type = params.HasParam("music") ? params.GetString("music") : "slaver";
        int music_param_index = g_music_track_param_options.find(p_music_type);

        if (music_param_index < 0) {
            music_param_index = 0;
        }

        ImGui_Separator();
        ImGui_Text("Selected music:");

        if (ImGui_ListBox("###Selected Music", music_param_index, g_music_track_names, g_music_track_names.length())) {
            params.SetString("music", g_music_track_param_options[music_param_index]);
            parameters_changed = true;
        }

        bool custom_music_selected = false;

        if (music_param_index == kCustomMusicTrackParamIndex) {
            AddCustomMusicParams_();
            custom_music_selected = true;

            ImGui_Text("Custom Music Parameters:");

            const string plain_color = HexFromColor_(ImGui_GetStyleColorVec4(ImGuiCol_Text));
            const string red_color = HexFromColor_(vec4(0.9, 0.0, 0.0, 1.0));
            const string orange_color = HexFromColor_(vec4(0.9, 0.5, 0.0, 1.0));

            const float indent_size = 10;
            const float text_input_width = 350;
            const float control_padding_size = 4;
            const float icon_button_icon_size = 15;
            const float icon_button_padding_size = 2;
            const float icon_button_full_size = icon_button_padding_size + icon_button_icon_size + icon_button_padding_size;
            const float text_label_offset = control_padding_size + 19 + control_padding_size;

            ImGui_Indent(indent_size);

            string xml_filename_field_color = g_current_custom_music_xml_filename_exists ? plain_color : orange_color;

            ImGui_PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, vec2(text_label_offset, control_padding_size));
            ImGui_PushItemWidth(text_input_width);
            if (ImGui_InputText(xml_filename_field_color + "Music XML filename###music_xml_filename", g_music_custom_xml_filename, 256)) {
                parameters_changed = true;
            }
            ImGui_PopItemWidth();
            ImGui_PopStyleVar();

            if (!g_current_custom_music_xml_filename_exists && ImGui_IsItemHovered()) {
                ImGui_SetTooltip("Specified file does not exist.\nPlease pick a different file");
            }

            ImGui_SameLine(control_padding_size + indent_size + control_padding_size + text_input_width + control_padding_size);
            ImGui_PushID("open_music_xml_file_button");
            if (ImGui_ImageButton(g_load_item_icon, vec2(icon_button_icon_size), vec2(0), vec2(1), icon_button_padding_size)) {
                string new_music_xml_filename = GetUserPickedReadPath("xml", "Data/Music");
                if (new_music_xml_filename != "") {
                    g_music_custom_xml_filename = new_music_xml_filename;
                    parameters_changed = true;
                }
            }
            ImGui_PopID();

            ImGui_SameLine(ImGui_GetWindowWidth() - 60.0);
            ImGui_SmallButton("Help");

            if (ImGui_IsItemHovered()) {
                ImGui_SetTooltip(
                    "This is the Overgrowth music .xml file that will be used,"
                    "\n  that contains a " + orange_color + " multi-layer track" + plain_color + " of music."
                    "\n"
                    "\nThe filename should be in the form:"
                    "\n" + orange_color + "Data/Music/<your_mod_name>/<your_music_filename>.xml"
                    "\n"
                    "\n" + plain_color + "Put that custom music XML file in this folder in your mod:"
                    "\n" + orange_color + "<your_mod_name>/Data/Music/<your_mod_name>"
                    "\n"
                    "\n" + plain_color + "Replace " + orange_color + "<your_mod_name>" + plain_color + " with your actual mod folder name."
                    "\n" + "Of course, don't include the " + orange_color + "<>" + plain_color + " brackets in your folder name."
                    "\n"
                    "\n" + "Make your mod folder name unique, with your username and mod name,"
                    "\n" + "   e.g. " + orange_color + "constance_therium2"
                    "\n" + plain_color + "Don't put files directly in " + orange_color + "<your_mod_name>/Data/Music" + plain_color + ","
                    "\n" + "   to avoid conflict with other mods\n"
                );
            }

            bool current_custom_song_is_playing = (GetSong() == g_music_custom_song_name);
            string custom_song_name_field_color = current_custom_song_is_playing ? plain_color : orange_color;

            ImGui_PushItemWidth(text_input_width);
            if (ImGui_InputText(custom_song_name_field_color + "Music Song Name###music_song_name", g_music_custom_song_name, 256)) {
                parameters_changed = true;
            }
            ImGui_PopItemWidth();

            if (!current_custom_song_is_playing && ImGui_IsItemHovered()) {
                ImGui_SetTooltip(
                    "Specified song is not playing."
                    "\nThe song does not exist, is not in the current music XML, or some script is overriding the current song."
                    "\nPlease fix the song name"
                );
            }

            ImGui_SameLine(ImGui_GetWindowWidth() - 60.0);
            ImGui_SmallButton("Help");

            if (ImGui_IsItemHovered()) {
                ImGui_SetTooltip(
                    "This is the song name that will be used."
                    "\n"
                    "\nThe song name is inside the Overgrowth music.xml,"
                    "\n  and it must be " + orange_color + "a multi-layer track" + plain_color + " of music."
                    "\n"
                    "\nFor an example of this multi-layer track of music,"
                    "\n   take a look at the file " + red_color + "Data/Music/slaver_loop/layers.xml" + plain_color
                    + "\n   in the game's install directory."
                    "\n"
                    "\n  " + red_color + "<Song name=\"" + orange_color + "slavers1" + red_color + "\" type=\"layered\">" + plain_color
                    + "\n      <SongRef name=\"slavers_layer_0\" />"
                    + "\n      ..."
                    "\n"
                    "\nThe " + red_color + "<Song name" + plain_color + " parameter is what you want to put in this field."
                    "\n   e.g. " + orange_color + "slavers1" + plain_color
                    + "\n   (be careful not to accidentally use the " + red_color + "<SongRef name" + plain_color + " parameter)"
                    "\n"
                    "\nThis type of song entry (" + red_color + "type=\"" + orange_color + "layered" + red_color + "\"" + plain_color + ")"
                    "\n   is the parent track of a multi-layer track."
                    "\n"
                    "\nThe individual layers look like the " + red_color + "single" + plain_color + " track above:"
                    "\n"
                    "\n     " + red_color + "<Song name=\"" + orange_color + "slavers_layer_0" + red_color + "\" type=\"single\" ... />" + plain_color
                );
            }

            bool current_music_layer_prefix_exists = (GetLayerNames().find(music_prefix + "layer_0") != -1);
            string music_layer_prefix_field_color = current_music_layer_prefix_exists ? plain_color : orange_color;

            ImGui_PushItemWidth(text_input_width);
            if (ImGui_InputText(music_layer_prefix_field_color + "Music Layer Prefix###music_layer_prefix", music_prefix, 256)) {
                parameters_changed = true;
            }
            ImGui_PopItemWidth();

            if (!current_music_layer_prefix_exists && ImGui_IsItemHovered()) {
                ImGui_SetTooltip(
                    "Specified layer prefix is not in the current song."
                    "\nEither this is not a layered song, or the layer with the name " + music_prefix + "layer_0 is missing."
                    "\nPlease fix the layer prefix, or pick a different song"
                );
            }

            ImGui_SameLine(ImGui_GetWindowWidth() - 60.0);
            ImGui_SmallButton("Help");

            if (ImGui_IsItemHovered()) {
                ImGui_SetTooltip(
                    "This is the first part of the layer names in the selected song."
                    "\n"
                    "\nThis must match the first part of the " + orange_color + "name" + plain_color + " parameter"
                    "\n   in the " + red_color + "<SongRef" + plain_color + " entries in the song."
                    "\n"
                    "\nFor example, if the " + red_color + "<SongRef" + plain_color + " entry looks like this:" + plain_color
                    + "\n"
                    + "\n   " + red_color + "<SongRef name=\"" + orange_color + "slavers_" + red_color + "layer_0\" />" + plain_color
                    + "\n"
                    "\nThen the value of this field should be:"
                    "\n"
                    "\n   " + orange_color + "slavers_" + plain_color
                    "\n"
                    "\nBe careful not to accidentally use the " + red_color + "<Song name" + plain_color + " parameter."
                    "\n"
                    "\nMake sure your layers are named " + orange_color + "<prefix>" + red_color + "layer_0" + plain_color + ", " + orange_color + "<prefix>" + red_color + "layer_1" + plain_color + ", etc,"
                    "\n   in your layered song, or the song layers won't play correctly."
                    "\n"
                    "\n" + "Of course, don't include the " + orange_color + "<>" + plain_color + " brackets in your layer prefix."
                );
            }

            string success_sting_field_color = g_current_custom_success_sting_filename_exists ? plain_color : orange_color;

            ImGui_PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, vec2(text_label_offset, control_padding_size));
            ImGui_PushItemWidth(text_input_width);
            if (ImGui_InputText(success_sting_field_color + "Success Sting Filename###success_sting_filename", success_sting, 256)) {
                parameters_changed = true;
            }
            ImGui_PopItemWidth();
            ImGui_PopStyleVar();

            if (!g_current_custom_success_sting_filename_exists && ImGui_IsItemHovered()) {
                ImGui_SetTooltip("Specified file does not exist.\nPlease pick a different .wav filename");
            }

            ImGui_SameLine(control_padding_size + indent_size + control_padding_size + text_input_width + control_padding_size);
            ImGui_PushID("open_success_sting_file_button");
            if (ImGui_ImageButton(g_load_item_icon, vec2(icon_button_icon_size), vec2(0), vec2(1), icon_button_padding_size)) {
                string new_sting_filename = GetUserPickedReadPath("wav", "Data/Music");
                if (new_sting_filename != "") {
                    success_sting = new_sting_filename;
                    parameters_changed = true;
                }
            }
            ImGui_PopID();

            ImGui_SameLine(ImGui_GetWindowWidth() - 60.0);
            ImGui_SmallButton("Help");

            if (ImGui_IsItemHovered()) {
                ImGui_SetTooltip(
                    "This is the music that plays when the player has successfully completed a goal."
                    "\n"
                    "\nThe music must be a .wav file, and the filename should be in the form:"
                    "\n" + orange_color + "Data/Music/<your_mod_name>/<your_success_sting_filename>.wav"
                    "\n"
                    "\n" + plain_color + "Put that custom music .wav file in this folder in your mod:"
                    "\n" + orange_color + "<your_mod_name>/Data/Music/<your_mod_name>"
                    "\n"
                    "\n" + plain_color + "Replace " + orange_color + "<your_mod_name>" + plain_color + " with your actual mod folder name."
                    "\n" + "Of course, don't include the " + orange_color + "<>" + plain_color + " brackets in your folder name."
                    "\n"
                    "\n" + "Make your mod folder name unique, with your username and mod name,"
                    "\n" + "   e.g. " + orange_color + "constance_therium2"
                    "\n" + plain_color + "Don't put files directly in " + orange_color + "<your_mod_name>/Data/Music" + plain_color + ","
                    "\n" + "   to avoid conflict with other mods\n"
                );
            }

            string defeat_sting_field_color = g_current_custom_defeat_sting_filename_exists ? plain_color : orange_color;

            ImGui_PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, vec2(text_label_offset, control_padding_size));
            ImGui_PushItemWidth(text_input_width);
            if (ImGui_InputText(defeat_sting_field_color + "Defeat Sting Filename###defeat_sting_filename", defeat_sting, 256)) {
                parameters_changed = true;
            }
            ImGui_PopItemWidth();
            ImGui_PopStyleVar();

            if (!g_current_custom_defeat_sting_filename_exists && ImGui_IsItemHovered()) {
                ImGui_SetTooltip("Specified file does not exist.\nPlease pick a different file");
            }

            ImGui_SameLine(control_padding_size + indent_size + control_padding_size + text_input_width + control_padding_size);
            ImGui_PushID("open_defeat_sting_file_button");
            if (ImGui_ImageButton(g_load_item_icon, vec2(icon_button_icon_size), vec2(0), vec2(1), icon_button_padding_size)) {
                string new_sting_filename = GetUserPickedReadPath("wav", "Data/Music");
                if (new_sting_filename != "") {
                    defeat_sting = new_sting_filename;
                    parameters_changed = true;
                }
            }
            ImGui_PopID();

            ImGui_SameLine(ImGui_GetWindowWidth() - 60.0);
            ImGui_SmallButton("Help");

            if (ImGui_IsItemHovered()) {
                ImGui_SetTooltip(
                    "This is the music that plays when the player is defeated."
                    "\n"
                    "\nThe music must be a .wav file, and the filename should be in the form:"
                    "\n" + orange_color + "Data/Music/<your_mod_name>/<your_defeat_sting_filename>.wav"
                    "\n"
                    "\n" + plain_color + "Put that custom music .wav file in this folder in your mod:"
                    "\n" + orange_color + "<your_mod_name>/Data/Music/<your_mod_name>"
                    "\n"
                    "\n" + plain_color + "Replace " + orange_color + "<your_mod_name>" + plain_color + " with your actual mod folder name."
                    "\n" + "Of course, don't include the " + orange_color + "<>" + plain_color + " brackets in your folder name."
                    "\n"
                    "\n" + "Make your mod folder name unique, with your username and mod name,"
                    "\n" + "   e.g. " + orange_color + "constance_therium2"
                    "\n" + plain_color + "Don't put files directly in " + orange_color + "<your_mod_name>/Data/Music" + plain_color + ","
                    "\n" + "   to avoid conflict with other mods\n"
                );
            }

            ImGui_Unindent();
        }

        ImGui_End();
        ImGui_PopStyleVar();

        if (parameters_changed) {
            g_music_params_are_dirty = true;

            if (custom_music_selected) {
                params.SetString("music_custom_xml_filename", g_music_custom_xml_filename);
                params.SetString("music_custom_song_name", g_music_custom_song_name);
                params.SetString("music_custom_layer_prefix", music_prefix);
                params.SetString("music_custom_success_sting_wav_filename", success_sting);
                params.SetString("music_custom_defeat_sting_wav_filename", defeat_sting);
            }

            SetParameters();
        }
    }
}
