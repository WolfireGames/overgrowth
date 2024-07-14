#include "drika_shared.as";

enum dialogue_functions	{
							say = 0,
							actor_settings = 1,
							set_actor_position = 2,
							set_actor_animation = 3,
							set_actor_eye_direction = 4,
							set_actor_torso_direction = 5,
							set_actor_head_direction = 6,
							set_actor_omniscient = 7,
							set_camera_position = 8,
							fade_to_black = 9,
							settings = 10,
							start = 11,
							end = 12,
							set_actor_dialogue_control = 13,
							choice = 14,
							clear_dialogue = 15
						}

enum camera_transitions	{
							no_transition = 0,
							move_transition = 1,
							fade_transition = 2
						}

class DrikaDialogue : DrikaElement{

	dialogue_functions dialogue_function;
	int current_dialogue_function;
	string say_text;
	array<string> say_text_split;
	string voice_over_path;
	int voice_over_sound_id = -1;
	bool say_started = false;
	float say_timer = 0.0;
	bool auto_continue;
	float wait_timer = 0.0;
	int actor_id;
	string actor_name;
	vec4 dialogue_color;
	bool dialogue_done = false;
	int voice;
	vec3 target_actor_position;
	float target_actor_rotation;
	string target_actor_animation;
	string search_buffer = "";
	vec3 target_actor_eye_direction;
	float target_blink_multiplier;
	vec3 target_actor_torso_direction;
	float target_actor_torso_direction_weight;
	vec3 target_actor_head_direction;
	float target_actor_head_direction_weight;
	bool omniscient;
	vec3 target_camera_position;
	vec3 target_camera_rotation;
	float target_camera_zoom;
	float target_fade_to_black;
	float fade_to_black_duration;
	bool wait_for_fade = false;
	bool skip_move_transition = false;
	bool update_animation_list_scroll;

	int dialogue_layout;
	string dialogue_text_font;
	int dialogue_text_size;
	vec4 dialogue_text_color;
	bool dialogue_text_shadow;
	bool use_voice_sounds;
	bool show_names;
	bool show_avatar;
	bool use_fade;
	int dialogue_location;
	float dialogue_text_speed;

	string default_avatar_path = "Data/Textures/ui/menus/main/white_square.png";
	TextureAssetRef avatar = LoadTexture(default_avatar_path, TextureLoadFlags_NoMipmap | TextureLoadFlags_NoConvert |TextureLoadFlags_NoReduce);
	string avatar_path;
	bool anim_mirrored;
	bool anim_mobile;
	bool anim_super_mobile;
	bool anim_from_start;
	bool use_ik;
	bool wait_anim_end;
	bool custom_animation;
	bool dialogue_control;
	int current_choice;
	int nr_choices;
	int max_choices = 10;
	array<string> choice_texts(max_choices);
	array<int> choice_go_to_lines(max_choices);
	array<DrikaGoToLineSelect@> choice_elements(max_choices);
	array<bool> check_parameters(max_choices);
	array<string> parameter_names(max_choices);
	array<string> parameter_check_values(max_choices);
	array<check_modes> parameter_check_modes(max_choices);
	array<int> available_choices(max_choices);
	bool choice_ui_added = false;
	bool ui_sounds;
	string hover_sound_path;
	string click_sound_path;
	array<float> dof_settings;
	bool update_dof = false;
	bool enable_look_at_target;
	bool enable_move_with_target;
	DrikaTargetSelect@ look_at_target;
	DrikaTargetSelect@ move_with_target;
	camera_transitions camera_transition;
	int current_camera_transition;
	vec3 camera_translation_from;
	vec3 camera_rotation_from;
	float camera_transition_timer = 0.0;
	array<DialogueScriptEntry@> dialogue_script;
	float say_changed_timer;
	bool add_camera_shake;
	float position_shake_max_distance;
	float position_shake_slerp_speed;
	float position_shake_interval;
	float rotation_shake_max_distance;
	float rotation_shake_slerp_speed;
	float rotation_shake_interval;
	bool set_keyboard_focus = false;

	array<string> dialogue_function_names =	{
												"Say",
												"Actor Settings",
												"Set Actor Position",
												"Set Actor Animation",
												"Set Actor Eye Direction",
												"Set Actor Torso Direction",
												"Set Actor Head Direction",
												"Set Actor Omniscient",
												"Set Camera Position",
												"Fade To Black",
												"Settings",
												"Start",
												"End",
												"Set Actor Dialogue Control",
												"Choice",
												"Clear Dialogue"
											};

	array<string> camera_transition_names =	{
												"None",
												"Move Transition",
												"Fade Transition"
											};

	DrikaDialogue(JSONValue params = JSONValue()){
		dialogue_function = dialogue_functions(GetJSONInt(params, "dialogue_function", start));
		current_dialogue_function = dialogue_function;

		placeholder.default_scale = vec3(1.0);
		placeholder.Load(params);

		say_text = GetJSONString(params, "say_text", "Drika Hotspot Dialogue");
		voice_over_path = GetJSONString(params, "voice_over_path", "");
		dialogue_color = GetJSONVec4(params, "dialogue_color", vec4(1));
		voice = GetJSONInt(params, "voice", 0);
		avatar_path = GetJSONString(params, "avatar_path", "None");
		auto_continue = GetJSONBool(params, "auto_continue", false);
		if(avatar_path != "None"){
			avatar = LoadTexture(avatar_path, TextureLoadFlags_NoMipmap | TextureLoadFlags_NoConvert |TextureLoadFlags_NoReduce);
		}
		target_actor_position = GetJSONVec3(params, "target_actor_position", vec3(0.0));
		target_actor_rotation = GetJSONFloat(params, "target_actor_rotation", 0.0);
		target_actor_animation = GetJSONString(params, "target_actor_animation", "Data/Animations/r_dialogue_2handneck.anm");
		omniscient = GetJSONBool(params, "omniscient", true);
		target_camera_position = GetJSONVec3(params, "target_camera_position", vec3(0.0));
		target_camera_rotation = GetJSONVec3(params, "target_camera_rotation", vec3(0.0));
		target_camera_zoom = GetJSONFloat(params, "target_camera_zoom", 90.0);
		target_fade_to_black = GetJSONFloat(params, "target_fade_to_black", 1.0);
		fade_to_black_duration = GetJSONFloat(params, "fade_to_black_duration", 1.0);
		camera_transition = camera_transitions(GetJSONInt(params, "camera_transition", no_transition));
		current_camera_transition = camera_transition;

		// These values are no longer used, but still need to be loaded to maintain backwards compatibility.
		target_actor_eye_direction = GetJSONVec3(params, "target_actor_eye_direction", vec3(0.0));
		target_blink_multiplier = GetJSONFloat(params, "target_blink_multiplier", 1.0);
		target_actor_torso_direction = GetJSONVec3(params, "target_actor_torso_direction", vec3(0.0));
		target_actor_torso_direction_weight = GetJSONFloat(params, "target_actor_torso_direction_weight", 1.0);
		target_actor_head_direction = GetJSONVec3(params, "target_actor_head_direction", vec3(0.0));
		target_actor_head_direction_weight = GetJSONFloat(params, "target_actor_head_direction_weight", 1.0);

		dialogue_layout = GetJSONInt(params, "dialogue_layout", 0);
		dialogue_text_font = GetJSONString(params, "dialogue_text_font", "Data/Fonts/arial.ttf");
		dialogue_text_size = GetJSONInt(params, "dialogue_text_size", 50);
		dialogue_text_color = GetJSONVec4(params, "dialogue_text_color", vec4(1));
		dialogue_text_shadow = GetJSONBool(params, "dialogue_text_shadow", true);
		use_voice_sounds = GetJSONBool(params, "use_voice_sounds", true);
		show_names = GetJSONBool(params, "show_names", true);
		show_avatar = GetJSONBool(params, "show_avatar", true);
		use_fade = GetJSONBool(params, "use_fade", true);
		dialogue_location = GetJSONInt(params, "dialogue_location", dialogue_bottom);
		dialogue_text_speed = GetJSONFloat(params, "dialogue_text_speed", 50.0);

		anim_mirrored = GetJSONBool(params, "anim_mirrored", false);
		anim_mobile = GetJSONBool(params, "anim_mobile", false);
		anim_super_mobile = GetJSONBool(params, "anim_super_mobile", false);
		anim_from_start = GetJSONBool(params, "anim_from_start", true);
		use_ik = GetJSONBool(params, "use_ik", true);
		wait_anim_end = GetJSONBool(params, "wait_anim_end", false);
		custom_animation = GetJSONBool(params, "custom_animation", false);
		if(custom_animation){
			level.SendMessage("drika_dialogue_add_custom_animation " + target_actor_animation);
		}
		dialogue_control = GetJSONBool(params, "dialogue_control", true);
		nr_choices = GetJSONInt(params, "nr_choices", 4);
		ui_sounds = GetJSONBool(params, "ui_sounds", false);
		hover_sound_path = GetJSONString(params, "hover_sound_path", "Data/Sounds/ui_hover.xml");
		click_sound_path = GetJSONString(params, "click_sound_path", "Data/Sounds/ui_click.xml");

		for(int i = 0; i < max_choices; i++){
			int number = i + 1;
			choice_texts[i] = GetJSONString(params, "choice_" + number, "Pick choice nr " + number);
			@choice_elements[i] = DrikaGoToLineSelect("choice_" + number + "_go_to_line", params);
			
			check_parameters[i] = GetJSONBool(params, "check_parameter_" + number, false);
			parameter_names[i] = GetJSONString(params, "check_parameter_name_" + number, "Task Done");
			parameter_check_values[i] = GetJSONString(params, "check_parameter_value_" + number, "true");
			parameter_check_modes[i] = check_modes(GetJSONInt(params, "check_parameter_mode_" + number, check_mode_equals));
		}

		dof_settings = GetJSONFloatArray(params, "dof_settings", {0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
		add_camera_shake = GetJSONBool(params, "add_camera_shake", false);
		position_shake_max_distance = GetJSONFloat(params, "position_shake_max_distance", 0.2f);
		position_shake_slerp_speed = GetJSONFloat(params, "position_shake_slerp_speed", 0.2f);
		position_shake_interval = GetJSONFloat(params, "position_shake_interval", 0.1f);

		rotation_shake_max_distance = GetJSONFloat(params, "rotation_shake_max_distance", 10.0f);
		rotation_shake_slerp_speed = GetJSONFloat(params, "rotation_shake_slerp_speed", 0.2f);
		rotation_shake_interval = GetJSONFloat(params, "rotation_shake_interval", 0.15f);

		if(GetJSONValueAvailable(params, "identifier_track_target")){
			@look_at_target = DrikaTargetSelect(this, params, "track_target");
			@move_with_target = DrikaTargetSelect(this, params, "track_target");
		}else{
			@look_at_target = DrikaTargetSelect(this, params, "look_at_target");
			@move_with_target = DrikaTargetSelect(this, params, "move_with_target");
		}
		enable_look_at_target = GetJSONBool(params, "enable_look_at_target", false);
		enable_move_with_target = GetJSONBool(params, "enable_move_with_target", false);
		look_at_target.target_option = id_option | name_option | character_option | reference_option | team_option;
		move_with_target.target_option = id_option | name_option | character_option | reference_option | team_option;

		drika_element_type = drika_dialogue;
		has_settings = true;

		if(dialogue_function == say || dialogue_function == actor_settings || dialogue_function == set_actor_position || dialogue_function == set_actor_animation || dialogue_function == set_actor_eye_direction || dialogue_function == set_actor_torso_direction || dialogue_function == set_actor_head_direction || dialogue_function == set_actor_omniscient || dialogue_function == set_actor_dialogue_control){
			connection_types = {_movement_object};
		}
		@target_select = DrikaTargetSelect(this, params);
		target_select.target_option = id_option | name_option | character_option | reference_option | team_option;
	}

	void PostInit(){
		UpdateActorName();

		for(int i = 0; i < max_choices; i++){
			choice_elements[i].PostInit();
		}

		target_select.PostInit();
		look_at_target.PostInit();
		move_with_target.PostInit();

		// These functions use placeholders that weren't saved to the level xml at first. So convert that data into placeholder transforms.
		if(dialogue_function == set_actor_torso_direction || dialogue_function == set_actor_head_direction || dialogue_function == set_actor_eye_direction){
			bool no_placeholder = !placeholder.Exists();

			if(dialogue_function == set_actor_torso_direction){
				placeholder.name = "Set Actor Torso Direction Helper";

				if(no_placeholder){
					bool has_direction = target_actor_torso_direction != vec3(0.0);
					// Only create a placeholder when there is data to set.
					if(has_direction){
						placeholder.Retrieve();
						placeholder.SetTranslation(target_actor_torso_direction);
						placeholder.SetScale(target_actor_torso_direction_weight / 4.0f + 0.1f);
					}
				}else{
					placeholder.Retrieve();
				}
			}else if(dialogue_function == set_actor_head_direction){
				placeholder.name = "Set Actor Head Direction Helper";

				if(no_placeholder){
					bool has_direction = target_actor_head_direction != vec3(0.0);

					if(has_direction){
						placeholder.Retrieve();
						placeholder.SetTranslation(target_actor_head_direction);
						placeholder.SetScale(target_actor_head_direction_weight / 4.0f + 0.1f);
					}
				}else{
					placeholder.Retrieve();
				}
			}else if(dialogue_function == set_actor_eye_direction){
				placeholder.name = "Set Actor Eye Direction Helper";

				if(no_placeholder){
					bool has_direction = target_actor_eye_direction != vec3(0.0);

					if(has_direction){
						placeholder.Retrieve();
						placeholder.SetTranslation(target_actor_eye_direction);
						placeholder.SetScale(0.05f + 0.05f * target_blink_multiplier);
					}
				}else{
					placeholder.Retrieve();
				}
			}
		}
	}

	JSONValue GetSaveData(){
		JSONValue data;
		data["dialogue_function"] = JSONValue(dialogue_function);

		if(dialogue_function == say){
			data["say_text"] = JSONValue(say_text);
			data["auto_continue"] = JSONValue(auto_continue);
			data["voice_over_path"] = JSONValue(voice_over_path);
		}else if(dialogue_function == actor_settings){
			data["dialogue_color"] = JSONValue(JSONarrayValue);
			data["dialogue_color"].append(dialogue_color.x);
			data["dialogue_color"].append(dialogue_color.y);
			data["dialogue_color"].append(dialogue_color.z);
			data["dialogue_color"].append(dialogue_color.a);
			data["voice"] = JSONValue(voice);
			data["avatar_path"] = JSONValue(avatar_path);
		}else if(dialogue_function == set_actor_position){
			data["target_actor_position"] = JSONValue(JSONarrayValue);
			data["target_actor_position"].append(target_actor_position.x);
			data["target_actor_position"].append(target_actor_position.y);
			data["target_actor_position"].append(target_actor_position.z);
			data["target_actor_rotation"] = JSONValue(target_actor_rotation);
		}else if(dialogue_function == set_actor_animation){
			data["target_actor_animation"] = JSONValue(target_actor_animation);
			data["anim_mirrored"] = JSONValue(anim_mirrored);
			data["anim_mobile"] = JSONValue(anim_mobile);
			data["anim_super_mobile"] = JSONValue(anim_super_mobile);
			data["anim_from_start"] = JSONValue(anim_from_start);
			data["use_ik"] = JSONValue(use_ik);
			data["custom_animation"] = JSONValue(custom_animation);
			data["wait_anim_end"] = JSONValue(wait_anim_end);
		}else if(dialogue_function == set_actor_eye_direction){
			placeholder.Save(data);
		}else if(dialogue_function == set_actor_torso_direction){
			placeholder.Save(data);
		}else if(dialogue_function == set_actor_head_direction){
			placeholder.Save(data);
		}else if(dialogue_function == set_actor_omniscient){
			data["omniscient"] = JSONValue(omniscient);
		}else if(dialogue_function == set_camera_position){
			data["target_camera_position"] = JSONValue(JSONarrayValue);
			data["target_camera_position"].append(target_camera_position.x);
			data["target_camera_position"].append(target_camera_position.y);
			data["target_camera_position"].append(target_camera_position.z);
			data["target_camera_rotation"] = JSONValue(JSONarrayValue);
			data["target_camera_rotation"].append(target_camera_rotation.x);
			data["target_camera_rotation"].append(target_camera_rotation.y);
			data["target_camera_rotation"].append(target_camera_rotation.z);
			data["target_camera_zoom"] = JSONValue(target_camera_zoom);
			data["dof_settings"] = JSONValue(JSONarrayValue);
			for(uint i = 0; i < dof_settings.size(); i++){
				data["dof_settings"].append(dof_settings[i]);
			}
			data["add_camera_shake"] = JSONValue(add_camera_shake);
			if(add_camera_shake){
				data["position_shake_max_distance"] = JSONValue(position_shake_max_distance);
				data["position_shake_slerp_speed"] = JSONValue(position_shake_slerp_speed);
				data["position_shake_interval"] = JSONValue(position_shake_interval);
				data["rotation_shake_max_distance"] = JSONValue(rotation_shake_max_distance);
				data["rotation_shake_slerp_speed"] = JSONValue(rotation_shake_slerp_speed);
				data["rotation_shake_interval"] = JSONValue(rotation_shake_interval);
			}
			data["enable_look_at_target"] = JSONValue(enable_look_at_target);
			data["enable_move_with_target"] = JSONValue(enable_move_with_target);
			if(enable_look_at_target){
				look_at_target.SaveIdentifier(data);
			}
			if(enable_move_with_target){
				move_with_target.SaveIdentifier(data);
			}
			data["camera_transition"] = JSONValue(camera_transition);
		}else if(dialogue_function == fade_to_black){
			data["target_fade_to_black"] = JSONValue(target_fade_to_black);
			data["fade_to_black_duration"] = JSONValue(fade_to_black_duration);
		}else if(dialogue_function == settings){
			data["dialogue_layout"] = JSONValue(dialogue_layout);
			data["dialogue_text_font"] = JSONValue(dialogue_text_font);
			data["dialogue_text_size"] = JSONValue(dialogue_text_size);
			data["dialogue_text_shadow"] = JSONValue(dialogue_text_shadow);
			data["use_voice_sounds"] = JSONValue(use_voice_sounds);
			data["show_names"] = JSONValue(show_names);
			data["show_avatar"] = JSONValue(show_avatar);
			data["dialogue_location"] = JSONValue(dialogue_location);
			data["dialogue_text_speed"] = JSONValue(dialogue_text_speed);

			data["dialogue_text_color"] = JSONValue(JSONarrayValue);
			data["dialogue_text_color"].append(dialogue_text_color.x);
			data["dialogue_text_color"].append(dialogue_text_color.y);
			data["dialogue_text_color"].append(dialogue_text_color.z);
			data["dialogue_text_color"].append(dialogue_text_color.a);
		}else if(dialogue_function == set_actor_dialogue_control){
			data["dialogue_control"] = JSONValue(dialogue_control);
		}else if(dialogue_function == choice){
			data["nr_choices"] = JSONValue(nr_choices);
			data["ui_sounds"] = JSONValue(ui_sounds);
			if(ui_sounds){
				data["hover_sound_path"] = JSONValue(hover_sound_path);
				data["click_sound_path"] = JSONValue(click_sound_path);
			}

			for(int i = 0; i < nr_choices; i++){
				int number = i + 1;
				data["choice_" + number] = JSONValue(choice_texts[i]);
				data["check_parameter_" + number] = JSONValue(check_parameters[i]);
				if(check_parameters[i]){
					data["check_parameter_name_" + number] = JSONValue(parameter_names[i]);
					data["check_parameter_value_" + number] = JSONValue(parameter_check_values[i]);
					data["check_parameter_mode_" + number] = JSONValue(parameter_check_modes[i]);
				}
				choice_elements[i].SaveGoToLine(data);
			}
		}else if(dialogue_function == start){
			data["use_fade"] = JSONValue(use_fade);
		}else if(dialogue_function == end){
			data["use_fade"] = JSONValue(use_fade);
		}

		if(dialogue_function == say || dialogue_function == actor_settings || dialogue_function == set_actor_position || dialogue_function == set_actor_animation || dialogue_function == set_actor_eye_direction || dialogue_function == set_actor_torso_direction || dialogue_function == set_actor_head_direction || dialogue_function == set_actor_omniscient || dialogue_function == set_actor_dialogue_control){
			target_select.SaveIdentifier(data);
		}

		return data;
	}

	string GetDisplayString(){
		string display_string = "Dialogue ";
		display_string += dialogue_function_names[current_dialogue_function] + " ";
		UpdateActorName();

		if(dialogue_function == say){
			display_string += actor_name + " ";
			display_string += "\"" + say_text + "\"";
		}else if(dialogue_function == actor_settings){
			display_string += actor_name + " ";
		}else if(dialogue_function == set_actor_position){
			display_string += actor_name + " ";
		}else if(dialogue_function == set_actor_animation){
			display_string += actor_name + " ";
			display_string += target_actor_animation;
		}else if(dialogue_function == set_actor_eye_direction){
			display_string += actor_name + " ";
			display_string += (placeholder.GetScale().x - 0.05f) / 0.05f;
		}else if(dialogue_function == set_actor_torso_direction){
			display_string += actor_name + " ";
			display_string += (placeholder.GetScale().x - 0.1f) * 4.0f;
		}else if(dialogue_function == set_actor_head_direction){
			display_string += actor_name + " ";
			display_string += (placeholder.GetScale().x - 0.1f) * 4.0f;
		}else if(dialogue_function == set_actor_omniscient){
			display_string += actor_name + " ";
			display_string += omniscient;
		}else if(dialogue_function == set_camera_position){
			display_string += target_camera_zoom;
		}else if(dialogue_function == fade_to_black){
			display_string += target_fade_to_black + " ";
			display_string += fade_to_black_duration;
		}else if(dialogue_function == set_actor_dialogue_control){
			display_string += actor_name + " ";
			display_string += dialogue_control;
		}else if(dialogue_function == choice){
			for(int i = 0; i < nr_choices; i++){
				choice_elements[i].CheckLineAvailable();
			}
		}

		return display_string;
	}

	void UpdateActorName(){
		actor_name = target_select.GetTargetDisplayText();
	}

	void Delete(){
		Reset();
		placeholder.Remove();
		target_select.Delete();
		look_at_target.Delete();
		move_with_target.Delete();
	}

	void StartSettings(){
		target_select.CheckAvailableTargets();
		look_at_target.CheckAvailableTargets();
		move_with_target.CheckAvailableTargets();
		if(dialogue_function == say){
			set_keyboard_focus = true;
		}else if(dialogue_function == set_actor_animation){
			if(all_animations.size() == 0){
				level.SendMessage("drika_dialogue_get_animations " + hotspot.GetID());
			}
			QueryAnimation(search_buffer);
		}
	}

	void DrawEditing(){

		array<MovementObject@> targets = target_select.GetTargetMovementObjects();
		if(dialogue_function == say || dialogue_function == actor_settings || dialogue_function == set_actor_position || dialogue_function == set_actor_animation || dialogue_function == set_actor_eye_direction || dialogue_function == set_actor_torso_direction || dialogue_function == set_actor_head_direction || dialogue_function == set_actor_omniscient || dialogue_function == set_actor_dialogue_control){
			for(uint i = 0; i < targets.size(); i++){
				DebugDrawLine(targets[i].position, this_hotspot.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
			}
		}

		if(dialogue_function == set_actor_position){
			PlaceholderCheck();
			if(placeholder.IsSelected()){
				vec3 new_position = placeholder.GetTranslation();
				vec4 v = placeholder.GetRotationVec4();
				quaternion quat(v.x,v.y,v.z,v.a);
				vec3 facing = Mult(quat, vec3(0,0,1));
				float rot = atan2(facing.x, facing.z) * 180.0f / PI;

				float new_rotation = floor(rot + 0.5f);

				if(target_actor_position != new_position || target_actor_rotation != new_rotation){
					target_actor_position = new_position;
					target_actor_rotation = new_rotation;
					SetActorPosition();
				}
			}
		}else if(dialogue_function == set_actor_eye_direction){
			PlaceholderCheck();
			DebugDrawBillboard("Data/Textures/ui/eye_widget.tga", placeholder.GetTranslation(), 0.1, vec4(1.0), _delete_on_draw);

			for(uint i = 0; i < targets.size(); i++){
				vec3 head_pos = targets[i].rigged_object().GetAvgIKChainPos("head");
				DebugDrawLine(head_pos, placeholder.GetTranslation(), vec4(1.0), vec4(1.0), _delete_on_draw);
			}

			if(placeholder.IsSelected()){
				float scale = placeholder.GetScale().x;
				if(scale < 0.05f){
					placeholder.SetScale(vec3(0.05f));
				}
				if(scale > 0.1f){
					placeholder.SetScale(vec3(0.1f));
				}

				SetActorEyeDirection();
			}
		}else if(dialogue_function == set_actor_torso_direction){
			PlaceholderCheck();
			DebugDrawBillboard("Data/Textures/ui/torso_widget.tga", placeholder.GetTranslation(), 0.25, vec4(1.0), _delete_on_draw);

			for(uint i = 0; i < targets.size(); i++){
				vec3 torso_pos = targets[i].rigged_object().GetAvgIKChainPos("torso");
				DebugDrawLine(torso_pos, placeholder.GetTranslation(), vec4(1.0), vec4(1.0), _delete_on_draw);
			}

			if(placeholder.IsSelected()){
				float scale = placeholder.GetScale().x;

				if(scale < 0.1f){
					placeholder.SetScale(vec3(0.1f));
				}
				if(scale > 0.35f){
					placeholder.SetScale(vec3(0.35f));
				}

				SetActorTorsoDirection();
			}
		}else if(dialogue_function == set_actor_head_direction){
			PlaceholderCheck();
			DebugDrawBillboard("Data/Textures/ui/head_widget.tga", placeholder.GetTranslation(), 0.25, vec4(1.0), _delete_on_draw);

			for(uint i = 0; i < targets.size(); i++){
				vec3 head_pos = targets[i].rigged_object().GetAvgIKChainPos("head");
				DebugDrawLine(head_pos, placeholder.GetTranslation(), vec4(1.0), vec4(1.0), _delete_on_draw);
			}

			if(placeholder.IsSelected()){
				float scale = placeholder.GetScale().x;
				if(scale < 0.1f){
					placeholder.SetScale(vec3(0.1f));
				}
				if(scale > 0.35f){
					placeholder.SetScale(vec3(0.35f));
				}

				SetActorHeadDirection();
			}
		}else if(dialogue_function == set_camera_position){
			PlaceholderCheck();

			if(placeholder.IsSelected()){
				vec3 new_position = placeholder.GetTranslation();
				vec4 v = placeholder.GetRotationVec4();
				quaternion quat(v.x,v.y,v.z,v.a);
				vec3 front = Mult(quat, vec3(0,0,1));
				vec3 new_rotation;
				new_rotation.y = atan2(front.x, front.z) * 180.0f / PI;
				new_rotation.x = asin(front[1]) * -180.0f / PI;
				vec3 up = Mult(quat, vec3(0,1,0));
				vec3 expected_right = normalize(cross(front, vec3(0,1,0)));
				vec3 expected_up = normalize(cross(expected_right, front));
				new_rotation.z = atan2(dot(up,expected_right), dot(up, expected_up)) * 180.0f / PI;

				const float zoom_sensitivity = 3.5f;
				float new_zoom = min(150.0f, 90.0f / max(0.001f, (1.0f + (placeholder.GetScale().x - 1.0f) * zoom_sensitivity)));

				if(target_camera_position != new_position || target_camera_rotation != new_rotation || target_camera_zoom != new_zoom){
					target_camera_position = new_position;
					target_camera_rotation = new_rotation;
					target_camera_zoom = new_zoom;
				}
			}

			if(enable_look_at_target){
				array<vec3> look_target_positions = look_at_target.GetTargetPositions();

				for(uint j = 0; j < look_target_positions.size(); j++){
					DebugDrawLine(placeholder.GetTranslation(), look_target_positions[j], vec3(0.0, 1.0, 0.0), _delete_on_draw);
				}
			}

			if(enable_move_with_target){
				array<vec3> move_target_positions = move_with_target.GetTargetPositions();

				for(uint j = 0; j < move_target_positions.size(); j++){
					DebugDrawLine(placeholder.GetTranslation(), move_target_positions[j], vec3(0.0, 0.0, 1.0), _delete_on_draw);
				}
			}
		}
	}

	void StartEdit(){
		DrikaElement::StartEdit();
		Apply();
	}

	void Apply(){
		if(dialogue_function == set_actor_position){
			SetActorPosition();
		}else if(dialogue_function == actor_settings){
			SetActorSettings();
		}else if(dialogue_function == set_actor_animation){
			SetActorAnimation();
		}else if(dialogue_function == set_actor_eye_direction){
			SetActorEyeDirection();
		}else if(dialogue_function == set_actor_torso_direction){
			SetActorTorsoDirection();
		}else if(dialogue_function == set_actor_head_direction){
			SetActorHeadDirection();
		}else if(dialogue_function == set_actor_omniscient){
			SetActorOmniscient();
		}else if(dialogue_function == fade_to_black){
			SetFadeToBlack();
		}else if(dialogue_function == settings){
			SetDialogueSettings();
		}else if(dialogue_function == set_actor_dialogue_control){
			SetActorDialogueControl();
		}else if(dialogue_function == choice){
			Reset();
		}else if(dialogue_function == set_camera_position){
			SetCameraTransform(target_camera_position, target_camera_rotation);
			SetDialogueDOF();
		}
	}

	void EditDone(){
		if(dialogue_function != set_actor_torso_direction && dialogue_function != set_actor_head_direction && dialogue_function != set_actor_eye_direction){
			placeholder.Remove();
		}
		if(dialogue_function != set_actor_dialogue_control){
			Reset();
		}
		DrikaElement::EditDone();
	}

	void ApplySettings(){
		Apply();
	}

	void PlaceholderCheck(){
		if(!placeholder.Exists()){

			array<MovementObject@> targets = target_select.GetTargetMovementObjects();

			if(dialogue_function == set_actor_eye_direction){
				placeholder.name = "Set Actor Eye Direction Helper";
				placeholder.Create();
				vec3 direction;

				if(targets.size() > 0){
					direction = targets[0].position + vec3(0.0f, 1.0f, 0.0f);
				}else{
					direction = this_hotspot.GetTranslation() + vec3(0.0, 2.0, 0.0);
				}

				placeholder.SetTranslation(direction);
				placeholder.SetScale(0.1f);
			}else if(dialogue_function == set_actor_position){
				placeholder.path = "Data/Objects/placeholder/empty_placeholder.xml";
				placeholder.Create();

				if(target_actor_position == vec3(0.0)){
					if(targets.size() > 0){
						target_actor_position = targets[0].position;
					}else{
						//If this is a new set character position then use the hotspot as the default position.
						target_actor_position = this_hotspot.GetTranslation() + vec3(0.0, 2.0, 0.0);
					}
				}
				placeholder.SetTranslation(target_actor_position);
				placeholder.SetRotation(quaternion(vec4(0,1,0, target_actor_rotation * PI / 180.0f)));
				placeholder.SetPreview("Data/Objects/drika_spawn_placeholder.xml");
				placeholder.SetEditorDisplayName("Set Actor Position Helper");
			}else if(dialogue_function == set_actor_torso_direction){
				placeholder.name = "Set Actor Torso Direction Helper";
				placeholder.Create();
				vec3 direction;

				if(targets.size() > 0){
					direction = targets[0].position + vec3(0.0f, 0.5f, 0.0f);
				}else{
					direction = this_hotspot.GetTranslation() + vec3(0.0, 2.0, 0.0);
				}

				placeholder.SetScale(0.35);
				placeholder.SetTranslation(direction);
			}else if(dialogue_function == set_actor_head_direction){
				placeholder.name = "Set Actor Head Direction Helper";
				placeholder.Create();
				vec3 direction;

				if(target_actor_head_direction == vec3(0.0)){
					if(targets.size() > 0){
						direction = targets[0].position + vec3(0.0f, 0.9f, 0.0f);
					}else{
						direction = this_hotspot.GetTranslation() + vec3(0.0, 2.0, 0.0);
					}
				}
				placeholder.SetScale(0.35f);
				placeholder.SetTranslation(direction);
			}else if(dialogue_function == set_camera_position){
				// The camera placeholder needs to be a different object type for the preview window to show up.
				placeholder.path = "Data/Objects/placeholder/empty_placeholder.xml";
				placeholder.Create();

				if(target_camera_position == vec3(0.0)){
					target_camera_position = this_hotspot.GetTranslation() + vec3(0.0, 2.0, 0.0);
				}
				placeholder.SetTranslation(target_camera_position);

				const float zoom_sensitivity = 3.5f;
				float scale = (90.0f / target_camera_zoom - 1.0f) / zoom_sensitivity + 1.0f;
				placeholder.SetScale(vec3(scale));

				float deg2rad = PI / 180.0f;
				quaternion rot_y(vec4(0, 1, 0, target_camera_rotation.y * deg2rad));
				quaternion rot_x(vec4(1, 0, 0, target_camera_rotation.x * deg2rad));
				quaternion rot_z(vec4(0, 0, 1, target_camera_rotation.z * deg2rad));
				placeholder.SetRotation(rot_y * rot_x * rot_z);

				placeholder.SetPreview("Data/Objects/camera.xml");
				placeholder.SetEditorDisplayName("Set Camera Position Helper");
				placeholder.SetSpecialType(kCamPreview);
			}
		}
	}

	void DrawSettings(){

		float option_name_width = 155.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Dialogue Function");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		ImGui_PushItemWidth(second_column_width);
		if(ImGui_Combo("###Dialogue Function", current_dialogue_function, dialogue_function_names, dialogue_function_names.size())){
			placeholder.Remove();
			Reset();
			dialogue_function = dialogue_functions(current_dialogue_function);

			if(dialogue_function == say || dialogue_function == actor_settings || dialogue_function == set_actor_position || dialogue_function == set_actor_animation || dialogue_function == set_actor_eye_direction || dialogue_function == set_actor_torso_direction || dialogue_function == set_actor_head_direction || dialogue_function == set_actor_omniscient || dialogue_function == set_actor_dialogue_control){
				connection_types = {_movement_object};
			}else{
				connection_types = {};
			}

			if(dialogue_function == set_actor_animation){
				if(all_animations.size() == 0){
					level.SendMessage("drika_dialogue_get_animations " + hotspot.GetID());
				}
				QueryAnimation(search_buffer);
			}
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		if(connection_types.find(_movement_object) != -1){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Target Character");
			ImGui_NextColumn();
			ImGui_NextColumn();

			target_select.DrawSelectTargetUI();
		}

		if(dialogue_function == say){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Auto Continue");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_Checkbox("", auto_continue);
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Voice Over");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_Button("Load...")){
				string new_path = GetUserPickedReadPath("wav", GetLastReadPath("Data/Sounds"));
				if(new_path != ""){
					voice_over_path = new_path;
					Reset();
					SetLastReadPath(voice_over_path);
				}
			}
			ImGui_SameLine();
			ImGui_Text(voice_over_path);
			if(voice_over_path != ""){
				ImGui_SameLine();
				if(ImGui_Button("Clear")){
					voice_over_path = "";
					Reset();
				}
			}
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Text");
			ImGui_NextColumn();
			ImGui_SetTextBuf(say_text);

			if(set_keyboard_focus){
				set_keyboard_focus = false;
				ImGui_SetKeyboardFocusHere(0);
			}

			if(ImGui_InputTextMultiline("##TEXT", vec2(-1.0, -1.0))){
				say_text = ImGui_GetTextBuf();
				say_changed_timer = 1.0f;
			}

			ImGui_PopItemWidth();
		}else if(dialogue_function == actor_settings){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Dialogue Color");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_ColorEdit4("##Dialogue Color", dialogue_color)){

			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Voice");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_SliderInt("##Voice", voice, 0, 18, "%.0f")){
				level.SendMessage("drika_dialogue_test_voice " + voice);
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Avatar");
			ImGui_NextColumn();
			ImGui_BeginChild("avatar_select_ui", vec2(0, 50), false, ImGuiWindowFlags_AlwaysAutoResize);
			ImGui_Columns(2, false);
			ImGui_SetColumnWidth(0, 110);
			if(ImGui_Button("Pick Avatar")){
				string new_path = GetUserPickedReadPath("png", "Data/Textures");
				if(new_path != ""){
					new_path = ShortenPath(new_path);
					array<string> split_path = new_path.split(".");
					string extention = split_path[split_path.size() - 1];
					if(extention != "jpg" && extention != "png" && extention != "tga"){
						DisplayError("Load Avatar", "Only .png, .tga or .jpg files are allowed.");
					}else{
						avatar_path = new_path;
						avatar = LoadTexture(avatar_path, TextureLoadFlags_NoMipmap | TextureLoadFlags_NoConvert |TextureLoadFlags_NoReduce);
					}
				}
			}
			if(avatar_path != "None"){
				if(ImGui_Button("Clear Avatar")){
					avatar_path = "None";
					avatar = LoadTexture(default_avatar_path, TextureLoadFlags_NoMipmap | TextureLoadFlags_NoConvert |TextureLoadFlags_NoReduce);
				}
			}
			ImGui_NextColumn();
			ImGui_Image(avatar, vec2(50, 50));
			ImGui_EndChild();
		}else if(dialogue_function == set_actor_animation){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Animation");
			ImGui_NextColumn();

			if(ImGui_Checkbox("From Start", anim_from_start)){
				SetActorAnimation();
			}
			ImGui_SameLine();
			if(ImGui_Checkbox("Mirrored", anim_mirrored)){
				SetActorAnimation();
			}
			ImGui_SameLine();
			if(ImGui_Checkbox("Mobile", anim_mobile)){
				SetActorAnimation();
			}
			ImGui_SameLine();
			if(ImGui_Checkbox("Super Mobile", anim_super_mobile)){
				SetActorAnimation();
			}
			ImGui_SameLine();
			if(ImGui_Checkbox("Use IK", use_ik)){
				SetActorAnimation();
			}
			ImGui_SameLine();
			ImGui_Checkbox("Wait Animation End", wait_anim_end);

			float custom_width = 100.0f;

			ImGui_SetTextBuf(search_buffer);
			ImGui_Text("Search:");
			ImGui_SameLine();
			ImGui_PushItemWidth(ImGui_GetContentRegionAvailWidth() - custom_width);
			if(ImGui_InputText("", ImGuiInputTextFlags_AutoSelectAll)){
				search_buffer = ImGui_GetTextBuf();
				QueryAnimation(ImGui_GetTextBuf());
			}
			ImGui_PopItemWidth();

			ImGui_SameLine();
			ImGui_AlignTextToFramePadding();
			ImGui_PushItemWidth(ImGui_GetContentRegionAvailWidth());
			if(ImGui_Button("Custom...")){
				string new_path = GetUserPickedReadPath("anm", "Data/Animations");
				if(new_path != ""){
					new_path = ShortenPath(new_path);
					array<string> path_split = new_path.split("/");
					string file_name = path_split[path_split.size() - 1];
					string file_extension = file_name.substr(file_name.length() - 3, 3);

					if(file_extension == "anm" || file_extension == "ANM"){
						custom_animation = true;
						target_actor_animation = new_path;
						//Check if the animation is a custom or part of an existing group.
						for(uint i = 0; i < all_animations.size(); i++){
							if(all_animations[i].name != "Custom"){
								for(uint j = 0; j < all_animations[i].animations.size(); j++){
									if(all_animations[i].animations[j] == target_actor_animation){
										//Found the animation so it's not custom.
										custom_animation = false;
										break;
									}
								}
							}
							if(!custom_animation){
								break;
							}
						}

						//Register the custom animation so that it's saved to the SaveFile.
						if(custom_animation){
							level.SendMessage("drika_dialogue_add_new_custom_animation " + target_actor_animation);
						}
						SetActorAnimation();
						update_animation_list_scroll = true;
						QueryAnimation(search_buffer);
					}else{
						DisplayError("Animation issue", "Only .anm files are supported.");
					}
				}
			}
			ImGui_PopItemWidth();

			if(ImGui_BeginChildFrame(55, vec2(-1, -1), ImGuiWindowFlags_AlwaysAutoResize)){
				for(uint i = 0; i < current_animations.size(); i++){
					AddCategory(current_animations[i].name, current_animations[i].animations);
				}
				ImGui_EndChildFrame();
			}

		}else if(dialogue_function == set_actor_omniscient){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Set Omnicient to");
			ImGui_NextColumn();
			ImGui_Checkbox("", omniscient);
		}else if(dialogue_function == fade_to_black){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Target Alpha");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_SliderFloat("##Target Alpha", target_fade_to_black, 0.0, 1.0, "%.3f");
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Fade Duration");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_SliderFloat("##Fade Duration", fade_to_black_duration, 0.0, 10.0, "%.3f");
			ImGui_PopItemWidth();
		}else if(dialogue_function == settings){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Dialogue Layout");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_Combo("##Dialogue Layout", dialogue_layout, dialogue_layout_names, dialogue_layout_names.size());
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Font");
			ImGui_NextColumn();
			if(ImGui_Button("Set Font")){
				string new_path = GetUserPickedReadPath("ttf", "Data/Fonts");
				if(new_path != ""){
					new_path = ShortenPath(new_path);
					array<string> path_split = new_path.split("/");
					string file_name = path_split[path_split.size() - 1];
					string file_extension = file_name.substr(file_name.length() - 3, 3);

					if(file_extension == "ttf" || file_extension == "TTF"){
						dialogue_text_font = new_path;
					}else{
						DisplayError("Font issue", "Only ttf font files are supported.");
					}
				}
			}
			ImGui_SameLine();
			ImGui_AlignTextToFramePadding();
			ImGui_Text(dialogue_text_font);
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Dialogue Text Size");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_SliderInt("##Dialogue Text Size", dialogue_text_size, 1, 100, "%.0f");
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Dialogue Text Color");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_ColorEdit4("##Dialogue Text Color", dialogue_text_color);
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Dialogue Text Shadow");
			ImGui_NextColumn();
			ImGui_Checkbox("###Dialogue Text Shadow", dialogue_text_shadow);
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Use Voice Sounds");
			ImGui_NextColumn();
			ImGui_Checkbox("###Use Voice Sounds", use_voice_sounds);
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Show Name");
			ImGui_NextColumn();
			ImGui_Checkbox("###Show Name", show_names);
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Show Avatar");
			ImGui_NextColumn();
			ImGui_Checkbox("###Show Avatar", show_avatar);
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Dialogue Location");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_Combo("##Dialogue Location", dialogue_location, dialogue_location_names, dialogue_location_names.size());
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Text Speed");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_SliderFloat("##Text Speed", dialogue_text_speed, 1.0, 100.0, "%.0f");
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}else if(dialogue_function == set_actor_dialogue_control){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Set dialogue control to");
			ImGui_NextColumn();
			ImGui_Checkbox("", dialogue_control);
		}else if(dialogue_function == choice){

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Number of choices");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_SliderInt("###Number of choices", nr_choices, 1, max_choices, "%.0f")){
				Reset();
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			for(int i = 0; i < nr_choices; i++){
				int number = i + 1;
				ImGui_Separator();
				ImGui_AlignTextToFramePadding();
				ImGui_Text("Choice " + number);
				ImGui_NextColumn();
				ImGui_PushItemWidth(second_column_width);
				if(ImGui_InputText("##text" + number, choice_texts[i], 64)){
					Reset();
				}
				ImGui_PopItemWidth();
				ImGui_NextColumn();
				choice_elements[i].DrawGoToLineUI();

				ImGui_AlignTextToFramePadding();
				ImGui_Text("Parameter Check");
				ImGui_NextColumn();
				ImGui_Checkbox("###Parameter Check" + number, check_parameters[i]);

				if(check_parameters[i]){
					ImGui_NextColumn();
					ImGui_NextColumn();
					ImGui_PushItemWidth((second_column_width / 3.0) - 5.0);
					ImGui_InputText("##Parameter Name" + number, parameter_names[i], 64);
					ImGui_SameLine();
					int current_parameter_check_mode = parameter_check_modes[i];
					if(ImGui_Combo("##Parameter Check " + number, current_parameter_check_mode, check_mode_choices, check_mode_choices.size())){
						parameter_check_modes[i] = check_modes(current_parameter_check_mode);
					}
					ImGui_SameLine();
					ImGui_InputText("##Parameter Value" + number, parameter_check_values[i], 64);
					ImGui_PopItemWidth();
				}

				ImGui_NextColumn();
			}

			ImGui_Separator();
			ImGui_AlignTextToFramePadding();
			ImGui_Text("UI Sounds");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_Checkbox("###Use Sounds", ui_sounds);
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			if(ui_sounds){
				ImGui_AlignTextToFramePadding();
				ImGui_Text("Hover Sound");
				ImGui_NextColumn();
				if(ImGui_Button("Set Sound Path")){
					string new_path = GetUserPickedReadPath("wav", "Data/Sounds");
					// The path will be returned empty if the user cancels the file pick.
					if(new_path != ""){
						hover_sound_path = ShortenPath(new_path);
					}
				}
				ImGui_SameLine();
				ImGui_Text(hover_sound_path);
				ImGui_NextColumn();

				ImGui_AlignTextToFramePadding();
				ImGui_Text("Click Sound");
				ImGui_NextColumn();
				if(ImGui_Button("Set Sound Path")){
					string new_path = GetUserPickedReadPath("wav", "Data/Sounds");
					// The path will be returned empty if the user cancels the file pick.
					if(new_path != ""){
						click_sound_path = ShortenPath(new_path);
					}
				}
				ImGui_SameLine();
				ImGui_Text(click_sound_path);
				ImGui_NextColumn();
			}

		}else if(dialogue_function == set_camera_position){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Near Blur");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_SliderFloat("##Near Blur", dof_settings[0], 0.0f, 10.0f, "%.1f")){
				update_dof = true;
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Near Dist");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_SliderFloat("##Near Dist", dof_settings[1], 0.0f, 10.0f, "%.1f")){
				update_dof = true;
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Near Transition");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_SliderFloat("##Near Transition", dof_settings[2], 0.0f, 10.0f, "%.1f")){
				update_dof = true;
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Far Blur");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_SliderFloat("##Far Blur", dof_settings[3], 0.0f, 10.0f, "%.1f")){
				update_dof = true;
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Far Dist");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_SliderFloat("##Far Dist", dof_settings[4], 0.0f, 10.0f, "%.1f")){
				update_dof = true;
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Far Transition");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_SliderFloat("##Far Transition", dof_settings[5], 0.0f, 10.0f, "%.1f")){
				update_dof = true;
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_Separator();
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Look At Target");
			ImGui_NextColumn();
			ImGui_Checkbox("###Look At Target", enable_look_at_target);
			ImGui_NextColumn();

			if(enable_look_at_target){
				ImGui_AlignTextToFramePadding();
				ImGui_Text("Look Target");
				ImGui_NextColumn();
				ImGui_NextColumn();
				look_at_target.DrawSelectTargetUI();
			}
			ImGui_Separator();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Move With Target");
			ImGui_NextColumn();
			ImGui_Checkbox("###Move With Target", enable_move_with_target);
			ImGui_NextColumn();

			if(enable_move_with_target){
				ImGui_AlignTextToFramePadding();
				ImGui_Text("Move Target");
				ImGui_NextColumn();
				ImGui_NextColumn();
				move_with_target.DrawSelectTargetUI();
			}

			ImGui_Separator();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Transition Method");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_Combo("###Transition Method", current_camera_transition, camera_transition_names, camera_transition_names.size())){
				camera_transition = camera_transitions(current_camera_transition);
			}
			ImGui_NextColumn();

			ImGui_Separator();
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Add Camera Shake");
			ImGui_NextColumn();
			ImGui_Checkbox("###Add Camera Shake", add_camera_shake);
			ImGui_NextColumn();

			if(add_camera_shake){
				ImGui_AlignTextToFramePadding();
				ImGui_Text("Pos Shake Max");
				ImGui_NextColumn();
				ImGui_PushItemWidth(second_column_width);
				ImGui_SliderFloat("##Pos Shake Max", position_shake_max_distance, 0.1, 2.0, "%.1f");
				ImGui_PopItemWidth();
				ImGui_NextColumn();

				ImGui_AlignTextToFramePadding();
				ImGui_Text("Pos Shake Slerp Speed");
				ImGui_NextColumn();
				ImGui_PushItemWidth(second_column_width);
				ImGui_SliderFloat("##Pos Shake Slerp Speed", position_shake_slerp_speed, 0.01, 5.0, "%.2f");
				ImGui_PopItemWidth();
				ImGui_NextColumn();

				ImGui_AlignTextToFramePadding();
				ImGui_Text("Pos Shake Interval");
				ImGui_NextColumn();
				ImGui_PushItemWidth(second_column_width);
				ImGui_SliderFloat("##Pos Shake Interval", position_shake_interval, 0.01, 1.0, "%.2f");
				ImGui_PopItemWidth();
				ImGui_NextColumn();

				ImGui_AlignTextToFramePadding();
				ImGui_Text("Rot Shake Max");
				ImGui_NextColumn();
				ImGui_PushItemWidth(second_column_width);
				ImGui_SliderFloat("##Rot Shake Max", rotation_shake_max_distance, 0.1, 90.0, "%.1f");
				ImGui_PopItemWidth();
				ImGui_NextColumn();

				ImGui_AlignTextToFramePadding();
				ImGui_Text("Rot Shake Slerp Speed");
				ImGui_NextColumn();
				ImGui_PushItemWidth(second_column_width);
				ImGui_SliderFloat("##Rot Shake Slerp Speed", rotation_shake_slerp_speed, 0.01, 5.0, "%.2f");
				ImGui_PopItemWidth();
				ImGui_NextColumn();

				ImGui_AlignTextToFramePadding();
				ImGui_Text("Rot Shake Interval");
				ImGui_NextColumn();
				ImGui_PushItemWidth(second_column_width);
				ImGui_SliderFloat("##Rot Shake Interval", rotation_shake_interval, 0.01, 1.0, "%.2f");
				ImGui_PopItemWidth();
				ImGui_NextColumn();
			}
			ImGui_Separator();
		}else if(dialogue_function == start){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Use Fade");
			ImGui_NextColumn();
			ImGui_Checkbox("###Use Fade", use_fade);
			ImGui_NextColumn();
		}else if(dialogue_function == end){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Use Fade");
			ImGui_NextColumn();
			ImGui_Checkbox("###Use Fade", use_fade);
			ImGui_NextColumn();
		}
	}

	void SetDialogueDOF(){
		level.SendMessage("drika_set_dof " + dof_settings[0] + " " + dof_settings[1] + " " + dof_settings[2] + " " + dof_settings[3] + " " + dof_settings[4] + " " + dof_settings[5]);
	}

	void AddCategory(string category, array<string> items){
		//Skip the category if the animation list is empty.
		if(items.size() < 1){
			return;
		}

		if(ImGui_TreeNodeEx(category, ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_DefaultOpen)){
			for(uint i = 0; i < items.size(); i++){
				AddItem(items[i]);
			}
			ImGui_TreePop();
		}
	}

	void AddItem(string name){
		bool is_selected = name == target_actor_animation;
		if(ImGui_Selectable(name, is_selected)){
			target_actor_animation = name;
			SetActorAnimation();
		}

		if((ImGui_IsWindowAppearing() || update_animation_list_scroll) && is_selected){
			ImGui_SetScrollHere(0.5);
		}
	}

	void TargetChanged(){
		PlaceholderCheck();
		Apply();
	}

	void PreTargetChanged(){
		Reset();
	}

	void ReceiveMessage(string message, int param){
		if(message == "drika_dialogue_choice_select"){
			SelectChoice(param);
		}else if(message == "drika_dialogue_choice_pick" && !editing){
			current_choice = param;
			triggered = true;
		}
	}

	void ReceiveMessage(array<string> messages){
		if(messages[0] == "fade_out_done"){
			wait_for_fade = false;
		}else if(messages[0] == "old_camera_transform"){
			camera_translation_from = vec3(atof(messages[1]), atof(messages[2]), atof(messages[3]));
			camera_rotation_from = vec3(atof(messages[4]), atof(messages[5]), atof(messages[6]));

			//Skip the move transition if the beginning and end transform is the same.
			if(distance(camera_rotation_from, target_camera_rotation) < 0.1 && distance(camera_translation_from, target_camera_position) < 0.1){
				skip_move_transition = true;
			}
		}
	}

	void Reset(){
		dialogue_done = false;
		wait_for_fade = false;
		skip_move_transition = false;
		camera_transition_timer = 0.0;
		if(dialogue_function == say){
			if(say_started){
				level.SendMessage("drika_dialogue_clear");
			}
			SetTargetTalking(false);
			say_started = false;
			say_timer = 0.0;
			wait_timer = 0.0;
			StopSound(voice_over_sound_id);
		}else if(dialogue_function == fade_to_black){
			if(triggered){
				ResetFadeToBlack();
			}
		}else if(dialogue_function == set_actor_dialogue_control){
			array<MovementObject@> targets = target_select.GetTargetMovementObjects();

			for(uint i = 0; i < targets.size(); i++){
				RemoveDialogueActor(targets[i]);
			}
		}else if(dialogue_function == choice){
			if(choice_ui_added){
				level.SendMessage("drika_dialogue_clear");
			}
			choice_ui_added = false;
		}else if(dialogue_function == start){
			if(triggered){
				level.SendMessage("drika_dialogue_end");
			}
		}else if(dialogue_function == set_camera_position){
			level.SendMessage("drika_set_dof 0.0 0.0 0.0 0.0 0.0 0.0");
		}
		triggered = false;
	}

	void Update(){
		if(dialogue_function == say){
			UpdateSayDialogue(true);
			if(say_changed_timer > 0.0f){
				say_changed_timer -= time_step;
				if(say_changed_timer <= 0.0f){
					Reset();
				}
			}
		}else if(dialogue_function == choice){
			ShowChoiceDialogue(true);
		}else if(dialogue_function == set_camera_position){
			if(update_dof){
				update_dof = false;
				SetDialogueDOF();
			}
		}
	}

	bool Trigger(){
		if(dialogue_function == say){
			if(UpdateSayDialogue(false)){
				Reset();
				return true;
			}
		}else if(dialogue_function == actor_settings){
			SetActorSettings();
			return true;
		}else if(dialogue_function == set_actor_position){
			SetActorPosition();
			return true;
		}else if(dialogue_function == set_actor_animation){
			//Wait for the next update if the character has just been Reset();
			array<MovementObject@> targets = target_select.GetTargetMovementObjects();
			for(uint i = 0; i < targets.size(); i++){
				if(targets[i].GetIntVar("updated") == 0){
					return false;
				}
			}

			if(wait_anim_end){
				if(!triggered){
					SetActorAnimation();
					triggered = true;
				}else{
					for(uint i = 0; i < targets.size(); i++){
						if(targets[i].GetBoolVar("in_animation")){
							return false;
						}
						triggered = false;
						return true;
					}
				}
				return false;
			}else{
				SetActorAnimation();
				return true;
			}
		}else if(dialogue_function == set_actor_eye_direction){
			SetActorEyeDirection();
			return true;
		}else if(dialogue_function == set_actor_torso_direction){
			SetActorTorsoDirection();
			return true;
		}else if(dialogue_function == set_actor_head_direction){
			SetActorHeadDirection();
			return true;
		}else if(dialogue_function == set_actor_omniscient){
			SetActorOmniscient();
			return true;
		}else if(dialogue_function == set_camera_position){
			if(camera_transition == move_transition){
				if(triggered == false){
					triggered = true;
					level.SendMessage("drika_get_old_camera_transform " + this_hotspot.GetID());
					return false;
				}

				if(skip_move_transition){
					SetCameraTransform(target_camera_position, target_camera_rotation);
					SetDialogueDOF();
					skip_move_transition = false;
					triggered = false;
					return true;
				}

				float transition_duration = 1.0;
				float alpha = ApplyTween(camera_transition_timer / transition_duration, inOutSineTween);
				alpha = max(0.0, min(1.0, alpha));

				// Convert the X Y Z rotations into quaternions.
				float deg2rad = PI / 180.0f;
				quaternion rot_y(vec4(0, 1, 0, target_camera_rotation.y * deg2rad));
				quaternion rot_x(vec4(1, 0, 0, target_camera_rotation.x * deg2rad));
				quaternion rot_z(vec4(0, 0, 1, target_camera_rotation.z * deg2rad));
				quaternion target_rotation = rot_y * rot_x * rot_z;

				quaternion rot2_y(vec4(0, 1, 0, camera_rotation_from.y * deg2rad));
				quaternion rot2_x(vec4(1, 0, 0, camera_rotation_from.x * deg2rad));
				quaternion rot2_z(vec4(0, 0, 1, camera_rotation_from.z * deg2rad));
				quaternion from_rotation = rot2_y * rot2_x * rot2_z;

				// Use the alpha to mix the two quaternions, making them blend.
				quaternion mixed_rotation = mix(from_rotation, target_rotation, alpha);

				// Convert the resulting quaternion back into X Y Z rotation.
				vec3 front = Mult(mixed_rotation, vec3(0,0,1));
				vec3 new_mixed_rotation;
				new_mixed_rotation.y = atan2(front.x, front.z) * 180.0f / PI;
				new_mixed_rotation.x = asin(front[1]) * -180.0f / PI;
				vec3 up = Mult(mixed_rotation, vec3(0,1,0));
				vec3 expected_right = normalize(cross(front, vec3(0,1,0)));
				vec3 expected_up = normalize(cross(expected_right, front));
				new_mixed_rotation.z = atan2(dot(up,expected_right), dot(up, expected_up)) * 180.0f / PI;

				vec3 mixed_translation = mix(camera_translation_from, target_camera_position, alpha);

				SetCameraTransform(mixed_translation, new_mixed_rotation);
				SetDialogueDOF();

				if(camera_transition_timer >= transition_duration){
					camera_transition_timer = 0.0;
					triggered = false;
					return true;
				}

				camera_transition_timer += time_step;
				return false;
			}else if(camera_transition == fade_transition){
				if(wait_for_fade){
					//Waiting for the fade to end.
					return false;
				}else if(!triggered){
					//Starting the fade.
					level.SendMessage("drika_dialogue_fade_out_in " + this_hotspot.GetID());
					wait_for_fade = true;
					triggered = true;
					return false;
				}else{
					SetCameraTransform(target_camera_position, target_camera_rotation);
					SetDialogueDOF();
					triggered = false;
					return true;
				}
			}else{
				SetCameraTransform(target_camera_position, target_camera_rotation);
				SetDialogueDOF();
				return true;
			}
		}else if(dialogue_function == fade_to_black){
			SetFadeToBlack();
			return true;
		}else if(dialogue_function == settings){
			SetDialogueSettings();
			return true;
		}else if(dialogue_function == start){
			return StartDialogue();
		}else if(dialogue_function == end){
			return EndDialogue();
		}else if(dialogue_function == set_actor_dialogue_control){
			SetActorDialogueControl();
			return true;
		}else if(dialogue_function == choice){
			if(ShowChoiceDialogue(false)){
				Reset();
				return false;
			}
			return false;
		}else if(dialogue_function == clear_dialogue){
			level.SendMessage("allow_dialogue_move_in");
			level.SendMessage("drika_dialogue_clear");
			return true;
		}

		return false;
	}

	bool ShowChoiceDialogue(bool preview){
		if(!choice_ui_added){
			level.SendMessage("drika_dialogue_clear_say");
			current_choice = 0;
			choice_ui_added = true;
			string merged_choices;

			available_choices.resize(0);

			for(int i = 0; i < nr_choices; i++){
				// Skip adding this choice to the UI when parameter checking is enabled AND the parameter check returns false.
				if(check_parameters[i] && !CheckParameterValue(parameter_names[i], parameter_check_values[i], parameter_check_modes[i])){
					continue;
				}

				available_choices.insertLast(i);
				merged_choices += "\"" + ReplaceQuotes(choice_texts[i]) + "\"";
			}

			level.SendMessage("drika_dialogue_choice " + this_hotspot.GetID() + " " +  merged_choices);
		}


		if(!preview){
			if(GetInputPressed(0, "up") || GetInputPressed(0, "menu_up")){
				SelectChoice(current_choice - 1);
			}else if(GetInputPressed(0, "down") || GetInputPressed(0, "menu_down")){
				SelectChoice(current_choice + 1);
			}else if(GetInputPressed(0, "jump") || GetInputPressed(0, "skip_dialogue")){
				return GoToCurrentChoice();
			}else{
				for(int i = 0; i < int(available_choices.size()); i++){
					int choice_index = available_choices[i];
					int number = i + 1;
					if(GetInputPressed(0, "" + number)){
						return PickChoice(choice_elements[choice_index].GetTargetLineIndex());
					}
				}	
			}
		}

		if(triggered){
			return GoToCurrentChoice();
		}

		return false;
	}

	bool CheckParameterValue(string parameter_name, string parameter_check_value, check_modes check_mode){
		// If the variable doesn't exist then don't add the choice.
		SavedLevel@ data = save_file.GetSavedLevel("drika_data");
		if(data.GetValue("[" + parameter_name + "]") != "true"){
			return false;
		}

		string parameter_value = data.GetValue(parameter_name);

		switch(check_mode){
			case check_mode_equals:
				//Because floats aren't exact, we simply check if they are very very close.
				if(IsFloat(parameter_value) == true){
					return CheckEpsilon(parameter_value, parameter_check_value) == 1;
				}else{
					return parameter_value == parameter_check_value;
				}
			case check_mode_notequals:
				if(IsFloat(parameter_value) == true){
					return CheckEpsilon(parameter_value, parameter_check_value) != 1;
				}else{
					return parameter_value != parameter_check_value;
				}
			case check_mode_greaterthan:
				if(IsFloat(parameter_value) == true){
					return atof(parameter_value) > atof(parameter_check_value);
				}
				break;
			case check_mode_lessthan:
				if(IsFloat(parameter_value) == true){
					return atof(parameter_value) < atof(parameter_check_value);
				}
				break;
			case check_mode_variable:
				if(IsFloat(parameter_value) == true){
					return CheckEpsilon(parameter_value, parameter_check_value) == 1;
				}else{
					return parameter_value == parameter_check_value;
				}
			case check_mode_notvariable:
				if(IsFloat(parameter_value) == true){
					return CheckEpsilon(parameter_value, parameter_check_value) != 1;
				}else{
					return parameter_value != parameter_check_value;
				}
			case check_mode_greaterthanvariable:
				if(IsFloat(parameter_value) == true && IsFloat(parameter_check_value) == true){
					return atof(parameter_value) > atof(parameter_check_value);
				}
				break;
			case check_mode_lessthanvariable:
				if(IsFloat(parameter_value) == true && IsFloat(parameter_check_value) == true){
					return atof(parameter_value) < atof(parameter_check_value);
				}
				break;
		}

		return false;
	}

	void SelectChoice(int target_choice){
		if(target_choice > -1 && target_choice < int(available_choices.size()) && target_choice != current_choice){
			current_choice = target_choice;
			level.SendMessage("drika_dialogue_choice_select " + current_choice);
			if(ui_sounds){
				string hover_sound_path_extension = hover_sound_path.substr(hover_sound_path.length() - 3, 3);
				hover_sound_path_extension == "xml"?PlaySoundGroup(hover_sound_path):PlaySound(hover_sound_path);
			}
		}
	}

	bool GoToCurrentChoice(){
		int choice_index = available_choices[current_choice];
		int new_target_line = choice_elements[choice_index].GetTargetLineIndex();
		return PickChoice(new_target_line);
	}

	bool PickChoice(int new_target_line){
		if(new_target_line < 0 || new_target_line >= int(drika_elements.size())){
			Log(warning, "The Go to line isn't valid in the dialogue choice " + new_target_line);
			return false;
		}

		current_line = new_target_line;
		display_index = drika_indexes[new_target_line];

		if(ui_sounds){
			string click_sound_path_extension = click_sound_path.substr(click_sound_path.length() - 3, 3);
			click_sound_path_extension == "xml"?PlaySoundGroup(click_sound_path):PlaySound(click_sound_path);
		}

		return true;
	}

	bool StartDialogue(){
		if(wait_for_fade){
			//Waiting for the fade to end.
			return false;
		}else if(use_fade && !triggered){
			//Starting the fade.
			level.SendMessage("drika_dialogue_fade_out_in " + this_hotspot.GetID());
			wait_for_fade = true;
			triggered = true;
			return false;
		}else{
			//Fade is done, continue with the next function.
			in_dialogue_mode = true;
			triggered = false;
			level.SendMessage("drika_dialogue_start");
			return true;
		}
	}

	bool EndDialogue(){
		if(wait_for_fade){
			return false;
		}else if(level.DialogueCameraControl() && (use_fade && !triggered)){
			level.SendMessage("drika_dialogue_fade_out_in " + this_hotspot.GetID());
			wait_for_fade = true;
			triggered = true;
			return false;
		}else{
			in_dialogue_mode = false;
			triggered = false;
			ClearDialogueActors();
			level.SendMessage("drika_dialogue_end");
			return true;
		}
	}

	void SetDialogueSettings(){
		text_speed = 1.0f / dialogue_text_speed;
		string msg = "drika_dialogue_set_settings ";
		msg += dialogue_layout + " ";
		msg += dialogue_text_font + " ";
		msg += dialogue_text_size + " ";
		msg += dialogue_text_color.x + " ";
		msg += dialogue_text_color.y + " ";
		msg += dialogue_text_color.z + " ";
		msg += dialogue_text_color.a + " ";
		msg += dialogue_text_shadow + " ";
		msg += use_voice_sounds + " ";
		msg += show_names + " ";
		msg += show_avatar + " ";
		msg += dialogue_location + " ";
		level.SendMessage(msg);
	}

	void SetFadeToBlack(){
		string msg = "drika_dialogue_fade_to_black ";
		msg += target_fade_to_black + " ";
		msg += fade_to_black_duration;
		triggered = true;
		level.SendMessage(msg);
	}

	void ResetFadeToBlack(){
		string msg = "drika_dialogue_clear_fade_to_black ";
		msg += target_fade_to_black;
		level.SendMessage(msg);
	}

	void SetCameraTransform(vec3 position, vec3 rotation){
		string msg = "drika_dialogue_set_camera_position ";
		msg += floor(rotation.x * 100.0f + 0.5f) / 100.0f + " ";
		msg += floor(rotation.y * 100.0f + 0.5f) / 100.0f + " ";
		msg += floor(rotation.z * 100.0f + 0.5f) / 100.0f + " ";
		msg += position.x + " ";
		msg += position.y + " ";
		msg += position.z + " ";
		msg += target_camera_zoom + " ";

		msg += add_camera_shake + " ";
		if(add_camera_shake){
			msg += position_shake_max_distance + " ";
			msg += position_shake_slerp_speed + " ";
			msg += position_shake_interval + " ";
			msg += rotation_shake_max_distance + " ";
			msg += rotation_shake_slerp_speed + " ";
			msg += rotation_shake_interval + " ";
		}

		array<Object@> look_targets = look_at_target.GetTargetObjects();
		msg += enable_look_at_target + " ";
		msg += ((look_targets.size() > 0)?look_targets[0].GetID():-1) + " ";

		array<Object@> move_targets = move_with_target.GetTargetObjects();
		msg += enable_move_with_target + " ";
		msg += ((move_targets.size() > 0)?move_targets[0].GetID():-1) + " ";

		level.SendMessage(msg);
	}

	void SetActorOmniscient(){
		array<MovementObject@> targets = target_select.GetTargetMovementObjects();

		for(uint i = 0; i < targets.size(); i++){
			targets[i].ReceiveScriptMessage("set_omniscient " + omniscient);
		}
	}

	void SetActorHeadDirection(){
		array<MovementObject@> targets = target_select.GetTargetMovementObjects();
		float weight = (placeholder.GetScale().x - 0.1f) * 4.0f;
		vec3 direction = placeholder.GetTranslation();

		for(uint i = 0; i < targets.size(); i++){
			targets[i].ReceiveScriptMessage("set_head_target " + direction.x + " " + direction.y + " " + direction.z + " " + weight);
			if(!EditorModeActive()){
				targets[i].Execute("ai_look_target = vec3(" +  direction.x + ", " + direction.y + ", " + direction.z + ");");
				targets[i].Execute("ai_look_override_time = time + 10.0;");
				targets[i].Execute("blinking = false;blink_progress = 1.0f;blink_delay = 1.0f;blink_amount = 0.0f;blink_mult = 1.0f;");
				targets[i].Execute("Update(60);");
			}
		}
	}

	void SetActorTorsoDirection(){
		array<MovementObject@> targets = target_select.GetTargetMovementObjects();
		vec3 direction = placeholder.GetTranslation();
		float weight = (placeholder.GetScale().x - 0.1f) * 4.0f;

		for(uint i = 0; i < targets.size(); i++){
			targets[i].ReceiveScriptMessage("set_torso_target " + direction.x + " " + direction.y + " " + direction.z + " " + weight);
		}
	}

	void SetActorEyeDirection(){
		array<MovementObject@> targets = target_select.GetTargetMovementObjects();
		vec3 direction = placeholder.GetTranslation();
		float blink = (placeholder.GetScale().x - 0.05f) / 0.05f;

		for(uint i = 0; i < targets.size(); i++){
			targets[i].ReceiveScriptMessage("set_eye_dir " + direction.x + " " + direction.y + " " + direction.z + " " + blink);
		}
	}

	void SetActorSettings(){
		string msg = "drika_dialogue_set_actor_settings ";
		msg += "\"" + actor_name + "\" ";
		msg += dialogue_color.x + " " + dialogue_color.y + " " + dialogue_color.z + " " + dialogue_color.a + " ";
		msg += voice + " ";
		msg += avatar_path;
		level.SendMessage(msg);
	}

	int dialogue_progress = 0;
	float dialogue_timer = 0.0;

	bool UpdateSayDialogue(bool preview){
		//Some setup operations that only need to be done once.
		if(say_started == false){
			say_started = true;
			dialogue_progress = 0;
			dialogue_timer = 0.0;

			if(voice_over_path != ""){
				voice_over_sound_id = PlaySound(voice_over_path);
			}

			dialogue_script = InterpDialogueScript(say_text);

			level.SendMessage("drika_dialogue_clear_say");

			string nametag = "\"" + ReplaceQuotes(actor_name) + "\"";
			level.SendMessage("drika_dialogue_add_say " + nametag + " " + "\"" + ReplaceQuotes(say_text) + "\"");

			return false;
		}else if(say_started == true && (GetInputPressed(0, "skip_dialogue") && !preview)){
			SetTargetTalking(false);
			SkipWholeDialogue();
			return false;
		}else if(dialogue_done == true){
			if((GetInputPressed(0, "attack") || auto_continue) && !preview){
				level.SendMessage("drika_dialogue_skip");
				return true;
			}else{
				return false;
			}
		}else if(say_started == true){
			if(GetInputPressed(0, "attack") && !preview){
				level.SendMessage("drika_dialogue_skip");
				level.SendMessage("drika_dialogue_show_skip_message");
				SetTargetTalking(false);

				for(uint i = dialogue_progress; i < dialogue_script.size(); i++){
					DialogueScriptEntry@ entry = dialogue_script[i];
					dialogue_progress += 1;

					switch(entry.script_entry_type){
						case character_entry:
							level.SendMessage("drika_dialogue_next");
							break;
						case wait_entry:
							//Skip the wait entries.
							level.SendMessage("drika_dialogue_next");
							break;
						default:
							level.SendMessage("drika_dialogue_next");
							break;
					}
				}

				dialogue_done = true;
				return false;
			}else if(dialogue_timer <= 0.0){
				for(uint i = dialogue_progress; i < dialogue_script.size(); i++){
					DialogueScriptEntry@ entry = dialogue_script[i];
					dialogue_progress += 1;

					switch(entry.script_entry_type){
						case character_entry:
							level.SendMessage("drika_dialogue_next");
							dialogue_timer = text_speed;
							SetTargetTalking(true);
							return false;
						case wait_entry:
							level.SendMessage("drika_dialogue_next");
							dialogue_timer = entry.wait;
							SetTargetTalking(false);
							return false;
						default:
							level.SendMessage("drika_dialogue_next");
							break;
					}
				}

				//At the end of the dialogue.
				level.SendMessage("drika_dialogue_show_skip_message");
				dialogue_done = true;
				SetTargetTalking(false);
				return false;
			}

			dialogue_timer -= time_step;
			return false;
		}
		return false;
	}

	void SetTargetTalking(bool talking){
		array<MovementObject@> targets = target_select.GetTargetMovementObjects();
		for(uint i = 0; i < targets.size(); i++){
			targets[i].ReceiveScriptMessage(talking?"start_talking":"stop_talking");
		}
	}

	void SkipWholeDialogue(){
		Reset();
		while(true){
			//No end dialogue was found and the script has ended.
			if(current_line == int(drika_indexes.size() - 1)){
				script_finished = true;
				break;
			}else{
				current_line += 1;
				display_index = drika_indexes[current_line];
			}

			//When ending a dialogue just let it trigger.
			if(GetCurrentElement().drika_element_type == drika_dialogue){
				DrikaDialogue@ dialogue_function = cast<DrikaDialogue@>(GetCurrentElement());
				if(dialogue_function.dialogue_function == end){
					break;
				}else if(dialogue_function.dialogue_function == choice){
					//A dialogue choice can not be skipped.
					break;
				}
			}

			//Skip any dialogue say or sounds.
			if(GetCurrentElement().drika_element_type == drika_dialogue){
				DrikaDialogue@ dialogue_function = cast<DrikaDialogue@>(GetCurrentElement());
				if(dialogue_function.dialogue_function == say || dialogue_function.dialogue_function == fade_to_black){
					continue;
				}
			}else if(GetCurrentElement().drika_element_type == drika_go_to_line){
				continue;
			}else if(GetCurrentElement().drika_element_type == drika_play_sound){
				continue;
			}else if(GetCurrentElement().drika_element_type == drika_animation){
				DrikaAnimation@ animation_function = cast<DrikaAnimation@>(GetCurrentElement());
				animation_function.SkipAnimation();
				continue;
			}

			GetCurrentElement().Trigger();
		}
	}

	void SetActorPosition(){
		array<MovementObject@> targets = target_select.GetTargetMovementObjects();

		for(uint i = 0; i < targets.size(); i++){
			/* targets[i].rigged_object().anim_client().Reset(); */
			/* targets[i].Execute("dialogue_anim = \"Data/Animations/r_actionidle.anm\";"); */

			if(targets[i].GetIntVar("state") == _ragdoll_state){
				targets[i].Execute("WakeUp(_wake_stand);" +
									"EndGetUp();" +
									"unragdoll_time = 0.0f;");
			}

			targets[i].ReceiveScriptMessage("set_rotation " + target_actor_rotation);
			targets[i].ReceiveScriptMessage("set_dialogue_position " + target_actor_position.x + " " + target_actor_position.y + " " + target_actor_position.z);
			targets[i].Execute("this_mo.velocity = vec3(0.0, 0.0, 0.0);");
			targets[i].Execute("FixDiscontinuity();");
		}
	}

	void SetActorAnimation(){
		if(!FileExists(target_actor_animation)){
			Log(error, "Animation file does not exist : " + target_actor_animation);
			return;
		}
		array<MovementObject@> targets = target_select.GetTargetMovementObjects();

		for(uint i = 0; i < targets.size(); i++){
			string flags = "0";
			if(anim_mirrored) flags += " | _ANM_MIRRORED";
			if(anim_mobile) flags += " | _ANM_MOBILE";
			if(anim_super_mobile) flags += " | _ANM_SUPER_MOBILE";
			if(anim_from_start) flags += " | _ANM_FROM_START";

			string roll_fade = use_ik ? "roll_ik_fade = 0.0f;" : "roll_ik_fade = 1.0f;";
			string callback = "";
			if(wait_anim_end) callback += "in_animation = true;this_mo.rigged_object().anim_client().SetAnimationCallback(\"void EndAnim()\");";

			if(targets[i].GetIntVar("state") == _ragdoll_state){
				targets[i].Execute("WakeUp(_wake_stand);" +
									"EndGetUp();" +
									"unragdoll_time = 0.0f;");
				targets[i].Execute("FixDiscontinuity();");
			}

			targets[i].rigged_object().anim_client().Reset();
			targets[i].Execute(roll_fade + "this_mo.SetAnimation(\"" + target_actor_animation + "\", " + 10.0f + ", " + flags + ");dialogue_anim = \"" + target_actor_animation + "\";" + callback);
		}
	}

	void SetActorDialogueControl(){
		array<MovementObject@> targets = target_select.GetTargetMovementObjects();

		for(uint i = 0; i < targets.size(); i++){
			if(dialogue_control){
				AddDialogueActor(targets[i]);
			}else{
				RemoveDialogueActor(targets[i]);
			}
		}
	}
}
