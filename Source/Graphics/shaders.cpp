//-----------------------------------------------------------------------------
//           Name: shaders.cpp
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
#include "shaders.h"

#include <Graphics/textures.h>
#include <Graphics/graphics.h>

#include <Internal/common.h>
#include <Internal/textfile.h>
#include <Internal/datemodified.h>
#include <Internal/filesystem.h>
#include <Internal/profiler.h>
#include <Internal/config.h>

#include <Logging/logdata.h>
#include <Compat/fileio.h>
#include <Utility/assert.h>

#include <string>

#define SHADER_WARNINGS

extern bool g_single_pass_shadow_cascade;

void Shaders::Dispose() {
    CHECK_GL_ERROR();
    for (auto &program : programs) {
        if (program.gl_program != UNLOADED_SHADER_ID) {
            for (int shader_id : program.shader_ids) {
                if (shader_id >= 0) {
                    glDetachShader(program.gl_program, shaders[shader_id].gl_shader);
                }
            }
            glDeleteProgram(program.gl_program);
        }
    }
    for (auto &shader : shaders) {
        if (shader.gl_shader != UNLOADED_SHADER_ID) {
            glDeleteShader(shader.gl_shader);
        }
    }

    CHECK_GL_ERROR();

    shaders.clear();
    programs.clear();
}

namespace {
static bool IsCharInString(char chr, const char *str) {
    for (int i = 0; str[i] != '\0'; ++i) {
        if (chr == str[i]) {
            return true;
        }
    }
    return false;
}

static void CopySubString(const char *src, char *dst, const char *delim, int max_chars) {
    int i = 0;
    while (i < max_chars && !IsCharInString(src[i], delim)) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static int FindCharInString(char chr, const char *str, int max_chars) {
    for (int i = 0; i < max_chars; ++i) {
        if (str[i] == chr) {
            return i;
        }
    }
    return -1;
}

namespace PREPROC {
enum Type {
    INCLUDE,
    UNKNOWN,
    PRAGMA,
    VERSION
};
}

struct PreprocessorDirective {
    PREPROC::Type type;
    std::string data;
};

// Looks at a line of text and fills *pd with information about the preprocessor directive in that line
static void ParsePreprocessorDirective(const char *text, int line_break, PreprocessorDirective *pd) {
    pd->type = PREPROC::UNKNOWN;
    // Copy # directive into buf e.g. get 'include' from '#include "blah"'
    const int BUF_SIZE = 256;
    char buf[BUF_SIZE];
    CopySubString(&text[1], buf, " \t", min(BUF_SIZE - 1, line_break - 1));
    // If #include directive, read info between "" signs
    if (strcmp(buf, "include") == 0) {
        int open_quote_sign = FindCharInString('\"', text, line_break);
        if (open_quote_sign == -1) {
            return;
        }
        int close_quote_sign = FindCharInString('\"', &text[open_quote_sign + 1], line_break - (open_quote_sign + 1)) + open_quote_sign + 1;
        if (close_quote_sign == -1) {
            return;
        }
        CopySubString(&text[open_quote_sign + 1], buf, "", min(BUF_SIZE - 1, close_quote_sign - open_quote_sign - 1));
        pd->data = buf;
        pd->type = PREPROC::INCLUDE;
    }
    // If #pragma directive, read proceeding non-whitespace
    if (strcmp(buf, "pragma") == 0) {
        int first_char = strlen("#pragma");
        while (IsCharInString(text[first_char], " \t") && first_char < line_break) {
            ++first_char;
        }
        int second_char = first_char;
        while (!IsCharInString(text[second_char], " \t") && second_char < line_break) {
            ++second_char;
        }
        CopySubString(&text[first_char], buf, "", min(BUF_SIZE - 1, second_char - first_char));
        pd->data = buf;
        pd->type = PREPROC::PRAGMA;
    }
    if (strcmp(buf, "version") == 0) {
        int first_char = strlen("#version");
        while (IsCharInString(text[first_char], " \t") && first_char < line_break) {
            ++first_char;
        }
        int second_char = first_char;
        while (!IsCharInString(text[second_char], " \t") && second_char < line_break) {
            ++second_char;
        }
        CopySubString(&text[first_char], buf, "", min(BUF_SIZE - 1, second_char - first_char));
        pd->data = buf;
        pd->type = PREPROC::VERSION;
    }
    return;
}

static void HandleInclude(std::string *text, int line_start, const char *abs_path, int *cursor) {
    text->insert(line_start, "//");
    *cursor += 2;
    text->insert(*cursor, "\n");
    *cursor += 1;
    std::string include_text;
    textFileRead(abs_path, &include_text);
    text->insert(*cursor, include_text);
    --*cursor;
}

static void AddShaderDefinition(std::string *text, const std::string &def) {
    text->insert(0, "#define " + def + "\n");
}

static void Preprocess(Shader *shader, ShaderType type, const std::vector<std::string> &definitions, const std::string &shader_dir_path) {
    shader->include_files.clear();
    shader->include_modified.clear();
    shader->use_tangent = false;
    shader->transparent = false;
    shader->bind_out_color = false;
    shader->bind_out_vel = false;
    shader->depth_only = false;
    shader->particle = false;
    BlendMode blend_mode = _BM_NORMAL;
    std::string &text = shader->text;

    bool no_velocity_buf_defined = false;

    for (const auto &definition : definitions) {
        if (definition == "TANGENT") {
            shader->use_tangent = true;
        }
        if (definition == "ALPHA") {
            shader->transparent = true;
        }
        if (definition == "DEPTH_ONLY") {
            shader->depth_only = true;
        }
        if (definition == "PARTICLE") {
            shader->particle = true;
        }
        if (definition == "NO_VELOCITY_BUF") {
            no_velocity_buf_defined = true;
        }
    }

    int version = -1;
    int line_start = 0;
    for (int cursor = 0; text[cursor] != '\0'; ++cursor) {
        // Check each line one at a time
        if (IsCharInString(text[cursor], "\n\r")) {
            // Search line for # sign
            int pound_sign = -1;
            for (int j = line_start; j < cursor; ++j) {
                if (text[j] == '#') {
                    pound_sign = j;
                    break;
                }
            }
            if (pound_sign != -1) {
                PreprocessorDirective pd;
                ParsePreprocessorDirective(&text[pound_sign], cursor - pound_sign, &pd);
                bool recognized_pragma = false;
                switch (pd.type) {
                    case PREPROC::UNKNOWN:
                        LOGD << "Unknown preprocessor directive" << std::endl;
                        break;
                    case PREPROC::INCLUDE:
                        LOGD << "Include file \"" << pd.data << "\"." << std::endl;
                        ;
                        char rel_path[kPathSize], abs_path[kPathSize];
                        FormatString(rel_path, kPathSize, "%s%s", shader_dir_path.c_str(), pd.data.c_str());
                        if (FindFilePath(rel_path, abs_path, kPathSize, kDataPaths | kModPaths) == -1) {
                            FatalError("Error", "Could not find shader include file: %s", rel_path);
                        }
                        HandleInclude(&text, line_start, abs_path, &cursor);
                        shader->include_files.push_back(abs_path);
                        shader->include_modified.push_back(GetDateModifiedInt64(shader->include_files.back().c_str()));
                        break;
                    case PREPROC::PRAGMA:
                        LOGD << "Pragma \"" << pd.data << "\"" << std::endl;
                        if (pd.data == "transparent") {
                            shader->transparent = true;
                            recognized_pragma = true;
                        } else if (pd.data == "use_tangent") {
                            shader->use_tangent = true;
                            recognized_pragma = true;
                        } else if (pd.data == "blendmode_add") {
                            blend_mode = _BM_ADD;
                            recognized_pragma = true;
                        } else if (pd.data == "blendmode_multiply") {
                            blend_mode = _BM_MULTIPLY;
                            recognized_pragma = true;
                        } else if (pd.data == "bind_out_color") {
                            shader->bind_out_color = true;
                            recognized_pragma = true;
                        } else if (pd.data == "bind_out_vel") {
                            recognized_pragma = true;
                            if (no_velocity_buf_defined) {
                                shader->bind_out_vel = false;
                            } else {
                                shader->bind_out_vel = true;
                            }
                        }
                        if (recognized_pragma) {
                            text.insert(line_start, "//");
                        }
                        break;
                    case PREPROC::VERSION:
                        // Comment out current version definition
                        // So we can add it later at the beginning
                        text.insert(line_start, "//");
                        cursor += 2;
                        text.insert(cursor, "\n");
                        cursor += 1;
                        version = atoi(pd.data.c_str());
                        break;
                }
            }
            line_start = cursor + 1;
        }
    }

    AddShaderDefinition(&text, "GAMMA_CORRECT");
    if (type == _vertex) {
        AddShaderDefinition(&text, "VERTEX_SHADER");
    } else if (type == _fragment) {
        AddShaderDefinition(&text, "FRAGMENT_SHADER");
    } else if (type == _geom) {
        AddShaderDefinition(&text, "GEOMETRY_SHADER");
    } else if (type == _tess_ctrl) {
        AddShaderDefinition(&text, "TESSELATION_CONTROL_SHADER");
    } else if (type == _tess_eval) {
        AddShaderDefinition(&text, "TESSELATION_EVALUATOR_SHADER");
    }

    for (const auto &definition : definitions) {
        AddShaderDefinition(&text, definition);
    }
    if (GLAD_GL_ARB_sample_shading) {
        AddShaderDefinition(&text, "ARB_sample_shading_available");
    }
    if (GLAD_GL_ARB_shader_texture_lod && type == _fragment) {
        text.insert(0, "#extension GL_ARB_shader_texture_lod : enable\n");
    }

    // This must be the last directive added to make sure that #version is at the beginning of the file
    if (version == -1) {
        version = 130;
    }
    {
        static const int kBufSize = 256;
        char buf[kBufSize];
        FormatString(buf, kBufSize, "#version %d\n", version);
        text.insert(0, buf);
    }

    if (blend_mode == _BM_ADD) {
        shader->blend_src = GL_SRC_ALPHA;
        shader->blend_dst = GL_ONE;
    } else if (blend_mode == _BM_MULTIPLY) {
        shader->blend_src = GL_DST_COLOR;
        shader->blend_dst = GL_ONE_MINUS_SRC_ALPHA;
    } else {
        shader->blend_src = GL_SRC_ALPHA;
        shader->blend_dst = GL_ONE_MINUS_SRC_ALPHA;
    }
}
}  // namespace

void Shaders::ReloadShader(int which, ShaderType type) {
    if (which >= 0) {
        Shader &shader = shaders[which];
        if (shader.gl_shader != UNLOADED_SHADER_ID) {
            glDeleteShader(shader.gl_shader);
            shader.gl_shader = UNLOADED_SHADER_ID;
        }
        textFileRead(shader.name, &shader.text);
        Preprocess(&shader, type, shader.definitions, shader_dir_path);
        shader.modified = GetDateModifiedInt64(shader.name.c_str());
    }
}

void Shaders::Reload(bool force) {
    CHECK_GL_ERROR();
    for (unsigned i = 0; i < shaders.size(); ++i) {
        Shader &shader = shaders[i];
        bool modified = (shader.modified != GetDateModifiedInt64(shader.name.c_str()));
        if (!modified) {  // Check included files to see if they were modified
            for (unsigned j = 0; j < shader.include_files.size(); j++) {
                modified = (shader.include_modified[j] != GetDateModifiedInt64(shader.include_files[j].c_str()));
                if (modified) {
                    break;
                }
            }
        }
        if (modified || force) {
            ReloadShader(i, shader.type);
        }
    }
    unsigned num_programs = programs.size();
    for (unsigned i = 0; i < num_programs; ++i) {
        Program &program = programs[i];
        if (program.gl_program != UNLOADED_SHADER_ID) {
            bool unloaded = false;
            for (int shader_id : program.shader_ids) {
                if (shader_id >= 0 && shaders[shader_id].gl_shader == UNLOADED_SHADER_ID) {
                    unloaded = true;
                    break;
                }
            }
            if (unloaded) {
                glDeleteProgram(program.gl_program);
                program.gl_program = UNLOADED_SHADER_ID;
            }
        }
    }
    CHECK_GL_ERROR();
}

// Return the id of a shader
int Shaders::returnShader(const char *path, ShaderType type, const std::vector<std::string> &definitions) {
    std::string full_path;
    full_path = path;
    for (const auto &definition : definitions) {
        full_path += " #" + definition;
    }

    for (unsigned i = 0; i < shaders.size(); ++i) {
        if (full_path == shaders[i].full_name) {
            return i;
        }
    }

    static const int kBufSize = 512;
    char abs_path[kBufSize];
    if (FindFilePath(path, abs_path, kBufSize, kDataPaths | kModPaths) == -1) {
        FatalError("Error", "Could not find path for shader: %s", path);
    }

    shaders.push_back(Shader());
    Shader &shader = shaders.back();
    shader.name = abs_path;
    shader.full_name = full_path;
    shader.gl_shader = UNLOADED_SHADER_ID;
    shader.definitions = definitions;
    textFileRead(shader.name, &shader.text);
    Preprocess(&shader, type, definitions, shader_dir_path);
    shader.modified = GetDateModifiedInt64(shader.name.c_str());
    return shaders.size() - 1;
}

bool Shaders::createShader(int which, ShaderType type) {
    CHECK_GL_ERROR();
    Shader &shader = shaders[which];
    if (shader.gl_shader != UNLOADED_SHADER_ID) {
        return true;
    }

    if (type == _vertex) {
        shader.gl_shader = glCreateShader(GL_VERTEX_SHADER);
    } else if (type == _fragment) {
        shader.gl_shader = glCreateShader(GL_FRAGMENT_SHADER);
    } else if (type == _geom) {
        shader.gl_shader = glCreateShader(GL_GEOMETRY_SHADER);
    } else if (type == _tess_ctrl) {
        shader.gl_shader = glCreateShader(GL_TESS_CONTROL_SHADER);
    } else if (type == _tess_eval) {
        shader.gl_shader = glCreateShader(GL_TESS_EVALUATION_SHADER);
    }
    shader.type = type;

    const char *sources[] = {shader.text.c_str()};
    glShaderSource(shader.gl_shader, 1, sources, NULL);
    glCompileShader(shader.gl_shader);

    // Output shader source to file
    if (config["shader_debug"].toNumber<bool>()) {
        size_t last_slash = shader.full_name.rfind('/');
        if (last_slash == std::string::npos) {
            last_slash = shader.full_name.rfind('\\');
        }
        size_t hashpos = shader.full_name.find('#');
        std::string shader_name;

        if (hashpos != std::string::npos) {
            // If shader string contains '#' move type identifier to end
            int hashlen = shader.full_name.length() - hashpos;
            shader_name = shader.full_name.substr(last_slash + 1, shader.full_name.length() - last_slash - 7 - hashlen);
            shader_name += shader.full_name.substr(hashpos, hashlen);
            switch (shader.type) {
                case _vertex:
                    shader_name += ".vert";
                    break;
                case _fragment:
                    shader_name += ".frag";
                    break;
                case _geom:
                    shader_name += ".geom";
                    break;
                case _tess_eval:
                    shader_name += ".tess_eval";
                    break;
                case _tess_ctrl:
                    shader_name += ".tess_ctrl";
                    break;
                case kMaxShaderTypes:
                    LOGE << "Invalid enum value" << std::endl;
                    break;
            }
        } else {
            // Otherwise just remove directory from full name
            shader_name = shader.full_name.substr(last_slash + 1, shader.full_name.length() - last_slash - 1);
        }
        char buf[kPathSize];
        FormatString(buf, kPathSize, "%sData/CompiledShaders/%s", GetWritePath(CoreGameModID).c_str(), shader_name.c_str());
        FILE *file = my_fopen(buf, "w");
        if (file) {
            fwrite(shader.text.c_str(), sizeof(char), shader.text.length(), file);
            fclose(file);
        }
    }

    int status = 0;
    glGetShaderiv(shader.gl_shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        int len = 0;
        char buffer[1025] = {0};
        glGetShaderInfoLog(shader.gl_shader, 1024, &len, buffer);
        if (len != 0) {
            char errorbuffer[512];
            int last_slash = 0;
            for (int i = 0, len = shader.full_name.size(); i < len; ++i) {
                char chr = shader.full_name[i];
                if (chr == '/' || chr == '\\') {
                    last_slash = i + 1;
                }
            }
            FormatString(errorbuffer, 512, "Error(s) in %s", &(shader.full_name.c_str()[last_slash]));
            if (status == 0 || config["shader_debug"].toNumber<bool>()) {
                const int kBufSize = 2048;
                char err_buf[kBufSize];
                FormatString(err_buf, kBufSize, "%s\n%s", &(shader.full_name.c_str()[last_slash]), buffer);
                // fatal error or shader debugging requested, show to user
                ErrorResponse response = DisplayError(errorbuffer, err_buf, _ok_cancel_retry);
                if (response == _retry) {
                    ReloadShader(which, type);
                    return createShader(which, type);
                }
            } else {
                // only log it
                LOGE << errorbuffer << " " << buffer << std::endl;
            }
        }

        if (type == _geom || type == _tess_eval || type == _tess_ctrl) {
            glDeleteShader(shader.gl_shader);
            shader.gl_shader = UNLOADED_SHADER_ID;
            return false;
        }
    }
    CHECK_GL_ERROR();

    return true;
}

// Return the id of a shader variable
GLint Shaders::returnShaderVariable(const std::string &name, int which) {
    Program::UniformAddressCacheMap::iterator iter =
        programs[which].uniform_address_cache.find(name);
    if (iter == programs[which].uniform_address_cache.end()) {
        GLint addr = glGetUniformLocation(programs[which].gl_program, name.c_str());
        programs[which].uniform_address_cache[name] = addr;
        return addr;
    } else {
        return iter->second;
    }
}

GLint Shaders::returnShaderVariableIndex(const std::string &name, int program_id) {
    Program::UniformAddressCacheMap::iterator iter =
        programs[program_id].uniform_address_cache.find(name);
    if (iter == programs[program_id].uniform_address_cache.end()) {
        GLuint index;
        const char *name_cstr = name.c_str();
        glGetUniformIndices(programs[program_id].gl_program, 1, &name_cstr, &index);
        programs[program_id].uniform_address_cache[name] = index;
        return index;
    } else {
        return iter->second;
    }
}

GLint Shaders::returnShaderVariableOffset(int uniform_index, int program_id) {
    Program::UniformOffsetCacheMap::iterator iter =
        programs[program_id].uniform_offset_cache.find(uniform_index);
    if (iter == programs[program_id].uniform_offset_cache.end()) {
        GLuint index = uniform_index;
        GLint offset;
        glGetActiveUniformsiv(programs[program_id].gl_program, 1, &index, GL_UNIFORM_OFFSET, &offset);
        programs[program_id].uniform_offset_cache[uniform_index] = offset;
        return offset;
    } else {
        return iter->second;
    }
}

GLint Shaders::returnShaderBlockSize(int block_index, int program_id) {
    Program::UniformOffsetCacheMap::iterator iter =
        programs[program_id].uniform_offset_cache.find(-block_index - 1);
    if (iter == programs[program_id].uniform_offset_cache.end()) {
        GLint block_size;
        glGetActiveUniformBlockiv(programs[program_id].gl_program, block_index, GL_UNIFORM_BLOCK_DATA_SIZE, &block_size);
        programs[program_id].uniform_offset_cache[-block_index - 1] = block_size;
        return block_size;
    } else {
        return iter->second;
    }
}

GLint Shaders::returnShaderAttrib(const std::string &name, int which) {
    Program::AttribAddressCacheMap::iterator iter =
        programs[which].attrib_address_cache.find(name);
    if (iter == programs[which].attrib_address_cache.end()) {
        GLint addr = glGetAttribLocation(programs[which].gl_program, name.c_str());
        programs[which].attrib_address_cache[name] = addr;
        return addr;
    } else {
        return iter->second;
    }
}

int Shaders::returnProgram(std::string name, OptionalShaders optional_shaders) {
    std::string short_name;
    if (name.find(' ') != std::string::npos) {
        short_name = name.substr(0, name.find(' '));
    } else {
        short_name = name;
    }

    unsigned num_programs = programs.size();
    for (unsigned i = 0; i < num_programs; ++i) {
        if (name == programs[i].name) {
            return i;
        }
    }

    PROFILER_ZONE(g_profiler_ctx, "Assemble shader program");

    programs.push_back(Program());
    Program &program = programs.back();

    program.name = name;
    program.gl_program = UNLOADED_SHADER_ID;

    std::vector<std::string> definitions;
    size_t name_len = name.length() + 1;
    int def_start = -1;
    const int min_definition_length = 3;  // Must be at least 3 letters long

    for (size_t i = 0; i < name_len; ++i) {
        switch (name[i]) {
            case '#':
                def_start = i + 1;
                break;
            case ' ':
            case '\0':
                if (def_start != -1 && def_start < static_cast<int>(i) - (min_definition_length - 1)) {
                    definitions.push_back(
                        std::string(&name[def_start], i - def_start));
                }
                def_start = -1;
                break;
        }
    }

    std::string vertex_path = GetShaderPath(short_name, shader_dir_path, _vertex);
    std::string fragment_path = GetShaderPath(short_name, shader_dir_path, _fragment);
    std::string geom_path = GetShaderPath(short_name, shader_dir_path, _geom);
    std::string tess_eval_path = GetShaderPath(short_name, shader_dir_path, _tess_eval);
    std::string tess_ctrl_path = GetShaderPath(short_name, shader_dir_path, _tess_ctrl);

    if (optional_shaders & kGeometry && FileExists(geom_path.c_str(), kDataPaths | kModPaths)) {
        definitions.push_back("HAS_GEOM");
    }

    if (optional_shaders & kTesselation) {
        if (FileExists(tess_eval_path.c_str(), kDataPaths | kModPaths)) {
            definitions.push_back("HAS_TESS_EVAL");
        }

        if (FileExists(tess_ctrl_path.c_str(), kDataPaths | kModPaths)) {
            definitions.push_back("HAS_TESS_CTRL");
        }
    }

    for (int &shader_id : program.shader_ids) {
        shader_id = MISSING_GEOM_SHADER_ID;
    }

    program.shader_ids[_vertex] = returnShader(vertex_path.c_str(), _vertex, definitions);
    program.shader_ids[_fragment] = returnShader(fragment_path.c_str(), _fragment, definitions);

    if (optional_shaders & kGeometry && FileExists(geom_path.c_str(), kDataPaths | kModPaths)) {
        program.shader_ids[_geom] = returnShader(geom_path.c_str(), _geom, definitions);
    }

    if (optional_shaders & kTesselation) {
        if (FileExists(tess_eval_path.c_str(), kDataPaths | kModPaths)) {
            program.shader_ids[_tess_eval] = returnShader(tess_eval_path.c_str(), _tess_eval, definitions);
        }

        if (FileExists(tess_ctrl_path.c_str(), kDataPaths | kModPaths)) {
            program.shader_ids[_tess_ctrl] = returnShader(tess_ctrl_path.c_str(), _tess_ctrl, definitions);
        }
    }

    return programs.size() - 1;
}

bool Shaders::IsProgramTransparent(int which_program) {
    return shaders[programs[which_program].shader_ids[_fragment]].transparent;
}

bool Shaders::DoesProgramUseTangent(int which_program) {
    return shaders[programs[which_program].shader_ids[_fragment]].use_tangent ||
           shaders[programs[which_program].shader_ids[_vertex]].use_tangent;
}

void Shaders::createProgram(int which_program) {
    PROFILER_ZONE(g_profiler_ctx, "Shaders::createProgram");
    CHECK_GL_ERROR();
    Program &program = programs[which_program];
    if (program.gl_program != UNLOADED_SHADER_ID) {
        return;
    }
    program.gl_program = glCreateProgram();
    createShader(program.shader_ids[_vertex], _vertex);
    for (int i = _geom; i < kMaxShaderTypes; ++i) {
        if (program.shader_ids[i] >= 0) {
            bool success = createShader(program.shader_ids[i], (ShaderType)i);
            // FIXME: this is a really bad hack
            if (!success) {
                program.shader_ids[i] = MISSING_GEOM_SHADER_ID;
            }
        }
    }
    createShader(program.shader_ids[_fragment], _fragment);

    // So, the specification says that if the out refered to in the shader
    // doesn't exist, this call should be ignored, but the driver on MacOSX
    // for iris doesn't. Which causes a large amount of warnings to be generated
    // So far only envobject has two frag outputs, so the quick simple solution it to
    // check for envobject shader
    // Is this a good argument for using .xml files to define shader programs?
    // Or should we rather use a pragma in the shader to define this value?

    Shader *frag_shader = &shaders[program.shader_ids[_fragment]];
    if (frag_shader->depth_only == false) {
        if (frag_shader->bind_out_color) {
            glBindFragDataLocation(program.gl_program, 0, "out_color");
            LOGI << "Binding out color for " << frag_shader->name << std::endl;
        }

        if (frag_shader->particle == false) {
            if (frag_shader->bind_out_vel) {
                glBindFragDataLocation(program.gl_program, 1, "out_vel");
                LOGI << "Binding out vel for " << frag_shader->name << std::endl;
            }
        }
    }

    glAttachShader(program.gl_program, shaders[program.shader_ids[_vertex]].gl_shader);
    for (int i = _geom; i < kMaxShaderTypes; ++i) {
        if (program.shader_ids[i] >= 0) {
            glAttachShader(program.gl_program, shaders[program.shader_ids[(ShaderType)i]].gl_shader);
        }
    }
    glAttachShader(program.gl_program, shaders[program.shader_ids[_fragment]].gl_shader);
    glLinkProgram(program.gl_program);

    int status = 0;
    glGetProgramiv(program.gl_program, GL_LINK_STATUS, &status);
#ifndef SHADER_WARNINGS
    if (!status) {
#endif
        int len = 0;
        char buffer[1025] = {0};
        glGetProgramInfoLog(program.gl_program, 1024, &len, buffer);
        if (len != 0) {
            char errorbuffer[512];
            FormatString(errorbuffer, 512, "Error(s) linking program \"%s\"", program.name.c_str());
            if (status == 0 || config["shader_debug"].toNumber<bool>()) {
                // fatal error or shader debugging requested, show to user
                ErrorResponse response = DisplayError(errorbuffer, buffer, _ok_cancel_retry);

                if (response == _retry) {
                    ReloadShader(program.shader_ids[_vertex], _vertex);
                    ReloadShader(program.shader_ids[_geom], _geom);
                    ReloadShader(program.shader_ids[_fragment], _fragment);

                    glDeleteProgram(program.gl_program);
                    program.gl_program = UNLOADED_SHADER_ID;

                    createProgram(which_program);
                    return;
                }
            } else {
                // only log it
                LOGE << errorbuffer << " " << buffer << std::endl;
            }
        }
#ifndef SHADER_WARNINGS
    }
#endif

    if (config["shader_debug"].toNumber<bool>()) {
        // Log some uniform data from shader
        LOGI << "Shader: " << program.name << std::endl;
        GLint num_raw_uniforms = 0;
        glGetProgramiv(program.gl_program, GL_ACTIVE_UNIFORMS, &num_raw_uniforms);
        LOGI << "Active uniforms: " << num_raw_uniforms << std::endl;

        GLint max_name_uni_len = 0;
        glGetProgramiv(program.gl_program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_name_uni_len);
        LOGI << "Uniform name max length: " << max_name_uni_len << std::endl;

        std::vector<char> name_buf(max_name_uni_len + 1, '\0');

        LOGI << "Uniforms:" << std::endl;
        for (int i = 0; i < num_raw_uniforms; i++) {
            GLsizei real_len = 0;
            GLint attr_size = 0;
            GLenum gltype = GL_NONE;
            glGetActiveUniform(program.gl_program, i, max_name_uni_len, &real_len, &attr_size, &gltype, &name_buf[0]);
            LOGI << "Uniform #" << i << " " << &name_buf[0] << " length " << real_len << ", size " << attr_size << " type " << gltype << std::endl;
        }
    }

    int active_attribs = 0;
    glGetProgramiv(program.gl_program, GL_ACTIVE_ATTRIBUTES, &active_attribs);
    LOGD << program.name << " attribs: " << active_attribs << std::endl;

    CHECK_GL_ERROR();

    program.tex_uniform.resize(32);
    program.tex_uniform[0] = glGetUniformLocation(program.gl_program, "tex0");
    program.tex_uniform[1] = glGetUniformLocation(program.gl_program, "tex1");
    program.tex_uniform[2] = glGetUniformLocation(program.gl_program, "tex2");
    program.tex_uniform[3] = glGetUniformLocation(program.gl_program, "tex3");
    program.tex_uniform[4] = glGetUniformLocation(program.gl_program, "tex4");
    program.tex_uniform[5] = glGetUniformLocation(program.gl_program, "tex5");
    program.tex_uniform[6] = glGetUniformLocation(program.gl_program, "tex6");
    program.tex_uniform[7] = glGetUniformLocation(program.gl_program, "tex7");
    program.tex_uniform[8] = glGetUniformLocation(program.gl_program, "tex8");
    program.tex_uniform[9] = glGetUniformLocation(program.gl_program, "tex9");
    program.tex_uniform[10] = glGetUniformLocation(program.gl_program, "tex10");
    program.tex_uniform[11] = glGetUniformLocation(program.gl_program, "tex11");
    program.tex_uniform[12] = glGetUniformLocation(program.gl_program, "tex12");
    program.tex_uniform[13] = glGetUniformLocation(program.gl_program, "tex13");
    program.tex_uniform[14] = glGetUniformLocation(program.gl_program, "tex14");
    program.tex_uniform[15] = glGetUniformLocation(program.gl_program, "tex15");
    program.tex_uniform[16] = glGetUniformLocation(program.gl_program, "tex16");
    program.tex_uniform[17] = glGetUniformLocation(program.gl_program, "tex17");
    program.tex_uniform[18] = glGetUniformLocation(program.gl_program, "tex18");
    program.tex_uniform[19] = glGetUniformLocation(program.gl_program, "tex19");
    program.tex_uniform[20] = glGetUniformLocation(program.gl_program, "tex20");
    program.tex_uniform[21] = glGetUniformLocation(program.gl_program, "tex21");
    program.tex_uniform[22] = glGetUniformLocation(program.gl_program, "tex22");
    program.tex_uniform[23] = glGetUniformLocation(program.gl_program, "tex23");
    program.tex_uniform[24] = glGetUniformLocation(program.gl_program, "tex24");
    program.tex_uniform[25] = glGetUniformLocation(program.gl_program, "tex25");
    program.tex_uniform[26] = glGetUniformLocation(program.gl_program, "tex26");
    program.tex_uniform[27] = glGetUniformLocation(program.gl_program, "tex27");
    program.tex_uniform[28] = glGetUniformLocation(program.gl_program, "tex28");
    program.tex_uniform[29] = glGetUniformLocation(program.gl_program, "tex29");
    program.tex_uniform[30] = glGetUniformLocation(program.gl_program, "tex30");
    program.tex_uniform[31] = glGetUniformLocation(program.gl_program, "tex31");
    program.attrib_address_cache.clear();
    program.uniform_address_cache.clear();
    program.uniform_value_cache.clear();
    program.uniform_offset_cache.clear();

    program.commonShaderUniforms.resize(MAX_COMMON_UNIFORMS);
    program.commonShaderUniforms[NORMAL_MATRIX] = glGetUniformLocation(program.gl_program, "normalMatrix");
    program.commonShaderUniforms[MODEL_MATRIX] = glGetUniformLocation(program.gl_program, "modelMatrix");

    for (unsigned i = 0; i < 32; ++i) {
        if (program.tex_uniform[i] > 1000) {
            std::ostringstream oss;
            oss << "Program \"" << program.name << "\" tex uniform " << i << " is set to " << program.tex_uniform[i] << " which is outside"
                << " the normal range";
            LOGE << oss.str().c_str() << std::endl;
        }
    }

    GLuint shadow_cascade_idx = glGetUniformBlockIndex(program.gl_program, "ShadowCascades");
    if (shadow_cascade_idx != GL_INVALID_INDEX) {
        glUniformBlockBinding(program.gl_program, shadow_cascade_idx, UBO_SHADOW_CASCADES);
    }
}

Program *Shaders::GetCurrentProgram() {
    return bound_program == -1 ? 0 : &programs[bound_program];
}

GLint Shaders::GetTexUniform(int which) {
    return programs[bound_program].tex_uniform[which];
}

// Bind a shader program
void Shaders::setProgram(int which) {
    if (which == -1) {
        noProgram();
        return;
    }
    if (bound_program == which) return;
    CHECK_GL_ERROR();
    if (programs[which].gl_program == UNLOADED_SHADER_ID) {
        if (create_program_warning) {
            LOGW << "Loading shader which should be preloaded. " << level_path.GetOriginalPath() << ": " << programs[which].name << std::endl;
        }
        createProgram(which);
    }
    glUseProgram(programs[which].gl_program);
    Textures::Instance()->InvalidateBindCache();
    bound_program = which;
    CHECK_GL_ERROR();
}

// Stop using shaders
void Shaders::noProgram() {
    if (bound_program == -1) return;
    glUseProgram(0);
    bound_program = -1;
}

void Shaders::SetUniformMat4(const std::string &var_name, const GLfloat *data) {
    GLint var_id = returnShaderVariable(var_name, bound_program);
    SetUniformMat4(var_id, data);
}

void Shaders::SetUniformMat4(GLint var_id, const GLfloat *data) {
    if (var_id == -1) {
        return;
    }

    std::vector<GLfloat> &value = programs[bound_program].uniform_value_cache[var_id];

    bool changed = false;
    if (value.size() != 16) {
        value.resize(16);
        changed = true;
    }

    for (int i = 0; i < 16; i++) {
        if (value[i] != data[i]) {
            changed = true;
            value[i] = data[i];
        }
    }

    if (changed) {
        glUniformMatrix4fv(var_id, 1, GL_FALSE, data);
    }
}

void Shaders::SetUniformMat3(const std::string &var_name, const GLfloat *data) {
    GLint var_id = returnShaderVariable(var_name, bound_program);
    SetUniformMat3(var_id, data);
}

void Shaders::SetUniformMat3(GLint var_id, const GLfloat *data) {
    if (var_id == -1) {
        return;
    }

    std::vector<GLfloat> &value = programs[bound_program].uniform_value_cache[var_id];

    bool changed = false;
    if (value.size() != 9) {
        value.resize(9);
        changed = true;
    }

    for (int i = 0; i < 9; i++) {
        if (value[i] != data[i]) {
            changed = true;
            value[i] = data[i];
        }
    }

    if (changed) {
        glUniformMatrix3fv(var_id, 1, GL_FALSE, data);
    }
}

void Shaders::SetUniformVec3(const std::string &var_name, const vec3 &data_vec) {
    CHECK_GL_ERROR();
    GLint var_id = returnShaderVariable(var_name, bound_program);
    SetUniformVec3(var_id, data_vec);
    CHECK_GL_ERROR();
}

void Shaders::SetUniformVec3(GLint var_id, const vec3 &data_vec) {
    if (var_id == -1) {
        return;
    }

    CHECK_GL_ERROR();
    GLfloat *data = (GLfloat *)&data_vec;
    SetUniformVec3(var_id, data);
    CHECK_GL_ERROR();
}

void Shaders::SetUniformVec3(GLint var_id, const GLfloat *data) {
    if (var_id == -1) {
        return;
    }

    CHECK_GL_ERROR();
    std::vector<GLfloat> &value = programs[bound_program].uniform_value_cache[var_id];

    bool changed = false;
    if (value.size() != 3) {
        value.resize(3);
        changed = true;
    }

    for (int i = 0; i < 3; i++) {
        if (value[i] != data[i]) {
            changed = true;
            value[i] = data[i];
        }
    }

    if (changed) {
        glUniform3fv(var_id, 1, data);
    }
    CHECK_GL_ERROR();
}

void Shaders::SetUniformVec2(const std::string &var_name, const vec2 &data_vec) {
    CHECK_GL_ERROR();
    GLint var_id = returnShaderVariable(var_name, bound_program);
    SetUniformVec2(var_id, data_vec);
    CHECK_GL_ERROR();
}

void Shaders::SetUniformVec2(GLint var_id, const vec2 &data_vec) {
    if (var_id == -1) {
        return;
    }

    CHECK_GL_ERROR();
    GLfloat *data = (GLfloat *)&data_vec;
    SetUniformVec2(var_id, data);
    CHECK_GL_ERROR();
}

void Shaders::SetUniformVec2(GLint var_id, const GLfloat *data) {
    if (var_id == -1) {
        return;
    }

    CHECK_GL_ERROR();
    std::vector<GLfloat> &value = programs[bound_program].uniform_value_cache[var_id];

    bool changed = false;
    if (value.size() != 2) {
        value.resize(2);
        changed = true;
    }

    for (int i = 0; i < 2; i++) {
        if (value[i] != data[i]) {
            changed = true;
            value[i] = data[i];
        }
    }

    if (changed) {
        glUniform2fv(var_id, 1, data);
    }
    CHECK_GL_ERROR();
}

void Shaders::SetUniformFloat(const std::string &var_name, const float &data) {
    CHECK_GL_ERROR();
    GLint var_id = returnShaderVariable(var_name, bound_program);
    SetUniformFloat(var_id, data);
    CHECK_GL_ERROR();
}

void Shaders::SetUniformFloat(GLint var_id, const float &data) {
    if (var_id == -1) {
        return;
    }

    std::vector<GLfloat> &value = programs[bound_program].uniform_value_cache[var_id];

    bool changed = false;
    if (value.size() != 1) {
        value.resize(1);
        changed = true;
    }

    if (value[0] != data) {
        changed = true;
        value[0] = data;
    }

    if (changed) {
        glUniform1f(var_id, data);
    }
}

void Shaders::SetUniformInt(const std::string &var_name, const int &data, ForceFlag force_flag) {
    GLint var_id = returnShaderVariable(var_name, bound_program);
    SetUniformInt(var_id, data, force_flag);
}

void Shaders::SetUniformInt(GLint var_id, const int &data, ForceFlag force_flag) {
    if (var_id == -1) {
        return;
    }

    std::vector<GLfloat> &value = programs[bound_program].uniform_value_cache[var_id];

    bool changed = false;
    if (value.size() != 1) {
        value.resize(1);
        changed = true;
    }

    if (value[0] != (GLfloat)data) {
        changed = true;
        value[0] = (GLfloat)data;
    }

    if (changed || force_flag == kForce) {
        glUniform1i(var_id, data);
    }
}

int Shaders::GetProgramBlendSrc(int which_program) {
    return shaders[programs[which_program].shader_ids[_fragment]].blend_src;
}

int Shaders::GetProgramBlendDst(int which_program) {
    return shaders[programs[which_program].shader_ids[_fragment]].blend_dst;
}

void Shaders::SetUniformMat4Array(const std::string &var_name, const std::vector<mat4> &transforms) {
    GLint var_id = returnShaderVariable(var_name, bound_program);
    SetUniformMat4Array(var_id, transforms);
}

void Shaders::SetUniformMat4Array(GLint var_id, const std::vector<mat4> &transforms) {
    if (var_id == -1) {
        return;
    }

    glUniformMatrix4fv(var_id, transforms.size(), GL_FALSE, &transforms[0].entries[0]);
}

void Shaders::SetUniformVec4Array(const std::string &var_name, const std::vector<vec4> &val) {
    GLint var_id = returnShaderVariable(var_name, bound_program);
    SetUniformVec4Array(var_id, val);
}

void Shaders::SetUniformVec4Array(GLint var_id, const std::vector<vec4> &val) {
    if (var_id == -1) {
        return;
    }

    glUniform4fv(var_id, val.size(), &val[0].entries[0]);
}

void Shaders::SetUniformVec3Array(const std::string &var_name, const std::vector<vec3> &val) {
    GLint var_id = returnShaderVariable(var_name, bound_program);
    SetUniformVec3Array(var_id, val);
}

void Shaders::SetUniformVec3Array(GLint var_id, const std::vector<vec3> &val) {
    if (var_id == -1) {
        return;
    }

    glUniform3fv(var_id, val.size(), &val[0].entries[0]);
}

void Shaders::SetUniformVec3Array(GLint var_id, const vec3 *val, int size) {
    if (var_id == -1) {
        return;
    }

    glUniform3fv(var_id, size, (const GLfloat *)val);
}

void Shaders::ResetVRAM() {
    for (auto &shader : shaders) {
        shader.gl_shader = UNLOADED_SHADER_ID;
    }
    for (auto &program : programs) {
        program.gl_program = UNLOADED_SHADER_ID;
        program.attrib_address_cache.clear();
        program.uniform_address_cache.clear();
        program.uniform_value_cache.clear();
        program.uniform_offset_cache.clear();
    }
}

int Shaders::GetUBOBindIndex(int shader_id, const char *name) {
    LOG_ASSERT(shader_id >= 0 && shader_id < (int)programs.size());
    Program *program = &programs[shader_id];
    LOG_ASSERT(program->gl_program != UNLOADED_SHADER_ID);
    Program::UniformAddressCacheMap::iterator iter =
        program->uniform_address_cache.find(name);
    if (iter == program->uniform_address_cache.end()) {
        int programHandle = program->gl_program;
        GLuint blockIndex = glGetUniformBlockIndex(programHandle, name);
        program->uniform_address_cache[name] = blockIndex;
        return blockIndex;
    } else {
        return iter->second;
    }
}

void Shaders::SetUniformVec4(const std::string &var_name, const vec4 &data_vec) {
    CHECK_GL_ERROR();
    GLint var_id = returnShaderVariable(var_name, bound_program);
    SetUniformVec4(var_id, data_vec);
    CHECK_GL_ERROR();
}

void Shaders::SetUniformVec4(GLint var_id, const vec4 &data_vec) {
    if (var_id == -1) {
        return;
    }

    CHECK_GL_ERROR();
    GLfloat *data = (GLfloat *)&data_vec;
    SetUniformVec4(var_id, data);
    CHECK_GL_ERROR();
}

void Shaders::SetUniformVec4(GLint var_id, const GLfloat *data) {
    if (var_id == -1) {
        return;
    }

    if (data == NULL) {
        LOGE << "Data pointer is null." << std::endl;
        return;
    }

    CHECK_GL_ERROR();
    std::vector<GLfloat> &value = programs[bound_program].uniform_value_cache[var_id];

    bool changed = false;
    if (value.size() != 4) {
        value.resize(4);
        changed = true;
    }

    for (int i = 0; i < 4; i++) {
        if (value[i] != data[i]) {
            changed = true;
            value[i] = data[i];
        }
    }

    if (changed) {
        glUniform4fv(var_id, 1, data);
    }
    CHECK_GL_ERROR();
}

std::string GetShaderPath(const std::string &shader_name, const std::string &shader_dir_path, ShaderType type) {
    switch (type) {
        case _vertex:
            return shader_dir_path + shader_name + ".vert";
        case _fragment:
            return shader_dir_path + shader_name + ".frag";
        case _geom:
            return shader_dir_path + shader_name + ".geom";
        case _tess_eval:
            return shader_dir_path + shader_name + ".tess_eval";
        case _tess_ctrl:
            return shader_dir_path + shader_name + ".tess_ctrl";
        default:
            return "";
    }
}

void ShaderUniform::Submit() {
    switch (type) {
        case ShaderUniformType::UNKNOWN:
        case ShaderUniformType::SAMPLER2D:
        case ShaderUniformType::SAMPLER3D:
        case ShaderUniformType::SAMPLERCUBE:
            break;
        case ShaderUniformType::INT:
            glUniform1fv(address, 1, cachedValue);
            break;
        case ShaderUniformType::FLOAT:
            glUniform1fv(address, 1, cachedValue);
            break;
        case ShaderUniformType::VEC2:
            glUniform2fv(address, 1, cachedValue);
            break;
        case ShaderUniformType::VEC3:
            glUniform3fv(address, 1, cachedValue);
            break;
        case ShaderUniformType::VEC4:
            glUniform4fv(address, 1, cachedValue);
            break;
        case ShaderUniformType::MAT3:
            glUniformMatrix3fv(address, 1, GL_FALSE, cachedValue);
            break;
        case ShaderUniformType::MAT4:
            glUniformMatrix4fv(address, 1, GL_FALSE, cachedValue);
            break;
        default:
            break;
    }
}
