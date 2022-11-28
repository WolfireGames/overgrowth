//-----------------------------------------------------------------------------
//           Name: ascontext.cpp
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
#include "ascontext.h"

#include <Scripting/angelscript/asfuncs.h>
#include <Scripting/angelscript/scriptstdstring_extend.h>
#include <Scripting/angelscript/add_on/scriptstdstring/scriptstdstring.h>
#include <Scripting/angelscript/add_on/scriptarray/scriptarray.h>
#include <Scripting/angelscript/add_on/scriptdictionary/scriptdictionary.h>
#include <Scripting/angelscript/add_on/scripthelper/scripthelper.h>
#include <Scripting/angelscript/ascrashdump.h>
#include <Scripting/scriptfile.h>
#include <Scripting/scriptlogging.h>

#include <Internal/common.h>
#include <Internal/timer.h>
#include <Internal/profiler.h>

#include <Compat/fileio.h>
#include <Utility/assert.h>
#include <Logging/logdata.h>

#include <angelscript.h>

#include <stack>
#include <sstream>
#include <cassert>

extern Timer game_timer;
extern Timer ui_timer;
bool asdebugger_enabled = false;
bool asprofiler_enabled = false;

#ifdef _DEBUG
const bool kDebugLineCallback = false;
#else
const bool kDebugLineCallback = false;
#endif

int ASContext::CompileScript(const Path &path) {
    return module.CompileScript(path);
}

int ASContext::CompileScriptFromText(const std::string &text) {
    return module.CompileScriptFromText(text);
}

ASContext::ASContext(const char *name, const ASData &as_data) : scenegraph(as_data.scenegraph), gui(as_data.gui), activate_keyboard_events(false), context_name(name) {
    engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
    engine->SetEngineProperty(asEP_ALLOW_MULTILINE_STRINGS, true);
    engine->SetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES, true);
    engine->SetEngineProperty(asEP_OPTIMIZE_BYTECODE, true);

    // Configure the script engine with all the functions,
    // and variables that the script should be able to use.
    RegisterStdString(engine);
    RegisterScriptArray(engine, true);

    // The script compiler will write any compiler messages to the callback.
    engine->SetMessageCallback(asFUNCTION(MessageCallback), &angelscript_error_string, asCALL_CDECL);

    RegisterObjectMethod("array<T>", "void push_back(const T&in)", asMETHOD(CScriptArray, InsertLast), asCALL_THISCALL);
    RegisterObjectMethod("array<T>", "uint size() const", asMETHOD(CScriptArray, GetSize), asCALL_THISCALL);

    RegisterScriptDictionary(engine);
    RegisterStdStringUtils(engine);

    AttachStopwatch(this);
    if (asprofiler_enabled)
        AttachProfiler(this);
    else
        AttachTelemetry(this);
    AttachMathFuncs(this);
    Attach3DMathFuncs(this);
    RegisterStdString_Extend(this);
    AttachTimer(this);
    AttachSound(this);
    AttachParticles(this);
    AttachSky(this);
    AttachDebugDraw(this);
    AttachDecals(this);
    AttachError(this);
    AttachLog(this);
    AttachInfo(this);
    AttachJSON(this);
    AttachConfig(this);
    AttachStringUtil(this);
    AttachIO(this);
    AttachLevelDetails(this);
    AttachModding(this);
    AttachStorage(this);
    AttachDebug(this);

    if (engine->RegisterGlobalFunction("void Print(string &in)", asFUNCTION(PrintString), asCALL_CDECL) < 0) {
        DisplayError("Error", "Registration of Print function failed");
    }

    RegisterObjectType("ASContext", 0, asOBJ_REF | asOBJ_NOCOUNT);
    RegisterObjectMethod("ASContext", "void PrintGlobalVars()", asMETHOD(ASContext, PrintGlobalVars), asCALL_THISCALL);
    DocsCloseBrace();
    RegisterGlobalProperty("ASContext context", this);

    module.AttachToEngine(engine);
    module.SetErrorStringDestination(&angelscript_error_string);

    // Create a context that will execute the script.
    ctx = engine->CreateContext();
    if (ctx == 0) {
        FatalError("Error", "Failed to create the context.");
        return;
    }
    ctx->SetUserData(this, 0);

    if (asdebugger_enabled) {
        dbg.SetEngine(engine);
        dbg.SetModule(&module);
        ctx->SetLineCallback(asMETHOD(ASDebugger, LineCallback), &dbg, asCALL_THISCALL);
    }

    RegisterAngelscriptContext(name, this);
}

std::stack<ASContext *> active_context_stack;

void ASContext::run(asIScriptFunction *func, const ASArglist *args_ptr, ASArg *return_val) {
    LOGS << "Pushing a new callstack " << this << std::endl;
    active_context_stack.push(this);
    ctx->PushState();

    int r = ctx->Prepare(func);
    if (r < 0) {
        FatalError("Error", "Failed to prepare the context for function: \"%s\"", func->GetName());
    }

    // Set args
    if (args_ptr) {
        const ASArglist &args = *args_ptr;
        for (unsigned i = 0; i < args.size(); i++) {
            const ASArgType &type = args[i].type;
            switch (type) {
                case _as_char:
                case _as_bool:
                    ctx->SetArgByte(i, *((asBYTE *)args[i].data));
                    break;
                case _as_int:
                    ctx->SetArgDWord(i, *((asDWORD *)args[i].data));
                    break;
                case _as_unsigned:
                    ctx->SetArgDWord(i, *((asDWORD *)args[i].data));
                    break;
                case _as_float:
                    ctx->SetArgFloat(i, *((float *)args[i].data));
                    break;
                case _as_double:
                    ctx->SetArgDouble(i, *((double *)args[i].data));
                    break;
                case _as_address:
                    ctx->SetArgAddress(i, args[i].data);
                    break;
                case _as_object:
                    ctx->SetArgObject(i, args[i].data);
                    break;
                case _as_string:
                    ctx->SetArgObject(i, args[i].data);
                    // LOGE << "Unhandled _as_string case" << std::endl;
                    break;
                case _as_json:
                    LOGE << "Unhandled _as_json case" << std::endl;
                    break;
            }
        }
    }

    r = ctx->Execute();
    if (r != asEXECUTION_FINISHED) {
        // The execution didn't finish as we had planned. Determine why.
        if (r == asEXECUTION_ABORTED) {
            LOGE << "The script was aborted before it could finish. Probably it timed out." << std::endl;
        } else if (r == asEXECUTION_EXCEPTION) {
            LOGE << "The script ended with an exception." << std::endl;

            // Write some information about the script exception
            asIScriptFunction *func = ctx->GetExceptionFunction();
            LOGE << "context: " << context_name << std::endl;
            LOGE << "func: " << func->GetDeclaration() << std::endl;
            LOGE << "modl: " << func->GetModuleName() << std::endl;
            LOGE << "sect: " << func->GetScriptSectionName() << std::endl;
            LOGE << "About to get ctx line number" << std::endl;
            int line_num = ctx->GetExceptionLineNumber();
            LOGE << "uncorrected line: " << line_num << std::endl;
            LineFile corrected_line = module.GetCorrectedLine(line_num);
            LOGE << "line: " << corrected_line.line_number << std::endl;
            LOGE << "desc: " << ctx->GetExceptionString() << std::endl;
        } else if (r == asCONTEXT_NOT_PREPARED) {
            LOGE << "Context is not prepared" << std::endl;
        } else if (r != asEXECUTION_SUSPENDED)
            LOGE << "The script ended for some unforeseen reason (" << r << ")." << std::endl;
    } else {
        if (return_val) {
            switch (return_val->type) {
                case _as_char:
                case _as_bool:
                    (*(asBYTE *)return_val->data) = ctx->GetReturnByte();
                    break;
                case _as_int:
                    (*(asDWORD *)return_val->data) = ctx->GetReturnDWord();
                    break;
                case _as_unsigned:
                    (*(asDWORD *)return_val->data) = ctx->GetReturnDWord();
                    break;
                case _as_float:
                    (*(float *)return_val->data) = ctx->GetReturnFloat();
                    break;
                case _as_double:
                    (*(double *)return_val->data) = ctx->GetReturnDouble();
                    break;
                case _as_address:
                    return_val->data = ctx->GetReturnAddress();
                    break;
                case _as_object:
                    return_val->data = ctx->GetReturnObject();
                    break;
                case _as_string:
                    return_val->strData = *(std::string *)ctx->GetReturnObject();
                    break;
                case _as_json:
                    return_val->jsonData = *(SimpleJSONWrapper *)ctx->GetReturnObject();
                    break;
                default:
                    DisplayError("Error", "Angelscript function has unknown return type");
                    break;
            }
        }
    }
    ctx->PopState();
    LOGS << "Popping a callstack " << this << std::endl;
    active_context_stack.pop();
}

ASContext::~ASContext() {
    if (asdebugger_enabled)
        ASDebugger::RemoveContext(this);
    if (asprofiler_enabled)
        ASProfiler::RemoveContext(this);
    DeregisterAngelscriptContext(this);
    angelscript_error_string.clear();

    // We must release the contexts when no longer using them
    ctx->Release();
    // Release the engine
    engine->ShutDownAndRelease();
}

bool ASContext::LoadScript(const Path &path) {
    // Compile the script code
    int r = CompileScript(path);
    if (r < 0) {
        DisplayFormatError(_ok, true, "Error", "Could not compile script: %s", path.GetFullPath());
        return false;
    }

    if (LoadExpectedFunctions() == false) {
        std::stringstream ss;
        for (unsigned i = 0; i < expected_functions.size(); i++) {
            if (expected_functions[i].mandatory && expected_functions[i].func_ptr == NULL) {
                ss << expected_functions[i].definition << std::endl;
            }
        }
        DisplayFormatError(_ok_cancel, true, "Error", "Could not initialize script %s, missing expected functions:\n%s", path.GetFullPath(), ss.str().c_str());
        return false;
    }

    if (kDebugLineCallback) {
        ctx->SetLineCallback(asFUNCTION(DebugLineCallback), &module, asCALL_CDECL);
    }

    current_script = path;
    if (asdebugger_enabled)
        ASDebugger::AddContext(this);
    if (asprofiler_enabled) {
        ASProfiler::AddContext(this);
        profiler.SetContext(this);
    }

    return true;
}

void ASContext::LoadScriptFromText(const std::string &text) {
    // Compile the script code
    int r = CompileScriptFromText(text);
    if (r < 0) {
        FatalError("Error", "Could not compile script: hard-coded text");
        return;
    }

    if (LoadExpectedFunctions() == false) {
        std::stringstream ss;
        for (unsigned i = 0; i < expected_functions.size(); i++) {
            if (expected_functions[i].mandatory && expected_functions[i].func_ptr == NULL) {
                ss << expected_functions[i].definition << std::endl;
            }
        }
        DisplayFormatError(_ok_cancel, true, "Error", "Could not initialize script, missing expected functions:\n%s", ss.str().c_str());
        return;
    }

    if (kDebugLineCallback) {
        ctx->SetLineCallback(asFUNCTION(DebugLineCallback), &module, asCALL_CDECL);
    }
}

bool ASContext::Reload() {
    PROFILER_ZONE(g_profiler_ctx, "Live update check");
    if (module.SourceChanged()) {
        module.Recompile();

        if (LoadExpectedFunctions() == false) {
            std::stringstream ss;
            for (unsigned i = 0; i < expected_functions.size(); i++) {
                if (expected_functions[i].mandatory && expected_functions[i].func_ptr == NULL) {
                    ss << expected_functions[i].definition << std::endl;
                }
            }
            DisplayFormatError(_ok_cancel, true, "Error", "Could not fully reload script, missing expected functions:\n%s", ss.str().c_str());
            return false;
        }
        return true;
    } else {
        return false;
    }
}

bool ASContext::LoadExpectedFunctions() {
    bool ok = true;
    for (unsigned i = 0; i < expected_functions.size(); i++) {
        asIScriptFunction *func = module.GetFunctionID(expected_functions[i].definition);
        expected_functions[i].func_ptr = NULL;
        expected_functions[i].unloaded = false;
        if (func) {
            expected_functions[i].func_ptr = func;
        } else if (expected_functions[i].mandatory) {
            LOGF << "Could not load mandatory function " << expected_functions[i].definition << std::endl;
            ok = false;
        }
    }
    return ok;
}

bool ASContext::HasFunction(ASFunctionHandle handle) {
    if (handle < 0) {
        return false;
    }

    return expected_functions[handle].func_ptr;
}

bool ASContext::CallScriptFunction(ASFunctionHandle handle, const ASArglist *args, ASArg *return_val) {
    if (handle >= 0) {
        asIScriptFunction *func = expected_functions[handle].func_ptr;
        const std::string &definition = expected_functions[handle].definition;
        if (func) {
            profiler.CallScriptFunction(definition.c_str());
            run(func, args, return_val);
            profiler.LeaveScriptFunction();
            return true;
        } else {
            if (expected_functions[handle].unloaded) {
                LOGE << "Expected function " << expected_functions[handle].definition << " was not pre-loaded." << std::endl;
            }
            return false;
        }
    } else {
        LOGF << "Invalid handle" << std::endl;
        return false;
    }
}

bool ASContext::HasFunction(const std::string &function_definition) {
    asIScriptFunction *func = module.GetFunctionID(function_definition);
    return func != NULL;
}

bool ASContext::CallScriptFunction(const std::string &function_name, const ASArglist *args, ASArg *return_val, bool fail_message) {
    // Find the function id for the function we want to execute.
    asIScriptFunction *func = module.GetFunctionID(function_name);
    if (!func) {
        if (fail_message) {
            ErrorResponse response =
                DisplayError("Error", ("Function: " + function_name + " not found in file: \"" + module.GetScriptPath().GetFullPath() + "\".").c_str(), _ok_cancel_retry);

            if (response == _retry) {
                if (module.SourceChanged()) {
                    while (Reload() == false) {
                    }
                    return CallScriptFunction(function_name, args, return_val, fail_message);
                }
            }
        }
        return false;
    }

    run(func, args, return_val);
    return true;
}

void ASContext::Execute(const std::string &code, bool newContext) {
    if (newContext) {
        ExecuteString(engine, code.c_str(), module.GetInternalScriptModule(), (asIScriptContext *)0);
    } else {
        ExecuteString(engine, code.c_str(), module.GetInternalScriptModule(), ctx);
    }
}

void ASContext::RegisterGlobalFunction(const char *declaration, const asSFuncPtr &funcPointer, asDWORD callconv, const char *comment) {
    angelscript_error_string.clear();

    int r = engine->RegisterGlobalFunction(declaration, funcPointer, callconv);
    if (r < 0) {
        FatalError("Error", "Error registering function: \"%s\"\n%s", declaration, angelscript_error_string.c_str());
    }
    const int BUF_SIZE = 512;
    char doc_buf[BUF_SIZE];
    if (comment) {
        FormatString(doc_buf, BUF_SIZE, "%s; // %s\n", declaration, comment);
    } else {
        FormatString(doc_buf, BUF_SIZE, "%s;\n", declaration);
    }
    documentation += doc_buf;
}

void ASContext::RegisterGlobalFunctionThis(const char *declaration, const asSFuncPtr &funcPointer, asDWORD callconv, void *ptr, const char *comment) {
    angelscript_error_string.clear();

    int r = engine->RegisterGlobalFunction(declaration, funcPointer, callconv, ptr);
    if (r < 0) {
        FatalError("Error", "Error registering function: \"%s\"\n%s", declaration, angelscript_error_string.c_str());
    }
    const int BUF_SIZE = 512;
    char doc_buf[BUF_SIZE];
    if (comment) {
        FormatString(doc_buf, BUF_SIZE, "%s; // %s\n", declaration, comment);
    } else {
        FormatString(doc_buf, BUF_SIZE, "%s;\n", declaration);
    }
    documentation += doc_buf;
}

void ASContext::RegisterGlobalProperty(const char *declaration, void *pointer, const char *comment) {
    int r = engine->RegisterGlobalProperty(declaration, pointer);
    if (r < 0) {
        FatalError("Error", "Error registering property: \"%s\"", declaration);
    }
    const int BUF_SIZE = 512;
    char doc_buf[BUF_SIZE];
    if (comment) {
        FormatString(doc_buf, BUF_SIZE, "%s; // %s\n", declaration, comment);
    } else {
        FormatString(doc_buf, BUF_SIZE, "%s;\n", declaration);
    }
    documentation += doc_buf;
}

void ASContext::RegisterObjectType(const char *obj, int byteSize, asDWORD flags, const char *comment) {
    int r = engine->RegisterObjectType(obj, byteSize, flags);
    if (r < 0) {
        const char *error;
        switch (r) {
            case asINVALID_ARG:
                error = "asINVALID_ARG";
                break;
            case asINVALID_NAME:
                error = "asINVALID_NAME";
                break;
            case asALREADY_REGISTERED:
                error = "asALREADY_REGISTERED";
                break;
            case asNAME_TAKEN:
                error = "asNAME_TAKEN";
                break;
            case asLOWER_ARRAY_DIMENSION_NOT_REGISTERED:
                error = "asLOWER_ARRAY_DIMENSION_NOT_REGISTERED";
                break;
            case asINVALID_TYPE:
                error = "asINVALID_TYPE";
                break;
            case asNOT_SUPPORTED:
                error = "asNOT_SUPPORTED";
                break;
        }
        FatalError("Error", "Error registering type \"%s\" %s", obj, error);
    }
    const int BUF_SIZE = 512;
    char doc_buf[BUF_SIZE];
    if (comment) {
        FormatString(doc_buf, BUF_SIZE, "class %s { // %s\n", obj, comment);
    } else {
        FormatString(doc_buf, BUF_SIZE, "class %s {\n", obj);
    }
    documentation += doc_buf;
}

void ASContext::ExportDocs(const char *path) {
    FILE *file = my_fopen(path, "w");
    if (file) {
        fprintf(file, "//Mandatory functions in script\n");
        for (unsigned i = 0; i < expected_functions.size(); i++) {
            if (expected_functions[i].mandatory) {
                fprintf(file, "%s\n", expected_functions[i].definition.c_str());
            }
        }
        fprintf(file, "\n//Optional functions in script\n");
        for (unsigned i = 0; i < expected_functions.size(); i++) {
            if (expected_functions[i].mandatory == false) {
                fprintf(file, "%s\n", expected_functions[i].definition.c_str());
            }
        }
        fprintf(file, "\n//Interface\n");
        fwrite(documentation.c_str(), sizeof(char), documentation.length(), file);
        fclose(file);
    }
}

void ASContext::RegisterFuncdef(const char *declaration, const char *comment) {
    int r = engine->RegisterFuncdef(declaration);
    if (r < 0) {
        const char *error;
        switch (r) {
            case asWRONG_CONFIG_GROUP:
                error = "asWRONG_CONFIG_GROUP";
                break;
            case asNOT_SUPPORTED:
                error = "asNOT_SUPPORTED";
                break;
            case asINVALID_TYPE:
                error = "asINVALID_TYPE";
                break;
            case asINVALID_DECLARATION:
                error = "asINVALID_DECLARATION";
                break;
            case asNAME_TAKEN:
                error = "asNAME_TAKEN";
                break;
            case asWRONG_CALLING_CONV:
                error = "asWRONG_CALLING_CONV";
                break;
        }
        FatalError("Error", "Error registering funcdef of \"%s\" %s", declaration, error);
    }
    const int BUF_SIZE = 512;
    char doc_buf[BUF_SIZE];
    if (comment) {
        FormatString(doc_buf, BUF_SIZE, "    %s; // %s\n", declaration, comment);
    } else {
        FormatString(doc_buf, BUF_SIZE, "    %s;\n", declaration);
    }
    documentation += doc_buf;
}

void ASContext::RegisterObjectMethod(const char *obj, const char *declaration, const asSFuncPtr &funcPointer, asDWORD callConv, const char *comment) {
    int r = engine->RegisterObjectMethod(obj, declaration, funcPointer, callConv);
    if (r < 0) {
        const char *error;
        switch (r) {
            case asWRONG_CONFIG_GROUP:
                error = "asWRONG_CONFIG_GROUP";
                break;
            case asNOT_SUPPORTED:
                error = "asNOT_SUPPORTED";
                break;
            case asINVALID_TYPE:
                error = "asINVALID_TYPE";
                break;
            case asINVALID_DECLARATION:
                error = "asINVALID_DECLARATION";
                break;
            case asNAME_TAKEN:
                error = "asNAME_TAKEN";
                break;
            case asWRONG_CALLING_CONV:
                error = "asWRONG_CALLING_CONV";
                break;
        }
        FatalError("Error", "Error registering method of \"%s\" \"%s\" %s", obj, declaration, error);
    }
    const int BUF_SIZE = 512;
    char doc_buf[BUF_SIZE];
    if (comment) {
        FormatString(doc_buf, BUF_SIZE, "    %s; // %s\n", declaration, comment);
    } else {
        FormatString(doc_buf, BUF_SIZE, "    %s;\n", declaration);
    }
    documentation += doc_buf;
}

void ASContext::RegisterObjectProperty(const char *obj, const char *declaration, int byteOffset, const char *comment) {
    int r = engine->RegisterObjectProperty(obj, declaration, byteOffset);
    if (r < 0) {
        const char *error;
        switch (r) {
            case asWRONG_CONFIG_GROUP:
                error = "asWRONG_CONFIG_GROUP";
                break;
            case asINVALID_OBJECT:
                error = "asINVALID_OBJECT";
                break;
            case asINVALID_TYPE:
                error = "asINVALID_TYPE";
                break;
            case asINVALID_DECLARATION:
                error = "asINVALID_DECLARATION";
                break;
            case asNAME_TAKEN:
                error = "asNAME_TAKEN";
                break;
        }
        FatalError("Error", "Error registering property of \"%s\" \"%s\" %s", obj, declaration, error);
    }
    const int BUF_SIZE = 512;
    char doc_buf[BUF_SIZE];
    FormatString(doc_buf, BUF_SIZE, "    %s;\n", declaration);
    documentation += doc_buf;
}

void ASContext::RegisterObjectBehaviour(const char *obj, asEBehaviours behaviour, const char *declaration, const asSFuncPtr &funcPointer, asDWORD callConv, const char *comment) {
    int r = engine->RegisterObjectBehaviour(obj, behaviour, declaration, funcPointer, callConv);
    if (r < 0) {
        FatalError("Error", "Error registering object behaviour: \"%s\"", obj);
    }
    const int BUF_SIZE = 512;
    char doc_buf[BUF_SIZE];
    if (comment) {
        FormatString(doc_buf, BUF_SIZE, "    %s; // %s\n", declaration, comment);
    } else {
        FormatString(doc_buf, BUF_SIZE, "    %s;\n", declaration);
    }
    documentation += doc_buf;
}

ASFunctionHandle ASContext::RegisterExpectedFunction(const std::string &function_decl, bool mandatory) {
    // First check to see if we already have this function registered.
    for (unsigned i = 0; i < expected_functions.size(); i++) {
        if (expected_functions[i].definition == function_decl) {
            if (expected_functions[i].mandatory == false) {
                expected_functions[i].mandatory = mandatory;
            }
            return i;
        }
    }

    ASExpectedFunction f;

    f.definition = function_decl;
    f.mandatory = mandatory;
    f.func_ptr = NULL;
    f.unloaded = true;

    int32_t id = expected_functions.push_back(f);
    if (id >= 0) {
        return id;
    } else {
        LOGF << "Not enough slots for external functions" << std::endl;
        return -1;
    }
}

void ASContext::ResetGlobals() {
    module.ResetGlobals();
}

void ASContext::PrintGlobalVars() {
    module.PrintGlobalVars();
}

void ASContext::SaveGlobalVars() {
    module.SaveGlobalVars();
}

void ASContext::LoadGlobalVars() {
    module.LoadGlobalVars();
}

void *ASContext::GetVarPtr(const char *name) {
    return module.GetVarPtr(name);
}

void *ASContext::GetArrayVarPtr(const std::string &name, int index) {
    CScriptArray *array = (CScriptArray *)GetVarPtr(name.c_str());
    return array->At(index);
}

std::string ASContext::GetCallstack() {
    std::ostringstream oss;
    // Show the call stack
    for (asUINT n = 0; n < ctx->GetCallstackSize(); n++) {
        asIScriptFunction *func;
        const char *scriptSection;
        int line, column;
        func = ctx->GetFunction(n);
        line = ctx->GetLineNumber(n, &column, &scriptSection);
        LineFile lf = module.GetCorrectedLine(line);
        oss << lf.file << " line " << lf.line_number << " \"" << func->GetDeclaration() << "\"\n";
    }
    return oss.str();
}

void ASContext::DumpCallstack(std::ostream &out) {
    module.LogGlobalVars();
    asUINT sz = ctx->GetCallstackSize();
    for (asUINT n = 0; n < sz; n++) {
        asIScriptFunction *func;
        const char *scriptSection;
        int line, column;
        func = ctx->GetFunction(n);
        line = ctx->GetLineNumber(n, &column, &scriptSection);
        LineFile lf = module.GetCorrectedLine(line);

        out << lf.file << " line " << lf.line_number << " \"" << func->GetDeclaration() << "\"\n";
    }
}

void ASContext::LogCallstack() {
}

void ASContext::RegisterEnum(const char *declaration) {
    int r = engine->RegisterEnum(declaration);
    if (r < 0) {
        FatalError("Error", "Error registering enum: \"%s\"\n%s", declaration, angelscript_error_string.c_str());
    }
    const int BUF_SIZE = 512;
    char doc_buf[BUF_SIZE];
    FormatString(doc_buf, BUF_SIZE, "enum %s {\n", declaration);
    documentation += doc_buf;
}

void ASContext::RegisterEnumValue(const char *enum_declaration, const char *enum_val_string, int enum_val) {
    int r = engine->RegisterEnumValue(enum_declaration, enum_val_string, enum_val);
    if (r < 0) {
        FatalError("Error", "Error registering enum: \"%s::%s\"\n%s",
                   enum_declaration, enum_val_string, angelscript_error_string.c_str());
    }
    const int BUF_SIZE = 512;
    char doc_buf[BUF_SIZE];
    FormatString(doc_buf, BUF_SIZE, "    %s = %d,\n", enum_val_string, enum_val);
    documentation += doc_buf;
}

bool ASContext::TypeExists(const std::string &decl) {
    return engine->GetTypeIdByDecl(decl.c_str()) >= 0;
}

void ASContext::DocsCloseBrace() {
    documentation += "};\n";
}

void ASContext::ActivateKeyboardEvents() {
    activate_keyboard_events = true;
}

void ASContext::DeactivateKeyboardEvents() {
    activate_keyboard_events = false;
}

bool ASContext::IsKeyboardEventActivated() {
    return activate_keyboard_events;
}

const int DEBUG_LINE_INFO_SIZE = 512;
char debug_line_info[DEBUG_LINE_INFO_SIZE];

void DebugLineCallback(asIScriptContext *ctx, ASModule *asmod) {
    const char *scriptSection;
    int line = ctx->GetLineNumber(0, 0, &scriptSection);
    LineFile lf = asmod->GetCorrectedLine(line);
    FormatString(debug_line_info, DEBUG_LINE_INFO_SIZE, "Executing line %d in %s\n", lf.line_number, lf.file.GetFullPath());
}

std::pair<Path, int> ASContext::GetCallFile() {
    const char *scriptSection;
    int line, column;
    line = ctx->GetLineNumber(0, &column, &scriptSection);
    LineFile lf = module.GetCorrectedLine(line);

    return std::pair<Path, int>(lf.file, lf.line_number);
}

std::string ASContext::GetASFunctionNameFromMPState(uint32_t state) {
    if (HasFunction(mpCallBacks[state])) {
        ASFunctionHandle handle = mpCallBacks[state];

        return expected_functions[handle].definition;
    }

    return "";
}

void ASContext::RegisterMPStateCallback(uint32_t state, const std::string &callbackFunction) {
    if (mpCallBacks.find(state) != mpCallBacks.end()) {
        return;
    }

    ASFunctionHandle handle = RegisterExpectedFunction(callbackFunction, false);

    asIScriptFunction *func = module.GetFunctionID(expected_functions[handle].definition);

    if (func != nullptr) {
        mpCallBacks[state] = handle;
        LOGI << " STATE: " << state << " is now connected to " << callbackFunction << " via expected function handle: " << handle << std::endl;
        LoadExpectedFunctions();
    } else {
        LOGE << "FAILED TO BIND YOUR STATE: " << state << " TO " << callbackFunction << ". CHECK DEFINITION OF FUNCTION AND SCOPE OF CONTEXT" << std::endl;
    }
}

bool ASContext::CallMPCallBack(uint32_t state, const std::vector<char> &data) {
    if (mpCallBacks.find(state) != mpCallBacks.end()) {
        if (HasFunction(mpCallBacks[state])) {
            ASArglist args;
            asITypeInfo *arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<uint8>"));
            CScriptArray *array = CScriptArray::Create(arrayType, (asUINT)0);
            array->Reserve(data.size());
            for (const char &i : data) {
                array->InsertLast(const_cast<char *>(&i));
            }

            args.AddAddress((void *)array);

            CallScriptFunction(mpCallBacks[state], &args);

            array->Release();

            return true;
        } else {
            LOGE << this << " Failed to call function, state does not have a binded function. Handle ID: " << mpCallBacks[state] << std::endl;
            return false;
        }

        return false;
    }

    return false;
}

bool ASContext::CallMPCallBack(uint32_t state, const std::vector<unsigned int> &data) {
    if (mpCallBacks.find(state) != mpCallBacks.end()) {
        if (HasFunction(mpCallBacks[state])) {
            ASArglist args;
            asITypeInfo *arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<uint>"));
            CScriptArray *array = CScriptArray::Create(arrayType, (asUINT)0);
            array->Reserve(data.size());
            for (const unsigned int &i : data) {
                array->InsertLast((void *)&i);
            }

            args.AddAddress((void *)array);

            LOGD << "Calling function: " << GetASFunctionNameFromMPState(state) << std::endl;

            CallScriptFunction(mpCallBacks[state], &args);

            array->Release();

            return true;
        } else {
            LOGE << this << " Failed to call function, state does not have a binded function. Handle ID: " << mpCallBacks[state] << std::endl;
            return false;
        }
        LOGI << "MPCALLBACKS are aware of:" << state << " but not ASCONTEXT" << std::endl;
        return false;
    }
    LOGI << "MPCALLBACKS are NOT aware of:" << state << std::endl;
    return false;
}

asIScriptEngine *ASContext::GetEngine() {
    return engine;
}
