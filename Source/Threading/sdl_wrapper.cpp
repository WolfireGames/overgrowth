//-----------------------------------------------------------------------------
//           Name: sdl_wrapper.cpp
//      Developer: Wolfire Games LLC
//         Author: Max Danielsson
//    Description: This is a threadsafe wrapper around functions in SDL known
//                 to be used from mutiple threads.
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
#include "sdl_wrapper.h"

#include <SDL.h>

#include <mutex>

static std::mutex GetTicks_mutex;

unsigned int SDL_TS_GetTicks() {
    unsigned int v = 0;
    GetTicks_mutex.lock();
    v = SDL_GetTicks();
    GetTicks_mutex.unlock();
    return v;
}
