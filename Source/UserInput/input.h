//-----------------------------------------------------------------------------
//           Name: input.h
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

#include <UserInput/keyboard.h>
#include <UserInput/mouse.h>
#include <UserInput/joystick.h>

#include <Editors/editor_utilities.h>
#include <Internal/referencecounter.h>

#include <SDL_joystick.h>
#include <SDL_gamecontroller.h>
#include <SDL_haptic.h>
#include <SDL_events.h>

#include <vector>
#include <map>
#include <set>

class SceneGraph;
class Config;
struct SDL_Keysym;
class GameCursor;

typedef std::map<std::string, std::string> StrMap;
typedef std::map<std::string, StrMap> BindMap;  // Key binding map, e.g. bindings["gamepad"]["jump"] = "rightshoulder"

class Input {
   public:
    Input();
    void Initialize();
    void InitializeStringDescriptionMap();
    void Dispose();
    // take a control element and fill it based on the values we have.
    void ProcessController(int controller_id, float timestep);
    void UpdateKeyboardFocus();
    Keyboard& getKeyboard();
    Mouse& getMouse();
    bool GetGrabMouse();
    void SetGrabMouse(bool grab);
    void HandleEvent(const SDL_Event& event);
    void RequestQuit();
    bool WasQuitRequested();
    void SetInvertXMouseLook(bool val);
    void SetInvertYMouseLook(bool val);

    PlayerInput* GetController(int id);
    void ProcessControllers(float timestep);
    void SetUpForXPlayers(unsigned num_players);
    void SetFromConfig(const Config& config);
    float mouse_sensitivity() { return mouse_sensitivity_; }
    void SetMouseSensitivity(float val) { mouse_sensitivity_ = val; }
    void UpdateGamepadLookSensitivity();
    void UpdateGamepadDeadzone();
    void ClearQuitRequested();
    int PlayerOpenedMenu();
    void AllowControllerInput(bool allow);

    int AllocateRemotePlayerInput();

    bool IsKeyDown(const char* name);

    bool IsControllerConnected();

    bool live_updated;
    bool debug_keys;

    float last_controller_event_time;
    float last_mouse_event_time;
    float last_keyboard_event_time;

    bool ignore_mouse_frame;
    GameCursor* cursor;

    static Input* Instance() {
        static Input instance;
        return &instance;
    }

    StrMap key_string_description_map;
    struct JoystickStruct {
        JoystickStruct();
        SDL_Joystick* sdl_joystick;
        SDL_Haptic* sdl_haptic;
        std::vector<SDL_GameControllerButtonBind> gamepad_bind;
        Joystick joystick;
        int player_input;
    };
    typedef ReferenceCounter<JoystickStruct> RC_JoystickStruct;
    typedef std::map<int, RC_JoystickStruct> JoystickMap;

    void UseRawInput(bool val);

    void StartTextInput();
    void StopTextInput();

    std::string GetStringDescriptionForBinding(const std::string& type, const std::string& name);

    std::vector<std::string> GetAvailableBindingCategories();
    std::vector<std::string> GetAvailableBindings(const std::string& binding_category);
    std::set<std::string> GetAllAvailableBindings();
    std::string GetBindingValue(const std::string& binding_category, const std::string& binding);
    void SetBindingValue(std::string binding_category, std::string binding, std::string value);
    void SetKeyboardBindingValue(std::string binding_category, std::string binding, SDL_Scancode value);
    void SetMouseBindingValue(std::string binding_category, std::string binding, uint32_t button);
    void SetMouseBindingValue(std::string binding_category, std::string binding, std::string value);
    void SetControllerBindingValue(std::string binding_category, std::string binding, ControllerInput::Input input);

    JoystickMap& GetOpenJoysticks();

    std::vector<Keyboard::KeyboardPress> GetKeyboardInputs();
    std::vector<Mouse::MousePress> GetMouseInputs();
    std::vector<Joystick::JoystickPress> GetJoystickInputs(int player_index);

   private:
    unsigned num_players_;
    BindMap bindings_;
    std::vector<PlayerInput> player_inputs_;
    bool quit_requested_;
    JoystickMap open_joysticks_;
    Keyboard keyboard_;
    Mouse mouse_;

    bool allow_controller_input_;
    bool use_controller_input_;

    bool in_focus_;
    bool grab_mouse_;
    bool invert_x_mouse_look_;
    bool invert_y_mouse_look_;
    float mouse_sensitivity_;

    bool use_raw_input;
    uint32_t joystick_sequence_id;

    void HandleMouseButtonDown(int the_button, int clicks);
    void HandleMouseButtonUp(int the_button);
    void ProcessBindings();
    void OpenJoystick(int which);
    void CloseJoystick(int instance_id);
};

void UIShowCursor(bool show);
