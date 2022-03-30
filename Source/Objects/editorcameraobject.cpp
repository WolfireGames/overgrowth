//-----------------------------------------------------------------------------
//           Name: editorcameraobject.h
//      Developer: Wolfire Games LLC
//         Author: Phillip Isola
//    Description: Camera object for the editors
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

#include <Objects/editorcameraobject.h>
#include <Logging/logdata.h>
#include <Internal/common.h>

void EditorCameraObject::IgnoreInput(bool val) {
    //ignore_input = val;
    frozen = val;
#ifdef _DEBUG
    if (val) LOGI << "Ignoring camera input." << std::endl;
    else LOGI << "No longer ignoring camera input." << std::endl;
#endif
}

void EditorCameraObject::IgnoreMouseInput(bool val) {
    //ignore_input = val;
    ignore_mouse_input = val;
}

void EditorCameraObject::GetDisplayName(char* buf, int buf_size) {
    if( GetName().empty() ) {
        FormatString(buf, buf_size, "%d, Editor Camera", GetID());
    } else {
        FormatString(buf, buf_size, "%s, Editor Camera", GetName().c_str());
    }
}

