enum camera_params {	tint = 0,
						vignette_tint = 1,
						fov = 2,
						dof = 3,
						splitscreen = 4
					};

class DrikaSetCameraParam : DrikaElement{
	int current_type;
	string param_name;

	string string_param_before;
	string string_param_after;

	array<float> float_array_param_before;
	array<float> float_array_param_after;

	int int_param_before;
	int int_param_after;

	float float_param_before;
	float float_param_after;

	vec3 vec3_param_before;
	vec3 vec3_param_after;

	camera_params camera_param;
	param_types param_type;

	array<int> float_parameters = {fov};
	array<int> vec3_color_parameters = {tint, vignette_tint};
	array<int> float_array_parameters = {dof};
	array<int> function_parameters = {splitscreen};

	array<string> splitscreen_mode_choices = {"None", "Full", "Split"};

	array<string> param_names = {	"Tint",
	 								"Vignette Tint",
									"FOV",
									"DOF",
									"SplitScreen"
								};

	DrikaSetCameraParam(JSONValue params = JSONValue()){
		camera_param = camera_params(GetJSONInt(params, "camera_param", 0));
		current_type = camera_param;
		param_name = param_names[camera_param];

		drika_element_type = drika_set_camera_param;
		has_settings = true;
		SetParamType();
		InterpParam(params);
	}

	void PostInit(){
		GetBeforeParam();
	}

	JSONValue GetSaveData(){
		JSONValue data;
		data["camera_param"] = JSONValue(camera_param);
		string save_string;
		if(param_type == vec3_color_param){
			data["param_after"] = JSONValue(JSONarrayValue);
			data["param_after"].append(vec3_param_after.x);
			data["param_after"].append(vec3_param_after.y);
			data["param_after"].append(vec3_param_after.z);
		}else if(param_type == float_param){
			data["param_after"] = JSONValue(float_param_after);
		}else if(param_type == float_array_param){
			data["param_after"] = JSONValue(JSONarrayValue);
			for(uint i = 0; i < float_array_param_after.size(); i++){
				data["param_after"].append(float_array_param_after[i]);
			}
		}else if(param_type == function_param){
			data["param_after"] = JSONValue(int_param_after);
		}
		return data;
	}

	void SetParamType(){
		if(float_parameters.find(camera_param) != -1){
			param_type = float_param;
		}else if(vec3_color_parameters.find(camera_param) != -1){
			param_type = vec3_color_param;
		}else if(float_array_parameters.find(camera_param) != -1){
			param_type = float_array_param;
		}else if(function_parameters.find(camera_param) != -1){
			param_type = function_param;
		}
	}

	void InterpParam(JSONValue _params){
		if(param_type == vec3_color_param){
			vec3_param_after = GetJSONVec3(_params, "param_after", vec3(1.0));
		}else if(param_type == float_param){
			float_param_after = GetJSONFloat(_params, "param_after", 1.0);
		}else if(param_type == float_array_param){
			float_array_param_after = GetJSONFloatArray(_params, "param_after", {1.0, 1.0, 1.0, 1.0, 1.0, 1.0});
		}else if(param_type == function_param){
			int_param_after = GetJSONInt(_params, "param_after", 0);
		}
	}

	string GetDisplayString(){
		string display_string;
		if(param_type == float_param){
			display_string = "" + float_param_after;
		}else if(param_type == float_array_param){
			display_string = "";
			for(uint i = 0; i < float_array_param_after.size(); i++){
				display_string += ((i == 0)?"":" ") + float_array_param_after[i];
			}
		}else if(param_type == vec3_color_param){
			display_string = vec3_param_after.x + "," + vec3_param_after.y + "," + vec3_param_after.z;
		}else if(param_type == function_param){
			if(camera_param == splitscreen){
				display_string = splitscreen_mode_choices[int_param_after];
			}
		}
		return "SetCameraParam " + param_name + " " + display_string;
	}

	void StartEdit(){
		if(camera_param == dof){
			SetParameter(false);
		}
	}

	void EditDone(){
		if(camera_param == dof){
			SetParameter(true);
		}
	}

	void DrawSettings(){

		float option_name_width = 120.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Param Type");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();

		ImGui_PushItemWidth(second_column_width);
		if(ImGui_Combo("##Param Type", current_type, param_names, param_names.size())){
			camera_param = camera_params(current_type);
			param_name = param_names[current_type];
			SetParamType();
			GetBeforeParam();
			if(param_type == float_param){
				float_param_after = float_param_before;
			}else if(param_type == float_array_param){
				float_array_param_after = float_array_param_before;
			}else if(param_type == vec3_color_param){
				vec3_param_after = vec3_param_before;
			}
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		if(camera_param == fov){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("FOV");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_SliderFloat("##FOV", float_param_after, 0.0f, 100.0f, "%.0f");
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}else if(camera_param == dof){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Near Blur");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_SliderFloat("##Near Blur", float_array_param_after[0], 0.0f, 10.0f, "%.1f")){
				SetParameter(false);
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Near Dist");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_SliderFloat("##Near Dist", float_array_param_after[1], 0.0f, 10.0f, "%.1f")){
				SetParameter(false);
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Near Transition");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_SliderFloat("##Near Transition", float_array_param_after[2], 0.0f, 10.0f, "%.1f")){
				SetParameter(false);
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Far Blur");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_SliderFloat("##Far Blur", float_array_param_after[3], 0.0f, 10.0f, "%.1f")){
				SetParameter(false);
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Far Dist");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_SliderFloat("##Far Dist", float_array_param_after[4], 0.0f, 10.0f, "%.1f")){
				SetParameter(false);
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Far Transition");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_SliderFloat("##Far Transition", float_array_param_after[5], 0.0f, 10.0f, "%.1f")){
				SetParameter(false);
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}else if(param_type == float_param){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("After");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_SliderFloat("##After", float_param_after, -100.0f, 100.0f, "%.4f");
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}else if(param_type == vec3_color_param){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("After");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_ColorEdit3("##After", vec3_param_after);
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}else if(param_type == function_param){
			if(camera_param == splitscreen){
				ImGui_AlignTextToFramePadding();
				ImGui_Text("SplitScreen Mode");
				ImGui_NextColumn();
				ImGui_PushItemWidth(second_column_width);
				ImGui_Combo("##SplitScreen Mode", int_param_after, splitscreen_mode_choices, splitscreen_mode_choices.size());
				ImGui_PopItemWidth();
				ImGui_NextColumn();
			}
		}
	}

	void GetBeforeParam(){
		switch(camera_param){
			case tint:
				vec3_param_before = camera.GetTint();
				break;
			case vignette_tint:
				vec3_param_before = camera.GetVignetteTint();
				break;
			case fov:
				float_param_before = camera.GetFOV();
				break;
			case dof:
				float_array_param_before = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};
				break;
			case splitscreen:
				int_param_before = kModeNone;
				break;
			default:
				Log(warning, "Found a non standard parameter type. " + param_type);
				break;
		}
	}

	bool Trigger(){
		if(!triggered){
			GetBeforeParam();
			triggered = true;
		}
		return SetParameter(false);
	}

	bool SetParameter(bool reset){
		switch(camera_param){
			case tint:
				camera.SetTint(reset?vec3_param_before:vec3_param_after);
				break;
			case vignette_tint:
				{
					camera.SetVignetteTint(reset?vec3_param_before:vec3_param_after);
				}
				break;
			case fov:
				if(level.DialogueCameraControl()){
					level.SendMessage("drika_set_fov " + float_param_after);
					level.Execute("dialogue.cam_zoom = " + float_param_after + ";");
				}else{
					camera.SetFOV(reset?float_param_before:float_param_after);
				}
				break;
			case dof:
				{
					array<float>@ new_setting = reset?float_array_param_before:float_array_param_after;
					level.SendMessage("drika_set_dof " + new_setting[0] + " " + new_setting[1] + " " + new_setting[2] + " " + new_setting[3] + " " + new_setting[4] + " " + new_setting[5]);
					camera.SetDOF(new_setting[0],new_setting[1],new_setting[2],new_setting[3],new_setting[4],new_setting[5]);
				}
				break;
			case splitscreen:
				{
					int mode = reset?int_param_before:int_param_after;
					if(mode == kModeNone){
						SetSplitScreenMode(kModeNone);
					}else if(mode == kModeFull){
						SetSplitScreenMode(kModeFull);
					}else if(mode == kModeSplit){
						SetSplitScreenMode(kModeSplit);
					}
				}
				break;
			default:
				Log(warning, "Found a non standard parameter type. " + param_type);
				break;
		}
		return true;
	}

	void Reset(){
		if(triggered){
			SetParameter(true);
			triggered = false;
		}
	}
}
