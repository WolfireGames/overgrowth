class DrikaSetMorphTarget : DrikaElement{
	string morph_1;
	string morph_2;
	float weight;
	float smoothing_duration;
	bool two_way_morph;
	array<string> available_morphs;
	int morph_1_index;
	int morph_2_index;
	float timer = 0.0;

	DrikaSetMorphTarget(JSONValue params = JSONValue()){
		morph_1 = GetJSONString(params, "morph_1", "mouth_open");
		morph_2 = GetJSONString(params, "morph_2", "mouth_open");
		weight = GetJSONFloat(params, "weight", 1.0);
		smoothing_duration = GetJSONFloat(params, "smoothing_duration", 0.0);
		two_way_morph = GetJSONBool(params, "two_way_morph", false);

		@target_select = DrikaTargetSelect(this, params);
		target_select.target_option = id_option | name_option | character_option | reference_option | team_option;

		connection_types = {_movement_object};
		drika_element_type = drika_set_morph_target;
		has_settings = true;
	}

	void PostInit(){
		target_select.PostInit();
	}

	JSONValue GetSaveData(){
		JSONValue data;
		data["morph_1"] = JSONValue(morph_1);
		data["morph_2"] = JSONValue(morph_2);
		data["weight"] = JSONValue(weight);
		data["smoothing_duration"] = JSONValue(smoothing_duration);
		data["two_way_morph"] = JSONValue(two_way_morph);
		target_select.SaveIdentifier(data);
		return data;
	}

	void Delete(){
		SetMorphTarget(true);
		target_select.Delete();
	}

	string GetDisplayString(){
		return "SetMorphTarget " + target_select.GetTargetDisplayText() + " " + morph_1 + (two_way_morph?"+" + morph_2:"") + " " + weight;
	}

	void StartSettings(){
		target_select.CheckAvailableTargets();
		if(available_morphs.size() == 0){
			GetAvailableMorphs();
		}
		SetMorphTarget(false);
	}

	void StartEdit(){
		SetMorphTarget(false);
	}

	void EditDone(){
		SetMorphTarget(true);
	}

	void PreTargetChanged(){
		SetMorphTarget(true);
	}

	void TargetChanged(){
		GetAvailableMorphs();
		SetMorphTarget(false);
	}

	void GetMorphIndex(){
		morph_1_index = 0;
		morph_2_index = 0;
		for(uint i = 0; i < available_morphs.size(); i++){
			if(available_morphs[i] == morph_1){
				morph_1_index = i;
			}
			if(available_morphs[i] == morph_2){
				morph_2_index = i;
			}
		}
		morph_1 = available_morphs[morph_1_index];
		morph_2 = available_morphs[morph_2_index];
	}

	void DrawSettings(){

		float option_name_width = 140.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Target Character");
		ImGui_NextColumn();
		ImGui_NextColumn();
		target_select.DrawSelectTargetUI();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Two Way Morph");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		if(ImGui_Checkbox("###Two Way Morph", two_way_morph)){
			//A single morph target cannot go under 0.
			if(!two_way_morph && weight < 0.0){
				weight = 0.0;
			}
			SetMorphTarget(true);
			SetMorphTarget(false);
		}
		ImGui_NextColumn();

		float margin = 3.0;

		if(two_way_morph){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Morph Setting");
			ImGui_NextColumn();

			ImGui_PushItemWidth(second_column_width * 0.24 - margin);
			if(ImGui_Combo("###Morph 1", morph_1_index, available_morphs, available_morphs.size())){
				morph_1 = available_morphs[morph_1_index];
				SetMorphTarget(true);
				SetMorphTarget(false);
			}
			ImGui_PopItemWidth();

			ImGui_SameLine();

			ImGui_PushItemWidth(second_column_width * 0.5 - margin);
			if(ImGui_SliderFloat("###Weight", weight, -1.0f, 1.0f, "%.2f")){
				SetMorphTarget(true);
				SetMorphTarget(false);
			}
			ImGui_PopItemWidth();

			ImGui_SameLine();

			ImGui_PushItemWidth(second_column_width * 0.25 - margin);
			if(ImGui_Combo("###Morph 2", morph_2_index, available_morphs, available_morphs.size())){
				morph_2 = available_morphs[morph_2_index];
				SetMorphTarget(true);
				SetMorphTarget(false);
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}else{
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Morph Setting");
			ImGui_NextColumn();

			ImGui_PushItemWidth(second_column_width * 0.25);
			if(ImGui_Combo("###Morph 1", morph_1_index, available_morphs, available_morphs.size())){
				morph_1 = available_morphs[morph_1_index];
				SetMorphTarget(true);
				SetMorphTarget(false);
			}
			ImGui_PopItemWidth();

			ImGui_SameLine();

			ImGui_PushItemWidth(-1);
			if(ImGui_SliderFloat("###Weight", weight, 0.0f, 1.0f, "%.2f")){
				SetMorphTarget(true);
				SetMorphTarget(false);
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Smoothing Duration");
		ImGui_NextColumn();
		ImGui_PushItemWidth(second_column_width);
		ImGui_SliderFloat("##Smoothing Duration", smoothing_duration, 0.0f, 10.0f, "%.2f");
		ImGui_PopItemWidth();
		ImGui_NextColumn();
	}

	bool Trigger(){
		if(UpdateSmoothing()){
			SetMorphTarget(false);
			triggered = false;
			timer = 0.0;
			return true;
		}else{
			return false;
		}
	}

	bool UpdateSmoothing(){
		if(timer >= smoothing_duration){
			return true;
		}else{
			SetMorphTarget(false);
			timer += time_step;
		}
		return false;
	}

	void DrawEditing(){
		array<MovementObject@> targets = target_select.GetTargetMovementObjects();
		for(uint i = 0; i < targets.size(); i++){
			DebugDrawLine(targets[i].position, this_hotspot.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
		}
		UpdateSmoothing();
	}

	bool SetMorphTarget(bool reset){
		array<MovementObject@> targets = target_select.GetTargetMovementObjects();
		if(targets.size() == 0){return false;}

		if(reset){
			triggered = false;
			timer = 0.0;
		}

		float weight_weight = smoothing_duration == 0.0f?1.0:min(1.0, max(0.0, timer / smoothing_duration));
		for(uint i = 0; i < targets.size(); i++){
			if(reset){
				targets[i].rigged_object().SetMorphTargetWeight(morph_1, 0.0f, 1.0);
				targets[i].rigged_object().SetMorphTargetWeight(morph_2, 0.0f, 1.0);
			}else{
				if(two_way_morph){
					if(weight < 0.0){
						targets[i].rigged_object().SetMorphTargetWeight(morph_1, abs(weight), weight_weight);
						targets[i].rigged_object().SetMorphTargetWeight(morph_2, 0.0f, weight_weight);
					}else{
						targets[i].rigged_object().SetMorphTargetWeight(morph_1, 0.0f, weight_weight);
						targets[i].rigged_object().SetMorphTargetWeight(morph_2, weight, weight_weight);
					}
				}else{
					targets[i].rigged_object().SetMorphTargetWeight(morph_1, weight, weight_weight);
				}
			}
		}
		return true;
	}

	void ReceiveMessage(string message, string identifier){
		array<string> file_lines = message.split("\n");
		bool inside_morph = false;

		for(uint i = 0; i < file_lines.size(); i++){
			string line = file_lines[i];
			//Remove any tabs.
			line = join(line.split("\t"), "");
			if(line.findFirst("<morphs>") != -1){
				//Find the content within the <morphs> tags.
				inside_morph = true;
			}else if(line.findFirst("</morphs>") != -1){
				//Found the end of the morphs tag.
				break;
			}else if(inside_morph){
				//This line has a morph in it because it starts with a <.
				int tag_start = line.findFirst("<") + 1;
				if(tag_start != 0){
					int tag_end = line.findFirst(" ", tag_start);
					if(tag_end != -1){
						string new_morph_name = line.substr(tag_start, tag_end - tag_start);
						//Check if the morph is already added.
						if(available_morphs.find(new_morph_name) == -1){
							available_morphs.insertLast(new_morph_name);
						}
					}
				}
			}
		}
		GetMorphIndex();
	}

	void GetAvailableMorphs(){
		available_morphs.resize(0);

		array<MovementObject@> targets = target_select.GetTargetMovementObjects();
		for(uint i = 0; i < targets.size(); i++){
			level.SendMessage("drika_read_file " + hotspot.GetID() + " " + index + " " + targets[i].char_path);
		}
	}

	void Reset(){
		if(triggered){
			triggered = false;
			SetMorphTarget(true);
		}
	}
}
