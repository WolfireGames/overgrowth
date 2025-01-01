enum input_types{ 			button_pressed = 0,
							type_text = 1,
							button_down = 2,
							button_up = 3
				};

enum multi_input_types{ 	multi_button_pressed = 0,
							multi_button_down = 1,
							multi_button_up = 2
				};

enum input_modes{ 			single_input = 0,
							multi_input = 1
				};

enum input_identifiers{		up = 0,
							down = 1,
							left = 2,
							right = 3,
							jump = 4,
							crouch = 5,
							slow = 6,
							use_item = 7,
							drop = 8,
							skip_dialogue = 9,
							attack = 10,
							grab = 11,
							walk = 12,
							input_other = 13
					};

class InputData{
	input_identifiers input_identifier;
	string input_bind_name;
	string input_bind;

	InputData(input_identifiers _input_identifier, string _input_bind_name, string _input_bind){
		input_identifier = _input_identifier;
		input_bind_name = _input_bind_name;
		input_bind = _input_bind;
	}
}

class DrikaOnInput : DrikaElement{
	input_types input_type;
	multi_input_types multi_input_type;
	input_modes input_mode;
	array<string> input_names;
	int current_input_type;
	int current_input_mode;

	int input_index;
	InputData@ input;
	string other_input;

	int input_count;

	array<int> multi_input_buttons;
	array<string> multi_input_others;
	array<int> multi_input_types;
	array<DrikaGoToLineSelect@> input_lines;

	string typed_text;
	int character_index = 0;
	array<string> input_array;
	bool use_prompt;
	bool custom_prompt;
	string custom_prompt_path;
	float prompt_size;
	vec4 prompt_color = vec4(0.5, 0.5, 0.5, 1.0);
	string current_prompt_icon;

	array<string> input_type_names = { "Button Pressed", "Type Text", "Button Down", "Button Up" };
	array<string> multi_input_type_names = { "Button Pressed", "Button Down", "Button Up" };
	array<string> input_mode_names = { "Single Input", "Multi Input" };

	array<string> multi_inputs = { "up","down","left","right","jump","crouch","slow","item","drop","skip_dialogue","attack","grab","walk","w" };

	array<InputData@> inputs = {	InputData(up, "Forward", "up"),
									InputData(down, "Backward", "down"),
									InputData(left, "Left", "left"),
									InputData(right, "Right", "right"),
									InputData(jump, "Jump", "jump"),
									InputData(crouch, "Crouch", "crouch"),
									InputData(slow, "Slow Motion", "slow"),
									InputData(use_item, "Equip/sheathe item", "item"),
									InputData(drop, "Throw/pick-up item", "drop"),
									InputData(skip_dialogue, "Skip dialogue", "skip_dialogue"),
									InputData(attack, "Attack", "attack"),
									InputData(grab, "Grab", "grab"),
									InputData(walk, "Walk", "walk"),
									InputData(input_other, "Other", "w")
								};

	DrikaOnInput(JSONValue params = JSONValue()){
		placeholder.Load(params);
		placeholder.name = "Input Prompt Helper";

		input_type = input_types(GetJSONInt(params, "input_type", button_pressed));
		multi_input_type = multi_input_types(GetJSONInt(params, "multi_input_type", button_pressed));
		current_input_type = input_type;

		input_mode = input_modes(GetJSONInt(params, "input_mode", single_input));
		current_input_mode = input_mode;

		input_count = GetJSONInt(params, "input_count", 1);

		multi_input_buttons = GetJSONIntArray(params, "multi_input_buttons", array<int> = { 0 });
		multi_input_types = GetJSONIntArray(params, "multi_input_types", array<int> = { 0 });
		multi_input_others = GetJSONStringArray(params, "multi_input_others", array<string> = { "w" });

		input_lines.resize(input_count);

		for(int i = 0; i < input_count; i++){
			@input_lines[i] = DrikaGoToLineSelect("inputline" + i, params);
		}

		typed_text = GetJSONString(params, "typed_text", "Drika's Hotspot");
		input_index = GetJSONInt(params, "input_identifier", up);
		other_input = GetJSONString(params, "other_input", "w");
		use_prompt = GetJSONBool(params, "use_prompt", false);
		custom_prompt = GetJSONBool(params, "custom_prompt", false);
		custom_prompt_path = GetJSONString(params, "custom_prompt_path", "Data/Textures/UI/keyboard/f.png");
		prompt_size = GetJSONFloat(params, "prompt_size", 0.25);

		GetInputData();
		CreateInputList();
		SetInputArray();

		@target_select = DrikaTargetSelect(this, params);
		target_select.target_option = id_option | name_option | character_option | reference_option;

		connection_types = {_movement_object};

		drika_element_type = drika_on_input;
		has_settings = true;
	}

	void PostInit(){
		if(use_prompt){
			placeholder.Retrieve();
			GetIcon();
		}
		target_select.PostInit();

		for(int i = 0; i < input_count; i++){
			input_lines[i].PostInit();
		}
	}

	bool UsesPlaceholderObject(){
		return use_prompt;
	}

	void GetInputData(){
		for(uint i = 0; i < inputs.size(); i++){
			if(inputs[i].input_identifier == input_identifiers(input_index)){
				@input = inputs[i];
				break;
			}
		}
	}

	void CreateInputList(){
		for(uint i = 0; i < inputs.size(); i++){
			if(inputs[i].input_identifier >= int(input_names.size())){
				input_names.resize(inputs[i].input_identifier + 1);
			}
			input_names[inputs[i].input_identifier] = inputs[i].input_bind_name;
		}
	}

	void SetInputArray(){
		input_array.resize(0);
		for(uint i = 0; i < typed_text.length(); i++){
			if(typed_text.substr(i, 1) == " "){
				input_array.insertLast("space");
			}else{
				input_array.insertLast(typed_text.substr(i, 1));
			}
		}
	}

	JSONValue GetSaveData(){
		JSONValue data;
		data["input_type"] = JSONValue(input_type);
		data["multi_input_type"] = JSONValue(multi_input_type);
		data["input_mode"] = JSONValue(input_mode);
		placeholder.Save(data);

		data["input_count"] = JSONValue(input_count);

		data["multi_input_buttons"] = JSONValue(JSONarrayValue);
		data["multi_input_types"] = JSONValue(JSONarrayValue);
		data["multi_input_others"] = JSONValue(JSONarrayValue);

		for (int i = 0; i < input_count; i++){
			input_lines[i].SaveGoToLine(data);
		}

		for (int i = 0; i < input_count; i++)
			(data["multi_input_buttons"])[i] = JSONValue(multi_input_buttons[i]);

		for (int i = 0; i < input_count; i++)
			(data["multi_input_types"])[i] = JSONValue(multi_input_types[i]);

		for (int i = 0; i < input_count; i++)
			(data["multi_input_others"])[i] = JSONValue(multi_input_others[i]);

		if(input_type == type_text){
			data["typed_text"] = JSONValue(typed_text);
		}else{
			data["input_identifier"] = JSONValue(input.input_identifier);
			if(input.input_identifier == input_other){
				data["other_input"] = JSONValue(other_input);
			}
			data["use_prompt"] = JSONValue(use_prompt);
			if(use_prompt){
				data["prompt_size"] = JSONValue(prompt_size);
				data["custom_prompt"] = JSONValue(custom_prompt);
				if(custom_prompt){
					data["custom_prompt_path"] = JSONValue(custom_prompt_path);
				}
			}
		}
		target_select.SaveIdentifier(data);

		return data;
	}

	string GetDisplayString(){
		string display_string;

		if(input_mode == single_input){
			display_string = "OnInput " + target_select.GetTargetDisplayText() + " " + input_type_names[input_type] + " ";

			if(input_type == type_text){
				display_string += "\"" + typed_text + "\"";
			}else{
				display_string += "\"" + ((input.input_identifier == input_other)?(other_input):input.input_bind_name) + "\"";
			}
		}else{
			display_string = "Check for multiple inputs from " + target_select.GetTargetDisplayText();
		}

		return display_string;
	}

	void ApplySettings(){
		SetInputArray();
	}

	void StartSettings(){
		target_select.CheckAvailableTargets();
	}

	void DrawSettings(){

		float option_name_width = 120.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Target Character");
		ImGui_NextColumn();
		ImGui_NextColumn();

		target_select.DrawSelectTargetUI();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Input Mode");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		ImGui_PushItemWidth(second_column_width);
		if(ImGui_Combo("##Input Mode", current_input_mode, input_mode_names, input_mode_names.size())){
			input_mode = input_modes(current_input_mode);
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		if(input_mode == multi_input){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Number of inputs");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if (ImGui_SliderInt("##Input Count", input_count, 1, 10, "%.0f"))
			{
				if (input_count < 1) input_count = 1;
				if (input_count > 100) input_count = 100;
			}

			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_Separator();
			ImGui_NextColumn();
			ImGui_NextColumn();

			ImGui_Columns(4, false);
			ImGui_SetColumnWidth(0, option_name_width);
			ImGui_SetColumnWidth(1, option_name_width*2);
			ImGui_SetColumnWidth(2, option_name_width*1.5);

			if (int(multi_input_buttons.length()) != input_count){
				multi_input_buttons.resize(input_count);
				multi_input_types.resize(input_count);
				multi_input_others.resize(input_count);

				int old_length = input_lines.length();
				input_lines.resize(input_count);
				for(int i = old_length; i < input_count; i++){
					@input_lines[i] = DrikaGoToLineSelect("inputline" + i);
					input_lines[i].PostInit();
					Log(fatal, "added line " + i);
				}
			}

			for (int i = 0; i < input_count; i++){
				ImGui_AlignTextToFramePadding();
				ImGui_Text("Input " + (i+1) + ":");
				ImGui_NextColumn();
				ImGui_PushItemWidth(ImGui_GetContentRegionAvailWidth()*0.75);
				ImGui_Combo("##Button" + (i+1), multi_input_buttons[(i)], input_names, input_names.size());
				if(multi_input_buttons[(i)] == input_other){
					ImGui_SameLine();
					ImGui_InputText("##Input" + (i+1), multi_input_others[(i)], 64);
				}
				ImGui_PopItemWidth();
				ImGui_NextColumn();
				ImGui_PushItemWidth(ImGui_GetContentRegionAvailWidth());
				ImGui_Combo("##Type" + (i+1), multi_input_types[(i)], multi_input_type_names, multi_input_type_names.size());
				ImGui_PopItemWidth();
				ImGui_NextColumn();
				ImGui_PushItemWidth(ImGui_GetContentRegionAvailWidth());
				input_lines[i].DrawInputGoToLineUI();
				ImGui_PopItemWidth();
			}
		}

		if(input_mode == single_input){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Input Type");
			ImGui_NextColumn();
			second_column_width = ImGui_GetContentRegionAvailWidth();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_Combo("##Input Type", current_input_type, input_type_names, input_type_names.size())){
				input_type = input_types(current_input_type);
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			if(input_type == type_text){
				ImGui_AlignTextToFramePadding();
				ImGui_Text("Input");
				ImGui_NextColumn();
				ImGui_PushItemWidth(second_column_width);
				ImGui_InputText("##Input", typed_text, 64);
				ImGui_PopItemWidth();
				ImGui_NextColumn();
			}else{
				ImGui_AlignTextToFramePadding();
				ImGui_Text("Button");
				ImGui_NextColumn();
				ImGui_PushItemWidth(second_column_width);
				if(ImGui_Combo("##Button", input_index, input_names, input_names.size())){
					GetInputData();
					GetIcon();
				}
				ImGui_PopItemWidth();
				ImGui_NextColumn();

				if(input.input_identifier == input_other){
					ImGui_AlignTextToFramePadding();
					ImGui_Text("Input");
					ImGui_NextColumn();
					ImGui_PushItemWidth(second_column_width);
					if(ImGui_InputText("##Input", other_input, 64)){
						GetIcon();
					}
					ImGui_PopItemWidth();
					ImGui_NextColumn();
				}

				ImGui_AlignTextToFramePadding();
				ImGui_Text("Use prompt");
				ImGui_NextColumn();
				if(ImGui_Checkbox("###Use prompt", use_prompt)){
					GetIcon();
				}
				ImGui_NextColumn();

				if(use_prompt){
					ImGui_AlignTextToFramePadding();
					ImGui_Text("Prompt Size");
					ImGui_NextColumn();
					ImGui_PushItemWidth(second_column_width);
					ImGui_DragFloat("###Prompt Size", prompt_size, 0.001f, 0.0f, 5.0f, "%.2f");
					ImGui_PopItemWidth();
					ImGui_NextColumn();

					ImGui_AlignTextToFramePadding();
					ImGui_Text("Custom Prompt");
					ImGui_NextColumn();
					ImGui_Checkbox("###Custom Prompt", custom_prompt);
					ImGui_NextColumn();

					if(custom_prompt){
						ImGui_AlignTextToFramePadding();
						ImGui_Text("Path");
						ImGui_NextColumn();
						if(ImGui_Button("Set Path")){
							string new_path = "";
							new_path = GetUserPickedReadPath("png", "Data/Textures/UI");
							if(new_path != ""){
								custom_prompt_path = ShortenPath(new_path);
							}
						}
						ImGui_SameLine();
						ImGui_Text(custom_prompt_path);
						ImGui_NextColumn();
					}
				}
			}
		}
	}

	void TargetChanged(){
		GetIcon();
	}

	void DrawEditing(){
		array<MovementObject@> targets = target_select.GetTargetMovementObjects();
		for(uint i = 0; i < targets.size(); i++){
			DebugDrawLine(targets[i].position, this_hotspot.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
		}

		if(use_prompt && (input_type == button_pressed || input_type == button_down)){
			if(placeholder.Exists()){
				DebugDrawLine(placeholder.GetTranslation(), this_hotspot.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
			}else{
				placeholder.Create();
			}
		}else{
			if(placeholder.Exists()){
				placeholder.Remove();
			}
		}
		DrawPrompt();
	}

	void Delete(){
		placeholder.Remove();
		target_select.Delete();
	}

	void Reset(){
		character_index = 0;
	}

	bool Trigger(){
		array<MovementObject@> targets = target_select.GetTargetMovementObjects();
		if(targets.size() == 0){return false;}

		bool one_triggered = false;
		for(uint i = 0; i < targets.size(); i++){
			string check_input;
			if(input_mode == single_input){
				if(input_type == type_text){
					if(GetInputPressed(targets[i].controller_id, input_array[character_index])){
						character_index++;
						if(character_index == int(input_array.size())){
							character_index = 0;
							one_triggered = true;
						}
					}
				}else{

					if(input.input_identifier == input_other){
						check_input = other_input;
					}else{
						check_input = input.input_bind;
					}

					if(input_type == button_pressed && GetInputPressed(targets[i].controller_id, check_input)){
						one_triggered = true;
					}else if(input_type == button_down && GetInputDown(targets[i].controller_id, check_input)){
						one_triggered = true;
						}else if(input_type == button_up && !GetInputDown(targets[i].controller_id, check_input)){
						one_triggered = true;
					}
				}

			}else{
				for(int j = 0; j < input_count; j++){
					multi_input_buttons[(j)] == input_other ? check_input = multi_input_others[j] : check_input = multi_inputs[multi_input_buttons[j]];

					if(multi_input_types[j] == multi_button_pressed && GetInputPressed(targets[i].controller_id, check_input)){
						current_line = input_lines[j].GetTargetLineIndex();
						display_index = drika_indexes[input_lines[j].GetTargetLineIndex()];

					}else if(multi_input_types[j] == multi_button_down && GetInputDown(targets[i].controller_id, check_input)){
						current_line = input_lines[j].GetTargetLineIndex();
						display_index = drika_indexes[input_lines[j].GetTargetLineIndex()];

						}else if(multi_input_types[j] == multi_button_up && !GetInputDown(targets[i].controller_id, check_input)){
						current_line = input_lines[j].GetTargetLineIndex();
						display_index = drika_indexes[input_lines[j].GetTargetLineIndex()];
					}
				}
				one_triggered = false;
			}
		}
		DrawPrompt();
		return one_triggered;
	}

	void DrawPrompt(){
		if(use_prompt){
			if(placeholder.Exists()){
				if(custom_prompt){
					DebugDrawBillboard(custom_prompt_path, placeholder.GetTranslation(), prompt_size, prompt_color, _delete_on_draw);
				}else{
					DebugDrawBillboard(current_prompt_icon, placeholder.GetTranslation(), prompt_size, prompt_color, _delete_on_draw);
				}
			}
		}
	}

	void GetIcon(){
		if(!use_prompt){
			return;
		}
		array<MovementObject@> targets = target_select.GetTargetMovementObjects();
		//When the target MO is -1 then just get the keyboard icon so that it has something to render.
		if(targets.size() == 0){
			current_prompt_icon = GetKeyboardIcon();
		}else{
			for(uint i = 0; i < targets.size(); i++){
				current_prompt_icon = (targets[i].controller_id == 0)?GetKeyboardIcon():GetControllerIcon();
			}
		}
	}

	string GetKeyboardIcon(){
		string bind = input.input_bind;
		string binding_value = (input.input_identifier != input_other)?GetBindingValue("key", bind):other_input;
		string path = "Data/Textures/UI/keyboard/" + binding_value + ".png";

		if(FileExists(path)){
			return path;
		}

		return "Data/UI/spawner/thumbs/Hotspot/empty.png";
	}

	string GetControllerIcon(){
		string bind = input.input_bind;
		string binding_value = (input.input_identifier != input_other)?GetBindingValue("gamepad_" + GetConfigValueInt("menu_player_config"), bind):other_input;
		string path = "Data/Textures/UI/controller/" + binding_value + ".png";

		if(FileExists(path)){
			return path;
		}

		return "Data/UI/spawner/thumbs/Hotspot/empty.png";
	}

}
