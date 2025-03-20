//-----------------------------------------------------------------------------
//           Name: gankarm.as
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

bool played;

void Init() {
    Reset();
}

void Reset() {
    played = false;
}

void SetParameters() {
    params.AddString("Name", "Gunk");
}

void HandleEvent(string event, MovementObject@ mo) {
    if (event == "enter") {
        OnEnter(mo);
        played = true;
    }
}

void OnEnter(MovementObject@ mo) {
    if (!IsCharacterNamedGunk(mo)) {
        return;
    }
    ReduceArmInflation(mo);
}

bool IsCharacterNamedGunk(MovementObject@ mo) {
    Object@ obj = ReadObjectFromID(mo.GetID());
    ScriptParams@ script_params = obj.GetScriptParams();
    return script_params.HasParam("Name") && script_params.GetString("Name") == "Gunk";
}

void ReduceArmInflation(MovementObject@ mo) {
    mo.Execute("SetIKChainElementInflate(\"leftarm\",0,0.0f);");
    mo.Execute("SetIKChainElementInflate(\"leftarm\",1,0.0f);");
    mo.Execute("SetIKChainElementInflate(\"leftarm\",2,0.0f);");
    mo.Execute("SetIKChainElementInflate(\"leftarm\",3,0.0f);");
    mo.Execute("SetIKChainElementInflate(\"lefthand\",0,0.0f);");
    mo.Execute("SetIKChainElementInflate(\"lefthand\",1,0.0f);");
    mo.Execute("SetIKChainElementInflate(\"lefthand\",2,0.0f);");
    mo.Execute("SetIKChainElementInflate(\"lefthand\",-1,0.0f);");
    mo.Execute("SetIKChainElementInflate(\"lefthand\",-2,0.0f);");
    mo.Execute("SetIKChainElementInflate(\"lefthand\",-3,0.0f);");
    mo.Execute("SetIKChainElementInflate(\"leftfingers\",1.0,0.0f);");
    mo.Execute("SetIKChainElementInflate(\"leftfingers\",0.0,0.0f);");
    mo.Execute("SetIKChainElementInflate(\"leftfingers\",-1.0,0.0f);");
    mo.Execute("SetIKChainElementInflate(\"leftthumb\",-1.0,0.0f);");
    mo.Execute("SetIKChainElementInflate(\"leftthumb\",-1.0,0.0f);");
    mo.Execute("SetIKChainElementInflate(\"leftthumb\",-1.0,0.0f);");
    mo.Execute("this_mo.rigged_object().skeleton().SetBoneInflate(6, 0.0f);");
    mo.Execute("this_mo.rigged_object().skeleton().SetBoneInflate(13, 0.0f);");
    mo.Execute("this_mo.rigged_object().skeleton().SetBoneInflate(15, 0.0f);");
    mo.Execute("this_mo.rigged_object().skeleton().SetBoneInflate(14, 0.0f);");
}
