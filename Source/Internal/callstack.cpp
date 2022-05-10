//-----------------------------------------------------------------------------
//           Name: callstack.cpp
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
#include <Memory/allocation.h>
/*
 *  callstack.cpp
 *  PhoenixMac
 *
 *  Created by handley on 5/21/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "callstack.h"

#ifdef __APPLE__
#include <stdio.h>
#include <execinfo.h>
#include <memory.h>

// mac os implementation borrowed from cocoadev
uintptr_t *GetCallstack() {
    void *backtraceFrames[128];
    int frameCount = backtrace(&backtraceFrames[0], 128);

    uintptr_t *callstack = (uintptr_t *)OG_MALLOC(sizeof(uintptr_t) * (frameCount + 1));
    memcpy(callstack, backtraceFrames, sizeof(void *) * frameCount);
    callstack[frameCount] = 0;
    return callstack;
}
#endif
