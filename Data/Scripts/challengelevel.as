//-----------------------------------------------------------------------------
//           Name: challengelevel.as
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

#include "ui_effects.as"
#include "ui_tools.as"
#include "threatcheck.as"
#include "music_load.as"
#include "arena_meta_persistence.as"

bool reset_allowed = true;
float time = 0.0f;
float no_win_time = 0.0f;
string level_name;
int in_victory_trigger = 0;
const float _reset_delay = 4.0f;
float reset_timer = _reset_delay;

AHGUI::FontSetup titleFont("edosz", 120,HexColor("#fff"));
AHGUI::FontSetup subTitleFont("edosz", 65, HexColor("#fff"));
AHGUI::FontSetup greenValueFont("edosz", 65, HexColor("#0f0"));
AHGUI::FontSetup redValueFont("edosz", 65, HexColor("#f00"));
AHGUI::FontSetup tealValueFont("edosz", 65, HexColor("#028482"));

MusicLoad ml("Data/Music/challengelevel.xml");

int menu_item_spacing = 20;

AHGUI::MouseOverPulseColor buttonHover(
                                        HexColor("#ffde00"),
                                        HexColor("#ffe956"), .25 );

void Init(string p_level_name) {
    level_name = p_level_name;
}

SavedLevel@ GetSave() {
    SavedLevel @saved_level;
    if(save_file.GetLoadedVersion() == 1 && save_file.SaveExist("","",level_name)) {
        @saved_level = save_file.GetSavedLevel(level_name);
        saved_level.SetKey(GetCurrentLevelModsourceID(),"challenge_level",level_name);
    } else {
        @saved_level = save_file.GetSave(GetCurrentLevelModsourceID(),"challenge_level",level_name);
    }
    return saved_level; 
}

class Achievements {
    bool flawless_;
    bool no_first_strikes_;
    bool no_counter_strikes_;
    bool no_kills_;
    bool no_alert_;
    bool injured_;
    float total_block_damage_;
    float total_damage_;
    float total_blood_loss_;
    void Init() {
        flawless_ = true;
        no_first_strikes_ = true;
        no_counter_strikes_ = true;
        no_kills_ = true;
        no_alert_ = true;
        injured_ = false;
        total_block_damage_ = 0.0f;
        total_damage_ = 0.0f;
        total_blood_loss_ = 0.0f;
    }
    Achievements() {
        Init();
    }
    void UpdateDebugText() {
        DebugText("achmt0", "Flawless: "+flawless_, 0.5f);
        DebugText("achmt1", "No Injuries: "+!injured_, 0.5f);
        DebugText("achmt2", "No First Strikes: "+no_first_strikes_, 0.5f);
        DebugText("achmt3", "No Counter Strikes: "+no_counter_strikes_, 0.5f);
        DebugText("achmt4", "No Kills: "+no_kills_, 0.5f);
        DebugText("achmt5", "No Alerts: "+no_alert_, 0.5f);
        DebugText("achmt6", "Time: "+no_win_time, 0.5f);
        //DebugText("achmt_damage0", "Block damage: "+total_block_damage_, 0.5f);
        //DebugText("achmt_damage1", "Impact damage: "+total_damage_, 0.5f);
        //DebugText("achmt_damage2", "Blood loss: "+total_blood_loss_, 0.5f);

        SavedLevel @level = GetSave();
        DebugText("saved_achmt0", "Saved Flawless: "+(level.GetValue("flawless")=="true"), 0.5f);
        DebugText("saved_achmt1", "Saved No Injuries: "+(level.GetValue("no_injuries")=="true"), 0.5f);
        DebugText("saved_achmt2", "Saved No Kills: "+(level.GetValue("no_kills")=="true"), 0.5f);
        DebugText("saved_achmt3", "Saved No Alert: "+(level.GetValue("no_alert")=="true"), 0.5f);
        DebugText("saved_achmt4", "Saved Time: "+level.GetValue("time"), 0.5f);
    }
    void Save() {
        SavedLevel @saved_level = GetSave();
        if(flawless_) saved_level.SetValue("flawless","true");
        if(!injured_) saved_level.SetValue("no_injuries","true");
        if(no_kills_) saved_level.SetValue("no_kills","true");
        if(no_alert_) saved_level.SetValue("no_alert","true");
        string time_str = saved_level.GetValue("time");
        if(time_str == "" || no_win_time < atof(saved_level.GetValue("time"))){
            saved_level.SetValue("time", ""+no_win_time);
        }
        save_file.WriteInPlace();
    }
    void PlayerWasHit() {
        flawless_ = false;
    }
    void PlayerWasInjured() {
        injured_ = true;
        flawless_ = false;
    }
    void PlayerAttacked() {
        no_first_strikes_ = false;
    }
    void PlayerSneakAttacked() {
        no_first_strikes_ = false;
    }
    void PlayerCounterAttacked() {
        no_counter_strikes_ = false;
    }
    void EnemyDied() {
        no_kills_ = false;
    }
    void EnemyAlerted() {
        no_alert_ = false;
    }
    void PlayerBlockDamage(float val) {
        total_block_damage_ += val;
        PlayerWasHit();
    }
    void PlayerDamage(float val) {
        total_damage_ += val;
        PlayerWasInjured();
    }
    void PlayerBloodLoss(float val) {
        total_blood_loss_ += val;
        PlayerWasInjured();
    }
    bool GetValue(const string &in key){
        if(key == "flawless"){
            return flawless_;
        } else if(key == "no_kills"){
            return no_kills_;
        } else if(key == "no_injuries"){
            return !injured_;
        }
        return false; 
    }
};

Achievements achievements;

bool HasFocus(){
    return (challenge_end_gui.target_visible == 1.0f);
}

void Reset(){
    time = 0.0f;
    reset_allowed = true;
    reset_timer = _reset_delay;
    achievements.Init();
    challenge_end_gui.target_visible = 0.0;
}

void ReceiveMessage(string msg) {
    TokenIterator token_iter;
    token_iter.Init();
    if(!token_iter.FindNextToken(msg)){
        return;
    }
    string token = token_iter.GetToken(msg);
    if(token == "reset"){
        Reset();
    } else if(token == "achievement_event"){
        token_iter.FindNextToken(msg);
        AchievementEvent(token_iter.GetToken(msg));
    } else if(token == "achievement_event_float"){
        token_iter.FindNextToken(msg);
        string str = token_iter.GetToken(msg);
        token_iter.FindNextToken(msg);
        float val = atof(token_iter.GetToken(msg));
        AchievementEventFloat(str, val);
    } else if(token == "victory_trigger_enter"){
        ++in_victory_trigger;
        in_victory_trigger = max(1,in_victory_trigger);
    } else if(token == "victory_trigger_exit"){
        --in_victory_trigger;
    }
}

void DrawGUI() {
    challenge_end_gui.DrawGUI();
}

void AchievementEvent(string event_str){
    if(event_str == "player_was_hit"){
        achievements.PlayerWasHit();
    } else if(event_str == "player_was_injured"){
        achievements.PlayerWasInjured();
    } else if(event_str == "player_attacked"){
        achievements.PlayerAttacked();
    } else if(event_str == "player_sneak_attacked"){
        achievements.PlayerSneakAttacked();
    } else if(event_str == "player_counter_attacked"){
        achievements.PlayerCounterAttacked();
    } else if(event_str == "enemy_died"){
        achievements.EnemyDied();
    } else if(event_str == "enemy_alerted"){
        achievements.EnemyAlerted();
    }
}

void AchievementEventFloat(string event_str, float val){
    if(event_str == "player_block_damage"){
        achievements.PlayerBlockDamage(val);
    } else if(event_str == "player_damage"){
        achievements.PlayerDamage(val);
    } else if(event_str == "player_blood_loss"){
        achievements.PlayerBloodLoss(val);
    }
}

string StringFromFloatTime(float time){
    string time_str;
    int minutes = int(time) / 60;
    int seconds = int(time)-minutes*60;
    time_str += minutes + ":";
    if(seconds < 10){
        time_str += "0";
    }
    time_str += seconds;
    return time_str;
}

class ChallengeEndGUI : AHGUI::GUI{
    float visible;
    float target_visible;
    RibbonBackground ribbon_background;

    ChallengeEndGUI() {
        restrict16x9(false);

        super();

        ribbon_background.Init();

        Init();
    }

    ~ChallengeEndGUI() {
        
    }

    void RegenerateGUI()  {


    }

    void Init() {
        visible = 0.0;
        target_visible = 0.0;

        //setGuides(true);
    }

    void Update(){
        visible = UpdateVisible(visible, target_visible);

        if( visible > 0.1 ) {
            if( isVisible == false) {
                CreateGUI();
            }
            setVisible(true); 
        } else {
            setVisible(false);
        }

        AHGUI::GUI::update();

        ribbon_background.Update();
        UpdateMusic();
    }

    void CreateGUI() {
        array<string> mission_objectives;
        array<string> mission_objective_colors;
        bool success = true;

        for(int i=0; i<level.GetNumObjectives(); ++i){
            string objective = level.GetObjective(i);
            string mission_objective;
            string mission_objective_color;
            if(objective == "destroy_all"){
                int threats_possible = ThreatsPossible();
                int threats_remaining = ThreatsRemaining();
                if(threats_possible <= 0){
                    mission_objective = "  Defeat all enemies (N/A)";
                    mission_objective_color = "red";
                } else {
                    if(threats_remaining == 0){
                        mission_objective += "v ";
                        mission_objective_color = "green";
                    } else {
                        mission_objective += "x ";
                        mission_objective_color = "red";
                        success = false;
                    }
                    mission_objective += "defeat all enemies (" ;
                    mission_objective += (threats_possible - threats_remaining);
                    mission_objective += "/" ;
                    mission_objective += threats_possible;
                    mission_objective += ")";
                }
            }
            if(objective == "reach_a_trigger"){
                if(in_victory_trigger > 0){
                    mission_objective += "v ";
                    mission_objective_color = "green";
                } else {
                    mission_objective += "x ";
                    mission_objective_color = "red";
                    success = false;
                }
                mission_objective += "Reach the goal";
            }
            if(objective == "must_visit_trigger"){
                if(NumUnvisitedMustVisitTriggers() == 0){
                    mission_objective += "v ";
                    mission_objective_color = "green";
                } else {
                    mission_objective += "x ";
                    mission_objective_color = "red";
                    success = false;
                }
                mission_objective += "Visit all checkpoints";
            }
            if(objective == "reach_a_trigger_with_no_pursuers"){
                if(in_victory_trigger > 0 && NumActivelyHostileThreats() == 0){
                    mission_objective += "v ";
                    mission_objective_color = "green";
                } else {
                    mission_objective += "x ";
                    mission_objective_color = "red";
                    success = false;
                }
                mission_objective += "Reach the goal without any pursuers";
            }

            if(objective == "collect"){
                if(NumUnsatisfiedCollectableTargets() != 0){
                    success = false;
                    mission_objective += "x ";
                    mission_objective_color = "red";
                }  else {
                    mission_objective += "v ";
                    mission_objective_color = "green";
                }
                mission_objective += "Collect items";
            }

            mission_objectives.insertLast(mission_objective);
            mission_objective_colors.insertLast(mission_objective_color);
        }

        string title = success ? 'challenge complete' : 'challenge incomplete';

        root.clear(); 

        AHGUI::Divider@ horiz_root = root.addDivider( DDTop,
                                                    DOHorizontal,
                                                    ivec2( UNDEFINEDSIZEI, 2000 ) );
        
        horiz_root.addSpacer(200);

        AHGUI::Divider@ mainpane = horiz_root.addDivider( DDLeft,
                                                    DOVertical,
                                                    ivec2( UNDEFINEDSIZEI, 2000 ) );


        mainpane.addSpacer(300,DDTop);

        AHGUI::Text titleText = AHGUI::Text(title, titleFont);

        AHGUI::Image titleUnderline = AHGUI::Image("Textures/ui/challenge_mode/divider_strip_c.tga");
        titleUnderline.scaleToSizeY(40);

        mainpane.addElement(titleText, DDTop);
        mainpane.addElement(titleUnderline, DDTop);

        AHGUI::Text titleObjectives = AHGUI::Text("Objectives", subTitleFont);

        mainpane.addElement( titleObjectives, DDTop ); 

        for( uint i = 0; i < mission_objectives.size(); i++ ) {
            AHGUI::FontSetup fs;

            if(mission_objective_colors[i] == "red") {
                fs = redValueFont;
            } else {
                fs = greenValueFont;
            }

            AHGUI::Text objective = AHGUI::Text("  " + mission_objectives[i], fs);
            mainpane.addElement(objective,DDTop);
        }


        AHGUI::Text titleTime = AHGUI::Text("Time", subTitleFont);

        mainpane.addElement( titleTime, DDTop ); 

        AHGUI::FontSetup timefont;
        if(success){
            timefont = greenValueFont;
        } else {
            timefont = redValueFont;
        }

        AHGUI::Text time1Text = AHGUI::Text("  " + StringFromFloatTime(no_win_time),timefont);
        mainpane.addElement( time1Text, DDTop ); 
       
        SavedLevel @saved_level = GetSave();
        float best_time = atof(saved_level.GetValue("time"));
        if(best_time > 0.0f){
            AHGUI::Text time2Text = AHGUI::Text("  " + StringFromFloatTime(no_win_time),tealValueFont);
            mainpane.addElement( time2Text, DDTop );
        }

        int player_id = GetPlayerCharacterID();
        if(player_id != -1){
            for(int i=0; i<level.GetNumObjectives(); ++i){
                string objective = level.GetObjective(i);
                if(objective == "destroy_all"){
                    AHGUI::Text titleEnemies = AHGUI::Text("Enemies", subTitleFont);

                    mainpane.addElement( titleEnemies, DDTop ); 

                    AHGUI::Divider@ kills = mainpane.addDivider( DDTop, 
                                                                 DOHorizontal, 
                                                                 ivec2( UNDEFINEDSIZEI, 100 ) );
                    MovementObject@ player_char = ReadCharacter(player_id);
                    int num = GetNumCharacters();
                    for(int j=0; j<num; ++j){
                        MovementObject@ char = ReadCharacter(j);
                        if(!player_char.OnSameTeam(char)){
                            int knocked_out = char.GetIntVar("knocked_out");
                            if(knocked_out == 1 && char.GetFloatVar("blood_health") <= 0.0f){
                                knocked_out = 2;
                            }
                            AHGUI::Image img;
                            switch(knocked_out){
                            case 0:    
                                img = AHGUI::Image("Textures/ui/challenge_mode/ok.png");
                                break;
                            case 1:    
                                img = AHGUI::Image("Textures/ui/challenge_mode/ko.png");
                                break;
                            case 2:    
                                img = AHGUI::Image("Textures/ui/challenge_mode/dead.png");
                                break;
                            }
                            img.scaleToSizeY(70);
                            kills.addElement(img,DDLeft);
                        }
                    }
                }
            }
        }

        AHGUI::Text titleExtra = AHGUI::Text("Extra", subTitleFont);

        mainpane.addElement( titleExtra, DDTop ); 

        int num_achievements = level.GetNumAchievements();
        for(int i=0; i<num_achievements; ++i){
            string achievement = level.GetAchievement(i);
            string display_str;

            AHGUI::FontSetup extraFont = redValueFont;

            if(saved_level.GetValue(achievement) == "true"){
                extraFont = tealValueFont;
            }
            if(achievements.GetValue(achievement)){
                extraFont = greenValueFont;
            }

            if(achievement == "flawless"){
                display_str += "flawless";
            } else if(achievement == "no_kills"){
                display_str += "no kills";
            } else if(achievement == "no_injuries"){
                display_str = "never hurt";
            } else if(achievement == "no_alert"){
                display_str = "never seen";
            }
            AHGUI::Text extra_val = AHGUI::Text(display_str, extraFont);
            mainpane.addElement( extra_val, DDTop );
        }

        AHGUI::Divider@ buttonpane = mainpane.addDivider( DDTop,
                                                      DOHorizontal,
                                                      ivec2(1000, 400) );

        AHGUI::Image quitButton = AHGUI::Image("Textures/ui/challenge_mode/quit_icon_c.tga");
        quitButton.scaleToSizeX(250);
        quitButton.addMouseOverBehavior( buttonHover );
        quitButton.addLeftMouseClickBehavior(AHGUI::FixedMessageOnClick("quit"));
        buttonpane.addElement(quitButton,DDLeft);

        AHGUI::Image retryButton = AHGUI::Image("Textures/ui/challenge_mode/retry_icon_c.tga");
        retryButton.scaleToSizeX(250);
        retryButton.addMouseOverBehavior( buttonHover );
        retryButton.addLeftMouseClickBehavior(AHGUI::FixedMessageOnClick("retry"));
        buttonpane.addElement(retryButton,DDLeft);

        if( success ) {
            AHGUI::Image continueButton = AHGUI::Image("Textures/ui/challenge_mode/continue_icon_c.tga");
            continueButton.scaleToSizeX(250); 
            continueButton.addMouseOverBehavior( buttonHover );
            continueButton.addLeftMouseClickBehavior(AHGUI::FixedMessageOnClick("continue"));
            buttonpane.addElement(continueButton,DDLeft);
        }
    }

    ~ChallengeEndGUI() {
    }

    void DrawGUI(){
        float ui_scale = 0.5f;

        ribbon_background.DrawGUI(visible);

        hud.Draw();

        AHGUI::GUI::render(); 
    }

    void processMessage( AHGUI::Message@ message ) {
        if( message.name == "quit" ) {
            level.SendMessage("go_to_main_menu");
        } else if( message.name == "retry" ) {
            level.SendMessage("reset"); 
        } else if( message.name == "continue" ) {
            target_visible = 0.0f;
            SendGlobalMessage("levelwin");
        }
    }
}

ChallengeEndGUI challenge_end_gui;


void Update() {
    bool display_achievements = false;
    if(display_achievements && GetPlayerCharacterID() != -1){
        achievements.UpdateDebugText();
    }
    challenge_end_gui.Update();
    time += time_step;
    VictoryCheckNormal();
}

int NumUnvisitedMustVisitTriggers() {
    int num_hotspots = GetNumHotspots();
    int return_val = 0;
    for(int i=0; i<num_hotspots; ++i){
        Hotspot@ hotspot = ReadHotspot(i);
        if(hotspot.GetTypeString() == "must_visit_trigger"){
            if(!hotspot.GetBoolVar("visited")){
                ++return_val;
            }
        }
    }
    return return_val;
}

int NumUnsatisfiedCollectableTargets() {
    int num_hotspots = GetNumHotspots();
    int return_val = 0;
    for(int i=0; i<num_hotspots; ++i){
        Hotspot@ hotspot = ReadHotspot(i);
        if(hotspot.GetTypeString() == "collectable_target"){
            if(!hotspot.GetBoolVar("condition_satisfied")){
                ++return_val;
            }
        }
    }
    return return_val;
}

void VictoryCheckNormal() {
    int player_id = GetPlayerCharacterID();
    if(player_id == -1){
        return;
    }
    bool victory = true;
    bool display_victory_conditions = false;

    float max_reset_delay = _reset_delay;
    for(int i=0; i<level.GetNumObjectives(); ++i){
        string objective = level.GetObjective(i);
        if(objective == "destroy_all"){
            int threats_remaining = ThreatsRemaining();
            int threats_possible = ThreatsPossible();
            if(threats_remaining > 0 || threats_possible == 0){
                victory = false;
                if(display_victory_conditions){
                    DebugText("victory_a","Did not yet defeat all enemies",0.5f);
                }
            }
        }
        if(objective == "reach_a_trigger"){
            max_reset_delay = 1.0;
            if(in_victory_trigger <= 0){
                victory = false;
                if(display_victory_conditions){
                    DebugText("victory_b","Did not yet reach trigger",0.5f);
                }
            }
        }
        if(objective == "reach_a_trigger_with_no_pursuers"){
            max_reset_delay = 1.0;
            if(in_victory_trigger <= 0){
                victory = false;
                if(display_victory_conditions){
                    DebugText("victory_c","Did not yet reach trigger",0.5f);
                }
            } else if(NumActivelyHostileThreats() > 0){
                victory = false;
                if(display_victory_conditions){
                    DebugText("victory_c","Reached trigger, but still pursued",0.5f);
                }
            } 
        }
        if(objective == "must_visit_trigger"){
            max_reset_delay = 1.0;
            if(NumUnvisitedMustVisitTriggers() != 0){
                victory = false;
                if(display_victory_conditions){
                    DebugText("victory_d","Did not visit all must-visit triggers",0.5f);
                }
            } 
        }
        if(objective == "collect"){
            max_reset_delay = 1.0;
            if(NumUnsatisfiedCollectableTargets() != 0){
                victory = false;
                if(display_victory_conditions){
                    DebugText("victory_d","Did not visit all must-visit triggers",0.5f);
                }
            } 
        }
    }
    reset_timer = min(max_reset_delay, reset_timer);

    bool failure = false;
    MovementObject@ player_char = ReadCharacter(player_id);
    if(player_char.GetIntVar("knocked_out") != _awake){
        failure = true;
    }
    if(reset_timer > 0.0f && (victory || failure)){
        reset_timer -= time_step;
        if(reset_timer <= 0.0f){
            if(reset_allowed){
                challenge_end_gui.target_visible = 1.0;
                reset_allowed = false;
            }
            if(victory){
                achievements.Save();
            }
        }
    } else {
        reset_timer = _reset_delay;
        no_win_time = time;
    }
}

void UpdateMusic() {
    int player_id = GetPlayerCharacterID();
    if(player_id != -1 && ReadCharacter(player_id).GetIntVar("knocked_out") != _awake){
        PlaySong("challengelevel_sad");
        return;
    }
    int threats_remaining = ThreatsRemaining();
    if(threats_remaining == 0){
        PlaySong("challengelevel_ambient-happy");
        return;
    }
    if(player_id != -1 && ReadCharacter(player_id).QueryIntFunction("int CombatSong()") == 1){
        PlaySong("challengelevel_combat");
        return;
    }
    PlaySong("challengelevel_ambient-tense");
}
