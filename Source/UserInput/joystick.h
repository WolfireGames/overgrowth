//-----------------------------------------------------------------------------
//           Name: joystick.h
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
#pragma once

#include <UserInput/keyTranslator.h>

#include <string>
#include <map>
#include <vector>

class Joystick {
public:
    static const size_t button_input_buffer_size = 16;
    struct JoystickPress {
        uint32_t s_id;
        ControllerInput::Input input;
        float depth;
    };
    JoystickPress button_input_buffer[button_input_buffer_size];
    size_t button_input_buffer_count;

    float look_sensitivity_;
    float deadzone;

    Joystick(float look_sensitivity);
    bool HandleInputChange(ControllerInput::Input input, float val, uint32_t sequence_id);
    float GetButtonDown(const std::string &name) const;
    void ProcessBinding( ControllerInput::Input input, const std::string command);
    void SetButtonDown( const std::string &name, float depth );
    void ClearBinding();

    std::vector<JoystickPress> GetJoystickInputs();

    typedef std::map<std::string, float> ButtonMap;
    ButtonMap buttons_down_;
private:
    typedef std::multimap<ControllerInput::Input, std::string> BindingMap;
    
    BindingMap binding_;

    void AddInputBufferItem(ControllerInput::Input input, float depth, uint32_t sequence_id);
    float NormalizeJoystick(float value);
};

class KeyState {
public:
    const static float kDepthThreshold;
    KeyState();

    bool operator!=(const KeyState& other) const;

    int count;
    int depth_count; // This is only incremented when depth is greater than kDepthThreshold
    float depth;
};

struct PlayerInput {
    typedef std::map<std::string, KeyState> KeyDownMap;

    //Set depending on if there is a purpose of polling for a local character,
    //Usually set to false for controller 2-4 if split-screen is disabled.
    bool enabled;

    //Disable local processing of input for this PlayerInput, and instead expect input to
    //come from the Multiplayer system.
    bool remote_controlled;

    KeyDownMap key_down;

    PlayerInput();
};
