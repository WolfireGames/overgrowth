enum level_message_types {
							send_message,
							global_message
						};

class DrikaSendLevelMessage : DrikaElement{
	string message;
	string display_message;
	level_message_types level_message_type;
	int current_message_type;
	array<string> message_type_choices = {"Send Message", "Global Message"};
	string message_list = "go_to_main_menu\nreset\ndisplaytext\nscreen_message\ndisplayhud\nloadlevel\nmake_all_aware\nopen_menu";

	DrikaSendLevelMessage(JSONValue params = JSONValue()){
		message = GetJSONString(params, "message", "continue_drika_hotspot");
		level_message_type = level_message_types(GetJSONInt(params, "level_message_type", send_message));
		current_message_type = level_message_type;

		drika_element_type = drika_send_level_message;
		has_settings = true;
		SetDisplayMessage();
	}

	JSONValue GetSaveData(){
		JSONValue data;
		data["message"] = JSONValue(message);
		data["level_message_type"] = JSONValue(level_message_type);
		return data;
	}

	string GetDisplayString(){
		if(level_message_type == send_message){
			return "SendLevelMessage " + display_message;
		}else{
			return "SendGlobalLevelMessage " + display_message;
		}
	}

	void DrawSettings(){
		float option_name_width = 120.0;
		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Message Type");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();

		ImGui_PushItemWidth(second_column_width);
		if(ImGui_Combo("##Message Type", current_message_type, message_type_choices, message_type_choices.size())){
			level_message_type = level_message_types(current_message_type);
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Level Message");
		ImGui_NextColumn();

		ImGui_PushItemWidth(second_column_width);
		if(ImGui_InputText("###Level Message", message, 128)){
			SetDisplayMessage();
		}
		ImGui_PopItemWidth();

		if(ImGui_IsItemHovered()){
			ImGui_PushStyleColor(ImGuiCol_PopupBg, titlebar_color);
			ImGui_SetTooltip(message_list);
			ImGui_PopStyleColor();
		}

		ImGui_NextColumn();
	}

	void SetDisplayMessage(){
		display_message = join(message.split("\n"), "");
	}

	bool Trigger(){
		if(level_message_type == send_message){
			level.SendMessage(message);
		}else if(level_message_type == global_message){
			SendGlobalMessage(message);
		}
		return true;
	}
}
