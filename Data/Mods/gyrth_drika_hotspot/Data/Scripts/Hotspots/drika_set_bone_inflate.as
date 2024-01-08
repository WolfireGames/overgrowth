class DrikaSetBoneInflate : DrikaElement{
	string bone_name;
	float inflate_value;
	array<string> bone_names = {"rightear",
								"leftear",
								"rightarm",
								"leftarm",
								"head",
								"torso",
								"tail",
								"right_leg",
								"left_leg",
								"lefthand",
								"righthand",
								"leftfingers",
								"rightfingers",
								"leftthumb",
								"rightthumb",
								"index"};
	int current_index;
	int bone_index;
	int current_bone_index;

	DrikaSetBoneInflate(JSONValue params = JSONValue()){
		bone_name = GetJSONString(params, "bone_name", "torso");
		inflate_value = GetJSONFloat(params, "inflate_value", 0.0);
		bone_index = GetJSONInt(params, "bone_index", 11);
		current_bone_index = bone_index;

		@target_select = DrikaTargetSelect(this, params);
		target_select.target_option = id_option | name_option | character_option | reference_option | team_option;

		connection_types = {_movement_object};
		drika_element_type = drika_set_bone_inflate;
		has_settings = true;
	}

	void PostInit(){
		target_select.PostInit();
	}

	JSONValue GetSaveData(){
		JSONValue data;
		data["bone_name"] = JSONValue(bone_name);
		data["inflate_value"] = JSONValue(inflate_value);
		if(bone_name == "index"){
			data["bone_index"] = JSONValue(bone_index);
		}
		target_select.SaveIdentifier(data);
		return data;
	}

	void Delete(){
		if(triggered){
			SetBoneInflate(true);
		}
		target_select.Delete();
	}

	string GetDisplayString(){
		return "SetBoneInflate " + target_select.GetTargetDisplayText() + " " + bone_name + " " + ((bone_name == "index")?(bone_index + " "):"") + inflate_value;
	}

	void StartSettings(){
		target_select.CheckAvailableTargets();
	}

	void StartEdit(){
		current_index = 0;
		for(uint i = 0; i < bone_names.size(); i++){
			if(bone_names[i] == bone_name){
				current_index = i;
				break;
			}
		}
		SetBoneInflate(false);
	}

	void EditDone(){
		SetBoneInflate(true);
	}

	void DrawSettings(){

		float option_name_width = 120.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Target Character");
		ImGui_NextColumn();
		ImGui_NextColumn();

		target_select.DrawSelectTargetUI();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Bone");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		ImGui_PushItemWidth(second_column_width);
		if(ImGui_Combo("###Bone", current_index, bone_names, bone_names.size())){
			SetBoneInflate(true);
			bone_name = bone_names[current_index];
			SetBoneInflate(false);
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		if(bone_name == "index"){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Index");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_InputInt("###Index", current_bone_index)){
				SetBoneInflate(true);
				bone_index = current_bone_index;
				SetBoneInflate(false);
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Value");
		ImGui_NextColumn();
		ImGui_PushItemWidth(second_column_width);
		if(ImGui_SliderFloat("###Value", inflate_value, 0.0f, 1.0f, "%.2f")){
			SetBoneInflate(false);
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();
	}

	bool Trigger(){
		triggered = true;
		return SetBoneInflate(false);
	}

	void DrawEditing(){
		array<MovementObject@> targets = target_select.GetTargetMovementObjects();
		for(uint i = 0; i < targets.size(); i++){
			DebugDrawLine(targets[i].position, this_hotspot.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
		}
	}

	bool SetBoneInflate(bool reset){
		array<MovementObject@> targets = target_select.GetTargetMovementObjects();
		if(targets.size() == 0){return false;}
		for(uint i = 0; i < targets.size(); i++){
			if(bone_name == "leftfingers"){
				targets[i].rigged_object().skeleton().SetBoneInflate(13, reset?1.0f:inflate_value);
			}else if(bone_name == "leftthumb"){
				targets[i].rigged_object().skeleton().SetBoneInflate(15, reset?1.0f:inflate_value);
			}else if(bone_name == "rightfingers"){
				targets[i].rigged_object().skeleton().SetBoneInflate(26, reset?1.0f:inflate_value);
			}else if(bone_name == "rightthumb"){
				targets[i].rigged_object().skeleton().SetBoneInflate(28, reset?1.0f:inflate_value);
			}else if(bone_name == "righthand"){
				targets[i].rigged_object().skeleton().SetBoneInflate(24, reset?1.0f:inflate_value);
			}else if(bone_name == "lefthand"){
				targets[i].rigged_object().skeleton().SetBoneInflate(11, reset?1.0f:inflate_value);
			}else if(bone_name == "index"){
				targets[i].rigged_object().skeleton().SetBoneInflate(bone_index, reset?1.0f:inflate_value);
			}else{
				if(!targets[i].rigged_object().skeleton().IKBoneExists(bone_name)){
					continue;
				}

				int bone = targets[i].rigged_object().skeleton().IKBoneStart(bone_name);
				int chain_len = targets[i].rigged_object().skeleton().IKBoneLength(bone_name);

				for(int j = 0; j < chain_len; ++j){
					targets[i].rigged_object().skeleton().SetBoneInflate(bone, reset?1.0f:inflate_value);
					bone = targets[i].rigged_object().skeleton().GetParent(bone);

					if(bone == -1){
						break;
					}
				}
			}
		}
		return true;
	}

	void Reset(){
		if(triggered){
			triggered = false;
			SetBoneInflate(true);
		}
	}
}
