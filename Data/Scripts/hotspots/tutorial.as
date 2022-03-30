//-----------------------------------------------------------------------------
//           Name: tutorial.as
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

void SetParameters() {
    params.AddString("Type", "Default text");
}

void SetEnabled(bool val){
    array<int> ids;
    level.GetCollidingObjects(hotspot.GetID(), ids);
    for(int i=0, len=ids.size(); i<len; ++i){
        int id = ids[i];
        if(ObjectExists(id)){
            Object@ obj = ReadObjectFromID(id);
            if(obj.GetType() == _movement_object){
                MovementObject@ mo = ReadCharacterID(id);
                if(mo.controlled){
                    if(val){
                        if( GetConfigValueBool("tutorials") ) {
                            mo.ReceiveMessage("tutorial "+params.GetString("Type")+" enter");
                        }
                    } else {
                        mo.ReceiveMessage("tutorial "+params.GetString("Type")+" exit");
                    }
                }
            }
        }
    }
}

void HandleEvent(string event, MovementObject @mo) {
    if( event == "enter" 
        || event == "exit" 
        || event == "disengaged_player_control"  
        || event == "engaged_player_control" ) 
    {
        if(mo.controlled && ReadObjectFromID(hotspot.GetID()).GetEnabled()){
            if(event == "enter" || event == "engaged_player_control"){
                SetEnabled(true);
            }
            if(event == "exit" || event == "disengaged_player_control"){
                mo.ReceiveMessage("tutorial "+params.GetString("Type")+" exit");
            }
        }
    }
}
