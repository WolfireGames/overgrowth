//-----------------------------------------------------------------------------
//           Name: worker.cpp
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

#include <Logging/consolehandler.h>
#include <Logging/filehandler.h>
#include <Logging/logdata.h>

#include <Internal/common.h>
#include <Internal/error_response.h>

#include <Compat/processpool.h>
#include <Graphics/converttexture.h>
#include <Memory/allocation.h>

#include <cstdlib>

using std::endl;

Allocation alloc;

ErrorResponse DisplayError(const char* title, const char* contents, ErrorType type, bool allow_repetition) {
    return _continue;
}

// Replacement for the main program version of the same fucntion
void FatalError(const char* title, const char* fmt, ...) {
    static const int kBufSize = 1024;
    char err_buf[kBufSize];
    va_list args;
    va_start(args, fmt);
    VFormatString(err_buf, kBufSize, fmt, args);
    va_end(args);
    LOGF << title << "," << err_buf << endl;
    exit(10);
}

int ConvertTexture(int argc, const char* argv[]) {
    ConvertImage(argv[0], argv[1], argv[2], TextureData::Nice);
    return 0;
}

int main(int argc, char* argv[]) {
    alloc.Init();
    ConsoleHandler consoleHandler;
    LogSystem::RegisterLogHandler(
        LogSystem::info | LogSystem::warning | LogSystem::error | LogSystem::fatal,
        &consoleHandler);
    if (!ProcessPool::AmIAWorkerProcess(argc, argv)) {
        exit(1);
    }
    ProcessPool::JobMap jobs;
    jobs["ConvertTexture"] = &ConvertTexture;
    int ret = ProcessPool::WorkerProcessMain(jobs);
    LogSystem::DeregisterLogHandler(&consoleHandler);
    alloc.Dispose();
    return ret;
}
