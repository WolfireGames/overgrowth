//-----------------------------------------------------------------------------
//           Name: logdata.h
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

#include <iostream>
#include <ostream>
#include <sstream>
#include <cstring>

#include "Utility/compiler_macros.h"
#include <Compat/platform.h>

#define LOGGER_LIMIT 4

#define MESSAGE_LENGTH 2048 * 4

#define FILENAME_LENGTH 64
#define PREFIX_LENGTH 3

class LogHandler;

/*!
        Namespace for the logging system and overridden by macro to "logger"
        Use is "logger::youAction", eg.
        logger::debug << "this is a debug message" << std::endl;
*/

typedef unsigned LogTypeMask;

namespace LogSystem {

extern LogTypeMask enabledLogTypes;

/*!
        Channels for output by the log system.
*/
enum LogType {
    spam = 1U << 0,
    debug = 1U << 1,
    info = 1U << 2,
    warning = 1U << 3,
    error = 1U << 4,
    fatal = 1U << 5
};

struct HandlerInstance {
    LogTypeMask types;
    LogHandler* handler;
};

/*!
        Container object for one "<<" chain. Should only be used via macros.
*/
class LogData {
   public:
    /*!
            \param type is which channel to use.  \param prefix is what prefix to use. Should be set via macro. Messages can be muted via this prefix.  */
    LogData(LogType type, const char* prefix, const char* filename, int line);

    /*!
        Copy constructor
    */
    LogData(const LogData&);

    /*!
            Once the LogData object is destroyed the printing goes into
            action using other utility in the LogSystem namespace.
    */
    ~LogData();

    // the parameter indexes in the atttribute are strange because
    // class methods have an implicit this pointer
    void Format(const char* fmt, ...) __attribute__((format(printf, 2, 3)));

    const char* GetPrefix() { return m_prefix; }

    char m_message[MESSAGE_LENGTH];
    size_t m_message_pos;

    LogType m_type;

   private:
    char m_prefix[PREFIX_LENGTH];
    char m_filename[FILENAME_LENGTH];
    int m_line;
};

class LogVoid {
   public:
    /*!
            \param type is which channel to use.  \param prefix is what prefix to use. Should be set via macro. Messages can be muted via this prefix.  */
    LogVoid();

    /*!
        Copy constructor
    */
    LogVoid(const LogVoid&);

    /*!
            Once the LogData object is destroyed the printing goes into
            action using other utility in the LogSystem namespace.
    */
    ~LogVoid();

    // the parameter indexes in the atttribute are strange because
    // class methods have an implicit this pointer
    void Format(const char* fmt, ...) __attribute__((format(printf, 2, 3)));

    const char* GetPrefix() { return ""; }
};

extern HandlerInstance handlers[];

/*!
        \param prefix to be muted, same one as stated in the macro function for that channel. eg. "debug"
*/
void Mute(const char* prefix);

/*!
        \param prefix to be unmuted, same one as stated in the macro function for that channel. eg. "debug"
*/
void Unmute(const char* prefix);

/*!
Not that you can only register one handler pointer once, but you can do so on all channels
*/
void RegisterLogHandler(LogTypeMask types, LogHandler* newHandler);

void DeregisterLogHandler(LogHandler* newHandler);

void Flush();
}  // namespace LogSystem

typedef std::basic_ostream<char, std::char_traits<char> > CoutType;
typedef CoutType& (*StandardEndLine)(CoutType&);

LogSystem::LogData& operator<<(const LogSystem::LogData& data, StandardEndLine obj);
LogSystem::LogData& operator<<(const LogSystem::LogData& data, const std::string& obj);
LogSystem::LogData& operator<<(const LogSystem::LogData& data, const char* obj);

template <class T>
LogSystem::LogData& operator<<(const LogSystem::LogData& data, const T& obj) {
    LogSystem::LogData& temp = (LogSystem::LogData&)data;  // unix hack, nab compiler...

    if ((temp.m_type & LogSystem::enabledLogTypes) == 0) {
        return temp;
    }

    std::stringstream ss;
    ss << obj;

    std::string msg = ss.str();

    if (temp.m_message_pos + msg.length() < MESSAGE_LENGTH) {
        std::strcpy(temp.m_message + temp.m_message_pos, msg.c_str());
    } else if (temp.m_message_pos < (MESSAGE_LENGTH - 1)) {
        std::strncpy(temp.m_message + temp.m_message_pos, msg.c_str(), MESSAGE_LENGTH - temp.m_message_pos);
    }

    temp.m_message_pos += msg.length();

    return temp;
}

LogSystem::LogVoid& operator<<(const LogSystem::LogVoid& data, StandardEndLine obj);
LogSystem::LogVoid& operator<<(const LogSystem::LogVoid& data, const std::string& obj);
LogSystem::LogVoid& operator<<(const LogSystem::LogVoid& data, const char* obj);

template <class T>
LogSystem::LogVoid& operator<<(const LogSystem::LogVoid& data, const T& obj) {
    LogSystem::LogVoid& temp = (LogSystem::LogVoid&)data;  // unix hack, nab compiler...
    return temp;
}

#define LOGF LogSystem::LogVoid()
#define LOGE LogSystem::LogVoid()
#define LOGW LogSystem::LogVoid()
#define LOGI LogSystem::LogVoid()
#define LOGD LogSystem::LogVoid()
#define LOGS LogSystem::LogVoid()

#if LOG_LEVEL > 0
#undef LOGF
#define LOGF LogSystem::LogData(LogSystem::fatal, "__", __FILE__, __LINE__)
#endif

#if LOG_LEVEL > 1
#undef LOGE
#define LOGE LogSystem::LogData(LogSystem::error, "__", __FILE__, __LINE__)
#endif

#if LOG_LEVEL > 2
#undef LOGW
#define LOGW LogSystem::LogData(LogSystem::warning, "__", __FILE__, __LINE__)
#endif

#if LOG_LEVEL > 3
#undef LOGI
#define LOGI LogSystem::LogData(LogSystem::info, "__", __FILE__, __LINE__)
#endif

#if LOG_LEVEL > 4
#undef LOGD
#define LOGD LogSystem::LogData(LogSystem::debug, "__", __FILE__, __LINE__)
#endif

#if LOG_LEVEL > 5
#undef LOGS
#define LOGS LogSystem::LogData(LogSystem::spam, "__", __FILE__, __LINE__)
#endif

#define LOGW_ONCE(message)                \
    do {                                  \
        static bool once = true;          \
        if (once) {                       \
            LOGW << message << std::endl; \
            once = false;                 \
        }                                 \
    } while (false)
