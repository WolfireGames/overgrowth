enum play_sound_methods	{
							play_sound = 0,
							play_sound_at_location = 1,
							play_sound_loop = 2,
							play_sound_loop_at_location = 3,
							play_sound_group = 4,
							play_sound_group_gain = 5,
							play_sound_group_position = 6,
							play_sound_group_position_gain = 7,
							play_sound_group_position_priority = 8,
							stop_sounds = 9
						};

enum SoundType	{
					_sound_type_foley,
				    _sound_type_loud_foley,
				    _sound_type_voice,
				    _sound_type_combat
				};

class DrikaPlaySound : DrikaElement{
	string sound_path;
	play_sound_methods play_sound_method;
	int current_play_sound_method;
	float gain;
	int priority;
	array<int> sound_handles;
	bool is_spatial;
	bool is_group;
	bool has_gain;
	bool ai_sound;
	float max_range;
	SoundType sound_type;
	int current_sound_type;

	array<string> play_sound_method_names = {
												"Play Sound",
												"Play Sound At Location",
												"Play Sound Loop",
												"Play Sound Loop At Location",
												"Play Sound Group",
												"Play Sound Group With Gain",
												"Play Sound Group At Position",
												"Play Sound Group At Position With Gain",
												"Play Sound Group At Position With Priority",
												"Stop Sounds"
											};

	array<string> sound_priority_names = {
											"Max",
											"Very High",
											"High",
											"Medium",
											"Low"
										};

	array<string> sound_type_names =	{
											"Foley",
											"Loud Foley",
											"Voice",
											"Combat"
										};

	DrikaPlaySound(JSONValue params = JSONValue()){
		placeholder.Load(params);
		placeholder.name = "Play Sound Helper";

		priority = GetJSONInt(params, "priority", _sound_priority_max);
		gain = GetJSONFloat(params, "gain", 1.0);
		play_sound_method = play_sound_methods(GetJSONInt(params, "play_sound_method", play_sound));
		current_play_sound_method = play_sound_method;
		IdentifyPlayMethod();
		ai_sound = GetJSONBool(params, "ai_sound", false);
		max_range = GetJSONFloat(params, "max_range", 5.0);
		sound_type = SoundType(GetJSONInt(params, "sound_type", _sound_type_foley));
		current_sound_type = sound_type;
		sound_path = GetJSONString(params, "sound_path", "Data/Sounds/weapon_foley/impact/weapon_knife_hit_neck_2.wav");

		drika_element_type = drika_play_sound;
		has_settings = true;
	}

	JSONValue GetSaveData(){
		JSONValue data;
		placeholder.Save(data);
		data["sound_path"] = JSONValue(sound_path);
		data["play_sound_method"] = JSONValue(play_sound_method);
		data["priority"] = JSONValue(priority);
		data["gain"] = JSONValue(gain);
		data["ai_sound"] = JSONValue(ai_sound);
		data["max_range"] = JSONValue(max_range);
		data["sound_type"] = JSONValue(sound_type);
		return data;
	}

	void IdentifyPlayMethod(){
		if(play_sound_method == play_sound_group || play_sound_method == play_sound_group_gain || play_sound_method == play_sound_group_position || play_sound_method == play_sound_group_position_gain || play_sound_method == play_sound_group_position_priority){
			is_group = true;
		}else{
			is_group = false;
		}

		if(play_sound_method == play_sound_group_gain || play_sound_method == play_sound_group_position_gain || play_sound_method == play_sound_loop || play_sound_method == play_sound_loop_at_location){
			has_gain = true;
		}else{
			has_gain = false;
		}

		if(play_sound_method == play_sound_at_location || play_sound_method == play_sound_loop_at_location || play_sound_method == play_sound_group_position || play_sound_method == play_sound_group_position_gain || play_sound_method == play_sound_group_position_priority){
			is_spatial = true;
		}else{
			is_spatial = false;
		}
	}

	void PostInit(){
		placeholder.Retrieve();
	}

	void Delete(){
		Reset();
		placeholder.Remove();
	}

	string GetDisplayString(){
		return play_sound_method_names[current_play_sound_method] + " " + (current_play_sound_method != stop_sounds? sound_path: "");
	}

	void DrawSettings(){

		float option_name_width = 130.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		if (current_play_sound_method != stop_sounds)
			{
				ImGui_AlignTextToFramePadding();
				ImGui_Text("Sound Path");
				ImGui_NextColumn();
				if(ImGui_Button("Set Sound Path")){
					string new_path = "";
					if(is_group){
						new_path = GetUserPickedReadPath("xml", "Data/Sounds");
					}else{
						new_path = GetUserPickedReadPath("wav", "Data/Sounds");
					}
					// The path will be returned empty if the user cancels the file pick.
					if(new_path != ""){
						sound_path = ShortenPath(new_path);
						PreviewSound();
					}
				}
				ImGui_SameLine();
				ImGui_Text(sound_path);
				ImGui_NextColumn();
			}

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Play Sound Method");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		ImGui_PushItemWidth(second_column_width);
		if(ImGui_Combo("##Play Sound Method", current_play_sound_method, play_sound_method_names, play_sound_method_names.size())){
			play_sound_method = play_sound_methods(current_play_sound_method);
			IdentifyPlayMethod();
			//Trigger a draw editing once to delte or create placeholder object so that it can be previewed.
			DrawEditing();
			PreviewSound();
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		if(play_sound_method == play_sound_group_position_priority){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Sound Priority");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_Combo("##Sound Priority", priority, sound_priority_names, sound_priority_names.size())){
				PreviewSound();
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}

		if(has_gain){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Gain");
			ImGui_NextColumn();
			ImGui_PushItemWidth(second_column_width);
			if(ImGui_SliderFloat("###Gain", gain, 0.0f, 10.0f, "%.2f")){
				PreviewSound();
			}
			ImGui_PopItemWidth();
			ImGui_NextColumn();
		}

		if(is_spatial){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("AISound");
			ImGui_NextColumn();
			if(ImGui_Checkbox("###AISound", ai_sound)){
				PreviewSound();
			}
			ImGui_NextColumn();

			if(ai_sound){
				ImGui_AlignTextToFramePadding();
				ImGui_Text("AISound Max Range");
				ImGui_NextColumn();
				ImGui_PushItemWidth(second_column_width);
				if(ImGui_SliderFloat("###AISound Max Range", max_range, 0.0f, 10.0f, "%.2f")){
					PreviewSound();
				}
				ImGui_PopItemWidth();
				ImGui_NextColumn();

				ImGui_AlignTextToFramePadding();
				ImGui_Text("Sound Type");
				ImGui_NextColumn();
				ImGui_PushItemWidth(second_column_width);
				if(ImGui_Combo("###Sound Type", current_sound_type, sound_type_names, sound_type_names.size())){
					sound_type = SoundType(current_sound_type);
					PreviewSound();
				}
				ImGui_PopItemWidth();
				ImGui_NextColumn();
			}
		}
	}

	void DrawEditing(){
		if(is_spatial){
			if(placeholder.Exists()){
				DebugDrawLine(placeholder.GetTranslation(), this_hotspot.GetTranslation(), vec3(0.0, 1.0, 0.0), _delete_on_draw);
				DebugDrawBillboard("Data/Textures/ui/speaker.png", placeholder.GetTranslation(), 0.25, vec4(1.0), _delete_on_draw);
			}else{
				placeholder.Create();
			}
		}else{
			if(placeholder.Exists()){
				placeholder.Remove();
			}
		}
	}

	void StartEdit(){
		DrikaElement::StartEdit();
		PreviewSound();
	}

	void PreviewSound(){
		Reset();
		Trigger();
	}

	void EditDone(){
		DrikaElement::EditDone();
		Reset();
	}

	bool Trigger(){
		string extension = sound_path.substr(sound_path.length() - 3, sound_path.length());

		if(is_spatial){
			if(!placeholder.Exists()){
				placeholder.Create();
				return false;
			}
			if(ai_sound){
				AISound(placeholder.GetTranslation(), max_range, sound_type);
			}
		}

		if(is_group){
			if(extension != "xml"){
				Log(warning, "The selected sound file is not an xml.");
				return false;
			}
		}else{
			if(extension != "wav"){
				Log(warning, "The selected sound file is not a wav.");
				return false;
			}
		}

		switch(play_sound_method){
			case play_sound:
				sound_handles.insertLast(PlaySound(sound_path));
				break;
			case play_sound_at_location:
				sound_handles.insertLast(PlaySound(sound_path, placeholder.GetTranslation()));
				break;
			case play_sound_loop:
				sound_handles.insertLast(PlaySoundLoop(sound_path, gain));
				break;
			case play_sound_loop_at_location:
				sound_handles.insertLast(PlaySoundLoopAtLocation(sound_path, placeholder.GetTranslation(), gain));
				break;
			case play_sound_group:
				sound_handles.insertLast(PlaySoundGroup(sound_path));
				break;
			case play_sound_group_gain:
				sound_handles.insertLast(PlaySoundGroup(sound_path, gain));
				break;
			case play_sound_group_position:
				sound_handles.insertLast(PlaySoundGroup(sound_path, placeholder.GetTranslation()));
				break;
			case play_sound_group_position_gain:
				sound_handles.insertLast(PlaySoundGroup(sound_path, placeholder.GetTranslation(), gain));
				break;
			case play_sound_group_position_priority:
				sound_handles.insertLast(PlaySoundGroup(sound_path, placeholder.GetTranslation(), priority));
				break;
			case stop_sounds:

				for(uint i = 0; i < drika_elements.size(); i++)
				{
					if 	(drika_elements[i].drika_element_type == drika_play_sound)
						{drika_elements[i].Reset();}
				}
				break;
			default:
				break;
		}

		return true;
	}

	void AISound(vec3 pos, float max_range, SoundType type) {
		int player_id = -1;
		for(int i = 0; i < GetNumCharacters(); i++){
			if(ReadCharacter(i).is_player){
				player_id = ReadCharacter(i).GetID();
				break;
			}
		}

		if(player_id != -1){
			string msg = "nearby_sound " + pos.x + " " + pos.y + " " + pos.z + " " + max_range + " " + player_id + " " + type;
			for(int i = 0; i < GetNumCharacters(); i++){
				ReadCharacter(i).ReceiveScriptMessage(msg);
			}
		}
	}

	void Reset(){
		for(uint i = 0; i < sound_handles.size(); i++){
			StopSound(sound_handles[i]);
		}
		sound_handles.resize(0);
	}
}
