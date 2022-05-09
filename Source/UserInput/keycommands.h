//-----------------------------------------------------------------------------
//           Name: keycommands.h
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

#include <vector>

class Keyboard;

namespace KeyCommand {
enum EventType { kPressed,
                 kDown,
                 kUp };

// A KeyEvent is a key id and an event type
// e.g. "q was pressed this frame" or "alt key is currently down"
struct KeyEvent {
    EventType type_;
    SDL_Scancode sc_;
    SDL_Scancode sc2_;
    KeyEvent(EventType type, SDL_Scancode key_id) : type_(type), sc_(key_id) {}
    KeyEvent(){};
};

const int kMaxKeyEvents = 4;
const int kDisplayTextBufSize = 32;

// A command is a number of key events that must be simultaneously occurring
struct Command {
    char display_text[kDisplayTextBufSize];
    KeyEvent key_events[kMaxKeyEvents];
    int num_key_events;
    EventType state;

    Command();
};

// Loop through each KeyEvent in the KeyCommand, and return false if any of
// the necessary inputs are not reported by the keyboard, otherwise true
bool CheckPressed(const Keyboard& keyboard, int label, const uint32_t kimf);
bool CheckDown(const Keyboard& keyboard, int label, const uint32_t kimf);

const char* GetDisplayText(int label);

void HandleKeyDownEvent(const Keyboard& keyboard, SDL_Keysym key_id);
void ClearKeyPresses();

// Set up all of the default key commands
void Initialize();

enum Label {
    kQuit,
    kBack,
    kCloneTransform,
    kSnapTransform,
    kForceTranslate,
    kForceScale,
    kForceRotate,
    kNormalTransform,
    kEditScriptParams,
    kSingleSelected,
    kEditColor,
    kSearchScenegraph,
    kScenegraph,
    kTogglePlayer,
    kToggleDecalEditing,
    kToggleObjectEditing,
    kToggleHotspotEditing,
    kOpenSpawner,
    kSaveSelectedItems,
    kSaveLevel,
    kSaveLevelAs,
    kUndo,
    kRedo,
    kCut,
    kPaste,
    kCopy,
    kEnableImposter,
    kDisableImposter,
    kConnect,
    kDisconnect,
    kGroup,
    kUngroup,
    kBoxSelect,
    kDeselectAll,
    kSelectAll,
    kSelectSimilar,
    kAddToSelection,
    kBakeGI,
    kTest1,
    kKillSelected,
    kPrintObjects,
    kPause,
    kRefresh,
    kToggleLevelLoadStress,
    kNewLevel,
    kOpenLevel,
    kFrameSelected,
    kFrameSelectedForce,
    kLoadItemSearch,
    kMakeSelectedCharacterSavedCorpse,
    kReviveSelectedCharacterAndUnsaveCorpse,
    kOpenCustomEditor,

    kNumLabels
};

void BindString(const char* bind_str);
void FinalizeBindings();
}  // namespace KeyCommand
