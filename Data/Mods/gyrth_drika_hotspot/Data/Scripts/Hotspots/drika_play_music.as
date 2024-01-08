class DrikaPlayMusic : DrikaElement{
	string music_path;
	string song_path;
	string song_name;
	string before_song;
	bool from_beginning_no_fade;

	int current_music_mode;
	music_modes music_mode;

	array<string> music_mode_names = { "Song",
										"Song In Combat",
										"Song When Player Died",
										"Song When Enemies Defeated",
										"Song Ambient",
										"Silence" };

	DrikaPlayMusic(JSONValue params = JSONValue()){
		music_path = GetJSONString(params, "music_path", "Data/Music/drika_music.xml");
		song_path = GetJSONString(params, "song_path", "Data/Music/lugaru_menu_new.ogg");
		song_name = GetJSONString(params, "song_name", "lugaru_menu_new.ogg");
		from_beginning_no_fade = GetJSONBool(params, "from_beginning_no_fade", false);
		music_mode = music_modes(GetJSONInt(params, "music_mode", music_song));

		// Convert old saved data to keep backwards compatibility.
		if(GetJSONBool(params, "on_event", false) == true){
			int music_event = GetJSONInt(params, "music_event", music_song_in_combat);
			if(music_event == 0){
				music_mode = music_song_in_combat;
			}else if(music_event == 1){
				music_mode = music_song_player_died;
			}else if(music_event == 2){
				music_mode = music_song_enemies_defeated;
			}else if(music_event == 3){
				music_mode = music_song_ambient;
			}
		}

		current_music_mode = music_mode;

		drika_element_type = drika_play_music;
		has_settings = true;
	}

	void PostInit(){
		//To make sure the music xml is correct each time, write it again at startup.
		if(music_path != "Data/Music/drika_music.xml" && music_mode != music_silence){
			WriteMusicXML();
		}
	}

	JSONValue GetCheckpointData(){
		JSONValue data;
		data["triggered"] = triggered;
		if(triggered){
			data["current_song"] = JSONValue(GetSong());
		}
		return data;
	}

	void SetCheckpointData(JSONValue data = JSONValue()){
		triggered = data["triggered"].asBool();
		if(triggered){
			SetSong(data["current_song"].asString());
		}
	}

	JSONValue GetSaveData(){
		JSONValue data;
		data["music_path"] = JSONValue(music_path);
		data["song_path"] = JSONValue(song_path);
		data["song_name"] = JSONValue(song_name);
		data["from_beginning_no_fade"] = JSONValue(from_beginning_no_fade);
		data["music_mode"] = JSONValue(music_mode);
		return data;
	}

	string GetDisplayString(){
		string display_string = "PlayMusic ";
		display_string += music_mode_names[music_mode] + " ";
		if(music_mode != music_silence){
			display_string += song_name;
		}
		return display_string;
	}

	void DrawSettings(){

		float option_name_width = 175.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Play Music Mode");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		ImGui_PushItemWidth(second_column_width);
		if(ImGui_Combo("##Play Music Mode", current_music_mode, music_mode_names, music_mode_names.size())){
			music_mode = music_modes(current_music_mode);
			Play(false);
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		if(music_mode != music_silence){
			ImGui_AlignTextToFramePadding();
			ImGui_Text("Song Path");
			ImGui_NextColumn();

			if(ImGui_Button("Set Song Path")){
				string new_path = GetUserPickedReadPath("ogg", "Data/Music");
				if(new_path != ""){
					song_path = ShortenPath(new_path);
					SongPathCheck();
					GetSongName();
					music_path = "Data/Music/" + GetUniqueFileName() + ".xml";
					WriteMusicXML();
					Play(false);
				}
			}
			ImGui_SameLine();
			ImGui_Text(song_path);
			ImGui_NextColumn();
		}

		ImGui_AlignTextToFramePadding();
		ImGui_Text("From Beginning No Fade");
		ImGui_NextColumn();
		ImGui_Checkbox("###From Beginning No Fade", from_beginning_no_fade);
		ImGui_NextColumn();
	}

	void GetSongName(){
		array<string> split_path = song_path.split("/");
		song_name = split_path[split_path.size() - 1];
	}

	bool Trigger(){
		if(!triggered){
			GetPreviousSong();
		}
		triggered = true;
		return Play(false);
	}

	string GetUniqueFileName(){
		string filename = "";
		while(filename.length() < 10){
			string s('0');
			s[0] = rand() % (123 - 97) + 97;
			filename += s;
		}
		if(FileExists("Data/Music/" + filename + ".xml")){
			//Already exists so get a new one.
			return GetUniqueFileName();
		}else{
			return filename;
		}
	}

	void WriteMusicXML(){
		string msg = "write_music_xml ";
		msg += music_path + " ";
		msg += song_name + " ";
		msg += song_path;

		level.SendMessage(msg);
	}

	void GetPreviousSong(){
		before_song = GetSong();
	}

	void StartEdit(){
		Play(false);
	}

	void SongPathCheck(){
		if(!FileExists(song_path)){
			array<string> split_path = song_path.split("/");
			split_path.removeRange(0, 2);
			string new_path = join(split_path, "/");
			if(FileExists(song_path)){
				Log(warning, "Fixed path " + new_path);
				song_path = new_path;
			}
		}
	}

	bool Play(bool reset){
		if(music_mode != music_silence){
			SongPathCheck();
			if(reset){
				RemoveMusic(music_path);
			}else{
				if(!FileExists(music_path)){
					WriteMusicXML();
				}
				AddMusic(music_path);
			}
		}

		if(music_mode == music_song){
			if(from_beginning_no_fade){
				SetSong((reset?before_song:song_name));
			}else{
				PlaySong((reset?before_song:song_name));
			}
		}else if(music_mode == music_silence){
			if(from_beginning_no_fade){
				SetSong((reset?before_song:"silence"));
			}else{
				PlaySong((reset?before_song:"silence"));
			}
		}else{
			level.SendMessage("drika_music_event " + music_mode + " " + (reset?before_song:song_name) + " " + from_beginning_no_fade);
		}

		return true;
	}

	void Reset(){
		if(triggered){
			triggered = false;
			Play(true);
		}
	}
}
