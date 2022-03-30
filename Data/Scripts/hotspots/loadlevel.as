//-----------------------------------------------------------------------------
//           Name: loadlevel.as
//      Developer: Wolfire Games LLC
//    Script Type: Hotspot
//    Description:
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
}

string _default_path = "Data/Levels/levelname.xml";

void SetParameters() {
    params.AddString("Level to load", _default_path);
}

void HandleEvent(string event, MovementObject @mo){
    if(event == "enter"){
        OnEnter(mo);
    } else if(event == "exit"){
        OnExit(mo);
    }
}

void OnEnter(MovementObject @mo) {
    if(mo.controlled){
        string path = params.GetString("Level to load");
        if(path != _default_path){
            level.SendMessage("loadlevel \""+path+"\"");
        } else {
            level.SendMessage("displaytext \"Target level not set\"");
        }
    }
}

void OnExit(MovementObject @mo) {
    if(mo.controlled){
        level.SendMessage("cleartext");
    }
}
