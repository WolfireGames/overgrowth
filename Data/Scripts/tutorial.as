//-----------------------------------------------------------------------------
//           Name: tutorial.as
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

#include "ui_effects.as"
#include "threatcheck.as"
#include "ui_tools.as"
#include "tutorial_assignment_checks.as"
#include "music_load.as"

bool resetAllowed = true;
float time = 0.0f;
float noWinTime = 0.0f;
string levelName;
int inVictoryTrigger = 0;
const float ResetDelay = 4.0f;
float resetTimer = ResetDelay;
int currentAssignment = 0;
int lastAssignment = -1;
bool showBorders = false;
int assignmentTextSize = 70;
int footerTextSize = 50;
int enemyID = -1;
bool enemyAttacking = false;
bool enemyHighlighted = false;
float highlightTimer = 0.0f;
float highlightDuration = 0.5f;
bool highlightEnemy = false;

string enemyRabbitPath = "Data/Objects/IGF_Characters/IGF_GuardActor.xml";
string enemyWolfPath = "Data/Objects/IGF_Characters/IGF_WolfActor.xml";

array<int> playerKnifeID = {-1,-1,-1};
int knifeIDCounter = 0;

int enemyKnifeID = -1;
string knifePath = "Data/Items/rabbit_weapons/rabbit_knife.xml";
string swordPath = "Data/Items/DogWeapons/DogSword.xml";

string enemyKnifePath = "Data/Items/rabbit_weapons/rabbit_knife.xml";
int screen_height = 1500;
int screen_width = 2560;
vec4 backgroundColor = vec4(0.0f,0.0f,0.0f,0.5f);
bool inCombat = false;

float spawn_knife_countdown = 0.0;

MusicLoad ml("Data/Music/challengelevel.xml");

class Assignment{
    string text;
	string extraText = "";
    string origText = "";
    AssignmentCallback@ callback;
    Assignment(string _text, AssignmentCallback _callback){
        text = _text;
        @callback = @_callback;
    }
}

class KnockoutCountdown {
    int character_id;
    float countdown;
}

array<KnockoutCountdown> knockout_countdown;

array<Assignment@> assignments =
{Assignment("Welcome to the tutorial! Complete the tasks, or skip them with tab.", Delay(5.0f)),

Assignment("Move the mouse to rotate the camera.", MouseMove()),
Assignment("Use @up@, @left@, @down@ and @right@ to move around.", WASDMove()), //TODO get the input names dynamically so that it works with controllers and keyboard
Assignment("Press @jump@ to jump. Holding the button will result in a longer jump.", SpaceJump()),
Assignment("Hold @crouch@ to crouch and move quietly.", ShiftSneak()),
Assignment("Press @crouch@ while moving to roll. Rolling is useful for putting out fires or quickly picking up weapons.", ShiftRoll()),
Assignment("Press @crouch@ in the air to flip. Try different directions, but be careful how you land!", ShiftFlip()),
Assignment("Jump into a wall to wall-run, then press @jump@ or @crouch@ to jump or flip off of it.", WallJump()),
Assignment("Hold the Right Mouse Button while close to a ledge to grab it, then move towards it to climb up.", LedgeGrab()),

Assignment("There is now an enemy in the middle of the training area.", SendInEnemy("rabbit")),
Assignment("Hold Left Mouse Button near the enemy to attack. Your attack changes based on your distance, which direction you're moving, and whether you're crouching.", AnyAttack()),
Assignment("Perform a rabbit kick by attacking while in the air close to the enemy. This is your most powerful attack, but also the most dangerous and difficult.", LegCannon()),
Assignment("Hold Right Mouse Button and sneak behind the enemy unnoticed to choke them.", ChokeHold()),

Assignment("The enemy can now block your attacks. Vary your attacks to break through his guard!", ActivateEnemy()),
Assignment("When your attack is blocked or dodged, you are vulnerable to throws. Attack the enemy and press Right Mouse Button at the right moment to escape from his throw.", ThrowEscape()),

Assignment("Click the Right Mouse Button to block incoming attacks. You can even block while lying on the ground!", BlockAttack()),
Assignment("When knocked over, you can press @crouch@ to quickly roll to your feet. Don't let enemies hit you while you're down.", RollFromGround()),
Assignment("If you keep holding Right Mouse Button after a block, you can attempt a throw.", ReverseAttack()),
Assignment("Defeat the enemy five times, however you like!", AttackCountDown()),
//Assignment("WEAPONS:", Delay(3.0f)),
Assignment("There's now a knife in the center of the training area. Hold @drop@ near the knife to pick it up. This also works while flipping or rolling!", PickUpKnife()),
Assignment("Sheathe and unsheathe weapons with the @item@ key.", SheatheKnife()),
Assignment("Cut the enemy by holding Left Mouse Click while in cutting range.", KillWolfSharp()),
Assignment("Grab the enemy from behind by pressing Right Mouse Click and press Left Mouse Click or @drop@ to perform a quick and silent execution.", KnifeGrabExecution()),
Assignment("When an enemy is nearby, throw the knife with @drop@.", KnifeThrow()),
Assignment("The enemy now has your knife! Without a weapon, you can't block sharp attacks. Try dodging by standing still, and then suddenly moving backwards or to the side.", Dodge()),
Assignment("After dodging, you can throw the enemy and take their weapon.", ReverseAttackArmed()),

Assignment("There is now a Wolf in the training area.", SendInEnemy("wolf")),
Assignment("Wolves are incredibly strong. Punching one is like punching a tree. The only unarmed rabbit attack they'll even notice is the jump kick.", LegCannon()),
Assignment("Pick up the sword on the pedestal.", PickUpSword()),
Assignment("Wolves are strong, but not impenetrable. Use the sword to kill the Wolf.", KillWolfSharp()),
Assignment("Try throwing the sword at the wolf using @drop@.", KnifeThrow()),
Assignment("You can increase the damage of thrown weapons by twisting them out again. Try holding @drop@ while flipping past the sword stuck in the wolf.", PullSword()),
Assignment("You are now ready to fight in a real battle! Press @quit@ and click 'main menu' to exit the tutorial, or 'retry' to start over.", EndLevel())};

string bottomText = "Press '@slow@' to skip to the next item. Press @quit@ to open the menu.";

string InsertKeysToString( string text )
{
    for( uint i = 0; i < text.length(); i++ ) {
        if( text[i] == '@'[0] ) {
            for( uint j = i + 1; j < text.length(); j++ ) {
                if( text[j] == '@'[0] ) {
                    string first_half = text.substr(0,i);
                    string second_half = text.substr(j+1);
                    string input = text.substr(i+1,j-i-1);
                    string middle = GetStringDescriptionForBinding("key", input);

                    text = first_half + middle + second_half;
                    i += middle.length();
                    break;    
                }
            }
        }
    }
    return text;
}

void Init(string _levelName) {
    bottomText = InsertKeysToString(bottomText);

    ActivateKeyboardEvents();
    levelName = _levelName;
    //lugaruGUI.AddFooter();
    lugaruGUI.AddInstruction();
}

class LugaruGUI : AHGUI::GUI {
    LugaruGUI() {
        // Call the superclass to set things up
        super();
    }
    void Render() {

        // Update the background
        // TODO: fold this into AHGUI
        hud.Draw();

        // Update the GUI
        AHGUI::GUI::render();
     }

     void processMessage( AHGUI::Message@ message ) {

        // Check to see if an exit has been requested
        if( message.name == "mainmenu" ) {
            //this_ui.SendCallback("back");
        }
    }

     void AddFooter() {
		AHGUI::Divider@ footerBackground = root.addDivider( DDBottomRight,  DOVertical, ivec2( AH_UNDEFINEDSIZE, 300 ) );
		footerBackground.setName("footerbackground");
		//Dark background
		AHGUI::Image background( "Textures/diffuse.tga" );
		background.setName("footerbackgroundimage");
		background.setColor(vec4(0.0,0.0,0.0,0.3));
		background.setSizeX(800);
		background.setSizeY(40 * 3);
		footerBackground.addFloatingElement(background, "footerbackgroundimage", ivec2(int(screen_width / 2.0f) - background.getSizeX() / 2, 0.0f), 0);

        AHGUI::Divider@ footer = footerBackground.addDivider( DDBottomRight,  DOVertical, ivec2( AH_UNDEFINEDSIZE, 300 ) );
        footer.setName("footer");
        footer.setVeritcalAlignment(BACenter);
        DisplayText(DDTop, footer, 8, bottomText, footerTextSize, vec4(0,0,0,1));
        if(showBorders){
            footer.setBorderSize( 10 );
            footer.setBorderColor( 0.0, 0.0, 1.0, 0.6 );
            footer.showBorder();
        }
    }
    void UpdateFooter(){
        AHGUI::Element@ footerElement = root.findElement("footer");
        if( footerElement is null  ) {
            DisplayError("GUI Error", "Unable to find footer");
        }
        AHGUI::Divider@ footer = cast<AHGUI::Divider>(footerElement);

        // Get rid of the old contents
        footer.clear();
        footer.clearUpdateBehaviors();
        footer.setDisplacement();
        DisplayText(DDTop, footer, 8, bottomText, footerTextSize, vec4(1,1,1,1));
    }
    void AddInstruction(){
		AHGUI::Divider@ container = root.addDivider( DDTop,  DOVertical, ivec2( AH_UNDEFINEDSIZE, 400 ) );
		container.setVeritcalAlignment(BACenter);
		AHGUI::Image background( "Textures/diffuse.tga" );
		background.setName("headerbackgroundimage");
		background.setColor(vec4(0.0,0.0,0.0,0.3));
		int sizeX = 1500;
		int sizeY = assignmentTextSize * 5;
		background.setSizeX(sizeX);
		background.setSizeY(sizeY);
		container.addFloatingElement(background, "headerbackground", ivec2(int(screen_width / 2.0f) - background.getSizeX() / 2, int(container.getSizeY() / 2.0f) - (sizeY / 2)), 0);
        AHGUI::Divider@ header = container.addDivider( DDCenter,  DOVertical, ivec2( AH_UNDEFINEDSIZE, AH_UNDEFINEDSIZE ) );
        header.setName("header");
        header.setVeritcalAlignment(BACenter);
        DisplayText(DDTop, header, 8, bottomText, assignmentTextSize, vec4(0,0,0,1));

        if(showBorders){
            header.setBorderSize( 10 );
            header.setBorderColor( 1.0, 0.0, 0.0, 0.6 );
            header.showBorder();

			container.setBorderSize( 10 );
            container.setBorderColor( 0.0, 1.0, 0.0, 0.6 );
            container.showBorder();
        }
    }
    void UpdateInstruction(bool fadein = false){
        AHGUI::Element@ headerElement = root.findElement("header");
        if( headerElement is null  ) {
            DisplayError("GUI Error", "Unable to find header");
        }
        AHGUI::Divider@ header = cast<AHGUI::Divider>(headerElement);
        // Get rid of the old contents
        header.clear();
        header.clearUpdateBehaviors();
        header.setDisplacement();
        if(lastAssignment != -1 && uint(lastAssignment) < assignments.size()){
            DisplayText(DDTop, header, 8, assignments[lastAssignment].text, assignmentTextSize, vec4(1,1,1,1), assignments[lastAssignment].extraText, footerTextSize);
        }
    }

    void HandleAssignmentChange(){
        if(currentAssignment == lastAssignment || uint(currentAssignment) == assignments.size()){
            return;
        }
        if(lastAssignment != -1 && uint(lastAssignment) < assignments.size()) {
			assignments[lastAssignment].callback.OnCompleted();
		}
        //Print("INITIALIZED THE NEXT ASSIGNMENT!----------\n");
        lastAssignment = currentAssignment;
        //UpdateFooter();
        UpdateInstruction();
        if(lastAssignment != -1 && uint(lastAssignment) < assignments.size()) {
            assignments[lastAssignment].callback.Init();
            if(assignments[lastAssignment].origText == ""){
                assignments[lastAssignment].origText = assignments[lastAssignment].text;
            }
        }
    }
    void Update(){
        if( spawn_knife_countdown > 0.0f ) {
            spawn_knife_countdown -= time_step;
            if( (spawn_knife_countdown > 0.0f) == false ) {
                SendInWeapon("");
            }
        }
        HandleAssignmentChange();
        CheckCurrentAssignment();
        AHGUI::GUI::update();
        ReviveCharacters();
        HandleEnemyHighlight();
    }

    void HandleEnemyHighlight(){
        if( enemyID != -1 ) {
            if(highlightEnemy){
                MovementObject@ enemy = ReadCharacterID(enemyID);
                if(enemyAttacking){
                    if(!enemyHighlighted){
                        //Print("Setting color to red\n");
                        Object@ obj = ReadObjectFromID(enemyID);
                        for(int i=0; i<4; ++i){
                            obj.SetPaletteColor(i, vec3(1,0,0));
                        }
                        enemyHighlighted = true;
                        highlightTimer = 0.0f;
                    }
                }
            }
            if( enemyHighlighted ) {
                if(highlightTimer > highlightDuration ) {
                    //Print("Setting color to white\n");
                    enemyAttacking = false;
                    enemyHighlighted = false;
                    Object@ obj = ReadObjectFromID(enemyID);
                    for(int i=0; i<4; ++i) {
                        obj.SetPaletteColor(i, vec3(1));
                    }
                } else {
                    highlightTimer += time_step;
                }
            }
        }
    }

    void DisplayText(DividerDirection dd, AHGUI::Divider@ div, int maxWords, string text, int textSize, vec4 color, string extraText = "", int extraTextSize = 0) {
        //The maxWords is the amount of words per line.
        array<string> sentences;

        text = InsertKeysToString( text );

        array<string> words = text.split(" ");
        string sentence;
        for(uint i = 0; i < words.size(); i++){
            sentence += words[i] + " ";
            if((i+1) % maxWords == 0 || words.size() == (i+1)){
                sentences.insertLast(sentence);
                sentence = "";
            }
        }
        for(uint k = 0; k < sentences.size(); k++){
            AHGUI::Text singleSentence( sentences[k], "OpenSans-Regular", textSize, color.x, color.y, color.z, color.a );
			singleSentence.setShadowed(true);
            //singleSentence.addUpdateBehavior( AHGUI::FadeIn( 1000, @inSine ) );
            div.addElement(singleSentence, dd);
            if(showBorders){
                singleSentence.setBorderSize(1);
                singleSentence.setBorderColor(1.0, 1.0, 1.0, 1.0);
                singleSentence.showBorder();
            }
        }
		if(extraText != ""){
			AHGUI::Text extraSentence( extraText, "OpenSans-Regular", extraTextSize, color.x, color.y, color.z, color.a );
			extraSentence.setShadowed(true);
			div.addElement(extraSentence, dd);
		}
	}
    void CheckCurrentAssignment() {
		/*else if(GetInputPressed(0, "esc")){
			level.SendMessage("dispose_level");
	        LoadLevel("back");
		}*/
        //Print("Got returned " + newBool + "\n");
        if(lastAssignment != -1 && uint(lastAssignment) < assignments.size()){
            if(assignments[lastAssignment].callback.CheckCompleted() ){
                currentAssignment++;
            }
        }
    }
}


void Reset(){
    time = 0.0f;
    resetAllowed = true;
    resetTimer = ResetDelay;
}

void PostReset(){
    for(uint i = 0; i < assignments.size(); i++){
        assignments[i].callback.Reset();
    }
    currentAssignment = 0;
    lastAssignment = -1;
}

int GetPlayerID() {
    int id = -1;
    int num = GetNumCharacters();
    for(int i=0; i<num; ++i){
        MovementObject@ char = ReadCharacter(i);
        if(char.controlled){
            id = char.GetID();
			break;
        }
    }
    return id;
}

void ReceiveMessage(string msg) {
    TokenIterator token_iter;
    token_iter.Init();

    if(!token_iter.FindNextToken(msg)){
        return;
    }
    string token = token_iter.GetToken(msg);
    //Log(info,"Token: " + token);
    //Log(info,"Full: " + msg);
    if( token == "hotspot_announce_items" ) {
        //If one of the player knifes leaves the knife area, respawn a new knife
        token_iter.FindNextToken(msg); 
        string source_id = token_iter.GetToken(msg);

        if( source_id == "player_knife_monitor" ) {
            token_iter.FindNextToken(msg); 
            string type = token_iter.GetToken(msg);

            if( type == "inside_list" ) {
                bool found_knife_on_zone = false;
                bool have_active_weapon = false;
                while( token_iter.FindNextToken(msg) ) {
                    string item_id = token_iter.GetToken(msg);

                    for( uint i = 0; i < playerKnifeID.length(); i++ ) {
                        if( playerKnifeID[i] != -1 ) {
                            have_active_weapon = true;
                        }
                        if( ("" + playerKnifeID[i]) == item_id ) {
                            found_knife_on_zone = true;
                        }
                    }
                }

                if( found_knife_on_zone == false && have_active_weapon == true ) {
                    spawn_knife_countdown = 5.0f;
                }
            }
        }
    } else if(token == "reset") {
        Reset();
    } else if(token == "achievement_event") {
        token_iter.FindNextToken(msg);
        string achievement = token_iter.GetToken(msg);
        Log(info,"achievement: " + achievement);
        if( lastAssignment >= 0 && lastAssignment < int(assignments.size()) ) {
            assignments[lastAssignment].callback.ReceiveAchievementEvent(achievement);
        }
        if(achievement == "ai_attacked"){
            if(!enemyAttacking){
                enemyAttacking = true;
            }
        }
    } else if(token == "achievement_event_float"){
        token_iter.FindNextToken(msg);
        string achievement = token_iter.GetToken(msg);
        Log(info, "achievement: " + achievement);
        token_iter.FindNextToken(msg);
        string value = token_iter.GetToken(msg);
        Log(info, "value: " + value);
        if( lastAssignment >= 0 && lastAssignment < int(assignments.size()) ) {
            assignments[lastAssignment].callback.ReceiveAchievementEventFloat(achievement, atof(value));
        }
    } else if(token == "send_in_enemy") {
        if( token_iter.FindNextToken(msg) ) {
            SendInEnemyChar( token_iter.GetToken(msg) );
        }
    } else if(token == "give_enemy_knife" ) {
        GiveEnemyKnife();
    } else if(token == "delete_enemy") {
        if( enemyKnifeID != -1 ) {
            DeleteObjectID(enemyKnifeID);
        }
        enemyKnifeID = -1;
        if( enemyID != -1 ) {
            DeleteObjectID(enemyID);
        }
        enemyID = -1;
    } else if(token == "send_in_weapon") {
        if( token_iter.FindNextToken(msg) ) {
            SendInWeapon(token_iter.GetToken(msg));
        }
    } else if(token == "delete_weapon") {
        for( uint i = 0; i < playerKnifeID.length(); i++ )
        {
            if( playerKnifeID[i] != -1 )
                DeleteObjectID(playerKnifeID[i]);
            playerKnifeID[i] = -1;
        }
    } else if(token == "set_highlight") {
		token_iter.FindNextToken(msg);
        string command = token_iter.GetToken(msg);
		if(command == "true") {
			highlightEnemy = true;
		} else if(command == "false") {
			highlightEnemy = false;
		}
	} else if(token == "post_reset") {
        PostReset();
    } else if(token == "character_knocked_out" || token == "character_died" || token == "cut_throat") {
		token_iter.FindNextToken(msg);
        string character_id = token_iter.GetToken(msg);
        
        KnockoutCountdown kc;
        kc.character_id = parseInt(character_id);
        kc.countdown = 2.0f;

        knockout_countdown.insertLast(kc);
        Log( info, "Added player " + character_id + " to queue for reawekening" );
	} else if(token == "revive_all") {
		for(int i = 0; i < GetNumCharacters(); i++){
            MovementObject@ char = ReadCharacter(i);
            char.Execute("Recover();");
        }
    } else if(token == "level_execute"){
        token_iter.FindNextToken(msg);
        string command = token_iter.GetToken(msg);
        level.Execute(command);
    } else if(token == "player_execute"){
        token_iter.FindNextToken(msg);
        string command = token_iter.GetToken(msg);
		int playerID = GetPlayerID();
		if(playerID != -1){
	        MovementObject@ player = ReadCharacterID(playerID);
	        player.Execute(command);
		}
    } else if(token == "enemy_execute"){
        token_iter.FindNextToken(msg);
        string command = token_iter.GetToken(msg);
		if(enemyID != -1){
            MovementObject@ enemy = ReadCharacterID(enemyID);
            //Print("Command: " + command + "\n");
            enemy.Execute(command);
        }
    }else if(token == "update_text_variables"){
        array<string> values;
        string lastVariable = "";
        while(true){
            bool nextToken = token_iter.FindNextToken(msg);
            if(nextToken){
                string new_value = token_iter.GetToken(msg);
                lastVariable = new_value;
                values.insertLast(new_value);
            }else{
                break;
            }
        }
        UpdateTextVariables(values);
	}else if(token == "extra_assignment_text"){
		string completeSentence;
		while(true){
            bool nextToken = token_iter.FindNextToken(msg);
            if(nextToken){
                completeSentence += token_iter.GetToken(msg);
				completeSentence += " ";
            }else{
                break;
            }
        }
		AddExtraAssignmentText(InsertKeysToString(completeSentence));
    }else if(token == "set_combat"){
		token_iter.FindNextToken(msg);
        string command = token_iter.GetToken(msg);
		if(command == "true"){
			inCombat = true;
		}else if(command == "false"){
			inCombat = false;
		}
	} else if(token == "player_invincible") {
        int player_id = GetPlayerID();
        if( player_id != -1 ) {
            Log(info, "PLAYER CAN'T DIE NOW" );
            MovementObject@ player = ReadCharacterID(player_id);
            player.Execute("invincible = true;"); 
        }
    }
}

LugaruGUI lugaruGUI;

bool HasFocus(){
    return false;
}

void DrawGUI() {
    lugaruGUI.Render();
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

void Update() {
    time += time_step;
    lugaruGUI.Update();
    SetPlaceholderPreviews();
	UpdateMusic();
}

void UpdateTextVariables(array<string> new_variables){
    string currentText = "";
    if( lastAssignment >= 0 && lastAssignment < int(assignments.size()) ) {
        currentText = assignments[lastAssignment].origText;
    }
    //Don't do anything if the string is empty. This is because it's not been assigned yet.
    if(currentText == ""){
        return;
    }
    for(uint i = 0; i<new_variables.size(); i++){
        currentText = join(currentText.split("%variable" + i + "%"), new_variables[i]);
    }
    if(lastAssignment != -1 && uint(lastAssignment) < assignments.size()) {
        assignments[lastAssignment].text = currentText;
    }
    //Print("New text " + currentText + "\n");
    lugaruGUI.UpdateInstruction();
}

void AddExtraAssignmentText(string extra_text){
    if(lastAssignment != -1 && uint(lastAssignment) < assignments.size()) {
        assignments[lastAssignment].extraText = extra_text;
    }
    lugaruGUI.UpdateInstruction();
}

void Initialize(){

}

void SendInEnemyChar(string type){
    string enemyPath;

    if( type == "wolf" ) {
        enemyPath = enemyWolfPath;
    } else {
        enemyPath = enemyRabbitPath;
    }

    if( enemyKnifeID != -1 ) {
        DeleteObjectID(enemyKnifeID);
    }
    enemyKnifeID = -1;

    if(enemyID != -1) {
        DeleteObjectID(enemyID);
        enemyID = -1;
    }
    //Don't spawn more than one enemy.
    if(enemyID == -1){
        enemyID = CreateObject(enemyPath);
        Object@ charObj = ReadObjectFromID(enemyID);
        //At first the enemy can't fight back and cannot die
        MovementObject@ enemy = ReadCharacterID(enemyID);
        enemy.Execute("SetHostile(false);
                       combat_allowed = false;
                       allow_active_block = false;");
        //Find the enemy spawn placeholder and put the new enemy at that point.
        array<int> @object_ids = GetObjectIDs();
        int num_objects = object_ids.length();
        for(int i=0; i<num_objects; ++i){
            Object @obj = ReadObjectFromID(object_ids[i]);
            ScriptParams@ params = obj.GetScriptParams();
            if(params.HasParam("Name")){
                string name_str = params.GetString("Name");
                if("enemy_spawn" == name_str){
                    charObj.SetTranslation(obj.GetTranslation());
                    break;
                }
            }
        }
		array<int> nav_points = GetObjectIDsType(33);
		if(nav_points.size() > 0){
			Object@ navObj = ReadObjectFromID(nav_points[0]);
			navObj.ConnectTo(charObj);
		}

    } else {
        MovementObject@ enemy = ReadCharacterID(enemyID);
        enemy.Execute("combat_allowed = false; 
                       allow_active_block = false;");
    }

    if(enemyID != -1){
        MovementObject@ enemy = ReadCharacterID(enemyID);
        if( type == "wolf" ) {
            enemy.Execute("max_ko_shield = 3; ko_shield = max_ko_shield;");
        } else {
            enemy.Execute("p_block_skill = 0.5;");
        }
    }
}

void GiveEnemyKnife() {
    Log(info, "Giving enemy knife");
    
    if( enemyID != -1 ) {
        if(enemyKnifeID != -1 ) {
            DeleteObjectID(enemyKnifeID);
        }
        enemyKnifeID = CreateObject(enemyKnifePath);
        //Object@ knifeObj = ReadObjectFromID(enemyKnifeID);
        MovementObject@ enemy = ReadCharacterID(enemyID);
        enemy.Execute("AttachWeapon(" + enemyKnifeID + ");");
    } else {
        Log(error, "Unable to give enemy knife, no enemy");
    }
}

string last_type = "knife";
void SendInWeapon( string type ) {
    string weapon;

    if( type == "" ) {
        type = last_type;
    }
    last_type = type;   

    if( type == "sword" ) {
        weapon = swordPath;
    } else {
        weapon = knifePath;
    }
      
    Log( info, "Spawning new player knife" );

    int player_id = GetPlayerID();
    knifeIDCounter = (knifeIDCounter + 1) % playerKnifeID.length();

    int knifeID =  playerKnifeID[knifeIDCounter];
    if(knifeID != -1 ) {
        DeleteObjectID(knifeID);
        knifeID = -1;
    }
    if(knifeID == -1){
        knifeID = CreateObject(weapon);
        Object@ knifeObj = ReadObjectFromID(knifeID);
        //Find the knife spawn placeholder and put the new knife at that point.
        array<int> @object_ids = GetObjectIDs();
        int num_objects = object_ids.length();
        for(int i=0; i<num_objects; ++i){
            Object @obj = ReadObjectFromID(object_ids[i]);
            ScriptParams@ params = obj.GetScriptParams();
            if(params.HasParam("Name")){
                string name_str = params.GetString("Name");
                if("weapon_spawn" == name_str){
                    knifeObj.SetTranslation(obj.GetTranslation());
                    knifeObj.SetRotation(obj.GetRotation());
                    break;
                }
            }
        }
    }
    playerKnifeID[knifeIDCounter] = knifeID;
}

bool HavePlayerKnifeInWorld() {
    for( uint i = 0; i < playerKnifeID.length(); i++ )
    {
        if( playerKnifeID[i] != -1 )
            return true;
    }
    return false;
}

int GetCharPrimaryWeapon(MovementObject@ mo) {
    return mo.GetArrayIntVar("weapon_slots",mo.GetIntVar("primary_weapon_slot"));
}

int GetCharPrimarySheathedWeapon(MovementObject@ mo) {
    return mo.GetArrayIntVar("weapon_slots", 3);
}

void ReviveCharacters() {
    for( uint i = 0; i < knockout_countdown.length(); i++ ) {
        if( knockout_countdown[i].countdown < 0.0f ) {
            if( knockout_countdown[i].character_id != -1 ){
                MovementObject@ char = ReadCharacterID(knockout_countdown[i].character_id);
                if( char !is null ) {
                    if(char.GetIntVar("knocked_out") != _awake){
                        char.Execute("Recover();");
                        if(!char.controlled) {
                            int playerID = GetPlayerID();
                            if(playerID != -1) {
                                char.Execute("situation.Notice(" + playerID + ");");
                            }
                        }
                    }
                }
            }
            knockout_countdown.removeAt(i);
            i -= 1;
        } else {
            knockout_countdown[i].countdown -= time_step;
        }
    }
}

void SomeFunction(){
    Log(info, "works");
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
            if("enemy_spawn" == name_str){
                SetSpawnPointPreview(obj, "Data/Objects/IGF_Characters/IGF_Guard.xml");
            }else if("weapon_spawn" == name_str){
                SetSpawnPointPreview(obj, "Data/Objects/Weapons/rabbit_weapons/rabbit_knife.xml");
            }else if("bush_spawn" == name_str){
                SetSpawnPointPreview(obj, "Data/Objects/Plants/Trees/temperate/green_bush.xml");
            }else if("pillar_spawn" == name_str){
                SetSpawnPointPreview(obj, "Data/Objects/Buildings/pillar1.xml");
            }
        }
    }
}

void UpdateMusic() {
	if(inCombat){
		PlaySong("challengelevel_combat");
        return;
	}else{
		PlaySong("challengelevel_ambient-happy");
        return;
	}
}

void KeyPressed( string command, bool repeated ) {
	Log(info,"Pressed " + command);
    if( command == "n" && repeated == false ) {
        if(lastAssignment != -1 && uint(lastAssignment) < assignments.size()){
            currentAssignment++;
        }
    }
}

void KeyReleased( string command ) {
}
