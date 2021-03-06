//-----------------------------------------------------------------------------
//           Name: teamswitch.as
//      Developer: Wolfire Games LLC
//    Script Type:
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

string displayText;
string changeToTeam;

void SetParameters() {

params.AddString("Change to Team","false");
    changeToTeam = params.GetString("Change to Team");
	
params.AddIntCheckbox("Play for NPCs", false);

}

void HandleEvent(string event, MovementObject @mo){
    if(event == "enter"){
        OnEnter(mo);
    } else if(event == "exit"){
        OnExit(mo);
    }
}

void OnEnter(MovementObject @mo) {
	if(mo.controlled || params.GetInt("Play for NPCs") == 1){
		Object@ obj = ReadObjectFromID(mo.GetID());
		ScriptParams@ params = obj.GetScriptParams();
		params.SetString("Teams", ""+changeToTeam+"");
	}
}

void OnExit(MovementObject @mo) {
    if(mo.controlled){
        level.Execute("ReceiveMessage(\"cleartext\")");
    }
}

