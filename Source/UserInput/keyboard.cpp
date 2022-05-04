//-----------------------------------------------------------------------------
//           Name: keyboard.cpp
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
#include "keyboard.h"

#include <Internal/timer.h>
#include <Logging/logdata.h>

#include <SDL.h>

//-----------------------------------------------------------------------------
//Functions
//-----------------------------------------------------------------------------

InputModeStackElement::InputModeStackElement() : id(-1),mask(0U)
{
    
}

Keyboard::Keyboard() :
        key_queue_length(0),
        keycode_input_buffer_sequence_counter(0),
        keycode_input_buffer_count(0), 
        last_key_delay(0.0f),
        polled(true),
        input_mode(KIMF_ANY)
{
    ::memset(key_queue, 0, sizeof(key_queue));
    last_key.sym = SDLK_UNKNOWN;
    last_key.scancode = SDL_SCANCODE_UNKNOWN;
}

void Keyboard::addKeyQueue( SDL_Keysym which_key ) {
    SDL_Keycode keycode = which_key.sym;
    if(strlen(key_queue)<250){
        if(strlen(SDL_GetKeyName(keycode))==1){
            key_queue[key_queue_length]=SDL_GetKeyName(keycode)[0];
            if(keys[SDL_SCANCODE_LSHIFT].down||keys[SDL_SCANCODE_RSHIFT].down){
                if(key_queue[key_queue_length]>='a'
                &&key_queue[key_queue_length]<='z'){
                    key_queue[key_queue_length]-=32;
                }
                if(key_queue[key_queue_length]=='1'){
                    key_queue[key_queue_length]='!';
                }
                if(key_queue[key_queue_length]=='2'){
                    key_queue[key_queue_length]='@';
                }
                if(key_queue[key_queue_length]=='3'){
                    key_queue[key_queue_length]='#';
                }
                if(key_queue[key_queue_length]=='4'){
                    key_queue[key_queue_length]='$';
                }
                if(key_queue[key_queue_length]=='5'){
                    key_queue[key_queue_length]='%';
                }
                if(key_queue[key_queue_length]=='6'){
                    key_queue[key_queue_length]='^';
                }
                if(key_queue[key_queue_length]=='7'){
                    key_queue[key_queue_length]='&';
                }
                if(key_queue[key_queue_length]=='8'){
                    key_queue[key_queue_length]='*';
                }
                if(key_queue[key_queue_length]=='9'){
                    key_queue[key_queue_length]='(';
                }
                if(key_queue[key_queue_length]=='0'){
                    key_queue[key_queue_length]=')';
                }
                if(key_queue[key_queue_length]=='`'){
                    key_queue[key_queue_length]='~';
                }
                if(key_queue[key_queue_length]=='-'){
                    key_queue[key_queue_length]='_';
                }
                if(key_queue[key_queue_length]=='='){
                    key_queue[key_queue_length]='+';
                }
                if(key_queue[key_queue_length]=='['){
                    key_queue[key_queue_length]='{';
                }
                if(key_queue[key_queue_length]==']'){
                    key_queue[key_queue_length]='}';
                }
                if(key_queue[key_queue_length]=='\\'){
                    key_queue[key_queue_length]='|';
                }
                if(key_queue[key_queue_length]==';'){
                    key_queue[key_queue_length]=':';
                }
                if(key_queue[key_queue_length]=='\''){
                    key_queue[key_queue_length]='"';
                }
                if(key_queue[key_queue_length]==','){
                    key_queue[key_queue_length]='<';
                }
                if(key_queue[key_queue_length]=='.'){
                    key_queue[key_queue_length]='>';
                }
                if(key_queue[key_queue_length]=='/'){
                    key_queue[key_queue_length]='?';
                }
            }
            if(key_queue[key_queue_length]=='\\'){
                key_queue_length++;
                key_queue[key_queue_length]='\\';
            }
            key_queue_length++;
        }
        else if(keycode == SDLK_SPACE){
            key_queue[key_queue_length]=' ';
            key_queue_length++;
        }
        else if(keycode == SDLK_KP_0){
            key_queue[key_queue_length]='0';
            key_queue_length++;
        }
        else if(keycode == SDLK_KP_1){
            key_queue[key_queue_length]='1';
            key_queue_length++;
        }
        else if(keycode == SDLK_KP_2){
            key_queue[key_queue_length]='2';
            key_queue_length++;
        }
        else if(keycode == SDLK_KP_3){
            key_queue[key_queue_length]='3';
            key_queue_length++;
        }
        else if(keycode == SDLK_KP_4){
            key_queue[key_queue_length]='4';
            key_queue_length++;
        }
        else if(keycode == SDLK_KP_5){
            key_queue[key_queue_length]='5';
            key_queue_length++;
        }
        else if(keycode == SDLK_KP_6){
            key_queue[key_queue_length]='6';
            key_queue_length++;
        }
        else if(keycode == SDLK_KP_7){
            key_queue[key_queue_length]='7';
            key_queue_length++;
        }
        else if(keycode == SDLK_KP_8){
            key_queue[key_queue_length]='8';
            key_queue_length++;
        }
        else if(keycode == SDLK_KP_9){
            key_queue[key_queue_length]='9';
            key_queue_length++;
        }
        else if(keycode == SDLK_KP_PERIOD){
            key_queue[key_queue_length]='.';
            key_queue_length++;
        }
        else if(keycode == SDLK_KP_DIVIDE){
            key_queue[key_queue_length]='/';
            key_queue_length++;
        }
        else if(keycode == SDLK_KP_MULTIPLY){
            key_queue[key_queue_length]='*';
            key_queue_length++;
        }
        else if(keycode == SDLK_KP_MINUS){
            key_queue[key_queue_length]='-';
            key_queue_length++;
        }
        else if(keycode == SDLK_KP_PLUS){
            key_queue[key_queue_length]='+';
            key_queue_length++;
        }
        else if(keycode == SDLK_KP_EQUALS){
            key_queue[key_queue_length]='=';
            key_queue_length++;
        }
        else if(keycode == SDLK_BACKSPACE){
            key_queue[key_queue_length]='\\';
            key_queue_length++;
            key_queue[key_queue_length]='<';
            key_queue_length++;
        }
        else if(keycode == SDLK_DELETE){
            key_queue[key_queue_length]='\\';
            key_queue_length++;
            key_queue[key_queue_length]='>';
            key_queue_length++;
        }
        else if(keycode == SDLK_LEFT){
            key_queue[key_queue_length]='\\';
            key_queue_length++;
            key_queue[key_queue_length]='a';
            key_queue_length++;
        }
        else if(keycode == SDLK_UP){
            key_queue[key_queue_length]='\\';
            key_queue_length++;
            key_queue[key_queue_length]='w';
            key_queue_length++;
        }
        else if(keycode == SDLK_DOWN){
            key_queue[key_queue_length]='\\';
            key_queue_length++;
            key_queue[key_queue_length]='s';
            key_queue_length++;
        }
        else if(keycode == SDLK_RIGHT){
            key_queue[key_queue_length]='\\';
            key_queue_length++;
            key_queue[key_queue_length]='d';
            key_queue_length++;
        }
        else if(keycode == SDLK_RETURN){
            key_queue[key_queue_length]='\\';
            key_queue_length++;
            key_queue[key_queue_length]='n';
            key_queue_length++;
        }
    }
}

void Keyboard::addKeyCodeBufferItem( SDL_Keysym which_key ) {
    if( keycode_input_buffer_count >= keycode_input_buffer_size ) {
        for( unsigned i = 1; i < keycode_input_buffer_count; i++ ) {
            keycode_input_buffer[i-1] = keycode_input_buffer[i]; 
        }      
        keycode_input_buffer_count--;
    }

    keycode_input_buffer[keycode_input_buffer_count].s_id = keycode_input_buffer_sequence_counter++;
    keycode_input_buffer[keycode_input_buffer_count].keycode = which_key.sym;
    keycode_input_buffer[keycode_input_buffer_count].scancode = which_key.scancode;
    keycode_input_buffer[keycode_input_buffer_count].mod = which_key.mod;
    keycode_input_buffer_count++;
}

//Handle key presses
void Keyboard::handleKeyDown( SDL_Keysym the_key ) {
    KeyStatus &key = keys[the_key.scancode];
    if(!key.down){
        key.down = true;
        key.pressed = true;
    }
    last_key = the_key;
    last_key_delay = 0.6f;
    addKeyQueue(the_key);
}

void Keyboard::handleKeyDownFirst( SDL_Keysym the_key ) {
    addKeyCodeBufferItem(the_key);
}

void Keyboard::Update(float timestep) {
    KeyStatus &key = keys[last_key.scancode];
    if(key.down){
        last_key_delay -= timestep;
        while(last_key_delay<0){
            addKeyQueue(last_key);
            last_key_delay+=.05f;
        }
    }
}

//Handle key releases
void Keyboard::handleKeyUp( SDL_Keysym the_key ) {
    KeyStatus &key = keys[the_key.scancode];
    key.down = false;
}

//Check if a key is pressed
bool Keyboard::isKeycodeDown(SDL_Keycode which_key, const uint32_t kimf ) const {
    SDL_Scancode sc = SDL_GetScancodeFromKey(which_key);
    if( kimf & input_mode ) {
        switch(which_key){
        case SDLK_CTRL: 
            return isKeycodeDown(SDLK_LCTRL,kimf)  || isKeycodeDown(SDLK_RCTRL,kimf);
        case SDLK_ALT: 
            return isKeycodeDown(SDLK_LALT,kimf)   || isKeycodeDown(SDLK_RALT,kimf);
        case SDLK_GUI: 
            return isKeycodeDown(SDLK_LGUI,kimf)   || isKeycodeDown(SDLK_RGUI,kimf);
        case SDLK_SHIFT: 
            return isKeycodeDown(SDLK_LSHIFT,kimf) || isKeycodeDown(SDLK_RSHIFT,kimf);
        default: {
                KeyStatusMap::const_iterator iter = keys.find(sc);
                if(iter != keys.end()) {
                    const KeyStatus &key = iter->second;
                    return key.down;
                } else {
                    return false;
                }
            }
        }
    }
    return false;
}

bool Keyboard::isScancodeDown(SDL_Scancode which_key, const uint32_t kimf ) const {
    KeyStatusMap::const_iterator iter = keys.find(which_key);
    if(iter != keys.end()) {
        const KeyStatus &key = iter->second;
        return key.down;
    } else {
        return false;
    }
}

bool Keyboard::isKeycodeCombinationDown(int key_combo, SDL_Keycode which_key, const uint32_t kimf) const {
    bool shift = false;
    bool ctrl  = false;
    bool gui   = false;
    if (key_combo & SHIFT) shift = true;
    if (key_combo & CTRL)  ctrl = true;
    if (key_combo & GUI)   gui = true;
    
    return (!ctrl  || isKeycodeDown(SDLK_CTRL,kimf)) && 
           (!shift || isKeycodeDown(SDLK_SHIFT,kimf)) && 
           (!gui   || isKeycodeDown(SDLK_GUI,kimf)) && 
           isKeycodeDown(which_key, kimf);
}

// Was the key combination pressed since we last checked?
//  note: only zeros which_key
bool Keyboard::wasKeycodeCombinationPressed(int key_combo, SDL_Keycode which_key, const uint32_t kimf) const {
    bool shift = false;
    bool ctrl = false;
    bool gui = false;
    if (key_combo & SHIFT) shift = true;
    if (key_combo & CTRL)  ctrl = true;
    if (key_combo & GUI)   gui = true;
    
    return (!ctrl   || isKeycodeDown(SDLK_CTRL,kimf)) && 
           (!shift || isKeycodeDown(SDLK_SHIFT,kimf)) && 
           (!gui   || isKeycodeDown(SDLK_GUI,kimf)) && 
           wasKeycodePressed(which_key,kimf);
}

bool Keyboard::isScancodeCombinationDown(int key_combo, SDL_Scancode which_key, const uint32_t kimf) const {
    bool shift = false;
    bool ctrl  = false;
    bool gui   = false;
    if (key_combo & SHIFT) shift = true;
    if (key_combo & CTRL)  ctrl = true;
    if (key_combo & GUI)   gui = true;
    
    return (!ctrl  || isKeycodeDown(SDLK_CTRL,kimf)) && 
           (!shift || isKeycodeDown(SDLK_SHIFT,kimf)) && 
           (!gui   || isKeycodeDown(SDLK_GUI,kimf)) && 
           isScancodeDown(which_key, kimf);
}

//Was the key pressed since we last checked?
bool Keyboard::wasKeycodePressed(SDL_Keycode the_key, const uint32_t kimf) const {
    SDL_Scancode sc = SDL_GetScancodeFromKey(the_key);
    if(kimf & input_mode) {
        KeyStatusMap::const_iterator iter = keys.find(sc);
        if(iter != keys.end()){
            const KeyStatus &key = iter->second;
            return key.pressed;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

//Was the key pressed since we last checked?
bool Keyboard::wasScancodePressed(SDL_Scancode the_key, const uint32_t kimf) const {
    if(kimf & input_mode) {
        KeyStatusMap::const_iterator iter = keys.find(the_key);
        if(iter != keys.end()){
            const KeyStatus &key = iter->second;
            return key.pressed;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

void Keyboard::clearKeyPresses() {
    for (auto & iter : keys){
        KeyStatus &key = iter.second;
        key.pressed=0;
    }
}

void Keyboard::clearBasicKeyPresses() {
    if(!isKeycodeDown(SDLK_CTRL, KIMF_ANY) &&
       !isKeycodeDown(SDLK_ALT, KIMF_ANY) &&
       !isKeycodeDown(SDLK_GUI, KIMF_ANY))
    {
       clearKeyPresses();
    }
}

void Keyboard::SetMode( const uint32_t kimf )
{
    input_mode = kimf;
}

uint32_t Keyboard::GetModes() const
{
    return input_mode;
}

std::vector<Keyboard::KeyboardPress> Keyboard::GetKeyboardInputs() {
    std::vector<Keyboard::KeyboardPress> presses;  
    presses.resize(keycode_input_buffer_count);
    for( unsigned i = 0; i < keycode_input_buffer_count; i++ ) {
        presses[i] = keycode_input_buffer[i];
    }
    return presses;
}

