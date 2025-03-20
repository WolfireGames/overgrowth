//-----------------------------------------------------------------------------
//           Name: dark_world_trigger.as
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
    params.AddInt("dark_world_level_id", -1);
}

void HandleEvent(string event, MovementObject@ mo) {
    if (event == "enter") {
        OnEnter(mo);
    }
}

void OnEnter(MovementObject@ mo) {
    if (!mo.controlled) {
        return;
    }
    int dark_world_level_id = params.GetInt("dark_world_level_id");
    if (!ObjectExists(dark_world_level_id)) {
        return;
    }
    Object@ obj = ReadObjectFromID(dark_world_level_id);
    obj.ReceiveScriptMessage("trigger_enter");
}

void Update() {
    // No update logic needed
}
