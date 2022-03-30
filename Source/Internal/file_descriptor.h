//-----------------------------------------------------------------------------
//           Name: file_descriptor.h
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
#include <cstdio>
#include <vector>
#include <stdint.h>

class FileDescriptor {
public:
    // Read num_bytes bytes into memory at dst
    // Returns false on error
    virtual bool ReadBytes(void* dst, int num_bytes)=0;
    // Write num_bytes bytes into file from src
    // Returns false on error
    virtual bool WriteBytes(const void* src, int num_bytes)=0;

    virtual ~FileDescriptor() {}
};

class DiskFileDescriptor : public FileDescriptor {
public:
    // See FileDescriptor::ReadBytes
    virtual bool ReadBytes(void* dst, int num_bytes);
    int ReadBytesPartial(void* dst, int num_bytes);
    // See FileDescriptor::WriteBytes
    virtual bool WriteBytes(const void* src, int num_bytes);
    // Open file at filename (UTF8 path) with given mode (e.g. r,w,rb,wb)
    bool Open(const std::string& filename, const std::string& mode);
    bool Close();
    int GetSize();
    DiskFileDescriptor():file_(NULL){}
    ~DiskFileDescriptor();
private:
    FILE* file_;
};

class MemReadFileDescriptor : public FileDescriptor {
public:
    // See FileDescriptor::ReadBytes
    virtual bool ReadBytes(void* dst, int num_bytes);
    // Cannot write to this descriptor
    virtual bool WriteBytes(const void* src, int num_bytes){return false;}
    MemReadFileDescriptor(void* ptr = NULL)
        :ptr_(ptr), index_(0) {}
private:
    void *ptr_;
    int index_;
};

class MemWriteFileDescriptor : public FileDescriptor {
public:
    // Cannot read from this descriptor
    virtual bool ReadBytes(void* dst, int num_bytes){return false;}
    // See FileDescriptor::WriteBytes
    virtual bool WriteBytes(const void* src, int num_bytes);
    MemWriteFileDescriptor(std::vector<uint8_t> &vec):vec_(vec){}
private:
    std::vector<uint8_t> &vec_;
};

