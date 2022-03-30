#include "save/therium.as"
#include "levelinfo.as"

bool disable_progress = false;

void Dispose() {
}

void Update() {
    //if( GetConfigValueBool("block_cheating_progress") ) {
    //    if( disable_progress == false && EditorModeActive() ) {
    //        Log(warning, "Detected that editor mode activated during block_cheating_progress, deactivating progress handling");
    //        disable_progress = true; 
    //    }
    //} else if( disable_progress ){
    //    Log(warning, "Detected that block_cheating_progress was disabled, re-enabling progress handling");
    //    disable_progress = false;
    //}
}

void EnterCampaign() {
    disable_progress = false;
    SavedLevel@ campaign_save = CampaignSave();
    Campaign campaign = GetCampaign(GetCurrCampaignID());

    Parameter initial_unlocked_levels = campaign.GetParameter()["unlocked"];
    for( uint i = 0;  i < initial_unlocked_levels.size(); i++ ) {
        campaign_save.AppendArrayValueIfUnique("unlocked_levels",initial_unlocked_levels[i].asString()); 
    }
}

void EnterLevel() {
    Log(info, "Entered level " + GetCurrLevelName() );
    if(!disable_progress) {
        Campaign campaign = GetCampaign(GetCurrCampaignID());
        ModLevel level = campaign.GetLevel(GetCurrLevelID());

        SavedLevel@ level_save = LevelSave(level);
        SavedLevel@ campaign_save = CampaignSave();
        SavedLevel@ global_save = GlobalSave();
        level_save.SetValue("level_played","true"); 
        campaign_save.SetValue("last_level_played",GetCurrLevelID());
        global_save.SetValue("last_campaign_played",GetCurrCampaignID());
        global_save.SetValue("last_level_played",GetCurrLevelID());
        save_file.WriteInPlace();
    } else {
        Log(warning, "Will not mark level as last-played, we've been in editor mode and block_cheating_progress is true");
    }
}

void LeaveLevel() {
}

void LeaveCampaign() { 
}

void ReceiveMessage(string msg) {
    Log(info, "Getting msg in lugaru: " + msg );
    TokenIterator token_iter;
    token_iter.Init();
    if(!token_iter.FindNextToken(msg)){
        return;
    }
    string token = token_iter.GetToken(msg);

    if(token == "levelwin" ) {
        token_iter.FindNextToken(msg);
        string chosen_path = token_iter.GetToken(msg);

        Campaign campaign = GetCampaign(GetCurrCampaignID());
        ModLevel level = campaign.GetLevel(GetCurrLevelID());
        Parameter level_proot = level.GetParameter();

        SavedLevel@ campaign_save = CampaignSave();
        SavedLevel@ current_level_save = LevelSave(level);

        if(!disable_progress){
            campaign_save.SetValue("last_level_finished",level.GetID());

            string current_difficulty = GetConfigValueString("difficulty_preset");
            current_level_save.AppendArrayValueIfUnique("finished_difficulties", current_difficulty);

            string next_level_id = level_proot["levelnext"][chosen_path].asString();
            if(next_level_id != "") {
                Log(info, "unlocking " + next_level_id);
                campaign_save.AppendArrayValueIfUnique("unlocked_levels",next_level_id); 
                LoadLevelID(next_level_id);
            } else {
                Log(error,"Current level " + GetCurrLevelID() + " doesn't have any next-level directive for the path "  + chosen_path + " , this is likely unintended.");
                SendLevelMessage("go_to_main_menu");		
            }
            save_file.WriteInPlace();
        } else {
            Log(warning, "Ignoring levelwin command, level has been in editor mode and block_cheating_progress is true");
            string next_level_id = level_proot["levelnext"][chosen_path].asString();
            if(next_level_id != "") {
                LoadLevelID(next_level_id);
            } else {
                SendLevelMessage("go_to_main_menu");		
            }
        }
    }
}
