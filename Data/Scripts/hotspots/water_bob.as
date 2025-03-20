//-----------------------------------------------------------------------------
//           Name: water_bob.as
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

float translation_scale;
float rotation_scale;
float time_scale;

array<int> object_ids;
array<vec3> original_translations;
array<quaternion> original_rotations;

void SetParameters() {
    params.AddString("Objects", "");
    params.AddFloatSlider("translation_scale", 4.0f, "min:0,max:5,step:0.001");
    params.AddFloatSlider("rotation_scale", 2.0f, "min:0,max:5,step:0.001");
    params.AddFloatSlider("time_scale", 0.2f, "min:0,max:2,step:0.001");

    translation_scale = params.GetFloat("translation_scale");
    rotation_scale = params.GetFloat("rotation_scale");
    time_scale = params.GetFloat("time_scale");

    InitializeObjects();
}

void InitializeObjects() {
    object_ids.resize(0);
    original_translations.resize(0);
    original_rotations.resize(0);

    TokenIterator token_iter;
    token_iter.Init();
    string objects_str = params.GetString("Objects");

    while (token_iter.FindNextToken(objects_str)) {
        int object_id = atoi(token_iter.GetToken(objects_str));
        if (!ObjectExists(object_id)) {
            continue;
        }
        Object@ obj = ReadObjectFromID(object_id);
        object_ids.insertLast(object_id);
        original_translations.insertLast(obj.GetTranslation());
        original_rotations.insertLast(obj.GetRotation());
    }
}

void Update() {
    for (uint i = 0; i < object_ids.length(); ++i) {
        int object_id = object_ids[i];
        if (!ObjectExists(object_id)) {
            continue;
        }
        Object@ obj = ReadObjectFromID(object_id);
        vec3 original_translation = original_translations[i];
        quaternion original_rotation = original_rotations[i];

        ApplyBobEffect(obj, original_translation, original_rotation);
    }
}

void ApplyBobEffect(Object@ obj, const vec3& in original_translation, const quaternion& in original_rotation) {
    float current_time = the_time * time_scale;
    float bob_offset = (
        sin(current_time) * 0.01f +
        sin(current_time * 2.7f) * 0.015f +
        sin(current_time * 4.3f) * 0.008f
    ) * translation_scale;
    vec3 new_position = original_translation;
    new_position.y += bob_offset;
    obj.SetTranslation(new_position);

    quaternion rotation_x = quaternion(vec4(1, 0, 0, sin(current_time * 3.0f) * 0.01f * rotation_scale));
    quaternion rotation_z = quaternion(vec4(0, 0, 1, sin(current_time * 3.7f) * 0.01f * rotation_scale));
    quaternion new_rotation = rotation_z * rotation_x * original_rotation;
    obj.SetRotation(new_rotation);
}
