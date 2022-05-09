//-----------------------------------------------------------------------------
//           Name: editor_types.h
//      Developer: Wolfire Games LLC
//    Description:
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

namespace EditorTypes {
enum Tool {
    ADD_ONCE,
    TRANSLATE,
    SCALE,
    ROTATE,
    OMNI,
    NO_TOOL,
    CONNECT,
    DISCONNECT
};

inline const char* GetToolString(Tool e) {
    switch (e) {
        case ADD_ONCE:
            return "ADD_ONCE";
        case TRANSLATE:
            return "TRANSLATE";
        case SCALE:
            return "SCALE";
        case ROTATE:
            return "ROTATE";
        case OMNI:
            return "OMNI";
        case NO_TOOL:
            return "NO_TOOL";
        case CONNECT:
            return "CONNECT";
        case DISCONNECT:
            return "DISCONNECT";
        default:
            return "(unknown tool type)";
    }
}
}  // namespace EditorTypes
