//-----------------------------------------------------------------------------
//           Name: keyTranslator.cpp
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
#include <UserInput/keyTranslator.h>
#include <UserInput/keyboard.h>

#include <Logging/logdata.h>
#include <Utility/strings.h>

#include <SDL.h>

#include <map>
#include <vector>
//#ifdef WIN32
	//#define NOMINMAX
    //#include <windows.h>
    //#include <XInput.h>
//#endif

struct CStrCmp
{
    bool operator()(const char* lhs, const char* rhs) const {
        return std::strcmp(lhs, rhs) < 0;
    }
};

typedef std::pair<const char*, SDL_Scancode> KeyPair;
typedef std::map<const char*, ControllerInput::Input, CStrCmp> StrToControllerMap;
typedef std::map<const char*, const char*, CStrCmp> InputToStrMap;

static std::vector<KeyPair> keys;
static std::map<std::string, SDL_Scancode> str_to_key_map;
static std::map<SDL_Scancode, const char*> key_to_str_map;
static InputToStrMap input_to_string_map;
static StrToControllerMap str_to_controller_map;

/*#ifdef WIN32
#include <XInput.h>
int XBoxToXInput(const std::string &s)
{
    static std::map<std::string, WORD> xbox_map;
    if(xbox_map.empty()){
        xbox_map["A"] = XINPUT_GAMEPAD_A;
        xbox_map["B"] = XINPUT_GAMEPAD_B;
        xbox_map["X"] = XINPUT_GAMEPAD_X;
        xbox_map["Y"] = XINPUT_GAMEPAD_Y;
        xbox_map["LB"] = XINPUT_GAMEPAD_LEFT_SHOULDER;
        xbox_map["RB"] = XINPUT_GAMEPAD_RIGHT_SHOULDER;
        xbox_map["L3"] = XINPUT_GAMEPAD_LEFT_THUMB;
        xbox_map["R3"] = XINPUT_GAMEPAD_RIGHT_THUMB;
        xbox_map["LT"] = 0x0400;
        xbox_map["RT"] = 0x0800;
        xbox_map["DPAD_L"] = XINPUT_GAMEPAD_DPAD_LEFT;
        xbox_map["DPAD_U"] = XINPUT_GAMEPAD_DPAD_UP;
        xbox_map["DPAD_R"] = XINPUT_GAMEPAD_DPAD_RIGHT;
        xbox_map["DPAD_D"] = XINPUT_GAMEPAD_DPAD_DOWN;
        xbox_map["START"] = XINPUT_GAMEPAD_START;
        xbox_map["BACK"] = XINPUT_GAMEPAD_BACK;
    }

    std::map<std::string, WORD>::iterator iter = xbox_map.find(s);
    if(iter != xbox_map.end()){
        return iter->second;
    } else {
        return 0;
    }
}
#endif*/

uint32_t SDL_SCANCODES[] = {
    SDL_SCANCODE_A,
    SDL_SCANCODE_B,
    SDL_SCANCODE_C,
    SDL_SCANCODE_D,
    SDL_SCANCODE_E,
    SDL_SCANCODE_F,
    SDL_SCANCODE_G,
    SDL_SCANCODE_H,
    SDL_SCANCODE_I,
    SDL_SCANCODE_J,
    SDL_SCANCODE_K,
    SDL_SCANCODE_L,
    SDL_SCANCODE_M,
    SDL_SCANCODE_N,
    SDL_SCANCODE_O,
    SDL_SCANCODE_P,
    SDL_SCANCODE_Q,
    SDL_SCANCODE_R,
    SDL_SCANCODE_S,
    SDL_SCANCODE_T,
    SDL_SCANCODE_U,
    SDL_SCANCODE_V,
    SDL_SCANCODE_W,
    SDL_SCANCODE_X,
    SDL_SCANCODE_Y,
    SDL_SCANCODE_Z,

    SDL_SCANCODE_1,
    SDL_SCANCODE_2,
    SDL_SCANCODE_3,
    SDL_SCANCODE_4,
    SDL_SCANCODE_5,
    SDL_SCANCODE_6,
    SDL_SCANCODE_7,
    SDL_SCANCODE_8,
    SDL_SCANCODE_9,
    SDL_SCANCODE_0,

    SDL_SCANCODE_RETURN,
    SDL_SCANCODE_ESCAPE,
    SDL_SCANCODE_BACKSPACE,
    SDL_SCANCODE_TAB,
    SDL_SCANCODE_SPACE,

    SDL_SCANCODE_MINUS,
    SDL_SCANCODE_EQUALS,
    SDL_SCANCODE_LEFTBRACKET,
    SDL_SCANCODE_RIGHTBRACKET,
    SDL_SCANCODE_BACKSLASH,
    SDL_SCANCODE_NONUSHASH,
    SDL_SCANCODE_SEMICOLON,
    SDL_SCANCODE_APOSTROPHE,
    SDL_SCANCODE_GRAVE,
    SDL_SCANCODE_COMMA,
    SDL_SCANCODE_PERIOD,
    SDL_SCANCODE_SLASH,
    SDL_SCANCODE_CAPSLOCK,
    SDL_SCANCODE_F1,
    SDL_SCANCODE_F2,
    SDL_SCANCODE_F3,
    SDL_SCANCODE_F4,
    SDL_SCANCODE_F5,
    SDL_SCANCODE_F6,
    SDL_SCANCODE_F7,
    SDL_SCANCODE_F8,
    SDL_SCANCODE_F9,
    SDL_SCANCODE_F10,
    SDL_SCANCODE_F11,
    SDL_SCANCODE_F12,
    SDL_SCANCODE_PRINTSCREEN,
    SDL_SCANCODE_SCROLLLOCK,
    SDL_SCANCODE_PAUSE,
    SDL_SCANCODE_INSERT,
    SDL_SCANCODE_HOME,
    SDL_SCANCODE_PAGEUP,
    SDL_SCANCODE_DELETE,
    SDL_SCANCODE_END,
    SDL_SCANCODE_PAGEDOWN,
    SDL_SCANCODE_RIGHT,
    SDL_SCANCODE_LEFT,
    SDL_SCANCODE_DOWN,
    SDL_SCANCODE_UP,
    SDL_SCANCODE_NUMLOCKCLEAR,
    SDL_SCANCODE_KP_DIVIDE,
    SDL_SCANCODE_KP_MULTIPLY,
    SDL_SCANCODE_KP_MINUS,
    SDL_SCANCODE_KP_PLUS,
    SDL_SCANCODE_KP_ENTER,
    SDL_SCANCODE_KP_1,
    SDL_SCANCODE_KP_2,
    SDL_SCANCODE_KP_3,
    SDL_SCANCODE_KP_4,
    SDL_SCANCODE_KP_5,
    SDL_SCANCODE_KP_6,
    SDL_SCANCODE_KP_7,
    SDL_SCANCODE_KP_8,
    SDL_SCANCODE_KP_9,
    SDL_SCANCODE_KP_0,
    SDL_SCANCODE_KP_PERIOD,
    SDL_SCANCODE_NONUSBACKSLASH,
    SDL_SCANCODE_APPLICATION,
    SDL_SCANCODE_POWER,
    SDL_SCANCODE_KP_EQUALS,
    SDL_SCANCODE_F13,
    SDL_SCANCODE_F14,
    SDL_SCANCODE_F15,
    SDL_SCANCODE_F16,
    SDL_SCANCODE_F17,
    SDL_SCANCODE_F18,
    SDL_SCANCODE_F19,
    SDL_SCANCODE_F20,
    SDL_SCANCODE_F21,
    SDL_SCANCODE_F22,
    SDL_SCANCODE_F23,
    SDL_SCANCODE_F24,
    SDL_SCANCODE_EXECUTE,
    SDL_SCANCODE_HELP,
    SDL_SCANCODE_MENU,
    SDL_SCANCODE_SELECT,
    SDL_SCANCODE_STOP,
    SDL_SCANCODE_AGAIN,
    SDL_SCANCODE_UNDO,
    SDL_SCANCODE_CUT,
    SDL_SCANCODE_COPY,
    SDL_SCANCODE_PASTE,
    SDL_SCANCODE_FIND,
    SDL_SCANCODE_MUTE,
    SDL_SCANCODE_VOLUMEUP,
    SDL_SCANCODE_VOLUMEDOWN,

    SDL_SCANCODE_KP_COMMA,
    SDL_SCANCODE_KP_EQUALSAS400,

    SDL_SCANCODE_INTERNATIONAL1,

    SDL_SCANCODE_INTERNATIONAL2,
    SDL_SCANCODE_INTERNATIONAL3,
    SDL_SCANCODE_INTERNATIONAL4,
    SDL_SCANCODE_INTERNATIONAL5,
    SDL_SCANCODE_INTERNATIONAL6,
    SDL_SCANCODE_INTERNATIONAL7,
    SDL_SCANCODE_INTERNATIONAL8,
    SDL_SCANCODE_INTERNATIONAL9,
    SDL_SCANCODE_LANG1,
    SDL_SCANCODE_LANG2,
    SDL_SCANCODE_LANG3,
    SDL_SCANCODE_LANG4,
    SDL_SCANCODE_LANG5,
    SDL_SCANCODE_LANG6,
    SDL_SCANCODE_LANG7,
    SDL_SCANCODE_LANG8,
    SDL_SCANCODE_LANG9,

    SDL_SCANCODE_ALTERASE,
    SDL_SCANCODE_SYSREQ,
    SDL_SCANCODE_CANCEL,
    SDL_SCANCODE_CLEAR,
    SDL_SCANCODE_PRIOR,
    SDL_SCANCODE_RETURN2,
    SDL_SCANCODE_SEPARATOR,
    SDL_SCANCODE_OUT,
    SDL_SCANCODE_OPER,
    SDL_SCANCODE_CLEARAGAIN,
    SDL_SCANCODE_CRSEL,
    SDL_SCANCODE_EXSEL,

    SDL_SCANCODE_KP_00,
    SDL_SCANCODE_KP_000,
    SDL_SCANCODE_THOUSANDSSEPARATOR,
    SDL_SCANCODE_DECIMALSEPARATOR,
    SDL_SCANCODE_CURRENCYUNIT,
    SDL_SCANCODE_CURRENCYSUBUNIT,
    SDL_SCANCODE_KP_LEFTPAREN,
    SDL_SCANCODE_KP_RIGHTPAREN,
    SDL_SCANCODE_KP_LEFTBRACE,
    SDL_SCANCODE_KP_RIGHTBRACE,
    SDL_SCANCODE_KP_TAB,
    SDL_SCANCODE_KP_BACKSPACE,
    SDL_SCANCODE_KP_A,
    SDL_SCANCODE_KP_B,
    SDL_SCANCODE_KP_C,
    SDL_SCANCODE_KP_D,
    SDL_SCANCODE_KP_E,
    SDL_SCANCODE_KP_F,
    SDL_SCANCODE_KP_XOR,
    SDL_SCANCODE_KP_POWER,
    SDL_SCANCODE_KP_PERCENT,
    SDL_SCANCODE_KP_LESS,
    SDL_SCANCODE_KP_GREATER,
    SDL_SCANCODE_KP_AMPERSAND,
    SDL_SCANCODE_KP_DBLAMPERSAND,
    SDL_SCANCODE_KP_VERTICALBAR,
    SDL_SCANCODE_KP_DBLVERTICALBAR,
    SDL_SCANCODE_KP_COLON,
    SDL_SCANCODE_KP_HASH,
    SDL_SCANCODE_KP_SPACE,
    SDL_SCANCODE_KP_AT,
    SDL_SCANCODE_KP_EXCLAM,
    SDL_SCANCODE_KP_MEMSTORE,
    SDL_SCANCODE_KP_MEMRECALL,
    SDL_SCANCODE_KP_MEMCLEAR,
    SDL_SCANCODE_KP_MEMADD,
    SDL_SCANCODE_KP_MEMSUBTRACT,
    SDL_SCANCODE_KP_MEMMULTIPLY,
    SDL_SCANCODE_KP_MEMDIVIDE,
    SDL_SCANCODE_KP_PLUSMINUS,
    SDL_SCANCODE_KP_CLEAR,
    SDL_SCANCODE_KP_CLEARENTRY,
    SDL_SCANCODE_KP_BINARY,
    SDL_SCANCODE_KP_OCTAL,
    SDL_SCANCODE_KP_DECIMAL,
    SDL_SCANCODE_KP_HEXADECIMAL,

    SDL_SCANCODE_LCTRL,
    SDL_SCANCODE_LSHIFT,
    SDL_SCANCODE_LALT,
    SDL_SCANCODE_LGUI,
    SDL_SCANCODE_RCTRL,
    SDL_SCANCODE_RSHIFT,
    SDL_SCANCODE_RALT,
    SDL_SCANCODE_RGUI,

    SDL_SCANCODE_MODE,

    SDL_SCANCODE_AUDIONEXT,
    SDL_SCANCODE_AUDIOPREV,
    SDL_SCANCODE_AUDIOSTOP,
    SDL_SCANCODE_AUDIOPLAY,
    SDL_SCANCODE_AUDIOMUTE,
    SDL_SCANCODE_MEDIASELECT,
    SDL_SCANCODE_WWW,
    SDL_SCANCODE_MAIL,
    SDL_SCANCODE_CALCULATOR,
    SDL_SCANCODE_COMPUTER,
    SDL_SCANCODE_AC_SEARCH,
    SDL_SCANCODE_AC_HOME,
    SDL_SCANCODE_AC_BACK,
    SDL_SCANCODE_AC_FORWARD,
    SDL_SCANCODE_AC_STOP,
    SDL_SCANCODE_AC_REFRESH,
    SDL_SCANCODE_AC_BOOKMARKS,

    SDL_SCANCODE_BRIGHTNESSDOWN,
    SDL_SCANCODE_BRIGHTNESSUP,
    SDL_SCANCODE_DISPLAYSWITCH,

    SDL_SCANCODE_KBDILLUMTOGGLE,
    SDL_SCANCODE_KBDILLUMDOWN,
    SDL_SCANCODE_KBDILLUMUP,
    SDL_SCANCODE_EJECT,
    SDL_SCANCODE_SLEEP,

    SDL_SCANCODE_APP1,
    SDL_SCANCODE_APP2
};

/*
uint32_t SDLKS[] = {
    SDLK_RETURN,
    SDLK_ESCAPE,
    SDLK_BACKSPACE,
    SDLK_TAB,
    SDLK_SPACE,
    SDLK_EXCLAIM,
    SDLK_QUOTEDBL,
    SDLK_HASH,
    SDLK_PERCENT,
    SDLK_DOLLAR,
    SDLK_AMPERSAND,
    SDLK_QUOTE,
    SDLK_LEFTPAREN,
    SDLK_RIGHTPAREN,
    SDLK_ASTERISK,
    SDLK_PLUS,
    SDLK_COMMA,
    SDLK_MINUS,
    SDLK_PERIOD,
    SDLK_SLASH,
    SDLK_0,
    SDLK_1,
    SDLK_2,
    SDLK_3,
    SDLK_4,
    SDLK_5,
    SDLK_6,
    SDLK_7,
    SDLK_8,
    SDLK_9,
    SDLK_COLON,
    SDLK_SEMICOLON,
    SDLK_LESS,
    SDLK_EQUALS,
    SDLK_GREATER,
    SDLK_QUESTION,
    SDLK_AT,
    
    SDLK_LEFTBRACKET,
    SDLK_BACKSLASH,
    SDLK_RIGHTBRACKET,
    SDLK_CARET,
    SDLK_UNDERSCORE,
    SDLK_BACKQUOTE,
    SDLK_a,
    SDLK_b,
    SDLK_c,
    SDLK_d,
    SDLK_e,
    SDLK_f,
    SDLK_g,
    SDLK_h,
    SDLK_i,
    SDLK_j,
    SDLK_k,
    SDLK_l,
    SDLK_m,
    SDLK_n,
    SDLK_o,
    SDLK_p,
    SDLK_q,
    SDLK_r,
    SDLK_s,
    SDLK_t,
    SDLK_u,
    SDLK_v,
    SDLK_w,
    SDLK_x,
    SDLK_y,
    SDLK_z,
    
    SDLK_CAPSLOCK,
    
    SDLK_F1,
    SDLK_F2,
    SDLK_F3,
    SDLK_F4,
    SDLK_F5,
    SDLK_F6,
    SDLK_F7,
    SDLK_F8,
    SDLK_F9,
    SDLK_F10,
    SDLK_F11,
    SDLK_F12,
    
    SDLK_PRINTSCREEN,
    SDLK_SCROLLLOCK,
    SDLK_PAUSE,
    SDLK_INSERT,
    SDLK_HOME,
    SDLK_PAGEUP,
    SDLK_DELETE,
    SDLK_END,
    SDLK_PAGEDOWN,
    SDLK_RIGHT,
    SDLK_LEFT,
    SDLK_DOWN,
    SDLK_UP,
    
    SDLK_NUMLOCKCLEAR,
    SDLK_KP_DIVIDE,
    SDLK_KP_MULTIPLY,
    SDLK_KP_MINUS,
    SDLK_KP_PLUS,
    SDLK_KP_ENTER,
    SDLK_KP_1,
    SDLK_KP_2,
    SDLK_KP_3,
    SDLK_KP_4,
    SDLK_KP_5,
    SDLK_KP_6,
    SDLK_KP_7,
    SDLK_KP_8,
    SDLK_KP_9,
    SDLK_KP_0,
    SDLK_KP_PERIOD,
    
    SDLK_APPLICATION,
    SDLK_POWER,
    SDLK_KP_EQUALS,
    SDLK_F13,
    SDLK_F14,
    SDLK_F15,
    SDLK_F16,
    SDLK_F17,
    SDLK_F18,
    SDLK_F19,
    SDLK_F20,
    SDLK_F21,
    SDLK_F22,
    SDLK_F23,
    SDLK_F24,
    SDLK_EXECUTE,
    SDLK_HELP,
    SDLK_MENU,
    SDLK_SELECT,
    SDLK_STOP,
    SDLK_AGAIN,
    SDLK_UNDO,
    SDLK_CUT,
    SDLK_COPY,
    SDLK_PASTE,
    SDLK_FIND,
    SDLK_MUTE,
    SDLK_VOLUMEUP,
    SDLK_VOLUMEDOWN,
    SDLK_KP_COMMA,
    SDLK_KP_EQUALSAS400,
    
    SDLK_ALTERASE,
    SDLK_SYSREQ,
    SDLK_CANCEL,
    SDLK_CLEAR,
    SDLK_PRIOR,
//    SDLK_RETURN2,
    SDLK_SEPARATOR,
    SDLK_OUT,
    SDLK_OPER,
    SDLK_CLEARAGAIN,
    SDLK_CRSEL,
    SDLK_EXSEL,
    
    SDLK_KP_00,
    SDLK_KP_000,
    SDLK_THOUSANDSSEPARATOR,
    SDLK_DECIMALSEPARATOR,
    SDLK_CURRENCYUNIT,
    SDLK_CURRENCYSUBUNIT,
    SDLK_KP_LEFTPAREN,
    SDLK_KP_RIGHTPAREN,
    SDLK_KP_LEFTBRACE,
    SDLK_KP_RIGHTBRACE,
    SDLK_KP_TAB,
    SDLK_KP_BACKSPACE,
    SDLK_KP_A,
    SDLK_KP_B,
    SDLK_KP_C,
    SDLK_KP_D,
    SDLK_KP_E,
    SDLK_KP_F,
    SDLK_KP_XOR,
    SDLK_KP_POWER,
    SDLK_KP_PERCENT,
    SDLK_KP_LESS,
    SDLK_KP_GREATER,
    SDLK_KP_AMPERSAND,
    SDLK_KP_DBLAMPERSAND,
    SDLK_KP_VERTICALBAR,
    SDLK_KP_DBLVERTICALBAR,
    SDLK_KP_COLON,
    SDLK_KP_HASH,
    SDLK_KP_SPACE,
    SDLK_KP_AT,
    SDLK_KP_EXCLAM,
    SDLK_KP_MEMSTORE,
    SDLK_KP_MEMRECALL,
    SDLK_KP_MEMCLEAR,
    SDLK_KP_MEMADD,
    SDLK_KP_MEMSUBTRACT,
    SDLK_KP_MEMMULTIPLY,
    SDLK_KP_MEMDIVIDE,
    SDLK_KP_PLUSMINUS,
    SDLK_KP_CLEAR,
    SDLK_KP_CLEARENTRY,
    SDLK_KP_BINARY,
    SDLK_KP_OCTAL,
    SDLK_KP_DECIMAL,
    SDLK_KP_HEXADECIMAL,
    
    SDLK_LCTRL,
    SDLK_LSHIFT,
    SDLK_LALT,
    SDLK_LGUI,
    SDLK_RCTRL,
    SDLK_RSHIFT,
    SDLK_RALT,
    SDLK_RGUI,
    
    SDLK_MODE,
    
    SDLK_AUDIONEXT,
    SDLK_AUDIOPREV,
    SDLK_AUDIOSTOP,
    SDLK_AUDIOPLAY,
    SDLK_AUDIOMUTE,
    SDLK_MEDIASELECT,
    SDLK_WWW,
    SDLK_MAIL,
    SDLK_CALCULATOR,
    SDLK_COMPUTER,
    SDLK_AC_SEARCH,
    SDLK_AC_HOME,
    SDLK_AC_BACK,
    SDLK_AC_FORWARD,
    SDLK_AC_STOP,
    SDLK_AC_REFRESH,
    SDLK_AC_BOOKMARKS,
    
    SDLK_BRIGHTNESSDOWN,
    SDLK_BRIGHTNESSUP,
    SDLK_DISPLAYSWITCH,
    SDLK_KBDILLUMTOGGLE,
    SDLK_KBDILLUMDOWN,
    SDLK_KBDILLUMUP,
    SDLK_EJECT,
    SDLK_SLEEP
};
*/

static std::vector<char> key_translation_memory;

void InitKeyTranslator() {
    keys.clear();
    key_translation_memory.resize(1024);
    std::vector<std::pair<int,uint32_t> > offsets;

    size_t cur_index = 0;
    for( size_t i = 0; i < sizeof(SDL_SCANCODES)/sizeof(uint32_t); i++ ) {
        const char* scancodename = SDL_GetScancodeName((SDL_Scancode)SDL_SCANCODES[i]);  
        size_t memlen = strlen(scancodename) + 1;
        if(cur_index + memlen > key_translation_memory.size()) {
            key_translation_memory.resize(key_translation_memory.size()+1024);
        }
        memcpy(&key_translation_memory[cur_index], scancodename, memlen);
        UTF8InPlaceLower(&key_translation_memory[cur_index]);
        offsets.push_back(std::pair<int,uint32_t>(cur_index,SDL_SCANCODES[i]));
        cur_index += memlen;
    }

    for( size_t i = 0; i < offsets.size(); i++ ) {
        keys.push_back(KeyPair(&key_translation_memory[offsets[i].first], (SDL_Scancode)offsets[i].second));
    }

    keys.push_back(KeyPair("backspace", SDL_SCANCODE_BACKSPACE));
    keys.push_back(KeyPair("tab",SDL_SCANCODE_TAB));
    keys.push_back(KeyPair("clear",SDL_SCANCODE_CLEAR));
    keys.push_back(KeyPair("return",SDL_SCANCODE_RETURN));
    keys.push_back(KeyPair("pause",SDL_SCANCODE_PAUSE));
    keys.push_back(KeyPair("esc",SDL_SCANCODE_ESCAPE));
    keys.push_back(KeyPair("space",SDL_SCANCODE_SPACE));
    //keys.push_back(KeyPair("!",SDL_SCANCODE_EXCLAIM));
    //keys.push_back(KeyPair("\"",SDL_SCANCODE_QUOTEDBL));
    //keys.push_back(KeyPair("#",SDL_SCANCODE_HASH));
    //keys.push_back(KeyPair("$",SDL_SCANCODE_DOLLAR));
    //keys.push_back(KeyPair("&",SDL_SCANCODE_AMPERSAND));
    //keys.push_back(KeyPair("\'",SDL_SCANCODE_QUOTE));
    //keys.push_back(KeyPair("(",SDL_SCANCODE_LEFTPAREN));
    //keys.push_back(KeyPair(")",SDL_SCANCODE_RIGHTPAREN));
    //keys.push_back(KeyPair("*",SDL_SCANCODE_ASTERISK));
    //keys.push_back(KeyPair("+",SDL_SCANCODE_PLUS));
    keys.push_back(KeyPair(",",SDL_SCANCODE_COMMA));
    keys.push_back(KeyPair("-",SDL_SCANCODE_MINUS));
    keys.push_back(KeyPair(".",SDL_SCANCODE_PERIOD));
    keys.push_back(KeyPair("/",SDL_SCANCODE_SLASH));
    keys.push_back(KeyPair("0",SDL_SCANCODE_0));
    keys.push_back(KeyPair("1",SDL_SCANCODE_1));
    keys.push_back(KeyPair("2",SDL_SCANCODE_2));
    keys.push_back(KeyPair("3",SDL_SCANCODE_3));
    keys.push_back(KeyPair("4",SDL_SCANCODE_4));
    keys.push_back(KeyPair("5",SDL_SCANCODE_5));
    keys.push_back(KeyPair("6",SDL_SCANCODE_6));
    keys.push_back(KeyPair("7",SDL_SCANCODE_7));
    keys.push_back(KeyPair("8",SDL_SCANCODE_8));
    keys.push_back(KeyPair("9",SDL_SCANCODE_9));
    //keys.push_back(KeyPair(":",SDL_SCANCODE_COLON));
    keys.push_back(KeyPair(";",SDL_SCANCODE_SEMICOLON));
    //keys.push_back(KeyPair("<",SDL_SCANCODE_LESS));
    keys.push_back(KeyPair("=",SDL_SCANCODE_EQUALS));
    //keys.push_back(KeyPair(">",SDL_SCANCODE_GREATER));
    //keys.push_back(KeyPair("?",SDL_SCANCODE_QUESTION));
    //keys.push_back(KeyPair("@",SDL_SCANCODE_AT));
    keys.push_back(KeyPair("[",SDL_SCANCODE_LEFTBRACKET));
    keys.push_back(KeyPair("\\",SDL_SCANCODE_BACKSLASH));
    keys.push_back(KeyPair("]",SDL_SCANCODE_RIGHTBRACKET));
    //keys.push_back(KeyPair("^",SDL_SCANCODE_CARET));
    //keys.push_back(KeyPair("_",SDL_SCANCODE_UNDERSCORE));
    keys.push_back(KeyPair("`",SDL_SCANCODE_GRAVE));
    keys.push_back(KeyPair("a",SDL_SCANCODE_A));
    keys.push_back(KeyPair("b",SDL_SCANCODE_B));
    keys.push_back(KeyPair("c",SDL_SCANCODE_C));
    keys.push_back(KeyPair("d",SDL_SCANCODE_D));
    keys.push_back(KeyPair("e",SDL_SCANCODE_E));
    keys.push_back(KeyPair("f",SDL_SCANCODE_F));
    keys.push_back(KeyPair("g",SDL_SCANCODE_G));
    keys.push_back(KeyPair("h",SDL_SCANCODE_H));
    keys.push_back(KeyPair("i",SDL_SCANCODE_I));
    keys.push_back(KeyPair("j",SDL_SCANCODE_J));
    keys.push_back(KeyPair("k",SDL_SCANCODE_K));
    keys.push_back(KeyPair("l",SDL_SCANCODE_L));
    keys.push_back(KeyPair("m",SDL_SCANCODE_M));
    keys.push_back(KeyPair("n",SDL_SCANCODE_N));
    keys.push_back(KeyPair("o",SDL_SCANCODE_O));
    keys.push_back(KeyPair("p",SDL_SCANCODE_P));
    keys.push_back(KeyPair("q",SDL_SCANCODE_Q));
    keys.push_back(KeyPair("r",SDL_SCANCODE_R));
    keys.push_back(KeyPair("s",SDL_SCANCODE_S));
    keys.push_back(KeyPair("t",SDL_SCANCODE_T));
    keys.push_back(KeyPair("u",SDL_SCANCODE_U));
    keys.push_back(KeyPair("v",SDL_SCANCODE_V));
    keys.push_back(KeyPair("w",SDL_SCANCODE_W));
    keys.push_back(KeyPair("x",SDL_SCANCODE_X));
    keys.push_back(KeyPair("y",SDL_SCANCODE_Y));
    keys.push_back(KeyPair("z",SDL_SCANCODE_Z));
    keys.push_back(KeyPair("delete",SDL_SCANCODE_DELETE));
    keys.push_back(KeyPair("keypad0",SDL_SCANCODE_KP_0));
    keys.push_back(KeyPair("keypad1",SDL_SCANCODE_KP_1));
    keys.push_back(KeyPair("keypad2",SDL_SCANCODE_KP_2));
    keys.push_back(KeyPair("keypad3",SDL_SCANCODE_KP_3));
    keys.push_back(KeyPair("keypad4",SDL_SCANCODE_KP_4));
    keys.push_back(KeyPair("keypad5",SDL_SCANCODE_KP_5));
    keys.push_back(KeyPair("keypad6",SDL_SCANCODE_KP_6));
    keys.push_back(KeyPair("keypad7",SDL_SCANCODE_KP_7));
    keys.push_back(KeyPair("keypad8",SDL_SCANCODE_KP_8));
    keys.push_back(KeyPair("keypad9",SDL_SCANCODE_KP_9));
    keys.push_back(KeyPair("keypad.",SDL_SCANCODE_KP_PERIOD));
    keys.push_back(KeyPair("keypad/",SDL_SCANCODE_KP_DIVIDE));
    keys.push_back(KeyPair("keypad*",SDL_SCANCODE_KP_MULTIPLY));
    keys.push_back(KeyPair("keypad-",SDL_SCANCODE_KP_MINUS));
    keys.push_back(KeyPair("keypad+",SDL_SCANCODE_KP_PLUS));
    keys.push_back(KeyPair("keypadenter",SDL_SCANCODE_KP_ENTER));
    keys.push_back(KeyPair("keypad=",SDL_SCANCODE_KP_EQUALS));
    keys.push_back(KeyPair("up",SDL_SCANCODE_UP));
    keys.push_back(KeyPair("down",SDL_SCANCODE_DOWN));
    keys.push_back(KeyPair("right",SDL_SCANCODE_RIGHT));
    keys.push_back(KeyPair("left",SDL_SCANCODE_LEFT));
    keys.push_back(KeyPair("insert",SDL_SCANCODE_INSERT));
    keys.push_back(KeyPair("home",SDL_SCANCODE_HOME));
    keys.push_back(KeyPair("end",SDL_SCANCODE_END));
    keys.push_back(KeyPair("pageup",SDL_SCANCODE_PAGEUP));
    keys.push_back(KeyPair("pagedown",SDL_SCANCODE_PAGEDOWN));
    keys.push_back(KeyPair("f1",SDL_SCANCODE_F1));
    keys.push_back(KeyPair("f2",SDL_SCANCODE_F2));
    keys.push_back(KeyPair("f3",SDL_SCANCODE_F3));
    keys.push_back(KeyPair("f4",SDL_SCANCODE_F4));
    keys.push_back(KeyPair("f5",SDL_SCANCODE_F5));
    keys.push_back(KeyPair("f6",SDL_SCANCODE_F6));
    keys.push_back(KeyPair("f7",SDL_SCANCODE_F7));
    keys.push_back(KeyPair("f8",SDL_SCANCODE_F8));
    keys.push_back(KeyPair("f9",SDL_SCANCODE_F9));
    keys.push_back(KeyPair("f10",SDL_SCANCODE_F10));
    keys.push_back(KeyPair("f11",SDL_SCANCODE_F11));
    keys.push_back(KeyPair("f12",SDL_SCANCODE_F12));
    keys.push_back(KeyPair("f13",SDL_SCANCODE_F13));
    keys.push_back(KeyPair("f14",SDL_SCANCODE_F14));
    keys.push_back(KeyPair("f15",SDL_SCANCODE_F15));
    keys.push_back(KeyPair("numlock",SDL_SCANCODE_NUMLOCKCLEAR));
    keys.push_back(KeyPair("capslock",SDL_SCANCODE_CAPSLOCK));
    keys.push_back(KeyPair("scrollock",SDL_SCANCODE_SCROLLLOCK));
    keys.push_back(KeyPair("rshift",SDL_SCANCODE_RSHIFT));
    keys.push_back(KeyPair("lshift",SDL_SCANCODE_LSHIFT));
    //keys.push_back(KeyPair("shift",SDL_SCANCODE_SHIFT));
    keys.push_back(KeyPair("rctrl",SDL_SCANCODE_RCTRL));
    keys.push_back(KeyPair("lctrl",SDL_SCANCODE_LCTRL));
    //keys.push_back(KeyPair("ctrl",SDL_SCANCODE_CTRL));
    keys.push_back(KeyPair("ralt",SDL_SCANCODE_RALT));
    keys.push_back(KeyPair("lalt",SDL_SCANCODE_LALT));
    keys.push_back(KeyPair("rgui",SDL_SCANCODE_RGUI));
    keys.push_back(KeyPair("lgui",SDL_SCANCODE_LGUI));
    keys.push_back(KeyPair("mode",SDL_SCANCODE_MODE));
    keys.push_back(KeyPair("help",SDL_SCANCODE_HELP));
    keys.push_back(KeyPair("print",SDL_SCANCODE_PRINTSCREEN));
    keys.push_back(KeyPair("sysreq",SDL_SCANCODE_SYSREQ));
    keys.push_back(KeyPair("menu",SDL_SCANCODE_MENU));
    keys.push_back(KeyPair("power",SDL_SCANCODE_POWER));

    int num_keys = keys.size();
    for(int i=0; i<num_keys; ++i){
        const KeyPair &key_pair = keys[i];
        str_to_key_map[key_pair.first] = key_pair.second;
        key_to_str_map[key_pair.second] = key_pair.first;
    }

    str_to_controller_map["RB"] = ControllerInput::RB;
    str_to_controller_map["LB"] = ControllerInput::LB;
    str_to_controller_map["A"] = ControllerInput::A;
    str_to_controller_map["B"] = ControllerInput::B;
    str_to_controller_map["X"] = ControllerInput::X;
    str_to_controller_map["Y"] = ControllerInput::Y;
    str_to_controller_map["START"] = ControllerInput::START;
    str_to_controller_map["BACK"] = ControllerInput::BACK;
    str_to_controller_map["GUIDE"] = ControllerInput::GUIDE;
    str_to_controller_map["L_STICK_X+"] = ControllerInput::L_STICK_XP;
    str_to_controller_map["L_STICK_X-"] = ControllerInput::L_STICK_XN;
    str_to_controller_map["L_STICK_Y+"] = ControllerInput::L_STICK_YP;
    str_to_controller_map["L_STICK_Y-"] = ControllerInput::L_STICK_YN;
    str_to_controller_map["R_STICK_X+"] = ControllerInput::R_STICK_XP;
    str_to_controller_map["R_STICK_X-"] = ControllerInput::R_STICK_XN;
    str_to_controller_map["R_STICK_Y+"] = ControllerInput::R_STICK_YP;
    str_to_controller_map["R_STICK_Y-"] = ControllerInput::R_STICK_YN;
    str_to_controller_map["L_TRIGGER"] = ControllerInput::L_TRIGGER;
    str_to_controller_map["R_TRIGGER"] = ControllerInput::R_TRIGGER;
    str_to_controller_map["RT"] = ControllerInput::R_TRIGGER; // Backwards compat
    str_to_controller_map["LT"] = ControllerInput::L_TRIGGER; // Backwards compat
    str_to_controller_map["R_STICK_PRESSED"] = ControllerInput::R_STICK_PRESSED;
    str_to_controller_map["L_STICK_PRESSED"] = ControllerInput::L_STICK_PRESSED;
    str_to_controller_map["D_UP"] = ControllerInput::D_UP;
    str_to_controller_map["D_DOWN"] = ControllerInput::D_DOWN;
    str_to_controller_map["D_RIGHT"] = ControllerInput::D_RIGHT;
    str_to_controller_map["D_LEFT"] = ControllerInput::D_LEFT;

    input_to_string_map["lshift"] = "Shift";
    input_to_string_map["rshift"] = "Shift";
    input_to_string_map["space"]  = "Space";
    input_to_string_map["tab"]  = "Tab";
    input_to_string_map["q"] = "Q";
    input_to_string_map["e"] = "E";
    input_to_string_map["w"] = "W";
    input_to_string_map["a"] = "A";
    input_to_string_map["s"] = "S";
    input_to_string_map["d"] = "D";
    input_to_string_map["mouse0"] = "Left mouse button";
    input_to_string_map["mouse1"] = "Middle mouse button";
    input_to_string_map["mouse2"] = "Right mouse button";
}

SDL_Scancode StringToSDLScancode(const std::string &s) {
    std::map<std::string, SDL_Scancode>::const_iterator iter = str_to_key_map.find(UTF8ToLower(s));
    if(iter != str_to_key_map.end()){
        return iter->second;
    } else {
        return (SDL_Scancode)SDL_SCANCODE_SYSREQ;
    }
}

const char* SDLScancodeToString(SDL_Scancode key) {
    std::map<SDL_Scancode, const char*>::const_iterator iter = key_to_str_map.find(key);
    if(iter != key_to_str_map.end()){
        return iter->second;
    } else {
        return NULL;
    }
}

const char* SDLKeycodeToString(SDL_Keycode keycode) {
    SDL_Scancode key = SDL_GetScancodeFromKey(keycode);
    std::map<SDL_Scancode, const char*>::const_iterator iter = key_to_str_map.find(key);
    if(iter != key_to_str_map.end()){
        return iter->second;
    } else {
        return NULL;
    }
}

std::string StringFromInput(const std::string& input) {
    InputToStrMap::iterator iter = input_to_string_map.find(input.c_str());
    if(iter != input_to_string_map.end()) {
        return iter->second;
    }

    return input;
}

std::string SDLLocaleAdjustedStringFromScancode( SDL_Scancode scancode ) {
    std::string str = std::string(SDL_GetKeyName(SDL_GetKeyFromScancode(scancode)));
    if( str.empty() ) {
        str = std::string(SDL_GetScancodeName(scancode));
        if( str.empty() ) {
            str = std::string(SDLScancodeToString(scancode));
        }
    }
    return str;
}

std::string StringFromMouseButton(int button) {
    switch(button) {
        case 0:
            return "Left mouse";
        case 1:
            return "Middle mouse";
        case 2:
            return "Right mouse";
        default: {
            char buffer[32];
            sprintf(buffer, "Mouse%d", button);
            return buffer;
        }
    }
}

std::string StringFromMouseString(const std::string& text) {
    if(text == "mousescrollup") {
        return "scroll up";
    } else if(text == "mousescrolldown") {
        return "scroll down";
    } else if(text == "mousescrollleft") {
        return "scroll left";
    } else if(text == "mousescrollright") {
        return "scroll right";
    } else {
        int button = std::atoi(text.c_str() + 5);
        return StringFromMouseButton(button);
    }
    return "";
}

std::string StringFromControllerInput(ControllerInput::Input input) {
    switch(input) {
        case ControllerInput::A:
        case ControllerInput::B:
        case ControllerInput::X:
        case ControllerInput::Y:
        case ControllerInput::D_UP:
        case ControllerInput::D_RIGHT:
        case ControllerInput::D_DOWN:
        case ControllerInput::D_LEFT:
        case ControllerInput::START:
        case ControllerInput::BACK:
        case ControllerInput::GUIDE:
        case ControllerInput::L_STICK_PRESSED:
        case ControllerInput::R_STICK_PRESSED:
        case ControllerInput::LB:
        case ControllerInput::RB:
            return SDL_GameControllerGetStringForButton(ControllerToSDLControllerButton(input));
        case ControllerInput::L_STICK_XN: {
            const static std::string value = std::string(StringFromControllerInput(ControllerInput::L_STICK_X)) + "-";
            return value.c_str();
        }
        case ControllerInput::L_STICK_XP: {
            const static std::string value = std::string(StringFromControllerInput(ControllerInput::L_STICK_X)) + "+";
            return value.c_str();
        }
        case ControllerInput::L_STICK_YN: {
            const static std::string value = std::string(StringFromControllerInput(ControllerInput::L_STICK_Y)) + "-";
            return value.c_str();
        }
        case ControllerInput::L_STICK_YP: {
            const static std::string value = std::string(StringFromControllerInput(ControllerInput::L_STICK_Y)) + "+";
            return value.c_str();
        }
        case ControllerInput::R_STICK_XN: {
            const static std::string value = std::string(StringFromControllerInput(ControllerInput::R_STICK_X)) + "-";
            return value.c_str();
        }
        case ControllerInput::R_STICK_XP: {
            const static std::string value = std::string(StringFromControllerInput(ControllerInput::R_STICK_X)) + "+";
            return value.c_str();
        }
        case ControllerInput::R_STICK_YN: {
            const static std::string value = std::string(StringFromControllerInput(ControllerInput::R_STICK_Y)) + "-";
            return value.c_str();
        }
        case ControllerInput::R_STICK_YP: {
            const static std::string value = std::string(StringFromControllerInput(ControllerInput::R_STICK_Y)) + "+";
            return value.c_str();
        }
        case ControllerInput::L_STICK_X:
        case ControllerInput::L_STICK_Y:
        case ControllerInput::R_STICK_X:
        case ControllerInput::R_STICK_Y:
        case ControllerInput::L_TRIGGER:
        case ControllerInput::R_TRIGGER:
            return SDL_GameControllerGetStringForAxis(ControllerToSDLControllerAxis(input));
        default:
            return "UNKNOWN KEY";
    }
}

ControllerInput::Input SDLControllerButtonToController(SDL_GameControllerButton button) {
    switch(button) {
        case SDL_CONTROLLER_BUTTON_A: return ControllerInput::A;
        case SDL_CONTROLLER_BUTTON_B: return ControllerInput::B;
        case SDL_CONTROLLER_BUTTON_X: return ControllerInput::X;
        case SDL_CONTROLLER_BUTTON_Y: return ControllerInput::Y;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return ControllerInput::LB;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return ControllerInput::RB;
        case SDL_CONTROLLER_BUTTON_BACK: return ControllerInput::BACK;
        case SDL_CONTROLLER_BUTTON_GUIDE: return ControllerInput::GUIDE;
        case SDL_CONTROLLER_BUTTON_START: return ControllerInput::START;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK: return ControllerInput::L_STICK_PRESSED;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return ControllerInput::R_STICK_PRESSED;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: return ControllerInput::D_UP;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return ControllerInput::D_DOWN;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return ControllerInput::D_LEFT;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return ControllerInput::D_RIGHT;
        case SDL_CONTROLLER_BUTTON_INVALID:
            LOGW << "Got unhandled SDL_CONTROLLER_BUTTON_INVALID" << std::endl;
            return ControllerInput::NONE;
        case SDL_CONTROLLER_BUTTON_MAX:
            LOGW << "Got unhandled SDL_CONTROLLER_BUTTON_MAX" << std::endl;
            return ControllerInput::NONE;
        default:
            LOGW << "Got default" << std::endl;
            return ControllerInput::NONE;
    }
}

ControllerInput::Input SDLControllerAxisToController(SDL_GameControllerAxis axis) {
    switch(axis) {
        case SDL_CONTROLLER_AXIS_LEFTX: return ControllerInput::L_STICK_X;
        case SDL_CONTROLLER_AXIS_LEFTY: return ControllerInput::L_STICK_Y;
        case SDL_CONTROLLER_AXIS_RIGHTX: return ControllerInput::R_STICK_X;
        case SDL_CONTROLLER_AXIS_RIGHTY: return ControllerInput::R_STICK_Y;
        case SDL_CONTROLLER_AXIS_TRIGGERLEFT: return ControllerInput::L_TRIGGER;
        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: return ControllerInput::R_TRIGGER;
        case SDL_CONTROLLER_AXIS_INVALID:
            LOGW << "Got unhandled SDL_CONTROLLER_AXIS_INVALID" << std::endl;
            return ControllerInput::NONE;
        case SDL_CONTROLLER_AXIS_MAX:
            LOGW << "Got unhandled SDL_CONTROLLER_AXIS_MAX" << std::endl;
            return ControllerInput::NONE;
        default:
            LOGW << "Got default" << std::endl;
            return ControllerInput::NONE;
    }
}

SDL_GameControllerButton ControllerToSDLControllerButton(ControllerInput::Input input) {
    switch(input) {
        case ControllerInput::A: return SDL_CONTROLLER_BUTTON_A;
        case ControllerInput::B: return SDL_CONTROLLER_BUTTON_B;
        case ControllerInput::X: return SDL_CONTROLLER_BUTTON_X;
        case ControllerInput::Y: return SDL_CONTROLLER_BUTTON_Y;
        case ControllerInput::LB: return SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
        case ControllerInput::RB: return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
        case ControllerInput::BACK: return SDL_CONTROLLER_BUTTON_BACK;
        case ControllerInput::GUIDE: return SDL_CONTROLLER_BUTTON_GUIDE;
        case ControllerInput::START: return SDL_CONTROLLER_BUTTON_START;
        case ControllerInput::L_STICK_PRESSED: return SDL_CONTROLLER_BUTTON_LEFTSTICK;
        case ControllerInput::R_STICK_PRESSED: return SDL_CONTROLLER_BUTTON_RIGHTSTICK;
        case ControllerInput::D_UP: return SDL_CONTROLLER_BUTTON_DPAD_UP;
        case ControllerInput::D_DOWN: return SDL_CONTROLLER_BUTTON_DPAD_DOWN;
        case ControllerInput::D_LEFT: return SDL_CONTROLLER_BUTTON_DPAD_LEFT;
        case ControllerInput::D_RIGHT: return SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
        case ControllerInput::NONE:
            LOGW << "Got unhandled NONE" << std::endl;
            return SDL_CONTROLLER_BUTTON_INVALID;
        default:
            LOGW << "Got unhandled default" << std::endl;
            return SDL_CONTROLLER_BUTTON_INVALID;
    }
}

SDL_GameControllerAxis ControllerToSDLControllerAxis(ControllerInput::Input input) {
    switch(input) {
        case ControllerInput::L_STICK_XN:
        case ControllerInput::L_STICK_XP:
        case ControllerInput::L_STICK_X:
            return SDL_CONTROLLER_AXIS_LEFTX;
        case ControllerInput::L_STICK_YN:
        case ControllerInput::L_STICK_YP:
        case ControllerInput::L_STICK_Y:
            return SDL_CONTROLLER_AXIS_LEFTY;
        case ControllerInput::R_STICK_XN:
        case ControllerInput::R_STICK_XP:
        case ControllerInput::R_STICK_X:
            return SDL_CONTROLLER_AXIS_RIGHTX;
        case ControllerInput::R_STICK_YN:
        case ControllerInput::R_STICK_YP:
        case ControllerInput::R_STICK_Y:
            return SDL_CONTROLLER_AXIS_RIGHTY;
        case ControllerInput::L_TRIGGER: return SDL_CONTROLLER_AXIS_TRIGGERLEFT;
        case ControllerInput::R_TRIGGER: return SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
        case ControllerInput::NONE:
            LOGW << "Got unhandled NONE" << std::endl;
            return SDL_CONTROLLER_AXIS_INVALID;
        default:
            LOGW << "Got unhandled default" << std::endl;
            return SDL_CONTROLLER_AXIS_INVALID;
    }
}

ControllerInput::Input SDLStringToController(const char* string) {
    SDL_GameControllerButton button = SDL_GameControllerGetButtonFromString(string);
    if(button != SDL_CONTROLLER_BUTTON_INVALID) {
        return SDLControllerButtonToController(button);
    }
    char buffer[128];
    strcpy(buffer, string);
    int last = strlen(buffer) - 1;
    if(last > 0) {
        if(buffer[last] == '+' || buffer[last] == '-') {
            buffer[last] = '\0';
        }
        SDL_GameControllerAxis axis = SDL_GameControllerGetAxisFromString(buffer);
        if(axis != SDL_CONTROLLER_AXIS_INVALID) {
            ControllerInput::Input input = SDLControllerAxisToController(axis);
            switch(input) {
                case ControllerInput::L_STICK_X:
                    if(string[last] == '+') {
                        return ControllerInput::L_STICK_XP;
                    } else if(string[last] == '-') {
                        return ControllerInput::L_STICK_XN;
                    } else {
                        return input;
                    }
                case ControllerInput::L_STICK_Y:
                    if(string[last] == '+') {
                        return ControllerInput::L_STICK_YP;
                    } else if(string[last] == '-') {
                        return ControllerInput::L_STICK_YN;
                    } else {
                        return input;
                    }
                case ControllerInput::R_STICK_X:
                    if(string[last] == '+') {
                        return ControllerInput::R_STICK_XP;
                    } else if(string[last] == '-') {
                        return ControllerInput::R_STICK_XN;
                    } else {
                        return input;
                    }
                case ControllerInput::R_STICK_Y:
                    if(string[last] == '+') {
                        return ControllerInput::R_STICK_YP;
                    } else if(string[last] == '-') {
                        return ControllerInput::R_STICK_YN;
                    } else {
                        return input;
                    }
                default:
                    return input;
            }
        }
    }

    LOGW << "Tried converting an invalid string \"" << string << "\" to a controller input" << std::endl;
    return ControllerInput::NONE;
}
