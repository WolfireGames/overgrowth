//-----------------------------------------------------------------------------
//           Name: keyTranslator.h
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

#include <SDL.h>

#include <string>

// Represents a combination of controller axes and buttons
namespace ControllerInput {
enum Input {
    A = 0,
    B,
    X,
    Y,
    D_UP,
    D_RIGHT,
    D_DOWN,
    D_LEFT,
    START,
    BACK,
    GUIDE,
    L_STICK_PRESSED,
    R_STICK_PRESSED,
    LB,
    RB,
    L_STICK_X,
    L_STICK_Y,
    R_STICK_X,
    R_STICK_Y,
    L_TRIGGER,
    R_TRIGGER,
    NUM_INPUTS,
    // There are constraints upon existing inputs, therefore they should not
    // count to the number of available inputs.
    L_STICK_XN,
    L_STICK_XP,
    L_STICK_YN,
    L_STICK_YP,
    R_STICK_XN,
    R_STICK_XP,
    R_STICK_YN,
    R_STICK_YP,
    NONE
};
}

void InitKeyTranslator();
// Translate to human readable strings
std::string SDLLocaleAdjustedStringFromScancode(SDL_Scancode scancode);
std::string StringFromMouseButton(int button);
std::string StringFromMouseString(const std::string& text);  // e.g. mouse0 -> Left Mouse Button
std::string StringFromControllerInput(ControllerInput::Input input);
std::string StringFromInput(const std::string& input);  // e.g. lshift -> Shift

SDL_Scancode StringToSDLScancode(const std::string& s);
/*#ifdef WIN32
    int XBoxToXInput(const std::string &s);
#endif*/
const char* SDLScancodeToString(SDL_Scancode key);
const char* SDLKeycodeToString(SDL_Keycode key);

// Convert between internal values
ControllerInput::Input SDLControllerButtonToController(SDL_GameControllerButton button);
ControllerInput::Input SDLControllerAxisToController(SDL_GameControllerAxis axis);

SDL_GameControllerButton ControllerToSDLControllerButton(ControllerInput::Input input);
SDL_GameControllerAxis ControllerToSDLControllerAxis(ControllerInput::Input input);

// SDL string, sort of. Converts leftx+, leftx-, etc. as well, even though these
// are not directory SDL strings.
ControllerInput::Input SDLStringToController(const char* string);
