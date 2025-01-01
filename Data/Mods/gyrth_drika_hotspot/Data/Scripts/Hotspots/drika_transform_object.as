enum transform_modes	{
							transform_to_placeholder = 0,
							transform_to_target = 1,
							move_towards_target = 2,
							transform_specific = 3,
							transform_to_hotspot = 4
						}


class DrikaTransformObject : DrikaElement{
	vec3 before_translation;
	quaternion before_rotation;
	vec3 before_scale;
	DrikaTargetSelect@ target_location;
	bool use_target_location;
	bool use_target_rotation;
	bool use_target_scale;
	vec3 translation_offset;
	transform_modes transform_mode;
	int current_transform_mode;
	float move_speed;
	float extra_yaw;

	float s_transform_x;
	float s_transform_y;
	float s_transform_z;

	float s_scale_x;
	float s_scale_y;
	float s_scale_z;

	float s_rotate_x;
	float s_rotate_y;
	float s_rotate_z;

	bool s_transform_x_relative;
	bool s_transform_y_relative;
	bool s_transform_z_relative;

	bool s_scale_x_relative;
	bool s_scale_y_relative;
	bool s_scale_z_relative;

	bool s_rotate_relative;

	bool s_transform_to_target_first;
	bool s_transform_to_bone;
	bool s_copy_bone_rotation;
	bool s_flatten_rotation;
	bool s_copy_bone_position;
	
	string s_bone_name;

	array<string> transform_mode_names =	{
												"Transform To Placeholder",
												"Transform To Target",
												"Move To Target",
												"Transform Specific",
												"Transform To Hotspot"
											};

	DrikaTransformObject(JSONValue params = JSONValue()){
		placeholder.Load(params);
		placeholder.name = "Transform Object Helper";
		placeholder.default_scale = vec3(1.0);

		translation_offset = GetJSONVec3(params, "translation_offset", vec3(0.0));
		drika_element_type = drika_transform_object;
		connection_types = {_movement_object, _env_object, _decal_object, _item_object, _hotspot_object};

		@target_select = DrikaTargetSelect(this, params);
		target_select.target_option = id_option | name_option | character_option | reference_option | team_option | item_option;

		transform_mode = transform_modes(GetJSONInt(params, "transform_mode", transform_to_placeholder));
		current_transform_mode = transform_mode;
		use_target_location = GetJSONBool(params, "use_target_location", true);
		use_target_rotation = GetJSONBool(params, "use_target_rotation", false);
		use_target_scale = GetJSONBool(params, "use_target_scale", false);
		move_speed = GetJSONFloat(params, "move_speed", 1.0);
		extra_yaw = GetJSONFloat(params, "extra_yaw", 0.0);
		@target_location = DrikaTargetSelect(this, params, "target_location");
		target_location.target_option = id_option | name_option | character_option | reference_option | team_option | item_option;

		s_transform_x = GetJSONFloat(params, "s_transform_x", 0.0);
		s_transform_y = GetJSONFloat(params, "s_transform_y", 0.0);
		s_transform_z = GetJSONFloat(params, "s_transform_z", 0.0);

		s_scale_x = GetJSONFloat(params, "s_scale_x", 1.0);
		s_scale_y = GetJSONFloat(params, "s_scale_y", 1.0);
		s_scale_z = GetJSONFloat(params, "s_scale_z", 1.0);

		s_rotate_x = GetJSONFloat(params, "s_rotate_x", 0.0);
		s_rotate_y = GetJSONFloat(params, "s_rotate_y", 0.0);
		s_rotate_z = GetJSONFloat(params, "s_rotate_z", 0.0);

		s_transform_x_relative = GetJSONBool(params, "s_transform_x_relative", false);
		s_transform_y_relative = GetJSONBool(params, "s_transform_y_relative", false);
		s_transform_z_relative = GetJSONBool(params, "s_transform_z_relative", false);

		s_scale_x_relative = GetJSONBool(params, "s_scale_x_relative", false);
		s_scale_y_relative = GetJSONBool(params, "s_scale_y_relative", false);
		s_scale_z_relative = GetJSONBool(params, "s_scale_z_relative", false);

		s_rotate_relative = GetJSONBool(params, "s_rotate_relative", false);

		s_transform_to_target_first = GetJSONBool(params, "s_transform_to_target_first", false);
		s_transform_to_bone = GetJSONBool(params, "s_transform_to_bone", false);
		s_copy_bone_rotation = GetJSONBool(params, "s_copy_bone_rotation", false);
		s_flatten_rotation = GetJSONBool(params, "s_flatten_rotation", false);
		s_copy_bone_position = GetJSONBool(params, "s_copy_bone_position", false);
		
		s_bone_name = GetJSONString(params, "s_bone_name", "");

		has_settings = true;
	}

	void PostInit(){
		if(transform_mode == transform_to_placeholder){
			placeholder.Retrieve();
		}
		target_select.PostInit();
		target_location.PostInit();
	}

	bool UsesPlaceholderObject(){
		return (transform_mode == transform_to_placeholder);
	}

	JSONValue GetSaveData(){
		JSONValue data;
		target_select.SaveIdentifier(data);
		data["transform_mode"] = JSONValue(transform_mode);
		if(transform_mode == transform_to_target){
			target_location.SaveIdentifier(data);
			data["translation_offset"] = JSONValue(JSONarrayValue);
			data["translation_offset"].append(translation_offset.x);
			data["translation_offset"].append(translation_offset.y);
			data["translation_offset"].append(translation_offset.z);
			data["use_target_location"] = JSONValue(use_target_location);
			data["use_target_rotation"] = JSONValue(use_target_rotation);
			data["use_target_scale"] = JSONValue(use_target_scale);
		}else if(transform_mode == transform_to_placeholder){
			placeholder.Save(data);
		}else if(transform_mode == move_towards_target){
			target_location.SaveIdentifier(data);
			data["move_speed"] = JSONValue(move_speed);
			data["extra_yaw"] = JSONValue(extra_yaw);
		}else if(transform_mode == transform_specific){
			target_location.SaveIdentifier(data);
			data["translation_offset"] = JSONValue(JSONarrayValue);
			data["translation_offset"].append(translation_offset.x);
			data["translation_offset"].append(translation_offset.y);
			data["translation_offset"].append(translation_offset.z);
			data["use_target_location"] = JSONValue(use_target_location);
			data["use_target_rotation"] = JSONValue(use_target_rotation);
			data["use_target_scale"] = JSONValue(use_target_scale);
			data["s_transform_x"] = JSONValue(s_transform_x);
			data["s_transform_y"] = JSONValue(s_transform_y);
			data["s_transform_z"] = JSONValue(s_transform_z);
			data["s_scale_x"] = JSONValue(s_scale_x);
			data["s_scale_y"] = JSONValue(s_scale_y);
			data["s_scale_z"] = JSONValue(s_scale_z);
			data["s_rotate_x"] = JSONValue(s_rotate_x);
			data["s_rotate_y"] = JSONValue(s_rotate_y);
			data["s_rotate_z"] = JSONValue(s_rotate_z);
			data["s_transform_x_relative"] = JSONValue(s_transform_x_relative);
			data["s_transform_y_relative"] = JSONValue(s_transform_y_relative);
			data["s_transform_z_relative"] = JSONValue(s_transform_z_relative);
			data["s_scale_x_relative"] = JSONValue(s_scale_x_relative);
			data["s_scale_y_relative"] = JSONValue(s_scale_y_relative);
			data["s_scale_z_relative"] = JSONValue(s_scale_z_relative);
			data["s_rotate_relative"] = JSONValue(s_rotate_relative);
			data["s_transform_to_target_first"] = JSONValue(s_transform_to_target_first);
			data["s_transform_to_bone"] = JSONValue(s_transform_to_bone);
			data["s_copy_bone_rotation"] = JSONValue(s_copy_bone_rotation);
			data["s_flatten_rotation"] = JSONValue(s_flatten_rotation);
			data["s_copy_bone_position"] = JSONValue(s_copy_bone_position);
			data["s_bone_name"] = JSONValue(s_bone_name);
		}
		return data;
	}

	void GetBeforeParam(){
		array<Object@> targets = target_select.GetTargetObjects();
		for(uint i = 0; i < targets.size(); i++){
			before_translation = GetTargetTranslation(targets[i]);
			before_rotation = GetTargetRotation(targets[i]);
			before_scale = targets[i].GetScale();
		}
	}

	void Delete(){
		if(transform_mode == transform_to_placeholder){
			placeholder.Remove();
		}
		target_select.Delete();
		target_location.Delete();
	}

	string GetDisplayString(){
		return transform_mode_names[current_transform_mode] + " " + target_select.GetTargetDisplayText();
	}

	void SetPlaceholderTransform(){
		if(transform_mode == transform_to_placeholder){
			array<Object@> targets = target_select.GetTargetObjects();
			for(uint i = 0; i < targets.size(); i++){
				placeholder.SetTranslation(targets[i].GetTranslation());
				placeholder.SetRotation(targets[i].GetRotation());
				vec3 bounds = targets[i].GetBoundingBox();
				bounds.x = (bounds.x == 0.0)?1.0:bounds.x;
				bounds.y = (bounds.y == 0.0)?1.0:bounds.y;
				bounds.z = (bounds.z == 0.0)?1.0:bounds.z;
				placeholder.SetScale(bounds * targets[i].GetScale());
			}
		}
	}

	void StartSettings(){
		target_select.CheckAvailableTargets();
		target_location.CheckAvailableTargets();
	}

	void DrawSettings(){
		float option_name_width = 150.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Transform Target");
		ImGui_NextColumn();
		ImGui_NextColumn();
		target_select.DrawSelectTargetUI();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Transform Mode");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		ImGui_PushItemWidth(second_column_width);
		if(ImGui_Combo("###Transform Mode", current_transform_mode, transform_mode_names, transform_mode_names.size())){
			transform_mode = transform_modes(current_transform_mode);
			if(transform_mode != transform_to_placeholder){
				placeholder.Remove();
			}
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();
		ImGui_Separator();

		if(transform_mode == transform_to_target){
			target_location.DrawSelectTargetUI();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Translation Offset");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_DragFloat3("###Translation Offset", translation_offset, 0.001f, -5.0f, 5.0f, "%.3f")){

			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Use Target Location");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_Checkbox("###Use Target Location", use_target_location)){

			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Use Target Rotation");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_Checkbox("###Use Target Rotation", use_target_rotation)){

			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Use Target Scale");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_Checkbox("###Use Target Scale", use_target_scale)){

			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}else if(transform_mode == move_towards_target){
			target_location.DrawSelectTargetUI();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Speed");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_DragFloat("###Speed", move_speed, 0.001f, 0.01f, 50.0f, "%.3f")){

			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Extra Yaw");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_DragFloat("###Extra Yaw", extra_yaw, 0.01f, 0.0f, 360.0f, "%.3f")){

			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}else if(transform_mode == transform_specific){
			ImGui_Columns(4, false);
			ImGui_SetColumnWidth(0, option_name_width);

			ImGui_AlignTextToFramePadding();

			ImGui_NextColumn();
			ImGui_NextColumn();
			ImGui_NextColumn();
			ImGui_NextColumn();

			ImGui_Text("Translation:");
			ImGui_NextColumn();
			ImGui_Text(s_transform_x_relative? "x+" : "x=");
			ImGui_SameLine();
			if(ImGui_Checkbox("###Transform X Relative", s_transform_x_relative)){}
			if(ImGui_IsItemHovered()){
			ImGui_PushStyleColor(ImGuiCol_PopupBg, titlebar_color);
			ImGui_SetTooltip("Check for relative transformations");
			ImGui_PopStyleColor();
			}
			ImGui_SameLine();
			ImGui_PushItemWidth(second_column_width);
			ImGui_InputFloat("###Transform X", s_transform_x, 0, 5, 3);
			ImGui_PopItemWidth();
			ImGui_NextColumn();
			ImGui_Text(s_transform_y_relative? "y+" : "y=");
			ImGui_SameLine();
			if(ImGui_Checkbox("###Transform Y Relative", s_transform_y_relative)){}
			if(ImGui_IsItemHovered()){
			ImGui_PushStyleColor(ImGuiCol_PopupBg, titlebar_color);
			ImGui_SetTooltip("Check for relative transformations");
			ImGui_PopStyleColor();
			}
			ImGui_SameLine();
			ImGui_PushItemWidth(second_column_width);
			ImGui_InputFloat("###Transform Y", s_transform_y, 0, 5, 3);
			ImGui_PopItemWidth();
			ImGui_NextColumn();
			ImGui_Text(s_transform_z_relative? "z+" : "z=");
			ImGui_SameLine();
			if(ImGui_Checkbox("###Transform Z Relative", s_transform_z_relative)){}
			if(ImGui_IsItemHovered()){
			ImGui_PushStyleColor(ImGuiCol_PopupBg, titlebar_color);
			ImGui_SetTooltip("Check for relative transformations");
			ImGui_PopStyleColor();
			}
			ImGui_SameLine();
			ImGui_PushItemWidth(second_column_width);
			ImGui_InputFloat("###Transform Z", s_transform_z, 0, 5, 3);
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_Text("Scale:");
			ImGui_NextColumn();
			ImGui_Text(s_scale_x_relative? "x+" : "x=");
			ImGui_SameLine();
			if(ImGui_Checkbox("###Scale X Relative", s_scale_x_relative)){}
			if(ImGui_IsItemHovered()){
			ImGui_PushStyleColor(ImGuiCol_PopupBg, titlebar_color);
			ImGui_SetTooltip("Check for relative transformations");
			ImGui_PopStyleColor();
			}
			ImGui_SameLine();
			ImGui_PushItemWidth(second_column_width);
			ImGui_InputFloat("###Scale X", s_scale_x, 0, 5, 3);
			ImGui_PopItemWidth();
			ImGui_NextColumn();
			ImGui_Text(s_scale_y_relative? "y+" : "y=");
			ImGui_SameLine();
			if(ImGui_Checkbox("###Scale Y Relative", s_scale_y_relative)){}
			if(ImGui_IsItemHovered()){
			ImGui_PushStyleColor(ImGuiCol_PopupBg, titlebar_color);
			ImGui_SetTooltip("Check for relative transformations");
			ImGui_PopStyleColor();
			}
			ImGui_SameLine();
			ImGui_PushItemWidth(second_column_width);
			ImGui_InputFloat("###Scale Y", s_scale_y, 0, 5, 3);
			ImGui_PopItemWidth();
			ImGui_NextColumn();
			ImGui_Text(s_scale_z_relative? "z+" : "z=");
			ImGui_SameLine();
			if(ImGui_Checkbox("###Scale Z Relative", s_scale_z_relative)){}
			if(ImGui_IsItemHovered()){
			ImGui_PushStyleColor(ImGuiCol_PopupBg, titlebar_color);
			ImGui_SetTooltip("Check for relative transformations");
			ImGui_PopStyleColor();
			}
			ImGui_SameLine();
			ImGui_PushItemWidth(second_column_width);
			ImGui_InputFloat("###Scale Z", s_scale_z, 0, 5, 3);
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_Text("Rotation:");
			ImGui_NextColumn();
			ImGui_Text(s_rotate_relative? "x+" : "x=");
			ImGui_SameLine();
			ImGui_PushItemWidth(second_column_width);
			ImGui_InputFloat("###Rotate X", s_rotate_x, 0, 5, 3);
			ImGui_PopItemWidth();
			ImGui_NextColumn();
			ImGui_Text(s_rotate_relative? "y+" : "y=");
			ImGui_SameLine();
			ImGui_PushItemWidth(second_column_width);
			ImGui_InputFloat("###Rotate Y", s_rotate_y, 0, 5, 3);
			ImGui_PopItemWidth();
			ImGui_NextColumn();
			ImGui_Text(s_rotate_relative? "z+" : "z=");
			ImGui_SameLine();
			ImGui_PushItemWidth(second_column_width);
			ImGui_InputFloat("###Rotate Z", s_rotate_z, 0, 5, 3);
			ImGui_PopItemWidth();
			ImGui_NextColumn();

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Relative rotation?");
			ImGui_NextColumn();
			if(ImGui_Checkbox("###Rotate Relative", s_rotate_relative)){}
			if(ImGui_IsItemHovered()){
			ImGui_PushStyleColor(ImGuiCol_PopupBg, titlebar_color);
			ImGui_SetTooltip("If checked, the object will ADD to the current rotation, rather than setting it");
			ImGui_PopStyleColor();
			}
			ImGui_NextColumn();
			ImGui_NextColumn();

			ImGui_NextColumn();
			ImGui_NextColumn();
			ImGui_NextColumn();
			ImGui_NextColumn();

			ImGui_Separator();
			ImGui_Columns(2, false);
			ImGui_SetColumnWidth(0, option_name_width);

			ImGui_AlignTextToFramePadding();
			ImGui_Text("Transform to target?");
			ImGui_NextColumn();
			if(ImGui_Checkbox("###Transform To Target First", s_transform_to_target_first)){}
			if(ImGui_IsItemHovered()){
			ImGui_PushStyleColor(ImGuiCol_PopupBg, titlebar_color);
			ImGui_SetTooltip("If checked, this will be done first");
			ImGui_PopStyleColor();
			}
			ImGui_NextColumn();
			if(s_transform_to_target_first || s_transform_to_bone){
				if(!s_transform_to_target_first){ImGui_Separator();}
				target_location.DrawSelectTargetUI();
			}
			if(s_transform_to_target_first == true){
				ImGui_AlignTextToFramePadding();
				ImGui_Text("Translation Offset");
				ImGui_NextColumn();
				ImGui_PushItemWidth(second_column_width);
				if(ImGui_DragFloat3("###Translation Offset", translation_offset, 0.001f, -5.0f, 5.0f, "%.3f")){

				}
				ImGui_PopItemWidth();
				ImGui_NextColumn();

				ImGui_AlignTextToFramePadding();
				ImGui_Text("Use Target Location");
				ImGui_NextColumn();
				ImGui_PushItemWidth(second_column_width);
				if(ImGui_Checkbox("###Use Target Location", use_target_location)){

				}
				ImGui_PopItemWidth();
				ImGui_NextColumn();

				ImGui_AlignTextToFramePadding();
				ImGui_Text("Use Target Rotation");
				ImGui_NextColumn();
				ImGui_PushItemWidth(second_column_width);
				if(ImGui_Checkbox("###Use Target Rotation", use_target_rotation)){

				}
				ImGui_PopItemWidth();
				ImGui_NextColumn();

				ImGui_AlignTextToFramePadding();
				ImGui_Text("Use Target Scale");
				ImGui_NextColumn();
				ImGui_PushItemWidth(second_column_width);
				if(ImGui_Checkbox("###Use Target Scale", use_target_scale)){

				}
				ImGui_PopItemWidth();
				ImGui_NextColumn();
			}
			
			ImGui_Separator();
			
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Transform to bone?");
			ImGui_NextColumn();
			if(ImGui_Checkbox("###Transform To Bone", s_transform_to_bone)){}
			if(ImGui_IsItemHovered()){
			ImGui_PushStyleColor(ImGuiCol_PopupBg, titlebar_color);
			ImGui_SetTooltip("If checked, this will be applied before XYZ specific transformations");
			ImGui_PopStyleColor();
			}
			ImGui_NextColumn();
			
			if(s_transform_to_bone == true){

				ImGui_AlignTextToFramePadding();
				ImGui_Text("Bone Name");
				ImGui_NextColumn();
				ImGui_PushItemWidth(second_column_width);
				ImGui_InputText("###BoneName", s_bone_name, 64);
				ImGui_PopItemWidth();
				if(ImGui_IsItemHovered()){
				ImGui_PushStyleColor(ImGuiCol_PopupBg, titlebar_color);
				ImGui_SetTooltip("leftarm\nrightarm\nleft_leg\nright_leg\nhead\nleftear\nrightear\ntorso\ntail");
				ImGui_PopStyleColor();
				}
				
				ImGui_NextColumn();
				ImGui_Text("Copy Bone Position?");
				ImGui_NextColumn();
				if(ImGui_Checkbox("###Copy Bone Position", s_copy_bone_position)){}
				ImGui_NextColumn();
				ImGui_Text("Copy Bone Rotation?");
				ImGui_NextColumn();
				if(ImGui_Checkbox("###Copy Bone Rotation", s_copy_bone_rotation)){}
				ImGui_NextColumn();
				if(s_copy_bone_rotation){
					ImGui_Text("Flatten Rotation?");
					ImGui_NextColumn();
					if(ImGui_Checkbox("###Flatten Rotation", s_flatten_rotation)){}
					ImGui_NextColumn();
				}
			}
		}
	}

	void TargetChanged(){
		SetPlaceholderTransform();
	}

	bool Trigger(){
		if(!triggered){
			GetBeforeParam();
		}
		triggered = true;
		return ApplyTransform(false);
	}

	void DrawEditing(){
		if(transform_mode == transform_to_placeholder){
			if(placeholder.Exists()){
				array<Object@> targets = target_select.GetTargetObjects();
				for(uint i = 0; i < targets.size(); i++){
					DebugDrawLine(targets[i].GetTranslation(), placeholder.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
				}
				DebugDrawLine(placeholder.GetTranslation(), this_hotspot.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
				DrawGizmo(placeholder.GetTranslation(), placeholder.GetRotation(), placeholder.GetScale(), placeholder.IsSelected());
			}else{
				placeholder.Create();
				SetPlaceholderTransform();
				StartEdit();
			}
		}else if(transform_mode == transform_to_target || transform_mode == move_towards_target){
			array<Object@> target_location_objects = target_location.GetTargetObjects();
			array<Object@> targets = target_select.GetTargetObjects();

			for(uint i = 0; i < targets.size(); i++){
				DebugDrawLine(targets[i].GetTranslation(), this_hotspot.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
				for(uint j = 0; j < target_location_objects.size(); j++){
					DebugDrawLine(targets[i].GetTranslation(), GetTargetTranslation(target_location_objects[j]) + translation_offset, vec3(0.0, 1.0, 0.0), _delete_on_draw);
				}
			}

			for(uint j = 0; j < target_location_objects.size(); j++){
				vec3 gizmo_location = use_target_location?GetTargetTranslation(target_location_objects[j]):before_translation;
				quaternion gizmo_rotation = use_target_rotation?GetTargetRotation(target_location_objects[j]):before_rotation;
				vec3 gizmo_scale = use_target_scale?target_location_objects[j].GetScale():before_scale;
				DrawGizmo(gizmo_location + translation_offset, gizmo_rotation, gizmo_scale, true);
			}
		}else if(transform_mode == transform_to_hotspot){
			array<Object@> targets = target_select.GetTargetObjects();
			for(uint i = 0; i < targets.size(); i++){
				DebugDrawLine(targets[i].GetTranslation(), this_hotspot.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
			}
		}
	}

	bool ApplyTransform(bool reset){
		array<Object@> targets = target_select.GetTargetObjects();

		for(uint i = 0; i < targets.size(); i++){
			if(transform_mode == transform_to_target){
				array<Object@> target_location_objects = target_location.GetTargetObjects();
				for(uint j = 0; j < target_location_objects.size(); j++){
					if(use_target_location){
						SetTargetTranslation(targets[i], reset?before_translation:GetTargetTranslation(target_location_objects[j]) + translation_offset);
					}
					if(use_target_rotation){
						SetTargetRotation(targets[i], reset?before_rotation:GetTargetRotation(target_location_objects[j]));
					}
					if(use_target_scale){
						vec3 scale = target_location_objects[j].GetScale();
						vec3 bounds = targets[i].GetBoundingBox();
						bounds.x = (bounds.x == 0.0)?1.0:bounds.x;
						bounds.y = (bounds.y == 0.0)?1.0:bounds.y;
						bounds.z = (bounds.z == 0.0)?1.0:bounds.z;
						vec3 new_scale = vec3(scale.x / bounds.x, scale.y / bounds.y, scale.z / bounds.z);
						targets[i].SetScale(reset?before_scale:new_scale);
					}
				}
			}else if(transform_mode == transform_to_placeholder){
				if(!placeholder.Exists()){
					Log(warning, "Placeholder does not exist!");
					return false;
				}
				SetTargetTranslation(targets[i], reset?before_translation:placeholder.GetTranslation());
				SetTargetRotation(targets[i], reset?before_rotation:placeholder.GetRotation());
				vec3 scale = placeholder.GetScale();
				vec3 bounds = targets[i].GetBoundingBox();
				bounds.x = (bounds.x == 0.0)?1.0:bounds.x;
				bounds.y = (bounds.y == 0.0)?1.0:bounds.y;
				bounds.z = (bounds.z == 0.0)?1.0:bounds.z;
				vec3 new_scale = vec3(scale.x / bounds.x, scale.y / bounds.y, scale.z / bounds.z);
				targets[i].SetScale(reset?before_scale:new_scale);
			}else if(transform_mode == move_towards_target){
				array<Object@> target_location_objects = target_location.GetTargetObjects();
				for(uint j = 0; j < target_location_objects.size(); j++){

					vec3 move_direction = normalize(GetTargetTranslation(target_location_objects[j]) - GetTargetTranslation(targets[i]));
					vec3 new_translation = GetTargetTranslation(targets[i]) + (move_direction * time_step * move_speed);

					float rotation_y = atan2(-move_direction.x, -move_direction.z) + (extra_yaw / 180.0f * PI);
					float rotation_x = asin(move_direction.y);
					float rotation_z = 0.0f;
					quaternion new_rotation = quaternion(vec4(0,1,0,rotation_y)) * quaternion(vec4(1,0,0,rotation_x)) * quaternion(vec4(0,0,1,rotation_z));

					SetTargetTranslation(targets[i], reset?before_translation:new_translation);
					SetTargetRotation(targets[i], reset?before_rotation:new_rotation);
				}
			}else if(transform_mode == transform_specific){
				array<Object@> target_location_objects = target_location.GetTargetObjects();
				
				if(s_transform_to_target_first == true){
					for(uint j = 0; j < target_location_objects.size(); j++){
						if(use_target_location){
							SetTargetTranslation(targets[i], reset?before_translation:GetTargetTranslation(target_location_objects[j]) + translation_offset);
						}
						if(use_target_rotation){
							SetTargetRotation(targets[i], reset?before_rotation:GetTargetRotation(target_location_objects[j]));
						}
						if(use_target_scale){
							vec3 scale = target_location_objects[j].GetScale();
							vec3 bounds = targets[i].GetBoundingBox();
							bounds.x = (bounds.x == 0.0)?1.0:bounds.x;
							bounds.y = (bounds.y == 0.0)?1.0:bounds.y;
							bounds.z = (bounds.z == 0.0)?1.0:bounds.z;
							vec3 new_scale = vec3(scale.x / bounds.x, scale.y / bounds.y, scale.z / bounds.z);
							targets[i].SetScale(reset?before_scale:new_scale);
						}
					}
				}
				//Transform to bone stuff
				Log(fatal, "reached bone stuff");
				if (target_location_objects.size() > 0){
					if (target_location_objects[0].GetType() == _movement_object && s_transform_to_bone == true){
						int target_id = target_location_objects[0].GetID();
						MovementObject@ char = ReadCharacterID(target_id);
						if (char.rigged_object().skeleton().IKBoneExists(s_bone_name)){
							int bone = char.rigged_object().skeleton().IKBoneStart(s_bone_name);
							
							if (s_copy_bone_position == true) {
								SetTargetTranslation(targets[i], reset?before_translation: char.rigged_object().GetBonePosition(bone));
							}
							
							if (s_copy_bone_rotation == true){
								quaternion bone_rotation = QuaternionFromMat4(char.rigged_object().GetDisplayBoneMatrix(bone));
								vec3 direction = bone_rotation * vec3(0.0,1.0,0.0);
								vec3 flat_direction = normalize(vec3(direction.x, 0.0, direction.z));
								float rot = atan2(flat_direction.x, flat_direction.z) * 180.0f / PI;
								float new_rotation = floor(rot + 0.5f);
								quaternion flattened_rotation = quaternion(vec4(0, 1, 0, new_rotation * PI / 180.0f));
									if (s_flatten_rotation == true){
										SetTargetRotation(targets[i], reset?before_rotation: flattened_rotation);
									}else{
										SetTargetRotation(targets[i], reset?before_rotation: bone_rotation);
									}
							}
						}
					}
				}
				
				vec3 translation;
				vec3 scale;

				translation.x = (s_transform_x_relative? GetTargetTranslation(targets[i]).x : 0) + s_transform_x;
				translation.y = (s_transform_y_relative? GetTargetTranslation(targets[i]).y : 0) + s_transform_y;
				translation.z = (s_transform_z_relative? GetTargetTranslation(targets[i]).z : 0) + s_transform_z;
				SetTargetTranslation(targets[i], reset?before_translation:translation);


				scale.x = (s_scale_x_relative? targets[i].GetScale().x : 0) + s_scale_x;
				scale.y = (s_scale_y_relative? targets[i].GetScale().y : 0) + s_scale_y;
				scale.z = (s_scale_z_relative? targets[i].GetScale().z : 0) + s_scale_z;
				targets[i].SetScale(reset?before_scale:scale);

				float pi = atan(1)*4.0;
				quaternion new_rotation =
				quaternion(vec4(0,1,0,s_rotate_y*pi/180.0f)) *
				quaternion(vec4(1,0,0,s_rotate_x*pi/180.0f)) *
				quaternion(vec4(0,0,1,s_rotate_z*pi/180.0f)) *
				(s_rotate_relative? GetTargetRotation(targets[i]) : quaternion(0,0,0,1));


				SetTargetRotation(targets[i], reset?before_rotation:new_rotation);

			}else if(transform_mode == transform_to_hotspot){
				SetTargetTranslation(targets[i], reset?before_translation:this_hotspot.GetTranslation());
				SetTargetRotation(targets[i], reset?before_rotation:this_hotspot.GetRotation());
				targets[i].SetScale(reset?before_scale:vec3(1.0, 1.0, 1.0));
			}

			// TODO Remove this once internal_testing is released. Bug has been fixed there.
			RefreshChildren(targets[i]);
		}
		return true;
	}

	void Reset(){
		if(triggered){
			triggered = false;
			ApplyTransform(true);
		}
	}
}
