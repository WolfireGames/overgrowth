void Dispose() {
}

void Update() {
}

void EnterCampaign() {
    Campaign camp = GetCampaign(GetCurrCampaignID());

    array<ModLevel>@ levels = camp.GetLevels();
}

void EnterLevel() {
    Log(info, "Entered level " + GetCurrLevelName() );
}

void LeaveLevel() {
    Log(info, "Left level"+ GetCurrLevelName());
}

void LeaveCampaign() { 
    Log(info, "Left campaign"+ GetCurrCampaignID());
}

void ReceiveMessage(string msg) {
    TokenIterator token_iter;
    token_iter.Init();

    if(!token_iter.FindNextToken(msg)){
        return;
    }

    string token = token_iter.GetToken(msg);

    if(Online_IsClient()) {
        return;
    }
    
    if(token == "levelwin") {
        string curr_id = GetCurrLevelID();

        Log( info, "Setting " + GetCurrLevel() + " as finished level " );


        Campaign camp = GetCampaign(GetCurrCampaignID());

        array<ModLevel>@ levels = camp.GetLevels();
	array<string> level_ids;

        for(uint i = 0; i < levels.size(); i++) {
            if(curr_id != levels[i].GetID()) {
                level_ids.push_back(levels[i].GetID());
            }
        }
	
        if(level_ids.size() > 0) {
            LoadLevelID(level_ids[rand() % level_ids.size()]);
        }
    }
}

