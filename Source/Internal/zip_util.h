//-----------------------------------------------------------------------------
//           Name: zip_util.h
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

#include <string>

enum OverwriteType {
    _NO_OVERWRITE,
    _YES_OVERWRITE,
    _APPEND_OVERWRITE
};

struct ExpandedZipEntry;
class ExpandedZipFile {
    char *buf;
    char *filename_buf;
    ExpandedZipEntry *entries;
    unsigned num_entries;

public:
    ExpandedZipFile();
    ~ExpandedZipFile();
    void Dispose();
    void ResizeEntries(unsigned num_entries);
    void ResizeFilenameBuffer(unsigned num_chars);
    void ResizeDataBuffer(unsigned num_bytes);
    void SetFilename(unsigned offset, const char* data, unsigned size);
    void SetData(unsigned offset, const char* data, unsigned size);
    void SetEntry(unsigned which, unsigned file_name_offset, unsigned data_offset, unsigned size);
    void GetEntry( unsigned which, const char* &filename, const char* &data, unsigned &size );
    unsigned GetNumEntries(){ return num_entries; }
};

void Zip(const std::string &src_file_path, const std::string &zip_file_path, const std::string &in_zip_file_path, OverwriteType overwrite);
void UnZip(const std::string &zip_file_path, ExpandedZipFile &expanded_zip_file);
void PrintZipFileInfo(const std::string &zip_file_path);
