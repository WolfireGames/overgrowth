//-----------------------------------------------------------------------------
//           Name: dialogues.h
//      Developer: Wolfire Games LLC
//    Description: This is a simple wrapper for displaying save/load dialogue boxes
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

namespace Dialog {
enum DialogErr {
    NO_ERR,
    NO_SELECTION,
    INTERNAL_BUFFER_TOO_SMALL,
    USER_BUFFER_TOO_SMALL,
    GET_CWD_FAILED,
    UNKNOWN_ERR
};

inline const char* DialogErrString( DialogErr e )
{
    switch( e )
    {
        case NO_ERR: return "NO_ERR";
        case NO_SELECTION: return "NO_SELECTION";
        case INTERNAL_BUFFER_TOO_SMALL: return "INTERNAL_BUFFER_TOO_SMALL";
        case USER_BUFFER_TOO_SMALL: return "USER_BUFFER_TOO_SMALL";
        case GET_CWD_FAILED: return "GET_CWD_FAILED";
        case UNKNOWN_ERR: return "UNKNOWN_ERR";
        default: return "(no string correspondent)";
    }
}

void Initialize();
DialogErr readFile( const char* extension, int extension_count, const char* initial_dir, char *path_buffer, int PATH_BUFFER_SIZE);
DialogErr writeFile( const char* extension, int extension_count, const char* initial_dir, char *path_buffer, int PATH_BUFFER_SIZE);
}
