//-----------------------------------------------------------------------------
//           Name: drika_wait_level_message.as
//      Developer:
//         Author: Gyrth, Fason7
//    Script Type: Drika Element
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

class DrikaWaitLevelMessage : DrikaElement {
    string message;
    bool received_message = false;

    DrikaWaitLevelMessage(int _index, string _message) {
        index = _index;
        message = _message;
        drika_element_type = drika_wait_level_message;
        display_color = vec4(110, 94, 180, 255);
        has_settings = true;
    }

    string GetSaveString() {
        return "wait_level_message " + message;
    }

    string GetDisplayString() {
        return "WaitLevelMessage " + message;
    }

    void AddSettings() {
        ImGui_Text("Wait for message:");
        ImGui_InputText("Message", message, 64);
    }

    void ReceiveMessage(string _message) {
        if (_message != message) {
            return;
        }
        Log(info, "Received correct message");
        received_message = true;
    }

    bool Trigger() {
        return received_message;
    }

    void Reset() {
        received_message = false;
    }
}
