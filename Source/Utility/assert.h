//-----------------------------------------------------------------------------
//           Name: assert.h
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

#include <Logging/logdata.h>

#include <cassert>

#if FATAL_LOG_ASSERTS

#define LOG_ASSERT_MSG(value,message) if(!(value)){LOGF << "Failed assert \"" #value "\""  << " Message: " <<  message << std::endl;} assert(value)

#define LOG_ASSERT(value) if(!(value)){LOGF << "Failed assert \"" #value "\"" << std::endl;} assert(value)
#define LOG_ASSERT2(value, actual) if(!(value)){LOGF << "Failed assert \"" #value "\"" << ". Actual " #actual << actual << std::endl;} assert(value)

#define LOG_ASSERT_EQ(v1,v2) if(!(v1==v2)){LOGF << "Failed assert: \"" #v1 "(value: " << v1 << ")" << " == " << #v2 << "(value:" << v2 << ")\"" << std::endl;} assert(v1==v2)

#define LOG_ASSERT_LT(v1,v2) if(!(v1<v2)){LOGF << "Failed assert: \"" #v1 "(value: " << v1 << ")" << " < " << #v2 << "(value:" << v2 << ")\"" << std::endl;} assert(v1<v2)
#define LOG_ASSERT_GT(v1,v2) if(!(v1>v2)){LOGF << "Failed assert: \"" #v1 "(value: " << v1 << ")" << " > " << #v2 << "(value:" << v2 << ")\"" << std::endl;} assert(v1>v2)

#define LOG_ASSERT_LTEQ(v1,v2) if(!(v1<=v2)){LOGF << "Failed assert: \"" #v1 "(value: " << v1 << ")" << " <= " << #v2 << "(value:" << v2 << ")\"" << std::endl;} assert(v1<=v2)
#define LOG_ASSERT_GTEQ(v1,v2) if(!(v1>=v2)){LOGF << "Failed assert: \"" #v1 "(value: " << v1 << ")" << " >= " << #v2 << "(value:" << v2 << ")\"" << std::endl;} assert(v1>=v2)

#elif SINGLE_SHOT_ASSERTS

#define LOG_ASSERT_MSG(value,message) if(!(value)){static bool print = true; if(print){LOGF << "Failed assert \"" #value "\""  << " Message: " <<  message << " Will mute repeating errors." << std::endl; print = false;}}

#define LOG_ASSERT(value) if(!(value)){static bool print = true; if(print){LOGF << "Failed assert \"" #value "\" Will mute repeating errors." << std::endl; print = false;}}
#define LOG_ASSERT2(value, actual) if(!(value)){static bool print = true; if(print){LOGF << "Failed assert \"" #value "\"" << ". Actual " #actual << actual << " Will mute repeating errors." << std::endl; print = false;}}

#define LOG_ASSERT_EQ(v1,v2) if(!(v1==v2)){static bool print = true; if(print){LOGF << "Failed assert: \"" #v1 "(value: " << v1 << ")" << " == " << #v2 << "(value:" << v2 << ")\" Will mute repeating errors." << std::endl; print = false;}} 

#define LOG_ASSERT_LT(v1,v2) if(!(v1<v2)){static bool print = true; if(print){LOGF << "Failed assert: \"" #v1 "(value: " << v1 << ")" << " < " << #v2 << "(value:" << v2 << ")\" Will mute repeating errors." << std::endl; print = false;}}
#define LOG_ASSERT_GT(v1,v2) if(!(v1>v2)){static bool print = true; if(print){LOGF << "Failed assert: \"" #v1 "(value: " << v1 << ")" << " > " << #v2 << "(value:" << v2 << ")\" Will mute repeating errors." << std::endl; print = false;}}

#define LOG_ASSERT_LTEQ(v1,v2) if(!(v1<=v2)){static bool print = true; if(print){LOGF << "Failed assert: \"" #v1 "(value: " << v1 << ")" << " <= " << #v2 << "(value:" << v2 << ")\" Will mute repeating errors." << std::endl; print = false;}}
#define LOG_ASSERT_GTEQ(v1,v2) if(!(v1>=v2)){static bool print = true; if(print){LOGF << "Failed assert: \"" #v1 "(value: " << v1 << ")" << " >= " << #v2 << "(value:" << v2 << ")\" Will mute repeating errors." << std::endl; print = false;}}

#else

#define LOG_ASSERT_MSG(value,message) if(!(value)){LOGF << "Failed assert \"" #value "\""  << " Message: " <<  message << std::endl;}

#define LOG_ASSERT(value) if(!(value)){LOGF << "Failed assert \"" #value "\"" << std::endl;}
#define LOG_ASSERT2(value, actual) if(!(value)){LOGF << "Failed assert \"" #value "\"" << ". Actual " #actual << actual << std::endl;}

#define LOG_ASSERT_EQ(v1,v2) if(!(v1==v2)){LOGF << "Failed assert: \"" #v1 "(value: " << v1 << ")" << " == " << #v2 << "(value:" << v2 << ")\"" << std::endl;} 

#define LOG_ASSERT_LT(v1,v2) if(!(v1<v2)){LOGF << "Failed assert: \"" #v1 "(value: " << v1 << ")" << " < " << #v2 << "(value:" << v2 << ")\"" << std::endl;}
#define LOG_ASSERT_GT(v1,v2) if(!(v1>v2)){LOGF << "Failed assert: \"" #v1 "(value: " << v1 << ")" << " > " << #v2 << "(value:" << v2 << ")\"" << std::endl;}

#define LOG_ASSERT_LTEQ(v1,v2) if(!(v1<=v2)){LOGF << "Failed assert: \"" #v1 "(value: " << v1 << ")" << " <= " << #v2 << "(value:" << v2 << ")\"" << std::endl;}
#define LOG_ASSERT_GTEQ(v1,v2) if(!(v1>=v2)){LOGF << "Failed assert: \"" #v1 "(value: " << v1 << ")" << " >= " << #v2 << "(value:" << v2 << ")\"" << std::endl;}

#endif

#define LOG_ASSERT_ONCE(value) if(!(value)){static bool print = true; if(print){LOGF << "Failed assert \"" #value "\" Will mute repeating errors." << std::endl; print = false;}}
