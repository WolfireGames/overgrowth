//-----------------------------------------------------------------------------
//           Name: water_bob_fast.as
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

const float k_translation_default_scale = 4.0f;
const float k_rotation_default_scale = 2.0f;
const float k_time_default_scale = 0.2f;
const float k_translation_min_scale = 0.0f;
const float k_translation_max_scale = 5.0f;
const float k_rotation_min_scale = 0.0f;
const float k_rotation_max_scale = 5.0f;
const float k_time_min_scale = 0.0f;
const float k_time_max_scale = 2.0f;

float translation_scale;
float rotation_scale;
float time_scale;

array<int> target_object_ids;
array<vec3> original_translations;
array<quaternion> original_rotations;

bool reload_targets_queued = true;
bool is_editor_enabled = false;
string new_connection_object_id_input;
bool new_connection_object_id_valid = false;
bool new_connection_object_id_input_was_focused = false;

void SetParameters() {
    params.AddFloatSlider("translation_scale", k_translation_default_scale, "min:" + k_translation_min_scale + ",max:" + k_translation_max_scale + ",step:0.001");
    params.AddFloatSlider("rotation_scale", k_rotation_default_scale, "min:" + k_rotation_min_scale + ",max:" + k_rotation_max_scale + ",step:0.001");
    params.AddFloatSlider("time_scale", k_time_default_scale, "min:" + k_time_min_scale + ",max:" + k_time_max_scale + ",step:0.001");

    translation_scale = params.GetFloat("translation_scale");
    rotation_scale = params.GetFloat("rotation_scale");
    time_scale = params.GetFloat("time_scale");

    ReloadTargets();
}

void ReloadTargets() {
    target_object_ids.resize(0);
    original_translations.resize(0);
    original_rotations.resize(0);

    array<int> connected_object_ids = hotspot.GetConnectedObjects();

    if (params.HasParam("Objects")) {
        TokenIterator token_iter;
        token_iter.Init();
        string objects_str = params.GetString("Objects");

        while (token_iter.FindNextToken(objects_str)) {
            int object_id = atoi(token_iter.GetToken(objects_str));
            if (ObjectExists(object_id) && connected_object_ids.find(object_id) == -1) {
                Object@ obj = ReadObjectFromID(object_id);
                hotspot.ConnectTo(obj);
                connected_object_ids.insertLast(object_id);
            }
        }
        params.Remove("Objects");
    }

    for (uint i = 0; i < connected_object_ids.length(); ++i) {
        int object_id = connected_object_ids[i];
        if (!ObjectExists(object_id)) {
            continue;
        }
        Object@ obj = ReadObjectFromID(object_id);
        target_object_ids.insertLast(object_id);

        vec3 orig_translation;
        quaternion orig_rotation;

        if (!params.HasParam("SavedTransform" + object_id)) {
            orig_translation = obj.GetTranslation();
            orig_rotation = obj.GetRotation();
            SaveTransform(object_id, orig_translation, orig_rotation);
        } else {
            LoadSavedTransform(object_id, orig_translation, orig_rotation);
        }

        original_translations.insertLast(orig_translation);
        original_rotations.insertLast(orig_rotation);
    }
}

void SaveTransform(int object_id, const vec3& in translation, const quaternion& in rotation) {
    string transform_str = translation.x + " " + translation.y + " " + translation.z + " "
                         + rotation.x + " " + rotation.y + " " + rotation.z + " " + rotation.w;
    params.AddString("SavedTransform" + object_id, transform_str);
}

void LoadSavedTransform(int object_id, vec3& out translation, quaternion& out rotation) {
    string transform_str = params.GetString("SavedTransform" + object_id);
    TokenIterator token_iter;
    token_iter.Init();

    if (!token_iter.FindNextToken(transform_str)) return;
    translation.x = atof(token_iter.GetToken(transform_str));
    if (!token_iter.FindNextToken(transform_str)) return;
    translation.y = atof(token_iter.GetToken(transform_str));
    if (!token_iter.FindNextToken(transform_str)) return;
    translation.z = atof(token_iter.GetToken(transform_str));
    if (!token_iter.FindNextToken(transform_str)) return;
    rotation.x = atof(token_iter.GetToken(transform_str));
    if (!token_iter.FindNextToken(transform_str)) return;
    rotation.y = atof(token_iter.GetToken(transform_str));
    if (!token_iter.FindNextToken(transform_str)) return;
    rotation.z = atof(token_iter.GetToken(transform_str));
    if (!token_iter.FindNextToken(transform_str)) return;
    rotation.w = atof(token_iter.GetToken(transform_str));
}

void PreDraw(float curr_game_time) {
    if (reload_targets_queued) {
        ReloadTargets();
        reload_targets_queued = false;
    }

    for (uint i = 0; i < target_object_ids.length(); ++i) {
        int object_id = target_object_ids[i];
        if (!ObjectExists(object_id)) {
            continue;
        }
        Object@ obj = ReadObjectFromID(object_id);
        vec3 orig_translation = original_translations[i];
        quaternion orig_rotation = original_rotations[i];

        ApplyBobEffect(obj, orig_translation, orig_rotation, curr_game_time, object_id);
    }
}

void ApplyBobEffect(Object@ obj, const vec3& in orig_translation, const quaternion& in orig_rotation, float curr_game_time, int object_id) {
    float curr_y_translation = (
        sin(curr_game_time * time_scale + object_id) * 0.01f +
        sin(curr_game_time * 2.7f * time_scale + object_id) * 0.015f +
        sin(curr_game_time * 4.3f * time_scale + object_id) * 0.008f
    ) * translation_scale;

    quaternion rotation_x = quaternion(vec4(1, 0, 0, sin(curr_game_time * 3.0f * time_scale + object_id) * 0.01f * rotation_scale));
    quaternion rotation_z = quaternion(vec4(0, 0, 1, sin(curr_game_time * 3.7f * time_scale + object_id) * 0.01f * rotation_scale));
    quaternion new_rotation = rotation_z * rotation_x * orig_rotation;

    vec3 new_position = orig_translation;
    new_position.y += curr_y_translation;
    obj.SetTranslationRotationFast(new_position, new_rotation);
}

void DrawEditor() {
    if (!is_editor_enabled) {
        return;
    }

    bool is_updated = false;
    int hotspot_id = hotspot.GetID();
    Object@ hotspot_obj = ReadObjectFromID(hotspot_id);

    ImGui_PushStyleVar(ImGuiStyleVar_WindowMinSize, vec2(440, 250));
    ImGui_Begin("Water Bob Hotspot - id: " + hotspot_id, is_editor_enabled);

    ImGui_SameLine(ImGui_GetWindowWidth() - 60.0f);
    if (ImGui_SmallButton("Help")) {
        ImGui_SetTooltip(
            "Select hotspot then ALT-click to connect to other objects.\n"
            "Connected objects will bob like they are on top of water.\n"
            "To water-bob groups or prefabs, see manual connect UI below."
        );
    }

    ImGui_Text("Properties:");

    float new_translation_scale = translation_scale;
    if (ImGui_SliderFloat("Translation Scale", new_translation_scale, k_translation_min_scale, k_translation_max_scale)) {
        params.SetFloat("translation_scale", new_translation_scale);
        is_updated = true;
    }

    float new_rotation_scale = rotation_scale;
    if (ImGui_SliderFloat("Rotation Scale", new_rotation_scale, k_rotation_min_scale, k_rotation_max_scale)) {
        params.SetFloat("rotation_scale", new_rotation_scale);
        is_updated = true;
    }

    float new_time_scale = time_scale;
    if (ImGui_SliderFloat("Time Scale", new_time_scale, k_time_min_scale, k_time_max_scale)) {
        params.SetFloat("time_scale", new_time_scale);
        is_updated = true;
    }

    ImGui_NewLine();
    ImGui_Separator();
    ImGui_NewLine();

    ImGui_PushItemWidth(150.0f);
    if (ImGui_InputText("Connect group or prefab with Object Id", new_connection_object_id_input, 7, ImGuiInputTextFlags_CharsDecimal)) {
        int new_object_id = atoi(new_connection_object_id_input);
        new_connection_object_id_valid = IsValidConnection(new_object_id);
    }
    ImGui_PopItemWidth();

    bool connect_triggered = false;
    if (ImGui_Button("Connect")) {
        connect_triggered = true;
    }

    if (connect_triggered && new_connection_object_id_valid) {
        int new_object_id = atoi(new_connection_object_id_input);
        hotspot.ConnectTo(ReadObjectFromID(new_object_id));
        new_connection_object_id_input = "";
        new_connection_object_id_valid = false;
        reload_targets_queued = true;
    }

    if (!new_connection_object_id_valid && new_connection_object_id_input != "") {
        ImGui_TextColored(vec4(1.0f, 0.5f, 0.0f, 1.0f), "Invalid Object ID");
    }

    DisplayConnectedObjects();

    ImGui_End();
    ImGui_PopStyleVar();

    if (is_updated) {
        SetParameters();
    }
}

bool IsValidConnection(int object_id) {
    if (object_id == hotspot.GetID()) {
        return false;
    }
    if (!ObjectExists(object_id)) {
        return false;
    }
    Object@ obj = ReadObjectFromID(object_id);
    return AcceptConnectionsTo(obj);
}

void DisplayConnectedObjects() {
    array<int> connected_object_ids = hotspot.GetConnectedObjects();

    ImGui_NewLine();
    ImGui_Text("Connected Objects:");
    ImGui_Indent();

    for (uint i = 0; i < connected_object_ids.length(); ++i) {
        int object_id = connected_object_ids[i];
        ImGui_Text("Object " + object_id);
        ImGui_SameLine();
        if (ImGui_SmallButton("X###disconnect_" + i)) {
            hotspot.Disconnect(ReadObjectFromID(object_id));
            reload_targets_queued = true;
        }
    }

    if (connected_object_ids.length() == 0) {
        ImGui_Text("None");
    }

    ImGui_Unindent();
}

bool AcceptConnectionsTo(Object@ other) {
    int other_type = other.GetType();
    return other_type == _env_object || other_type == _decal_object || other_type == _hotspot_object ||
           other_type == _group || other_type == _item_object || other_type == _ambient_sound_object ||
           other_type == _dynamic_light_object || other_type == _prefab;
}

void Dispose() {
    for (uint i = 0; i < target_object_ids.length(); ++i) {
        int object_id = target_object_ids[i];
        if (!ObjectExists(object_id)) {
            continue;
        }
        Object@ obj = ReadObjectFromID(object_id);
        ResetToSavedTransform(obj, object_id);
    }
}

void ResetToSavedTransform(Object@ obj, int object_id) {
    vec3 orig_translation;
    quaternion orig_rotation;
    if (params.HasParam("SavedTransform" + object_id)) {
        LoadSavedTransform(object_id, orig_translation, orig_rotation);
        obj.SetTranslationRotationFast(orig_translation, orig_rotation);
    }
}

void LaunchCustomGUI() {
    is_editor_enabled = true;
}

bool ObjectInspectorReadOnly() {
    return true;
}
