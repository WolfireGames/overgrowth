//-----------------------------------------------------------------------------
//           Name: dialogue_inspect.as
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
    played = params.HasParam("Start Disabled") ? true : false;
}

void Init() {
    Reset();
}

void SetParameters() {
    params.AddString("Dialogue", "Default text");
    params.AddIntCheckbox("Automatic", false);
    params.AddIntCheckbox("Visible in game", false);
}

void ReceiveMessage(string msg) {
    if (msg == "player_pressed_attack") {
        TryToPlayDialogue();
    } else if (msg == "reset") {
        Reset();
    } else if (msg == "activate") {
        ActivateIfNecessary();
    }
}

void ActivateIfNecessary() {
    if (!played) {
        return;
    }
    played = false;
    array<int> collides_with;
    level.GetCollidingObjects(hotspot.GetID(), collides_with);
    for (int i = 0, len = collides_with.size(); i < len; ++i) {
        int id = collides_with[i];
        if (IsControlledMovementObject(id) && params.GetInt("Automatic") == 1) {
            TryToPlayDialogue();
        }
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
    if (played || !IsPlayerInValidState()) {
        return;
    }
    level.SendMessage("start_dialogue \"" + params.GetString("Dialogue") + "\"");
    played = true;
}

bool IsPlayerInValidState() {
    for (int i = 0, len = GetNumCharacters(); i < len; ++i) {
        MovementObject@ mo = ReadCharacter(i);
        if (mo.controlled && mo.QueryIntFunction("int CanPlayDialogue()") == 1) {
            return true;
        }
    }
    return false;
}

bool IsControlledMovementObject(int id) {
    if (!ObjectExists(id)) {
        return false;
    }
    Object@ obj = ReadObjectFromID(id);
    return obj.GetType() == _movement_object && ReadCharacterID(id).controlled;
}

void PreDraw(float curr_game_time) {
    if (played) {
        return;
    }
    array<int> collides_with;
    level.GetCollidingObjects(hotspot.GetID(), collides_with);
    for (int i = 0, len = collides_with.size(); i < len; ++i) {
        int id = collides_with[i];
        if (IsControlledMovementObject(id) && level.QueryIntFunction("int HasCameraControl()") == 0) {
            MovementObject@ mo = ReadCharacterID(id);
            mo.Execute("note_request_time = time;");
        }
    }
}

void Draw() {
    if (params.GetInt("Visible in game") == 1 || EditorModeActive()) {
        DrawHotspotIcon();
    }
    if (!played && level.QueryIntFunction("int HasCameraControl()") == 0) {
        DrawExclamationOrQuestionMarks();
    }
}

void DrawHotspotIcon() {
    Object@ obj = ReadObjectFromID(hotspot.GetID());
    DebugDrawBillboard("Data/UI/spawner/thumbs/Hotspot/sign_icon.png",
                       obj.GetTranslation() + obj.GetScale()[1] * vec3(0.0f, 0.5f, 0.0f),
                       2.0f,
                       vec4(1.0f),
                       _delete_on_draw);
}

void DrawExclamationOrQuestionMarks() {
    if (params.HasParam("Exclamation Character")) {
        DrawIconAboveCharacter(params.GetInt("Exclamation Character"), "Data/Textures/ui/stealth_debug/exclamation.tga", vec3(0.0, 1.6, 0.0));
    }
    if (params.HasParam("Question Character")) {
        vec3 offset = params.HasParam("Offset") ? vec3(0.4, 0.0, -0.4) : vec3(0.0);
        DrawIconAboveCharacter(params.GetInt("Question Character"), "Data/Textures/ui/stealth_debug/question.tga", vec3(0.0, 1.6, 0.0) + offset);
    }
}

void DrawIconAboveCharacter(int id, string icon_path, vec3 offset) {
    if (!ObjectExists(id) || ReadObjectFromID(id).GetType() != _movement_object) {
        return;
    }
    vec3 position = ReadCharacterID(id).position + offset;
    DebugDrawBillboard(icon_path, position, 1.0f, vec4(1.0f), _delete_on_draw);
}
