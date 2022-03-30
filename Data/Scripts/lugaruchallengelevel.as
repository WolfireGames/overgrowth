//-----------------------------------------------------------------------------
//           Name: lugaruchallengelevel.as
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
#include "arena_meta_persistence.as"
#include "ui_tools.as"

enum ChallengeGUIState {
    agsFighting,
    agsEndScreen,
	agsInvalidState
};

class ChallengeGUI : AHGUI::GUI {

    // fancy ribbon background stuff
    float visible = 0.0f;
    float target_visible = 1.0f;

    ChallengeGUIState currentState = agsFighting; // Token for our state machine
    ChallengeGUIState lastState = agsInvalidState;    // Last seen state, to detect changes

    // Selection screen
    bool dataOutdated = false;
    JSON profileData;
    bool dataLoaded = false;
    int profileId = -1;         // Which profile are we working with
	string difficulty = "";
    string user_name;
	int score = 0;
    bool showBorders = true;
    int textSize = 60;
    int challengeTextSize = 120;
    vec4 textColor = vec4(0.8, 0.8, 0.8, 1.0);
    int minimapTextSize = 30;
    int minimapIconSize = 30;
    int levels_finished = -1;
    bool inputEnabled = false;
    array<string> inputName;
    bool bloodEffectEnabled = false;
    float bloodAlpha = 1.0;
    float bloodDisplayTime = 0.0f;
    float inputTime = 0.0f;
	JSONValue challengeData;
	int challengeLevelsFinished = -1;
	string levelName;
	float timerTime = 0;
	float challengeDuration = 0;
	bool timerStarted = false;
	int points = 0;
	int pointsToAdd = 0;
	int activeLevelIndex = 0;

    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *
     */
    ChallengeGUI() {
        // Call the superclass to set things up
        super();

    }

	void handleStateChange() {
		//see if anything has changed
		if( lastState == currentState ) {
			return;
		}
		// Record the change
		lastState = currentState;
		// First clear the old screen
		clear();
		switch( currentState ) {
			case agsInvalidState: {
				// For completeness -- throw an error and move on
				DisplayError("GUI Error", "GUI in invalid state");
			}
			break;
			case agsFighting: {
				ShowFightingUI();
			}
			break;
			case agsEndScreen: {
				ShowEndScreenUI();
				updateHighscore();
				WritePersistentInfo(true);
			}
			break;
		}
	}
	void ShowFightingUI(){
        AHGUI::Divider@ mainPane = root.addDivider( DDTop, DOHorizontal, ivec2( AH_UNDEFINEDSIZE, AH_UNDEFINEDSIZE ) );
		mainPane.addSpacer( 50, DDLeft );
		DisplayText(mainPane, DDTop, "Score: " + points, challengeTextSize, textColor, true, "ScoreText");
        if(showBorders){
            mainPane.setBorderSize( 1 );
            mainPane.setBorderColor( 0.0, 0.0, 1.0, 0.6 );
            mainPane.showBorder();
        }
    }
	void ShowEndScreenUI(){
        AHGUI::Divider@ mainPane = root.addDivider( DDTop, DOVertical, ivec2( 2562, 1440 ) );
		mainPane.setHorizontalAlignment(BALeft);
		mainPane.setBackgroundImage("Textures/LugaruMenu/LugaruChallengeBackground.png");
		AHGUI::Divider@ title = mainPane.addDivider( DDTop, DOHorizontal, ivec2( AH_UNDEFINEDSIZE, 300 ) );
		DisplayText(title, DDCenter, "Level Cleared!", textSize, textColor, true);

		AHGUI::Divider@ scorePane = mainPane.addDivider( DDTop, DOVertical, ivec2( AH_UNDEFINEDSIZE, 300 ) );
		DisplayText(scorePane, DDTop, "Score:          " + (points + pointsToAdd), textSize, textColor, true);
		DisplayText(scorePane, DDTop, "Time:           " + GetTime(int(challengeDuration)), textSize, textColor, true);
		DisplayText(scorePane, DDTop, "Merciful", textSize, textColor, true);
		DisplayText(scorePane, DDTop, "Divide and Conquer", textSize, textColor, true);

		AHGUI::Divider@ footer = mainPane.addDivider( DDBottom, DOHorizontal, ivec2( AH_UNDEFINEDSIZE, 300 ) );
		DisplayText(footer, DDCenter, "Press escape to return to menu or space to continue", textSize, textColor, true);

        if(showBorders){
            mainPane.setBorderSize( 1 );
            mainPane.setBorderColor( 0.0, 0.0, 1.0, 0.6 );
            mainPane.showBorder();

			title.setBorderSize( 1 );
            title.setBorderColor( 1.0, 0.0, 1.0, 0.6 );
            title.showBorder();

			scorePane.setBorderSize( 1 );
            scorePane.setBorderColor( 1.0, 1.0, 1.0, 0.6 );
            scorePane.showBorder();

			footer.setBorderSize( 1 );
            footer.setBorderColor( 1.0, 0.0, 0.0, 0.6 );
            footer.showBorder();
        }
    }
	void updateHighscore(){
		for(uint i = 0; i < challengeData.size(); i++){
			if(challengeData[i]["levelname"].asString() == levelName){
				activeLevelIndex = i;
				int newScore = points + pointsToAdd;
				Log(info,"highscore " + challengeData[i]["highscore"].asInt() + " points " + newScore + "!");
				if(challengeData[i]["highscore"].asInt() < newScore){
					Log(info,"New Highscore!");
					challengeData[i]["highscore"] = JSONValue(newScore);
					challengeData[i]["besttime"] = JSONValue(int(challengeDuration));
					if(challengeLevelsFinished <= activeLevelIndex){
						challengeLevelsFinished++;
					}
				}else{
					Log(info,"Not a new Highscore :(");
				}
				break;
			}
		}
	}
	string GetTime(int seconds){
		string bestTime;
		int numSeconds = seconds % 60;
		int numMinutes = seconds / 60;

		if(numSeconds < 10){
			bestTime = numMinutes + ":0" + numSeconds;
		}else{
			bestTime = numMinutes + ":" + numSeconds;
		}
		return bestTime;
	}
	void processMessage( AHGUI::Message@ message ) {
        // Check to see if an exit has been requested
        if( message.name == "mainmenu" ) {
            //WritePersistentInfo( false );
            //this_ui.SendCallback("back");
        }
	}
	void update() {
        // Do state machine stuff
        handleStateChange();
        // Update the GUI
        AHGUI::GUI::update();
    }

	void render() {
	   hud.Draw();
	   // Update the GUI
	   AHGUI::GUI::render();
	}

    SavedLevel@ GetSave() {
        return save_file.GetSave("","linear_campaign_progress","lugaru_challanges");
    }

	void ReadPersistentInfo() {
        UpgradeSave();
        SavedLevel@ saved_level = GetSave();
        // First we determine if we have a session -- if not we're not going to read data
        JSONValue sessionParams = getSessionParameters();
        if( !sessionParams.isMember("started") ) {
            // Nothing is started, so we shouldn't actually read the data
            Log(info,"could not find started :(");
            return;
        }
        // read in campaign_started
        string profiles_str = saved_level.GetValue("lugaru_profiles");

        //Print("Profiles string : " + profiles_str + "\n");


        if( profiles_str != "" ) {
            // Parse the JSON
            if( !profileData.parseString( profiles_str ) ) {
                DisplayError("Persistence Error", "Unable to parse profile information");
            }

            // Now check the version
            if( profileData.getRoot()[ "version" ].asInt() == SAVEVERSION ) {
                dataOutdated = false;
            }
            else {
                // We have a save version from a previous incarnation
                // For now we'll just nuke it and restart
                dataOutdated = true;
                //profileData = generateNewProfileSet();
            }
        }
        dataLoaded = true;
        // Now see if we have a profile in this session -- if so load it
        JSONValue profiles = profileData.getRoot()["profiles"];
		Log(info,profileData.writeString(true));
		for( uint i = 0; i < profiles.size(); ++i ) {
			if(profiles[i]["active"].asString() == "true"){
				setDataFrom(profiles[i]["id"].asInt());
				break;
			}
		}
    }
	JSONValue getSessionParameters() {

        SavedLevel @saved_level = GetSave();

        string lugaru_session_str = saved_level.GetValue("lugaru_session");

        // sanity check
        if( lugaru_session_str == "" ) {
            lugaru_session_str = "{}";

            // write it back
            saved_level.SetValue("lugaru_session", lugaru_session_str );
            Log(info,"Write back the lugaru session");
        }

        JSON sessionJSON;

        // sanity check
        if( !sessionJSON.parseString( lugaru_session_str ) ) {
            DisplayError("Persistence Error", "Unable to parse session information");

        }
        return sessionJSON.getRoot();
    }
	void setDataFrom( int targetId ) {

		JSONValue profiles = profileData.getRoot()["profiles"];

		bool profileFound = false;

		for( uint i = 0; i < profiles.size(); ++i ) {
			if( profiles[ i ]["id"].asInt() == targetId ) {
				profileFound = true;
				// Copy all the values back
				profileId = targetId;
				levels_finished = profiles[ i ]["levels_finished"].asInt();
				user_name = profiles[ i ]["user_name"].asString();
				difficulty = profiles[ i ]["difficulty"].asString();
				challengeData = profiles[ i ]["challengeData"];
				challengeLevelsFinished = profiles[ i ]["challenge_levels_finished"].asInt();
				// We're done here
				break;
			}
		}
		// Sanity checking
		if( !profileFound ) {
			DisplayError("Persistence Error", "Profile id " + targetId + " not found in store.");
		}
	}
	void setSessionParameters( JSONValue session ) {
        SavedLevel @saved_level = GetSave();

        // set the value to the stringified JSON
        JSON sessionJSON;
        sessionJSON.getRoot() = session;
        string arena_session_str = sessionJSON.writeString(false);
        saved_level.SetValue("lugaru_session", arena_session_str );

        // write out the changes
        Log(info,"Writing session");
        save_file.WriteInPlace();

    }
    void WritePersistentInfo( bool moveDataToStore = true ) {

        // Make sure we've got information to write -- this is not an error
        if( !dataLoaded ) return;

        // Make sure our current data has been written back to the JSON structure
        if( moveDataToStore ) {
            Log(info,"Writing data to profile");
            writeDataToProfiles(); // This'll do nothing if we haven't set a profile
        }

        SavedLevel @saved_level = GetSave();

        // Render the JSON to a string
        string profilesString = profileData.writeString(false);

        // Set the value and write to disk
        saved_level.SetValue( "lugaru_profiles", profilesString );
        //Print("Profiles string : " + profilesString + "\n");
		Log( info, profileData.writeString(true) );
        save_file.WriteInPlace();

    }
	void writeDataToProfiles() {
        // Make sure that the data is good
        if( profileId == -1  ) {
            DisplayError("Persistence Error", "Trying to store an uninitialized profile.");
        }

        bool profileFound = false;

        for( uint i = 0; i < profileData.getRoot()["profiles"].size(); ++i ) {
            if( profileData.getRoot()["profiles"][ i ]["id"].asInt() == profileId ) {
                profileFound = true;

                // Copy all the values back
                profileData.getRoot()["profiles"][ i ][ "levels_finished" ] = JSONValue( levels_finished );
                profileData.getRoot()["profiles"][ i ][ "difficulty" ] = JSONValue( difficulty );
				profileData.getRoot()["profiles"][ i ]["challengeData"] = challengeData;
				profileData.getRoot()["profiles"][ i ]["challenge_levels_finished"] = JSONValue(challengeLevelsFinished);
                // We're done here
                break;
            }
        }
        // Sanity checking
        if( !profileFound ) {
            DisplayError("Persistence Error", "Profile id " + profileId + " not found in store.");
        }
    }
	void updateTimer(){
		bool triggered = true;
		array<int> characterIDs;
		GetCharacters(characterIDs);
		for(uint i = 0; i < characterIDs.size(); i++){
			MovementObject @char = ReadCharacterID(characterIDs[i]);
			if(!char.controlled && char.GetIntVar("knocked_out") == _awake){
				triggered = false;
			}
		}
		if(!timerStarted && triggered){
			timerStarted = true;
		}else if(timerStarted){
			timerTime += time_step;
			if(timerTime > 5.0f){
				currentState = agsEndScreen;
			}
		}
		challengeDuration += time_step;
	}
	void updatePoints(){
		if(pointsToAdd != 0){
			points++;
			pointsToAdd--;
			updateScore();
		}
	}
	void updateScore(){
		AHGUI::Text@ scoreText = cast<AHGUI::Text>(root.findElement( "ScoreText" ));
		if(scoreText !is null){
			scoreText.setText("Score " + points);
		}
	}
	void updateKeypresses(){
		if(currentState == agsEndScreen){
			if(GetInputPressed(0, "esc")){
				level.SendMessage("go_to_main_menu");
			}else if(GetInputPressed(0, "space")){
				//Load next challenge level
				string nextLevelName = challengeData[activeLevelIndex+1]["levelname"].asString();
				if(nextLevelName != ""){
					LoadLevel("LugaruChallenge/" + nextLevelName + ".xml");
				}
			}
		}
	}
	void DisplayText(AHGUI::Divider@ div, DividerDirection dd, string text, int textSize, vec4 color, bool shadowed, string textName = "singleSentence"){
        AHGUI::Text singleSentence( text, "OpenSans-Regular", textSize, color.x, color.y, color.z, color.a );
		singleSentence.setName(textName);
		singleSentence.setShadowed(shadowed);
        div.addElement(singleSentence, dd);
        if(showBorders){
            singleSentence.setBorderSize(1);
            singleSentence.setBorderColor(1.0, 1.0, 1.0, 1.0);
            singleSentence.showBorder( false );
        }
    }
}
ChallengeGUI challengeGUI;

void ReceiveMessage(string msg) {
    TokenIterator token_iter;
    token_iter.Init();
    if(!token_iter.FindNextToken(msg)){
        return;
    }
    string token = token_iter.GetToken(msg);
    if(token == "dispose_level"){
        gui.RemoveAll();
    } else if(token == "achievement_event"){
		//Print("Token: " + token + "\n");
        token_iter.FindNextToken(msg);
		//Print("Message: " + msg + "\n");
        AchievementEvent(token_iter.GetToken(msg));
    } else if(token == "achievement_event_float"){
		//Print("Token: " + token + "\n");
        token_iter.FindNextToken(msg);
        string str = token_iter.GetToken(msg);
		//Print("Message: " + str + "\n");
        token_iter.FindNextToken(msg);
        float val = atof(token_iter.GetToken(msg));
        AchievementEventFloat(str, val);
    }
}

void AchievementEvent(string event_str){
    if(event_str == "player_was_hit"){
        //achievements.PlayerWasHit();
    }
}

void AchievementEventFloat(string event_str, float val){
	if(challengeGUI.currentState == agsFighting){

	    if(event_str == "ai_damage"){
	        challengeGUI.pointsToAdd += int(val*100);
	    }
	}
}

bool HasFocus(){
    return false;
}

void Init(string str){
	challengeGUI.levelName = str;
    challengeGUI.ReadPersistentInfo();
}

void Update(){
    challengeGUI.update();
	challengeGUI.updateTimer();
	challengeGUI.updatePoints();
	challengeGUI.updateKeypresses();
}

void DrawGUI(){
    challengeGUI.render();
}

void Draw(){
}

bool CanGoBack(){
	return false;
}
void Dispose(){

}
