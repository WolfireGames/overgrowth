//-----------------------------------------------------------------------------
//           Name: launcher.as
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
// Credit to Steelraven7.

void SetParameters() {
	params.AddString("Velocity x", "0.0");
	params.AddString("Velocity y (up)", "0.0");
	params.AddString("Velocity z", "0.0");
	params.AddString("Trigger on entry", "1");
	params.AddString("Trigger on exit", "0");
}

void HandleEvent(string event, MovementObject @mo){
    if(event == "enter" && params.GetString("Trigger on entry") != "0") {
        Launch(mo);
    }
    else if(event == "exit" && params.GetString("Trigger on exit") != "0"){
        Launch(mo);
    }
}

void Launch(MovementObject @mo) {

	//If player is ragdollized, don't launch since this way of launching ragdolls may cause problems.
	if(mo.GetIntVar("state") == 4) return;

	mo.velocity.x = params.GetFloat("Velocity x");
	mo.velocity.y = params.GetFloat("Velocity y (up)");
	mo.velocity.z = params.GetFloat("Velocity z");
    mo.Execute("SetOnGround(false);");
    mo.Execute("pre_jump = false;");
}