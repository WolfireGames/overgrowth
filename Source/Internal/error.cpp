//-----------------------------------------------------------------------------
//           Name: error.cpp
//      Developer: Wolfire Games LLC
//         Author: David Rosen
//    Description: This is a simple wrapper for displaying error messages
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
#include "error.h"

#include <Internal/config.h>
#include <Internal/common.h>
#include <Internal/modloading.h>

#include <UserInput/input.h>
#include <Logging/logdata.h>
#include <Graphics/graphics.h>
#include <Threading/thread_sanity.h>
#include <Compat/os_dialogs.h>
#include <Utility/stacktrace.h>

#include <map>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>

extern Config config;

void FatalError(const char* title, const char* fmt, ...) {
    static const int kBufSize = 1024;
    char err_buf[kBufSize];
    va_list args;
    va_start(args, fmt);
    VFormatString(err_buf, kBufSize, fmt, args);
    va_end(args);
    DisplayError(title, err_buf, _ok);
    LOGF << "Showing fatal message: " << title << "," << err_buf << std::endl;
    LOGF << "Shutting down after fatal error" << std::endl;
    LogSystem::Flush();
    _exit(1);
}

ErrorResponse DisplayFormatError(ErrorType type,
                           bool allow_repetition,
                           const char* title,
                           const char* fmtcontents, 
                           ... ) {
    static const int kBufSize = 2048;
    char err_buf[kBufSize];
    va_list args;
    va_start(args, fmtcontents);
    VFormatString(err_buf, kBufSize, fmtcontents, args);
    va_end(args);
    return DisplayError(title, err_buf, type, allow_repetition);
}

struct ErrorDisplay {
    int id;
    std::string title;
    std::string pretext;
    std::string contents;
    ErrorType type;
};

std::mutex error_queue_mutex;
static int error_id_counter = 1;
std::vector<ErrorDisplay> error_queue;
std::map<int,ErrorResponse> error_return;

std::mutex display_last_queued_error_mutex;

ErrorResponse DisplayLastQueuedError() {
    ErrorResponse response = _continue;

    display_last_queued_error_mutex.lock();

    ErrorDisplay ed;
    ed.id = 0;
    ed.type = _ok;
    bool show_error = false;

    error_queue_mutex.lock();
    if( error_queue.size() > 0 ) {
        ed = error_queue[0];
        error_queue.erase(error_queue.begin());
        show_error = true;
    }

    error_queue_mutex.unlock();

    std::stringstream ss;

    ss << ed.pretext << std::endl << ed.contents;

    if( show_error ) {
        response = OSDisplayError(
                        ed.title.c_str(),
                        ss.str().c_str(),
                        ed.type
        );

        switch( response ) {
        case _er_exit:
            LOGI.Format("\"Cancel\" chosen, shutting down program.");
            LogSystem::Flush();
            _exit(1);
            break;
        case _retry:
            LOGI.Format("\"Retry\" chosen");
            break;
        case _continue:
            LOGI.Format("\"Continue\" chosen");
            break;
        }
    }

    error_queue_mutex.lock();
    error_return[ed.id] = response;
    error_queue_mutex.unlock();

    display_last_queued_error_mutex.unlock();
    return response;
}

static int PushDisplayError(ErrorDisplay& ed) {
    int id;
    error_queue_mutex.lock();
    id = error_id_counter++;
    ed.id = id;
    error_queue.push_back(ed);
    error_queue_mutex.unlock();
    return id;
}

ErrorResponse WaitResponseForDisplayError(int error_id) {
    ErrorResponse ret;
    bool result = false;
    while(result == false) {
        error_queue_mutex.lock();
        result = (error_return.find(error_id) != error_return.end());
        error_queue_mutex.unlock();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    error_queue_mutex.lock();
    ret = error_return[error_id];
    error_return.erase(error_return.find(error_id));
    error_queue_mutex.unlock();
    return ret;
}

std::mutex display_error_mutex;
std::map<std::string, int> error_message_history;

ErrorResponse DisplayError(const char* title, const char* contents, ErrorType type, bool allow_repetition) {

    display_error_mutex.lock();

    bool no_log = false;
    if(type == _ok_no_log){
        no_log = true;
        type = _ok;
    }

    if( !no_log ) {  
        LOGI << "Displaying message: " << title << ", " << contents << std::endl;
        LOGI << GenerateStacktrace() << std::endl;
    }

    if( config["no_dialogues"].toBool() )
    {
        display_error_mutex.unlock();
        return _continue;
    }

    if(!allow_repetition && error_message_history[contents]) {
        display_error_mutex.unlock();
        return _continue;
    }

    error_message_history[contents]++;

    std::stringstream modlist;

    bool active_mods = false;
    int active_count = 0;
    std::vector<ModInstance*> mods = ModLoading::Instance().GetAllMods();
    for( unsigned i = 0; i < mods.size(); i++ ) {
        if( mods[i]->IsActive() && mods[i]->IsCore() == false) {
            if( active_mods == false ) {
                modlist << "Following mods are active" << std::endl;
                active_mods = true;
            } else {
                if( (active_count % 5) == 0 ) {
                    modlist << "," << std::endl;
                } else {
                    modlist << ", ";
                }
            }
            modlist << mods[i]->id;
            active_count++;
        }
    }
    modlist << std::endl << "Before reporting, see if disabling mods makes a difference and include this info." << std::endl;
     
    ErrorDisplay ed;

    ed.title = title;
    if( active_mods ) {
        ed.pretext = modlist.str();
    }
    ed.contents = contents;
    ed.type = type;

    int error_id = PushDisplayError(ed);
    
    ErrorResponse response;

//On windows, showing this dialogue on the non-main window seems fine. (not sure i like it though, might wanna not do this, it's helpful for stacktraces in visual studio however).
#if PLATFORM_WINDOWS
    response = DisplayLastQueuedError();
#else
    if( IsMainThread() ) {
        response = DisplayLastQueuedError();
    } else {
        response = WaitResponseForDisplayError(error_id);
    }
#endif
    
    display_error_mutex.unlock();

    return response;
}

void DisplayFormatMessage(const char* title,
                           const char* fmtcontents,
                           ... ) {
    static const int kBufSize = 2048;
    char mess_buf[kBufSize];
    va_list args;
    va_start(args, fmtcontents);
    VFormatString(mess_buf, kBufSize, fmtcontents, args);
    va_end(args);
    return DisplayMessage(title, mess_buf);
}

void DisplayMessage(const char* title,
                    const char* contents) {
    display_error_mutex.lock();

    if( config["no_dialogues"].toBool() )
    {
        display_error_mutex.unlock();
        return;
    }

    ErrorDisplay ed;

    ed.title = title;
    ed.contents = contents;
    ed.type = _ok;

    int error_id = PushDisplayError(ed);

    ErrorResponse response;

//On windows, showing this dialogue on the non-main window seems fine. (not sure i like it though, might wanna not do this, it's helpful for stacktraces in visual studio however).
#if PLATFORM_WINDOWS
    response = DisplayLastQueuedError();
#else
    if( IsMainThread() ) {
        response = DisplayLastQueuedError();
    } else {
        response = WaitResponseForDisplayError(error_id);
    }
#endif

    display_error_mutex.unlock();
}
