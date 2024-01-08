enum checkpoint_modes 	{
							save = 0,
							load = 1
						};

class DrikaCheckpoint : DrikaElement{
	array<string> mode_names = {"Save", "Load"};
	checkpoint_modes checkpoint_mode;
	int current_checkpoint_mode;
	int current_save_data;
	bool wait_for_fade = false;
	bool use_fade = true;

	DrikaCheckpoint(JSONValue params = JSONValue()){
		checkpoint_mode = checkpoint_modes(GetJSONInt(params, "checkpoint_mode", save));
		current_checkpoint_mode = checkpoint_mode;
		drika_element_type = drika_checkpoint;
		has_settings = true;
	}

	void PostInit(){

	}

	void ReceiveMessage(array<string> messages){
		if(messages[0] == "fade_out_done"){
			wait_for_fade = false;
			triggered = true;
		}
	}

	void Reset(){
		wait_for_fade = false;
		triggered = false;
	}

	JSONValue GetSaveData(){
		JSONValue data;
		data["checkpoint_mode"] = JSONValue(checkpoint_mode);
		return data;
	}

	string GetDisplayString(){
		string display_string = "Checkpoint ";
		display_string += mode_names[checkpoint_mode];

		return display_string;
	}

	void DrawSettings(){
		float option_name_width = 75.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Mode");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		ImGui_PushItemWidth(second_column_width);
		if(ImGui_Combo("##Mode", current_checkpoint_mode, mode_names, mode_names.size())){
			checkpoint_mode = checkpoint_modes(current_checkpoint_mode);
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		if(checkpoint_mode == load){

		}else if(checkpoint_mode == save){

		}
	}

	void SaveCheckpoint(){
		triggered = true;
	}

	bool LoadCheckpoint(){
		if(wait_for_fade){
			//Waiting for the fade to end.
			return false;
		}else if(use_fade && !triggered){
			//Starting the fade.
			level.SendMessage("drika_dialogue_fade_out_in " + this_hotspot.GetID());
			wait_for_fade = true;
			return false;
		}else{
			triggered = true;
			//Fade is done, continue with the next function.
			return true;
		}
	}

	bool Trigger(){
		if(checkpoint_mode == load){
			return LoadCheckpoint();
		}else if(checkpoint_mode == save){
			SaveCheckpoint();
		}
		return true;
	}

	void PostTrigger(){
		if(triggered){
			triggered = false;
			if(checkpoint_mode == load){
				string msg = "drika_load_checkpoint";
				level.SendMessage(msg);
			}else if(checkpoint_mode == save){
				string msg = "drika_save_checkpoint";
				level.SendMessage(msg);
			}
		}
	}
}
