//-----------------------------------------------------------------------------
//           Name: compat.h
//      Developer: Wolfire Games LLC
//    Description: Compiler macros to detect the target platform
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

// Check pointer size
#if defined _M_X64 || defined _M_AMD64 || defined __amd64__ || defined __amd64 || defined __x86_64__ || defined __x86_64 || defined _WIN64 || defined __64BIT__ || defined __LP64 || defined _LP64 || defined __LP64__ || defined _ADDR64
#define PLATFORM_64 1
#else
#define PLATFORM_32 1
#endif

// Check windows
#if defined _WIN32 || defined_WIN64
#define PLATFORM_WINDOWS 1
#endif

// Check unix
#if defined __unix__
#define PLATFORM_UNIX 1
#endif

// Check Linux
#if defined linux || defined __linux
#define PLATFORM_LINUX 1
#endif

// Check macos
#if defined __APPLE__
#define PLATFORM_MACOSX 1
#endif