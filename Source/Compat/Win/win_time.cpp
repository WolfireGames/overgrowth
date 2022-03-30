//-----------------------------------------------------------------------------
//           Name: win_time.cpp
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
#include <Compat/time.h>

#define NOMINMAX
#include <windows.h>

uint64_t GetPrecisionTime() {
    LARGE_INTEGER tick;
    QueryPerformanceCounter(&tick);
    return (uint64_t)tick.QuadPart;
}

uint64_t ToNanoseconds(uint64_t time){
    LARGE_INTEGER ticksPerSecond;
    QueryPerformanceFrequency(&ticksPerSecond);
    
    time *= 1000000000;
    time /= ticksPerSecond.QuadPart;

    return time;
}
