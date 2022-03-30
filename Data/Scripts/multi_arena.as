//-----------------------------------------------------------------------------
//           Name: multi_arena.as
//      Developer: Wolfire Games LLC
//    Script Type:
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
#include "multi_arena/multi_arena_text.as"
#include "multi_arena/multi_arena_battle.as"
#include "arena_meta_persistence.as"
#include "music_load.as"

MusicLoad ml("Data/Music/arena.xml");

enum MusicState
{
    PlayerDead,
    Tempo1,
    Tempo2,
    Tempo3,
    Tempo4,
    Tempo5
}

class CountdownTimer
{
    CountdownTimer( float _duration )
    {
        duration = _duration;
        Reset();
    }

    float start_time;
    float duration;
    bool checked = false;

    bool Check()
    {
        if( checked == false )
        {
            if( the_time - start_time > duration )
            {
                checked = true;
                return true;
            }
        } 
        return false;
    }

    void Reset()
    {
        start_time = the_time;
        checked = false;
    }
};


class MusicSegmentHandler
{
    MusicState current_state;
    MusicState last_used_state; 
    
    string current_segment;
    
    CountdownTimer player_last_hurt_countdown(10.0f);
    CountdownTimer aggression_reduction_counter(3.0f);

    int aggression_count;

    MusicSegmentHandler()
    {
        aggression_count = 0;
        current_segment = "";
        current_state = PlayerDead;
        last_used_state = Tempo1;
    }

    void UpdatePlayedSegment()
    {
        if( last_used_state != current_state )
        { 
            bool do_queue = false;
            string segment = "";
            switch( current_state )
            {
                case PlayerDead:
                    //Let segment finish when ending =)
                    do_queue = true;
                    segment = "fight1-ver1";
                    break; 
                case Tempo1:
                    segment = "fight2-ver1";
                    break;
                case Tempo2:
                    segment = "fight3-ver1";//"fight3-ver2"] );
                    break;
                case Tempo3:
                    segment = "fight4-ver1"; //"fight4-ver2"] );
                    break;
                case Tempo4:
                    segment = "fight5-ver1"; //"fight5-ver2"] ); 
                    break;
                case Tempo5:
                    segment = "fight6-ver1";
                    break;
            }

            if( current_segment != segment )
            {
                Log(info, "Setting Segment to " + segment );
                if( do_queue )
                    QueueSegment( segment );
                else
                    PlaySegment( segment );
                current_segment = segment;
            }
            last_used_state = current_state;
        }
    }

    void CheckPlayerHitReduce()
    {
        if( player_last_hurt_countdown.Check() )
        {
            switch( current_state )
            {
                case Tempo2:
                    current_state = Tempo1;
                    break;
                case Tempo3:
                    current_state = Tempo2;
                    break;
                default:
                    break;
            }
        }
    }
    
    void CheckAnyAggressionReduce()
    {
        if( aggression_reduction_counter.Check() )
        {
            if( aggression_count > 0 )
                aggression_count--;
            aggression_reduction_counter.Reset();
        }

        switch( current_state )
        {
            case Tempo2:
                if( aggression_count < 3 )
                {
                    current_state = Tempo1;
                }
                break;
            case Tempo3:
                if( aggression_count < 5 )
                {
                    current_state = Tempo2;
                }
                break;
            case Tempo4:
                if( aggression_count < 7 )
                {
                    current_state = Tempo3;
                }
                break;
            case Tempo5:
                if( aggression_count < 12 )
                {
                    current_state = Tempo4;
                }
                break;
        }
    }

    void Update()
    {
        CheckPlayerHitReduce();
        CheckAnyAggressionReduce();

        UpdatePlayedSegment();
    }

    
    void PlayerDied()
    {
        /*
        switch( current_state )
        {
            case PlayerDead:
                //Nothing;
                break;
            default:
                current_state = PlayerDead;
                break;
        }
        */
    }   

    void PlayerRevived()
    {
        /*
        switch( current_state )
        {
            case PlayerDead:
                current_state = Tempo1;
                break;
            default:
                break;
        }
        */
    }

    void PlayerKilledCharacter()
    {
        Log( info, "PlayerKilledCharacter" );
        switch( current_state )
        {
            case Tempo1:
                current_state = Tempo3;
                if( aggression_count < 6 )
                    aggression_count = 6;
                break;
            case Tempo2:
                current_state = Tempo4;
                if( aggression_count < 8 )
                    aggression_count = 8;
                break;
            case Tempo3:
                current_state = Tempo5;
                if( aggression_count < 13 )
                    aggression_count = 13;
                break;
            case Tempo4:
                current_state = Tempo5;
                if( aggression_count < 13 )
                    aggression_count = 13;
                break;
        }
    }

    void AggressionShown()
    {
        aggression_count++;
        Log( info, "Aggro:" + aggression_count );
        aggression_reduction_counter.Reset();

        switch( current_state )
        {
            case Tempo1:
                if( aggression_count >= 3 )
                {
                    current_state = Tempo2;
                }
                break;
            case Tempo2:
                if( aggression_count >= 5 )
                {
                    current_state = Tempo3;
                }
                break;
            case Tempo3:
                if( aggression_count >= 7 )
                {
                    current_state = Tempo4;
                }
                break;
        }
    }

    void StartFight()
    {
        switch( current_state )
        {
            case PlayerDead:
                current_state = Tempo1;
                break;
            default:
        }
    }

    void MatchEnded()
    {
        aggression_count /= 2;
        switch( current_state )
        {
            case PlayerDead:
                //Nothing;
                break;
            default:
                current_state = PlayerDead;
                break;
        }
    }
};

MusicSegmentHandler msh;

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

uint64 global_time; // in ms

array<int> match_score;

string level_name;

void AddMetaEvent(MetaEventType type, string data) {
    int next_meta_event_end = (meta_event_end+1)%kMaxMetaEvents;
    if(next_meta_event_end == meta_event_start) {
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
}


// Called by level.cpp at start of level
void Init(string str) {
    PlaySong("sub-arena-loop");
    QueueSegment("fight1-ver1");
    
    level_name = str;
    
    global_data.ReadPersistentInfo();

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

    level.SendMessage("disable_retry");
}

// This script has no need of input focus
bool HasFocus() {
    return false;
}

// This script has no GUI elements
void DrawGUI() {
    // Do not draw the GUI in editor mode (it makes it hard to edit)
    if(GetPlayerCharacterID() == -1) {
        return;
    }

    float ui_scale = 0.5f;
    float visible = 1.0f;
    float display_time = time;

    {   
        HUDImage @image = hud.AddImage();
        image.SetImageFromPath(level.GetPath("diffuse_tex"));
        float stretch = GetScreenHeight() / image.GetHeight();
        image.position.x = GetScreenWidth() * 0.4f - 200;
        image.position.y = ((1.0-visible) * GetScreenHeight() * -1.2);
        image.position.z = 3;
        image.tex_scale.y = 20;
        image.tex_scale.x = 20;
        image.color = vec4(0.6f,0.8f,0.6f,text_visible*0.8f);
        image.scale = vec3(460 / image.GetWidth(), stretch, 1.0);
    }

    {   
        HUDImage @image = hud.AddImage();
        image.SetImageFromText(level.GetTextElement(main_text_id)); 
        image.position.x = int(GetScreenWidth() * 0.4f - 200 + 10);
        image.position.y = GetScreenHeight()-500;
        image.position.z = 4;
        image.color = vec4(1,1,1,text_visible);
    }

    {   
        HUDImage @image = hud.AddImage();
        image.SetImageFromText(level.GetTextElement(ingame_text_id)); 
        image.position.x = GetScreenWidth()/2-256;
        image.position.y = GetScreenHeight()-500;
        image.position.z = 4;
        image.color = vec4(1,1,1,1);
    }
}


void CharactersNoticeEachOther() {
    int num_chars = GetNumCharacters();
    for(int i=0; i<num_chars; ++i) {
         MovementObject@ char = ReadCharacter(i);
         for(int j=i+1; j<num_chars; ++j) {
             MovementObject@ char2 = ReadCharacter(j);
             Log(info, "Telling characters " + char.GetID() + " and " + char2.GetID() + " to notice each other.");
             char.ReceiveScriptMessage("notice " + char2.GetID());
             char2.ReceiveScriptMessage("notice " + char.GetID());
         }
     }
}

// Structures to hold the parameters for a battle
//  we need to make these global as running a script fragment doesn't have
//  access to local variables

array< array< dictionary >> battleTeams; // The characters, sorted into teams
array< dictionary > battleItems; // Items spawned for this battle 
string battleMessage; // Custom message for starting the battle

// Spawn all of the objects that we'll need in the level of given total difficulty
void SetUpLevel(float initial_difficulty) {

    battle.battleDifficulty = initial_difficulty;

    // Make sure things are clear
    battleMessage = "";
    battleTeams.resize(0);
    battleItems.resize(0);

    // Go through and find all the possible battle instances
    array<JSONValue> battleInstanceValues;
    array<int> @allObjectIds = GetObjectIDs();
    for( uint objectIndex = 0; objectIndex < allObjectIds.length(); objectIndex++ ) {
        Object @obj = ReadObjectFromID( allObjectIds[ objectIndex ] );
        ScriptParams@ params = obj.GetScriptParams();
        if(params.HasParam("Name") && params.GetString("Name") == "arena_battle" ) {
            if( params.HasParam("Battles") ) {
                JSON battlesJSON = params.GetJSON("Battles");
                
                string save_file = "Data/BattleJSONDumps/" + level_name + "-battle_data.json";
                Log(info, "Battle Setup Found, saving to: " + save_file);
                StartWriteFile();
                AddFileString( battlesJSON.writeString(true) );
                WriteFileToWriteDir( save_file );

                // Pull out the battle array
                JSONValue battlesValues = battlesJSON.getRoot()["data"]["battles"];

                Log(info, "Found " + battlesValues.size() + " battles");
                for( uint battleIndex = 0; battleIndex < battlesValues.size(); battleIndex++ ) {
                    battleInstanceValues.insertLast( battlesValues[ battleIndex ] );
                    
                    JSONValue battleName = battlesValues[ battleIndex ]["name"];

                    Log(info, "Adding battle: " + battleName.asString());
                }
                Log(info, "Done adding battles");

            }
        }
    }
    Log(info, "Done adding all battles");

    if( battleInstanceValues.length() == 0 ) {
        //DisplayError("Error", "No battle instances found in arena level" );
    }
    else {
        // By default, pick one at random
        int battleInstanceIndex = rand() % battleInstanceValues.length();

        //If we are in an active session, get the requested fight
        if( global_data.getSessionProfile() >= 0 )
        {
            JSONValue world_node = global_data.getCurrentWorldNode();
            if( world_node.type() != JSONnullValue )
            {
                if( world_node["type"].asString() == "arena_instance" )
                {
                    JSONValue arena_instance = global_data.getArenaInstance(world_node["target_id"].asString());

                    if(arena_instance.type() != JSONnullValue)
                    {
                        bool found_match = false;
                        for( uint i = 0; i < battleInstanceValues.length(); i++ )
                        {
                            if( arena_instance["battle"].asString() == battleInstanceValues[i]["name"].asString() )
                            {
                                battleInstanceIndex = i;
                                found_match = true;
                                break;
                            }
                        } 

                        if( !found_match )
                        {
                            Log( error, "Arena " + level_name + " has no battle with the name " + arena_instance["battle"].asString() + ". Requested from the arena_instance " + arena_instance["id"].asString());
                        }
                    }
                    else
                    {
                        Log( error, "There is no arena_instance with the name " + world_node["target_id"].asString() + ". As requested from the world_node: " + world_node["id"].asString()); 
                    }
                }
                else
                {
                    Log(error, "Running session, but we're not in an arena_instance node. We are in the world_node " + world_node["id"].asString());
                }
            }
            else
            {
                Log(error, "There is no valid world_node for session " + global_data.getSessionProfile() );
            }
        }
        else
        {
            Log(info, "Running level without active arena session, going into fallback mode");
        }
         
        JSONValue thisBattle = battleInstanceValues[ battleInstanceIndex ];

        JSONValue battleAttributes = thisBattle["attributes"];
        
        string gamemode = "normal";
        if( battleAttributes.isMember("gamemode") ) {
            gamemode = battleAttributes["gamemode"].asString();
        }

        battleMessage = battleAttributes["intro"].asString();
        battle.initialize( gamemode, thisBattle );

        battle.spawn();
    }

    // Reset level timer info
    target_time = 10.0f;
    time = 0.0f;
    // Reset audience info
    audience_excitement = 0.0f;
    total_excitement = 0.0f;
    // Audience size increases exponentially based on difficulty
    audience_size = int((rand()%1000+100)*pow(4.0f,initial_difficulty)*0.1f);
    if(audience_sound_handle == -1) {
        audience_sound_handle = PlaySoundLoop(level.GetPath("crowd_sound"),0.0f);
    }
    level_outcome = kUnknown;
    int num_scores = 4;
    match_score.resize(num_scores);
    for(int i=0; i<num_scores; ++i) {
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

//    AddMetaEvent(kDisplay, "Welcome to the arena!");
//    AddMetaEvent(kWait, "2.0");

    if( battle.weaponsInUse ) {
        AddMetaEvent(kDisplay, "This is a fight to the death!");
        AddMetaEvent(kMessage, "set_meta_state 1 one_point");
    } else {
        AddMetaEvent(kDisplay, "Two points to win!");
        AddMetaEvent(kMessage, "set_meta_state 1 two_points");
    }
    //AddMetaEvent(kWait, "2.0");

    if( battleMessage == "" ) {
        AddMetaEvent(kDisplay, "Time to fight!");    
    }
    else {
        AddMetaEvent(kDisplay, battleMessage);   
    }

    AddMetaEvent(kMessage, "set_all_hostile true");
    AddMetaEvent(kMessage, "set_meta_state 0 fighting");
    AddMetaEvent(kWait, "2.0");

    AddMetaEvent(kDisplay, "");
    //TimedSlowMotion(0.5f,7.0f, 0.0f);
}

enum MessageParseType {
    kSimple = 0,
    kOneInt = 1,
    kTwoInt = 2
}

void AggressionDetected(int attacker_id) {
    if(meta_states[0] != "fighting" && meta_states[0] != "fighting_over_but_allowed") {
        if(ReadCharacterID(attacker_id).controlled) {
            ClearMeta();
            AddMetaEvent(kMessage, "set_meta_state 0 disqualify");
            AddMetaEvent(kDisplay, "Player is disqualified!");
            AddMetaEvent(kWait, "2.0");
            EndMatch(false);
        }
    }

    //Player is being aggressive.
    if(ReadCharacterID(attacker_id).controlled) {
        msh.AggressionShown();
    }

}

// Parse string messages and react to them
void ReceiveMessage(string msg) {

    TokenIterator token_iter;
    token_iter.Init();
    if(!token_iter.FindNextToken(msg)) {
        return;
    }

    // Handle simple tokens, or mark as requiring extra parameters
    MessageParseType type = kSimple;
    string token = token_iter.GetToken(msg);


    if( token != "added_object" && token != "notify_deleted" )
    {
        //Log( info, "ArenaMessage: " + msg );
    }

    if(token == "post_reset") {
        SetUpLevel(curr_difficulty);    
    } else if(token == "restore_health") {
        int num_chars = GetNumCharacters();
        for(int i=0; i<num_chars; ++i) {
            MovementObject@ char = ReadCharacter(i);
            if(char.GetIntVar("zone_killed") == 0){
                char.ReceiveScriptMessage("restore_health");
            }
        }
    } else if(token == "new_match") {
        Log(info, "new_match received");
        msh.PlayerRevived();

        // Check if we're on a valid profile.
        if( global_data.getSessionProfile() >= 0 ) {
            // Go back to the arena menu
            LoadLevel("back");
        }
        else {
            // Proceed to a new battle in this arena
            curr_difficulty = GetRandomDifficultyNearPlayerSkill();
            SetUpLevel(curr_difficulty);    
        }
    } else if(token == "set_all_hostile") {
        token_iter.FindNextToken(msg);
        string param = token_iter.GetToken(msg);
        if(param == "false") {
            SetAllHostile(false);
        } else if(param == "true") {
            SetAllHostile(true);
        }
    } else if(token == "set") {
        token_iter.FindNextToken(msg);
        string param1 = token_iter.GetToken(msg);
        token_iter.FindNextToken(msg);
        string param2 = token_iter.GetToken(msg);
        if(param1 == "show_text") {
            if(param2 == "false") {
                show_text = false;
            } else if(param2 == "true") {
                show_text = true;
            }
        }
    } else if(token == "set_intro_text") {
        SetIntroText();
    } else if(token == "wait_for_player_move") {
        token_iter.FindNextToken(msg);
        string param1 = token_iter.GetToken(msg);
        wait_player_move_dist = atof(param1); 
        initial_player_pos_set = false;
    } else if(token == "wait_for_click") {
        wait_for_click = true;
    } else if(token == "set_meta_state") {
        token_iter.FindNextToken(msg);
        string param1 = token_iter.GetToken(msg);
        token_iter.FindNextToken(msg);
        string param2 = token_iter.GetToken(msg);
        meta_states[atoi(param1)] = param2;
    } else if(token == "dispose_level") {
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

    if(type == kOneInt) {
        token_iter.FindNextToken(msg);
        int char_a = atoi(token_iter.GetToken(msg));
        if(token == "character_died") {
            Log(info, "Player "+char_a+" was killed");
            audience_excitement += 4.0f;
        } else if(token == "character_knocked_out") {
            Log(info, "Player "+char_a+" was knocked out");
            audience_excitement += 3.0f;
        } else if(token == "character_start_flip") {
            Log(info, "Player "+char_a+" started a flip");
            audience_excitement += 0.4f;
        } else if(token == "character_start_roll") {
            Log(info, "Player "+char_a+" started a roll");
            audience_excitement += 0.4f;
        } else if(token == "character_failed_flip") {
            Log(info, "Player "+char_a+" failed a flip");
            audience_excitement += 1.0f;
        } else if(token == "item_hit") {
            Log(info, "Player "+char_a+" was hit by an item");
            audience_excitement += 1.5f;
        }

        if( token == "character_died" )
        {
            if( char_a != battle.playerObjectId )
            {
                Object@ player_obj = ReadObjectFromID(battle.playerObjectId);
                ScriptParams@ player_params = player_obj.GetScriptParams();

                Object@ other_obj = ReadObjectFromID(char_a);
                ScriptParams@ other_params = other_obj.GetScriptParams();
        

                if( player_params.GetString("Teams") != other_params.GetString("Teams") )
                {
                    msh.PlayerKilledCharacter();
                    Log(info,"Player got a kill\n");
                    global_data.player_kills++;
                }
            }
            else
            {
                msh.PlayerDied();
                Log(info,"Player got mortally wounded\n");
                global_data.player_deaths++;
                global_data.addHiddenState( "mortally_wounded" );
                //Instantly end match as the player lost due to bad damage.
                EndMatch(false);
            }
        }
        else if( token == "character_knocked_out" )
        {
            if( char_a != battle.playerObjectId )
            {
                Object@ player_obj = ReadObjectFromID(battle.playerObjectId);
                ScriptParams@ player_params = player_obj.GetScriptParams();

                Object@ other_obj = ReadObjectFromID(char_a);
                ScriptParams@ other_params = other_obj.GetScriptParams();
        
                if( player_params.GetString("Teams") != other_params.GetString("Teams") )
                {
                    msh.PlayerKilledCharacter();
                    Log(info,"Player got a ko\n");
                    global_data.player_kos++;
                }
            }
            else
            {
                msh.PlayerDied();
                global_data.addHiddenState( "knocked_out" );
            }
        }
    } else if(type == kTwoInt) {
        token_iter.FindNextToken(msg);
        int char_a = atoi(token_iter.GetToken(msg));
        token_iter.FindNextToken(msg);
        int char_b = atoi(token_iter.GetToken(msg));
        if(token == "knocked_over") {
            Log(info, "Player "+char_a+" was knocked over by player "+char_b);
            audience_excitement += 1.5f;
            AggressionDetected(char_b);
        } else if(token == "passive_blocked") {
            Log(info, "Player "+char_a+" passive-blocked an attack by player "+char_b);
            audience_excitement += 0.5f;
            AggressionDetected(char_b);
        } else if(token == "active_blocked") {
            Log(info, "Player "+char_a+" active-blocked an attack by player "+char_b);
            audience_excitement += 0.7f;
            AggressionDetected(char_b);
        } else if(token == "dodged") {
            Log(info, "Player "+char_a+" dodged an attack by player "+char_b);
            audience_excitement += 0.7f;
            AggressionDetected(char_b);
        } else if(token == "character_attack_feint") {
            Log(info, "Player "+char_a+" feinted an attack aimed at "+char_b);
            audience_excitement += 0.4f;
        } else if(token == "character_attack_missed") {
            Log(info, "Player "+char_a+" missed an attack aimed at "+char_b);
            audience_excitement += 0.4f;    
        } else if(token == "character_throw_escape") {
            Log(info, "Player "+char_a+" escaped a throw attempt by "+char_b);
            audience_excitement += 0.7f;        
        } else if(token == "character_thrown") {
            Log(info, "Player "+char_a+" was thrown by "+char_b);
            audience_excitement += 1.5f;
        } else if(token == "cut") {
            Log(info, "Player "+char_a+" was cut by "+char_b);
            audience_excitement += 2.0f;
            AggressionDetected(char_b);
        }
    }
}

float ProbabilityOfWin(float a, float b) {
    float a2 = a*a;
    float b2 = b*b;
    return a2 / (a2 + b2);
}

float GetRandomDifficultyNearPlayerSkill() {
    float var = global_data.player_skill * RangedRandomFloat(0.5f,1.5f);
    var = min(max(var, MIN_PLAYER_SKILL), MAX_PLAYER_SKILL);
    return var;
}

bool has_ended_match = false;
void EndMatch(bool victory) {
        msh.MatchEnded();
        float win_prob = ProbabilityOfWin(global_data.player_skill, curr_difficulty);
        float excitement_level = 1.0f-pow(0.9f,total_excitement*0.3f); 
        const float kMatchImportance = 0.3f; // How much this match influences your skill evaluation
        
        if(!victory) { // Decrease difficulty on failure
            level_outcome = kFailure;   
            float audience_fan_ratio = 0.0f;
            if(win_prob < 0.5f) { // If you were predicted to lose, you still gain some fans
                audience_fan_ratio += (0.5f - win_prob) * kMatchImportance;
            }
            audience_fan_ratio += (1.0f - audience_fan_ratio) * excitement_level * 0.4f;
            int new_fans = int(audience_size * audience_fan_ratio);
            global_data.fan_base += new_fans;
            global_data.player_skill -= global_data.player_skill * win_prob * kMatchImportance;
            global_data.player_skill = min(max(global_data.player_skill, MIN_PLAYER_SKILL), MAX_PLAYER_SKILL);
            global_data.player_loses++;
            SetLoseText(new_fans, excitement_level);
        } else if(victory) { // Increase difficulty on win
            level_outcome = kVictory;
            global_data.player_skill += curr_difficulty * (1.0f - win_prob) * kMatchImportance;        
            global_data.player_skill = min(max(global_data.player_skill, MIN_PLAYER_SKILL), MAX_PLAYER_SKILL);
            float audience_fan_ratio = (1.0f - win_prob) * kMatchImportance;
            audience_fan_ratio += (1.0f - audience_fan_ratio) * excitement_level;
            int new_fans = int(audience_size * audience_fan_ratio);
            global_data.fan_base += new_fans;
            global_data.player_wins++;
            SetWinText(new_fans, global_data.fan_base, excitement_level);
        }
        

    if( not has_ended_match )
    {
        if( global_data.getSessionProfile() >= 0 ) //Are we running on a valid profile?
        {
            global_data.done_with_current_node = true;
            global_data.arena_victory = victory;
            global_data.ResolveWorldNode();
            global_data.WritePersistentInfo();
        }
        else
        {
            Log( info, "There is no loaded profile, not calculating arena resulting state" );
        }

        has_ended_match = true;
    }
                   
    AddMetaEvent(kMessage, "set_all_hostile false");
    AddMetaEvent(kMessage, "set_meta_state 0 fighting_over_but_allowed");
    AddMetaEvent(kDisplay, "The match is over!");
    AddMetaEvent(kWait, "2.0");
    AddMetaEvent(kDisplay, "");
    AddMetaEvent(kMessage, "set show_text true");
    //AddMetaEvent(kMessage, "wait_for_click");
    AddMetaEvent(kMessage, "set show_text false");
    AddMetaEvent(kMessage, "new_match");

    Log( error, "Ending match" );
}

// Check if level should be reset
void VictoryCheck() {
    if(level_outcome == kUnknown) {
        bool multiple_teams_alive = false;
        bool any_team_alive = false;
        string team_alive;
                
        int num = GetNumCharacters();
        for(int i=0; i<num; ++i) {
            MovementObject@ char = ReadCharacter(i);
            if(char.GetIntVar("knocked_out") == _awake) {
                ScriptParams@ params = ReadObjectFromID(char.GetID()).GetScriptParams(); 
                if(!any_team_alive) {
                    any_team_alive = true;
                    team_alive = params.GetString("Teams");
                } else if(team_alive != params.GetString("Teams")) {
                    multiple_teams_alive = true;
                }
            }
        }

        if(!multiple_teams_alive && meta_states[0] == "fighting") {

            if( battle.gamemode == "waves" && battle.spawnNextWave() ) 
            {
        
            }
            else
            { 
                // Store id of the team that won the last round
                int last_round_winner = -1;
                if("0" == team_alive) {
                    last_round_winner = 0;
                } else if("1" == team_alive) {
                    last_round_winner = 1;
                } else if("2" == team_alive) {
                    last_round_winner = 2;
                } else if("3" == team_alive) {
                    last_round_winner = 3;
                }
                // Increment winning team score
                if(last_round_winner != -1) {
                    ++match_score[last_round_winner];
                }
                
                Log(info, "Last round winner: "+last_round_winner);
                int max_score = 1;
                if(meta_states[1] == "two_points") {
                    max_score = 2;
                }

                if(match_score[last_round_winner] < max_score) {
                    ClearMeta();
                    AddMetaEvent(kMessage, "set_all_hostile false");
                    AddMetaEvent(kMessage, "set_meta_state 0 fighting_over_but_allowed");
                    AddMetaEvent(kDisplay, "The round is over!");
                    AddMetaEvent(kWait, "2.0");
                    AddMetaEvent(kMessage, "set_meta_state 0 resetting");
                    AddMetaEvent(kMessage, "restore_health");
                    if(last_round_winner != -1) {
                        string team_color = "A";
                        switch(last_round_winner) {
                        case 0: team_color = "Red"; break;
                        case 1: team_color = "Blue"; break;
                        case 2: team_color = "Tan"; break;
                        case 3: team_color = "Cyan"; break;
                        }
                        string display_text = team_color+" team has "+match_score[last_round_winner]+" point";
                        if(match_score[last_round_winner] != 1) {
                            display_text += "s";
                        }
                        display_text += "!";
                        //AddMetaEvent(kDisplay, display_text);
                        //AddMetaEvent(kWait, "2.0");
                    }
                    //AddMetaEvent(kDisplay, "Get ready for the next round...");
                    //AddMetaEvent(kWait, "2.0");
                    AddMetaEvent(kDisplay, "Fight!");
                    AddMetaEvent(kMessage, "set_all_hostile true");
                    AddMetaEvent(kMessage, "set_meta_state 0 fighting");
                    AddMetaEvent(kWait, "2.0");
                    AddMetaEvent(kDisplay, "");
                } else {
                    bool victory = false;
                    int num_chars = GetNumCharacters();
                    for(int i=0; i<num_chars; ++i) {
                        MovementObject@ char = ReadCharacter(i);
                        Object@ obj = ReadObjectFromID(char.GetID());
                        ScriptParams@ params = obj.GetScriptParams();                    
                        if(char.controlled && params.HasParam("Teams") && params.GetString("Teams") == team_alive) {
                            victory = true;
                        }
                    }
                    ClearMeta();
                    EndMatch(victory);
                }
            }
        }        
    }

    if(show_text) {
        text_visible += time_step;
        text_visible = min(1.0f, text_visible);
    } else {
        text_visible -= time_step;
        text_visible = max(0.0f, text_visible);
    }
}

void OddsFromProbability(float prob, int &out a, int &out b) {
    // Escape if 0 or 1, to avoid divide by zero
    if(prob == 0.0f) {
        a = 0;
        b = 1;
        return;
    }
    if(prob == 1.0f) {
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
    for(int i=1; i<10; ++i) {
        for(int j=1; j<10; ++j) {
            float val = i/float(j);
            float dist = abs(target_ratio - val);
            if(closest_numerator == -1 || dist < closest_dist) {
                closest_dist = dist;
                closest_numerator = i;
                closest_denominator = j;
            }
        }
    }
    if(closest_numerator == 1 && closest_denominator == 9) {
        closest_denominator = int(1.0f/target_ratio);
    }
    if(closest_numerator == 9 && closest_denominator == 1) {
        closest_numerator = int(target_ratio);
    }
    a = closest_numerator;
    b = closest_denominator;
}

void SetAllHostile(bool val) {
    int num_chars = GetNumCharacters();
    if(val == true) {
        msh.StartFight();
        for(int i=0; i<num_chars; ++i) {
             MovementObject@ char = ReadCharacter(i);
             char.ReceiveScriptMessage("set_combat_allowed true");
         }
         CharactersNoticeEachOther();
    } else {
        for(int i=0; i<num_chars; ++i) {
            MovementObject@ char = ReadCharacter(i);
            char.ReceiveScriptMessage("set_combat_allowed false");   
        }
    }
}

void SendMessageToAllCharacters(const string &in msg) {
    int num_chars = GetNumCharacters();
    for(int i=0; i<num_chars; ++i) {
        MovementObject@ char = ReadCharacter(i);
        char.ReceiveScriptMessage(msg);   
    }
}

// Returns true if a wait event was encountered
void ProcessMetaEvent(MetaEvent me) {
    switch(me.type) {
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

    if( wait_player_move_dist > 0.0f ) {
        if(battle.playerObjectId != -1) {

            MovementObject@ player_char = ReadCharacterID(battle.playerObjectId);
            if(!initial_player_pos_set) {

                initial_player_pos = player_char.position;
                initial_player_pos_set = true;
            }
            if(xz_distance_squared(initial_player_pos, player_char.position) > wait_player_move_dist) {
                wait_player_move_dist = 0.0f;
            }
        }
    }

    if(wait_for_click) {
        if(GetInputDown(0, "attack")) {
            wait_for_click = false;
        }
    }
    
}

bool MetaEventWaiting() {
    bool waiting = false;
    if(global_time < meta_event_wait) {
        waiting = true;
    }
    if(wait_player_move_dist > 0.0f) {
        waiting = true;
    }
    if(wait_for_click) {
        waiting = true;
    }
    return waiting;
}

void Update() { 
    msh.Update();

    global_time += uint64(time_step * 1000);

    //SetPlaceholderPreviews();
    if(DebugKeysEnabled() && GetInputPressed(0, "t")) {
        curr_difficulty = GetRandomDifficultyNearPlayerSkill();
        SetUpLevel(curr_difficulty);
    }

    VictoryCheck();

    time += time_step;
    // Get total amount of character movement
    float total_char_speed = 0.0f;
    int num = GetNumCharacters();
    for(int i=0; i<num; ++i) {
        MovementObject@ char = ReadCharacter(i);
        if(char.GetIntVar("knocked_out") == _awake) {
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
    if(crowd_cheer_vel > 0.0f) {
        crowd_cheer_vel *= 0.99f;
    } else {
        crowd_cheer_vel *= 0.95f;
    }
    crowd_cheer_amount += crowd_cheer_vel * time_step;
    crowd_cheer_amount = max(crowd_cheer_amount, 0.1f);
    SetSoundGain(audience_sound_handle, crowd_cheer_amount*2.0f);
    SetSoundPitch(audience_sound_handle, min(0.8f + crowd_cheer_amount * 0.5f,1.2f));
    
    bool debug_text = false;
    if(debug_text) {
        DebugText("a","Difficulty: "+curr_difficulty*2.0f,0.5f);
        DebugText("ab","Player Skill: "+global_data.player_skill*2.0f,0.5f);
        float prob = ProbabilityOfWin(global_data.player_skill, curr_difficulty);
        DebugText("ac","Probability of player win: "+ProbabilityOfWin(global_data.player_skill, curr_difficulty),0.5f);
        int a, b;
        OddsFromProbability(1.0f-prob, a, b);
        DebugText("ad","Odds against win: "+a+":"+b,0.5f);
        DebugText("b","Target time: "+target_time,0.5f);
        DebugText("c","Current time: "+time,0.5f);
        DebugText("d","Excitement: "+audience_excitement,0.5f);
        DebugText("g","Total excitement: "+total_excitement,0.5f);
        DebugText("h","Audience size:  "+audience_size,0.5f);
        DebugText("ha","Fans:  "+global_data.fan_base,0.5f);
        DebugText("hb","target_crowd_cheer_amount:  "+target_crowd_cheer_amount,0.5f);
        DebugText("i","crowd_cheer_amount:  "+crowd_cheer_amount,0.5f);
        DebugText("j","crowd_cheer_vel:  "+crowd_cheer_vel,0.5f);
    }

    UpdateMetaEventWait();
    while(meta_event_start != meta_event_end && !MetaEventWaiting()) {
        ProcessMetaEvent(meta_events[meta_event_start]);
        meta_event_start = (meta_event_start+1)%kMaxMetaEvents;
    }
}
