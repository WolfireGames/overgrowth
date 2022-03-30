#include "threatcheck.as"

void SetParameters() {
    params.AddString("music", "atense");
}

string music_prefix;
void Init() {
    if(params.GetString("music") == "atense"){
        AddMusic("Data/Music/atense.xml");
    } else if(params.GetString("music") == "challengelevel"){
        AddMusic("Data/Music/challengelevel.xml");
    } else if(params.GetString("music") == "b4b"){
        AddMusic("Data/Music/b4b.xml");
    } else if(params.GetString("music") == "b4b2"){
        AddMusic("Data/Music/b4b2.xml");
    } else if(params.GetString("music") == "bancients"){
        AddMusic("Data/Music/bancients.xml");
    } else if(params.GetString("music") == "bboss"){
        AddMusic("Data/Music/bboss.xml");
    } else if(params.GetString("music") == "bending"){
        AddMusic("Data/Music/bending.xml");
    } else if(params.GetString("music") == "bfending"){
        AddMusic("Data/Music/bfending.xml");
    } else if(params.GetString("music") == "b-s2"){
        AddMusic("Data/Music/b-s2.xml");
    } else if(params.GetString("music") == "e-a4-a"){
        AddMusic("Data/Music/e-a4-a.xml");
    } else if(params.GetString("music") == "e-a4-b"){
        AddMusic("Data/Music/e-a4-b.xml");
    } else if(params.GetString("music") == "e-b3"){
        AddMusic("Data/Music/e-b3.xml");
    } else if(params.GetString("music") == "hub"){
        AddMusic("Data/Music/hub.xml");
    } else if(params.GetString("music") == "prologue"){
        AddMusic("Data/Music/prologue.xml");
    } else if(params.GetString("music") == "reaper"){
        AddMusic("Data/Music/reaper.xml");
    } else if(params.GetString("music") == "S2A3"){
        AddMusic("Data/Music/S2A3.xml");
    } else if(params.GetString("music") == "spiritual"){
        AddMusic("Data/Music/spiritual.xml");
    } else if(params.GetString("music") == "e-a4-a2"){
        AddMusic("Data/Music/e-a4-a2.xml");
    } else if(params.GetString("music") == "lugaru"){
        AddMusic("Data/Music/lugaru.xml");
    } else if(params.GetString("music") == "empry"){
        AddMusic("Data/Music/empry.xml");
	}
    music_prefix = params.GetString("music") + "_";

	bool postinit = false;
    level.ReceiveLevelEvents(hotspot.GetID());
}
bool postinit = false;
void PostInit(){
    AddMusic(params.GetString("music"));
    postinit = true;
}

void Dispose() {
    level.StopReceivingLevelEvents(hotspot.GetID());
}


float blackout_amount = 0.0;
float ko_time = -1.0;

void Update() {
    int player_id = GetPlayerCharacterID();
    if(player_id != -1 && ReadCharacter(player_id).GetIntVar("knocked_out") != _awake){
        PlaySong(music_prefix + "sad");
        return;
    }
    int threats_remaining = ThreatsRemaining();
    if(threats_remaining == 0){
        PlaySong(music_prefix + "ambient-happy");
        return;
    }
    if(player_id != -1 && ReadCharacter(player_id).QueryIntFunction("int CombatSong()") == 1){
        PlaySong(music_prefix + "combat");
        return;
    }
    PlaySong(music_prefix + "ambient-tense");

    if(!postinit){
        PostInit();
    }
	
	blackout_amount = 0.0;
	if(player_id != -1 && ObjectExists(player_id)){
		MovementObject@ char = ReadCharacter(player_id);
		if(char.GetIntVar("knocked_out") != _awake){
			if(ko_time == -1.0f){
				ko_time = the_time;
			}
			if(ko_time < the_time - 1.0){
				if(GetInputPressed(0, "attack") || ko_time < the_time - 5.0){
	            	level.SendMessage("reset"); 				               
				}
			}
            blackout_amount = 0.2 + 0.6 * (1.0 - pow(0.5, (the_time - ko_time)));
		} else {
			ko_time = -1.0f;
		}
	} else {
        ko_time = -1.0f;
    }
}

array<float> dof_params;

void ReceiveMessage(string msg) {
    TokenIterator token_iter;
    token_iter.Init();

    if(!token_iter.FindNextToken(msg)) {
        return;
    }

    string token = token_iter.GetToken(msg);

    if(token == "therium2_enable_object") {
        if(!token_iter.FindNextToken(msg)) {
            Log(warning, "therium2_enable_object - missing required parameters. Syntax is: \"therium2_enable_object object_id\"");
            return;
        }

        int object_id = atoi(token_iter.GetToken(msg));

        HandleSetEnabledMessage("therium2_enable_object", object_id, true);
    } else if(token == "therium2_disable_object") {
        if(!token_iter.FindNextToken(msg)) {
            Log(warning, "therium2_disable_object missing required parameters. Syntax is: \"therium2_disable_object object_id\"");
            return;
        }

        int object_id = atoi(token_iter.GetToken(msg));

        HandleSetEnabledMessage("therium2_disable_object", object_id, false);
    } else if(token == "level_event" &&
       token_iter.FindNextToken(msg))
    {
        string sub_msg = token_iter.GetToken(msg);
        if(sub_msg == "set_camera_dof"){
            dof_params.resize(0);
            while(token_iter.FindNextToken(msg)){
                dof_params.push_back(atof(token_iter.GetToken(msg)));
            }
            if(dof_params.size() == 6){
                camera.SetDOF(dof_params[0], dof_params[1], dof_params[2], dof_params[3], dof_params[4], dof_params[5]);
            }
        }
        if(sub_msg == "therium2_enable_object") {
            if(!token_iter.FindNextToken(msg)) {
                Log(warning, "therium2_enable_object - missing required parameters. Syntax is: \"therium2_enable_object object_id\"");
                return;
            }

            int object_id = atoi(token_iter.GetToken(msg));

            HandleSetEnabledMessage("therium2_enable_object", object_id, true);
        } else if(sub_msg == "therium2_disable_object") {
            if(!token_iter.FindNextToken(msg)) {
                Log(warning, "therium2_disable_object missing required parameters. Syntax is: \"therium2_disable_object object_id\"");
                return;
            }

            int object_id = atoi(token_iter.GetToken(msg));

            HandleSetEnabledMessage("therium2_disable_object", object_id, false);
        }
    }
}

void HandleSetEnabledMessage(string message_name, int object_id, bool is_enabled) {
    if(object_id == -1 || !ObjectExists(object_id)) {
        Log(warning, message_name + " - unable to find object with id: " + object_id);
        return;
    }

    Object@ obj = ReadObjectFromID(object_id);

    if(obj.GetEnabled() == is_enabled) {
        Log(warning, message_name + " - object id: " + object_id + " - was already in desired state is_enabled: " + is_enabled);
    }

    obj.SetEnabled(is_enabled);
    Log(info, message_name + " - set object id: " + object_id + " - is_enabled: " + is_enabled);
}

void PreDraw(float curr_game_time) {
    camera.SetTint(camera.GetTint() * (1.0 - blackout_amount));
}

void Draw(){
    if(EditorModeActive()){
        Object@ obj = ReadObjectFromID(hotspot.GetID());
        DebugDrawBillboard("Data/Textures/therium/logo256.png",
                           obj.GetTranslation(),
                           obj.GetScale()[1]*2.0,
                           vec4(vec3(0.5), 1.0),
                           _delete_on_draw);
    }
}
