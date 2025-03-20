//-----------------------------------------------------------------------------
//           Name: teammate_alive.as
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
    params.AddIntCheckbox("Play Only If Dead", false);
    params.AddIntCheckbox("Play for NPCs", false);
    params.AddIntCheckbox("Play If No Combat", true);
    params.AddString("All Friends Neutralized", "Default text");
    params.AddString("Some Friends Neutralized", "Default text");
    params.AddString("No Friends Neutralized", "Default text");
}

void HandleEvent(string event, MovementObject@ mo) {
    if (event == "enter") {
        OnEnter(mo);
    }
}

void OnEnter(MovementObject@ mo) {
    if (!ShouldPlayDialogue(mo)) {
        return;
    }
    DetermineAndPlayDialogue(mo);
    has_played = true;
}

bool ShouldPlayDialogue(MovementObject@ mo) {
    if (has_played && params.GetInt("Play Once") == 1) {
        return false;
    }
    if (params.GetInt("Play Only If Dead") == 1 && mo.GetIntVar("knocked_out") == _awake) {
        return false;
    }
    if (!mo.controlled && params.GetInt("Play for NPCs") == 0) {
        return false;
    }
    if (params.GetInt("Play If No Combat") == 1 && mo.QueryIntFunction("int CombatSong()") != 0) {
        return false;
    }
    return true;
}

void DetermineAndPlayDialogue(MovementObject@ mo) {
    bool everyone_alive = true;
    bool everyone_dead = true;
    int num_chars = GetNumCharacters();

    for (int i = 0; i < num_chars; ++i) {
        MovementObject@ character = ReadCharacter(i);
        if (character.controlled || !mo.OnSameTeam(character)) {
            continue;
        }
        if (character.GetIntVar("knocked_out") == _awake) {
            everyone_dead = false;
        } else {
            everyone_alive = false;
        }
    }

    if (everyone_alive) {
        level.SendMessage("start_dialogue \"" + params.GetString("No Friends Neutralized") + "\"");
    } else if (everyone_dead) {
        level.SendMessage("start_dialogue \"" + params.GetString("All Friends Neutralized") + "\"");
    } else {
        level.SendMessage("start_dialogue \"" + params.GetString("Some Friends Neutralized") + "\"");
    }
}
