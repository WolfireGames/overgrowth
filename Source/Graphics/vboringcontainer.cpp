//-----------------------------------------------------------------------------
//           Name: vboringcontainer.cpp
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
#include "vboringcontainer.h"

#include <Graphics/graphics.h>
#include <Logging/logdata.h>
#include <Utility/assert.h>
#include <Internal/profiler.h>

#include <SDL_assert.h>

const unsigned kAlignPadding = 64;

const unsigned kBufferSizeMultiplier = 32;

static int buffer_size_multipler_offset = 0;

VBORingContainer::VBORingContainer(GLuint _storage_size, char flags, bool ignore_multiplier) : used_size(0),
                                                                                               allocated_size(0),
                                                                                               gl_VBO(-1),
                                                                                               storage_size(0),
                                                                                               hint(0),
                                                                                               storage_multiplier(kBufferSizeMultiplier + buffer_size_multipler_offset++ % 32) {
    SetHint(_storage_size, flags & (kVBOStatic | kVBODynamic | kVBOStream), ignore_multiplier);
    next_offset = storage_size;

    if (flags & kVBOElement) {
        target = GL_ELEMENT_ARRAY_BUFFER;
    } else if (flags & kVBOFloat) {
        target = GL_ARRAY_BUFFER;
    } else {
        target = 0xFFFFFFFF;
        // No target flag set
        SDL_assert(false);
    }

    element = flags & kVBOElement;
    force_reload = flags & kVBOForceReBufferData;
}

void VBORingContainer::SetHint(GLuint storage, char flags, bool ignore_multiplier) {
    LOG_ASSERT(((flags & ~(kVBOStatic | kVBODynamic | kVBOStream)) == 0));
    GLenum old_hint = hint;
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
    // Indicate we want to reallocate the buffer;
    if (old_hint != hint)
        next_offset = storage_size;

    if (storage % kAlignPadding) {
        storage = storage + kAlignPadding - storage % kAlignPadding;
    }

    if (hint == GL_STATIC_DRAW) {
        storage_size_hint = storage;
    } else {
        storage_size_hint = storage * (ignore_multiplier ? 1 : storage_multiplier);
    }
}

void VBORingContainer::Fill(GLuint size, void* data) {
    if (size > 0) {
        Graphics* graphics = Graphics::Instance();
        int old_gl_VBO = gl_VBO;
        if (gl_VBO == -1) {
            GLuint val;
            glGenBuffers(1, &val);
            gl_VBO = val;
        }

        used_size = size;
        // pad utilized size to ensure maxixum use of bus.
        allocated_size = size;
        if (allocated_size % kAlignPadding) {
            allocated_size = allocated_size + kAlignPadding - allocated_size % kAlignPadding;
        }

        LOG_ASSERT(data);

        while (allocated_size > storage_size) {
            if (storage_size == storage_size_hint) {
                LOGW << "Requested size is larger than internal preallocated size, resizing to a larger buffer." << std::endl;
                storage_size_hint = storage_size_hint * 4;
            }

            storage_size = storage_size_hint;
            next_offset = storage_size;
        }

        graphics->BindVBO(target, gl_VBO);

        // if our next size is too big to fit in the remainder of the buffer, create a new one.
        if (next_offset + allocated_size > storage_size) {
            if (old_gl_VBO != -1) {
                PROFILER_ENTER(g_profiler_ctx, "Orphan VBORingContainer buffer");
            } else {
                PROFILER_ENTER(g_profiler_ctx, "Create VBORingContainer buffer");
            }
            storage_size = storage_size_hint;
            glBufferData(target, storage_size, NULL, hint);
            next_offset = 0;
            PROFILER_LEAVE(g_profiler_ctx);
        }

        const bool kUseMapBufferRange = false;
        if (kUseMapBufferRange) {
            void* mapped = glMapBufferRange(target, next_offset, used_size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
            memcpy(mapped, data, used_size);
            glUnmapBuffer(target);
        } else {
            PROFILER_ZONE(g_profiler_ctx, "glBufferSubData");
            glBufferSubData(target, next_offset, used_size, data);
        }

        current_offset = next_offset;
        next_offset = next_offset + allocated_size;

        graphics->BindVBO(target, 0);
    }
}

void VBORingContainer::Dispose() {
    if (valid() == false) {
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
    GLuint val = gl_VBO;
    glDeleteBuffers(1, &val);
    gl_VBO = -1;
    next_offset = storage_size;
    current_offset = 0;
    allocated_size = 0;
    used_size = 0;
}

void VBORingContainer::Bind() const {
    if (valid() == false) {
        LOG_ASSERT(false);
        return;
    }

    if (element) {
        Graphics::Instance()->BindElementVBO(gl_VBO);
    } else {
        Graphics::Instance()->BindArrayVBO(gl_VBO);
    }
}

bool VBORingContainer::valid() const {
    return gl_VBO != -1;
}

unsigned VBORingContainer::size() {
    return valid() ? used_size : 0;
}

uintptr_t VBORingContainer::offset() const {
    if (valid()) {
        return (uintptr_t)(current_offset);
    } else {
        return 0;
    }
}

VBORingContainer::~VBORingContainer() {
    Dispose();
}

static GLint max_ubo_size = -1;
static GLint ubo_alignment = -1;

void UniformRingBuffer::Create(int desired_size) {
    if (max_ubo_size == -1) {
        glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &max_ubo_size);
        glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &ubo_alignment);
    }
    size = desired_size;  // Note: Does not have to be smaller than max_ubo_size - that's the max that can be *bound* at once

    GLuint uboHandle;
    glGenBuffers(1, &uboHandle);
    gl_id = uboHandle;
    glBindBuffer(GL_UNIFORM_BUFFER, gl_id);
    glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_DYNAMIC_DRAW);

    offset = 0;
    next_offset = 0;
}

void UniformRingBuffer::Fill(int data_size, void* data) {
    glBindBuffer(GL_UNIFORM_BUFFER, gl_id);
    if (data_size > size || data_size > max_ubo_size) {
        FatalError("Error", "Data is too big for uniform ring buffer");
    }
    if (data_size + next_offset > size) {
        PROFILER_ZONE(g_profiler_ctx, "orphan buffer");
        glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_DYNAMIC_DRAW);  // orphan buffer?
        offset = 0;
        next_offset = 0;
    }

    const bool kUseMemoryMap = false;
    if (kUseMemoryMap) {
        void* mapped;
        {
            PROFILER_ZONE(g_profiler_ctx, "glMapBufferRange");
            mapped = glMapBufferRange(GL_UNIFORM_BUFFER, next_offset, data_size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
        }
        {
            PROFILER_ZONE(g_profiler_ctx, "memcpy");
            memcpy(mapped, data, data_size);
        }
        {
            PROFILER_ZONE(g_profiler_ctx, "glUnmapBuffer");
            glUnmapBuffer(GL_UNIFORM_BUFFER);
        }
    } else {
        PROFILER_ZONE(g_profiler_ctx, "glBufferSubData");
        glBufferSubData(GL_UNIFORM_BUFFER, next_offset, data_size, data);
    }

    offset = next_offset;
    next_offset += data_size;
    next_offset = ((next_offset + (ubo_alignment - 1)) / ubo_alignment) * ubo_alignment;
}
