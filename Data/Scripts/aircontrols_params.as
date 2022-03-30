//-----------------------------------------------------------------------------
//           Name: aircontrol_params.as
//      Developer: Wolfire Games LLC
//    Script Type:
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

float p_jump_initial_velocity = 5.0f; // y-axis (up) velocity of jump used in GetJumpVelocity()
float p_jump_air_control = 3.0f; // multiplier for the amount of directional control available while in the air
float p_jump_fuel = 5.0f; // used to set the amount of "fuel" available at the start of a jump (how long you can sustain/increase your jump by holding space)
float p_jump_fuel_burn = 10.0f; // multiplier for amount of fuel used per time_step (how much the jump is sustained/increased while holding space)
