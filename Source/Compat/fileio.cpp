//-----------------------------------------------------------------------------
//           Name: fileio.cpp
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
#include "fileio.h"

#include <Internal/filesystem.h>
#include <Internal/casecorrectpath.h>

#ifndef NO_ERR
#include <Internal/error.h>
#endif

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

using std::string;
using std::wstring;
using std::ios_base;
using std::fstream;
using std::ifstream;
using std::ofstream;

#ifdef _WIN32
wstring UTF16fromUTF8( const string& path_utf8 ) {
    int size = MultiByteToWideChar(CP_UTF8, 0, path_utf8.c_str(), -1, NULL, 0);
    wstring path_utf16;
    path_utf16.resize(size);
    MultiByteToWideChar(CP_UTF8, 0, path_utf8.c_str(), -1, &path_utf16[0], size);
    return path_utf16;
}

string UTF8fromUTF16( const wstring& path_utf16) {
	int size = WideCharToMultiByte(CP_UTF8, 0, path_utf16.c_str(), -1, NULL, 0, NULL, NULL );
	string output;
	output.resize(size);
	WideCharToMultiByte(CP_UTF8, 0, path_utf16.c_str(), -1, &output[0], size, NULL, NULL );
	return output;
}
#endif

FILE* my_fopen( const char* abs_path, const char* mode ) {
#ifdef _WIN32
    FILE* file = _wfopen(UTF16fromUTF8(abs_path).c_str(), UTF16fromUTF8(mode).c_str());
#else
    FILE* file = fopen(abs_path, mode);
#endif
    if(!file){
        // If writing, make sure the necessary directories exist
        if(mode[0] == 'w'){
            CreateParentDirs(abs_path);
#ifdef _WIN32
            file = _wfopen(UTF16fromUTF8(abs_path).c_str(), UTF16fromUTF8(mode).c_str());
#else
            file = fopen(abs_path, mode);
#endif
        } else {
            return NULL;
        }
    }
    if(!file){
        return NULL;
    }
    return file;
}

void my_fstream_open( fstream &file, const string& path, ios_base::openmode mode) {
#ifdef _WIN32
	file.open(UTF16fromUTF8(path).c_str(), mode);
#else
	file.open(path.c_str(), mode);
#endif
}

void my_ifstream_open( ifstream &file, const string& path, ios_base::openmode mode /*= ios_base::in*/ ) {
#ifdef _WIN32
	file.open(UTF16fromUTF8(path).c_str(), mode);
#else
	file.open(path.c_str(), mode);
#endif
}

void my_ofstream_open( ofstream &file, const string& path, ios_base::openmode mode /*= ios_base::out*/ ) {
#ifdef _WIN32
    file.open(UTF16fromUTF8(path).c_str(), mode);
#else
    file.open(path.c_str(), mode);
#endif
}
