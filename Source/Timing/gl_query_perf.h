//-----------------------------------------------------------------------------
//           Name: gl_query_perf.h
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

#ifdef TIMER_QUERY_TIMING
#include <Compat/fileio.h>
#include <Internal/integer.h>
#include <Utility/disallow_copy_and_assign.h>

#include <vector>
#include <map>
#include <queue>
#include <string>
#include <opengl.h>
#include <stack>
#include <set>

/*
 * This class creates a structure around using the GL_ARB_timer_query extension
 * avaiable in OpenGL 3.2+
 */

struct PerfQuery {
    int line;
    GLuint frame;
    const char* file;
    GLuint query_id;
};

class GLTimerQueryPerf {
    static const size_t MAX_QUERY_COUNT = 2048 * 16;
    bool perf_available;

    bool begun_perf;
    GLuint frame_counter;
    GLuint query_ids[MAX_QUERY_COUNT];
    GLuint query_ids_used;

    PerfQuery queries[MAX_QUERY_COUNT];
    GLuint query_count;

    std::set<const char*> file_names;

    std::ofstream csv_output;

   public:
    void Init();
    void Finalize();
    void PerfGPUBegin(const char* file, const int line);
    void PerfGPUEnd();
    void PostFrameSwap();
};

extern GLTimerQueryPerf* glTimingQuery;

#define GL_TIMER_QUERY_INIT() \
    glTimingQuery->Init()

#define GL_TIMER_QUERY_DISPOSE() \
    glTimingQuery->Dispose()

#define GL_TIMER_QUERY_START() \
    if (glTimingQuery) glTimingQuery->PerfGPUBegin(__FILE__, __LINE__)

#define GL_TIMER_QUERY_END() \
    if (glTimingQuery) glTimingQuery->PerfGPUEnd()

#define GL_TIMER_QUERY_SWAP() \
    glTimingQuery->PostFrameSwap()

#define GL_TIMER_QUERY_FINALIZE() \
    glTimingQuery->Finalize()

#else

#define GL_TIMER_QUERY_INIT()

#define GL_TIMER_QUERY_START()

#define GL_TIMER_QUERY_END()

#define GL_TIMER_QUERY_SWAP()

#define GL_TIMER_QUERY_FINALIZE()

#endif
