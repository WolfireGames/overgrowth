//-----------------------------------------------------------------------------
//           Name: adv_dialogue_kocheck.as
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

bool played;

void Reset() {
    played = false;
}

void Init() {
    Reset();
}

void SetParameters() {
    params.AddIntCheckbox("Play Once", true);
    params.AddIntCheckbox("Play Only If Dead", false);
    params.AddIntCheckbox("Play for NPCs", false);
    params.AddIntCheckbox("Play If No Combat", true);
    params.AddString("All Enemies Neutralized", "Default text");
    params.AddString("Some Enemies Neutralized", "Default text");
    params.AddString("No Enemies Neutralized", "Default text");
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

    PlayDialogue(mo);
    played = true;
}

bool ShouldPlayDialogue(MovementObject@ mo) {
    if (params.GetInt("Play Only If Dead") != 0 && mo.GetIntVar("knocked_out") <= 0) {
        return false;
    }
    if (played && params.GetInt("Play Once") != 0) {
        return false;
    }
    if (!mo.controlled && params.GetInt("Play for NPCs") == 0) {
        return false;
    }
    if (params.GetInt("Play If No Combat") != 0 && mo.QueryIntFunction("int CombatSong()") != 0) {
        return false;
    }
    return true;
}

void PlayDialogue(MovementObject@ mo) {
    int num_chars = GetNumCharacters();
    bool all_conscious = true;
    bool all_neutralized = true;

    for (int i = 0; i < num_chars; ++i) {
        MovementObject@ char = ReadCharacter(i);
        if (!char.controlled && !mo.OnSameTeam(char)) {
            if (char.GetIntVar("knocked_out") > 0) {
                all_conscious = false;
            } else {
                all_neutralized = false;
            }
        }
    }

    if (all_conscious) {
        level.SendMessage("start_dialogue \"" + params.GetString("No Enemies Neutralized") + "\"");
    } else if (all_neutralized) {
        level.SendMessage("start_dialogue \"" + params.GetString("All Enemies Neutralized") + "\"");
    } else {
        level.SendMessage("start_dialogue \"" + params.GetString("Some Enemies Neutralized") + "\"");
    }
}
