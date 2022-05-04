//-----------------------------------------------------------------------------
//           Name: keycommands.cpp
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
#include "keycommands.h"

#include <Internal/error.h>
#include <Internal/checksum.h>
#include <Internal/common.h>

#include <UserInput/keyboard.h>
#include <UserInput/keyTranslator.h>

#include <Logging/logdata.h>

#include <SDL.h>

#include <algorithm>

using namespace KeyCommand;

// Array of key commands, one for each label
static Command commands[kNumLabels];
static Command *commands_sorted_by_length[kNumLabels];
typedef std::pair<unsigned long, int> HashInt;
typedef std::vector<HashInt> HashIntVector;
HashIntVector hash_to_command;

struct HashIntCompare {
    bool operator()(const HashInt& a, const HashInt& b){
        return a.first < b.first;
    }
    bool operator()(const HashInt& a, unsigned long b){
        return a.first < b;
    }
};

Command::Command(): num_key_events(0), state(kUp)
{
    display_text[0] = '\0';
}

bool KeyCommand::CheckPressed( const Keyboard& keyboard, int label, const uint32_t kimf ) {
    if( kimf & keyboard.GetModes() )
    {
        if(commands[label].num_key_events == 0){
            return false;
        } else {
            return commands[label].state == kPressed;
        }
    }
    else
    {
        return false;
    }
}

#ifdef PLATFORM_MACOSX
    const int command_key = Keyboard::GUI;
    const SDL_Scancode command_SDL_SCANCODE_LEFT_key = SDL_SCANCODE_LGUI;
    const SDL_Scancode command_SDL_SCANCODE_RIGHT_key = SDL_SCANCODE_RGUI;
#else
    const int command_key = Keyboard::CTRL;
    const SDL_Scancode command_SDL_SCANCODE_LEFT_key = SDL_SCANCODE_LCTRL;
    const SDL_Scancode command_SDL_SCANCODE_RIGHT_key = SDL_SCANCODE_RCTRL;
#endif

bool KeyCommand::CheckDown( const Keyboard& keyboard, int label, const uint32_t kimf ) {
    if( kimf & keyboard.GetModes() )
    {
        if(commands[label].num_key_events == 0){
            return false;
        } else {
            const Command &cmd = commands[label];
            for(int i=0, len=cmd.num_key_events; i<len; ++i){
                const KeyEvent &key_event = cmd.key_events[i];
                bool key_valid = false;
                switch(key_event.type_){
                case kPressed:
                case kDown:
                    if( key_event.sc_ == key_event.sc2_ ) {
                        key_valid = keyboard.isScancodeDown(key_event.sc_, KIMF_ANY);
                    } else {
                        key_valid = keyboard.isScancodeDown(key_event.sc_, KIMF_ANY) || keyboard.isScancodeDown(key_event.sc2_, KIMF_ANY);
                    }
                    break;
                case kUp:
                    if( key_event.sc_ == key_event.sc2_ ) {
                        key_valid = !keyboard.isScancodeDown(key_event.sc_, KIMF_ANY);
                    } else {
                        key_valid = !keyboard.isScancodeDown(key_event.sc_, KIMF_ANY) && !keyboard.isScancodeDown(key_event.sc2_, KIMF_ANY);
                    }
                    break;
                }
                if(!key_valid){
                    return false;
                }
            }
            return true;
        }
    }
    else
    {
        return false;
    }
}

const char* KeyCommand::GetDisplayText(int label) {
    return commands[label].display_text;
}

bool CompareSubstr(const char* str, const char* substr, int substr_len){
    int i=0;
    while(str[i] != '\0' && substr[i] != '\0' && i < substr_len){
        if(str[i] != substr[i]){
            return false;
        }
        ++i;
    }
    return str[i] == '\0' && i == substr_len;
}

void KeyCommand::BindString(const char* bind_str) {
    KeyEvent events[kMaxKeyEvents];
    enum ParseStage {kInputs, kCommand};
    ParseStage parse_stage = kInputs;
    int num_events = 0;
    int i = 0;
    int start_token = 0;
    std::string str;
    std::string display;
    str.reserve(255);
    display.reserve(255);
    while(true){
        char c = bind_str[i];
        if(parse_stage == kInputs){
            if(c == '+' || c == ':'){
                if(c == ':'){
                    parse_stage = kCommand;
                }
                if(num_events >= kMaxKeyEvents){
                    FatalError("Error", "Too many key events in %s", bind_str);
                }
                // ignore white space at start or end
                while(bind_str[start_token] == ' ' || bind_str[start_token] == '\t'){
                    ++start_token;
                }
                int end_token = i-1;
                while(bind_str[end_token] == ' ' || bind_str[end_token] == '\t'){
                    --end_token;
                }
                // Read token and attempt to parse key name
                int token_len = end_token-start_token+1;
                if(CompareSubstr("cmd", &bind_str[start_token], token_len)){
#ifdef __MACOSX__
                    display += "Cmd";
#else
                    display += "Ctrl";
#endif
                    events[num_events].sc_ = command_SDL_SCANCODE_LEFT_key;
                    events[num_events].sc2_ = command_SDL_SCANCODE_RIGHT_key;
                    events[num_events].type_ = kDown;
                    ++num_events;
                } else if(CompareSubstr("shift", &bind_str[start_token], token_len)){
                    display += "Shift";
                    events[num_events].sc_ = SDL_SCANCODE_LSHIFT;
                    events[num_events].sc2_ = SDL_SCANCODE_RSHIFT;
                    events[num_events].type_ = kDown;
                    ++num_events;
                } else if(CompareSubstr("alt", &bind_str[start_token], token_len)){
#ifdef __MACOSX__
                    display += "Option";
#else
                    display += "Alt";
#endif
                    events[num_events].sc_ = SDL_SCANCODE_LALT;
                    events[num_events].sc2_ = SDL_SCANCODE_RALT;
                    events[num_events].type_ = kDown;
                    ++num_events;
                } else if(CompareSubstr("ctrl", &bind_str[start_token], token_len)){
                    display += "Ctrl";
                    events[num_events].sc_ = SDL_SCANCODE_LCTRL;
                    events[num_events].sc2_ = SDL_SCANCODE_RCTRL;
                    events[num_events].type_ = kDown;
                    ++num_events;
                } else {
                    str.clear();
                    for(int j=start_token; j<=end_token; ++j){
                        str += bind_str[j];
                    }
                    if(str.length() == 1 && str[0] >= 'a' && str[0] <= 'z'){
                        display += (str[0] + ('A' - 'a'));
                    } else {
                        display += str;
                    }
                    SDL_Scancode key = StringToSDLScancode(str);
                    if(key == SDL_SCANCODE_SYSREQ){
                        FatalError("Error", "Unknown key: %s", str.c_str());
                    } else {
                        events[num_events].sc_ = key;
                        events[num_events].sc2_ = key;
                        events[num_events].type_ = kPressed;
                        ++num_events;
                    }
                }
                if(c == '+'){
                    display = display + "+";
                }
                start_token = i+1;
            }
        } else if(parse_stage == kCommand) {
            if(c == '\0'){
                while(bind_str[start_token] == ' ' || bind_str[start_token] == '\t'){
                    ++start_token;
                }
                int end_token = i-1;
                while(bind_str[end_token] == ' ' || bind_str[end_token] == '\t'){
                    --end_token;
                }
                str.clear();
                for(int j=start_token; j<=end_token; ++j){
                    str += bind_str[j];
                }
                int id = -1;
                unsigned long hash = djb2_string_hash(str.c_str());
                HashIntVector::iterator iter = std::lower_bound(hash_to_command.begin(), hash_to_command.end(), hash, HashIntCompare());
                if(iter->first != hash) {
                    LOGW << "Could not find " << str << " in hash map" << std::endl;
                } else {
                    id = iter->second;
                    Command& kc = commands[id];
                    kc.num_key_events = num_events;
                    for(int j=0; j<num_events; ++j){
                        kc.key_events[j] = events[j];
                    }
                    FormatString(kc.display_text, KeyCommand::kDisplayTextBufSize, "%s", display.c_str());
                }
            }
        }
        if(c == '\0'){
            break;
        }
        ++i;
    }
}

void AddPairToHash(const char* cstr, int id){
    hash_to_command.resize(hash_to_command.size()+1);
    HashInt &new_pair = hash_to_command.back();
    new_pair.first = djb2_string_hash(cstr);
    new_pair.second = id;
}

int CommandLengthCompare (const void* a, const void* b){
    return (*((Command**)b))->num_key_events - (*((Command**)a))->num_key_events;
}

void KeyCommand::FinalizeBindings() {
    // Prepare list of commands sorted by length (so they can be evaluated from most-specific to least)
    for(int i=0; i<kNumLabels; ++i){
        commands_sorted_by_length[i] = &commands[i];
    }
    qsort(commands_sorted_by_length, kNumLabels, sizeof(commands_sorted_by_length[0]), CommandLengthCompare);
}

void KeyCommand::Initialize() {
    // Create structure to map string hashes to command labels
    hash_to_command.reserve(kNumLabels);
    AddPairToHash("toggle_level_load_stress", kToggleLevelLoadStress);
    AddPairToHash("pause",kPause);
    AddPairToHash("refresh",kRefresh);
    AddPairToHash("quit",kQuit);
    AddPairToHash("back",kBack);
    AddPairToHash("clone_transform",kCloneTransform);
    AddPairToHash("snap_transform",kSnapTransform);
    AddPairToHash("force_rotate",kForceRotate);
    AddPairToHash("force_scale",kForceScale);
    AddPairToHash("force_translate",kForceTranslate);
    AddPairToHash("normal_transform",kNormalTransform);
    AddPairToHash("edit_script_params",kEditScriptParams);
    AddPairToHash("single_selected", kSingleSelected);
    AddPairToHash("edit_color",kEditColor);
    AddPairToHash("search_scenegraph",kSearchScenegraph);
    AddPairToHash("scenegraph",kScenegraph);
    AddPairToHash("toggle_player",kTogglePlayer);
    AddPairToHash("toggle_decal_editing",kToggleDecalEditing);
    AddPairToHash("toggle_object_editing",kToggleObjectEditing);
    AddPairToHash("toggle_hotspot_editing",kToggleHotspotEditing);
    AddPairToHash("open_spawner",kOpenSpawner);
    AddPairToHash("save_selected_items",kSaveSelectedItems);
    AddPairToHash("save_level",kSaveLevel);
    AddPairToHash("save_level_as",kSaveLevelAs);
    AddPairToHash("undo",kUndo);
    AddPairToHash("redo",kRedo);
    AddPairToHash("cut",kCut);
    AddPairToHash("paste",kPaste);
    AddPairToHash("copy",kCopy);
    AddPairToHash("enable_imposter",kEnableImposter);
    AddPairToHash("disable_imposter",kDisableImposter);
    AddPairToHash("connect",kConnect);
    AddPairToHash("disconnect",kDisconnect);
    AddPairToHash("group",kGroup);
    AddPairToHash("ungroup",kUngroup);
    AddPairToHash("box_select",kBoxSelect);
    AddPairToHash("deselect_all",kDeselectAll);
    AddPairToHash("select_all",kSelectAll);
    AddPairToHash("select_similar",kSelectSimilar);
    AddPairToHash("add_to_selection",kAddToSelection);
    AddPairToHash("bake_gi",kBakeGI);
    AddPairToHash("test1",kTest1);
    AddPairToHash("print_objects", kPrintObjects);
    AddPairToHash("kill_selected", kKillSelected);
    AddPairToHash("new_level", kNewLevel);
    AddPairToHash("open_level", kOpenLevel);
    AddPairToHash("open_custom_editor", kOpenCustomEditor);
    AddPairToHash("frame_selected", kFrameSelected);
    AddPairToHash("frame_selected_force", kFrameSelectedForce);
    AddPairToHash("load_item_search", kLoadItemSearch);
    AddPairToHash("make_selected_character_saved_corpse", kMakeSelectedCharacterSavedCorpse);
    AddPairToHash("revive_selected_character_and_unsave_corpse", kReviveSelectedCharacterAndUnsaveCorpse);

    std::sort(hash_to_command.begin(), hash_to_command.end(), HashIntCompare());

    // Make sure there are no hash collisions
    for(int i=0, len=hash_to_command.size()-1; i<len; ++i) {
        if(hash_to_command[i].first == hash_to_command[i+1].first) {
            FatalError("Error", "Hash collision in KeyCommand hash map");
        }
    }
    for(auto & command : commands){
        command.num_key_events = 0;
    }
}

void KeyCommand::HandleKeyDownEvent( const Keyboard &keyboard,  SDL_Keysym key_id ) {
    // Check for key_pressed events from longest command to shortest
    int cmd_length = -1;
    SDL_Scancode sc = key_id.scancode;
    for(auto & i : commands_sorted_by_length){
        Command &cmd = *i;
        if(cmd.num_key_events < cmd_length) {
            return;
        }
        bool cmd_valid = true;
        for(int j=0; j<cmd.num_key_events; ++j){
            const KeyEvent &key_event = cmd.key_events[j];
            bool key_valid = false;

            switch(key_event.type_){
            case kPressed:
                if( key_event.sc_ == key_event.sc2_ ) {
                    key_valid = (sc == key_event.sc_);
                } else {
                    key_valid = (sc == key_event.sc_) || (sc == key_event.sc2_);
                }
                break;
            case kDown:
                if( key_event.sc_ == key_event.sc2_ ) {
                    key_valid = keyboard.isScancodeDown(key_event.sc_,KIMF_ANY);
                } else {
                    key_valid = keyboard.isScancodeDown(key_event.sc_,KIMF_ANY) || keyboard.isScancodeDown(key_event.sc2_,KIMF_ANY);
                }
                break;
            case kUp:
                if( key_event.sc_ == key_event.sc2_ ) {
                    key_valid = !keyboard.isScancodeDown(key_event.sc_,KIMF_ANY);
                } else {
                    key_valid = !keyboard.isScancodeDown(key_event.sc_,KIMF_ANY) && !keyboard.isScancodeDown(key_event.sc2_,KIMF_ANY);
                }
                break;
            }
            if(!key_valid){
                cmd_valid = false;
                break;
            }
        }
        if(cmd_valid){
            cmd_length = cmd.num_key_events;
            cmd.state = kPressed;
        }
    }
}

void KeyCommand::ClearKeyPresses() {
    for(auto & command : commands){
        command.state = kUp;
    }
}
