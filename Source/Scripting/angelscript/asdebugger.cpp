//-----------------------------------------------------------------------------
//           Name: asdebugger.cpp
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
#include "asdebugger.h"

#include <GUI/dimgui/dimgui.h>
#include <GUI/dimgui/imgui_impl_sdl_gl3.h>

#include <Scripting/angelscript/ascontext.h>
#include <Scripting/angelscript/add_on/scriptarray/scriptarray.h>

#include <Main/engine.h>
#include <Graphics/graphics.h>
#include <UserInput/input.h>

#include <angelscript.h>

#include <set>

std::vector<ASContext*> ASDebugger::active_contexts;
std::vector<std::pair<std::string, int> > ASDebugger::global_breakpoints;

ASDebugger::ASDebugger()
    : paused(false), break_next(false), current_line(-1), current_stack_level(0), ctx(NULL), auto_scroll(false), show_local_variables(false), show_global_variables(false), show_stack_trace(false) {
    for (auto& global_breakpoint : global_breakpoints) {
        ToggleBreakpoint(global_breakpoint.first.c_str(), global_breakpoint.second);
    }
}

ASDebugger::~ASDebugger() {
}

void ASDebugger::AddContext(ASContext* context) {
    bool add = true;
    for (auto& active_context : active_contexts) {
        if (active_context == context) {
            add = false;
            break;
        }
    }

    if (add)
        active_contexts.push_back(context);
}

void ASDebugger::RemoveContext(ASContext* context) {
    for (size_t i = 0; i < active_contexts.size(); ++i) {
        if (active_contexts[i] == context) {
            active_contexts.erase(active_contexts.begin() + i);
            break;
        }
    }
}

const std::vector<ASContext*>& ASDebugger::GetActiveContexts() {
    return active_contexts;
}

void ASDebugger::AddGlobalBreakpoint(const char* file, int line) {
    bool found = false;
    for (auto& global_breakpoint : global_breakpoints) {
        if (strcmp(global_breakpoint.first.c_str(), file) == 0 && global_breakpoint.second == line) {
            found = true;
            break;
        }
    }

    if (!found) {
        global_breakpoints.push_back(std::pair<std::string, int>(file, line));
        for (auto& active_context : active_contexts) {
            active_context->dbg.AddBreakpoint(file, line);
        }
    }
}

void ASDebugger::RemoveGlobalBreakpoint(const char* file, int line) {
    for (size_t i = 0; i < global_breakpoints.size(); ++i) {
        if (strcmp(global_breakpoints[i].first.c_str(), file) == 0 && global_breakpoints[i].second == line) {
            global_breakpoints.erase(global_breakpoints.begin() + i);
            for (auto& active_context : active_contexts) {
                active_context->dbg.RemoveBreakpoint(file, line);
            }
            break;
        }
    }
}

const std::vector<std::pair<std::string, int> >& ASDebugger::GetGlobalBreakpoints() {
    return global_breakpoints;
}

void ASDebugger::SetModule(ASModule* module) {
    this->module = module;
}

void ASDebugger::PrintCType(Variable& variable, asIScriptContext* ctx) {
    void* ptr = variable.ptr;
    if (variable.type_id & asTYPEID_OBJHANDLE) {
        ptr = *(void**)ptr;
        if (!ptr) {
            ImGui::Text("%s points to null", variable.name);
            return;
        }
    }

    if (variable.type_id & asTYPEID_SCRIPTOBJECT) {
        if (ImGui::TreeNodeEx(variable.name, 0)) {
            asIScriptObject* obj = (asIScriptObject*)ptr;
            for (asUINT i = 0; i < obj->GetPropertyCount(); ++i) {
                Variable member;
                member.type_id = obj->GetPropertyTypeId(i);
                member.name = obj->GetPropertyName(i);
                member.ptr = obj->GetAddressOfProperty(i);
                PrintVar(member, ctx);
            }
            ImGui::TreePop();
        }
        return;
    }
    asITypeInfo* info = ctx->GetEngine()->GetTypeInfoById(variable.type_id);
    if (strcmp(info->GetName(), "string") == 0) {
        std::string* string = (std::string*)ptr;
        if (string) {
            ImGui::Text("%s = \"%s\"", variable.name, string->c_str());
        } else {
            ImGui::Text("%s = ?", variable.name);
        }
    } else if (strcmp(info->GetName(), "array") == 0) {
        if (ImGui::TreeNodeEx(variable.name, 0)) {
            CScriptArray* arr = (CScriptArray*)ptr;
            int new_type_id = info->GetSubTypeId(0);
            for (size_t i = 0; i < arr->GetSize(); ++i) {
                char buffer[16];
                sprintf(buffer, "[%d]", (int)i);
                Variable variable;
                variable.name = buffer;
                variable.type_id = new_type_id;
                variable.ptr = arr->At(i);
                PrintVar(variable, ctx);
            }
            ImGui::TreePop();
        }
    } else {
        if (info->GetPropertyCount() > 0) {
            if (ImGui::TreeNodeEx(variable.name, 0)) {
                for (size_t i = 0; i < info->GetPropertyCount(); ++i) {
                    int property_offset;
                    Variable property;
                    info->GetProperty(i, &property.name, &property.type_id, NULL, NULL, &property_offset);
                    property.ptr = (char*)ptr + property_offset;
                    PrintVar(property, ctx);
                }

                ImGui::TreePop();
            }
        } else {
            ImGui::Text("%s is of unknown type %s and no members are registered", variable.name, info->GetName());
        }
    }
}

void ASDebugger::PrintVar(Variable& variable, asIScriptContext* ctx) {
    switch (variable.type_id) {
        case asTYPEID_BOOL:
            ImGui::Text("%s = %d\n", variable.name, *(bool*)variable.ptr);
            break;
        case asTYPEID_INT8:
            ImGui::Text("%s = %d\n", variable.name, (int)*(int8_t*)variable.ptr);
            break;
        case asTYPEID_INT16:
            ImGui::Text("%s = %d\n", variable.name, (int)*(int16_t*)variable.ptr);
            break;
        case asTYPEID_INT32:
            ImGui::Text("%s = %ld\n", variable.name, (long)(*(int32_t*)variable.ptr));  // These casts are done to avoid warnings on 32/64 bit builds
            break;
        case asTYPEID_INT64:
            ImGui::Text("%s = %lld\n", variable.name, (long long)(*(int64_t*)variable.ptr));
            break;
        case asTYPEID_UINT8:
            ImGui::Text("%s = %u\n", variable.name, (unsigned int)*(uint8_t*)variable.ptr);
            break;
        case asTYPEID_UINT16:
            ImGui::Text("%s = %u\n", variable.name, (unsigned int)*(uint16_t*)variable.ptr);
            break;
        case asTYPEID_UINT32:
            ImGui::Text("%s = %lu\n", variable.name, (unsigned long)(*(uint32_t*)variable.ptr));
            break;
        case asTYPEID_UINT64:
            ImGui::Text("%s = %llu\n", variable.name, (unsigned long long)(*(uint64_t*)variable.ptr));
            break;
        case asTYPEID_FLOAT:
        case asTYPEID_DOUBLE:
            ImGui::Text("%s = %f\n", variable.name, *(float*)variable.ptr);
            break;
        default: {
            if (variable.ptr == NULL)
                return;
            PrintCType(variable, ctx);
            return;
        }
    }
}

void ASDebugger::Print() {
    std::set<void*> printed_variables;
    asUINT callstack_size = ctx->GetCallstackSize();
    for (asUINT stack_level = 0; stack_level < callstack_size; ++stack_level) {
        int variable_count = ctx->GetVarCount(stack_level);

        for (int var_index = 0; var_index < variable_count; ++var_index) {
            Variable variable;
            variable.name = ctx->GetVarName(var_index, stack_level);
            variable.type_id = ctx->GetVarTypeId(var_index, stack_level);
            variable.ptr = ctx->GetAddressOfVar(var_index, stack_level);

            if (printed_variables.find(variable.ptr) != printed_variables.end())
                continue;
            printed_variables.insert(variable.ptr);
            local_variables.push_back(variable);
        }
    }

    asIScriptFunction* function = ctx->GetFunction();
    if (!function)
        return;
    asIScriptModule* script_mod = function->GetModule();
    if (!script_mod)
        return;

    for (asUINT variable_index = 0; variable_index < script_mod->GetGlobalVarCount(); ++variable_index) {
        Variable variable;
        script_mod->GetGlobalVar(variable_index, &variable.name, NULL, &variable.type_id);
        variable.ptr = script_mod->GetAddressOfGlobalVar(variable_index);
        if (printed_variables.find(variable.ptr) != printed_variables.end())
            continue;
        printed_variables.insert(variable.ptr);
        global_variables.push_back(variable);
    }

    asIScriptEngine* engine = ctx->GetEngine();
    for (asUINT variable_index = 0; variable_index < engine->GetGlobalPropertyCount(); ++variable_index) {
        Variable variable;
        engine->GetGlobalPropertyByIndex(variable_index, &variable.name, NULL, &variable.type_id, NULL, NULL, &variable.ptr);
        if (printed_variables.find(variable.ptr) != printed_variables.end())
            continue;
        printed_variables.insert(variable.ptr);
        global_variables.push_back(variable);
    }
}

void ASDebugger::Break() {
    break_next = true;
    auto_scroll = true;
}

void ASDebugger::ToggleBreakpoint(const char* file_name, int line) {
    std::string file(file_name);

    Breakpoint_iter breakpoint_iter = breakpoints.find(file);
    if (breakpoint_iter == breakpoints.end())
        breakpoints.insert(std::pair<std::string, std::vector<int> >(file, std::vector<int>(1, line)));
    else {
        std::vector<int>& lines = breakpoint_iter->second;

        for (size_t i = 0; i < lines.size(); ++i) {
            if (lines[i] == line) {
                lines.erase(lines.begin() + i);
                return;
            }
        }

        breakpoint_iter->second.push_back(line);
    }
}

void ASDebugger::AddBreakpoint(const char* file_name, int line) {
    std::string file(file_name);

    Breakpoint_iter breakpoint_iter = breakpoints.find(file);
    if (breakpoint_iter == breakpoints.end())
        breakpoints.insert(std::pair<std::string, std::vector<int> >(file, std::vector<int>(1, line)));
    else {
        std::vector<int>& lines = breakpoint_iter->second;

        for (int i : lines) {
            if (i == line) {
                return;
            }
        }

        breakpoint_iter->second.push_back(line);
    }
}

void ASDebugger::RemoveBreakpoint(const char* file_name, int line) {
    std::string file(file_name);

    Breakpoint_iter breakpoint_iter = breakpoints.find(file);
    if (breakpoint_iter != breakpoints.end()) {
        std::vector<int>& lines = breakpoint_iter->second;

        for (size_t i = 0; i < lines.size(); ++i) {
            if (lines[i] == line) {
                lines.erase(lines.begin() + i);
                return;
            }
        }
    }
}

const std::vector<int>* ASDebugger::GetBreakpoints(const char* file_name) {
    std::string file(file_name);

    Breakpoint_iter breakpoint_iter = breakpoints.find(file);
    if (breakpoint_iter != breakpoints.end())
        return &(breakpoint_iter->second);
    else
        return NULL;
}

void ASDebugger::LineCallback(asIScriptContext* ctx) {
    if (paused) {
        // After suspending, the line callback is called once again
        local_variables.clear();
        global_variables.clear();
        Print();
        DebugLoop();
        return;
    }
    this->ctx = ctx;

    LineFile lf = GetLF(ctx);

    current_file = lf.file;
    current_line = lf.line_number;

    if (break_next) {
        break_next = false;
        paused = true;
        ctx->Suspend();
        return;
    }

    std::string file_path = lf.file.GetOriginalPathStr();
    file_path = file_path.substr(file_path.find_last_of("\\/") + 1);

    const std::vector<int>* breakpoints = GetBreakpoints(file_path.c_str());
    if (breakpoints == NULL)
        return;
    else {
        for (int breakpoint : *breakpoints) {
            if (breakpoint == current_line) {
                paused = true;
                auto_scroll = true;
                ctx->Suspend();
            }
        }
    }
}

void ASDebugger::StepOver(asIScriptContext* ctx) {
    LineFile lf = GetLF(ctx);

    int stack_level = ctx->GetCallstackSize();

    if (stack_level <= current_stack_level) {
        current_file = lf.file;
        current_line = lf.line_number;
        ctx->Suspend();
        paused = true;
        auto_scroll = true;
    } else {
        Continue(ctx);
    }
}

void ASDebugger::StepInto(asIScriptContext* ctx) {
    LineFile lf = GetLF(ctx);

    if ((int)lf.line_number != current_line) {
        current_file = lf.file;
        current_line = lf.line_number;
        ctx->Suspend();
        paused = true;
        auto_scroll = true;
    }
}

void ASDebugger::StepOut(asIScriptContext* ctx) {
    int stack_level = ctx->GetCallstackSize();

    if (stack_level < current_stack_level) {
        LineFile lf = GetLF(ctx);
        current_file = lf.file;
        current_line = lf.line_number;
        ctx->Suspend();
        paused = true;
        auto_scroll = true;
    }
}

void ASDebugger::Continue(asIScriptContext* ctx) {
    LineFile lf = GetLF(ctx);
    if ((int)lf.line_number != current_line)
        continue_break = true;

    if (continue_break) {
        std::string file_path = lf.file.GetOriginalPathStr();
        file_path = file_path.substr(file_path.find_last_of("\\/") + 1);

        const std::vector<int>* breakpoints = GetBreakpoints(file_path.c_str());
        if (breakpoints == NULL)
            return;
        else {
            for (int breakpoint : *breakpoints) {
                if (breakpoint == (int)lf.line_number) {
                    current_file = lf.file;
                    current_line = lf.line_number;
                    ctx->Suspend();
                    paused = true;
                    auto_scroll = true;
                }
            }
        }
    }
}

void ASDebugger::DebugLoop() {
    Graphics* graphics = Graphics::Instance();
    Input* input = Input::Instance();

    input->cursor->SetVisible(true);
    input->SetGrabMouse(false);
    while (paused) {
        ctx->ClearLineCallback();
        PollEvents();

        bool imgui_begun = true;  // ImGui::FrameBegun();
        if (imgui_begun) {
            ImGui::EndFrame();
        }

        ImGui_ImplSdlGL3_NewFrame(graphics->sdl_window_, input->GetGrabMouse());
        bool update_variables = true;
        switch (UpdateGUI()) {
            case CONTINUE:
                continue_break = false;
                ctx->SetLineCallback(asMETHOD(ASDebugger, Continue), this, asCALL_THISCALL);
                paused = false;
                do {
                    int ret = ctx->Execute();
                    if (ret == asEXECUTION_FINISHED) {
                        ctx->SetLineCallback(asMETHOD(ASDebugger, LineCallback), this, asCALL_THISCALL);
                        ImGui::Render();
                        if (imgui_begun) {
                            ImGui_ImplSdlGL3_NewFrame(graphics->sdl_window_, input->GetGrabMouse());
                        }
                        return;
                    }
                } while (!paused);
                break;
            case OVER:
                current_stack_level = ctx->GetCallstackSize();
                ctx->SetLineCallback(asMETHOD(ASDebugger, StepOver), this, asCALL_THISCALL);
                paused = false;
                do {
                    int ret = ctx->Execute();
                    if (ret == asEXECUTION_FINISHED) {
                        ctx->SetLineCallback(asMETHOD(ASDebugger, LineCallback), this, asCALL_THISCALL);
                        ImGui::Render();
                        if (imgui_begun) {
                            ImGui_ImplSdlGL3_NewFrame(graphics->sdl_window_, input->GetGrabMouse());
                        }
                        return;
                    }
                } while (!paused);
                break;
            case INTO:
                ctx->SetLineCallback(asMETHOD(ASDebugger, StepInto), this, asCALL_THISCALL);
                paused = false;
                do {
                    int ret = ctx->Execute();
                    if (ret == asEXECUTION_FINISHED) {
                        ctx->SetLineCallback(asMETHOD(ASDebugger, LineCallback), this, asCALL_THISCALL);
                        ImGui::Render();
                        if (imgui_begun) {
                            ImGui_ImplSdlGL3_NewFrame(graphics->sdl_window_, input->GetGrabMouse());
                        }
                        return;
                    }
                } while (!paused);
                break;
            case LEAVE:
                current_stack_level = ctx->GetCallstackSize();
                ctx->SetLineCallback(asMETHOD(ASDebugger, StepOut), this, asCALL_THISCALL);
                paused = false;
                do {
                    int ret = ctx->Execute();
                    if (ret == asEXECUTION_FINISHED) {
                        ctx->SetLineCallback(asMETHOD(ASDebugger, LineCallback), this, asCALL_THISCALL);
                        ImGui::Render();
                        if (imgui_begun) {
                            ImGui_ImplSdlGL3_NewFrame(graphics->sdl_window_, input->GetGrabMouse());
                        }
                        return;
                    }
                } while (!paused);
                break;
            default:
                update_variables = false;
                break;
        }
        if (update_variables) {
            local_variables.clear();
            global_variables.clear();
            Print();
        }

        // Back up state
        std::stack<ViewportDims> viewport_dims_stack;
        std::stack<GLuint> fbo_stack;

        graphics->PushViewport();     // Viewport is set to entire window
        graphics->PushFramebuffer();  // Framebuffer is set to 0
        // Back these up to temporarily so no assertions fail/states change
        viewport_dims_stack = graphics->viewport_dims_stack;
        fbo_stack = graphics->fbo_stack;

        graphics->viewport_dims_stack = std::stack<ViewportDims>();
        graphics->fbo_stack = std::stack<GLuint>();

        // Changes states
        graphics->setViewport(0, 0, graphics->window_dims[0], graphics->window_dims[1]);
        graphics->bindFramebuffer(0);
        graphics->Clear(true);

        // Draw
        ImGui::Render();
        input->cursor->Draw();

        // Swap
        graphics->SwapToScreen();

        // Restore state
        graphics->viewport_dims_stack = viewport_dims_stack;
        graphics->fbo_stack = fbo_stack;
        graphics->PopFramebuffer();
        graphics->PopViewport();

        static uint32_t last_time = 0;
        uint32_t diff = 16 - (SDL_TS_GetTicks() - last_time) - 1;
        if (diff > 15) {
            diff = 15;
        }
        SDL_Delay(diff);
        last_time = SDL_TS_GetTicks();
    }

    ctx->SetLineCallback(asMETHOD(ASDebugger, LineCallback), this, asCALL_THISCALL);
}

void ASDebugger::PollEvents() {
    Graphics* graphics = Graphics::Instance();
    Input* input = Input::Instance();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ProcessEventImGui(&event);
        // Redirect input events to the controller
        switch (event.type) {
            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_FOCUS_LOST:
                        input->HandleEvent(event);
                        break;
                    case SDL_WINDOWEVENT_FOCUS_GAINED:
                        input->HandleEvent(event);
                        break;
                }
                break;
            case SDL_QUIT:
                input->RequestQuit();
                return;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEWHEEL:
                if (!WantMouseImGui()) {
                    input->HandleEvent(event);
                }
                break;
            case SDL_KEYDOWN:
            case SDL_TEXTINPUT:
                if (!WantKeyboardImGui()) {
                    input->HandleEvent(event);
                }
                break;
            default:
                input->HandleEvent(event);
        }
    }
}

void ASDebugger::VariablesMenu(const char* window_name, ImGuiTextFilter& filter, std::vector<Variable>& variables, bool& show_variables) {
    if (show_variables) {
        ImGui::SetNextWindowSize(ImVec2(640.0f, 480.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin(window_name, &show_variables);
        if (ImGui::InputText("Filters:", filter.InputBuf, IM_ARRAYSIZE(filter.InputBuf))) {
            filter.Build();
        }

        if (filter.IsActive()) {
            for (size_t i = 0; i < variables.size(); ++i) {
                if (filter.PassFilter(variables[i].name)) {
                    ImGui::PushID(i);
                    PrintVar(variables[i], ctx);
                    ImGui::PopID();
                }
            }
        } else {
            for (size_t i = 0; i < variables.size(); ++i) {
                ImGui::PushID(i);
                PrintVar(variables[i], ctx);
                ImGui::PopID();
            }
        }
        ImGui::End();
    }
}

static char as_debugger_buffer[512];
static ImGuiTextFilter local_filter;
static ImGuiTextFilter global_filter;
ASDebugger::BreakAction ASDebugger::UpdateGUI() {
    BreakAction action = NONE;

    ImGui::SetNextWindowSize(ImVec2(1024.0f, 768.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Angelscript debugger", &show_performance);

    ImGui::Text("Currently executing %s", current_file.resolved);

    if (ImGui::Button("Continue")) {
        action = CONTINUE;
    }
    ImGui::SameLine();
    if (ImGui::Button("Step over")) {
        action = OVER;
    }
    ImGui::SameLine();
    if (ImGui::Button("Step into")) {
        action = INTO;
    }
    ImGui::SameLine();
    if (ImGui::Button("Step out")) {
        action = LEAVE;
    }

    ImGui::Checkbox("Show local variables", &show_local_variables);
    ImGui::SameLine();
    ImGui::Checkbox("Show global variables", &show_global_variables);
    ImGui::SameLine();
    ImGui::Checkbox("Show stack trace", &show_stack_trace);

    const ScriptFile* script = ScriptFileUtil::GetScriptFile(current_file);
    const char* file_name = script->file_path.GetFullPath();
    int last_slash = 0;
    for (size_t i = 0; i < std::strlen(file_name); ++i) {
        if (file_name[i] == '\\' || file_name[i] == '/')
            last_slash = i;
    }
    file_name += last_slash + 1;

    const std::string& script_source = script->unexpanded_contents;
    int start_index = 0;
    int end_index = 0;

    ImGui::BeginChild("ASSourceMain", ImVec2(0, 0), false, 0);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::Columns(2);

    int line_nr = 1;

    const int MAX_LINE_LENGTH = 255;
    char line[MAX_LINE_LENGTH + 1];

    const std::vector<int>* breakpoints = GetBreakpoints(file_name);

    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    while (end_index != (int)script_source.size()) {
        end_index = std::min(script_source.find('\n', start_index), script_source.size());

        int copy_length = std::min(MAX_LINE_LENGTH, end_index - start_index);
        std::memcpy(line, script_source.c_str() + start_index, copy_length);
        line[copy_length] = '\0';
        start_index = end_index + 1;

        bool is_breakpoint = false;
        if (breakpoints != NULL) {
            for (int breakpoint : *breakpoints) {
                if (breakpoint == line_nr) {
                    is_breakpoint = true;
                    break;
                }
            }
        }

        bool is_current_line = false;
        if (line_nr == GetCurrentLine()) {
            is_current_line = true;
        }

        if (is_current_line) {
            if (is_breakpoint)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.0f, 0.5f));
            else
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.8f, 0.0f, 0.5f));

            if (auto_scroll) {
                ImGui::SetScrollHereY();
                auto_scroll = false;
            }
        } else if (is_breakpoint)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.0f, 0.0f, 0.5f));

        ImGui::Text("%d", line_nr);
        ImGui::NextColumn();
        ImGui::PushID(line_nr);
        ImGui::SetColumnOffset(-1, 36);
        if (ImGui::ButtonEx(line, ImVec2(ImGui::GetWindowWidth() - 36, 0), ImGuiButtonFlags_AlignTextBaseLine)) {
            ToggleBreakpoint(file_name, line_nr);
        }
        ImGui::NextColumn();
        ImGui::PopID();
        if (is_current_line || is_breakpoint)
            ImGui::PopStyleColor();

        ++line_nr;
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
    ImGui::EndChild();
    ImGui::End();

    if (show_stack_trace) {
        const char* file;
        for (asUINT i = 0; i < ctx->GetCallstackSize(); ++i) {
            int line = ctx->GetLineNumber(i, 0, &file);
            LineFile lineFile = module->GetCorrectedLine(line);
            ImGui::Text("%s:%d %s", lineFile.file.GetOriginalPath(), lineFile.line_number, ctx->GetFunction(i)->GetDeclaration());
        }
    }

    VariablesMenu("Angelscript local variables", local_filter, local_variables, show_local_variables);
    VariablesMenu("Angelscript global variables", global_filter, global_variables, show_global_variables);

    return action;
}

LineFile ASDebugger::GetLF(asIScriptContext* ctx) {
    const char* scriptSection;
    int line = ctx->GetLineNumber(0, 0, &scriptSection);
    return module->GetCorrectedLine(line);
}
