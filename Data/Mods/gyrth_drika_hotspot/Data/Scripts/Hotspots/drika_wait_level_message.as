class DrikaWaitLevelMessage : DrikaElement{
	string message;
	bool received_message = false;

	DrikaWaitLevelMessage(JSONValue params = JSONValue()){
		message = GetJSONString(params, "message", "continue_drika_hotspot");

		drika_element_type = drika_wait_level_message;
		has_settings = true;
	}

	JSONValue GetSaveData(){
		JSONValue data;
		data["message"] = JSONValue(message);
		return data;
	}

	string GetDisplayString(){
		return "WaitLevelMessage " + message;
	}

	void DrawSettings(){

		float option_name_width = 120.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Wait for message");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		ImGui_PushItemWidth(second_column_width);
		ImGui_InputText("###Message", message, 128);
		ImGui_PopItemWidth();
		ImGui_NextColumn();
	}

	void ReceiveMessage(string _message){
		if(_message == message){
			Log(info, "Received correct message " + message);
			received_message = true;
		}
	}

	bool Trigger(){
		if(received_message){
			received_message = false;
			return true;
		}else{
			return false;
		}
	}

	void Reset(){
		received_message = false;
	}
}
