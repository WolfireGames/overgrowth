//-----------------------------------------------------------------------------
//           Name: dialogue_new.as
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
    params.AddIntCheckbox("Play only If dead", false);
    params.AddIntCheckbox("Play for npcs", false);
    params.AddString("Dialogue", "Default text");
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
    level.SendMessage("start_dialogue \"" + params.GetString("Dialogue") + "\"");
    played = true;
}

bool ShouldPlayDialogue(MovementObject@ mo) {
    if (params.GetInt("Play only If dead") != 0 && mo.GetIntVar("knocked_out") <= 0) {
        return false;
    }
    if (played && params.GetInt("Play Once") != 0) {
        return false;
    }
    bool play_for_npcs = params.GetInt("Play for npcs") != 0;
    if (!mo.controlled && !play_for_npcs) {
        return false;
    }
    return true;
}
