class DrikaGoToLineSelect{
	string name;
	int import_index;
	int index;
	DrikaElement@ target_element;

	DrikaGoToLineSelect(string _name, JSONValue params = JSONValue()){
		name = _name;
		index = GetJSONInt(params, name, 0);
		import_index = GetJSONInt(params, name + "_import_index", -1);
		if(duplicating_function){
			GetTargetElement();
		}
	}

	void PostInit(){
		if(importing){
			if(import_index != -1){
				//Get the element from the imported functions.
				@target_element = GetImportElement(import_index);
			}else{
				//However if the target is not included in the import data, try to get the element on the same index.
				GetTargetElement();
			}
		}else if(duplicating_hotspot || !duplicating_function){
			GetTargetElement();
		}
	}

	void GetTargetElement(){
		if(int(drika_indexes.size()) > index && index != -1){
			if(int(drika_elements.size()) > drika_indexes[index]){
				@target_element = drika_elements[drika_indexes[index]];
				return;
			}
		}
		@target_element = null;
	}

	void SaveGoToLine(JSONValue &inout data){
		if(@target_element != null){
			data[name] = JSONValue(target_element.index);
			if(exporting){
				data[name + "_import_index"] = JSONValue(target_element.export_index);
			}
		}
	}

	void CheckLineAvailable(){
		//Elements can be deleted when this function isn't being edited. So this function is used to continuesly check the target element.
		if(@target_element == null || target_element.deleted){
			//If the line_element gets deleted then just pick the first one.
			if(drika_elements.size() > 0){
				//Check if the first element wasn't the one that got deleted.
				if(!drika_elements[0].deleted){
					@target_element = drika_elements[0];
					return;
				}
			}
			@target_element = null;
		}
	}

	int GetTargetLineIndex(){
		if(@target_element != null){
			return target_element.index;
		}
		return 0;
	}

	void DrawGoToLineUI(){
		if(@target_element == null){
			return;
		}

		string preview_value = target_element.line_number + target_element.GetDisplayString();
		ImGui_AlignTextToFramePadding();
		ImGui_Text("Go to line");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		ImGui_PushItemWidth(second_column_width);
		ImGui_PushStyleColor(ImGuiCol_Text, target_element.GetDisplayColor());

		if(ImGui_BeginCombo("###line" + name, preview_value, ImGuiComboFlags_HeightLarge)){
			for(uint i = 0; i < drika_indexes.size(); i++){
				int item_no = drika_indexes[i];
				bool is_selected = (target_element.index == drika_elements[drika_indexes[i]].index);
				vec4 text_color = drika_elements[item_no].GetDisplayColor();

				string display_string = drika_elements[item_no].line_number + drika_elements[item_no].GetDisplayString();
				display_string = join(display_string.split("\n"), "");
				float space_for_characters = ImGui_CalcTextSize(display_string).x;

				if(space_for_characters > ImGui_GetWindowContentRegionWidth()){
					display_string = display_string.substr(0, int(display_string.length() * (ImGui_GetWindowContentRegionWidth() / space_for_characters)) - 3) + "...";
				}

				ImGui_PushStyleColor(ImGuiCol_Text, text_color);
				if(ImGui_Selectable(display_string, is_selected)){
					@target_element = drika_elements[item_no];
				}
				ImGui_PopStyleColor();
			}
			ImGui_EndCombo();
		}
		ImGui_PopStyleColor();
		ImGui_PopItemWidth();
		ImGui_NextColumn();
	}

	void DrawInputGoToLineUI(){
		if(@target_element == null){
			return;
		}
		//Log(fatal, "function called" + name);
		string preview_value = target_element.line_number + target_element.GetDisplayString();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		ImGui_PushItemWidth(second_column_width);
		ImGui_PushStyleColor(ImGuiCol_Text, target_element.GetDisplayColor());
		if(ImGui_BeginCombo("###line" + name, preview_value, ImGuiComboFlags_HeightLarge)){
			for(uint i = 0; i < drika_indexes.size(); i++){
				int item_no = drika_indexes[i];
				bool is_selected = (target_element.index == drika_elements[drika_indexes[i]].index);
				vec4 text_color = drika_elements[item_no].GetDisplayColor();

				string display_string = drika_elements[item_no].line_number + drika_elements[item_no].GetDisplayString();
				display_string = join(display_string.split("\n"), "");
				float space_for_characters = ImGui_CalcTextSize(display_string).x;

				if(space_for_characters > ImGui_GetWindowContentRegionWidth()){
					display_string = display_string.substr(0, int(display_string.length() * (ImGui_GetWindowContentRegionWidth() / space_for_characters)) - 3) + "...";
				}

				ImGui_PushStyleColor(ImGuiCol_Text, text_color);
				if(ImGui_Selectable(display_string, is_selected)){
					@target_element = drika_elements[item_no];
				}
				ImGui_PopStyleColor();
			}
			ImGui_EndCombo();
		}
		ImGui_PopStyleColor();
		ImGui_PopItemWidth();
		ImGui_NextColumn();
	}
}
