enum level_params { 	achievements = 0,
						custom_shaders = 1,
						fog_amount = 2,
						gpu_particle_field = 3,
						hdr_black_point = 4,
						hdr_bloom_multiplier = 5,
						hdr_white_point = 6,
						level_boundaries = 7,
						load_tip = 8,
						objectives = 9,
						saturation = 10,
						sky_brightness = 11,
						sky_rotation = 12,
						sky_tint = 13,
						sun_position = 14,
						sun_color = 15,
						sun_intensity = 16,
						other = 17
					};

class DrikaSetLevelParam : DrikaElement{
	int current_type;
	string param_name;
	bool has_function = false;
	bool delete_before = false;

	string string_param_before;
	string string_param_after;

	float float_param_before;
	float float_param_after;

	int int_param_before;
	int int_param_after;

	vec3 vec3_param_before;
	vec3 vec3_param_after;

	level_params level_param;
	param_types param_type;

	array<int> string_parameters = {achievements, custom_shaders, gpu_particle_field, load_tip, objectives, other};
	array<int> float_parameters = {fog_amount, hdr_black_point, hdr_bloom_multiplier, hdr_white_point, saturation, sky_brightness, sky_rotation, sun_intensity};
	array<int> vec3_parameters = {sun_position};
	array<int> vec3_color_parameters = {sky_tint, sun_color};
	array<int> int_parameters = {level_boundaries};
	array<int> function_parameters = {sky_tint, hdr_black_point, hdr_bloom_multiplier, hdr_white_point, sun_position, sun_color, sun_intensity};

	array<int> mult_100_params = {hdr_black_point, hdr_bloom_multiplier, hdr_white_point, saturation, sky_brightness};

	array<string> param_names = {	"Achievements",
	 								"Custom Shader",
									"Fog amount",
									"GPU Particle Field",
									"HDR Black point",
									"HDR Bloom multiplier",
									"HDR White point",
									"Level Boundaries",
									"Load Tip",
									"Objectives",
									"Saturation",
									"Sky Brightness",
									"Sky Rotation",
									"Sky Tint",
									"Sun Position",
									"Sun Color",
									"Sun Intensity",
									"Other..."
								};

	DrikaSetLevelParam(JSONValue params = JSONValue()){
		level_param = level_params(GetJSONInt(params, "level_param", 0));
		current_type = level_param;
		param_name = param_names[current_type];

		drika_element_type = drika_set_level_param;
		SetParamType();
		InterpParam(params);
		has_settings = true;
	}

	JSONValue GetSaveData(){
		JSONValue data;
		data["level_param"] = JSONValue(level_param);
		data["param_name"] = JSONValue(param_name);

		string save_string;
		if(level_param == other){
			data["param_after"] = JSONValue(string_param_after);
		}else if(param_type == string_param){
			data["param_after"] = JSONValue(string_param_after);
			save_string = string_param_after;
		}else if(param_type == vec3_param || param_type == vec3_color_param){
			data["param_after"] = JSONValue(JSONarrayValue);
			data["param_after"].append(vec3_param_after.x);
			data["param_after"].append(vec3_param_after.y);
			data["param_after"].append(vec3_param_after.z);
		}else if(param_type == float_param){
			data["param_after"] = JSONValue(float_param_after);
		}else if(param_type == int_param){
			data["param_after"] = JSONValue(int_param_after);
		}
		return data;
	}

	void SetParamType(){
		if(string_parameters.find(level_param) != -1){
			param_type = string_param;
		}else if(float_parameters.find(level_param) != -1){
			param_type = float_param;
		}else if(vec3_parameters.find(level_param) != -1){
			param_type = vec3_param;
		}else if(vec3_color_parameters.find(level_param) != -1){
			param_type = vec3_color_param;
		}else if(int_parameters.find(level_param) != -1){
			param_type = int_param;
		}
		if(function_parameters.find(level_param) != -1){
			has_function = true;
		}else{
			has_function = false;
		}
	}

	void InterpParam(JSONValue _params){
		if(param_type == vec3_param || param_type == vec3_color_param){
			vec3_param_after = GetJSONVec3(_params, "param_after", vec3(1));
		}else if(param_type == float_param){
			float_param_after = GetJSONFloat(_params, "param_after", 0.0);
		}else if(param_type == int_param){
			int_param_after = GetJSONInt(_params, "param_after", 0);
		}else if(param_type == string_param){
			if(level_param == other){
				param_name = GetJSONString(_params, "param_name", "");
				string_param_after = GetJSONString(_params, "param_after", "");
			}else{
				string_param_after = GetJSONString(_params, "param_after", "");
			}
		}
	}

	string GetDisplayString(){
		string display_string;
		if(level_param == other){
			display_string = string_param_after;
		}else if(param_type == string_param){
			display_string = string_param_after;
		}else if(param_type == vec3_param || param_type == vec3_color_param){
			display_string = Vec3ToString(vec3_param_after);
		}else if(param_type == float_param){
			display_string = "" + float_param_after;
		}else if(param_type == int_param){
			display_string = "" + int_param_after;
		}
		return "SetLevelParam " + param_name + " " + display_string;
	}

	void DrawSettings(){

		float option_name_width = 110.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Param Type");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		ImGui_PushItemWidth(second_column_width);
		if(ImGui_Combo("##Param Type", current_type, param_names)){
			level_param = level_params(current_type);
			param_name = param_names[current_type];
			SetParamType();
			GetBeforeParam();
			if(param_type == string_param){
				string_param_after = string_param_before;
			}else if(param_type == float_param){
				float_param_after = float_param_before;
			}else if(param_type == int_param){
				int_param_after = int_param_before;
			}else if(param_type == vec3_param || param_type == vec3_color_param){
				vec3_param_after = vec3_param_before;
			}
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		if(param_type == string_param){
			if(level_param == other){
				ImGui_AlignTextToFramePadding();
				ImGui_Text("Param Name");
				ImGui_NextColumn();
				ImGui_PushItemWidth(second_column_width);
				ImGui_InputText("###Param Name", param_name, 128);
				ImGui_PopItemWidth();
				ImGui_NextColumn();
			}
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Value");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_InputText("###Value", string_param_after, 128);
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}else if(param_type == float_param){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Value");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_SliderFloat("###Value", float_param_after, -500.0f, 500.0f, "%.3f");
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}else if(param_type == vec3_param){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Value");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_SliderFloat3("###Value", vec3_param_after, -1.0f, 1.0f, "%.3f");
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}else if(param_type == vec3_color_param){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Value");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_ColorEdit3("###Value", vec3_param_after);
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}else if(param_type == int_param){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Value");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			bool checked = (int_param_after == 1);
			if(ImGui_Checkbox("###Value", checked)){
				int_param_after = checked?1:0;
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}
	}

	void GetBeforeParam(){
		if(has_function){
			switch(level_param){
				case sun_position:
					vec3_param_before = GetSunPosition();
					break;
				case sun_color:
					vec3_param_before = GetSunColor();
					break;
				case sun_intensity:
					float_param_before = GetSunAmbient();
					break;
				case sky_tint:
					vec3_param_before = GetSkyTint();
					break;
				case hdr_black_point:
					float_param_before = GetHDRBlackPoint() * 100.0f;
					break;
				case hdr_bloom_multiplier:
					float_param_before = GetHDRBloomMult() * 100.0f;
					break;
				case hdr_white_point:
					float_param_before = GetHDRWhitePoint() * 100.0f;
					break;
				default:
					Log(warning, "Found a non standard parameter type. " + param_type);
					break;
			}
		}else{
			ScriptParams@ params = level.GetScriptParams();

			if(!params.HasParam(param_name)){
				delete_before = true;
				return;
			}else{
				delete_before = false;
			}

			if(param_type == string_param){
				if(!params.HasParam(param_name)){
					params.AddString(param_name, string_param_after);
				}
				string_param_before = params.GetString(param_name);
			}else if(param_type == float_param){
				if(!params.HasParam(param_name)){
					params.AddFloat(param_name, float_param_after);
				}
				float current_value = params.GetFloat(param_name);
				if(mult_100_params.find(level_param) != -1){
					current_value *= 100.0f;
				}
				float_param_before = current_value;
			}else if(param_type == int_param){
				if(!params.HasParam(param_name)){
					params.AddInt(param_name, int_param_after);
				}
				int_param_before = params.GetInt(param_name);
			}
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
		if(has_function){
			switch(level_param){
				case sky_tint:
					{
						ScriptParams@ params = level.GetScriptParams();
						vec3 new_color = reset?vec3_param_before:vec3_param_after;
						params.SetString("Sky Tint", int(new_color.x * 255) + ", " + int(new_color.y * 255) + ", " + int(new_color.z * 255));
						SetSkyTint(new_color);
						RefreshGlobalReflection();
					}
					break;
				case sun_position:
					SetSunPosition(reset?vec3_param_before:vec3_param_after);
					RefreshGlobalReflection();
					break;
				case sun_color:
					SetSunColor(reset?vec3_param_before:vec3_param_after);
					RefreshGlobalReflection();
					break;
				case sun_intensity:
					SetSunAmbient(reset?float_param_before:float_param_after);
					RefreshGlobalReflection();
					break;
				case hdr_black_point:
					SetHDRBlackPoint((reset?float_param_before:float_param_after) / 100.0f);
					break;
				case hdr_bloom_multiplier:
					SetHDRBloomMult((reset?float_param_before:float_param_after) / 100.0f);
					break;
				case hdr_white_point:
					SetHDRWhitePoint((reset?float_param_before:float_param_after) / 100.0f);
					break;
				default:
					Log(warning, "Found a non standard parameter type. " + param_type);
					break;
			}
		}else{
			ScriptParams@ params = level.GetScriptParams();

			if(reset && delete_before){
				params.Remove(param_name);
				return true;
			}

			if(param_type == string_param){
				params.SetString(param_name, reset?string_param_before:string_param_after);
			}else if(param_type == float_param){
				float new_value = reset?float_param_before:float_param_after;
				if(mult_100_params.find(level_param) != -1){
					new_value /= 100.0f;
				}
				params.SetFloat(param_name, new_value);
			}else if(param_type == int_param){
				params.SetInt(param_name, reset?int_param_before:int_param_after);
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

	void RefreshGlobalReflection(){
		array<int> reflection_objects = GetObjectIDsType(_reflection_capture_object);

		for(uint i = 0; i < reflection_objects.size(); i++){
			Object@ obj = ReadObjectFromID(reflection_objects[i]);
			obj.SetTranslation(obj.GetTranslation());
			/* DebugDrawWireSphere(obj.GetTranslation(), 1.0, vec3(1.0f), _persistent); */
		}
	}
}
