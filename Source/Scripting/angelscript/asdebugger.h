//-----------------------------------------------------------------------------
//           Name: asdebugger.h
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

#include <Scripting/angelscript/add_on/debugger/debugger.h>
#include <Scripting/angelscript/asmodule.h>

#include <imgui.h>
#include <imgui_internal.h>

class ASContext;

class ASDebugger : public CDebugger
{
private:
	// LEAVE was named OUT, but on windows OUT is #defined
    enum BreakAction { CONTINUE, OVER, INTO, LEAVE, NONE };

    struct Variable
    {
        const char* name;
        int type_id;
        void* ptr;
    };
public:
    ASDebugger();
    ~ASDebugger();

    void SetModule(ASModule* module);

    void Print();

    void Break();

    void LineCallback(asIScriptContext* ctx);

    void ToggleBreakpoint(const char* file_name, int line);
    void AddBreakpoint(const char* file_name, int line);
    void RemoveBreakpoint(const char* file_name, int line);
    const std::vector<int>* GetBreakpoints(const char* file_name);

    int GetCurrentLine() const { return current_line; }
    const Path& GetCurrentFile() const { return current_file; }

    static void AddContext(ASContext* context);
    static void RemoveContext(ASContext* context);
    static const std::vector<ASContext*>& GetActiveContexts();

    static void AddGlobalBreakpoint(const char* file, int line);
    static void RemoveGlobalBreakpoint(const char* file, int line);
    static const std::vector<std::pair<std::string, int> >& GetGlobalBreakpoints();
private:
    typedef std::map<std::string, std::vector<int> >::iterator Breakpoint_iter;
    std::map<std::string, std::vector<int> > breakpoints;

    ASModule* module;

    bool auto_scroll;
    bool break_next;
    bool paused;
    bool continue_break;
    bool show_local_variables;
    bool show_global_variables;
    bool show_stack_trace;
    BreakAction break_action;

    Path current_file;
    int current_line;
    int current_stack_level;

    asIScriptContext* ctx;
    std::vector<Variable> local_variables;
    std::vector<Variable> global_variables;

    static std::vector<ASContext*> active_contexts;
    static std::vector<std::pair<std::string, int> > global_breakpoints;

    LineFile GetLF(asIScriptContext* ctx);

    void DebugLoop();
    void PollEvents();
    BreakAction UpdateGUI();

    void StepOver(asIScriptContext* ctx);
    void StepOut(asIScriptContext* ctx);
    void StepInto(asIScriptContext* ctx);
    void Continue(asIScriptContext* ctx);

    void PrintCType(Variable& variable, asIScriptContext* ctx);
    void PrintVar(Variable& variable, asIScriptContext* ctx);
    void VariablesMenu(const char* window_name, ImGuiTextFilter& filter, std::vector<Variable>& variables, bool& show_variables);
};
