//-----------------------------------------------------------------------------
//           Name: ragdoll.as
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
	params.AddString("Recovery time", "5.0");
	params.AddString("Damage dealt", "0.0");
	params.AddString("Upward force", "0.0");
	params.AddString("Ragdoll type", "0");
}

void HandleEvent(string event, MovementObject @mo){
    if(event == "enter"){
        OnEnter(mo);
    }
}

void OnEnter(MovementObject @mo) {
    string ragdollType = "_RGDL_FALL";

    if(params.GetFloat("Ragdoll type") == 1) {
    	ragdollType = "_RGDL_INJURED";
    }
    else if(params.GetFloat("Ragdoll type") == 2) {
    	ragdollType = "_RGDL_LIMP";
    }
    else if(params.GetFloat("Ragdoll type") == 3) {
    	ragdollType = "_RGDL_ANIMATION";
    }
    mo.Execute("DropWeapon(); Ragdoll("+ragdollType+"); HandleRagdollImpactImpulse(vec3(0.0f,"+params.GetFloat("Upward force")+",0.0f), this_mo.rigged_object().GetAvgIKChainPos(\"torso\"), "+params.GetFloat("Damage dealt")+"); roll_recovery_time = "+params.GetFloat("Recovery time")+"; recovery_time = "+params.GetFloat("Recovery time")+";");
}