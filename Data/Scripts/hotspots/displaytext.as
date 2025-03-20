//-----------------------------------------------------------------------------
//           Name: displaytext.as
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

void Init() {
    // No initialization needed
}

void SetParameters() {
    params.AddString("Display Text", "Default text");
}

void HandleEvent(string event, MovementObject@ mo) {
    if (event != "enter" && event != "exit" && event != "disengaged_player_control" && event != "engaged_player_control") {
        return;
    }

    if (event == "exit") {
        level.SendMessage("cleartext");
        return;
    }

    if (!IsPlayerInHotspot()) {
        level.SendMessage("cleartext");
        return;
    }

    level.SendMessage("displaytext \"" + params.GetString("Display Text") + "\"");
}

bool IsPlayerInHotspot() {
    array<int> collides_with;
    level.GetCollidingObjects(hotspot.GetID(), collides_with);

    for (uint i = 0; i < collides_with.length(); ++i) {
        if (!ObjectExists(collides_with[i])) {
            continue;
        }
        Object@ obj = ReadObjectFromID(collides_with[i]);
        if (obj.GetType() != _movement_object) {
            continue;
        }
        MovementObject@ mo1 = ReadCharacterID(collides_with[i]);
        if (mo1.controlled) {
            return true;
        }
    }
    return false;
}
