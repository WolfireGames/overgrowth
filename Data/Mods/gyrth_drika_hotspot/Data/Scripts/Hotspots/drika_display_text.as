class DrikaDisplayText : DrikaElement{
	string display_message;
	int font_size;
	string font_path;

	DrikaDisplayText(JSONValue params = JSONValue()){
		display_message = GetJSONString(params, "display_message", "Drika Display Message");
		font_size = GetJSONInt(params, "font_size", 10);
		font_path = GetJSONString(params, "font_path", "Data/Fonts/Cella.ttf");

		drika_element_type = drika_display_text;
		has_settings = true;
	}

	JSONValue GetSaveData(){
		JSONValue data;
		data["display_message"] = JSONValue(display_message);
		data["font_size"] = JSONValue(font_size);
		data["font_path"] = JSONValue(font_path);
		return data;
	}

	string GetDisplayString(){
		array<string> split_message = display_message.split("\n");
		return "DisplayText " + split_message[0];
	}

	void StartEdit(){
		ShowText(ReplaceVariablesFromText(display_message), font_size, font_path);
	}

	void EditDone(){
		ShowText("", font_size, font_path);
	}

	void DrawSettings(){
		float option_name_width = 120.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Font Path");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		if(ImGui_Button("Set Font Path")){
			string new_path = GetUserPickedReadPath("ttf", "Data/Fonts");
			if(new_path != ""){
				new_path = ShortenPath(new_path);
				array<string> path_split = new_path.split("/");
				string file_name = path_split[path_split.size() - 1];
				string file_extension = file_name.substr(file_name.length() - 3, 3);

				if(file_extension == "ttf" || file_extension == "TTF"){
					font_path = new_path;
					ShowText(ReplaceVariablesFromText(display_message), font_size, font_path);
				}else{
					DisplayError("Font issue", "Only ttf font files are supported.");
				}
			}
		}
		ImGui_SameLine();
		ImGui_Text(font_path);
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Font Size");
		ImGui_NextColumn();
		ImGui_PushItemWidth(second_column_width);
		if(ImGui_SliderInt("##Font Size", font_size, 0, 100, "%.0f")){
			ShowText(ReplaceVariablesFromText(display_message), font_size, font_path);
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		ImGui_NextColumn();
		ImGui_AlignTextToFramePadding();
		ImGui_TextWrapped("Entering words between {braces} will cause that word to be interpreted as a variable.");
		ImGui_TextWrapped("If you want to display a word between braces on screen, just add a backslash in front of that \\{word}.");
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Text");
		ImGui_NextColumn();
		ImGui_SetTextBuf(display_message);
		if(ImGui_InputTextMultiline("##TEXT", vec2(-1.0, -1.0))){
			display_message = ImGui_GetTextBuf();
			ShowText(ReplaceVariablesFromText(display_message), font_size, font_path);
		}
		ImGui_NextColumn();
	}

	void Reset(){
		if(triggered){
			ShowText("", font_size, font_path);
		}
	}

	bool Trigger(){
		if(!triggered){
			triggered = true;
		}
		ShowText(ReplaceVariablesFromText(display_message), font_size, font_path);
		return true;
	}
}
