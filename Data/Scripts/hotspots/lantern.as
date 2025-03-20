//-----------------------------------------------------------------------------
//           Name: lantern.as
//      Developer: Wolfire Games LLC
//    Script Type: Hotspot
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
bool init_done = false;

void Init() {
    ReadObjectFromID(hotspot.GetID()).SetScale(0.5f);
}

void Update() {
    if (!init_done) {
        FindSavedLantern();
        init_done = true;
    }
    if (!EnsureLightAndLampExist()) {
        return;
    }
    UpdateLightPosition();
}

void CreateLight() {
    light_id = CreateObject("Data/Objects/lights/dynamic_light.xml");
    Object@ light = ReadObjectFromID(light_id);
    light.SetScaleable(true);
    light.SetTintable(true);
    light.SetSelectable(true);
    light.SetTranslatable(true);
}

void CreateLantern() {
    lamp_id = CreateObject("Data/Objects/lantern_small.xml", false);
    Object@ lamp = ReadObjectFromID(lamp_id);
    lamp.GetScriptParams().SetInt("BelongsTo", hotspot.GetID());
    Object@ hotspot_obj = ReadObjectFromID(hotspot.GetID());
    lamp.SetTranslation(hotspot_obj.GetTranslation());
    lamp.SetSelectable(true);
    lamp.SetTranslatable(true);
}

void FindSavedLantern() {
    array<int> all_objs = GetObjectIDsType(_item_object);
    for (uint i = 0; i < all_objs.length(); ++i) {
        Object@ current_obj = ReadObjectFromID(all_objs[i]);
        ScriptParams@ current_param = current_obj.GetScriptParams();
        if (current_param.HasParam("BelongsTo") && current_param.GetInt("BelongsTo") == hotspot.GetID()) {
            lamp_id = all_objs[i];
            return;
        }
    }
}

bool EnsureLightAndLampExist() {
    if (light_id == -1 || !ObjectExists(light_id)) {
        CreateLight();
        return false;
    }
    if (lamp_id == -1 || !ObjectExists(lamp_id)) {
        CreateLantern();
        return false;
    }
    return true;
}

void UpdateLightPosition() {
    ItemObject@ io = ReadItemID(lamp_id);
    Object@ light = ReadObjectFromID(light_id);
    light.SetTranslation(io.GetPhysicsPosition());
}
