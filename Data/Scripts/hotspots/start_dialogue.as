//-----------------------------------------------------------------------------
//           Name: start_dialogue.as
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
float visible_amount = 0.0f;
float last_game_time = 0.0f;

void SetParameters() {
    params.AddString("Dialogue", "Default text");
    params.AddIntCheckbox("Automatic", true);
    params.AddIntCheckbox("Fade", true);
    params.AddIntCheckbox("VisibleInGame", true);
    params.AddString("Color", "1.0 1.0 1.0");
}

void Init() {
    Reset();
}

void Reset() {
    has_played = params.HasParam("Start Disabled");
}

void ReceiveMessage(string msg) {
    if (msg == "player_pressed_attack") {
        TryToPlayDialogue();
    } else if (msg == "reset") {
        Reset();
    } else if (msg == "activate") {
        if (has_played) {
            has_played = false;
            CheckForAutomaticDialogue();
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
    if (has_played) {
        return;
    }
    if (!IsPlayerInValidState()) {
        return;
    }
    string dialogue = params.GetString("Dialogue");
    if (params.GetInt("Fade") == 1) {
        level.SendMessage("start_dialogue_fade \"" + dialogue + "\"");
    } else {
        level.SendMessage("start_dialogue \"" + dialogue + "\"");
    }
    has_played = true;
}

bool IsPlayerInValidState() {
    int num_characters = GetNumCharacters();
    for (int i = 0; i < num_characters; ++i) {
        MovementObject@ mo = ReadCharacter(i);
        if (mo.controlled && mo.QueryIntFunction("int CanPlayDialogue()") == 1) {
            return true;
        }
    }
    return false;
}

void PreDraw(float curr_game_time) {
    UpdateVisibility(curr_game_time);
}

void UpdateVisibility(float curr_game_time) {
    const float fade_speed = 2.0f;
    float delta_time = (curr_game_time - last_game_time) * fade_speed;
    if (!has_played && level.QueryIntFunction("int HasCameraControl()") == 0) {
        visible_amount = min(visible_amount + delta_time, 1.0f);
    } else {
        visible_amount = max(visible_amount - delta_time, 0.0f);
    }
    last_game_time = curr_game_time;
}

void Draw() {
    if (params.GetInt("VisibleInGame") == 1 || EditorModeActive()) {
        DrawIcon();
    }
    if (visible_amount > 0.0f) {
        DrawExclamationOrQuestionMarks();
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

void DrawExclamationOrQuestionMarks() {
    vec3 color = GetColorFromParams();
    vec3 offset = params.HasParam("Offset") ? vec3(0.4f, 0.0f, -0.4f) : vec3(0.0f);
    float animation_offset = sin(the_time * 3.0f) * 0.03f;
    float scale = 1.0f + sin(the_time * 3.0f) * 0.05f;

    DrawIconsForCharacters("Exclamation Character", "Data/Textures/ui/stealth_debug/exclamation_themed.png", color, offset, animation_offset, scale);
    DrawIconsForCharacters("Question Character", "Data/Textures/ui/stealth_debug/question_themed.png", color, offset, animation_offset, scale);
}

void DrawIconsForCharacters(const string& in param_name, const string& in icon_path, const vec3& in color, const vec3& in offset, float animation_offset, float scale) {
    if (!params.HasParam(param_name)) {
        return;
    }
    TokenIterator token_iter;
    token_iter.Init();
    string str = params.GetString(param_name);
    while (token_iter.FindNextToken(str)) {
        int id = atoi(token_iter.GetToken(str));
        if (ObjectExists(id) && ReadObjectFromID(id).GetType() == _movement_object) {
            vec3 position = ReadCharacterID(id).position + vec3(0, 1.6f + animation_offset, 0) + offset;
            DebugDrawBillboard(icon_path, position, scale, vec4(color, visible_amount), _delete_on_draw);
        }
    }
}

vec3 GetColorFromParams() {
    vec3 color(1.0f);
    if (!params.HasParam("Color")) {
        return color;
    }
    TokenIterator token_iter;
    token_iter.Init();
    string str = params.GetString("Color");
    if (token_iter.FindNextToken(str)) {
        color.x = atof(token_iter.GetToken(str));
        if (token_iter.FindNextToken(str)) {
            color.y = atof(token_iter.GetToken(str));
            if (token_iter.FindNextToken(str)) {
                color.z = atof(token_iter.GetToken(str));
            }
        }
    }
    return color;
}

void CheckForAutomaticDialogue() {
    array<int> colliding_objects;
    level.GetCollidingObjects(hotspot.GetID(), colliding_objects);
    for (uint i = 0; i < colliding_objects.length(); ++i) {
        int id = colliding_objects[i];
        if (ObjectExists(id) && ReadObjectFromID(id).GetType() == _movement_object) {
            MovementObject@ mo = ReadCharacterID(id);
            if (mo.controlled && params.GetInt("Automatic") == 1) {
                TryToPlayDialogue();
            }
        }
    }
}
