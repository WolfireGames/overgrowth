//-----------------------------------------------------------------------------
//           Name: start_dialogue_inside.as
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
bool player_inside = false;
array<int> chars_inside;


void Reset() {
		chars_inside.resize(0);
		Object@ obj = ReadObjectFromID(hotspot.GetID());
		vec3 pos = obj.GetTranslation();
		vec3 scale = obj.GetScale();
		int num_chars = GetNumCharacters();			//loop all characters and check if they are inside
		for(int i=0; i<num_chars; ++i){								//check all character if they are alive
            MovementObject @char = ReadCharacter(i);
			if(!char.controlled){
				vec3 rel_pos = char.position - pos;
				if(
				abs(rel_pos.x)<scale.x*2.0f &&
				abs(rel_pos.y)<scale.y*2.0f &&
				abs(rel_pos.z)<scale.z*2.0f ){
					chars_inside.insertLast(char.GetID());
					Log(warning, "added a char inside");		
				}
			}
		}
		Log(warning, "found chars inside:" + chars_inside.size());	
    played = false;
}

void Init() {
    Reset();
}

void SetParameters() {
	params.AddIntCheckbox("Play Once", true);
	params.AddIntCheckbox("Player Must Be Inside", true);
    params.AddString("Dialogue", "Default text");
}

void HandleEvent(string event, MovementObject @mo){
    if(event == "enter"){
        OnEnter(mo);
    } else if(event == "exit"){
        OnExit(mo);
    }
}

void OnEnter(MovementObject @mo) {
    if(mo.controlled){
        player_inside = true;
    }
}

void OnExit(MovementObject @mo) {
        player_inside = false;
}

void Update() {
	if(player_inside || params.GetInt("Player Must Be Inside") == 0){
		bool all_dead = true;
		int num_chars = chars_inside.size();
		for(int i=0; i< num_chars; ++i){
            MovementObject@ char = ReadCharacterID(chars_inside[i]);
			if(!(char.GetIntVar("knocked_out") > 0))all_dead = false;
		}
		if(all_dead && (!played ||  params.GetInt("Play Once") == 0)){
			Log(warning, "start dialogue" + chars_inside.size());	
			level.SendMessage("start_dialogue \""+params.GetString("Dialogue")+"\"");
			played = true;
		}
	}
	
}