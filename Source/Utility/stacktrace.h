//-----------------------------------------------------------------------------
//           Name: stacktrace.h
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
#include <sstream>

#include <Compat/platform.h>
#if PLATFORM_LINUX || PLATFORM_MACOSX
#include <Memory/allocation.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <execinfo.h>

#include <csignal>
#include <ctime>
#include <cstdlib>
#include <cstdio>
inline std::string GenerateStacktrace()
{
    void *array[20];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 20);

    char ** backtrace = backtrace_symbols(array, size);

    std::stringstream ss;
    for( size_t i = 0; i < size; i++ )
    {
        ss << backtrace[i] << std::endl; 
    }
    
    OG_FREE(backtrace);

    return ss.str();
}
#elif defined PLATFORM_WINDOWS && defined OG_DEBUG
#define NOMINMAX
#include <Windows.h>
#include <DbgHelp.h>

#include <sstream>
#include <algorithm>

inline std::string GenerateStacktrace()
{
    const int maxCallers = 62;

    void* callerStack[maxCallers];
    unsigned short frames = RtlCaptureStackBackTrace(0, maxCallers, callerStack, NULL);

    IMAGEHLP_LINE64 line;
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    HANDLE handle = GetCurrentProcess();

    std::stringstream ss;

    const static unsigned short MAX_CALLERS_SHOWN = 24;
    frames = std::min(frames, MAX_CALLERS_SHOWN);
    for (int i = 0; i < frames; ++i)
    {
        DWORD whyMicrosoft;
        if (SymGetLineFromAddr64(handle, (DWORD64)callerStack[i], &whyMicrosoft, &line))
        {
            std::string filename(line.FileName);
            filename = filename.substr(filename.find_last_of("/\\") + 1);
            filename = filename.substr(0, filename.find_last_of('.'));

            ss << line.FileName << "(" << line.LineNumber << ")" << std::endl;
        }
        else
            ss << "Couldn't get line numbers. Possibly an external lib " << std::endl;
    }

    return ss.str();
}
#else
inline std::string GenerateStacktrace()
{
    return std::string("No stacktrace available on this platform");
}
#endif
