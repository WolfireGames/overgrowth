//-----------------------------------------------------------------------------
//           Name: os_dialogs_win.cpp
//      Developer: Wolfire Games LLC
//         Author: David Rosen
//    Description: This is a simple wrapper for displaying error messages
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

#include <Compat/os_dialogs.h>
#include <Logging/logdata.h>
#include <UserInput/input.h>
#include <Internal/config.h>

#define NOMINMAX
#include <windows.h>

#include <map>
#include <string>


ErrorResponse OSDisplayError(const char* title, const char* contents, ErrorType type) {
    bool old_mouse = Input::Instance()->GetGrabMouse();
    Input::Instance()->SetGrabMouse(false);
    UIShowCursor(1);

    int msgboxID;
    switch (type) {
        case _ok_cancel_retry:
            msgboxID = MessageBox(NULL,
                                  contents, 
                                  title, 
                                  MB_CANCELTRYCONTINUE | 
                                  MB_DEFBUTTON2 | 
                                  MB_ICONHAND | 
                                  MB_TOPMOST | 
                                  MB_SETFOREGROUND);
            break;    
        case _ok_cancel:
            msgboxID = MessageBox(NULL,
                                  contents, 
                                  title, 
                                  MB_OKCANCEL | 
                                  MB_DEFBUTTON1 | 
                                  MB_ICONHAND | 
                                  MB_TOPMOST | 
                                  MB_SETFOREGROUND);
            break;    
        case _ok:
            msgboxID = MessageBox(NULL,
                                  contents, 
                                  title, 
                                  MB_DEFBUTTON1 | 
                                  MB_ICONHAND | 
                                  MB_TOPMOST | 
                                  MB_SETFOREGROUND);
            break;    
    }
    switch (msgboxID) {
        case IDCANCEL:
            return _er_exit;
            break;
        case IDTRYAGAIN:
        case IDRETRY:
            Input::Instance()->SetGrabMouse(old_mouse);
            UIShowCursor(0);
            return _retry;
            break;
        case IDCONTINUE:
        case IDOK:
            Input::Instance()->SetGrabMouse(old_mouse);
            UIShowCursor(0);
            return _continue;
            break;
    }
    return _continue;
}
