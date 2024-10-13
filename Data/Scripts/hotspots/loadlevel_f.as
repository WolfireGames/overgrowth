//-----------------------------------------------------------------------------
//           Name: loadlevel_f.as
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

string default_path = "Data/Levels/levelname.xml";

void Init() {
    // No initialization needed
}

void SetParameters() {
    params.AddString("Level to load", default_path);
}

void HandleEvent(string event, MovementObject@ mo) {
    if (event == "enter") {
        OnEnter(mo);
    }
}

void OnEnter(MovementObject@ mo) {
    if (mo.controlled) {
        return;
    }
    string path = params.GetString("Level to load");
    if (path != default_path) {
        level.SendMessage("loadlevel \"" + path + "\"");
    } else {
        level.SendMessage("displaytext \"Target level not set\"");
    }
}
