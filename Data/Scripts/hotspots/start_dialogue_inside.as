//-----------------------------------------------------------------------------
//           Name: start_dialogue_inside.as
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
bool is_player_inside = false;
array<int> characters_inside;

void Init() {
    Reset();
}

void Reset() {
    has_played = false;
    characters_inside.resize(0);
    CollectCharactersInside();
}

void SetParameters() {
    params.AddIntCheckbox("Play Once", true);
    params.AddIntCheckbox("Player Must Be Inside", true);
    params.AddString("Dialogue", "Default text");
}

void HandleEvent(string event, MovementObject@ mo) {
    if (event == "enter") {
        OnEnter(mo);
    } else if (event == "exit") {
        OnExit(mo);
    }
}

void OnEnter(MovementObject@ mo) {
    if (mo.controlled) {
        is_player_inside = true;
    }
}

void OnExit(MovementObject@ mo) {
    if (mo.controlled) {
        is_player_inside = false;
    }
}

void Update() {
    if (!ShouldCheckDialogueTrigger()) {
        return;
    }
    if (AllCharactersInsideAreDead() && (!has_played || params.GetInt("Play Once") == 0)) {
        PlayDialogue();
    }
}

void CollectCharactersInside() {
    Object@ hotspot_obj = ReadObjectFromID(hotspot.GetID());
    vec3 hotspot_pos = hotspot_obj.GetTranslation();
    vec3 hotspot_scale = hotspot_obj.GetScale();

    int num_chars = GetNumCharacters();
    for (int i = 0; i < num_chars; ++i) {
        MovementObject@ character = ReadCharacter(i);
        if (!character.controlled && IsCharacterInsideHotspot(character, hotspot_pos, hotspot_scale)) {
            characters_inside.insertLast(character.GetID());
        }
    }
}

bool IsCharacterInsideHotspot(MovementObject@ character, const vec3& in hotspot_pos, const vec3& in hotspot_scale) {
    vec3 rel_pos = character.position - hotspot_pos;
    return abs(rel_pos.x) < hotspot_scale.x * 2.0f &&
           abs(rel_pos.y) < hotspot_scale.y * 2.0f &&
           abs(rel_pos.z) < hotspot_scale.z * 2.0f;
}

bool ShouldCheckDialogueTrigger() {
    return is_player_inside || params.GetInt("Player Must Be Inside") == 0;
}

bool AllCharactersInsideAreDead() {
    for (uint i = 0; i < characters_inside.length(); ++i) {
        MovementObject@ character = ReadCharacterID(characters_inside[i]);
        if (character.GetIntVar("knocked_out") == _awake) {
            return false;
        }
    }
    return true;
}

void PlayDialogue() {
    level.SendMessage("start_dialogue \"" + params.GetString("Dialogue") + "\"");
    has_played = true;
}
