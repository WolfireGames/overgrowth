class DrikaSetObjectParam : DrikaElement{
	int current_type;
	string param_name;
	array<BeforeValue@> params_before;

	string string_param_after;
	int int_param_after = 0;
	float float_param_after = 0.0;

	param_types param_type;
	array<string> param_type_choices = {"String", "Integer", "Float"};

	DrikaSetObjectParam(JSONValue params = JSONValue()){
		param_name = GetJSONString(params, "param_name", "drika_param");
		param_type = param_types(GetJSONInt(params, "param_type", 0));
		current_type = param_type;

		@target_select = DrikaTargetSelect(this, params);
		target_select.target_option = id_option | name_option | character_option | reference_option | team_option;

		connection_types = {_env_object, _movement_object};
		drika_element_type = drika_set_object_param;
		has_settings = true;
		InterpParam(params);
	}

	void PostInit(){
		target_select.PostInit();
	}

	JSONValue GetSaveData(){
		JSONValue data;
		data["param_name"] = JSONValue(param_name);
		data["param_type"] = JSONValue(param_type);
		if(param_type == string_param){
			data["param_after"] = JSONValue(string_param_after);
		}else if(param_type == float_param){
			data["param_after"] = JSONValue(float_param_after);
		}else if(param_type == int_param){
			data["param_after"] = JSONValue(int_param_after);
		}
		target_select.SaveIdentifier(data);
		return data;
	}

	void InterpParam(JSONValue _params){
		if(param_type == float_param){
			float_param_after = GetJSONFloat(_params, "param_after", 0.0);
		}else if(param_type == int_param){
			int_param_after = GetJSONInt(_params, "param_after", 0);
		}else if(param_type == string_param){
			string_param_after = GetJSONString(_params, "param_after", "");
		}
	}

	void GetBeforeParam(){
		array<Object@> targets = target_select.GetTargetObjects();
		params_before.resize(0);
		for(uint i = 0; i < targets.size(); i++){
			ScriptParams@ params = targets[i].GetScriptParams();
			params_before.insertLast(BeforeValue());
			//If the param does not exist then just remove it when resetting.
			if(!params.HasParam(param_name)){
				params_before[i].delete_before = true;
				return;
			}else{
				params_before[i].delete_before = false;
			}
			if(param_type == string_param){
				params_before[i].string_value = params.GetString(param_name);
			}else if(param_type == float_param){
				params_before[i].float_value = params.GetFloat(param_name);
			}else if(param_type == int_param){
				params_before[i].int_value = params.GetInt(param_name);
			}
		}
	}

	string GetDisplayString(){
		string display_string;
		if(param_type == string_param){
			display_string = string_param_after;
		}else if(param_type == float_param){
			display_string = "" + float_param_after;
		}else if(param_type == int_param){
			display_string = "" + int_param_after;
		}
		return "SetObjectParam " + target_select.GetTargetDisplayText() + " " + param_name + " " + display_string;
	}

	void StartSettings(){
		target_select.CheckAvailableTargets();
	}

	void DrawSettings(){

		float option_name_width = 140.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Target");
		ImGui_NextColumn();
		ImGui_NextColumn();
		target_select.DrawSelectTargetUI();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Param Name");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		ImGui_PushItemWidth(second_column_width);
		ImGui_InputText("###Param Name", param_name, 128);
		ImGui_PopItemWidth();
		ImGui_AlignTextToFramePadding();
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Param Type");
		ImGui_NextColumn();
		ImGui_PushItemWidth(second_column_width);
		if(ImGui_Combo("###Param Type", current_type, param_type_choices, param_type_choices.size())){
			param_type = param_types(current_type);
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		if(param_type == string_param){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Value");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_InputText("###Value", string_param_after, 128);
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}else if(param_type == int_param){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Value");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_InputInt("###Value", int_param_after);
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}else{
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Value");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_SliderFloat("###Value", float_param_after, -1000.0f, 1000.0f, "%.4f");
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}
	}

	void DrawEditing(){
		array<Object@> targets = target_select.GetTargetObjects();
		for(uint i = 0; i < targets.size(); i++){
			DebugDrawLine(targets[i].GetTranslation(), this_hotspot.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
		}
	}

	bool Trigger(){
		if(!triggered){
			GetBeforeParam();
		}
		triggered = true;
		return SetParameter(false);
	}

	bool SetParameter(bool reset){
		array<Object@> targets = target_select.GetTargetObjects();
		for(uint i = 0; i < targets.size(); i++){
			ScriptParams@ params = targets[i].GetScriptParams();

			if(reset && params_before[i].delete_before){
				params.Remove(param_name);
				return true;
			}

			if(!params.HasParam(param_name)){
				if(param_type == string_param){
					params.AddString(param_name, reset?params_before[i].string_value:string_param_after);
				}else if(param_type == int_param){
					params.AddInt(param_name, reset?params_before[i].int_value:int_param_after);
				}else if(param_type == float_param){
					params.AddFloatSlider(param_name, reset?params_before[i].float_value:float_param_after, "min:0,max:1000,step:0.0001,text_mult:1");
				}
			}else{
				if(param_type == string_param){
					params.SetString(param_name, reset?params_before[i].string_value:string_param_after);
				}else if(param_type == int_param){
					params.SetInt(param_name, reset?params_before[i].int_value:int_param_after);
				}else if(param_type == float_param){
					params.Remove(param_name);
					params.AddFloatSlider(param_name, reset?params_before[i].float_value:float_param_after, "min:0,max:1000,step:0.0001,text_mult:1");
					/* params.SetFloat(param_name, reset?params_before[i].float_value:float_param_after); */
				}
			}
		}
		return true;
	}

	void Reset(){
		if(triggered){
			triggered = false;
			SetParameter(true);
		}
	}

	void Delete(){
		target_select.Delete();
	}
}
