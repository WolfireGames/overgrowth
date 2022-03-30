//-----------------------------------------------------------------------------
//           Name: joystick.cpp
//      Developer: Wolfire Games LLC
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
#include "joystick.h"

#include <Logging/logdata.h>

#include <sstream>
#include <map>
#include <algorithm>
#include <cstdlib>
#include <cmath>

Joystick::Joystick(float look_sensitivity) :
    look_sensitivity_(look_sensitivity),
    deadzone(0.1f),
    button_input_buffer_count(0)
{
}

bool Joystick::HandleInputChange( ControllerInput::Input input, float val, uint32_t sequence_id ) {
    float depth = val;
    std::ostringstream oss;
    switch(input) {
        case ControllerInput::L_STICK_XN:
        case ControllerInput::L_STICK_XP:
        case ControllerInput::L_STICK_YN:
        case ControllerInput::L_STICK_YP:
        case ControllerInput::R_STICK_XN:
        case ControllerInput::R_STICK_XP:
        case ControllerInput::R_STICK_YN:
        case ControllerInput::R_STICK_YP:
        case ControllerInput::L_TRIGGER:
        case ControllerInput::R_TRIGGER:
            depth = NormalizeJoystick(std::fabs(val));
            break;
        default:
            break;
    }

    bool ret_val = false;
    if(depth > 0.0f) {
        AddInputBufferItem(input, depth, sequence_id);
        ret_val = true;
    }

    std::pair<BindingMap::iterator, BindingMap::iterator> iter_pair = binding_.equal_range(input);
    if(iter_pair.first != binding_.end()) {
        for(BindingMap::iterator iter = iter_pair.first; iter != iter_pair.second; ++iter) {
            SetButtonDown(iter->second, depth); 
        }
    }

    return ret_val;
}

void Joystick::ProcessBinding( ControllerInput::Input input, const std::string command) {
    binding_.insert(std::pair<ControllerInput::Input, std::string>(input, command));
}

float Joystick::GetButtonDown( const std::string &name ) const {
    ButtonMap::const_iterator iter(buttons_down_.find(name));
    if(iter != buttons_down_.end()){
        return iter->second;
    } else {
        return false;
    }
}

void Joystick::SetButtonDown( const std::string &name, float depth ) {
    ButtonMap::iterator iter(buttons_down_.find(name));
    if(iter != buttons_down_.end()){
        iter->second = depth;
    } else {
        buttons_down_.insert(std::pair<std::string, bool>(name, depth));
    }
}

void Joystick::ClearBinding() {
    binding_.clear();
}

void Joystick::AddInputBufferItem(ControllerInput::Input input, float depth, uint32_t sequence_id)
{
    if( button_input_buffer_count >= button_input_buffer_size ) {
        for( unsigned i = 1; i < button_input_buffer_count; i++ ) {
            button_input_buffer[i-1] = button_input_buffer[i];
        }
        button_input_buffer_count--;
    }

    button_input_buffer[button_input_buffer_count].s_id = sequence_id;
    button_input_buffer[button_input_buffer_count].input = input;
    button_input_buffer[button_input_buffer_count].depth = depth;
    button_input_buffer_count++;
}

std::vector<Joystick::JoystickPress> Joystick::GetJoystickInputs()
{
    std::vector<Joystick::JoystickPress> presses;
    presses.resize(button_input_buffer_count);
    for( unsigned i = 0; i < button_input_buffer_count; i++ ) {
        presses[i] = button_input_buffer[i];
    }
    return presses;
}

float Joystick::NormalizeJoystick(float value) {
    // Do not let kDeadzone become 32767.0f, or divide by 0 will occur
    const float kDeadzone = std::min(32767.0f * deadzone, 32000.0f); // Not actually a constant, but sort of
    // Deadzone
    if(value < kDeadzone && value > -kDeadzone) {
        return 0;
    }
    // Subtract deadzone from value and divisor
    value -= kDeadzone;
    float axis = 0.0f;
    axis = pow(value / (32767.0f - kDeadzone), 2.0f);
    // Clamp each axis to {-1,1}
    if (axis < -1.0f) {
        axis = -1.0f;
    } 
    if (axis > 1.0f) {
        axis = 1.0f;
    }
    return axis;
}

const float KeyState::kDepthThreshold = 0.5f;

KeyState::KeyState()
    : count(0)
    , depth_count(0)
    , depth(0)
{ }

bool KeyState::operator!=(const KeyState & other) const {
    return count != other.count || depth != other.depth || depth != other.depth_count;
}

PlayerInput::PlayerInput() : 
    enabled(true),
    remote_controlled(false)
{ }
