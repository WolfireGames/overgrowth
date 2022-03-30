//-----------------------------------------------------------------------------
//           Name: arena_level.as
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
#include "music_load.as"

MusicLoad ml("Data/Music/lugaru_new.xml");

// Difficulty of current collection of enemies
float curr_difficulty;

// For trying to complete level in a given amount of time
float target_time;
float time;

// Audience info
float audience_excitement;
float total_excitement;
int audience_size;
int audience_sound_handle;
float crowd_cheer_amount;
float crowd_cheer_vel;

// Persistent info
int fan_base;
float player_skill;
const float MIN_PLAYER_SKILL = 0.5f;
const float MAX_PLAYER_SKILL = 1.9f;
array<vec3> player_colors;

// Level state
vec3 initial_player_pos;
bool initial_player_pos_set;

enum LevelOutcome {
    kUnknown = 0,
    kVictory = 1,
    kFailure = 2
}
LevelOutcome level_outcome;

enum MetaEventType {
    kDisplay,
    kWait,
    kMessage
}

class MetaEvent {
    MetaEventType type;
    string data;
}

const int kMaxMetaStates = 10;
array<string> meta_states;

const int kMaxMetaEvents = 100;
array<MetaEvent> meta_events;
int meta_event_start;
int meta_event_end;
uint64 meta_event_wait;
float wait_player_move_dist;
bool wait_for_click;

int player_id;

uint64 global_time; // in ms

array<int> match_score;

string level_name;

// Text display info
int main_text_id;
int ingame_text_id;
float text_visible;
bool show_text;

// All objects spawned by the script
array<int> spawned_object_ids;

int TextInit(int width, int height){
    int id = level.CreateTextElement();
    TextCanvasTexture @text = level.GetTextElement(id);
    text.Create(width, height);
    return id;
}

void AddMetaEvent(MetaEventType type, string data) {
    int next_meta_event_end = (meta_event_end+1)%kMaxMetaEvents;
    if(next_meta_event_end == meta_event_start){
        DisplayError("Error", "Too many meta events to add new one");
        return;
    }
    meta_events[meta_event_end].type = type;
    meta_events[meta_event_end].data = data;
    meta_event_end = next_meta_event_end;
}

void ClearMetaEvents() {
    meta_event_start = 0;
    meta_event_end = 0;
}

void ClearMeta() {
    ClearMetaEvents();
    UpdateIngameText("");
    show_text = false;
    meta_event_wait = 0;
    wait_player_move_dist = 0.0f;
    wait_for_click = false;
    score_hud.target_blackout_amount = 0.0f;
    score_hud.target_skill_visible_amount = 0.0f;
    score_hud.target_score_visible_amount = 0.0f;
}

void ReadPersistentInfo() {
    SavedLevel @saved_level = save_file.GetSavedLevel(level_name);
    string fan_base_str = saved_level.GetValue("fan_base");
    if(fan_base_str == ""){
        fan_base = 0;
    } else {
        fan_base = atoi(fan_base_str);
    }
    string player_color_str = saved_level.GetValue("player_colors");
    if(player_color_str == ""){
        SetPlayerColors();
    } else {
        TokenIterator token_iter;
        token_iter.Init();
        player_colors.resize(4);
        for(int i=0; i<4; ++i){
            for(int j=0; j<3; ++j){
                token_iter.FindNextToken(player_color_str);
                player_colors[i][j] = atof(token_iter.GetToken(player_color_str));
            }
        }
    }
    string player_skill_str = saved_level.GetValue("player_skill");
    if(player_skill_str == ""){
        player_skill = MIN_PLAYER_SKILL;
    } else {
        player_skill = atof(player_skill_str);
    }
}

void WritePersistentInfo() {
    SavedLevel @saved_level = save_file.GetSavedLevel(level_name);
    saved_level.SetValue("fan_base",""+fan_base);
    string player_colors_str;
    for(int i=0; i<4; ++i){
        for(int j=0; j<3; ++j){
            player_colors_str += player_colors[i][j];
            player_colors_str += " ";
        }
    }
    saved_level.SetValue("player_colors",""+player_colors_str);
    saved_level.SetValue("player_skill",""+player_skill);
    save_file.WriteInPlace();
}

// Called by level.cpp at start of level
void Init(string str) {
    PlaySong("lugaru_combat");

    level_name = str;
    ReadPersistentInfo();

    meta_states.resize(kMaxMetaStates);
    meta_events.resize(kMaxMetaEvents);
    int meta_event_start = 0;
    int meta_event_end = 0;
    meta_event_wait = global_time;
    wait_player_move_dist = 0.0f;
    wait_for_click = false;

    main_text_id = TextInit(512,512);
    ingame_text_id = TextInit(512,512);
    curr_difficulty = GetRandomDifficultyNearPlayerSkill();
    audience_sound_handle = -1;
    SetUpLevel(curr_difficulty);
    crowd_cheer_amount = 0.0f;
    crowd_cheer_vel = 0.0f;
    show_text = false;
    text_visible = 0.0f;
    ReceiveMessage("update_skill");

    level.SendMessage("disable_retry");
}

void DeleteObjectsInList(array<int> &inout ids){
    int num_ids = ids.length();
    for(int i=0; i<num_ids; ++i){
        Log(info, "Test");
        DeleteObjectID(ids[i]);
    }
    ids.resize(0);
}

// Instantiate an object at the location of another object
// E.g. create a character at the location of a placeholder spawn point
Object@ SpawnObjectAtSpawnPoint(Object@ spawn, string &in path){
    int obj_id = CreateObject(path, true);
    spawned_object_ids.push_back(obj_id);
    Object @new_obj = ReadObjectFromID(obj_id);
    new_obj.SetTranslation(spawn.GetTranslation());
    vec4 rot_vec4 = spawn.GetRotationVec4();
    quaternion q(rot_vec4.x, rot_vec4.y, rot_vec4.z, rot_vec4.a);
    new_obj.SetRotation(q);
    return new_obj;
}

// Attach a specific preview path to a given placeholder object
void SetSpawnPointPreview(Object@ spawn, string &in path){
    PlaceholderObject@ placeholder_object = cast<PlaceholderObject@>(spawn);
    placeholder_object.SetPreview(path);
}

// Find spawn points and set which object is displayed as a preview
void SetPlaceholderPreviews() {
    array<int> @object_ids = GetObjectIDs();
    int num_objects = object_ids.length();
    for(int i=0; i<num_objects; ++i){
        Object @obj = ReadObjectFromID(object_ids[i]);
        ScriptParams@ params = obj.GetScriptParams();
        if(params.HasParam("Name")){
            string name_str = params.GetString("Name");
            if("character_spawn" == name_str){
                SetSpawnPointPreview(obj,level.GetPath("spawn_preview"));
            }
            if("weapon_spawn" == name_str){
                SetSpawnPointPreview(obj,level.GetPath("weap_preview"));
            }
        }
    }
}

// This script has no need of input focus
bool HasFocus(){
    return false;
}

float MoveTowards(float curr, float target, float amount){
    float val;
    if(curr < target){
        val = min(curr + amount, target);
    } else if(curr > target){
        val = max(curr - amount, target);
    } else {
        val = target;
    }
    return val;
}

class ScoreHUD {
    array<int> players_per_team;
    array<vec3> team_colors;
    int num_teams;
    int max_score;

    float target_skill_visible_amount;
    float skill_visible_amount;

    float target_blackout_amount;
    float blackout_amount;

    float player_skill_display;
    float target_player_skill_display;

    float level_skill_display;
    float target_level_skill_display;

    float score_visible_amount;
    float target_score_visible_amount;

    void Update() {
        player_skill_display = MoveTowards(player_skill_display, target_player_skill_display, 0.2f * time_step);
        level_skill_display = MoveTowards(level_skill_display, target_level_skill_display, 1.0f * time_step);
        blackout_amount = MoveTowards(blackout_amount, target_blackout_amount, 2.0f * time_step);
        skill_visible_amount = MoveTowards(skill_visible_amount, target_skill_visible_amount, 2.0f * time_step);
        score_visible_amount = MoveTowards(score_visible_amount, target_score_visible_amount, 2.0f * time_step);
    }

    void DrawGUIHUD() {
        if(blackout_amount > 0.0f){
            HUDImage @blackout_image = hud.AddImage();
            blackout_image.SetImageFromPath("Data/Textures/diffuse.tga");
            blackout_image.position.y = (GetScreenWidth() + GetScreenHeight())*-1.0f;
            blackout_image.position.x = (GetScreenWidth() + GetScreenHeight())*-1.0f;
            blackout_image.position.z = -2.0f;
            blackout_image.scale = vec3(GetScreenWidth() + GetScreenHeight())*2.0f;
            blackout_image.color = vec4(0.0f,0.0f,0.0f,blackout_amount);
        }

        float display_height = GetScreenHeight() - 100;
        float height = 20;

        if(skill_visible_amount > 0.0f){
            float width = 400;
            vec3 color = vec3(1.0f);
            // Display skill bar
            {
                HUDImage @image = hud.AddImage();
                image.SetImageFromPath(level.GetPath("diffuse_tex"));
                image.position.x = GetScreenWidth() * 0.5f - width * 0.5f;
                image.position.y = display_height - height * 0.5f;
                image.position.z = 3;
                image.color = vec4(color,skill_visible_amount);
                image.scale = vec3(width / image.GetWidth(), height / image.GetHeight(), 1.0);
            }
            // Display player skill
            {
                float small_width = 5;
                HUDImage @image = hud.AddImage();
                image.SetImageFromPath(level.GetPath("diffuse_tex"));
                image.position.x = GetScreenWidth() * 0.5f - width * 0.5f + player_skill_display * (width - small_width);
                image.position.y = display_height - height * 0.5f + height;
                image.position.z = 3;
                image.color = vec4(color,skill_visible_amount);
                image.scale = vec3(small_width / image.GetWidth(), height / image.GetHeight(), 1.0);
            }
            // Display match difficulty
            {
                float skill_display = level_skill_display;

                float small_width = 5;
                HUDImage @image = hud.AddImage();
                image.SetImageFromPath(level.GetPath("diffuse_tex"));
                image.position.x = GetScreenWidth() * 0.5f - width * 0.5f + skill_display * (width - small_width);
                image.position.y = display_height - height * 0.5f - height;
                image.position.z = 3;
                image.color = vec4(color,skill_visible_amount);
                image.scale = vec3(small_width / image.GetWidth(), height / image.GetHeight(), 1.0);
            }
        }

        if(score_visible_amount > 0.0f){
            // Display current score
            for(int i=0; i<int(players_per_team.size()); ++i){
                players_per_team[i] = 0;
            }

            int num_chars = GetNumCharacters();
            for(int i=0; i<num_chars; ++i){
                MovementObject@ char = ReadCharacter(i);
                ScriptParams@ params = ReadObjectFromID(char.GetID()).GetScriptParams();
                int team = atoi(params.GetString("Teams"));
                if(team+1 > int(players_per_team.size())) {
                    players_per_team.resize(team+1);
                }
                ++players_per_team[team];
            }

            int num_teams = 0;
            for(int i=0; i<int(players_per_team.size()); ++i){
                if(players_per_team[i] > 0){
                    ++num_teams;
                    if(int(team_colors.size()) < num_teams){
                        team_colors.resize(num_teams);
                    }
                    team_colors[num_teams-1] = ColorFromTeam(i);
                }
            }

            const int team_display_width = 20;
            int total_width = team_display_width * num_teams + team_display_width * (num_teams - 1);
            for(int i=0; i<num_teams; ++i){
                for(int j=0; j<MaxScore(); ++j){
                    {
                        HUDImage @image = hud.AddImage();
                        image.SetImageFromPath(level.GetPath("diffuse_tex"));
                        image.position.x = GetScreenWidth() * 0.5f + team_display_width * 2 * i - total_width / 2;
                        image.position.y = display_height - height * 0.5f - height - team_display_width - team_display_width*j*2;
                        image.position.z = 3;
                        float opac = 0.25;
                        if(match_score[i] > j) {
                            opac = 1.0;
                        }
                        image.color = vec4(team_colors[i],opac * score_visible_amount);
                        image.scale = vec3(team_display_width / image.GetWidth(), team_display_width / image.GetHeight(), 1.0);
                    }
                }
            }
        }
    }
}

ScoreHUD score_hud;


// This script has no GUI elements
void DrawGUI() {
    // Do not draw the GUI in editor mode (it makes it hard to edit)
    /*if(GetPlayerCharacterID() == -1){
        return;
    }*/
    score_hud.DrawGUIHUD();

    float ui_scale = 0.5f;
    float visible = 1.0f;
    float display_time = time;

    {   HUDImage @image = hud.AddImage();
        image.SetImageFromPath(level.GetPath("diffuse_tex"));
        float stretch = GetScreenHeight() / image.GetHeight();
        image.position.x = GetScreenWidth() * 0.4f - 200;
        image.position.y = ((1.0-visible) * GetScreenHeight() * -1.2);
        image.position.z = 3;
        image.tex_scale.y = 20;
        image.tex_scale.x = 20;
        image.color = vec4(0.6f,0.8f,0.6f,text_visible*0.8f);
        image.scale = vec3(460 / image.GetWidth(), stretch, 1.0);}

    {   HUDImage @image = hud.AddImage();
        image.SetImageFromText(level.GetTextElement(main_text_id));
        image.position.x = int(GetScreenWidth() * 0.4f - 200 + 10);
        image.position.y = GetScreenHeight()-500;
        image.position.z = 4;
        image.color = vec4(1,1,1,text_visible);}

    {   HUDImage @image = hud.AddImage();
        image.SetImageFromText(level.GetTextElement(ingame_text_id));
        image.position.x = GetScreenWidth()/2-256;
        image.position.y = GetScreenHeight()-500;
        image.position.z = 4;
        image.color = vec4(1,1,1,1);}
}

// Convert byte colors to float colors (255,0,0) to (1.0f,0.0f,0.0f)
vec3 FloatTintFromByte(const vec3 &in tint){
    vec3 float_tint;
    float_tint.x = tint.x / 255.0f;
    float_tint.y = tint.y / 255.0f;
    float_tint.z = tint.z / 255.0f;
    return float_tint;
}

// Create a random color tint, avoiding excess saturation
vec3 RandReasonableColor(){
    vec3 color;
    color.x = rand()%255;
    color.y = rand()%255;
    color.z = rand()%255;
    float avg = (color.x + color.y + color.z) / 3.0f;
    color = mix(color, vec3(avg), 0.7f);
    return color;
}

class SpawnPoint {
    int team;
    int obj_id;
}

vec3 GetRandomFurColor() {
    vec3 fur_color_byte;
    int rnd = rand()%6;
    switch(rnd){
    case 0: fur_color_byte = vec3(255); break;
    case 1: fur_color_byte = vec3(34); break;
    case 2: fur_color_byte = vec3(137); break;
    case 3: fur_color_byte = vec3(105,73,54); break;
    case 4: fur_color_byte = vec3(53,28,10); break;
    case 5: fur_color_byte = vec3(172,124,62); break;
    }
    return FloatTintFromByte(fur_color_byte);
}

void SetPlayerColors() {
    player_colors.resize(4);
    player_colors[0] = GetRandomFurColor();
    player_colors[1] = GetRandomFurColor();
    player_colors[3] = GetRandomFurColor();
}

vec3 ColorFromTeam(int which_team){
    switch(which_team){
        case 0: return vec3(1,0,0);
        case 1: return vec3(0,0,1);
        case 2: return vec3(0,0.5f,0.5f);
        case 3: return vec3(1,1,0);
    }
    return vec3(1,1,1);
}

// Create a random enemy at spawn point obj, with a given skill level
void CreateEnemy(Object@ obj, float difficulty, int team){
    string actor_path; // Path to actor xml
    int fur_channel = -1; // Which tint mask channel corresponds to fur
    int rnd = rand()%2+1;
    switch(rnd){
    case 0:
        actor_path = level.GetPath("char_civ");
        break;
    case 1:
        fur_channel = 1;
        actor_path = level.GetPath("char_guard");
        break;
    case 2:
        fur_channel = 0;
        actor_path = level.GetPath("char_raider");
        break;
    }
    // Spawn actor
    Object@ char_obj = SpawnObjectAtSpawnPoint(obj,actor_path);
    // Set palette colors randomly, darkening based on skill
    for(int i=0; i<4; ++i){
        vec3 color = FloatTintFromByte(RandReasonableColor());
        float tint_amount = 0.5f;
        color = mix(color, ColorFromTeam(team), tint_amount);
        color = mix(color, vec3(1.0-difficulty), 0.5f);
        char_obj.SetPaletteColor(i, color);
    }
    char_obj.SetPaletteColor(fur_channel, GetRandomFurColor());
    // Set character parameters based on difficulty
    ScriptParams@ params = char_obj.GetScriptParams();
    params.SetString("Teams", ""+team);
    params.SetFloat("Character Scale", mix(RangedRandomFloat(0.9f,1.0f), RangedRandomFloat(1.0f,1.1f), difficulty));
    params.SetFloat("Fat", mix(RangedRandomFloat(0.4f,0.7f), RangedRandomFloat(0.4f,0.5f), difficulty));
    params.SetFloat("Muscle", mix(RangedRandomFloat(0.3f,0.5f), RangedRandomFloat(0.5f,0.7f), difficulty));
    params.SetFloat("Ear Size", RangedRandomFloat(0.5f,2.5f));
    params.SetFloat("Block Follow-up", mix(RangedRandomFloat(0.01f,0.25f), RangedRandomFloat(0.75f,1.0f), difficulty));
    params.SetFloat("Block Skill", mix(RangedRandomFloat(0.01f,0.25f), RangedRandomFloat(0.5f,0.8f), difficulty));
    params.SetFloat("Movement Speed", mix(RangedRandomFloat(0.8f,1.0f), RangedRandomFloat(0.9f,1.1f), difficulty));
    params.SetFloat("Attack Speed", mix(RangedRandomFloat(0.8f,1.0f), RangedRandomFloat(0.9f,1.1f), difficulty));
    float damage = mix(RangedRandomFloat(0.3f,0.5f), RangedRandomFloat(0.9f,1.1f), difficulty);
    params.SetFloat("Attack Knockback", damage);
    params.SetFloat("Attack Damage", damage);
    params.SetFloat("Aggression", RangedRandomFloat(0.25f,0.75f));
    params.SetFloat("Ground Aggression", mix(0.0f, 1.0f, difficulty));
    params.SetFloat("Damage Resistance", mix(RangedRandomFloat(0.6f,0.8f), RangedRandomFloat(0.9f,1.1f), difficulty));
    params.SetInt("Left handed", (rand()%5==0)?1:0);
    if(rand()%2==0){
        params.SetString("Unarmed Stance Override", level.GetPath("alt_stance_anim"));
    }
    char_obj.UpdateScriptParams();
}

void CharactersNoticeEachOther() {
    int num_chars = GetNumCharacters();
    for(int i=0; i<num_chars; ++i){
         MovementObject@ char = ReadCharacter(i);
         for(int j=i+1; j<num_chars; ++j){
             MovementObject@ char2 = ReadCharacter(j);
             Log(info, "Telling characters " + char.GetID() + " and " + char2.GetID() + " to notice each other.");
             char.ReceiveScriptMessage("notice " + char2.GetID());
             char2.ReceiveScriptMessage("notice " + char.GetID());
         }
     }
}

// Spawn all of the objects that we'll need in the level of given total difficulty
void SetUpLevel(float initial_difficulty){
    // Remove all spawned objects
    DeleteObjectsInList(spawned_object_ids);
    spawned_object_ids.resize(0);

    // Determine which spawn points we are using
    bool knife_test = false;
    int game_type_int = rand()%3;
    if(knife_test){
        game_type_int = 0;
    }

    // Identify all the spawn points for the current game type
    array<int> @object_ids = GetObjectIDs();
    array<SpawnPoint> character_spawns;
    int num_objects = object_ids.length();
    for(int i=0; i<num_objects; ++i){
        Object @obj = ReadObjectFromID(object_ids[i]);
        ScriptParams@ params = obj.GetScriptParams();
        if(params.HasParam("Name")){
            string name_str = params.GetString("Name");
            if("character_spawn" == name_str){
                bool correct_game_type = false;
                if(params.HasParam("game_type")){
                    string game_type = params.GetString("game_type");
                    obj.SetEditorLabel("game type: " + game_type);
                    if(game_type == ""+game_type_int){
                        correct_game_type = true;
                    }
                }
                if(correct_game_type){
                    character_spawns.resize(character_spawns.size() + 1);
                    SpawnPoint@ sp = character_spawns[character_spawns.size() - 1];
                    sp.obj_id = object_ids[i];
                    if(params.HasParam("team")){
                        string team_str = params.GetString("team");
                        sp.team = atoi(team_str);
                    } else {
                        sp.team = -1;
                    }
                }
            }
            /*if("weapon_spawn" == name_str){
                Object@ weap_obj = SpawnObjectAtSpawnPoint(obj,"Data/Items/DogWeapons/DogBroadSword.xml");
            }*/
        }
    }

    // Spawn characters for each spawn point, with randomly selected player
    int num_char_spawns = character_spawns.size();
    player_id = rand()%num_char_spawns;
    for(int i=0; i<num_char_spawns; ++i){
        Object @obj = ReadObjectFromID(character_spawns[i].obj_id);
        if(i != player_id){
            CreateEnemy(obj, initial_difficulty-0.5f, character_spawns[i].team);
        } else {
            Object@ char_obj = SpawnObjectAtSpawnPoint(obj, level.GetPath("char_player"));
            char_obj.SetPlayer(true);
            ScriptParams@ char_params = char_obj.GetScriptParams();
            int team = character_spawns[i].team;
            char_params.SetString("Teams", ""+team);

            vec3 color = FloatTintFromByte(RandReasonableColor());
            float tint_amount = 0.5f;
            color = mix(color, ColorFromTeam(team), tint_amount);
            color = mix(color, vec3(1.0-(player_skill-0.5f)), 0.5f);
            player_colors[2] = color;

            for(int j=0; j<4; ++j){
                char_obj.SetPaletteColor(j, player_colors[j]);
            }
        }
    }

    // Determine which weapons we are using
    bool use_weapons = knife_test || rand()%3==0;
    if(use_weapons){
        string weap_str;
        int rnd = rand()%4;
        if(knife_test){
            rnd = 0;
        }
        switch(rnd){
        case 0: weap_str = level.GetPath("weap_knife"); break;
        case 1: weap_str = level.GetPath("weap_big_sword"); break;
        case 2: weap_str = level.GetPath("weap_sword"); break;
        case 3: weap_str = level.GetPath("weap_spear"); break;
        }

        int num_chars = GetNumCharacters();
        for(int i=0; i<num_chars; ++i){
            MovementObject@ char_obj = ReadCharacter(i);
            Object@ obj = ReadObjectFromID(char_obj.GetID());
            Object@ item_obj = SpawnObjectAtSpawnPoint(obj,weap_str);
            ScriptParams@ params = obj.GetScriptParams();
            bool mirrored = false;
            if(params.HasParam("Left handed") && params.GetInt("Left handed") != 0){
                mirrored = true;
            }
            obj.AttachItem(item_obj, _at_grip, mirrored);
        }
    }

    // Divide up difficulty to provide a mix of weak and strong enemies
    // that add up to the given total difficulty
    /*float difficulty = initial_difficulty;
    Print("Total difficulty: "+difficulty+"\n");
    array<float> enemy_difficulties;
    while(difficulty > 0.0f){
        if(difficulty < 1.0f){
            enemy_difficulties.push_back(difficulty);
            difficulty = 0.0f;
        } else if(difficulty < 1.5f){
            if(rand()%2 == 0){
                enemy_difficulties.push_back(difficulty);
                difficulty = 0.0f;
            } else {
                float temp_difficulty = RangedRandomFloat(0.5f,min(1.5f, difficulty-0.5));
                enemy_difficulties.push_back(temp_difficulty);
                difficulty -= temp_difficulty;
            }
        } else {
            float temp_difficulty = RangedRandomFloat(0.5f,1.5f);
            enemy_difficulties.push_back(temp_difficulty);
            difficulty -= temp_difficulty;
        }
    }
    bool print_difficulties = false;
    if(print_difficulties){
        Print("Enemy difficulties: ");
        for(int i=0; i<int(enemy_difficulties.size()); ++i){
            Print(""+enemy_difficulties[i]+", ");
        }
        Print("\n");
    }
    // Assign each enemy to a random unused spawn point, and instantiate them there
    int num_enemies = min(enemy_difficulties.size(), enemy_spawns.size());
    array<int> chosen(enemy_spawns.size(), 0);
    for(int i=0; i<num_enemies; ++i){
        int next_enemy = rand()%(enemy_spawns.size() - i);
        int counter = -1;
        int j = 0;
        while (next_enemy >= 0){
            ++counter;
            while(chosen[counter] != 0){
                ++counter;
            }
            --next_enemy;
        }
        chosen[counter] = 1;
        Object @obj = ReadObjectFromID(enemy_spawns[counter]);
        CreateEnemy(obj, enemy_difficulties[i]-0.5f);
    } */

    // Reset level timer info
    target_time = 10.0f;
    time = 0.0f;
    // Reset audience info
    audience_excitement = 0.0f;
    total_excitement = 0.0f;
    // Audience size increases exponentially based on difficulty
    audience_size = int((rand()%1000+100)*pow(4.0f,initial_difficulty)*0.1f);
    if(audience_sound_handle == -1){
        audience_sound_handle = PlaySoundLoop(level.GetPath("crowd_sound"),0.0f);
    }
    level_outcome = kUnknown;
    int num_scores = 4;
    match_score.resize(num_scores);
    for(int i=0; i<num_scores; ++i){
        match_score[i] = 0;
    }

    SetAllHostile(false);
    CharactersNoticeEachOther();

    ClearMeta();
    AddMetaEvent(kMessage, "set_meta_state 0 pre_intro");
    AddMetaEvent(kMessage, "set_intro_text");
    AddMetaEvent(kMessage, "set show_text true");
    //AddMetaEvent(kMessage, "wait_for_player_move 1.0");

    AddMetaEvent(kMessage, "set show_text false");
    AddMetaEvent(kMessage, "set_meta_state 0 intro");
    //AddMetaEvent(kWait, "0.5");

    AddMetaEvent(kDisplay, "Welcome to the arena!");
    //AddMetaEvent(kWait, "2.0");

    if(use_weapons){
        AddMetaEvent(kDisplay, "This is a fight to the death!");
        AddMetaEvent(kMessage, "set_meta_state 1 one_point");
    } else {
        AddMetaEvent(kDisplay, "Two points to win!");
        AddMetaEvent(kMessage, "set_meta_state 1 two_points");
    }
    //AddMetaEvent(kWait, "2.0");

    AddMetaEvent(kDisplay, "Time to fight!");
    AddMetaEvent(kMessage, "set_all_hostile true");
    AddMetaEvent(kMessage, "set_meta_state 0 fighting");
    //AddMetaEvent(kWait, "2.0");

    AddMetaEvent(kDisplay, "");
    //TimedSlowMotion(0.5f,7.0f, 0.0f);
}

enum MessageParseType {
    kSimple = 0,
    kOneInt = 1,
    kTwoInt = 2
}

void AggressionDetected(int attacker_id){
    /*if(meta_states[0] != "fighting" && meta_states[0] != "fighting_over_but_allowed"){
        if(ReadCharacterID(attacker_id).controlled){
            ClearMeta();
            AddMetaEvent(kMessage, "set_meta_state 0 disqualify");
            AddMetaEvent(kDisplay, "Player is disqualified!");
            AddMetaEvent(kWait, "2.0");
            EndMatch(false);
        }
    }*/

}

// Parse string messages and react to them
void ReceiveMessage(string msg) {
    TokenIterator token_iter;
    token_iter.Init();
    if(!token_iter.FindNextToken(msg)){
        return;
    }

    // Handle simple tokens, or mark as requiring extra parameters
    MessageParseType type = kSimple;
    string token = token_iter.GetToken(msg);
    if(token == "post_reset"){
        SetUpLevel(curr_difficulty);
    } else if(token == "restore_health"){
        int num_chars = GetNumCharacters();
        for(int i=0; i<num_chars; ++i){
            MovementObject@ char = ReadCharacter(i);
            char.ReceiveScriptMessage("restore_health");
        }
    } else if(token == "randomize_difficulty") {
        curr_difficulty = GetRandomDifficultyNearPlayerSkill();
    } else if(token == "new_match"){
        SetUpLevel(curr_difficulty);
    } else if(token == "set_all_hostile"){
        token_iter.FindNextToken(msg);
        string param = token_iter.GetToken(msg);
        if(param == "false"){
            SetAllHostile(false);
        } else if(param == "true"){
            SetAllHostile(true);
        }
    } else if(token == "set"){
        token_iter.FindNextToken(msg);
        string param1 = token_iter.GetToken(msg);
        token_iter.FindNextToken(msg);
        string param2 = token_iter.GetToken(msg);
        if(param1 == "show_text"){
            if(param2 == "false"){
                show_text = false;
            } else if(param2 == "true"){
                show_text = true;
            }
        }
        if(param1 == "skill_visible"){
            if(param2 == "false"){
                score_hud.target_blackout_amount = 0.0f;
                score_hud.target_skill_visible_amount = 0.0f;
            } else if(param2 == "true"){
                score_hud.target_blackout_amount = 1.0f;
                score_hud.target_skill_visible_amount = 1.0f;
            }
        }
        if(param1 == "score_visible"){
            if(param2 == "false"){
                score_hud.target_blackout_amount = 0.0f;
                score_hud.target_score_visible_amount = 0.0f;
            } else if(param2 == "true"){
                score_hud.target_blackout_amount = 0.75f;
                score_hud.target_score_visible_amount = 1.0f;
            }
        }
    } else if(token == "set_intro_text"){
        SetIntroText();
    } else if(token == "update_skill"){
        score_hud.target_player_skill_display = (player_skill - MIN_PLAYER_SKILL) / (MAX_PLAYER_SKILL - MIN_PLAYER_SKILL);
        score_hud.target_level_skill_display = (curr_difficulty - MIN_PLAYER_SKILL) / (MAX_PLAYER_SKILL - MIN_PLAYER_SKILL);
    } else if(token == "wait_for_player_move"){
        token_iter.FindNextToken(msg);
        string param1 = token_iter.GetToken(msg);
        wait_player_move_dist = atof(param1);
        initial_player_pos_set = false;
    } else if(token == "wait_for_click"){
        wait_for_click = true;
    } else if(token == "set_meta_state"){
        token_iter.FindNextToken(msg);
        string param1 = token_iter.GetToken(msg);
        token_iter.FindNextToken(msg);
        string param2 = token_iter.GetToken(msg);
        meta_states[atoi(param1)] = param2;
    } else if(token == "dispose_level"){
        StopSound(audience_sound_handle);
    } else if(token == "knocked_over" ||
              token == "passive_blocked" ||
              token == "active_blocked" ||
              token == "dodged" ||
              token == "character_attack_feint" ||
              token == "character_attack_missed" ||
              token == "character_throw_escape" ||
              token == "character_thrown" ||
              token == "cut")
    {
        type = kTwoInt;
    } else if(token == "character_died" ||
              token == "character_knocked_out" ||
              token == "character_start_flip" ||
              token == "character_start_roll" ||
              token == "character_failed_flip"||
              token == "item_hit")
    {
        type = kOneInt;
    }

    if(type == kOneInt){
        token_iter.FindNextToken(msg);
        int char_a = atoi(token_iter.GetToken(msg));
        if(token == "character_died"){
            Log(info, "Player "+char_a+" was killed");
            audience_excitement += 4.0f;
        } else if(token == "character_knocked_out"){
            Log(info, "Player "+char_a+" was knocked out");
            audience_excitement += 3.0f;
        } else if(token == "character_start_flip"){
            Log(info, "Player "+char_a+" started a flip");
            audience_excitement += 0.4f;
        } else if(token == "character_start_roll"){
            Log(info, "Player "+char_a+" started a roll");
            audience_excitement += 0.4f;
        } else if(token == "character_failed_flip"){
            Log(info, "Player "+char_a+" failed a flip");
            audience_excitement += 1.0f;
        } else if(token == "item_hit"){
            Log(info, "Player "+char_a+" was hit by an item");
            audience_excitement += 1.5f;
        }
    } else if(type == kTwoInt){
        token_iter.FindNextToken(msg);
        int char_a = atoi(token_iter.GetToken(msg));
        token_iter.FindNextToken(msg);
        int char_b = atoi(token_iter.GetToken(msg));
        if(token == "knocked_over"){
            Log(info, "Player "+char_a+" was knocked over by player "+char_b);
            audience_excitement += 1.5f;
            AggressionDetected(char_b);
        } else if(token == "passive_blocked"){
            Log(info, "Player "+char_a+" passive-blocked an attack by player "+char_b);
            audience_excitement += 0.5f;
            AggressionDetected(char_b);
        } else if(token == "active_blocked"){
            Log(info, "Player "+char_a+" active-blocked an attack by player "+char_b);
            audience_excitement += 0.7f;
            AggressionDetected(char_b);
        } else if(token == "dodged"){
            Log(info, "Player "+char_a+" dodged an attack by player "+char_b);
            audience_excitement += 0.7f;
            AggressionDetected(char_b);
        } else if(token == "character_attack_feint"){
            Log(info, "Player "+char_a+" feinted an attack aimed at "+char_b);
            audience_excitement += 0.4f;
        } else if(token == "character_attack_missed"){
            Log(info, "Player "+char_a+" missed an attack aimed at "+char_b);
            audience_excitement += 0.4f;
        } else if(token == "character_throw_escape"){
            Log(info, "Player "+char_a+" escaped a throw attempt by "+char_b);
            audience_excitement += 0.7f;
        } else if(token == "character_thrown"){
            Log(info, "Player "+char_a+" was thrown by "+char_b);
            audience_excitement += 1.5f;
        } else if(token == "cut"){
            Log(info, "Player "+char_a+" was cut by "+char_b);
            audience_excitement += 2.0f;
            AggressionDetected(char_b);
        }
    }
}

float ProbabilityOfWin(float a, float b){
    float a2 = a*a;
    float b2 = b*b;
    return a2 / (a2 + b2);
}

float GetRandomDifficultyNearPlayerSkill() {
    float var = player_skill * RangedRandomFloat(0.5f,1.5f);
    var = min(max(var, MIN_PLAYER_SKILL), MAX_PLAYER_SKILL);
    return var;
}

void EndMatch(bool victory){
    float win_prob = ProbabilityOfWin(player_skill, curr_difficulty);
    float excitement_level = 1.0f-pow(0.9f,total_excitement*0.3f);
    const float kMatchImportance = 0.3f; // How much this match influences your skill evaluation

    if(!victory){ // Decrease difficulty on failure
        level_outcome = kFailure;
        float audience_fan_ratio = 0.0f;
        if(win_prob < 0.5f){ // If you were predicted to lose, you still gain some fans
            audience_fan_ratio += (0.5f - win_prob) * kMatchImportance;
        }
        audience_fan_ratio += (1.0f - audience_fan_ratio) * excitement_level * 0.4f;
        int new_fans = int(audience_size * audience_fan_ratio);
        fan_base += new_fans;
        player_skill -= player_skill * win_prob * kMatchImportance;
        player_skill = min(max(player_skill, MIN_PLAYER_SKILL), MAX_PLAYER_SKILL);
        SetLoseText(new_fans, excitement_level);
        PlaySoundGroup("Data/Sounds/versus/fight_lose2.xml");
    } else if(victory){ // Increase difficulty on win
        level_outcome = kVictory;
        player_skill += curr_difficulty * (1.0f - win_prob) * kMatchImportance;
        player_skill = min(max(player_skill, MIN_PLAYER_SKILL), MAX_PLAYER_SKILL);
        float audience_fan_ratio = (1.0f - win_prob) * kMatchImportance;
        audience_fan_ratio += (1.0f - audience_fan_ratio) * excitement_level;
        int new_fans = int(audience_size * audience_fan_ratio);
        fan_base += new_fans;
        SetWinText(new_fans, fan_base, excitement_level);
        PlaySoundGroup("Data/Sounds/versus/fight_win2.xml");
    }

    WritePersistentInfo();

    AddMetaEvent(kWait, "1.0");
    AddMetaEvent(kMessage, "set_all_hostile false");
    AddMetaEvent(kMessage, "set_meta_state 0 fighting_over_but_allowed");
    AddMetaEvent(kMessage, "set skill_visible true");
    AddMetaEvent(kWait, "1.0");
    AddMetaEvent(kMessage, "update_skill");
    AddMetaEvent(kWait, "1.0");
    for(int i=0; i<8; ++i){
        AddMetaEvent(kMessage, "randomize_difficulty");
        AddMetaEvent(kMessage, "update_skill");
        AddMetaEvent(kWait, "0.1");
    }
    AddMetaEvent(kWait, "0.8");
    AddMetaEvent(kDisplay, "");
    AddMetaEvent(kMessage, "new_match");
    AddMetaEvent(kMessage, "set skill_visible false");
    AddMetaEvent(kWait, "1.0");

}

int MaxScore() {
    int max_score = 1;
    if(meta_states[1] == "two_points"){
        max_score = 2;
    }
    return max_score;
}

// Check if level should be reset
void VictoryCheck() {
    if(level_outcome == kUnknown){
        bool multiple_teams_alive = false;
        bool any_team_alive = false;
        string team_alive;

        int num = GetNumCharacters();
        for(int i=0; i<num; ++i){
            MovementObject@ char = ReadCharacter(i);
            if(char.GetIntVar("knocked_out") == _awake){
                ScriptParams@ params = ReadObjectFromID(char.GetID()).GetScriptParams();
                if(!any_team_alive){
                    any_team_alive = true;
                    team_alive = params.GetString("Teams");
                } else if(team_alive != params.GetString("Teams")){
                    multiple_teams_alive = true;
                }
            }
        }

        if(!multiple_teams_alive && meta_states[0] == "fighting"){
            // Store id of the team that won the last round
            int last_round_winner = -1;
            if("0" == team_alive){
                last_round_winner = 0;
            } else if("1" == team_alive){
                last_round_winner = 1;
            } else if("2" == team_alive){
                last_round_winner = 2;
            } else if("3" == team_alive){
                last_round_winner = 3;
            }
            // Increment winning team score
            if(last_round_winner != -1){
                ++match_score[last_round_winner];
            }

            Log(info, "Last round winner: " + last_round_winner);
            int max_score = MaxScore();


            if(match_score[last_round_winner] < max_score){
                ClearMeta();
                AddMetaEvent(kMessage, "set_all_hostile false");
                AddMetaEvent(kMessage, "set_meta_state 0 fighting_over_but_allowed");
                AddMetaEvent(kMessage, "set_meta_state 0 resetting");
                if(last_round_winner != -1){
                    bool victory = false;
                    int num_chars = GetNumCharacters();
                    for(int i=0; i<num_chars; ++i){
                        MovementObject@ char = ReadCharacter(i);
                        Object@ obj = ReadObjectFromID(char.GetID());
                        ScriptParams@ params = obj.GetScriptParams();
                        if(char.controlled && params.HasParam("Teams") && params.GetString("Teams") == team_alive){
                            victory = true;
                        }
                    }
                    if(victory){
                        PlaySoundGroup("Data/Sounds/versus/fight_win1.xml");
                    } else {
                        PlaySoundGroup("Data/Sounds/versus/fight_lose1.xml");
                    }
                }
                AddMetaEvent(kMessage, "set score_visible true");
                AddMetaEvent(kWait, "2.0");
                AddMetaEvent(kMessage, "set score_visible false");
                AddMetaEvent(kMessage, "restore_health");

                AddMetaEvent(kDisplay, "Get ready for the next round...");
                //AddMetaEvent(kWait, "2.0");
                AddMetaEvent(kDisplay, "Fight!");
                AddMetaEvent(kMessage, "set_all_hostile true");
                AddMetaEvent(kMessage, "set_meta_state 0 fighting");
                //AddMetaEvent(kWait, "2.0");
                AddMetaEvent(kDisplay, "");
            } else {
                bool victory = false;
                int num_chars = GetNumCharacters();
                for(int i=0; i<num_chars; ++i){
                    MovementObject@ char = ReadCharacter(i);
                    Object@ obj = ReadObjectFromID(char.GetID());
                    ScriptParams@ params = obj.GetScriptParams();
                    if(char.controlled && params.HasParam("Teams") && params.GetString("Teams") == team_alive){
                        victory = true;
                    }
                }
                ClearMeta();
                EndMatch(victory);
            }
        }
    }

    if(show_text){
        text_visible += time_step;
        text_visible = min(1.0f, text_visible);
    } else {
        text_visible -= time_step;
        text_visible = max(0.0f, text_visible);
    }
}

void UpdateIngameText(string str) {
    TextCanvasTexture @text = level.GetTextElement(ingame_text_id);
    text.ClearTextCanvas();
    string font_str = level.GetPath("font");
    TextStyle style;
    style.font_face_id = GetFontFaceID(font_str, 48);

    vec2 pen_pos = vec2(0,256);
    int line_break_dist = 42;
    text.SetPenPosition(pen_pos);
    text.SetPenColor(255,255,255,255);
    text.SetPenRotation(0.0f);

    text.AddText(str, style, UINT32MAX);

    text.UploadTextCanvasToTexture();
}

void SetIntroText() {
    TextCanvasTexture @text = level.GetTextElement(main_text_id);
    text.ClearTextCanvas();
    string font_str = level.GetPath("font");
    TextStyle small_style, big_style;
    small_style.font_face_id = GetFontFaceID(font_str, 48);
    big_style.font_face_id = GetFontFaceID(font_str, 72);

    vec2 pen_pos = vec2(0,256);
    text.SetPenPosition(pen_pos);
    text.SetPenColor(0,0,0,255);
    text.SetPenRotation(0.0f);
    text.AddText("Odds are ", small_style, UINT32MAX);
    float prob = ProbabilityOfWin(player_skill, curr_difficulty);
    int a, b;
    OddsFromProbability(1.0f-prob, a, b);
    if(a < b){
        text.AddText("" + b+":"+a, big_style, UINT32MAX);
        text.AddText(" in your favor", small_style, UINT32MAX);
    } else if(a > b){
        text.AddText("" + a+":"+b, big_style, UINT32MAX);
        text.AddText(" against you", small_style, UINT32MAX);
    } else {
        text.AddText("even", small_style, UINT32MAX);
    }

    int line_break_dist = 42;
    pen_pos.y += line_break_dist;
    text.SetPenPosition(pen_pos);
    text.AddText("There are ",small_style, UINT32MAX);
    text.AddText(""+audience_size,big_style, UINT32MAX);
    text.AddText(" spectators",small_style, UINT32MAX);

    pen_pos.y += line_break_dist;
    text.SetPenPosition(pen_pos);
    text.AddText("Good luck!",small_style, UINT32MAX);

    text.UploadTextCanvasToTexture();
}

void SetWinText(int new_fans, int total_fans, float excitement_level) {
    TextCanvasTexture @text = level.GetTextElement(main_text_id);
    text.ClearTextCanvas();
    string font_str = level.GetPath("font");
    TextStyle small_style, big_style;
    small_style.font_face_id = GetFontFaceID(font_str, 48);
    big_style.font_face_id = GetFontFaceID(font_str, 72);

    vec2 pen_pos = vec2(0,256);
    text.SetPenPosition(pen_pos);
    text.SetPenColor(0,0,0,255);
    text.SetPenRotation(0.0f);
    text.AddText("You won!", small_style, UINT32MAX);

    int line_break_dist = 42;
    pen_pos.y += line_break_dist;
    text.SetPenPosition(pen_pos);
    text.AddText("You gained ",small_style, UINT32MAX);
    text.AddText(""+new_fans,big_style, UINT32MAX);
    text.AddText(" fans",small_style, UINT32MAX);

    pen_pos.y += line_break_dist;
    text.SetPenPosition(pen_pos);
    text.AddText("Your fanbase totals ",small_style, UINT32MAX);
    text.AddText(""+total_fans,big_style, UINT32MAX);

    pen_pos.y += line_break_dist;
    text.SetPenPosition(pen_pos);
    text.AddText("Your skill assessment is now ",small_style, UINT32MAX);
    text.AddText(""+int((player_skill-0.5f)*40+1),big_style, UINT32MAX);

    pen_pos.y += line_break_dist;
    text.SetPenPosition(pen_pos);
    text.AddText("Audience ",small_style, UINT32MAX);
    text.AddText(""+int(excitement_level * 100.0f) + "%",big_style, UINT32MAX);
    text.AddText(" entertained",small_style, UINT32MAX);

    text.UploadTextCanvasToTexture();
}

void SetLoseText(int new_fans, float excitement_level) {
    TextCanvasTexture @text = level.GetTextElement(main_text_id);
    text.ClearTextCanvas();
    string font_str = level.GetPath("font");
    TextStyle small_style, big_style;
    small_style.font_face_id = GetFontFaceID(font_str, 48);
    big_style.font_face_id = GetFontFaceID(font_str, 72);
    int line_break_dist = 42;

    vec2 pen_pos = vec2(0,256);
    text.SetPenPosition(pen_pos);
    text.SetPenColor(0,0,0,255);
    text.SetPenRotation(0.0f);
    text.AddText("You were defeated.", small_style, UINT32MAX);

    if(new_fans > 0){
        pen_pos.y += line_break_dist;
        text.SetPenPosition(pen_pos);
        text.AddText("You gained ",small_style, UINT32MAX);
        text.AddText(""+new_fans, big_style, UINT32MAX);
        text.AddText(" new fans",small_style, UINT32MAX);

        pen_pos.y += line_break_dist;
        text.SetPenPosition(pen_pos);
        text.AddText("Your fanbase totals ",small_style, UINT32MAX);
        text.AddText(""+fan_base,big_style, UINT32MAX);
    }

    pen_pos.y += line_break_dist;
    text.SetPenPosition(pen_pos);
    text.AddText("Your skill assessment is now ",small_style, UINT32MAX);
    text.AddText(""+int((player_skill-0.5f)*40+1),big_style, UINT32MAX);

    pen_pos.y += line_break_dist;
    text.SetPenPosition(pen_pos);
    text.AddText("Audience ",small_style, UINT32MAX);
    text.AddText(""+int(excitement_level * 100.0f) + "%",big_style, UINT32MAX);
    text.AddText(" entertained",small_style, UINT32MAX);

    text.UploadTextCanvasToTexture();
}

void OddsFromProbability(float prob, int &out a, int &out b) {
    // Escape if 0 or 1, to avoid divide by zero
    if(prob == 0.0f){
        a = 0;
        b = 1;
        return;
    }
    if(prob == 1.0f){
        a = 1;
        b = 0;
        return;
    }
    string str;
    // If probability is 0.5, we want 1:1 odds, or 1.0 ratio
    // If probability is 0.33, we want 1:2 odds, or 0.5 ratio
    // Convert probability to ratio
    float target_ratio = prob / (1.0f - prob);
    int closest_numerator = -1;
    int closest_denominator = -1;
    float closest_dist = 0.0f;
    for(int i=1; i<10; ++i){
        for(int j=1; j<10; ++j){
            float val = i/float(j);
            float dist = abs(target_ratio - val);
            if(closest_numerator == -1 || dist < closest_dist){
                closest_dist = dist;
                closest_numerator = i;
                closest_denominator = j;
            }
        }
    }
    if(closest_numerator == 1 && closest_denominator == 9){
        closest_denominator = int(1.0f/target_ratio);
    }
    if(closest_numerator == 9 && closest_denominator == 1){
        closest_numerator = int(target_ratio);
    }
    a = closest_numerator;
    b = closest_denominator;
}

void SetAllHostile(bool val){
    int num_chars = GetNumCharacters();
    if(val == true){
        for(int i=0; i<num_chars; ++i){
             MovementObject@ char = ReadCharacter(i);
             char.ReceiveScriptMessage("set_combat_allowed true");
         }
         CharactersNoticeEachOther();
    } else {
        for(int i=0; i<num_chars; ++i){
            MovementObject@ char = ReadCharacter(i);
            char.ReceiveScriptMessage("set_combat_allowed false");
        }
    }
}

void SendMessageToAllCharacters(const string &in msg){
    int num_chars = GetNumCharacters();
    for(int i=0; i<num_chars; ++i){
        MovementObject@ char = ReadCharacter(i);
        char.ReceiveScriptMessage(msg);
    }
}

// Returns true if a wait event was encountered
void ProcessMetaEvent(MetaEvent me){
    switch(me.type){
    case kWait:
        meta_event_wait = uint64(global_time + 1000 * atof(me.data));
        break;
    case kDisplay:
        UpdateIngameText(me.data);
        break;
    case kMessage:
        ReceiveMessage(me.data);
        break;
    }
}

void UpdateMetaEventWait() {
    if(wait_player_move_dist > 0.0f){
        if(player_id != -1){
            MovementObject@ player_char = ReadCharacter(player_id);
            if(!initial_player_pos_set){
                initial_player_pos = player_char.position;
                initial_player_pos_set = true;
            }
            if(xz_distance_squared(initial_player_pos, player_char.position) > wait_player_move_dist){
                wait_player_move_dist = 0.0f;
            }
        }
    }
    if(wait_for_click){
        if(GetInputDown(0, "attack")){
            wait_for_click = false;
        }
    }
}

bool MetaEventWaiting(){
    bool waiting = false;
    if(global_time < meta_event_wait){
        waiting = true;
    }
    if(wait_player_move_dist > 0.0f){
        waiting = true;
    }
    if(wait_for_click){
        waiting = true;
    }
    return waiting;
}

void Update() {
    score_hud.Update();

    global_time += uint64(time_step * 1000);

    SetPlaceholderPreviews();
    if(DebugKeysEnabled() && GetInputPressed(0, "t")){
        curr_difficulty = GetRandomDifficultyNearPlayerSkill();
        SetUpLevel(curr_difficulty);
    }

    VictoryCheck();
    time += time_step;
    // Get total amount of character movement
    float total_char_speed = 0.0f;
    int num = GetNumCharacters();
    for(int i=0; i<num; ++i){
        MovementObject@ char = ReadCharacter(i);
        if(char.GetIntVar("knocked_out") == _awake){
            total_char_speed += length(char.velocity);
        }
    }
    // Decay excitement based on total character movement
    float excitement_decay_rate = 1.0f / (1.0f + total_char_speed / 14.0f);
    excitement_decay_rate *= 3.0f;
    audience_excitement *= pow(0.05f, 0.001f*excitement_decay_rate);
    total_excitement += audience_excitement * time_step;
    // Update crowd sound effect volume and pitch based on excitement
    float target_crowd_cheer_amount = audience_excitement * 0.1f + 0.15f;
    crowd_cheer_vel += (target_crowd_cheer_amount - crowd_cheer_amount) * time_step * 10.0f;
    if(crowd_cheer_vel > 0.0f){
        crowd_cheer_vel *= 0.99f;
    } else {
        crowd_cheer_vel *= 0.95f;
    }
    crowd_cheer_amount += crowd_cheer_vel * time_step;
    crowd_cheer_amount = max(crowd_cheer_amount, 0.1f);
    SetSoundGain(audience_sound_handle, crowd_cheer_amount*2.0f);
    SetSoundPitch(audience_sound_handle, min(0.8f + crowd_cheer_amount * 0.5f,1.2f));

    bool debug_text = false;
    if(debug_text){
        DebugText("a","Difficulty: "+curr_difficulty*2.0f,0.5f);
        DebugText("ab","Player Skill: "+player_skill*2.0f,0.5f);
        float prob = ProbabilityOfWin(player_skill, curr_difficulty);
        DebugText("ac","Probability of player win: "+ProbabilityOfWin(player_skill, curr_difficulty),0.5f);
        int a, b;
        OddsFromProbability(1.0f-prob, a, b);
        DebugText("ad","Odds against win: "+a+":"+b,0.5f);
        DebugText("b","Target time: "+target_time,0.5f);
        DebugText("c","Current time: "+time,0.5f);
        DebugText("d","Excitement: "+audience_excitement,0.5f);
        DebugText("g","Total excitement: "+total_excitement,0.5f);
        DebugText("h","Audience size:  "+audience_size,0.5f);
        DebugText("ha","Fans:  "+fan_base,0.5f);
        DebugText("hb","target_crowd_cheer_amount:  "+target_crowd_cheer_amount,0.5f);
        DebugText("i","crowd_cheer_amount:  "+crowd_cheer_amount,0.5f);
        DebugText("j","crowd_cheer_vel:  "+crowd_cheer_vel,0.5f);
    }

    UpdateMetaEventWait();
    while(meta_event_start != meta_event_end && !MetaEventWaiting()){
        ProcessMetaEvent(meta_events[meta_event_start]);
        meta_event_start = (meta_event_start+1)%kMaxMetaEvents;
    }
}
