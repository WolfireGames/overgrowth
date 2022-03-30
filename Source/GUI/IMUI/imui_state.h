//-----------------------------------------------------------------------------
//           Name: imui_state.h
//      Developer: Wolfire Games LLC
//    Description: Internal helper class for creating adhoc GUIs as part of the UI tools  
//        License: Read below
//-----------------------------------------------------------------------------
//
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

#include <GUI/IMUI/im_support.h>

/*******************************************************************************************/
/**
 * @brief The current state of the GUI passed during update
 *
 */
struct GUIState
{
    vec2 mousePosition;
    IMUIContext::ButtonState leftMouseState;
    bool inheritedMouseOver;
    bool inheritedMouseDown;
    IMUIContext::ButtonState inheritedMouseState;
    bool clickHandled;
    
    GUIState() :
        inheritedMouseOver(false),
        inheritedMouseDown(false),
        clickHandled(false)
    {}
    
};

