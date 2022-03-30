#include "save/general.as"
#include "levelinfo.as"

void SetLevelPlayed(string name) {
    SavedLevel@ level_save = GetLevelSave(name);
    level_save.SetValue("level_played","true");
}

void SetLastLevelPlayed(string name) {
    Log(info, "Setting last played level in campaign " + GetCurrCampaignID() + " as " + name);
    GetCampaignSave().SetValue("last_level_played",name);
    GetGlobalSave().SetValue("last_campaign_played",GetCurrCampaignID());
    GetGlobalSave().SetValue("last_level_played",name);
    save_file.WriteInPlace();
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
        for( uint i = 0; i < level_save.GetArraySize("finished_difficulties"); i++ ) {
            if( level_save.GetArrayValue("finished_difficulties", i) == current_difficulty) {
                previously_finished = true;
            }
        }

        if( previously_finished == false ) {
            level_save.AppendArrayValue("finished_difficulties", current_difficulty);
        }
    }

    save_file.WriteInPlace();
}

void UnlockLevel( string name ) {
    Log(info, "Unlocking " + name + " in campaign " + GetCurrCampaignID() ); 
    if( IsLevelUnlocked(name) == false ) {
        SavedLevel @campaign = GetCampaignSave();
        campaign.AppendArrayValue("unlocked_levels",name);
        save_file.WriteInPlace();
    }
}

bool IsLastLevel(string name) {
    Campaign camp = GetCampaign(GetCurrCampaignID());

    array<ModLevel>@ levels = camp.GetLevels();

    if( levels.size() > 0 ) {
        if( levels[levels.size()-1].GetID() == name ) {
            return true;
        }
    } 
    return false;
}

string GetFollowingLevel(string name) {
    Campaign camp = GetCampaign(GetCurrCampaignID());

    array<ModLevel>@ levels = camp.GetLevels();
    
    for( uint i = 0; i < levels.size()-1; i++ ) {
        if( name == levels[i].GetID() ) {
            return levels[i+1].GetID();
        }
    }
    return "";
}

string GetLevelPath(string id) {
    Campaign camp = GetCampaign(GetCurrCampaignID());

    array<ModLevel>@ levels = camp.GetLevels();
    
    for( uint i = 0; i < levels.size(); i++ ) {
        if( id == levels[i].GetID() ) {
            return levels[i].GetPath();
        }
    }
    return "";
}
