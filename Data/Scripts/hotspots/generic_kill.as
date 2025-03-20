//-----------------------------------------------------------------------------
//           Name: generic_kill.as
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

void Init() {
    // No initialization needed
}

void SetParameters() {
    params.AddIntCheckbox("KillNPC", true);
    params.AddIntCheckbox("KillPlayer", true);
}

void HandleEvent(string event, MovementObject@ mo) {
    if (event == "enter") {
        OnEnter(mo);
    }
}

void OnEnter(MovementObject@ mo) {
    bool kill_player = params.GetInt("KillPlayer") == 1;
    bool kill_npc = params.GetInt("KillNPC") == 1;

    if ((mo.is_player && kill_player) || (!mo.is_player && kill_npc)) {
        KillCharacter(mo);
    }
}

void KillCharacter(MovementObject@ mo) {
    mo.Execute("TakeBloodDamage(1.0f);Ragdoll(_RGDL_FALL);zone_killed=1;");
}
