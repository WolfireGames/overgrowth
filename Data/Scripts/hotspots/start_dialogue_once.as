//-----------------------------------------------------------------------------
//           Name: start_dialogue_once.as
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
    params.AddString("Dialogue", "Default text");
    params.AddIntCheckbox("Automatic", true);
    params.AddIntCheckbox("Visible in game", true);
}

void ReceiveMessage(string msg) {
    if (msg == "player_pressed_attack") {
        TryToPlayDialogue();
    } else if (msg == "reset") {
        Reset();
    }
}

void HandleEvent(string event, MovementObject@ mo) {
    if (event == "enter") {
        OnEnter(mo);
    }
}

void OnEnter(MovementObject@ mo) {
    if (mo.controlled && params.GetInt("Automatic") == 1) {
        TryToPlayDialogue();
    }
}

void TryToPlayDialogue() {
    if (has_played || !IsPlayerInValidState()) {
        return;
    }
    level.SendMessage("start_dialogue \"" + params.GetString("Dialogue") + "\"");
    has_played = true;
}

bool IsPlayerInValidState() {
    int num_chars = GetNumCharacters();
    for (int i = 0; i < num_chars; ++i) {
        MovementObject@ mo = ReadCharacter(i);
        if (mo.controlled && mo.QueryIntFunction("int CanPlayDialogue()") == 1) {
            return true;
        }
    }
    return false;
}

void Draw() {
    if (params.GetInt("Visible in game") == 1 || EditorModeActive()) {
        DrawIcon();
    }
}

void DrawIcon() {
    Object@ obj = ReadObjectFromID(hotspot.GetID());
    vec3 position = obj.GetTranslation() + obj.GetScale().y * vec3(0.0f, 0.5f, 0.0f);
    DebugDrawBillboard(
        "Data/UI/spawner/thumbs/Hotspot/sign_icon.png",
        position,
        2.0f,
        vec4(1.0f),
        _delete_on_draw
    );
}
