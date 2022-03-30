#include "save/linear.as"
#include "campaign_common.as"

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
    SavedLevel @camp_save = save_file.GetSave(GetCurrCampaignID(),"linear_campaign","");
            
    Campaign camp = GetCampaign(GetCurrCampaignID());

    array<ModLevel>@ levels = camp.GetLevels();

    //Unlock all levels
    for( uint i = 0; i < levels.size(); i++ ) {
        UnlockLevel(levels[i].GetID());
    }
}

void EnterLevel() {
    Log(info, "Entered level " + GetCurrLevelName() );
    if(!disable_progress) {
        SetLevelPlayed(GetCurrLevelID());
        SetLastLevelPlayed(GetCurrLevelID());
    } else {
        Log(warning, "Will not mark level as last-played, we've been in editor mode and block_cheating_progress is true");
    }
}

void LeaveLevel() {
    Log(info, "Left level"+ GetCurrLevelName());
}

void LeaveCampaign() { 
    Log(info, "Left campaign"+ GetCurrCampaignID());
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
        if(!disable_progress){
            string curr_id = GetCurrLevelID();
            LevelFinished(curr_id);

            Log( info, "Setting " + GetCurrLevel() + " as finished level " );

            if( IsLastLevel(curr_id) ) {
                //We can roll credits here maybe.
                SendLevelMessage("go_to_main_menu");		
            } else {
                string next_level_id = GetFollowingLevel(curr_id);
                if(next_level_id != ""){
                    UnlockLevel(next_level_id); 

                    LoadLevelID(next_level_id);
                } else {
                    Log(error, "unexpected error" );
                    SendLevelMessage("go_to_main_menu");		
                }
            }
        } else {
            Log(warning, "Ignoring levelwin command, level has been in editor mode and block_cheating_progress is true");
            string curr_id = GetCurrLevelID();
            if(IsLastLevel(curr_id) == false) {
                string next_level_id = GetFollowingLevel(curr_id);
                LoadLevelID(next_level_id);
            } else {
                SendLevelMessage("go_to_main_menu");		
            }
        }
    }
}
