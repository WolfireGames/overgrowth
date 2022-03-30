//-----------------------------------------------------------------------------
//           Name: resetcharacter.as
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

void SetParameters() {
}

void HandleEvent(string event, MovementObject @mo){
    if(event == "enter"){
        OnEnter(mo);
    } else if (event == "exit"){
    	OnExit(mo);
    }
}

void OnEnter(MovementObject @mo) {
	Object@ charObject = ReadObjectFromID(mo.GetID());
	mo.Execute("Recover();");
	mo.Execute("Reset();");
	mo.position = charObject.GetTranslation();
	mo.velocity = vec3(0);
	//mo.Execute("SetParameters();");
	mo.Execute("PostReset();");
	mo.Execute("ResetSecondaryAnimation();");
	if(mo.controlled){
		level.SendMessage("achievement_event character_reset_hotspot");
	}
}

void OnExit(MovementObject @mo) {;

}