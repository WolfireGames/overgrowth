enum create_delete_modes{
							_create_at_placeholder,
							_delete_object,
							_create_at_hotspot
						};

class DrikaCreateDelete : DrikaElement{
	string object_path;
	array<int> spawned_object_ids;
	create_delete_modes create_delete_mode;

	array<string> create_delete_mode_names = 	{
													"Create At Placeholder",
													"Delete Object",
													"Create At Hotspot"
												};

	DrikaCreateDelete(JSONValue params = JSONValue()){
		placeholder.Load(params);
		placeholder.name = "Create Object Helper";
		placeholder.default_scale = vec3(1.0);

		object_path = GetJSONString(params, "object_path", default_preview_mesh);
		drika_element_type = drika_create_delete;
		reference_string = GetJSONString(params, "reference_string", "");
		AttemptRegisterReference(reference_string);
		create_delete_mode = create_delete_modes(GetJSONInt(params, "create_delete_mode", _create_at_placeholder));

		@target_select = DrikaTargetSelect(this, params);
		target_select.target_option = reference_option;

		has_settings = true;
		placeholder.object_path = object_path;
	}

	void Delete(){
		Reset();
		placeholder.Remove();
		target_select.Delete();
		RemoveReference(this);
	}

	void PostInit(){
		if(create_delete_mode == _create_at_placeholder){
			placeholder.SetMode(_at_cube);
			placeholder.Retrieve();
		}else if(create_delete_mode == _create_at_hotspot){
			placeholder.SetMode(_at_hotspot);
		}else if(create_delete_mode == _delete_object){
			target_select.PostInit();
		}
	}

	bool UsesPlaceholderObject(){
		return (create_delete_mode == _create_at_placeholder);
	}

	JSONValue GetCheckpointData(){
		JSONValue data;
		data["triggered"] = triggered;

		if(create_delete_mode == _create_at_placeholder || create_delete_mode == _create_at_hotspot){
			if(triggered){
				data["target_ids"] = JSONValue(JSONarrayValue);
				for(uint i = 0; i < spawned_object_ids.size(); i++){
					data["target_ids"].append(spawned_object_ids[i]);
				}
			}
		}else if(create_delete_mode == _delete_object){

		}
		return data;
	}

	void SetCheckpointData(JSONValue data = JSONValue()){
		if(create_delete_mode == _create_at_placeholder || create_delete_mode == _create_at_hotspot){
			bool checkpoint_triggered = data["triggered"].asBool();

			//The hotspot got reset and the target doesn't exist anymore.
			if(checkpoint_triggered && !triggered){
				spawned_object_ids.resize(0);
				Trigger();
			//The hotspot has not been reset but triggered.
			}else if(checkpoint_triggered){
				JSONValue target_ids = data["target_ids"];
				bool missing_object = false;

				spawned_object_ids.resize(0);
				for(uint i = 0; i < target_ids.size(); i++){
					if(!ObjectExists(target_ids[i].asInt())){
						missing_object = true;
						break;
					}else{
						Log(warning, "Not missing " + object_path + " " + target_ids[i].asInt());
					}
					spawned_object_ids.insertLast(target_ids[i].asInt());
				}

				if(missing_object){
					Log(warning, "Missing " + object_path);
					spawned_object_ids.resize(0);
					Trigger();
				}
			}
		}else if(create_delete_mode == _delete_object){

		}
	}

	JSONValue GetSaveData(){
		JSONValue data;
		data["create_delete_mode"] = JSONValue(create_delete_mode);
		if(create_delete_mode == _create_at_placeholder){
			data["object_path"] = JSONValue(object_path);
			data["reference_string"] = JSONValue(reference_string);
			placeholder.Save(data);
		}else if(create_delete_mode == _create_at_hotspot){
			data["object_path"] = JSONValue(object_path);
			data["reference_string"] = JSONValue(reference_string);
		}else if(create_delete_mode == _delete_object){
			target_select.SaveIdentifier(data);
		}
		return data;
	}

	string GetDisplayString(){
		string display_string = create_delete_mode_names[create_delete_mode];

		if(create_delete_mode == _create_at_placeholder){
			display_string += " " + object_path + " " + reference_string;
		}else if(create_delete_mode == _create_at_hotspot){
			display_string += " " + object_path + " " + reference_string;
		}else{
			display_string += " " + target_select.GetTargetDisplayText();
		}

		return display_string;
	}

	void StartSettings(){
		if(create_delete_mode == _delete_object){
			target_select.CheckAvailableTargets();
		}
	}

	void StartEdit(){
		DrikaElement::StartEdit();
		if(create_delete_mode == _create_at_placeholder){
			placeholder.AddPlaceholderObject();
		}else if(create_delete_mode == _create_at_hotspot){
			placeholder.AddPlaceholderObject();
		}
	}

	void DrawSettings(){
		float option_name_width = 120.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Mode");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		ImGui_PushItemWidth(second_column_width);
		int current_mode = create_delete_mode;
		if(ImGui_Combo("##Mode", current_mode, create_delete_mode_names, create_delete_mode_names.size())){
			create_delete_mode = create_delete_modes(current_mode);
			if(create_delete_mode == _create_at_placeholder){
				placeholder.SetMode(_at_cube);
				AttemptRegisterReference(reference_string);
			}else if(create_delete_mode == _create_at_hotspot){
				placeholder.SetMode(_at_hotspot);
				placeholder.RemoveCubeObject();
				AttemptRegisterReference(reference_string);
			}else if(create_delete_mode == _delete_object){
				RemoveReference(this);
				placeholder.Remove();
				target_select.CheckAvailableTargets();
			}
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		if(create_delete_mode == _create_at_placeholder || create_delete_mode == _create_at_hotspot){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Object Path");
			ImGui_NextColumn();

			if(ImGui_Button("Set Object Path")){
				string new_path = GetUserPickedReadPath("xml", "Data/Objects");
				if(new_path != ""){
					object_path = ShortenPath(new_path);
					placeholder.object_path = object_path;
					placeholder.UpdatePlaceholderPreview();
				}
			}
			ImGui_SameLine();
			ImGui_Text(object_path);
			ImGui_NextColumn();
			DrawSetReferenceUI();
		}else if(create_delete_mode == _delete_object){
			target_select.DrawSelectTargetUI();
		}
	}

	void DrawEditing(){
		if(create_delete_mode == _create_at_placeholder){
			if(placeholder.Exists()){
				DebugDrawLine(placeholder.GetTranslation(), this_hotspot.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
				DrawGizmo(placeholder.GetTranslation(), placeholder.GetRotation(), placeholder.GetScale(), placeholder.IsSelected());
				placeholder.DrawEditing();
			}else{
				placeholder.Create();
				StartEdit();
			}
		}else if(create_delete_mode == _create_at_hotspot){
			if(placeholder.PlaceholderExists()){
				placeholder.DrawEditing();
			}else{
				StartEdit();
			}
		}else if(create_delete_mode == _delete_object){
			array<Object@> targets = target_select.GetTargetObjects();
			for(uint i = 0; i < targets.size(); i++){
				DebugDrawLine(targets[i].GetTranslation(), this_hotspot.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
			}
		}
	}

	void Reset(){
		if(create_delete_mode == _create_at_placeholder || create_delete_mode == _create_at_hotspot){
			if(triggered){
				for(uint i = 0; i < spawned_object_ids.size(); i++){
					DeleteObjectID(spawned_object_ids[i]);
				}
				spawned_object_ids.resize(0);
				triggered = false;
			}
		}
	}

	bool Trigger(){
		triggered = true;
		if(create_delete_mode == _create_at_placeholder){
			if(placeholder.Exists()){
				//Check if any of the spawned items have been deleted first.
				for(uint i = 0; i < spawned_object_ids.size(); i++){
					if(!ObjectExists(spawned_object_ids[i])){
						spawned_object_ids.removeAt(i);
						i--;
					}
				}

				int spawned_object_id = CreateObject(object_path);
				spawned_object_ids.insertLast(spawned_object_id);

				Object@ spawned_object = ReadObjectFromID(spawned_object_id);
				spawned_object.SetSelectable(true);
				spawned_object.SetTranslatable(true);
				spawned_object.SetScalable(true);
				spawned_object.SetRotatable(true);
				spawned_object.SetTranslation(placeholder.GetTranslation());
				spawned_object.SetRotation(placeholder.GetRotation());
				// Since weapons and character can't be scaled, skip setting the scale on them.
				if(spawned_object.GetType() == _env_object || spawned_object.GetType() == _hotspot_object){
					spawned_object.SetScale(placeholder.GetScale());
				}
				AttemptRegisterReference(reference_string);
				return true;
			}else{
				placeholder.Create();
				return false;
			}
		}else if(create_delete_mode == _create_at_hotspot){
			//Check if any of the spawned items have been deleted first.
			for(uint i = 0; i < spawned_object_ids.size(); i++){
				if(!ObjectExists(spawned_object_ids[i])){
					spawned_object_ids.removeAt(i);
					i--;
				}
			}

			int spawned_object_id = CreateObject(object_path);
			spawned_object_ids.insertLast(spawned_object_id);

			Object@ spawned_object = ReadObjectFromID(spawned_object_id);
			spawned_object.SetSelectable(true);
			spawned_object.SetTranslatable(true);
			spawned_object.SetScalable(true);
			spawned_object.SetRotatable(true);
			spawned_object.SetTranslation(this_hotspot.GetTranslation());
			spawned_object.SetRotation(this_hotspot.GetRotation());
			AttemptRegisterReference(reference_string);
			return true;
		}else if(create_delete_mode == _delete_object){
			array<Object@> targets = target_select.GetTargetObjects();
			for(uint i = 0; i < targets.size(); i++){
				Log(warning, "Delete object name " + targets[i].GetName());
				Log(warning, "Delete object id " + targets[i].GetID());
				//Don't delete any DHS placeholders/helpers.
				if(targets[i].GetName().findFirst("Helper") == -1){
					QueueDeleteObjectID(targets[i].GetID());
				}
			}
			return true;
		}
		return false;
	}

	array<int> GetReferenceObjectIDs(){
		if(!triggered && create_delete_mode == _create_at_placeholder){
			placeholder.UpdatePlaceholderTransform();
			return {placeholder.cube_object.GetID()};
		}else{
			//Make sure the spawned objects are still there.
			array<int> reference_object_ids;
			for(uint i = 0; i < spawned_object_ids.size(); i++){
				if(ObjectExists(spawned_object_ids[i])){
					reference_object_ids.insertLast(spawned_object_ids[i]);
				}
			}
			return reference_object_ids;
		}
	}

	void ReceiveMessage(string message, string identifier){
		placeholder.ReceiveMessage(message, identifier);
	}

	void ReceiveMessage(string message){
		placeholder.ReceiveMessage(message);
	}

	string GetReferenceString(){
		if(create_delete_mode == _create_at_placeholder || create_delete_mode == _create_at_hotspot){
			return reference_string;
		}else{
			return "";
		}
	}

	void HotspotStartEdit(){
		if(create_delete_mode == _create_at_placeholder || create_delete_mode == _create_at_hotspot){
			placeholder.AddPlaceholderObject();
		}
	}

	void HotspotStopEdit(){
		if(create_delete_mode == _create_at_placeholder || create_delete_mode == _create_at_hotspot){
			placeholder.HidePlaceholderObject();
		}
	}
}
