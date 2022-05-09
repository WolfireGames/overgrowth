//-----------------------------------------------------------------------------
//           Name: file_descriptor.cpp
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
#include "file_descriptor.h"

#include <Compat/fileio.h>
#include <Logging/logdata.h>

#include <cstring>

bool DiskFileDescriptor::ReadBytes(void* dst, int num_bytes) {
    int elements_read = fread(dst, num_bytes, 1, file_);
    if (ferror(file_)) {
        perror("File error:");
    }
    if (feof(file_)) {
        LOGE << "End of file" << std::endl;
    }
    return (elements_read == 1);
}

bool DiskFileDescriptor::WriteBytes(const void* src, int num_bytes) {
    return (fwrite(src, num_bytes, 1, file_) == 1);
}

bool DiskFileDescriptor::Open(const std::string& abs_path, const std::string& mode) {
    file_ = my_fopen(abs_path.c_str(), mode.c_str());
    return (file_ != NULL);
}

bool DiskFileDescriptor::Close() {
    int ret = fclose(file_);
    file_ = 0;
    return ret == 0;
}

int DiskFileDescriptor::GetSize() {
    int index = ftell(file_);
    fseek(file_, 0, SEEK_END);
    int file_size = ftell(file_);
    fseek(file_, index, SEEK_SET);
    return file_size;
}

DiskFileDescriptor::~DiskFileDescriptor() {
    if (file_) {
        fclose(file_);
    }
}

int DiskFileDescriptor::ReadBytesPartial(void* dst, int num_bytes) {
    return fread(dst, 1, num_bytes, file_);
}

bool MemReadFileDescriptor::ReadBytes(void* dst, int num_bytes) {
    if (!ptr_) {
        return false;
    }
    if (num_bytes == 0) {
        return true;
    }
    memcpy(dst, (char*)ptr_ + index_, num_bytes);
    index_ += num_bytes;
    return true;  // memcpy performs no error checking
}

bool MemWriteFileDescriptor::WriteBytes(const void* src, int num_bytes) {
    if (num_bytes == 0) {
        return true;
    }
    vec_.resize(vec_.size() + num_bytes);
    uint8_t* dst = &vec_[vec_.size() - num_bytes];
    memcpy(dst, src, num_bytes);
    return true;  // memcpy performs no error checking
}
