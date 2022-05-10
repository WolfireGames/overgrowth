//-----------------------------------------------------------------------------
//           Name: time.cpp
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
#include "time.h"
#include <SDL.h>

uint64_t GetPrecisionTime() {
    return SDL_GetPerformanceCounter();
}

uint64_t ToNanoseconds(uint64_t time) {
    uint64_t ticksPerSecond = SDL_GetPerformanceFrequency();

    // Multiply with 1e9 to get nanoseconds
    return time * 1e9 / ticksPerSecond;
}
