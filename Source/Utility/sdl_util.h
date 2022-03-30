//-----------------------------------------------------------------------------
//           Name: sdl_util.h
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

#include <SDL.h>

static const char* SDL_GLprofile_string( SDL_GLprofile v ) {
        switch( v ) {
            case SDL_GL_CONTEXT_PROFILE_CORE:
                return "SDL_GL_CONTEXT_PROFILE_CORE";
            case SDL_GL_CONTEXT_PROFILE_COMPATIBILITY:
                return "SDL_GL_CONTEXT_PROFILE_COMPATIBILITY";
            case SDL_GL_CONTEXT_PROFILE_ES:
                return "SDL_GL_CONTEXT_PROFILE_ES";
            default:
                return "SDL_GL_CONTEXT_PROFILE_UNKNOWN";
        }
}
