//-----------------------------------------------------------------------------
//           Name: player_jump_height.as
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

bool is_triggered = false;

void Init() {
    level.ReceiveLevelEvents(hotspot.GetID());
}

void Dispose() {
    level.StopReceivingLevelEvents(hotspot.GetID());
}

void SetParameters() {
    params.AddFloatSlider("Initial Jetpack Fuel", 5.0f, "min:0.0,max:20.0");
    params.AddFloatSlider("Initial Jump Velocity Multiplier", 1.0f, "min:0.0,max:5.0,step:0.1,text_mult:100");
}

void ReceiveMessage(string message) {
    if (message == "level_event achievement_event player_jumped") {
        is_triggered = true;
    }
}

void Update() {
    if (!is_triggered) {
        return;
    }
    is_triggered = false;
    float new_jetpack_fuel = params.GetFloat("Initial Jetpack Fuel");
    float jump_velocity_multiplier = params.GetFloat("Initial Jump Velocity Multiplier");
    ApplyJumpSettings(new_jetpack_fuel, jump_velocity_multiplier);
}

void ApplyJumpSettings(float jetpack_fuel, float velocity_multiplier) {
    for (int i = 0; i < GetNumCharacters(); ++i) {
        MovementObject@ character = ReadCharacter(i);
        if (ReadObjectFromID(character.GetID()).GetPlayer()) {
            character.Execute(
                "jump_info.jetpack_fuel = " + jetpack_fuel + ";" +
                "this_mo.velocity.y *= " + velocity_multiplier + ";");
        }
    }
}
