//-----------------------------------------------------------------------------
//           Name: asmodule.cpp
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
#include "asmodule.h"

#include <Scripting/scriptfile.h>
#include <Scripting/angelscript/add_on/scriptarray/scriptarray.h>
#include <Scripting/angelscript/add_on/scriptdictionary/scriptdictionary.h>

#include <Math/vec2.h>
#include <Math/vec3.h>
#include <Math/vec4.h>
#include <Math/ivec2.h>
#include <Math/ivec3.h>
#include <Math/ivec4.h>

#include <Internal/config.h>
#include <Internal/profiler.h>
#include <Internal/modloading.h>
#include <Internal/common.h>
#include <Internal/filesystem.h>

#include <GUI/IMUI/imgui.h>
#include <GUI/IMUI/imui.h>
#include <GUI/IMUI/im_container.h>
#include <GUI/IMUI/im_divider.h>
#include <GUI/IMUI/im_element.h>
#include <GUI/IMUI/im_image.h>
#include <GUI/IMUI/im_message.h>
#include <GUI/IMUI/im_selection_list.h>
#include <GUI/IMUI/im_spacer.h>
#include <GUI/IMUI/im_text.h>
#include <GUI/IMUI/im_behaviors.h>
#include <GUI/IMUI/imui_state.h>
#include <GUI/IMUI/im_support.h>
#include <GUI/IMUI/im_tween.h>
#include <GUI/IMUI/im_behaviors.h>
#include <GUI/IMUI/imui_state.h>

#include <AI/navmesh.h>
#include <Compat/fileio.h>
#include <Version/version.h>
#include <Logging/logdata.h>
#include <Utility/strings.h>
#include <Game/attackscript.h>
#include <Graphics/bonetransform.h>

#include <sstream>
#include <cstring>

enum ASErrType {
    _ERR = 0,
    _WARN = 1,
    _INFO = 2
};

std::string StringFromErrType(ASErrType type) {
    switch (type) {
        case _ERR:
            return "ERR";
        case _WARN:
            return "WARN";
        case _INFO:
            return "INFO";
    }
    return "";
}

struct ASErrReport {
    ASErrType type;
    unsigned line_number;
    unsigned column_number;
    std::string message;
    std::string file_name;

    ASErrReport(const std::string& err_line);
};

ASErrReport::ASErrReport(const std::string& err_line) {
    // Lines are like this:
    //  script (544, 20) : INFO : Compiling const float offset
    size_t open_paren = err_line.find('(');
    size_t comma = err_line.find(',');
    std::string first_num = err_line.substr(open_paren + 1, comma - open_paren - 1);
    size_t close_paren = err_line.find(')');
    std::string second_num = err_line.substr(comma + 2, close_paren - comma - 2);
    size_t first_colon = err_line.find(':');
    size_t second_colon = err_line.find(':', first_colon + 1);
    std::string type_str = err_line.substr(first_colon + 2,
                                           second_colon - first_colon - 3);
    message = err_line.substr(second_colon + 2);
    line_number = atoi(first_num.c_str());
    column_number = atoi(second_num.c_str());
    if (type_str == "ERR ") {
        type = _ERR;
    } else if (type_str == "WARN") {
        type = _WARN;
    } else if (type_str == "INFO") {
        type = _INFO;
    }
}

typedef std::list<ASErrReport> ErrList;

std::string CorrectASError(const std::string& input_error,
                           const ScriptFile& script_file) {
    // Break down error message into a list of errors
    ErrList errors;
    {
        size_t line_break = 0;
        unsigned input_error_size = input_error.size();
        do {
            size_t next_line_break = input_error.find('\n', line_break + 1);
            if (next_line_break == std::string::npos) {
                next_line_break = input_error_size;
            }
            if (next_line_break - line_break < 10) {
                break;
            }
            std::string err_line = input_error.substr(line_break,
                                                      next_line_break - line_break);
            errors.push_back(ASErrReport(err_line));
            line_break = next_line_break;
        } while (line_break != input_error_size);
    }

    // Correct line numbers and file names
    {
        ErrList::iterator iter = errors.begin();
        for (; iter != errors.end(); ++iter) {
            ASErrReport& err = (*iter);
            LineFile line_file = script_file.GetCorrectedLine(err.line_number);
            err.line_number = line_file.line_number;
            err.file_name = line_file.file.GetFullPath();
            err.file_name = err.file_name.substr(err.file_name.rfind('/') + 1);
        }
    }

    // Create new error message string
    std::string err_message;
    {
        std::ostringstream oss;
        ErrList::iterator iter = errors.begin();
        for (; iter != errors.end(); ++iter) {
            ASErrReport& err = (*iter);
            if (err.type != _INFO) {
                oss << "    " << StringFromErrType(err.type) << ": "
                    << err.file_name << " (" << err.line_number
                    << ", " << err.column_number << ") "
                    << err.message << "\n";
            } else {
                oss << err.message << "\n";
            }
        }
        err_message = oss.str();
    }

    return err_message;
}

int ASModule::CompileScriptFromText(const std::string& text) {
    func_map_.clear();
    fast_var_ptr_map.clear();
    bool retry = true;
    while (retry) {
        const std::string& script = text;

        std::string filename = "Hard-coded text";
        // Add the script sections that will be compiled into executable code.
        // If we want to combine more than one file into the same script, then
        // we can call AddScriptSection() several times for the same module and
        // the script engine will treat them all as if they were one. The script
        // section name, will allow us to localize any errors in the script code.
        int r = module_->AddScriptSection("script", &script[0], script.length());
        if (r < 0) {
            static const int kBufSize = 512;
            char buf[kBufSize];
            FormatString(buf, kBufSize, "Error in \"%s\"", filename.c_str());
            FatalError(buf, "AddScriptSection() failed");
            return -1;
        }

        // Compile the script. If there are any compiler messages they will
        // be written to the message stream that we set right after creating the
        // script engine. If there are no errors, and no warnings, nothing will
        // be written to the stream.
        active_angelscript_error_string->clear();
        r = module_->Build();
        if (r < 0) {
            ErrorResponse err;
            err = DisplayError(("Error in \"" + filename + "\"").c_str(),
                               active_angelscript_error_string->c_str(),
                               _ok_cancel_retry);
            if (err != _retry) {
                return -1;
            }
        } else if (!active_angelscript_error_string->empty()) {
            ErrorResponse err;
            err = DisplayError(("Warnings in \"" + filename + "\"").c_str(),
                               active_angelscript_error_string->c_str(),
                               _ok_cancel_retry);
            if (err == _continue) {
                retry = false;
            }
        } else {
            retry = false;
        }

        script_path_ = Path();
    }

    // The engine doesn't keep a copy of the script sections after Build() has
    // returned. So if the script needs to be recompiled, then all the script
    // sections must be added again.

    // If we want to have several scripts executing at different times but
    // that have no direct relation with each other, then we can compile them
    // into separate script modules. Each module use their own namespace and
    // scope, so function names, and global variables will not conflict with
    // each other.

    return 0;
}

class CBytecodeStream : public asIBinaryStream {
   public:
    CBytecodeStream(FILE* fp) : f(fp) {}

    int Write(const void* ptr, asUINT size) override {
        if (size == 0) return -1;
        return fwrite(ptr, size, 1, f);
    }
    int Read(void* ptr, asUINT size) override {
        if (size == 0) return -1;
        return fread(ptr, size, 1, f);
    }

   protected:
    FILE* f;
};

const uint32_t script_bytecode_ver = 10;
const uint32_t script_bytecode_program_build_length = 512;

int ASModule::CompileScript(const Path& path) {
    fast_var_ptr_map.clear();
    func_map_.clear();
    bool retry = true;
    while (retry) {
        if (module_->GetEngine()) {
            asIScriptEngine* engine = module_->GetEngine();
            module_->Discard();
            AttachToEngine(engine);
        }
        // Load the script file, along with all its include files
        const ScriptFile& script_file = *ScriptFileUtil::GetScriptFile(path);
        script_file_ptr_ = &script_file;
        modified_ = script_file.latest_modification;

        // Hash the contents of the script, and use that to find the storage location
        // of the script bytecode
        bool bytecode_loaded = false;
        int last_slash = 0;
        std::string original_path = path.GetOriginalPathStr();
        for (int i = original_path.length() - 2; i >= 0; --i) {
            if (original_path[i] == '/' || original_path[i] == '\\') {
                last_slash = i + 1;
                break;
            }
        }
        const char* script_name = &original_path[last_slash];

        if (config["dump_include_scripts"].toBool()) {
            std::stringstream debug_script_path;
            debug_script_path << GetWritePath(script_file.file_path.GetModsource()).c_str() << "Data/ScriptDebug/" << script_name << ".full.as";
            FILE* debug_script_file = my_fopen(debug_script_path.str().c_str(), "wb");
            LOGI << "Dumping script " << script_name << std::endl;
            if (debug_script_file) {
                fwrite(script_file.contents.c_str(), sizeof(char), script_file.contents.size(), debug_script_file);
                fclose(debug_script_file);
            }
        }

        static const int kBufSize = 512;
        char buf[kBufSize];
        FormatString(buf, kBufSize, "%sData/ScriptBytecode/%s.bytecode", GetWritePath(script_file.file_path.GetModsource()).c_str(), script_name);
        FILE* file = my_fopen(buf, "rb");
        if (file) {
            // Compare the bytecode hash to the script hash to make sure it matches

            // We check the base version first, because this allows us to change anything that comes after.
            uint32_t ver;
            fread(&ver, sizeof(uint32_t), 1, file);
            if (script_bytecode_ver == ver) {
                char build_id_string[script_bytecode_program_build_length + 1];
                memset(build_id_string, '\0', script_bytecode_program_build_length + 1);
                fread(build_id_string, sizeof(char) * script_bytecode_program_build_length, 1, file);

                unsigned long file_hash;
                fread(&file_hash, sizeof(unsigned long), 1, file);
                if (file_hash == script_file.hash && strcmp(build_id_string, GetFullBuildString().c_str()) == 0) {
                    // Hash matches, so load the bytecode into the module
                    CBytecodeStream byte_code_stream(file);
                    if (module_->LoadByteCode(&byte_code_stream) < 0) {
                        LOGE << "Problem loading saved script bytecode from file \"" << file << "\", recompiling." << std::endl;
                    } else {
                        bytecode_loaded = true;
                        retry = false;
                    }
                } else {
                    LOGI << "Script byte code for " << buf << " appears outdated, recompiling." << std::endl;
                }
            } else {
                LOGI << "Script byte code base version for " << buf << " appears outdated, recompiling." << std::endl;
            }
            fclose(file);
        }
        if (!bytecode_loaded) {
            std::string script = script_file.contents;
            // Get the last section of the path and store it for pretty filename display
            int last_slash_pos = path.GetFullPathStr().rfind('/');
            std::string filename = path.GetFullPathStr().substr(last_slash_pos + 1);

            // Add the script contents to the module
            int r = module_->AddScriptSection("script", &script[0], script.length());
            if (r < 0) {
                static const int kBufSize = 512;
                char buf[kBufSize];
                FormatString(buf, kBufSize, "Error in \"%s\"", filename.c_str());
                FatalError(buf, "AddScriptSection() failed");
                return -1;
            }

            // Clear error messages
            active_angelscript_error_string->clear();

            // Compile the module
            r = module_->Build();
            if (r < 0) {
                // There was an error! Print error, and retry
                std::string title = "Error in \"" + filename + "\" from mod " + ModLoading::Instance().GetModName(script_file.file_path.GetModsource());
                bool showModPollution = !config.HasKey("list_include_files_when_logging_mod_errors") ||
                                        config["list_include_files_when_logging_mod_errors"].toBool();
                std::string corrected_error =
                    title + ":\n\n" +
                    (showModPollution ? (script_file.GetModPollutionInformation() + "\n") : "") +
                    CorrectASError(*(active_angelscript_error_string),
                                   script_file);
                ErrorResponse err;
                err = DisplayError(title.c_str(),
                                   corrected_error.c_str(),
                                   _ok_cancel_retry);
                if (err != _retry) {
                    return -1;
                }
            } else if (!active_angelscript_error_string->empty()) {
                // There were warnings! Print warnings, and retry or continue
                std::string title = "Warnings in \"" + filename + "\" from mod " + ModLoading::Instance().GetModName(script_file.file_path.GetModsource());
                bool showModPollution = !config.HasKey("list_include_files_when_logging_mod_errors") ||
                                        config["list_include_files_when_logging_mod_errors"].toBool();
                std::string corrected_error =
                    title + ":\n\n" +
                    (showModPollution ? (script_file.GetModPollutionInformation() + "\n") : "") +
                    CorrectASError(*(active_angelscript_error_string),
                                   script_file);
                ErrorResponse err;
                err = DisplayError(title.c_str(),
                                   corrected_error.c_str(),
                                   _ok_cancel_retry);
                if (err == _continue) {
                    retry = false;
                }
            } else {
                // Compile successful!
                retry = false;
            }

// If compile was successful, save bytecode

// Disabling on linux amd64 platoform because this routine does not work on that platform atm.
// waiting for feedback on issue
#if !(PLATFORM_LINUX && PLATFORM_64)
            if (!retry) {
                FILE* file = my_fopen(buf, "wb");
                if (file) {
                    fwrite(&script_bytecode_ver, sizeof(uint32_t), 1, file);

                    char build_id_string[script_bytecode_program_build_length + 1];
                    memset(build_id_string, '\0', script_bytecode_program_build_length + 1);
                    memcpy(build_id_string, GetFullBuildString().c_str(), sizeof(char) * GetFullBuildString().length());
                    fwrite(build_id_string, sizeof(char) * script_bytecode_program_build_length, 1, file);

                    unsigned long temp = script_file.hash;
                    fwrite(&temp, sizeof(unsigned long), 1, file);
                    CBytecodeStream byte_code_stream(file);
                    if (module_->SaveByteCode(&byte_code_stream) < 0) {
                        DisplayError("Error", "Problem saving script bytecode: ");
                    }
                    fclose(file);
                } else {
                    char err_msg[kBufSize];
                    FormatString(err_msg, kBufSize, "Problem saving script bytecode to %s", buf);
                    DisplayError("Error", err_msg);
                }
            }
#endif
        }

        script_path_ = path;
    }
    return 0;
}

bool ASModule::SourceChanged() {
    if (ScriptFileUtil::GetLatestModification(script_path_) > modified_) {
        return true;
    } else {
        return false;
    }
}

void ASModule::PrintScriptClassVars(asIScriptObject* obj) {
    printf("Script class vars:\n");
    int c = obj->GetPropertyCount();
    for (int n = 0; n < c; ++n) {
        const char* name = obj->GetPropertyName(n);
        printf("%s\n", name);
    }
}

void FillVar(VarStorage& new_var, const char* name, int type_id, void* ptr, std::list<ScriptObjectInstance>& handle_var_list, asIScriptModule* module);

void ScriptObject::Populate(asIScriptObject* obj, std::list<ScriptObjectInstance>& handle_var_list, asIScriptModule* module) {
    int c = obj->GetPropertyCount();
    for (int n = 0; n < c; n++) {
        const char* name = obj->GetPropertyName(n);
        int type_id = obj->GetPropertyTypeId(n);
        void* ptr = obj->GetAddressOfProperty(n);
        VarStorage new_var;
        FillVar(new_var, name, type_id, ptr, handle_var_list, module);
        storage.push_back(new_var);
    }
}

void ScriptObject::destroy() {
    std::list<VarStorage>::iterator iter = storage.begin();
    for (; iter != storage.end(); ++iter) {
        iter->destroy();
    }
}

std::string VarStorage::GetString(unsigned depth) {
    std::string type_str;
    std::string val_str;
    std::ostringstream oss;

    if (type == _vs_bool) {
        type_str = "bool";
        val_str = *(bool*)var ? "true" : "false";
    } else if (type == _vs_int8) {
        type_str = "int8";
        oss << *(char*)var;
        val_str = oss.str();
    } else if (type == _vs_int16) {
        type_str = "int16";
        oss << *(short*)var;
        val_str = oss.str();
    } else if (type == _vs_int32) {
        type_str = "int32";
        oss << *(int*)var;
        val_str = oss.str();
    } else if (type == _vs_int64) {
        type_str = "int64";
        oss << *(int64_t*)var;
        val_str = oss.str();
    } else if (type == _vs_uint8) {
        type_str = "uint8";
        oss << *(unsigned char*)var;
        val_str = oss.str();
    } else if (type == _vs_uint16) {
        type_str = "uint16";
        oss << *(unsigned short*)var;
        val_str = oss.str();
    } else if (type == _vs_uint32) {
        type_str = "uint32";
        oss << *(unsigned*)var;
        val_str = oss.str();
    } else if (type == _vs_uint64) {
        type_str = "uint64";
        oss << *(uint64_t*)var;
        val_str = oss.str();
    } else if (type == _vs_float) {
        type_str = "float";
        oss << *(float*)var;
        val_str = oss.str();
    } else if (type == _vs_enum) {
        type_str = "enum";
        for (int i = 0; i < size; i++) {
            oss << ((char*)var)[i] << " ";
        }
        val_str = oss.str();
    } else if (type == _vs_app_obj) {
        AppObject& ao = *(AppObject*)var;
        if (ao.type == _ao_vec3) {
            type_str = "vec3";
            vec3& vec = *(vec3*)ao.var;
            oss << "[" << vec[0] << ", " << vec[1] << ", " << vec[2] << "]";
            val_str = oss.str();
        } else if (ao.type == _ao_string) {
            type_str = "string";
            std::string& str = *(std::string*)ao.var;
            val_str = "\"" + str + "\"";
        } else {
            type_str = "unknown app object";
        }
    } else if (type == _vs_script_obj) {
        type_str = "script object";
        std::string total;
        for (unsigned i = 0; i < depth; ++i) {
            total += "    ";
        }
        ScriptObject& so = *(ScriptObject*)var;
        total += so.type_name + " " + name + ":";
        std::list<VarStorage>::iterator iter = so.storage.begin();
        for (; iter != so.storage.end(); ++iter) {
            total += "\n";
            total += (*iter).GetString(depth + 1);
        }
        return total;
    } else if (type == _vs_app_obj_handle) {
        type_str = "app obj handle";
    } else if (type == _vs_template) {
        TemplateObject& to = *(TemplateObject*)var;
        if (to.type == _to_array) {
            type_str = "array";
            type_str += "<" + to.sub_type_name + ">";
            std::string total;
            for (unsigned i = 0; i < depth; ++i) {
                total += "    ";
            }
            total += type_str + " " + name + ":";
            std::list<VarStorage>& storage = *(std::list<VarStorage>*)to.var;
            std::list<VarStorage>::iterator iter = storage.begin();
            for (; iter != storage.end(); ++iter) {
                total += "\n";
                total += (*iter).GetString(depth + 1);
            }
            return total;
        } else {
            type_str = "template";
        }
        type_str += "<" + to.sub_type_name + ">";

    } else {
        type_str = "unknown";
    }
    std::string total;
    for (unsigned i = 0; i < depth; ++i) {
        total += "    ";
    }
    total += type_str + " " + name + " = " + val_str;
    return total;
}

void VarStorage::destroy() {
    switch (type) {
        case _vs_bool:
            delete (bool*)var;
            break;
        case _vs_int8:
            delete (char*)var;
            break;
        case _vs_int16:
            delete (short*)var;
            break;
        case _vs_int32:
            delete (int*)var;
            break;
        case _vs_int64:
            delete (int64_t*)var;
            break;
        case _vs_uint8:
            delete (unsigned char*)var;
            break;
        case _vs_uint16:
            delete (unsigned short*)var;
            break;
        case _vs_uint32:
            delete (unsigned int*)var;
            break;
        case _vs_uint64:
            delete (uint64_t*)var;
            break;
        case _vs_float:
            delete (float*)var;
            break;
        case _vs_enum:
            delete[] (char*)var;
            break;
        case _vs_app_obj: {
            AppObject* ao = (AppObject*)var;
            ao->destroy();
            delete ao;
            break;
        }
        case _vs_script_obj: {
            ScriptObject* so = (ScriptObject*)var;
            so->destroy();
            delete so;
            break;
        }
        case _vs_template: {
            TemplateObject* to = (TemplateObject*)var;
            to->destroy();
            delete to;
            break;
        }
        case _vs_script_obj_handle: {
            // We don't do nuthing, because we have a handle to a _vs_script_obj
            break;
        }
        case _vs_app_obj_handle: {
            AppObjectHandle* sao = (AppObjectHandle*)var;
            sao->destroy();
            delete sao;
            break;
        }
        case _vs_template_handle: {
            break;
        }

        case _vs_unknown: {
            break;
        }

        default: {
            LOGE << "(Leak) No destroy() routine for VarStorage type: " << type << std::endl;
            break;
        }
    }
}

VarStorage::VarStorage() : var(NULL), type(_vs_unknown) {
}

ScriptObjectInstance::ScriptObjectInstance(uintptr_t ptr, VarStorage& vari, bool _from_script_obj_value) : ptr(ptr), var(vari), new_ptr(NULL), reconstructed(false), from_script_obj_value(_from_script_obj_value) {
}

void ScriptObjectInstance::destroy() {
    if (new_ptr) {
        new_ptr->Release();
    }
    var.destroy();
}

void FillVar(VarStorage& new_var, const char* name, int type_id, void* ptr, std::list<ScriptObjectInstance>& handle_var_list, asIScriptModule* module) {
    new_var.name = name;
    new_var.type = _vs_unknown;
    switch (type_id) {
        case asTYPEID_BOOL:
            new_var.type = _vs_bool;
            new_var.var = new bool;
            *(bool*)new_var.var = *(bool*)ptr;
            break;
        case asTYPEID_INT8:
            new_var.type = _vs_int8;
            new_var.var = new char;
            *(char*)new_var.var = *(char*)ptr;
            break;
        case asTYPEID_INT16:
            new_var.type = _vs_int16;
            new_var.var = new short;
            *(short*)new_var.var = *(short*)ptr;
            break;
        case asTYPEID_INT32:
            new_var.type = _vs_int32;
            new_var.var = new int;
            *(int*)new_var.var = *(int*)ptr;
            break;
        case asTYPEID_INT64:
            new_var.type = _vs_int64;
            new_var.var = new int64_t;
            *(int64_t*)new_var.var = *(int64_t*)ptr;
            break;
        case asTYPEID_UINT8:
            new_var.type = _vs_uint8;
            new_var.var = new unsigned char;
            *(unsigned char*)new_var.var = *(unsigned char*)ptr;
            break;
        case asTYPEID_UINT16:
            new_var.type = _vs_uint16;
            new_var.var = new unsigned short;
            *(unsigned short*)new_var.var = *(unsigned short*)ptr;
            break;
        case asTYPEID_UINT32:
            new_var.type = _vs_uint32;
            new_var.var = new unsigned;
            *(unsigned*)new_var.var = *(unsigned*)ptr;
            break;
        case asTYPEID_UINT64:
            new_var.type = _vs_uint64;
            new_var.var = new uint64_t;
            *(uint64_t*)new_var.var = *(uint64_t*)ptr;
            break;
        case asTYPEID_FLOAT:
            new_var.type = _vs_float;
            new_var.var = new float;
            *(float*)new_var.var = *(float*)ptr;
            break;
        default:
            asIScriptEngine* engine = module->GetEngine();
            asITypeInfo* type = engine->GetTypeInfoById(type_id);
            if (type_id & asTYPEID_APPOBJECT) {
                if (type_id & asTYPEID_OBJHANDLE) {
                    new_var.type = _vs_app_obj_handle;
                    const char* type_name = type->GetName();
                    AppObjectHandle* oh = new AppObjectHandle();
                    oh->var = *(void**)ptr;
                    new_var.var = oh;

                    if (strmtch(type_name, "IMFadeIn")) {
                        IMFadeIn* d = *(static_cast<IMFadeIn**>(ptr));
                        oh->type = _ao_IMFadeIn;
                        if (d) {
                            d->AddRef();
                        }
                    } else if (strmtch(type_name, "IMMoveIn")) {
                        IMMoveIn* d = *(static_cast<IMMoveIn**>(ptr));
                        oh->type = _ao_IMMoveIn;
                        if (d) {
                            d->AddRef();
                        }
                    } else if (strmtch(type_name, "IMChangeTextFadeOutIn")) {
                        IMChangeTextFadeOutIn* d = *(static_cast<IMChangeTextFadeOutIn**>(ptr));
                        oh->type = _ao_IMChangeTextFadeOutIn;
                        if (d) {
                            d->AddRef();
                        }
                    } else if (strmtch(type_name, "IMChangeImageFadeOutIn")) {
                        IMChangeImageFadeOutIn* d = *(static_cast<IMChangeImageFadeOutIn**>(ptr));
                        oh->type = _ao_IMChangeImageFadeOutIn;
                        if (d) {
                            d->AddRef();
                        }
                    } else if (strmtch(type_name, "IMPulseAlpha")) {
                        IMPulseAlpha* d = *(static_cast<IMPulseAlpha**>(ptr));
                        oh->type = _ao_IMPulseAlpha;
                        if (d) {
                            d->AddRef();
                        }
                    } else if (strmtch(type_name, "IMPulseBorderAlpha")) {
                        IMPulseBorderAlpha* d = *(static_cast<IMPulseBorderAlpha**>(ptr));
                        oh->type = _ao_IMPulseBorderAlpha;
                        if (d) {
                            d->AddRef();
                        }
                    } else if (strmtch(type_name, "IMPulseBorderAlpha")) {
                        IMPulseBorderAlpha* d = *(static_cast<IMPulseBorderAlpha**>(ptr));
                        oh->type = _ao_IMPulseBorderAlpha;
                        if (d) {
                            d->AddRef();
                        }
                    } else if (strmtch(type_name, "IMMouseOverMove")) {
                        IMMouseOverMove* d = *(static_cast<IMMouseOverMove**>(ptr));
                        oh->type = _ao_IMMouseOverMove;
                        if (d) {
                            d->AddRef();
                        }
                    } else if (strmtch(type_name, "IMMouseOverScale")) {
                        IMMouseOverScale* d = *(static_cast<IMMouseOverScale**>(ptr));
                        oh->type = _ao_IMMouseOverScale;
                        if (d) {
                            d->AddRef();
                        }
                    } else if (strmtch(type_name, "IMMouseOverShowBorder")) {
                        IMMouseOverShowBorder* d = *(static_cast<IMMouseOverShowBorder**>(ptr));
                        oh->type = _ao_IMMouseOverShowBorder;
                        if (d) {
                            d->AddRef();
                        }
                    } else if (strmtch(type_name, "IMMouseOverShowBorder")) {
                        IMMouseOverShowBorder* d = *(static_cast<IMMouseOverShowBorder**>(ptr));
                        oh->type = _ao_IMMouseOverShowBorder;
                        if (d) {
                            d->AddRef();
                        }
                    } else if (strmtch(type_name, "IMMouseOverPulseColor")) {
                        IMMouseOverPulseColor* d = *(static_cast<IMMouseOverPulseColor**>(ptr));
                        oh->type = _ao_IMMouseOverPulseColor;
                        if (d) {
                            d->AddRef();
                        }
                    } else if (strmtch(type_name, "IMMouseOverPulseBorder")) {
                        IMMouseOverPulseBorder* d = *(static_cast<IMMouseOverPulseBorder**>(ptr));
                        oh->type = _ao_IMMouseOverPulseBorder;
                        if (d) {
                            d->AddRef();
                        }
                    } else if (strmtch(type_name, "IMMouseOverPulseBorderAlpha")) {
                        IMMouseOverPulseBorderAlpha* d = *(static_cast<IMMouseOverPulseBorderAlpha**>(ptr));
                        oh->type = _ao_IMMouseOverPulseBorderAlpha;
                        if (d) {
                            d->AddRef();
                        }
                    } else if (strmtch(type_name, "IMMouseOverFadeIn")) {
                        IMMouseOverFadeIn* d = *(static_cast<IMMouseOverFadeIn**>(ptr));
                        oh->type = _ao_IMMouseOverFadeIn;
                        if (d) {
                            d->AddRef();
                        }
                    } else if (strmtch(type_name, "IMFixedMessageOnMouseOver")) {
                        IMFixedMessageOnMouseOver* d = *(static_cast<IMFixedMessageOnMouseOver**>(ptr));
                        oh->type = _ao_IMFixedMessageOnMouseOver;
                        if (d) {
                            d->AddRef();
                        }
                    } else if (strmtch(type_name, "IMFixedMessageOnClick")) {
                        IMFixedMessageOnClick* d = *(static_cast<IMFixedMessageOnClick**>(ptr));
                        oh->type = _ao_IMFixedMessageOnClick;
                        if (d) {
                            d->AddRef();
                        }
                    } else if (strmtch(type_name, "IMMessage")) {
                        IMMessage* d = *(static_cast<IMMessage**>(ptr));
                        oh->type = _ao_IMMessage;
                        if (d) {
                            d->AddRef();
                        }
                    } else if (strmtch(type_name, "IMGUI")) {
                        IMGUI* d = *(static_cast<IMGUI**>(ptr));
                        oh->type = _ao_IMGUI;
                        if (d) {
                            d->AddRef();
                        }
                    } else if (strmtch(type_name, "IMElement")) {
                        IMElement* d = *(static_cast<IMElement**>(ptr));
                        oh->type = _ao_IMElement;
                        if (d) {
                            d->AddRef();
                        }
                    } else if (strmtch(type_name, "IMContainer")) {
                        IMContainer* d = *(static_cast<IMContainer**>(ptr));
                        oh->type = _ao_IMContainer;
                        if (d) {
                            d->AddRef();
                        }
                    } else if (strmtch(type_name, "IMDivider")) {
                        IMDivider* d = *(static_cast<IMDivider**>(ptr));
                        oh->type = _ao_IMDivider;
                        if (d) {
                            d->AddRef();
                        }
                    } else if (strmtch(type_name, "IMImage")) {
                        IMImage* d = *(static_cast<IMImage**>(ptr));
                        oh->type = _ao_IMImage;
                        if (d) {
                            d->AddRef();
                        }
                    } else if (strmtch(type_name, "IMText")) {
                        IMText* d = *(static_cast<IMText**>(ptr));
                        oh->type = _ao_IMText;
                        if (d) {
                            d->AddRef();
                        }
                    } else if (strmtch(type_name, "IMTextSelectionList")) {
                        IMTextSelectionList* d = *(static_cast<IMTextSelectionList**>(ptr));
                        oh->type = _ao_IMTextSelectionList;
                        if (d) {
                            d->AddRef();
                        }
                    } else if (strmtch(type_name, "IMSpacer")) {
                        IMSpacer* d = *(static_cast<IMSpacer**>(ptr));
                        oh->type = _ao_IMSpacer;
                        if (d) {
                            d->AddRef();
                        }
                        // -- Begin scenegraph objects --
                    } else if (strmtch(type_name, "Object")) {
                        oh->type = _ao_game_object;
                    } else if (strmtch(type_name, "AmbientSoundObject")) {
                        oh->type = _ao_ambient_sound_object;
                    } else if (strmtch(type_name, "CameraObject")) {
                        oh->type = _ao_camera_object;
                    } else if (strmtch(type_name, "DecalObject")) {
                        oh->type = _ao_decal_object;
                    } else if (strmtch(type_name, "DynamicLightObject")) {
                        oh->type = _ao_dynamic_light_object;
                    } else if (strmtch(type_name, "EnvObject")) {
                        oh->type = _ao_env_object;
                    } else if (strmtch(type_name, "Group")) {
                        oh->type = _ao_group_object;
                    } else if (strmtch(type_name, "Hotspot")) {
                        oh->type = _ao_hotspot_object;
                    } else if (strmtch(type_name, "ItemObject")) {
                        oh->type = _ao_item_object;
                    } else if (strmtch(type_name, "LightProbeObject")) {
                        oh->type = _ao_light_probe_object;
                    } else if (strmtch(type_name, "LightVolumeObject")) {
                        oh->type = _ao_light_volume_object;
                    } else if (strmtch(type_name, "MovementObject")) {
                        oh->type = _ao_movement_object;
                    } else if (strmtch(type_name, "NavmeshConnectionObject")) {
                        oh->type = _ao_navmesh_connection_object;
                    } else if (strmtch(type_name, "NavmeshHintObject")) {
                        oh->type = _ao_navmesh_hint_object;
                    } else if (strmtch(type_name, "NavmeshRegionObject")) {
                        oh->type = _ao_navmesh_region_object;
                    } else if (strmtch(type_name, "PathPointObject")) {
                        oh->type = _ao_path_point_object;
                    } else if (strmtch(type_name, "PlaceholderObject")) {
                        oh->type = _ao_placeholder_object;
                    } else if (strmtch(type_name, "ReflectionCaptureObject")) {
                        oh->type = _ao_reflection_capture_object;
                    } else if (strmtch(type_name, "RiggedObject")) {
                        oh->type = _ao_rigged_object;
                    } else if (strmtch(type_name, "TerrainObject")) {
                        oh->type = _ao_terrain_object;
                        // -- End scenegraph objects --
                    } else {
                        LOGE << "Unhandled handle for name " << name << std::endl;
                        LOGE << "Typename " << type_name << std::endl;
                        delete oh;
                        new_var.var = NULL;
                        assert(false);
                    }
                } else {  // Assumed a direct value.
                    new_var.type = _vs_app_obj;
                    new_var.var = new AppObject();
                    AppObject& ao = *(AppObject*)new_var.var;
                    ao.type = _ao_ignored;
                    const char* type_name = type->GetName();
                    const char* name_space = type->GetNamespace();
                    if (strcmp(type_name, "vec3") == 0) {
                        ao.type = _ao_vec3;
                        ao.var = new vec3();
                        *(vec3*)ao.var = *(vec3*)ptr;
                    } else if (strcmp(type_name, "ivec2") == 0) {
                        ao.type = _ao_ivec2;
                        ao.var = new ivec2();
                        *(ivec2*)ao.var = *(ivec2*)ptr;
                    } else if (strcmp(type_name, "ivec3") == 0) {
                        ao.type = _ao_ivec3;
                        ao.var = new ivec3();
                        *(ivec3*)ao.var = *(ivec3*)ptr;
                    } else if (strcmp(type_name, "ivec4") == 0) {
                        ao.type = _ao_ivec4;
                        ao.var = new ivec4();
                        *(ivec4*)ao.var = *(ivec4*)ptr;
                    } else if (strcmp(type_name, "string") == 0) {
                        ao.type = _ao_string;
                        ao.var = new std::string();
                        *(std::string*)ao.var = *(std::string*)ptr;
                    } else if (strcmp(type_name, "AttackScriptGetter") == 0) {
                        ao.type = _ao_attack_script_getter;
                        ao.var = new AttackScriptGetter();
                        *(AttackScriptGetter*)ao.var = *(AttackScriptGetter*)ptr;
                    } else if (strcmp(type_name, "BoneTransform") == 0) {
                        ao.type = _ao_bone_transform;
                        ao.var = new BoneTransform();
                        *(BoneTransform*)ao.var = *(BoneTransform*)ptr;
                    } else if (strcmp(type_name, "quaternion") == 0) {
                        ao.type = _ao_quaternion;
                        ao.var = new quaternion();
                        *(quaternion*)ao.var = *(quaternion*)ptr;
                    } else if (strcmp(type_name, "vec2") == 0) {
                        ao.type = _ao_vec2;
                        ao.var = new vec2();
                        *(vec2*)ao.var = *(vec2*)ptr;
                    } else if (strcmp(type_name, "vec4") == 0) {
                        ao.type = _ao_vec4;
                        ao.var = new vec4();
                        *(vec4*)ao.var = *(vec4*)ptr;
                    } else if (strcmp(type_name, "NavPath") == 0) {
                        ao.type = _ao_nav_path;
                        ao.var = new NavPath();
                    } else if (strcmp(type_name, "mat4") == 0) {
                        ao.type = _ao_mat4;
                        ao.var = new mat4();
                        *(mat4*)ao.var = *(mat4*)ptr;
                    } else if (strcmp(type_name, "JSON") == 0) {
                        ao.type = _ao_json;
                        ao.var = new SimpleJSONWrapper();
                        *(SimpleJSONWrapper*)ao.var = *(SimpleJSONWrapper*)ptr;
                    } else if (strcmp(type_name, "FontSetup") == 0) {
                        ao.type = _ao_fontsetup;
                        ao.var = new FontSetup();
                        *(FontSetup*)ao.var = *(FontSetup*)ptr;
                    } else if (strcmp(type_name, "ModLevel") == 0) {
                        ao.type = _ao_modlevel;
                        ao.var = new ModInstance::Level();
                        *(ModInstance::Level*)ao.var = *(ModInstance::Level*)ptr;
                    } else if (strcmp(type_name, "TextureAssetRef") == 0) {
                        ao.type = _ao_textureassetref;
                        ao.var = new TextureAssetRef();
                        *(TextureAssetRef*)ao.var = *(TextureAssetRef*)ptr;
                    } else if (strcmp(type_name, "ModID") == 0) {
                        ao.type = _ao_modid;
                        ao.var = new ModID();
                        *(ModID*)ao.var = *(ModID*)ptr;
                    } else if (strcmp(type_name, "SpawnerItem") == 0) {
                        ao.type = _ao_spawneritem;
                        ao.var = new ModInstance::Item();
                        *(ModInstance::Item*)ao.var = *(ModInstance::Item*)ptr;
                    } else if (strcmp(type_name, "IMFadeIn") == 0) {
                        LOGW << "IMFadeIn can not be stored as a value in global space and reloaded, it will be reset to initial values. Variable name: " << name << std::endl;
                    } else if (strcmp(type_name, "IMMoveIn") == 0) {
                        LOGW << "IMMoveIn can not be stored as a value in global space and reloaded, it will be reset to initial values. Variable name: " << name << std::endl;
                    } else if (strcmp(type_name, "IMChangeTextFadeOutIn") == 0) {
                        LOGW << "IMChangeTextFadeOutIn can not be stored as a value in global space and reloaded, it will be reset to initial values. Variable name: " << name << std::endl;
                    } else if (strcmp(type_name, "IMChangeImageFadeOutIn") == 0) {
                        LOGW << "IMChangeImageFadeOutIn can not be stored as a value in global space and reloaded, it will be reset to initial values. Variable name: " << name << std::endl;
                    } else if (strcmp(type_name, "IMPulseAlpha") == 0) {
                        LOGW << "IMPulseAlpha can not be stored as a value in global space and reloaded, it will be reset to initial values. Variable name: " << name << std::endl;
                    } else if (strcmp(type_name, "IMPulseBorderAlpha") == 0) {
                        LOGW << "IMPulseBorderAlpha can not be stored as a value in global space and reloaded, it will be reset to initial values. Variable name: " << name << std::endl;
                    } else if (strcmp(type_name, "IMMouseOverScale") == 0) {
                        LOGW << "IMMouseOverScale can not be stored as a value in global space and reloaded, it will be reset to initial values. Variable name: " << name << std::endl;
                    } else if (strcmp(type_name, "IMMouseOverMove") == 0) {
                        LOGW << "IMMouseOverMove can not be stored as a value in global space and reloaded, it will be reset to initial values. Variable name: " << name << std::endl;
                    } else if (strcmp(type_name, "IMMouseOverShowBorder") == 0) {
                        LOGW << "IMMouseOverShowBorder can not be stored as a value in global space and reloaded, it will be reset to initial values. Variable name: " << name << std::endl;
                    } else if (strcmp(type_name, "IMMouseOverPulseColor") == 0) {
                        LOGW << "IMMouseOverPulseColor can not be stored as a value in global space and reloaded, it will be reset to initial values. Variable name: " << name << std::endl;
                    } else if (strcmp(type_name, "IMMouseOverPulseBorder") == 0) {
                        LOGW << "IMMouseOverPulseBorder can not be stored as a value in global space and reloaded, it will be reset to initial values. Variable name: " << name << std::endl;
                    } else if (strcmp(type_name, "IMMouseOverPulseBorderAlpha") == 0) {
                        LOGW << "IMMouseOverPulseBorderAlpha can not be stored as a value in global space and reloaded, it will be reset to initial values. Variable name: " << name << std::endl;
                    } else if (strcmp(type_name, "IMMouseOverFadeIn") == 0) {
                        LOGW << "IMMouseOverFadeIn can not be stored as a value in global space and reloaded, it will be reset to initial values. Variable name: " << name << std::endl;
                    } else if (strcmp(type_name, "IMFixedMessageOnMouseOver") == 0) {
                        LOGW << "IMFixedMessageOnMouseOver can not be stored as a value in global space and reloaded, it will be reset to initial values. Variable name: " << name << std::endl;
                    } else if (strcmp(type_name, "IMFixedMessageOnClick") == 0) {
                        LOGW << "IMFixedMessageOnClick can not be stored as a value in global space and reloaded, it will be reset to initial values. Variable name: " << name << std::endl;
                    } else if (strcmp(type_name, "IMMessage") == 0) {
                        LOGW << "IMMessage can not be stored as a value in global space and reloaded, it will be reset to initial values. Variable name: " << name << std::endl;
                    } else if (strcmp(type_name, "IMElement") == 0) {
                        LOGW << "IMElements can not be stored as a value in global space and reloaded, it will be reset to initial values. Variable name: " << name << std::endl;
                    } else if (strcmp(type_name, "IMContainer") == 0) {
                        LOGW << "IMContainer can not be stored by value in global space and reloaded, it will be reset to initial values. Variable name: " << name << std::endl;
                    } else if (strcmp(type_name, "IMDivider") == 0) {
                        LOGW << "IMDivider can not be stored by value in global space and reloaded, it will be reset to initial values. Variable name: " << name << std::endl;
                    } else if (strcmp(type_name, "IMImage") == 0) {
                        LOGW << "IMImage can not be stored by value in global space and reloaded, it will be reset to initial values. Variable name: " << name << std::endl;
                    } else if (strcmp(type_name, "IMText") == 0) {
                        LOGW << "IMText can not be stored by value in global space and reloaded, it will be reset to initial values. Variable name: " << name << std::endl;
                    } else if (strcmp(type_name, "IMSelectionList") == 0) {
                        LOGW << "IMSelectionList can not be stored by value in global space and reloaded, it will be reset to initial values. Variable name: " << name << std::endl;
                    } else if (strcmp(type_name, "IMSpacer") == 0) {
                        LOGW << "IMSpacer can not be stored by value in global space and reloaded, it will be reset to initial values. Variable name: " << name << std::endl;
                    } else if (strcmp(type_name, "IMUIText") == 0) {
                        LOGW << "IMUIText can not be stored by value in global space and reloaded, it will be reset to initial values. Variable name: " << name << std::endl;
                    } else if (strcmp(type_name, "IMUIImage") == 0) {
                        LOGW << "IMUIImage can not be stored by value in global space and reloaded, it will be reset to initial values. Variable name: " << name << std::endl;
                    } else if (strcmp(type_name, "dictionary") == 0) {
                        DisplayError("Warning", "Hotloading for dictionaries are not supported due to their construction complexity.", _ok_cancel, false);
                        /*
                        ao.type = _ao_dictionary;
                        ao.original = ptr;
                        std::list<std::pair<std::string,VarStorage> > *storage
                            = new std::list<std::pair<std::string,VarStorage> >();
                        ao.var = storage;
                        CScriptDictionary* dictionary = (CScriptDictionary*)ptr;
                        CScriptDictionary::CIterator dictit =  dictionary->begin();
                        for( ;dictit != dictionary->end(); dictit++ ) {
                            storage->resize(storage->size()+1);
                            storage->back().first = dictit.GetKey();
                            uintptr_t vadr = reinterpret_cast<uintptr_t>(dictit.GetAddressOfValue());
                            FillVar(storage->back().second,"",dictit.GetTypeId(),reinterpret_cast<void*>(vadr),handle_var_list,module);
                        }
                        */
                    } else {
                        DisplayError("Warning", ("Unknown app object type: " + std::string(type_name)).c_str(), _ok_cancel, false);
                        ao.type = _ao_unknown;
                    }
                }
            } else if (type_id & asTYPEID_SCRIPTOBJECT) {
                if (type_id & asTYPEID_OBJHANDLE) {
                    asIScriptObject* script_object_ptr = *(static_cast<asIScriptObject**>(ptr));
                    if (((uintptr_t)script_object_ptr) != 0) {
                        bool found_prev_constructed = false;
                        std::list<ScriptObjectInstance>::iterator hvs_it = handle_var_list.begin();
                        for (; hvs_it != handle_var_list.end(); hvs_it++) {
                            if (hvs_it->ptr == ((uintptr_t)script_object_ptr)) {
                                found_prev_constructed = true;
                            }
                        }

                        if (found_prev_constructed == false) {
                            VarStorage var_storage;

                            asITypeInfo* actual_type = script_object_ptr->GetObjectType();
                            var_storage.name = "handle_source";
                            var_storage.type = _vs_script_obj;
                            var_storage.var = new ScriptObject();
                            ScriptObject& so = *(ScriptObject*)var_storage.var;
                            so.type_name = actual_type->GetName();
                            so.orig_ptr = (uintptr_t)script_object_ptr;

                            if (actual_type->GetNamespace()) {
                                so.name_space = std::string(actual_type->GetNamespace());
                            }

                            // LOGI << "TYPENAME: " << so.type_name << std::endl;
                            handle_var_list.push_back(
                                ScriptObjectInstance(
                                    (uintptr_t) * (static_cast<asIScriptObject**>(ptr)),
                                    var_storage,
                                    false));
                            asIScriptObject* obj = *(static_cast<asIScriptObject**>(ptr));
                            so.Populate(obj, handle_var_list, module);
                        }
                    }

                    new_var.type = _vs_script_obj_handle;
                    new_var.var = (void*)*(static_cast<asIScriptObject**>(ptr));
                } else {
                    bool found_prev_constructed = false;
                    asIScriptObject* script_object_ptr = static_cast<asIScriptObject*>(ptr);
                    std::list<ScriptObjectInstance>::iterator hvs_it = handle_var_list.begin();
                    for (; hvs_it != handle_var_list.end(); hvs_it++) {
                        if (hvs_it->ptr == ((uintptr_t)script_object_ptr)) {
                            found_prev_constructed = true;
                            hvs_it->from_script_obj_value = true;
                            hvs_it->var.name = name;
                        }
                    }

                    new_var.type = _vs_script_obj;
                    new_var.var = new ScriptObject();
                    ScriptObject& so = *(ScriptObject*)new_var.var;
                    so.type_name = type->GetName();
                    so.orig_ptr = (uintptr_t)ptr;

                    if (type->GetNamespace()) {
                        so.name_space = std::string(type->GetNamespace());
                    }

                    if (found_prev_constructed == false) {
                        VarStorage vs;
                        handle_var_list.push_back(
                            ScriptObjectInstance(
                                (uintptr_t)ptr,
                                vs,
                                true));
                    }

                    so.Populate(script_object_ptr, handle_var_list, module);
                }
            } else if (type_id & asTYPEID_TEMPLATE) {
                if (type_id & asTYPEID_OBJHANDLE) {
                    // TODO: Implement this similar to app_objects
                    LOGW << "We can't recover a reference to a template type from global variable storage, consider refactoring to avoid this, variable name: " << name << " it will be default/invalid." << std::endl;
                    new_var.type = _vs_template_handle;
                    new_var.var = NULL;
                } else {
                    new_var.type = _vs_template;
                    new_var.var = new TemplateObject();
                    TemplateObject& to = *(TemplateObject*)new_var.var;
                    const char* type_name = type->GetName();
                    int sub_type_id = type->GetSubTypeId();
                    asITypeInfo* sub_type = engine->GetTypeInfoById(sub_type_id);
                    if (sub_type) {
                        to.sub_type_name = sub_type->GetName();
                    } else {
                        if (sub_type_id == asTYPEID_BOOL) {
                            to.sub_type_name = "bool";
                        } else if (sub_type_id == asTYPEID_INT8) {
                            to.sub_type_name = "int8";
                        } else if (sub_type_id == asTYPEID_INT16) {
                            to.sub_type_name = "int16";
                        } else if (sub_type_id == asTYPEID_INT32) {
                            to.sub_type_name = "int32";
                        } else if (sub_type_id == asTYPEID_UINT8) {
                            to.sub_type_name = "uint8";
                        } else if (sub_type_id == asTYPEID_UINT16) {
                            to.sub_type_name = "uint16";
                        } else if (sub_type_id == asTYPEID_UINT32) {
                            to.sub_type_name = "uint32";
                        } else if (sub_type_id == asTYPEID_FLOAT) {
                            to.sub_type_name = "float";
                        } else {
                            to.sub_type_name = "unknown";
                        }
                    }
                    if (strcmp(type_name, "array") == 0) {
                        to.type = _to_array;
                        to.var = new std::list<VarStorage>();
                        std::list<VarStorage>& storage = *(std::list<VarStorage>*)to.var;
                        CScriptArray* array = (CScriptArray*)ptr;
                        unsigned size = array->GetSize();
                        for (unsigned i = 0; i < size; ++i) {
                            storage.resize(i + 1);
                            FillVar(storage.back(), "", sub_type_id, array->At(i), handle_var_list, module);
                        }
                    } else {
                        LOGE << "Unknown template type name: " << type_name << std::endl;
                        to.type = _to_unknown;
                    }
                }
            } else {
                if (type) {
                    asDWORD type_flags = type->GetFlags();
                    if (type_flags & asOBJ_ENUM) {
                        new_var.type = _vs_enum;
                        new_var.var = new char[type->GetSize()];
                        new_var.size = type->GetSize();
                        memcpy(new_var.var, ptr, type->GetSize());
                    } else {
                        LOGW << "Is unknown type " << std::endl;
                    }
                } else {
                    LOGE << "Type is null for some reason" << std::endl;
                }
            }
            break;
    }
}

ASModule::~ASModule() {
    std::list<VarStorage>::iterator iter = saved_vars_.begin();
    for (; iter != saved_vars_.end(); ++iter) {
        iter->destroy();
    }

    std::list<ScriptObjectInstance>::iterator iterh = saved_handle_vars_.begin();
    for (; iterh != saved_handle_vars_.end(); ++iterh) {
        iterh->destroy();
    }
}

void ASModule::SetErrorStringDestination(std::string* error_string) {
    active_angelscript_error_string = error_string;
}

void ASModule::PrintGlobalVars() {
    std::list<VarStorage> storage;
    std::list<ScriptObjectInstance> handle_var_storage;
    GetGlobalVars(storage, handle_var_storage);
    std::list<VarStorage>::iterator iter = storage.begin();
    for (; iter != storage.end(); ++iter) {
        VarStorage& vs = (*iter);
        printf("%s\n", vs.GetString().c_str());
    }
}

void ASModule::PrintAllTypes() {
    /*
    asIScriptEngine engine = module_->GetEngine();
    for( int i = 0; i < engine->GetTypedefCount(); i++ ) {
        asITypeInfo *typeinfo = engine->GetTypeInfoByIndex(i);
    }
    */
}

void ASModule::LogGlobalVars() {
    std::list<VarStorage> storage;
    std::list<ScriptObjectInstance> handle_var_storage;
    GetGlobalVars(storage, handle_var_storage);
    std::list<VarStorage>::iterator iter = storage.begin();
    for (; iter != storage.end(); ++iter) {
        VarStorage& vs = (*iter);
        LOGE.Format("%s\n", vs.GetString().c_str());
    }
}

void* ASModule::GetVarPtr(const char* name) {
    int index = module_->GetGlobalVarIndexByName(name);
    return module_->GetAddressOfGlobalVar(index);
}

void* ASModule::GetVarPtrCache(const char* name) {
    std::map<void*, void*>::iterator iter = fast_var_ptr_map.find((void*)name);
    if (iter != fast_var_ptr_map.end()) {
        return iter->second;
    } else {
        int index = module_->GetGlobalVarIndexByName(name);
        void* addr = module_->GetAddressOfGlobalVar(index);
        fast_var_ptr_map[(void*)name] = addr;
        return addr;
    }
}

void ASModule::GetGlobalVars(std::list<VarStorage>& storage, std::list<ScriptObjectInstance>& handle_var_list) {
    int c = module_->GetGlobalVarCount();
    for (int n = 0; n < c; n++) {
        const char* name;
        const char** name_hdl = &name;
        int type_id;
        bool is_const;
        module_->GetGlobalVar(n, name_hdl, 0, &type_id, &is_const);
        if (is_const) {
            continue;
        }
        void* ptr = module_->GetAddressOfGlobalVar(n);
        VarStorage new_var;
        FillVar(new_var, name, type_id, ptr, handle_var_list, module_);
        storage.push_back(new_var);
    }
}

void ASModule::SaveGlobalVars() {
    GetGlobalVars(saved_vars_, saved_handle_vars_);
}

void ASModule::LoadVar(ValueParent var_type, void* parent_ptr, int parent_index, const VarStorage& vs, void* ptr, std::list<ScriptObjectInstance>& handle_var_list, std::list<ScriptObjectInstanceHandleRemap>& app_obj_handle_offset, asIScriptModule* module) {
    if (vs.type == _vs_bool) {
        *(bool*)ptr = *(bool*)vs.var;
    } else if (vs.type == _vs_int8) {
        *(char*)ptr = *(char*)vs.var;
    } else if (vs.type == _vs_int16) {
        *(short*)ptr = *(short*)vs.var;
    } else if (vs.type == _vs_int32) {
        *(int*)ptr = *(int*)vs.var;
    } else if (vs.type == _vs_int64) {
        *(int64_t*)ptr = *(int64_t*)vs.var;
    } else if (vs.type == _vs_uint8) {
        *(unsigned char*)ptr = *(unsigned char*)vs.var;
    } else if (vs.type == _vs_uint16) {
        *(unsigned short*)ptr = *(unsigned short*)vs.var;
    } else if (vs.type == _vs_uint32) {
        *(unsigned int*)ptr = *(unsigned int*)vs.var;
    } else if (vs.type == _vs_uint64) {
        *(uint64_t*)ptr = *(uint64_t*)vs.var;
    } else if (vs.type == _vs_float) {
        *(float*)ptr = *(float*)vs.var;
    } else if (vs.type == _vs_enum) {
        memcpy(ptr, vs.var, vs.size);
    } else if (vs.type == _vs_app_obj) {
        AppObject& ao = *(AppObject*)vs.var;
        if (ao.type == _ao_vec3) {
            *(vec3*)ptr = *(vec3*)ao.var;
        } else if (ao.type == _ao_ivec2) {
            *(ivec2*)ptr = *(ivec2*)ao.var;
        } else if (ao.type == _ao_ivec3) {
            *(ivec3*)ptr = *(ivec3*)ao.var;
        } else if (ao.type == _ao_ivec4) {
            *(ivec4*)ptr = *(ivec4*)ao.var;
        } else if (ao.type == _ao_string) {
            *(std::string*)ptr = *(std::string*)ao.var;
        } else if (ao.type == _ao_attack_script_getter) {
            *(AttackScriptGetter*)ptr = *(AttackScriptGetter*)ao.var;
        } else if (ao.type == _ao_bone_transform) {
            *(BoneTransform*)ptr = *(BoneTransform*)ao.var;
        } else if (ao.type == _ao_quaternion) {
            *(quaternion*)ptr = *(quaternion*)ao.var;
        } else if (ao.type == _ao_vec2) {
            *(vec2*)ptr = *(vec2*)ao.var;
        } else if (ao.type == _ao_vec4) {
            *(vec4*)ptr = *(vec4*)ao.var;
        } else if (ao.type == _ao_nav_path) {
            *(NavPath*)ptr = *(NavPath*)ao.var;
        } else if (ao.type == _ao_mat4) {
            *(mat4*)ptr = *(mat4*)ao.var;
        } else if (ao.type == _ao_json) {
            *(SimpleJSONWrapper*)ptr = *(SimpleJSONWrapper*)ao.var;
        } else if (ao.type == _ao_fontsetup) {
            *(FontSetup*)ptr = *(FontSetup*)ao.var;
        } else if (ao.type == _ao_modlevel) {
            *(ModInstance::Level*)ptr = *(ModInstance::Level*)ao.var;
        } else if (ao.type == _ao_textureassetref) {
            *(TextureAssetRef*)ptr = *(TextureAssetRef*)ao.var;
        } else if (ao.type == _ao_modid) {
            *(ModID*)ptr = *(ModID*)ao.var;
        } else if (ao.type == _ao_spawneritem) {
            *(ModInstance::Item*)ptr = *(ModInstance::Item*)ao.var;
        } else if (ao.type == _ao_dictionary) {
            /*
            std::list<std::pair<int,VarStorage> >* storage = static_cast<std::list<std::pair<int,VarStorage> >* >(ao.var);
            CScriptDictionary* dictionary = (CScriptDictionary*)ptr;

            std::list<std::pair<int,VarStorage> >::iterator storit = storage->begin();
            for( ; storit != storage->end(); storit++ ) {

            }
            */
        } else if (ao.type == _ao_ignored) {
        } else {
            DisplayError("Error", "Unknown ao.type");
        }
    } else if (vs.type == _vs_script_obj) {
        bool found = false;
        ScriptObject& so = *(ScriptObject*)vs.var;
        asIScriptObject* obj = (asIScriptObject*)ptr;

        std::list<ScriptObjectInstance>::iterator hvs_it = handle_var_list.begin();
        for (; hvs_it != handle_var_list.end(); hvs_it++) {
            if (hvs_it->ptr == so.orig_ptr) {
                hvs_it->new_ptr = obj;
                hvs_it->new_ptr->AddRef();
                hvs_it->reconstructed = true;
                found = true;
            }
        }

        if (found == false) {
            LOGE << "Could not find a handle for this script_obj." << std::endl;
        }

        unsigned count = 0;
        std::list<VarStorage>::const_iterator iter2 = so.storage.begin();
        for (; iter2 != so.storage.end(); ++iter2) {
            void* so_ptr = obj->GetAddressOfProperty(count);
            const VarStorage& so_vs = (*iter2);
            LoadVar(_vp_ScriptObject, obj, count, so_vs, so_ptr, handle_var_list, app_obj_handle_offset, module);
            ++count;
        }
    } else if (vs.type == _vs_script_obj_handle) {
        std::list<ScriptObjectInstance>::iterator hvs_it = handle_var_list.begin();
        for (; hvs_it != handle_var_list.end(); hvs_it++) {
            if (hvs_it->ptr == (uintptr_t)vs.var) {
                if (hvs_it->from_script_obj_value == false) {
                    if (hvs_it->reconstructed == false) {
                        hvs_it->reconstructed = true;

                        ScriptObject& so = *(ScriptObject*)hvs_it->var.var;
                        asIScriptEngine* engine = module->GetEngine();

                        if (so.name_space.empty() == false) {
                            module->SetDefaultNamespace(so.name_space.c_str());
                        }

                        asITypeInfo* script_object_type_info = static_cast<asITypeInfo*>(module->GetTypeInfoByName(so.type_name.c_str()));
                        if (script_object_type_info != NULL) {
                            // Create a new instance of asIScriptObject
                            asIScriptObject* obj = static_cast<asIScriptObject*>(engine->CreateUninitializedScriptObject(script_object_type_info));
                            hvs_it->new_ptr = obj;
                            hvs_it->new_ptr->AddRef();
                            if (obj != NULL) {
                                unsigned count = 0;
                                std::list<VarStorage>::const_iterator iter2 = so.storage.begin();
                                for (; iter2 != so.storage.end(); ++iter2) {
                                    void* so_ptr = obj->GetAddressOfProperty(count);
                                    const VarStorage& so_vs = (*iter2);
                                    LoadVar(_vp_ScriptObject, obj, count, so_vs, so_ptr, handle_var_list, app_obj_handle_offset, module);
                                    ++count;
                                }
                            }
                        } else {
                            LOGE << "No matching typename " << so.type_name << " in recompiled module." << std::endl;
                        }

                        module->SetDefaultNamespace("");
                    }
                }

                if (hvs_it->reconstructed) {
                    if (hvs_it->new_ptr) {
                        (*(asIScriptObject**)ptr) = hvs_it->new_ptr;
                        (*(asIScriptObject**)ptr)->AddRef();
                    } else {
                        LOGE << "No pointer created for ScriptObjectType" << std::endl;
                    }
                } else {
                    ScriptObjectInstanceHandleRemap soihr;

                    soihr.var_type = var_type;
                    soihr.parent_ptr = parent_ptr;
                    soihr.parent_index = parent_index;
                    soihr.orig_ptr = (uintptr_t)vs.var;

                    app_obj_handle_offset.push_back(soihr);
                }
            }
        }
    } else if (vs.type == _vs_app_obj_handle) {
        AppObjectHandle* aoh = static_cast<AppObjectHandle*>(vs.var);
        if (aoh->var) {
            *(void**)ptr = aoh->var;
            if (aoh->type == _ao_IMFadeIn) {
                IMFadeIn* v = static_cast<IMFadeIn*>(aoh->var);
                v->AddRef();
            } else if (aoh->type == _ao_IMMoveIn) {
                IMMoveIn* v = static_cast<IMMoveIn*>(aoh->var);
                v->AddRef();
            } else if (aoh->type == _ao_IMChangeTextFadeOutIn) {
                IMChangeTextFadeOutIn* v = static_cast<IMChangeTextFadeOutIn*>(aoh->var);
                v->AddRef();
            } else if (aoh->type == _ao_IMChangeImageFadeOutIn) {
                IMChangeImageFadeOutIn* v = static_cast<IMChangeImageFadeOutIn*>(aoh->var);
                v->AddRef();
            } else if (aoh->type == _ao_IMPulseAlpha) {
                IMPulseAlpha* v = static_cast<IMPulseAlpha*>(aoh->var);
                v->AddRef();
            } else if (aoh->type == _ao_IMPulseBorderAlpha) {
                IMPulseBorderAlpha* v = static_cast<IMPulseBorderAlpha*>(aoh->var);
                v->AddRef();
            } else if (aoh->type == _ao_IMMouseOverMove) {
                IMMouseOverMove* v = static_cast<IMMouseOverMove*>(aoh->var);
                v->AddRef();
            } else if (aoh->type == _ao_IMMouseOverScale) {
                IMMouseOverScale* v = static_cast<IMMouseOverScale*>(aoh->var);
                v->AddRef();
            } else if (aoh->type == _ao_IMMouseOverShowBorder) {
                IMMouseOverShowBorder* v = static_cast<IMMouseOverShowBorder*>(aoh->var);
                v->AddRef();
            } else if (aoh->type == _ao_IMMouseOverPulseColor) {
                IMMouseOverPulseColor* v = static_cast<IMMouseOverPulseColor*>(aoh->var);
                v->AddRef();
            } else if (aoh->type == _ao_IMMouseOverPulseBorder) {
                IMMouseOverPulseBorder* v = static_cast<IMMouseOverPulseBorder*>(aoh->var);
                v->AddRef();
            } else if (aoh->type == _ao_IMMouseOverPulseBorderAlpha) {
                IMMouseOverPulseBorderAlpha* v = static_cast<IMMouseOverPulseBorderAlpha*>(aoh->var);
                v->AddRef();
            } else if (aoh->type == _ao_IMMouseOverFadeIn) {
                IMMouseOverFadeIn* v = static_cast<IMMouseOverFadeIn*>(aoh->var);
                v->AddRef();
            } else if (aoh->type == _ao_IMFixedMessageOnMouseOver) {
                IMFixedMessageOnMouseOver* v = static_cast<IMFixedMessageOnMouseOver*>(aoh->var);
                v->AddRef();
            } else if (aoh->type == _ao_IMFixedMessageOnClick) {
                IMFixedMessageOnClick* v = static_cast<IMFixedMessageOnClick*>(aoh->var);
                v->AddRef();
            } else if (aoh->type == _ao_IMMessage) {
                IMMessage* v = static_cast<IMMessage*>(aoh->var);
                v->AddRef();
            } else if (aoh->type == _ao_IMGUI) {
                IMGUI* v = static_cast<IMGUI*>(aoh->var);
                v->AddRef();
            } else if (aoh->type == _ao_IMElement) {
                IMElement* v = static_cast<IMElement*>(aoh->var);
                v->AddRef();
            } else if (aoh->type == _ao_IMContainer) {
                IMContainer* v = static_cast<IMContainer*>(aoh->var);
                v->AddRef();
            } else if (aoh->type == _ao_IMDivider) {
                IMDivider* v = static_cast<IMDivider*>(aoh->var);
                v->AddRef();
            } else if (aoh->type == _ao_IMImage) {
                IMImage* v = static_cast<IMImage*>(aoh->var);
                v->AddRef();
            } else if (aoh->type == _ao_IMText) {
                IMText* v = static_cast<IMText*>(aoh->var);
                v->AddRef();
            } else if (aoh->type == _ao_IMTextSelectionList) {
                IMTextSelectionList* v = static_cast<IMTextSelectionList*>(aoh->var);
                v->AddRef();
            } else if (aoh->type == _ao_IMSpacer) {
                IMSpacer* v = static_cast<IMSpacer*>(aoh->var);
                v->AddRef();
                // -- Begin scenegraph objects --
            } else if (aoh->type == _ao_game_object) {
                // Nothing to do here
            } else if (aoh->type == _ao_ambient_sound_object) {
                // Nothing to do here
            } else if (aoh->type == _ao_camera_object) {
                // Nothing to do here
            } else if (aoh->type == _ao_decal_object) {
                // Nothing to do here
            } else if (aoh->type == _ao_dynamic_light_object) {
                // Nothing to do here
            } else if (aoh->type == _ao_env_object) {
                // Nothing to do here
            } else if (aoh->type == _ao_group_object) {
                // Nothing to do here
            } else if (aoh->type == _ao_hotspot_object) {
                // Nothing to do here
            } else if (aoh->type == _ao_item_object) {
                // Nothing to do here
            } else if (aoh->type == _ao_light_probe_object) {
                // Nothing to do here
            } else if (aoh->type == _ao_light_volume_object) {
                // Nothing to do here
            } else if (aoh->type == _ao_movement_object) {
                // Nothing to do here
            } else if (aoh->type == _ao_navmesh_connection_object) {
                // Nothing to do here
            } else if (aoh->type == _ao_navmesh_hint_object) {
                // Nothing to do here
            } else if (aoh->type == _ao_navmesh_region_object) {
                // Nothing to do here
            } else if (aoh->type == _ao_path_point_object) {
                // Nothing to do here
            } else if (aoh->type == _ao_placeholder_object) {
                // Nothing to do here
            } else if (aoh->type == _ao_reflection_capture_object) {
                // Nothing to do here
            } else if (aoh->type == _ao_rigged_object) {
                // Nothing to do here
            } else if (aoh->type == _ao_terrain_object) {
                // Nothing to do here
                // -- End scenegraph objects --
            } else {
                LOGE << "Unhandled _ao_ type " << std::endl;
                assert(false);
            }
        } else {
            *(void**)ptr = NULL;
        }
    } else if (vs.type == _vs_template) {
        TemplateObject& to = *(TemplateObject*)vs.var;
        if (to.type == _to_array) {
            const std::list<VarStorage>& storage = *(std::list<VarStorage>*)to.var;
            CScriptArray* array = (CScriptArray*)ptr;
            array->Resize(storage.size());
            unsigned count = 0;
            std::list<VarStorage>::const_iterator iter = storage.begin();
            for (; iter != storage.end(); ++iter) {
                LoadVar(_vp_Array, array, count, *iter, array->At(count), handle_var_list, app_obj_handle_offset, module);
                ++count;
            }
        }
    }
}

void ASModule::LoadGlobalVars() {
    const std::list<VarStorage>& storage = saved_vars_;
    std::list<ScriptObjectInstanceHandleRemap> app_obj_handle_offset;

    std::list<VarStorage>::const_iterator iter = storage.begin();
    for (; iter != storage.end(); ++iter) {
        const VarStorage& vs = (*iter);
        int index = module_->GetGlobalVarIndexByName(vs.name.c_str());
        void* ptr = module_->GetAddressOfGlobalVar(index);
        if (ptr) {
            LoadVar(_vp_Global, module_, index, vs, ptr, saved_handle_vars_, app_obj_handle_offset, module_);
        }
    }

    std::list<ScriptObjectInstanceHandleRemap>::iterator soihr_it;
    std::list<ScriptObjectInstance>::iterator hvs_it;
    for (soihr_it = app_obj_handle_offset.begin();
         soihr_it != app_obj_handle_offset.end();
         soihr_it++) {
        for (hvs_it = saved_handle_vars_.begin();
             hvs_it != saved_handle_vars_.end();
             hvs_it++) {
            if (hvs_it->ptr == soihr_it->orig_ptr) {
                void* ptr = NULL;
                switch (soihr_it->var_type) {
                    case _vp_Global:
                        ptr = static_cast<asIScriptModule*>(soihr_it->parent_ptr)->GetAddressOfGlobalVar(soihr_it->parent_index);
                        break;
                    case _vp_ScriptObject:
                        ptr = static_cast<asIScriptObject*>(soihr_it->parent_ptr)->GetAddressOfProperty(soihr_it->parent_index);
                        break;
                    case _vp_Array:
                        ptr = static_cast<CScriptArray*>(soihr_it->parent_ptr)->At(soihr_it->parent_index);
                        break;
                }

                if (hvs_it->new_ptr && ptr) {
                    (*(asIScriptObject**)ptr) = hvs_it->new_ptr;
                    (*(asIScriptObject**)ptr)->AddRef();
                } else {
                    LOGE << "No pointer created for ScriptObjectType, it should have." << std::endl;
                }
            }
        }
    }
}

void ASModule::Recompile() {
    PROFILER_ZONE(g_profiler_ctx, "Recompile");
    SaveGlobalVars();
    int r = CompileScript(script_path_);
    if (r < 0) {
        FatalError("Error", "Could not compile script: %s", script_path_.GetFullPath());
        return;
    }
    LoadGlobalVars();

    std::list<VarStorage>::iterator iter = saved_vars_.begin();
    for (; iter != saved_vars_.end(); ++iter) {
        iter->destroy();
    }
    saved_vars_.clear();
}

void ASModule::AttachToEngine(asIScriptEngine* engine) {
    module_ = engine->GetModule(0, asGM_ALWAYS_CREATE);
}

const Path& ASModule::GetScriptPath() {
    return script_path_;
}

LineFile ASModule::GetCorrectedLine(int line) {
    return script_file_ptr_->GetCorrectedLine(line);
}

void ASModule::ResetGlobals() {
    module_->ResetGlobalVars();
    fast_var_ptr_map.clear();
}

asIScriptFunction* ASModule::GetFunctionID(const std::string& function_name) {
    std::map<std::string, asIScriptFunction*>::const_iterator iter = func_map_.find(function_name);
    asIScriptFunction* func;
    if (iter == func_map_.end()) {
        func = module_->GetFunctionByDecl(function_name.c_str());
        func_map_.insert(std::pair<std::string, asIScriptFunction*>(function_name, func));
    } else {
        func = iter->second;
    }
    return func;
}

asIScriptModule* ASModule::GetInternalScriptModule() {
    return module_;
}

AppObject::AppObject() {
    var = NULL;
}

void AppObject::destroy() {
    switch (type) {
        case _ao_vec3:
            delete (vec3*)var;
            break;
        case _ao_ivec2:
            delete (ivec2*)var;
            break;
        case _ao_ivec3:
            delete (ivec3*)var;
            break;
        case _ao_ivec4:
            delete (ivec4*)var;
            break;
        case _ao_string:
            delete (std::string*)var;
            break;
        case _ao_attack_script_getter:
            delete (AttackScriptGetter*)var;
            break;
        case _ao_bone_transform:
            delete (BoneTransform*)var;
            break;
        case _ao_quaternion:
            delete (quaternion*)var;
            break;
        case _ao_vec2:
            delete (vec2*)var;
            break;
        case _ao_vec4:
            delete (vec4*)var;
            break;
        case _ao_nav_path:
            delete (NavPath*)var;
            break;
        case _ao_mat4:
            delete (mat4*)var;
            break;
        case _ao_json:
            delete (SimpleJSONWrapper*)var;
            break;
        case _ao_IMUIImage:
            delete (IMUIImage*)var;
            break;
        case _ao_IMUIText:
            delete (IMUIText*)var;
            break;
        case _ao_modlevel:
            delete (ModInstance::Level*)var;
            break;
        case _ao_textureassetref:
            delete (TextureAssetRef*)var;
            break;
        case _ao_modid:
            delete (ModID*)var;
            break;
        case _ao_spawneritem:
            delete (ModInstance::Item*)var;
            break;
        case _ao_unknown:
            break;
        case _ao_ignored:
            break;
        default:
            LOGE << "No destroy for unhandled type: " << AppObjectTypeString(type) << " in AppObject" << std::endl;
            break;
    }
    var = NULL;
}

AppObjectHandle::AppObjectHandle() {
    type = _ao_unknown;
    var = NULL;
}

void AppObjectHandle::destroy() {
    if (var) {
        switch (type) {
            case _ao_IMFadeIn:
                static_cast<IMFadeIn*>(var)->Release();
                break;
            case _ao_IMMoveIn:
                static_cast<IMMoveIn*>(var)->Release();
                break;
            case _ao_IMChangeTextFadeOutIn:
                static_cast<IMChangeTextFadeOutIn*>(var)->Release();
                break;
            case _ao_IMChangeImageFadeOutIn:
                static_cast<IMChangeImageFadeOutIn*>(var)->Release();
                break;
            case _ao_IMPulseAlpha:
                static_cast<IMPulseAlpha*>(var)->Release();
                break;
            case _ao_IMPulseBorderAlpha:
                static_cast<IMPulseBorderAlpha*>(var)->Release();
                break;
            case _ao_IMMouseOverMove:
                static_cast<IMMouseOverMove*>(var)->Release();
                break;
            case _ao_IMMouseOverScale:
                static_cast<IMMouseOverScale*>(var)->Release();
                break;
            case _ao_IMMouseOverShowBorder:
                static_cast<IMMouseOverShowBorder*>(var)->Release();
                break;
            case _ao_IMMouseOverPulseColor:
                static_cast<IMMouseOverPulseColor*>(var)->Release();
                break;
            case _ao_IMMouseOverPulseBorder:
                static_cast<IMMouseOverPulseBorder*>(var)->Release();
                break;
            case _ao_IMMouseOverPulseBorderAlpha:
                static_cast<IMMouseOverPulseBorderAlpha*>(var)->Release();
                break;
            case _ao_IMMouseOverFadeIn:
                static_cast<IMMouseOverFadeIn*>(var)->Release();
                break;
            case _ao_IMFixedMessageOnMouseOver:
                static_cast<IMFixedMessageOnMouseOver*>(var)->Release();
                break;
            case _ao_IMFixedMessageOnClick:
                static_cast<IMFixedMessageOnClick*>(var)->Release();
                break;
            case _ao_IMMessage:
                static_cast<IMMessage*>(var)->Release();
                break;
            case _ao_IMGUI:
                static_cast<IMGUI*>(var)->Release();
                break;
            case _ao_IMElement:
                static_cast<IMElement*>(var)->Release();
                break;
            case _ao_IMContainer:
                static_cast<IMContainer*>(var)->Release();
                break;
            case _ao_IMDivider:
                static_cast<IMDivider*>(var)->Release();
                break;
            case _ao_IMImage:
                static_cast<IMImage*>(var)->Release();
                break;
            case _ao_IMText:
                static_cast<IMText*>(var)->Release();
                break;
            case _ao_IMTextSelectionList:
                static_cast<IMTextSelectionList*>(var)->Release();
                break;
            case _ao_IMSpacer:
                static_cast<IMSpacer*>(var)->Release();
                break;
            // -- Begin scenegraph objects --
            case _ao_game_object:
            case _ao_ambient_sound_object:
            case _ao_camera_object:
            case _ao_decal_object:
            case _ao_dynamic_light_object:
            case _ao_env_object:
            case _ao_group_object:
            case _ao_hotspot_object:
            case _ao_item_object:
            case _ao_light_probe_object:
            case _ao_light_volume_object:
            case _ao_movement_object:
            case _ao_navmesh_connection_object:
            case _ao_navmesh_hint_object:
            case _ao_navmesh_region_object:
            case _ao_path_point_object:
            case _ao_placeholder_object:
            case _ao_reflection_capture_object:
            case _ao_rigged_object:
            case _ao_terrain_object:
                // Nothing to do here
                break;
            // -- End scenegraph objects
            default:
                LOGE << "No destroy for unhandled type: " << AppObjectTypeString(type) << " in AppObject" << std::endl;
                break;
        }
    }
    type = _ao_unknown;
    var = NULL;
}

TemplateObject::TemplateObject() {
    var = NULL;
}

void TemplateObject::destroy() {
    switch (type) {
        case _to_array: {
            std::list<VarStorage>* varlist = (std::list<VarStorage>*)var;
            std::list<VarStorage>::iterator iter = varlist->begin();
            for (; iter != varlist->end(); ++iter) {
                iter->destroy();
            }
            delete varlist;
            break;
        }
        default:
            break;
    }
}

const char* AppObjectTypeString(enum AppObjectType aot) {
    switch (aot) {
        case _ao_vec3:
            return "_ao_vec3";
            break;
        case _ao_vec4:
            return "_ao_vec4";
            break;
        case _ao_ivec2:
            return "_ao_ivec2";
            break;
        case _ao_ivec3:
            return "_ao_ivec3";
            break;
        case _ao_ivec4:
            return "_ao_ivec4";
            break;
        case _ao_string:
            return "_ao_string";
            break;
        // -- Begin scenegraph objects --
        case _ao_ambient_sound_object:
            return "_ao_ambient_sound_object";
            break;
        case _ao_camera_object:
            return "_ao_camera_object";
            break;
        case _ao_decal_object:
            return "_ao_decal_object";
            break;
        case _ao_dynamic_light_object:
            return "_ao_dynamic_light_object";
            break;
        case _ao_env_object:
            return "_ao_env_object";
            break;
        case _ao_game_object:
            return "_ao_game_object";
            break;
        case _ao_group_object:
            return "_ao_group_object";
            break;
        case _ao_hotspot_object:
            return "_ao_hotspot_object";
            break;
        case _ao_item_object:
            return "_ao_item_object";
            break;
        case _ao_light_probe_object:
            return "_ao_light_probe_object";
            break;
        case _ao_light_volume_object:
            return "_ao_light_volume_object";
            break;
        case _ao_movement_object:
            return "_ao_movement_object";
            break;
        case _ao_navmesh_connection_object:
            return "_ao_navmesh_connection_object";
            break;
        case _ao_navmesh_hint_object:
            return "_ao_navmesh_hint_object";
            break;
        case _ao_navmesh_region_object:
            return "_ao_navmesh_region_object";
            break;
        case _ao_path_point_object:
            return "_ao_path_point_object";
            break;
        case _ao_placeholder_object:
            return "_ao_placeholder_object";
            break;
        case _ao_reflection_capture_object:
            return "_ao_reflection_capture_object";
            break;
        case _ao_rigged_object:
            return "_ao_rigged_object";
            break;
        case _ao_terrain_object:
            return "_ao_terrain_object";
            break;
        // -- End scenegraph objects --
        case _ao_attack_script_getter:
            return "_ao_attack_script_getter";
            break;
        case _ao_bone_transform:
            return "_ao_bone_transform";
            break;
        case _ao_quaternion:
            return "_ao_quaternion";
            break;
        case _ao_vec2:
            return "_ao_vec2";
            break;
        case _ao_nav_path:
            return "_ao_nav_path";
            break;
        case _ao_mat4:
            return "_ao_mat4";
            break;
        case _ao_json:
            return "_ao_json";
            break;
        case _ao_fontsetup:
            return "_ao_fontsetup";
            break;
        case _ao_modlevel:
            return "_ao_modlevel";
            break;
        case _ao_textureassetref:
            return "_ao_texetureassetref";
            break;
        case _ao_modid:
            return "_ao_modid";
            break;
        case _ao_spawneritem:
            return "_ao_spawneritem";
            break;
        case _ao_IMFadeIn:
            return "_ao_IMFadeIn";
            break;
        case _ao_IMMoveIn:
            return "_ao_IMMoveIn";
            break;
        case _ao_IMChangeTextFadeOutIn:
            return "_ao_IMChangeTextFadeOutIn";
            break;
        case _ao_IMChangeImageFadeOutIn:
            return "_ao_IMChangeImageFadeOutIn";
            break;
        case _ao_IMPulseAlpha:
            return "_ao_IMPulseAlpha";
            break;
        case _ao_IMPulseBorderAlpha:
            return "_ao_IMPulseBorderAlpha";
            break;
        case _ao_IMMouseOverMove:
            return "_ao_IMMouseOverMove";
            break;
        case _ao_IMMouseOverScale:
            return "_ao_IMMouseOverScale";
            break;
        case _ao_IMMouseOverShowBorder:
            return "_ao_IMMouseOverShowBorder";
            break;
        case _ao_IMMouseOverPulseColor:
            return "_ao_IMMouseOverPulseColor";
            break;
        case _ao_IMMouseOverPulseBorder:
            return "_ao_IMMouseOverPulseBorder";
            break;
        case _ao_IMMouseOverPulseBorderAlpha:
            return "_ao_IMMouseOverPulseBorderAlpha";
            break;
        case _ao_IMMouseOverFadeIn:
            return "_ao_IMMouseOverFadeIn";
            break;
        case _ao_IMFixedMessageOnMouseOver:
            return "_ao_IMFixedMessageOnMouseOver";
            break;
        case _ao_IMFixedMessageOnClick:
            return "_ao_IMFixedMessageOnClick";
            break;
        case _ao_IMMessage:
            return "_ao_IMMessage";
            break;
        case _ao_IMGUI:
            return "_ao_IMGUI";
            break;
        case _ao_IMElement:
            return "_ao_IMElement";
            break;
        case _ao_IMContainer:
            return "_ao_IMContainer";
            break;
        case _ao_IMDivider:
            return "_ao_IMDivider";
            break;
        case _ao_IMImage:
            return "_ao_IMImage";
            break;
        case _ao_IMText:
            return "_ao_IMText";
            break;
        case _ao_IMTextSelectionList:
            return "_ao_IMTextSelectionList";
            break;
        case _ao_IMSpacer:
            return "_ao_IMSpacer";
            break;
        case _ao_IMUIImage:
            return "_ao_IMUIImage";
            break;
        case _ao_IMUIText:
            return "_ao_IMUIText";
            break;
        case _ao_dictionary:
            return "_ao_dictionary";
            break;
        case _ao_unknown:
            return "_ao_unknown";
            break;
        case _ao_ignored:
            return "_ao_ignored";
            break;
        default:
            return "UNKNOWN TYPE";
            break;
    }
}

std::string GetTypeIDString(int type_id) {
    std::stringstream ss;
    if (type_id & asTYPEID_VOID) {
        ss << " asTYPEID_VOID";
    }
    if (type_id & asTYPEID_BOOL) {
        ss << " asTYPEID_BOOL";
    }
    if (type_id & asTYPEID_INT8) {
        ss << " asTYPEID_INT8";
    }
    if (type_id & asTYPEID_INT16) {
        ss << " asTYPEID_INT16";
    }
    if (type_id & asTYPEID_INT32) {
        ss << " asTYPEID_INT32";
    }
    if (type_id & asTYPEID_INT64) {
        ss << " asTYPEID_INT64";
    }
    if (type_id & asTYPEID_UINT8) {
        ss << " asTYPEID_UINT8";
    }
    if (type_id & asTYPEID_UINT16) {
        ss << " asTYPEID_UINT16";
    }
    if (type_id & asTYPEID_UINT32) {
        ss << " asTYPEID_UINT32";
    }
    if (type_id & asTYPEID_UINT64) {
        ss << " asTYPEID_UINT64";
    }
    if (type_id & asTYPEID_FLOAT) {
        ss << " asTYPEID_FLOAT";
    }
    if (type_id & asTYPEID_DOUBLE) {
        ss << " asTYPEID_DOUBLE";
    }
    if (type_id & asTYPEID_OBJHANDLE) {
        ss << " asTYPEID_OBJHANDLE";
    }
    if (type_id & asTYPEID_HANDLETOCONST) {
        ss << " asTYPEID_HANDLETOCONST";
    }
    if (type_id & asTYPEID_MASK_OBJECT) {
        ss << " asTYPEID_MASK_OBJECT";
    }
    if (type_id & asTYPEID_APPOBJECT) {
        ss << " asTYPEID_APPOBJECT";
    }
    if (type_id & asTYPEID_SCRIPTOBJECT) {
        ss << " asTYPEID_SCRIPTOBJECT";
    }
    if (type_id & asTYPEID_TEMPLATE) {
        ss << " asTYPEID_TEMPLATE";
    }
    if (type_id & asTYPEID_MASK_SEQNBR) {
        ss << " asTYPEID_MASK_SEQNBR";
    }
    return ss.str();
}
