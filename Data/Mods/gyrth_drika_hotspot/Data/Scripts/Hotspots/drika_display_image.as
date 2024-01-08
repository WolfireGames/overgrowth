class DrikaDisplayImage : DrikaElement{
	string image_path;
	vec4 tint;
	float scale;
	bool clear_image = false;

	DrikaDisplayImage(JSONValue params = JSONValue()){
		image_path = GetJSONString(params, "image_path", "Data/Textures/drika_hotspot.png");
		tint = GetJSONVec4(params, "tint", vec4(1.0f));
		scale = GetJSONFloat(params, "scale", 1.0);
		clear_image = GetJSONBool(params, "clear_image", false);

		drika_element_type = drika_display_image;
		has_settings = true;
	}

	JSONValue GetSaveData(){
		JSONValue data;
		data["image_path"] = JSONValue(image_path);
		data["scale"] = JSONValue(scale);
		data["clear_image"] = JSONValue(clear_image);
		data["tint"] = JSONValue(JSONarrayValue);
		data["tint"].append(tint.x);
		data["tint"].append(tint.y);
		data["tint"].append(tint.z);
		data["tint"].append(tint.a);
		return data;
	}

	string GetDisplayString(){
		if(clear_image){
			return "DisplayImage clear";
		}else{
			return "DisplayImage " + image_path;
		}
	}

	void StartEdit(){
		if(!clear_image){
			ShowImage(image_path, tint, scale);
		}
	}

	void EditDone(){
		ShowImage("", tint, scale);
	}

	void DrawSettings(){

		float option_name_width = 120.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Clear Image");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		if(ImGui_Checkbox("###Clear Image", clear_image)){
			if(clear_image){
				ShowImage("", tint, scale);
			}else{
				ShowImage(image_path, tint, scale);
			}
		}
		ImGui_NextColumn();

		if(!clear_image){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Image Path");
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			if(ImGui_Button("Set Image Path")){
				string new_path = GetUserPickedReadPath("png", "Data/Textures");
				if(new_path != ""){
					image_path = ShortenPath(new_path);
					ShowImage(image_path, tint, scale);
				}
			}
			ImGui_SameLine();
			ImGui_Text(image_path);
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Scale");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_SliderFloat("##Scale", scale, 0.0f, 5.0f, "%.1f")){
				ShowImage(image_path, tint, scale);
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Color");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_ColorEdit4("##Color", tint)){
				ShowImage(image_path, tint, scale);
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}
	}

	void Reset(){
		if(triggered){
			ShowImage("", tint, scale);
		}
	}

	bool Trigger(){
		if(!triggered){
			triggered = true;
		}
		if(clear_image){
			ShowImage("", tint, scale);
		}else{
			ShowImage(image_path, tint, scale);
		}
		return true;
	}
}
