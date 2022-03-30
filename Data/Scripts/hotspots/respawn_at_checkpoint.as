//-----------------------------------------------------------------------------
//           Name: respawn_at_checkpoint.as
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
    }
}

void OnEnter(MovementObject @mo) {
    Log(info, "Entered");
    float latest_time = -1.0f;
    int best_obj = -1;
    array<int> @object_ids = GetObjectIDsType(_hotspot_object);
    for(int i=0, len=object_ids.size(); i<len; ++i){
        Object@ obj = ReadObjectFromID(object_ids[i]);
        ScriptParams@ params = obj.GetScriptParams();
        if(params.HasParam("LastEnteredTime")){
            Log(info, "Found hotspot");
            float curr_time = params.GetFloat("LastEnteredTime");
            if(curr_time > latest_time){
                Log(info, "Best time: " + curr_time);
                best_obj = object_ids[i];
                latest_time = curr_time;
            } else {
                Log(info, "Bad time: " + curr_time);                
            }
        }
    }
    
    if(best_obj != -1){
        mo.position = ReadObjectFromID(best_obj).GetTranslation();
        mo.velocity = vec3(0.0);
    }
}
