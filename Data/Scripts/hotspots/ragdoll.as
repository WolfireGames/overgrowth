//-----------------------------------------------------------------------------
//           Name: ragdoll.as
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
    params.AddString("RecoveryTime", "5.0");
    params.AddString("DamageDealt", "0.0");
    params.AddString("UpwardForce", "0.0");
    params.AddString("RagdollType", "0");
}

void HandleEvent(string event, MovementObject@ mo) {
    if (event == "enter") {
        OnEnter(mo);
    }
}

void OnEnter(MovementObject@ mo) {
    string ragdoll_type = GetRagdollType();
    string script = BuildRagdollScript(ragdoll_type);
    mo.Execute(script);
}

string GetRagdollType() {
    float type = params.GetFloat("RagdollType");
    if (type == 1) {
        return "_RGDL_INJURED";
    } else if (type == 2) {
        return "_RGDL_LIMP";
    } else if (type == 3) {
        return "_RGDL_ANIMATION";
    }
    return "_RGDL_FALL";
}

string BuildRagdollScript(const string& in ragdoll_type) {
    float upward_force = params.GetFloat("UpwardForce");
    float damage_dealt = params.GetFloat("DamageDealt");
    float recovery_time = params.GetFloat("RecoveryTime");

    string script =
        "DropWeapon(); "
        "Ragdoll(" + ragdoll_type + "); "
        "HandleRagdollImpactImpulse(vec3(0.0f," + upward_force + ",0.0f), "
        "this_mo.rigged_object().GetAvgIKChainPos(\"torso\"), " + damage_dealt + "); "
        "roll_recovery_time = " + recovery_time + "; "
        "recovery_time = " + recovery_time + ";";

    return script;
}
