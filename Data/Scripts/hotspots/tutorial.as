//-----------------------------------------------------------------------------
//           Name: tutorial.as
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

void SetParameters() {
    params.AddString("Type", "Default text");
}

void HandleEvent(string event, MovementObject@ mo) {
    if (!mo.controlled || !ReadObjectFromID(hotspot.GetID()).GetEnabled()) {
        return;
    }

    if (event == "enter" || event == "engaged_player_control") {
        SetEnabled(true);
    } else if (event == "exit" || event == "disengaged_player_control") {
        mo.ReceiveMessage("tutorial " + params.GetString("Type") + " exit");
    }
}

void SetEnabled(bool enabled) {
    array<int>@ ids = array<int>();
    level.GetCollidingObjects(hotspot.GetID(), ids);
    for (uint i = 0; i < ids.length(); ++i) {
        int id = ids[i];
        if (!ObjectExists(id)) {
            continue;
        }
        Object@ obj = ReadObjectFromID(id);
        if (obj.GetType() != _movement_object) {
            continue;
        }
        MovementObject@ mo = ReadCharacterID(id);
        if (!mo.controlled) {
            continue;
        }
        if (enabled) {
            if (GetConfigValueBool("tutorials")) {
                mo.ReceiveMessage("tutorial " + params.GetString("Type") + " enter");
            }
        } else {
            mo.ReceiveMessage("tutorial " + params.GetString("Type") + " exit");
        }
    }
}
