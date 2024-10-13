//-----------------------------------------------------------------------------
//           Name: start_dialoguealivedead.as
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

bool has_played = false;

void Init() {
    Reset();
}

void Reset() {
    has_played = false;
}

void SetParameters() {
    params.AddIntCheckbox("Play Once", true);
    params.AddString("Dialogue", "Default text");
}

void Update() {
    if (has_played && params.GetInt("Play Once") == 1) {
        return;
    }
    if (HasAliveAndDeadCharactersInside()) {
        level.SendMessage("start_dialogue \"" + params.GetString("Dialogue") + "\"");
        has_played = true;
    }
}

bool HasAliveAndDeadCharactersInside() {
    bool has_alive = false;
    bool has_dead = false;
    Object@ hotspot_obj = ReadObjectFromID(hotspot.GetID());
    vec3 hotspot_pos = hotspot_obj.GetTranslation();
    vec3 hotspot_scale = hotspot_obj.GetScale();

    int num_chars = GetNumCharacters();
    for (int i = 0; i < num_chars; ++i) {
        MovementObject@ mo = ReadCharacter(i);
        if (IsCharacterInsideHotspot(mo, hotspot_pos, hotspot_scale)) {
            if (mo.GetIntVar("knocked_out") == _awake) {
                has_alive = true;
            } else {
                has_dead = true;
            }
        }
    }
    return has_alive && has_dead;
}

bool IsCharacterInsideHotspot(MovementObject@ mo, const vec3& in hotspot_pos, const vec3& in hotspot_scale) {
    vec3 rel_pos = mo.position - hotspot_pos;
    return abs(rel_pos.x) < hotspot_scale.x * 2.0f &&
           abs(rel_pos.y) < hotspot_scale.y * 2.0f &&
           abs(rel_pos.z) < hotspot_scale.z * 2.0f;
}

void HandleEvent(string event, MovementObject@ mo) {
    // No actions needed on events
}
