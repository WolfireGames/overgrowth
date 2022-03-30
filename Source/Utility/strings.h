//-----------------------------------------------------------------------------
//           Name: strings.h
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

#include <Internal/integer.h>

#include <string>
#include <vector>
#include <sstream>
#include <cstring>

#define ARRLEN(arr) sizeof(arr)/sizeof(arr[0])

bool endswith(const char* str, const char* ending);
bool beginswith(const char* str, const char* begin);

inline bool strmtch( const char* f, const char* s ) {
    return strcmp(f,s) == 0;
}

inline bool strmtch( const std::string& f, const char* s ) {
    return strmtch( f.c_str(), s );
}

inline bool strmtch( const std::string& f, const std::string& s ) {
    return strmtch( f.c_str(), s.c_str() );
}

inline bool strmtch( const char* f, const std::string& s ) {
    return strmtch( f, s.c_str() );
}

inline bool strsfit( const char* str, size_t memsize ) {
    return strlen(str) < memsize;
}

bool pstrmtch( const char* first, const char* second, size_t count);

bool strcont( const char* string, const char* substring );

struct StringEqual {
    bool operator()(const char* lhs, const char* rhs) const {
        return strmtch(lhs, rhs);
    }
};

struct StringLess {
    bool operator()(const char* lhs, const char* rhs) const {
        return strcmp(lhs, rhs) < 0;
    }
};

#define SOURCE_TOO_LONG 1
#define SOURCE_IS_NULL 2
#define DESTINATION_IS_ZERO_LENGTH 3

inline int strscpy(char* dest, const char* src, size_t memsize) {
    if( memsize > 0 ) {
        if(src) {
            if(strsfit(src,memsize)) {
                strncpy(dest,src,memsize);
                return 0;
            } else {
                strncpy(dest,src,memsize);
                dest[memsize-1]='\0';
                return SOURCE_TOO_LONG;
            }
        } else {
            strncpy(dest,"",memsize);
            dest[memsize-1]='\0';
            return SOURCE_IS_NULL;
        }
    } else {
        return DESTINATION_IS_ZERO_LENGTH;
    }
}

int FindStringInArray( const char** values, size_t values_size, const char* q );
int FindStringInArray( const std::vector< std::pair< const char*, const char* > >& values, const char* q );
int FindStringInArray( const std::vector< const char* >& values, const char* q );
void UTF8InPlaceLower( char* arr );
std::string UTF8ToLower(const std::string& s);
int UTF8ToLower( char* out, size_t outsize, const char* in );
int UTF8ToUpper( char* out, size_t outsize, const char* in );
bool hasBeginning( const std::string &fullString, const std::string &beginning );
bool hasEnding (std::string const &fullString, std::string const &ending);
bool hasAnyOfEndings( const std::string &fullString, const std::vector<std::string> &endings);

int CountCharsInString(const char* str, char the_char);
int FindNthCharFromBack(const char* str, char the_char, int n);

#define SAYS_TRUE 1
#define SAYS_FALSE 0
#define SAYS_TRUE_NULL_INPUT -2
#define SAYS_TRUE_NO_MATCH -1
int saysTrue(const char* str);

const char* nullAsEmpty(const char* v);

uint32_t GetCodepointCount( const std::string &utf8in );
uint32_t GetLengthInBytesForNCodepoints( const std::string& utf8in, uint32_t codepoint_index );

#ifdef WIN32
#define NOMINMAX
#include <windows.h>
std::wstring fromUTF8( const std::string& path_utf8 );
std::string fromUTF16( const std::wstring& path_utf16 );
#endif

static std::vector<std::string>& split(const std::string &s, char delim, std::vector<std::string> &elems) 
{
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}
