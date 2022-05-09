//-----------------------------------------------------------------------------
//           Name: profiler.cpp
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
#include "profiler.h"

#include <Internal/error.h>
#include <Memory/stack_allocator.h>
#include <Logging/logdata.h>

#include <stdlib.h>
#include <stdio.h>

#if PROFILER_DEBUG
#include <cassert>
#include <cstdarg>
#include <string>
#include <vector>

#define GPUMARKBUFSIZE 512

struct ProfStack {
    std::vector<std::string> messages;
};

__thread unsigned int stack_depth = 0;
__thread ProfStack *prof_stack = NULL;

class SanityCheck {
   public:
    SanityCheck() {
        assert(stack_depth == 0);
    }

    ~SanityCheck() {
        assert(stack_depth == 0);
    }
} san_check;

void profileEnter(ProfilerContext *ctx, const char *msg, va_list args) {
    if (prof_stack == NULL) {
        prof_stack = new ProfStack;
    }
    assert(prof_stack != NULL);

    char buf[GPUMARKBUFSIZE];
    int ret = vsnprintf(buf, GPUMARKBUFSIZE, msg, args);
    // We want to crash if this fails
    assert(ret >= 0);
    if (ret >= GPUMARKBUFSIZE) {
        ret = GPUMARKBUFSIZE - 1;
    }
    prof_stack->messages.push_back(std::string(buf, ret));

    stack_depth++;
}

void profileEnter(ProfilerContext *ctx, const char *msg, ...) {
    va_list args;

    va_start(args, msg);
    profileEnter(ctx, msg, args);
    va_end(args);
}

void profileLeave(ProfilerContext *ctx) {
    assert(prof_stack != NULL);

    prof_stack->messages.pop_back();

    assert(stack_depth > 0);
    stack_depth--;
}

ProfilerScopedZone::ProfilerScopedZone(ProfilerContext *ctx_, const char *msg, ...)
    : ctx(ctx_), expected_depth(stack_depth) {
    va_list args;

    va_start(args, msg);
    profileEnter(ctx, msg, args);
    va_end(args);
}

ProfilerScopedZone::~ProfilerScopedZone() {
    assert(stack_depth == expected_depth + 1);

    profileLeave(ctx);
}

#elif !defined(NTELEMETRY)

void ProfilerContext::Init(StackAllocator* stack_allocator) {
    memory = stack_allocator->Alloc(ProfilerContext::kMemSize);
    if (!memory) {
        FatalError("Error",
                   "Memory allocation error in file %s : %d", __FILE__, __LINE__);
    }
    tmLoadTelemetry(TM_LOAD_CHECKED_LIBRARY);  // Load telemetry dll
    if (TM_OK != tmStartup()) {
        FatalError("Error",
                   "Could not start up Telemetry -- "
                   "check if DLL is in correct place");
    }
    if (TM_OK != tmInitializeContext(
                     &tm_context, memory, ProfilerContext::kMemSize)) {
        FatalError("Error", "Could not initialize Telemetry context");
    }
    if (TM_OK != tmOpen(
                     tm_context, "Overgrowth", __DATE__ " " __TIME__,
                     "localhost", TMCT_TCP, TELEMETRY_DEFAULT_PORT,
                     TMOF_DEFAULT | TMOF_INIT_NETWORKING, 1000)) {
        LOGE << "Could not open Telemetry -- check if server is open" << std::endl;
    }
    tmThreadName(tm_context, 0, "Main thread");
}

void ProfilerContext::Dispose(StackAllocator* stack_allocator) {
    tmClose(tm_context);
    tmShutdownContext(tm_context);
    if (TM_OK != tmShutdown()) {
        FatalError("Error", "Could not shutdown telemetry");
    }
    stack_allocator->Free(memory);
}

#elif GPU_MARKERS

#include <Internal/snprintf.h>
#include <Threading/thread_sanity.h>

#include <opengl.h>

#include <cassert>
#include <cstdarg>

#define GPUMARKBUFSIZE 512

ProfilerScopedGPUZone::ProfilerScopedGPUZone(const char *msg, ...) {
    AssertMainThread();

    if (GLAD_GL_KHR_debug) {
        char buf[GPUMARKBUFSIZE];
        va_list args;

        va_start(args, msg);

        int ret = vsnprintf(buf, GPUMARKBUFSIZE, msg, args);
        va_end(args);

        // We want to crash if this fails
        // If we want to continue we'd have to keep track of whether this succeeded to avoid GL errors
        assert(ret >= 0);

        if (ret < GPUMARKBUFSIZE) {
            buf[ret] = '\0';
        } else {
            buf[GPUMARKBUFSIZE - 1] = '\0';
        }

        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, buf);
    }
}

ProfilerScopedGPUZone::~ProfilerScopedGPUZone() {
    AssertMainThread();

    if (GLAD_GL_KHR_debug) {
        glPopDebugGroup();
    }
}

#endif  // NTELEMETRY
