//-----------------------------------------------------------------------------
//           Name: remove_object.as
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
    params.AddString("ObjectNameToDisappear", "Unknown");
}

void HandleEvent(string event, MovementObject@ mo) {
    if (event == "enter") {
        OnEnter();
    }
}

void OnEnter() {
    string target_name = params.GetString("ObjectNameToDisappear");
    array<int>@ object_ids = GetObjectIDs();
    for (uint i = 0; i < object_ids.length(); ++i) {
        Object@ obj = ReadObjectFromID(object_ids[i]);
        ScriptParams@ obj_params = obj.GetScriptParams();
        if (obj_params.HasParam("Name") && obj_params.GetString("Name") == target_name) {
            DeleteObjectID(object_ids[i]);
        }
    }
}
