class DrikaQuickLaunchElement{
	string query;
	string json;
	drika_element_types type = none;

	DrikaQuickLaunchElement(string _query, string _json){
		query = _query;
		json = _json;

		JSON data;
		if(!data.parseString(json)){
			Log(warning, "Unable to parse the JSON in the quick launch element! " + json);
		}else{
			JSONValue root = data.getRoot();
			type = drika_element_types(root["function"].asInt());
		}
	}
}

class DrikaQuickLaunch{

	bool open_quick_launch = false;
	string quick_launch_search_buffer = "";
	string database_path = "Data/Scripts/drika_quick_launch_database.json";
	array<DrikaQuickLaunchElement@> database;
	array<DrikaQuickLaunchElement@> results;
	int selected_item = 0;
	bool update_quick_launch_scroll = false;
	bool quick_launch_open = false;

	DrikaQuickLaunch(){
	}

	void Init(){
		JSON data;

		if(!data.parseFile(database_path)){
			Log(warning, "Unable to parse the JSON in the quick launch database!");
			return;
		}

		JSONValue root = data.getRoot();
		array<string> list_groups = root.getMemberNames();
		list_groups.sortAsc();

		for(uint i = 0; i < list_groups.size(); i++){
			database.insertLast(DrikaQuickLaunchElement(list_groups[i], root[list_groups[i]].asString()));
		}
	}

	void Draw(){
		ImGui_PushStyleVar(ImGuiStyleVar_WindowMinSize, vec2(300, 150));
		ImGui_SetNextWindowSize(vec2(1000.0f, 450.0f), ImGuiSetCond_Always);

		if(open_quick_launch){
			ImGui_OpenPopup("Quick Launch");
			quick_launch_search_buffer = "";
			selected_item = 0;
			update_quick_launch_scroll = true;
			QueryElement(quick_launch_search_buffer);
			open_quick_launch = false;
		}

		if(ImGui_BeginPopupModal("Quick Launch", ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar)){
			ImGui_SetWindowFontScale(1.5);
			ImGui_AlignTextToFramePadding();
			ImGui_SetTextBuf(quick_launch_search_buffer);
			ImGui_Text("Search");
			ImGui_SameLine();
			ImGui_PushItemWidth(ImGui_GetContentRegionAvailWidth());

			if(ImGui_InputText("", ImGuiInputTextFlags_AutoSelectAll)){
				quick_launch_search_buffer = ImGui_GetTextBuf();
				QueryElement(quick_launch_search_buffer);
				selected_item = 0;
			}

			if(!quick_launch_open){
				Log(warning, "Set keyboard focus");
				ImGui_SetKeyboardFocusHere(-1);
				quick_launch_open = true;
			}

			//The lctrl check only works once when the popup opens. When after that, it's always returning false.
			if(!GetInputDown(0, "lctrl") && ImGui_IsKeyPressed(ImGui_GetKeyIndex(ImGuiKey_Enter)) && results.size() != 0){
				AddDHSFunction(results[selected_item].json);
			}

			ImGui_PopItemWidth();

			ImGui_BeginChild("Quick Launch Elements", vec2(-1, -1), false, ImGuiWindowFlags_NoTitleBar);
			for(uint i = 0; i < results.size(); i++){
				vec4 text_color = display_colors[results[i].type];
				ImGui_PushStyleColor(ImGuiCol_Text, text_color);
				bool line_selected = selected_item == int(i);

				string display_string = results[i].query;
				display_string = join(display_string.split("\n"), "");
				float space_for_characters = ImGui_CalcTextSize(display_string).x;

				if(space_for_characters > ImGui_GetWindowContentRegionWidth()){
					display_string = display_string.substr(0, int(display_string.length() * (ImGui_GetWindowContentRegionWidth() / space_for_characters)) - 3) + "...";
				}

				if(ImGui_Selectable(display_string, line_selected, ImGuiSelectableFlags_DontClosePopups)){
					AddDHSFunction(results[i].json);
				}

				if(update_quick_launch_scroll && line_selected){
					update_quick_launch_scroll = false;
					ImGui_SetScrollHere(0.5);
				}

				ImGui_PopStyleColor();
			}

			ImGui_EndChild();

			if(ImGui_IsKeyPressed(ImGui_GetKeyIndex(ImGuiKey_UpArrow), true)){
				if(selected_item > 0){
					selected_item -= 1;
					update_quick_launch_scroll = true;
				}
			}else if(ImGui_IsKeyPressed(ImGui_GetKeyIndex(ImGuiKey_DownArrow), true)){
				if(selected_item < int(results.size() - 1)){
					selected_item += 1;
					update_quick_launch_scroll = true;
				}
			}

			vec4 window_info = GetWindowInfo();

			ImGui_EndPopup();

			if(CheckClosePopup(window_info)){
				quick_launch_open = false;
			}
		}
		ImGui_PopStyleVar();
	}

	void QueryElement(string query){
		results.resize(0);
		//If the query is empty then just return the whole database.
		if(query == ""){
			results = database;
		}else{
			//The query can be multiple words separated by spaces.
			array<string> split_query = query.split(" ");

			for(uint i = 0; i < database.size(); i++){
				bool found_result = true;
				for(uint j = 0; j < split_query.size(); j++){
					//Could not find part of query in the database.
					if(ToLowerCase(database[i].query).findFirst(ToLowerCase(split_query[j])) == -1){
						found_result = false;
						break;
					}
				}
				//Only if all parts of the query are found then add the result.
				if(found_result){
					results.insertLast(database[i]);
				}
			}
		}
	}

	void AddDHSFunction(string json_data){
		JSON data;
		if(!data.parseString(json_data)){
			Log(warning, "Unable to parse the JSON in the Quick Launch Database! " + results[selected_item].json);
		}else{
			adding_function = true;
			DrikaElement@ new_element = InterpElement(none, data.getRoot());
			post_init_queue.insertLast(@new_element);
			InsertElement(new_element);
			element_added = true;
			update_scroll = true;
			multi_select = {current_line};
			ImGui_CloseCurrentPopup();
			quick_launch_open = false;
		}
	}

	void Update(){
		if(GetInputDown(0, "lctrl") && GetInputPressed(0, "return")){
			open_quick_launch = true;
		}
	}
}
