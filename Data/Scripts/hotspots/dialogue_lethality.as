//-----------------------------------------------------------------------------
//           Name: dialogue_lethality.as
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

bool played = false;

void Reset() {
    played = false;
}

void Init() {
    Reset();
}

void SetParameters() {
    params.AddIntCheckbox("Play Once", true);
    params.AddIntCheckbox("Play Lethal Dialogue", false);
    params.AddIntCheckbox("Play for npcs", false);
    params.AddString("Dialogue", "Test");
    params.AddString("Lethal Dialogue", "Test2");
}

void HandleEvent(string event, MovementObject@ mo) {
    if (event == "enter") {
        OnEnter(mo);
    }
}

void OnEnter(MovementObject@ mo) {
    if (played && params.GetInt("Play Once") != 0) {
        return;
    }
    bool play_for_npcs = params.GetInt("Play for npcs") != 0;
    if (!mo.controlled && !play_for_npcs) {
        return;
    }
    if (mo.GetIntVar("no_kills_") == 1 || params.GetInt("Play Lethal Dialogue") != 0) {
        level.SendMessage("start_dialogue \"" + params.GetString("Lethal Dialogue") + "\"");
    } else {
        level.SendMessage("start_dialogue \"" + params.GetString("Dialogue") + "\"");
    }
    played = true;
}
