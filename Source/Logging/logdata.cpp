//-----------------------------------------------------------------------------
//           Name: logdata.cpp
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

#include <Logging/logdata.h>
#include <Logging/loghandler.h>

#include <Utility/strings.h>

#include <sstream>
#include <iomanip>
#include <mutex>

#include <cassert>
#include <cstdio>
#include <cstdarg>

using std::mutex;
using std::stringstream;
using std::string;
using std::strcmp;
using std::strcpy;
using std::strlen;
using std::strncpy;
using std::setw;

LogSystem::HandlerInstance LogSystem::handlers[LOGGER_LIMIT];

char ignoreList[1024*8] = { 0 };

LogTypeMask LogSystem::enabledLogTypes = 0;

static mutex logMutex;

void LogSystem::Mute( const char* prefix )
{
	logMutex.lock();
	{
		stringstream ss;
		ss << ignoreList;

		string tt;
		while( ss.rdbuf()->in_avail() != 0 )
		{
			ss >> tt;
			if( strcmp( prefix, tt.c_str() ) == 0 )
			{
				logMutex.unlock();
				return;
			}
		}
	}

	stringstream ss;
	ss << prefix << " " << ignoreList;

	string msg = ss.str();
	strcpy( ignoreList, msg.c_str() );

	logMutex.unlock();
}

void LogSystem::Unmute( const char* prefix )
{
	logMutex.lock();

	stringstream ss;
	stringstream out;
	ss << ignoreList;

	string tt;
	while( ss.rdbuf()->in_avail() != 0 )
	{
		ss >> tt;
		if( strcmp( prefix, tt.c_str() ) != 0 )
			out << tt;
	}

	string msg = out.str();
	strcpy( ignoreList, msg.c_str() );

	logMutex.unlock();
}

void LogSystem::RegisterLogHandler( LogTypeMask types, LogHandler* newHandler )
{
	logMutex.lock();

    bool worked = false;

    //Disable if enabled first.
    enabledLogTypes = 0;
    for(auto & handler : handlers)
    {
        if( handler.handler == newHandler )
        {
            handler.types = 0U;
            handler.handler = NULL;
        } else if (handler.handler != NULL) {
            enabledLogTypes |= handler.types;
        }
    }

    //Then enable.
    for(auto & handler : handlers)
    {
        if( handler.handler == NULL )
        {
            handler.types = types;
            handler.handler = newHandler;
            worked = true;
            break;
        } 
    }

    enabledLogTypes |= types;

    assert( worked );

	logMutex.unlock();
}

void LogSystem::DeregisterLogHandler( LogHandler* newHandler )
{
	logMutex.lock();

    enabledLogTypes = 0;
    for(auto & handler : handlers)
    {
        if( handler.handler == newHandler )
        {
            handler.types = 0U;
            handler.handler = NULL;
        } else if (handler.handler != NULL) {
            enabledLogTypes |= handler.types;
        }
    }

	logMutex.unlock();
}

void LogSystem::Flush()
{
    for(auto & handler : handlers)
    {
        if( handler.handler != NULL)
        {
            handler.handler->Flush();
        }
    }
}

LogSystem::LogData& operator<< ( const LogSystem::LogData& data, StandardEndLine obj )
{
	LogSystem::LogData& temp = (LogSystem::LogData&)data; // unix hack, nab compiler...

	if ((temp.m_type & LogSystem::enabledLogTypes) == 0) {
		return temp;
	}

    stringstream ss;
    obj( ss );
    
    string msg = ss.str();

    if( temp.m_message_pos + msg.size() < MESSAGE_LENGTH )
    {
        strcpy( temp.m_message + temp.m_message_pos, msg.c_str() );
    }
    else if( temp.m_message_pos < (MESSAGE_LENGTH - 1))
    {
        strncpy( temp.m_message + temp.m_message_pos, msg.c_str(), MESSAGE_LENGTH - temp.m_message_pos );
    }
    temp.m_message_pos += msg.length();

	return temp;
}

LogSystem::LogData& operator<< ( const LogSystem::LogData& data, const string& obj )
{
	LogSystem::LogData& temp = (LogSystem::LogData&)data; // unix hack, nab compiler...

	if ((temp.m_type & LogSystem::enabledLogTypes) == 0) {
		return temp;
	}

    if( temp.m_message_pos + obj.length() < MESSAGE_LENGTH )
    {
        strcpy( temp.m_message + temp.m_message_pos, obj.c_str() );
    }
    else if( temp.m_message_pos < (MESSAGE_LENGTH - 1))
    {
        strncpy( temp.m_message + temp.m_message_pos, obj.c_str(), MESSAGE_LENGTH - temp.m_message_pos );
    }
    temp.m_message_pos += obj.length();

    return temp;
}

LogSystem::LogData& operator<< ( const LogSystem::LogData& data, const char* obj )
{
	LogSystem::LogData& temp = (LogSystem::LogData&)data; // unix hack, nab compiler...

	if ((temp.m_type & LogSystem::enabledLogTypes) == 0) {
		return temp;
	}

    if( obj == NULL ) {
        obj = "NULL";
    }

    size_t objlen = strlen( obj );

    if( temp.m_message_pos + objlen < MESSAGE_LENGTH )
    {
        strcpy( temp.m_message + temp.m_message_pos, obj );
    }
    else if( temp.m_message_pos < (MESSAGE_LENGTH - 1))
    {
        strncpy( temp.m_message + temp.m_message_pos, obj, MESSAGE_LENGTH - temp.m_message_pos );
    }
    temp.m_message_pos += objlen;
    
    return temp;
}

LogSystem::LogData::LogData(LogType type, const char* prefix, const char* filename, int line )
{
    const char *end = filename + strlen( filename );    

#ifdef PLATFORM_UNIX
	while( end != NULL && *(end-1) != '/' && end > filename )
#else
	while( end != NULL && *(end-1) != '\\' && end > filename )
#endif
        end--;

    strscpy(m_filename, end, FILENAME_LENGTH);
	strscpy(m_prefix, prefix, PREFIX_LENGTH);
    m_line = line;
	m_type = type;
	m_message[0] = '\0';
    m_message_pos = 0;
}

LogSystem::LogData::LogData( const LogData& old )
{
    strncpy(m_filename, old.m_filename, FILENAME_LENGTH);
	strncpy(m_prefix, old.m_prefix, PREFIX_LENGTH);
    m_line = old.m_line;
	m_type = old.m_type;

    m_message[0] = '\0';
    m_message_pos = 0;
}

LogSystem::LogData::~LogData()
{
	if( m_message_pos == 0 )
		return;

	logMutex.lock();

	if (ignoreList[0] != '\0')
	{
		stringstream ss;
		ss << ignoreList;

		string tt;
		while( ss.rdbuf()->in_avail() != 0 )
		{
			ss >> tt;
			if( strcmp( m_prefix, tt.c_str() ) == 0 )
			{
				logMutex.unlock();
				return;
			}
		}
	}
	
	stringstream ss;

	     switch( m_type )
         {
            case LogSystem::spam:
                ss << "[s]";
                break;
            case LogSystem::debug:
                ss << "[d]";
                break;
            case LogSystem::info:
                ss << "[i]";
                break;
            case LogSystem::warning:
                ss << "[w]";
                break;
            case LogSystem::error:
                ss << "[e]";
                break;
            case LogSystem::fatal:
                ss << "[f]";
                break;
            default:
                ss << "[u]";
                break;
         }

	ss << "[" << setw( 2 ) << m_prefix << "]:";

    if( strlen( m_filename ) > 0 )
        ss << setw( 15 ) << m_filename << ":";

    if( m_line > 0 )
        ss << setw( 4 ) << m_line << ": ";

    m_message[MESSAGE_LENGTH-2] = '\n';
    m_message[MESSAGE_LENGTH-1] = '\0';

	string msg = ss.str();

    for(auto & handler : handlers)
    {
        if( handler.handler != NULL && 
            (m_type & handler.types) )
        {
            handler.handler->Log( m_type, m_line, m_filename, m_prefix, msg.c_str(), m_message );
        }
    }

	logMutex.unlock();
}

void LogSystem::LogData::Format( const char* fmt, ... )
{
    char buf[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, 2048, fmt, args);
    va_end(args);

    *this << buf;
}

LogSystem::LogVoid& operator<< ( const LogSystem::LogVoid& data, StandardEndLine obj )
{
	LogSystem::LogVoid& temp = (LogSystem::LogVoid&)data; // unix hack, nab compiler...

	return temp;
}

LogSystem::LogVoid& operator<< ( const LogSystem::LogVoid& data, const string& obj )
{
	LogSystem::LogVoid& temp = (LogSystem::LogVoid&)data; // unix hack, nab compiler...

    return temp;
}

LogSystem::LogVoid& operator<< ( const LogSystem::LogVoid& data, const char* obj )
{
	LogSystem::LogVoid& temp = (LogSystem::LogVoid&)data; // unix hack, nab compiler...
    
    return temp;
}

LogSystem::LogVoid::LogVoid( )
{
}

LogSystem::LogVoid::LogVoid( const LogVoid& old )
{
}

LogSystem::LogVoid::~LogVoid()
{
}

void LogSystem::LogVoid::Format( const char* fmt, ... )
{
}
