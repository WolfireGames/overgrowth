enum drika_input_box_modes	{
								everything,
								onlynumbers,
								onlyletters
							};

class DrikaUserInterface : DrikaElement{
	ui_functions ui_function;
	drika_input_box_modes drika_input_box_mode;
	int current_ui_function;
	string image_path;
	string fullscreen_image_path;

	string button_image_path;
	bool ui_sounds;
	string hover_sound_path;
	string click_sound_path;

	string input_image_path;
	int input_max_length;
	int current_input_box_mode;

	DrikaGoToLineSelect@ button_line;

	ivec2 size;
	ivec2 position;
	ivec2 position_offset;
	ivec2 size_offset;
	ivec2 max_offset;
	float rotation;
	vec4 color;
	bool keep_aspect;
	array<string> content;
	string text_content;
	string button_text;
	string input_text; //The actual text content stored
	string input_variable; //The name of the variable used to store it in
	string ui_element_identifier;
	bool ui_element_added = false;
	string font_name;
	int font_size;
	vec4 font_color;
	bool shadowed;
	DrikaUserInterface@ font_element = null;
	array<DrikaUserInterface@> text_elements;
	bool animated;
	float animation_speed;

	bool use_fade_in;
	int fade_in_duration;
	int fade_in_tween_type;

	bool use_move_in;
	int move_in_duration;
	int move_in_tween_type;
	ivec2 move_in_offset;

	bool use_fade_out;
	int fade_out_duration;
	int fade_out_tween_type;

	bool use_move_out;
	int move_out_duration;
	int move_out_tween_type;
	ivec2 move_out_offset;

	array<string> ui_function_names =		{
												"Clear",
												"Image",
												"Text",
												"Font",
												"Button",
												"Input Box",
												"Fullscreen Image"
											};

	array<string> input_box_mode_names =	{
												"All Inputs",
												"Only Numbers",
												"Only Letters"
											};

	DrikaUserInterface(JSONValue params = JSONValue()){
		ui_function = ui_functions(GetJSONInt(params, "ui_function", ui_clear));
		current_ui_function = ui_function;

		image_path = GetJSONString(params, "image_path", "Textures/ui/menus/credits/overgrowth.png");
		fullscreen_image_path = GetJSONString(params, "fullscreen_image_path", "Data/Textures/drika_vignette.tga");

		button_image_path = GetJSONString(params, "button_image_path", "Textures/ui/menus/main/button-diamond.png");

		input_image_path = GetJSONString(params, "input_image_path", "Textures/ui/menus/main/button-back-rect.png");
		input_max_length = GetJSONInt(params, "input_max_length", 100);
		drika_input_box_mode = drika_input_box_modes(GetJSONInt(params, "drika_input_box_mode", everything));
		current_input_box_mode = drika_input_box_mode;

		@button_line = DrikaGoToLineSelect("button_line", params);
		ui_sounds = GetJSONBool(params, "ui_sounds", false);
		hover_sound_path = GetJSONString(params, "hover_sound_path", "Data/Sounds/ui_hover.xml");
		click_sound_path = GetJSONString(params, "click_sound_path", "Data/Sounds/ui_click.xml");

		rotation = GetJSONFloat(params, "rotation", 0.0);
		position = GetJSONIVec2(params, "position", ivec2(ui_snap_scale, ui_snap_scale) * 5);
		color = GetJSONVec4(params, "color", vec4(1.0, 1.0, 1.0, 1.0));
		keep_aspect = GetJSONBool(params, "keep_aspect", false);
		size = GetJSONIVec2(params, "size", ivec2(720 - (720 % ui_snap_scale), 255 - (255 % ui_snap_scale)));
		position_offset = GetJSONIVec2(params, "position_offset", ivec2(0, 0));
		size_offset = GetJSONIVec2(params, "size_offset", ivec2(0, 0));

		font_name = GetJSONString(params, "font_name", "edosz");
		font_size = GetJSONInt(params, "font_size", 75);
		font_color = GetJSONVec4(params, "font_color", vec4(1.0, 1.0, 1.0, 1.0));
		shadowed = GetJSONBool(params, "shadowed", true);
		animated = GetJSONBool(params, "animated", false);
		animation_speed = GetJSONFloat(params, "animation_speed", 60.0);

		use_fade_in = GetJSONBool(params, "use_fade_in", false);
		fade_in_duration = GetJSONInt(params, "fade_in_duration", 1000);
		fade_in_tween_type = GetJSONInt(params, "fade_in_tween_type", 0);

		use_move_in = GetJSONBool(params, "use_move_in", false);
		move_in_duration = GetJSONInt(params, "move_in_duration", 1000);
		move_in_tween_type = GetJSONInt(params, "move_in_tween_type", 0);
		move_in_offset = GetJSONIVec2(params, "move_in_offset", ivec2(100, 100));

		use_fade_out = GetJSONBool(params, "use_fade_out", false);
		fade_out_duration = GetJSONInt(params, "fade_out_duration", 1000);
		fade_out_tween_type = GetJSONInt(params, "fade_out_tween_type", 0);

		use_move_out = GetJSONBool(params, "use_move_out", false);
		move_out_duration = GetJSONInt(params, "move_out_duration", 1000);
		move_out_tween_type = GetJSONInt(params, "move_out_tween_type", 0);
		move_out_offset = GetJSONIVec2(params, "move_out_offset", ivec2(100, 100));

		text_content = GetJSONString(params, "text_content", "Example Text");
		button_text = GetJSONString(params, "button_text", "Click Here");
		input_variable = GetJSONString(params, "input_variable", "drika_input_box");
		// This identifier is used to get the messages between the hotspot and levelscript to the right objects.
		// So for example this Text function is linked to a UIText element in drika_controller's ui_elements array.
		ui_element_identifier = GetUniqueID();

		drika_element_type = drika_user_interface;
		has_settings = true;
	}

	void PostInit(){
		UpdateExternalResource();
		// The font needs to be available from the start so that during editing the text are using the correct font.
		if(ui_function == ui_font){
			AddUIElement();
		}
		SendUIInstruction("set_z_order", {index});
		button_line.PostInit();

		if(CheckParamExists(input_variable) == true){
			input_text = ReadParamValue(input_variable);
		}else{
			input_text = "";
		}
	}

	// Checkpoints are saved and loaded in drika_controller.as levelscript.
	// The UI Elements that were on screen during a save are restored in this levelscript,
	// so the only thing this function needs to keep track of is if the element was added or not.
	JSONValue GetCheckpointData(){
		JSONValue data;
		data["ui_element_added"] = ui_element_added;
		return data;
	}

	void SetCheckpointData(JSONValue data = JSONValue()){
		//The current ui elements are handled by the levelscript (drika_controller) so no need to add them here.
		ui_element_added = data["ui_element_added"].asBool();
	}

	void ReorderDone(){
		UpdateExternalResource();
		// The Z order is used to draw elements on top of each other.
		// To make this simple for the user we use the function index, so Text at index 8 is drawn below Text at index9 for example.
		SendUIInstruction("set_z_order", {index});
	}

	// Buttons and text use a font function that is higher on the DHS function list.
	// This function is called when the user is reordering the function list or when this function is added to the list.
	// This can be when the level loads or when the user adds a new User Interface function.
	void UpdateExternalResource(){
		if(ui_function == ui_text || ui_function == ui_button || ui_function == ui_input){
			DrikaUserInterface@ new_font_element = GetPreviousUIElementOfType({ui_font});
			// Register this Text or Button with the Font so that it can send messages back when the font changes settings.
			if(new_font_element !is font_element){
				if(font_element !is null){
					font_element.RemoveTextElement(this);
				}
				@font_element = @new_font_element;
				if(font_element !is null){
					font_element.AddTextElement(this);
				}
				//If the ui element is already on screen (while editing) then update the font.
				if(ui_element_added){
					FontHasChanged();
				}
			}
		}
	}

	// Update behaviors are effects like fading and moving. They can be added and deleted when the user is editing the UI element as well.
	void SendInUpdateBehaviour(){
		if(use_fade_in){
			AddUpdateBehavior("fade_in", fade_in_duration, fade_in_tween_type);
		}
		if(use_move_in){
			AddUpdateBehavior("move_in", move_in_duration, move_in_tween_type, move_in_offset);
		}
	}

	void SendOutUpdateBehaviour(){
		if(use_fade_out){
			AddUpdateBehavior("fade_out", fade_out_duration, fade_out_tween_type);
		}
		if(use_move_out){
			AddUpdateBehavior("move_out", move_out_duration, move_out_tween_type, move_out_offset);
		}
	}

	void SendRemoveUpdatebehaviour(){
		SendUIInstruction("remove_update_behaviour", {"fade_in" + ui_element_identifier});
		SendUIInstruction("remove_update_behaviour", {"move_in" + ui_element_identifier});
		SendUIInstruction("remove_update_behaviour", {"fade_out" + ui_element_identifier});
		SendUIInstruction("remove_update_behaviour", {"move_out" + ui_element_identifier});
	}

	void AddUpdateBehavior(string name, int duration, int tween_type, ivec2 offset = ivec2()){
		SendUIInstruction("add_update_behaviour", {name, duration, tween_type, ui_element_identifier, offset.x, offset.y, show_editor});
	}

	// These next two functions are used by Font to keep track of Text and Button elements.
	void AddTextElement(DrikaUserInterface@ new_text_element){
		for(uint i = 0; i < text_elements.size(); i++){
			if(text_elements[i].ui_element_identifier == new_text_element.ui_element_identifier){
				//Already added to the list.
				return;
			}
		}
		text_elements.insertLast(new_text_element);
	}

	void RemoveTextElement(DrikaUserInterface@ text_element){
		for(uint i = 0; i < text_elements.size(); i++){
			if(text_elements[i].ui_element_identifier == text_element.ui_element_identifier){
				//Found and removed from the list.
				text_elements.removeAt(i);
				break;
			}
		}
	}

	// Loop over the functions backwards, starting at the current index to find a specific functon type.
	// This is used to find a User Interface Font function, but may be useful in the future for other functions.
	DrikaUserInterface@ GetPreviousUIElementOfType(array<ui_functions> function_types){
		for(int i = index - 1; i > -1; i--){
			if(drika_elements[drika_indexes[i]].drika_element_type == drika_user_interface){
				DrikaUserInterface@ found_ui_element = cast<DrikaUserInterface@>(drika_elements[drika_indexes[i]]);
				if(function_types.find(found_ui_element.ui_function) != -1){
					return found_ui_element;
				}
			}
		}
		return null;
	}

	// Loop over all the functions in the current DHS to find User Interface functions.
	array<DrikaUserInterface@> GetAllUIElements(){
		array<DrikaUserInterface@> collection;
		for(uint i = 0; i < drika_elements.size(); i++){
			if(drika_elements[i].drika_element_type == drika_user_interface){
				DrikaUserInterface@ found_ui_element = cast<DrikaUserInterface@>(drika_elements[i]);
				collection.insertLast(found_ui_element);
			}
		}
		return collection;
	}

	// SaveData is not only used to save it to a level xml, but also for checkpoints and create UI Elements in drika_controller.
	// Since it has all the information needed to create the UI, just send over all of it and let the controller pick the data it needs.
	JSONValue GetSaveData(){
		JSONValue data;
		data["ui_function"] = JSONValue(ui_function);
		data["ui_element_identifier"] = JSONValue(ui_element_identifier);

		if(ui_function == ui_image || ui_function == ui_text || ui_function == ui_button || ui_function == ui_fullscreen_image){
			data["use_fade_in"] = JSONValue(use_fade_in);
			if(use_fade_in){
				data["fade_in_duration"] = JSONValue(fade_in_duration);
				data["fade_in_tween_type"] = JSONValue(fade_in_tween_type);
			}
			data["use_move_in"] = JSONValue(use_move_in);
			if(use_move_in){
				data["move_in_duration"] = JSONValue(move_in_duration);
				data["move_in_tween_type"] = JSONValue(move_in_tween_type);
				data["move_in_offset"] = JSONValue(JSONarrayValue);
				data["move_in_offset"].append(move_in_offset.x);
				data["move_in_offset"].append(move_in_offset.y);
			}

			data["use_fade_out"] = JSONValue(use_fade_out);
			if(use_fade_in){
				data["fade_out_duration"] = JSONValue(fade_out_duration);
				data["fade_out_tween_type"] = JSONValue(fade_out_tween_type);
			}
			data["use_move_out"] = JSONValue(use_move_out);
			if(use_move_in){
				data["move_out_duration"] = JSONValue(move_out_duration);
				data["move_out_tween_type"] = JSONValue(move_out_tween_type);
				data["move_out_offset"] = JSONValue(JSONarrayValue);
				data["move_out_offset"].append(move_out_offset.x);
				data["move_out_offset"].append(move_out_offset.y);
			}
		}

		if(ui_function == ui_image){
			data["image_path"] = JSONValue(image_path);
			data["keep_aspect"] = JSONValue(keep_aspect);
			data["rotation"] = JSONValue(rotation);
			data["position"] = JSONValue(JSONarrayValue);
			data["position"].append(position.x);
			data["position"].append(position.y);
			data["color"] = JSONValue(JSONarrayValue);
			data["color"].append(color.x);
			data["color"].append(color.y);
			data["color"].append(color.z);
			data["color"].append(color.a);
			data["size"] = JSONValue(JSONarrayValue);
			data["size"].append(size.x);
			data["size"].append(size.y);
			data["size_offset"] = JSONValue(JSONarrayValue);
			data["size_offset"].append(size_offset.x);
			data["size_offset"].append(size_offset.y);
			data["position_offset"] = JSONValue(JSONarrayValue);
			data["position_offset"].append(position_offset.x);
			data["position_offset"].append(position_offset.y);
			data["animated"] = JSONValue(animated);
			data["animation_speed"] = JSONValue(animation_speed);
		}else if(ui_function == ui_text){
			data["rotation"] = JSONValue(rotation);
			data["position"] = JSONValue(JSONarrayValue);
			data["position"].append(position.x);
			data["position"].append(position.y);
			data["text_content"] = JSONValue(text_content);
			data["use_fade_in"] = JSONValue(use_fade_in);
		}else if(ui_function == ui_font){
			data["font_name"] = JSONValue(font_name);
			data["font_size"] = JSONValue(font_size);
			data["font_color"] = JSONValue(JSONarrayValue);
			data["font_color"].append(font_color.x);
			data["font_color"].append(font_color.y);
			data["font_color"].append(font_color.z);
			data["font_color"].append(font_color.a);
			data["shadowed"] = JSONValue(shadowed);
		}else if(ui_function == ui_button){
			data["button_image_path"] = JSONValue(button_image_path);
			data["keep_aspect"] = JSONValue(keep_aspect);
			data["rotation"] = JSONValue(rotation);
			data["position"] = JSONValue(JSONarrayValue);
			data["position"].append(position.x);
			data["position"].append(position.y);
			data["color"] = JSONValue(JSONarrayValue);
			data["color"].append(color.x);
			data["color"].append(color.y);
			data["color"].append(color.z);
			data["color"].append(color.a);
			data["size"] = JSONValue(JSONarrayValue);
			data["size"].append(size.x);
			data["size"].append(size.y);
			data["button_text"] = JSONValue(button_text);
			data["ui_sounds"] = JSONValue(ui_sounds);
			if(ui_sounds){
				data["hover_sound_path"] = JSONValue(hover_sound_path);
				data["click_sound_path"] = JSONValue(click_sound_path);
			}
			button_line.SaveGoToLine(data);
		}else if(ui_function == ui_input){
			data["input_image_path"] = JSONValue(input_image_path);
			data["input_max_length"] = JSONValue(input_max_length);
			data["keep_aspect"] = JSONValue(keep_aspect);
			data["rotation"] = JSONValue(rotation);
			data["position"] = JSONValue(JSONarrayValue);
			data["position"].append(position.x);
			data["position"].append(position.y);
			data["color"] = JSONValue(JSONarrayValue);
			data["color"].append(color.x);
			data["color"].append(color.y);
			data["color"].append(color.z);
			data["color"].append(color.a);
			data["size"] = JSONValue(JSONarrayValue);
			data["size"].append(size.x);
			data["size"].append(size.y);
			data["input_variable"] = JSONValue(input_variable);
			data["drika_input_box_mode"] = JSONValue(drika_input_box_mode);
		}else if(ui_function == ui_fullscreen_image){
			data["fullscreen_image_path"] = JSONValue(fullscreen_image_path);
			data["color"] = JSONValue(JSONarrayValue);
			data["color"].append(color.x);
			data["color"].append(color.y);
			data["color"].append(color.z);
			data["color"].append(color.a);
		}
		return data;
	}

	string GetDisplayString(){
		string display_string = "UserInterface ";
		display_string += ui_function_names[ui_function] + " ";

		if(ui_function == ui_text){
			display_string += ("") + "\"" + text_content + "\"";
		}else if(ui_function == ui_font){
			display_string += font_name + " " + font_size;
		}else if(ui_function == ui_image){
			display_string += "\"" + image_path + "\"";
		}else if(ui_function == ui_button){
			button_line.CheckLineAvailable();
			string display_line = button_line.GetTargetLineIndex();
			string displayed_text = button_text != "" ? button_text : image_path;
			display_string += "\"" + displayed_text + "\", target Line " + display_line;
		}else if(ui_function == ui_input){
			display_string += input_box_mode_names[current_input_box_mode]  + ", target var \"" + input_variable + "\"";
		}else if(ui_function == ui_fullscreen_image){
			display_string += "\"" + fullscreen_image_path + "\"";
		}

		return display_string;
	}

	void DrawSettings(){
		float option_name_width = 140.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("UI Function");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		ImGui_PushItemWidth(second_column_width);
		if(ImGui_Combo("##UI Function", current_ui_function, ui_function_names, ui_function_names.size())){
			if(current_ui_function != ui_function){
				SendRemoveUpdatebehaviour();
				RemoveUIElement();
				ui_function = ui_functions(current_ui_function);
				StartEdit();
				ReorderElements();
			}
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		if(ui_function == ui_image){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Image Path");
			ImGui_NextColumn();
			if(ImGui_Button("Set Image Path")){
				string new_path = GetUserPickedReadPath("png", "Data/Images");
				if(new_path != ""){
					new_path = ShortenPath(new_path);
					//Remove the Data/ in the beginning of the path because IMImage starts in Data/.
					array<string> split_path = new_path.split("/");
					split_path.removeAt(0);
					image_path = join(split_path, "/");
					SendUIInstruction("set_image_path", {image_path});
				}
			}
			ImGui_SameLine();
			ImGui_Text(image_path);
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Animated");
			ImGui_NextColumn();
			if(ImGui_Checkbox("##Animated", animated)){
				SendUIInstruction("set_animated", {animated});
			}
			ImGui_NextColumn();

			if(animated){
				ImGui_AlignTextToFramePadding();
				ImGui_Text("Animation Speed");
				ImGui_NextColumn();
				ImGui_PushItemWidth(second_column_width);
				if(ImGui_SliderFloat("###Animation Speed", animation_speed, 0.0, 60.0f, "%.0f")){
					SendUIInstruction("set_animation_speed", {animation_speed});
				}
				ImGui_PopItemWidth();
				ImGui_NextColumn();
			}

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Position");
			ImGui_NextColumn();
			ImGui_PushItemWidth((second_column_width / 2.0) - 4.0);
			if(ImGui_DragInt("##Position X", position.x, 1.0, 0, 2560, "%.0f")){
				SendUIInstruction("set_position", {position.x, position.y});
			}
			ImGui_SameLine();
			if(ImGui_DragInt("##Position Y", position.y, 1.0, 0, 1440, "%.0f")){
				SendUIInstruction("set_position", {position.x, position.y});
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Size");
			ImGui_NextColumn();
			ImGui_PushItemWidth((second_column_width / 2.0) - 4.0);
			if(ImGui_DragInt("##size_x", size.x, 1.0, 1.0f, 1000, "%.0f")){
				SendUIInstruction("set_size", {size.x, size.y});
			}
			ImGui_SameLine();
			if(ImGui_DragInt("##size_y", size.y, 1.0, 1.0f, 1000, "%.0f")){
				SendUIInstruction("set_size", {size.x, size.y});
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Position Offset");
			ImGui_NextColumn();
			ImGui_PushItemWidth((second_column_width / 2.0) - 4.0);
			if(ImGui_DragInt("##position_offset_x", position_offset.x, 1.0, 0.0f, max_offset.x, "%.0f")){
				SendUIInstruction("set_position_offset", {position_offset.x, position_offset.y});
			}
			ImGui_SameLine();
			if(ImGui_DragInt("##position_offset_y", position_offset.y, 1.0, 0.0f, max_offset.y, "%.0f")){
				SendUIInstruction("set_position_offset", {position_offset.x, position_offset.y});
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Size Offset");
			ImGui_NextColumn();
			ImGui_PushItemWidth((second_column_width / 2.0) - 4.0);
			if(ImGui_DragInt("##size_offset_x", size_offset.x, 1.0, 1.0f, max_offset.x, "%.0f")){
				SendUIInstruction("set_size_offset", {size_offset.x, size_offset.y});
			}
			ImGui_SameLine();
			if(ImGui_DragInt("##size_offset_y", size_offset.y, 1.0, 1.0f, max_offset.y, "%.0f")){
				SendUIInstruction("set_size_offset", {size_offset.x, size_offset.y});
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Rotation");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_SliderFloat("###Rotation", rotation, -360, 360, "%.0f")){
				SendUIInstruction("set_rotation", {rotation});
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Color");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_ColorEdit4("###Color", color, ImGuiColorEditFlags_HEX | ImGuiColorEditFlags_Uint8)){
				SendUIInstruction("set_color", {color.x, color.y, color.z, color.a});
			}
			ImGui_PopItemWidth();

			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Keep aspect ratio");
			ImGui_NextColumn();
			if(ImGui_Checkbox("##Keep aspect ratio", keep_aspect)){
				SendUIInstruction("set_aspect_ratio", {keep_aspect});
			}
			ImGui_NextColumn();
		}else if(ui_function == ui_text){
			ImGui_AlignTextToFramePadding();

			ImGui_NextColumn();
			ImGui_AlignTextToFramePadding();
			ImGui_TextWrapped("Entering words between {braces} will cause that word to be interpreted as a variable.");
			ImGui_TextWrapped("If you want to display a word between braces on screen, just add a backslash in front of that \\{word}.");
			ImGui_NextColumn();

			ImGui_Text("Text");
			ImGui_NextColumn();
			ImGui_SetTextBuf(text_content);
			ImGui_PushItemWidth(second_column_width);

			if(ImGui_InputTextMultiline("##TEXT", vec2(-1.0, ImGui_GetWindowHeight() / 3.0), ImGuiInputTextFlags_AllowTabInput)){
				text_content = ImGui_GetTextBuf();

				for(int i = 0; i < int(text_content.length()); i++)
				{
					if (text_content[i] == "\""[0])
					{
						text_content.insert(i, "\\");
						i++;
					}
				}

				SendUIInstruction("set_content", {("\"" + text_content + " " + "\"")});
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Position");

			ImGui_NextColumn();
			ImGui_PushItemWidth((second_column_width / 2.0) - 4.0);
			if(ImGui_DragInt("##Position X", position.x, 1.0, 0, 2560, "%.0f")){
				SendUIInstruction("set_position", {position.x, position.y});
			}
			ImGui_SameLine();
			if(ImGui_DragInt("##Position Y", position.y, 1.0, 0, 1440, "%.0f")){
				SendUIInstruction("set_position", {position.x, position.y});
			}
			ImGui_PopItemWidth();

			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Rotation");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_SliderFloat("###rotation", rotation, -360, 360, "%.0f")){
				SendUIInstruction("set_rotation", {rotation});
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}else if(ui_function == ui_font){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Font");
			ImGui_NextColumn();
			if(ImGui_Button("Pick Font")){
				string new_path = GetUserPickedReadPath("ttf", "Data/Fonts");
				if(new_path != ""){
					new_path = ShortenPath(new_path);
					array<string> path_split = new_path.split("/");
					string file_name = path_split[path_split.size() - 1];
					string file_extension = file_name.substr(file_name.length() - 3, 3);

					if(file_extension == "ttf" || file_extension == "TTF"){
						font_name = file_name.substr(0, file_name.length() - 4);
						SendUIInstruction("set_font", {font_name});
					}else{
						DisplayError("Font issue", "Only ttf font files are supported.");
					}
				}
			}
			ImGui_SameLine();
			ImGui_Text(font_name);
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Font Color");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_ColorEdit4("###Font Color", font_color, ImGuiColorEditFlags_HEX | ImGuiColorEditFlags_Uint8)){
				SendUIInstruction("set_font_color", {font_color.x, font_color.y, font_color.z, font_color.a});
			}
			ImGui_PopItemWidth();

			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Shadowed");
			ImGui_NextColumn();
			if(ImGui_Checkbox("##Shadowed", shadowed)){
				SendUIInstruction("set_shadowed", {shadowed});
			}

			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Text Size");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_DragInt("###Text Size", font_size, 0.5, 1, 100)){
				SendUIInstruction("set_font_size", {font_size});
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}else if(ui_function == ui_button){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Button Image");
			ImGui_NextColumn();
			if(ImGui_Button("Set Image")){
				string new_path = GetUserPickedReadPath("png", "Data/Images");
				if(new_path != ""){
					new_path = ShortenPath(new_path);
					//Remove the Data/ in the beginning of the path because IMImage starts in Data/.
					array<string> split_path = new_path.split("/");
					split_path.removeAt(0);
					button_image_path = join(split_path, "/");
					SendUIInstruction("set_button_image_path", {button_image_path});
				}
			}
			ImGui_SameLine();
			ImGui_Text(button_image_path);
			ImGui_NextColumn();

			ImGui_Separator();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Position");
			ImGui_NextColumn();
			ImGui_PushItemWidth((second_column_width / 2.0) - 4.0);
			if(ImGui_DragInt("##Button Position X", position.x, 1.0, 0, 2560, "%.0f")){
				SendUIInstruction("set_position", {position.x, position.y});
			}
			ImGui_SameLine();
			if(ImGui_DragInt("##Button Position Y", position.y, 1.0, 0, 1440, "%.0f")){
				SendUIInstruction("set_position", {position.x, position.y});
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Size");
			ImGui_NextColumn();
			ImGui_PushItemWidth((second_column_width / 2.0) - 4.0);
			if(ImGui_DragInt("##size_x", size.x, 1.0, 1.0f, 1000, "%.0f")){
				SendUIInstruction("set_size", {size.x, size.y});
			}
			ImGui_SameLine();
			if(ImGui_DragInt("##size_y", size.y, 1.0, 1.0f, 1000, "%.0f")){
				SendUIInstruction("set_size", {size.x, size.y});
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Rotation");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_SliderFloat("###Button Rotation", rotation, -360, 360, "%.0f")){
				SendUIInstruction("set_rotation", {rotation});
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Color");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_ColorEdit4("###Color", color, ImGuiColorEditFlags_HEX | ImGuiColorEditFlags_Uint8)){
				SendUIInstruction("set_color", {color.x, color.y, color.z, color.a});
			}
			ImGui_PopItemWidth();

			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Keep aspect ratio");
			ImGui_NextColumn();
			if(ImGui_Checkbox("##Button keep aspect ratio", keep_aspect)){
				SendUIInstruction("set_aspect_ratio", {keep_aspect});
			}
			ImGui_NextColumn();
			ImGui_Separator();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Label");
			ImGui_NextColumn();
			ImGui_SetTextBuf(button_text);
			ImGui_PushItemWidth(second_column_width);

			if(ImGui_InputText( "##BUTTON TEXT", ImGuiInputTextFlags_AllowTabInput)){
				button_text = ImGui_GetTextBuf();

				for(int i = 0; i < int(button_text.length()); i++)
				{
					if (button_text[i] == "\""[0])
					{
						button_text.insert(i, "\\");
						i++;
					}
				}

				SendUIInstruction("set_content", {("\"" + button_text + " " + "\"")});
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			button_line.DrawGoToLineUI();

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
		}else if(ui_function == ui_input){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Input Box Image");
			ImGui_NextColumn();
			if(ImGui_Button("Set Box Image")){
				string new_path = GetUserPickedReadPath("png", "Data/Images");
				if(new_path != ""){
					new_path = ShortenPath(new_path);
					//Remove the Data/ in the beginning of the path because IMImage starts in Data/.
					array<string> split_path = new_path.split("/");
					split_path.removeAt(0);
					input_image_path = join(split_path, "/");
					SendUIInstruction("set_input_image_path", {input_image_path});
				}
			}
			ImGui_SameLine();
			ImGui_Text(button_image_path);
			ImGui_NextColumn();

			ImGui_Separator();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Position");
			ImGui_NextColumn();
			ImGui_PushItemWidth((second_column_width / 2.0) - 4.0);
			if(ImGui_DragInt("##Input Position X", position.x, 1.0, 0, 2560, "%.0f")){
				SendUIInstruction("set_position", {position.x, position.y});
			}
			ImGui_SameLine();
			if(ImGui_DragInt("##Input Position Y", position.y, 1.0, 0, 1440, "%.0f")){
				SendUIInstruction("set_position", {position.x, position.y});
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Size");
			ImGui_NextColumn();
			ImGui_PushItemWidth((second_column_width / 2.0) - 4.0);
			if(ImGui_DragInt("##size_x", size.x, 1.0, 1.0f, 1000, "%.0f")){
				SendUIInstruction("set_size", {size.x, size.y});
			}
			ImGui_SameLine();
			if(ImGui_DragInt("##size_y", size.y, 1.0, 1.0f, 1000, "%.0f")){
				SendUIInstruction("set_size", {size.x, size.y});
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Rotation");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_SliderFloat("###Input Rotation", rotation, -360, 360, "%.0f")){
				SendUIInstruction("set_rotation", {rotation});
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Color");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_ColorEdit4("###Color", color, ImGuiColorEditFlags_HEX | ImGuiColorEditFlags_Uint8)){
				SendUIInstruction("set_color", {color.x, color.y, color.z, color.a});
			}
			ImGui_PopItemWidth();

			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Keep aspect ratio");
			ImGui_NextColumn();
			if(ImGui_Checkbox("##Input keep aspect ratio", keep_aspect)){
				SendUIInstruction("set_aspect_ratio", {keep_aspect});
			}
			ImGui_NextColumn();
			ImGui_Separator();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Max Input Length");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_SliderInt("###Max Input Length", input_max_length, 0, 100, "%.0f")){
				SendUIInstruction("set_input_max_length", {input_max_length});
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Input Box Mode");
			ImGui_NextColumn();
			second_column_width = ImGui_GetContentRegionAvailWidth();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_Combo("##Input Box Mode", current_input_box_mode, input_box_mode_names, input_box_mode_names.size())){
				drika_input_box_mode = drika_input_box_modes(current_input_box_mode);
				SendUIInstruction("set_input_box_mode", {drika_input_box_mode});
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Input Variable");
			ImGui_NextColumn();
			ImGui_SetTextBuf(input_variable);
			ImGui_PushItemWidth(second_column_width);

			if(ImGui_InputText( "##INPUT VARIABLE", ImGuiInputTextFlags_AllowTabInput)){
				input_variable = ImGui_GetTextBuf();

				for(int i = 0; i < int(input_variable.length()); i++)
				{
					if (input_variable[i] == "\""[0])
					{
						input_variable.insert(i, "\\");
						i++;
					}
				}

				SendUIInstruction("set_content", {("\"" + input_variable + " " + "\"")});
			}

			if (CheckParamExists(input_variable) == true && input_variable != ""){
				ImGui_NextColumn();
				ImGui_NextColumn();
				ImGui_Text("Variable " + input_variable + " is set to " + ReadParamValue(input_variable));
			}

			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}else if(ui_function == ui_fullscreen_image){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Image Path");
			ImGui_NextColumn();
			if(ImGui_Button("Set Image Path")){
				string new_path = GetUserPickedReadPath("png", "Data/Images");
				if(new_path != ""){
					fullscreen_image_path = ShortenPath(new_path);
					SendUIInstruction("set_image_path", {fullscreen_image_path});
				}
			}
			ImGui_SameLine();
			ImGui_Text(fullscreen_image_path);
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Color");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_ColorEdit4("###Color", color, ImGuiColorEditFlags_HEX | ImGuiColorEditFlags_Uint8)){
				SendUIInstruction("set_color", {color.x, color.y, color.z, color.a});
			}
			ImGui_PopItemWidth();

			ImGui_NextColumn();
		}

		if(ui_function == ui_image || ui_function == ui_text || ui_function == ui_button || ui_function == ui_input || ui_function == ui_fullscreen_image){
			//Fade in UI-------------------------------------------------------------------------------------------------
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Use Fade In");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_Checkbox("##Use Fade In", use_fade_in)){
				SendRemoveUpdatebehaviour();
				SendInUpdateBehaviour();
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			if(use_fade_in){
				DrawSelectTween("Fade In", fade_in_tween_type, fade_in_duration, ivec2());
			}

			if(ui_function != ui_fullscreen_image){
				//Move in UI-------------------------------------------------------------------------------------------------
				ImGui_AlignTextToFramePadding();
				ImGui_Text("Use Move In");
				ImGui_NextColumn();
				ImGui_PushItemWidth(second_column_width);
				if(ImGui_Checkbox("##Use Move In", use_move_in)){
					SendRemoveUpdatebehaviour();
					SendInUpdateBehaviour();
				}
				ImGui_PopItemWidth();
				ImGui_NextColumn();

				if(use_move_in){
					DrawSelectTween("Move In", move_in_tween_type, move_in_duration, move_in_offset);
				}
			}

			//Fade out UI-------------------------------------------------------------------------------------------------
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Use Fade Out");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_Checkbox("##Use Fade Out", use_fade_out)){
				SendRemoveUpdatebehaviour();
				SendOutUpdateBehaviour();
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			if(use_fade_out){
				DrawSelectTween("Fade Out", fade_out_tween_type, fade_out_duration, ivec2());
			}

			if(ui_function != ui_fullscreen_image){
				//Move out UI-------------------------------------------------------------------------------------------------
				ImGui_AlignTextToFramePadding();
				ImGui_Text("Use Move Out");
				ImGui_NextColumn();
				ImGui_PushItemWidth(second_column_width);
				if(ImGui_Checkbox("##Use Move Out", use_move_out)){
					SendRemoveUpdatebehaviour();
					SendOutUpdateBehaviour();
				}
				ImGui_PopItemWidth();
				ImGui_NextColumn();

				if(use_move_out){
					DrawSelectTween("Move Out", move_out_tween_type, move_out_duration, move_out_offset);
				}
			}
		}
	}

	// Tweening is used for all the update behaviors, so to prevent duplicate code,
	// this function draws all of the Dear Imgui tween settings.
	void DrawSelectTween(string name, int &inout tween_type, int &inout duration, ivec2 &inout offset){
		ImGui_AlignTextToFramePadding();
		ImGui_Text(name + " Duration");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();

		ImGui_PushItemWidth(second_column_width);
		if(ImGui_DragInt("##" + name + " Duration", duration, 1.0, 1, 10000)){
			SendRemoveUpdatebehaviour();
			if(name.findFirst("In") != -1){
				SendInUpdateBehaviour();
			}else{
				SendOutUpdateBehaviour();
			}
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text(name + " Tween");
		ImGui_NextColumn();
		ImGui_PushItemWidth(second_column_width);

		if(ImGui_BeginCombo("##" + name + " Tween Type", tween_types[tween_type], ImGuiComboFlags_HeightRegular)){
			for(uint i = 0; i < tween_types.size(); i++){
				if(ImGui_Selectable(tween_types[i], tween_type == int(i))){
					tween_type = i;
					SendRemoveUpdatebehaviour();
					if(name.findFirst("In") != -1){
						SendInUpdateBehaviour();
					}else{
						SendOutUpdateBehaviour();
					}
				}
				DrawTweenGraph(IMTweenType(i));
			}
			ImGui_EndCombo();
		}

		ImGui_PopItemWidth();
		ImGui_NextColumn();

		if(name.findFirst("Move") != -1){
			ImGui_AlignTextToFramePadding();
			ImGui_Text(name + " Offset");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_DragInt2("##" + name + " Offset", offset)){
				SendRemoveUpdatebehaviour();
				if(name.findFirst("In") != -1){
					SendInUpdateBehaviour();
				}else{
					SendOutUpdateBehaviour();
				}
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}
	}

	string ReadParamValue(string key){
		SavedLevel@ data = save_file.GetSavedLevel("drika_data");
		return data.GetValue(key);
	}

	bool CheckParamExists(string key){
		return ReadParamValue("[" + key + "]") == "true";
	}

	// The StartEdit function is called when the user clicks on this function in the DHS list.
	void StartEdit(){
		// Tell the drika_controller that we want to start editing a UI element.
		// This will display the grid if that option is enabled and tells the drika_controller which hotspot id to send messages to.
		SendLevelMessage("drika_edit_ui", {true, hotspot.GetID(), show_grid, ui_snap_scale});
		AddUIElement();
		// This message is send to the ui_element in the drika_controller ui_elements array.
		SendLevelMessage("drika_ui_set_editing", {true});
	}

	bool AddUIElement(){
		// To add the UI Element to screen we send of a bunch of data to the drika_controller.
		// This included everything that is saved to the level xml in JSON **plus** some extra values.
		if(ui_function == ui_clear){
			// To clear the UI we just loop over all the functions and check if it's a User Interface function.
			// Then we request it to remove itself from the screen.
			// This means two different hotspots can display elements without removing each other's content.
			array<DrikaUserInterface@> target_elements = GetAllUIElements();
			for(uint i = 0; i < target_elements.size(); i++){
				//Make sure the fonts are still available when clearing the screen.
				if(target_elements[i].ui_function == ui_image || target_elements[i].ui_function == ui_text || target_elements[i].ui_function == ui_button || target_elements[i].ui_function == ui_input || target_elements[i].ui_function == ui_fullscreen_image){
					target_elements[i].RequestRemoveUIElement();
				}
			}

			// We still need to "add" the clear element to the drika_controller so that it can hide the cursor if a button was shown.
			JSONValue data = GetSaveData();
			data["type"] = JSONValue(ui_clear);
			SendJSONMessage("drika_ui_add_element", data);
		}else if(ui_function == ui_image){
			if(!ui_element_added){
				JSONValue data = GetSaveData();
				data["type"] = JSONValue(ui_image);
				data["index"] = JSONValue(index);
				data["hotspot_id"] = JSONValue(hotspot.GetID());
				SendJSONMessage("drika_ui_add_element", data);
			}
			SendRemoveUpdatebehaviour();
			SendInUpdateBehaviour();
			ui_element_added = true;
		}else if(ui_function == ui_text){
			if(!ui_element_added){
				JSONValue data = GetSaveData();
				data["type"] = JSONValue(ui_text);
				data["index"] = JSONValue(index);
				data["hotspot_id"] = JSONValue(hotspot.GetID());
				if(font_element is null){
					data["font_id"] = JSONValue("");
				}else{
					data["font_id"] = JSONValue(font_element.ui_element_identifier);
				}
				SendJSONMessage("drika_ui_add_element", data);
			}
			ui_element_added = true;
			SendRemoveUpdatebehaviour();
			SendInUpdateBehaviour();
		}else if(ui_function == ui_font){
			if(!ui_element_added){
				ui_element_added = true;
				JSONValue data = GetSaveData();
				data["index"] = JSONValue(index);
				data["type"] = JSONValue(ui_font);
				data["hotspot_id"] = JSONValue(hotspot.GetID());
				SendJSONMessage("drika_ui_add_element", data);
			}
		}else if(ui_function == ui_button){
			if(!ui_element_added){
				JSONValue data = GetSaveData();
				data["type"] = JSONValue(ui_button);
				data["index"] = JSONValue(index);
				data["hotspot_id"] = JSONValue(hotspot.GetID());
				if(font_element is null){
					data["font_id"] = JSONValue("");
				}else{
					data["font_id"] = JSONValue(font_element.ui_element_identifier);
				}
				SendJSONMessage("drika_ui_add_element", data);
			}
			SendRemoveUpdatebehaviour();
			SendInUpdateBehaviour();
			ui_element_added = true;

		}else if(ui_function == ui_input){
			if(!ui_element_added){
				JSONValue data = GetSaveData();
				data["type"] = JSONValue(ui_input);
				data["index"] = JSONValue(index);
				data["hotspot_id"] = JSONValue(hotspot.GetID());
				if(font_element is null){
					data["font_id"] = JSONValue("");
				}else{
					data["font_id"] = JSONValue(font_element.ui_element_identifier);
				}
				SendJSONMessage("drika_ui_add_element", data);
			}
			SendRemoveUpdatebehaviour();
			SendInUpdateBehaviour();
			ui_element_added = true;
		}else if(ui_function == ui_fullscreen_image){
			if(!ui_element_added){
				JSONValue data = GetSaveData();
				data["type"] = JSONValue(ui_fullscreen_image);
				data["index"] = JSONValue(index);
				data["hotspot_id"] = JSONValue(hotspot.GetID());
				SendJSONMessage("drika_ui_add_element", data);
			}
			SendRemoveUpdatebehaviour();
			SendInUpdateBehaviour();
			ui_element_added = true;
		}
		return true;
	}

	void SendJSONMessage(string message_name, JSONValue json_value){
		string msg = message_name + " ";

		JSON data;
		data.getRoot() = json_value;

		// Convert the JSONValue into a string so that it can be send to the level.
		string json_string = data.writeString(false);
		// Level messages strip out the " so add an extra \ to prevent this.
		json_string = join(json_string.split("\""), "\\\"");

		msg += "\"" + json_string + "\"";
		level.SendMessage(msg);
	}

	void SendLevelMessage(string param_1, array<string> params = {}){
		//This message goes to the drika_controller levelscript and then to the correct ui_element.
		string msg = param_1 + " ";
		msg += ui_element_identifier + " ";
		for(uint i = 0; i < params.size(); i++){
			msg += params[i] + " ";
		}
		level.SendMessage(msg);
	}

	void SendUIInstruction(string param_1, array<string> params){
		//This message goes to the drika_controller levelscript and then to the correct ui_element.
		string msg = "drika_ui_instruction ";
		msg += ui_element_identifier + " ";
		msg += param_1 + " ";
		for(uint i = 0; i < params.size(); i++){
			msg += params[i] + " ";
		}
		level.SendMessage(msg);
	}

	void ReadUIInstruction(array<string> instruction){
		// This function comes from the ui_element on screen -> drika_controller levelscript -> drika_hotspot -> here.
		/* Log(warning, "Got instruction " + instruction[0]); */
		// When an Image gets dragged or resized it send a message back to this function to keep size and position the same between classes.
		if(instruction[0] == "set_position"){
			position.x = atoi(instruction[1]);
			position.y = atoi(instruction[2]);
		}else if(instruction[0] == "set_size"){
			size.x = atoi(instruction[1]);
			size.y = atoi(instruction[2]);
		}
	}

	// Receive an event back from the drika_controller like button presses.
	// This can later be expanded upon with interactive elements like slider, text inputs etc.
	void ReadUIFunctionEvent(array<string> instruction){
		if(instruction[0] == "button_clicked"){
			if(button_line !is null && !show_editor){
				current_line = button_line.GetTargetLineIndex();
				display_index = drika_indexes[button_line.GetTargetLineIndex()];

				if(ui_sounds){
					string click_sound_path_extension = click_sound_path.substr(click_sound_path.length() - 3, 3);
					click_sound_path_extension == "xml"?PlaySoundGroup(click_sound_path):PlaySound(click_sound_path);
				}
			}
		}else if(instruction[0] == "button_hovered"){
			if(ui_sounds){
				string hover_sound_path_extension = hover_sound_path.substr(hover_sound_path.length() - 3, 3);
				hover_sound_path_extension == "xml"?PlaySoundGroup(hover_sound_path):PlaySound(hover_sound_path);
			}
		}
	}

	// The user clicks on a different function or closes DHS.
	// Tell drika_controller to stop showing the grid and the UI Element that editing has stopped.
	void EditDone(){
		SendLevelMessage("drika_edit_ui", {false});
		SendLevelMessage("drika_ui_set_editing", {false});
	}

	// The user has edited the settings of this function and closed the Edit window.
	void ApplySettings(){
		SendRemoveUpdatebehaviour();
		// The UI Font should only update once when the settings are closed.
		// Doing this on every settings change can cause issues. Like sliders changing the font size for example.
		// That will delete and create a new font very rapidly.
		if(ui_function == ui_font){
			SendFontHasChanged();
		}
	}

	// Is called when the user presses L or the level is reset by code.
	void Reset(){
		RemoveUIElement();
	}

	void RemoveUIElement(){
		if(ui_element_added){
			if(ui_function == ui_image || ui_function == ui_text || ui_function == ui_font || ui_function == ui_button || ui_function == ui_input || ui_function == ui_fullscreen_image){
				SendLevelMessage("drika_ui_remove_element");
				ui_element_added = false;
			}
		}
	}

	void RequestRemoveUIElement(){
		if(ui_element_added){
			if(ui_function == ui_image || ui_function == ui_text || ui_function == ui_button || ui_function == ui_input || ui_function == ui_fullscreen_image){
				if(use_fade_out || use_move_out){
					SendOutUpdateBehaviour();
					if(!show_editor){
						ui_element_added = false;
					}
				}else{
					RemoveUIElement();
				}
			}
			if(!show_editor){
				// Get a new unique identifier in case any removed elements are fading out.
				ui_element_identifier = GetUniqueID();
			}
		}
	}

	void Delete(){
		RemoveUIElement();
	}

	bool Trigger(){
		return AddUIElement();
	}

	void SendFontHasChanged(){
		// This function is only used by Font.
		// Go over all the Text and Buttons that use this Font and tell them the font settings have changed.
		for(uint i = 0; i < text_elements.size(); i++){
			text_elements[i].FontHasChanged();
		}
	}

	void FontHasChanged(){
		// An empty font identifier is used to tell drika_controller to use the default font.
		if(font_element is null){
			SendUIInstruction("font_changed", {""});
		}else{
			SendUIInstruction("font_changed", {font_element.ui_element_identifier});
		}
	}
}
