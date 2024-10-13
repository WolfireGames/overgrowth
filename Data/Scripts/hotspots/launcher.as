//-----------------------------------------------------------------------------
//           Name: launcher.as
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

// Credit to Steelraven7.

void Init() {
    // No initialization needed
}

void SetParameters() {
    params.AddFloat("Velocity x", 0.0f);
    params.AddFloat("Velocity y (up)", 0.0f);
    params.AddFloat("Velocity z", 0.0f);
    params.AddInt("Trigger on entry", 1);
    params.AddInt("Trigger on exit", 0);
}

void HandleEvent(string event, MovementObject@ mo) {
    if (event == "enter" && params.GetInt("Trigger on entry") != 0) {
        Launch(mo);
    } else if (event == "exit" && params.GetInt("Trigger on exit") != 0) {
        Launch(mo);
    }
}

void Launch(MovementObject@ mo) {
    if (mo.GetIntVar("state") == 4) {
        return;
    }
    mo.velocity.x = params.GetFloat("Velocity x");
    mo.velocity.y = params.GetFloat("Velocity y (up)");
    mo.velocity.z = params.GetFloat("Velocity z");
    mo.Execute("SetOnGround(false); pre_jump = false;");
}
