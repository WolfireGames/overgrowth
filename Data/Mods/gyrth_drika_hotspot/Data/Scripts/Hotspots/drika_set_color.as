enum color_types{ 	object_tint = 0,
					object_palette_color = 1
				};

class DrikaSetColor : DrikaElement{
	int num_palette_colors = 0;
	array<string> palette_names;
	int palette_slot;
	int current_palette_slot;
	vec3 before_color;
	vec3 after_color;
	vec3 starting_color;
	int current_color_type;
	color_types color_type;
	array<string> palette_indexes;
	array<string> color_type_choices = {"Tint", "Palette Color"};
	float overbright;
	bool transition;
	IMTweenType tween_type;
	int current_tween_type;
	float transition_duration;
	float timer;

	DrikaSetColor(JSONValue params = JSONValue()){
		color_type = color_types(GetJSONInt(params, "color_type", 0));
		current_color_type = color_type;
		palette_slot = GetJSONInt(params, "palette_slot", 0);
		current_palette_slot = palette_slot;
		after_color = GetJSONVec3(params, "after_color", vec3(1));
		overbright = GetJSONFloat(params, "overbright", 0.0f);
		transition = GetJSONBool(params, "transition", false);
		transition_duration = GetJSONFloat(params, "transition_duration", 1.0f);
		tween_type = IMTweenType(GetJSONInt(params, "ease_function", linearTween));
		current_tween_type = tween_type;

		@target_select = DrikaTargetSelect(this, params);
		target_select.target_option = id_option | name_option | character_option | reference_option | team_option;

		connection_types = {_movement_object, _env_object, _item_object};
		drika_element_type = drika_set_color;
		has_settings = true;
	}

	void PostInit(){
		target_select.PostInit();
		GetBeforeColor();
	}

	JSONValue GetSaveData(){
		JSONValue data;
		data["color_type"] = JSONValue(color_type);
		data["palette_slot"] = JSONValue(palette_slot);
		data["overbright"] = JSONValue(overbright);
		data["after_color"] = JSONValue(JSONarrayValue);
		data["after_color"].append(after_color.x);
		data["after_color"].append(after_color.y);
		data["after_color"].append(after_color.z);
		data["transition"] = JSONValue(transition);
		if(transition){
			data["ease_function"] = JSONValue(tween_type);
			data["transition_duration"] = JSONValue(transition_duration);
		}
		target_select.SaveIdentifier(data);
		return data;
	}

	void Delete(){
		SetColor(true);
		target_select.Delete();
    }

	string GetDisplayString(){
		string display_string = "SetColor " + target_select.GetTargetDisplayText() + " " + Vec3ToString(after_color);

		if(transition){
			display_string += " Transition " + transition_duration;
		}

		return display_string;
	}

	void StartEdit(){
		GetNumPaletteColors();
		/* DrikaElement::StartEdit(); */
		SetColor(false);
	}

	void EditDone(){
		SetColor(true);
	}

	void GetNumPaletteColors(){
		if(color_type == object_palette_color){
			array<Object@> targets = target_select.GetTargetObjects();
			palette_indexes.resize(0);
			num_palette_colors = 0;
			for(uint i = 0; i < targets.size(); i++){
				if(targets[i].GetType() == _movement_object){
					num_palette_colors = targets[i].GetNumPaletteColors();

					MovementObject@ char = ReadCharacterID(targets[i].GetID());
					level.SendMessage("drika_read_file " + hotspot.GetID() + " " + index + " " + char.char_path + " " + "character_file");

					for(int j = 0; j < num_palette_colors; j++){
						palette_indexes.insertLast("" + j);
					}
				}
			}

			while(num_palette_colors != 0 && palette_slot >= num_palette_colors){
				palette_slot -= 1;
				current_palette_slot = palette_slot;
			}
		}
	}

	void ReceiveMessage(string message, string identifier){
		if(identifier == "character_file"){
			string obj_path = GetStringBetween(message, "obj_path = \"", "\"");
			level.SendMessage("drika_read_file " + hotspot.GetID() + " " + index + " " + obj_path + " " + "object_file");
		}else if(identifier == "object_file"){
			palette_names.resize(0);
			string red = GetStringBetween(message, "label_red=\"", "\"");
			string green = GetStringBetween(message, "label_green=\"", "\"");
			string blue = GetStringBetween(message, "label_blue=\"", "\"");
			string alpha = GetStringBetween(message, "label_alpha=\"", "\"");

			if(red != "") palette_names.insertLast(red);
			if(green != "") palette_names.insertLast(green);
			if(blue != "") palette_names.insertLast(blue);
			if(alpha != "") palette_names.insertLast(alpha);
		}
	}

	void PreTargetChanged(){
		SetColor(true);
	}

	void TargetChanged(){
		GetNumPaletteColors();
		GetBeforeColor();
		SetColor(false);
	}

	void StartSettings(){
		target_select.CheckAvailableTargets();
	}

	void DrawSettings(){
		float option_name_width = 145.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Target");
		ImGui_NextColumn();
		ImGui_NextColumn();

		target_select.DrawSelectTargetUI();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Color Type");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		ImGui_PushItemWidth(second_column_width);
		if(ImGui_Combo("###Color Type", current_color_type, color_type_choices, color_type_choices.size())){
			SetColor(true);
			color_type = color_types(current_color_type);
			GetNumPaletteColors();
			GetBeforeColor();
			SetColor(false);
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		if(color_type == object_palette_color){
			if(num_palette_colors == 0){
				return;
			}
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Palette Slot");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_Combo("###Palette Slot", current_palette_slot, palette_names, palette_names.size())){
				SetColor(true);
				palette_slot = current_palette_slot;
				GetBeforeColor();
				SetColor(false);
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Color");
		ImGui_NextColumn();
		ImGui_PushItemWidth(second_column_width);
		if(ImGui_ColorEdit3("###Color", after_color)){
			SetColor(false);
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Overbright");
		ImGui_NextColumn();
		ImGui_PushItemWidth(second_column_width);
		if(ImGui_SliderFloat("##Overbright", overbright, 0.0f, 10.0f, "%.1f")){
			SetColor(false);
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Transition");
		ImGui_NextColumn();
		ImGui_PushItemWidth(second_column_width);
		ImGui_Checkbox("###Transition", transition);
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		if(transition){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Transition Duration");
			ImGui_NextColumn();

			ImGui_PushItemWidth(second_column_width);
			if(ImGui_DragFloat("###Transition Duration", transition_duration, 0.01f, 0.0f, 5.0f, "%.3f")){

			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Easing Function");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);

			if(ImGui_BeginCombo("###Easing Function", tween_types[current_tween_type], ImGuiComboFlags_HeightLarge)){
				for(uint i = 0; i < tween_types.size(); i++){
					if(ImGui_Selectable(tween_types[i], current_tween_type == int(i))){
						current_tween_type = i;
						tween_type = IMTweenType(current_tween_type);
					}
					DrawTweenGraph(tween_type);
				}
				ImGui_EndCombo();
			}

			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}
	}

	void DrawEditing(){
		array<Object@> targets = target_select.GetTargetObjects();
		for(uint i = 0; i < targets.size(); i++){
			if(targets[i].GetType() == _movement_object){
				MovementObject@ char = ReadCharacterID(targets[i].GetID());
				DebugDrawLine(char.position, this_hotspot.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
			}else{
				DebugDrawLine(targets[i].GetTranslation(), this_hotspot.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
			}
		}
	}

	bool Trigger(){
		bool done = SetColor(false);
		triggered = true;
		return done;
	}

	void GetBeforeColor(){
		array<Object@> targets = target_select.GetTargetObjects();
		for(uint i = 0; i < targets.size(); i++){
			if(color_type == object_palette_color){
				if(targets[i].GetType() == _movement_object && targets[i].GetNumPaletteColors() > palette_slot){
					before_color = targets[i].GetPaletteColor(palette_slot);
				}
			}else if(color_type == object_tint){
				before_color = targets[i].GetTint();
			}
		}
	}

	void GetStartingColor(){
		array<Object@> targets = target_select.GetTargetObjects();
		for(uint i = 0; i < targets.size(); i++){
			if(color_type == object_palette_color){
				if(targets[i].GetType() == _movement_object && targets[i].GetNumPaletteColors() > palette_slot){
					starting_color = targets[i].GetPaletteColor(palette_slot);
				}
			}else if(color_type == object_tint){
				starting_color = targets[i].GetTint();
			}
		}
	}

	bool SetColor(bool reset){
		if(transition && !reset){
			return SetColorTransition();
		}else{
			array<Object@> targets = target_select.GetTargetObjects();
			float multiplier = 1.0 + overbright;

			for(uint i = 0; i < targets.size(); i++){
				if(color_type == object_palette_color){
					if(targets[i].GetType() == _movement_object && targets[i].GetNumPaletteColors() > palette_slot){
						targets[i].SetPaletteColor(palette_slot, reset?before_color:after_color*multiplier);
					}
				}else if(color_type == object_tint){
					targets[i].SetTint(reset?before_color:after_color*multiplier);
				}
			}

			return true;
		}
	}

	bool SetColorTransition(){
		array<Object@> targets = target_select.GetTargetObjects();
		float multiplier = 1.0 + overbright;

		if(!triggered){
			GetStartingColor();
			timer = 0.0;
		}

		timer += time_step;

		vec3 transition_color = mix(starting_color, after_color, ApplyTween((timer / transition_duration), IMTweenType(tween_type)));

		for(uint i = 0; i < targets.size(); i++){
			if(color_type == object_palette_color){
				if(targets[i].GetType() == _movement_object && targets[i].GetNumPaletteColors() > palette_slot){
					targets[i].SetPaletteColor(palette_slot, transition_color * multiplier);
				}
			}else if(color_type == object_tint){
				targets[i].SetTint(transition_color * multiplier);
			}
		}

		return timer >= transition_duration;
	}

	void Reset(){
		if(triggered){
			SetColor(true);
			triggered = false;
		}
	}
}
