class DrikaLoadLevel : DrikaElement{
	string level_path;

	DrikaLoadLevel(JSONValue params = JSONValue()){
		level_path = GetJSONString(params, "level_path", "Data/Levels/nothing.xml");
		drika_element_type = drika_load_level;
		has_settings = true;
	}

	JSONValue GetSaveData(){
		JSONValue data;
		data["level_path"] = JSONValue(level_path);
		return data;
	}

	string GetDisplayString(){
		return "LoadLevel " + level_path;
	}

	void DrawSettings(){

		float option_name_width = 120.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Level Path");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();

		if(ImGui_Button("Set Level Path")){
			string new_path = GetUserPickedReadPath("xml", "Data/Levels");
			if(new_path != ""){
				level_path = ShortenPath(new_path);
			}
		}
		ImGui_SameLine();
		ImGui_Text(level_path);
	}

	bool Trigger(){
		LoadLevel(level_path);
		return true;
	}
}
