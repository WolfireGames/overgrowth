//-----------------------------------------------------------------------------
//           Name: fileio.h
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

#include <stdio.h>
#include <fstream>

using std::wstring;
using std::string;
using std::fstream;
using std::ifstream;
using std::ofstream;
using std::ios_base;

#ifdef _WIN32
wstring UTF16fromUTF8( const string& path_utf8 );
string UTF8fromUTF16( const wstring& path_utf16);
#endif
FILE* my_fopen(const char* path, const char* mode);
void my_fstream_open(fstream &file, const string& path, ios_base::openmode mode);
void my_ifstream_open(ifstream &file, const string& path, ios_base::openmode mode = ios_base::in);
void my_ofstream_open(ofstream &file, const string& path, ios_base::openmode mode = ios_base::out);
