class DrikaWait : DrikaElement{
	float timer;
	int duration;

	DrikaWait(JSONValue params = JSONValue()){
		duration = GetJSONInt(params, "duration", 1000);
		timer = duration / 1000.0;

		drika_element_type = drika_wait;
		has_settings = true;
	}

	JSONValue GetSaveData(){
		JSONValue data;
		data["duration"] = JSONValue(duration);
		return data;
	}

	string GetDisplayString(){
		return "Wait " + duration;
	}

	void DrawSettings(){
		float option_name_width = 130.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Wait miliseconds");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();

		ImGui_PushItemWidth(second_column_width);
		ImGui_InputInt("###Duration", duration);
		ImGui_PopItemWidth();
		ImGui_NextColumn();
	}

	bool Trigger(){
		if(timer <= 0.0){
			Reset();
			return true;
		}else{
			timer -= time_step;
			return false;
		}
	}

	void Reset(){
		timer = duration / 1000.0;
	}
}
