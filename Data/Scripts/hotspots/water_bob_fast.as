//-----------------------------------------------------------------------------
//           Name: water_bob_fast.as
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

float translation_scale;
const float kTranslationDefaultScale = 4.0;
const int kTranslationMinScale = 0;
const int kTranslationMaxScale = 5;

float rotation_scale;
const float kRotationDefaultScale = 2.0;
const int kRotationMinScale = 0;
const int kRotationMaxScale = 5;

float time_scale;
const float kTimeDefaultScale = 0.2;
const int kTimeMinScale = 0;
const int kTimeMaxScale = 2;

void SetParameters() {
  params.AddFloatSlider("translation_scale", kTranslationDefaultScale, "min:" + kTranslationMinScale + ",max:" + kTranslationMaxScale + ",step:0.001");
  params.AddFloatSlider("rotation_scale", kRotationDefaultScale, "min:" + kRotationMinScale + ",max:" + kRotationMaxScale + ",step:0.001");
  params.AddFloatSlider("time_scale", kTimeDefaultScale, "min:" + kTimeMinScale + ",max:" + kTimeMaxScale + ",step:0.001");

  translation_scale = params.GetFloat("translation_scale");
  rotation_scale = params.GetFloat("rotation_scale");
  time_scale = params.GetFloat("time_scale");

  ReloadTargets();
}

array<int> target_object_ids;
array<vec3> orig_translations;
array<quaternion> orig_rotations;

void ReloadTargets() {
  target_object_ids.resize(0);
  orig_translations.resize(0);
  orig_rotations.resize(0);

  array<int> connected_object_ids = hotspot.GetConnectedObjects();

  // Get saved object ids from Objects param, and convert to connections
  if(params.HasParam("Objects")) {
    TokenIterator object_ids_iter;
    object_ids_iter.Init();

    string str = params.GetString("Objects");

    while(object_ids_iter.FindNextToken(str)) {
      int id = atoi(object_ids_iter.GetToken(str));

      if(ObjectExists(id) && connected_object_ids.find(id) == -1) {
        Object@ obj = ReadObjectFromID(id);
        hotspot.ConnectTo(obj);
        connected_object_ids.insertLast(id);
      }
    }

    params.Remove("Objects");
  }

  int connected_object_count = connected_object_ids.length();

  for(int connected_ids_index = 0; connected_ids_index < connected_object_count; ++connected_ids_index) {
    int id = connected_object_ids[connected_ids_index];

    if(ObjectExists(id)) {
      Object@ obj = ReadObjectFromID(id);
      target_object_ids.insertLast(id);

      vec3 orig_translation;
      quaternion orig_rotation;

      if(!params.HasParam("SavedTransform" + id)) {
        vec3 translation = obj.GetTranslation();
        quaternion quat = obj.GetRotation();
        string transform_str;

        for(int i = 0; i < 3; ++i) {
          transform_str += translation[i] + " ";
        }

        transform_str += quat.x + " ";
        transform_str += quat.y + " ";
        transform_str += quat.z + " ";
        transform_str += quat.w;

        params.AddString("SavedTransform" + id, transform_str);

        orig_translation = translation;
        orig_rotation = quat;
      } else {
        ExtractSavedTransform(id, orig_translation, orig_rotation);
      }

      orig_translations.insertLast(orig_translation);
      orig_rotations.insertLast(orig_rotation);
    }
  }
}

void ExtractSavedTransform(int id, vec3 &inout orig_translation, quaternion &inout orig_rotation) {
  string transform_str = params.GetString("SavedTransform" + id);
  TokenIterator saved_transforms_iter;
  saved_transforms_iter.Init();

  saved_transforms_iter.FindNextToken(transform_str);
  orig_translation[0] = atof(saved_transforms_iter.GetToken(transform_str));
  saved_transforms_iter.FindNextToken(transform_str);
  orig_translation[1] = atof(saved_transforms_iter.GetToken(transform_str));
  saved_transforms_iter.FindNextToken(transform_str);
  orig_translation[2] = atof(saved_transforms_iter.GetToken(transform_str));

  saved_transforms_iter.FindNextToken(transform_str);
  orig_rotation.x = atof(saved_transforms_iter.GetToken(transform_str));
  saved_transforms_iter.FindNextToken(transform_str);
  orig_rotation.y = atof(saved_transforms_iter.GetToken(transform_str));
  saved_transforms_iter.FindNextToken(transform_str);
  orig_rotation.z = atof(saved_transforms_iter.GetToken(transform_str));
  saved_transforms_iter.FindNextToken(transform_str);
  orig_rotation.w = atof(saved_transforms_iter.GetToken(transform_str));
}

bool reload_targets_is_queued = true;

void PreDraw(float curr_game_time) {
  if(reload_targets_is_queued) {
    ReloadTargets();
    reload_targets_is_queued = false;
  }

  int object_id_count = target_object_ids.length();

  for(int i = 0; i < object_id_count; ++i) {
    int id = target_object_ids[i];

    if(ObjectExists(id)) {
      float curr_y_translation = (
          sin(curr_game_time * time_scale + id) * 0.01 +
          sin(curr_game_time * 2.7 * time_scale + id) * 0.015 +
          sin(curr_game_time * 4.3 * time_scale + id) * 0.008) *
        translation_scale;

      quaternion current_rotation;
      current_rotation = quaternion(vec4(1, 0, 0, sin(curr_game_time * 3.0 * time_scale + id) * 0.01 * rotation_scale));
      current_rotation = quaternion(vec4(0, 0, 1, sin(curr_game_time * 3.7 * time_scale + id) * 0.01 * rotation_scale)) * current_rotation;

      Object@ obj = ReadObjectFromID(id);
      vec3 pos = orig_translations[i];
      pos[1] += curr_y_translation;
      obj.SetTranslationRotationFast(pos, orig_rotations[i] * current_rotation);
    }
  }
}

bool is_editor_enabled = false;

void LaunchCustomGUI() {
  is_editor_enabled = true;
}

string new_connection_object_id_input;
bool new_connection_object_id_valid_is_valid = false;
bool new_connection_object_id_input_was_focused = false;

void DrawEditor() {
  bool is_updated = false;

  int hotspot_id = hotspot.GetID();
  Object@ hotspot_obj = ReadObjectFromID(hotspot_id);
  DebugDrawBillboard("Data/UI/spawner/thumbs/Hotspot/water.png",
                     hotspot_obj.GetTranslation() + vec3(0.0, 0.5, 0.0),
                     hotspot_obj.GetScale()[1] * 2.0,
                     vec4(1.0),
                     _delete_on_draw);
  DebugDrawText(hotspot_obj.GetTranslation() + vec3(0.0, 1.0, 0.0), "Water Bob", 1.0f, true, _delete_on_draw);

  if(is_editor_enabled) {
    ImGui_PushStyleVar(ImGuiStyleVar_WindowMinSize, vec2(440, 250));
    ImGui_Begin("Water Bob Hotspot - id: " + hotspot_id, is_editor_enabled);

    ImGui_SameLine(ImGui_GetWindowWidth() - 60.0);
    ImGui_SmallButton("Help");

    if(ImGui_IsItemHovered()) {
      ImGui_SetTooltip(
        "Select hotspot then ALT-click to connect to other objects.\n" +
        "Connected objects will bob like they are on top of water.\n" +
        "To water-bob groups or prefabs, see manual connect UI below.");
    }

    ImGui_Text("Properties:");

    float new_translation_scale = params.HasParam("translation_scale") ? params.GetFloat("translation_scale") : kTranslationDefaultScale;

    if(ImGui_SliderFloat("Translation Scale", new_translation_scale, kTranslationMinScale, kTranslationMaxScale)) {
      params.SetFloat("translation_scale", new_translation_scale);
      is_updated = true;
    }

    float new_rotation_scale = params.HasParam("rotation_scale") ? params.GetFloat("rotation_scale") : kRotationDefaultScale;

    if(ImGui_SliderFloat("Rotation Scale", new_rotation_scale, kRotationMinScale, kRotationMaxScale)) {
      params.SetFloat("rotation_scale", new_rotation_scale);
      is_updated = true;
    }

    float new_time_scale = params.HasParam("time_scale") ? params.GetFloat("time_scale") : kTimeDefaultScale;

    if(ImGui_SliderFloat("Time Scale", new_time_scale, kTimeMinScale, kTimeMaxScale)) {
      params.SetFloat("time_scale", new_time_scale);
      is_updated = true;
    }

    Object@ new_connection_target;
    const int kObjectIdMaxCharacterCount = 6;
    const int kCharacterWidth = 7;
    const int kTextMarginWidth = 4;

    ImGui_NewLine();
    ImGui_Separator();
    ImGui_NewLine();

    ImGui_PushItemWidth(kTextMarginWidth + (kObjectIdMaxCharacterCount * kCharacterWidth) + kTextMarginWidth);
    if(ImGui_InputText("Connect group or prefab with Object Id", new_connection_object_id_input, kObjectIdMaxCharacterCount + 1, ImGuiInputTextFlags_CharsDecimal)) {
      int new_connection_object_id = atoi(new_connection_object_id_input);
      new_connection_object_id_valid_is_valid = new_connection_object_id != hotspot.GetID() && ObjectExists(new_connection_object_id) && AcceptConnectionsTo(ReadObjectFromID(new_connection_object_id));
    }
    ImGui_PopItemWidth();

    bool connect_triggered = false;

    bool object_id_input_is_focused = ImGui_IsItemActive();
    if(!object_id_input_is_focused && new_connection_object_id_input_was_focused && ImGui_IsKeyPressed(ImGui_GetKeyIndex(ImGuiKey_Enter))) {
      // Enter key press detected
      // TODO: Would use ImGuiInputTextFlags_EnterReturnsTrue instead, but the text buffer doesn't get updated currently by the engine, so cannot
      connect_triggered = true;
    }
    new_connection_object_id_input_was_focused = object_id_input_is_focused;

    ImGui_SameLine();
    ImGui_BeginDisabled(!new_connection_object_id_valid_is_valid);
    if(ImGui_Button("Connect")) {
      connect_triggered = true;
    }
    ImGui_EndDisabled();

    if(!new_connection_object_id_valid_is_valid && !new_connection_object_id_input.isEmpty()) {
      int new_connection_object_id = atoi(new_connection_object_id_input);
      vec4 error_color(1.0, 0.5, 0.0, 1.0);

      if(new_connection_object_id == hotspot.GetID()) {
        ImGui_TextColored(error_color, "Connection to self not allowed");
      } else if(ObjectExists(new_connection_object_id)) {
        ImGui_TextColored(error_color, "Connection to that object type not allowed");
      } else {
        ImGui_TextColored(error_color, "No object with specified id");
      }
    }

    if(connect_triggered && new_connection_object_id_valid_is_valid) {
      int new_connection_object_id = atoi(new_connection_object_id_input);
      hotspot.ConnectTo(ReadObjectFromID(new_connection_object_id));
      new_connection_object_id_input = "";
      new_connection_object_id_valid_is_valid = false;
    }

    array<int> connected_object_ids = hotspot.GetConnectedObjects();
    int connected_object_count = connected_object_ids.length();

    ImGui_NewLine();
    ImGui_Text("Connected Objects:");

    ImGui_Indent();
    for(int i = 0; i < connected_object_count; ++i) {
      ImGui_Text("Object " + connected_object_ids[i]);

      ImGui_SameLine();
      if(ImGui_SmallButton("X###disconnect_" + i)) {
        hotspot.Disconnect(ReadObjectFromID(connected_object_ids[i]));
      }
    }
    if(connected_object_count == 0) {
      ImGui_Text("none");
    }
    ImGui_Unindent();

    ImGui_End();
    ImGui_PopStyleVar();  // End ImGuiStyleVar_WindowMinSize
  }

  if(is_updated) {
    SetParameters();
  }
}

bool ObjectInspectorReadOnly() {
  return true;
}

bool AcceptConnectionsTo(Object@ other) {
  return IsAcceptedConnectionType(other);
}

bool ConnectTo(Object@ other) {
  bool result = false;

  if(IsAcceptedConnectionType(other)) {
    ClearSavedTransform(other);

    reload_targets_is_queued = true;
    result = true;
  }

  return result;
}

bool Disconnect(Object@ other) {
  bool result = false;

  if(IsAcceptedConnectionType(other)) {
    ResetToSavedTransform(other);
    ClearSavedTransform(other);

    reload_targets_is_queued = true;
    result = true;
  }

  return result;
}

bool IsAcceptedConnectionType(Object@ other) {
  bool result = false;
  int other_type = other.GetType();

  result =
    other_type == _env_object ||
    other_type == _decal_object ||
    other_type == _hotspot_object ||
    other_type == _group ||
    other_type == _item_object ||
    other_type == _ambient_sound_object ||
    other_type == _dynamic_light_object ||
    other_type == _prefab;

  return result;
}

void ResetToSavedTransform(Object@ target) {
  int target_id = target.GetID();

  if(params.HasParam("SavedTransform" + target_id)) {
    vec3 orig_translation;
    quaternion orig_rotation;

    ExtractSavedTransform(target_id, orig_translation, orig_rotation);
    target.SetTranslationRotationFast(orig_translation, orig_rotation);
  }
}

void ClearSavedTransform(Object@ target) {
  int target_id = target.GetID();

  if(params.HasParam("SavedTransform" + target_id)) {
    params.Remove("SavedTransform" + target_id);
  }
}

void Dispose() {
  int target_object_ids_count = target_object_ids.length();

  for(int i = 0; i < target_object_ids_count; ++i) {
    int id = target_object_ids[i];

    if(ObjectExists(id)) {
      Object@ obj = ReadObjectFromID(id);
      ResetToSavedTransform(obj);
    }
  }
}
