//-----------------------------------------------------------------------------
//           Name: vbocontainer.cpp
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
#include "vbocontainer.h"

#include <Graphics/graphics.h>
#include <Logging/logdata.h>
#include <Utility/assert.h>
#include <Internal/profiler.h>

#include <SDL_assert.h>

VBOContainer::VBOContainer() : is_valid(false),
                               storage(0) {}

void VBOContainer::Fill(char flags, GLuint size, void* data) {
    if (size > 0) {
        LOG_ASSERT(data);

        Graphics* graphics = Graphics::Instance();
        GLenum target;
        if (flags & kVBOElement) {
            target = GL_ELEMENT_ARRAY_BUFFER;
        } else if (flags & kVBOFloat) {
            target = GL_ARRAY_BUFFER;
        } else {
            target = 0xFFFFFFFF;
            // No target flag set
            SDL_assert(false);
        }
        GLenum hint;
        if (flags & kVBOStatic) {
            hint = GL_STATIC_DRAW;
        } else if (flags & kVBODynamic) {
            hint = GL_DYNAMIC_DRAW;
        } else if (flags & kVBOStream) {
            hint = GL_STREAM_DRAW;
        } else {
            hint = 0xFFFFFFFF;
            // No hint flag set
            SDL_assert(false);
        }
        if (flags & kVBOStatic || !is_valid) {
            element = flags & kVBOElement;
            if (is_valid) {
                LOGS << "Disposing " << gl_VBO << std::endl;
                Dispose();
            }
            glGenBuffers(1, &gl_VBO);
            // LOGI << "Generated vbo: " << gl_VBO << std::endl;
            graphics->BindVBO(target, gl_VBO);
            glBufferData(target, size, data, hint);
            is_valid = true;
            storage = size;
        } else {
            graphics->BindVBO(target, gl_VBO);

            /*
             * Some early tests indicate that no BufferData orphaning and memory mapped transfers are
             * the most efficient combination for MacOSX intel.
             * Generally, i never expect BufferData orphaning in combination with mapping will ever be more efficient
             * as it tells the driver to invalidate or orphan the data twice.
             *
             * MacOSX Intel:
             *    kAlwaysOrphan = false
             *    kUseMapRange = true
             * MacOSX Nvidia:
             *    ....
             *    ....
             * Windows ATI:
             *    ....
             *    ....
             * Windows NVIDIA:
             *    ....
             *    ....
             * Windows Intel:
             *    ....
             *    ....
             * Linux NVIDIA:
             *    ....
             *    ....
             * Linux Intel:
             *    ....
             *    ....
             * Linux ATI:
             *    ....
             *    ....
             */
            const bool kAlwaysOrphan = false;
            bool orphaned = false;
            if (size > storage) {
                LOGW << "Reallocating buffer because of resize" << std::endl;
                PROFILER_ZONE(g_profiler_ctx, "glBufferData");
                storage = size;
                glBufferData(target, storage, NULL, hint);
                orphaned = true;
            }

            // if( !(flags & kVBOForceReBufferData) ){
            if (orphaned == false && kAlwaysOrphan) {
                PROFILER_ZONE(g_profiler_ctx, "glBufferData orphan");
                glBufferData(target, storage, NULL, hint);
            }

            const bool kUseMapRange = true;
            if (kUseMapRange) {
                PROFILER_ZONE(g_profiler_ctx, "glMapBufferRange");
                // Using the Unsynchronized bit here seems incorrect.
                // Because we can't actually guarantee on the client side
                // that the GL environment isn't done with the buffer yet.
                // For this we have to do a ring buffer implementation instead.
                // Which we might do anyways..
                // void* mapped = glMapBufferRange(target, 0, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_INVALIDATE_RANGE_BIT );
                void* mapped = glMapBufferRange(target, 0, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
                memcpy(mapped, data, size);
                glUnmapBuffer(target);
            } else {
                PROFILER_ZONE(g_profiler_ctx, "glBufferSubData");
                glBufferSubData(target, 0, size, data);
            }
        }
        graphics->BindVBO(target, 0);
    }
    size_ = size;
}

void VBOContainer::Dispose() {
    if (!is_valid) {
        return;
    }
    Graphics* graphics = Graphics::Instance();

    GLenum target;
    if (element) {
        target = GL_ELEMENT_ARRAY_BUFFER;
    } else {
        target = GL_ARRAY_BUFFER;
    }

    // LOGI << "Disposing vbo: " << gl_VBO << " for target " << target << std::endl;
    LogSystem::Flush();
    graphics->UnbindVBO(target, gl_VBO);
    glDeleteBuffers(1, &gl_VBO);
    is_valid = false;
    size_ = 0;
}

void VBOContainer::Bind() const {
    if (!is_valid) {
        LOG_ASSERT(false);
        return;
    }

    if (element) {
        Graphics::Instance()->BindElementVBO(gl_VBO);
    } else {
        Graphics::Instance()->BindArrayVBO(gl_VBO);
    }
}

bool VBOContainer::valid() const {
    return is_valid;
}

unsigned VBOContainer::size() {
    return valid() ? size_ : 0;
}

uintptr_t VBOContainer::offset() const {
    return 0;
}

VBOContainer::~VBOContainer() {
    Dispose();
}
