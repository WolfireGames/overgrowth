SavedLevel@ CampaignSave() {
    return save_file.GetSave(GetCurrCampaignID(), "linear_campaign","");
}

SavedLevel@ LevelSave(ModLevel level) {
    return save_file.GetSave(GetCurrCampaignID(), "linear_campaign",level.GetID());
}

SavedLevel@ GlobalSave() {
    return save_file.GetSave("","global","");
}

SavedLevel@ GetGlobalSave() {
    return save_file.GetSave("","global","");
}

SavedLevel@ GetCampaignSave(string campaign_id) {
    return save_file.GetSave(campaign_id,"linear_campaign","");
}

SavedLevel@ GetCampaignSave() {
    return GetCampaignSave(GetCurrCampaignID());
}

SavedLevel@ GetLevelSave(string level_id) {
    return GetLevelSave(GetCurrCampaignID(),level_id);
}

SavedLevel@ GetLevelSave(string campaign_id,string level_id) {
    return save_file.GetSave(campaign_id, "linear_campaign",level_id);
}

string GetLastLevelPlayed(string campaign_id) {
    return GetCampaignSave(campaign_id).GetValue("last_level_played");
}

int GetHighestDifficultyFinished(string name) {
    return GetHighestDifficultyFinished(GetCurrCampaignID(),name);
}

int GetHighestDifficultyFinished(string campaign, string name) {
    int highest = 0;
    SavedLevel@ level_save = GetLevelSave(campaign,name);
    array<string> valid_options = GetConfigValueOptions("difficulty_preset");
    
    for( uint i = 0; i < valid_options.size(); i++ ) {
        for( uint k = 0; k < level_save.GetArraySize("finished_difficulties"); k++ ) {
            if( level_save.GetArrayValue("finished_difficulties", k) == valid_options[i] ) {
                highest = i+1;
            }
        }
    }
    return highest;
}

int GetHighestDifficultyFinishedCampaign(string campaign_id ) {
    int highest = 0;
    Campaign campaign = GetCampaign(campaign_id);
    array<ModLevel>@ levels = campaign.GetLevels();
    array<string> valid_options = GetConfigValueOptions("difficulty_preset");
    for( uint i = 0; i < valid_options.size(); i++ ) {
        bool all_ok = true;
        for( uint k = 0; k < levels.length(); k++ ) {
            ModLevel level = levels[k];

            if(!level.CompletionOptional() && GetHighestDifficultyFinished(campaign_id,level.GetID()) < int(i+1)) {
                all_ok = false;
            }
        }
        if( all_ok ) {
            highest = i+1;
        }
    }
    return highest;
}

uint GetHighestDifficultyIndex( array<string>@ finished ) {
    uint highest = 0;
    array<string> valid_options = GetConfigValueOptions("difficulty_preset");
    
    for( uint i = 0; i < valid_options.size(); i++ ) {
        for( uint k = 0; k < finished.size(); k++ ) {
            if(finished[k] == valid_options[i]) {
                highest = i+1;
            }
        }
    }
    return highest;
}

int GetCurrentDifficulty() {
    array<string> valid_options = GetConfigValueOptions("difficulty_preset");
    string current = GetConfigValueString("difficulty_preset");
    
    for( uint i = 0; i < valid_options.size(); i++ ) {
        if( valid_options[i] == current ) {
            return i+1;
        }
    }
    return 0;
}

bool IsLastCampaignPlayed(Campaign campaign) {
    return GetGlobalSave().GetValue("last_level_played") == campaign.GetID();
}

bool IsLastPlayedLevel(ModLevel level){
    SavedLevel @campaign = GetCampaignSave();
    if( campaign.GetValue("last_level_played") == level.GetID()) {
        return true;
    }
    return false;
}

bool IsLastPlayedLevel(LevelInfo@ level){
    SavedLevel @campaign = GetCampaignSave();
    if( campaign.GetValue("last_level_played") == level.id ) {
        return true;
    }
    return false;
}

bool IsLevelUnlocked(ModLevel level ) {
    return IsLevelUnlocked(level.GetID());
}

bool IsLevelUnlocked(LevelInfo@ level ) {
    return IsLevelUnlocked(level.id);
}

bool IsLevelUnlocked( string level_id ) {
    SavedLevel @campaign = GetCampaignSave();
    for( uint i = 0; i < campaign.GetArraySize("unlocked_levels"); i++ ) {
        if( campaign.GetArrayValue("unlocked_levels",i) == level_id ) {
            return true;
        }
    }
    return false;
}

bool GetLevelPlayed(string name) {
    SavedLevel@ level_save = GetLevelSave(name);
    return level_save.GetValue("level_played") == "true";
}
