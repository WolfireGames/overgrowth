//-----------------------------------------------------------------------------
//           Name: explosive3.as
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
	params.AddString("Smoke particle amount", "5.0");
}

void HandleEvent(string event, MovementObject @mo){
    if(event == "enter"){
        OnEnter(mo);
    }
	else if(event == "exit"){
        OnExit(mo);
    }
}
void OnEnter(MovementObject @mo) {
		Object@ thisHotspot = ReadObjectFromID(hotspot.GetID());
		vec3 explosion_point = thisHotspot.GetTranslation();
		MakeMetalSparks(explosion_point);
		float speed = 5.0f;
		for(int i=0; i<(params.GetFloat("Smoke particle amount")); i++){
				MakeParticle("Data/Particles/explosion_smoke.xml",mo.position,
				vec3(RangedRandomFloat(-speed,speed),RangedRandomFloat(-speed,speed),RangedRandomFloat(-speed,speed)));
		}
	  PlaySound("Data/Sounds/explosives/explosion3.wav");
}
void OnExit(MovementObject @mo) {
}
void MakeMetalSparks(vec3 pos){
    int num_sparks = 60;
		float speed = 20.0f;
    for(int i=0; i<num_sparks; ++i){
        MakeParticle("Data/Particles/explosion_fire.xml",pos,vec3(RangedRandomFloat(-speed,speed),
                                                         RangedRandomFloat(-speed,speed),
                                                         RangedRandomFloat(-speed,speed)));
    }
}
