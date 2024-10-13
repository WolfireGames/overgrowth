//-----------------------------------------------------------------------------
//           Name: spike_tip.as
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

void HandleEvent(string event, MovementObject@ mo) {
    if (event == "enter") {
        SendMessageToParent("arm_spike");
    } else if (event == "exit") {
        SendMessageToParent("disarm_spike");
    }
}

void SendMessageToParent(const string& in message) {
    if (!params.HasParam("Parent")) {
        return;
    }
    int parent_id = params.GetInt("Parent");
    if (!ObjectExists(parent_id)) {
        return;
    }
    Object@ parent = ReadObjectFromID(parent_id);
    parent.ReceiveScriptMessage(message);
}

void Draw() {
    // No drawing needed
}
