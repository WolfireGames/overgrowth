//-----------------------------------------------------------------------------
//           Name: keyboard.h
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

#include <Internal/integer.h>

#include <SDL.h>

#include <map>
#include <string>
#include <vector>

//-----------------------------------------------------------------------------
// Class Definition
//-----------------------------------------------------------------------------

enum KeyboardInputModeFlag
{
    KIMF_NO                              = 0,
    KIMF_MENU                            = 1UL << 0,
    KIMF_PLAYING                         = 1UL << 1,
    KIMF_LEVEL_EDITOR_GENERAL            = 1UL << 2,
    KIMF_LEVEL_EDITOR_DIALOGUE_EDITOR    = 1UL << 3,
    KIMF_GUI_GENERAL                     = 1UL << 4,
    KIMF_ANY                             = 0x0FFFFFFF
};

const int SDLK_CTRL = 1000;
const int SDLK_SHIFT = 1001;
const int SDLK_GUI = 1002;
const int SDLK_ALT = 1003;

struct KeyStatus {
    bool down;
    bool pressed;
    KeyStatus():
        down(false),
        pressed(false)
    {}
};

struct InputModeStackElement
{
    InputModeStackElement();

    int32_t id;
    uint32_t mask;
};

class Keyboard
{
    private:    
        uint32_t input_mode;            
    public:
        typedef std::map<SDL_Scancode, KeyStatus> KeyStatusMap;
        KeyStatusMap keys;

        enum KeyComboFlags {
            SHIFT = 0x10000000,
            CTRL = 0x01000000,
            GUI = 0x00100000
        };
        char key_queue[256];
        int key_queue_length;
        
        static const size_t keycode_input_buffer_size = 16;
        uint16_t keycode_input_buffer_sequence_counter;
        struct KeyboardPress {
            uint16_t s_id;
            uint32_t keycode;
            uint32_t scancode;
            uint16_t mod;
        };
        KeyboardPress keycode_input_buffer[keycode_input_buffer_size];
        size_t keycode_input_buffer_count;
    
        std::vector<KeyboardPress> GetKeyboardInputs();
        SDL_Keysym last_key;
        float last_key_delay;
        bool polled;

        Keyboard();

        void addKeyQueue( SDL_Keysym which_key );
        void addKeyCodeBufferItem( SDL_Keysym which_key );
        void Update(float timestep);

        void handleKeyDown( SDL_Keysym the_key );
        void handleKeyDownFirst( SDL_Keysym the_key );
        void handleKeyUp( SDL_Keysym the_key );

        bool isKeycodeDown(SDL_Keycode which_key, const uint32_t kimf ) const;
        bool isScancodeDown(SDL_Scancode which_key, const uint32_t kimf) const;

        bool isKeycodeCombinationDown(int key_combo, SDL_Keycode which_key, const uint32_t kimf) const;
        bool isScancodeCombinationDown(int key_combo, SDL_Scancode which_key, const uint32_t kimf) const;

        bool wasKeycodeCombinationPressed(int key_combo, SDL_Keycode which_key, const uint32_t kimf) const;
        bool wasScancodeCombinationPressed(int key_combo, SDL_Scancode which_key, const uint32_t kimf) const;

        bool wasKeycodePressed(SDL_Keycode which_key,const uint32_t kimf) const;
        bool wasScancodePressed(SDL_Scancode which_key,const uint32_t kimf) const;

        void clearKeyPresses();
        void clearBasicKeyPresses();

        void SetMode( const uint32_t kimf );
        uint32_t GetModes() const;

};
