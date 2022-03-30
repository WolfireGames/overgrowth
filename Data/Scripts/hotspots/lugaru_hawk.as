//-----------------------------------------------------------------------------
//           Name: lugaru_hawk.as
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

float time = 0;
float next_sound_time = 0;
float sound_interval = 30.0f;
int hawk_id = -1;

void Init() {
	next_sound_time = RangedRandomFloat(0.0f, sound_interval) + 2.0f + the_time;
}

void Dispose() {
	if(hawk_id != -1){
		if(ObjectExists(hawk_id)){
			DeleteObjectID(hawk_id);
		}
		hawk_id = -1;
	}
}

void Update() {
	if(hawk_id == -1){
		hawk_id = CreateObject("Data/Prototypes/Lugaru/Hawk_Offset.xml", true);
	}
	if(ObjectExists(hawk_id)){
		Object@ hawk_obj = ReadObjectFromID(hawk_id);
		Object@ this_hotspot = ReadObjectFromID(hotspot.GetID());
		hawk_obj.SetTranslation(this_hotspot.GetTranslation());
	    hawk_obj.SetRotation(this_hotspot.GetRotation() * quaternion(vec4(0.0, 1.0, 0.0, the_time * 0.5)));
	}

    if(the_time > next_sound_time){
		next_sound_time += sound_interval;
    	//PlaySound("Data/Sounds/lugaru/hawk.ogg");
    }

}
