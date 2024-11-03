///Scar of Ouruin Level Hotspot
///This hotspot listens for level messages and sets DHS variables based on those messages

void SetParameters(){
	params.AddString("level_name", "explore");
}

void Init(){
	level.ReceiveLevelEvents(hotspot.GetID());
}

void Dispose() {
	level.StopReceivingLevelEvents(hotspot.GetID());
}

void SaveDataToFile(string variable_name, string value){ 	//This custom function works like the Write mode in DHS ReadWriteSaveFile
	SavedLevel@ data = save_file.GetSavedLevel("drika_data"); //Here we define where we are saving the data - DHS uses drika_data
	
	data.SetValue(variable_name, value); //First we set the variable and give it a value
	data.SetValue(("[" + variable_name + "]"), "true"); //This additional variable is what DHS uses to check if the variable exists
	
	save_file.WriteInPlace();
}

void AddVariableToVariable(string variable1, string variable2){ //Take two variables, convert them to integers, add them together and write the result to variable2
	SavedLevel@ data = save_file.GetSavedLevel("drika_data");
	int value1 = atoi(data.GetValue(variable1));
	int value2 = atoi(data.GetValue(variable2));
	string value_to_save = "" + (value1 + value2);
	SaveDataToFile(variable2, value_to_save);
}

void AddValueToVariable(int integer1, string variable2){
    SavedLevel@ data = save_file.GetSavedLevel("drika_data");
    int value1 = integer1;
    int value2 = atoi(data.GetValue(variable2));
    string value_to_save = "" + (value1 + value2);
    SaveDataToFile(variable2, value_to_save);
}

array<string> tracked_values = {
	"eliminations",
	"kills",
	"throatscut",
	"alerts",
	"zonekills",
	"overkills",
	"ko"
};

array<int> list_ids;
array<int> list_states;

void CharacterStates() {
	list_ids.resize(0);
	list_states.resize(0);

	// Adds all inlevel characters into the array (does not empty list beforehand).
	GetCharacters(list_ids);
	list_states.resize(list_ids.length());

	for (int i = 0; i < int(list_ids.length()); ++i)
	{
		MovementObject@ character_mo = ReadCharacterID(list_ids[i]);
		Object@ object_name = ReadObjectFromID(character_mo.GetID());
		int ko_state = character_mo.GetIntVar("knocked_out");
		string ko_status = character_mo.GetIntVar("knocked_out");
		
		// ko_state can have one of these values
		// _awake = 0, _unconscious = 1, _dead = 2
		
		list_states[i] = ko_state;
		
		// Save character state to DHS variable (ex: ouruin_explore_enemy_[ID]_status)
		if(object_name.GetEnabled() == true) {
			SaveDataToFile("ouruin_" + params.GetString("level_name") + "_enemy_" + list_ids[i] + "_status", ko_status);
		}
	}
}


// Will print all level_events (except tutorial level_events)
bool log_all_levelevents = true;
// Will print all event messages no matter what
bool log_all_events = false;

void ReceiveMessage(string msg){ //This function listens for level messages
	TokenIterator token_iter;
	token_iter.Init(); //A token iterator reads through a message searching for strings/values separated by spaces
	if(!token_iter.FindNextToken(msg)) { //If there is no message then no need to do anything - back out of the function
		return;
	}

	string token = token_iter.GetToken(msg); //Now we find our first token and pass it to a 'string' variable
	if (log_all_events) {
		Log(info, msg);
	}
	if(token == "level_event"){
		// iterate to the next token in 'msg'
		// DON'T FORGET THIS, DUMBASS
		token_iter.FindNextToken(msg);
		token = token_iter.GetToken(msg);

		if (log_all_levelevents && (!log_all_events) && token != "tutorial" && token != "added_object" && token != "notify_deleted" && token != "moved_objects" && token != "drika_dialogue_next") {
			Log(info, msg);
		}

		// Set all variables to 0 upon first starting level
		if(token == "level_start"){
			for (int i = 0; i < int(tracked_values.length()); ++i) {
				SaveDataToFile("ouruin_" + params.GetString("level_name") + "_" + tracked_values[i] + "_current", "0");
			}
		}

		//Now we check for each message one by one and if we encounter a message we do the appropriate action.
		//Note that you can extend this function to read more complex multi-part messages if needed
//		if(token == "message1"){
//			SaveDataToFile("count1" ,"value1"); //Call this if you want to manually set a variable
//		}
//		if(token == "message2"){
//			AddVariableToVariable("stored_count1", "count1"); //Call this to add two variables together. The first gets added to the second.
//		}
		
		// Increment elimination count
		if(token == "enemy_eliminated"){
			Log(info, "Enemy eliminated!");
			AddValueToVariable(1, "ouruin_" + params.GetString("level_name") + "_eliminations" + "_current");
		}
		// Increment KO count
		if(token == "enemy_ko"){
			Log(info, "Enemy KO'd!");
			AddValueToVariable(1, "ouruin_" + params.GetString("level_name") + "_ko" + "_current");
		}
		// Increment kill count
		if(token == "enemy_killed"){
			AddValueToVariable(1, "ouruin_" + params.GetString("level_name") + "_kills" + "_current");
			Log(info, "Enemy killed!");
		}
		// Increment throats cut count
		if(token == "cut_throat"){
			AddValueToVariable(1, "ouruin_" + params.GetString("level_name") + "_throatscut" + "_current");
			Log(info, "Throat slashed!");
		}
		// Increment alert count
		if(token == "enemy_alerted"){
			AddValueToVariable(1, "ouruin_" + params.GetString("level_name") + "_alerts" + "_current");
			Log(info, "Enemy alerted!");
		}
		// Increment zone kill count
		if(token == "enemy_zone_killed"){
			AddValueToVariable(1, "ouruin_" + params.GetString("level_name") + "_zonekills" + "_current");
			Log(info, "Enemy zone killed!");
		}
		// Increment overkill count
		if(token == "enemy_overkilled"){
			AddValueToVariable(1, "ouruin_" + params.GetString("level_name") + "_overkills" + "_current");
			Log(info, "Enemy overkilled!");
		}

		// Update character lethality state variables
		if(token == "update_lethality"){
			CharacterStates();
		}
		
		if(token == "reset"){
			// Return current variables to 0
			for (int i = 0; i < int(tracked_values.length()); ++i) {
				SaveDataToFile("ouruin_" + params.GetString("level_name") + "_" + tracked_values[i] + "_current", "0");
			}
			Log(info, "Current variables reset!");
		}
	}
}

void DrawEditor() {
    Object@ obj = ReadObjectFromID(hotspot.GetID());
    DebugDrawBillboard("Data/Images/ouruin/scarlogosquare.png",
                       obj.GetTranslation(),
                       obj.GetScale()[1]*3.0,
                       vec4(vec3(0.5), 1.0),
                       _delete_on_draw);
}