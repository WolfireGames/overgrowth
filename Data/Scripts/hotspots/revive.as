//-----------------------------------------------------------------------------
//           Name: revive.as
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

void Init() {
}

void HandleEvent(string event, MovementObject @mo){
    if(event == "enter"){
        OnEnter(mo);
    } if(event == "exit"){
        //Print("Exited Revive\n");
    }
}

void OnEnter(MovementObject @mo) {
    //Print("Entered Revive\n");
	int num_chars = GetNumCharacters();
				for(int i=0; i<num_chars; ++i){
					MovementObject @char = ReadCharacter(i);
	char.Execute("RecoverHealth();cut_throat = false;lives = p_lives;ko_shield = max_ko_shield;");
	}
}