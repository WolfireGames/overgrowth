// These variables are used in the script and should not be changed by the user.
const float PI = 3.14159265359f;

Object@ this_hotspot = ReadObjectFromID(hotspot.GetID());
int citizen_id = -1;
int pathpoint_id = -1;
float proximity_check_timer = 0.0f;
DialogueLine @dialogue_line;
string drawn_dialogue_line = "";
string idle_animation = "";
int voice = 0;
bool citizen_placed_and_animated = false;
int player_inside_hotspot = -1;
int player_talking_id = -1;
float transition_to_player_timer = 0.0f;
float transition_to_idle_timer = 0.0f;
float post_dialogue_wait_timer = 0.0f;
float dialogue_progress = 0.0f;
citizen_states citizen_state = idle;
dialogue_draw_states dialogue_draw_state = none;
player_proximity_states player_proximity_state = interactive;

enum citizen_states{
								idle,
								waiting_for_input,
								transition_to_player,
								speaking,
								post_dialogue_wait,
								transition_to_idle
					}

enum dialogue_draw_states{
								none,
								draw_continues,
								draw_fade
						}

enum player_proximity_states{
								interactive,
								frozen,
								disabled
							}

enum Species {
    _rabbit = 0,
    _wolf = 1,
    _dog = 2,
    _rat = 3,
    _cat = 4
};

class DialogueLine{
	string text;
	// The probability range is between 1-10.
	int probability;

	DialogueLine(string _text, int _probability){
		text = _text;
		probability = _probability;
	}
}

// These are the variables that can be changed to get the desired effect.
array<string> character_collection = {	
											"Data/Objects/IGF_Characters/base_guard_actor.xml",
											"Data/Objects/IGF_Characters/pale_rabbit_civ_actor.xml",
											"Data/Objects/IGF_Characters/pale_turner_actor.xml"
									};

array<array<vec3>> palette_collection = {	
											{vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0)},
											{vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 0.0)},
											{vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 1.0)},
											{vec3(1.0, 1.0, 1.0), vec3(1.0, 1.0, 1.0), vec3(1.0, 1.0, 1.0), vec3(1.0, 1.0, 1.0)}
										};

array<string> idle_animations = {	
											"Data/Animations/bound_standing.anm",
											"Data/Animations/bound_standing2.anm",
											"Data/Animations/r_actionidle.anm",
											"Data/Animations/r_dialogue_worriedpose_2.anm",
											"Data/Animations/drika_idle.anm",
											"Data/Animations/amet_handhip.anm",
											"Data/Animations/amet_idle.anm",
											"Data/Animations/blue_idle.anm",
											"Data/Animations/dart_idle.anm",
											"Data/Animations/donatas_idle.anm",
											"Data/Animations/homeless_idle.anm",
											"Data/Animations/yuiek_idle.anm",
											"Data/Animations/shopkeep_idle.anm",
											"Data/Animations/r_crossarms_idle.anm",
											"Data/Animations/r_crouch.anm",
											"Data/Animations/r_crouch_sad.anm",
											"Data/Animations/r_dialogue_armcross.anm",
											"Data/Animations/r_dialogue_handhips.anm",
											"Data/Animations/r_dialogue_thoughtful.anm",
											"Data/Animations/r_dialogue_worriedpose_1.anm",
											"Data/Animations/r_idle.anm",
											"Data/Animations/r_meditate.anm",
											"Data/Animations/r_sit_cross_legged.anm",
											"Data/Animations/r_woundedcrouchthreat.anm",
											"Data/Animations/r_woundedidlearmed2.anm",
											"Data/Animations/trailergaze.anm",
											"Data/Animations/foltiep_6.anm",
											"Data/Animations/jairoidle.anm",
											"Data/Animations/wowanothersit.anm",
											"Data/Animations/interestingsit.anm",
											"Data/Animations/promotalk.anm",
											"Data/Animations/monk_idle.anm",
											"Data/Animations/nei_confront1.anm",
											"Data/Animations/folti_idle.anm",
											"Data/Animations/joker_idle.anm"
								};
									

// The chance for this hotspot to create a random citizen.
// For example 5 gives a one in five chance of spawning.
const int SPAWN_PROBABILITY = 4;
const float SCALE_MINIMUM = 0.78f;
const float SCALE_MAXIMUM = 1.15f;
const float MUSCLE_MINIMUM = 0.2f;
const float MUSCLE_MAXIMUM = 0.9f;
const float FAT_MINIMUM = 0.35f;
const float FAT_MAXIMUM = 0.75f;
const float EAR_SIZE_MINIMUM = 0.1f;
const float EAR_SIZE_MAXIMUM = 2.8f;
const float PLAYER_PROXIMITY_DISABLE_THRESHOLD = 70.0;
const float PLAYER_PROXIMITY_FROZEN_THRESHOLD = 15.0;
const bool USE_VOICE_SOUNDS = true;
const float DIALOGUE_TEXT_SPEED = 25.0f;
const float TRANSITION_TO_PLAYER_DURATION = 0.25;
const float TRANSITION_TO_IDLE_DURATION = 0.75;
const float POST_DIALOGUE_WAIT_DURATION = 0.5;

void Init(){
	CreateCharacter();
}

void Dispose(){
	if(GetInputDown(0, "delete")){
		DeleteCharacter();
	}
}

void Reset(){
	DeleteCharacter();
	dialogue_draw_state = none;
	citizen_placed_and_animated = false;
	player_inside_hotspot = -1;
	player_talking_id = -1;
	citizen_state = idle;
	player_proximity_state = interactive;
	CreateCharacter();
}

void CreateCharacter(){
	if(citizen_id != -1){return;}
	// There is a chance we DON'T create the character and an empty hotspot is left.
	if(rand() % SPAWN_PROBABILITY != 0){return;}
	voice = rand() % 19;

	// Create a new pathpoint for the character to use but don't save it to the level xml.
	pathpoint_id = CreateObject("Data/Objects/pathpoint/pathpoint.xml", true);
	Object@ pathpoint_obj = ReadObjectFromID(pathpoint_id);
	// Set the pathpoint to the center of this hotspot.
	pathpoint_obj.SetTranslation(this_hotspot.GetTranslation());
	pathpoint_obj.SetRotation(this_hotspot.GetRotation());
	ScriptParams@ pathpoint_params = pathpoint_obj.GetScriptParams();

	// Pick a random character to start modifying.
	int character_index = rand() % character_collection.size();
	// Choose a palette of colors that fit good together.
	int palette_index = rand() % palette_collection.size();

	// Create the character, but exclude it from the save so a new character is spawned next time.
	citizen_id = CreateObject(character_collection[character_index], true);
	Object@ char_obj = ReadObjectFromID(citizen_id);
	// Move the character to this hotspot location.
	char_obj.SetTranslation(this_hotspot.GetTranslation());
	char_obj.SetRotation(this_hotspot.GetRotation());

	int num_palette = char_obj.GetNumPaletteColors();
	array<vec3> palette = palette_collection[palette_index];

	// Apply all the palette colors to the character.
	for(uint i = 0; i < palette.size(); i++){
		// Make sure the pallete index exists on this character before setting it.
		if(int(i) <= num_palette){
			char_obj.SetPaletteColor(i, palette[i]);
		}
	}

	@dialogue_line = GetDialogueLine();

	// The parameters like fat, muscle and ear size are set in script parameters.
	ScriptParams@ char_params = char_obj.GetScriptParams();
	MovementObject@ char = ReadCharacterID(citizen_id);
	// Generate a random value between the set minimum and maximum.
	char_params.SetFloat("Character Scale", RangedRandomFloat(SCALE_MINIMUM, SCALE_MAXIMUM));
	float muscle = RangedRandomFloat(MUSCLE_MINIMUM, MUSCLE_MAXIMUM);
	char_params.SetFloat("Muscle", muscle);
	float fat = RangedRandomFloat(FAT_MINIMUM, FAT_MAXIMUM);
	char_params.SetFloat("Fat", fat);
	float ear_size = RangedRandomFloat(EAR_SIZE_MINIMUM, EAR_SIZE_MAXIMUM);
	char_params.SetFloat("Ear Size", ear_size);
	char_params.SetString("Teams", "guard, turner");

	int idle_animation_index = rand() % idle_animations.size();
	idle_animation = idle_animations[idle_animation_index];
	// The SetParameter function takes care of setting the global variables and scaling the riggedobject.
	string command = "SetParameters();";
	char.Execute(command);

	// Setting the animation as a hotspot parameter will make the AI use this animation when the pathpoint is reached.
	pathpoint_params.SetString("Type", idle_animation);
	// Connecting the pathpoint will make the AI start walking towards it.
	char_obj.ConnectTo(pathpoint_obj);
}

int GetWeightedRandomChoice(array<float> weights){
	float accumulated_weight = 0.0;
	array<float> entries = {};

	for(uint i = 0; i < weights.size(); i++){
		accumulated_weight += weights[i];
		entries.insertLast(accumulated_weight);
	}

	float random_float = RangedRandomFloat(0.0, 1.0) * accumulated_weight;

	for(uint i = 0; i < weights.size(); i++){
		if(entries[i] >= random_float){
			return i;
		}
	}

	return 0;
}

DialogueLine@ GetDialogueLine(){
	JSON json_file;
	JSONValue json_values;

	if(!json_file.parseFile("Data/Scripts/hotspots/random_citizen_dialogue.json")){
		Log(error, "Could not load the citizen dialogue JSON file!");
		return DialogueLine("Error", 1);
	}

	json_values = json_file.getRoot();

	// The weights to determine a dialogue between Blurbs, Conditions and Political.
	array<float> category_weights = {50.0, 25.0, 25.0};

	switch(GetWeightedRandomChoice(category_weights)){
		case 0:
			return GetBlurb(json_values);
		case 1:
			return GetCondition(json_values);
		case 2:
			return GetPolitical(json_values);
	}

	return DialogueLine("Error", 1);
}

DialogueLine@ GetBlurb(JSONValue json_values){
	// The weights to determine a dialogue between Dismissals and Racism.
	array<float> blurb_weights = {70.0, 30.0};

	switch(GetWeightedRandomChoice(blurb_weights)){
		case 0:
			return GetDismissal(json_values);
		case 1:
			return GetRacism(json_values);
	}
	
	return DialogueLine("Error", 1);
}

DialogueLine@ GetDismissal(JSONValue json_values){
	JSONValue dismissals = json_values["blurbs"]["dismissals"];
	array<string> texts = {};
	array<float> weights = {};
	MovementObject@ char = ReadCharacterID(citizen_id);
	int species = char.GetIntVar("species");

	for(uint i = 0; i < dismissals.size(); i++){
		string text = dismissals[i].asString();
		float weight = 10.0;

		if(FindAndReplace(text, "[uncommon]", "")){
			weight = 5.0;
		}else if(FindAndReplace(text, "[rare]", "")){
			weight = 1.0;
		}else if(FindAndReplace(text, "[Cat]", "")){
			// Only add this line if the character is a cat.
			if(species != _cat){continue;}
		}else if(FindAndReplace(text, "[Rabbit]", "")){
			// Do not add this line if the character isn't a rabbit.
			if(species != _rabbit){continue;}
		}else if(FindAndReplace(text, "[Non-Rabbit]", "")){
			// Do not add this line if the character is a rabbit.
			if(species == _rabbit){continue;}
		}

		texts.insertLast(text);
		weights.insertLast(weight);
	}

	string chosen_text = texts[GetWeightedRandomChoice(weights)];
	ReplaceWords(chosen_text, json_values);
	return DialogueLine(chosen_text, 1);
}

DialogueLine@ GetRacism(JSONValue json_values){
	JSONValue racisms = json_values["blurbs"]["racism"];
	array<string> texts = {};
	array<float> weights = {};
	MovementObject@ char = ReadCharacterID(citizen_id);
	int species = char.GetIntVar("species");

	for(uint i = 0; i < racisms.size(); i++){
		string text = racisms[i].asString();
		float weight = 10.0;

		if(FindAndReplace(text, "[Cat]", "")){
			// Only add this line if the character is a cat.
			if(species != _cat){continue;}
		}else if(FindAndReplace(text, "[Non-Rabbit]", "")){
			if(species == _rabbit){continue;}
		}else if(FindAndReplace(text, "[Non-Dog]", "")){
			if(species == _dog){continue;}
		}else if(FindAndReplace(text, "[Dog]", "")){
			if(species != _dog){continue;}
		}else if(FindAndReplace(text, "[Predator]", "")){
			// Predators are dogs, cats and wolves combined.
			if(species != _dog && species != _cat && species != _wolf){continue;}
		}

		texts.insertLast(text);
		weights.insertLast(weight);
	}

	string chosen_text = texts[GetWeightedRandomChoice(weights)];
	ReplaceWords(chosen_text, json_values);
	return DialogueLine(chosen_text, 1);
}

DialogueLine@ GetCondition(JSONValue json_values){
	// The weights to determine a dialogue between Pain, Mental, Environmental and Drugs.
	array<float> condition_weights = {8.0, 25.0, 25.0, 525.0};

	switch(GetWeightedRandomChoice(condition_weights)){
		case 0:
			return GetPain(json_values);
		case 1:
			return GetMental(json_values);
		case 2:
			return GetEnvironmental(json_values);
		case 3:
			return GetDrugs(json_values);
	}
	
	return DialogueLine("Error", 1);
}

DialogueLine@ GetPain(JSONValue json_values){
	JSONValue pains = json_values["conditions"]["pain"];
	array<string> texts = {};
	array<float> weights = {};

	for(uint i = 0; i < pains.size(); i++){
		string text = pains[i].asString();
		weights.insertLast(1.0);
		texts.insertLast(text);
	}

	string chosen_text = texts[GetWeightedRandomChoice(weights)];

	JSONValue pain_suffixes = json_values["conditions"]["pain suffixes"];
	array<string> texts_suffixes = {};
	array<float> weights_suffixes = {};

	for(uint i = 0; i < pain_suffixes.size(); i++){
		string text = pain_suffixes[i].asString();
		weights_suffixes.insertLast(1.0);
		texts_suffixes.insertLast(text);
	}

	string chosen_text_suffix = texts[GetWeightedRandomChoice(weights_suffixes)];
	string combined_pain = chosen_text + "\n" + chosen_text_suffix;

	ReplaceWords(combined_pain, json_values);
	return DialogueLine(combined_pain, 1);
}

DialogueLine@ GetMental(JSONValue json_values){
	JSONValue mentals = json_values["conditions"]["mental"];
	array<string> texts = {};
	array<float> weights = {};

	for(uint i = 0; i < mentals.size(); i++){
		string text = mentals[i].asString();
		float weight = 10.0;

		if(FindAndReplace(text, "[uncommon]", "")){
			weight = 5.0;
		}else if(FindAndReplace(text, "[rare]", "")){
			weight = 1.0;
		}

		texts.insertLast(text);
		weights.insertLast(weight);
	}

	string chosen_text = texts[GetWeightedRandomChoice(weights)];
	ReplaceWords(chosen_text, json_values);
	return DialogueLine(chosen_text, 1);
}

DialogueLine@ GetEnvironmental(JSONValue json_values){
	JSONValue environmentals = json_values["conditions"]["environmental"];
	array<string> texts = {};
	array<float> weights = {};

	for(uint i = 0; i < environmentals.size(); i++){
		string text = environmentals[i].asString();
		float weight = 10.0;

		if(FindAndReplace(text, "[uncommon]", "")){
			weight = 5.0;
		}else if(FindAndReplace(text, "[rare]", "")){
			weight = 1.0;
		}

		texts.insertLast(text);
		weights.insertLast(weight);
	}

	string chosen_text = texts[GetWeightedRandomChoice(weights)];
	ReplaceWords(chosen_text, json_values);
	return DialogueLine(chosen_text, 1);
}

DialogueLine@ GetDrugs(JSONValue json_values){
	JSONValue drugs = json_values["conditions"]["drugs"];
	array<string> texts = {};
	array<float> weights = {};

	for(uint i = 0; i < drugs.size(); i++){
		string text = drugs[i].asString();
		float weight = 10.0;

		texts.insertLast(text);
		weights.insertLast(weight);
	}

	string chosen_text = texts[GetWeightedRandomChoice(weights)];
	ReplaceWords(chosen_text, json_values);
	return DialogueLine(chosen_text, 1);
}

DialogueLine@ GetPolitical(JSONValue json_values){
	// The weights to determine a dialogue between Cinderbreathe, Arliss and Cinderblood.
	array<float> political_weights = {25.0, 25.0, 25.0};

	switch(GetWeightedRandomChoice(political_weights)){
		case 0:
			return GetCinderbreathe(json_values);
		case 1:
			return GetArliss(json_values);
		case 2:
			return GetCinderblood(json_values);
	}
	
	return DialogueLine("Error", 1);
}

DialogueLine@ GetCinderbreathe(JSONValue json_values){
	JSONValue cinderbreathes = json_values["political"]["cinderbreathe"];
	array<string> texts = {};
	array<float> weights = {};

	for(uint i = 0; i < cinderbreathes.size(); i++){
		string text = cinderbreathes[i].asString();
		float weight = 10.0;

		texts.insertLast(text);
		weights.insertLast(weight);
	}

	string chosen_text = texts[GetWeightedRandomChoice(weights)];
	ReplaceWords(chosen_text, json_values);
	return DialogueLine(chosen_text, 1);
}

DialogueLine@ GetArliss(JSONValue json_values){
	JSONValue arlisses = json_values["political"]["arliss"];
	array<string> texts = {};
	array<float> weights = {};
	MovementObject@ char = ReadCharacterID(citizen_id);
	int species = char.GetIntVar("species");

	for(uint i = 0; i < arlisses.size(); i++){
		string text = arlisses[i].asString();
		float weight = 10.0;

		if(FindAndReplace(text, "[rare]", "")){
			weight = 1.0;
		}else if(FindAndReplace(text, "[Rabbit]", "")){
			if(species != _rabbit){continue;}
		}else if(FindAndReplace(text, "[Non-Cat]", "")){
			if(species == _cat){continue;}
		}

		texts.insertLast(text);
		weights.insertLast(weight);
	}

	string chosen_text = texts[GetWeightedRandomChoice(weights)];
	ReplaceWords(chosen_text, json_values);
	return DialogueLine(chosen_text, 1);
}

DialogueLine@ GetCinderblood(JSONValue json_values){
	JSONValue cinderbloods = json_values["political"]["cinderblood"];
	array<string> texts = {};
	array<float> weights = {};

	for(uint i = 0; i < cinderbloods.size(); i++){
		string text = cinderbloods[i].asString();
		float weight = 10.0;

		if(FindAndReplace(text, "[rare]", "")){
			weight = 1.0;
		}

		texts.insertLast(text);
		weights.insertLast(weight);
	}

	string chosen_text = texts[GetWeightedRandomChoice(weights)];
	ReplaceWords(chosen_text, json_values);
	return DialogueLine(chosen_text, 1);
}

void ReplaceWords(string &inout text, JSONValue json_values){
	JSONValue words = json_values["words"];
	array<string> member_names = words.getMemberNames();

	for(uint i = 0; i < member_names.size(); i++){
		string find = member_names[i];
		JSONValue list = json_values["words"][find];
		array<string> replacements = {};
		array<float> weights = {};

		for(uint j = 0; j < list.size(); j++){
			string replacement = list[j].asString();
			float weight = 10.0;

			if(FindAndReplace(replacement, "[uncommon]", "")){
				weight = 5.0;
			}else if(FindAndReplace(replacement, "[rare]", "")){
				weight = 1.0;
			}

			weights.insertLast(weight);
			replacements.insertLast(replacement);
		}

		string replacement = replacements[GetWeightedRandomChoice(weights)];
		FindAndReplace(text, find, replacement);
	}
}

bool FindAndReplace(string &inout text, string find, string replacement){
	bool found = text.findFirst(find) != -1;
	array<string> split_text = text.split(find);
	text = join(split_text, replacement);
	return found;
}

void PlaceAndAnimateCitizen(){
	if(citizen_placed_and_animated){return;}
	if(citizen_id == -1 || !MovementObjectExists(citizen_id)){return;}

	citizen_placed_and_animated = true;

	MovementObject@ citizen = ReadCharacterID(citizen_id);	
	citizen.static_char = true;
	citizen.ReceiveScriptMessage("set_dialogue_control true");
	citizen.ReceiveScriptMessage("set_dialogue_position " + citizen.position.x + " " + citizen.position.y + " " + citizen.position.z);
	vec3 new_position = this_hotspot.GetTranslation();
	vec4 v = this_hotspot.GetRotationVec4();
	quaternion quat(v.x,v.y,v.z,v.a);
	vec3 facing = Mult(quat, vec3(-1.0, 0.0, 0.0));
	float rot = atan2(facing.x, facing.z) * 180.0f / PI;
	float char_rotation = floor(rot + 0.5f);
	citizen.ReceiveScriptMessage("set_rotation " + char_rotation);
	citizen.ReceiveScriptMessage("set_animation " + idle_animation);
	citizen.Execute("FixDiscontinuity();");

	// DebugDrawText(this_hotspot.GetTranslation(), dialogue_line.text, 1.0, false, _persistent);
}

void DeleteCharacter(){
	if(citizen_id != -1 && MovementObjectExists(citizen_id)){
		ForgetCharacter(citizen_id);
		QueueDeleteObjectID(citizen_id);
	}
	citizen_id= -1;

	if(pathpoint_id != -1 && ObjectExists(pathpoint_id)){
		QueueDeleteObjectID(pathpoint_id);
	}
	pathpoint_id= -1;
}

void ForgetCharacter(int id){
	array<int> movement_objects = GetObjectIDsType(_movement_object);
	for(uint i = 0; i < movement_objects.size(); i++){
		if(MovementObjectExists(movement_objects[i])){
			MovementObject@ char = ReadCharacterID(movement_objects[i]);
			char.Execute("situation.MovementObjectDeleted(" + id + ");");
		}
	}
}

void HandleEvent(string event, MovementObject @mo){
	if(citizen_id == -1 || !MovementObjectExists(citizen_id)){return;}
	if(!mo.is_player){return;}

	if(event == "enter"){
		citizen_state = waiting_for_input;
		player_inside_hotspot = mo.GetID();
	}else if(event == "exit"){
		player_inside_hotspot = -1;
	}
}

void Update(){
	// While editing and having this hotspot selected, keep updating the character position and rotation.
	if(EditorModeActive() && this_hotspot.IsSelected() && citizen_id != -1){
		MovementObject@ citizen = ReadCharacterID(citizen_id);
		citizen.position = this_hotspot.GetTranslation();
		citizen.SetRotationFromFacing(this_hotspot.GetRotation() * vec3(0.0, 0.0, 1.0));
		citizen.ReceiveScriptMessage("set_dialogue_position " + citizen.position.x + " " + citizen.position.y + " " + citizen.position.z);
		vec3 new_position = this_hotspot.GetTranslation();
		vec4 v = this_hotspot.GetRotationVec4();
		quaternion quat(v.x,v.y,v.z,v.a);
		vec3 facing = Mult(quat, vec3(-1.0, 0.0, 0.0));
		float rot = atan2(facing.x, facing.z) * 180.0f / PI;
		float char_rotation = floor(rot + 0.5f);
		citizen.ReceiveScriptMessage("set_rotation " + char_rotation);
	}

	PlaceAndAnimateCitizen();
	CheckPlayerProximity();
	UpdateState();
}

void UpdateState(){
	if(player_proximity_state != interactive){return;}
	if(citizen_id == -1 || !MovementObjectExists(citizen_id)){return;}

	switch(citizen_state){
		case idle:
			UpdateIdle();
			break;
		case waiting_for_input:
			UpdateWaitingForInput();
			break;
		case transition_to_player:
			UpdateTransitionToPlayer();
			break;
		case speaking:
			UpdateSpeaking();
			break;
		case post_dialogue_wait:
			UpdatePostDialogueWait();
			break;
		case transition_to_idle:
			UpdateTransitionToIdle();
			break;
		default:
			Log(error, "Unknown state in random citizen script!");
			break;
	}
}

void UpdateIdle(){

}

void UpdateWaitingForInput(){
	if(player_inside_hotspot == -1 || !MovementObjectExists(player_inside_hotspot)){
		player_inside_hotspot = -1;
		citizen_state = idle;
		return;
	}

	MovementObject@ player = ReadCharacterID(player_inside_hotspot);

	if(GetInputPressed(player.controller_id, "attack")){
		citizen_state = transition_to_player;
		player_talking_id = player_inside_hotspot;
		transition_to_player_timer = 0.0f;
		PlayLineStartSound();
	}
}

void UpdateTransitionToPlayer(){
	transition_to_player_timer += time_step;

	MovementObject@ player = ReadCharacterID(player_talking_id);
	MovementObject@ citizen = ReadCharacterID(citizen_id);

	vec3 player_head_position = player.rigged_object().GetAvgIKChainPos("head");
	
	float weight = transition_to_player_timer / TRANSITION_TO_PLAYER_DURATION;
	citizen.ReceiveScriptMessage("set_head_target " + player_head_position.x + " " + player_head_position.y + " " + player_head_position.z + " " + weight);
	citizen.ReceiveScriptMessage("set_torso_target " + player_head_position.x + " " + player_head_position.y + " " + player_head_position.z + " " + weight);

	if(transition_to_player_timer >= TRANSITION_TO_PLAYER_DURATION){
		citizen.ReceiveScriptMessage("start_talking");
		dialogue_progress = 0.0f;
		citizen_state = speaking;
	}
}

void UpdateSpeaking(){
	MovementObject@ player = ReadCharacterID(player_talking_id);
	MovementObject@ citizen = ReadCharacterID(citizen_id);

	vec3 player_head_position = player.rigged_object().GetAvgIKChainPos("head");
	citizen.ReceiveScriptMessage("set_head_target " + player_head_position.x + " " + player_head_position.y + " " + player_head_position.z + " " + 1.0);
	citizen.ReceiveScriptMessage("set_torso_target " + player_head_position.x + " " + player_head_position.y + " " + player_head_position.z + " " + 1.0);

	int line_length = dialogue_line.text.length();
	string joined_line = "";

	dialogue_draw_state = draw_continues;
	dialogue_progress += time_step * DIALOGUE_TEXT_SPEED / line_length;

	for(int p = 0; p < line_length; p++){
		if( p <= (line_length * dialogue_progress) ){
			joined_line += dialogue_line.text.substr(p, 1);
		}else{
			if(dialogue_line.text.substr(p, 1) == "\n"){
				joined_line += "\n";
			}else{
				joined_line += " ";
			}
		}
	}

	// Every time the text changes one character, play the sound effect.
	if(drawn_dialogue_line != joined_line){
		PlayLineContinueSound();
	}

	drawn_dialogue_line = joined_line;

	if(dialogue_progress >= 1.0){
		citizen.ReceiveScriptMessage("stop_talking");
		dialogue_draw_state = draw_fade;
		citizen_state = post_dialogue_wait;
		post_dialogue_wait_timer = 0.0f;
	}
}

void UpdatePostDialogueWait(){
	post_dialogue_wait_timer += time_step;

	MovementObject@ player = ReadCharacterID(player_talking_id);
	MovementObject@ citizen = ReadCharacterID(citizen_id);

	vec3 player_head_position = player.rigged_object().GetAvgIKChainPos("head");
	citizen.ReceiveScriptMessage("set_head_target " + player_head_position.x + " " + player_head_position.y + " " + player_head_position.z + " " + 1.0);
	citizen.ReceiveScriptMessage("set_torso_target " + player_head_position.x + " " + player_head_position.y + " " + player_head_position.z + " " + 1.0);

	if(post_dialogue_wait_timer >= POST_DIALOGUE_WAIT_DURATION){
		transition_to_idle_timer = TRANSITION_TO_IDLE_DURATION;
		citizen_state = transition_to_idle;
	}
}

void UpdateTransitionToIdle(){
	transition_to_idle_timer -= time_step;
	float weight = transition_to_idle_timer / TRANSITION_TO_IDLE_DURATION;

	MovementObject@ citizen = ReadCharacterID(citizen_id);
	MovementObject@ player = ReadCharacterID(player_talking_id);

	vec3 player_head_position = player.rigged_object().GetAvgIKChainPos("head");
	citizen.ReceiveScriptMessage("set_head_target " + player_head_position.x + " " + player_head_position.y + " " + player_head_position.z + " " + weight);
	citizen.ReceiveScriptMessage("set_torso_target " + player_head_position.x + " " + player_head_position.y + " " + player_head_position.z + " " + weight);

	if(transition_to_idle_timer <= 0.0){
		// The player is still inside the hotspot so go back to checking for input.
		if(player_inside_hotspot == player_talking_id){
			citizen_state = waiting_for_input;
		}else{
			citizen_state = idle;
			player_talking_id = -1;
		}
	}
}

void CheckPlayerProximity(){
	if(citizen_id == -1 || !MovementObjectExists(citizen_id)){return;}

	proximity_check_timer -= time_step;
	if(proximity_check_timer <= 0.0){
		// Use a random timer value so that not all the hotspots are updating at the same time causing lag spikes.
		proximity_check_timer = RangedRandomFloat(1.0, 2.0);
		// Skip setting the character enabled when there is no character.
		if(citizen_id == -1 || !MovementObjectExists(citizen_id)){return;}
		// Retrieve the current player in the scene.
		for(int i = 0; i < GetNumCharacters(); i++){
			MovementObject@ char = ReadCharacter(i);
			if(char.is_player){
				Object@ citizen_obj = ReadObjectFromID(citizen_id);
				MovementObject@ citizen = ReadCharacterID(citizen_id);
				
				// Disable the character when the player is too far away, and enable it again when it is in proximity.
				float player_distance = 0.0f;
				// Use the camera position when editing the scene.
				if(EditorModeActive()){
					player_distance = distance(camera.GetPos(), this_hotspot.GetTranslation());
				}else{
					player_distance = distance(char.position, this_hotspot.GetTranslation());
				}

				switch(player_proximity_state){
					case(interactive):
						if(player_distance > PLAYER_PROXIMITY_FROZEN_THRESHOLD){
							player_proximity_state = frozen;
							ScriptParams@ citizen_params = citizen_obj.GetScriptParams();
							citizen.ReceiveScriptMessage("set_dialogue_control false");
							citizen_params.SetString("dead_body", "");
						}
						break;
					case(frozen):
						if(player_distance < PLAYER_PROXIMITY_FROZEN_THRESHOLD){
							player_proximity_state = interactive;
							ScriptParams@ citizen_params = citizen_obj.GetScriptParams();
							citizen_placed_and_animated = false;
							citizen_params.Remove("dead_body");
						}else if(player_distance > PLAYER_PROXIMITY_DISABLE_THRESHOLD){
							ForgetCharacter(citizen_id);
							citizen_obj.SetEnabled(false);
							player_proximity_state = disabled;
						}
						break;
					case(disabled):
						if(player_distance < PLAYER_PROXIMITY_DISABLE_THRESHOLD){
							citizen_obj.SetEnabled(true);
							player_proximity_state = frozen;
						}
						break;
					default:
						Log(error, "Unknown player proximity state in random citizen script!");
						break;
				}
			}
		}
	}
}

void Draw(){
	if(citizen_id == -1 || !MovementObjectExists(citizen_id)){return;}

	if(dialogue_draw_state != none){
		vec3 head_position = ReadCharacterID(citizen_id).rigged_object().GetAvgIKChainPos("head");
		vec3 offset = vec3(0.0, 0.25, 0.0);
		int draw_method = _delete_on_draw;

		if(dialogue_draw_state == draw_fade){
			draw_method = _fade;
			dialogue_draw_state = none;
		}
		
		DebugDrawText(head_position - offset, drawn_dialogue_line, 1.0, false, draw_method);
	}
}

void PlayLineContinueSound() {
	if(!USE_VOICE_SOUNDS){return;}
    switch(voice){
        case 0: PlaySoundGroup("Data/Sounds/concrete_foley/fs_light_concrete_edgecrawl.xml"); break;
        case 1: PlaySoundGroup("Data/Sounds/drygrass_foley/fs_light_drygrass_crouchwalk.xml"); break;
        case 2: PlaySoundGroup("Data/Sounds/cloth_foley/cloth_fabric_crouchwalk.xml"); break;
        case 3: PlaySoundGroup("Data/Sounds/dirtyrock_foley/fs_light_dirtyrock_crouchwalk.xml"); break;
        case 4: PlaySoundGroup("Data/Sounds/cloth_foley/cloth_leather_crouchwalk.xml"); break;
        case 5: PlaySoundGroup("Data/Sounds/grass_foley/fs_light_grass_run.xml", 0.5); break;
        case 6: PlaySoundGroup("Data/Sounds/gravel_foley/fs_light_gravel_crouchwalk.xml"); break;
        case 7: PlaySoundGroup("Data/Sounds/sand_foley/fs_light_sand_crouchwalk.xml", 0.7); break;
        case 8: PlaySoundGroup("Data/Sounds/snow_foley/fs_light_snow_run.xml", 0.5); break;
        case 9: PlaySoundGroup("Data/Sounds/wood_foley/fs_light_wood_crouchwalk.xml", 0.4); break;
        case 10: PlaySoundGroup("Data/Sounds/water_foley/mud_fs_walk.xml", 0.4); break;
        case 11: PlaySoundGroup("Data/Sounds/concrete_foley/fs_heavy_concrete_walk.xml", 0.5); break;
        case 12: PlaySoundGroup("Data/Sounds/drygrass_foley/fs_heavy_drygrass_walk.xml", 0.4); break;
        case 13: PlaySoundGroup("Data/Sounds/dirtyrock_foley/fs_heavy_dirtyrock_walk.xml", 0.5); break;
        case 14: PlaySoundGroup("Data/Sounds/grass_foley/fs_heavy_grass_walk.xml", 0.3); break;
        case 15: PlaySoundGroup("Data/Sounds/gravel_foley/fs_heavy_gravel_walk.xml", 0.3); break;
        case 16: PlaySoundGroup("Data/Sounds/sand_foley/fs_heavy_sand_run.xml", 0.3); break;
        case 17: PlaySoundGroup("Data/Sounds/snow_foley/fs_heavy_snow_crouchwalk.xml", 0.3); break;
        case 18: PlaySoundGroup("Data/Sounds/wood_foley/fs_heavy_wood_walk.xml", 0.3); break;
    }
}

void PlayLineStartSound(){
	if(!USE_VOICE_SOUNDS){return;}
	switch(voice){
		case 0: PlaySoundGroup("Data/Sounds/concrete_foley/fs_light_concrete_run.xml"); break;
		case 1: PlaySoundGroup("Data/Sounds/drygrass_foley/fs_light_drygrass_walk.xml"); break;
		case 2: PlaySoundGroup("Data/Sounds/cloth_foley/cloth_fabric_choke_move.xml"); break;
		case 3: PlaySoundGroup("Data/Sounds/dirtyrock_foley/fs_light_dirtyrock_run.xml"); break;
		case 4: PlaySoundGroup("Data/Sounds/cloth_foley/cloth_leather_choke_move.xml"); break;
		case 5: PlaySoundGroup("Data/Sounds/grass_foley/bf_grass_medium.xml", 0.5); break;
		case 6: PlaySoundGroup("Data/Sounds/gravel_foley/fs_light_gravel_run.xml"); break;
		case 7: PlaySoundGroup("Data/Sounds/sand_foley/fs_light_sand_run.xml", 0.7); break;
		case 8: PlaySoundGroup("Data/Sounds/snow_foley/bf_snow_light.xml", 0.5); break;
		case 9: PlaySoundGroup("Data/Sounds/wood_foley/fs_light_wood_run.xml", 0.4); break;
		case 10: PlaySoundGroup("Data/Sounds/water_foley/mud_fs_run.xml", 0.4); break;
		case 11: PlaySoundGroup("Data/Sounds/concrete_foley/fs_heavy_concrete_run.xml", 0.5); break;
		case 12: PlaySoundGroup("Data/Sounds/drygrass_foley/fs_heavy_drygrass_run.xml", 0.4); break;
		case 13: PlaySoundGroup("Data/Sounds/dirtyrock_foley/fs_heavy_dirtyrock_run.xml", 0.5); break;
		case 14: PlaySoundGroup("Data/Sounds/grass_foley/fs_heavy_grass_run.xml", 0.3); break;
		case 15: PlaySoundGroup("Data/Sounds/gravel_foley/fs_heavy_gravel_run.xml", 0.3); break;
		case 16: PlaySoundGroup("Data/Sounds/sand_foley/fs_heavy_sand_jump.xml", 0.3); break;
		case 17: PlaySoundGroup("Data/Sounds/snow_foley/fs_heavy_snow_jump.xml", 0.3); break;
		case 18: PlaySoundGroup("Data/Sounds/wood_foley/fs_heavy_wood_run.xml", 0.3); break;
	}
}