//-----------------------------------------------------------------------------
//           Name: profiler.h
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

class StackAllocator;

#if defined(OGDA) || defined(OG_WORKER)

#define PROFILER_ENTER(x,y) do{}while(0)
#define PROFILER_LEAVE(x) do{}while(0)
#define PROFILER_ZONE(x,y) do{}while(0)

struct ProfilerContext {}; // TODO: handle better?

#else 

// for figuring out why profiler leaks zones
// only tested on Linux...
#define PROFILER_DEBUG 0

#if PROFILER_DEBUG

    #undef NTELEMETRY
    #define NTELEMETRY 1

    struct ProfilerContext {
        void Init(StackAllocator* stack_allocator) {}
        void Dispose(StackAllocator* stack_allocator) {}
    };

    struct ProfilerScopedZone {
        ProfilerContext *ctx;
        void *pointer;
        unsigned int expected_depth;

        ProfilerScopedZone(ProfilerContext *ctx_, const char *msg, ...);
        ~ProfilerScopedZone();
    };

    void profileEnter(ProfilerContext *ctx, const char *msg, ...);
    void profileLeave(ProfilerContext *ctx);


    #define MERGE_HELPER(a,b) a##b
    #define MERGE(a,b) MERGE_HELPER(a,b)

    #define PROFILER_ENTER(ctx, ...)                profileEnter(ctx, __VA_ARGS__);
    #define PROFILER_LEAVE(ctx)                     profileLeave(ctx);
    #define PROFILER_ZONE(ctx, ...)                 ProfilerScopedZone MERGE(p, __LINE__) (ctx, __VA_ARGS__);
    #define PROFILER_ZONE_STALL(ctx, str)           ProfilerScopedZone MERGE(p, __LINE__) (ctx,  str);
    #define PROFILER_ZONE_IDLE(ctx, str)            ProfilerScopedZone MERGE(p, __LINE__) (ctx,  str);
    #define PROFILER_GPU_ZONE(ctx, ...)             ProfilerScopedZone MERGE(p, __LINE__) (ctx, __VA_ARGS__);
    #define PROFILER_ENTER_STALL_THRESHOLD(ctx, match_id_ptr, u_sec, str) profileEnter(ctx, str);
    #define PROFILER_LEAVE_THRESHOLD(ctx, match_id) profileLeave(ctx);
    #define PROFILER_ENTER_DYNAMIC_STRING(ctx, str) profileEnter(ctx, str);
    #define PROFILER_ZONE_DYNAMIC_STRING(ctx, str)  ProfilerScopedZone MERGE(p, __LINE__) (ctx, str);
    #define PROFILER_NAME_TIMELINE(ctx, str)
    #define PROFILER_NAME_THREAD(ctx, str)
    #define PROFILER_TICK(ctx)

    #define PROFILED_TEXTURE_MUTEX_LOCK \
        texture_mutex.lock();
    
    #define PROFILED_TEXTURE_MUTEX_UNLOCK \
        texture_mutex.unlock();

#elif GPU_MARKERS

    #undef NTELEMETRY
    #define NTELEMETRY 1

    struct ProfilerContext {
        void Init(StackAllocator* stack_allocator) {}
        void Dispose(StackAllocator* stack_allocator) {}
    };

    struct ProfilerScopedGPUZone {
        ProfilerScopedGPUZone(const char *msg, ...);
        ~ProfilerScopedGPUZone();
    };

    #define MERGE_HELPER(a,b) a##b
    #define MERGE(a,b) MERGE_HELPER(a,b)

    #define PROFILER_ENTER(ctx, ...)
    #define PROFILER_LEAVE(ctx)
    #define PROFILER_ZONE(ctx, ...)
    #define PROFILER_ZONE_STALL(ctx, str)
    #define PROFILER_ZONE_IDLE(ctx, str)
    #define PROFILER_GPU_ZONE(ctx, ...) ProfilerScopedGPUZone MERGE(p, __LINE__) (__VA_ARGS__);
    #define PROFILER_ENTER_STALL_THRESHOLD(ctx, match_id_ptr, u_sec, str)
    #define PROFILER_LEAVE_THRESHOLD(ctx, match_id)
    #define PROFILER_ENTER_DYNAMIC_STRING(ctx, str)
    #define PROFILER_ZONE_DYNAMIC_STRING(ctx, str)
    #define PROFILER_NAME_TIMELINE(ctx, str)
    #define PROFILER_NAME_THREAD(ctx, str)
    #define PROFILER_TICK(ctx)

    #define PROFILED_TEXTURE_MUTEX_LOCK \
        texture_mutex.lock();

    #define PROFILED_TEXTURE_MUTEX_UNLOCK \
        texture_mutex.unlock();

#elif !defined(NTELEMETRY)
    #include <telemetry.h>

    class ProfilerContext {
    public:
        static const int kMemSize = 2*1024*1024;
        HTELEMETRY tm_context;
        void* memory;
        const char* err_str;

        void Init(StackAllocator* stack_allocator);
        void Dispose(StackAllocator* stack_allocator);
    };

    #define PROFILER_ENTER(ctx, ...) tmEnter((ctx)->tm_context, TMZF_NONE, __VA_ARGS__)
    #define PROFILER_LEAVE(ctx) tmLeave((ctx)->tm_context)
    #define PROFILER_ZONE(ctx, ...) tmZone((ctx)->tm_context, TMZF_NONE, __VA_ARGS__)
    #define PROFILER_ZONE_STALL(ctx, str) tmZone((ctx)->tm_context, TMZF_STALL, (str))
    #define PROFILER_ZONE_IDLE(ctx, str) tmZone((ctx)->tm_context, TMZF_IDLE, (str))
    #define PROFILER_GPU_ZONE(ctx, ...) tmZone((ctx)->tm_context, TMZF_NONE, __VA_ARGS__)
    #define PROFILER_ENTER_STALL_THRESHOLD(ctx, match_id_ptr, u_sec, str) tmEnterEx((ctx)->tm_context, (match_id_ptr), 0, (u_sec), __FILE__, __LINE__, TMZF_STALL, (str))
    #define PROFILER_LEAVE_THRESHOLD(ctx, match_id) tmLeaveEx( (ctx)->tm_context, (match_id), 0,  __FILE__, __LINE__ )
    #define PROFILER_ENTER_DYNAMIC_STRING(ctx, str) tmEnter((ctx)->tm_context, TMZF_NONE, "%s", tmDynamicString((ctx)->tm_context, (str)))
    #define PROFILER_ZONE_DYNAMIC_STRING(ctx, str) tmZone((ctx)->tm_context, TMZF_NONE, "%s", tmDynamicString((ctx)->tm_context, (str)))
    #define PROFILER_NAME_TIMELINE(ctx, str) tmSetTimelineSectionName((ctx)->tm_context, (str))
    #define PROFILER_NAME_THREAD(ctx, str) tmThreadName((ctx)->tm_context, 0, (str))
    #define PROFILER_TICK(ctx) tmTick((ctx)->tm_context);

    #define PROFILED_TEXTURE_MUTEX_LOCK \
        TmU64 matchid;\
        PROFILER_ENTER_STALL_THRESHOLD(g_profiler_ctx, \
            &matchid, 500, "texture mutex");\
        texture_mutex.lock();\
        PROFILER_LEAVE_THRESHOLD(g_profiler_ctx, matchid);

    #define PROFILED_TEXTURE_MUTEX_UNLOCK \
        texture_mutex.unlock();

#else
    struct ProfilerContext {
        void Init(StackAllocator* stack_allocator) {}
        void Dispose(StackAllocator* stack_allocator) {}
    };

    #define PROFILER_ENTER(ctx, ...)
    #define PROFILER_LEAVE(ctx)
    #define PROFILER_ZONE(ctx, ...)
    #define PROFILER_ZONE_STALL(ctx, str)
    #define PROFILER_ZONE_IDLE(ctx, str)
    #define PROFILER_GPU_ZONE(ctx, ...)
    #define PROFILER_ENTER_STALL_THRESHOLD(ctx, match_id_ptr, u_sec, str)
    #define PROFILER_LEAVE_THRESHOLD(ctx, match_id)
    #define PROFILER_ENTER_DYNAMIC_STRING(ctx, str)
    #define PROFILER_ZONE_DYNAMIC_STRING(ctx, str)
    #define PROFILER_NAME_TIMELINE(ctx, str)
    #define PROFILER_NAME_THREAD(ctx, str)
    #define PROFILER_TICK(ctx)

    #define PROFILED_TEXTURE_MUTEX_LOCK \
        texture_mutex.lock();

    #define PROFILED_TEXTURE_MUTEX_UNLOCK \
        texture_mutex.unlock();

#endif

#endif

extern ProfilerContext* g_profiler_ctx;
