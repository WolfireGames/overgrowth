//-----------------------------------------------------------------------------
//           Name: consolehandler.cpp
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

#include <iostream>

using std::cout;
using std::cerr;
using std::flush;

ConsoleHandler::ConsoleHandler()
{
}

ConsoleHandler::~ConsoleHandler()
{
}

void ConsoleHandler::Log( LogSystem::LogType type, int row, const char* filename, const char* cat, const char* message_prefix, const char* message )
{

	/*
   #ifdef _WIN32

   switch( m_type )
   {
   case LogSystem::LogType::debug :
   system( "Color 0A" );
   break;

   case LogSystem::LogType::error :
   system( "Color 04" );
   break;

   case LogSystem::LogType::fatal :
   system( "Color 0C" );
   break;

   case LogSystem::LogType::warning :
   system( "Color 0E" );
   break;

   }
   // c fatal
   // a debug
   // e warning

   #endif
   */

#if defined(PLATFORM_LINUX)

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
	switch( type )
	{
	case LogSystem::spam :
        cout << KNRM;
		break;

	case LogSystem::debug :
		break;

	case LogSystem::error :
		cout << KRED;
		break;

	case LogSystem::fatal :
		cout << KRED;
		break;

	case LogSystem::warning :
		cout << KYEL;
		break;

	case LogSystem::info :
		cout << KGRN;
		break;
	}

    cout << message_prefix;
	cout << message;

    cout << KNRM;
#elif defined(PLATFORM_UNIX)
    cout << message_prefix;
	cout << message;
#else
    //We restrict output on windows because slow console.
	if (type != LogSystem::debug && type != LogSystem::spam )
	{
		//Using fprint here because std::err and std::out doesn't function.
		fprintf( stderr, "%s", message_prefix );
		fprintf( stderr, "%s", message );
	}
#endif
}

void ConsoleHandler::Flush()
{
    flush(cerr);
}
