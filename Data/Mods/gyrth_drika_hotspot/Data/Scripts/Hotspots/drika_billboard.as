enum billboard_types{	billboard_image_at_placeholder = 0,
						billboard_text_at_placeholder = 1,
						billboard_image_at_target = 2,
						billboard_text_at_target = 3,
						billboard_delete = 4
					};

class DrikaBillboard : DrikaElement{
	string image_path;
	float image_size = 1.0;
	vec4 image_color = vec4(1.0);
	string billboard_text_string;
	float overbright;
	float height_offset;

	int current_billboard_type;
	billboard_types billboard_type;
	array<string> billboard_type_choices = {"Image At Placeholder", "Text At Placeholder", "Image At Target", "Text At Target", "Delete"};

	DrikaBillboard(JSONValue params = JSONValue()){
		placeholder.Load(params);
		placeholder.name = "Billboard Helper";
		placeholder.default_scale = vec3(1.0);

		image_path = GetJSONString(params, "image_path", "Data/Textures/ui/ogicon.png");
		image_color = GetJSONVec4(params, "image_color", vec4(1));
		image_size = GetJSONFloat(params, "image_size", 1.0f);
		overbright = GetJSONFloat(params, "overbright", 0.0f);
		billboard_text_string = GetJSONString(params, "billboard_text_string", "Example billboard text.");
		height_offset = GetJSONFloat(params, "height_offset", 0.0);
		reference_string = GetJSONString(params, "reference_string", "");

		billboard_type = billboard_types(GetJSONInt(params, "billboard_type", billboard_image_at_placeholder));
		current_billboard_type = billboard_type;

		@target_select = DrikaTargetSelect(this, params);
		target_select.target_option = id_option | name_option | character_option | reference_option | team_option;

		drika_element_type = drika_billboard;
		has_settings = true;
	}

	JSONValue GetSaveData(){
		JSONValue data;

		if(billboard_type == billboard_image_at_placeholder || billboard_type == billboard_image_at_target){
			data["image_path"] = JSONValue(image_path);
			data["image_size"] = JSONValue(image_size);
			data["image_color"] = JSONValue(JSONarrayValue);
			data["image_color"].append(image_color.x);
			data["image_color"].append(image_color.y);
			data["image_color"].append(image_color.z);
			data["image_color"].append(image_color.a);
			data["overbright"] = JSONValue(overbright);
		}else if(billboard_type == billboard_text_at_placeholder || billboard_type == billboard_text_at_target){
			data["billboard_text_string"] = JSONValue(billboard_text_string);
		}

		if(billboard_type == billboard_image_at_target || billboard_type == billboard_text_at_target){
			target_select.SaveIdentifier(data);
			data["height_offset"] = JSONValue(height_offset);
		}

		data["reference_string"] = JSONValue(reference_string);
		data["billboard_type"] = JSONValue(billboard_type);

		placeholder.Save(data);
		return data;
	}

	bool UsesPlaceholderObject(){
		return (billboard_type == billboard_image_at_placeholder || billboard_type == billboard_text_at_placeholder);
	}

	void PostInit(){
		placeholder.Retrieve();
	}

	void Delete(){
		Reset();
		placeholder.Remove();
	}

	void Reset(){
		RemoveContinuesUpdateElement(this);
	}

	string GetDisplayString(){
		if(billboard_type == billboard_image_at_placeholder ){
			return "Billboard Image At Placeholder " + image_path;
		}else if(billboard_type == billboard_image_at_target){
			return "Billboard Image At Target " + target_select.GetTargetDisplayText() + " " + image_path;
		}else if(billboard_type == billboard_text_at_placeholder){
			return "Billboard Text At Placeholder \"" + billboard_text_string + "\"";
		}else if(billboard_type == billboard_text_at_target){
			return "Billboard Text At Target \"" + billboard_text_string + "\"";
		}else if(billboard_type == billboard_delete){
			return "Billboard Delete \"" + reference_string + "\"";
		}
		return "Billboard";
	}

	void DrawSettings(){

		float option_name_width = 120.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Billboard Type");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		ImGui_PushItemWidth(second_column_width);
		if(ImGui_Combo("##Billboard Type", current_billboard_type, billboard_type_choices, billboard_type_choices.size())){
			billboard_type = billboard_types(current_billboard_type);
			Reset();
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		if(billboard_type == billboard_image_at_target || billboard_type == billboard_text_at_target || billboard_type == billboard_text_at_placeholder || billboard_type == billboard_image_at_placeholder){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Name");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);

			if(ImGui_InputText("##Name", reference_string, 64)){
				AddContinuesUpdateElement(this);
			}

			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}else if(billboard_type == billboard_delete){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Name");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);

			if(ImGui_InputText("##Name", reference_string, 64)){
				
			}

			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}

		if(billboard_type == billboard_image_at_target || billboard_type == billboard_text_at_target){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Target");
			ImGui_NextColumn();
			ImGui_NextColumn();
			target_select.DrawSelectTargetUI();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Height Offset");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_DragFloat("###Height Offset", height_offset, 0.0f, 0.0f, 5.0f, "%.1f");
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}

		if(billboard_type == billboard_image_at_placeholder || billboard_type == billboard_image_at_target){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Image Path");
			ImGui_NextColumn();
			if(ImGui_Button("Set Image Path")){
				string new_path = GetUserPickedReadPath("png", "Data/Images");
				if(new_path != ""){
					image_path = ShortenPath(new_path);
				}
			}
			ImGui_PopItemWidth();
			ImGui_SameLine();
			ImGui_Text(image_path);
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Image Size");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_SliderFloat("##Image Size", image_size, 0.0f, 10.0f, "%.2f");
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Tint");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_ColorEdit4("##Tint", image_color);
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Overbright");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			ImGui_SliderFloat("##Overbright", overbright, 0.0f, 10.0f, "%.1f");
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}else if(billboard_type == billboard_text_at_placeholder || billboard_type == billboard_text_at_target){
			ImGui_AlignTextToFramePadding();

			ImGui_NextColumn();
			ImGui_AlignTextToFramePadding();
			ImGui_TextWrapped("Entering words between {braces} will cause that word to be interpreted as a variable.");
			ImGui_TextWrapped("If you want to display a word between braces on screen, just add a backslash in front of that \\{word}.");
			ImGui_NextColumn();

			ImGui_Text("Text");
			ImGui_NextColumn();
			ImGui_SetTextBuf(billboard_text_string);
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_InputTextMultiline("##TEXT", vec2(-1.0, -1.0))){
				billboard_text_string = ImGui_GetTextBuf();
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}
	}

	void DrawEditing(){
		PlaceholderCheck();

		if(UsesPlaceholderObject()){
			DebugDrawLine(placeholder.GetTranslation(), this_hotspot.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
		}
	}

	void PlaceholderCheck(){
		if(!UsesPlaceholderObject() && placeholder.Exists()){
			placeholder.Remove();
		}else if(UsesPlaceholderObject() && !placeholder.Exists()){
			placeholder.Create();
			StartEdit();
		}
	}

	void StartEdit(){
		DrikaElement::StartEdit();
		Reset();
	}

	void StartSettings(){
		target_select.CheckAvailableTargets();
	}

	bool Trigger(){
		if(billboard_type == billboard_delete){
			RemoveContinuesUpdateElementByName(reference_string);
			return true;
		}else{
			if(placeholder.Exists()){
				AddContinuesUpdateElement(this);
				return true;
			}else{
				placeholder.Create();
				return false;
			}
		}
	}

	void Update(){
		float multiplier = 1.0 + overbright;
		vec4 combined_color = vec4(image_color.x * multiplier, image_color.y * multiplier, image_color.z * multiplier, image_color.a);

		if(billboard_type == billboard_image_at_target || billboard_type == billboard_text_at_target){
			array<Object@> targets = target_select.GetTargetObjects();
			for(uint i = 0; i < targets.size(); i++){
				vec3 target_location = GetTargetTranslation(targets[i]) + vec3(0.0, height_offset, 0.0);

				if(billboard_type == billboard_image_at_target){
					DebugDrawBillboard(image_path, target_location, image_size, combined_color, _delete_on_update);
				}else if(billboard_type == billboard_text_at_target){
					DebugDrawText(target_location, ReplaceVariablesFromText(billboard_text_string), 1.0, false, _delete_on_update);
				}
			}
		}else{
			if(billboard_type == billboard_image_at_placeholder){
				DebugDrawBillboard(image_path, placeholder.GetTranslation(), image_size, combined_color, _delete_on_update);
			}else if(billboard_type == billboard_text_at_placeholder){
				DebugDrawText(placeholder.GetTranslation(), ReplaceVariablesFromText(billboard_text_string), 1.0, false, _delete_on_update);
			}
		}
	}
}
