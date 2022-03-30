//-----------------------------------------------------------------------------
//           Name: integer.h
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

#include <limits>

#if defined(_MSC_VER) && _MSC_VER < 0x1000 //check for windows versions less than vs2010

#include <limits.h>

typedef signed __int8		int8_t;
typedef signed __int16		int16_t;
typedef signed __int32		int32_t;
typedef signed __int64		int64_t;
typedef unsigned __int8		uint8_t;
typedef unsigned __int16	uint16_t;
typedef unsigned __int32	uint32_t;
typedef unsigned __int64	uint64_t;

#ifdef _WIN64
	typedef signed __int64		intptr_t;
	typedef unsigned __int64	uintptr_t;
#else
	typedef signed __int32		intptr_t;
	typedef unsigned __int32	uintptr_t;
#endif

#else
#include <stdint.h>
#endif

#ifdef max
#undef max
#endif

#define UINT32MAX std::numeric_limits<uint32_t>::max()
