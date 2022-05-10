//-----------------------------------------------------------------------------
//           Name: hardware_specs.cpp
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
#include "hardware_specs.h"

#include <Internal/datemodified.h>
#include <Internal/common.h>
#include <Internal/filesystem.h>

#include <Compat/fileio.h>
#include <Graphics/graphics.h>
#include <Logging/logdata.h>
#include <Memory/allocation.h>
#include <Utility/strings.h>
#include <Version/version.h>

#include <algorithm>
#include <iostream>
#include <opengl.h>

const unsigned kHardwareReportVersion = 5;

namespace {
const int TEMP_STRING_LENGTH = 256;
}

#include <cctype>
GLVendor GetGLVendor() {
    const GLubyte *vendor_cstring = glGetString(GL_VENDOR);
    if (vendor_cstring != NULL) {
        std::string vendor_string((const char *)vendor_cstring);
        std::transform(vendor_string.begin(),
                       vendor_string.end(),
                       vendor_string.begin(),
                       (int (*)(int))std::tolower);
        if (vendor_string.find("nvidia") != std::string::npos) {
            return _nvidia;
        } else if (vendor_string.find("ati") != std::string::npos) {
            return _ati;
        } else if (vendor_string.find("intel") != std::string::npos) {
            return _intel;
        }
    } else {
        LOGE << "Calling glGetString(GL_VENDOR) returned NULL." << std::endl;
    }

    return _unknown;
}

#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX 0x9047

#define GL_VBO_FREE_MEMORY_ATI 0x87FB
#define GL_TEXTURE_FREE_MEMORY_ATI 0x87FC
#define GL_RENDERBUFFER_FREE_MEMORY_ATI 0x87FD

void QueryVRAM(std::string &total_string) {
    char temp_string[TEMP_STRING_LENGTH];
    if (GLAD_GL_NVX_gpu_memory_info) {
        GLint param;
        glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &param);
        FormatString(temp_string, TEMP_STRING_LENGTH, "VRAM: %d MB\n", param / 1024);
        total_string += temp_string;
    } else if (GLAD_GL_ATI_meminfo) {
        GLint param[4];
        glGetIntegerv(GL_VBO_FREE_MEMORY_ATI, &param[0]);
        FormatString(temp_string, TEMP_STRING_LENGTH, "VRAM VBO: %d %d %d %d MB\n",
                     param[0] / 1024, param[1] / 1024, param[2] / 1024, param[3] / 1024);
        total_string += temp_string;
        glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, &param[0]);
        FormatString(temp_string, TEMP_STRING_LENGTH, "VRAM TEX: %d %d %d %d MB\n",
                     param[0] / 1024, param[1] / 1024, param[2] / 1024, param[3] / 1024);
        total_string += temp_string;
        glGetIntegerv(GL_RENDERBUFFER_FREE_MEMORY_ATI, &param[0]);
        FormatString(temp_string, TEMP_STRING_LENGTH, "VRAM RB: %d %d %d %d MB\n",
                     param[0] / 1024, param[1] / 1024, param[2] / 1024, param[3] / 1024);
        total_string += temp_string;
    } else {
        FormatString(temp_string, TEMP_STRING_LENGTH, "VRAM query not supported.\n");
        total_string += temp_string;
    }
}

struct GLQUERY_INFO {
    const char *m_name;
    GLint m_GLint;
    unsigned int m_numValues;
};

#ifndef ARRAYSIZE
#define ARRAYSIZE(p) (sizeof(p) / sizeof(p[0]))
#endif
#define GLQUERY_INFO_ENTRY(name, num_values) {#name, name, num_values},
static const GLQUERY_INFO GLQUERY_INFOS[] =
    {
        GLQUERY_INFO_ENTRY(GL_MAX_SAMPLES, 1)
            GLQUERY_INFO_ENTRY(GL_MAX_COLOR_TEXTURE_SAMPLES, 1)
                GLQUERY_INFO_ENTRY(GL_MAX_COMBINED_UNIFORM_BLOCKS, 1)
        // GLQUERY_INFO_ENTRY(GL_MAX_COMPUTE_UNIFORM_BLOCKS, 1) GL 4.3
        GLQUERY_INFO_ENTRY(GL_MAX_DEPTH_TEXTURE_SAMPLES, 1)
            GLQUERY_INFO_ENTRY(GL_MAX_FRAGMENT_INPUT_COMPONENTS, 1)
                GLQUERY_INFO_ENTRY(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, 1)
                    GLQUERY_INFO_ENTRY(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, 1)
                        GLQUERY_INFO_ENTRY(GL_MAX_FRAGMENT_UNIFORM_VECTORS, 1)
                            GLQUERY_INFO_ENTRY(GL_MAX_GEOMETRY_UNIFORM_BLOCKS, 1)
                                GLQUERY_INFO_ENTRY(GL_MAX_INTEGER_SAMPLES, 1)
                                    GLQUERY_INFO_ENTRY(GL_MAX_RENDERBUFFER_SIZE, 1)
                                        GLQUERY_INFO_ENTRY(GL_MAX_TEXTURE_IMAGE_UNITS, 1)
                                            GLQUERY_INFO_ENTRY(GL_MAX_TEXTURE_SIZE, 1)
                                                GLQUERY_INFO_ENTRY(GL_MAX_TEXTURE_BUFFER_SIZE, 1)
                                                    GLQUERY_INFO_ENTRY(GL_MAX_UNIFORM_BLOCK_SIZE, 1)
                                                        GLQUERY_INFO_ENTRY(GL_MAX_UNIFORM_BUFFER_BINDINGS, 1)
        // GLQUERY_INFO_ENTRY(GL_MAX_UNIFORM_LOCATIONS, 1) GL 4.3
        GLQUERY_INFO_ENTRY(GL_MAX_VARYING_COMPONENTS, 1)
            GLQUERY_INFO_ENTRY(GL_MAX_VARYING_VECTORS, 1)
                GLQUERY_INFO_ENTRY(GL_MAX_VERTEX_ATTRIBS, 1)
                    GLQUERY_INFO_ENTRY(GL_MAX_VERTEX_OUTPUT_COMPONENTS, 1)
                        GLQUERY_INFO_ENTRY(GL_MAX_VERTEX_UNIFORM_BLOCKS, 1)
                            GLQUERY_INFO_ENTRY(GL_MAX_VERTEX_UNIFORM_COMPONENTS, 1)
                                GLQUERY_INFO_ENTRY(GL_MAX_VERTEX_UNIFORM_VECTORS, 1)
                                    GLQUERY_INFO_ENTRY(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, 1)
                                        GLQUERY_INFO_ENTRY(GL_MAX_VIEWPORT_DIMS, 2)};
#undef GLQUERY_INFO_ENTRY

static bool initialized = false;

static bool support_s3tc = false;

static int max_texture_size = 0;

static std::map<std::string, int> integer_limits;
static std::map<std::string, ivec2> ivec2_limits;
static std::vector<std::string> extensions;

static void LazyInit() {
    if (initialized == false) {
        for (auto i : GLQUERY_INFOS) {
            GLint values[2];
            glGetIntegerv(i.m_GLint, values);
            GLenum err = glGetError();
            if (err != GL_NONE) {
                integer_limits[i.m_name] = -1;
            } else if (i.m_numValues == 1) {
                integer_limits[i.m_name] = values[0];

                if (i.m_GLint == GL_MAX_TEXTURE_SIZE) {
                    max_texture_size = values[0];
                }
            } else {
                ivec2_limits[i.m_name] = ivec2(values[0], values[1]);
            }
        }

        GLint numExtensions = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
        if (numExtensions != 0) {
            for (int i = 0; i < numExtensions; i++) {
                const char *extension_string = (const char *)glGetStringi(GL_EXTENSIONS, i);
                if (extension_string) {
                    extensions.push_back(extension_string);

                    if (strmtch("GL_EXT_texture_compression_s3tc", extension_string)) {
                        support_s3tc = true;
                    }
                }
            }
        }

        initialized = true;
    }
}

void PrintGPU(std::string &total_string, bool short_output) {
    LazyInit();
    char temp_string[TEMP_STRING_LENGTH];
    CHECK_GL_ERROR();

    GLVendor vendor = GetGLVendor();
    CHECK_GL_ERROR();
    switch (vendor) {
        case _unknown:
            FormatString(temp_string, TEMP_STRING_LENGTH, "GPU Vendor: %s\n", glGetString(GL_VENDOR));
            total_string += temp_string;
            break;
        case _ati:
            FormatString(temp_string, TEMP_STRING_LENGTH, "GPU Vendor: ATI\n");
            total_string += temp_string;
            break;
        case _nvidia:
            FormatString(temp_string, TEMP_STRING_LENGTH, "GPU Vendor: NVIDIA\n");
            total_string += temp_string;
            break;
        case _intel:
            FormatString(temp_string, TEMP_STRING_LENGTH, "GPU Vendor: INTEL\n");
            total_string += temp_string;
            break;
    }
    CHECK_GL_ERROR();

    FormatString(temp_string, TEMP_STRING_LENGTH, "GL Renderer: %s\n", glGetString(GL_RENDERER));
    total_string += temp_string;
    FormatString(temp_string, TEMP_STRING_LENGTH, "GL Version: %s\n", glGetString(GL_VERSION));
    total_string += temp_string;
    unsigned driver_version = GetDriverVersion(vendor);

    CHECK_GL_ERROR();
    if (driver_version != 0) {
        FormatString(temp_string, TEMP_STRING_LENGTH, "GL Driver Version: %u\n", driver_version);
    } else {
        FormatString(temp_string, TEMP_STRING_LENGTH, "GL Driver Version: unknown\n");
    }
    total_string += temp_string;

    CHECK_GL_ERROR();

    QueryVRAM(total_string);

    FormatString(temp_string, TEMP_STRING_LENGTH, "GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    total_string += temp_string;
    CHECK_GL_ERROR();

    if (short_output == false) {
        if (GLAD_GL_NV_gpu_program4) {
            FormatString(temp_string, TEMP_STRING_LENGTH, "Shader Model: 4.0 or better\n");
            total_string += temp_string;
        } else if (GLAD_GL_NV_vertex_program3) {
            FormatString(temp_string, TEMP_STRING_LENGTH, "Shader Model: 3.0 or better\n");
            total_string += temp_string;
        } else if (GLAD_GL_ARB_fragment_program) {
            FormatString(temp_string, TEMP_STRING_LENGTH, "Shader Model: 2.0 or better\n");
            total_string += temp_string;
        } else {
            FormatString(temp_string, TEMP_STRING_LENGTH, "Shader Model: below 2.0\n");
            total_string += temp_string;
        }

        total_string += "\n[OpenGL Limits]\n";

        std::map<std::string, int>::iterator integer_limit_it = integer_limits.begin();
        for (; integer_limit_it != integer_limits.end(); integer_limit_it++) {
            FormatString(temp_string, TEMP_STRING_LENGTH, "%s = %d\n", integer_limit_it->first.c_str(), integer_limit_it->second);
            total_string += temp_string;
        }

        std::map<std::string, ivec2>::iterator ivec2_limit_it = ivec2_limits.begin();
        for (; ivec2_limit_it != ivec2_limits.end(); ivec2_limit_it++) {
            FormatString(temp_string, TEMP_STRING_LENGTH, "%s[2] = {%d, %d}\n", ivec2_limit_it->first.c_str(), ivec2_limit_it->second[0], ivec2_limit_it->second[1]);
            total_string += temp_string;
        }

        total_string += "\n[Available OpenGL extensions]\n";
        for (auto &extension : extensions) {
            FormatString(temp_string, TEMP_STRING_LENGTH, "%s\n", extension.c_str());
            total_string += temp_string;
        }
    }
}

void PrintSpecs() {
    FILE *test_file;
    char write_path[kPathSize];
    GetHWReportPath(write_path, kPathSize);
    test_file = my_fopen(write_path, "r");
    if (test_file != NULL) {
        // Get the first line (should contain version number)
        char read_string[255];
        fgets(read_string, 255, test_file);

        // If first character is not 'R' then it probably doesn't contain
        // a version number. Otherwise, it should have "Report version: x", so read
        // so read x as an integer.
        unsigned int version = 0;
        if (read_string[0] == 'R') {
            version = atoi(&read_string[16]);
        }

        fclose(test_file);

        if (version == kHardwareReportVersion) {
            return;
        }
    }
    CHECK_GL_ERROR();

    LOGI << "Printing specs:" << std::endl;
    LOGI << "---------------" << std::endl;

    std::string total_string;

    CHECK_GL_ERROR();
    char temp_string[256];
    FormatString(temp_string, TEMP_STRING_LENGTH, "Report Version: %u\n", kHardwareReportVersion);
    total_string += temp_string;

    total_string += "\n";

    FormatString(temp_string, TEMP_STRING_LENGTH, "Build: %s\n", GetBuildIDString());
    total_string += temp_string;
    FormatString(temp_string, TEMP_STRING_LENGTH, "Version: %s\n", GetBuildVersion());
    total_string += temp_string;
    FormatString(temp_string, TEMP_STRING_LENGTH, "Timestamp: %s\n", GetBuildTimestamp());
    total_string += temp_string;

    total_string += "\n";

    FormatString(temp_string, TEMP_STRING_LENGTH, "OS: %s\n", GetPlatform());
    total_string += temp_string;
    FormatString(temp_string, TEMP_STRING_LENGTH, "Arch: %s\n", GetArch());
    total_string += temp_string;

    total_string += "\n";

    CHECK_GL_ERROR();
    PrintGPU(total_string, false);
    CHECK_GL_ERROR();

    total_string += "\n";

    FILE *file = my_fopen(write_path, "w");
    fputs(total_string.c_str(), file);
    fclose(file);
}

std::map<std::string, int> GetHardwareLimitationsInt() {
    LazyInit();
    return integer_limits;
}

bool HasHardwareS3TCSupport() {
    LazyInit();
    return support_s3tc;
}

int GetGLMaxTextureSize() {
    LazyInit();
    return max_texture_size;
}
