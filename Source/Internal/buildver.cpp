//-----------------------------------------------------------------------------
//           Name: global_config.h
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

// This is in a seperate file so that we can recompile it every time
//  without it forcing a recompile on something ccache would otherwise not
//  have to rebuild...this file's checksum changes every time you build it
//  due to the __DATE__ and __TIME__ macros.

// The makefile will rebuild this file everytime it relinks an executable
//  so that we'll always have a unique build string.

// APPNAME and APPREV need to be predefined in the build system.
//  The rest are supposed to be supplied by the compiler.

#ifndef APPID
#define APPID UnknownAPPID
#endif

#ifndef APPREV
#define APPREV UnknownAPPREV
#endif

#ifndef __VERSION__
#define __VERSION__ (Unknown compiler version)
#endif

#ifndef __DATE__
#define __DATE__ (Unknown build date)
#endif

#ifndef __TIME__
#define __TIME__ (Unknown build time)
#endif

#ifndef COMPILER
#if (defined __GNUC__)
#define COMPILER "GCC"
#elif (defined _MSC_VER)
#define COMPILER "Visual Studio"
#else
#error Please define your platform.
#endif
#endif

// macro mess so we can turn APPID and APPREV into a string literal...
#define MAKEBUILDVERSTRINGLITERAL2(id, rev) \
#id ", Revision " #rev ", Built " __DATE__ " " __TIME__ ", by " COMPILER " version " __VERSION__

#define MAKEBUILDVERSTRINGLITERAL(id, rev) MAKEBUILDVERSTRINGLITERAL2(id, rev)

const char *GBuildVer = MAKEBUILDVERSTRINGLITERAL(APPID, APPREV);

// end of buildver.cpp ...
