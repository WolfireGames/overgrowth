//-----------------------------------------------------------------------------
//           Name: torch.as
//      Developer: Wolfire Games LLC
//    Script Type: Hotspot
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
bool is_initialized = false;

void Init() {
    SetHotspotScale(0.5f);
}

void Update() {
    if (!is_initialized) {
        InitializeTorch();
        is_initialized = true;
    }

    if (lamp_id == -1 || !ObjectExists(lamp_id)) {
        CreateTorch();
        return;
    }

    if (light_id == -1 || !ObjectExists(light_id)) {
        Print("No flame hotspot found\n");
        return;
    }

    UpdateLightPosition();
}

void InitializeTorch() {
    FindSavedTorch();
    FindFlameHotspot();
}

void SetHotspotScale(float scale) {
    Object@ hotspot_obj = ReadObjectFromID(hotspot.GetID());
    hotspot_obj.SetScale(scale);
}

void CreateTorch() {
    lamp_id = CreateObject("Data/Items/torch.xml", false);
    Object@ lamp_obj = ReadObjectFromID(lamp_id);
    ScriptParams@ lamp_params = lamp_obj.GetScriptParams();
    lamp_params.SetInt("BelongsTo", hotspot.GetID());

    Object@ hotspot_obj = ReadObjectFromID(hotspot.GetID());
    lamp_obj.SetTranslation(hotspot_obj.GetTranslation());
    lamp_obj.SetSelectable(true);
    lamp_obj.SetTranslatable(true);
}

void FindSavedTorch() {
    array<int>@ item_ids = GetObjectIDsType(_item_object);
    for (uint i = 0; i < item_ids.length(); ++i) {
        Object@ obj = ReadObjectFromID(item_ids[i]);
        ScriptParams@ obj_params = obj.GetScriptParams();
        if (obj_params.HasParam("BelongsTo") && obj_params.GetInt("BelongsTo") == hotspot.GetID()) {
            lamp_id = item_ids[i];
            return;
        }
    }
}

void FindFlameHotspot() {
    array<int>@ hotspot_ids = GetObjectIDsType(_hotspot_object);
    for (uint i = 0; i < hotspot_ids.length(); ++i) {
        Object@ obj = ReadObjectFromID(hotspot_ids[i]);
        ScriptParams@ obj_params = obj.GetScriptParams();
        if (!obj_params.HasParam("FlameTaken")) {
            continue;
        }
        int flame_taken = obj_params.GetInt("FlameTaken");
        if (flame_taken == 0 || flame_taken == hotspot.GetID()) {
            obj_params.SetInt("FlameTaken", hotspot.GetID());
            light_id = hotspot_ids[i];
            return;
        }
    }
}

void UpdateLightPosition() {
    ItemObject@ torch_item = ReadItemID(lamp_id);
    Object@ light_obj = ReadObjectFromID(light_id);
    mat4 torch_transform = torch_item.GetPhysicsTransform();
    quaternion torch_rotation = QuaternionFromMat4(torch_transform.GetRotationPart());
    vec3 new_position = torch_item.GetPhysicsPosition() + (torch_rotation * vec3(0.0f, 0.35f, 0.0f)) + vec3(0.0f, -0.25f, 0.0f);
    light_obj.SetTranslation(new_position);
}
