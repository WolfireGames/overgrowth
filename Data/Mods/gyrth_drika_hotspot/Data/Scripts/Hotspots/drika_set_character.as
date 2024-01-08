class DrikaSetCharacter : DrikaElement{
	string character_path;
	array<BeforeValue> original_character_paths;
	bool cache_skeleton_info = true;

	DrikaSetCharacter(JSONValue params = JSONValue()){
		character_path = GetJSONString(params, "character_path", "Data/Characters/guard.xml");
		cache_skeleton_info = GetJSONBool(params, "cache_skeleton_info", true);

		@target_select = DrikaTargetSelect(this, params);
		target_select.target_option = id_option | name_option | character_option | reference_option | team_option;

		connection_types = {_movement_object};
		drika_element_type = drika_set_character;
		has_settings = true;
	}

	void PostInit(){
		target_select.PostInit();
	}

	JSONValue GetSaveData(){
		JSONValue data;
		data["character_path"] = JSONValue(character_path);
		data["cache_skeleton_info"] = JSONValue(cache_skeleton_info);
		target_select.SaveIdentifier(data);
		return data;
	}

	string GetDisplayString(){
		return "SetCharacter " + target_select.GetTargetDisplayText() + " " + character_path;
	}

	void GetOriginalCharacter(){
		array<MovementObject@> targets = target_select.GetTargetMovementObjects();
		original_character_paths.resize(0);
		for(uint i = 0; i < targets.size(); i++){
			original_character_paths.insertLast(BeforeValue());
			original_character_paths[i].string_value = targets[i].char_path;
		}
	}

	void StartSettings(){
		target_select.CheckAvailableTargets();
	}

	void DrawSettings(){
		float option_name_width = 150.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Target Character");
		ImGui_NextColumn();
		ImGui_NextColumn();

		target_select.DrawSelectTargetUI();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Character Path");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();

		if(ImGui_Button("Set Character Path")){
			string new_path = GetUserPickedReadPath("xml", "Data/Characters");
			if(new_path != ""){
				character_path = ShortenPath(new_path);
			}
		}
		ImGui_AlignTextToFramePadding();
		ImGui_SameLine();
		ImGui_Text(character_path);
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Cache Skeleton Info");
		ImGui_NextColumn();
		ImGui_Checkbox("###Cache Skeleton Info", cache_skeleton_info);
		ImGui_NextColumn();
	}

	bool Trigger(){
		if(!triggered){
			GetOriginalCharacter();
		}
		triggered = true;
		return SetParameter(false);
	}

	void DrawEditing(){
		array<MovementObject@> targets = target_select.GetTargetMovementObjects();
		for(uint i = 0; i < targets.size(); i++){
			DebugDrawLine(targets[i].position, this_hotspot.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
		}
	}

	bool SetParameter(bool reset){
		array<MovementObject@> targets = target_select.GetTargetMovementObjects();
		if(targets.size() == 0){return false;}
		for(uint i = 0; i < targets.size(); i++){
			if(targets[i].char_path == (reset?original_character_paths[i].string_value:character_path)){
				continue;
			}
			targets[i].char_path = reset?original_character_paths[i].string_value:character_path;
			string command =	"character_getter.Load(this_mo.char_path);" +
								"this_mo.RecreateRiggedObject(this_mo.char_path);";

			if(cache_skeleton_info){
				command += "CacheSkeletonInfo();";
			}
			if(targets[i].GetIntVar("state") == _ragdoll_state){
				command += "Recover();";
			}
			targets[i].Execute(command);
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
