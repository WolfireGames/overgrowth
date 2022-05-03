//-----------------------------------------------------------------------------
//           Name: stopwatch.cpp
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
#include "stopwatch.h"

#include <Compat/time.h>
#include <Logging/logdata.h>
#include <Threading/sdl_wrapper.h>

#include <SDL.h>

void BusyWaitMilliseconds( uint32_t how_many )
{    
    Stopwatch watch;
    while(watch.ReportMilliseconds()<how_many){
    }
}

void Stopwatch::Start() {
    start_time = SDL_TS_GetTicks();
}

void Stopwatch::Stop(std::string text) {
    uint32_t time_elapsed = StopAndReport();
    LOGI << time_elapsed << " " << text << std::endl;
}

uint32_t Stopwatch::StopAndReport() {
    uint32_t time = SDL_TS_GetTicks();
    uint32_t time_elapsed = time - start_time;
    start_time = time;

    return time_elapsed;
}

uint32_t Stopwatch::ReportMilliseconds() {
    uint32_t time = SDL_TS_GetTicks();
    uint32_t time_elapsed = time - start_time;

    return time_elapsed;
}

void PrecisionStopwatch::Start() {
    start_time = GetPrecisionTime();
}

void PrecisionStopwatch::Stop(std::string text) {
    uint64_t time_elapsed = StopAndReportNanoseconds();
    LOGI << time_elapsed << " " <<  text << std::endl;
}

uint64_t PrecisionStopwatch::StopAndReportNanoseconds() {
    uint64_t time = GetPrecisionTime();
    uint64_t time_elapsed = time - start_time;
    start_time = time;

    return ToNanoseconds(time_elapsed);
}
