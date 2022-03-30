//-----------------------------------------------------------------------------
//           Name: input_state.as
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

#include "serializer.as"

enum Input {
      Roll // 0
    , Flip // 1
    , JumpOffWall // 2
    , Dodging // 2
    , Feint // 3 
    , FlipOffWall // 4
    , ThrowEnemy // 5
    , RollFromRagDoll // 6
    , StartActiveBlock // 7
    , AccelerateJump // 8
    , Attack // 9
    , CancelAnimation // 10
    , CounterThrow // 11
    , Crouch // 12
    , Drag // 13
    , DropItem // 14 
    , GrabLedge // 15
    , Jump // 16
    , PickUpItem // 17
    , SheatheItem // 18
    , ThroatCut // 21
    , ThrowItem // 22
    , Unsheathe // 23
    , WalkUp // 24
    , WalkDown // 25
    , WalkRight // 26
    , WalkLeft // 27
    , InputCount = WalkLeft + 1
};


class InputState {  
    uint32       inputs;
    array<bool> currentInput;
    InputState() { 
        currentInput = array<bool>(InputCount, false);
        clearInputCache();
    }

    void clearInputCache() { 
        for (int i = 0; i < InputCount; i++) {
            currentInput[i] = false;
        }
    }

    bool IsInput(Input i) { 
        if (currentInput[i]) {
            return true;
        } else if ((inputs & (1 << i)) != 0) {
            currentInput[i] = true;
            inputs = inputs & (~(1 << i));
            return true;
        } 
        return false;
    }


    void SetInput(Input i, bool on) {
        if (on) {
            inputs = inputs |  (1 << i);

        } else {
            inputs = inputs & ~(1 << i);
        }

        currentInput[i] = on;
    }
};
