//-----------------------------------------------------------------------------
//           Name: asfuncs.cpp
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
#include "asfuncs.h"

#include <Objects/hotspot.h>
#include <Objects/decalobject.h>
#include <Objects/dynamiclightobject.h>
#include <Objects/placeholderobject.h>
#include <Objects/movementobject.h>
#include <Objects/itemobject.h>
#include <Objects/group.h>
#include <Objects/prefab.h>
#include <Objects/envobject.h>
#include <Objects/pathpointobject.h>

#include <Scripting/angelscript/asfuncs.h>
#include <Scripting/angelscript/ascontext.h>
#include <Scripting/angelscript/add_on/scriptarray/scriptarray.h>

#include <UserInput/keyTranslator.h>
#include <UserInput/input.h>

#include <Graphics/textures.h>
#include <Graphics/sky.h>
#include <Graphics/particles.h>
#include <Graphics/bonetransform.h>
#include <Graphics/flares.h>

#include <Math/enginemath.h>
#include <Math/mat3.h>
#include <Math/mat4.h>
#include <Math/vec2math.h>
#include <Math/vec3math.h>
#include <Math/ivec2math.h>
#include <Math/ivec3.h>
#include <Math/ivec4.h>

#include <Internal/common.h>
#include <Internal/file_descriptor.h>
#include <Internal/stopwatch.h>
#include <Internal/dialogues.h>
#include <Internal/config.h>
#include <Internal/filesystem.h>
#include <Internal/profiler.h>
#include <Internal/modloading.h>
#include <Internal/message.h>
#include <Internal/varstring.h>
#include <Internal/locale.h>

#include <Online/online.h>
#include <Online/online_utility.h>
#include <Online/online_datastructures.h>

#include <Scripting/scriptlogging.h>
#include <Scripting/angelscript/add_on/scriptarray/scriptarray.h>

#include <Main/scenegraph.h>
#include <Main/engine.h>

#include <GUI/dimgui/settings_screen.h>
#include <GUI/IMUI/im_image.h>
#include <GUI/gui.h>

#include <Utility/strings.h>
#include <Utility/assert.h>

#include <Editors/map_editor.h>
#include <Editors/actors_editor.h>

#include <Compat/compat.h>
#include <AI/navmesh.h>
#include <Version/version.h>
#include <Memory/allocation.h>
#include <Steam/steamworks.h>
#include <Asset/Asset/levelinfo.h>

#include <angelscript.h>
#include <imgui.h>

#include <cmath>
#include <iostream>
#include <sstream>
#include <stack>

#ifdef max
#undef max
#endif

SceneGraph* the_scenegraph = NULL;
extern std::stack<ASContext*> active_context_stack;
extern Timer game_timer;
extern Timer ui_timer;
extern Config default_config;

static ASContext* GetActiveASContext() {
    asIScriptContext* ctx = asGetActiveContext();
    if (ctx) {
        return (ASContext*)ctx->GetUserData(0);
    } else {
        return NULL;
    }
}

void MessageCallback(const asSMessageInfo* msg, void* param) {
    const char* type = "ERR ";
    if (msg->type == asMSGTYPE_WARNING)
        type = "WARN";
    else if (msg->type == asMSGTYPE_INFORMATION)
        type = "INFO";

    std::ostringstream oss;
    oss << msg->section << " (" << msg->row << ", " << msg->col << ") : ";
    oss << type << " : " << msg->message << "\n";

    *((std::string*)param) += oss.str();
    //    printf("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
}

static uint32_t socket_id_invalid = SOCKET_ID_INVALID;

static uint32_t AS_CreateSocketTCP(std::string& host, uint16_t port) {
    return Engine::Instance()->GetASNetwork()->CreateSocketTCP(host, port);
}

static void AS_DestroySocketTCP(uint32_t socket) {
    return Engine::Instance()->GetASNetwork()->DestroySocketTCP(socket);
}

static int AS_SocketTCPSend(uint32_t socket, const CScriptArray& array) {
    uint8_t* datc = (uint8_t*)alloc.stack.Alloc(array.GetSize());
    for (unsigned i = 0; i < array.GetSize(); i++) {
        datc[i] = *(uint8_t*)array.At(i);
    }
    int ret = Engine::Instance()->GetASNetwork()->SocketTCPSend(socket, datc, array.GetSize());
    alloc.stack.Free(datc);
    return ret;
}

static bool AS_IsValidSocketTCP(uint32_t socket) {
    return Engine::Instance()->GetASNetwork()->IsValidSocketTCP(socket);
}

static CScriptArray* AS_GetPlayerStates() {
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<PlayerState>@"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);

    std::vector<PlayerState> vals = Online::Instance()->GetPlayerStates();

    array->Reserve(vals.size());

    for (auto& val : vals) {
        array->InsertLast((void*)&val);
    }

    return array;
}

static void PlayerStateDefaultConstructor(PlayerState* self) {
    new (self) PlayerState();
}

void AttachASNetwork(ASContext* context) {
    context->RegisterObjectType("PlayerState", sizeof(PlayerState), asOBJ_VALUE | asOBJ_POD);
    context->RegisterObjectProperty("PlayerState", "string playername", asOFFSET(PlayerState, playername));
    context->RegisterObjectProperty("PlayerState", "int avatar_id", asOFFSET(PlayerState, object_id));
    context->RegisterObjectProperty("PlayerState", "int ping", asOFFSET(PlayerState, ping));
    context->RegisterObjectBehaviour("PlayerState", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(PlayerStateDefaultConstructor), asCALL_CDECL_OBJLAST);

    context->RegisterGlobalFunction("array<PlayerState>@ GetPlayerStates()", asFUNCTION(AS_GetPlayerStates), asCALL_CDECL);

    context->RegisterGlobalProperty("uint SOCKET_ID_INVALID", &socket_id_invalid);
    context->RegisterGlobalFunction("uint CreateSocketTCP(string& host, uint16 port)",
                                    asFUNCTION(AS_CreateSocketTCP),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("void DestroySocketTCP(uint socket)",
                                    asFUNCTION(AS_DestroySocketTCP),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("int SocketTCPSend(uint socket, const array<uint8>& data)",
                                    asFUNCTION(AS_SocketTCPSend),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("bool IsValidSocketTCP(uint socket)",
                                    asFUNCTION(AS_IsValidSocketTCP),
                                    asCALL_CDECL);
}

// Function implementation with native calling convention
void PrintString(std::string& str) {
    printf("%s", str.c_str());
    // LogSystem::LogData( LogSystem::info,"us","Print()",0) << " " << str;
}

float AS_GetMoveXAxis(int controller_id) {
    PlayerInput* controller = Input::Instance()->GetController(controller_id);

    if (controller == NULL) {
        return 0.0f;
    }

    float left_depth = controller->key_down["left"].depth;
    float right_depth = controller->key_down["right"].depth;
    return right_depth - left_depth;
}

float AS_GetMoveYAxis(int controller_id) {
    PlayerInput* controller = Input::Instance()->GetController(controller_id);

    if (controller == NULL) {
        return 0.0f;
    }

    float up_depth = controller->key_down["up"].depth;
    float down_depth = controller->key_down["down"].depth;
    return down_depth - up_depth;
}

static bool AS_GetInputDownFiltered(int controller_id, const std::string& which_key, uint32_t filter) {
    PlayerInput* controller = Input::Instance()->GetController(controller_id);

    if (controller == NULL) {
        return false;
    }

    bool key_down = false;

    PlayerInput::KeyDownMap::iterator iter = controller->key_down.find(which_key);
    if (iter != controller->key_down.end()) {
        return iter->second.depth_count != 0;
    } else {
        if (which_key == "move_left") {
            key_down = controller->key_down["left"].count != 0;
        } else if (which_key == "move_right") {
            key_down = controller->key_down["right"].count != 0;
        } else if (which_key == "move_up") {
            key_down = controller->key_down["up"].count != 0;
        } else if (which_key == "move_down") {
            key_down = controller->key_down["down"].count != 0;
        } else if (which_key == "skip_dialogue") {
            key_down = controller->key_down["skip_dialogue"].count != 0;
        } else if (which_key == "mouse0") {
            key_down = Input::Instance()->getMouse().mouse_down_[Mouse::LEFT] != 0;
        } else if (which_key == "mousescrollup") {
            key_down = Input::Instance()->getMouse().wheel_delta_y_ > 0;
        } else if (which_key == "mousescrolldown") {
            key_down = Input::Instance()->getMouse().wheel_delta_y_ < 0;
        } else if (which_key == "mousescrollleft") {
            key_down = Input::Instance()->getMouse().wheel_delta_x_ < 0;
        } else if (which_key == "mousescrollright") {
            key_down = Input::Instance()->getMouse().wheel_delta_x_ > 0;
        } else {
            SDL_Scancode key = StringToSDLScancode(which_key);
            if (key != SDL_SCANCODE_SYSREQ) {
                key_down = Input::Instance()->getKeyboard().isScancodeDown(key, filter);
            }
        }
    }

    return key_down;
}

static bool AS_GetInputDown(int controller_id, const std::string& which_key) {
    return AS_GetInputDownFiltered(controller_id, which_key, KIMF_PLAYING);
}

static bool AS_GetInputPressedFiltered(int controller_id, const std::string& which_key, uint32_t filter) {
    PlayerInput* controller = Input::Instance()->GetController(controller_id);

    if (controller == NULL) {
        return false;
    }

    bool key_down = false;

    PlayerInput::KeyDownMap::iterator iter = controller->key_down.find(which_key);
    if (iter != controller->key_down.end()) {
        return iter->second.depth_count == 1;
    } else {
        if (which_key == "move_left") {
            key_down = controller->key_down["left"].count == 1;
        } else if (which_key == "move_right") {
            key_down = controller->key_down["right"].count == 1;
        } else if (which_key == "move_up") {
            key_down = controller->key_down["up"].count == 1;
        } else if (which_key == "move_down") {
            key_down = controller->key_down["down"].count == 1;
        } else if (which_key == "mouse0") {
            key_down = Input::Instance()->getMouse().mouse_down_[Mouse::LEFT] == 1;
        } else {
            SDL_Scancode key = StringToSDLScancode(which_key);
            if (key != SDL_SCANCODE_SYSREQ) {
                key_down = Input::Instance()->getKeyboard().wasScancodePressed(key, filter);
            }
        }
    }

    return key_down;
}

static bool AS_GetInputPressed(int controller_id, const std::string& which_key) {
    return AS_GetInputPressedFiltered(controller_id, which_key, KIMF_PLAYING);
}

void AS_ActivateKeyboardEvents() {
    ASContext* ctx = GetActiveASContext();

    if (ctx) {
        ctx->ActivateKeyboardEvents();
    }
}

void AS_DeactivateKeyboardEvents() {
    ASContext* ctx = GetActiveASContext();

    if (ctx) {
        ctx->DeactivateKeyboardEvents();
    }
}

const std::string AS_GetClipboard() {
    return SDL_GetClipboardText();
}

void AS_SetClipboard(const std::string& text) {
    SDL_SetClipboardText(text.c_str());
}

void AS_StartTextInput() {
    Input::Instance()->StartTextInput();
}

void AS_StopTextInput() {
    Input::Instance()->StopTextInput();
}

static int AS_GetCodeForKey(std::string name) {
    return StringToSDLScancode(name);
}

static std::string AS_GetLocaleStringForScancode(int scancode) {
    return SDLLocaleAdjustedStringFromScancode((SDL_Scancode)scancode);
}

static std::string AS_GetStringForMouseButton(int button) {
    return StringFromMouseButton(button);
}

static std::string AS_GetStringForControllerInput(int input) {
    return StringFromControllerInput((ControllerInput::Input)input);
}

static std::string AS_GetStringForMouseString(const std::string& text) {
    return StringFromMouseString(text);
}

static uint32_t AS_GetInputMode() {
    return Input::Instance()->getKeyboard().GetModes();
}

static bool AS_IsKeyDown(int which_key) {
    return Input::Instance()->getKeyboard().isScancodeDown((SDL_Scancode)which_key, KIMF_PLAYING);
}

float AS_GetLookXAxis(int controller_id) {
    PlayerInput* controller = Input::Instance()->GetController(controller_id);

    if (controller == NULL) {
        return 0.0f;
    }

    float left_depth = controller->key_down["look_left"].depth;
    float right_depth = controller->key_down["look_right"].depth;
    return right_depth - left_depth;
}

float AS_GetLookYAxis(int controller_id) {
    PlayerInput* controller = Input::Instance()->GetController(controller_id);

    if (controller == NULL) {
        return 0.0f;
    }

    float up_depth = controller->key_down["look_up"].depth;
    float down_depth = controller->key_down["look_down"].depth;
    return down_depth - up_depth;
}

void AS_SetGrabMouse(bool grab) {
    return Input::Instance()->SetGrabMouse(grab);
}

bool AS_GetDebugKeysEnabled() {
    return Input::Instance()->debug_keys && Engine::Instance()->current_engine_state_.type == kEngineEditorLevelState;
}

bool AS_EditorEnabled() {
    return Engine::Instance()->current_engine_state_.type == kEngineEditorLevelState;
}

static void AS_LoadEditorLevel() {
    Engine* engine = Engine::Instance();
    engine->NewLevel();
}

std::string AS_GetStringDescriptionForBinding(const std::string& type, const std::string& name) {
    if (type == "xbox")  // Backwards compat
        return Input::Instance()->GetStringDescriptionForBinding("controller", name);
    else
        return Input::Instance()->GetStringDescriptionForBinding(type, name);
}

static CScriptArray* AS_GetRawKeyboardInputs() {
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<KeyboardPress>"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);

    std::vector<Keyboard::KeyboardPress> vals = Input::Instance()->GetKeyboardInputs();

    array->Reserve(vals.size());

    for (auto& val : vals) {
        array->InsertLast((void*)&val);
    }

    return array;
}

static CScriptArray* AS_GetRawMouseInputs() {
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<MousePress>"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);

    std::vector<Mouse::MousePress> vals = Input::Instance()->GetMouseInputs();

    array->Reserve(vals.size());

    for (auto& val : vals) {
        array->InsertLast((void*)&val);
    }

    return array;
}

static CScriptArray* AS_GetRawJoystickInputs(int which) {
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<ControllerPress>"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);

    std::vector<Joystick::JoystickPress> vals = Input::Instance()->GetJoystickInputs(which);

    array->Reserve(vals.size());

    for (auto& val : vals) {
        array->InsertLast((void*)&val);
    }

    return array;
}

static bool AS_IsControllerConnected() {
    return Input::Instance()->IsControllerConnected();
}

void AttachUIQueries(ASContext* context) {
    context->RegisterGlobalFunction("bool GetInputDown(int controller_id, const string &in input_label)",
                                    asFUNCTION(AS_GetInputDown),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("bool GetInputDownFiltered(int controller_id, const string &in input_label, uint filter)",
                                    asFUNCTION(AS_GetInputDownFiltered),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("bool GetInputPressed(int controller_id, const string &in input_label)",
                                    asFUNCTION(AS_GetInputPressed),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("bool GetInputPressedFiltered(int controller_id, const string &in input_label, uint filter)",
                                    asFUNCTION(AS_GetInputPressedFiltered),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("void ActivateKeyboardEvents()",
                                    asFUNCTION(AS_ActivateKeyboardEvents),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("void DeactivateKeyboardEvents()",
                                    asFUNCTION(AS_DeactivateKeyboardEvents),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("string GetClipboard()",
                                    asFUNCTION(AS_GetClipboard),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("void SetClipboard(string text)",
                                    asFUNCTION(AS_SetClipboard),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("void StartTextInput()",
                                    asFUNCTION(AS_StartTextInput),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("void StopTextInput()",
                                    asFUNCTION(AS_StopTextInput),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("float GetLookXAxis(int controller_id)",
                                    asFUNCTION(AS_GetLookXAxis),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("float GetLookYAxis(int controller_id)",
                                    asFUNCTION(AS_GetLookYAxis),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("float GetMoveXAxis(int controller_id)",
                                    asFUNCTION(AS_GetMoveXAxis),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("float GetMoveYAxis(int controller_id)",
                                    asFUNCTION(AS_GetMoveYAxis),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("void SetGrabMouse(bool)",
                                    asFUNCTION(AS_SetGrabMouse),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("bool DebugKeysEnabled()",
                                    asFUNCTION(AS_GetDebugKeysEnabled),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("bool EditorEnabled()",
                                    asFUNCTION(AS_EditorEnabled),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("void LoadEditorLevel()",
                                    asFUNCTION(AS_LoadEditorLevel),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("bool IsKeyDown(int key_code)",
                                    asFUNCTION(AS_IsKeyDown),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("int GetCodeForKey(string key_name)",
                                    asFUNCTION(AS_GetCodeForKey),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("string GetStringDescriptionForBinding( const string& in, const string& in )",
                                    asFUNCTION(AS_GetStringDescriptionForBinding),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("string GetLocaleStringForScancode(int scancode)",
                                    asFUNCTION(AS_GetLocaleStringForScancode),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("string GetStringForMouseButton(int button)",
                                    asFUNCTION(AS_GetStringForMouseButton),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("string GetStringForControllerInput(int input)",
                                    asFUNCTION(AS_GetStringForControllerInput),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("string GetStringForMouseString(const string& text)",
                                    asFUNCTION(AS_GetStringForMouseString),
                                    asCALL_CDECL);

    context->RegisterEnum("KeyboardInputModeFlag");
    context->RegisterEnumValue("KeyboardInputModeFlag", "KIMF_NO", KIMF_NO);
    context->RegisterEnumValue("KeyboardInputModeFlag", "KIMF_MENU", KIMF_MENU);
    context->RegisterEnumValue("KeyboardInputModeFlag", "KIMF_PLAYING", KIMF_PLAYING);
    context->RegisterEnumValue("KeyboardInputModeFlag", "KIMF_LEVEL_EDITOR_GENERAL", KIMF_LEVEL_EDITOR_GENERAL);
    context->RegisterEnumValue("KeyboardInputModeFlag", "KIMF_LEVEL_EDITOR_DIALOGUE_EDITOR", KIMF_LEVEL_EDITOR_DIALOGUE_EDITOR);
    context->RegisterEnumValue("KeyboardInputModeFlag", "KIMF_GUI_GENERAL", KIMF_GUI_GENERAL);
    context->RegisterEnumValue("KeyboardInputModeFlag", "KIMF_ANY", KIMF_ANY);

    context->RegisterEnum("SDLNumeric");
    context->RegisterEnumValue("SDLNumeric", "K_ESCAPE", SDLK_ESCAPE);
    context->RegisterEnumValue("SDLNumeric", "K_ENTER", SDLK_RETURN);
    context->RegisterEnumValue("SDLNumeric", "KP_ENTER", SDLK_KP_ENTER);
    context->RegisterEnumValue("SDLNumeric", "K_BACKSPACE", SDLK_BACKSPACE);
    context->RegisterEnumValue("SDLNumeric", "K_TAB", SDLK_TAB);
    context->RegisterEnumValue("SDLNumeric", "K_0", SDLK_0);
    context->RegisterEnumValue("SDLNumeric", "K_1", SDLK_1);
    context->RegisterEnumValue("SDLNumeric", "K_2", SDLK_2);
    context->RegisterEnumValue("SDLNumeric", "K_3", SDLK_3);
    context->RegisterEnumValue("SDLNumeric", "K_4", SDLK_4);
    context->RegisterEnumValue("SDLNumeric", "K_5", SDLK_5);
    context->RegisterEnumValue("SDLNumeric", "K_6", SDLK_6);
    context->RegisterEnumValue("SDLNumeric", "K_7", SDLK_7);
    context->RegisterEnumValue("SDLNumeric", "K_8", SDLK_8);
    context->RegisterEnumValue("SDLNumeric", "K_9", SDLK_9);
    context->RegisterEnumValue("SDLNumeric", "KP_0", SDLK_KP_0);
    context->RegisterEnumValue("SDLNumeric", "KP_1", SDLK_KP_1);
    context->RegisterEnumValue("SDLNumeric", "KP_2", SDLK_KP_2);
    context->RegisterEnumValue("SDLNumeric", "KP_3", SDLK_KP_3);
    context->RegisterEnumValue("SDLNumeric", "KP_4", SDLK_KP_4);
    context->RegisterEnumValue("SDLNumeric", "KP_5", SDLK_KP_5);
    context->RegisterEnumValue("SDLNumeric", "KP_6", SDLK_KP_6);
    context->RegisterEnumValue("SDLNumeric", "KP_7", SDLK_KP_7);
    context->RegisterEnumValue("SDLNumeric", "KP_8", SDLK_KP_8);
    context->RegisterEnumValue("SDLNumeric", "KP_9", SDLK_KP_9);

    context->RegisterGlobalFunction("uint GetInputMode()",
                                    asFUNCTION(AS_GetInputMode),
                                    asCALL_CDECL);

    context->RegisterObjectType("KeyboardPress", sizeof(Keyboard::KeyboardPress), asOBJ_VALUE | asOBJ_POD);
    context->RegisterObjectProperty("KeyboardPress", "uint16 s_id", asOFFSET(Keyboard::KeyboardPress, s_id));
    context->RegisterObjectProperty("KeyboardPress", "uint32 keycode", asOFFSET(Keyboard::KeyboardPress, keycode));
    context->RegisterObjectProperty("KeyboardPress", "uint32 scancode", asOFFSET(Keyboard::KeyboardPress, scancode));
    context->RegisterObjectProperty("KeyboardPress", "uint16 mod", asOFFSET(Keyboard::KeyboardPress, mod));

    context->RegisterGlobalProperty("float last_keyboard_event_time", &(Input::Instance()->last_keyboard_event_time));
    context->RegisterGlobalProperty("float last_mouse_event_time", &(Input::Instance()->last_mouse_event_time));
    context->RegisterGlobalProperty("float last_controller_event_time", &(Input::Instance()->last_controller_event_time));

    context->RegisterGlobalFunction("array<KeyboardPress>@ GetRawKeyboardInputs()",
                                    asFUNCTION(AS_GetRawKeyboardInputs),
                                    asCALL_CDECL);

    context->RegisterEnum("MouseButton");
    context->RegisterEnumValue("MouseButton", "LEFT", Mouse::MouseButton::LEFT);
    context->RegisterEnumValue("MouseButton", "MIDDLE", Mouse::MouseButton::MIDDLE);
    context->RegisterEnumValue("MouseButton", "RIGHT", Mouse::MouseButton::RIGHT);
    context->RegisterEnumValue("MouseButton", "FOURTH", Mouse::MouseButton::FOURTH);
    context->RegisterEnumValue("MouseButton", "FIFTH", Mouse::MouseButton::FIFTH);
    context->RegisterEnumValue("MouseButton", "SIXTH", Mouse::MouseButton::SIXTH);
    context->RegisterEnumValue("MouseButton", "SEVENTH", Mouse::MouseButton::SEVENTH);
    context->RegisterEnumValue("MouseButton", "EIGHT", Mouse::MouseButton::EIGHT);
    context->RegisterEnumValue("MouseButton", "NINTH", Mouse::MouseButton::NINTH);
    context->RegisterEnumValue("MouseButton", "TENTH", Mouse::MouseButton::TENTH);
    context->RegisterEnumValue("MouseButton", "TWELFTH", Mouse::MouseButton::TWELFTH);

    context->RegisterObjectType("MousePress", sizeof(Mouse::MousePress), asOBJ_VALUE | asOBJ_POD);
    context->RegisterObjectProperty("MousePress", "uint16 s_id", asOFFSET(Mouse::MousePress, s_id));
    context->RegisterObjectProperty("MousePress", "MouseButton button", asOFFSET(Mouse::MousePress, button));

    context->RegisterGlobalFunction("array<MousePress>@ GetRawMouseInputs()",
                                    asFUNCTION(AS_GetRawMouseInputs),
                                    asCALL_CDECL);

    context->RegisterEnum("ControllerInput");
    context->RegisterEnumValue("ControllerInput", "A", ControllerInput::A);
    context->RegisterEnumValue("ControllerInput", "B", ControllerInput::B);
    context->RegisterEnumValue("ControllerInput", "X", ControllerInput::X);
    context->RegisterEnumValue("ControllerInput", "Y", ControllerInput::Y);
    context->RegisterEnumValue("ControllerInput", "D_UP", ControllerInput::D_UP);
    context->RegisterEnumValue("ControllerInput", "D_RIGHT", ControllerInput::D_RIGHT);
    context->RegisterEnumValue("ControllerInput", "D_DOWN", ControllerInput::D_DOWN);
    context->RegisterEnumValue("ControllerInput", "D_LEFT", ControllerInput::D_LEFT);
    context->RegisterEnumValue("ControllerInput", "START", ControllerInput::START);
    context->RegisterEnumValue("ControllerInput", "BACK", ControllerInput::BACK);
    context->RegisterEnumValue("ControllerInput", "GUIDE", ControllerInput::GUIDE);
    context->RegisterEnumValue("ControllerInput", "L_STICK_PRESSED", ControllerInput::L_STICK_PRESSED);
    context->RegisterEnumValue("ControllerInput", "R_STICK_PRESSED", ControllerInput::R_STICK_PRESSED);
    context->RegisterEnumValue("ControllerInput", "LB", ControllerInput::LB);
    context->RegisterEnumValue("ControllerInput", "RB", ControllerInput::RB);
    context->RegisterEnumValue("ControllerInput", "L_STICK_XN", ControllerInput::L_STICK_XN);
    context->RegisterEnumValue("ControllerInput", "L_STICK_XP", ControllerInput::L_STICK_XP);
    context->RegisterEnumValue("ControllerInput", "L_STICK_YN", ControllerInput::L_STICK_YN);
    context->RegisterEnumValue("ControllerInput", "L_STICK_YP", ControllerInput::L_STICK_YP);
    context->RegisterEnumValue("ControllerInput", "R_STICK_XN", ControllerInput::R_STICK_XN);
    context->RegisterEnumValue("ControllerInput", "R_STICK_XP", ControllerInput::R_STICK_XP);
    context->RegisterEnumValue("ControllerInput", "R_STICK_YN", ControllerInput::R_STICK_YN);
    context->RegisterEnumValue("ControllerInput", "R_STICK_YP", ControllerInput::R_STICK_YP);
    context->RegisterEnumValue("ControllerInput", "L_TRIGGER", ControllerInput::L_TRIGGER);
    context->RegisterEnumValue("ControllerInput", "R_TRIGGER", ControllerInput::R_TRIGGER);

    context->RegisterObjectType("ControllerPress", sizeof(Joystick::JoystickPress), asOBJ_VALUE | asOBJ_POD);
    context->RegisterObjectProperty("ControllerPress", "uint32 s_id", asOFFSET(Joystick::JoystickPress, s_id));
    context->RegisterObjectProperty("ControllerPress", "ControllerInput input", asOFFSET(Joystick::JoystickPress, input));
    context->RegisterObjectProperty("ControllerPress", "float depth", asOFFSET(Joystick::JoystickPress, depth));

    context->RegisterGlobalFunction("array<ControllerPress>@ GetRawControllerInputs(int which)",
                                    asFUNCTION(AS_GetRawJoystickInputs),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("bool IsControllerConnected()",
                                    asFUNCTION(AS_IsControllerConnected),
                                    asCALL_CDECL);
}

void ASDisplayError(const std::string& title, const std::string& contents) {
    DisplayError(title.c_str(), contents.c_str(), _ok);
}

void AttachError(ASContext* context) {
    context->RegisterGlobalFunction("void DisplayError(const string &in title, const string &in contents)",
                                    asFUNCTION(ASDisplayError),
                                    asCALL_CDECL);
}

float as_min(float a, float b) {
    return a < b ? a : b;
}

float as_max(float a, float b) {
    return a > b ? a : b;
}

int as_min_int(int a, int b) {
    return a < b ? a : b;
}

int as_max_int(int a, int b) {
    return a > b ? a : b;
}

float asPow(float a, float b) {
    return (float)pow(a, b);
}

// As AngelScript doesn't allow bitwise manipulation of float types we'll provide a couple of
// functions for converting float values to IEEE 754 formatted values etc. This also allow us to
// provide a platform agnostic representation to the script so the scripts don't have to worry
// about whether the CPU uses IEEE 754 floats or some other representation
float fpFromIEEE(asUINT raw) {
    // TODO: Identify CPU family to provide proper conversion
    //        if the CPU doesn't natively use IEEE style floats
    return *reinterpret_cast<float*>(&raw);
}
asUINT fpToIEEE(float fp) {
    return *reinterpret_cast<asUINT*>(&fp);
}
double fpFromIEEE(asQWORD raw) {
    return *reinterpret_cast<double*>(&raw);
}
asQWORD fpToIEEE(double fp) {
    return *reinterpret_cast<asQWORD*>(&fp);
}

static uint32_t uint32max = UINT32MAX;

void AttachMathFuncs(ASContext* context) {
    context->RegisterGlobalProperty("uint UINT32MAX", &uint32max);

    // Conversion between floating point and IEEE bits representations
    context->RegisterGlobalFunction("float fpFromIEEE(uint)", asFUNCTIONPR(fpFromIEEE, (asUINT), float), asCALL_CDECL);
    context->RegisterGlobalFunction("uint fpToIEEE(float)", asFUNCTIONPR(fpToIEEE, (float), asUINT), asCALL_CDECL);
    context->RegisterGlobalFunction("double fpFromIEEE(uint64)", asFUNCTIONPR(fpFromIEEE, (asQWORD), double), asCALL_CDECL);
    context->RegisterGlobalFunction("uint64 fpToIEEE(double)", asFUNCTIONPR(fpToIEEE, (double), asQWORD), asCALL_CDECL);

    context->RegisterGlobalFunction("float min(float,float)", asFUNCTION(as_min), asCALL_CDECL);
    context->RegisterGlobalFunction("float max(float,float)", asFUNCTION(as_max), asCALL_CDECL);
    context->RegisterGlobalFunction("int min(int,int)", asFUNCTION(as_min_int), asCALL_CDECL);
    context->RegisterGlobalFunction("int max(int,int)", asFUNCTION(as_max_int), asCALL_CDECL);
    context->RegisterGlobalFunction("float mix(float a,float b,float amount)", asFUNCTIONPR(mix, (float, float, float), float), asCALL_CDECL);

    context->RegisterGlobalFunction("float cos(float)", asFUNCTIONPR(cosf, (float), float), asCALL_CDECL);
    context->RegisterGlobalFunction("float sin(float)", asFUNCTIONPR(sinf, (float), float), asCALL_CDECL);
    context->RegisterGlobalFunction("float tan(float)", asFUNCTIONPR(tanf, (float), float), asCALL_CDECL);

    context->RegisterGlobalFunction("float acos(float)", asFUNCTIONPR(acosf, (float), float), asCALL_CDECL);
    context->RegisterGlobalFunction("float asin(float)", asFUNCTIONPR(asinf, (float), float), asCALL_CDECL);
    context->RegisterGlobalFunction("float atan(float)", asFUNCTIONPR(atanf, (float), float), asCALL_CDECL);
    context->RegisterGlobalFunction("float atan2(float,float)", asFUNCTIONPR(atan2f, (float, float), float), asCALL_CDECL);

    context->RegisterGlobalFunction("float log(float)", asFUNCTIONPR(logf, (float), float), asCALL_CDECL);
    context->RegisterGlobalFunction("float log10(float)", asFUNCTIONPR(log10f, (float), float), asCALL_CDECL);

    context->RegisterGlobalFunction("float pow(float val, float exponent)", asFUNCTIONPR(asPow, (float, float), float), asCALL_CDECL);
    context->RegisterGlobalFunction("float sqrt(float)", asFUNCTIONPR(sqrtf, (float), float), asCALL_CDECL);
    context->RegisterGlobalFunction("int rand()", asFUNCTIONPR(rand, (void), int), asCALL_CDECL);
    context->RegisterGlobalFunction("float RangedRandomFloat(float min, float max)", asFUNCTION(RangedRandomFloat), asCALL_CDECL);

    context->RegisterGlobalFunction("float ceil(float)", asFUNCTIONPR(ceilf, (float), float), asCALL_CDECL);
    context->RegisterGlobalFunction("float abs(float)", asFUNCTIONPR(fabsf, (float), float), asCALL_CDECL);
    context->RegisterGlobalFunction("float floor(float)", asFUNCTIONPR(floorf, (float), float), asCALL_CDECL);
}

#include "Math/quaternions.h"

static void quaternionDefaultConstructor(quaternion* self) {
    new (self) quaternion();
}

static void quaternionCopyConstructor(const quaternion& other, quaternion* self) {
    new (self) quaternion(other);
}

static void quaternionInitConstructor(float x, float y, float z, float w, quaternion* self) {
    new (self) quaternion(x, y, z, w);
}

static void quaternionInitConstructor2(vec4 vec, quaternion* self) {
    new (self) quaternion(vec);
}

static void ASMult_Generic(asIScriptGeneric* gen) {
    quaternion* a = (quaternion*)gen->GetArgObject(0);
    vec3* b = (vec3*)gen->GetArgObject(1);
    vec3 prod = *b;
    QuaternionMultiplyVector(a, &prod);
    gen->SetReturnObject(&prod);
}

/*
static void quatquatmultgeneric(asIScriptGeneric *gen) {
    quaternion *a = (quaternion*)gen->GetObject();
    quaternion *b = (quaternion*)gen->GetArgObject(0);
    quaternion prod = (*a)*(*b);
    gen->SetReturnObject(&prod);
}
*/

static quaternion quatquatmult(quaternion* a, const quaternion& b) {
    return *a * b;
}

static vec3 quatvecmult(quaternion* a, const vec3& b) {
    vec3 prod = b;
    QuaternionMultiplyVector(a, &prod);
    return prod;
}

static void quatdestructor(void* memory) {
    ((quaternion*)memory)->~quaternion();
}

static const quaternion& quatassign(quaternion* self, const quaternion& other) {
    return (*self) = other;
}

quaternion ASQuatMix(const quaternion& a, const quaternion& b, float alpha) {
    return mix(a, b, alpha);
}

void AttachQuaternionFuncs(ASContext* context) {
    // Register the type
    context->RegisterObjectType("quaternion", sizeof(quaternion), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);

    // Register the object properties
    context->RegisterObjectProperty("quaternion", "float x", asOFFSET(quaternion, entries));
    context->RegisterObjectProperty("quaternion", "float y", asOFFSET(quaternion, entries) + sizeof(float));
    context->RegisterObjectProperty("quaternion", "float z", asOFFSET(quaternion, entries) + sizeof(float) * 2);
    context->RegisterObjectProperty("quaternion", "float w", asOFFSET(quaternion, entries) + sizeof(float) * 3);

    // Register the constructors
    context->RegisterObjectBehaviour("quaternion", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(quaternionDefaultConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("quaternion", asBEHAVE_CONSTRUCT, "void f(const quaternion &in)", asFUNCTION(quaternionCopyConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("quaternion", asBEHAVE_CONSTRUCT, "void f(float, float, float, float)", asFUNCTION(quaternionInitConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("quaternion", asBEHAVE_CONSTRUCT, "void f(vec4)", asFUNCTION(quaternionInitConstructor2), asCALL_CDECL_OBJLAST, "Axis-angle (axis.x, axis.y, axis.z, angle_radians)");
    context->RegisterObjectBehaviour("quaternion", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(quatdestructor), asCALL_CDECL_OBJLAST);

    // Register the operator overloads
    context->RegisterObjectMethod("quaternion", "quaternion &opAssign(const quaternion &in)", asFUNCTION(quatassign), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("quaternion", "quaternion &opAddAssign(const quaternion &in)", asMETHOD(quaternion, operator+=), asCALL_THISCALL);
    context->RegisterObjectMethod("quaternion", "bool opEquals(const quaternion &in) const", asFUNCTIONPR(operator==, (const quaternion&, const quaternion&), bool), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("quaternion", "quaternion opAdd(const quaternion &in) const", asFUNCTIONPR(operator+, (const quaternion&, const quaternion&), const quaternion), asCALL_CDECL_OBJFIRST);
    // context->RegisterObjectMethod("quaternion", "quaternion opMul(const quaternion &in) const", asFUNCTIONPR(operator*, (const quaternion&, const quaternion&), const quaternion), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("quaternion", "quaternion opMul(const quaternion &in) const", asFUNCTION(quatquatmult), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("quaternion", "vec3 opMul(const vec3 &in) const", asFUNCTION(quatvecmult), asCALL_CDECL_OBJFIRST);
    context->DocsCloseBrace();

    // Register the object methods
    context->RegisterGlobalFunction("vec3 Mult(quaternion, vec3)", asFUNCTION(ASMult_Generic), asCALL_GENERIC, "Applies a quaternion rotation to a vector");
    context->RegisterGlobalFunction("quaternion mix(const quaternion &in a, const quaternion &in b, float alpha)", asFUNCTION(ASQuatMix), asCALL_CDECL);
    context->RegisterGlobalFunction("quaternion invert(quaternion quat)", asFUNCTIONPR(invert_by_val, (quaternion), quaternion), asCALL_CDECL);
    context->RegisterGlobalFunction("void GetRotationBetweenVectors(const vec3 &in start, const vec3 &in end, quaternion &out rotation)", asFUNCTION(GetRotationBetweenVectors), asCALL_CDECL);
}

static void ASSetSunPosition(vec3 pos) {
    the_scenegraph->primary_light.pos = normalize(pos);
    the_scenegraph->flares.flares[0]->position = the_scenegraph->primary_light.pos;
}

static void ASSetSunColor(vec3 color) {
    the_scenegraph->primary_light.color = color;
    the_scenegraph->flares.flares[0]->color = color;
}

static void ASSetSunAmbient(float ambient) {
    the_scenegraph->primary_light.intensity = ambient;
}

static vec3 ASGetSunPosition() {
    return the_scenegraph->primary_light.pos;
}

static vec3 ASGetSunColor() {
    return (the_scenegraph->primary_light.color);
}

static float ASGetSunAmbient() {
    return the_scenegraph->primary_light.intensity;
}

static void ASSetSkyTint(vec3 color) {
    the_scenegraph->sky->sky_tint = color;
}

static vec3 ASGetSkyTint() {
    return the_scenegraph->sky->sky_tint;
}

static vec3 ASGetBaseSkyTint() {
    return the_scenegraph->sky->sky_base_tint;
}

static void ASSetFlareDiffuse(float diffuse) {
    the_scenegraph->flares.flares[0]->diffuse = diffuse;
}

static void ASSetHDRWhitePoint(float val) {
    Graphics::Instance()->hdr_white_point = val;
}

static void ASSetHDRBlackPoint(float val) {
    Graphics::Instance()->hdr_black_point = val;
}

static void ASSetHDRBloomMult(float val) {
    Graphics::Instance()->hdr_bloom_mult = val;
}

static float ASGetHDRWhitePoint() {
    return Graphics::Instance()->hdr_white_point;
}

static float ASGetHDRBlackPoint() {
    return Graphics::Instance()->hdr_black_point;
}

static float ASGetHDRBloomMult() {
    return Graphics::Instance()->hdr_bloom_mult;
}

void AttachSky(ASContext* context) {
    // Register the object methods
    context->RegisterGlobalFunction("void SetSunPosition(vec3)", asFUNCTION(ASSetSunPosition), asCALL_CDECL);
    context->RegisterGlobalFunction("void SetSunColor(vec3)", asFUNCTION(ASSetSunColor), asCALL_CDECL);
    context->RegisterGlobalFunction("void SetSunAmbient(float)", asFUNCTION(ASSetSunAmbient), asCALL_CDECL);
    context->RegisterGlobalFunction("vec3 GetSunPosition()", asFUNCTION(ASGetSunPosition), asCALL_CDECL);
    context->RegisterGlobalFunction("vec3 GetSunColor()", asFUNCTION(ASGetSunColor), asCALL_CDECL);
    context->RegisterGlobalFunction("float GetSunAmbient()", asFUNCTION(ASGetSunAmbient), asCALL_CDECL);
    context->RegisterGlobalFunction("void SetFlareDiffuse(float)", asFUNCTION(ASSetFlareDiffuse), asCALL_CDECL);
    context->RegisterGlobalFunction("void SetSkyTint(vec3)", asFUNCTION(ASSetSkyTint), asCALL_CDECL);
    context->RegisterGlobalFunction("vec3 GetSkyTint()", asFUNCTION(ASGetSkyTint), asCALL_CDECL);
    context->RegisterGlobalFunction("vec3 GetBaseSkyTint()", asFUNCTION(ASGetBaseSkyTint), asCALL_CDECL);
    context->RegisterGlobalFunction("void SetHDRWhitePoint(float)", asFUNCTION(ASSetHDRWhitePoint), asCALL_CDECL);
    context->RegisterGlobalFunction("void SetHDRBlackPoint(float)", asFUNCTION(ASSetHDRBlackPoint), asCALL_CDECL);
    context->RegisterGlobalFunction("void SetHDRBloomMult(float)", asFUNCTION(ASSetHDRBloomMult), asCALL_CDECL);
    context->RegisterGlobalFunction("float GetHDRWhitePoint(void)", asFUNCTION(ASGetHDRWhitePoint), asCALL_CDECL);
    context->RegisterGlobalFunction("float GetHDRBlackPoint(void)", asFUNCTION(ASGetHDRBlackPoint), asCALL_CDECL);
    context->RegisterGlobalFunction("float GetHDRBloomMult(void)", asFUNCTION(ASGetHDRBloomMult), asCALL_CDECL);
}

std::vector<char> file_buf;
int file_index;

bool ASLoadFile(const std::string& path) {
    DiskFileDescriptor file;
    char abs_path[kPathSize];
    if (-1 == FindFilePath(path.c_str(), abs_path, kPathSize, kModPaths | kDataPaths | kAbsPath | kWriteDir | kModWriteDirs)) {
        const int kBufSize = 512;
        char err[kBufSize];
        FormatString(err, kBufSize, "Could not find file: %s", path.c_str());
        DisplayError("Error", err);
        return false;
    }
    if (!file.Open(abs_path, "r")) {
        const int kBufSize = 512;
        char err[kBufSize];
        FormatString(err, kBufSize, "Could not open file: %s", abs_path);
        DisplayError("Error", err);
        return false;
    }
    int file_size = file.GetSize();
    file_buf.resize(file_size);

    int bytes_read = file.ReadBytesPartial(&file_buf[0], file_size);
    file_buf.resize(bytes_read + 1);
    file_buf.back() = '\0';

    file.Close();
    file_index = 0;
    return true;
}

std::string ASGetFileLine() {
    std::string ret;
    int str_max = (int)file_buf.size();
    if (file_index == str_max || file_buf[file_index] == '\0') {
        return "end";
    }
    int str_end = file_index;
    for (int i = file_index; i < str_max; ++i) {
        str_end = i;
        if (file_buf[i] == '\n' || file_buf[i] == '\r' || file_buf[i] == '\0') {
            break;
        }
    }
    ret.resize(str_end - file_index);
    for (int i = file_index; i < str_end; ++i) {
        ret[i - file_index] = file_buf[i];
    }
    file_index = str_end + 1;
    return ret;
}

void ASStartWriteFile() {
    file_buf.clear();
}

void ASAddFileString(const std::string& str) {
    int index = file_buf.size();
    int str_len = str.length();
    file_buf.resize(index + str_len);
    for (int i = 0; i < str_len; ++i) {
        file_buf[index] = str[i];
        ++index;
    }
}

bool ASWriteFile(const std::string& path) {
    DiskFileDescriptor file;
    if (!file.Open(path, "w")) {
        return false;
    }
    file.WriteBytes(&file_buf[0], file_buf.size());
    file.Close();
    return true;
}

bool ASWriteFileKeepBackup(const std::string& path) {
    DiskFileDescriptor file;
    CreateBackup(path.c_str());

    if (!file.Open(path, "w")) {
        return false;
    }
    file.WriteBytes(&file_buf[0], file_buf.size());
    file.Close();
    return true;
}

bool ASWriteFileToWriteDir(const std::string& path) {
    DiskFileDescriptor file;
    std::string full_path = std::string(GetWritePath(CoreGameModID).c_str()) + "/" + path;

    CreateParentDirs(full_path);

    if (!file.Open(full_path, "w")) {
        return false;
    }

    file.WriteBytes(&file_buf[0], file_buf.size());
    file.Close();

    return true;
}

std::string ASGetLocalizedDialoguePath(const std::string& path) {
    const std::string& locale = config["language"].str();
    char buffer[kPathSize];
    strcpy(buffer, path.c_str());
    ApplicationPathSeparators(buffer);
    if (memcmp(buffer, "Data/Dialogues/", strlen("Data/Dialogues/")) == 0) {
        FormatString(buffer, kPathSize, "Data/DialoguesLocalized/%s/%s", locale.c_str(), path.c_str() + strlen("Data/Dialogues/"));
    }

    Path found_path = FindFilePath(buffer, kAnyPath, false);
    if (found_path.isValid()) {
        return found_path.GetFullPath();
    } else {
        FormatString(buffer, kPathSize, "Data/DialoguesLocalized/en_us/%s", path.c_str() + strlen("Data/Dialogues/"));
        found_path = FindFilePath(buffer, kAnyPath, false);
        if (found_path.isValid()) {
            return found_path.GetFullPath();
        }

        return path;
    }
}

void AttachSimpleFile(ASContext* context) {
    context->RegisterGlobalFunction("bool LoadFile(const string &in)", asFUNCTION(ASLoadFile), asCALL_CDECL);
    context->RegisterGlobalFunction("string GetFileLine()", asFUNCTION(ASGetFileLine), asCALL_CDECL);
    context->RegisterGlobalFunction("void StartWriteFile()", asFUNCTION(ASStartWriteFile), asCALL_CDECL);
    context->RegisterGlobalFunction("void AddFileString(const string &in)", asFUNCTION(ASAddFileString), asCALL_CDECL);
    context->RegisterGlobalFunction("bool WriteFile(const string &in)", asFUNCTION(ASWriteFile), asCALL_CDECL);
    context->RegisterGlobalFunction("bool WriteFileKeepBackup(const string &in)", asFUNCTION(ASWriteFileKeepBackup), asCALL_CDECL);
    context->RegisterGlobalFunction("bool WriteFileToWriteDir(const string &in)", asFUNCTION(ASWriteFileToWriteDir), asCALL_CDECL);
    context->RegisterGlobalFunction("string GetLocalizedDialoguePath(const string &in)", asFUNCTION(ASGetLocalizedDialoguePath), asCALL_CDECL);
}

static void BoneTransformDefaultConstructor(BoneTransform* self) {
    new (self) BoneTransform();
}

static void BoneTransformCopyConstructor(BoneTransform* self, const mat4& other) {
    new (self) BoneTransform(other);
}

static void BoneTransformCopyConstructor2(BoneTransform* self, const BoneTransform& other) {
    new (self) BoneTransform(other);
}

static BoneTransform BoneTransformOpMult(BoneTransform* self, const BoneTransform& other) {
    return *self * other;
}

static bool BoneTransformOpEquals(BoneTransform* self, const BoneTransform& other) {
    return (*self == other);
}

static BoneTransform BoneTransformOpMultQuat(quaternion* self, const BoneTransform& other) {
    return *self * other;
}

static vec3 BoneTransformOpMult2(BoneTransform* self, const vec3& other) {
    return *self * other;
}

static BoneTransform BoneTransformInvert(const BoneTransform& transform) {
    return invert(transform);
}

static BoneTransform BoneTransformMix(const BoneTransform& a, const BoneTransform& b, float alpha) {
    return mix(a, b, alpha);
}

static mat4 BoneTransformGetMat4(BoneTransform* self) {
    return self->GetMat4();
}

static void AttachBoneTransform(ASContext* context) {
    // Register the type
    context->RegisterObjectType("BoneTransform",
                                sizeof(BoneTransform),
                                asOBJ_VALUE | asOBJ_APP_CLASS | asOBJ_POD,
                                "An efficient way to define an unscaled transformation");
    context->RegisterObjectProperty("BoneTransform", "quaternion rotation", asOFFSET(BoneTransform, rotation));
    context->RegisterObjectProperty("BoneTransform", "vec3 origin", asOFFSET(BoneTransform, origin));

    // Register the constructors
    context->RegisterObjectBehaviour("BoneTransform", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(BoneTransformDefaultConstructor), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectBehaviour("BoneTransform", asBEHAVE_CONSTRUCT, "void f(const mat4 &in)", asFUNCTION(BoneTransformCopyConstructor), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectBehaviour("BoneTransform", asBEHAVE_CONSTRUCT, "void f(const BoneTransform &in)", asFUNCTION(BoneTransformCopyConstructor2), asCALL_CDECL_OBJFIRST);

    // Register the operator overloads
    context->RegisterObjectMethod("BoneTransform", "bool opEquals(const BoneTransform &in) const", asFUNCTION(BoneTransformOpEquals), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("BoneTransform", "BoneTransform opMul(const BoneTransform &in) const", asFUNCTION(BoneTransformOpMult), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("quaternion", "BoneTransform opMul(const BoneTransform &in) const", asFUNCTION(BoneTransformOpMultQuat), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("BoneTransform", "vec3 opMul(const vec3 &in) const", asFUNCTION(BoneTransformOpMult2), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("BoneTransform", "mat4 GetMat4() const", asFUNCTION(BoneTransformGetMat4), asCALL_CDECL_OBJFIRST);
    context->DocsCloseBrace();
    context->RegisterGlobalFunction("BoneTransform invert(const BoneTransform &in)", asFUNCTION(BoneTransformInvert), asCALL_CDECL);
    context->RegisterGlobalFunction("BoneTransform mix(const BoneTransform &in a, const BoneTransform &in b, float alpha)", asFUNCTION(BoneTransformMix), asCALL_CDECL);
}

static void mat4DefaultConstructor(mat4* self) {
    new (self) mat4();
}

static void mat4CopyConstructor(const mat4& other, mat4* self) {
    new (self) mat4(other);
}

void ASSetTranslationPart(vec3 val, mat4* self) {
    self->SetTranslationPart(val);
}

vec3 ASGetTranslationPart(mat4* self) {
    return self->GetTranslationPart();
}

void ASSetRotationPart(mat4 val, mat4* self) {
    self->SetRotationPart(val);
}

mat4 ASGetRotationPart(mat4* self) {
    return self->GetRotationPart();
}

void ASSetColumn(int which, vec3 val, mat4* self) {
    self->SetColumn(which, val);
}

vec3 ASGetColumn(int which, mat4* self) {
    return self->GetColumn(which).xyz();
}

mat4 AStranspose(mat4 mat) {
    return transpose(mat);
}

mat4 ASinvert(mat4 mat) {
    return invert(mat);
}

static void mat4multgeneric(asIScriptGeneric* gen) {
    mat4* a = (mat4*)gen->GetObject();
    mat4* b = (mat4*)gen->GetArgObject(0);
    mat4 prod = (*a) * (*b);
    gen->SetReturnObject(&prod);
}

static void mat4multvec3generic(asIScriptGeneric* gen) {
    mat4* a = (mat4*)gen->GetObject();
    vec3* b = (vec3*)gen->GetArgObject(0);
    vec3 prod = (*a) * (*b);
    gen->SetReturnObject(&prod);
}

static void mat4multvec4generic(asIScriptGeneric* gen) {
    mat4* a = (mat4*)gen->GetObject();
    vec4* b = (vec4*)gen->GetArgObject(0);
    vec4 prod = (*a) * (*b);
    gen->SetReturnObject(&prod);
}

static quaternion ASQuaternionFromMat4(const mat4& mat) {
    return QuaternionFromMat4(mat);
}

static mat4 ASMatrixMix(const mat4& a, const mat4& b, float alpha) {
    return mix(a, b, alpha);
}

static float& mat4index(unsigned int which, mat4* mat) {
    return mat->entries[which];
}

static const float& mat4indexconst(unsigned int which, mat4* mat) {
    return mat->entries[which];
}

static void mat3DefaultConstructor(mat3* self) {
    new (self) mat3();
}

static void mat3CopyConstructor(const mat3& other, mat3* self) {
    new (self) mat3(other);
}

static float& mat3index(unsigned int which, mat3* mat) {
    return mat->entries[which];
}

static const float& mat3indexconst(unsigned int which, mat3* mat) {
    return mat->entries[which];
}

static vec3 mat3multvec3(mat3* mat, const vec3& vec) {
    return *mat * vec;
}

void AttachMatrixFuncs(ASContext* context) {
    // Register the type
    context->RegisterObjectType("mat4",
                                sizeof(mat4),
                                asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CDA | asOBJ_APP_CLASS_ALLFLOATS);

    // Register the constructors
    context->RegisterObjectBehaviour("mat4", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(mat4DefaultConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("mat4", asBEHAVE_CONSTRUCT, "void f(const mat4 &in)", asFUNCTION(mat4CopyConstructor), asCALL_CDECL_OBJLAST);

    // Register the operator overloads
    context->RegisterObjectMethod("mat4", "float &opIndex(uint)", asFUNCTION(mat4index), asCALL_CDECL_OBJLAST);
    context->RegisterObjectMethod("mat4", "const float &opIndex(uint) const", asFUNCTION(mat4indexconst), asCALL_CDECL_OBJLAST);
    context->RegisterObjectMethod("mat4", "mat4 opMul(mat4) const", asFUNCTION(mat4multgeneric), asCALL_GENERIC);
    context->RegisterObjectMethod("mat4", "vec3 opMul(vec3) const", asFUNCTION(mat4multvec3generic), asCALL_GENERIC);
    context->RegisterObjectMethod("mat4", "vec3 opMul(vec4) const", asFUNCTION(mat4multvec4generic), asCALL_GENERIC);
    context->RegisterObjectMethod("mat4", "void SetTranslationPart(vec3)", asFUNCTION(ASSetTranslationPart), asCALL_CDECL_OBJLAST);
    context->RegisterObjectMethod("mat4", "vec3 GetTranslationPart() const", asFUNCTION(ASGetTranslationPart), asCALL_CDECL_OBJLAST);
    context->RegisterObjectMethod("mat4", "void SetRotationPart(mat4)", asFUNCTION(ASSetRotationPart), asCALL_CDECL_OBJLAST);
    context->RegisterObjectMethod("mat4", "mat4 GetRotationPart() const", asFUNCTION(ASGetRotationPart), asCALL_CDECL_OBJLAST);
    context->RegisterObjectMethod("mat4", "void SetRotationX(float)", asMETHOD(mat4, SetRotationX), asCALL_THISCALL);
    context->RegisterObjectMethod("mat4", "void SetRotationY(float)", asMETHOD(mat4, SetRotationY), asCALL_THISCALL);
    context->RegisterObjectMethod("mat4", "void SetRotationZ(float)", asMETHOD(mat4, SetRotationZ), asCALL_THISCALL);
    context->RegisterObjectMethod("mat4", "void SetColumn(int, vec3)", asFUNCTION(ASSetColumn), asCALL_CDECL_OBJLAST);
    context->RegisterObjectMethod("mat4", "vec3 GetColumn(int)", asFUNCTION(ASGetColumn), asCALL_CDECL_OBJLAST);
    context->DocsCloseBrace();
    context->RegisterGlobalFunction("mat4 transpose(mat4)", asFUNCTION(AStranspose), asCALL_CDECL);
    context->RegisterGlobalFunction("mat4 invert(mat4)", asFUNCTION(ASinvert), asCALL_CDECL);
    context->RegisterGlobalFunction("mat4 mix(const mat4 &in a, const mat4 &in b, float alpha)", asFUNCTION(ASMatrixMix), asCALL_CDECL);

    // Register the type
    context->RegisterObjectType("mat3",
                                sizeof(mat3),
                                asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CDA | asOBJ_APP_CLASS_ALLFLOATS);

    // Register the constructors
    context->RegisterObjectBehaviour("mat3", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(mat3DefaultConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("mat3", asBEHAVE_CONSTRUCT, "void f(const mat3 &in)", asFUNCTION(mat3CopyConstructor), asCALL_CDECL_OBJLAST);

    // Register the operator overloads
    context->RegisterObjectMethod("mat3", "float &opIndex(uint)", asFUNCTION(mat3index), asCALL_CDECL_OBJLAST);
    context->RegisterObjectMethod("mat3", "const float &opIndex(uint) const", asFUNCTION(mat3indexconst), asCALL_CDECL_OBJLAST);
    context->RegisterObjectMethod("mat3", "vec3 opMul(const vec3 &in) const", asFUNCTION(mat3multvec3), asCALL_CDECL_OBJFIRST);
    context->DocsCloseBrace();

    AttachBoneTransform(context);
}

static void vec3DefaultConstructor(vec3* self) {
    new (self) vec3();
}

static void vec3CopyConstructor(const vec3& other, vec3* self) {
    new (self) vec3(other);
}

static void vec3InitConstructor(float x, float y, float z, vec3* self) {
    new (self) vec3(x, y, z);
}

static void vec3InitConstructor2(float val, vec3* self) {
    new (self) vec3(val);
}

void AttachVec3Funcs(ASContext* context) {
    // Register the type
    context->RegisterObjectType("vec3",
                                sizeof(vec3),
                                asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CA | asOBJ_APP_CLASS_ALLFLOATS);

    // Register the object properties
    context->RegisterObjectProperty("vec3", "float x", asOFFSET(vec3, entries));
    context->RegisterObjectProperty("vec3", "float y", asOFFSET(vec3, entries) + sizeof(float));
    context->RegisterObjectProperty("vec3", "float z", asOFFSET(vec3, entries) + sizeof(float) * 2);

    // Register the constructors
    context->RegisterObjectBehaviour("vec3", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(vec3DefaultConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("vec3", asBEHAVE_CONSTRUCT, "void f(const vec3 &in)", asFUNCTION(vec3CopyConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("vec3", asBEHAVE_CONSTRUCT, "void f(float, float, float)", asFUNCTION(vec3InitConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("vec3", asBEHAVE_CONSTRUCT, "void f(float)", asFUNCTION(vec3InitConstructor2), asCALL_CDECL_OBJLAST);

    // Register the operator overloads
    context->RegisterObjectMethod("vec3", "vec3 &opAddAssign(const vec3 &in)", asFUNCTIONPR(operator+=, (vec3&, const vec3&), vec3&), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("vec3", "vec3 &opSubAssign(const vec3 &in)", asFUNCTIONPR(operator-=, (vec3&, const vec3&), vec3&), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("vec3", "vec3 &opMulAssign(float)", asFUNCTIONPR(operator*=, (vec3&, float), vec3&), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("vec3", "vec3 &opDivAssign(float)", asFUNCTIONPR(operator/=, (vec3&, float), vec3&), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("vec3", "bool opEquals(const vec3 &in) const", asFUNCTIONPR(operator==, (const vec3&, const vec3&), bool), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("vec3", "vec3 opAdd(const vec3 &in) const", asFUNCTIONPR(operator+, (const vec3&, const vec3&), vec3), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("vec3", "vec3 opSub(const vec3 &in) const", asFUNCTIONPR(operator-, (const vec3&, const vec3&), vec3), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("vec3", "vec3 opMul(float) const", asFUNCTIONPR(operator*, (const vec3&, float), vec3), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("vec3", "vec3 opMul(const vec3& in) const", asFUNCTIONPR(operator*, (const vec3&, const vec3&), vec3), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("vec3", "vec3 opMul_r(float) const", asFUNCTIONPR(operator*, (float, const vec3&), vec3), asCALL_CDECL_OBJLAST);
    context->RegisterObjectMethod("vec3", "vec3 opDiv(float) const", asFUNCTIONPR(operator/, (const vec3&, float), vec3), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("vec3", "float &opIndex(int)", asMETHODPR(vec3, operator[], (const int), float&), asCALL_THISCALL);
    context->DocsCloseBrace();

    // Register the object methods
    context->RegisterGlobalFunction("float length(const vec3 &in)", asFUNCTIONPR(length, (const vec3&), float), asCALL_CDECL);
    context->RegisterGlobalFunction("float length_squared(const vec3 &in)", asFUNCTIONPR(length_squared, (const vec3&), float), asCALL_CDECL);
    context->RegisterGlobalFunction("float dot(const vec3 &in, const vec3 &in)", asFUNCTIONPR(dot, (const vec3&, const vec3&), float), asCALL_CDECL);
    context->RegisterGlobalFunction("float distance(const vec3 &in, const vec3 &in)", asFUNCTIONPR(distance, (const vec3&, const vec3&), float), asCALL_CDECL);
    context->RegisterGlobalFunction("float distance_squared(const vec3 &in, const vec3 &in)", asFUNCTIONPR(distance_squared, (const vec3&, const vec3&), float), asCALL_CDECL);
    context->RegisterGlobalFunction("float xz_distance(const vec3 &in, const vec3 &in)", asFUNCTIONPR(xz_distance, (const vec3&, const vec3&), float), asCALL_CDECL);
    context->RegisterGlobalFunction("float xz_distance_squared(const vec3 &in, const vec3 &in)", asFUNCTIONPR(xz_distance_squared, (const vec3&, const vec3&), float), asCALL_CDECL);
    context->RegisterGlobalFunction("vec3 normalize(const vec3 &in)", asFUNCTIONPR(normalize, (const vec3&), vec3), asCALL_CDECL);
    context->RegisterGlobalFunction("vec3 cross(const vec3 &in, const vec3 &in)", asFUNCTIONPR(cross, (const vec3&, const vec3&), vec3), asCALL_CDECL);
    context->RegisterGlobalFunction("vec3 reflect(const vec3 &in vec, const vec3 &in normal)", asFUNCTIONPR(reflect, (const vec3&, const vec3&), vec3), asCALL_CDECL);
    context->RegisterGlobalFunction("vec3 mix(vec3 a,vec3 b,float alpha)", asFUNCTIONPR(mix, (vec3, vec3, float), vec3), asCALL_CDECL);
}

static void vec2DefaultConstructor(vec2* self) {
    new (self) vec2();
}

static void vec2CopyConstructor(const vec2& other, vec2* self) {
    new (self) vec2(other);
}

static void vec2InitConstructor(float x, float y, vec2* self) {
    new (self) vec2(x, y);
}

static void vec2InitConstructor2(float val, vec2* self) {
    new (self) vec2(val);
}

void AttachVec2Funcs(ASContext* context) {
    // Register the type
    context->RegisterObjectType("vec2",
                                sizeof(vec2),
                                asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CA | asOBJ_APP_CLASS_ALLFLOATS);

    // Register the object properties
    context->RegisterObjectProperty("vec2", "float x", asOFFSET(vec2, entries));
    context->RegisterObjectProperty("vec2", "float y", asOFFSET(vec2, entries) + sizeof(float));

    // Register the constructors
    context->RegisterObjectBehaviour("vec2", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(vec2DefaultConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("vec2", asBEHAVE_CONSTRUCT, "void f(const vec2 &in)", asFUNCTION(vec2CopyConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("vec2", asBEHAVE_CONSTRUCT, "void f(float, float)", asFUNCTION(vec2InitConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("vec2", asBEHAVE_CONSTRUCT, "void f(float)", asFUNCTION(vec2InitConstructor2), asCALL_CDECL_OBJLAST);

    // Register the operator overloads
    context->RegisterObjectMethod("vec2", "vec2 &opAddAssign(const vec2 &in)", asFUNCTIONPR(operator+=, (vec2&, const vec2&), vec2&), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("vec2", "vec2 &opSubAssign(const vec2 &in)", asFUNCTIONPR(operator-=, (vec2&, const vec2&), vec2&), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("vec2", "vec2 &opMulAssign(float)", asFUNCTIONPR(operator*=, (vec2&, float), vec2&), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("vec2", "vec2 &opDivAssign(float)", asFUNCTIONPR(operator/=, (vec2&, float), vec2&), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("vec2", "bool opEquals(const vec2 &in) const", asFUNCTIONPR(operator==, (const vec2&, const vec2&), bool), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("vec2", "vec2 opAdd(const vec2 &in) const", asFUNCTIONPR(operator+, (const vec2&, const vec2&), vec2), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("vec2", "vec2 opSub(const vec2 &in) const", asFUNCTIONPR(operator-, (const vec2&, const vec2&), vec2), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("vec2", "vec2 opMul(float) const", asFUNCTIONPR(operator*, (const vec2&, float), vec2), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("vec2", "vec2 opMul_r(float) const", asFUNCTIONPR(operator*, (float, const vec2&), vec2), asCALL_CDECL_OBJLAST);
    context->RegisterObjectMethod("vec2", "vec2 opDiv(float) const", asFUNCTIONPR(operator/, (const vec2&, float), vec2), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("vec2", "float &opIndex(int)", asMETHODPR(vec2, operator[], (const int), float&), asCALL_THISCALL);
    context->DocsCloseBrace();

    // Register the object methods
    context->RegisterGlobalFunction("float length(const vec2 &in)", asFUNCTIONPR(length, (const vec2&), float), asCALL_CDECL);
    context->RegisterGlobalFunction("float length_squared(const vec2 &in)", asFUNCTIONPR(length_squared, (const vec2&), float), asCALL_CDECL);
    context->RegisterGlobalFunction("float dot(const vec2 &in, const vec2 &in)", asFUNCTIONPR(dot, (const vec2&, const vec2&), float), asCALL_CDECL);
    context->RegisterGlobalFunction("float distance(const vec2 &in, const vec2 &in)", asFUNCTIONPR(distance, (const vec2&, const vec2&), float), asCALL_CDECL);
    context->RegisterGlobalFunction("float distance_squared(const vec2 &in, const vec2 &in)", asFUNCTIONPR(distance_squared, (const vec2&, const vec2&), float), asCALL_CDECL);
    context->RegisterGlobalFunction("vec2 normalize(const vec2 &in)", asFUNCTIONPR(normalize, (const vec2&), vec2), asCALL_CDECL);
    context->RegisterGlobalFunction("vec2 reflect(const vec2 &in vec, const vec2 &in normal)", asFUNCTIONPR(reflect, (const vec2&, const vec2&), vec2), asCALL_CDECL);
    context->RegisterGlobalFunction("vec2 mix(vec2 a,vec2 b,float alpha)", asFUNCTIONPR(mix, (vec2, vec2, float), vec2), asCALL_CDECL);
}

static void vec4DefaultConstructor(vec4* self) {
    new (self) vec4();
}

static void vec4CopyConstructor(const vec4& other, vec4* self) {
    new (self) vec4(other);
}

static void vec4InitConstructor(float x, float y, float z, float a, vec3* self) {
    new (self) vec4(x, y, z, a);
}

static void vec4InitConstructor2(float val, vec4* self) {
    new (self) vec4(val);
}

static void vec4InitConstructor3(const vec3& vec, float val, vec4* self) {
    new (self) vec4(vec, val);
}

vec4 ASMix(vec4 a, vec4 b, float alpha) {
    vec4 result;
    for (int i = 0; i < 4; ++i) {
        result[i] = a[i] * (1.0f - alpha) + b[i] * alpha;
    }
    return result;
}

void AttachVec4Funcs(ASContext* context) {
    // Register the type
    context->RegisterObjectType("vec4", sizeof(vec4), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CA | asOBJ_APP_CLASS_ALLFLOATS);

    // Register the object properties
    context->RegisterObjectProperty("vec4", "float x", asOFFSET(vec4, entries));
    context->RegisterObjectProperty("vec4", "float y", asOFFSET(vec4, entries) + sizeof(float));
    context->RegisterObjectProperty("vec4", "float z", asOFFSET(vec4, entries) + sizeof(float) * 2);
    context->RegisterObjectProperty("vec4", "float a", asOFFSET(vec4, entries) + sizeof(float) * 3);

    // Register the constructors
    context->RegisterObjectBehaviour("vec4", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(vec4DefaultConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("vec4", asBEHAVE_CONSTRUCT, "void f(const vec4 &in)", asFUNCTION(vec4CopyConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("vec4", asBEHAVE_CONSTRUCT, "void f(float, float, float, float)", asFUNCTION(vec4InitConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("vec4", asBEHAVE_CONSTRUCT, "void f(const vec3 &in, float)", asFUNCTION(vec4InitConstructor3), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("vec4", asBEHAVE_CONSTRUCT, "void f(float)", asFUNCTION(vec4InitConstructor2), asCALL_CDECL_OBJLAST);
    context->RegisterGlobalFunction("vec4 mix(vec4 a,vec4 b,float alpha)", asFUNCTION(ASMix), asCALL_CDECL);

    // Register the operator overloads
    context->RegisterObjectMethod("vec4", "float &opIndex(int)", asMETHODPR(vec4, operator[], (const int), float&), asCALL_THISCALL);
    context->DocsCloseBrace();
}

static void ivec2DefaultConstructor(ivec2* self) {
    new (self) ivec2();
}

static void ivec2CopyConstructor(const ivec2& other, ivec2* self) {
    new (self) ivec2(other);
}

static void ivec2InitConstructor(int x, int y, ivec2* self) {
    new (self) ivec2(x, y);
}

static void ivec2InitConstructor2(int val, ivec2* self) {
    new (self) ivec2(val);
}

void AttachIVec2Funcs(ASContext* context) {
    // Register the type
    context->RegisterObjectType("ivec2",
                                sizeof(ivec2),
                                asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CA | asOBJ_APP_CLASS_ALLINTS);

    // Register the object properties
    context->RegisterObjectProperty("ivec2", "int x", asOFFSET(ivec2, entries));
    context->RegisterObjectProperty("ivec2", "int y", asOFFSET(ivec2, entries) + sizeof(int));

    // Register the constructors
    context->RegisterObjectBehaviour("ivec2", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ivec2DefaultConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("ivec2", asBEHAVE_CONSTRUCT, "void f(const ivec2 &in)", asFUNCTION(ivec2CopyConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("ivec2", asBEHAVE_CONSTRUCT, "void f(int, int)", asFUNCTION(ivec2InitConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("ivec2", asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTION(ivec2InitConstructor2), asCALL_CDECL_OBJLAST);

    context->RegisterObjectMethod("ivec2", "ivec2 &opAddAssign(const ivec2 &in)", asFUNCTIONPR(operator+=, (ivec2&, const ivec2&), ivec2&), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("ivec2", "ivec2 &opSubAssign(const ivec2 &in)", asFUNCTIONPR(operator-=, (ivec2&, const ivec2&), ivec2&), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("ivec2", "ivec2 &opMulAssign(int)", asFUNCTIONPR(operator*=, (ivec2&, int), ivec2&), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("ivec2", "ivec2 &opDivAssign(int)", asFUNCTIONPR(operator/=, (ivec2&, int), ivec2&), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("ivec2", "ivec2 opAdd(const ivec2 &in) const", asFUNCTIONPR(operator+, (const ivec2&, const ivec2&), ivec2), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("ivec2", "ivec2 opSub(const ivec2 &in) const", asFUNCTIONPR(operator-, (const ivec2&, const ivec2&), ivec2), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("ivec2", "ivec2 opMul(int) const", asFUNCTIONPR(operator*, (const ivec2&, int), ivec2), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("ivec2", "ivec2 opDiv(int) const", asFUNCTIONPR(operator/, (const ivec2&, int), ivec2), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("ivec2", "int &opIndex(int)", asMETHODPR(ivec2, operator[], (const int), int&), asCALL_THISCALL);
    context->DocsCloseBrace();
}

static void ivec3DefaultConstructor(ivec3* self) {
    new (self) ivec3();
}

static void ivec3CopyConstructor(const ivec3& other, ivec3* self) {
    new (self) ivec3(other);
}

static void ivec3InitConstructor(int x, int y, int z, ivec3* self) {
    new (self) ivec3(x, y, z);
}

static void ivec3InitConstructor2(int val, ivec3* self) {
    new (self) ivec3(val);
}

void AttachIVec3Funcs(ASContext* context) {
    // Register the type
    context->RegisterObjectType("ivec3",
                                sizeof(ivec3),
                                asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CA | asOBJ_APP_CLASS_ALLINTS);

    // Register the object properties
    context->RegisterObjectProperty("ivec3", "int x", asOFFSET(ivec3, entries));
    context->RegisterObjectProperty("ivec3", "int y", asOFFSET(ivec3, entries) + sizeof(int));

    // Register the constructors
    context->RegisterObjectBehaviour("ivec3", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ivec3DefaultConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("ivec3", asBEHAVE_CONSTRUCT, "void f(const ivec3 &in)", asFUNCTION(ivec3CopyConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("ivec3", asBEHAVE_CONSTRUCT, "void f(int, int, int)", asFUNCTION(ivec3InitConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("ivec3", asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTION(ivec3InitConstructor2), asCALL_CDECL_OBJLAST);

    // Register the operator overloads
    context->RegisterObjectMethod("ivec3", "int &opIndex(int)", asMETHODPR(ivec3, operator[], (const int), int&), asCALL_THISCALL);
    context->DocsCloseBrace();
}

static void ivec4DefaultConstructor(ivec4* self) {
    new (self) ivec4();
}

static void ivec4CopyConstructor(const ivec4& other, ivec4* self) {
    new (self) ivec4(other);
}

static void ivec4InitConstructor(int x, int y, int z, int w, ivec4* self) {
    new (self) ivec4(x, y, z, w);
}

static void ivec4InitConstructor2(int val, ivec4* self) {
    new (self) ivec4(val);
}

void AttachIVec4Funcs(ASContext* context) {
    // Register the type
    context->RegisterObjectType("ivec4",
                                sizeof(ivec4),
                                asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CA | asOBJ_APP_CLASS_ALLINTS);

    // Register the object properties
    context->RegisterObjectProperty("ivec4", "int x", asOFFSET(ivec4, entries));
    context->RegisterObjectProperty("ivec4", "int y", asOFFSET(ivec4, entries) + sizeof(int));

    // Register the constructors
    context->RegisterObjectBehaviour("ivec4", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ivec4DefaultConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("ivec4", asBEHAVE_CONSTRUCT, "void f(const ivec4 &in)", asFUNCTION(ivec4CopyConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("ivec4", asBEHAVE_CONSTRUCT, "void f(int, int, int, int)", asFUNCTION(ivec4InitConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("ivec4", asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTION(ivec4InitConstructor2), asCALL_CDECL_OBJLAST);

    // Register the operator overloads
    context->RegisterObjectMethod("ivec4", "int &opIndex(int)", asMETHODPR(ivec4, operator[], (const int), int&), asCALL_THISCALL);
    context->DocsCloseBrace();
}

void Attach3DMathFuncs(ASContext* context) {
    AttachVec2Funcs(context);
    AttachVec3Funcs(context);
    AttachVec4Funcs(context);
    AttachIVec2Funcs(context);
    AttachIVec3Funcs(context);
    AttachIVec4Funcs(context);
    AttachQuaternionFuncs(context);
    AttachMatrixFuncs(context);

    context->RegisterGlobalFunction("mat4 Mat4FromQuaternion(const quaternion &in)", asFUNCTION(Mat4FromQuaternion), asCALL_CDECL);
    context->RegisterGlobalFunction("mat3 Mat3FromQuaternion(const quaternion &in)", asFUNCTION(Mat3FromQuaternion), asCALL_CDECL);
    context->RegisterGlobalFunction("quaternion QuaternionFromMat4(const mat4 &in)", asFUNCTION(ASQuaternionFromMat4), asCALL_CDECL);
}

#include "Graphics/camera.h"

Camera* GetCamera(int* id) { return (*id == -1 ? ActiveCameras::Get() : ActiveCameras::GetCamera(*id)); }

static vec3 AS_MetaCameraGetFacing(int* camera_id) { return GetCamera(camera_id)->GetFacing(); }

static void AS_MetaCameraFixDiscontinuity(int* camera_id) { GetCamera(camera_id)->FixDiscontinuity(); }

static vec3 AS_MetaCameraGetFlatFacing(int* camera_id) { return GetCamera(camera_id)->GetFlatFacing(); }

static vec3 AS_MetaCameraGetMouseRay(int* camera_id) { return GetCamera(camera_id)->GetMouseRay(); }

static float AS_MetaCameraGetXRotation(int* camera_id) { return GetCamera(camera_id)->GetXRotation(); }

static void AS_MetaCameraSetXRotation(int* camera_id, float val) { GetCamera(camera_id)->SetXRotation(val); }

static float AS_MetaCameraGetYRotation(int* camera_id) { return GetCamera(camera_id)->GetYRotation(); }

static void AS_MetaCameraSetYRotation(int* camera_id, float val) { GetCamera(camera_id)->SetYRotation(val); }

static float AS_MetaCameraGetZRotation(int* camera_id) { return GetCamera(camera_id)->GetZRotation(); }

static void AS_MetaCameraSetZRotation(int* camera_id, float val) { GetCamera(camera_id)->SetZRotation(val); }

static vec3 AS_MetaCameraGetPos(int* camera_id) { return GetCamera(camera_id)->GetPos(); }

static vec3 AS_MetaCameraGetUpVector(int* camera_id) { return GetCamera(camera_id)->GetUpVector(); }

static void AS_MetaCameraSetPos(int* camera_id, vec3 val) { GetCamera(camera_id)->SetPos(val); }

static void AS_MetaCameraSetFacing(int* camera_id, vec3 val) { GetCamera(camera_id)->SetFacing(val); }

static void AS_MetaCameraSetUp(int* camera_id, vec3 val) { GetCamera(camera_id)->SetUp(val); }

static void AS_MetaCameraCalcFacing(int* camera_id) { GetCamera(camera_id)->CalcFacing(); }

static void AS_MetaCameraCalcUp(int* camera_id) { GetCamera(camera_id)->calcUp(); }

static void AS_MetaCameraSetVelocity(int* camera_id, vec3 val) { GetCamera(camera_id)->SetVelocity(val); }

static void AS_MetaCameraLookAt(int* camera_id, vec3 val) { GetCamera(camera_id)->LookAt(val); }

static void AS_MetaCameraSetFOV(int* camera_id, float val) { GetCamera(camera_id)->SetFOV(val); }

static float AS_MetaCameraGetFOV(int* camera_id, float val) { return GetCamera(camera_id)->GetFOV(); }

static bool AS_MetaCameraGetAutoCamera(int* camera_id) { return GetCamera(camera_id)->GetAutoCamera(); }

static void AS_MetaCameraSetDistance(int* camera_id, float val) { GetCamera(camera_id)->SetDistance(val); }

static void AS_MetaCameraSetInterpSteps(int* camera_id, int val) { GetCamera(camera_id)->SetInterpSteps(val); }

static int AS_MetaCameraGetFlags(int* camera_id) { return GetCamera(camera_id)->GetFlags(); }

static void AS_MetaCameraSetFlags(int* camera_id, int val) { return GetCamera(camera_id)->SetFlags(val); }

static void AS_MetaCameraSetDOF(int* camera_id, float near_blur, float near_dist, float near_transition, float far_blur, float far_dist, float far_transition) {
    Camera* camera = GetCamera(camera_id);
    camera->near_blur_amount = near_blur;
    camera->near_sharp_dist = near_dist;
    camera->near_blur_transition_size = near_transition;
    camera->far_blur_amount = far_blur;
    camera->far_sharp_dist = pow(far_dist, 0.5f);
    camera->far_blur_transition_size = far_transition;
}

static const vec3& ASGetTint(int* camera_id) { return GetCamera(camera_id)->tint; }

static void ASSetTint(int* camera_id, const vec3& val) { GetCamera(camera_id)->tint = val; }

static const vec3& ASGetVignetteTint(int* camera_id) { return GetCamera(camera_id)->vignette_tint; }

static void ASSetVignetteTint(int* camera_id, const vec3& val) { GetCamera(camera_id)->vignette_tint = val; }

static bool GetSplitscreen() {
    return Engine::Instance()->GetSplitScreen();
}

static int GetScreenWidth() {
    return Graphics::Instance()->window_dims[0];
}

static int GetScreenHeight() {
    return Graphics::Instance()->window_dims[1];
}

static int GetBloodLevel() {
    return Graphics::Instance()->config_.blood();
}

static const vec3& GetBloodTint() {
    return Graphics::Instance()->config_.blood_color();
}

void AttachScreenWidth(ASContext* context) {
    context->RegisterGlobalFunction("int GetScreenWidth()", asFUNCTION(GetScreenWidth), asCALL_CDECL);
    context->RegisterGlobalFunction("int GetScreenHeight()", asFUNCTION(GetScreenHeight), asCALL_CDECL);
}

static int zero_camera_id = -1;
void AttachMovementObjectCamera(ASContext* context, MovementObject* mo) {
    context->RegisterObjectType("Camera", 0, asOBJ_REF | asOBJ_NOHANDLE);
    context->RegisterObjectMethod("Camera", "vec3 &GetTint()", asFUNCTION(ASGetTint), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Camera", "void SetTint(const vec3 &in)", asFUNCTION(ASSetTint), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Camera", "vec3 &GetVignetteTint()", asFUNCTION(ASGetVignetteTint), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Camera", "void SetVignetteTint(const vec3 &in)", asFUNCTION(ASSetVignetteTint), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectMethod("Camera", "void FixDiscontinuity()", asFUNCTION(AS_MetaCameraFixDiscontinuity), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Camera", "vec3 GetFacing()", asFUNCTION(AS_MetaCameraGetFacing), asCALL_CDECL_OBJFIRST);
    // context->RegisterObjectMethod("Camera","void SetFacing()",asFUNCTION(AS_MetaCameraSetFacing), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Camera", "vec3 GetFlatFacing()", asFUNCTION(AS_MetaCameraGetFlatFacing), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Camera", "vec3 GetMouseRay()", asFUNCTION(AS_MetaCameraGetMouseRay), asCALL_CDECL_OBJFIRST, "Direction that mouse cursor is pointing");
    context->RegisterObjectMethod("Camera", "float GetXRotation()", asFUNCTION(AS_MetaCameraGetXRotation), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Camera", "void SetXRotation(float)", asFUNCTION(AS_MetaCameraSetXRotation), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Camera", "float GetYRotation()", asFUNCTION(AS_MetaCameraGetYRotation), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Camera", "void SetYRotation(float)", asFUNCTION(AS_MetaCameraSetYRotation), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Camera", "float GetZRotation()", asFUNCTION(AS_MetaCameraGetZRotation), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Camera", "void SetZRotation(float)", asFUNCTION(AS_MetaCameraSetZRotation), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Camera", "vec3 GetPos()", asFUNCTION(AS_MetaCameraGetPos), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Camera", "vec3 GetUpVector()", asFUNCTION(AS_MetaCameraGetUpVector), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Camera", "void SetPos(vec3)", asFUNCTION(AS_MetaCameraSetPos), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Camera", "void SetFacing(vec3)", asFUNCTION(AS_MetaCameraSetFacing), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Camera", "void SetUp(vec3)", asFUNCTION(AS_MetaCameraSetUp), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Camera", "void CalcFacing()", asFUNCTION(AS_MetaCameraCalcFacing), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Camera", "void SetVelocity(vec3)", asFUNCTION(AS_MetaCameraSetVelocity), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Camera", "void LookAt(vec3 target)", asFUNCTION(AS_MetaCameraLookAt), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Camera", "void SetFOV(float)", asFUNCTION(AS_MetaCameraSetFOV), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Camera", "float GetFOV()", asFUNCTION(AS_MetaCameraGetFOV), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Camera", "bool GetAutoCamera()", asFUNCTION(AS_MetaCameraGetAutoCamera), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Camera", "void SetDistance(float)", asFUNCTION(AS_MetaCameraSetDistance), asCALL_CDECL_OBJFIRST, "From orbit point, for chase camera");
    context->RegisterObjectMethod("Camera", "void SetInterpSteps(int)", asFUNCTION(AS_MetaCameraSetInterpSteps), asCALL_CDECL_OBJFIRST, "Number of time steps between camera updates");
    context->RegisterObjectMethod("Camera", "int GetFlags()", asFUNCTION(AS_MetaCameraGetFlags), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Camera", "void SetFlags(int)", asFUNCTION(AS_MetaCameraSetFlags), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Camera", "void CalcUp()", asFUNCTION(AS_MetaCameraCalcUp), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Camera", "void SetDOF(float near_blur, float near_dist, float near_transition, float far_blur, float far_dist, float far_transition)", asFUNCTION(AS_MetaCameraSetDOF), asCALL_CDECL_OBJFIRST);

    context->DocsCloseBrace();
    context->RegisterGlobalProperty("Camera camera", mo ? &mo->camera_id : &zero_camera_id);
    context->RegisterGlobalFunction("bool GetSplitscreen()", asFUNCTION(GetSplitscreen), asCALL_CDECL);
    context->RegisterGlobalFunction("int GetBloodLevel()", asFUNCTION(GetBloodLevel), asCALL_CDECL);
    context->RegisterGlobalFunction("const vec3 &GetBloodTint()", asFUNCTION(GetBloodTint), asCALL_CDECL);
    context->RegisterEnum("CameraFlags");
    context->RegisterEnumValue("CameraFlags", "kEditorCamera", Camera::kEditorCamera);
    context->RegisterEnumValue("CameraFlags", "kPreviewCamera", Camera::kPreviewCamera);
    context->DocsCloseBrace();
}

void AttachActiveCamera(ASContext* context) {
    AttachMovementObjectCamera(context, NULL);
}

#include "Internal/timer.h"
static void ASTimedSlowMotion(float target_time_scale, float how_long, float delay) {
    Online* online = Online::Instance();
    if (online->IsHosting()) {
        online->Send<OnlineMessages::TimedSlowMotion>(target_time_scale, how_long, delay);
    }
    game_timer.AddTimedSlowMotionLayer(target_time_scale, how_long, delay);
}

static uint64_t GetPerformanceCounter() {
    return SDL_GetPerformanceCounter();
}

static uint64_t GetPerformanceFrequency() {
    return SDL_GetPerformanceFrequency();
}

static bool GetMenuPaused() {
    return Engine::Instance()->menu_paused;
}

void AttachTimer(ASContext* context) {
    context->RegisterGlobalProperty("float time_step", &game_timer.timestep, "Time in seconds between engine time steps");
    context->RegisterGlobalProperty("float the_time", &game_timer.game_time, "The current time in seconds since engine started (in-game time)");
    context->RegisterGlobalProperty("float ui_time", &ui_timer.game_time, "The current time in seconds since engine started (absolute time)");
    context->RegisterGlobalFunction("void TimedSlowMotion(float target_time_scale, float how_long, float delay)", asFUNCTION(ASTimedSlowMotion), asCALL_CDECL, "Used to trigger brief periods of slow motion");
    context->RegisterGlobalFunction("uint64 GetPerformanceCounter()", asFUNCTION(GetPerformanceCounter), asCALL_CDECL, "Get high precision time info for profiling");
    context->RegisterGlobalFunction("uint64 GetPerformanceFrequency()", asFUNCTION(GetPerformanceFrequency), asCALL_CDECL, "Used to convert PerformanceCounter into seconds");
    context->RegisterGlobalFunction("bool GetMenuPaused()", asFUNCTION(GetMenuPaused), asCALL_CDECL, "Is game paused by a menu");
}

#include "Physics/physics.h"
void AttachPhysics(ASContext* context) {
    Physics* physics = Physics::Instance();

    context->RegisterObjectType("Physics", 0, asOBJ_REF | asOBJ_NOHANDLE);
    context->RegisterObjectProperty("Physics", "vec3 gravity_vector", asOFFSET(Physics, gravity));
    context->DocsCloseBrace();
    context->RegisterGlobalProperty("Physics physics", physics);
}

#include "Sound/sound.h"
static int ASPlaySound(std::string path) {
    Online* online = Online::Instance();

    if (online->IsHosting()) {
        online->Send<OnlineMessages::AudioPlaySoundMessage>(path);
    }

    SoundPlayInfo spi;
    spi.path = path;
    spi.flags = spi.flags | SoundFlags::kRelative;
    int handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
    Engine::Instance()->GetSound()->Play(handle, spi);
    return handle;
}

static int ASPlaySoundLoop(const std::string& path, float gain) {
    Online* online = Online::Instance();

    if (online->IsHosting()) {
        online->Send<OnlineMessages::AudioPlaySoundLoopMessage>(path, gain);
    }

    SoundPlayInfo spi;
    spi.path = path;
    spi.flags = spi.flags | SoundFlags::kRelative;
    spi.looping = true;
    spi.volume = gain;
    int handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
    Engine::Instance()->GetSound()->Play(handle, spi);
    return handle;
}

static int ASPlaySoundLoopAtLocation(const std::string& path, vec3 pos, float gain) {
    Online* online = Online::Instance();

    if (online->IsHosting()) {
        online->Send<OnlineMessages::AudioPlaySoundLoopAtLocationMessage>(path, gain, pos);
    }

    SoundPlayInfo spi;
    spi.path = path;
    spi.looping = true;
    spi.volume = gain;
    spi.position = pos;
    int handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
    Engine::Instance()->GetSound()->Play(handle, spi);
    return handle;
}

static int ASPlaySoundAtLocation(std::string path, vec3 location) {
    Online* online = Online::Instance();

    if (online->IsHosting()) {
        online->Send<OnlineMessages::AudioPlaySoundLocationMessage>(path, location);
    }

    SoundPlayInfo spi;
    spi.path = path;
    spi.position = location;
    int handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
    Engine::Instance()->GetSound()->Play(handle, spi);
    return handle;
}

static int ASPlaySoundGroupRelative(std::string path) {
    // SoundGroupRef sgr = SoundGroups::Instance()->ReturnRef(path);
    Online* online = Online::Instance();

    if (online->IsHosting()) {
        online->Send<OnlineMessages::AudioPlaySoundGroupRelativeMessage>(path);
    }
    SoundGroupRef sgr = Engine::Instance()->GetAssetManager()->LoadSync<SoundGroup>(path);
    SoundGroupPlayInfo sgpi(SoundGroupPlayInfo(*sgr, vec3(0.0f)));
    sgpi.flags = sgpi.flags | SoundFlags::kRelative;

    int handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
    Engine::Instance()->GetSound()->PlayGroup(handle, sgpi);
    return handle;
}

static int ASPlaySoundGroupRelativeGain(std::string path, float gain) {
    // SoundGroupRef sgr = SoundGroups::Instance()->ReturnRef(path);
    Online* online = Online::Instance();

    if (online->IsHosting()) {
        online->Send<OnlineMessages::AudioPlaySoundGroupRelativeGainMessage>(path, gain);
    }

    SoundGroupRef sgr = Engine::Instance()->GetAssetManager()->LoadSync<SoundGroup>(path);
    SoundGroupPlayInfo sgpi(SoundGroupPlayInfo(*sgr, vec3(0.0f)));
    sgpi.flags = sgpi.flags | SoundFlags::kRelative;
    sgpi.gain = gain;

    int handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
    Engine::Instance()->GetSound()->PlayGroup(handle, sgpi);
    return handle;
}

static int ASPlaySoundGroup(std::string path, vec3 location) {
    Online* online = Online::Instance();

    if (online->IsHosting()) {
        online->Send<OnlineMessages::AudioPlaySoundGroupMessage>(path, location);
    }

    int handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
    // SoundGroupRef sgr = SoundGroups::Instance()->ReturnRef(path);
    SoundGroupRef sgr = Engine::Instance()->GetAssetManager()->LoadSync<SoundGroup>(path);
    SoundGroupPlayInfo sgpi(*sgr, location);
    Engine::Instance()->GetSound()->PlayGroup(handle, sgpi);
    return handle;
}

static int ASPlaySoundGroupPriority(std::string path, vec3 location, int priority) {
    // SoundGroupRef sgr = SoundGroups::Instance()->ReturnRef(path);

    Online* online = Online::Instance();

    if (online->IsHosting()) {
        online->Send<OnlineMessages::AudioPlayGroupPriorityMessage>(path, location, priority);
    }

    SoundGroupRef sgr = Engine::Instance()->GetAssetManager()->LoadSync<SoundGroup>(path);
    SoundGroupPlayInfo sgpi(*sgr, location);
    sgpi.priority = priority;
    int handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
    Engine::Instance()->GetSound()->PlayGroup(handle, sgpi);
    return handle;
}

static int ASPlaySoundGroupGain(std::string path, vec3 location, float gain) {
    // SoundGroupRef sgr = SoundGroups::Instance()->ReturnRef(path);
    Online* online = Online::Instance();

    if (online->IsHosting()) {
        online->Send<OnlineMessages::AudioPlaySoundGroupGainMessage>(path, location, gain);
    }

    SoundGroupRef sgr = Engine::Instance()->GetAssetManager()->LoadSync<SoundGroup>(path);
    SoundGroupPlayInfo sgpi(*sgr, location);
    sgpi.gain = gain;
    int handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
    Engine::Instance()->GetSound()->PlayGroup(handle, sgpi);
    return handle;
}

static void ASSetRemoteAirWhoosh(int id, float volume, float pitch) {
    // Send the whoosh to the client
    for (const auto& it : Online::Instance()->online_session->player_states) {
        if (it.second.object_id == id) {  // TODO This assumes that player_id == peer_id!
            Peer* peer = Online::Instance()->GetPeerFromID(it.first);
            if (peer != nullptr) {
                Online::Instance()->SendTo<OnlineMessages::WhooshSoundMessage>(peer->conn_id, volume, pitch);
            } else {
                LOGW << "Tried ASSetRemoteAirWhoosh(). We managed to find I PlayerState, but couldn't resolve a peer." << std::endl;
            }
            break;
        }
    }
}

static void ASSetAirWhoosh(float volume, float pitch) {
    Engine::Instance()->GetSound()->setAirWhoosh(volume, pitch);
}

static void ASUpdateListener(vec3 a, vec3 b, vec3 c, vec3 d) {
    Engine::Instance()->GetSound()->updateListener(a, b, c, d);
}

static void ASStopSound(int which) {
    Engine::Instance()->GetSound()->Stop(which);
}

static void ASSetSoundGain(int which, float gain) {
    Engine::Instance()->GetSound()->SetVolume(which, gain);
}

static void ASSetSoundPitch(int which, float pitch) {
    Engine::Instance()->GetSound()->SetPitch(which, pitch);
}

static void ASSetSoundPosition(int which, vec3 pos) {
    Engine::Instance()->GetSound()->SetPosition(which, pos);
}

static void ASPlaySong(const std::string& type) {
    Engine::Instance()->GetSound()->TransitionToSong(type);
}

static void ASQueueSegment(const std::string& name) {
    Engine::Instance()->GetSound()->QueueSegment(name);
}

static void ASPlaySegment(const std::string& name) {
    Engine::Instance()->GetSound()->TransitionToSegment(name);
}

static bool ASAddMusic(const std::string& path) {
    Path p = FindFilePath(path);
    if (p.valid) {
        Engine::Instance()->GetSound()->AddMusic(p);
        return true;
    } else {
        LOGE << "Invalid path, can't load music" << p << std::endl;
        return false;
    }
}

static bool ASRemoveMusic(const std::string& path) {
    Path p = FindFilePath(path);
    if (p.valid) {
        Engine::Instance()->GetSound()->RemoveMusic(p);
        return true;
    } else {
        LOGE << "Invalid path, can't remove music" << p << std::endl;
        return false;
    }
}

static void ASSetSegment(const std::string& name) {
    Engine::Instance()->GetSound()->SetSegment(name);
}

static std::string ASGetSegment() {
    return Engine::Instance()->GetSound()->GetSegment();
}

static void ASSetSong(const std::string& name) {
    Engine::Instance()->GetSound()->SetSong(name);
}

static std::string ASGetSong() {
    return Engine::Instance()->GetSound()->GetSongName();
}

static CScriptArray* ASGetLayerNames() {
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<string>"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);

    std::map<std::string, float> layer_gains = Engine::Instance()->GetSound()->GetLayerGains();

    array->Reserve(layer_gains.size());

    std::map<std::string, float>::iterator layerit = layer_gains.begin();

    for (; layerit != layer_gains.end(); layerit++) {
        array->InsertLast((void*)&(layerit->first));
    }

    return array;
}

static void ASSetLayerGain(const std::string& layer, float gain) {
    Engine::Instance()->GetSound()->SetLayerGain(layer, gain);
}

static float ASGetLayerGain(const std::string& layer) {
    std::map<std::string, float> layer_gains = Engine::Instance()->GetSound()->GetLayerGains();

    std::map<std::string, float>::iterator layerit = layer_gains.find(layer);

    if (layerit != layer_gains.end()) {
        return layerit->second;
    } else {
        return 0.0f;
    }
}

void ASReloadMods() {
    ModLoading::Instance().Reload();
}

const int as_sound_priority_max = _sound_priority_max;
const int as_sound_priority_very_high = _sound_priority_very_high;
const int as_sound_priority_high = _sound_priority_high;
const int as_sound_priority_med = _sound_priority_med;
const int as_sound_priority_low = _sound_priority_low;

void AttachSound(ASContext* context) {
    context->RegisterGlobalFunction("void UpdateListener(vec3 pos, vec3 vel, vec3 facing, vec3 up)", asFUNCTION(ASUpdateListener), asCALL_CDECL);
    context->RegisterGlobalFunction("int PlaySound(string path)", asFUNCTION(ASPlaySound), asCALL_CDECL);
    context->RegisterGlobalFunction("int PlaySoundLoop(const string &in path, float gain)", asFUNCTION(ASPlaySoundLoop), asCALL_CDECL);
    context->RegisterGlobalFunction("int PlaySoundLoopAtLocation(const string &in path, vec3 pos, float gain)", asFUNCTION(ASPlaySoundLoopAtLocation), asCALL_CDECL);
    context->RegisterGlobalFunction("void SetSoundGain(int handle, float gain)", asFUNCTION(ASSetSoundGain), asCALL_CDECL);
    context->RegisterGlobalFunction("void SetSoundPitch(int handle, float pitch)", asFUNCTION(ASSetSoundPitch), asCALL_CDECL);
    context->RegisterGlobalFunction("void SetSoundPosition(int handle, vec3 pos)", asFUNCTION(ASSetSoundPosition), asCALL_CDECL);
    context->RegisterGlobalFunction("void StopSound(int handle)", asFUNCTION(ASStopSound), asCALL_CDECL);
    context->RegisterGlobalFunction("int PlaySound(string path, vec3 position)", asFUNCTION(ASPlaySoundAtLocation), asCALL_CDECL);
    context->RegisterGlobalFunction("int PlaySoundGroup(string path)", asFUNCTION(ASPlaySoundGroupRelative), asCALL_CDECL);
    context->RegisterGlobalFunction("int PlaySoundGroup(string path, float gain)", asFUNCTION(ASPlaySoundGroupRelativeGain), asCALL_CDECL);
    context->RegisterGlobalFunction("int PlaySoundGroup(string path, vec3 position)", asFUNCTION(ASPlaySoundGroup), asCALL_CDECL);
    context->RegisterGlobalFunction("int PlaySoundGroup(string path, vec3 position, float gain)", asFUNCTION(ASPlaySoundGroupGain), asCALL_CDECL);
    context->RegisterGlobalFunction("int PlaySoundGroup(string path, vec3 position, int priority)", asFUNCTION(ASPlaySoundGroupPriority), asCALL_CDECL);
    context->RegisterGlobalFunction("void SetAirWhoosh(float volume, float pitch)", asFUNCTION(ASSetAirWhoosh), asCALL_CDECL);
    context->RegisterGlobalFunction("void SetRemoteAirWhoosh(int id, float volume, float pitch)", asFUNCTION(ASSetRemoteAirWhoosh), asCALL_CDECL);
    context->RegisterGlobalProperty("const int _sound_priority_max", (void*)&as_sound_priority_max);
    context->RegisterGlobalProperty("const int _sound_priority_very_high", (void*)&as_sound_priority_very_high);
    context->RegisterGlobalProperty("const int _sound_priority_high", (void*)&as_sound_priority_high);
    context->RegisterGlobalProperty("const int _sound_priority_med", (void*)&as_sound_priority_med);
    context->RegisterGlobalProperty("const int _sound_priority_low", (void*)&as_sound_priority_low);

    context->RegisterGlobalFunction("bool AddMusic(const string& in)", asFUNCTION(ASAddMusic), asCALL_CDECL);
    context->RegisterGlobalFunction("bool RemoveMusic(const string& in)", asFUNCTION(ASRemoveMusic), asCALL_CDECL);

    // Old music API (keep for backwards compatability)
    context->RegisterGlobalFunction("void PlaySong(const string& in)", asFUNCTION(ASPlaySong), asCALL_CDECL);
    context->RegisterGlobalFunction("void SetSong(const string& in)", asFUNCTION(ASSetSong), asCALL_CDECL);
    context->RegisterGlobalFunction("string GetSong()", asFUNCTION(ASGetSong), asCALL_CDECL);

    context->RegisterGlobalFunction("void SetSegment(const string& in)", asFUNCTION(ASSetSegment), asCALL_CDECL);
    context->RegisterGlobalFunction("void QueueSegment(const string& in)", asFUNCTION(ASQueueSegment), asCALL_CDECL);
    context->RegisterGlobalFunction("void PlaySegment(const string& in)", asFUNCTION(ASPlaySegment), asCALL_CDECL);
    context->RegisterGlobalFunction("string GetSegment()", asFUNCTION(ASGetSegment), asCALL_CDECL);

    context->RegisterGlobalFunction("array<string>@ GetLayerNames()", asFUNCTION(ASGetLayerNames), asCALL_CDECL);
    context->RegisterGlobalFunction("void SetLayerGain(const string &in layer, float gain)", asFUNCTION(ASSetLayerGain), asCALL_CDECL);
    context->RegisterGlobalFunction("float GetLayerGain(const string &in layer)", asFUNCTION(ASGetLayerGain), asCALL_CDECL);

    context->RegisterGlobalFunction("void ReloadMods()", asFUNCTION(ASReloadMods), asCALL_CDECL);
}

unsigned ASMakeParticle(std::string path, vec3 pos, vec3 vel) {
    return the_scenegraph->particle_system->MakeParticle(the_scenegraph, path, pos, vel, vec3(1.0f));
}

unsigned ASMakeParticleColor(std::string path, vec3 pos, vec3 vel, vec3 color) {
    return the_scenegraph->particle_system->MakeParticle(the_scenegraph, path, pos, vel, color);
}

void ASConnectParticles(unsigned a, unsigned b) {
    the_scenegraph->particle_system->ConnectParticles(a, b);
}

void ASTintParticle(unsigned id, const vec3& color) {
    the_scenegraph->particle_system->TintParticle(id, color);
}

void AttachParticles(ASContext* context) {
    context->RegisterGlobalFunction("void ConnectParticles(uint32 id_a, uint32 id_b)", asFUNCTION(ASConnectParticles), asCALL_CDECL, "Used for ribbon particles, like throat-cut blood");
    context->RegisterGlobalFunction("uint32 MakeParticle(string path, vec3 pos, vec3 vel)", asFUNCTION(ASMakeParticle), asCALL_CDECL);
    context->RegisterGlobalFunction("uint32 MakeParticle(string path, vec3 pos, vec3 vel, vec3 color)", asFUNCTION(ASMakeParticleColor), asCALL_CDECL);
    context->RegisterGlobalFunction("void TintParticle(uint32 id, const vec3 &in color)", asFUNCTION(ASTintParticle), asCALL_CDECL);
}

static PrecisionStopwatch stopwatch;

void ASStartStopwatch() {
    stopwatch.Start();
}

void ASStopAndReportStopwatch() {
    stopwatch.StopAndReportNanoseconds();
}

void AttachStopwatch(ASContext* context) {
    context->RegisterGlobalFunction("void StartStopwatch()", asFUNCTION(ASStartStopwatch), asCALL_CDECL);
    context->RegisterGlobalFunction("uint64 StopAndReportStopwatch()", asFUNCTION(ASStopAndReportStopwatch), asCALL_CDECL);
}

void ASEnterTelemetryZone(const std::string& str) {
    PROFILER_ENTER_DYNAMIC_STRING(g_profiler_ctx, str.c_str());
}

void ASLeaveTelemetryZone() {
    PROFILER_LEAVE(g_profiler_ctx);
}

/*void ASEnterTelemetryZoneProfiler( const std::string& str ) {
    LOGI << str << std::endl;
}

void ASLeaveTelemetryZoneProfiler() {
    LOGI << "Leave" << std::endl;
}*/

void AttachProfiler(ASContext* context) {
    context->RegisterGlobalFunctionThis("void EnterTelemetryZone(const string& in name)", asMETHOD(ASProfiler, ASEnterTelemetryZone), asCALL_THISCALL_ASGLOBAL, &context->profiler);
    context->RegisterGlobalFunctionThis("void LeaveTelemetryZone()", asMETHOD(ASProfiler, ASLeaveTelemetryZone), asCALL_THISCALL_ASGLOBAL, &context->profiler);
}

void AttachTelemetry(ASContext* context) {
    context->RegisterGlobalFunction("void EnterTelemetryZone(const string& in name)", asFUNCTION(ASEnterTelemetryZone), asCALL_CDECL);
    context->RegisterGlobalFunction("void LeaveTelemetryZone()", asFUNCTION(ASLeaveTelemetryZone), asCALL_CDECL);
}

#include "Graphics/pxdebugdraw.h"

const int _delete_on_draw_val = _delete_on_draw;
const int _delete_on_update_val = _delete_on_update;
const int _persistent_val = _persistent;
const int _fade_val = _fade;

int ASDebugDrawLine(vec3 start, vec3 end, vec3 color, int lifespan_int) {
    DDLifespan lifespan = LifespanFromInt(lifespan_int);
    return DebugDraw::Instance()->AddLine(start, end, color, lifespan);
}

int ASDebugDrawBillboard(const std::string& texture_path, vec3 center, float scale, vec4 color, int lifespan_int) {
    DDLifespan lifespan = LifespanFromInt(lifespan_int);
    TextureAssetRef tex_ref = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(texture_path);
    return DebugDraw::Instance()->AddBillboard(tex_ref->GetTextureRef(), center, scale, color, kStraightBlend, lifespan);
}

int ASDebugDrawLine2(vec3 start, vec3 end, vec3 color, vec3 color2, int lifespan_int) {
    DDLifespan lifespan = LifespanFromInt(lifespan_int);
    return DebugDraw::Instance()->AddLine(start, end, color, color2, lifespan);
}

int ASDebugDrawLine3(vec3 start, vec3 end, vec4 color, vec4 color2, int lifespan_int) {
    DDLifespan lifespan = LifespanFromInt(lifespan_int);
    return DebugDraw::Instance()->AddLine(start, end, color, color2, lifespan);
}

int ASDebugDrawRibbon(vec3 start, vec3 end, vec4 color, vec4 color2, float start_width, float end_width, int lifespan_int) {
    DDLifespan lifespan = LifespanFromInt(lifespan_int);
    return DebugDraw::Instance()->AddRibbon(start, end, color, color2, start_width, end_width, lifespan);
}

int ASDebugDrawRibbon2(int lifespan_int) {
    DDLifespan lifespan = LifespanFromInt(lifespan_int);
    return DebugDraw::Instance()->AddRibbon(lifespan);
}

void ASAddDebugDrawRibbonPoint(int which, vec3 pos, vec4 color, float width) {
    DebugDrawElement* element = DebugDraw::Instance()->GetElement(which);
    DebugDrawRibbon* ribbon = (DebugDrawRibbon*)element;
    ribbon->AddPoint(pos, color, width);
}

int ASDebugDrawLines(const CScriptArray& array, vec4 color, int lifespan_int) {
    std::vector<vec3> data;

    data.reserve((int)array.GetSize());
    for (int n = 0; n < (int)array.GetSize(); n++) {
        data.push_back(*((vec3*)array.At(n)));
    }

    DDLifespan lifespan = LifespanFromInt(lifespan_int);
    return DebugDraw::Instance()->AddLines(data, color, lifespan, _DD_NO_FLAG);
}

int ASDebugDrawText(vec3 pos, std::string text, float scale, bool screen_space, int lifespan_int) {
    DDLifespan lifespan = LifespanFromInt(lifespan_int);
    return DebugDraw::Instance()->AddText(pos, text, scale, lifespan, screen_space ? _DD_SCREEN_SPACE : _DD_NO_FLAG);
}

bool ASDebugSetPosition(int id, vec3 pos) {
    return DebugDraw::Instance()->SetPosition(id, pos);
}

int ASDebugDrawSphere(vec3 pos, float radius, vec3 color, int lifespan_int) {
    DDLifespan lifespan = LifespanFromInt(lifespan_int);
    return DebugDraw::Instance()->AddWireSphere(pos, radius, color, lifespan);
}

int ASDebugDrawWireMesh(std::string path, mat4 transform, vec4 color, int lifespan_int) {
    DDLifespan lifespan = LifespanFromInt(lifespan_int);
    return DebugDraw::Instance()->AddWireMesh(path, transform, color, lifespan);
}

int ASDebugDrawScaledSphere(vec3 pos, float radius, vec3 scale, vec3 color, int lifespan_int) {
    DDLifespan lifespan = LifespanFromInt(lifespan_int);
    return DebugDraw::Instance()->AddWireScaledSphere(pos, radius, scale, color, lifespan);
}

int ASDebugDrawScaledSphere2(vec3 pos, float radius, vec3 scale, vec4 color, int lifespan_int) {
    DDLifespan lifespan = LifespanFromInt(lifespan_int);
    return DebugDraw::Instance()->AddWireScaledSphere(pos, radius, scale, color, lifespan);
}

int ASDebugDrawCylinder(vec3 pos,
                        float radius,
                        float height,
                        vec3 color,
                        int lifespan_int) {
    DDLifespan lifespan = LifespanFromInt(lifespan_int);
    DebugDraw* dd = DebugDraw::Instance();
    return dd->AddWireCylinder(pos, radius, height, color, lifespan);
}

int ASDebugDrawBox(vec3 pos, vec3 dimensions, vec3 color, int lifespan_int) {
    DDLifespan lifespan = LifespanFromInt(lifespan_int);
    return DebugDraw::Instance()->AddWireBox(pos, dimensions, color, lifespan);
}

int ASDebugDrawCircle(mat4 transform, vec4 color, int lifespan_int) {
    DDLifespan lifespan = LifespanFromInt(lifespan_int);
    return DebugDraw::Instance()->AddCircle(transform, color, lifespan);
}

void ASDebugDrawClear(int which) {
    DebugDraw::Instance()->Remove(which);
}

void ASDebugText(std::string key, std::string text, float time) {
    GetActiveASContext()->gui->AddDebugText(key, text, time);
}

int ASDebugDrawPoint(vec3 pos, vec4 color, int lifespan_int) {
    DDLifespan lifespan = LifespanFromInt(lifespan_int);
    return DebugDraw::Instance()->AddPoint(pos, color, lifespan, _DD_NO_FLAG);
}

void AttachDebugDraw(ASContext* context) {
    context->RegisterGlobalProperty("const int _delete_on_update",
                                    (void*)&_delete_on_update_val);
    context->RegisterGlobalProperty("const int _fade",
                                    (void*)&_fade_val);
    context->RegisterGlobalProperty("const int _delete_on_draw",
                                    (void*)&_delete_on_draw_val);
    context->RegisterGlobalProperty("const int _persistent",
                                    (void*)&_persistent_val);
    context->RegisterGlobalFunction("int DebugDrawLine(vec3 start, vec3 end, vec3 color, int lifespan)",
                                    asFUNCTION(ASDebugDrawLine), asCALL_CDECL);
    context->RegisterGlobalFunction("int DebugDrawBillboard(const string &in path, vec3 center, float scale, vec4 color, int lifespan)",
                                    asFUNCTION(ASDebugDrawBillboard), asCALL_CDECL);
    context->RegisterGlobalFunction("int DebugDrawLine(vec3 start, vec3 end, vec3 start_color, vec3 end_color, int lifespan)",
                                    asFUNCTION(ASDebugDrawLine2), asCALL_CDECL);
    context->RegisterGlobalFunction("int DebugDrawLine(vec3 start, vec3 end, vec4 start_color, vec4 end_color, int lifespan)",
                                    asFUNCTION(ASDebugDrawLine3), asCALL_CDECL);
    context->RegisterGlobalFunction("int DebugDrawRibbon(vec3 start, vec3 end, vec4 start_color, vec4 end_color, float start_width, float end_width, int lifespan)",
                                    asFUNCTION(ASDebugDrawRibbon), asCALL_CDECL);
    context->RegisterGlobalFunction("int DebugDrawRibbon(int lifespan)",
                                    asFUNCTION(ASDebugDrawRibbon2), asCALL_CDECL);
    context->RegisterGlobalFunction("void AddDebugDrawRibbonPoint(int which, vec3 pos, vec4 color, float width)",
                                    asFUNCTION(ASAddDebugDrawRibbonPoint), asCALL_CDECL);
    context->RegisterGlobalFunction("int DebugDrawLines(const array<vec3> &vertices, vec4 color, int lifespan)",
                                    asFUNCTION(ASDebugDrawLines), asCALL_CDECL);
    context->RegisterGlobalFunction("int DebugDrawText(vec3 pos, string text, float scale, bool screen_space, int lifespan)",
                                    asFUNCTION(ASDebugDrawText), asCALL_CDECL);
    context->RegisterGlobalFunction("int DebugSetPosition(int id, vec3 pos)",
                                    asFUNCTION(ASDebugSetPosition), asCALL_CDECL);
    context->RegisterGlobalFunction("int DebugDrawWireSphere(vec3 pos, float radius, vec3 color, int lifespan)",
                                    asFUNCTION(ASDebugDrawSphere), asCALL_CDECL);
    context->RegisterGlobalFunction("int DebugDrawWireMesh(string path, mat4 transform, vec4 color, int lifespan)",
                                    asFUNCTION(ASDebugDrawWireMesh), asCALL_CDECL);
    context->RegisterGlobalFunction("int DebugDrawWireScaledSphere(vec3 pos, float radius, vec3 scale, vec3 color, int lifespan)",
                                    asFUNCTION(ASDebugDrawScaledSphere), asCALL_CDECL);
    context->RegisterGlobalFunction("int DebugDrawWireScaledSphere(vec3 pos, float radius, vec3 scale, vec4 color, int lifespan)",
                                    asFUNCTION(ASDebugDrawScaledSphere2), asCALL_CDECL);
    context->RegisterGlobalFunction("int DebugDrawWireCylinder(vec3 pos, float radius, float height, vec3 color, int lifespan)",
                                    asFUNCTION(ASDebugDrawCylinder), asCALL_CDECL);
    context->RegisterGlobalFunction("int DebugDrawWireBox(vec3 pos, vec3 dimensions, vec3 color, int lifespan)",
                                    asFUNCTION(ASDebugDrawBox), asCALL_CDECL);
    context->RegisterGlobalFunction("int DebugDrawCircle(mat4 transform, vec4 color, int lifespan)",
                                    asFUNCTION(ASDebugDrawCircle), asCALL_CDECL);
    context->RegisterGlobalFunction("void DebugDrawRemove(int id)",
                                    asFUNCTION(ASDebugDrawClear), asCALL_CDECL);
    context->RegisterGlobalFunction("void DebugText(string key, string display_text, float lifetime)",
                                    asFUNCTION(ASDebugText), asCALL_CDECL);
    context->RegisterGlobalFunction("string FloatString(float val, int digits)",
                                    asFUNCTION(FloatString), asCALL_CDECL);
    context->RegisterGlobalFunction("int DebugDrawPoint(vec3 pos, vec4 color, int lifespan)",
                                    asFUNCTION(ASDebugDrawPoint), asCALL_CDECL);
}

static bool ObjectExists(int id) {
    bool result = false;

    if (id >= 0) {
        result = the_scenegraph->DoesObjectWithIdExist(id);
    }

    return result;
}

Object* ReadObjectFromID(int id) {
    Object* obj = the_scenegraph->GetObjectFromID(id);
    if (!obj) {
        std::string callstack = active_context_stack.top()->GetCallstack();
        std::ostringstream oss;
        oss << "There is no object " << id << "\n Called from:\n"
            << callstack;
        FatalError("Error", "There is no object %d\n Called from:\n%s", id, callstack.c_str());
    }
    return obj;
}

void ASSetTranslation(Object* obj, const vec3& vec) {
    switch (obj->GetType()) {
        case _env_object:
            ((EnvObject*)obj)->SetTranslation(vec);
            ((EnvObject*)obj)->UpdatePhysicsTransform();
            break;
        case _group:
            ((Group*)obj)->SetTranslation(vec);
            break;
        case _prefab:
            ((Prefab*)obj)->SetTranslation(vec);
            break;
        case _decal_object:
            ((DecalObject*)obj)->SetTranslation(vec);
            break;
        case _movement_object:
            ((MovementObject*)obj)->SetTranslation(vec);
            break;
        default:
            obj->SetTranslation(vec);
            break;
    }
}

bool ASIsSelected(Object* obj) {
    if (obj) {
        return obj->Selected();
    } else {
        return false;
    }
}

void ASSetSelected(Object* obj, bool val) {
    if (val && obj->permission_flags & Object::CAN_SELECT) {
        obj->Select(true);
    } else {
        obj->Select(false);
    }
}

void ASSetEnabled(Object* obj, bool val) {
    obj->SetEnabled(val);
}

void ASSetCollisionEnabled(Object* obj, bool val) {
    obj->SetCollisionEnabled(val);
}

bool ASGetEnabled(Object* obj) {
    return obj->enabled_;
}

void ASDeselectAll() {
    MapEditor::DeselectAll(the_scenegraph);
}

CScriptArray* ASGetSelected() {
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<int>"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);
    array->Reserve(the_scenegraph->objects_.size());

    const SceneGraph::object_list& objects = the_scenegraph->objects_;
    for (auto obj : objects) {
        if (obj->Selected()) {
            int val = obj->GetID();
            array->InsertLast(&val);
        }
    }
    return array;
}

vec3 ASGetScale(Object* obj) {
    return obj->GetScale();
}

void ASSetScale(Object* obj, const vec3& vec) {
    switch (obj->GetType()) {
        case _env_object:
            ((EnvObject*)obj)->SetScale(vec);
            ((EnvObject*)obj)->UpdatePhysicsTransform();
            break;
        case _decal_object:
            ((DecalObject*)obj)->SetScale(vec);
            break;
        case _movement_object:
            ((MovementObject*)obj)->SetScale(vec);
            break;
        default:
            obj->SetScale(vec);
            break;
    }
}

vec3 ASGetTranslation(Object* obj) {
    return obj->GetTranslation();
}

void ASSetRotation(Object* obj, const quaternion& quat) {
    switch (obj->GetType()) {
        case _env_object:
            ((EnvObject*)obj)->SetRotation(quat);
            ((EnvObject*)obj)->UpdatePhysicsTransform();
            break;
        case _decal_object:
            ((DecalObject*)obj)->SetRotation(quat);
            break;
        case _movement_object:
            ((MovementObject*)obj)->SetRotation(quat);
            break;
        default:
            obj->SetRotation(quat);
            break;
    }
}

void ASSetTranslationRotationFast(Object* obj, const vec3& vec, const quaternion& quat) {
    switch (obj->GetType()) {
        case _env_object:
            ((EnvObject*)obj)->SetTranslationRotationFast(vec, quat);
            break;
        case _group:
            ((Group*)obj)->SetTranslationRotationFast(vec, quat);
            break;
        default:
            obj->SetTranslationRotationFast(vec, quat);
            break;
    }
}

quaternion ASGetRotation(Object* obj) {
    return obj->GetRotation();
}

vec4 ASGetRotationVec4(Object* obj) {
    quaternion q = obj->GetRotation();
    float* e = q.entries;
    return vec4(e[0], e[1], e[2], e[3]);
}

EntityType ASGetType(Object* obj) {
    return obj->GetType();
}

CScriptArray* ASGetObjectIDArrayType(int type) {
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<int>"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);
    array->Reserve(the_scenegraph->objects_.size());

    const SceneGraph::object_list& objects = the_scenegraph->objects_;
    for (auto obj : objects) {
        if (obj->GetType() == type) {
            int val = obj->GetID();
            if (val != -1) {
                array->InsertLast(&val);
            }
        }
    }
    return array;
}

CScriptArray* ASGetObjectIDArray() {
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<int>"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);
    array->Reserve(the_scenegraph->objects_.size());

    const SceneGraph::object_list& objects = the_scenegraph->objects_;
    for (auto object : objects) {
        int val = object->GetID();
        if (val != -1) {
            array->InsertLast(&val);
        }
    }
    return array;
}

void ASDeleteObjectID(int val) {
    LOGD << "Deleting id: " << val << std::endl;
    the_scenegraph->map_editor->DeleteID(val);
}

void ASQueueDeleteObjectID(int val) {
    the_scenegraph->object_ids_to_delete.push_back(val);
}

int ASCreateObject(const std::string& path, bool exclude_from_save) {
    int id = -1;
    EntityDescriptionList desc_list;
    std::string file_type;
    Path source;
    ActorsEditor_LoadEntitiesFromFile(path, desc_list, &file_type, &source);
    for (auto& i : desc_list) {
        Object* obj = MapEditor::AddEntityFromDesc(the_scenegraph, i, false);
        if (obj) {
            obj->exclude_from_undo = exclude_from_save;
            obj->exclude_from_save = exclude_from_save;
            obj->permission_flags = 0;
            id = obj->GetID();
        } else {
            LOGE << "Failed at creating object from: " << path << std::endl;
        }
    }
    return id;
}

int ASCreateObject2(const std::string& path) {
    return ASCreateObject(path, true);
}

int ASDuplicateObject(const Object* obj) {
    return the_scenegraph->map_editor->DuplicateObject(obj);
}

void ASSetPlayer(Object* obj, bool val) {
    if (obj->GetType() == _movement_object) {
        ((MovementObject*)obj)->is_player = val;
    }
}

bool ASGetPlayer(Object* obj) {
    if (obj->GetType() == _movement_object) {
        return static_cast<MovementObject*>(obj)->is_player;
    }
    return false;
}

int ASGetNumPaletteColors(Object* obj) {
    OGPalette* palette = obj->GetPalette();
    if (!palette) {
        return 0;
    } else {
        return palette->size();
    }
}

vec3 GetPaletteColor(Object* obj, int which) {
    OGPalette* palette = obj->GetPalette();
    return palette->at(which).color;
}

void SetPaletteColor(Object* obj, int which, const vec3& color) {
    OGPalette* palette = obj->GetPalette();
    if (palette && (int)palette->size() > which) {
        palette->at(which).color = color;
        obj->ApplyPalette(*palette);
    }
}

vec3 GetTint(Object* obj) {
    switch (obj->GetType()) {
        case _env_object:
            return ((EnvObject*)obj)->GetColorTint();
            break;
        case _decal_object:
            return ((DecalObject*)obj)->color_tint_component_.tint_;
            break;
        case _dynamic_light_object:
            return ((DynamicLightObject*)obj)->GetTint();
            break;
        default:
            return vec3(0.0f);
            break;
    }
}

void SetTint(Object* obj, const vec3& color) {
    obj->ReceiveObjectMessage(OBJECT_MSG::SET_COLOR, &color);
}

bool ConnectTo(Object* obj, Object* other) {
    if (obj && other)
        return obj->ConnectTo(*other);

    return false;
}

bool Disconnect(Object* obj, Object* other) {
    if (obj && other)
        return obj->Disconnect(*other);

    return false;
}

void AttachItem(Object* movement_object_base, Object* item_object_base, int type, bool mirrored) {
    ItemObject* item_object = (ItemObject*)item_object_base;
    MovementObject* movement_object = (MovementObject*)movement_object_base;
    AttachmentSlotList attachment_slots;
    movement_object->rigged_object()->AvailableItemSlots(item_object->item_ref(), &attachment_slots);
    for (auto& slot : attachment_slots) {
        if (slot.mirrored == mirrored && slot.type == type) {
            movement_object->AttachItemToSlotEditor(item_object->GetID(), slot.type, slot.mirrored, slot.attachment_ref);
            break;
        }
    }
}

ScriptParams* GetScriptParams(Object* obj) {
    return obj->GetScriptParams();
}

std::string GetLabel(Object* obj) {
    switch (obj->GetType()) {
        case _env_object:
            return ((EnvObject*)obj)->GetLabel();
            break;
        default:
            return "";
            break;
    }
}

void ASUpdateScriptParams(Object* obj) {
    obj->UpdateScriptParams();
}

void SetBit(int* flags, int bit, bool val) {
    if (val) {
        *flags |= bit;
    } else {
        *flags &= ~bit;
    }
}

void ASSetCopyable(Object* obj, bool val) {
    SetBit(&obj->permission_flags, Object::CAN_COPY, val);
}

void ASSetSelectable(Object* obj, bool val) {
    SetBit(&obj->permission_flags, Object::CAN_SELECT, val);
}

void ASSetDeletable(Object* obj, bool val) {
    SetBit(&obj->permission_flags, Object::CAN_DELETE, val);
}

void ASSetRotatable(Object* obj, bool val) {
    SetBit(&obj->permission_flags, Object::CAN_ROTATE, val);
}

void ASSetTranslatable(Object* obj, bool val) {
    SetBit(&obj->permission_flags, Object::CAN_TRANSLATE, val);
}

void ASSetScalable(Object* obj, bool val) {
    SetBit(&obj->permission_flags, Object::CAN_SCALE, val);
}

void ASSetEditorLabel(Object* obj, std::string value) {
    obj->editor_label = value;
}

std::string ASGetEditorLabel(Object* obj) {
    return obj->editor_label;
}

void ASSetEditorLabelOffset(Object* obj, vec3 offset) {
    obj->editor_label_offset = offset;
}

vec3 ASGetEditorLabelOffset(Object* obj) {
    return obj->editor_label_offset;
}

void ASSetEditorLabelScale(Object* obj, float scale) {
    obj->editor_label_scale = scale;
}

float ASGetEditorLabelScale(Object* obj) {
    return obj->editor_label_scale;
}

template <class A, class B>
B* refCast(A* a) {
    if (!a) return 0;
    return dynamic_cast<B*>(a);
}

mat4 ASGetTransform(Object* object) {
    return object->GetTransform();
}

static vec3 ASGetBoundingBox(Object* object) {
    if (object->GetType() == _env_object) {
        return static_cast<EnvObject*>(object)->GetBoundingBoxSize();
    } else {
        return vec3(0.0f);
    }
}

static bool ASIsExcludedFromSave(Object* obj) {
    if (obj) {
        return obj->exclude_from_save;
    } else {
        LOGE << "Got NULL" << std::endl;
        return true;
    }
}

static bool ASIsExcludedFromUndo(Object* obj) {
    if (obj) {
        return obj->exclude_from_undo;
    } else {
        LOGE << "Got NULL" << std::endl;
        return true;
    }
}

static int ASGetParent(Object* obj) {
    return obj->parent ? obj->parent->GetID() : -1;
}

CScriptArray* ASGetChildren(Object* obj) {
    if (obj->GetType() != _group) {
        return NULL;
    }

    Group* group = static_cast<Group*>(obj);

    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<int>"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);
    array->Reserve(group->children.size());

    for (const auto& iter : group->children) {
        int val = iter.direct_ptr->GetID();
        if (val != -1) {
            array->InsertLast(&val);
        }
    }
    return array;
}

static int ASObjectGetID(Object* obj) {
    return obj->GetID();
}

static std::string ASObjectGetName(Object* obj) {
    return obj->GetName();
}

static void ASObjectSetName(Object* obj, const std::string& string) {
    obj->SetName(string);
}

void AttachObject(ASContext* context) {
    context->RegisterEnum("EntityType");
    context->RegisterEnumValue("EntityType", "_any_type", _any_type);
    context->RegisterEnumValue("EntityType", "_no_type", _no_type);
    context->RegisterEnumValue("EntityType", "_camera_type", _camera_type);
    context->RegisterEnumValue("EntityType", "_terrain_type", _terrain_type);
    context->RegisterEnumValue("EntityType", "_env_object", _env_object);
    context->RegisterEnumValue("EntityType", "_movement_object", _movement_object);
    context->RegisterEnumValue("EntityType", "_spawn_point", _spawn_point);
    context->RegisterEnumValue("EntityType", "_decal_object", _decal_object);
    context->RegisterEnumValue("EntityType", "_hotspot_object", _hotspot_object);
    context->RegisterEnumValue("EntityType", "_group", _group);
    context->RegisterEnumValue("EntityType", "_rigged_object", _rigged_object);
    context->RegisterEnumValue("EntityType", "_item_object", _item_object);
    context->RegisterEnumValue("EntityType", "_path_point_object", _path_point_object);
    context->RegisterEnumValue("EntityType", "_ambient_sound_object", _ambient_sound_object);
    context->RegisterEnumValue("EntityType", "_placeholder_object", _placeholder_object);
    context->RegisterEnumValue("EntityType", "_light_probe_object", _light_probe_object);
    context->RegisterEnumValue("EntityType", "_dynamic_light_object", _dynamic_light_object);
    context->RegisterEnumValue("EntityType", "_navmesh_hint_object", _navmesh_hint_object);
    context->RegisterEnumValue("EntityType", "_navmesh_region_object", _navmesh_region_object);
    context->RegisterEnumValue("EntityType", "_navmesh_connection_object", _navmesh_connection_object);
    context->RegisterEnumValue("EntityType", "_reflection_capture_object", _reflection_capture_object);
    context->RegisterEnumValue("EntityType", "_light_volume_object", _light_volume_object);
    context->RegisterEnumValue("EntityType", "_prefab", _prefab);

    context->DocsCloseBrace();
    context->RegisterEnum("AttachmentType");
    context->RegisterEnumValue("AttachmentType", "_at_attachment", _at_attachment);
    context->RegisterEnumValue("AttachmentType", "_at_grip", _at_grip);
    context->RegisterEnumValue("AttachmentType", "_at_sheathe", _at_sheathe);
    context->RegisterEnumValue("AttachmentType", "_at_unspecified", _at_unspecified);
    context->DocsCloseBrace();
    context->RegisterEnum("ConnectionType");
    context->RegisterEnumValue("ConnectionType", "kCTNone", Object::kCTNone);
    context->RegisterEnumValue("ConnectionType", "kCTMovementObjects", Object::kCTMovementObjects);
    context->RegisterEnumValue("ConnectionType", "kCTItemObjects", Object::kCTItemObjects);
    context->RegisterEnumValue("ConnectionType", "kCTEnvObjectsAndGroups", Object::kCTEnvObjectsAndGroups);
    context->RegisterEnumValue("ConnectionType", "kCTPathPoints", Object::kCTPathPoints);
    context->RegisterEnumValue("ConnectionType", "kCTPlaceholderObjects", Object::kCTPlaceholderObjects);
    context->RegisterEnumValue("ConnectionType", "kCTNavmeshConnections", Object::kCTNavmeshConnections);
    context->RegisterEnumValue("ConnectionType", "kCTHotspots", Object::kCTHotspots);
    context->DocsCloseBrace();
    context->RegisterObjectMethod("Object", "int GetID()", asFUNCTION(ASObjectGetID), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "string GetName()", asFUNCTION(ASObjectGetName), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "void SetName(const string &in)", asFUNCTION(ASObjectSetName), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "bool IsSelected()", asFUNCTION(ASIsSelected), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "void SetSelected(bool)", asFUNCTION(ASSetSelected), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "void SetEnabled(bool)", asFUNCTION(ASSetEnabled), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "void SetCollisionEnabled(bool)", asFUNCTION(ASSetCollisionEnabled), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "bool GetEnabled()", asFUNCTION(ASGetEnabled), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "void SetTranslation(const vec3 &in)", asFUNCTION(ASSetTranslation), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "vec3 GetTranslation()", asFUNCTION(ASGetTranslation), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "void SetScale(const vec3 &in)", asFUNCTION(ASSetScale), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "mat4 GetTransform()", asFUNCTION(ASGetTransform), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "vec3 GetScale()", asFUNCTION(ASGetScale), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "void SetRotation(const quaternion &in)", asFUNCTION(ASSetRotation), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "void SetTranslationRotationFast(const vec3 &in, const quaternion &in)", asFUNCTION(ASSetTranslationRotationFast), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "quaternion GetRotation()", asFUNCTION(ASGetRotation), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "vec4 GetRotationVec4()", asFUNCTION(ASGetRotationVec4), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "EntityType GetType()", asFUNCTION(ASGetType), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "void SetPlayer(bool)", asFUNCTION(ASSetPlayer), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "bool GetPlayer()", asFUNCTION(ASGetPlayer), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "int GetNumPaletteColors()", asFUNCTION(ASGetNumPaletteColors), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "vec3 GetPaletteColor(int which)", asFUNCTION(GetPaletteColor), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "void SetPaletteColor(int which, const vec3 &in color)", asFUNCTION(SetPaletteColor), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "vec3 GetTint()", asFUNCTION(GetTint), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "void SetTint(const vec3 &in)", asFUNCTION(SetTint), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "bool ConnectTo(Object@)", asFUNCTION(ConnectTo), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "bool Disconnect(Object@)", asFUNCTION(Disconnect), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "void AttachItem(Object@, AttachmentType, bool mirrored)", asFUNCTION(AttachItem), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "void ReceiveScriptMessage(const string &in)", asMETHOD(Object, ReceiveScriptMessage), asCALL_THISCALL);
    context->RegisterObjectMethod("Object", "void QueueScriptMessage(const string &in)", asMETHOD(Object, QueueScriptMessage), asCALL_THISCALL);
    context->RegisterObjectMethod("Object", "void UpdateScriptParams()", asFUNCTION(ASUpdateScriptParams), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "void SetCopyable(bool)", asFUNCTION(ASSetCopyable), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "void SetSelectable(bool)", asFUNCTION(ASSetSelectable), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "void SetDeletable(bool)", asFUNCTION(ASSetDeletable), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "void SetScalable(bool)", asFUNCTION(ASSetScalable), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "void SetTranslatable(bool)", asFUNCTION(ASSetTranslatable), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "void SetRotatable(bool)", asFUNCTION(ASSetRotatable), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "void SetEditorLabel(string)", asFUNCTION(ASSetEditorLabel), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "string GetEditorLabel()", asFUNCTION(ASGetEditorLabel), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "void SetEditorLabelOffset(vec3)", asFUNCTION(ASSetEditorLabelOffset), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "vec3 GetEditorLabelOffset()", asFUNCTION(ASGetEditorLabelOffset), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "void SetEditorLabelScale(float)", asFUNCTION(ASSetEditorLabelScale), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "float GetEditorLabelScale()", asFUNCTION(ASGetEditorLabelScale), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "ScriptParams@ GetScriptParams()", asFUNCTION(GetScriptParams), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "string GetLabel()", asFUNCTION(GetLabel), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "vec3 GetBoundingBox()", asFUNCTION(ASGetBoundingBox), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "bool IsExcludedFromSave()", asFUNCTION(ASIsExcludedFromSave), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "bool IsExcludedFromUndo()", asFUNCTION(ASIsExcludedFromUndo), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "int GetParent()", asFUNCTION(ASGetParent), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "array<int>@ GetChildren()", asFUNCTION(ASGetChildren), asCALL_CDECL_OBJFIRST);

    context->DocsCloseBrace();
    context->RegisterGlobalFunction("Object@ ReadObjectFromID(int)", asFUNCTION(ReadObjectFromID), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ObjectExists()", asFUNCTION(ObjectExists), asCALL_CDECL);
    context->RegisterGlobalFunction("array<int> @GetObjectIDs()", asFUNCTION(ASGetObjectIDArray), asCALL_CDECL);
    context->RegisterGlobalFunction("array<int> @GetObjectIDsType(int type)", asFUNCTION(ASGetObjectIDArrayType), asCALL_CDECL);
    context->RegisterGlobalFunction("void DeleteObjectID(int)", asFUNCTION(ASDeleteObjectID), asCALL_CDECL);
    context->RegisterGlobalFunction("void QueueDeleteObjectID(int)", asFUNCTION(ASQueueDeleteObjectID), asCALL_CDECL);
    context->RegisterGlobalFunction("int CreateObject(const string &in path, bool exclude_from_save)", asFUNCTION(ASCreateObject), asCALL_CDECL);
    context->RegisterGlobalFunction("int CreateObject(const string &in path)", asFUNCTION(ASCreateObject2), asCALL_CDECL);
    context->RegisterGlobalFunction("int DuplicateObject(Object@ obj)", asFUNCTION(ASDuplicateObject), asCALL_CDECL);
    context->RegisterGlobalFunction("void DeselectAll()", asFUNCTION(ASDeselectAll), asCALL_CDECL);
    context->RegisterGlobalFunction("array<int> @GetSelected()", asFUNCTION(ASGetSelected), asCALL_CDECL);

    PathPointObject::RegisterToScript(context);
    context->RegisterObjectMethod("Object", "PathPointObject@ opCast()", asFUNCTION((refCast<Object, PathPointObject>)), asCALL_CDECL_OBJLAST);
    context->RegisterObjectMethod("Object", "Hotspot@ opCast()", asFUNCTION((refCast<Object, Hotspot>)), asCALL_CDECL_OBJLAST);
}

static void ASSetEditorDisplayName(PlaceholderObject* obj, const std::string& name) {
    obj->editor_display_name = name;
}

static void ASSetUnsavedChanges(PlaceholderObject* obj, bool changes) {
    obj->unsaved_changes = changes;
}

static bool ASGetUnsavedChanges(PlaceholderObject* obj) {
    return obj->unsaved_changes;
}

void AttachPlaceholderObject(ASContext* context) {
    context->RegisterEnum("PlaceholderObjectType");
    context->RegisterEnumValue("PlaceholderObjectType", "kCamPreview", PlaceholderObject::kCamPreview);
    context->RegisterEnumValue("PlaceholderObjectType", "kPlayerConnect", PlaceholderObject::kPlayerConnect);
    context->RegisterEnumValue("PlaceholderObjectType", "kSpawn", PlaceholderObject::kSpawn);
    context->RegisterObjectType("PlaceholderObject", 0, asOBJ_REF | asOBJ_NOCOUNT);
    context->RegisterObjectMethod("PlaceholderObject", "void SetPreview(const string &in path)", asMETHOD(PlaceholderObject, SetPreview), asCALL_THISCALL);
    context->RegisterObjectMethod("PlaceholderObject", "void SetBillboard(const string &in path)", asMETHOD(PlaceholderObject, SetBillboard), asCALL_THISCALL);
    context->RegisterObjectMethod("PlaceholderObject", "void SetSpecialType(int)", asMETHOD(PlaceholderObject, SetSpecialType), asCALL_THISCALL);
    context->RegisterObjectMethod("PlaceholderObject", "void SetEditorDisplayName(const string &in name)", asFUNCTION(ASSetEditorDisplayName), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("PlaceholderObject", "int GetConnectID()", asMETHOD(PlaceholderObject, GetConnectID), asCALL_THISCALL);
    context->RegisterObjectMethod("PlaceholderObject", "uint64 GetConnectToTypeFilterFlags()", asMETHOD(PlaceholderObject, GetConnectToTypeFilterFlags), asCALL_THISCALL, "Returns a bit mask, with EntityType for bit indices");
    context->RegisterObjectMethod("PlaceholderObject", "void SetConnectToTypeFilterFlags(uint64 flags)", asMETHOD(PlaceholderObject, SetConnectToTypeFilterFlags), asCALL_THISCALL, "Set a bit mask, with EntityType for bit indices");
    context->RegisterObjectMethod("PlaceholderObject", "void SetUnsavedChanges(bool changes)", asFUNCTION(ASSetUnsavedChanges), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("PlaceholderObject", "bool GetUnsavedChanges()", asFUNCTION(ASGetUnsavedChanges), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Object", "PlaceholderObject@ opCast()", asFUNCTION((refCast<Object, PlaceholderObject>)), asCALL_CDECL_OBJLAST);
    context->DocsCloseBrace();
}

void ASClearTemporaryDecals() {
    // the_scenegraph->decals->ClearTemporary();
    while (!the_scenegraph->dynamic_decals.empty()) {
        DecalObject* obj = the_scenegraph->dynamic_decals.front();
        obj->SetParent(NULL);
        the_scenegraph->UnlinkObject(obj);
        obj->Dispose();
        delete (obj);
    }
}

void AttachDecals(ASContext* context) {
    context->RegisterGlobalFunction("void ClearTemporaryDecals()",
                                    asFUNCTION(ASClearTemporaryDecals), asCALL_CDECL, "Like blood splats and footprints");
}

void ResetLevel() {
    the_scenegraph->QueueLevelReset();
}

int ASGetNumCharacters() {
    return the_scenegraph->movement_objects_.size();
}

int ASGetNumHotspots() {
    return the_scenegraph->hotspots_.size();
}

struct TokenIterator {
    int64_t token_start;
    int64_t token_end;
    int64_t last_token_end;
    void Init() {
        token_start = 0;
        token_end = 0;
        last_token_end = 0;
    }
    bool FindNextToken(const std::string& str) {
        bool in_quotes = false;
        const char* cstr = str.c_str();
        const char* chr = &cstr[last_token_end];
        if (*chr == '\0') {
            return false;
        }
        while (*chr == ' ') {
            ++chr;
        }
        if (*chr == '\"') {
            in_quotes = true;
            ++chr;
        }
        token_start = (int64_t)(chr - cstr);
        bool escape_character = false;
        while (true) {
            if ((*chr == ' ' && !in_quotes) || (*chr == '\"' && !escape_character) || *chr == '\0') {
                break;
            }
            escape_character = false;
            if (*chr == '\\' && !escape_character) {
                escape_character = true;
            }
            ++chr;
        }
        token_end = (int64_t)(chr - cstr);
        last_token_end = token_end;
        if (in_quotes && *chr == '\"') {
            ++chr;
            ++last_token_end;
        }
        // printf("Tokenized: %s\n", str.substr(token_start, token_end-token_start).c_str());
        return true;
    }
    std::string GetToken(const std::string& str) {
        int64_t len = token_end - token_start;
        std::vector<char> token;
        token.resize(len + 1);
        int64_t index = 0;
        bool escape = false;
        for (int i = 0; i < len; ++i) {
            char c = str[token_start + i];
            if (c == '\\' && !escape) {
                escape = true;
            } else {
                if (escape && c != '"' && c != '\\') {
                    token[index] = '\\';
                    ++index;
                }
                escape = false;
                token[index] = c;
                ++index;
            }
        }
        token[index] = '\0';
        std::string tok = &token[0];
        return tok;
    }
};

void Breakpoint(int val) {
    LOGI << "Breakpoint: " << val << std::endl;
}

void AttachTokenIterator(ASContext* context) {
    context->RegisterObjectType("TokenIterator", sizeof(TokenIterator), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS);
    context->RegisterObjectMethod("TokenIterator", "void Init()", asMETHOD(TokenIterator, Init), asCALL_THISCALL);
    context->RegisterObjectMethod("TokenIterator", "bool FindNextToken(const string& in)", asMETHOD(TokenIterator, FindNextToken), asCALL_THISCALL);
    context->RegisterObjectMethod("TokenIterator", "string GetToken(const string& in)", asMETHOD(TokenIterator, GetToken), asCALL_THISCALL);
    context->DocsCloseBrace();
    context->RegisterGlobalFunction("void Breakpoint(int)", asFUNCTION(Breakpoint), asCALL_CDECL);
}

MovementObject* ASReadCharacter(int which) {
    if (which < 0 || which >= (int)the_scenegraph->movement_objects_.size()) {
        std::string callstack = active_context_stack.top()->GetCallstack();
        FatalError("Error", "There is no movement object #%d\n Called from:\n%s", which, callstack.c_str());
    }
    return (MovementObject*)the_scenegraph->movement_objects_[which];
}

Hotspot* ASReadHotspot(int which) {
    if (which < 0 || which >= (int)the_scenegraph->hotspots_.size()) {
        std::string callstack = active_context_stack.top()->GetCallstack();
        FatalError("Error", "There is no hotspot %d\n Called from:\n%s", which, callstack.c_str());
    }
    return (Hotspot*)the_scenegraph->hotspots_[which];
}

MovementObject* ASReadCharacterID(int which) {
    if (which == -1) {
        std::string callstack = active_context_stack.top()->GetCallstack();
        LOGW << "Called ReadCharacter ID with parameter -1 from\n " << callstack.c_str() << std::endl;
        return NULL;
    }
    Object* obj = the_scenegraph->GetObjectFromID(which);
    if (!obj || obj->GetType() != _movement_object) {
        std::string callstack = active_context_stack.top()->GetCallstack();
        LOGE.Format("There is no movement object with id %d\n Called from:\n%s\n", which, callstack.c_str());
        return NULL;
    }
    return (MovementObject*)obj;
}

int ASGetNumItems() {
    return the_scenegraph->item_objects_.size();
}

ItemObject* ASReadItem(int which) {
    if (which < 0 || which >= (int)the_scenegraph->item_objects_.size()) {
        std::string callstack = active_context_stack.top()->GetCallstack();
        FatalError("Error", "There is no item object %d\n Called from:\n%s", which, callstack.c_str());
    }
    return (ItemObject*)the_scenegraph->item_objects_[which];
}

ItemObject* ASReadItemID(int which) {
    Object* obj = the_scenegraph->GetObjectFromID(which);
    if (!obj || obj->GetType() != _item_object) {
        std::string callstack = active_context_stack.top()->GetCallstack();
        FatalError("Error", "There is no item object %d\n Called from:\n%s", which, callstack.c_str());
    }
    return (ItemObject*)obj;
}

EnvObject* ASReadEnvObjectID(int which) {
    Object* obj = the_scenegraph->GetObjectFromID(which);
    if (!obj || obj->GetType() != _env_object) {
        FatalError("Error", "There is no env object %d", which);
    }
    return (EnvObject*)obj;
}

bool ASObjectExists(int which) {
    return the_scenegraph->DoesObjectWithIdExist(which);
}

bool ASMovementObjectExists(int id) {
    Object* obj = the_scenegraph->GetObjectFromID(id);
    if (obj && obj->GetType() == _movement_object) {
        return true;
    } else {
        return false;
    }
}

bool ASIsGroupDerived(int id) {
    Object* obj = the_scenegraph->GetObjectFromID(id);
    if (obj && obj->IsGroupDerived()) {
        return true;
    } else {
        return false;
    }
}

#include "Scripting/angelscript/add_on/scriptarray/scriptarray.h"
void GetCharactersInSphere(vec3 origin, float radius, CScriptArray* array) {
    ContactInfoCallback cb;
    the_scenegraph->abstract_bullet_world_->GetSphereCollisions(origin, radius, cb);
    for (int i = 0; i < cb.contact_info.size(); ++i) {
        BulletObject* bo = cb.contact_info[i].object;
        if (bo && bo->owner_object && the_scenegraph->IsObjectSane(bo->owner_object) && bo->owner_object->GetType() == _movement_object) {
            int val = ((MovementObject*)bo->owner_object)->GetID();
            array->InsertLast(&val);
        }
    }
}

void GetCharacters(CScriptArray* array) {
    for (auto& movement_object : the_scenegraph->movement_objects_) {
        int val = movement_object->GetID();
        array->InsertLast(&val);
    }
}

void GetCharactersInHull(std::string path, mat4 transform, CScriptArray* array) {
    ContactInfoCallback cb;
    the_scenegraph->abstract_bullet_world_->GetConvexHullCollisions(path, transform, cb);
    for (int i = 0; i < cb.contact_info.size(); ++i) {
        BulletObject* bo = cb.contact_info[i].object;
        if (bo && bo->owner_object && the_scenegraph->IsObjectSane(bo->owner_object) && bo->owner_object->GetType() == _movement_object) {
            MovementObject* mo = (MovementObject*)bo->owner_object;
            if (mo->visible) {
                int val = mo->GetID();
                array->InsertLast(&val);
            }
        }
    }
}

void GetObjectsInHull(std::string path, mat4 transform, CScriptArray* array) {
    ContactInfoCallback cb;
    the_scenegraph->abstract_bullet_world_->GetConvexHullCollisions(path, transform, cb);
    for (int i = 0; i < cb.contact_info.size(); ++i) {
        BulletObject* bo = cb.contact_info[i].object;
        if (bo && bo->owner_object && the_scenegraph->IsObjectSane(bo->owner_object)) {
            int val = ((Object*)bo->owner_object)->GetID();
            array->InsertLast(&val);
        }
    }
}

static void CreateCustomHull(const std::string& key, const CScriptArray& array) {
    std::vector<vec3> data;
    for (int i = 0, len = array.GetSize(); i < len; ++i) {
        data.push_back(*((vec3*)array.At(i)));
    }
    the_scenegraph->abstract_bullet_world_->CreateCustomHullShape(key, data);
}

#include "Objects/movementobject.h"
#include "Objects/itemobject.h"

bool ASDoesItemFitInItem(int a, int b) {
    Object* obj_a = the_scenegraph->GetObjectFromID(a);
    Object* obj_b = the_scenegraph->GetObjectFromID(b);
    if (!obj_a || obj_a->GetType() != _item_object) {
        std::string callstack = active_context_stack.top()->GetCallstack();
        FatalError("Error", "There is no item object %d.\n Called from:\n%s", a, callstack.c_str());
    };
    if (!obj_b || obj_b->GetType() != _item_object) {
        std::string callstack = active_context_stack.top()->GetCallstack();
        FatalError("Error", "There is no item object %d.\n Called from:\n%s", b, callstack.c_str());
    }
    ItemObject* item_obj_a = (ItemObject*)obj_a;
    ItemObject* item_obj_b = (ItemObject*)obj_b;
    return item_obj_b->item_ref()->GetContains() == item_obj_a->item_ref()->path_;
}

float ASGetFriction(const vec3& pos) {
    return the_scenegraph->GetMaterialFriction(pos);
}

void ASSetPaused(bool paused) {
    Engine::Instance()->menu_paused = paused;
    Engine::Instance()->CommitPause();
}

int ASFindFirstCharacterInGroup(int id) {
    Object* obj = the_scenegraph->GetObjectFromID(id);
    if (obj && obj->IsGroupDerived()) {
        Group* group = static_cast<Group*>(obj);
        std::vector<Object*> children;
        group->GetTopDownCompleteChildren(&children);
        for (auto& i : children) {
            if (i->GetType() == _movement_object) {
                return i->GetID();
            }
        }
    }
    return -1;
}

void AttachScenegraph(ASContext* context, SceneGraph* scenegraph) {
    the_scenegraph = scenegraph;

    context->RegisterGlobalFunction("int GetNumCharacters()",
                                    asFUNCTION(ASGetNumCharacters), asCALL_CDECL);
    context->RegisterGlobalFunction("int GetNumHotspots()",
                                    asFUNCTION(ASGetNumHotspots), asCALL_CDECL);
    context->RegisterGlobalFunction("int GetNumItems()",
                                    asFUNCTION(ASGetNumItems), asCALL_CDECL);
    context->RegisterGlobalFunction("void GetCharactersInSphere(vec3 position, float radius, array<int>@ id_array)",
                                    asFUNCTION(GetCharactersInSphere), asCALL_CDECL);
    context->RegisterGlobalFunction("void GetCharacters(array<int>@ id_array)",
                                    asFUNCTION(GetCharacters), asCALL_CDECL);
    context->RegisterGlobalFunction("void GetCharactersInHull(string model_path, mat4, array<int>@ id_array)",
                                    asFUNCTION(GetCharactersInHull), asCALL_CDECL);
    context->RegisterGlobalFunction("void GetObjectsInHull(string model_path, mat4, array<int>@ id_array)",
                                    asFUNCTION(GetObjectsInHull), asCALL_CDECL);
    context->RegisterGlobalFunction("void ResetLevel()",
                                    asFUNCTION(ResetLevel), asCALL_CDECL);
    context->RegisterGlobalFunction("bool DoesItemFitInItem(int item_id, int holster_item_id)",
                                    asFUNCTION(ASDoesItemFitInItem), asCALL_CDECL);
    context->RegisterGlobalFunction("float GetFriction(const vec3 &in position)",
                                    asFUNCTION(ASGetFriction), asCALL_CDECL);
    context->RegisterGlobalFunction("void CreateCustomHull(const string &in key, const array<vec3> &vertices)",
                                    asFUNCTION(CreateCustomHull), asCALL_CDECL);

    context->RegisterObjectType("Object", 0, asOBJ_REF | asOBJ_NOCOUNT);

    DefineRiggedObjectTypePublic(context);
    if (!context->TypeExists("MovementObject")) {
        DefineMovementObjectTypePublic(context);
        context->DocsCloseBrace();
    }
    DefineItemObjectTypePublic(context);
    DefineEnvObjectTypePublic(context);
    DefineHotspotTypePublic(context);

    AttachObject(context);

    context->RegisterGlobalFunction("MovementObject@ ReadCharacter(int index)", asFUNCTION(ASReadCharacter), asCALL_CDECL, "e.g. first character in scene");
    context->RegisterGlobalFunction("MovementObject@ ReadCharacterID(int id)", asFUNCTION(ASReadCharacterID), asCALL_CDECL, "e.g. character with object ID 39");
    context->RegisterGlobalFunction("Hotspot@ ReadHotspot(int index)", asFUNCTION(ASReadHotspot), asCALL_CDECL);
    context->RegisterGlobalFunction("ItemObject@ ReadItem(int index)", asFUNCTION(ASReadItem), asCALL_CDECL);
    context->RegisterGlobalFunction("ItemObject@ ReadItemID(int id)", asFUNCTION(ASReadItemID), asCALL_CDECL);
    context->RegisterGlobalFunction("EnvObject@ ReadEnvObjectID(int id)", asFUNCTION(ASReadEnvObjectID), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ObjectExists(int id)", asFUNCTION(ASObjectExists), asCALL_CDECL);
    context->RegisterGlobalFunction("bool MovementObjectExists(int id)", asFUNCTION(ASMovementObjectExists), asCALL_CDECL);
    context->RegisterGlobalFunction("bool IsGroupDerived(int id)", asFUNCTION(ASIsGroupDerived), asCALL_CDECL);
    context->RegisterGlobalFunction("void SetPaused(bool paused)", asFUNCTION(ASSetPaused), asCALL_CDECL);
    context->RegisterGlobalFunction("int FindFirstCharacterInGroup(int id)", asFUNCTION(ASFindFirstCharacterInGroup), asCALL_CDECL);
}

#include "Game/level.h"

void ASClearUndoHistory() {
    the_scenegraph->map_editor->ClearUndoHistory();
}

void ASRibbonItemSetEnabled(const std::string& str, bool val) {
}

void ASRibbonItemSetToggled(const std::string& str, bool val) {
}

void ASRibbonItemFlash(const std::string& str) {
}

bool ASMediaMode() {
    return Graphics::Instance()->media_mode();
}

void ASSetMediaMode(bool val) {
    Graphics::Instance()->SetMediaMode(val);
}

static void ASSetInterlevelData(const std::string& key, const std::string& val) {
    Engine::Instance()->interlevel_data[key] = val;
}

static std::string ASGetInterlevelData(const std::string& key) {
    return Engine::Instance()->interlevel_data[key];
}

void AttachLevel(ASContext* context) {
    Level::DefineLevelTypePublic(context);
    context->RegisterGlobalProperty("Level level", the_scenegraph->level);
    context->RegisterGlobalFunction("void ClearUndoHistory()", asFUNCTION(ASClearUndoHistory), asCALL_CDECL);
    context->RegisterGlobalFunction("void RibbonItemSetEnabled(const string &in, bool)", asFUNCTION(ASRibbonItemSetEnabled), asCALL_CDECL);
    context->RegisterGlobalFunction("void RibbonItemSetToggled(const string &in, bool)", asFUNCTION(ASRibbonItemSetToggled), asCALL_CDECL);
    context->RegisterGlobalFunction("void RibbonItemFlash(const string &in)", asFUNCTION(ASRibbonItemFlash), asCALL_CDECL);
    context->RegisterGlobalFunction("bool MediaMode()", asFUNCTION(ASMediaMode), asCALL_CDECL);
    context->RegisterGlobalFunction("void SetMediaMode(bool)", asFUNCTION(ASSetMediaMode), asCALL_CDECL);
}

void AttachInterlevelData(ASContext* context) {
    context->RegisterGlobalFunction("void SetInterlevelData(const string &in, const string &in)", asFUNCTION(ASSetInterlevelData), asCALL_CDECL);
    context->RegisterGlobalFunction("string GetInterlevelData(const string &in)", asFUNCTION(ASGetInterlevelData), asCALL_CDECL);
}

static void AS_TextureAssetRefConstructor(void* memory) {
    new (memory) TextureAssetRef();
}

static void AS_TextureAssetRefCopyConstructor(void* memory, const TextureAssetRef& other) {
    new (memory) TextureAssetRef(other);
}

static void AS_TextureAssetRefDestructor(void* memory) {
    ((TextureAssetRef*)memory)->~TextureAssetRef();
}

static const TextureAssetRef& AS_TextureAssetRefOpAssign(TextureAssetRef* self, const TextureAssetRef& other) {
    return (*self) = other;
}

static bool AS_TextureAssetRefOpEquals(TextureAssetRef* self, const TextureAssetRef& other) {
    return (*self) == other;
}

static int AS_TextureAssetRefOpCmp(TextureAssetRef* self, const TextureAssetRef& other) {
    return (*self) < other ? -1 : ((*self) == other ? 0 : 1);
}

static TextureAssetRef AS_LoadTexture(const std::string& path, uint32_t texture_load_flags) {
    return Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(path, texture_load_flags, 0x0);
}

bool AS_ImGui_Begin(const std::string& name, int flags) {
    return ImGui::Begin(name.c_str(), NULL, flags);
}

bool AS_ImGui_Begin2(const std::string& name, bool& open, int flags) {
    return ImGui::Begin(name.c_str(), &open, flags);
}

void AS_ImGui_End() {
    ImGui::End();
}

bool AS_ImGui_BeginChild(const std::string& str_id, const vec2& size, bool border, int extra_flags) {
    return ImGui::BeginChild(str_id.c_str(), ImVec2(size[0], size[1]), border, extra_flags);
}

bool AS_ImGui_BeginChild2(unsigned int id, const vec2& size, bool border, int extra_flags) {
    return ImGui::BeginChild(id, ImVec2(size[0], size[1]), border, extra_flags);
}

void AS_ImGui_EndChild() {
    return ImGui::EndChild();
}

vec2 AS_ImGui_GetContentRegionMax() {
    ImVec2 result = ImGui::GetContentRegionMax();
    return vec2(result.x, result.y);
}

vec2 AS_ImGui_GetContentRegionAvail() {
    ImVec2 result = ImGui::GetContentRegionAvail();
    return vec2(result.x, result.y);
}

float AS_ImGui_GetContentRegionAvailWidth() {
    return ImGui::GetContentRegionAvailWidth();
}

vec2 AS_ImGui_GetWindowContentRegionMin() {
    ImVec2 result = ImGui::GetWindowContentRegionMin();
    return vec2(result.x, result.y);
}

vec2 AS_ImGui_GetWindowContentRegionMax() {
    ImVec2 result = ImGui::GetWindowContentRegionMax();
    return vec2(result.x, result.y);
}

float AS_ImGui_GetWindowContentRegionWidth() {
    return ImGui::GetWindowContentRegionWidth();
}

vec2 AS_ImGui_GetWindowPos() {
    ImVec2 result = ImGui::GetWindowPos();
    return vec2(result.x, result.y);
}

vec2 AS_ImGui_GetWindowSize() {
    ImVec2 result = ImGui::GetWindowSize();
    return vec2(result.x, result.y);
}

float AS_ImGui_GetWindowWidth() {
    return ImGui::GetWindowWidth();
}

float AS_ImGui_GetWindowHeight() {
    return ImGui::GetWindowHeight();
}

bool AS_ImGui_IsWindowCollapsed() {
    return ImGui::IsWindowCollapsed();
}

bool AS_ImGui_IsWindowAppearing() {
    return ImGui::IsWindowAppearing();
}

void AS_ImGui_SetWindowFontScale(float scale) {
    return ImGui::SetWindowFontScale(scale);
}

void AS_ImGui_SetNextWindowPos(const vec2& pos, int cond) {
    ImGui::SetNextWindowPos(ImVec2(pos[0], pos[1]), cond);
}

void AS_ImGui_SetNextWindowPosCenter(int cond) {
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), cond, ImVec2(0.5f, 0.5f));
}

void AS_ImGui_SetNextWindowSize(const vec2& size, int cond) {
    ImGui::SetNextWindowSize(ImVec2(size[0], size[1]), cond);
}

void AS_ImGui_SetNextWindowContentSize(const vec2& size) {
    ImGui::SetNextWindowContentSize(ImVec2(size[0], size[1]));
}

void AS_ImGui_SetNextWindowContentWidth(float width) {
    ImGui::SetNextWindowContentSize(ImVec2(width, 0.0f));
}

void AS_ImGui_SetNextWindowCollapsed(bool collapsed, int cond) {
    ImGui::SetNextWindowCollapsed(collapsed, cond);
}

void AS_ImGui_SetNextWindowFocus() {
    ImGui::SetNextWindowFocus();
}

void AS_ImGui_SetWindowPos(const vec2& pos, int cond) {
    ImGui::SetWindowPos(ImVec2(pos[0], pos[1]), cond);
}

void AS_ImGui_SetWindowSize(const vec2& size, int cond) {
    ImGui::SetWindowSize(ImVec2(size[0], size[1]), cond);
}

void AS_ImGui_SetWindowCollapsed(bool collapsed, int cond) {
    ImGui::SetWindowCollapsed(collapsed, cond);
}

void AS_ImGui_SetWindowFocus() {
    ImGui::SetWindowFocus();
}

void AS_ImGui_SetWindowPos2(const std::string& name, const vec2& pos, int cond) {
    ImGui::SetWindowPos(name.c_str(), ImVec2(pos[0], pos[1]), cond);
}

void AS_ImGui_SetWindowSize2(const std::string& name, const vec2& size, int cond) {
    ImGui::SetWindowSize(name.c_str(), ImVec2(size[0], size[1]), cond);
}

void AS_ImGui_SetWindowCollapsed2(const std::string& name, bool collapsed, int cond) {
    ImGui::SetWindowCollapsed(name.c_str(), collapsed, cond);
}

void AS_ImGui_SetWindowFocus2(const std::string& name) {
    ImGui::SetWindowFocus(name.c_str());
}

float AS_ImGui_GetScrollX() {
    return ImGui::GetScrollX();
}

float AS_ImGui_GetScrollY() {
    return ImGui::GetScrollY();
}

float AS_ImGui_GetScrollMaxX() {
    return ImGui::GetScrollMaxX();
}

float AS_ImGui_GetScrollMaxY() {
    return ImGui::GetScrollMaxY();
}

void AS_ImGui_SetScrollX(float scroll_x) {
    ImGui::SetScrollX(scroll_x);
}

void AS_ImGui_SetScrollY(float scroll_y) {
    ImGui::SetScrollY(scroll_y);
}

void AS_ImGui_SetScrollHere(float center_y_ratio = 0.5f) {
    ImGui::SetScrollHereY(center_y_ratio);
}

void AS_ImGui_SetScrollFromPosY(float pos_y, float center_y_ratio = 0.5f) {
    ImGui::SetScrollFromPosY(pos_y, center_y_ratio);
}

void AS_ImGui_SetKeyboardFocusHere(int val) {
    ImGui::SetKeyboardFocusHere(val);
}

void AS_ImGui_PushStyleColor(int idx, const vec4& color) {
    ImVec4 col(color[0], color[1], color[2], color[3]);
    ImGui::PushStyleColor(idx, col);
}

void AS_ImGui_PopStyleColor(int count) {
    ImGui::PopStyleColor(count);
}

void AS_ImGui_PushStyleVar(int idx, float val) {
    ImGui::PushStyleVar(idx, val);
}

void AS_ImGui_PushStyleVar2(int idx, const vec2& value) {
    ImVec2 val(value[0], value[1]);
    ImGui::PushStyleVar(idx, val);
}

void AS_ImGui_PushStyleVar3(int idx, bool value) {
    ImGui::PushStyleVar(idx, value);
}

void AS_ImGui_PushStyleVar4(int idx, int value) {
    ImGui::PushStyleVar(idx, (float)value);
}

void AS_ImGui_BeginDisabled(bool is_disabled) {
    ImGui::BeginDisabled(is_disabled);
}

void AS_ImGui_EndDisabled(bool is_disabled) {
    ImGui::EndDisabled();
}

void AS_ImGui_PopStyleVar(int count) {
    ImGui::PopStyleVar(count);
}

vec4 AS_ImGui_GetStyleColorVec4(int idx) {
    ImVec4 ret = ImGui::GetStyleColorVec4(idx);
    return vec4(ret.x, ret.y, ret.z, ret.w);
}

uint32_t AS_ImGui_GetColorU32(uint32_t idx, float alpha_mul) {
    return ImGui::GetColorU32(idx, alpha_mul);
}

uint32_t AS_ImGui_GetColorU32_2(const vec3& col) {
    return ImGui::GetColorU32(ImVec4(col[0], col[1], col[2], 1.0f));
}

uint32_t AS_ImGui_GetColorU32_3(const vec4& col) {
    return ImGui::GetColorU32(ImVec4(col[0], col[1], col[2], col[3]));
}

uint32_t AS_ImGui_GetColorU32_4(float r, float g, float b, float alpha_mul) {
    return ImGui::GetColorU32(ImVec4(r, g, b, alpha_mul));
}

void AS_ImGui_PushItemWidth(float val) {
    ImGui::PushItemWidth(val);
}

void AS_ImGui_PopItemWidth() {
    ImGui::PopItemWidth();
}

float AS_ImGui_CalcItemWidth() {
    return ImGui::CalcItemWidth();
}

void AS_ImGui_PushTextWrapPos(float wrap_pos_x) {
    ImGui::PushTextWrapPos(wrap_pos_x);
}

void AS_ImGui_PopTextWrapPos() {
    ImGui::PopTextWrapPos();
}

void AS_ImGui_PushAllowKeyboardFocus(bool allow_keyboard_focus) {
    ImGui::PushAllowKeyboardFocus(allow_keyboard_focus);
}

void AS_ImGui_PopAllowKeyboardFocus() {
    ImGui::PopAllowKeyboardFocus();
}

void AS_ImGui_PushButtonRepeat(bool repeat) {
    ImGui::PushButtonRepeat(repeat);
}

void AS_ImGui_PopButtonRepeat() {
    ImGui::PopButtonRepeat();
}

void AS_ImGui_Separator() {
    ImGui::Separator();
}

void AS_ImGui_SameLine(float pos_x, float spacing_w) {
    ImGui::SameLine(pos_x, spacing_w);
}

void AS_ImGui_NewLine() {
    ImGui::NewLine();
}

void AS_ImGui_Spacing() {
    ImGui::Spacing();
}

void AS_ImGui_Dummy(const vec2& size) {
    ImVec2 temp_size(size[0], size[1]);
    ImGui::Dummy(temp_size);
}

void AS_ImGui_Indent(float indent_w) {
    ImGui::Indent(indent_w);
}

void AS_ImGui_Unindent(float indent_w) {
    ImGui::Unindent(indent_w);
}

void AS_ImGui_BeginGroup() {
    ImGui::BeginGroup();
}

void AS_ImGui_EndGroup() {
    ImGui::EndGroup();
}

vec2 AS_ImGui_GetCursorPos() {
    ImVec2 result = ImGui::GetCursorPos();
    return vec2(result.x, result.y);
}

float AS_ImGui_GetCursorPosX() {
    return ImGui::GetCursorPosX();
}

float AS_ImGui_GetCursorPosY() {
    return ImGui::GetCursorPosY();
}

void AS_ImGui_SetCursorPos(const vec2& local_pos) {
    ImVec2 temp_local_pos(local_pos[0], local_pos[1]);
    ImGui::SetCursorPos(temp_local_pos);
}

void AS_ImGui_SetCursorPosX(float x) {
    ImGui::SetCursorPosX(x);
}

void AS_ImGui_SetCursorPosY(float y) {
    ImGui::SetCursorPosY(y);
}

vec2 AS_ImGui_GetCursorStartPos() {
    ImVec2 result = ImGui::GetCursorStartPos();
    return vec2(result.x, result.y);
}

vec2 AS_ImGui_GetCursorScreenPos() {
    ImVec2 result = ImGui::GetCursorScreenPos();
    return vec2(result.x, result.y);
}

void AS_ImGui_SetCursorScreenPos(const vec2& pos) {
    ImVec2 temp_pos(pos[0], pos[1]);
    ImGui::SetCursorScreenPos(temp_pos);
}

void AS_ImGui_AlignTextToFramePadding() {
    return ImGui::AlignTextToFramePadding();
}

float AS_ImGui_GetTextLineHeight() {
    return ImGui::GetTextLineHeight();
}

float AS_ImGui_GetTextLineHeightWithSpacing() {
    return ImGui::GetTextLineHeightWithSpacing();
}

float AS_ImGui_GetFrameHeight() {
    return ImGui::GetFrameHeight();
}

float AS_ImGui_GetFrameHeightWithSpacing() {
    return ImGui::GetFrameHeightWithSpacing();
}

void AS_ImGui_Columns(int count, bool border) {
    ImGui::Columns(count, NULL, border);
}

void AS_ImGui_Columns2(int count, const std::string& id, bool border) {
    ImGui::Columns(count, id.c_str(), border);
}

void AS_ImGui_NextColumn() {
    ImGui::NextColumn();
}

int AS_ImGui_GetColumnIndex() {
    return ImGui::GetColumnIndex();
}

float AS_ImGui_GetColumnOffset(int column_index) {
    return ImGui::GetColumnOffset(column_index);
}

void AS_ImGui_SetColumnOffset(int column_index, float offset_x) {
    ImGui::SetColumnOffset(column_index, offset_x);
}

float AS_ImGui_GetColumnWidth(int column_index) {
    return ImGui::GetColumnWidth(column_index);
}

void AS_ImGui_SetColumnWidth(int column_index, float width) {
    return ImGui::SetColumnWidth(column_index, width);
}

int AS_ImGui_GetColumnsCount() {
    return ImGui::GetColumnsCount();
}

void AS_ImGui_PushID(const std::string& str) {
    ImGui::PushID(str.c_str());
}

void AS_ImGui_PushID2(const void* ptr_id) {
    ImGui::PushID(ptr_id);
}

void AS_ImGui_PushID3(int int_id) {
    ImGui::PushID(int_id);
}

void AS_ImGui_PopID() {
    ImGui::PopID();
}

unsigned int AS_ImGui_GetID(const std::string& str_id) {
    return ImGui::GetID(str_id.c_str());
}

unsigned int AS_ImGui_GetID2(const void* ptr_id) {
    return ImGui::GetID(ptr_id);
}

void AS_ImGui_Text(const std::string& str) {
    ImGui::Text("%s", str.c_str());
}

void AS_ImGui_TextColored(const vec4& col, const std::string& str) {
    ImGui::TextColored(ImVec4(col[0], col[1], col[2], col[3]), "%s", str.c_str());
}

void AS_ImGui_TextDisabled(const std::string& label) {
    ImGui::TextDisabled("%s", label.c_str());
}

void AS_ImGui_TextWrapped(const std::string& label) {
    ImGui::TextWrapped("%s", label.c_str());
}

void AS_ImGui_LabelText(const std::string& str, const std::string& label) {
    ImGui::LabelText(str.c_str(), "%s", label.c_str());
}

void AS_ImGui_Bullet() {
    ImGui::Bullet();
}

void AS_ImGui_BulletText(const std::string& label) {
    ImGui::BulletText("%s", label.c_str());
}

bool AS_ImGui_Button(const std::string& label, const vec2& size) {
    ImVec2 size_temp(size[0], size[1]);
    return ImGui::Button(label.c_str(), size_temp);
}

bool AS_ImGui_SmallButton(const std::string& label) {
    return ImGui::SmallButton(label.c_str());
}

bool AS_ImGui_InvisibleButton(const std::string& str_id, const vec2& size) {
    ImVec2 size_temp(size[0], size[1]);
    return ImGui::InvisibleButton(str_id.c_str(), size_temp);
}

void AS_ImGui_Image(const TextureAssetRef& user_texture_ref, const vec2& size, const vec2& uv0, const vec2& uv1, const vec4& tint_color, const vec4& border_color) {
    if (user_texture_ref.valid()) {
        ImVec2 size_temp(size[0], size[1]);
        ImVec2 uv0_temp(uv0[0], uv0[1]);
        ImVec2 uv1_temp(uv1[0], uv1[1]);
        ImVec4 tint_color_temp(tint_color[0], tint_color[1], tint_color[2], tint_color[3]);
        ImVec4 border_color_temp(border_color[0], border_color[1], border_color[2], border_color[3]);
        Textures::Instance()->EnsureInVRAM(user_texture_ref);
        ImGui::Image(reinterpret_cast<ImTextureID>(Textures::Instance()->returnTexture(user_texture_ref)), size_temp, uv0_temp, uv1_temp, tint_color_temp, border_color_temp);
    }
}

bool AS_ImGui_ImageButton(const TextureAssetRef& user_texture_ref, const vec2& size, const vec2& uv0, const vec2& uv1, int frame_padding, const vec4& background_color, const vec4& tint_color) {
    if (user_texture_ref.valid()) {
        ImVec2 size_temp(size[0], size[1]);
        ImVec2 uv0_temp(uv0[0], uv0[1]);
        ImVec2 uv1_temp(uv1[0], uv1[1]);
        ImVec4 background_color_temp(background_color[0], background_color[1], background_color[2], background_color[3]);
        ImVec4 tint_color_temp(tint_color[0], tint_color[1], tint_color[2], tint_color[3]);
        Textures::Instance()->EnsureInVRAM(user_texture_ref);
        return ImGui::ImageButton(reinterpret_cast<ImTextureID>(Textures::Instance()->returnTexture(user_texture_ref)), size_temp, uv0_temp, uv1_temp, frame_padding, background_color_temp, tint_color_temp);
    }

    return false;
}

bool AS_ImGui_Checkbox(const std::string& label, bool& value) {
    return ImGui::Checkbox(label.c_str(), &value);
}

bool AS_ImGui_CheckboxFlags(const std::string& label, unsigned int& flags, unsigned int flags_value) {
    return ImGui::CheckboxFlags(label.c_str(), &flags, flags_value);
}

bool AS_ImGui_RadioButton(const std::string& label, bool active) {
    return ImGui::RadioButton(label.c_str(), active);
}

bool AS_ImGui_RadioButton2(const std::string& label, int& value, int v_button) {
    return ImGui::RadioButton(label.c_str(), &value, v_button);
}

bool AS_ImGui_BeginCombo(const std::string& label, const std::string& preview_value, int flags) {
    return ImGui::BeginCombo(label.c_str(), preview_value.c_str(), flags);
}

void AS_ImGui_EndCombo() {
    ImGui::EndCombo();
}

bool AS_ImGui_Combo(const std::string& label, int& current_item, const CScriptArray& items, int height_in_items) {
    const int items_count = items.GetSize();
    std::vector<const char*> items_data;

    items_data.reserve(items_count);
    for (int n = 0; n < items_count; ++n) {
        items_data.push_back(reinterpret_cast<const std::string*>(items.At(n))->c_str());
    }

    return ImGui::Combo(label.c_str(), &current_item, &items_data[0], items_count, height_in_items);
}

bool AS_ImGui_Combo2(const std::string& label, int& current_item, const std::string& items_separated_by_zeros, int height_in_items) {
    return ImGui::Combo(label.c_str(), &current_item, items_separated_by_zeros.c_str(), height_in_items);
}

bool AS_ImGui_ColorButton(const vec4& color, bool small_height, bool outline_border) {
    ImVec4 temp_color(color[0], color[1], color[2], color[3]);
    return ImGui::ColorButton("", temp_color, small_height);
}

bool AS_ImGui_ColorButton2(const std::string& desc, const vec4& color, ImGuiColorEditFlags flags, const vec2& size) {
    ImVec4 temp_color(color[0], color[1], color[2], color[3]);
    ImVec2 temp_size(size[0], size[1]);
    return ImGui::ColorButton(desc.c_str(), temp_color, flags, temp_size);
}

bool AS_ImGui_ColorEdit3(const std::string& label, vec3& color) {
    return ImGui::ColorEdit3(label.c_str(), color.entries);
}

bool AS_ImGui_ColorEdit4(const std::string& label, vec4& color, bool show_alpha) {
    ImGuiColorEditFlags flags = 0x01;
    if (!show_alpha) {
        flags = ImGuiColorEditFlags_NoAlpha;
    }
    return ImGui::ColorEdit4(label.c_str(), color.entries, flags);
}

bool AS_ImGui_ColorEdit4_2(const std::string& label, vec4& color, ImGuiColorEditFlags flags) {
    return ImGui::ColorEdit4(label.c_str(), color.entries, flags);
}

bool AS_ImGui_ColorPicker3(const std::string label, vec3& color, int flags) {
    return ImGui::ColorPicker3(label.c_str(), color.entries, flags);
}

bool AS_ImGui_ColorPicker4(const std::string label, vec4& color, int flags) {
    return ImGui::ColorPicker4(label.c_str(), color.entries, flags);
}

bool AS_ImGui_ColorPicker4_2(const std::string label, vec4& color, int flags, vec4 ref_color) {
    return ImGui::ColorPicker4(label.c_str(), color.entries, flags, ref_color.entries);
}

void AS_ImGui_ColorEditMode(int mode) {
    // From old imgui version
    // ImGuiColorEditMode_UserSelect = -2;
    // ImGuiColorEditMode_UserSelectShowButton = -1;
    // ImGuiColorEditMode_RGB = 0
    // ImGuiColorEditMode_HSV = 1;
    // ImGuiColorEditMode_HEX = 2;

    switch (mode) {
        case 0:
            ImGui::SetColorEditOptions(ImGuiColorEditFlags_RGB);
            break;
        case 1:
            ImGui::SetColorEditOptions(ImGuiColorEditFlags_HSV);
            break;
        case 2:
            ImGui::SetColorEditOptions(ImGuiColorEditFlags_HEX);
            break;
        default:
            // UserSelect and UserSelectShowButton doesn't exist anymore, so default to RGB
            ImGui::SetColorEditOptions(ImGuiColorEditFlags_RGB);
            break;
    }
}

bool AS_ImGui_DragFloat(const std::string& label, float& value, float v_speed, float v_min, float v_max, const std::string& display_format, float power) {
    return ImGui::DragFloat(label.c_str(), &value, v_speed, v_min, v_max, display_format.c_str(), power);
}

bool AS_ImGui_DragFloat2_Vector(const std::string& label, vec2& value, float v_speed, float v_min, float v_max, const std::string& display_format, float power) {
    return ImGui::DragFloat2(label.c_str(), value.entries, v_speed, v_min, v_max, display_format.c_str(), power);
}

bool AS_ImGui_DragFloat3_Vector(const std::string& label, vec3& value, float v_speed, float v_min, float v_max, const std::string& display_format, float power) {
    return ImGui::DragFloat3(label.c_str(), value.entries, v_speed, v_min, v_max, display_format.c_str(), power);
}

bool AS_ImGui_DragFloat4_Vector(const std::string& label, vec4& value, float v_speed, float v_min, float v_max, const std::string& display_format, float power) {
    return ImGui::DragFloat4(label.c_str(), value.entries, v_speed, v_min, v_max, display_format.c_str(), power);
}

bool AS_ImGui_DragFloatRange2_Vector(const std::string& label, float& v_current_min, float& v_current_max, float v_speed, float v_min, float v_max, const std::string& display_format) {
    return ImGui::DragFloatRange2(label.c_str(), &v_current_min, &v_current_max, v_speed, v_min, v_max, display_format.c_str());
}

bool AS_ImGui_DragFloatRange2_Vector2(const std::string& label, float& v_current_min, float& v_current_max, float v_speed, float v_min, float v_max, const std::string& display_format, const std::string& display_format_max, float power) {
    LOGW << "This version of \"ImGui_DragFloatRange2_Vector2\" is obsolete, use the version without the optional power parameter!" << std::endl;
    return ImGui::DragFloatRange2(label.c_str(), &v_current_min, &v_current_max, v_speed, v_min, v_max, display_format.c_str(), display_format_max.c_str());
}

bool AS_ImGui_DragInt(const std::string& label, int& value, float v_speed, int v_min, int v_max, const std::string& display_format) {
    return ImGui::DragInt(label.c_str(), &value, v_speed, v_min, v_max, display_format.c_str());
}

bool AS_ImGui_DragInt2_Vector(const std::string& label, ivec2& value, float v_speed, int v_min, int v_max, const std::string& display_format) {
    return ImGui::DragInt2(label.c_str(), value.entries, v_speed, v_min, v_max, display_format.c_str());
}

bool AS_ImGui_DragInt3_Vector(const std::string& label, ivec3& value, float v_speed, int v_min, int v_max, const std::string& display_format) {
    return ImGui::DragInt3(label.c_str(), value.entries, v_speed, v_min, v_max, display_format.c_str());
}

bool AS_ImGui_DragInt4_Vector(const std::string& label, ivec4& value, float v_speed, int v_min, int v_max, const std::string& display_format) {
    return ImGui::DragInt4(label.c_str(), value.entries, v_speed, v_min, v_max, display_format.c_str());
}

bool AS_ImGui_DragIntRange2_Vector(const std::string& label, int& v_current_min, int& v_current_max, float v_speed, int v_min, int v_max, const std::string& display_format) {
    return ImGui::DragIntRange2(label.c_str(), &v_current_min, &v_current_max, v_speed, v_min, v_max, display_format.c_str());
}

bool AS_ImGui_DragIntRange2_Vector2(const std::string& label, int& v_current_min, int& v_current_max, float v_speed, int v_min, int v_max, const std::string& display_format, const std::string& display_format_max) {
    return ImGui::DragIntRange2(label.c_str(), &v_current_min, &v_current_max, v_speed, v_min, v_max, display_format.c_str(), display_format_max.c_str());
}

const int kBufSize = 262144;
char buf[kBufSize];

int imgui_text_input_CursorPos;
int imgui_text_input_SelectionStart;
int imgui_text_input_SelectionEnd;

bool AS_ImGui_InputText(const std::string& label, int flags) {
    return ImGui::InputText(label.c_str(), buf, kBufSize, flags);
}

std::string AS_ImGui_GetTextBuf() {
    return std::string(buf);
}

void AS_ImGui_SetTextBuf(const std::string& contents) {
    FormatString(buf, kBufSize, "%s", contents.c_str());
}

bool AS_ImGui_InputText2(const std::string& label, std::string& text_buffer, int buffer_size, int flags) {
    text_buffer.reserve(buffer_size);
    AS_ImGui_SetTextBuf(text_buffer);
    bool ret_val = ImGui::InputText(label.c_str(), buf, buffer_size, flags);
    if (ret_val) {
        size_t new_size = strlen(buf);
        if (text_buffer.size() < new_size) {
            text_buffer.insert(text_buffer.size(), new_size - text_buffer.size(), 0);
        } else if (text_buffer.size() > new_size) {
            text_buffer.erase(text_buffer.end() - (text_buffer.size() - new_size), text_buffer.end());
        }
        strcpy(&text_buffer[0], buf);
    }
    return ret_val;
}

int ImGui_TextInputCallback(ImGuiInputTextCallbackData* data) {
    imgui_text_input_CursorPos = data->CursorPos;
    imgui_text_input_SelectionStart = data->SelectionStart;
    imgui_text_input_SelectionEnd = data->SelectionEnd;
    return 0;
}

bool AS_ImGui_InputTextMultiline(const std::string& label, const vec2& size, int flags) {
    return ImGui::InputTextMultiline(label.c_str(), buf, kBufSize, ImVec2(size[0], size[1]), flags | ImGuiInputTextFlags_CallbackAlways, ImGui_TextInputCallback);
}

bool AS_ImGui_InputTextMultiline2(const std::string& label, std::string& text_buffer, int buffer_size, const vec2& size, int flags) {
    text_buffer.reserve(buffer_size);
    AS_ImGui_SetTextBuf(text_buffer);
    bool ret_val = ImGui::InputTextMultiline(label.c_str(), buf, buffer_size, ImVec2(size[0], size[1]), flags | ImGuiInputTextFlags_CallbackAlways, ImGui_TextInputCallback);
    if (ret_val) {
        size_t new_size = strlen(buf);
        if (text_buffer.size() < new_size) {
            text_buffer.insert(text_buffer.size(), new_size - text_buffer.size(), 0);
        } else if (text_buffer.size() > new_size) {
            text_buffer.erase(text_buffer.end() - (text_buffer.size() - new_size), text_buffer.end());
        }
        strcpy(&text_buffer[0], buf);
    }
    return ret_val;
}

bool AS_ImGui_InputFloat(const std::string& label, float& value, float step, float step_fast, const char* format, int extra_flags) {
    return ImGui::InputFloat(label.c_str(), &value, step, step_fast, format, extra_flags);
}

bool AS_ImGui_InputFloat2_Vector(const std::string& label, vec2& value, const char* format, int extra_flags) {
    return ImGui::InputFloat2(label.c_str(), value.entries, format, extra_flags);
}

bool AS_ImGui_InputFloat3_Vector(const std::string& label, vec3& value, const char* format, int extra_flags) {
    return ImGui::InputFloat3(label.c_str(), value.entries, format, extra_flags);
}

bool AS_ImGui_InputFloat4_Vector(const std::string& label, vec4& value, const char* format, int extra_flags) {
    return ImGui::InputFloat4(label.c_str(), value.entries, format, extra_flags);
}

bool AS_ImGui_InputInt(const std::string& label, int& value, int step, int step_fast, int extra_flags) {
    return ImGui::InputInt(label.c_str(), &value, step, step_fast, extra_flags);
}

bool AS_ImGui_SliderFloat(const std::string& label, float& v, float v_min, float v_max, const std::string& display_format, float power) {
    return ImGui::SliderFloat(label.c_str(), &v, v_min, v_max, display_format.c_str(), power);
}

bool AS_ImGui_SliderFloat2(const std::string& label, vec2& v, float v_min, float v_max, const std::string& display_format, float power) {
    return ImGui::SliderFloat2(label.c_str(), v.entries, v_min, v_max, display_format.c_str(), power);
}

bool AS_ImGui_SliderFloat3(const std::string& label, vec3& v, float v_min, float v_max, const std::string& display_format, float power) {
    return ImGui::SliderFloat3(label.c_str(), v.entries, v_min, v_max, display_format.c_str(), power);
}

bool AS_ImGui_SliderFloat4(const std::string& label, vec4& v, float v_min, float v_max, const std::string& display_format, float power) {
    return ImGui::SliderFloat4(label.c_str(), v.entries, v_min, v_max, display_format.c_str(), power);
}

bool AS_ImGui_SliderAngle(const std::string& label, float& v_rad, float v_degrees_min, float v_degrees_max) {
    return ImGui::SliderAngle(label.c_str(), &v_rad, v_degrees_min, v_degrees_max);
}

bool AS_ImGui_SliderInt(const std::string& label, int& v, int v_min, int v_max, const std::string& display_format) {
    return ImGui::SliderInt(label.c_str(), &v, v_min, v_max, display_format.c_str());
}

bool AS_ImGui_SliderInt2(const std::string& label, ivec2& v, int v_min, int v_max, const std::string& display_format) {
    return ImGui::SliderInt2(label.c_str(), v.entries, v_min, v_max, display_format.c_str());
}

bool AS_ImGui_SliderInt3(const std::string& label, ivec3& v, int v_min, int v_max, const std::string& display_format) {
    return ImGui::SliderInt3(label.c_str(), v.entries, v_min, v_max, display_format.c_str());
}

bool AS_ImGui_SliderInt4(const std::string& label, ivec4& v, int v_min, int v_max, const std::string& display_format) {
    return ImGui::SliderInt4(label.c_str(), v.entries, v_min, v_max, display_format.c_str());
}

bool AS_ImGui_TreeNode(const std::string& label) {
    return ImGui::TreeNode(label.c_str());
}

bool AS_ImGui_TreeNode2(const std::string& str_id, const std::string& label) {
    return ImGui::TreeNode(str_id.c_str(), "%s", label.c_str());
}

bool AS_ImGui_TreeNode3(const void* ref, int typeId, const std::string& label) {
    return ImGui::TreeNode(ref, "%s", label.c_str());
}

bool AS_ImGui_TreeNodeEx(const std::string& label, int flags) {
    return ImGui::TreeNodeEx(label.c_str(), flags);
}

bool AS_ImGui_TreeNodeEx2(const std::string& str_id, int flags, const std::string& label) {
    return ImGui::TreeNodeEx(str_id.c_str(), flags, "%s", label.c_str());
}

bool AS_ImGui_TreeNodeEx3(const void* ref, int flags, const std::string& label) {
    return ImGui::TreeNodeEx(ref, flags, "%s", label.c_str());
}

void AS_ImGui_TreePush() {
    ImGui::TreePush(static_cast<const void*>(NULL));
}

void AS_ImGui_TreePush2(const std::string& str_id) {
    ImGui::TreePush(str_id.c_str());
}

void AS_ImGui_TreePush3(const void* ref, int typeId) {
    ImGui::TreePush(ref);
}

void AS_ImGui_TreePop() {
    ImGui::TreePop();
}

void AS_ImGui_TreeAdvanceToLabelPos() {
    ImGui::TreeAdvanceToLabelPos();
}

float AS_ImGui_GetTreeNodeToLabelSpacing() {
    return ImGui::GetTreeNodeToLabelSpacing();
}

void AS_ImGui_SetNextTreeNodeOpen(bool is_open, int cond) {
    ImGui::SetNextTreeNodeOpen(is_open, cond);
}

bool AS_ImGui_CollapsingHeader(const std::string& label, int flags) {
    return ImGui::CollapsingHeader(label.c_str(), flags);
}

bool AS_ImGui_CollapsingHeader2(const std::string& label, bool& is_open, int flags) {
    return ImGui::CollapsingHeader(label.c_str(), &is_open, flags);
}

bool AS_ImGui_Selectable(const std::string& name, bool selected, int flags, const vec2& size) {
    ImVec2 temp_size(size[0], size[1]);
    return ImGui::Selectable(name.c_str(), selected, flags, temp_size);
}

bool AS_ImGui_SelectableToggle(const std::string& name, bool& selected, int flags, const vec2& size) {
    ImVec2 temp_size(size[0], size[1]);
    return ImGui::Selectable(name.c_str(), &selected, flags, temp_size);
}

bool AS_ImGui_ListBox(const std::string& label, int& current_item, const CScriptArray& items, int height_in_items) {
    const int items_count = items.GetSize();
    std::vector<const char*> items_data;

    items_data.reserve(items_count);
    for (int n = 0; n < items_count; ++n) {
        items_data.push_back(reinterpret_cast<const std::string*>(items.At(n))->c_str());
    }

    return ImGui::ListBox(label.c_str(), &current_item, items_data.data(), items_count, height_in_items);
}

bool AS_ImGui_ListBoxHeader(const std::string& label) {
    ImVec2 size_temp(0, 0);
    return ImGui::BeginListBox(label.c_str(), size_temp);
}

bool AS_ImGui_ListBoxHeader2(const std::string& label, const vec2& size) {
    ImVec2 size_temp(size[0], size[1]);
    return ImGui::BeginListBox(label.c_str(), size_temp);
}

bool AS_ImGui_ListBoxHeader3(const std::string& label, int items_count, int height_in_items) {
    return ImGui::BeginListBox(label.c_str());  // , items_count, height_in_items);
}

void AS_ImGui_ListBoxFooter() {
    ImGui::ListBoxFooter();
}

void AS_ImGui_SetTooltip(const std::string& text) {
    ImGui::SetTooltip("%s", text.c_str());
}

void AS_ImGui_BeginTooltip() {
    ImGui::BeginTooltip();
}

void AS_ImGui_EndTooltip() {
    ImGui::EndTooltip();
}

bool AS_ImGui_BeginMenuBar() {
    return ImGui::BeginMenuBar();
}

void AS_ImGui_EndMenuBar() {
    ImGui::EndMenuBar();
}

bool AS_ImGui_BeginMenu(const std::string& label, bool enabled) {
    return ImGui::BeginMenu(label.c_str(), enabled);
}

void AS_ImGui_EndMenu() {
    ImGui::EndMenu();
}

bool AS_ImGui_MenuItem(const std::string& label, bool selected, bool enabled) {
    return ImGui::MenuItem(label.c_str(), NULL, &selected, enabled);
}

void AS_ImGui_OpenPopup(const std::string& str_id) {
    ImGui::OpenPopup(str_id.c_str());
}

void AS_ImGui_OpenPopupOnItemClick(const std::string& label, int mouse_button) {
    ImGui::OpenPopupOnItemClick(label.c_str(), mouse_button);
}

bool AS_ImGui_BeginPopup(const std::string& str_id) {
    return ImGui::BeginPopup(str_id.c_str());
}

bool AS_ImGui_BeginPopupModal(const std::string& name, int flags) {
    return ImGui::BeginPopupModal(name.c_str(), NULL, flags);
}

bool AS_ImGui_BeginPopupModal2(const std::string& name, bool& open, int flags) {
    return ImGui::BeginPopupModal(name.c_str(), &open, flags);
}

bool AS_ImGui_BeginPopupContextItem(const std::string& str_id, int mouse_button) {
    return ImGui::BeginPopupContextItem(str_id.c_str(), mouse_button);
}

bool AS_ImGui_BeginPopupContextWindow(bool also_over_items, int mouse_button) {
    return ImGui::BeginPopupContextWindow(NULL, also_over_items, mouse_button);
}

bool AS_ImGui_BeginPopupContextWindow2(bool also_over_items, const std::string& str_id, int mouse_button) {
    return ImGui::BeginPopupContextWindow(str_id.c_str(), also_over_items, mouse_button);
}

bool AS_ImGui_BeginPopupContextVoid(int mouse_button) {
    return ImGui::BeginPopupContextVoid(NULL, mouse_button);
}

bool AS_ImGui_BeginPopupContextVoid2(const std::string& str_id, int mouse_button) {
    return ImGui::BeginPopupContextVoid(str_id.c_str(), mouse_button);
}

void AS_ImGui_EndPopup() {
    ImGui::EndPopup();
}

bool AS_ImGui_IsPopupOpen(const std::string& str_id) {
    return ImGui::IsPopupOpen(str_id.c_str());
}

void AS_ImGui_CloseCurrentPopup() {
    ImGui::CloseCurrentPopup();
}

bool AS_ImGui_IsItemHovered() {
    return ImGui::IsItemHovered();
}

bool AS_ImGui_IsItemHoveredRect() {
    return ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly);
}

bool AS_ImGui_IsItemActive() {
    return ImGui::IsItemActive();
}

bool AS_ImGui_IsItemClicked(int mouse_button) {
    return ImGui::IsItemClicked(mouse_button);
}

bool AS_ImGui_IsItemVisible() {
    return ImGui::IsItemVisible();
}

bool AS_ImGui_IsAnyItemHovered() {
    return ImGui::IsAnyItemHovered();
}

bool AS_ImGui_IsAnyItemActive() {
    return ImGui::IsAnyItemActive();
}

vec2 AS_ImGui_GetItemRectMin() {
    ImVec2 result = ImGui::GetItemRectMin();
    return vec2(result.x, result.y);
}

vec2 AS_ImGui_GetItemRectMax() {
    ImVec2 result = ImGui::GetItemRectMax();
    return vec2(result.x, result.y);
}

vec2 AS_ImGui_GetItemRectSize() {
    ImVec2 result = ImGui::GetItemRectSize();
    return vec2(result.x, result.y);
}

void AS_ImGui_SetItemAllowOverlap() {
    ImGui::SetItemAllowOverlap();
}

bool AS_ImGui_IsWindowHovered() {
    return ImGui::IsWindowHovered();
}

bool AS_ImGui_IsWindowFocused() {
    return ImGui::IsWindowFocused();
}

bool AS_ImGui_IsRootWindowFocused() {
    return ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow);
}

bool AS_ImGui_IsRootWindowOrAnyChildFocused() {
    return ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
}

bool AS_ImGui_IsRootWindowOrAnyChildHovered() {
    return ImGui::IsWindowHovered(ImGuiFocusedFlags_RootAndChildWindows);
}

bool AS_ImGui_IsRectVisible(const vec2& size) {
    ImVec2 temp_size(size[0], size[1]);
    return ImGui::IsRectVisible(temp_size);
}

bool AS_ImGui_IsRectVisible2(const vec2& rect_min, const vec2& rect_max) {
    ImVec2 temp_rect_min(rect_min[0], rect_min[1]);
    ImVec2 temp_rect_max(rect_max[0], rect_max[1]);
    return ImGui::IsRectVisible(temp_rect_min, temp_rect_max);
}

bool AS_ImGui_WantCaptureMouse() {
    return ImGui::GetIO().WantCaptureMouse;
}

bool AS_ImGui_IsPosHoveringAnyWindow(const vec2& pos) {
    LOGW_ONCE("Don\'t use IsPosHoveringAnyWindow, use WantCaptureMouse instead. Doesn\'t work as expected? Send in a ticket");
    return AS_ImGui_WantCaptureMouse();
}

float AS_ImGui_GetTime() {
    return (float)ImGui::GetTime();
}

int AS_ImGui_GetFrameCount() {
    return ImGui::GetFrameCount();
}

std::string AS_ImGui_GetStyleColorName(int idx) {
    return std::string(ImGui::GetStyleColorName(idx));  // Note: Original returned const char*, not a copy
}
//
// vec2 AS_ImGui_CalcItemRectClosestPoint(const vec2& pos, bool on_edge, float outward) {
//    ImVec2 temp_pos(pos[0], pos[1]);
//    ImVec2 result = ImGui::CalcItemRectClosestPoint(temp_pos, on_edge, outward);
//    return vec2(result.x, result.y);
//}

vec2 AS_ImGui_CalcTextSize(const std::string& text, bool hide_text_after_double_hash, float wrap_width) {
    ImVec2 result = ImGui::CalcTextSize(text.c_str(), NULL, hide_text_after_double_hash, wrap_width);
    return vec2(result.x, result.y);
}

void AS_ImGui_CalcListClipping(int items_count, float items_height, int& out_items_display_start, int& out_items_display_end) {
    return ImGui::CalcListClipping(items_count, items_height, &out_items_display_start, &out_items_display_end);
}

bool AS_ImGui_BeginChildFrame(unsigned int id, const vec2& size, int extra_flags) {
    ImVec2 temp_size(size[0], size[1]);
    return ImGui::BeginChildFrame(id, temp_size, extra_flags);
}

void AS_ImGui_EndChildFrame() {
    return ImGui::EndChildFrame();
}

vec4 AS_ImGui_ColorConvertU32ToFloat4(unsigned int value) {
    ImVec4 result = ImGui::ColorConvertU32ToFloat4(value);
    return vec4(result.x, result.y, result.z, result.w);
}

unsigned int AS_ImGui_ColorConvertFloat4ToU32(const vec4& value) {
    ImVec4 temp_value(value[0], value[1], value[2], value[3]);
    return ImGui::ColorConvertFloat4ToU32(temp_value);
}

void AS_ImGui_ColorConvertRGBtoHSV(float r, float g, float b, float& out_h, float& out_s, float& out_v) {
    ImGui::ColorConvertRGBtoHSV(r, g, b, out_h, out_s, out_v);
}

void AS_ImGui_ColorConvertHSVtoRGB(float h, float s, float v, float& out_r, float& out_g, float& out_b) {
    ImGui::ColorConvertHSVtoRGB(h, s, v, out_r, out_g, out_b);
}

int AS_ImGui_GetKeyIndex(int imgui_key) {
    return ImGui::GetKeyIndex(imgui_key);
}

bool AS_ImGui_IsKeyDown(int key_index) {
    return ImGui::IsKeyDown(key_index);
}

bool AS_ImGui_IsKeyPressed(int key_index, bool repeat) {
    return ImGui::IsKeyPressed(key_index, repeat);
}

bool AS_ImGui_IsKeyReleased(int key_index) {
    return ImGui::IsKeyReleased(key_index);
}

bool AS_ImGui_IsMouseDown(int button) {
    return ImGui::IsMouseDown(button);
}

bool AS_ImGui_IsMouseClicked(int button, bool repeat) {
    return ImGui::IsMouseClicked(button, repeat);
}

bool AS_ImGui_IsMouseDoubleClicked(int button) {
    return ImGui::IsMouseDoubleClicked(button);
}

bool AS_ImGui_IsMouseReleased(int button) {
    return ImGui::IsMouseReleased(button);
}

bool AS_ImGui_IsMouseHoveringWindow() {
    return ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
}

bool AS_ImGui_IsMouseHoveringAnyWindow() {
    return ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
}

bool AS_ImGui_IsMouseHoveringRect(const vec2& r_min, const vec2& r_max, bool clip) {
    ImVec2 temp_r_min(r_min[0], r_min[1]);
    ImVec2 temp_r_max(r_max[0], r_max[1]);
    return ImGui::IsMouseHoveringRect(temp_r_min, temp_r_max, clip);
}

bool AS_ImGui_IsMouseDragging(int button, float lock_threshold) {
    return ImGui::IsMouseDragging(button, lock_threshold);
}

vec2 AS_ImGui_GetMousePos() {
    ImVec2 result = ImGui::GetMousePos();
    return vec2(result.x, result.y);
}

vec2 AS_ImGui_GetMousePosOnOpeningCurrentPopup() {
    ImVec2 result = ImGui::GetMousePosOnOpeningCurrentPopup();
    return vec2(result.x, result.y);
}

vec2 AS_ImGui_GetMouseDragDelta(int button, float lock_threshold) {
    ImVec2 result = ImGui::GetMouseDragDelta(button, lock_threshold);
    return vec2(result.x, result.y);
}

void AS_ImGui_ResetMouseDragDelta(int button) {
    ImGui::ResetMouseDragDelta(button);
}

int AS_ImGui_GetMouseCursor() {
    return ImGui::GetMouseCursor();
}

void AS_ImGui_SetMouseCursor(int type) {
    ImGui::SetMouseCursor(type);
}

void AS_ImGui_CaptureKeyboardFromApp(bool capture) {
    ImGui::CaptureKeyboardFromApp(capture);
}

void AS_ImGui_CaptureMouseFromApp(bool capture) {
    ImGui::CaptureMouseFromApp(capture);
}

std::string AS_ImGui_GetClipboardText() {
    return std::string(ImGui::GetClipboardText());
}

void AS_ImGui_SetClipboardText(const std::string& text) {
    ImGui::SetClipboardText(text.c_str());
}

bool ImGui_TryGetWindowDrawList(ImDrawList** draw_list) {
    bool result = false;
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    if (window && window->DrawList) {
        *draw_list = window->DrawList;
        result = true;
    }

    return result;
}

void AS_ImDrawList_AddLine(const vec2& a, const vec2& b, uint32_t col, float thickness) {
    ImDrawList* draw_list;

    if (ImGui_TryGetWindowDrawList(&draw_list)) {
        draw_list->AddLine(ImVec2(a[0], a[1]), ImVec2(b[0], b[1]), col, thickness);
    }
}

void AS_ImDrawList_AddRect(const vec2& a, const vec2& b, uint32_t col, float rounding, int rounding_corners_flags, float thickness) {
    ImDrawList* draw_list;

    if (ImGui_TryGetWindowDrawList(&draw_list)) {
        draw_list->AddRect(ImVec2(a[0], a[1]), ImVec2(b[0], b[1]), col, rounding, rounding_corners_flags, thickness);
    }
}

void AS_ImDrawList_AddRectFilled(const vec2& a, const vec2& b, uint32_t col, float rounding, int rounding_corners_flags) {
    ImDrawList* draw_list;

    if (ImGui_TryGetWindowDrawList(&draw_list)) {
        draw_list->AddRectFilled(ImVec2(a[0], a[1]), ImVec2(b[0], b[1]), col, rounding, rounding_corners_flags);
    }
}

void AS_ImDrawList_AddRectFilledMultiColor(const vec2& a, const vec2& b, uint32_t col_upr_left, uint32_t col_upr_right, uint32_t col_bot_right, uint32_t col_bot_left) {
    ImDrawList* draw_list;

    if (ImGui_TryGetWindowDrawList(&draw_list)) {
        draw_list->AddRectFilledMultiColor(ImVec2(a[0], a[1]), ImVec2(b[0], b[1]), col_upr_left, col_upr_right, col_bot_right, col_bot_left);
    }
}

void AS_ImDrawList_AddQuad(const vec2& a, const vec2& b, const vec2& c, const vec2& d, uint32_t col, float thickness) {
    ImDrawList* draw_list;

    if (ImGui_TryGetWindowDrawList(&draw_list)) {
        draw_list->AddQuad(ImVec2(a[0], a[1]), ImVec2(b[0], b[1]), ImVec2(c[0], c[1]), ImVec2(d[0], d[1]), col, thickness);
    }
}

void AS_ImDrawList_AddQuadFilled(const vec2& a, const vec2& b, const vec2& c, const vec2& d, uint32_t col) {
    ImDrawList* draw_list;

    if (ImGui_TryGetWindowDrawList(&draw_list)) {
        draw_list->AddQuadFilled(ImVec2(a[0], a[1]), ImVec2(b[0], b[1]), ImVec2(c[0], c[1]), ImVec2(d[0], d[1]), col);
    }
}

void AS_ImDrawList_AddTriangle(const vec2& a, const vec2& b, const vec2& c, uint32_t col, float thickness) {
    ImDrawList* draw_list;

    if (ImGui_TryGetWindowDrawList(&draw_list)) {
        draw_list->AddTriangle(ImVec2(a[0], a[1]), ImVec2(b[0], b[1]), ImVec2(c[0], c[1]), col, thickness);
    }
}

void AS_ImDrawList_AddTriangleFilled(const vec2& a, const vec2& b, const vec2& c, uint32_t col) {
    ImDrawList* draw_list;

    if (ImGui_TryGetWindowDrawList(&draw_list)) {
        draw_list->AddTriangleFilled(ImVec2(a[0], a[1]), ImVec2(b[0], b[1]), ImVec2(c[0], c[1]), col);
    }
}

void AS_ImDrawList_AddCircle(const vec2& center, float radius, uint32_t col, int num_segments, float thickness) {
    ImDrawList* draw_list;

    if (ImGui_TryGetWindowDrawList(&draw_list)) {
        draw_list->AddCircle(ImVec2(center[0], center[1]), radius, col, num_segments, thickness);
    }
}

void AS_ImDrawList_AddCircleFilled(const vec2& center, float radius, uint32_t col, int num_segments) {
    ImDrawList* draw_list;

    if (ImGui_TryGetWindowDrawList(&draw_list)) {
        draw_list->AddCircleFilled(ImVec2(center[0], center[1]), radius, col, num_segments);
    }
}

void AS_ImDrawList_AddText(const vec2& pos, uint32_t col, const std::string& text) {
    ImDrawList* draw_list;

    if (ImGui_TryGetWindowDrawList(&draw_list)) {
        draw_list->AddText(ImVec2(pos[0], pos[1]), col, text.c_str());
    }
}

void AS_ImDrawList_AddImage(const TextureAssetRef& user_texture_ref, const vec2& a, const vec2& b, const vec2& uv_a, const vec2& uv_b, uint32_t tint_color) {
    ImDrawList* draw_list;

    if (user_texture_ref.valid() && ImGui_TryGetWindowDrawList(&draw_list)) {
        Textures::Instance()->EnsureInVRAM(user_texture_ref);
        draw_list->AddImage(
            reinterpret_cast<ImTextureID>(Textures::Instance()->returnTexture(user_texture_ref)),
            ImVec2(a[0], a[1]),
            ImVec2(b[0], b[1]),
            ImVec2(uv_a[0], uv_a[1]),
            ImVec2(uv_b[0], uv_b[1]),
            tint_color);
    }
}

void AS_ImDrawList_AddImageQuad(const TextureAssetRef& user_texture_ref, const vec2& a, const vec2& b, const vec2& c, const vec2& d, const vec2& uv_a, const vec2& uv_b, const vec2& uv_c, const vec2& uv_d, uint32_t tint_color) {
    ImDrawList* draw_list;

    if (user_texture_ref.valid() && ImGui_TryGetWindowDrawList(&draw_list)) {
        Textures::Instance()->EnsureInVRAM(user_texture_ref);
        draw_list->AddImageQuad(
            reinterpret_cast<ImTextureID>(Textures::Instance()->returnTexture(user_texture_ref)),
            ImVec2(a[0], a[1]),
            ImVec2(b[0], b[1]),
            ImVec2(c[0], c[1]),
            ImVec2(d[0], d[1]),
            ImVec2(uv_a[0], uv_a[1]),
            ImVec2(uv_b[0], uv_b[1]),
            ImVec2(uv_c[0], uv_c[1]),
            ImVec2(uv_d[0], uv_d[1]),
            tint_color);
    }
}

void AS_ImDrawList_AddImageRounded(const TextureAssetRef& user_texture_ref, const vec2& a, const vec2& b, const vec2& uv_a, const vec2& uv_b, uint32_t tint_color, float rounding, int rounding_corners) {
    ImDrawList* draw_list;

    if (user_texture_ref.valid() && ImGui_TryGetWindowDrawList(&draw_list)) {
        Textures::Instance()->EnsureInVRAM(user_texture_ref);
        draw_list->AddImageRounded(
            reinterpret_cast<ImTextureID>(Textures::Instance()->returnTexture(user_texture_ref)),
            ImVec2(a[0], a[1]),
            ImVec2(b[0], b[1]),
            ImVec2(uv_a[0], uv_a[1]),
            ImVec2(uv_b[0], uv_b[1]),
            tint_color,
            rounding,
            rounding_corners);
    }
}

void AS_ImDrawList_AddBezierCurve(const vec2& pos0, const vec2& cp0, const vec2& cp1, const vec2& pos1, uint32_t col, float thickness, int num_segments) {
    ImDrawList* draw_list;

    if (ImGui_TryGetWindowDrawList(&draw_list)) {
        draw_list->AddBezierCurve(ImVec2(pos0[0], pos0[1]), ImVec2(cp0[0], cp0[1]), ImVec2(cp1[0], cp1[1]), ImVec2(pos1[0], pos1[1]), col, thickness, num_segments);
    }
}

void AS_ImDrawList_PathClear() {
    ImDrawList* draw_list;

    if (ImGui_TryGetWindowDrawList(&draw_list)) {
        draw_list->PathClear();
    }
}

void AS_ImDrawList_PathLineTo(const vec2& pos) {
    ImDrawList* draw_list;

    if (ImGui_TryGetWindowDrawList(&draw_list)) {
        draw_list->PathLineTo(ImVec2(pos[0], pos[1]));
    }
}

void AS_ImDrawList_PathLineToMergeDuplicate(const vec2& pos) {
    ImDrawList* draw_list;

    if (ImGui_TryGetWindowDrawList(&draw_list)) {
        draw_list->PathLineToMergeDuplicate(ImVec2(pos[0], pos[1]));
    }
}

void AS_ImDrawList_PathFillConvex(uint32_t col) {
    ImDrawList* draw_list;

    if (ImGui_TryGetWindowDrawList(&draw_list)) {
        draw_list->PathFillConvex(col);
    }
}

void AS_ImDrawList_PathStroke(uint32_t col, bool closed, float thickness) {
    ImDrawList* draw_list;

    if (ImGui_TryGetWindowDrawList(&draw_list)) {
        draw_list->PathStroke(col, closed, thickness);
    }
}

void AS_ImDrawList_PathArcTo(const vec2& center, float radius, float a_min, float a_max, int num_segments) {
    ImDrawList* draw_list;

    if (ImGui_TryGetWindowDrawList(&draw_list)) {
        draw_list->PathArcTo(ImVec2(center[0], center[1]), radius, a_min, a_max, num_segments);
    }
}

void AS_ImDrawList_PathArcToFast(const vec2& center, float radius, int a_min_of_12, int a_max_of_12) {
    ImDrawList* draw_list;

    if (ImGui_TryGetWindowDrawList(&draw_list)) {
        draw_list->PathArcToFast(ImVec2(center[0], center[1]), radius, a_min_of_12, a_max_of_12);
    }
}

void AS_ImDrawList_PathBezierCurveTo(const vec2& p1, const vec2& p2, const vec2& p3, int num_segments) {
    ImDrawList* draw_list;

    if (ImGui_TryGetWindowDrawList(&draw_list)) {
        draw_list->PathBezierCurveTo(ImVec2(p1[0], p1[1]), ImVec2(p2[0], p2[1]), ImVec2(p3[0], p3[1]), num_segments);
    }
}

void AS_ImDrawList_PathRect(const vec2& rect_min, const vec2& rect_max, float rounding, int rounding_corners_flags) {
    ImDrawList* draw_list;

    if (ImGui_TryGetWindowDrawList(&draw_list)) {
        draw_list->PathRect(ImVec2(rect_min[0], rect_min[1]), ImVec2(rect_max[0], rect_max[1]), rounding, rounding_corners_flags);
    }
}

void AS_ImGui_DrawSettings() {
    DrawSettingsImGui(the_scenegraph, IMGST_WINDOW);
}

std::string GetUserPickedWritePath(const std::string& suffix, const std::string& default_path) {
    const int BUF_SIZE = 512;
    char buf[BUF_SIZE];
    Dialog::DialogErr err = Dialog::writeFile(suffix.c_str(), 1, default_path.c_str(), buf, BUF_SIZE);
    if (!err) {
        return std::string(buf);
    } else {
        return std::string("");
    }
}

std::string GetUserPickedReadPath(const std::string& suffix, const std::string& default_path) {
    Input::Instance()->ignore_mouse_frame = true;
    const int BUF_SIZE = 512;
    char buffer[BUF_SIZE];
    Dialog::DialogErr err = Dialog::readFile(suffix.c_str(), 1, default_path.c_str(), buffer, BUF_SIZE);

    if (err != Dialog::NO_ERR) {
        LOGE << Dialog::DialogErrString(err) << std::endl;
        return std::string("");
    } else {
        std::string shortened = FindShortestPath(std::string(buffer));
        return shortened;
    }
}

std::string AS_FindShortestPath(const std::string& path) {
    return FindShortestPath2(path).GetOriginalPathStr();
}

std::string AS_FindFilePath(const std::string& path) {
    return FindFilePath(path).GetFullPathStr();
}

// Register Dear ImGui functions to be accessible to an angelscript context
void AttachIMGUI(ASContext* context) {
    // TextureLoadFlags - This is a phoenix engine thing, not a Dear ImGui thing, but is necessary in order to feed images to Dear ImGui
    context->RegisterEnum("TextureLoadFlags");
    context->RegisterEnumValue("TextureLoadFlags", "TextureLoadFlags_SRGB", PX_SRGB);
    context->RegisterEnumValue("TextureLoadFlags", "TextureLoadFlags_NoMipmap", PX_NOMIPMAP);
    context->RegisterEnumValue("TextureLoadFlags", "TextureLoadFlags_NoConvert", PX_NOCONVERT);
    context->RegisterEnumValue("TextureLoadFlags", "TextureLoadFlags_NoLiveUpdate", PX_NOLIVEUPDATE);
    context->RegisterEnumValue("TextureLoadFlags", "TextureLoadFlags_NoReduce", PX_NOREDUCE);

    // TextureAssetRef - This is a phoenix engine thing, not a Dear ImGui thing, but is necessary in order to feed images to Dear ImGui
    context->RegisterObjectType("TextureAssetRef", sizeof(TextureAssetRef), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
    context->RegisterObjectBehaviour("TextureAssetRef", asBEHAVE_CONSTRUCT, "void TextureAssetRef()", asFUNCTION(AS_TextureAssetRefConstructor), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectBehaviour("TextureAssetRef", asBEHAVE_CONSTRUCT, "void TextureAssetRef(const TextureAssetRef &in other)", asFUNCTION(AS_TextureAssetRefCopyConstructor), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectBehaviour("TextureAssetRef", asBEHAVE_DESTRUCT, "void TextureAssetRef()", asFUNCTION(AS_TextureAssetRefDestructor), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("TextureAssetRef", "bool IsValid()", asMETHOD(TextureAssetRef, valid), asCALL_THISCALL);
    context->RegisterObjectMethod("TextureAssetRef", "void Clear()", asMETHOD(TextureAssetRef, clear), asCALL_THISCALL);
    context->RegisterObjectMethod("TextureAssetRef", "TextureAssetRef& opAssign(const TextureAssetRef &in other)", asFUNCTION(AS_TextureAssetRefOpAssign), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("TextureAssetRef", "TextureAssetRef& opEquals(const TextureAssetRef &in other)", asFUNCTION(AS_TextureAssetRefOpEquals), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("TextureAssetRef", "TextureAssetRef& opCmp(const TextureAssetRef &in other)", asFUNCTION(AS_TextureAssetRefOpCmp), asCALL_CDECL_OBJFIRST);

    context->RegisterGlobalFunction("TextureAssetRef LoadTexture(const string &in name, uint32 texture_load_flags = 0)", asFUNCTION(AS_LoadTexture), asCALL_CDECL);

    // Window
    context->RegisterGlobalFunction("bool ImGui_Begin(const string &in name, int flags = 0)", asFUNCTION(AS_ImGui_Begin), asCALL_CDECL, "bool ImGui_Begin(const string &in name, ImGuiWindowFlags flags = 0)");
    context->RegisterGlobalFunction("bool ImGui_Begin(const string &in name, bool &inout is_open, int flags = 0)", asFUNCTION(AS_ImGui_Begin2), asCALL_CDECL, "bool ImGui_Begin(const string &in name, bool &inout is_open, ImGuiWindowFlags flags = 0)");
    context->RegisterGlobalFunction("void ImGui_End()", asFUNCTION(AS_ImGui_End), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_BeginChild(const string &in str_id, const vec2 &in size = vec2(0,0), bool border = false, int extra_flags = 0)", asFUNCTION(AS_ImGui_BeginChild), asCALL_CDECL, "bool ImGui_BeginChild(const string &in str_id, const vec2 &in size = vec2(0,0), bool border = false, ImGuiWindowFlags extra_flags = 0)");
    context->RegisterGlobalFunction("bool ImGui_BeginChild(uint id, const vec2 &in size = vec2(0,0), bool border = false, int extra_flags = 0)", asFUNCTION(AS_ImGui_BeginChild2), asCALL_CDECL, "bool ImGui_BeginChild(uint id, const vec2 &in size = vec2(0,0), bool border = false, ImGuiWindowFlags extra_flags = 0)");
    context->RegisterGlobalFunction("void ImGui_EndChild()", asFUNCTION(AS_ImGui_EndChild), asCALL_CDECL);
    context->RegisterGlobalFunction("vec2 ImGui_GetContentRegionMax()", asFUNCTION(AS_ImGui_GetContentRegionMax), asCALL_CDECL);
    context->RegisterGlobalFunction("vec2 ImGui_GetContentRegionAvail()", asFUNCTION(AS_ImGui_GetContentRegionAvail), asCALL_CDECL);
    context->RegisterGlobalFunction("float ImGui_GetContentRegionAvailWidth()", asFUNCTION(AS_ImGui_GetContentRegionAvailWidth), asCALL_CDECL);
    context->RegisterGlobalFunction("vec2 ImGui_GetWindowContentRegionMin()", asFUNCTION(AS_ImGui_GetWindowContentRegionMin), asCALL_CDECL);
    context->RegisterGlobalFunction("vec2 ImGui_GetWindowContentRegionMax()", asFUNCTION(AS_ImGui_GetWindowContentRegionMax), asCALL_CDECL);
    context->RegisterGlobalFunction("float ImGui_GetWindowContentRegionWidth()", asFUNCTION(AS_ImGui_GetWindowContentRegionWidth), asCALL_CDECL);
    context->RegisterGlobalFunction("vec2 ImGui_GetWindowPos()", asFUNCTION(AS_ImGui_GetWindowPos), asCALL_CDECL);
    context->RegisterGlobalFunction("vec2 ImGui_GetWindowSize()", asFUNCTION(AS_ImGui_GetWindowSize), asCALL_CDECL);
    context->RegisterGlobalFunction("float ImGui_GetWindowWidth()", asFUNCTION(AS_ImGui_GetWindowWidth), asCALL_CDECL);
    context->RegisterGlobalFunction("float ImGui_GetWindowHeight()", asFUNCTION(AS_ImGui_GetWindowHeight), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsWindowCollapsed()", asFUNCTION(AS_ImGui_IsWindowCollapsed), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsWindowAppearing()", asFUNCTION(AS_ImGui_IsWindowAppearing), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_SetWindowFontScale(float scale)", asFUNCTION(AS_ImGui_SetWindowFontScale), asCALL_CDECL);

    context->RegisterGlobalFunction("void ImGui_SetNextWindowPos(const vec2 &in pos, int cond = 0)", asFUNCTION(AS_ImGui_SetNextWindowPos), asCALL_CDECL, "void ImGui_SetNextWindowPos(const vec2 &in pos, ImGuiSetCond cond = 0)");
    context->RegisterGlobalFunction("void ImGui_SetNextWindowPosCenter(int cond = 0)", asFUNCTION(AS_ImGui_SetNextWindowPosCenter), asCALL_CDECL, "void ImGui_SetNextWindowPosCenter(ImGuiSetCond cond = 0)");
    context->RegisterGlobalFunction("void ImGui_SetNextWindowSize(const vec2 &in size, int cond = 0)", asFUNCTION(AS_ImGui_SetNextWindowSize), asCALL_CDECL, "void ImGui_SetNextWindowSize(const vec2 &in size, ImGuiSetCond cond = 0)");
    context->RegisterGlobalFunction("void ImGui_SetNextWindowContentSize(const vec2 &in size)", asFUNCTION(AS_ImGui_SetNextWindowContentSize), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_SetNextWindowContentWidth(float width)", asFUNCTION(AS_ImGui_SetNextWindowContentWidth), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_SetNextWindowCollapsed(bool collapsed, int cond = 0)", asFUNCTION(AS_ImGui_SetNextWindowCollapsed), asCALL_CDECL, "void ImGui_SetNextWindowCollapsed(bool collapsed, ImGuiSetCond cond = 0)");
    context->RegisterGlobalFunction("void ImGui_SetNextWindowFocus()", asFUNCTION(AS_ImGui_SetNextWindowFocus), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_SetWindowPos(const vec2 &in pos, int cond = 0)", asFUNCTION(AS_ImGui_SetWindowPos), asCALL_CDECL, "void ImGui_SetWindowPos(const vec2 &in pos, ImGuiSetCond cond = 0)");
    context->RegisterGlobalFunction("void ImGui_SetWindowSize(const vec2 &in size, int cond = 0)", asFUNCTION(AS_ImGui_SetWindowSize), asCALL_CDECL, "void ImGui_SetWindowSize(const vec2 &in size, ImGuiSetCond cond = 0)");
    context->RegisterGlobalFunction("void ImGui_SetWindowCollapsed(bool collapsed, int cond = 0)", asFUNCTION(AS_ImGui_SetWindowCollapsed), asCALL_CDECL, "void ImGui_SetWindowCollapsed(bool collapsed, ImGuiSetCond cond = 0)");
    context->RegisterGlobalFunction("void ImGui_SetWindowFocus()", asFUNCTION(AS_ImGui_SetWindowFocus), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_SetWindowPos(const string &in name, const vec2 &in pos, int cond = 0)", asFUNCTION(AS_ImGui_SetWindowPos2), asCALL_CDECL, "void ImGui_SetWindowPos(const string &in name, const vec2 &in pos, ImGuiSetCond cond = 0)");
    context->RegisterGlobalFunction("void ImGui_SetWindowSize(const string &in name, const vec2 &in size, int cond = 0)", asFUNCTION(AS_ImGui_SetWindowSize2), asCALL_CDECL, "void ImGui_SetWindowSize(const string &in name, const vec2 &in size, ImGuiSetCond cond = 0)");
    context->RegisterGlobalFunction("void ImGui_SetWindowCollapsed(const string &in name, bool collapsed, int cond = 0)", asFUNCTION(AS_ImGui_SetWindowCollapsed2), asCALL_CDECL, "void ImGui_SetWindowCollapsed(const string &in name, bool collapsed, ImGuiSetCond cond = 0)");
    context->RegisterGlobalFunction("void ImGui_SetWindowFocus(const string &in name)", asFUNCTION(AS_ImGui_SetWindowFocus2), asCALL_CDECL);

    context->RegisterGlobalFunction("float ImGui_GetScrollX()", asFUNCTION(AS_ImGui_GetScrollX), asCALL_CDECL);
    context->RegisterGlobalFunction("float ImGui_GetScrollY()", asFUNCTION(AS_ImGui_GetScrollY), asCALL_CDECL);
    context->RegisterGlobalFunction("float ImGui_GetScrollMaxX()", asFUNCTION(AS_ImGui_GetScrollMaxX), asCALL_CDECL);
    context->RegisterGlobalFunction("float ImGui_GetScrollMaxY()", asFUNCTION(AS_ImGui_GetScrollMaxY), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_SetScrollX(float scroll_x)", asFUNCTION(AS_ImGui_SetScrollX), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_SetScrollY(float scroll_y)", asFUNCTION(AS_ImGui_SetScrollY), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_SetScrollHere(float center_y_ratio = 0.5f)", asFUNCTION(AS_ImGui_SetScrollHere), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_SetScrollFromPosY(float pos_y, float center_y_ratio = 0.5f)", asFUNCTION(AS_ImGui_SetScrollFromPosY), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_SetKeyboardFocusHere(int offset = 0)", asFUNCTION(AS_ImGui_SetKeyboardFocusHere), asCALL_CDECL);

    // Parameters stacks (shared)
    context->RegisterGlobalFunction("void ImGui_PushStyleColor(int idx, const vec4 &in col)", asFUNCTION(AS_ImGui_PushStyleColor), asCALL_CDECL, "void ImGui_PushStyleColor(ImGuiCol idx, const vec4 &in col)");
    context->RegisterGlobalFunction("void ImGui_PopStyleColor(int count = 1)", asFUNCTION(AS_ImGui_PopStyleColor), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_PushStyleVar(int idx, float val)", asFUNCTION(AS_ImGui_PushStyleVar), asCALL_CDECL, "void ImGui_PushStyleVar(ImGuiStyleVar idx, float val)");
    context->RegisterGlobalFunction("void ImGui_PushStyleVar(int idx, const vec2 &in val)", asFUNCTION(AS_ImGui_PushStyleVar2), asCALL_CDECL, "void ImGui_PushStyleVar(ImGuiStyleVar idx, const vec2 &in val)");
    context->RegisterGlobalFunction("void ImGui_PushStyleVar(int idx, bool val)", asFUNCTION(AS_ImGui_PushStyleVar3), asCALL_CDECL, "void ImGui_PushStyleVar(ImGuiStyleVar idx, bool val)");
    context->RegisterGlobalFunction("void ImGui_PushStyleVar(int idx, int val)", asFUNCTION(AS_ImGui_PushStyleVar4), asCALL_CDECL, "void ImGui_PushStyleVar(ImGuiStyleVar idx, int val)");
    context->RegisterGlobalFunction("void ImGui_BeginDisabled(bool is_disabled)", asFUNCTION(AS_ImGui_BeginDisabled), asCALL_CDECL, "void ImGui_BeginDisabled(bool is_disabled)");
    context->RegisterGlobalFunction("void ImGui_EndDisabled()", asFUNCTION(AS_ImGui_EndDisabled), asCALL_CDECL, "void ImGui_EndDisabled()");
    context->RegisterGlobalFunction("void ImGui_PopStyleVar(int count = 1)", asFUNCTION(AS_ImGui_PopStyleVar), asCALL_CDECL);
    context->RegisterGlobalFunction("vec4 ImGui_GetStyleColorVec4(int idx)", asFUNCTION(AS_ImGui_GetStyleColorVec4), asCALL_CDECL, "string ImGui_GetStyleColName(ImGuiCol idx)");  // Note: Original returned const char*, not a copy
    context->RegisterGlobalFunction("uint32 ImGui_GetColorU32(uint32 idx, float alpha_mul = 1.0f)", asFUNCTION(AS_ImGui_GetColorU32), asCALL_CDECL, "uint32 GetColorU32(ImGuiCol idx, float alpha_mul = 1.0f)");
    context->RegisterGlobalFunction("uint32 ImGui_GetColorU32(const vec3 &in col)", asFUNCTION(AS_ImGui_GetColorU32_2), asCALL_CDECL);
    context->RegisterGlobalFunction("uint32 ImGui_GetColorU32(const vec4 &in col)", asFUNCTION(AS_ImGui_GetColorU32_3), asCALL_CDECL);
    context->RegisterGlobalFunction("uint32 ImGui_GetColorU32(float r, float g, float b, float alpha_mul = 1.0f)", asFUNCTION(AS_ImGui_GetColorU32_4), asCALL_CDECL);

    // Parameters stacks (current window)
    context->RegisterGlobalFunction("void ImGui_PushItemWidth(float item_width)", asFUNCTION(AS_ImGui_PushItemWidth), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_PopItemWidth()", asFUNCTION(AS_ImGui_PopItemWidth), asCALL_CDECL);
    context->RegisterGlobalFunction("float ImGui_CalcItemWidth()", asFUNCTION(AS_ImGui_CalcItemWidth), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_PushTextWrapPos(float wrap_pos_x = 0.0f)", asFUNCTION(AS_ImGui_PushTextWrapPos), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_PopTextWrapPos()", asFUNCTION(AS_ImGui_PopTextWrapPos), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_PushAllowKeyboardFocus(bool allow_keyboard_focus)", asFUNCTION(AS_ImGui_PushAllowKeyboardFocus), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_PopAllowKeyboardFocus()", asFUNCTION(AS_ImGui_PopAllowKeyboardFocus), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_PushButtonRepeat(bool repeat)", asFUNCTION(AS_ImGui_PushButtonRepeat), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_PopButtonRepeat()", asFUNCTION(AS_ImGui_PopButtonRepeat), asCALL_CDECL);

    // Cursor / Layout
    context->RegisterGlobalFunction("void ImGui_Separator()", asFUNCTION(AS_ImGui_Separator), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_SameLine(float pos_x = 0.0, float spacing_w = -1.0)", asFUNCTION(AS_ImGui_SameLine), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_NewLine()", asFUNCTION(AS_ImGui_NewLine), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_Spacing()", asFUNCTION(AS_ImGui_Spacing), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_Dummy(const vec2 &in size)", asFUNCTION(AS_ImGui_Dummy), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_Indent(float indent_w = 0.0f)", asFUNCTION(AS_ImGui_Indent), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_Unindent(float indent_w = 0.0f)", asFUNCTION(AS_ImGui_Unindent), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_BeginGroup()", asFUNCTION(AS_ImGui_BeginGroup), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_EndGroup()", asFUNCTION(AS_ImGui_EndGroup), asCALL_CDECL);
    context->RegisterGlobalFunction("vec2 ImGui_GetCursorPos()", asFUNCTION(AS_ImGui_GetCursorPos), asCALL_CDECL);
    context->RegisterGlobalFunction("float ImGui_GetCursorPosX()", asFUNCTION(AS_ImGui_GetCursorPosX), asCALL_CDECL);
    context->RegisterGlobalFunction("float ImGui_GetCursorPosY()", asFUNCTION(AS_ImGui_GetCursorPosY), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_SetCursorPos(const vec2 &in local_pos)", asFUNCTION(AS_ImGui_SetCursorPos), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_SetCursorPosX(float x)", asFUNCTION(AS_ImGui_SetCursorPosX), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_SetCursorPosY(float y)", asFUNCTION(AS_ImGui_SetCursorPosY), asCALL_CDECL);
    context->RegisterGlobalFunction("vec2 ImGui_GetCursorStartPos()", asFUNCTION(AS_ImGui_GetCursorStartPos), asCALL_CDECL);
    context->RegisterGlobalFunction("vec2 ImGui_GetCursorScreenPos()", asFUNCTION(AS_ImGui_GetCursorScreenPos), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_SetCursorScreenPos(const vec2 &in pos)", asFUNCTION(AS_ImGui_SetCursorScreenPos), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_AlignFirstTextHeightToWidgets()", asFUNCTION(AS_ImGui_AlignTextToFramePadding), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_AlignTextToFramePadding()", asFUNCTION(AS_ImGui_AlignTextToFramePadding), asCALL_CDECL);
    context->RegisterGlobalFunction("float ImGui_GetTextLineHeight()", asFUNCTION(AS_ImGui_GetTextLineHeight), asCALL_CDECL);
    context->RegisterGlobalFunction("float ImGui_GetTextLineHeightWithSpacing()", asFUNCTION(AS_ImGui_GetTextLineHeightWithSpacing), asCALL_CDECL);
    context->RegisterGlobalFunction("float ImGui_GetItemsLineHeightWithSpacing()", asFUNCTION(AS_ImGui_GetFrameHeightWithSpacing), asCALL_CDECL);
    context->RegisterGlobalFunction("float ImGui_GetFrameHeight()", asFUNCTION(AS_ImGui_GetFrameHeight), asCALL_CDECL);
    context->RegisterGlobalFunction("float ImGui_GetFrameHeightWithSpacing()", asFUNCTION(AS_ImGui_GetFrameHeightWithSpacing), asCALL_CDECL);

    // Columns
    context->RegisterGlobalFunction("bool ImGui_Columns(int count = 1, bool border = true)", asFUNCTION(AS_ImGui_Columns), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_Columns(int count, const string &in id, bool border = true)", asFUNCTION(AS_ImGui_Columns2), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_NextColumn()", asFUNCTION(AS_ImGui_NextColumn), asCALL_CDECL);
    context->RegisterGlobalFunction("int ImGui_GetColumnIndex()", asFUNCTION(AS_ImGui_GetColumnIndex), asCALL_CDECL);
    context->RegisterGlobalFunction("float ImGui_GetColumnOffset(int column_index = -1)", asFUNCTION(AS_ImGui_GetColumnOffset), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_SetColumnOffset(int column_index, float offset_x)", asFUNCTION(AS_ImGui_SetColumnOffset), asCALL_CDECL);
    context->RegisterGlobalFunction("float ImGui_GetColumnWidth(int column_index = -1)", asFUNCTION(AS_ImGui_GetColumnWidth), asCALL_CDECL);
    context->RegisterGlobalFunction("float ImGui_SetColumnWidth(int column_index, float width)", asFUNCTION(AS_ImGui_SetColumnWidth), asCALL_CDECL);
    context->RegisterGlobalFunction("int ImGui_GetColumnsCount()", asFUNCTION(AS_ImGui_GetColumnsCount), asCALL_CDECL);

    // ID scopes
    context->RegisterGlobalFunction("void ImGui_PushID(const string &in str)", asFUNCTION(AS_ImGui_PushID), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_PushID(const ? &in ptr_id)", asFUNCTION(AS_ImGui_PushID2), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_PushID(int int_id)", asFUNCTION(AS_ImGui_PushID3), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_PopID()", asFUNCTION(AS_ImGui_PopID), asCALL_CDECL);
    context->RegisterGlobalFunction("uint ImGui_GetID(const string &in str_id)", asFUNCTION(AS_ImGui_GetID), asCALL_CDECL);
    context->RegisterGlobalFunction("uint ImGui_GetID(const ? &in ptr_id)", asFUNCTION(AS_ImGui_GetID2), asCALL_CDECL);

    // Widgets: Text
    context->RegisterGlobalFunction("void ImGui_Text(const string &in label)", asFUNCTION(AS_ImGui_Text), asCALL_CDECL, "Note: label is not a format string, unlike in Dear ImGui. Use string concatenation instead");
    context->RegisterGlobalFunction("void ImGui_TextColored(const vec4 &in color, const string &in label)", asFUNCTION(AS_ImGui_TextColored), asCALL_CDECL, "Note: label is not a format string, unlike in Dear ImGui. Use string concatenation instead");
    context->RegisterGlobalFunction("void ImGui_TextDisabled(const string &in label)", asFUNCTION(AS_ImGui_TextDisabled), asCALL_CDECL, "Note: label is not a format string, unlike in Dear ImGui. Use string concatenation instead");
    context->RegisterGlobalFunction("void ImGui_TextWrapped(const string &in label)", asFUNCTION(AS_ImGui_TextWrapped), asCALL_CDECL, "Note: label is not a format string, unlike in Dear ImGui. Use string concatenation instead");
    context->RegisterGlobalFunction("void ImGui_LabelText(const string &in str, const string &in label)", asFUNCTION(AS_ImGui_LabelText), asCALL_CDECL, "Note: label is not a format string, unlike in Dear ImGui. Use string concatenation instead");
    context->RegisterGlobalFunction("void ImGui_Bullet()", asFUNCTION(AS_ImGui_Bullet), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_BulletText(const string &in label)", asFUNCTION(AS_ImGui_BulletText), asCALL_CDECL, "Note: label is not a format string, unlike in Dear ImGui. Use string concatenation instead");

    // Widgets: Main
    context->RegisterGlobalFunction("bool ImGui_Button(const string &in, const vec2 &in size = vec2(0,0))", asFUNCTION(AS_ImGui_Button), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_SmallButton(const string &in label)", asFUNCTION(AS_ImGui_SmallButton), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_InvisibleButton(const string &in str_id, const vec2 &in size)", asFUNCTION(AS_ImGui_InvisibleButton), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_Image(const TextureAssetRef &in texture, const vec2 &in size, const vec2 &in uv0 = vec2(0,0), const vec2 &in uv1 = vec2(1,1), const vec4 &in tint_color = vec4(1,1,1,1), const vec4 &in border_color = vec4(0,0,0,0))", asFUNCTION(AS_ImGui_Image), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_ImageButton(const TextureAssetRef &in texture, const vec2 &in size, const vec2 &in uv0 = vec2(0,0), const vec2 &in uv1 = vec2(1,1), int frame_padding = -1, const vec4 &in background_color = vec4(0,0,0,0), const vec4 &in tint_color = vec4(1,1,1,1))", asFUNCTION(AS_ImGui_ImageButton), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_Checkbox(const string &in label, bool &inout value)", asFUNCTION(AS_ImGui_Checkbox), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_CheckboxFlags(const string &in label, uint &inout flags, uint flags_value)", asFUNCTION(AS_ImGui_CheckboxFlags), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_RadioButton(const string &in label, bool active)", asFUNCTION(AS_ImGui_RadioButton), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_RadioButton(const string &in label, int &inout value, int v_button)", asFUNCTION(AS_ImGui_RadioButton2), asCALL_CDECL);

    // Widgets: Combo box
    context->RegisterGlobalFunction("bool ImGui_BeginCombo(const string &in label, const string& in preview_value, int flags = 0)", asFUNCTION(AS_ImGui_BeginCombo), asCALL_CDECL, "bool ImGui_BeginCombo(const string &in label, const string& in preview_value, ImGuiComboFlags flags = 0)");
    context->RegisterGlobalFunction("void ImGui_EndCombo()", asFUNCTION(AS_ImGui_EndCombo), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_Combo(const string &in label, int &inout current_item, const array<string> &in items, int popup_max_height_in_items = -1)", asFUNCTION(AS_ImGui_Combo), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_Combo(const string &in label, int &inout current_item, const string &in items_separated_by_zeros, int popup_max_height_in_items = -1)", asFUNCTION(AS_ImGui_Combo2), asCALL_CDECL);

    // Widgets: Drags
    context->RegisterGlobalFunction("bool ImGui_DragFloat(const string &in label, float &inout value, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const string &in display_format = \"%.3f\", float power = 1.0f)", asFUNCTION(AS_ImGui_DragFloat), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_DragFloat2(const string &in label, vec2 &inout value, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const string &in display_format = \"%.3f\", float power = 1.0f)", asFUNCTION(AS_ImGui_DragFloat2_Vector), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_DragFloat3(const string &in label, vec3 &inout value, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const string &in display_format = \"%.3f\", float power = 1.0f)", asFUNCTION(AS_ImGui_DragFloat3_Vector), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_DragFloat4(const string &in label, vec4 &inout value, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const string &in display_format = \"%.3f\", float power = 1.0f)", asFUNCTION(AS_ImGui_DragFloat4_Vector), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_DragFloatRange2(const string &in label, float &inout v_current_min, float &inout v_current_max, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const string &in display_format = \"%.3f\")", asFUNCTION(AS_ImGui_DragFloatRange2_Vector), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_DragFloatRange2(const string &in label, float &inout v_current_min, float &inout v_current_max, float v_speed, float v_min, float v_max, const string &in display_format, const string &in display_format_max, float power = 1.0f)", asFUNCTION(AS_ImGui_DragFloatRange2_Vector2), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_DragInt(const string &in label, int &inout value, float v_speed = 1.0f, int v_min = 0, int v_max = 0, const string &in display_format = \"%.0f\")", asFUNCTION(AS_ImGui_DragInt), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_DragInt2(const string &in label, ivec2 &inout value, float v_speed = 1.0f, int v_min = 0, int v_max = 0, const string &in display_format = \"%.0f\")", asFUNCTION(AS_ImGui_DragInt2_Vector), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_DragInt3(const string &in label, ivec3 &inout value, float v_speed = 1.0f, int v_min = 0, int v_max = 0, const string &in display_format = \"%.0f\")", asFUNCTION(AS_ImGui_DragInt3_Vector), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_DragInt4(const string &in label, ivec4 &inout value, float v_speed = 1.0f, int v_min = 0, int v_max = 0, const string &in display_format = \"%.0f\")", asFUNCTION(AS_ImGui_DragInt4_Vector), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_DragIntRange2(const string &in label, int &inout v_current_min, int &inout v_current_max, float v_speed = 1.0f, int v_min = 0.0f, int v_max = 0.0f, const string &in display_format = \"%.3f\")", asFUNCTION(AS_ImGui_DragIntRange2_Vector), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_DragIntRange2(const string &in label, int &inout v_current_min, int &inout v_current_max, float v_speed, int v_min, int v_max, const string &in display_format, const string &in display_format_max)", asFUNCTION(AS_ImGui_DragIntRange2_Vector2), asCALL_CDECL);

    // Widgets: Input with Keyboard
    context->RegisterGlobalFunction("bool ImGui_InputText(const string &in label, int flags = 0)", asFUNCTION(AS_ImGui_InputText), asCALL_CDECL, "bool ImGui_InputText(const string &in label, ImGuiInputTextFlags flags = 0)");
    context->RegisterGlobalFunction("bool ImGui_InputText(const string &in label, string &inout text_buffer, int buffer_size, int flags = 0)", asFUNCTION(AS_ImGui_InputText2), asCALL_CDECL, "bool ImGui_InputText(const string &in label, string &inout text_buffer, int buffer_size, ImGuiInputTextFlags flags = 0)");
    context->RegisterGlobalFunction("bool ImGui_InputTextMultiline(const string &in label, const vec2 &in size = vec2(0,0), int flags = 0)", asFUNCTION(AS_ImGui_InputTextMultiline), asCALL_CDECL, "bool ImGui_InputTextMultiline(const string &in label, const vec2 &in size = vec2(0,0), ImGuiInputTextFlags flags = 0)");
    context->RegisterGlobalFunction("bool ImGui_InputTextMultiline(const string &in label, string &inout text_buffer, int buffer_size, const vec2 &in size = vec2(0,0), int flags = 0)", asFUNCTION(AS_ImGui_InputTextMultiline2), asCALL_CDECL, "bool ImGui_InputTextMultiline(const string &in label, string &inout text_buffer, int buffer_size, const vec2 &in size = vec2(0,0), ImGuiInputTextFlags flags = 0)");
    context->RegisterGlobalFunction("bool ImGui_InputFloat(const string &in label, float &inout value, float step = 0.0f, float step_fast = 0.0f, int decimal_precision = -1, int extra_flags = 0)", asFUNCTION(AS_ImGui_InputFloat), asCALL_CDECL, "bool ImGui_InputFloat(const string &in label, float &inout value, float step = 0.0f, float step_fast = 0.0f, int decimal_precision = -1, ImGuiInputTextFlags extra_flags = 0)");
    context->RegisterGlobalFunction("bool ImGui_InputFloat2(const string &in label, vec2 &inout value, int decimal_precision = -1, int extra_flags = 0)", asFUNCTION(AS_ImGui_InputFloat2_Vector), asCALL_CDECL, "bool ImGui_InputFloat2(const string &in label, vec2 &inout value, int decimal_precision = -1, ImGuiInputTextFlags extra_flags = 0)");
    context->RegisterGlobalFunction("bool ImGui_InputFloat3(const string &in label, vec3 &inout value, int decimal_precision = -1, int extra_flags = 0)", asFUNCTION(AS_ImGui_InputFloat3_Vector), asCALL_CDECL, "bool ImGui_InputFloat3(const string &in label, vec3 &inout value, int decimal_precision = -1, ImGuiInputTextFlags extra_flags = 0)");
    context->RegisterGlobalFunction("bool ImGui_InputFloat4(const string &in label, vec4 &inout value, int decimal_precision = -1, int extra_flags = 0)", asFUNCTION(AS_ImGui_InputFloat4_Vector), asCALL_CDECL, "bool ImGui_InputFloat4(const string &in label, vec4 &inout value, int decimal_precision = -1, ImGuiInputTextFlags extra_flags = 0)");
    context->RegisterGlobalFunction("bool ImGui_InputInt(const string &in label, int &inout value, int step = 1, int step_fast = 100, int extra_flags = 0)", asFUNCTION(AS_ImGui_InputInt), asCALL_CDECL, "bool ImGui_InputInt(const string &in label, int &inout value, int step = 1, int step_fast = 100, ImGuiInputTextFlags extra_flags = 0)");

    // Widgets:: Sliders
    context->RegisterGlobalFunction("bool ImGui_SliderFloat(const string &in label, float &inout v, float v_min, float v_max, const string &in display_format = \"%.3f\", float power = 1.0f)", asFUNCTION(AS_ImGui_SliderFloat), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_SliderFloat2(const string &in label, vec2 &inout v, float v_min, float v_max, const string &in display_format = \"%.3f\", float power = 1.0f)", asFUNCTION(AS_ImGui_SliderFloat2), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_SliderFloat3(const string &in label, vec3 &inout v, float v_min, float v_max, const string &in display_format = \"%.3f\", float power = 1.0f)", asFUNCTION(AS_ImGui_SliderFloat3), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_SliderFloat4(const string &in label, vec4 &inout v, float v_min, float v_max, const string &in display_format = \"%.3f\", float power = 1.0f)", asFUNCTION(AS_ImGui_SliderFloat4), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_SliderAngle(const string &in label, float &inout v_rad, float v_degrees_min = -360.0f, float v_degrees_max = +360.0f)", asFUNCTION(AS_ImGui_SliderAngle), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_SliderInt(const string &in label, int &inout v, int v_min, int v_max, const string &in display_format = \"%.0f\")", asFUNCTION(AS_ImGui_SliderInt), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_SliderInt2(const string &in label, ivec2 &inout v, int v_min, int v_max, const string &in display_format = \"%.0f\")", asFUNCTION(AS_ImGui_SliderInt2), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_SliderInt3(const string &in label, ivec3 &inout v, int v_min, int v_max, const string &in display_format = \"%.0f\")", asFUNCTION(AS_ImGui_SliderInt3), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_SliderInt4(const string &in label, ivec4 &inout v, int v_min, int v_max, const string &in display_format = \"%.0f\")", asFUNCTION(AS_ImGui_SliderInt4), asCALL_CDECL);

    // Widgets: Color Editor/Picker
    context->RegisterGlobalFunction("bool ImGui_ColorEdit3(const string &in label, vec3 &inout color, int flags = 0)", asFUNCTION(AS_ImGui_ColorEdit3), asCALL_CDECL, "bool ImGui_ColorEdit3(const string &in label, vec3 &inout color, ImGuiColorEditFlags flags = 0");
    context->RegisterGlobalFunction("bool ImGui_ColorEdit4(const string &in label, vec4 &inout color, bool show_alpha = true)", asFUNCTION(AS_ImGui_ColorEdit4), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_ColorEdit4(const string &in label, vec4 &inout color, int flags)", asFUNCTION(AS_ImGui_ColorEdit4_2), asCALL_CDECL, "void ImGui_ColorEdit4(const string in& label, vec4 &inout color, ImGuiColorEditFlags flags = 0");
    context->RegisterGlobalFunction("bool ImGui_ColorPicker3(const string &in label, vec3 &inout color, int flags = 0)", asFUNCTION(AS_ImGui_ColorPicker3), asCALL_CDECL, "bool ImGui_ColorPicker3(const string &in label, vec3 &inout color, ImGuiColorEditFlags flags = 0)");
    context->RegisterGlobalFunction("bool ImGui_ColorPicker4(const string &in label, vec4 &inout color, int flags = 0)", asFUNCTION(AS_ImGui_ColorPicker4), asCALL_CDECL, "bool ImGui_ColorPicker4(const string &in label, vec4 &inout color, ImGuiColorEditFlags flags = 0)");
    context->RegisterGlobalFunction("bool ImGui_ColorPicker4(const string &in label, vec4 &inout color, int flags, vec4 ref_col)", asFUNCTION(AS_ImGui_ColorPicker4_2), asCALL_CDECL, "bool ImGui_ColorPicker4(const string &in label, vec4 &inout color, ImGuiColorEditFlags flags, const vec4 &in ref_color)");
    context->RegisterGlobalFunction("bool ImGui_ColorButton(const vec4 &in col, bool small_height = false, bool outline_border = true)", asFUNCTION(AS_ImGui_ColorButton), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_ColorButton(const string &in desc, const vec4 &in col, int flags = 0, const vec2 &in size = vec2(0, 0))", asFUNCTION(AS_ImGui_ColorButton2), asCALL_CDECL, "bool ImGui_ColorButton(const string &in desc, const vec4 &in col, ImGuiColorEditFlags flags = 0, const vec2 &in size = vec2(0, 0)");
    context->RegisterGlobalFunction("void ImGui_ColorEditMode(int)", asFUNCTION(AS_ImGui_ColorEditMode), asCALL_CDECL, "void ImGui_ColorEditMode(ImGuiColorEditMode mode)");

    // Widgets: Trees
    context->RegisterGlobalFunction("bool ImGui_TreeNode(const string &in label)", asFUNCTION(AS_ImGui_TreeNode), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_TreeNode(const string &in str_id, const string &in label)", asFUNCTION(AS_ImGui_TreeNode2), asCALL_CDECL, "Note: label is not a format string, unlike in Dear ImGui. Use string concatenation instead");
    context->RegisterGlobalFunction("bool ImGui_TreeNode(const ? &in ptr_id, const string &in label)", asFUNCTION(AS_ImGui_TreeNode3), asCALL_CDECL, "Note: label is not a format string, unlike in Dear ImGui. Use string concatenation instead");
    context->RegisterGlobalFunction("bool ImGui_TreeNodeEx(const string &in label, int flags = 0)", asFUNCTION(AS_ImGui_TreeNodeEx), asCALL_CDECL, "bool ImGui_TreeNodeEx(const string &in label, ImGuiTreeNodeFlags_ flags = 0)");
    context->RegisterGlobalFunction("bool ImGui_TreeNodeEx(const string &in str_id, int flags, const string &in label)", asFUNCTION(AS_ImGui_TreeNodeEx2), asCALL_CDECL, "bool ImGui_TreeNodeEx(const string &in str_id, ImGuiTreeNodeFlags_ flags, const string &in label) - Note: label is not a format string, unlike in Dear ImGui. Use string concatenation instead");
    context->RegisterGlobalFunction("bool ImGui_TreeNodeEx(const ? &in ptr_id, int flags, const string &in label)", asFUNCTION(AS_ImGui_TreeNodeEx3), asCALL_CDECL, "bool ImGui_TreeNodeEx(const ? &in ptr_id, ImGuiTreeNodeFlags_ flags, const string &in label) - Note: label is not a format string, unlike in Dear ImGui. Use string concatenation instead");
    context->RegisterGlobalFunction("void ImGui_TreePush()", asFUNCTION(AS_ImGui_TreePush), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_TreePush(const string &in str_id)", asFUNCTION(AS_ImGui_TreePush2), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_TreePush(const ? &in ptr_id)", asFUNCTION(AS_ImGui_TreePush3), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_TreePop()", asFUNCTION(AS_ImGui_TreePop), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_TreeAdvanceToLabelPos()", asFUNCTION(AS_ImGui_TreeAdvanceToLabelPos), asCALL_CDECL);
    context->RegisterGlobalFunction("float ImGui_GetTreeNodeToLabelSpacing()", asFUNCTION(AS_ImGui_GetTreeNodeToLabelSpacing), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_SetNextTreeNodeOpen(bool is_open, int cond = 0)", asFUNCTION(AS_ImGui_SetNextTreeNodeOpen), asCALL_CDECL, "void ImGui_SetNextTreeNodeOpen(bool is_open, ImGuiSetCond cond = 0)");
    context->RegisterGlobalFunction("bool ImGui_CollapsingHeader(const string &in label, int flags = 0)", asFUNCTION(AS_ImGui_CollapsingHeader), asCALL_CDECL, "bool ImGui_CollapsingHeader(const string &in label, ImGuiTreeNodeFlags flags = 0)");
    context->RegisterGlobalFunction("bool ImGui_CollapsingHeader(const string &in label, bool &inout is_open, int flags = 0)", asFUNCTION(AS_ImGui_CollapsingHeader2), asCALL_CDECL, "bool ImGui_CollapsingHeader(const string &in label, bool &inout is_open, ImGuiTreeNodeFlags flags = 0)");

    // Widgets: Selectable / Lists
    context->RegisterGlobalFunction("bool ImGui_Selectable(const string &in label, bool selected = false, int flags = 0, const vec2 &in size = vec2(0,0))", asFUNCTION(AS_ImGui_Selectable), asCALL_CDECL, "ImGui_Selectable(const string &in label, bool selected = false, ImGuiSelectableFlags flags = 0, const vec2& size = vec2(0,0))");
    context->RegisterGlobalFunction("bool ImGui_SelectableToggle(const string &in label, bool &inout selected, int flags = 0, const vec2 &in size = vec2(0,0))", asFUNCTION(AS_ImGui_SelectableToggle), asCALL_CDECL, "ImGui_SelectableToggle(const string &in label, bool &inout selected, ImGuiSelectableFlags flags = 0, const vec2& size = vec2(0,0))");
    context->RegisterGlobalFunction("bool ImGui_ListBox(const string &in label, int &inout current_item, const array<string> &in items, int height_in_items = -1)", asFUNCTION(AS_ImGui_ListBox), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_ListBoxHeader(const string &in label)", asFUNCTION(AS_ImGui_ListBoxHeader), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_ListBoxHeader(const string &in label, const vec2 &in size)", asFUNCTION(AS_ImGui_ListBoxHeader2), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_ListBoxHeader(const string &in label, int items_count, int height_in_items = -1)", asFUNCTION(AS_ImGui_ListBoxHeader3), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_ListBoxFooter()", asFUNCTION(AS_ImGui_ListBoxFooter), asCALL_CDECL);

    // Tooltips
    context->RegisterGlobalFunction("void ImGui_SetTooltip(const string &in text)", asFUNCTION(AS_ImGui_SetTooltip), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_BeginTooltip()", asFUNCTION(AS_ImGui_BeginTooltip), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_EndTooltip()", asFUNCTION(AS_ImGui_EndTooltip), asCALL_CDECL);

    // Menus
    context->RegisterGlobalFunction("bool ImGui_BeginMenuBar()", asFUNCTION(AS_ImGui_BeginMenuBar), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_EndMenuBar()", asFUNCTION(AS_ImGui_EndMenuBar), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_BeginMenu(const string &in label, bool enabled = true)", asFUNCTION(AS_ImGui_BeginMenu), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_EndMenu()", asFUNCTION(AS_ImGui_EndMenu), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_MenuItem(const string &in label, bool selected = false, bool enabled = true)", asFUNCTION(AS_ImGui_MenuItem), asCALL_CDECL);

    // Popups
    context->RegisterGlobalFunction("void ImGui_OpenPopup(const string &in popup_id)", asFUNCTION(AS_ImGui_OpenPopup), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_OpenPopupOnItemClick(const string &in popup_id, int mouse_button = 1)", asFUNCTION(AS_ImGui_OpenPopupOnItemClick), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_BeginPopup(const string &in popup_id)", asFUNCTION(AS_ImGui_BeginPopup), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_BeginPopupModal(const string &in name, int extra_flags = 0)", asFUNCTION(AS_ImGui_BeginPopupModal), asCALL_CDECL, "bool ImGui_BeginPopupModal(const string &in name, ImGuiWindowFlags extra_flags = 0)");
    context->RegisterGlobalFunction("bool ImGui_BeginPopupModal(const string &in name, bool &inout is_open, int extra_flags = 0)", asFUNCTION(AS_ImGui_BeginPopupModal2), asCALL_CDECL, "bool ImGui_BeginPopupModal(const string &in name, bool &inout is_open, ImGuiWindowFlags extra_flags = 0)");
    context->RegisterGlobalFunction("bool ImGui_BeginPopupContextItem(const string &in popup_id, int mouse_button = 1)", asFUNCTION(AS_ImGui_BeginPopupContextItem), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_BeginPopupContextWindow(bool also_over_items = true, int mouse_button = 1)", asFUNCTION(AS_ImGui_BeginPopupContextWindow), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_BeginPopupContextWindow(bool also_over_items, const string &in popup_id, int mouse_button = 1)", asFUNCTION(AS_ImGui_BeginPopupContextWindow2), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_BeginPopupContextVoid(int mouse_button = 1)", asFUNCTION(AS_ImGui_BeginPopupContextVoid), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_BeginPopupContextVoid(const string &in popup_id, int mouse_button = 1)", asFUNCTION(AS_ImGui_BeginPopupContextVoid2), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_EndPopup()", asFUNCTION(AS_ImGui_EndPopup), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsPopupOpen(const string &in str_id)", asFUNCTION(AS_ImGui_IsPopupOpen), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_CloseCurrentPopup()", asFUNCTION(AS_ImGui_CloseCurrentPopup), asCALL_CDECL);

    // Utilities
    context->RegisterGlobalFunction("bool ImGui_IsItemHovered()", asFUNCTION(AS_ImGui_IsItemHovered), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsItemHoveredRect()", asFUNCTION(AS_ImGui_IsItemHoveredRect), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsItemActive()", asFUNCTION(AS_ImGui_IsItemActive), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsItemClicked(int mouse_button = 0)", asFUNCTION(AS_ImGui_IsItemClicked), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsItemVisible()", asFUNCTION(AS_ImGui_IsItemVisible), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsAnyItemHovered()", asFUNCTION(AS_ImGui_IsAnyItemHovered), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsAnyItemActive()", asFUNCTION(AS_ImGui_IsAnyItemActive), asCALL_CDECL);
    context->RegisterGlobalFunction("vec2 ImGui_GetItemRectMin()", asFUNCTION(AS_ImGui_GetItemRectMin), asCALL_CDECL);
    context->RegisterGlobalFunction("vec2 ImGui_GetItemRectMax()", asFUNCTION(AS_ImGui_GetItemRectMax), asCALL_CDECL);
    context->RegisterGlobalFunction("vec2 ImGui_GetItemRectSize()", asFUNCTION(AS_ImGui_GetItemRectSize), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_SetItemAllowOverlap()", asFUNCTION(AS_ImGui_SetItemAllowOverlap), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsWindowHovered()", asFUNCTION(AS_ImGui_IsWindowHovered), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsWindowFocused()", asFUNCTION(AS_ImGui_IsWindowFocused), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsRootWindowFocused()", asFUNCTION(AS_ImGui_IsRootWindowFocused), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsRootWindowOrAnyChildFocused()", asFUNCTION(AS_ImGui_IsRootWindowOrAnyChildFocused), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsRootWindowOrAnyChildHovered()", asFUNCTION(AS_ImGui_IsRootWindowOrAnyChildHovered), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsRectVisible(const vec2 &in size)", asFUNCTION(AS_ImGui_IsRectVisible), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsRectVisible(const vec2 &in rect_min, const vec2 &in rect_max)", asFUNCTION(AS_ImGui_IsRectVisible2), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsPosHoveringAnyWindow(const vec2 &in pos)", asFUNCTION(AS_ImGui_IsPosHoveringAnyWindow), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_WantCaptureMouse()", asFUNCTION(AS_ImGui_WantCaptureMouse), asCALL_CDECL);
    context->RegisterGlobalFunction("float ImGui_GetTime()", asFUNCTION(AS_ImGui_GetTime), asCALL_CDECL);
    context->RegisterGlobalFunction("int ImGui_GetFrameCount()", asFUNCTION(AS_ImGui_GetFrameCount), asCALL_CDECL);
    context->RegisterGlobalFunction("string ImGui_GetStyleColName(int idx)", asFUNCTION(AS_ImGui_GetStyleColorName), asCALL_CDECL, "string ImGui_GetStyleColName(ImGuiCol idx)");      // Note: Original returned const char*, not a copy
    context->RegisterGlobalFunction("string ImGui_GetStyleColorName(int idx)", asFUNCTION(AS_ImGui_GetStyleColorName), asCALL_CDECL, "string ImGui_GetStyleColorName(ImGuiCol idx)");  // Note: Original returned const char*, not a copy
    // context->RegisterGlobalFunction("vec2 ImGui_CalcItemRectClosestPoint(const vec2 &in pos, bool on_edge = false, float outward = 0.0f)", asFUNCTION(AS_ImGui_CalcItemRectClosestPoint), asCALL_CDECL);
    context->RegisterGlobalFunction("vec2 ImGui_CalcTextSize(const string &in text, bool hide_text_after_double_hash = false, float wrap_width = -1.0f)", asFUNCTION(AS_ImGui_CalcTextSize), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_CalcListClipping(int items_count, float items_height, int &out out_items_display_start, int &out out_items_display_end)", asFUNCTION(AS_ImGui_CalcListClipping), asCALL_CDECL);

    context->RegisterGlobalFunction("bool ImGui_BeginChildFrame(uint id, const vec2 &in size, int extra_flags = 0)", asFUNCTION(AS_ImGui_BeginChildFrame), asCALL_CDECL, "bool ImGui_BeginChildFrame(uint id, const vec2 &in size, ImGuiWindowFlags extra_flags = 0)");
    context->RegisterGlobalFunction("void ImGui_EndChildFrame()", asFUNCTION(AS_ImGui_EndChildFrame), asCALL_CDECL);

    context->RegisterGlobalFunction("vec4 ImGui_ColorConvertU32ToFloat4(uint value)", asFUNCTION(AS_ImGui_ColorConvertU32ToFloat4), asCALL_CDECL);
    context->RegisterGlobalFunction("uint ImGui_ColorConvertFloat4ToU32(const vec4 &in value)", asFUNCTION(AS_ImGui_ColorConvertFloat4ToU32), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_ColorConvertRGBtoHSV(float r, float g, float b, float &out out_h, float &out out_s, float &out out_v)", asFUNCTION(AS_ImGui_ColorConvertRGBtoHSV), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_ColorConvertHSVtoRGB(float h, float s, float v, float &out out_r, float &out out_g, float &out out_b)", asFUNCTION(AS_ImGui_ColorConvertHSVtoRGB), asCALL_CDECL);

    // Inputs
    context->RegisterGlobalFunction("int ImGui_GetKeyIndex(int imgui_key)", asFUNCTION(AS_ImGui_GetKeyIndex), asCALL_CDECL, "int ImGui_GetKeyIndex(ImGuiKey key)");
    context->RegisterGlobalFunction("bool ImGui_IsKeyDown(int key_index)", asFUNCTION(AS_ImGui_IsKeyDown), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsKeyPressed(int key_index, bool repeat = true)", asFUNCTION(AS_ImGui_IsKeyPressed), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsKeyReleased(int key_index)", asFUNCTION(AS_ImGui_IsKeyReleased), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsMouseDown(int button)", asFUNCTION(AS_ImGui_IsMouseDown), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsMouseClicked(int button, bool repeat = false)", asFUNCTION(AS_ImGui_IsMouseClicked), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsMouseDoubleClicked(int button)", asFUNCTION(AS_ImGui_IsMouseDoubleClicked), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsMouseReleased(int button)", asFUNCTION(AS_ImGui_IsMouseReleased), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsMouseHoveringWindow()", asFUNCTION(AS_ImGui_IsMouseHoveringWindow), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsMouseHoveringAnyWindow()", asFUNCTION(AS_ImGui_IsMouseHoveringAnyWindow), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsMouseHoveringRect(const vec2 &in r_min, const vec2 &in r_max, bool clip = true)", asFUNCTION(AS_ImGui_IsMouseHoveringRect), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ImGui_IsMouseDragging(int button = 0, float lock_threshold = -1.0f)", asFUNCTION(AS_ImGui_IsMouseDragging), asCALL_CDECL);
    context->RegisterGlobalFunction("vec2 ImGui_GetMousePos()", asFUNCTION(AS_ImGui_GetMousePos), asCALL_CDECL);
    context->RegisterGlobalFunction("vec2 ImGui_GetMousePosOnOpeningCurrentPopup()", asFUNCTION(AS_ImGui_GetMousePosOnOpeningCurrentPopup), asCALL_CDECL);
    context->RegisterGlobalFunction("vec2 ImGui_GetMouseDragDelta(int button = 0, float lock_threshold = -1.0f)", asFUNCTION(AS_ImGui_GetMouseDragDelta), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_ResetMouseDragDelta(int button = 0)", asFUNCTION(AS_ImGui_ResetMouseDragDelta), asCALL_CDECL);
    context->RegisterGlobalFunction("int ImGui_GetMouseCursor()", asFUNCTION(AS_ImGui_GetMouseCursor), asCALL_CDECL, "ImGuiMouseCursor ImGui_GetMouseCursor()");
    context->RegisterGlobalFunction("void ImGui_SetMouseCursor(int type)", asFUNCTION(AS_ImGui_SetMouseCursor), asCALL_CDECL, "void ImGui_SetMouseCursor(ImGuiMouseCursor type)");
    context->RegisterGlobalFunction("void ImGui_CaptureKeyboardFromApp(bool capture = true)", asFUNCTION(AS_ImGui_CaptureKeyboardFromApp), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_CaptureMouseFromApp(bool capture = true)", asFUNCTION(AS_ImGui_CaptureMouseFromApp), asCALL_CDECL);

    // Helpers functions to access functions pointers in ImGui::GetIO()
    context->RegisterGlobalFunction("string ImGui_GetClipboardText()", asFUNCTION(AS_ImGui_GetClipboardText), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_SetClipboardText(const string &in value)", asFUNCTION(AS_ImGui_SetClipboardText), asCALL_CDECL);

    // Flags for ImGui::Begin()
    context->RegisterEnum("ImGuiWindowFlags_");
    context->RegisterEnumValue("ImGuiWindowFlags_", "ImGuiWindowFlags_NoTitleBar", ImGuiWindowFlags_NoTitleBar);
    context->RegisterEnumValue("ImGuiWindowFlags_", "ImGuiWindowFlags_NoResize", ImGuiWindowFlags_NoResize);
    context->RegisterEnumValue("ImGuiWindowFlags_", "ImGuiWindowFlags_NoMove", ImGuiWindowFlags_NoMove);
    context->RegisterEnumValue("ImGuiWindowFlags_", "ImGuiWindowFlags_NoScrollbar", ImGuiWindowFlags_NoScrollbar);
    context->RegisterEnumValue("ImGuiWindowFlags_", "ImGuiWindowFlags_NoScrollWithMouse", ImGuiWindowFlags_NoScrollWithMouse);
    context->RegisterEnumValue("ImGuiWindowFlags_", "ImGuiWindowFlags_NoCollapse", ImGuiWindowFlags_NoCollapse);
    context->RegisterEnumValue("ImGuiWindowFlags_", "ImGuiWindowFlags_AlwaysAutoResize", ImGuiWindowFlags_AlwaysAutoResize);
    context->RegisterEnumValue("ImGuiWindowFlags_", "ImGuiWindowFlags_NoSavedSettings", ImGuiWindowFlags_NoSavedSettings);
    context->RegisterEnumValue("ImGuiWindowFlags_", "ImGuiWindowFlags_NoInputs", ImGuiWindowFlags_NoInputs);
    context->RegisterEnumValue("ImGuiWindowFlags_", "ImGuiWindowFlags_MenuBar", ImGuiWindowFlags_MenuBar);
    context->RegisterEnumValue("ImGuiWindowFlags_", "ImGuiWindowFlags_HorizontalScrollbar", ImGuiWindowFlags_HorizontalScrollbar);
    context->RegisterEnumValue("ImGuiWindowFlags_", "ImGuiWindowFlags_NoFocusOnAppearing", ImGuiWindowFlags_NoFocusOnAppearing);
    context->RegisterEnumValue("ImGuiWindowFlags_", "ImGuiWindowFlags_NoBringToFrontOnFocus", ImGuiWindowFlags_NoBringToFrontOnFocus);
    context->RegisterEnumValue("ImGuiWindowFlags_", "ImGuiWindowFlags_AlwaysVerticalScrollbar", ImGuiWindowFlags_AlwaysVerticalScrollbar);
    context->RegisterEnumValue("ImGuiWindowFlags_", "ImGuiWindowFlags_AlwaysHorizontalScrollbar", ImGuiWindowFlags_AlwaysHorizontalScrollbar);
    context->RegisterEnumValue("ImGuiWindowFlags_", "ImGuiWindowFlags_AlwaysUseWindowPadding", ImGuiWindowFlags_AlwaysUseWindowPadding);
    // context->RegisterEnumValue("ImGuiWindowFlags_","ImGuiWindowFlags_ResizeFromAnySide", ImGuiWindowFlags_ResizeFromAnySide); Obsolete

    // Flags for ImGui::InputText()
    context->RegisterEnum("ImGuiInputTextFlags_");
    context->RegisterEnumValue("ImGuiInputTextFlags_", "ImGuiInputTextFlags_CharsDecimal", ImGuiInputTextFlags_CharsDecimal);
    context->RegisterEnumValue("ImGuiInputTextFlags_", "ImGuiInputTextFlags_CharsHexadecimal", ImGuiInputTextFlags_CharsHexadecimal);
    context->RegisterEnumValue("ImGuiInputTextFlags_", "ImGuiInputTextFlags_CharsUppercase", ImGuiInputTextFlags_CharsUppercase);
    context->RegisterEnumValue("ImGuiInputTextFlags_", "ImGuiInputTextFlags_CharsNoBlank", ImGuiInputTextFlags_CharsNoBlank);
    context->RegisterEnumValue("ImGuiInputTextFlags_", "ImGuiInputTextFlags_AutoSelectAll", ImGuiInputTextFlags_AutoSelectAll);
    context->RegisterEnumValue("ImGuiInputTextFlags_", "ImGuiInputTextFlags_EnterReturnsTrue", ImGuiInputTextFlags_EnterReturnsTrue);
    context->RegisterEnumValue("ImGuiInputTextFlags_", "ImGuiInputTextFlags_CallbackCompletion", ImGuiInputTextFlags_CallbackCompletion);
    context->RegisterEnumValue("ImGuiInputTextFlags_", "ImGuiInputTextFlags_CallbackHistory", ImGuiInputTextFlags_CallbackHistory);
    context->RegisterEnumValue("ImGuiInputTextFlags_", "ImGuiInputTextFlags_CallbackAlways", ImGuiInputTextFlags_CallbackAlways);
    context->RegisterEnumValue("ImGuiInputTextFlags_", "ImGuiInputTextFlags_CallbackCharFilter", ImGuiInputTextFlags_CallbackCharFilter);
    context->RegisterEnumValue("ImGuiInputTextFlags_", "ImGuiInputTextFlags_AllowTabInput", ImGuiInputTextFlags_AllowTabInput);
    context->RegisterEnumValue("ImGuiInputTextFlags_", "ImGuiInputTextFlags_CtrlEnterForNewLine", ImGuiInputTextFlags_CtrlEnterForNewLine);
    context->RegisterEnumValue("ImGuiInputTextFlags_", "ImGuiInputTextFlags_NoHorizontalScroll", ImGuiInputTextFlags_NoHorizontalScroll);
    context->RegisterEnumValue("ImGuiInputTextFlags_", "ImGuiInputTextFlags_AlwaysInsertMode", ImGuiInputTextFlags_AlwaysInsertMode);
    context->RegisterEnumValue("ImGuiInputTextFlags_", "ImGuiInputTextFlags_ReadOnly", ImGuiInputTextFlags_ReadOnly);
    context->RegisterEnumValue("ImGuiInputTextFlags_", "ImGuiInputTextFlags_Password", ImGuiInputTextFlags_Password);
    context->RegisterEnumValue("ImGuiInputTextFlags_", "ImGuiInputTextFlags_NoUndoRedo", ImGuiInputTextFlags_NoUndoRedo);

    // Flags for ImGui::TreeNodeEx(), ImGui::CollapsingHeader*()
    context->RegisterEnum("ImGuiTreeNodeFlags_");
    context->RegisterEnumValue("ImGuiTreeNodeFlags_", "ImGuiTreeNodeFlags_Selected", ImGuiTreeNodeFlags_Selected);
    context->RegisterEnumValue("ImGuiTreeNodeFlags_", "ImGuiTreeNodeFlags_Framed", ImGuiTreeNodeFlags_Framed);
    context->RegisterEnumValue("ImGuiTreeNodeFlags_", "ImGuiTreeNodeFlags_AllowOverlapMode", ImGuiTreeNodeFlags_AllowItemOverlap);
    context->RegisterEnumValue("ImGuiTreeNodeFlags_", "ImGuiTreeNodeFlags_AllowItemOverlap", ImGuiTreeNodeFlags_AllowItemOverlap);
    context->RegisterEnumValue("ImGuiTreeNodeFlags_", "ImGuiTreeNodeFlags_NoTreePushOnOpen", ImGuiTreeNodeFlags_NoTreePushOnOpen);
    context->RegisterEnumValue("ImGuiTreeNodeFlags_", "ImGuiTreeNodeFlags_NoAutoOpenOnLog", ImGuiTreeNodeFlags_NoAutoOpenOnLog);
    context->RegisterEnumValue("ImGuiTreeNodeFlags_", "ImGuiTreeNodeFlags_DefaultOpen", ImGuiTreeNodeFlags_DefaultOpen);
    context->RegisterEnumValue("ImGuiTreeNodeFlags_", "ImGuiTreeNodeFlags_OpenOnDoubleClick", ImGuiTreeNodeFlags_OpenOnDoubleClick);
    context->RegisterEnumValue("ImGuiTreeNodeFlags_", "ImGuiTreeNodeFlags_OpenOnArrow", ImGuiTreeNodeFlags_OpenOnArrow);
    context->RegisterEnumValue("ImGuiTreeNodeFlags_", "ImGuiTreeNodeFlags_Leaf", ImGuiTreeNodeFlags_Leaf);
    context->RegisterEnumValue("ImGuiTreeNodeFlags_", "ImGuiTreeNodeFlags_Bullet", ImGuiTreeNodeFlags_Bullet);
    context->RegisterEnumValue("ImGuiTreeNodeFlags_", "ImGuiTreeNodeFlags_FramePadding", ImGuiTreeNodeFlags_FramePadding);
    context->RegisterEnumValue("ImGuiTreeNodeFlags_", "ImGuiTreeNodeFlags_CollapsingHeader", ImGuiTreeNodeFlags_CollapsingHeader);

    // Flags for ImGui::Selectable()
    context->RegisterEnum("ImGuiSelectableFlags_");
    context->RegisterEnumValue("ImGuiSelectableFlags_", "ImGuiSelectableFlags_DontClosePopups", ImGuiSelectableFlags_DontClosePopups);
    context->RegisterEnumValue("ImGuiSelectableFlags_", "ImGuiSelectableFlags_SpanAllColumns", ImGuiSelectableFlags_SpanAllColumns);
    context->RegisterEnumValue("ImGuiSelectableFlags_", "ImGuiSelectableFlags_AllowDoubleClick", ImGuiSelectableFlags_AllowDoubleClick);

    // Flags for ImGui::BeginCombo()
    context->RegisterEnum("ImGuiComboFlags_");
    context->RegisterEnumValue("ImGuiComboFlags_", "ImGuiComboFlags_PopupAlignLeft", ImGuiComboFlags_PopupAlignLeft);
    context->RegisterEnumValue("ImGuiComboFlags_", "ImGuiComboFlags_HeightSmall", ImGuiComboFlags_HeightSmall);
    context->RegisterEnumValue("ImGuiComboFlags_", "ImGuiComboFlags_HeightRegular", ImGuiComboFlags_HeightRegular);
    context->RegisterEnumValue("ImGuiComboFlags_", "ImGuiComboFlags_HeightLarge", ImGuiComboFlags_HeightLarge);
    context->RegisterEnumValue("ImGuiComboFlags_", "ImGuiComboFlags_HeightLargest", ImGuiComboFlags_HeightLargest);

    // Flags for ImGui::IsWindowFocused
    // ...

    // A key identifier (ImGui-side enum)
    context->RegisterEnum("ImGuiKey_");
    context->RegisterEnumValue("ImGuiKey_", "ImGuiKey_Tab", ImGuiKey_Tab);
    context->RegisterEnumValue("ImGuiKey_", "ImGuiKey_LeftArrow", ImGuiKey_LeftArrow);
    context->RegisterEnumValue("ImGuiKey_", "ImGuiKey_RightArrow", ImGuiKey_RightArrow);
    context->RegisterEnumValue("ImGuiKey_", "ImGuiKey_UpArrow", ImGuiKey_UpArrow);
    context->RegisterEnumValue("ImGuiKey_", "ImGuiKey_DownArrow", ImGuiKey_DownArrow);
    context->RegisterEnumValue("ImGuiKey_", "ImGuiKey_PageUp", ImGuiKey_PageUp);
    context->RegisterEnumValue("ImGuiKey_", "ImGuiKey_PageDown", ImGuiKey_PageDown);
    context->RegisterEnumValue("ImGuiKey_", "ImGuiKey_Home", ImGuiKey_Home);
    context->RegisterEnumValue("ImGuiKey_", "ImGuiKey_End", ImGuiKey_End);
    context->RegisterEnumValue("ImGuiKey_", "ImGuiKey_Delete", ImGuiKey_Delete);
    context->RegisterEnumValue("ImGuiKey_", "ImGuiKey_Backspace", ImGuiKey_Backspace);
    context->RegisterEnumValue("ImGuiKey_", "ImGuiKey_Enter", ImGuiKey_Enter);
    context->RegisterEnumValue("ImGuiKey_", "ImGuiKey_Escape", ImGuiKey_Escape);
    context->RegisterEnumValue("ImGuiKey_", "ImGuiKey_A", ImGuiKey_A);
    context->RegisterEnumValue("ImGuiKey_", "ImGuiKey_C", ImGuiKey_C);
    context->RegisterEnumValue("ImGuiKey_", "ImGuiKey_V", ImGuiKey_V);
    context->RegisterEnumValue("ImGuiKey_", "ImGuiKey_X", ImGuiKey_X);
    context->RegisterEnumValue("ImGuiKey_", "ImGuiKey_Y", ImGuiKey_Y);
    context->RegisterEnumValue("ImGuiKey_", "ImGuiKey_Z", ImGuiKey_Z);
    context->RegisterEnumValue("ImGuiKey_", "ImGuiKey_COUNT", ImGuiKey_COUNT);

    // Enumeration for PushStyleColor() / PopStyleColor()
    context->RegisterEnum("ImGuiCol_");
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_Text", ImGuiCol_Text);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_TextDisabled", ImGuiCol_TextDisabled);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_WindowBg", ImGuiCol_WindowBg);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_ChildWindowBg", ImGuiCol_ChildBg);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_ChildBg", ImGuiCol_ChildBg);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_PopupBg", ImGuiCol_PopupBg);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_Border", ImGuiCol_Border);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_BorderShadow", ImGuiCol_BorderShadow);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_FrameBg", ImGuiCol_FrameBg);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_FrameBgHovered", ImGuiCol_FrameBgHovered);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_FrameBgActive", ImGuiCol_FrameBgActive);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_TitleBg", ImGuiCol_TitleBg);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_TitleBgCollapsed", ImGuiCol_TitleBgCollapsed);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_TitleBgActive", ImGuiCol_TitleBgActive);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_MenuBarBg", ImGuiCol_MenuBarBg);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_ScrollbarBg", ImGuiCol_ScrollbarBg);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_ScrollbarGrab", ImGuiCol_ScrollbarGrab);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_ScrollbarGrabHovered", ImGuiCol_ScrollbarGrabHovered);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_ScrollbarGrabActive", ImGuiCol_ScrollbarGrabActive);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_ComboBg", ImGuiCol_PopupBg);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_CheckMark", ImGuiCol_CheckMark);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_SliderGrab", ImGuiCol_SliderGrab);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_SliderGrabActive", ImGuiCol_SliderGrabActive);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_Button", ImGuiCol_Button);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_ButtonHovered", ImGuiCol_ButtonHovered);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_ButtonActive", ImGuiCol_ButtonActive);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_Header", ImGuiCol_Header);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_HeaderHovered", ImGuiCol_HeaderHovered);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_HeaderActive", ImGuiCol_HeaderActive);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_Column", ImGuiCol_Separator);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_ColumnHovered", ImGuiCol_SeparatorHovered);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_ColumnActive", ImGuiCol_SeparatorActive);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_ResizeGrip", ImGuiCol_ResizeGrip);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_ResizeGripHovered", ImGuiCol_ResizeGripHovered);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_ResizeGripActive", ImGuiCol_ResizeGripActive);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_CloseButton", ImGuiCol_Button);                // Obsolete, Equal to "ImGuiCol_Button"!
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_CloseButtonHovered", ImGuiCol_ButtonHovered);  // Obsolete, Equal to "ImGuiCol_ButtonHovered"!
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_CloseButtonActive", ImGuiCol_ButtonActive);    // Obsolete, Equal to "ImGuiCol_ButtonActive"!
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_PlotLines", ImGuiCol_PlotLines);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_PlotLinesHovered", ImGuiCol_PlotLinesHovered);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_PlotHistogram", ImGuiCol_PlotHistogram);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_PlotHistogramHovered", ImGuiCol_PlotHistogramHovered);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_TextSelectedBg", ImGuiCol_TextSelectedBg);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_ModalWindowDarkening", ImGuiCol_ModalWindowDimBg);
    context->RegisterEnumValue("ImGuiCol_", "ImGuiCol_COUNT", ImGuiCol_COUNT);

    // Enumeration for PushStyleVar() / PopStyleVar()
    // NB: the enum only refers to fields of ImGuiStyle() which makes sense to be pushed/poped in UI code. Feel free to add others.
    context->RegisterEnum("ImGuiStyleVar_");
    context->RegisterEnumValue("ImGuiStyleVar_", "ImGuiStyleVar_Alpha", ImGuiStyleVar_Alpha);                        // float
    context->RegisterEnumValue("ImGuiStyleVar_", "ImGuiStyleVar_WindowPadding", ImGuiStyleVar_WindowPadding);        // ImVec2
    context->RegisterEnumValue("ImGuiStyleVar_", "ImGuiStyleVar_WindowRounding", ImGuiStyleVar_WindowRounding);      // float
    context->RegisterEnumValue("ImGuiStyleVar_", "ImGuiStyleVar_WindowMinSize", ImGuiStyleVar_WindowMinSize);        // ImVec2
    context->RegisterEnumValue("ImGuiStyleVar_", "ImGuiStyleVar_ChildWindowRounding", ImGuiStyleVar_ChildRounding);  // float
    context->RegisterEnumValue("ImGuiStyleVar_", "ImGuiStyleVar_ChildRounding", ImGuiStyleVar_ChildRounding);        // float
    context->RegisterEnumValue("ImGuiStyleVar_", "ImGuiStyleVar_FramePadding", ImGuiStyleVar_FramePadding);          // ImVec2
    context->RegisterEnumValue("ImGuiStyleVar_", "ImGuiStyleVar_FrameRounding", ImGuiStyleVar_FrameRounding);        // float
    context->RegisterEnumValue("ImGuiStyleVar_", "ImGuiStyleVar_ItemSpacing", ImGuiStyleVar_ItemSpacing);            // ImVec2
    context->RegisterEnumValue("ImGuiStyleVar_", "ImGuiStyleVar_ItemInnerSpacing", ImGuiStyleVar_ItemInnerSpacing);  // ImVec2
    context->RegisterEnumValue("ImGuiStyleVar_", "ImGuiStyleVar_IndentSpacing", ImGuiStyleVar_IndentSpacing);        // float
    context->RegisterEnumValue("ImGuiStyleVar_", "ImGuiStyleVar_GrabMinSize", ImGuiStyleVar_GrabMinSize);            // float
    context->RegisterEnumValue("ImGuiStyleVar_", "ImGuiStyleVar_ButtonTextAlign", ImGuiStyleVar_ButtonTextAlign);    // flags ImGuiAlign_*
                                                                                                                     //  context->RegisterEnumValue("ImGuiStyleVar_", "ImGuiStyleVar_Enabled", ImGuiStyleVar_Enabled);
    context->RegisterEnumValue("ImGuiStyleVar_", "ImGuiStyleVar_Count_", ImGuiStyleVar_COUNT);

    // Enumeration for EditColor4/ColorPicker4
    context->RegisterEnum("ImGuiColorEditFlags_");
    context->RegisterEnumValue("ImGuiColorEditFlags_", "ImGuiColorEditFlags_NoAlpha", ImGuiColorEditFlags_NoAlpha);
    context->RegisterEnumValue("ImGuiColorEditFlags_", "ImGuiColorEditFlags_NoPicker", ImGuiColorEditFlags_NoPicker);
    context->RegisterEnumValue("ImGuiColorEditFlags_", "ImGuiColorEditFlags_NoOptions", ImGuiColorEditFlags_NoOptions);
    context->RegisterEnumValue("ImGuiColorEditFlags_", "ImGuiColorEditFlags_NoSmallPreview", ImGuiColorEditFlags_NoSmallPreview);
    context->RegisterEnumValue("ImGuiColorEditFlags_", "ImGuiColorEditFlags_NoInputs", ImGuiColorEditFlags_NoInputs);
    context->RegisterEnumValue("ImGuiColorEditFlags_", "ImGuiColorEditFlags_NoTooltip", ImGuiColorEditFlags_NoTooltip);
    context->RegisterEnumValue("ImGuiColorEditFlags_", "ImGuiColorEditFlags_NoLabel", ImGuiColorEditFlags_NoLabel);
    context->RegisterEnumValue("ImGuiColorEditFlags_", "ImGuiColorEditFlags_NoSidePreview", ImGuiColorEditFlags_NoSidePreview);
    context->RegisterEnumValue("ImGuiColorEditFlags_", "ImGuiColorEditFlags_AlphaBar", ImGuiColorEditFlags_AlphaBar);
    context->RegisterEnumValue("ImGuiColorEditFlags_", "ImGuiColorEditFlags_AlphaPreview", ImGuiColorEditFlags_AlphaPreview);
    context->RegisterEnumValue("ImGuiColorEditFlags_", "ImGuiColorEditFlags_AlphaPreviewHalf", ImGuiColorEditFlags_AlphaPreviewHalf);
    context->RegisterEnumValue("ImGuiColorEditFlags_", "ImGuiColorEditFlags_HDR", ImGuiColorEditFlags_HDR);
    context->RegisterEnumValue("ImGuiColorEditFlags_", "ImGuiColorEditFlags_RGB", ImGuiColorEditFlags_RGB);
    context->RegisterEnumValue("ImGuiColorEditFlags_", "ImGuiColorEditFlags_HSV", ImGuiColorEditFlags_HSV);
    context->RegisterEnumValue("ImGuiColorEditFlags_", "ImGuiColorEditFlags_HEX", ImGuiColorEditFlags_HEX);
    context->RegisterEnumValue("ImGuiColorEditFlags_", "ImGuiColorEditFlags_Uint8", ImGuiColorEditFlags_Uint8);
    context->RegisterEnumValue("ImGuiColorEditFlags_", "ImGuiColorEditFlags_Float", ImGuiColorEditFlags_Float);
    context->RegisterEnumValue("ImGuiColorEditFlags_", "ImGuiColorEditFlags_PickerHueBar", ImGuiColorEditFlags_PickerHueBar);
    context->RegisterEnumValue("ImGuiColorEditFlags_", "ImGuiColorEditFlags_PickerHueWheel", ImGuiColorEditFlags_PickerHueWheel);

    // Enumeration for ColorEditMode()
    // This is from an old imgui version, but kept for backwards compatibility
    context->RegisterEnum("ImGuiColorEditMode_");
    context->RegisterEnumValue("ImGuiColorEditMode_", "ImGuiColorEditMode_UserSelect", -2);            // ImGuiColorEditMode_UserSelect
    context->RegisterEnumValue("ImGuiColorEditMode_", "ImGuiColorEditMode_UserSelectShowButton", -1);  // ImGuiColorEditMode_UserSelectShowButton
    context->RegisterEnumValue("ImGuiColorEditMode_", "ImGuiColorEditMode_RGB", 0);                    // ImGuiColorEditMode_RGB
    context->RegisterEnumValue("ImGuiColorEditMode_", "ImGuiColorEditMode_HSV", 1);                    // ImGuiColorEditMode_HSV
    context->RegisterEnumValue("ImGuiColorEditMode_", "ImGuiColorEditMode_HEX", 2);                    // ImGuiColorEditMode_HEX

    // Condition flags for ImGui::SetWindow***(), SetNextWindow***(), SetNextTreeNode***() functions
    context->RegisterEnum("ImGuiSetCond_");
    context->RegisterEnumValue("ImGuiSetCond_", "ImGuiCond_Always", ImGuiCond_Always);
    context->RegisterEnumValue("ImGuiSetCond_", "ImGuiCond_Once", ImGuiCond_Once);
    context->RegisterEnumValue("ImGuiSetCond_", "ImGuiCond_FirstUseEver", ImGuiCond_FirstUseEver);
    context->RegisterEnumValue("ImGuiSetCond_", "ImGuiCond_Appearing", ImGuiCond_Appearing);

    context->RegisterEnumValue("ImGuiSetCond_", "ImGuiSetCond_Always", ImGuiCond_Always);              // Obsolete, Equal to "ImGuiCond_Always"!
    context->RegisterEnumValue("ImGuiSetCond_", "ImGuiSetCond_Once", ImGuiCond_Once);                  // Obsolete, Equal to "ImGuiCond_Once"!
    context->RegisterEnumValue("ImGuiSetCond_", "ImGuiSetCond_FirstUseEver", ImGuiCond_FirstUseEver);  // Obsolete, Equal to "ImGuiCond_FirstUseEver"!
    context->RegisterEnumValue("ImGuiSetCond_", "ImGuiSetCond_Appearing", ImGuiCond_Appearing);        // Obsolete, Equal to "ImGuiCond_Appearing"!

    // Primitives
    context->RegisterGlobalFunction("void ImDrawList_AddLine(const vec2 &in a, const vec2 &in b, uint32 col, float thickness = 1.0f)", asFUNCTION(AS_ImDrawList_AddLine), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImDrawList_AddRect(const vec2 &in a, const vec2 &in b, uint32 col, float rounding = 0.0f, int rounding_corners_flags = 0xF, float thickness = 1.0f)", asFUNCTION(AS_ImDrawList_AddRect), asCALL_CDECL, "void ImDrawList_AddRect(const vec2 &in a, const vec2 &in b, uint32 col, float rounding = 0.0f, int rounding_corners_flags = ImDrawCornerFlags_All, float thickness = 1.0f) - a: upper-left, b: lower-right, rounding_corners_flags: 4-bits corresponding to which corner to round");
    context->RegisterGlobalFunction("void ImDrawList_AddRectFilled(const vec2 &in a, const vec2 &in b, uint32 col, float rounding = 0.0f, int rounding_corners_flags = 0xF)", asFUNCTION(AS_ImDrawList_AddRectFilled), asCALL_CDECL, "void ImDrawList_AddRectFilled(const vec2 &in a, const vec2 &in b, uint32 col, float rounding = 0.0f, int rounding_corners_flags = ImDrawCornerFlags_All) - a: upper-left, b: lower-right");
    context->RegisterGlobalFunction("void ImDrawList_AddRectFilledMultiColor(const vec2 &in a, const vec2 &in b, uint32 col_upr_left, uint32 col_upr_right, uint32 col_bot_right, uint32 col_bot_left)", asFUNCTION(AS_ImDrawList_AddRectFilledMultiColor), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImDrawList_AddQuad(const vec2 &in a, const vec2 &in b, const vec2 &in c, const vec2 &in d, uint32 col, float thickness = 1.0f)", asFUNCTION(AS_ImDrawList_AddQuad), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImDrawList_AddQuadFilled(const vec2 &in a, const vec2 &in b, const vec2 &in c, const vec2 &in d, uint32 col)", asFUNCTION(AS_ImDrawList_AddQuadFilled), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImDrawList_AddTriangle(const vec2 &in a, const vec2 &in b, const vec2 &in c, uint32 col, float thickness = 1.0f)", asFUNCTION(AS_ImDrawList_AddTriangle), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImDrawList_AddTriangleFilled(const vec2 &in a, const vec2 &in b, const vec2 &in c, uint32 col)", asFUNCTION(AS_ImDrawList_AddTriangleFilled), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImDrawList_AddCircle(const vec2 &in center, float radius, uint32 col, int num_segments = 12, float thickness = 1.0f)", asFUNCTION(AS_ImDrawList_AddCircle), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImDrawList_AddCircleFilled(const vec2 &in center, float radius, uint32 col, int num_segments = 12)", asFUNCTION(AS_ImDrawList_AddCircleFilled), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImDrawList_AddText(const vec2 &in pos, uint32 col, const string &in text)", asFUNCTION(AS_ImDrawList_AddText), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImDrawList_AddImage(const TextureAssetRef &in texture, const vec2 &in a, const vec2 &in b, const vec2 &in uv_a = vec2(0, 0), const vec2 &in uv_b = vec2(1, 1), uint32 tint_color = 0xFFFFFFFF)", asFUNCTION(AS_ImDrawList_AddImage), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImDrawList_AddImageQuad(const TextureAssetRef &in texture, const vec2 &in a, const vec2 &in b, const vec2 &in c, const vec2 &in d, const vec2 &in uv_a = vec2(0, 0), const vec2 &in uv_b = vec2(1, 0), const vec2 &in uv_c = vec2(1, 1), const vec2 &in uv_d = vec2(0, 1), uint32 tint_color = 0xFFFFFFFF)", asFUNCTION(AS_ImDrawList_AddImageQuad), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImDrawList_AddImageRounded(const TextureAssetRef &in texture, const vec2 &in a, const vec2 &in b, const vec2 &in uv_a, const vec2 &in uv_b, uint32 tint_color, float rounding, int rounding_corners = 0xF)", asFUNCTION(AS_ImDrawList_AddImageRounded), asCALL_CDECL, "void ImDrawList_AddImageRounded(const TextureAssetRef &in texture, const vec2 &in a, const vec2 &in b, const vec2 &in uv_a, const vec2 &in uv_b, uint32 tint_color, float rounding, int rounding_corners = ImDrawCornerFlags_All)");
    context->RegisterGlobalFunction("void ImDrawList_AddBezierCurve(const vec2 &in pos0, const vec2 &in cp0, const vec2 &in cp1, const vec2 &in pos1, uint32 col, float thickness, int num_segments = 0)", asFUNCTION(AS_ImDrawList_AddBezierCurve), asCALL_CDECL);

    // Stateful path API, add points then finish with PathFill() or PathStroke()
    context->RegisterGlobalFunction("void ImDrawList_PathClear()", asFUNCTION(AS_ImDrawList_PathClear), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImDrawList_PathLineTo(const vec2 &in pos)", asFUNCTION(AS_ImDrawList_PathLineTo), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImDrawList_PathLineToMergeDuplicate(const vec2 &in pos)", asFUNCTION(AS_ImDrawList_PathLineToMergeDuplicate), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImDrawList_PathFillConvex(uint32 col)", asFUNCTION(AS_ImDrawList_PathFillConvex), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImDrawList_PathStroke(uint32 col, bool closed, float thickness = 1.0f)", asFUNCTION(AS_ImDrawList_PathStroke), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImDrawList_PathArcTo(const vec2 &in center, float radius, float a_min, float a_max, int num_segments = 10)", asFUNCTION(AS_ImDrawList_PathArcTo), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImDrawList_PathArcToFast(const vec2 &in center, float radius, int a_min_of_12, int a_max_of_12)", asFUNCTION(AS_ImDrawList_PathArcToFast), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImDrawList_PathBezierCurveTo(const vec2 &in p1, const vec2 &in p2, const vec2 &in p3, int num_segments = 0)", asFUNCTION(AS_ImDrawList_PathBezierCurveTo), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImDrawList_PathRect(const vec2 &in rect_min, const vec2 &in rect_max, float rounding = 0.0f, int rounding_corners_flags = 0xF)", asFUNCTION(AS_ImDrawList_PathRect), asCALL_CDECL, "void ImDrawList_PathRect(const vec2 &in rect_min, const vec2 &in rect_max, float rounding = 0.0f, ImDrawCornerFlags rounding_corners_flags = ImDrawCornerFlags_All)");

    // flags: for ImDrawList::AddRect*() etc.
    context->RegisterEnum("ImDrawCornerFlags_");
    context->RegisterEnumValue("ImDrawCornerFlags_", "ImDrawCornerFlags_TopLeft", ImDrawCornerFlags_TopLeft);
    context->RegisterEnumValue("ImDrawCornerFlags_", "ImDrawCornerFlags_TopRight", ImDrawCornerFlags_TopRight);
    context->RegisterEnumValue("ImDrawCornerFlags_", "ImDrawCornerFlags_BotLeft", ImDrawCornerFlags_BotLeft);
    context->RegisterEnumValue("ImDrawCornerFlags_", "ImDrawCornerFlags_BotRight", ImDrawCornerFlags_BotRight);
    context->RegisterEnumValue("ImDrawCornerFlags_", "ImDrawCornerFlags_Top", ImDrawCornerFlags_Top);
    context->RegisterEnumValue("ImDrawCornerFlags_", "ImDrawCornerFlags_Bot", ImDrawCornerFlags_Bot);
    context->RegisterEnumValue("ImDrawCornerFlags_", "ImDrawCornerFlags_Left", ImDrawCornerFlags_Left);
    context->RegisterEnumValue("ImDrawCornerFlags_", "ImDrawCornerFlags_Right", ImDrawCornerFlags_Right);
    context->RegisterEnumValue("ImDrawCornerFlags_", "ImDrawCornerFlags_All", ImDrawCornerFlags_All);

    // Phoenix engine custom state
    context->RegisterGlobalFunction("string ImGui_GetTextBuf()", asFUNCTION(AS_ImGui_GetTextBuf), asCALL_CDECL);
    context->RegisterGlobalFunction("void ImGui_SetTextBuf(const string &in value)", asFUNCTION(AS_ImGui_SetTextBuf), asCALL_CDECL);
    context->RegisterGlobalProperty("int imgui_text_input_CursorPos", &imgui_text_input_CursorPos);
    context->RegisterGlobalProperty("int imgui_text_input_SelectionStart", &imgui_text_input_SelectionStart);
    context->RegisterGlobalProperty("int imgui_text_input_SelectionEnd", &imgui_text_input_SelectionEnd);

    // Phoenix engine custom functions
    context->RegisterGlobalFunction("void ImGui_DrawSettings()", asFUNCTION(AS_ImGui_DrawSettings), asCALL_CDECL);

    // TODO: Move these elsewhere!
    //       These are not Dear ImGui functions, so do not belong here
    context->RegisterGlobalFunction("string GetUserPickedWritePath(const string &in suffix, const string &in default_path)", asFUNCTION(GetUserPickedWritePath), asCALL_CDECL);
    context->RegisterGlobalFunction("string GetUserPickedReadPath(const string &in suffix, const string &in default_path)", asFUNCTION(GetUserPickedReadPath), asCALL_CDECL);

    context->RegisterGlobalFunction("string FindShortestPath(const string &in path)", asFUNCTION(AS_FindShortestPath), asCALL_CDECL);
    context->RegisterGlobalFunction("string FindFilePath(const string &in path)", asFUNCTION(AS_FindFilePath), asCALL_CDECL);
}

NavPath ASGetPath2(vec3 start, vec3 end, uint16_t inclusive_poly_flags, uint16_t exclusive_poly_flags) {
    if (the_scenegraph->GetNavMesh()) {
        return the_scenegraph->GetNavMesh()->FindPath(start, end, inclusive_poly_flags, exclusive_poly_flags);
    } else {
        NavPath path;
        path.waypoints.push_back(end);
        return path;
    }
}

NavPath ASGetPath(vec3 start, vec3 end) {
    return ASGetPath2(
        start,
        end,
        SAMPLE_POLYFLAGS_ALL,
        SAMPLE_POLYFLAGS_NONE);
}

vec3 ASNavRaycast(vec3 start, vec3 end) {
    if (the_scenegraph->GetNavMesh()) {
        return the_scenegraph->GetNavMesh()->RayCast(start, end);
    } else {
        return end;
    }
}

vec3 ASNavRaycastSlide(vec3 start, vec3 end, int depth) {
    if (the_scenegraph->GetNavMesh()) {
        return the_scenegraph->GetNavMesh()->RayCastSlide(start, end, depth);
    } else {
        return end;
    }
}

static NavPoint ASGetNavPoint(vec3 point) {
    if (the_scenegraph->GetNavMesh()) {
        return the_scenegraph->GetNavMesh()->GetNavPoint(point);
    } else {
        return NavPoint(point);
    }
}

static vec3 ASGetNavPointPos(vec3 point) {
    if (the_scenegraph->GetNavMesh()) {
        return the_scenegraph->GetNavMesh()->GetNavPoint(point).GetPoint();
    } else {
        return point;
    }
}

static void NavPathConstructor(void* memory) {
    new (memory) NavPath();
}

static void NavPathDestructor(void* memory) {
    ((NavPath*)memory)->~NavPath();
}

static const NavPath& NavPathAssign(NavPath* self, const NavPath& other) {
    return (*self) = other;
}

static void NavPathCopyConstructor(void* memory, const NavPath& other) {
    new (memory) NavPath(other);
}

static void NavPointConstructor(void* memory) {
    new (memory) NavPoint();
}

static void NavPointDestructor(void* memory) {
    ((NavPoint*)memory)->~NavPoint();
}

static const NavPoint& NavPointAssign(NavPoint* self, const NavPoint& other) {
    return (*self) = other;
}

static void NavPointCopyConstructor(void* memory, const NavPoint& other) {
    new (memory) NavPoint(other);
}

void AttachNavMesh(ASContext* context) {
    context->RegisterEnum("SamplePolyFlag");
    context->RegisterEnumValue("SamplePolyFlag", "POLYFLAGS_NONE", SAMPLE_POLYFLAGS_NONE);
    context->RegisterEnumValue("SamplePolyFlag", "POLYFLAGS_WALK", SAMPLE_POLYFLAGS_WALK);
    context->RegisterEnumValue("SamplePolyFlag", "POLYFLAGS_SWIM", SAMPLE_POLYFLAGS_SWIM);
    context->RegisterEnumValue("SamplePolyFlag", "POLYFLAGS_DOOR", SAMPLE_POLYFLAGS_DOOR);
    context->RegisterEnumValue("SamplePolyFlag", "POLYFLAGS_JUMP1", SAMPLE_POLYFLAGS_JUMP1);
    context->RegisterEnumValue("SamplePolyFlag", "POLYFLAGS_JUMP2", SAMPLE_POLYFLAGS_JUMP2);
    context->RegisterEnumValue("SamplePolyFlag", "POLYFLAGS_JUMP3", SAMPLE_POLYFLAGS_JUMP3);
    context->RegisterEnumValue("SamplePolyFlag", "POLYFLAGS_JUMP4", SAMPLE_POLYFLAGS_JUMP4);
    context->RegisterEnumValue("SamplePolyFlag", "POLYFLAGS_JUMP5", SAMPLE_POLYFLAGS_JUMP5);
    context->RegisterEnumValue("SamplePolyFlag", "POLYFLAGS_JUMP_ALL", SAMPLE_POLYFLAGS_JUMP_ALL);
    context->RegisterEnumValue("SamplePolyFlag", "POLYFLAGS_DISABLED", SAMPLE_POLYFLAGS_DISABLED);
    context->RegisterEnumValue("SamplePolyFlag", "POLYFLAGS_ALL", SAMPLE_POLYFLAGS_ALL);

    context->RegisterEnum("NavPathFlag");
    context->RegisterEnumValue("NavPathFlag", "DT_STRAIGHTPATH_START", DT_STRAIGHTPATH_START);
    context->RegisterEnumValue("NavPathFlag", "DT_STRAIGHTPATH_END", DT_STRAIGHTPATH_END);
    context->RegisterEnumValue("NavPathFlag", "DT_STRAIGHTPATH_OFFMESH_CONNECTION", DT_STRAIGHTPATH_OFFMESH_CONNECTION);

    context->RegisterObjectType("NavPath", sizeof(NavPath), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
    context->RegisterObjectBehaviour("NavPath", asBEHAVE_CONSTRUCT, "void NavPath()", asFUNCTION(NavPathConstructor), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectBehaviour("NavPath", asBEHAVE_CONSTRUCT, "void NavPath(const NavPath &in other)", asFUNCTION(NavPathCopyConstructor), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectBehaviour("NavPath", asBEHAVE_DESTRUCT, "void NavPath()", asFUNCTION(NavPathDestructor), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("NavPath", "NavPath& opAssign(const NavPath &in other)", asFUNCTION(NavPathAssign), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectProperty("NavPath", "bool success", asOFFSET(NavPath, success));
    context->RegisterObjectMethod("NavPath", "int NumPoints()", asMETHOD(NavPath, NumPoints), asCALL_THISCALL);
    context->RegisterObjectMethod("NavPath", "vec3 GetPoint(int)", asMETHOD(NavPath, GetPoint), asCALL_THISCALL);
    context->RegisterObjectMethod("NavPath", "uint8 GetFlag(int)", asMETHOD(NavPath, GetFlag), asCALL_THISCALL);
    context->DocsCloseBrace();

    context->RegisterObjectType("NavPoint", sizeof(NavPoint), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
    context->RegisterObjectBehaviour("NavPoint", asBEHAVE_CONSTRUCT, "void NavPoint()", asFUNCTION(NavPointConstructor), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectBehaviour("NavPoint", asBEHAVE_CONSTRUCT, "void NavPoint(const NavPoint &in other)", asFUNCTION(NavPointCopyConstructor), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectBehaviour("NavPoint", asBEHAVE_DESTRUCT, "void NavPoint()", asFUNCTION(NavPointDestructor), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("NavPoint", "NavPoint& opAssign(const NavPoint &in other)", asFUNCTION(NavPointAssign), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectMethod("NavPoint", "bool IsSuccess()", asMETHOD(NavPoint, IsSuccess), asCALL_THISCALL);
    context->RegisterObjectMethod("NavPoint", "vec3 GetPoint()", asMETHOD(NavPoint, GetPoint), asCALL_THISCALL);
    context->DocsCloseBrace();

    context->RegisterGlobalFunction("NavPath GetPath(vec3 start, vec3 end)", asFUNCTION(ASGetPath), asCALL_CDECL);
    context->RegisterGlobalFunction("NavPath GetPath(vec3 start, vec3 end, uint16 include_poly_flags, uint16 exclude_poly_flags)", asFUNCTION(ASGetPath2), asCALL_CDECL);
    context->RegisterGlobalFunction("vec3 NavRaycast(vec3 start, vec3 end)", asFUNCTION(ASNavRaycast), asCALL_CDECL);
    context->RegisterGlobalFunction("vec3 NavRaycastSlide(vec3 start, vec3 end, int depth)", asFUNCTION(ASNavRaycastSlide), asCALL_CDECL);
    context->RegisterGlobalFunction("NavPoint GetNavPoint(vec3)", asFUNCTION(ASGetNavPoint), asCALL_CDECL);
    context->RegisterGlobalFunction("vec3 GetNavPointPos(vec3)", asFUNCTION(ASGetNavPointPos), asCALL_CDECL);
}

const int as_plant_movement_msg = _plant_movement_msg;
const int as_editor_msg = _editor_msg;

void ASPrintCallstack() {
    std::string callstack = active_context_stack.top()->GetCallstack();
    LOGI << "Callstack: \n"
         << callstack << std::endl;
}

void ASSendMessage(int target, int type, vec3 vec_a, vec3 vec_b) {
    Object* obj = the_scenegraph->GetObjectFromID(target);
    if (!obj) {
        std::string callstack = active_context_stack.top()->GetCallstack();
        std::ostringstream oss;
        oss << "No object with id: " << target << "\n";
        oss << "Called from:\n"
            << callstack;
        DisplayError("Error", oss.str().c_str());
        return;
    }
    obj->ReceiveASVec3Message(type, vec_a, vec_b);
}

void ASSendMessageString(int type, std::string msg) {
    the_scenegraph->map_editor->ReceiveMessage(msg);
}

static void ASSendGlobalMessage(std::string msg) {
    SceneGraph* s = Engine::Instance()->GetSceneGraph();
    if (s) {
        s->SendScriptMessageToAllObjects(msg);
        s->level->Message(msg);
    }

    ScriptableCampaign* sc = Engine::Instance()->GetCurrentCampaign();

    if (sc) {
        sc->ReceiveMessage(msg);
    }
}

void AttachMessages(ASContext* context) {
    context->RegisterGlobalFunction("void SendMessage(int target, int type, vec3 vec_a, vec3 vec_b)", asFUNCTION(ASSendMessage), asCALL_CDECL);
    context->RegisterGlobalFunction("void SendMessage(int type, string msg)", asFUNCTION(ASSendMessageString), asCALL_CDECL);
    context->RegisterGlobalFunction("void SendGlobalMessage(string msg)", asFUNCTION(ASSendGlobalMessage), asCALL_CDECL);
    context->RegisterGlobalProperty("int _plant_movement_msg", (void*)&as_plant_movement_msg);
    context->RegisterGlobalProperty("int _editor_msg", (void*)&as_editor_msg);
}

float ASatof(const std::string& str) {
    return (float)atof(str.c_str());
}

int ASatoi(const std::string& str) {
    return atoi(str.c_str());
}

static void ASLoadLevel(std::string path) {
    Engine* engine = Engine::Instance();
    engine->ScriptableUICallback(path);
}

static void ASLoadLevelID(std::string id) {
    Engine* engine = Engine::Instance();
    Online* online = Online::Instance();

    if (online->IsHosting()) {
        // mp.changeLevel(id);
    }

    engine->ScriptableUICallback("load_campaign_level " + id);
}

static void ASSetCampaignID(std::string id) {
    Engine* engine = Engine::Instance();
    engine->ScriptableUICallback("set_campaign " + id);
}

static std::string ASGetCurrLevelAbsPath() {
    SceneGraph* s = Engine::Instance()->GetSceneGraph();
    if (s) {
        return s->level_path_.GetAbsPathStr();
    } else {
        return "";
    }
}

static std::string ASGetCurrLevel() {
    SceneGraph* s = Engine::Instance()->GetSceneGraph();
    if (s) {
        return s->level_path_.GetFullPath();
    } else {
        return "";
    }
}

static std::string ASGetCurrLevelID() {
    return Engine::Instance()->GetCurrentLevelID();
}

static std::string ASGetCurrCampaignID() {
    ScriptableCampaign* sc = Engine::Instance()->GetCurrentCampaign();
    if (sc) {
        return sc->GetCampaignID();
    } else {
        return "";
    }
}

static std::string ASGetCurrLevelRelPath() {
    SceneGraph* s = Engine::Instance()->GetSceneGraph();
    if (s) {
        return s->level_path_.GetOriginalPath();
    } else {
        return "";
    }
}

static std::string ASGetLevelName(const std::string& path) {
    if (FileExists(path, kAnyPath)) {
        LevelInfoAssetRef levelinfo = Engine::Instance()->GetAssetManager()->LoadSync<LevelInfoAsset>(path);

        if (levelinfo.valid()) {
            return levelinfo->GetLevelName();
        } else {
            LOGW << "Failed to load levelinfo for " << path << std::endl;
        }
    } else {
        LOGW << "Failed to load levelinfo for " << path << std::endl;
    }
    return "";
}

static std::string ASGetCurrLevelName() {
    return ASGetLevelName(ASGetCurrLevelAbsPath());
}

static std::string ASGetCurrentMenuModsourceID() {
    return ModLoading::Instance().GetModID(Engine::Instance()->GetLatestMenuPath().GetModsource());
}

static std::string ASGetCurrentLevelModsourceID() {
    return ModLoading::Instance().GetModID(Engine::Instance()->GetLatestLevelPath().GetModsource());
}

static std::string ASGetCurrentCampaignModsourceID() {
    //    return ModLoading::Instance().GetModID(Engine::Instnace()->GetLatestCampaign
    return "";
}

static bool ASEditorModeActive() {
    SceneGraph* s = Engine::Instance()->GetSceneGraph();
    if (s) {
        return s->map_editor->state_ != MapEditor::kInGame;
    } else {
        return false;
    }
}

static void ASAssert(asIScriptGeneric* gen) {
    bool arg0 = gen->GetArgByte(0);
    if (arg0 == 0) {
        LOGE << "Failed assert in angelscript" << std::endl;
    }
}

static void ASSetSplitScreenMode(ForcedSplitScreenMode mode) {
    Engine::Instance()->SetForcedSplitScreenMode(mode);
}

void AttachEngine(ASContext* context) {
    context->RegisterEnum("EngineState");
    context->RegisterEnumValue("EngineState", "kEngineNoState", kEngineNoState);
    context->RegisterEnumValue("EngineState", "kEngineLevelState", kEngineLevelState);
    context->RegisterEnumValue("EngineState", "kEngineEditorLevelState", kEngineEditorLevelState);
    context->RegisterEnumValue("EngineState", "kEngineEngineScriptableUIState", kEngineScriptableUIState);
    context->RegisterEnumValue("EngineState", "kEngineCampaignState", kEngineCampaignState);

    context->RegisterEnum("SplitScreenMode");
    context->RegisterEnumValue("SplitScreenMode", "kModeNone", kForcedModeNone);
    context->RegisterEnumValue("SplitScreenMode", "kModeFull", kForcedModeFull);
    context->RegisterEnumValue("SplitScreenMode", "kModeSplit", kForcedModeSplit);

    context->RegisterGlobalFunction("void LoadLevel(string level_path)",
                                    asFUNCTION(ASLoadLevel), asCALL_CDECL);
    context->RegisterGlobalFunction("void LoadLevelID(string id)",
                                    asFUNCTION(ASLoadLevelID), asCALL_CDECL);
    context->RegisterGlobalFunction("void SetCampaignID(string id)",
                                    asFUNCTION(ASSetCampaignID), asCALL_CDECL);

    context->RegisterGlobalFunction("string GetCurrLevelAbsPath()",
                                    asFUNCTION(ASGetCurrLevelAbsPath), asCALL_CDECL);
    context->RegisterGlobalFunction("string GetCurrLevel()",
                                    asFUNCTION(ASGetCurrLevel), asCALL_CDECL);
    context->RegisterGlobalFunction("string GetCurrLevelRelPath()",
                                    asFUNCTION(ASGetCurrLevelRelPath), asCALL_CDECL);
    context->RegisterGlobalFunction("string GetLevelName(const string& path)",
                                    asFUNCTION(ASGetLevelName), asCALL_CDECL);
    context->RegisterGlobalFunction("string GetCurrLevelName()",
                                    asFUNCTION(ASGetCurrLevelName), asCALL_CDECL);
    context->RegisterGlobalFunction("string GetCurrLevelID()",
                                    asFUNCTION(ASGetCurrLevelID), asCALL_CDECL);

    context->RegisterGlobalFunction("string GetCurrCampaignID()",
                                    asFUNCTION(ASGetCurrCampaignID), asCALL_CDECL);

    context->RegisterGlobalFunction("string GetCurrentMenuModsourceID()",
                                    asFUNCTION(ASGetCurrentMenuModsourceID), asCALL_CDECL);
    context->RegisterGlobalFunction("string GetCurrentLevelModsourceID()",
                                    asFUNCTION(ASGetCurrentLevelModsourceID), asCALL_CDECL);

    context->RegisterGlobalFunction("bool EditorModeActive()", asFUNCTION(ASEditorModeActive), asCALL_CDECL);

    context->RegisterGlobalFunction("void assert(bool val)", asFUNCTION(ASAssert), asCALL_GENERIC);

    context->RegisterGlobalFunction("void SetSplitScreenMode(SplitScreenMode mode)", asFUNCTION(ASSetSplitScreenMode), asCALL_CDECL);
}

bool AS_Online_IsActive() {
    return Online::Instance()->IsActive();
}

void AS_Online_SendKillMessage(uint32_t victim_id, uint32_t killer_id) {
    Online* online = Online::Instance();

    PlayerState victim;
    PlayerState killer;
    if (online->TryGetPlayerState(victim, victim_id)) {
        if (online->TryGetPlayerState(killer, killer_id)) {
            online->SendRawChatMessage(victim.playername + " was killed by " + killer.playername);
        } else {
            online->SendRawChatMessage(victim.playername + " died");
        }
    }
}

void AS_Online_SendChatMessage(const std::string& message) {
    Online::Instance()->BroadcastChatMessage(message);
}

void AS_Online_SendChatSystemMessage(const std::string& system_message) {
    Online::Instance()->SendRawChatMessage(system_message);
}

void AS_Online_Host() {
    Online::Instance()->StartHostingMultiplayer();
}

void AS_Online_Connect(const std::string& ip) {
    Online::Instance()->ConnectByIPAddress(ip.c_str());
}

bool AS_Online_IsClient() {
    return Online::Instance()->IsClient();
}

void AS_Online_SetAvatarCameraAttachedMode(bool mode) {
    Online::Instance()->SetAvatarCameraAttachedMode(mode);
}

bool AS_Online_IsAvatarCameraAttached() {
    return Online::Instance()->IsAvatarCameraAttached();
}

bool AS_Online_IsHosting() {
    return Online::Instance()->IsHosting();
}

void AS_Online_Close() {
    Online::Instance()->QueueStopMultiplayer();
}

void AS_Online_IsLobbyFull() {
    Online::Instance()->IsLobbyFull();
}

uint32_t AS_Online_RegisterState(std::string state) {
    return Online::Instance()->RegisterMPState(state);
}

void AS_Online_SendStateToPeer(uint32_t state, uint32_t avatar_id, const CScriptArray& data) {
    const int items_count = data.GetSize();
    std::vector<uint32_t> array_data;
    array_data.resize(items_count);

    for (int n = 0; n < items_count; n++) {
        array_data[n] = (*static_cast<const unsigned int*>(data.At(n)));
    }

    ASContext* ctx = GetActiveASContext();

    LOGD << "Calling: " << ctx->GetASFunctionNameFromMPState(state) << " on client with state:" << state << std::endl;

    Online::Instance()->Send<OnlineMessages::AngelscriptObjectData>(avatar_id, state, array_data);
}

void AS_Online_RegisterStateCallback(uint32_t state, std::string function) {
    ASContext* ctx = GetActiveASContext();
    ctx->RegisterMPStateCallback(state, function);
}

void ASCallStateMPCallback(uint32_t state, std::vector<char>& data) {
    ASContext* ctx = GetActiveASContext();
    ctx->CallMPCallBack(state, data);
}

bool AS_Online_IsValidPlayerName(const std::string& name) {
    return OnlineUtility::IsValidPlayerName(name);
}

void AS_Online_NetFrameworkHasFriendInviteOverlay() {
    Online::Instance()->NetFrameworkHasFriendInviteOverlay();
}

void AS_Online_ActivateInviteDialog() {
    Online::Instance()->ActivateGameOverlayInviteDialog();
}

void AS_Online_AddSyncState(uint32_t state, const CScriptArray& data) {
    // This is the same as bellow, please write helper function instead of copy code
    const int items_count = data.GetSize();
    std::vector<char> array_data;
    array_data.resize(items_count);

    for (int n = 0; n < items_count; n++) {
        array_data[n] = (*reinterpret_cast<const char*>(data.At(n)));
    }

    Online::Instance()->AddSyncState(state, array_data);
}

void AS_Online_SendState(uint32_t state, const CScriptArray& data) {
    const int items_count = data.GetSize();
    std::vector<char> array_data;
    array_data.resize(items_count);

    for (int n = 0; n < items_count; n++) {
        array_data[n] = (*reinterpret_cast<const char*>(data.At(n)));
    }

    Online::Instance()->Send<OnlineMessages::AngelscriptData>(state, array_data, false);
}

bool AS_Online_HasActiveIncompatibleMods() {
    return OnlineUtility::HasActiveIncompatibleMods();
}

std::string AS_Online_GetActiveIncompatibleModsString() {
    return OnlineUtility::GetActiveIncompatibleModsString();
}

void AttachOnline(ASContext* context) {
    context->RegisterGlobalFunction("void Online_Connect(string ip)", asFUNCTION(AS_Online_Connect), asCALL_CDECL);
    context->RegisterGlobalFunction("void Online_Host()", asFUNCTION(AS_Online_Host), asCALL_CDECL);
    context->RegisterGlobalFunction("void Online_SendKillMessage(uint victim, uint killer)", asFUNCTION(AS_Online_SendKillMessage), asCALL_CDECL);
    context->RegisterGlobalFunction("void Online_SendChatMessage(string message)", asFUNCTION(AS_Online_SendChatMessage), asCALL_CDECL);
    context->RegisterGlobalFunction("void Online_SendChatSystemMessage(string system_message)", asFUNCTION(AS_Online_SendChatSystemMessage), asCALL_CDECL);
    context->RegisterGlobalFunction("bool Online_IsActive()", asFUNCTION(AS_Online_IsActive), asCALL_CDECL);
    context->RegisterGlobalFunction("bool Online_IsClient(void)", asFUNCTION(AS_Online_IsClient), asCALL_CDECL);
    context->RegisterGlobalFunction("bool Online_IsHosting(void)", asFUNCTION(AS_Online_IsHosting), asCALL_CDECL);
    context->RegisterGlobalFunction("void Online_SetAvatarCameraAttachedMode(bool mode)", asFUNCTION(AS_Online_SetAvatarCameraAttachedMode), asCALL_CDECL);
    context->RegisterGlobalFunction("bool Online_IsAvatarCameraAttached()", asFUNCTION(AS_Online_IsAvatarCameraAttached), asCALL_CDECL);
    context->RegisterGlobalFunction("void Online_SendStateToPeer(uint state, uint avatar_id, const array<uint>& data)", asFUNCTION(AS_Online_SendStateToPeer), asCALL_CDECL);
    context->RegisterGlobalFunction("uint Online_RegisterState(string state)", asFUNCTION(AS_Online_RegisterState), asCALL_CDECL);
    context->RegisterGlobalFunction("uint Online_RegisterStateCallback(uint state, string callback)", asFUNCTION(AS_Online_RegisterStateCallback), asCALL_CDECL);
    context->RegisterGlobalFunction("void Online_SendState(uint state, const array<uint8>& data)", asFUNCTION(AS_Online_SendState), asCALL_CDECL);
    context->RegisterGlobalFunction("void Online_AddSyncState(uint state, const array<uint8>& data)", asFUNCTION(AS_Online_AddSyncState), asCALL_CDECL);
    context->RegisterGlobalFunction("bool Online_IsValidPlayerName(string name)", asFUNCTION(AS_Online_IsValidPlayerName), asCALL_CDECL);
    context->RegisterGlobalFunction("void Online_Close()", asFUNCTION(AS_Online_Close), asCALL_CDECL);
    context->RegisterGlobalFunction("bool Online_IsLobbyFull()", asFUNCTION(AS_Online_IsLobbyFull), asCALL_CDECL);
    context->RegisterGlobalFunction("bool Online_HasActiveIncompatibleMods()", asFUNCTION(AS_Online_HasActiveIncompatibleMods), asCALL_CDECL);
    context->RegisterGlobalFunction("string Online_GetActiveIncompatibleModsString()", asFUNCTION(AS_Online_GetActiveIncompatibleModsString), asCALL_CDECL);

    context->RegisterGlobalFunction("bool Online_HasFriendInviteOverlay()", asFUNCTION(AS_Online_NetFrameworkHasFriendInviteOverlay), asCALL_CDECL);
    context->RegisterGlobalFunction("void Online_ActivateInviteDialog()", asFUNCTION(AS_Online_ActivateInviteDialog), asCALL_CDECL);
}

void AttachStringConvert(ASContext* context) {
    context->RegisterGlobalFunction("float atof(const string &in str)", asFUNCTION(ASatof), asCALL_CDECL);
    context->RegisterGlobalFunction("int atoi(const string &in str)", asFUNCTION(ASatoi), asCALL_CDECL);
}

void ASWriteString(SavedChunk* chunk, const std::string& str) {
    chunk->desc.AddString(EDF_SCRIPT_PARAMS, str);
}

std::string ASReadString(SavedChunk* chunk) {
    EntityDescriptionField* edf = chunk->desc.GetField(EDF_SCRIPT_PARAMS);
    LOG_ASSERT(edf);
    std::string str;
    edf->ReadString(&str);
    return str;
}

void AttachUndo(ASContext* context) {
    context->RegisterObjectType("SavedChunk", 0, asOBJ_REF | asOBJ_NOCOUNT);
    context->RegisterObjectMethod("SavedChunk", "void WriteString(const string &in str)", asFUNCTION(ASWriteString), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("SavedChunk", "string ReadString()", asFUNCTION(ASReadString), asCALL_CDECL_OBJFIRST);
}

static void ASLog(LogSystem::LogType level, const std::string& str) {
    if (active_context_stack.size() > 0) {
        std::pair<Path, int> d = active_context_stack.top()->GetCallFile();

        if (memcmp(d.first.GetOriginalPath(), "Data/Scripts/", 13) == 0)
            LogSystem::LogData(level, "us", d.first.GetOriginalPath() + 13, d.second) << str << std::endl;
        else
            LogSystem::LogData(level, "us", d.first.GetOriginalPath(), d.second) << str << std::endl;
    } else {
        LogSystem::LogData(level, "us", "NO_CONTEXT", 0) << str << std::endl;
    }
}

void AttachLog(ASContext* context) {
    context->RegisterEnum("LogType");

    context->RegisterEnumValue("LogType", "fatal", LogSystem::fatal);
    context->RegisterEnumValue("LogType", "error", LogSystem::error);
    context->RegisterEnumValue("LogType", "warning", LogSystem::warning);
    context->RegisterEnumValue("LogType", "info", LogSystem::info);
    context->RegisterEnumValue("LogType", "debug", LogSystem::debug);
    context->RegisterEnumValue("LogType", "spam", LogSystem::spam);

    context->RegisterGlobalFunction("void Log( LogType level, const string &in str )", asFUNCTION(ASLog), asCALL_CDECL);
}

static std::string ASGetBuildVersionShort() {
    return std::string(GetBuildVersion());
}

static std::string ASGetBuildVersionFull() {
    return std::string(GetBuildVersion()) + "_" + std::string(GetBuildIDString());
}

static std::string ASGetBuildTimestamp() {
    return std::string(GetBuildTimestamp());
}

void AttachInfo(ASContext* context) {
    context->RegisterGlobalFunction("string GetBuildVersionShort( )", asFUNCTION(ASGetBuildVersionShort), asCALL_CDECL);
    context->RegisterGlobalFunction("string GetBuildVersionFull( )", asFUNCTION(ASGetBuildVersionFull), asCALL_CDECL);
    context->RegisterGlobalFunction("string GetBuildTimestamp( )", asFUNCTION(ASGetBuildTimestamp), asCALL_CDECL);
}

/*******
 *
 * JSON suport
 *
 */

#include "JSON/jsonhelper.h"

static void JSONValueConstructor(void* memory) {
    new (memory) Json::Value();
}

static void JSONValueTypeConstructor(void* memory, Json::ValueType& type) {
    new (memory) Json::Value(type);
}

static void JSONValueIntConstructor(void* memory, Json::Int& value) {
    new (memory) Json::Value(value);
}

static void JSONValueUIntConstructor(void* memory, Json::UInt& value) {
    new (memory) Json::Value(value);
}

static void JSONValueInt64Constructor(void* memory, Json::Int64& value) {
    new (memory) Json::Value(value);
}

static void JSONValueUInt64Constructor(void* memory, Json::UInt64& value) {
    new (memory) Json::Value(value);
}

static void JSONValueDoubleConstructor(void* memory, double& value) {
    new (memory) Json::Value(value);
}

static void JSONValueStringConstructor(void* memory, std::string& value) {
    new (memory) Json::Value(value);
}

static void JSONValueBoolConstructor(void* memory, bool& value) {
    new (memory) Json::Value(value);
}

static void JSONValueValueConstructor(void* memory, Json::Value& value) {
    new (memory) Json::Value(value);
}

static void JSONValueDestructor(void* memory) {
    ((Json::Value*)memory)->~Value();
}

static Json::Value& JSONValueAssign(Json::Value* self, const Json::Value& other) {
    return (*self) = other;
}

static Json::Value& JSONValueConstStringIndex(Json::Value* self, const std::string& key) {
    return (*self)[key];
}

static Json::Value& JSONValueConstIntIndex(Json::Value* self, const int& key) {
    return (*self)[key];
}

static bool JSONValueRemoveIndex(Json::Value* self, const unsigned int i) {
    Json::Value temp;
    return self->removeIndex(i, &temp);
}

static bool JSONValueRemoveMember(Json::Value* self, const std::string& key) {
    Json::Value temp;
    return self->removeMember(key, &temp);
}

static bool JSONValueIsMember(Json::Value* self, const std::string& key) {
    return self->isMember(key);
}

static CScriptArray* JSONValueGetMembers(Json::Value* self) {
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<string>"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);

    std::vector<std::string> members = self->getMemberNames();

    array->Reserve(members.size());

    for (const auto& member : members) {
        array->InsertLast((void*)(&member));
    }
    return array;
}

static std::string JSONValueTypeName(Json::Value* self) {
    switch (self->type()) {
        case Json::nullValue: {
            return "null";
        } break;
        case Json::intValue: {
            return "int";
        } break;
        case Json::uintValue: {
            return "uint";
        } break;
        case Json::realValue: {
            return "real";
        } break;
        case Json::stringValue: {
            return "string";
        } break;
        case Json::booleanValue: {
            return "boolean";
        } break;
        case Json::arrayValue: {
            return "array";
        } break;
        case Json::objectValue: {
            return "object";
        } break;

        default: {
            return "unknown";
        } break;
    }
}

static void SimpleJSONWrapperConstructor(void* memory) {
    new (memory) SimpleJSONWrapper();
}

static void SimpleJSONWrapperDestructor(void* memory) {
    ((SimpleJSONWrapper*)memory)->~SimpleJSONWrapper();
}

static SimpleJSONWrapper& SimpleJSONWrapperAssign(SimpleJSONWrapper* self, const SimpleJSONWrapper& other) {
    return (*self) = other;
}

void AttachJSON(ASContext* context) {
    // Register the JSON value type enumeration
    context->RegisterEnum("JsonValueType");
    context->RegisterEnumValue("JsonValueType", "JSONnullValue", Json::nullValue);

    context->RegisterEnumValue("JsonValueType", "JSONintValue", Json::intValue);
    context->RegisterEnumValue("JsonValueType", "JSONuintValue", Json::uintValue);
    context->RegisterEnumValue("JsonValueType", "JSONrealValue", Json::realValue);
    context->RegisterEnumValue("JsonValueType", "JSONstringValue", Json::stringValue);
    context->RegisterEnumValue("JsonValueType", "JSONbooleanValue", Json::booleanValue);
    context->RegisterEnumValue("JsonValueType", "JSONarrayValue", Json::arrayValue);
    context->RegisterEnumValue("JsonValueType", "JSONobjectValue", Json::objectValue);

    // Attach the JSON Value
    context->RegisterObjectType("JSONValue", sizeof(Json::Value), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);

    context->RegisterObjectBehaviour("JSONValue", asBEHAVE_CONSTRUCT, "void JSONValue()", asFUNCTION(JSONValueConstructor), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectBehaviour("JSONValue", asBEHAVE_CONSTRUCT, "void JSONValue( JsonValueType &in )", asFUNCTION(JSONValueTypeConstructor), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectBehaviour("JSONValue", asBEHAVE_CONSTRUCT, "void JSONValue( int &in )", asFUNCTION(JSONValueIntConstructor), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectBehaviour("JSONValue", asBEHAVE_CONSTRUCT, "void JSONValue( uint &in)", asFUNCTION(JSONValueUIntConstructor), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectBehaviour("JSONValue", asBEHAVE_CONSTRUCT, "void JSONValue( int64 &in)", asFUNCTION(JSONValueInt64Constructor), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectBehaviour("JSONValue", asBEHAVE_CONSTRUCT, "void JSONValue( uint64 &in )", asFUNCTION(JSONValueUInt64Constructor), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectBehaviour("JSONValue", asBEHAVE_CONSTRUCT, "void JSONValue( double &in )", asFUNCTION(JSONValueDoubleConstructor), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectBehaviour("JSONValue", asBEHAVE_CONSTRUCT, "void JSONValue( string &in )", asFUNCTION(JSONValueStringConstructor), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectBehaviour("JSONValue", asBEHAVE_CONSTRUCT, "void JSONValue( bool &in )", asFUNCTION(JSONValueBoolConstructor), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectBehaviour("JSONValue", asBEHAVE_CONSTRUCT, "void JSONValue( JSONValue &in )", asFUNCTION(JSONValueValueConstructor), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectBehaviour("JSONValue", asBEHAVE_DESTRUCT, "void JSONValue()", asFUNCTION(JSONValueDestructor), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectMethod("JSONValue", "JSONValue& opAssign(const JSONValue &in other)", asFUNCTION(JSONValueAssign), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectMethod("JSONValue", "JSONValue& opIndex( const string &in )", asFUNCTION(JSONValueConstStringIndex), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectMethod("JSONValue", "JSONValue& opIndex( const int &in )", asFUNCTION(JSONValueConstIntIndex), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectMethod("JSONValue", "string asString()", asMETHOD(Json::Value, asString), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "JsonValueType type()", asMETHOD(Json::Value, type), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "string typeName()", asFUNCTION(JSONValueTypeName), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectMethod("JSONValue", "int asInt()", asMETHOD(Json::Value, asInt), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "uint asUInt()", asMETHOD(Json::Value, asUInt), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "int64 asInt64()", asMETHOD(Json::Value, asInt64), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "uint64 asUInt64()", asMETHOD(Json::Value, asUInt64), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "float asFloat()", asMETHOD(Json::Value, asFloat), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "double asDouble()", asMETHOD(Json::Value, asDouble), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "bool asBool()", asMETHOD(Json::Value, asBool), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "bool isNull()", asMETHOD(Json::Value, isNull), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "bool isBool()", asMETHOD(Json::Value, isBool), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "bool isInt()", asMETHOD(Json::Value, isInt), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "bool isInt64()", asMETHOD(Json::Value, isInt64), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "bool isUInt()", asMETHOD(Json::Value, isUInt), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "bool isUInt64()", asMETHOD(Json::Value, isUInt64), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "bool isIntegral()", asMETHOD(Json::Value, isIntegral), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "bool isDouble()", asMETHOD(Json::Value, isDouble), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "bool isNumeric()", asMETHOD(Json::Value, isNumeric), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "bool isString()", asMETHOD(Json::Value, isString), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "bool isArray()", asMETHOD(Json::Value, isArray), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "bool isObject()", asMETHOD(Json::Value, isObject), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "bool isConvertibleTo(JsonValueType type)", asMETHOD(Json::Value, isConvertibleTo), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "uint size()", asMETHOD(Json::Value, size), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "bool empty()", asMETHOD(Json::Value, empty), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "void clear()", asMETHOD(Json::Value, empty), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "void resize(uint64)", asMETHOD(Json::Value, resize), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "bool isValidIndex(uint64)", asMETHOD(Json::Value, isValidIndex), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "JSONValue& append(const JSONValue &in)", asMETHOD(Json::Value, append), asCALL_THISCALL);

    context->RegisterObjectMethod("JSONValue", "bool removeMember( const string &in )", asFUNCTION(JSONValueRemoveMember), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectMethod("JSONValue", "bool removeIndex( uint  i )", asFUNCTION(JSONValueRemoveIndex), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectMethod("JSONValue", "bool isMember(const string &in)", asFUNCTION(JSONValueIsMember), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectMethod("JSONValue", "array<string>@ getMemberNames()", asFUNCTION(JSONValueGetMembers), asCALL_CDECL_OBJFIRST);

    // Attach the wrapper
    context->RegisterObjectType("JSON", sizeof(SimpleJSONWrapper), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);

    context->RegisterObjectBehaviour("JSON", asBEHAVE_CONSTRUCT, "void JSON()", asFUNCTION(SimpleJSONWrapperConstructor), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectBehaviour("JSON", asBEHAVE_DESTRUCT, "void JSON()", asFUNCTION(SimpleJSONWrapperDestructor), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectMethod("JSON", "JSON& opAssign(const JSON &in other)", asFUNCTION(SimpleJSONWrapperAssign), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectMethod("JSON", "bool parseString(string &in)", asMETHOD(SimpleJSONWrapper, parseString), asCALL_THISCALL);

    context->RegisterObjectMethod("JSON", "bool parseFile(string &in)", asMETHOD(SimpleJSONWrapper, parseFile), asCALL_THISCALL);

    context->RegisterObjectMethod("JSON", "string writeString(bool=false)", asMETHOD(SimpleJSONWrapper, writeString), asCALL_THISCALL);

    context->RegisterObjectMethod("JSON", "JSONValue& getRoot()", asMETHOD(SimpleJSONWrapper, getRoot), asCALL_THISCALL);
}

std::string AS_GetConfigValueString(std::string string) {
    if (string == "overall") {
        return config.GetSettingsPreset();
    } else if (string == "difficulty_preset") {
        return config.GetClosestDifficulty();
    } else {
        return config[string].str();
    }
}

static CScriptArray* AS_GetConfigValueOptions(std::string string) {
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<string>"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);

    std::vector<std::string> values;

    if (string == "overall") {
        values = config.GetSettingsPresets();
    } else if (string == "difficulty_preset") {
        values = config.GetDifficultyPresets();
    } else {
    }

    array->Reserve(values.size());

    std::vector<std::string>::iterator valueit = values.begin();

    for (; valueit != values.end(); valueit++) {
        array->InsertLast((void*)&(*valueit));
    }

    return array;
}

float AS_GetConfigValueFloat(std::string string) {
    return config[string].toNumber<float>();
}

bool AS_GetConfigValueBool(std::string string) {
    return config[string].toBool();
}

void AS_SetConfigValueString(std::string key, std::string value) {
    if (key == "overall") {
        config.SetSettingsToPreset(value);
        config.ReloadStaticSettings();
        config.ReloadDynamicSettings();
    } else if (key == "difficulty_preset") {
        config.SetDifficultyPreset(value);
        config.ReloadDynamicSettings();
    } else {
        config.GetRef(key) = value;
        config.ReloadDynamicSettings();
    }
}

void AS_SetConfigValueBool(std::string key, bool value) {
    config.GetRef(key) = value;
    config.ReloadDynamicSettings();
}

void AS_SetConfigValueInt(std::string key, int value) {
    config.GetRef(key) = value;
    config.ReloadDynamicSettings();
}

int AS_GetConfigValueInt(std::string string) {
    return config[string].toNumber<int>();
}

void AS_SetConfigValueFloat(std::string key, float value) {
    config.GetRef(key) = value;
    config.ReloadDynamicSettings();
}

int AS_GetMonitorCount() {
    return SDL_GetNumVideoDisplays();
}

static CScriptArray* AS_GetPossibleResolutions() {
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<vec2>"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);

    std::vector<Resolution> resolutions = config.GetPossibleResolutions();

    array->Reserve(resolutions.size());

    for (auto& resolution : resolutions) {
        vec2 cur_res = vec2(resolution.w, resolution.h);
        array->InsertLast((void*)&(cur_res));
    }
    return array;
}

void AS_ReloadStaticValues() {
    config.ReloadStaticSettings();
}

static CScriptArray* ASGetAvailableBindingCategories() {
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<string>"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);

    std::vector<std::string> descs = Input::Instance()->GetAvailableBindingCategories();

    array->Reserve(descs.size());

    std::vector<std::string>::iterator descit = descs.begin();

    for (; descit != descs.end(); descit++) {
        array->InsertLast((void*)&(*descit));
    }

    return array;
}

static CScriptArray* ASGetAvailableBindings(const std::string& category) {
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<string>"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);

    std::vector<std::string> descs = Input::Instance()->GetAvailableBindings(category);

    array->Reserve(descs.size());

    std::vector<std::string>::iterator descit = descs.begin();

    for (; descit != descs.end(); descit++) {
        array->InsertLast((void*)&(*descit));
    }

    return array;
}

static std::string ASGetBindingValue(std::string binding_category, std::string binding) {
    return Input::Instance()->GetBindingValue(binding_category, binding);
}

static void ASSetBindingValue(std::string binding_category, std::string binding, std::string value) {
    Input::Instance()->SetBindingValue(binding_category, binding, value);
}

static void ASSetKeyboardBindingValue(std::string binding_category, std::string binding, uint32_t scancode) {
    Input::Instance()->SetKeyboardBindingValue(binding_category, binding, (SDL_Scancode)scancode);
}

static void ASSetMouseBindingValue(std::string binding_category, std::string binding, uint32_t button) {
    Input::Instance()->SetMouseBindingValue(binding_category, binding, button);
}

static void ASSetMouseBindingValueString(std::string binding_category, std::string binding, std::string text) {
    Input::Instance()->SetMouseBindingValue(binding_category, binding, text);
}

static void ASSetControllerBindingValue(std::string binding_category, std::string binding, uint32_t input) {
    Input::Instance()->SetControllerBindingValue(binding_category, binding, (ControllerInput::Input)input);
}

static void ASSaveConfig() {
    std::string config_path = GetConfigPath();
    if (config.HasChangedSinceLastSave()) {
        LOGI << "Saving config at request from angel script" << std::endl;
        config.Save(config_path);
    }
}

static bool ASConfigHasKey(std::string key) {
    return config.HasKey(key);
}

static void ASResetBinding(std::string category, std::string binding) {
    const std::string config_value = category + "[" + binding + "]";
    if (memcmp(category.c_str(), "gamepad_", strlen("gamepad_")) == 0) {
        config.RemoveConfig(config_value);
    } else {
        config.GetRef(config_value) = default_config.GetRef(config_value);
    }
    config.ReloadStaticSettings();
}

void AttachConfig(ASContext* context) {
    context->RegisterGlobalFunction("string GetConfigValueString(string index)",
                                    asFUNCTION(AS_GetConfigValueString),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("array<string>@ GetConfigValueOptions(string index)",
                                    asFUNCTION(AS_GetConfigValueOptions),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("float GetConfigValueFloat(string index)",
                                    asFUNCTION(AS_GetConfigValueFloat),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("void SetConfigValueFloat(string key, float value)",
                                    asFUNCTION(AS_SetConfigValueFloat),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("bool GetConfigValueBool(string index)",
                                    asFUNCTION(AS_GetConfigValueBool),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("void SetConfigValueString(string key, string value)",
                                    asFUNCTION(AS_SetConfigValueString),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("void SetConfigValueBool(string key, bool value)",
                                    asFUNCTION(AS_SetConfigValueBool),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("void SetConfigValueInt(string key, int value)",
                                    asFUNCTION(AS_SetConfigValueInt),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("int GetConfigValueInt(string key)",
                                    asFUNCTION(AS_GetConfigValueInt),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("int GetMonitorCount()",
                                    asFUNCTION(AS_GetMonitorCount),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("array<vec2>@ GetPossibleResolutions()",
                                    asFUNCTION(AS_GetPossibleResolutions),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("void ReloadStaticValues()",
                                    asFUNCTION(AS_ReloadStaticValues),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("array<string>@ GetAvailableBindingCategories()",
                                    asFUNCTION(ASGetAvailableBindingCategories),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("array<string>@ GetAvailableBindings(const string& in)",
                                    asFUNCTION(ASGetAvailableBindings),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("string GetBindingValue(string binding_category, string binding)",
                                    asFUNCTION(ASGetBindingValue),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("void SetBindingValue(string binding_category, string binding, string value)",
                                    asFUNCTION(ASSetBindingValue),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("void SetKeyboardBindingValue(string binding_category, string binding, uint32 scancode)",
                                    asFUNCTION(ASSetKeyboardBindingValue),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("void SetMouseBindingValue(string binding_category, string binding, uint32 button)",
                                    asFUNCTION(ASSetMouseBindingValue),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("void SetMouseBindingValue(string binding_category, string binding, string text)",
                                    asFUNCTION(ASSetMouseBindingValueString),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("void SetControllerBindingValue(string binding_category, string binding, uint32 input)",
                                    asFUNCTION(ASSetControllerBindingValue),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("void SaveConfig()",
                                    asFUNCTION(ASSaveConfig),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("bool ConfigHasKey(string key)",
                                    asFUNCTION(ASConfigHasKey),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("void ResetBinding(string binding_category, string binding)",
                                    asFUNCTION(ASResetBinding),
                                    asCALL_CDECL);
}

static std::string AS_ToUpper(std::string& in) {
    size_t len = in.size() + 1 + (in.size() / 10) + 10;
    char* output = (char*)alloca(len);  // We pad for the trailing null sign and potential codepoint expansion of some characters.
    UTF8ToUpper(output, len, in.c_str());
    return std::string(output);
}

uint32_t AS_GetLengthInBytesForNCodepoints(const std::string& utf8in, uint32_t codepoint_index) {
    return GetLengthInBytesForNCodepoints(utf8in, codepoint_index);
}

uint32_t AS_GetCodepointCount(const std::string& utf8in) {
    return GetCodepointCount(utf8in);
}

void AttachStringUtil(ASContext* context) {
    context->RegisterGlobalFunction("string ToUpper(string &in)",
                                    asFUNCTION(AS_ToUpper),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("uint GetLengthInBytesForNCodepoints( const string& in, uint codepoint_index )",
                                    asFUNCTION(AS_GetLengthInBytesForNCodepoints),
                                    asCALL_CDECL);
    context->RegisterGlobalFunction("uint GetCodepointCount( const string& in )",
                                    asFUNCTION(AS_GetCodepointCount),
                                    asCALL_CDECL);
}

bool AS_DirectoryExists(std::string& in) {
    char out[kPathSize];
    return 0 == FindFilePath(in.c_str(), out, kPathSize, kModPaths | kDataPaths | kWriteDir | kModWriteDirs, false) && !isFile(out);
}

bool AS_FileExists(std::string& in) {
    char out[kPathSize];
    return 0 == FindFilePath(in.c_str(), out, kPathSize, kModPaths | kDataPaths | kWriteDir | kModWriteDirs, false) && isFile(out);
}

void AttachIO(ASContext* context) {
    context->RegisterGlobalFunction("bool DirectoryExists(string& in)", asFUNCTION(AS_DirectoryExists), asCALL_CDECL);
    context->RegisterGlobalFunction("bool FileExists(string& in)", asFUNCTION(AS_FileExists), asCALL_CDECL);
}

void AttachDebug(ASContext* context) {
    context->RegisterGlobalFunction("void PrintCallstack()", asFUNCTION(ASPrintCallstack), asCALL_CDECL);
}

static void ASParameterConstructor(void* m) {
    new (m) ModInstance::Parameter();
}

static void ASParameterCopyConstructor(const ModInstance::Parameter& other, void* m) {
    new (m) ModInstance::Parameter(other);
}

static void ASParameterDestructor(void* m) {
    ((ModInstance::Parameter*)m)->~Parameter();
}

static ModInstance::Parameter& ASParameterOpAssign(ModInstance::Parameter* self, const ModInstance::Parameter& other) {
    *self = other;
    return *self;
}

static ModInstance::Parameter ParameterConstStringIndex(ModInstance::Parameter* self, const std::string& key) {
    if (strmtch(self->type, "table")) {
        for (auto& parameter : self->parameters) {
            if (strmtch(parameter.name, key.c_str())) {
                return parameter;
            }
        }
    }
    return ModInstance::Parameter();
}

static ModInstance::Parameter ParameterConstIntIndex(ModInstance::Parameter* self, const int& key) {
    if (key >= 0 && key < (int)self->parameters.size()) {
        return self->parameters[key];
    }
    return ModInstance::Parameter();
}

static bool ASParameterIsEmpty(ModInstance::Parameter* self) {
    return strmtch(self->type, "empty");
}

static bool ASParameterIsString(ModInstance::Parameter* self) {
    return strmtch(self->type, "string");
}

static bool ASParameterIsArray(ModInstance::Parameter* self) {
    return strmtch(self->type, "array");
}

static bool ASParameterIsTable(ModInstance::Parameter* self) {
    return strmtch(self->type, "table");
}

static uint32_t ASParameterSize(ModInstance::Parameter* self) {
    return self->parameters.size();
}

static std::string ASParameterAsString(ModInstance::Parameter* self) {
    return std::string(self->value);
}

static std::string ASParameterGetName(ModInstance::Parameter* self) {
    return std::string(self->name);
}

static bool ASParameterContains(ModInstance::Parameter* self, const std::string& val) {
    for (auto& parameter : self->parameters) {
        if (strmtch(parameter.value, val.c_str())) {
            return true;
        }
    }
    return false;
}

static bool ASParameterContainsName(ModInstance::Parameter* self, const std::string& val) {
    for (auto& parameter : self->parameters) {
        if (strmtch(parameter.name, val.c_str())) {
            return true;
        }
    }
    return false;
}

// Data parsed from the level structure
struct ASLevelDetails {
    char name[128];
};

static void ASLevelDetailsConstruct(ASLevelDetails* obj) {
    obj->name[0] = '\0';
}

static std::string ASLevelDetailsGetName(ASLevelDetails* obj) {
    return std::string(obj->name);
}

void AttachLevelDetails(ASContext* context) {
    context->RegisterObjectType("LevelDetails", sizeof(ASLevelDetails), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_C);
    context->RegisterObjectBehaviour("LevelDetails", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ASLevelDetailsConstruct), asCALL_CDECL_OBJLAST);

    context->RegisterObjectMethod("LevelDetails", "string GetName()", asFUNCTION(ASLevelDetailsGetName), asCALL_CDECL_OBJFIRST);

    context->DocsCloseBrace();
}

static void ASModIDConstructor(void* memory) {
    new (memory) ModID();
}

static void ASModIDDestructor(void* memory) {
    ((ModID*)memory)->~ModID();
}

static bool ASModIDValid(void* memory) {
    return ((ModID*)memory)->Valid();
}

static void ASMenuItemConstructor(void* memory) {
    new (memory) ModInstance::MenuItem();
}

static void ASMenuItemDestructor(void* memory) {
    ((ModInstance::MenuItem*)memory)->~MenuItem();
}

static std::string ASMenuItemGetTitle(void* memory) {
    return std::string(((ModInstance::MenuItem*)memory)->title);
}

static std::string ASMenuItemGetCategory(void* memory) {
    return std::string(((ModInstance::MenuItem*)memory)->category);
}

static std::string ASMenuItemGetPath(void* memory) {
    return std::string(((ModInstance::MenuItem*)memory)->path);
}

static std::string ASMenuItemGetThumbnail(void* memory) {
    return std::string(((ModInstance::MenuItem*)memory)->thumbnail);
}

static void ASSpawnerItemConstructor(void* memory) {
    new (memory) ModInstance::Item();
}

static void ASSpawnerItemDestructor(void* memory) {
    ((ModInstance::Item*)memory)->~Item();
}

static std::string ASSpawnerItemGetTitle(void* memory) {
    return std::string(((ModInstance::Item*)memory)->title);
}

static std::string ASSpawnerItemGetCategory(void* memory) {
    return std::string(((ModInstance::Item*)memory)->category);
}

static std::string ASSpawnerItemGetPath(void* memory) {
    return std::string(((ModInstance::Item*)memory)->path);
}

static std::string ASSpawnerItemGetThumbnail(void* memory) {
    return std::string(((ModInstance::Item*)memory)->thumbnail);
}
static bool ASModIsActive(ModID& sid) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        return mod->IsActive();
    } else {
        return false;
    }
}

static bool ASModNeedsRestart(ModID& sid) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        return mod->NeedsRestart();
    } else {
        return false;
    }
}

static bool ASModIsValid(ModID& sid) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        return mod->IsValid();
    } else {
        return false;
    }
}

static bool ASModIsCore(ModID& sid) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        return mod->IsCore();
    } else {
        return false;
    }
}

static bool ASModCanActivate(ModID& sid) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        return mod->CanActivate();
    } else {
        return false;
    }
}

static int ASModGetSource(ModID& sid) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        return mod->modsource;
    } else {
        return 0;
    }
}

static std::string ASModGetID(ModID& sid) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        return std::string(mod->id);
    } else {
        return std::string();
    }
}

static std::string ASModGetName(ModID& sid) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        return std::string(mod->name);
    } else {
        return std::string();
    }
}

static bool ASModGetSupportsOnline(ModID& sid) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        return mod->SupportsOnline();
    } else {
        return false;
    }
}

static bool ASModGetSupportsCurrentVersion(ModID& sid) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        return mod->ExplicitVersionSupport();
    } else {
        return false;
    }
}

static std::string ASModGetAuthor(ModID& sid) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        return std::string(mod->author);
    } else {
        return std::string();
    }
}

static std::string ASModGetCategory(ModID& sid) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        return std::string(mod->category);
    } else {
        return std::string();
    }
}

static std::string ASModGetDescription(ModID& sid) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        return std::string(mod->description);
    } else {
        return std::string();
    }
}

static std::string ASModGetVersion(ModID& sid) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        return std::string(mod->version);
    } else {
        return std::string();
    }
}

static std::string ASModGetTags(ModID& sid) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        return std::string(mod->GetTagsListString());
    } else {
        return std::string();
    }
}

static bool ASModActivation(ModID& sid, bool active) {
    ModInstance* inst = ModLoading::Instance().GetMod(sid);
    if (inst) {
        return inst->Activate(active);
    } else {
        return false;
    }
}

static std::string ASGetModPath(ModID& sid) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        return mod->path;
    } else {
        return std::string();
    }
}

static std::string ASGetModValidityString(ModID& sid) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        return mod->GetValidityErrors();
    } else {
        return "";
    }
}

static std::string ASGetModThumbnail(ModID& sid) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        return mod->thumbnail.str();
    } else {
        return "";
    }
}

static IMImage* ASGetModThumbnailImage(ModID& sid) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    IMImage* image = NULL;
    if (mod) {
        Path thumb = mod->GetFullAbsThumbnailPath();
        if (thumb.isValid() && IsImageFile(thumb)) {
            return IMImage::ASFactoryPath(thumb);
        } else {
            return IMImage::ASFactory("Images/thumb_fallback.png");
        }
    } else {
        return IMImage::ASFactory("Images/thumb_fallback.png");
    }
}

static CScriptArray* ASGetModSids() {
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<ModID>"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);

    std::vector<ModID> sids = ModLoading::Instance().GetModsSid();

    array->Reserve(sids.size());

    for (auto& sid : sids) {
        array->InsertLast((void*)&sid);
    }
    return array;
}

static CScriptArray* ASGetActiveModSids() {
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<ModID>"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);

    std::vector<ModID> sids = ModLoading::Instance().GetModsSid();

    array->Reserve(sids.size());

    for (auto& sid : sids) {
        if (ModLoading::Instance().IsActive(sid)) {
            array->InsertLast((void*)&sid);
        }
    }
    return array;
}

static CScriptArray* ASModGetMenuItems(ModID& sid) {
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<MenuItem>"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);

    ModInstance* mod = ModLoading::Instance().GetMod(sid);

    if (mod) {
        std::vector<ModInstance::MenuItem>& mmi = mod->main_menu_items;

        array->Reserve(mmi.size());

        for (auto& i : mmi) {
            array->InsertLast((void*)&i);
        }
    }
    return array;
}

static CScriptArray* ASModGetSpawnerItems(ModID& sid) {
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<SpawnerItem>"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);

    ModInstance* mod = ModLoading::Instance().GetMod(sid);

    if (mod) {
        std::vector<ModInstance::Item>& msi = mod->items;

        array->Reserve(msi.size());

        for (auto& i : msi) {
            array->InsertLast((void*)&i);
        }
    }

    return array;
}

static CScriptArray* ASModGetAllSpawnerItems(bool only_include_active) {
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<SpawnerItem>"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);

    const std::vector<ModInstance*>& mods = ModLoading::Instance().GetMods();

    unsigned item_count = 0;

    for (auto mod : mods) {
        if (!only_include_active || mod->IsActive()) {
            item_count += mod->items.size();
        }
    }

    array->Reserve(item_count);

    for (auto mod : mods) {
        if (!only_include_active || mod->IsActive()) {
            const std::vector<ModInstance::Item>& msi = mod->items;

            for (const auto& i : msi) {
                array->InsertLast((void*)&i);
            }
        }
    }

    return array;
}

static ModInstance::Campaign AsModGetCampaign(ModID& sid) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);

    if (mod && !mod->campaigns.empty()) {
        return mod->campaigns[0];
    } else {
        return ModInstance::Campaign();
    }
}

static CScriptArray* ASModGetCampaigns(ModID& sid) {
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<Campaign>"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);

    ModInstance* mod = ModLoading::Instance().GetMod(sid);

    if (mod) {
        std::vector<ModInstance::Campaign>& campaigns = mod->campaigns;

        array->Reserve(campaigns.size());

        for (auto& campaign : campaigns) {
            array->InsertLast((void*)&campaign);
        }
    }
    return array;
}

static ModInstance::Campaign ASGetCampaign(std::string& campaign_id) {
    return ModLoading::Instance().GetCampaign(campaign_id);
}

static CScriptArray* ASGetCampaigns() {
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<Campaign>"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);

    std::vector<ModInstance::Campaign> campaigns = ModLoading::Instance().GetCampaigns();

    array->Reserve(campaigns.size());

    for (auto& campaign : campaigns) {
        array->InsertLast((void*)&campaign);
    }

    return array;
}

static void ASLevelConstructor(void* m) {
    new (m) ModInstance::Level();
}

static void ASLevelCopyConstructor(const ModInstance::Level& other, void* m) {
    new (m) ModInstance::Level(other);
}

static void ASLevelDestructor(void* m) {
    ((ModInstance::Level*)m)->~Level();
}

static ModInstance::Level& ASLevelOpAssign(ModInstance::Level* self, const ModInstance::Level& other) {
    *self = other;
    return *self;
}

static std::string ASLevelGetTitle(void* m) {
    return std::string(((ModInstance::Level*)m)->title);
}

static std::string ASLevelGetID(void* m) {
    return std::string(((ModInstance::Level*)m)->id);
}

static bool ASLevelGetSupportsOnline(void* m) {
    return ((ModInstance::Level*)m)->supports_online;
}

static bool ASLevelGetRequiresOnline(void* m) {
    return ((ModInstance::Level*)m)->requires_online;
}

static std::string ASLevelGetThumbnail(void* m) {
    return std::string(((ModInstance::Level*)m)->thumbnail);
}

static std::string ASLevelGetPath(void* m) {
    return std::string(((ModInstance::Level*)m)->path);
}

static ASLevelDetails ASLevelGetLevelDetails(void* m) {
    ASLevelDetails li;
    ASLevelDetailsConstruct(&li);

    std::string path = AssemblePath("Data/Levels/", ((ModInstance::Level*)m)->path);
    if (FileExists(path, kAnyPath)) {
        LevelInfoAssetRef levelinfo = Engine::Instance()->GetAssetManager()->LoadSync<LevelInfoAsset>(AssemblePath("Data/Levels/", ((ModInstance::Level*)m)->path));

        if (levelinfo.valid()) {
            strscpy(li.name, levelinfo->GetLevelName().c_str(), 128);
        } else {
            LOGW << "Failed to load levelinfo for " << ((ModInstance::Level*)m)->path << std::endl;
        }
    } else {
        LOGW << "Unable to load level details for " << path << std::endl;
    }

    return li;
}

static bool ASLevelCompletionOptional(void* m) {
    return ((ModInstance::Level*)m)->completion_optional;
}

static ModInstance::Parameter ASLevelGetParameter(void* m) {
    ModInstance::Level* level = static_cast<ModInstance::Level*>(m);
    return level->parameter;
}

static void ASCampaignConstructor(void* m) {
    new (m) ModInstance::Campaign();
}

static void ASCampaignCopyConstructor(const ModInstance::Campaign& other, void* m) {
    new (m) ModInstance::Campaign(other);
}

static void ASCampaignDestructor(void* m) {
    ((ModInstance::Campaign*)m)->~Campaign();
}

static ModInstance::Parameter ASCampaignGetParameter(void* m) {
    ModInstance::Campaign* level = static_cast<ModInstance::Campaign*>(m);
    return level->parameter;
}

static ModInstance::Campaign& ASCampaignOpAssign(ModInstance::Campaign* self, const ModInstance::Campaign& other) {
    *self = other;
    return *self;
}

static std::string ASCampaignGetID(void* m) {
    return std::string(((ModInstance::Campaign*)m)->id);
}

static std::string ASCampaignGetTitle(void* m) {
    return std::string(((ModInstance::Campaign*)m)->title);
}

static bool ASCampaignGetSupportsOnline(void* m) {
    return (((ModInstance::Campaign*)m)->supports_online);
}

static bool ASCampaignGetRequiresOnline(void* m) {
    return (((ModInstance::Campaign*)m)->requires_online);
}

static std::string ASCampaignGetThumbnail(void* m) {
    return std::string(((ModInstance::Campaign*)m)->thumbnail);
}

static std::string ASCampaignGetMainScript(void* m) {
    return std::string(((ModInstance::Campaign*)m)->main_script);
}

static std::string ASCampaignGetMenuScript(void* m) {
    return std::string(((ModInstance::Campaign*)m)->menu_script);
}

static std::string ASCampaignGetAttribute(void* m, std::string& id) {
    std::vector<ModInstance::Attribute>& attributes = ((ModInstance::Campaign*)m)->attributes;
    for (auto& attribute : attributes) {
        if (strmtch(attribute.id, id.c_str())) {
            return attribute.value.str();
        }
    }
    return "";
}

static CScriptArray* ASCampaignGetLevels(void* m) {
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<ModLevel>"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);

    std::vector<ModInstance::Level> mmi = ((ModInstance::Campaign*)m)->levels;

    array->Reserve(mmi.size());

    for (auto& i : mmi) {
        array->InsertLast((void*)&i);
    }

    return array;
}

static ModInstance::Level ASCampaignGetLevel(ModInstance::Campaign* m, const std::string& id) {
    for (auto& level : m->levels) {
        if (strmtch(id, level.id)) {
            return level;
        }
    }
    return ModInstance::Level();
}

static CScriptArray* ASModGetModCampaignLevels(ModID& sid) {
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<ModLevel>"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);

    ModInstance* mod = ModLoading::Instance().GetMod(sid);

    if (mod && !mod->campaigns.empty()) {
        std::vector<ModInstance::Level>& mmi = mod->campaigns[0].levels;

        array->Reserve(mmi.size());

        for (auto& i : mmi) {
            array->InsertLast((void*)&i);
        }
    }
    return array;
}

static CScriptArray* ASModGetModSingleLevels(ModID& sid) {
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<ModLevel>"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);

    ModInstance* mod = ModLoading::Instance().GetMod(sid);

    if (mod) {
        std::vector<ModInstance::Level>& mmi = mod->levels;

        array->Reserve(mmi.size());

        for (auto& i : mmi) {
            array->InsertLast((void*)&i);
        }
    }
    return array;
}

static ModInstance::UserVote ASModGetModUserVote(ModID& sid) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        return mod->GetUserVote();
    }
    return ModInstance::k_VoteUnknown;
}

static void ASRequestModSetUserVote(ModID& sid, bool voteup) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        mod->RequestVoteSet(voteup);
    }
}

static void ASRequestModSetFavorite(ModID& sid, bool fav) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        mod->RequestFavoriteSet(fav);
    }
}

static bool ASModIsFavorite(ModID& sid) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        return mod->IsFavorite();
    } else {
        return false;
    }
}

static void ASRequestWorkshopSubscribe(ModID& sid) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        mod->RequestSubscribe();
    }
}

static void ASRequestWorkshopUnSubscribe(ModID& sid) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        mod->RequestUnsubscribe();
    }
}

static bool ASIsWorkshopSubscribed(ModID& sid) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        return mod->IsSubscribed();
    } else {
        return true;
    }
}

static bool ASIsWorkshopMod(ModID& sid) {
    ModInstance* mod = ModLoading::Instance().GetMod(sid);
    if (mod) {
        return mod->modsource == ModSourceSteamworks;
    } else {
        return true;
    }
}

static void ASOpenModWorkshopPage(ModID& id) {
#if ENABLE_STEAMWORKS
    Steamworks::Instance()->OpenWebPageToMod(id);
#endif
}

static void ASOpenModAuthorWorkshopPage(ModID& id) {
#if ENABLE_STEAMWORKS
    Steamworks::Instance()->OpenWebPageToModAuthor(id);
#endif
}

static void ASOpenWorkshop() {
#if ENABLE_STEAMWORKS
    Steamworks::Instance()->OpenWebPageToWorkshop();
#endif
}

static void ASDeactivateAllMods() {
    std::vector<ModInstance*> mods = ModLoading::Instance().GetAllMods();
    for (auto& mod : mods) {
        mod->Activate(false);
    }
}

static bool ASIsWorkshopAvailable() {
#if ENABLE_STEAMWORKS
    return Steamworks::Instance()->UserCanAccessWorkshop();
#else
    return false;
#endif
}

static void ASSaveModConfig() {
    LOGI << "Saving mod config" << std::endl;
    ModLoading::Instance().SaveModConfig();
}

uint32_t ASWorkshopSubscribedNotInstalledCount() {
#if ENABLE_STEAMWORKS
    if (Steamworks::Instance()->GetUGC()) {
        return Steamworks::Instance()->GetUGC()->SubscribedNotInstalledCount();
    }
#endif

    return 0;
}

uint32_t ASWorkshopDownloadingCount() {
#if ENABLE_STEAMWORKS
    if (Steamworks::Instance()->GetUGC()) {
        return Steamworks::Instance()->GetUGC()->DownloadingCount();
    }
#endif
    return 0;
}

uint32_t ASWorkshopDownloadPendingCount() {
#if ENABLE_STEAMWORKS
    if (Steamworks::Instance()->GetUGC()) {
        return Steamworks::Instance()->GetUGC()->DownloadPendingCount();
    }
#endif
    return 0;
}

uint32_t ASWorkshopNeedsUpdateCount() {
#if ENABLE_STEAMWORKS
    if (Steamworks::Instance()->GetUGC()) {
        return Steamworks::Instance()->GetUGC()->NeedsUpdateCount();
    }
#endif
    return 0;
}

float ASWorkshopTotalDownloadProgress() {
#if ENABLE_STEAMWORKS
    if (Steamworks::Instance()->GetUGC()) {
        return Steamworks::Instance()->GetUGC()->TotalDownloadProgress();
    }
#endif
    return 0.0f;
}

void AttachModding(ASContext* context) {
    context->RegisterEnum("UserVote");

    context->RegisterEnumValue("UserVote", "k_VoteUnknown", ModInstance::k_VoteUnknown);
    context->RegisterEnumValue("UserVote", "k_VoteNone", ModInstance::k_VoteNone);
    context->RegisterEnumValue("UserVote", "k_VoteUp", ModInstance::k_VoteUp);
    context->RegisterEnumValue("UserVote", "k_VoteDown", ModInstance::k_VoteDown);

    context->RegisterObjectType("Parameter", sizeof(ModInstance::Parameter), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
    context->RegisterObjectBehaviour("Parameter", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ASParameterConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("Parameter", asBEHAVE_CONSTRUCT, "void f(const Parameter &in other)", asFUNCTION(ASParameterCopyConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("Parameter", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(ASParameterDestructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectMethod("Parameter", "Parameter& opAssign(const Parameter &in other)", asFUNCTION(ASParameterOpAssign), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectMethod("Parameter", "Parameter opIndex( const string &in )", asFUNCTION(ParameterConstStringIndex), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Parameter", "Parameter opIndex( const int &in )", asFUNCTION(ParameterConstIntIndex), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectMethod("Parameter", "string getName()", asFUNCTION(ASParameterGetName), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectMethod("Parameter", "bool isEmpty()", asFUNCTION(ASParameterIsEmpty), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Parameter", "bool isString()", asFUNCTION(ASParameterIsString), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Parameter", "bool isArray()", asFUNCTION(ASParameterIsArray), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Parameter", "bool isTable()", asFUNCTION(ASParameterIsTable), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Parameter", "uint size()", asFUNCTION(ASParameterSize), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectMethod("Parameter", "string asString()", asFUNCTION(ASParameterAsString), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectMethod("Parameter", "bool contains(const string &in value)", asFUNCTION(ASParameterContains), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Parameter", "bool containsName(const string &in value)", asFUNCTION(ASParameterContainsName), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectType("ModID", sizeof(ModID), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CD);

    context->RegisterObjectBehaviour("ModID", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ASModIDConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("ModID", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(ASModIDDestructor), asCALL_CDECL_OBJLAST);

    context->RegisterObjectMethod("ModID", "bool Valid()", asFUNCTION(ASModIDValid), asCALL_CDECL_OBJFIRST);
    context->DocsCloseBrace();

    context->RegisterObjectType("MenuItem", sizeof(ModInstance::MenuItem), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CD);
    context->RegisterObjectBehaviour("MenuItem", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ASMenuItemConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("MenuItem", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(ASMenuItemDestructor), asCALL_CDECL_OBJLAST);

    context->RegisterObjectMethod("MenuItem", "string GetTitle()", asFUNCTION(ASMenuItemGetTitle), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("MenuItem", "string GetCategory()", asFUNCTION(ASMenuItemGetCategory), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("MenuItem", "string GetPath()", asFUNCTION(ASMenuItemGetPath), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("MenuItem", "string GetThumbnail()", asFUNCTION(ASMenuItemGetThumbnail), asCALL_CDECL_OBJFIRST);
    context->DocsCloseBrace();

    context->RegisterObjectType("SpawnerItem", sizeof(ModInstance::Item), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CD);
    context->RegisterObjectBehaviour("SpawnerItem", asBEHAVE_CONSTRUCT, "void SpawnerItem()", asFUNCTION(ASSpawnerItemConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("SpawnerItem", asBEHAVE_DESTRUCT, "void SpawnerItem()", asFUNCTION(ASSpawnerItemDestructor), asCALL_CDECL_OBJLAST);

    context->RegisterObjectMethod("SpawnerItem", "string GetTitle()", asFUNCTION(ASSpawnerItemGetTitle), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("SpawnerItem", "string GetCategory()", asFUNCTION(ASSpawnerItemGetCategory), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("SpawnerItem", "string GetPath()", asFUNCTION(ASSpawnerItemGetPath), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("SpawnerItem", "string GetThumbnail()", asFUNCTION(ASSpawnerItemGetThumbnail), asCALL_CDECL_OBJFIRST);
    context->DocsCloseBrace();

    context->RegisterObjectType("ModLevel", sizeof(ModInstance::Level), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);

    context->RegisterObjectBehaviour("ModLevel", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ASLevelConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("ModLevel", asBEHAVE_CONSTRUCT, "void f(const ModLevel &in other)", asFUNCTION(ASLevelCopyConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("ModLevel", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(ASLevelDestructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectMethod("ModLevel", "ModLevel& opAssign(const ModLevel &in other)", asFUNCTION(ASLevelOpAssign), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectMethod("ModLevel", "string GetTitle()", asFUNCTION(ASLevelGetTitle), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("ModLevel", "string GetID()", asFUNCTION(ASLevelGetID), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("ModLevel", "string GetThumbnail()", asFUNCTION(ASLevelGetThumbnail), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("ModLevel", "bool GetSupportsOnline()", asFUNCTION(ASLevelGetSupportsOnline), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("ModLevel", "bool GetRequiresOnline()", asFUNCTION(ASLevelGetRequiresOnline), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("ModLevel", "string GetPath()", asFUNCTION(ASLevelGetPath), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("ModLevel", "LevelDetails GetDetails()", asFUNCTION(ASLevelGetLevelDetails), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("ModLevel", "bool CompletionOptional()", asFUNCTION(ASLevelCompletionOptional), asCALL_CDECL_OBJFIRST);

    context->RegisterObjectMethod("ModLevel", "Parameter GetParameter()", asFUNCTION(ASLevelGetParameter), asCALL_CDECL_OBJFIRST);
    context->DocsCloseBrace();

    context->RegisterObjectType("Campaign", sizeof(ModInstance::Campaign), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
    context->RegisterObjectBehaviour("Campaign", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ASCampaignConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("Campaign", asBEHAVE_CONSTRUCT, "void f(const Campaign &in other)", asFUNCTION(ASCampaignCopyConstructor), asCALL_CDECL_OBJLAST);
    context->RegisterObjectBehaviour("Campaign", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(ASCampaignDestructor), asCALL_CDECL_OBJLAST);

    context->RegisterObjectMethod("Campaign", "Campaign& opAssign(const Campaign &in other)", asFUNCTION(ASCampaignOpAssign), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Campaign", "string GetID()", asFUNCTION(ASCampaignGetID), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Campaign", "string GetTitle()", asFUNCTION(ASCampaignGetTitle), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Campaign", "bool GetSupportsOnline()", asFUNCTION(ASCampaignGetSupportsOnline), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Campaign", "bool GetRequiresOnline()", asFUNCTION(ASCampaignGetRequiresOnline), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Campaign", "string GetThumbnail()", asFUNCTION(ASCampaignGetThumbnail), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Campaign", "string GetMainScript()", asFUNCTION(ASCampaignGetMainScript), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Campaign", "string GetMenuScript()", asFUNCTION(ASCampaignGetMenuScript), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Campaign", "string GetAttribute(string &in id)", asFUNCTION(ASCampaignGetAttribute), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Campaign", "array<ModLevel>@ GetLevels()", asFUNCTION(ASCampaignGetLevels), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Campaign", "ModLevel GetLevel(string &in id)", asFUNCTION(ASCampaignGetLevel), asCALL_CDECL_OBJFIRST);
    context->RegisterObjectMethod("Campaign", "Parameter GetParameter()", asFUNCTION(ASCampaignGetParameter), asCALL_CDECL_OBJFIRST);

    context->RegisterGlobalFunction("Campaign GetCampaign(string& campaign_id)", asFUNCTION(ASGetCampaign), asCALL_CDECL);
    context->RegisterGlobalFunction("array<Campaign>@ GetCampaigns()", asFUNCTION(ASGetCampaigns), asCALL_CDECL);

    context->RegisterGlobalFunction("bool ModIsActive(ModID& id)", asFUNCTION(ASModIsActive), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ModNeedsRestart(ModID& id)", asFUNCTION(ASModNeedsRestart), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ModIsValid(ModID& id)", asFUNCTION(ASModIsValid), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ModIsCore(ModID& id)", asFUNCTION(ASModIsCore), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ModCanActivate(ModID& id)", asFUNCTION(ASModCanActivate), asCALL_CDECL);
    context->RegisterGlobalFunction("int ModGetSource(ModID& id)", asFUNCTION(ASModGetSource), asCALL_CDECL);
    context->RegisterGlobalFunction("string ModGetID(ModID& id)", asFUNCTION(ASModGetID), asCALL_CDECL);
    context->RegisterGlobalFunction("string ModGetName(ModID& id)", asFUNCTION(ASModGetName), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ModGetSupportsOnline(ModID& id)", asFUNCTION(ASModGetSupportsOnline), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ModGetSupportsCurrentVersion(ModID& id)", asFUNCTION(ASModGetSupportsCurrentVersion), asCALL_CDECL);
    context->RegisterGlobalFunction("string ModGetAuthor(ModID& id)", asFUNCTION(ASModGetAuthor), asCALL_CDECL);
    context->RegisterGlobalFunction("string ModGetVersion(ModID& id)", asFUNCTION(ASModGetVersion), asCALL_CDECL);
    context->RegisterGlobalFunction("string ModGetTags(ModID& id)", asFUNCTION(ASModGetTags), asCALL_CDECL);
    context->RegisterGlobalFunction("string ModGetPath(ModID& sid)", asFUNCTION(ASGetModPath), asCALL_CDECL);
    context->RegisterGlobalFunction("string ModGetValidityString(ModID& sid)", asFUNCTION(ASGetModValidityString), asCALL_CDECL);
    context->RegisterGlobalFunction("string ModGetDescription(ModID& id)", asFUNCTION(ASModGetDescription), asCALL_CDECL);
    context->RegisterGlobalFunction("string ModGetThumbnail(ModID& sid)", asFUNCTION(ASGetModThumbnail), asCALL_CDECL);
    context->RegisterGlobalFunction("array<MenuItem>@ ModGetMenuItems(ModID& sid)", asFUNCTION(ASModGetMenuItems), asCALL_CDECL);
    context->RegisterGlobalFunction("array<SpawnerItem>@ ModGetSpawnerItems(ModID& sid)", asFUNCTION(ASModGetSpawnerItems), asCALL_CDECL);
    context->RegisterGlobalFunction("array<SpawnerItem>@ ModGetAllSpawnerItems(bool only_include_active = true)", asFUNCTION(ASModGetAllSpawnerItems), asCALL_CDECL);
    context->RegisterGlobalFunction("array<ModLevel>@ ModGetCampaignLevels(ModID& sid)", asFUNCTION(ASModGetModCampaignLevels), asCALL_CDECL);
    context->RegisterGlobalFunction("array<ModLevel>@ ModGetSingleLevels(ModID& sid)", asFUNCTION(ASModGetModSingleLevels), asCALL_CDECL);
    context->RegisterGlobalFunction("UserVote ModGetUserVote(ModID& sid)", asFUNCTION(ASModGetModUserVote), asCALL_CDECL);
    context->RegisterGlobalFunction("void RequestModSetUserVote(ModID& id, bool voteup)", asFUNCTION(ASRequestModSetUserVote), asCALL_CDECL);
    context->RegisterGlobalFunction("void RequestModSetFavorite(ModID& id, bool fav)", asFUNCTION(ASRequestModSetFavorite), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ModIsFavorite(ModID& id)", asFUNCTION(ASModIsFavorite), asCALL_CDECL);

    context->RegisterGlobalFunction("array<ModID>@ GetModSids()", asFUNCTION(ASGetModSids), asCALL_CDECL);
    context->RegisterGlobalFunction("array<ModID>@ GetActiveModSids()", asFUNCTION(ASGetActiveModSids), asCALL_CDECL);
    context->RegisterGlobalFunction("bool ModActivation(ModID& sid, bool active)", asFUNCTION(ASModActivation), asCALL_CDECL);

    context->RegisterGlobalFunction("void RequestWorkshopSubscribe(ModID& id)", asFUNCTION(ASRequestWorkshopSubscribe), asCALL_CDECL);
    context->RegisterGlobalFunction("void RequestWorkshopUnSubscribe(ModID& id)", asFUNCTION(ASRequestWorkshopUnSubscribe), asCALL_CDECL);
    context->RegisterGlobalFunction("bool IsWorkshopSubscribed(ModID& id)", asFUNCTION(ASIsWorkshopSubscribed), asCALL_CDECL);
    context->RegisterGlobalFunction("bool IsWorkshopMod(ModID& id)", asFUNCTION(ASIsWorkshopMod), asCALL_CDECL);

    context->RegisterGlobalFunction("bool IsWorkshopAvailable()", asFUNCTION(ASIsWorkshopAvailable), asCALL_CDECL);

    context->RegisterGlobalFunction("void SaveModConfig()", asFUNCTION(ASSaveModConfig), asCALL_CDECL);

    context->RegisterGlobalFunction("void OpenModWorkshopPage(ModID& id)", asFUNCTION(ASOpenModWorkshopPage), asCALL_CDECL);
    context->RegisterGlobalFunction("void OpenModAuthorWorkshopPage(ModID& id)", asFUNCTION(ASOpenModAuthorWorkshopPage), asCALL_CDECL);
    context->RegisterGlobalFunction("void OpenWorkshop()", asFUNCTION(ASOpenWorkshop), asCALL_CDECL);
    context->RegisterGlobalFunction("void DeactivateAllMods()", asFUNCTION(ASDeactivateAllMods), asCALL_CDECL);

    context->RegisterGlobalFunction("uint WorkshopSubscribedNotInstalledCount()", asFUNCTION(ASWorkshopSubscribedNotInstalledCount), asCALL_CDECL);
    context->RegisterGlobalFunction("uint WorkshopDownloadingCount()", asFUNCTION(ASWorkshopDownloadingCount), asCALL_CDECL);
    context->RegisterGlobalFunction("uint WorkshopDownloadPendingCount()", asFUNCTION(ASWorkshopDownloadPendingCount), asCALL_CDECL);
    context->RegisterGlobalFunction("uint WorkshopNeedsUpdateCount()", asFUNCTION(ASWorkshopNeedsUpdateCount), asCALL_CDECL);
    context->RegisterGlobalFunction("float WorkshopTotalDownloadProgress()", asFUNCTION(ASWorkshopTotalDownloadProgress), asCALL_CDECL);
}

void AttachIMGUIModding(ASContext* context) {
    context->RegisterGlobalFunction("IMImage@ ModGetThumbnailImage(ModID& sid)", asFUNCTION(ASGetModThumbnailImage), asCALL_CDECL);
}

static std::map<std::string, std::string> storage_string;
static std::map<std::string, int32_t> storage_int32;

void ASStorageSetString(std::string index, std::string value) {
    storage_string[index] = value;
}

bool ASStorageHasString(std::string index) {
    return storage_string.find(index) != storage_string.end();
}

std::string ASStorageGetString(std::string index) {
    return storage_string[index];
}

void ASStorageSetInt32(std::string index, int32_t value) {
    storage_int32[index] = value;
}

bool ASStorageHasInt32(std::string index) {
    return storage_int32.find(index) != storage_int32.end();
}

int32_t ASStorageGetInt32(std::string index) {
    return storage_int32[index];
}

// Routine for storing angelscript data over the course of single run.
void AttachStorage(ASContext* context) {
    context->RegisterGlobalFunction("void StorageSetString(string index, string value)", asFUNCTION(ASStorageSetString), asCALL_CDECL);
    context->RegisterGlobalFunction("bool StorageHasString(string index)", asFUNCTION(ASStorageHasString), asCALL_CDECL);
    context->RegisterGlobalFunction("string StorageGetString(string index)", asFUNCTION(ASStorageGetString), asCALL_CDECL);

    context->RegisterGlobalFunction("void StorageSetInt32(string index, int value)", asFUNCTION(ASStorageSetInt32), asCALL_CDECL);
    context->RegisterGlobalFunction("bool StorageHasInt32(string index)", asFUNCTION(ASStorageHasInt32), asCALL_CDECL);
    context->RegisterGlobalFunction("int StorageGetInt32(string index)", asFUNCTION(ASStorageGetInt32), asCALL_CDECL);
}


static CScriptArray* ASGetLocaleShortcodes() {
    auto& locales = GetLocales();
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<string>"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);
    array->Reserve(locales.size());

    for (auto& locale : locales) {
        // InsertLast doesn't actually do anything but copy from the pointer,
        // so a const_cast would be fine, but maybe an update to AS could change
        // that
        std::string str = locale.first;
        array->InsertLast(&str);
    }

    return array;
}

static CScriptArray* ASGetLocaleNames() {
    auto& locales = GetLocales();
    asIScriptContext* ctx = asGetActiveContext();
    asIScriptEngine* engine = ctx->GetEngine();
    asITypeInfo* arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<string>"));
    CScriptArray* array = CScriptArray::Create(arrayType, (asUINT)0);
    array->Reserve(locales.size());

    for (auto& locale : locales) {
        // InsertLast doesn't actually do anything but copy from the pointer,
        // so a const_cast would be fine, but maybe an update to AS could change
        // that
        std::string str = locale.second;
        array->InsertLast(&str);
    }

    return array;
}

static std::string ASGetLocalizedLevelName(const std::string& shortcode, const std::string& path) {
    auto& localized_levels = GetLocalizedLevelMaps();
    auto loc_it = localized_levels.find(shortcode);
    if (loc_it != localized_levels.end()) {
        auto it = loc_it->second.find("Data/Levels/" + path);
        if (it != loc_it->second.end()) {
            return it->second.name;
        }
    }
    return "";
}

void AttachLocale(ASContext* context) {
    context->RegisterGlobalFunction("array<string>@ GetLocaleShortcodes()", asFUNCTION(ASGetLocaleShortcodes), asCALL_CDECL);
    context->RegisterGlobalFunction("array<string>@ GetLocaleNames()", asFUNCTION(ASGetLocaleNames), asCALL_CDECL);
    context->RegisterGlobalFunction("string GetLocalizedLevelName(const string &in locale_shortcode, const string &in path)", asFUNCTION(ASGetLocalizedLevelName), asCALL_CDECL);
}
