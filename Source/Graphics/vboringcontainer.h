//-----------------------------------------------------------------------------
//           Name: vboringcontainer.h
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
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

#include <Internal/referencecounter.h>
#include <Internal/integer.h>

#include <Graphics/vboenums.h>

#include <opengl.h>

#ifndef V_MIBIBYTE
#define V_MIBIBYTE 1024*1024
#endif

#ifndef V_KIBIBYTE
#define V_KIBIBYTE 1024
#endif

class VBORingContainer {
    int gl_VBO;
    char flags;
    bool element;
    bool force_reload;

    GLuint current_offset;
    GLuint next_offset;
    //Used space (requested by last loaded buffer) plus alignment padding
    GLuint allocated_size;
    //Amount of space requested to use for last filled buffer
    GLuint used_size;
    
    //Total size of entire ring buffer
    GLuint storage_size;
    GLuint storage_size_hint;

    GLenum target;
    GLenum hint;

    int storage_multiplier;
public:
    void Dispose();
    void SetHint( GLuint storage_size, char flags, bool ignore_multiplier = false );
    void Fill(GLuint size, void* data);
    void Bind() const;
    bool valid() const;
    unsigned size(); //returns size in bytes.
    uintptr_t offset() const;
    
    VBORingContainer( GLuint storage_size, char flags, bool ignore_multiplier = false );
    ~VBORingContainer();
private:
    VBORingContainer( const VBORingContainer& other );
    VBORingContainer& operator=(const VBORingContainer& other);
};

typedef ReferenceCounter<VBORingContainer> RC_VBORingContainer;

class UniformRingBuffer {
public:
    int size;
    int gl_id;
    int offset;
    int next_offset;

    UniformRingBuffer():
        gl_id(-1),
        size(0),
        offset(0),
        next_offset(0)
    {}

    void Create(int desired_size);
    void Fill(int data_size, void* data);
};
