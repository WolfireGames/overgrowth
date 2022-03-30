//-----------------------------------------------------------------------------
//           Name: thread_sanity.cpp
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
#include "thread_sanity.h"

#include <Logging/logdata.h>

#include <thread>
#include <cassert>

using std::thread;
using std::endl;

static thread::id main_thread_id;

//Record which thread is the main one so that we can assert that we are in the main thread going forward.
void RegisterMainThreadID()
{
    main_thread_id = std::this_thread::get_id();
}

bool AssertMainThread()
{
    if( main_thread_id != std::this_thread::get_id() )
    {
        LOGE << "It appears a thread has entered a code block that is limited only to the main thread" << endl;
        assert(false);
        return false;
    }
    return true;
}


bool IsMainThread()
{
    return main_thread_id == std::this_thread::get_id();
}
