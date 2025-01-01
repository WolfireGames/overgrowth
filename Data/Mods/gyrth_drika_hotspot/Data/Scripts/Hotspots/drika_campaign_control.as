enum campaign_control_options {	finish_current_and_load_next = 0,
								finish_current = 1,
								load_next = 2
								};

class DrikaCampaignControl : DrikaElement{

	array<string> campaign_control_names = {	"Finish current level and load next level",
												"Finish current level",
												"Load next level"
											};

	int current_campaign_control_option;
	campaign_control_options campaign_control_option;

	string campaign_title;
	string campaign_id = "";
	string level_title;
	string level_id = "";

	string campaign_thumbnail_path;
	TextureAssetRef campaign_thumbnail;
	string campaign_main_script;
	string campaign_menu_script;

	string level_thumbnail_path;
	TextureAssetRef level_thumbnail;
	string level_path;
	bool level_completion_optional;

	DrikaCampaignControl(JSONValue params = JSONValue()){
		campaign_control_option = campaign_control_options(GetJSONInt(params, "campaign_control_option", 0));
		current_campaign_control_option = campaign_control_option;
		drika_element_type = drika_campaign_control;
		has_settings = true;
	}

	JSONValue GetSaveData(){
		JSONValue data;

		data["campaign_control_option"] = JSONValue(campaign_control_option);

		return data;
	}

	void PostInit(){
		GetCampaignAndLevelInfo();
	}

	string GetDisplayString(){
		return "CampaignControl " + campaign_control_names[campaign_control_option];
	}

	void StartSettings(){
		GetCampaignAndLevelInfo();
	}

	void GetCampaignAndLevelInfo(){
		array<Campaign> campaigns = GetCampaigns();

		string current_level = join(GetCurrLevelRelPath().split("Data/Levels/"), "");

		for(uint i = 0; i < campaigns.size(); i++){
			array<ModLevel> campaign_levels = campaigns[i].GetLevels();

			for(uint j = 0; j < campaign_levels.size(); j++){
				if(campaign_levels[j].GetPath() == current_level){
					campaign_id = campaigns[i].GetID();
					level_id = campaign_levels[j].GetID();
				}
			}
		}
		
		Campaign campaign = GetCampaign(campaign_id);
		campaign_thumbnail_path = campaign.GetThumbnail();

		if(FileExists(campaign_thumbnail_path)){
			campaign_thumbnail = LoadTexture(campaign_thumbnail_path, TextureLoadFlags_NoMipmap | TextureLoadFlags_NoConvert |TextureLoadFlags_NoReduce);
		}else if(FileExists("Data/" + campaign_thumbnail_path)){
			campaign_thumbnail = LoadTexture("Data/" + campaign_thumbnail_path, TextureLoadFlags_NoMipmap | TextureLoadFlags_NoConvert |TextureLoadFlags_NoReduce);
		}else{
			campaign_thumbnail = LoadTexture("Data/Textures/ui/main_menu/overgrowth.png", TextureLoadFlags_NoMipmap | TextureLoadFlags_NoConvert |TextureLoadFlags_NoReduce);
		}
		
		campaign_main_script = campaign.GetMainScript();
		campaign_menu_script = campaign.GetMenuScript();

		ModLevel level = campaign.GetLevel(level_id);
		level_thumbnail_path = level.GetThumbnail();

		if(FileExists(level_thumbnail_path)){
			level_thumbnail = LoadTexture(level_thumbnail_path, TextureLoadFlags_NoMipmap | TextureLoadFlags_NoConvert |TextureLoadFlags_NoReduce);
		}else if(FileExists("Data/" + level_thumbnail_path)){
			level_thumbnail = LoadTexture("Data/" + level_thumbnail_path, TextureLoadFlags_NoMipmap | TextureLoadFlags_NoConvert |TextureLoadFlags_NoReduce);
		}else{
			level_thumbnail = LoadTexture("Data/Textures/ui/main_menu/overgrowth.png", TextureLoadFlags_NoMipmap | TextureLoadFlags_NoConvert |TextureLoadFlags_NoReduce);
		}

		level_path = level.GetPath();
		level_completion_optional = level.CompletionOptional();

		SetLevelPlayed(level_id);
        SetLastLevelPlayed(level_id);
	}

	void DrawSettings(){
		float option_name_width = 120.0;

		ImGui_Columns(2, false);
		ImGui_SetColumnWidth(0, option_name_width);

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Option");
		ImGui_NextColumn();
		float second_column_width = ImGui_GetContentRegionAvailWidth();
		ImGui_PushItemWidth(second_column_width);
		if(ImGui_Combo("##Option", current_campaign_control_option, campaign_control_names, 15)){
			campaign_control_option = campaign_control_options(current_campaign_control_option);
		}
		ImGui_PopItemWidth();
		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Campaign info");
		ImGui_NextColumn();

		if(campaign_id == ""){
			ImGui_PushStyleColor(ImGuiCol_Text, vec4(1.0, 0.0, 0.0, 1.0));
			ImGui_Text("Could not find current campaign.");
			ImGui_PopStyleColor();
		}else{
			ImGui_BeginChild("Campaign info", vec2(0.0, 110.0), false);

			ImGui_Columns(2, false);
			ImGui_SetColumnWidth(0.0, second_column_width / 2.0);

			ImGui_Text("ID : " + campaign_id);
			ImGui_Text("Main script : " + campaign_main_script);
			ImGui_Text("Menu script : " + campaign_menu_script);
			ImGui_NextColumn();
			ImGui_Image(campaign_thumbnail, vec2(180.0, 101.0));

			ImGui_EndChild();
		}

		ImGui_NextColumn();

		ImGui_AlignTextToFramePadding();
		ImGui_Text("Level info");
		ImGui_NextColumn();

		if(level_id == ""){
			ImGui_PushStyleColor(ImGuiCol_Text, vec4(1.0, 0.0, 0.0, 1.0));
			ImGui_Text("This level is not part of any campaign.");
			ImGui_PopStyleColor();
		}else{
			ImGui_BeginChild("Level info", vec2(0.0, 110.0), false);

			ImGui_Columns(2, false);
			ImGui_SetColumnWidth(0.0, second_column_width / 2.0);

			ImGui_Text("ID : " + level_id);
			ImGui_Text("Path : " + level_path);
			ImGui_Text("Completion optional : " + level_completion_optional);
			ImGui_NextColumn();
			ImGui_Image(level_thumbnail, vec2(180.0, 101.0));

			ImGui_EndChild();
		}

		ImGui_NextColumn();
	}
	bool Trigger(){
		if(campaign_control_option == finish_current_and_load_next){
			LevelFinished(level_id);

			if( IsLastLevel(level_id) ) {
				level.SendMessage("go_to_main_menu");
			} else {
				string next_level_id = GetFollowingLevel(level_id);
				if(next_level_id != ""){
					UnlockLevel(next_level_id); 
					// LoadLevelID(next_level_id);
					LoadLevel(GetLevelPath(next_level_id));
				} else {
					level.SendMessage("go_to_main_menu");
				}
			}
		}else if(campaign_control_option == finish_current){
			LevelFinished(level_id);
		}else if(campaign_control_option == load_next){
			if( IsLastLevel(level_id) ) {
				level.SendMessage("go_to_main_menu");
			} else {
				string next_level_id = GetFollowingLevel(level_id);
				if(next_level_id != ""){
					UnlockLevel(next_level_id); 
					// LoadLevelID(next_level_id);
					LoadLevel(GetLevelPath(next_level_id));
				} else {
					level.SendMessage("go_to_main_menu");
				}
			}
		}

		return true;
	}

	void LevelFinished(string name) {
		GetCampaignSave().SetValue("last_level_finished",name);
		SavedLevel@ level_save = GetLevelSave(name);

		array<string> valid_options = GetConfigValueOptions("difficulty_preset");
		string current_difficulty = GetConfigValueString("difficulty_preset");
		bool standard_difficulty = false;
		
		for( uint i = 0; i < valid_options.size(); i++ ) {
			if( current_difficulty == valid_options[i] ) {
				standard_difficulty = true;
			} 
		}

		if( standard_difficulty ) {
			bool previously_finished = false;
			for( uint i = 0; i < level_save.GetArraySize("finished_difficulties"); i++ ){
				if( level_save.GetArrayValue("finished_difficulties", i) == current_difficulty){
					previously_finished = true;
				}
			}

			if( previously_finished == false ){
				level_save.AppendArrayValue("finished_difficulties", current_difficulty);
			}
		}

		save_file.WriteInPlace();
	}

	void UnlockLevel(string name){
		if(IsLevelUnlocked(name) == false){
			SavedLevel @campaign = GetCampaignSave();
			campaign.AppendArrayValue("unlocked_levels", name);
			save_file.WriteInPlace();
		}
	}

	bool IsLastLevel(string name){
		Campaign camp = GetCampaign(campaign_id);

		array<ModLevel>@ levels = camp.GetLevels();

		if( levels.size() > 0 ) {
			if( levels[levels.size()-1].GetID() == name ){
				return true;
			}
		} 
		return false;
	}

	string GetFollowingLevel(string name){
		Campaign camp = GetCampaign(campaign_id);

		array<ModLevel>@ levels = camp.GetLevels();
		
		for( uint i = 0; i < levels.size()-1; i++ ) {
			if( name == levels[i].GetID() ) {
				return levels[i+1].GetID();
			}
		}
		return "";
	}

	string GetLevelPath(string id){
		Campaign camp = GetCampaign(campaign_id);

		array<ModLevel>@ levels = camp.GetLevels();
		
		for( uint i = 0; i < levels.size(); i++ ) {
			if( id == levels[i].GetID() ) {
				return levels[i].GetPath();
			}
		}
		return "";
	}

	bool IsLevelUnlocked( string level_id ){
		SavedLevel @campaign = GetCampaignSave();
		for( uint i = 0; i < campaign.GetArraySize("unlocked_levels"); i++ ) {
			if( campaign.GetArrayValue("unlocked_levels", i) == level_id ) {
				return true;
			}
		}
		return false;
	}

	SavedLevel@ GetCampaignSave(){
		return GetCampaignSave(campaign_id);
	}

	SavedLevel@ GetCampaignSave(string campaign_id){
		return save_file.GetSave(campaign_id, "linear_campaign", "");
	}

	SavedLevel@ GetLevelSave(string level_id){
		return GetLevelSave(campaign_id, level_id);
	}

	SavedLevel@ GetLevelSave(string campaign_id, string level_id){
		return save_file.GetSave(campaign_id, "linear_campaign", level_id);
	}

	void SetLevelPlayed(string name) {
		SavedLevel@ level_save = GetLevelSave(name);
		level_save.SetValue("level_played","true");
	}

	void SetLastLevelPlayed(string name) {
		GetCampaignSave().SetValue("last_level_played", name);
		GetGlobalSave().SetValue("last_campaign_played", campaign_id);
		GetGlobalSave().SetValue("last_level_played", name);
		save_file.WriteInPlace();
	}

	SavedLevel@ GetGlobalSave() {
		return save_file.GetSave("", "global", "");
	}
}
