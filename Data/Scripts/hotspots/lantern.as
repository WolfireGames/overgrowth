//-----------------------------------------------------------------------------
//           Name: lantern.as
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

int light_id = -1;
int lamp_id = -1;
vec3 light_color = vec3(8.0f,7.75f,6.75f);
vec3 light_range = vec3(500.0f);
bool init_done = false;

void Init() {
    Object@ hotspot_obj = ReadObjectFromID(hotspot.GetID());
    hotspot_obj.SetScale(0.5f);
}
void Update() {
    if(!init_done){
        FindSavedLantern();
        init_done = true;
    }

    if(light_id == -1 || !ObjectExists(light_id)){
        CreateLight();
    }else if(lamp_id == -1 || !ObjectExists(lamp_id)){
        CreateLantern();
    }else{
        ItemObject@ io = ReadItemID(lamp_id);
        Object@ light = ReadObjectFromID(light_id);
        light.SetTranslation(io.GetPhysicsPosition());
    }
}

void CreateLight(){
    //Setting up the actual light emitter.
    light_id = CreateObject("Data/Objects/lights/dynamic_light.xml");
    Object@ light = ReadObjectFromID(light_id);
    light.SetScaleable(true);
    light.SetTintable(true);
    light.SetSelectable(true);
    light.SetTranslatable(true);
}

void CreateLantern(){
    //This is the lantern model that can be equipt.
    lamp_id = CreateObject("Data/Objects/lantern_small.xml", false);
    Object@ lamp = ReadObjectFromID(lamp_id);
    ScriptParams@ lamp_params = lamp.GetScriptParams();
    lamp_params.SetInt("BelongsTo", hotspot.GetID());
    Object@ hotspot_obj = ReadObjectFromID(hotspot.GetID());
    lamp.SetTranslation(hotspot_obj.GetTranslation());
    lamp.SetSelectable(true);
    lamp.SetTranslatable(true);

}

void FindSavedLantern(){
    array<int> all_obj = GetObjectIDsType(_item_object);
    Print("found nr " + all_obj.size() + "\n");
    for(uint i = 0; i < all_obj.size(); i++){
        Object@ current_obj = ReadObjectFromID(all_obj[i]);
        ScriptParams@ current_param = current_obj.GetScriptParams();
        Print("If " + all_obj[i] + " belongs to \n");
        if(current_param.HasParam("BelongsTo")){
            if(current_param.GetInt("BelongsTo") == hotspot.GetID()){
                lamp_id = all_obj[i];
                return;
            }
        }
    }
}
