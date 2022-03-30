//-----------------------------------------------------------------------------
//           Name: adv_dialogue_kocheck.as
//      Developer: Wolfire Games LLC
//    Script Type: Hotspot
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
//   Copyright 2022 Wolfire Games LLC
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//-----------------------------------------------------------------------------

bool played;

void Reset() {
    played = false;
}

void Init() {
    Reset();
	//vec3 end = char.rigged_object().GetAvgIKChainPos("torso");
}

void SetParameters() {
	params.AddIntCheckbox("Play Once", true);
	params.AddIntCheckbox("Play Only If Dead", false);
	params.AddIntCheckbox("Play for NPCs", false);
	params.AddIntCheckbox("Play If No Combat", true);

    params.AddString("All Enemies Neutralized", "Default text");
    params.AddString("Some Enemies Neutralized", "Default text");
    params.AddString("No Enemies Neutralized", "Default text");
}

void HandleEvent(string event, MovementObject @mo){
    if(event == "enter"){
        OnEnter(mo);
    } else if(event == "exit"){
        OnExit(mo);
    }
}

void OnEnter(MovementObject @mo) {
    if((mo.GetIntVar("knocked_out") > 0 || params.GetInt("Play Only If Dead") == 0)								//condition for "Play Only If Dead"
		&& (!played || params.GetInt("Play Once") == 0)															//condition for "Play once"
		&& (mo.controlled || params.GetInt("Play for NPCs") == 1)												//condition for "Play for NPCs"
		&& (mo.QueryIntFunction("int CombatSong()") == 0) || params.GetInt("Play If No Combat") == 0){			//condition for No Combat

		
		int num_chars = GetNumCharacters();
		bool everyone_alive = true;
		bool everyone_dead = true;
		for(int i=0; i<num_chars; ++i){												//check all character if they are alive
            MovementObject @char = ReadCharacter(i);
			if(!char.controlled && !mo.OnSameTeam(char)){
				if(char.GetIntVar("knocked_out") > 0){
					everyone_alive = false;
					Log(warning, "dude is ko");
				} else {
					everyone_dead = false;
					Log(warning, "dude is conscious");
				}
			} else {
				Log(warning, "this is the player char");
			}
		}
		
		if(everyone_alive){															//branches for different security states
			Log(warning, "everyone's alive");
			level.SendMessage("start_dialogue \""+params.GetString("No Enemies Neutralized")+"\"");
		} else if(everyone_dead){
			Log(warning, "everyone's dead");
			level.SendMessage("start_dialogue \""+params.GetString("All Enemies Neutralized")+"\"");
		} else {
			Log(warning, "some survived");
			level.SendMessage("start_dialogue \""+params.GetString("Some Enemies Neutralized")+"\"");
		}
        played = true;
    }
}

void OnExit(MovementObject @mo) {
}