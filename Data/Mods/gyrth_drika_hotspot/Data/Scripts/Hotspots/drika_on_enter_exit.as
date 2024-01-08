enum hotspot_trigger_types	{
								on_character_enter = 0,
								on_character_exit = 1,
								while_character_inside = 2,
								while_character_outside = 3,
								on_item_enter = 4,
								on_item_exit = 5,
								while_item_inside = 6,
								while_item_outside = 7,
								on_object_enter = 8,
								on_object_exit = 9,
								while_object_inside = 10,
								while_object_outside = 11,
								while_character_in_view = 12
							};

array<string> hotspot_trigger_choices = {
											"On Character Enter",
											"On Character Exit",
											"While Character Inside",
											"While Character Outside",
											"On Item Enter",
											"On Item Exit",
											"While Item Inside",
											"While Item Outside",
											"On Object Enter",
											"On Object Exit",
											"While Object Inside",
											"While Object Outside",
											"While Character In View"
										};

class DrikaOnEnterExit : DrikaElement{
	int new_hotspot_trigger_type;
	bool external_hotspot;
	int external_hotspot_id;
	Object@ external_hotspot_obj = null;
	bool reset_when_false;
	array<int> objects_inside;
	bool got_objects_inside = false;
	array<int> reference_ids;
	bool initial_setup_done = false;
	bool check_all;
	bool view_obstruction_check;
	float camera_fov;

	vec3 external_hotspot_translation;
	quaternion external_hotspot_rotation;
	vec3 external_hotspot_scale;

	hotspot_trigger_types hotspot_trigger_type;

	DrikaOnEnterExit(JSONValue params = JSONValue()){
		@target_select = DrikaTargetSelect(this, params);

		hotspot_trigger_type = hotspot_trigger_types(GetJSONInt(params, "hotspot_trigger_type", on_character_enter));

		reference_string = GetJSONString(params, "reference_string", "");
		AttemptRegisterReference(reference_string);
		external_hotspot = GetJSONBool(params, "external_hotspot", false);
		external_hotspot_translation = GetJSONVec3(params, "external_hotspot_translation", this_hotspot.GetTranslation() + vec3(0.0, 2.0, 0.0));
		external_hotspot_rotation = GetJSONQuaternion(params, "external_hotspot_rotation", quaternion());
		external_hotspot_scale = GetJSONVec3(params, "external_hotspot_scale", vec3(0.25));
		external_hotspot_id = GetJSONInt(params, "external_hotspot_id", -1);
		reset_when_false = GetJSONBool(params, "reset_when_false", false);
		check_all = GetJSONBool(params, "check_all", false);

		//Converting old savedata into new, to be removed later on.
		drika_element_types function_type = drika_element_types(params["function"].asInt());
		if(function_type == drika_on_item_enter_exit){
			int old_trigger_type = GetJSONInt(params, "hotspot_trigger_type", 0);
			if(old_trigger_type == 0){
				hotspot_trigger_type = on_item_enter;
			}else if(old_trigger_type == 1){
				hotspot_trigger_type = on_item_exit;
			}else if(old_trigger_type == 2){
				hotspot_trigger_type = while_item_inside;
			}else if(old_trigger_type == 3){
				hotspot_trigger_type = while_item_outside;
			}
		}else{
			int old_character_type = GetJSONInt(params, "target_character_type", -1);
			if(old_character_type != -1){
				if(old_character_type == 1){
					target_select.identifier_type = team;
				}else if(old_character_type == 2){
					target_select.identifier_type = any_character;
				}else if(old_character_type == 3){
					target_select.identifier_type = any_player;
				}else if(old_character_type == 4){
					target_select.identifier_type = any_npc;
				}

				int old_trigger_type = GetJSONInt(params, "hotspot_trigger_type", 0);
				if(old_trigger_type == 0){
					hotspot_trigger_type = on_character_enter;
				}else if(old_trigger_type == 1){
					hotspot_trigger_type = on_character_exit;
				}else if(old_trigger_type == 2){
					hotspot_trigger_type = while_character_inside;
				}else if(old_trigger_type == 3){
					hotspot_trigger_type = while_character_outside;
				}

				target_select.object_id = GetJSONInt(params, "object_id", -1);
				target_select.character_team = GetJSONString(params, "character_team", "");
			}
		}

		new_hotspot_trigger_type = hotspot_trigger_type;

		connection_types = {};
		drika_element_type = drika_on_enter_exit;
		has_settings = true;
		SetTargetOptions();
	}

	void SetTargetOptions(){
		if(IsCharacterFunction()){
			target_select.target_option = id_option | name_option | character_option | reference_option | team_option | any_character_option | any_player_option | any_npc_option;
			connection_types = {_movement_object};
		}else if(IsItemFunction()){
			target_select.target_option = id_option | name_option | item_option | reference_option | any_item_option;
			connection_types = {_item_object};
		}else{
			target_select.target_option = id_option | name_option | reference_option;
			connection_types = {_env_object};
		}
	}

	bool IsCharacterFunction(){
		if(hotspot_trigger_type == on_character_enter || hotspot_trigger_type == on_character_exit || hotspot_trigger_type == while_character_inside || hotspot_trigger_type == while_character_outside || hotspot_trigger_type == while_character_in_view){
			return true;
		}
		return false;
	}
	bool IsItemFunction(){
		if(hotspot_trigger_type == on_item_enter || hotspot_trigger_type == on_item_exit || hotspot_trigger_type == while_item_inside || hotspot_trigger_type == while_item_outside){
			return true;
		}
		return false;
	}

	bool IsObjectFunction(){
		if(hotspot_trigger_type == on_object_enter || hotspot_trigger_type == on_object_exit || hotspot_trigger_type == while_object_inside || hotspot_trigger_type == while_object_outside){
			return true;
		}
		return false;
	}

	bool IsWhileFunction(){
		if(hotspot_trigger_type == while_character_inside || hotspot_trigger_type == while_character_outside || hotspot_trigger_type == while_item_inside || hotspot_trigger_type == while_item_outside || hotspot_trigger_type == while_object_inside || hotspot_trigger_type == while_object_outside || hotspot_trigger_type == while_character_in_view){
			return true;
		}
		return false;
	}

	JSONValue GetSaveData(){
		JSONValue data;
		data["hotspot_trigger_type"] = JSONValue(hotspot_trigger_type);
		data["reference_string"] = JSONValue(reference_string);
		data["external_hotspot"] = JSONValue(external_hotspot);
		if(external_hotspot){
			data["external_hotspot_id"] = JSONValue(external_hotspot_id);
			if(exporting){
				vec3 translation = external_hotspot_obj.GetTranslation();
				quaternion rotation = external_hotspot_obj.GetRotation();
				vec3 scale = external_hotspot_obj.GetScale();

				data["external_hotspot_translation"] = JSONValue(JSONarrayValue);
				data["external_hotspot_translation"].append(translation.x);
				data["external_hotspot_translation"].append(translation.y);
				data["external_hotspot_translation"].append(translation.z);

				data["external_hotspot_rotation"] = JSONValue(JSONarrayValue);
				data["external_hotspot_rotation"].append(rotation.x);
				data["external_hotspot_rotation"].append(rotation.y);
				data["external_hotspot_rotation"].append(rotation.z);
				data["external_hotspot_rotation"].append(rotation.w);

				data["external_hotspot_scale"] = JSONValue(JSONarrayValue);
				data["external_hotspot_scale"].append(scale.x);
				data["external_hotspot_scale"].append(scale.y);
				data["external_hotspot_scale"].append(scale.z);
			}
		}
		if(IsWhileFunction()){
			data["reset_when_false"] = JSONValue(reset_when_false);
			data["check_all"] = JSONValue(check_all);
		}

		if(hotspot_trigger_type == while_character_in_view){
			data["view_obstruction_check"] = JSONValue(view_obstruction_check);
		}

		target_select.SaveIdentifier(data);
		return data;
	}

	void PostInit(){
		if(external_hotspot){
			if(duplicating_hotspot){
				if(ObjectExists(external_hotspot_id)){
					//Use the same transform as the original external hotspot.
					Object@ old_hotspot = ReadObjectFromID(external_hotspot_id);
					CreateExternalHotspot();
					external_hotspot_obj.SetScale(old_hotspot.GetScale());
					external_hotspot_obj.SetTranslation(old_hotspot.GetTranslation());
					external_hotspot_obj.SetRotation(old_hotspot.GetRotation());
				}else{
					external_hotspot_id = -1;
				}
			}else if(importing){
				CreateExternalHotspot();
				external_hotspot_obj.SetTranslation(external_hotspot_translation);
				external_hotspot_obj.SetRotation(external_hotspot_rotation);
				external_hotspot_obj.SetScale(external_hotspot_scale);
			}else{
				if(ObjectExists(external_hotspot_id)){
					@external_hotspot_obj = ReadObjectFromID(external_hotspot_id);
					external_hotspot_obj.SetName("Drika External Hotspot");
				}else{
					CreateExternalHotspot();
				}
			}

			if(external_hotspot_obj !is null){
				external_hotspot_obj.SetSelected(false);
				external_hotspot_obj.SetSelectable(false);
			}
		}
		target_select.PostInit();
	}

	void Update(){
		if(external_hotspot_id != -1 && !ObjectExists(external_hotspot_id) && external_hotspot){
			external_hotspot = false;
			external_hotspot_id = -1;
			@external_hotspot_obj = null;
		}else if(external_hotspot_id == -1 && external_hotspot){
			CreateExternalHotspot();
		}else if(external_hotspot_id != -1 && !external_hotspot){
			QueueDeleteObjectID(external_hotspot_id);
			@external_hotspot_obj = null;
			external_hotspot_id = -1;
		}
	}

	void Delete(){
		if(external_hotspot && ObjectExists(external_hotspot_id)){
			QueueDeleteObjectID(external_hotspot_id);
			@external_hotspot_obj = null;
		}
		target_select.Delete();
	}

	void LeftClick(){
		if(this_hotspot.IsSelected() && ObjectExists(external_hotspot_id)){
			this_hotspot.SetSelected(false);
			external_hotspot_obj.SetSelected(true);
		}else if(ObjectExists(external_hotspot_id) && external_hotspot_obj.IsSelected()){
			external_hotspot_obj.SetSelected(false);
			this_hotspot.SetSelected(false);
		}else{
			if(ObjectExists(external_hotspot_id)){
				external_hotspot_obj.SetSelected(false);
			}
			this_hotspot.SetSelected(true);
		}
	}

	void EditDone(){
		if(external_hotspot && ObjectExists(external_hotspot_id)){
			external_hotspot_obj.SetSelected(false);
			external_hotspot_obj.SetSelectable(false);
		}
	}

	void StartEdit(){
		if(external_hotspot && ObjectExists(external_hotspot_id)){
			external_hotspot_obj.SetSelectable(true);
		}
	}

	void StartSettings(){
		target_select.CheckAvailableTargets();
	}

	string GetReference(){
		return reference_string;
	}

	string GetDisplayString(){
		string display_string = "";

		display_string += hotspot_trigger_choices[hotspot_trigger_type] + " ";
		display_string += target_select.GetTargetDisplayText();

		return display_string;
	}

	void DrawSettings(){
		float option_name_width = 170.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Trigger when");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		ImGui_PushItemWidth(second_column_width);
		if(ImGui_Combo("###Trigger when", new_hotspot_trigger_type, hotspot_trigger_choices, hotspot_trigger_choices.size())){
			hotspot_trigger_type = hotspot_trigger_types(new_hotspot_trigger_type);
			SetTargetOptions();
			target_select.CheckAvailableTargets();
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		if(IsWhileFunction()){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Reset When False");
			ImGui_NextColumn();
			ImGui_Checkbox("##Reset When False", reset_when_false);
			ImGui_NextColumn();
		}

		if(hotspot_trigger_type == while_character_in_view){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("View Obstruction Check");
			ImGui_NextColumn();
			ImGui_Checkbox("###View Obstruction Check", view_obstruction_check);
			ImGui_NextColumn();
		}

		target_select.DrawSelectTargetUI();

		if((target_select.identifier_type == team) && IsWhileFunction()){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Check Method");
			ImGui_NextColumn();
			ImGui_AlignTextToFramePadding();

			ImGui_PushStyleVar(ImGuiStyleVar_ItemSpacing, vec2(0.0));
			if(!check_all){
				ImGui_PushStyleColor(ImGuiCol_Button, item_hovered);
			}else{
				ImGui_PushStyleColor(ImGuiCol_ButtonHovered, titlebar_color);
			}
			if(ImGui_Button("Check All")){
				check_all = true;
			}
			ImGui_PopStyleColor();

			ImGui_SameLine();

			if(check_all){
				ImGui_PushStyleColor(ImGuiCol_Button, item_hovered);
			}else{
				ImGui_PushStyleColor(ImGuiCol_ButtonHovered, titlebar_color);
			}
			if(ImGui_Button("Check Any")){
				check_all = false;
			}
			ImGui_PopStyleVar();
			ImGui_PopStyleColor();

			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}

		ImGui_AlignTextToFramePadding();
		ImGui_Text("External Hotspot");
		ImGui_NextColumn();
		ImGui_Checkbox("###External Hotspot", external_hotspot);
		ImGui_NextColumn();

		DrawSetReferenceUI();
	}

	void DrawEditing(){
		if(IsCharacterFunction()){
			array<MovementObject@> chars = target_select.GetTargetMovementObjects();
			for(uint i = 0; i < chars.size(); i++){
				DebugDrawLine(chars[i].position, this_hotspot.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
			}
		}else if(IsItemFunction()){
			array<Object@> objs = target_select.GetTargetObjects();
			for(uint i = 0; i < objs.size(); i++){
				if(objs[i].GetType() == _item_object){
					ItemObject@ io = ReadItemID(objs[i].GetID());
					DebugDrawLine(io.GetPhysicsPosition(), this_hotspot.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
				}
			}
		}else{
			array<Object@> objs = target_select.GetTargetObjects();
			for(uint i = 0; i < objs.size(); i++){
				DebugDrawLine(objs[i].GetTranslation(), this_hotspot.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
			}
		}

		if(@external_hotspot_obj != null){
			DebugDrawLine(external_hotspot_obj.GetTranslation(), this_hotspot.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
		}
	}

	void CreateExternalHotspot(){
		external_hotspot_id = CreateObject("Data/Objects/Hotspots/drika_external_hotspot.xml", false);
		@external_hotspot_obj = ReadObjectFromID(external_hotspot_id);
		external_hotspot_obj.SetName("Drika External Hotspot");
		external_hotspot_obj.SetSelectable(true);
		external_hotspot_obj.SetTranslatable(true);
		external_hotspot_obj.SetScalable(true);
		external_hotspot_obj.SetRotatable(true);
		external_hotspot_obj.SetScale(vec3(0.25));
		external_hotspot_obj.SetTranslation(this_hotspot.GetTranslation() + vec3(0.0, 2.0, 0.0));
		ScriptParams@ external_params = external_hotspot_obj.GetScriptParams();
		external_params.SetInt("Target Drika Hotspot", this_hotspot.GetID());
	}

	void ReceiveMessage(string message, int param){
		//This is triggered by characters entering/exiting the current hotspot.
		if(!external_hotspot){
			CheckEvent(message, param);
		}

		if(hotspot_trigger_type == while_character_inside || hotspot_trigger_type == while_character_outside){
			//Send the enter/exit events to the next element in case those are used in the next element.
			if(hotspot_trigger_type == while_character_inside && InsideCheck() || hotspot_trigger_type == while_character_outside && !InsideCheck()){
				DrikaElement@ next_element = drika_elements[drika_indexes[index + 1]];
				next_element.ReceiveMessage(message, param);
			}
		}
	}

	void ReceiveMessage(string message, int param_1, int param_2){
		//This function is triggered when a character enters/exits an external hotspot.
		//Check if the enter/exit signal is from the external hotspot.
		if(param_2 == external_hotspot_id){
			CheckEvent(message, param_1);
		}


		if(hotspot_trigger_type == while_character_inside || hotspot_trigger_type == while_character_outside){
			//Send the enter/exit events to the next element in case those are used in the next element.
			if(hotspot_trigger_type == while_character_inside && InsideCheck() || hotspot_trigger_type == while_character_outside && !InsideCheck()){
				DrikaElement@ next_element = drika_elements[drika_indexes[index + 1]];
				next_element.ReceiveMessage(message, param_1, param_2);
			}
		}
	}

	void CheckEvent(string event, int char_id){
		if(hotspot_trigger_type == while_character_inside || hotspot_trigger_type == while_character_outside){
			return;
		}
		if(!ObjectExists(char_id)){
			return;
		}

		if((hotspot_trigger_type == on_character_enter && event == "enter") ||
			(hotspot_trigger_type == on_character_exit && event == "exit")){
			array<MovementObject@> chars = target_select.GetTargetMovementObjects();
			for(uint i = 0; i < chars.size(); i++){
				if(chars[i].GetID() == char_id){
					triggered = true;
					reference_ids.insertLast(char_id);
					return;
				}
			}
		}
	}

	void Reset(){
		triggered = false;
		got_objects_inside = false;
		initial_setup_done = false;
	}

	bool Trigger(){
		if(!initial_setup_done){
			reference_ids.resize(0);
			initial_setup_done = true;
		}

		if(hotspot_trigger_type == while_character_inside || hotspot_trigger_type == while_character_outside || hotspot_trigger_type == while_character_in_view){
			if(hotspot_trigger_type == while_character_inside && InsideCheck() || hotspot_trigger_type == while_character_outside && !InsideCheck() || hotspot_trigger_type == while_character_in_view && InsideViewCheck()){
				triggered = true;

				//If this is the last element then just return true to finish the script.
				if(current_line == int(drika_indexes.size() - 1)){
					Reset();
					return true;
				}
				DrikaElement@ next_element = drika_elements[drika_indexes[index + 1]];
				if(next_element.Trigger()){
					Reset();
					//The next element has finished so go to the next element.
					current_line += 1;
					return true;
				}
			}else{
				//If the while has been triggered, but the next function is not then be able to reset.
				if(triggered && reset_when_false){
					//At the end of the script so can't reset the next function.
					if(current_line == int(drika_indexes.size() - 1)){
						Reset();
						return true;
					}
					triggered = false;
					DrikaElement@ next_element = drika_elements[drika_indexes[index + 1]];
					next_element.Reset();
				}
			}
			return false;
		}else if(hotspot_trigger_type == on_character_enter || hotspot_trigger_type == on_character_exit){
			if(triggered){
				Reset();
				return true;
			}else{
				return false;
			}
		}else if(IsItemFunction()){
			if(!got_objects_inside){
				objects_inside = GetItemsInside();
			}
			array<Object@> objects = target_select.GetTargetObjects();


			if(hotspot_trigger_type == on_item_enter){
				array<int> new_objects_inside = GetItemsInside();
				for(uint i = 0; i < objects.size(); i++){
					int obj_id = objects[i].GetID();
					if(objects_inside.find(obj_id) == -1 && new_objects_inside.find(obj_id) != -1){
						reference_ids.insertLast(obj_id);
						Reset();
						return true;
					}
				}
				objects_inside = new_objects_inside;
			}else if(hotspot_trigger_type == on_item_exit){
				array<int> new_objects_inside = GetItemsInside();
				for(uint i = 0; i < objects.size(); i++){
					int obj_id = objects[i].GetID();
					if(objects_inside.find(obj_id) != -1 && new_objects_inside.find(obj_id) == -1){
						reference_ids.insertLast(obj_id);
						Reset();
						return true;
					}
				}
				objects_inside = new_objects_inside;
			}else if(hotspot_trigger_type == while_item_inside){
				for(uint i = 0; i < objects.size(); i++){
					int obj_id = objects[i].GetID();
					if(objects_inside.find(obj_id) != -1){
						reference_ids.insertLast(obj_id);
						Reset();
						return true;
					}
				}
			}else if(hotspot_trigger_type == while_item_outside){
				for(uint i = 0; i < objects.size(); i++){
					int obj_id = objects[i].GetID();
					if(objects_inside.find(obj_id) == -1){
						reference_ids.insertLast(obj_id);
						Reset();
						return true;
					}
				}
			}

			objects_inside = GetItemsInside();
			got_objects_inside = true;
			return false;
		}else if(IsObjectFunction()){
			if(!got_objects_inside){
				objects_inside = GetObjectsInside();
			}
			array<Object@> objects = target_select.GetTargetObjects();


			if(hotspot_trigger_type == on_object_enter){
				array<int> new_objects_inside = GetObjectsInside();
				for(uint i = 0; i < objects.size(); i++){
					int obj_id = objects[i].GetID();
					if(objects_inside.find(obj_id) == -1 && new_objects_inside.find(obj_id) != -1){
						reference_ids.insertLast(obj_id);
						Reset();
						return true;
					}
				}
				objects_inside = new_objects_inside;
			}else if(hotspot_trigger_type == on_object_exit){
				array<int> new_objects_inside = GetObjectsInside();
				for(uint i = 0; i < objects.size(); i++){
					int obj_id = objects[i].GetID();
					if(objects_inside.find(obj_id) != -1 && new_objects_inside.find(obj_id) == -1){
						reference_ids.insertLast(obj_id);
						Reset();
						return true;
					}
				}
				objects_inside = new_objects_inside;
			}else if(hotspot_trigger_type == while_object_inside){
				for(uint i = 0; i < objects.size(); i++){
					int obj_id = objects[i].GetID();
					if(objects_inside.find(obj_id) != -1){
						reference_ids.insertLast(obj_id);
						Reset();
						return true;
					}
				}
			}else if(hotspot_trigger_type == while_object_outside){
				for(uint i = 0; i < objects.size(); i++){
					int obj_id = objects[i].GetID();
					if(objects_inside.find(obj_id) == -1){
						reference_ids.insertLast(obj_id);
						Reset();
						return true;
					}
				}
			}

			objects_inside = GetObjectsInside();
			got_objects_inside = true;
			return false;
		}

		return false;
	}

	bool InsideCheck(){
		Object@ target_hotspot = external_hotspot?external_hotspot_obj:this_hotspot;
		reference_ids.resize(0);
		bool all_inside = true;
		bool one_inside = false;

		array<MovementObject@> chars = target_select.GetTargetMovementObjects();
		for(uint i = 0; i < chars.size(); i++){
			if(CharacterInside(chars[i], target_hotspot)){
				if(hotspot_trigger_type == while_character_inside){
					reference_ids.insertLast(chars[i].GetID());
				}
				one_inside = true;
			}else{
				if(hotspot_trigger_type == while_character_outside){
					reference_ids.insertLast(chars[i].GetID());
				}
				all_inside = false;
			}
		}

		if(check_all){
			return all_inside;
		}else{
			return one_inside;
		}
	}

	bool InsideViewCheck(){
		Object@ target_hotspot = external_hotspot?external_hotspot_obj:this_hotspot;
		reference_ids.resize(0);
		bool all_inside = true;
		bool one_inside = false;

		float camera_fov = camera.GetFOV();
		vec3 camera_facing = camera.GetFacing();
		vec3 camera_position = camera.GetPos();

		vec2 screen_dims = vec2(GetScreenWidth(), GetScreenHeight());
		float screen_aspect_ratio = screen_dims.x / screen_dims.y;

		array<MovementObject@> chars = target_select.GetTargetMovementObjects();
		for(uint i = 0; i < chars.size(); i++){

			RiggedObject@ rigged_object = chars[i].rigged_object();
			Skeleton@ skeleton = rigged_object.skeleton();
			int chest_bone = skeleton.IKBoneStart("torso");
			BoneTransform chest_frame_matrix = rigged_object.GetFrameMatrix(chest_bone);
			vec3 character_direction = normalize(chest_frame_matrix.origin - camera_position);

			bool view_obstruction_check_valid = true;
			if(view_obstruction_check){
				vec3 result = col.GetRayCollision(camera_position, chest_frame_matrix.origin);
				view_obstruction_check_valid = distance(chest_frame_matrix.origin, result) < 0.1;
			}

			float dot_product = dot(camera_facing, character_direction) / (length(camera_facing) * length(character_direction));
			float angle = acos(dot_product) / 3.14159265f * 180.0f;
			// Since the viewport isn't perfectly square at 90 degrees by default, we need to calculate
			// a new FOV based on the screen aspect ratio.
			float fov_radians = camera_fov * 3.1415f / 180.0f;
			float vertical_fov = 2.0 * atan(tan(fov_radians / 2.0) * screen_aspect_ratio);
			// Convert the radians back into degrees.
			vertical_fov = (vertical_fov / 3.14159265f * 180.0f);
			// TODO Do a vertical as well as a horizontal check to see if the character is offscreen.
			// Log(warning, vertical_fov + " onscreen " + (angle < vertical_fov / 2.0));
			// If the angle to the character is more than half the FOV then it's off screen.
			if(angle < vertical_fov / 2.0 && view_obstruction_check_valid){
				reference_ids.insertLast(chars[i].GetID());
				one_inside = true;
			}else{
				all_inside = false;
			}
		}

		if(check_all){
			return all_inside;
		}else{
			return one_inside;
		}
	}

	bool CharacterInside(MovementObject@ char, Object@ hotspot_obj){
		if(hotspot_obj is null){
			return false;
		}

		mat4 hotspot_transform = hotspot_obj.GetTransform();
		vec3 char_translation = char.position;
		vec3 local_space_translation = invert(hotspot_transform) * char_translation;

		bool is_inside = (	local_space_translation.x >= -2 && local_space_translation.x <= 2 &&
							local_space_translation.y >= -2 && local_space_translation.y <= 2 &&
							local_space_translation.z >= -2 && local_space_translation.z <= 2);

		return is_inside;
	}

	array<int> GetItemsInside(){
		Object@ target_hotspot = external_hotspot?external_hotspot_obj:this_hotspot;
		array<int> object_ids = GetObjectIDsType(_item_object);
		mat4 hotspot_transform = target_hotspot.GetTransform();
		array<int> inside_ids;

		for(uint i = 0; i < object_ids.size(); i++){
			ItemObject@ io = ReadItemID(object_ids[i]);

			vec3 io_translation = io.GetPhysicsPosition();
			vec3 local_space_translation = invert(hotspot_transform) * io_translation;

			if(local_space_translation.x >= -2 && local_space_translation.x <= 2 &&
				local_space_translation.y >= -2 && local_space_translation.y <= 2 &&
				local_space_translation.z >= -2 && local_space_translation.z <= 2){
				inside_ids.insertLast(object_ids[i]);
			}
		}
		return inside_ids;
	}

	array<int> GetObjectsInside(){
		Object@ target_hotspot = external_hotspot?external_hotspot_obj:this_hotspot;
		array<int> object_ids = GetObjectIDsType(_env_object);
		mat4 hotspot_transform = target_hotspot.GetTransform();
		array<int> inside_ids;

		for(uint i = 0; i < object_ids.size(); i++){
			Object@ obj = ReadObjectFromID(object_ids[i]);
			vec3 obj_translation = obj.GetTranslation();
			vec3 local_space_translation = invert(hotspot_transform) * obj_translation;

			if(local_space_translation.x >= -2 && local_space_translation.x <= 2 &&
				local_space_translation.y >= -2 && local_space_translation.y <= 2 &&
				local_space_translation.z >= -2 && local_space_translation.z <= 2){
				inside_ids.insertLast(object_ids[i]);
			}
		}
		return inside_ids;
	}

	array<int> GetReferenceObjectIDs(){
		return reference_ids;
	}
}
