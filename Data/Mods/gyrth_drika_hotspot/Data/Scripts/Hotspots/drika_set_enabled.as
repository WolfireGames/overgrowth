class DrikaSetEnabled : DrikaElement{
	bool enabled;
	array<BeforeValue> before_values;

	DrikaSetEnabled(JSONValue params = JSONValue()){
		enabled = GetJSONBool(params, "enabled", true);

		@target_select = DrikaTargetSelect(this, params);
		target_select.target_option = id_option | name_option | character_option | reference_option | team_option | batch_option | box_select_option;

		drika_element_type = drika_set_enabled;
		connection_types = {_env_object, _hotspot_object};
		has_settings = true;
	}

	void PostInit(){
		target_select.PostInit();
	}

	JSONValue GetCheckpointData(){
		JSONValue data;
		data["triggered"] = triggered;
		if(triggered){
			data["target_ids"] = JSONValue(JSONarrayValue);
			data["target_enabled"] = JSONValue(JSONarrayValue);
			data["before_values"] = JSONValue(JSONarrayValue);

			for(uint i = 0; i < before_values.size(); i++){
				data["before_values"].append(before_values[i].bool_value);
			}

			array<Object@> targets = target_select.GetTargetObjects();
			for(uint i = 0; i < targets.size(); i++){
				data["target_ids"].append(targets[i].GetID());
				data["target_enabled"].append(targets[i].GetEnabled());
			}
		}
		return data;
	}

	void SetCheckpointData(JSONValue data = JSONValue()){
		triggered = data["triggered"].asBool();
		if(triggered){
			JSONValue target_ids = data["target_ids"];
			JSONValue target_enabled = data["target_enabled"];
			JSONValue before = data["before_values"];

			//Also sync the before values if this function has been triggered.
			before_values.resize(0);
			for(uint i = 0; i < before.size(); i++){
				before_values.insertLast(BeforeValue());
				before_values[i].bool_value = before[i].asBool();
			}

			//if references are used for the target then the id might have changed. So just apply normally.
			if(target_select.identifier_type == reference){
				ApplyEnabled(false);
			}else{
				for(uint i = 0; i < target_ids.size(); i++){
					if(!ObjectExists(target_ids[i].asInt())){
						continue;
					}
					Object@ obj = ReadObjectFromID(target_ids[i].asInt());
					obj.SetEnabled(target_enabled[i].asBool());
				}
			}
		}
	}

	JSONValue GetSaveData(){
		JSONValue data;
		data["enabled"] = JSONValue(enabled);
		target_select.SaveIdentifier(data);
		return data;
	}

	void Delete(){
		Reset();
		target_select.Delete();
	}

	string GetDisplayString(){
		return "SetEnabled " + target_select.GetTargetDisplayText() + " " + enabled;
	}

	void StartSettings(){
		target_select.CheckAvailableTargets();
	}

	void DrawSettings(){
		float option_name_width = 120.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Target");
		ImGui_NextColumn();
		ImGui_NextColumn();

		target_select.DrawSelectTargetUI();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Set Enabled To");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		ImGui_Checkbox("###Set To", enabled);
	}

	bool Trigger(){
		if(!triggered){
			GetBeforeParam();
		}
		triggered = true;
		return ApplyEnabled(false);
	}

	void GetBeforeParam(){
		array<Object@> targets = target_select.GetTargetObjects();
		before_values.resize(0);
		for(uint i = 0; i < targets.size(); i++){
			before_values.insertLast(BeforeValue());
			before_values[i].bool_value = targets[i].GetEnabled();
		}
	}

	void DrawEditing(){
		array<Object@> targets = target_select.GetTargetObjects();
		for(uint i = 0; i < targets.size(); i++){
			DebugDrawLine(targets[i].GetTranslation(), this_hotspot.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
		}
	}

	bool ApplyEnabled(bool reset){
		array<Object@> targets = target_select.GetTargetObjects();
		for(uint i = 0; i < targets.size(); i++){
			targets[i].SetEnabled(reset?before_values[i].bool_value:enabled);
		}
		return true;
	}

	void Reset(){
		if(triggered){
			triggered = false;
			ApplyEnabled(true);
		}
	}
}
