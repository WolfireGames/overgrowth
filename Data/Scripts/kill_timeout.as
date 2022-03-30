//-----------------------------------------------------------------------------
//           Name: kill_timeout.as
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

void SetParameters() {
	params.AddFloatSlider("Delay",2.46,"min:0.01,max:10.0,step:0.01,text_mult:1");
}

array<Victim@> victims;
Object@ thisHotspot = ReadObjectFromID(hotspot.GetID());

class Victim{
	float timer = 0.0;
	int character_id = -1;
	Victim(float delay, int character_id){
		timer = delay;
		this.character_id = character_id;
	}
	bool UpdateTimer(float delta){
		timer -= delta;
		if(timer <= 0.0){
			KillCharacter(character_id);
			return true;
		}else{
			return false;
		}
	}
}

void KillCharacter(int character_id){
	MovementObject@ char = ReadCharacterID(character_id);
	char.Execute(	"SetKnockedOut(_dead);" +
					"Ragdoll(_RGDL_INJURED);");
}

void Reset(){
	victims.resize(0);
}

void HandleEvent(string event, MovementObject @mo){
	if(event == "enter"){
		OnEnter(mo);
	} else if(event == "exit"){
		OnExit(mo);
	}
}

void OnEnter(MovementObject @mo) {
	victims.insertLast(Victim(params.GetFloat("Delay"), mo.GetID()));
}

void OnExit(MovementObject @mo) {
	for(uint i = 0; i < victims.size(); i++){
		if(victims[i].character_id == mo.GetID()){
			victims.removeAt(i);
			return;
		}
	}
}

void Update(){
	for(uint i = 0; i < victims.size(); i++){
		if(victims[i].UpdateTimer(time_step)){
			victims.removeAt(i);
			return;
		}
	}
}
