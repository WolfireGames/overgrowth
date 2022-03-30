//-----------------------------------------------------------------------------
//           Name: vbocontainer.h
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

class VBOContainer {
    GLuint gl_VBO;
    bool element;
    GLuint storage;

    GLuint size_;
public:
    bool is_valid;
    void Dispose();
    void Fill(char flags, GLuint size, void* data);
    void Bind() const;
    bool valid() const;
    uintptr_t offset() const; 
    unsigned size(); //returns size in bytes.
    
    VBOContainer();
    ~VBOContainer();
private:
    VBOContainer( const VBOContainer& other );
    VBOContainer& operator=(const VBOContainer& other);
};

typedef ReferenceCounter<VBOContainer> RC_VBOContainer;
