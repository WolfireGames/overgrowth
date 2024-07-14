enum dialogue_layouts	{
							default_layout = 0,
							simple_layout = 1,
							breath_of_the_wild_layout = 2,
							chrono_trigger_layout = 3,
							fallout_3_layout = 4,
							luigis_mansion_layout = 5,
							mafia_layout = 6
						}

array<string> dialogue_layout_names =	{
											"Default",
											"Simple",
											"Breath Of The Wild",
											"Chrono Trigger",
											"Fallout 3",
											"Luigi's Mansion",
											"Mafia"
										};

enum ui_functions	{
						ui_clear = 0,
						ui_image = 1,
						ui_text = 2,
						ui_font = 3,
						ui_button = 4,
						ui_input = 5,
						ui_fullscreen_image = 6
					}

enum music_modes	{
						music_song = 0,
						music_song_in_combat = 1,
						music_song_player_died = 2,
						music_song_enemies_defeated = 3,
						music_song_ambient = 4,
						music_silence = 5
					}

array<string> tween_types = {
								"Linear",

								"InQuad",
								"OutQuad",
								"InOutQuad",
								"OutInQuad",

								"InCubic",
								"OutCubic",
								"InOutCubic",
								"OutInCubic",

								"InQuart",
								"OutQuart",
								"InOutQuart",
								"OutInQuart",

								"InQuint",
								"OutQuint",
								"InOutQuint",
								"OutInQuint",

								"InSine",
								"OutSine",
								"InOutSine",
								"OutInSine",

								"InExpo",
								"OutExpo",
								"InOutExpo",
								"OutInExpo",

								"InCirc",
								"OutCirc",
								"InOutCirc",
								"OutInCirc",

								"InBounce",
								"OutBounce",
								"InOutBounce",
								"OutInBounce"
							};

enum dialogue_locations {
							dialogue_bottom = 0,
							dialogue_top = 1,
							dialogue_center = 2
						}

array<string> dialogue_location_names =	{
											"Bottom",
											"Top",
											"Center"
										};

enum script_entry_types {
							empty_entry,
							character_entry,
							new_line_entry,
							wait_entry,
							start_red_text_entry,
							start_green_text_entry,
							start_blue_text_entry,
							end_coloured_text_entry
						}

enum special_fonts	{
						none,
						red,
						green,
						blue
					}

enum WeaponSlot {
    _held_left = 0,
    _held_right = 1,
    _sheathed_left = 2,
    _sheathed_right = 3,
    _sheathed_left_sheathe = 4,
    _sheathed_right_sheathe = 5,
};

class DialogueScriptEntry{
	script_entry_types script_entry_type;
	float wait;
	string character;
	IMText @text;
	int line;

	DialogueScriptEntry(script_entry_types script_entry_type){
		this.script_entry_type = script_entry_type;
	}
}

array<DialogueScriptEntry@> InterpDialogueScript(string script_text_input){
	string script_text = ReplaceVariablesFromText(script_text_input);
	array<DialogueScriptEntry@> new_dialogue_script;
	array<string> script_text_split;

	//Get all the character seperate.
	for(uint i = 0; i < script_text.length(); i++){
		script_text_split.insertLast(script_text.substr(i, 1));
	}

	for(uint i = 0; i < script_text_split.size(); i++){
		//When the next character is an opening bracket then it might be a command.
		if(script_text_split[i] == "["){
			//Get the locaton of the end bracket so that we can get the whole command inside.
			int end_bracket_index = script_text.findFirst("]", i);
			if(end_bracket_index != -1){
				string command = script_text.substr(i + 1, end_bracket_index - (i + 1));
				//Check if the first 4 letters turn out to be a wait command.
				if(command.substr(0, 4) == "wait"){
					float wait_amount = atof(command.substr(4, command.length() - 4));
					DialogueScriptEntry entry(wait_entry);
					entry.wait = wait_amount;
					new_dialogue_script.insertLast(entry);
					// Skip adding the whole content of inside the brackets.
					i = end_bracket_index;
					continue;
				}else if(command.substr(0, 3) == "red"){
					DialogueScriptEntry entry(start_red_text_entry);
					new_dialogue_script.insertLast(entry);
					i = end_bracket_index;
					continue;
				}else if(command.substr(0, 5) == "green"){
					DialogueScriptEntry entry(start_green_text_entry);
					new_dialogue_script.insertLast(entry);
					i = end_bracket_index;
					continue;
				}else if(command.substr(0, 4) == "blue"){
					DialogueScriptEntry entry(start_blue_text_entry);
					new_dialogue_script.insertLast(entry);
					i = end_bracket_index;
					continue;
				}else if(command.substr(0, 1) == "/"){
					DialogueScriptEntry entry(end_coloured_text_entry);
					new_dialogue_script.insertLast(entry);
					i = end_bracket_index;
					continue;
				}
			}
		}else if(script_text_split[i] == "\n"){
			DialogueScriptEntry entry(new_line_entry);
			new_dialogue_script.insertLast(entry);
			continue;
		}
		DialogueScriptEntry entry(character_entry);
		entry.character = script_text_split[i];
		new_dialogue_script.insertLast(entry);
	}

	return new_dialogue_script;
}

string ReplaceVariablesFromText(string input){ // This function reads the input for items between braces{} and interprets those as variables.
	int position_in_string = 0;

	while(uint(position_in_string) < input.length()) // First let's see if there are any braces in the string at all.
	{
		int start_brace_pos = input.findFirst("{", position_in_string); //We'll use these two statements a lot, so let's assign them to a variable.
		int end_brace_pos = input.findFirst("}", start_brace_pos);

		if (start_brace_pos >= 0 && end_brace_pos >= 0)
		{
			string unfiltered_braces = input.substr(start_brace_pos, end_brace_pos - start_brace_pos + 1); //First get the contents of the braces, including extra start braces inside
			string filtered_braces = unfiltered_braces.substr(unfiltered_braces.findLast("{"),unfiltered_braces.length()); //Reduce to the start brace closest to the end brace
			string stored_value;

			int difference = unfiltered_braces.length() - filtered_braces.length(); //We use this to figure out the start position to replace the {variable}

			if (start_brace_pos + difference == 0 || input[start_brace_pos + difference - 1] != "\\"[0]) //We need to check if there is a backslash first
			{
				input.erase(start_brace_pos + difference, filtered_braces.length()); //Here we erase the variable name
				stored_value = GetSavedVariable(filtered_braces.substr(1, filtered_braces.length() - 2)); //Get the actual contents of the variable
				input.insert(start_brace_pos + difference, stored_value); //And replace filtered_braces with those contents
			}
			else
			{
			input.erase(start_brace_pos + difference - 1,1); //If there was a backslash just before the variable, that's the only thing we want to remove
			stored_value = filtered_braces;
			}

			position_in_string = start_brace_pos + difference + stored_value.length(); //Let's update our position and continue checking
		}
		else
		{
			break;
		}
	}
	return input;
}

string ReplaceQuotes(string input_text){
	return join(input_text.split("\""), "\\\"");
}

string GetSavedVariable(string key){
	SavedLevel@ data = save_file.GetSavedLevel("drika_data");
	return (data.GetValue("[" + key + "]") == "true")? data.GetValue(key) : "--ERROR - " + key + " does not exist--";
}

float InQuad(float progress){
	return progress * progress;
}

float OutQuad(float progress){
	return 1 - (1 - progress) * (1 - progress);
}

float InOutQuad(float progress){
	return progress < 0.5 ? 2 * progress * progress : 1 - pow(-2 * progress + 2, 2) / 2;
}

float OutInQuad(float progress){
	return mix(OutQuad(progress), InQuad(progress), progress);
}

float InCubic(float progress){
	return progress * progress * progress;
}

float OutCubic(float progress){
	return 1 - pow(1 - progress, 3);
}

float InOutCubic(float progress){
	return progress < 0.5 ? 4 * progress * progress * progress : 1 - pow(-2 * progress + 2, 3) / 2;
}

float OutInCubic(float progress){
	return mix(OutCubic(progress), InCubic(progress), progress);
}

float InQuart(float progress){
	return progress * progress * progress * progress;
}

float OutQuart(float progress){
	return 1 - pow(1 - progress, 4);
}

float InOutQuart(float progress){
	return progress < 0.5 ? 8 * progress * progress * progress * progress : 1 - pow(-2 * progress + 2, 4) / 2;
}

float OutInQuart(float progress){
	return mix(OutQuart(progress), InQuart(progress), progress);
}

float InQuint(float progress){
	return progress * progress * progress * progress * progress;
}

float OutQuint(float progress){
	return 1 - pow(1 - progress, 5);
}

float InOutQuint(float progress){
	return progress < 0.5 ? 16 * progress * progress * progress * progress * progress : 1 - pow(-2 * progress + 2, 5) / 2;
}

float OutInQuint(float progress){
	return mix(OutQuint(progress), InQuint(progress), progress);
}

float InSine(float progress){
	return 1 - cos((progress * PI) / 2);
}

float OutSine(float progress){
	return sin((progress * PI) / 2);
}

float InOutSine(float progress){
	return -(cos(PI * progress) - 1) / 2;
}

float OutInSine(float progress){
	return mix(OutSine(progress), InSine(progress), progress);
}

float InExpo(float progress){
	return progress == 0 ? 0.0 : pow(2, 10 * progress - 10);
}

float OutExpo(float progress){
	return progress == 1 ? 1.0 : 1 - pow(2, -10 * progress);
}

float InOutExpo(float progress){
	return progress == 0
	  ? 0.0
	  : progress == 1
	  ? 1.0
	  : progress < 0.5 ? pow(2, 20 * progress - 10) / 2
	  : (2 - pow(2, -20 * progress + 10)) / 2;
}

float OutInExpo(float progress){
	return mix(OutExpo(progress), InExpo(progress), progress);
}

float InCirc(float progress){
	return 1 - sqrt(1 - pow(progress, 2));
}

float OutCirc(float progress){
	return sqrt(1 - pow(progress - 1, 2));
}

float InOutCirc(float progress){
	return progress < 0.5
	  ? (1 - sqrt(1 - pow(2 * progress, 2))) / 2
	  : (sqrt(1 - pow(-2 * progress + 2, 2)) + 1) / 2;
}

float OutInCirc(float progress){
	return mix(OutCirc(progress), InCirc(progress), progress);
}

float InBounce(float progress){
	return 1 - OutBounce(1 - progress);
}

float OutBounce(float progress){
	const float n1 = 7.5625;
	const float d1 = 2.75;

	if (progress < 1 / d1) {
	    return n1 * progress * progress;
	} else if (progress < 2 / d1) {
	    return n1 * (progress -= 1.5 / d1) * progress + 0.75;
	} else if (progress < 2.5 / d1) {
	    return n1 * (progress -= 2.25 / d1) * progress + 0.9375;
	} else {
	    return n1 * (progress -= 2.625 / d1) * progress + 0.984375;
	}
}

float InOutBounce(float progress){
	return progress < 0.5
	  ? (1 - OutBounce(1 - 2 * progress)) / 2
	  : (1 + OutBounce(2 * progress - 1)) / 2;
}

float OutInBounce(float progress){
	return mix(OutBounce(progress), InBounce(progress), progress);
}

float ApplyTween(float progress, IMTweenType tween_type){
	switch(tween_type){
		case linearTween:
			return progress;
		case inQuadTween:
			return InQuad(progress);
		case outQuadTween:
			return OutQuad(progress);
		case inOutQuadTween:
			return InOutQuad(progress);
		case outInQuadTween:
			return OutInQuad(progress);

		case inCubicTween:
			return InCubic(progress);
		case outCubicTween:
			return OutCubic(progress);
		case inOutCubicTween:
			return InOutCubic(progress);
		case outInCubicTween:
			return OutInCubic(progress);

		case inQuartTween:
			return InQuart(progress);
		case outQuartTween:
			return OutQuart(progress);
		case inOutQuartTween:
			return InOutQuart(progress);
		case outInQuartTween:
			return OutInQuart(progress);

		case inQuintTween:
			return InQuint(progress);
		case outQuintTween:
			return OutQuint(progress);
		case inOutQuintTween:
			return InOutQuint(progress);
		case outInQuintTween:
			return OutInQuint(progress);

		case inSineTween:
			return InSine(progress);
		case outSineTween:
			return OutSine(progress);
		case inOutSineTween:
			return InOutSine(progress);
		case outInSineTween:
			return OutInSine(progress);

		case inExpoTween:
			return InExpo(progress);
		case outExpoTween:
			return OutExpo(progress);
		case inOutExpoTween:
			return InOutExpo(progress);
		case outInExpoTween:
			return OutInExpo(progress);

		case inCircTween:
			return InCirc(progress);
		case outCircTween:
			return OutCirc(progress);
		case inOutCircTween:
			return InOutCirc(progress);
		case outInCircTween:
			return OutInCirc(progress);

		case inBounceTween:
			return InBounce(progress);
		case outBounceTween:
			return OutBounce(progress);
		case inOutBounceTween:
			return InOutBounce(progress);
		case outInBounceTween:
			return OutInBounce(progress);
	}
	Log(warning, "Tween value not found " + tween_type);
	return progress;
}

bool IsPlayerInCombat(MovementObject@ player){

	string command = 	'if(knocked_out != _awake){
							self_id = -1;
						}else{
							self_id = -1;
							for(uint i=0; i<situation.known_chars.size(); ++i){
								KnownChar@ known_char = situation.known_chars[i];
								if(!known_char.friendly){
									if(MovementObjectExists(known_char.id)) {
										MovementObject@ char = ReadCharacterID(known_char.id);
										if(char.GetIntVar(\"knocked_out\") == _awake){
											if(char.controlled){
												string command = \"self_id = situation.KnownID(\" + this_mo.GetID() + \");\";
												char.Execute(command);
												if((char.GetIntVar(\"self_id\") != -1)){
													self_id = 1;
													break;
												}
											}else{
												if(char.QueryIntFunction(\"int IsAggressive()\") == 1){
													self_id = 1;
													break;
												}
											}
										}
									}
								}
							}
						}';

	player.Execute(command);
	return (player.GetIntVar("self_id") == 1);
}