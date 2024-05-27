enum character_control_options { 	aggression = 0,
									attack_damage = 1,
									attack_knockback = 2,
									attack_speed = 3,
									block_followup = 4,
									block_skill = 5,
									cannot_be_disarmed = 6,
									character_scale = 7,
									damage_resistance = 8,
									ear_size = 9,
									fat = 10,
									focus_fov_distance = 11,
									focus_fov_horizontal = 12,
									focus_fov_vertical = 13,
									ground_aggression = 14,
									knocked_out_shield = 15,
									left_handed = 16,
									movement_speed = 17,
									muscle = 18,
									peripheral_fov_distance = 19,
									peripheral_fov_horizontal = 20,
									peripheral_fov_vertical = 21,
									species = 22,
									static_char = 23,
									teams = 24,
									fall_damage_mult = 25,
									fear_afraid_at_health_level = 26,
									fear_always_afraid_on_sight = 27,
									fear_causes_fear_on_sight = 28,
									fear_never_afraid_on_sight = 29,
									no_look_around = 30,
									stick_to_nav_mesh = 31,
									throw_counter_probability = 32,
									is_throw_trainer = 33,
									weapon_catch_skill = 34,
									wearing_metal_armor = 35,
									ignite = 36,
									extinguish = 37,
									is_player = 38,
									kill = 39,
									revive = 40,
									limp_ragdoll = 41,
									injured_ragdoll = 42,
									ragdoll = 43,
									cut_throat = 44,
									apply_damage = 45,
									wet = 46,
									attach_item = 47,
									sheathe_item = 48,
									idle_sway = 49
					};

enum value_type_options			{ 	manual_input = 0,
									variable = 1
					};

class DrikaCharacterControl : DrikaElement{
	int current_type;

	int current_value_type;
	
	string variable_input_1 = "";
	string variable_input_2 = "";
	
	string string_param_after = "";
	int int_param_after = 0;
	bool bool_param_after = false;
	float float_param_after = 0.0;
	float recovery_time;
	float roll_recovery_time;
	float damage_amount;
	float wet_amount;
	array<Object@> target_cache;
	int attachment_type;
	bool mirrored;
	DrikaTargetSelect@ item_select;

	array<BeforeValue@> params_before;

	param_types param_type;
	character_control_options character_control_option;
	value_type_options value_type_option;
	string param_name;

	array<int> string_parameters = {species, teams};
	array<int> float_parameters = {aggression, attack_damage, attack_knockback, attack_speed, block_followup, block_skill, character_scale, damage_resistance, ear_size, fat, focus_fov_distance, focus_fov_horizontal, focus_fov_vertical, ground_aggression, movement_speed, muscle, peripheral_fov_distance, peripheral_fov_horizontal, peripheral_fov_vertical, fall_damage_mult, fear_afraid_at_health_level, throw_counter_probability, weapon_catch_skill, idle_sway};
	array<int> int_parameters = {knocked_out_shield};
	array<int> bool_parameters = {cannot_be_disarmed, left_handed, static_char, fear_always_afraid_on_sight, fear_causes_fear_on_sight, fear_never_afraid_on_sight, no_look_around, stick_to_nav_mesh, is_throw_trainer, wearing_metal_armor};
	array<int> function_parameters = {ignite, extinguish, is_player, kill, revive, limp_ragdoll, injured_ragdoll, ragdoll, cut_throat, apply_damage, wet, attach_item, sheathe_item};

	array<string> param_names = {	"Aggression",
	 								"Attack Damage",
									"Attack Knockback",
									"Attack Speed",
									"Block Follow-up",
									"Block Skill",
									"Cannot Be Disarmed",
									"Character Scale",
									"Damage Resistance",
									"Ear Size",
									"Fat",
									"Focus FOV distance",
									"Focus FOV horizontal",
									"Focus FOV vertical",
									"Ground Aggression",
									"Knockout Shield",
									"Left handed",
									"Movement Speed",
									"Muscle",
									"Peripheral FOV distance",
									"Peripheral FOV horizontal",
									"Peripheral FOV vertical",
									"Species",
									"Static",
									"Teams",
									"Fall Damage Multiplier",
									"Fear - Afraid At Health Level",
									"Fear - Always Afraid On Sight",
									"Fear - Causes Fear On Sight",
									"Fear - Never Afraid On Sight",
									"No Look Around",
									"Stick To Nav Mesh",
									"Throw Counter Probability",
									"Throw Trainer",
									"Weapon Catch Skill",
									"Wearing Metal Armor",
									"Ignite",
									"Extinguish",
									"Is Player",
									"Kill",
									"Revive",
									"Limp Ragdoll",
									"Injured Ragdoll",
									"Ragdoll",
									"Cut Throat",
									"Apply Damage",
									"Wet",
									"Attach Item",
									"Sheathe Item",
									"Idle Sway"
								};

	array<string> attachment_type_names = 	{	"At Grip",
												"At Sheathe",
												"At Attachment",
												"At Unspecified"
											};
											
	array<string> value_type_names = 		{	"Manual Input",
												"Variable"
											};

	DrikaCharacterControl(JSONValue params = JSONValue()){
		character_control_option = character_control_options(GetJSONInt(params, "character_option", 0));
		value_type_option = value_type_options(GetJSONInt(params, "value_type_option", 0));
		current_type = character_control_option;
		current_value_type = value_type_option;
		param_type = param_types(GetJSONInt(params, "param_type", 0));

		@target_select = DrikaTargetSelect(this, params);
		target_select.target_option = id_option | name_option | character_option | reference_option | team_option;

		@item_select = DrikaTargetSelect(this, params, "item_select");
		item_select.target_option = id_option | name_option | item_option | reference_option;

		recovery_time = GetJSONFloat(params, "recovery_time", 1.0);
		roll_recovery_time = GetJSONFloat(params, "roll_recovery_time", 0.2);
		damage_amount = GetJSONFloat(params, "damage_amount", 1.0);
		wet_amount = GetJSONFloat(params, "wet_amount", 1.0);
		attachment_type = GetJSONInt(params, "attachment_type", _at_grip);
		mirrored = GetJSONBool(params, "mirrored", false);
		
		variable_input_1 = GetJSONString(params, "variable_input_1", "");
		variable_input_2 = GetJSONString(params, "variable_input_2", "");

		connection_types = {_movement_object};
		drika_element_type = drika_character_control;
		has_settings = true;
		SetParamType();
		InterpParam(params);
		SetParamName();
	}

	void PostInit(){
		target_select.PostInit();
		item_select.PostInit();
	}

	JSONValue GetSaveData(){
		JSONValue data;
		data["character_option"] = JSONValue(character_control_option);
		data["value_type_option"] = JSONValue(value_type_option);
		data["param_type"] = JSONValue(param_type);
		if(value_type_option == variable){
			data["variable_input_1"] = JSONValue(variable_input_1);
			data["variable_input_2"] = JSONValue(variable_input_2);
		}
		if(param_type == int_param){
			data["param_after"] = JSONValue(int_param_after);
		}else if(param_type == float_param){
			data["param_after"] = JSONValue(float_param_after);
		}else if(param_type == bool_param){
			data["param_after"] = JSONValue(bool_param_after);
		}else if(param_type == string_param){
			data["param_after"] = JSONValue(string_param_after);
		}
		if(character_control_option == limp_ragdoll || character_control_option == injured_ragdoll || character_control_option == ragdoll){
			data["recovery_time"] = JSONValue(recovery_time);
			data["roll_recovery_time"] = JSONValue(roll_recovery_time);
		}else if(character_control_option == apply_damage){
			data["damage_amount"] = JSONValue(damage_amount);
		}else if(character_control_option == wet){
			data["wet_amount"] = JSONValue(wet_amount);
		}else if(character_control_option == attach_item){
			data["attachment_type"] = JSONValue(attachment_type);
			data["mirrored"] = JSONValue(mirrored);
			item_select.SaveIdentifier(data);
		}
		target_select.SaveIdentifier(data);
		return data;
	}

	void SetParamType(){
		if(string_parameters.find(character_control_option) != -1){
			param_type = string_param;
		}else if(float_parameters.find(character_control_option) != -1){
			param_type = float_param;
		}else if(int_parameters.find(character_control_option) != -1){
			param_type = int_param;
		}else if(bool_parameters.find(character_control_option) != -1){
			param_type = bool_param;
		}else if(function_parameters.find(character_control_option) != -1){
			param_type = function_param;
		}
	}

	void InterpParam(JSONValue _params){
		if(param_type == float_param){
			float_param_after = GetJSONFloat(_params, "param_after", 0.0);
		}else if(param_type == int_param){
			int_param_after = GetJSONInt(_params, "param_after", 0);
		}else if(param_type == bool_param){
			bool_param_after = GetJSONBool(_params, "param_after", false);
		}else if(param_type == string_param){
			string_param_after = GetJSONString(_params, "param_after", "");
		}else if(param_type == function_param){
			//This type doesn't have any parameters.
		}
	}

	void DrawEditing(){
		array<MovementObject@> targets = target_select.GetTargetMovementObjects();
		for(uint i = 0; i < targets.size(); i++){
			DebugDrawLine(targets[i].position, this_hotspot.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
			if(character_control_option == attach_item){
				array<Object@> target_items = item_select.GetTargetObjects();
				for(uint j = 0; j < target_items.size(); j++){
					if(target_items[j].GetType() == _item_object){
						ItemObject@ io = ReadItemID(target_items[j].GetID());
						DebugDrawLine(io.GetPhysicsPosition(), targets[i].position, vec3(0.0, 0.0, 1.0), _delete_on_draw);
					}else{
						DebugDrawLine(target_items[j].GetTranslation(), targets[i].position, vec3(0.0, 0.0, 1.0), _delete_on_draw);
					}
				}
			}
		}
	}

	void SetParamName(){
		param_name = param_names[character_control_option];
	}

	void GetBeforeParam(){
		//Use the Objects in stead of MovementObject so that the params are available.
		array<Object@> targets = target_select.GetTargetObjects();
		params_before.resize(0);

		for(uint i = 0; i < targets.size(); i++){
			ScriptParams@ params = targets[i].GetScriptParams();
			params_before.insertLast(BeforeValue());

			if(!params.HasParam(param_name)){
				params_before[i].delete_before = true;
				return;
			}else{
				params_before[i].delete_before = false;
			}

			if(param_type == string_param){
				if(!params.HasParam(param_name)){
					params.AddString(param_name, string_param_after);
				}
				params_before[i].string_value = params.GetString(param_name);
			}else if(param_type == float_param){
				if(!params.HasParam(param_name)){
					params.AddFloat(param_name, float_param_after);
				}
				params_before[i].float_value = params.GetFloat(param_name);
			}else if(param_type == int_param){
				if(!params.HasParam(param_name)){
					params.AddInt(param_name, int_param_after);
				}
				params_before[i].int_value = params.GetInt(param_name);
			}else if(param_type == bool_param){
				if(!params.HasParam(param_name)){
					params.AddIntCheckbox(param_name, bool_param_after);
				}
				params_before[i].bool_value = (params.GetInt(param_name) == 1);
			}else if(character_control_option == is_player){
				if(targets[i].GetType() == _movement_object){
					MovementObject@ char = ReadCharacterID(targets[i].GetID());
					params_before[i].bool_value = char.is_player;
				}
			}
		}
	}

	string GetDisplayString(){
		string display_string;
		if(current_value_type == variable){
			display_string = "[" + variable_input_1 + "]";
		}else{
			if(param_type == int_param){
				display_string = "" + int_param_after;
			}else if(param_type == float_param){
				display_string = "" + float_param_after;
			}else if(param_type == bool_param){
				display_string = bool_param_after?"true":"false";
			}else if(param_type == string_param){
				display_string = string_param_after;
			}else if(character_control_option == attach_item){
				display_string = item_select.GetTargetDisplayText();
			}
		}
		return "CharacterControl " + target_select.GetTargetDisplayText() + " " + param_name + " " + display_string;
	}

	void StartSettings(){
		target_select.CheckAvailableTargets();
		item_select.CheckAvailableTargets();
	}

	void DrawSettings(){

		float option_name_width = 150.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Target Character");
		ImGui_NextColumn();
		ImGui_NextColumn();
		target_select.DrawSelectTargetUI();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Param Type");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		ImGui_PushItemWidth(second_column_width);
		if(ImGui_Combo("##Param Type", current_type, param_names, 15)){
			character_control_option = character_control_options(current_type);
			SetParamType();
			SetParamName();
			if(IsValidOption(character_control_option) == false){
				current_value_type = manual_input;
			}
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();
		
		if(IsValidOption(character_control_option) == true){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Value Type");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_Combo("##Value Type", current_value_type, value_type_names, 2)){
				value_type_option = value_type_options(current_value_type);
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}

		if(current_value_type == manual_input){
			switch(character_control_option){
				case aggression:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat(param_name, float_param_after, 0.0, 100.0, "%.2f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case attack_damage:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();
					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat(param_name, float_param_after, 0.0, 200.0, "%.1f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case attack_knockback:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat(param_name, float_param_after, 0.0, 200.0, "%.1f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case attack_speed:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat(param_name, float_param_after, 0.0, 200.0, "%.1f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case block_followup:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat(param_name, float_param_after, 0.0, 100.0, "%.1f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case block_skill:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat(param_name, float_param_after, 0.0, 100.0, "%.1f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case cannot_be_disarmed:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_Checkbox("###" + param_name, bool_param_after);
					ImGui_NextColumn();
					break;
				case character_scale:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat(param_name, float_param_after, 60, 140, "%.2f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case damage_resistance:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat(param_name, float_param_after, 0.0, 200.0, "%.1f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case ear_size:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat(param_name, float_param_after, 0.0, 300.0, "%.1f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case fat:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat(param_name, float_param_after, 0.0, 200.0, "%.3f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case focus_fov_distance:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat(param_name, float_param_after, 0.0, 100.0, "%.1f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case focus_fov_horizontal:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat(param_name, float_param_after, 0.573, 90.0, "%.2f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case focus_fov_vertical:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat(param_name, float_param_after, 0.573, 90.0, "%.2f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case ground_aggression:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat(param_name, float_param_after, 0.0, 100.0, "%.2f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case knocked_out_shield:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderInt(param_name, int_param_after, 0, 10);
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case left_handed:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_Checkbox("###" + param_name, bool_param_after);
					break;
				case movement_speed:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat(param_name, float_param_after, 10.0, 150.0, "%.1f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case muscle:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat(param_name, float_param_after, 0.0, 200.0, "%.3f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case peripheral_fov_distance:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat(param_name, float_param_after, 0.0, 100.0, "%.1f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case peripheral_fov_horizontal:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat(param_name, float_param_after, 0.573, 90.0, "%.2f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case peripheral_fov_vertical:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat(param_name, float_param_after, 0.573, 90.0, "%.2f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case species:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_PushItemWidth(second_column_width);
					ImGui_InputText(param_name, string_param_after, 64);
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case static_char:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_Checkbox("###" + param_name, bool_param_after);
					ImGui_NextColumn();
					break;
				case teams:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_PushItemWidth(second_column_width);
					ImGui_InputText(param_name, string_param_after, 64);
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case fall_damage_mult:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat(param_name, float_param_after, 0.0, 10.0, "%.1f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case fear_afraid_at_health_level:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat(param_name, float_param_after, 0.0, 100.0, "%.2f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case fear_always_afraid_on_sight:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_Checkbox("###" + param_name, bool_param_after);
					break;
				case fear_causes_fear_on_sight:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_Checkbox("###" + param_name, bool_param_after);
					break;
				case fear_never_afraid_on_sight:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_Checkbox("###" + param_name, bool_param_after);
					break;
				case no_look_around:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_Checkbox("###" + param_name, bool_param_after);
					break;
				case stick_to_nav_mesh:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_Checkbox("###" + param_name, bool_param_after);
					break;
				case throw_counter_probability:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat(param_name, float_param_after, 0.0, 100.0, "%.1f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case is_throw_trainer:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_Checkbox("###" + param_name, bool_param_after);
					break;
				case weapon_catch_skill:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat(param_name, float_param_after, 0.0, 100.0, "%.1f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case wearing_metal_armor:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_Checkbox("###" + param_name, bool_param_after);
					break;
				case ignite:
					break;
				case extinguish:
					break;
				case is_player:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_Checkbox("###" + param_name, bool_param_after);
					break;
				case kill:
					break;
				case revive:
					break;
				case limp_ragdoll:
					ImGui_AlignTextToFramePadding();
					ImGui_Text("Recovery Time");
					ImGui_NextColumn();
					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat("##Recovery Time", recovery_time, 0.0, 10.0, "%.1f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();

					ImGui_AlignTextToFramePadding();
					ImGui_Text("Roll Recovery Time");
					ImGui_NextColumn();
					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat("##Roll Recovery Time", roll_recovery_time, 0.0, 10.0, "%.1f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case injured_ragdoll:
					ImGui_AlignTextToFramePadding();
					ImGui_Text("Recovery Time");
					ImGui_NextColumn();
					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat("##Recovery Time", recovery_time, 0.0, 10.0, "%.1f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();

					ImGui_AlignTextToFramePadding();
					ImGui_Text("Roll Recovery Time");
					ImGui_NextColumn();
					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat("##Roll Recovery Time", roll_recovery_time, 0.0, 10.0, "%.1f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case ragdoll:
					ImGui_AlignTextToFramePadding();
					ImGui_Text("Recovery Time");
					ImGui_NextColumn();
					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat("##Recovery Time", recovery_time, 0.0, 10.0, "%.1f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();

					ImGui_AlignTextToFramePadding();
					ImGui_Text("Roll Recovery Time");
					ImGui_NextColumn();
					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat("##Roll Recovery Time", roll_recovery_time, 0.0, 10.0, "%.1f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case apply_damage:
					ImGui_AlignTextToFramePadding();
					ImGui_Text("Amount");
					ImGui_NextColumn();
					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat("##Amount", damage_amount, 0.0, 2.0, "%.1f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case cut_throat:
					break;
				case wet:
					ImGui_AlignTextToFramePadding();
					ImGui_Text("Amount");
					ImGui_NextColumn();
					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat("##Amount", wet_amount, 0.0, 1.0, "%.1f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case idle_sway:
					ImGui_AlignTextToFramePadding();
					ImGui_Text(param_name);
					ImGui_NextColumn();

					ImGui_PushItemWidth(second_column_width);
					ImGui_SliderFloat(param_name, float_param_after, 0.0, 100.0, "%.2f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case attach_item:
					ImGui_AlignTextToFramePadding();
					ImGui_Text("Target Item");
					ImGui_NextColumn();
					ImGui_NextColumn();
					item_select.DrawSelectTargetUI();

					ImGui_AlignTextToFramePadding();
					ImGui_Text("Attachment Type");
					ImGui_NextColumn();
					ImGui_PushItemWidth(second_column_width);
					if(ImGui_Combo("##Attachment Type", attachment_type, attachment_type_names, attachment_type_names.size())){

					}
					ImGui_PopItemWidth();
					ImGui_NextColumn();

					ImGui_AlignTextToFramePadding();
					ImGui_Text("Mirrored");
					ImGui_NextColumn();
					ImGui_PushItemWidth(second_column_width);
					ImGui_Checkbox("##Mirrored", mirrored);
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				default:
					Log(warning, "Found a non standard parameter type. " + param_type);
					break;
			}
		}else if 	(current_value_type == variable){
			bool var_is_valid;
			switch(character_control_option){
				case aggression:
				case attack_damage:
				case attack_knockback:
				case attack_speed:
				case block_followup:
				case block_skill:
				case character_scale:
				case damage_resistance:
				case ear_size:
				case fat:
				case focus_fov_distance:
				case focus_fov_horizontal:
				case focus_fov_vertical:
				case ground_aggression:
				case movement_speed:
				case muscle:
				case peripheral_fov_distance:
				case peripheral_fov_horizontal:
				case peripheral_fov_vertical:
				case fall_damage_mult:
				case fear_afraid_at_health_level:
				case throw_counter_probability:
				case weapon_catch_skill:
				case cannot_be_disarmed:
				case left_handed:
				case static_char:
				case fear_always_afraid_on_sight:
				case fear_causes_fear_on_sight:
				case fear_never_afraid_on_sight:
				case no_look_around:
				case stick_to_nav_mesh:
				case is_throw_trainer:
				case wearing_metal_armor:
				case is_player:
				case knocked_out_shield:
				case species:
				case teams:
					ImGui_AlignTextToFramePadding();
					ImGui_Text("Input Variable:");
					ImGui_NextColumn();
					ImGui_PushItemWidth(second_column_width);
					if(ImGui_InputText("##Variable1", variable_input_1, 64)){
						if(IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
					}
					
					IsValidParam(variable_input_1) == true ? var_is_valid = true : var_is_valid = false;
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					if (var_is_valid == false){
						ImGui_NextColumn();
						ImGui_PushItemWidth(second_column_width);
						ImGui_Text("Variable does not exist or is the wrong type.");
						ImGui_PopItemWidth();
						ImGui_NextColumn();
					}
					break;
					
				case limp_ragdoll:
				case injured_ragdoll:
				case ragdoll:
					ImGui_AlignTextToFramePadding();
					ImGui_Text("Recovery Var:");
					ImGui_NextColumn();
					ImGui_PushItemWidth(second_column_width);
					if(ImGui_InputText("##Variable1", variable_input_1, 64)){
						if(IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"recovery_time");
						}
					}
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					ImGui_AlignTextToFramePadding();
					ImGui_Text("Roll Recovery Var:");
					ImGui_NextColumn();
					ImGui_PushItemWidth(second_column_width);
					if(ImGui_InputText("##Variable2", variable_input_2, 64)){
						if(IsValidParam(variable_input_2) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"roll_recovery_time");
						}
					}
					IsValidParam(variable_input_1) == true && IsValidParam(variable_input_2) == true ? var_is_valid = true : var_is_valid = false;
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					if (var_is_valid == false){
						ImGui_NextColumn();
						ImGui_PushItemWidth(second_column_width);
						ImGui_Text("One or more variables do not exist or are the wrong type.");
						ImGui_PopItemWidth();
						ImGui_NextColumn();
					}
					break;
				
				case apply_damage:
					ImGui_AlignTextToFramePadding();
					ImGui_Text("Input Variable:");
					ImGui_NextColumn();
					ImGui_PushItemWidth(second_column_width);
					if(ImGui_InputText("##Variable1", variable_input_1, 64)){
						if(IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"damage_amount");
						}
					}
					IsValidParam(variable_input_1) == true ? var_is_valid = true : var_is_valid = false;
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case wet:
					ImGui_AlignTextToFramePadding();
					ImGui_Text("Input Variable:");
					ImGui_NextColumn();
					ImGui_PushItemWidth(second_column_width);
					if(ImGui_InputText("##Variable1", variable_input_1, 64)){
						if(IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"wet_amount");
						}
					}
					IsValidParam(variable_input_1) == true ? var_is_valid = true : var_is_valid = false;
					ImGui_PopItemWidth();
					ImGui_NextColumn();
					break;
				case ignite:
				case extinguish:
				case kill:
				case revive:
				case cut_throat:
				case idle_sway:
				case attach_item:
					ImGui_Text("Variable Input is not supported for this setting.");
					break;
					
				default:
					Log(warning, "Found a non standard parameter type. " + param_type);
					break;
			}
		}
	}
	
	bool IsValidOption(int input){
		if(input == ignite){return false;}
		if(input == extinguish){return false;}
		if(input == kill){return false;}
		if(input == revive){return false;}
		if(input == cut_throat){return false;}
		if(input == attach_item){return false;}
		if(input == sheathe_item){return false;}
		return true;
	}
	
	bool IsValidParam(string input){
		if		(CheckParamExists(input) == false){return false;}
		else if(param_type == bool_param && IsBool(ReadParamValue(input)) == false){return false;}
		else if((param_type == float_param || param_type == int_param) && IsFloat(ReadParamValue(input)) == false){return false;}
		return true;
	}
	
	void SetParamFromVariable(string var1, string var2, string param_input){
		string input = ReadParamValue(var1);
		string input2 = ReadParamValue(var2);
		
		if (param_input == "recovery_time"){
			IsFloat(input) ? recovery_time = atof(input) : recovery_time = 0.0f;
		}else if(param_input == "roll_recovery_time"){
			IsFloat(input2) ? roll_recovery_time = atof(input2) : roll_recovery_time = 0.0f;
		}else if(param_input == "damage_amount"){
			IsFloat(input) ? damage_amount = atof(input) : damage_amount = 0.0f;
		}else if(param_input == "wet_amount"){
			IsFloat(input) ? wet_amount = atof(input) : wet_amount = 0.0f;
		}else{
			switch(param_type){
				case float_param:
					IsFloat(input) ? float_param_after = atof(input) : float_param_after = 0.0f;
					break;
				case int_param:
					IsFloat(input) ? int_param_after = atoi(input) : int_param_after = 0;
					break;
				case bool_param:
					IsBool(input) ? bool_param_after = ReturnBool(input) : bool_param_after = false;
					break;
				case string_param:
					string_param_after = input;
					break;
				
				default:
					break;
			};
		}
	}
	
	bool ReturnBool(string input){
		return input == "true";
	}
	
	string ReadParamValue(string key){
		SavedLevel@ data = save_file.GetSavedLevel("drika_data");
		return data.GetValue(key);
	}

	bool CheckParamExists(string key){
		return ReadParamValue("[" + key + "]") == "true";
	}
	
	bool IsFloat(string test) {
		bool decimal_found = false;

		// Checking for certain edge cases
		if (test == "." || test == "-" || test == "-.") return false;
			for (uint i = 0; i < test.length(); i++)
			{
				if (test[i] == "."[0])
					{if (decimal_found) {return false;} else {decimal_found = true;} }

				else if (test[i] == "-"[0])
					{if (i > 0) {return false;} }

				else if (test[i] < "0"[0] || test[i] > "9"[0]) {return false;}
			}

		return true;
	}
		
	bool IsBool(string test) {
		if (test == "true" || test == "false") {return true;} else {return false;}
	}
		
	bool Trigger(){
		if(!triggered){
			GetBeforeParam();
		}
		triggered = true;
		return SetParameter(false);
	}

	bool SetParameter(bool reset){
		//Use the Objects in stead of MovementObject so that the params are available.
		array<Object@> targets;

		if(!reset){
			targets = target_select.GetTargetObjects();
			target_cache = targets;
		}else{
			targets = target_cache;
		}

		if(targets.size() == 0){return false;}
		for(uint i = 0; i < targets.size(); i++){
			ScriptParams@ params = targets[i].GetScriptParams();
			MovementObject@ char = ReadCharacterID(targets[i].GetID());

			if(reset && params_before[i].delete_before && param_type != function_param){
				params.Remove(param_name);
				return true;
			}

			if(!params.HasParam(param_name) && param_type != function_param){
				if(param_type == string_param){
					params.AddString(param_name, reset?params_before[i].string_value:string_param_after);
				}else if(param_type == int_param){
					params.AddInt(param_name, reset?params_before[i].int_value:int_param_after);
				}else if(param_type == float_param){
					params.AddFloatSlider(param_name, reset?params_before[i].float_value:float_param_after, "min:0,max:1000,step:0.0001,text_mult:1");
				}else if(param_type == bool_param){
					params.AddIntCheckbox(param_name, reset?params_before[i].bool_value:bool_param_after);
				}
			}else{
				switch(character_control_option){
					case aggression:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetFloat(param_name, reset?params_before[i].float_value:float_param_after / 100.0);
						break;
					case idle_sway:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetFloat(param_name, reset?params_before[i].float_value:float_param_after / 100.0);
						break;
					case attack_damage:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetFloat(param_name, reset?params_before[i].float_value:float_param_after / 100.0);
						break;
					case attack_knockback:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetFloat(param_name, reset?params_before[i].float_value:float_param_after / 100.0);
						break;
					case attack_speed:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetFloat(param_name, reset?params_before[i].float_value:float_param_after / 100.0);
						break;
					case block_followup:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetFloat(param_name, reset?params_before[i].float_value:float_param_after / 100.0);
						break;
					case block_skill:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetFloat(param_name, reset?params_before[i].float_value:float_param_after / 100.0);
						break;
					case cannot_be_disarmed:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetInt(param_name, (reset?params_before[i].bool_value:bool_param_after)?1:0);
						break;
					case character_scale:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetFloat(param_name, reset?params_before[i].float_value:float_param_after / 100.0);
						break;
					case damage_resistance:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetFloat(param_name, reset?params_before[i].float_value:float_param_after / 100.0);
						break;
					case ear_size:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetFloat(param_name, reset?params_before[i].float_value:float_param_after / 100.0);
						break;
					case fat:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetFloat(param_name, reset?params_before[i].float_value:float_param_after / 200.0);
						break;
					case focus_fov_distance:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetFloat(param_name, reset?params_before[i].float_value:float_param_after);
						break;
					case focus_fov_horizontal:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetFloat(param_name, reset?params_before[i].float_value:float_param_after / 57.2957);
						break;
					case focus_fov_vertical:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetFloat(param_name, reset?params_before[i].float_value:float_param_after / 57.2957);
						break;
					case ground_aggression:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetFloat(param_name, reset?params_before[i].float_value:float_param_after / 100.0);
						break;
					case knocked_out_shield:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetInt(param_name, reset?params_before[i].int_value:int_param_after);
						break;
					case left_handed:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetInt(param_name, (reset?params_before[i].bool_value:bool_param_after)?1:0);
						break;
					case movement_speed:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetFloat(param_name, reset?params_before[i].float_value:float_param_after / 100.0);
						break;
					case muscle:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetFloat(param_name, reset?params_before[i].float_value:float_param_after / 200.0);
						break;
					case peripheral_fov_distance:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetFloat(param_name, reset?params_before[i].float_value:float_param_after);
						break;
					case peripheral_fov_horizontal:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetFloat(param_name, reset?params_before[i].float_value:float_param_after / 57.2957);
						break;
					case peripheral_fov_vertical:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetFloat(param_name, reset?params_before[i].float_value:float_param_after / 57.2957);
						break;
					case species:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetString(param_name, reset?params_before[i].string_value:string_param_after);
						break;
					case static_char:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetInt(param_name, (reset?params_before[i].bool_value:bool_param_after)?1:0);
						break;
					case teams:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetString(param_name, reset?params_before[i].string_value:string_param_after);
						break;
					case fall_damage_mult:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetFloat(param_name, reset?params_before[i].float_value:float_param_after);
						break;
					case fear_afraid_at_health_level:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetFloat(param_name, reset?params_before[i].float_value:float_param_after / 100.0);
						break;
					case fear_always_afraid_on_sight:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetInt(param_name, (reset?params_before[i].bool_value:bool_param_after)?1:0);
						break;
					case fear_causes_fear_on_sight:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetInt(param_name, (reset?params_before[i].bool_value:bool_param_after)?1:0);
						break;
					case fear_never_afraid_on_sight:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetInt(param_name, (reset?params_before[i].bool_value:bool_param_after)?1:0);
						break;
					case no_look_around:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetInt(param_name, (reset?params_before[i].bool_value:bool_param_after)?1:0);
						break;
					case stick_to_nav_mesh:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetInt(param_name, (reset?params_before[i].bool_value:bool_param_after)?1:0);
						break;
					case throw_counter_probability:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetFloat(param_name, reset?params_before[i].float_value:float_param_after / 100.0);
						break;
					case is_throw_trainer:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetInt(param_name, (reset?params_before[i].bool_value:bool_param_after)?1:0);
						break;
					case weapon_catch_skill:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetFloat(param_name, reset?params_before[i].float_value:float_param_after / 100.0);
						break;
					case wearing_metal_armor:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						params.SetInt(param_name, (reset?params_before[i].bool_value:bool_param_after)?1:0);
						break;
					case ignite:
						if(targets[i].GetType() == _movement_object){
							if(!reset){
								char.ReceiveMessage("ignite");
							}else{
								char.ReceiveMessage("extinguish");
							}
						}
						break;
					case extinguish:
						if(!reset){
							if(targets[i].GetType() == _movement_object){
								char.ReceiveMessage("extinguish");
							}
						}
						break;
					case is_player:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"param_after");
						}
						if(targets[i].GetType() == _movement_object){
							char.is_player = reset?params_before[i].bool_value:bool_param_after;
						}
						break;
					case kill:
						if(!reset){
							char.Execute("temp_health = -1.0;permanent_health = -1.0;blood_health = -1.0;SetKnockedOut(_dead);CharacterDefeated();Ragdoll(_RGDL_LIMP);");
						}
						break;
					case revive:
						if(!reset){
							char.QueueScriptMessage("full_revive");
						}
						break;
					case limp_ragdoll:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true && IsValidParam(variable_input_2) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"recovery_time");
							SetParamFromVariable(variable_input_1,variable_input_2,"roll_recovery_time");
						}
						if(!reset){
							char.Execute("Ragdoll(_RGDL_LIMP);recovery_time = " + recovery_time + ";roll_recovery_time = " + roll_recovery_time + ";");
						}
						break;
					case injured_ragdoll:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true && IsValidParam(variable_input_2) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"recovery_time");
							SetParamFromVariable(variable_input_1,variable_input_2,"roll_recovery_time");
						}
						if(!reset){
							char.Execute("if(state != _ragdoll_state){string sound = \"Data/Sounds/hit/hit_hard.xml\";PlaySoundGroup(sound, this_mo.position);}Ragdoll(_RGDL_INJURED);recovery_time = " + recovery_time + ";roll_recovery_time = " + roll_recovery_time + ";");
						}
						break;
					case ragdoll:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true && IsValidParam(variable_input_2) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"recovery_time");
							SetParamFromVariable(variable_input_1,variable_input_2,"roll_recovery_time");
						}
						if(!reset){
							char.Execute("GoLimp();recovery_time = " + recovery_time + ";roll_recovery_time = " + roll_recovery_time + ";");
						}
						break;
					case cut_throat:
						if(!reset){
							char.Execute("CutThroat();");
						}
						break;
					case apply_damage:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"damage_amount");
						}
						if(!reset){
							char.Execute("TakeBloodDamage(" + damage_amount + ");if(knocked_out != _awake){Ragdoll(_RGDL_INJURED);}");
						}
						break;
					case wet:
						if(current_value_type == variable && IsValidParam(variable_input_1) == true){
							SetParamFromVariable(variable_input_1,variable_input_2,"wet_amount");
						}
						{
							char.rigged_object().SetWet(reset?0.0:wet_amount);
						}
						break;
					case attach_item:
						{
							array<Object@> target_items = item_select.GetTargetObjects();
							for(uint j = 0; j < target_items.size(); j++){
								if(target_items[j].GetType() != _item_object){return false;}
								int item_id = target_items[j].GetID();
								ItemObject@ io = ReadItemID(item_id);
								if(reset){
									char.Execute("this_mo.DetachItem(" + item_id + ");NotifyItemDetach(" + item_id + ");");
								}else{
									//Make sure the item is not held.
									if(io.IsHeld()) {
					                    int holder_id = io.HeldByWhom();
					                    MovementObject@ holder = ReadCharacterID(holder_id);
				                        holder.Execute("this_mo.DetachItem(" + item_id + ");NotifyItemDetach(" + item_id + ");");
					                }

									//Make sure the item is not stuck inside a body.
					                if(io.StuckInWhom() != -1) {
					                    int holder_id = io.StuckInWhom();
					                    MovementObject@ holder = ReadCharacterID(holder_id);
										holder.Execute("this_mo.DetachItem(" + item_id + ");NotifyItemDetach(" + item_id + ");");
					                }

									//Make sure the target slot is not occupied.
									int weap_slot = -1;
									if(attachment_type == _at_grip) {
								        if(mirrored) {
								            weap_slot = _held_left;
								        } else {
								            weap_slot = _held_right;
								        }
								    } else if(attachment_type == _at_sheathe) {
								        if(mirrored) {
								            weap_slot = _sheathed_right;
								        } else {
								            weap_slot = _sheathed_left;
								        }
								    }

									char.Execute(	"int weapon = weapon_slots[" + weap_slot + "]; " +
													"if(weapon != -1) {this_mo.DetachItem(weapon);NotifyItemDetach(weapon);}");

									//This method attaches the item permanently, with connection lines in the editor.
									/* targets[i].AttachItem(target_items[j], AttachmentType(attachment_type), mirrored); */

									//Do not try to attach an item at the sheathe if it can't. This will crash the game.
									if(attachment_type == _at_sheathe && !io.HasSheatheAttachment()){
										continue;
									}

					                char.Execute(	"this_mo.AttachItemToSlot(" + target_items[j].GetID() + ", " + attachment_type + ", " + mirrored + ");" +
					                				"HandleEditorAttachment(" + target_items[j].GetID() + ", " + attachment_type + ", " + mirrored + ");");
								}
							}
						}
						break;
					case sheathe_item:
						{
							if(!reset){
								string command = "if(weapon_slots[primary_weapon_slot] != -1){StartSheathing(primary_weapon_slot);}";
								char.Execute(command);
							}
						}
						break;
					default:
						Log(warning, "Found a non standard parameter type. " + param_type);
						break;
				}
			}

			//To make sure the parameters are being used, refresh them in aschar.
			if(targets[i].GetType() == _movement_object){
				char.Execute("SetParameters();");
			}
		}
		return true;
	}

	void Reset(){
		if(triggered){
			triggered = false;
			SetParameter(true);
		}
	}

	void Delete(){
		target_select.Delete();
	}
}
