//-----------------------------------------------------------------------------
//           Name: activate_characters.as
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

void SetParameters() {
    params.AddString("characters", "");
}

void HandleEvent(string event, MovementObject@ mo) {
    if (event == "enter") {
        OnEnter(mo);
    }
}

void OnEnter(MovementObject@ mo) {
    if (!mo.controlled || !params.HasParam("characters")) {
        return;
    }

    ActivateCharacters();

    if (params.HasParam("music_layer_override")) {
        int override = params.GetInt("music_layer_override");
        level.SendMessage("music_layer_override " + override);
    }
}

void ActivateCharacters() {
    TokenIterator token_iter;
    token_iter.Init();
    string str = params.GetString("characters");

    while (token_iter.FindNextToken(str)) {
        int id = atoi(token_iter.GetToken(str));
        if (ObjectExists(id) && ReadObjectFromID(id).GetType() == _movement_object) {
            ReadCharacterID(id).Execute("this_mo.static_char = false;");
        }
    }
}
