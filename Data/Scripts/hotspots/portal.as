//-----------------------------------------------------------------------------
//           Name: portal.as
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

void SetParameters() {
    params.AddString("Level to load", "Data/Levels/levelname.xml");
    params.AddInt("light_id", -1);
    params.AddInt("spawn_point", -1);

    int spawn_point_id = params.GetInt("spawn_point");
    if (spawn_point_id != -1 && ObjectExists(spawn_point_id)) {
        Object@ obj = ReadObjectFromID(spawn_point_id);
        if (obj.GetType() == _placeholder_object) {
            PlaceholderObject@ placeholder = cast<PlaceholderObject@>(obj);
            placeholder.SetPreview("Data/Objects/IGF_Characters/IGF_Guard.xml");
        }
    }
}

void HandleEvent(string event, MovementObject@ mo) {
    if (event == "enter") {
        OnEnter(mo);
    }
}

void OnEnter(MovementObject@ mo) {
    if (!mo.controlled) {
        return;
    }
    string path = params.GetString("Level to load");
    if (path != "Data/Levels/levelname.xml") {
        level.SendMessage("loadlevel \"" + path + "\"");
    } else {
        level.SendMessage("displaytext \"Target level not set\"");
    }
}

void Update() {
    int light_id = params.GetInt("light_id");
    if (light_id == -1 || !ObjectExists(light_id)) {
        return;
    }
    Object@ obj = ReadObjectFromID(light_id);
    if (obj.GetType() != _dynamic_light_object) {
        return;
    }
    UpdateLightBrightness(obj);
}

void UpdateLightBrightness(Object@ obj) {
    array<int> character_ids;
    GetCharactersInSphere(obj.GetTranslation(), 10.0f, character_ids);
    float brightness = 0.0f;
    for (uint i = 0; i < character_ids.length(); ++i) {
        float dist = distance(obj.GetTranslation(), ReadCharacterID(character_ids[i]).position);
        brightness = max(brightness, 10.0f - dist);
    }
    float pulse = sin(the_time * 23.0f) + sin(the_time * 5.0f) + sin(the_time * 17.0f);
    pulse = pulse / 3.0f / 2.0f + 0.5f;
    pulse = mix(pulse, 1.0f, 0.8f);
    obj.SetTint(vec3(brightness * 5.0f * pulse, 0.0f, 0.0f));
}
