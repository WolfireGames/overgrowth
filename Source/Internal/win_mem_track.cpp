//-----------------------------------------------------------------------------
//           Name: win_mem_track.cpp
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
#include "win_mem_track.h"

#include <Logging/logdata.h>
#include <Utility/assert.h>

bool mem_track_enable = false;
// Memory check functions adapted from Doom 3 source
#if defined(_DEBUG) && defined(_WIN32)
#define NOMINMAX
#include <Windows.h>
#include <crtdbg.h>

#include <cstdio>
#include <cassert>

static unsigned int debug_total_alloc = 0;
static unsigned int debug_total_alloc_count = 0;
static unsigned int debug_current_alloc = 0;
static unsigned int debug_current_alloc_count = 0;
static unsigned int debug_frame_alloc = 0;
static unsigned int debug_frame_alloc_count = 0;

typedef struct CrtMemBlockHeader {
    struct _CrtMemBlockHeader *pBlockHeaderNext;  // Pointer to the block allocated just before this one:
    struct _CrtMemBlockHeader *pBlockHeaderPrev;  // Pointer to the block allocated just after this one
    char *szFileName;                             // File name
    int nLine;                                    // Line number
    size_t nDataSize;                             // Size of user block
    int nBlockUse;                                // Type of block
    long lRequest;                                // Allocation number
    byte gap[4];                                  // Buffer just before (lower than) the user's memory:
} CrtMemBlockHeader;

int AllocHook(int nAllocType, void *pvData, size_t nSize, int nBlockUse, long lRequest, const unsigned char *szFileName, int nLine) {
    CrtMemBlockHeader *pHead;
    byte *temp;

    if (nBlockUse == _CRT_BLOCK) {
        return (TRUE);
    }

    // get a pointer to memory block header
    temp = (byte *)pvData;
    temp -= 32;
    pHead = (CrtMemBlockHeader *)temp;

    switch (nAllocType) {
        case _HOOK_ALLOC:
            if (mem_track_enable) {
                bool break_here = true;
            }
            debug_total_alloc += nSize;
            debug_current_alloc += nSize;
            debug_frame_alloc += nSize;
            debug_total_alloc_count++;
            debug_current_alloc_count++;
            debug_frame_alloc_count++;
            break;

        case _HOOK_FREE:
            LOG_ASSERT(pHead->gap[0] == 0xfd && pHead->gap[1] == 0xfd && pHead->gap[2] == 0xfd && pHead->gap[3] == 0xfd);

            debug_current_alloc -= pHead->nDataSize;
            debug_current_alloc_count--;
            debug_total_alloc_count++;
            debug_frame_alloc_count++;
            break;

        case _HOOK_REALLOC:
            LOG_ASSERT(pHead->gap[0] == 0xfd && pHead->gap[1] == 0xfd && pHead->gap[2] == 0xfd && pHead->gap[3] == 0xfd);

            debug_current_alloc -= pHead->nDataSize;
            debug_total_alloc += nSize;
            debug_current_alloc += nSize;
            debug_frame_alloc += nSize;
            debug_total_alloc_count++;
            debug_current_alloc_count--;
            debug_frame_alloc_count++;
            break;
    }
    return (TRUE);
}

void WinMemTrack::PrintTotal() {
    LOGI.Format("Total allocation %8dk in %d blocks\n", debug_total_alloc / 1024, debug_total_alloc_count);
    LOGI.Format("Current allocation %8dk in %d blocks\n", debug_current_alloc / 1024, debug_current_alloc_count);
}

void WinMemTrack::PrintFrame() {
    LOGI.Format("Frame: %8dk in %5d blocks\n", debug_frame_alloc / 1024, debug_frame_alloc_count);
    debug_frame_alloc = 0;
    debug_frame_alloc_count = 0;
}

void WinMemTrack::Attach() {
    _CrtSetAllocHook(AllocHook);
}
#else
void WinMemTrack::PrintTotal() {}
void WinMemTrack::PrintFrame() {}
void WinMemTrack::Attach() {}
#endif
