//-----------------------------------------------------------------------------
//           Name: attackscript.cpp
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
#include "attackscript.h"

#include <Math/enginemath.h>
#include <Math/vec3math.h>

#include <Scripting/angelscript/ascontext.h>
#include <Main/engine.h>

#include <stack>
#include <sstream>

extern std::stack<ASContext *> active_context_stack;

static const std::string attack_getter_empty_path = "";
static const std::string attack_getter_empty_material_event = "";

bool AttackScriptGetter::Load(std::string _path) {
    if (_path.empty()) {
        return false;
    }
    path_ = _path;
    if (_path[_path.size() - 2] == ' ' &&
        _path[_path.size() - 1] == 'm') {
        mirror_ = true;
        _path.resize(_path.size() - 2);
    } else {
        mirror_ = false;
    }
    // Attacks::Instance()->ReturnRef(_path);
    attack_ref_ = Engine::Instance()->GetAssetManager()->LoadSync<Attack>(_path);
    return true;
}

std::string AttackScriptGetter::GetUnblockedAnimPath() {
    return attack_ref_.valid() ? attack_ref_->unblocked_anim_path : attack_getter_empty_path;
}

std::string AttackScriptGetter::GetBlockedAnimPath() {
    return attack_ref_.valid() ? attack_ref_->blocked_anim_path : attack_getter_empty_path;
}

std::string AttackScriptGetter::GetThrowAnimPath() {
    return attack_ref_.valid() ? attack_ref_->throw_anim_path : attack_getter_empty_path;
}

std::string AttackScriptGetter::GetThrownAnimPath() {
    return attack_ref_.valid() ? attack_ref_->thrown_anim_path : attack_getter_empty_path;
}

std::string AttackScriptGetter::GetSpecial() {
    return attack_ref_.valid() ? attack_ref_->special : attack_getter_empty_path;
}

int AttackScriptGetter::GetHeight() {
    return attack_ref_.valid() ? attack_ref_->height : AttackHeight::_medium;
}

int AttackScriptGetter::GetDirection() {
    return attack_ref_.valid() ? attack_ref_->direction : AttackDirection::_front;
}

int AttackScriptGetter::GetSwapStance() {
    return (int)(attack_ref_.valid() ? attack_ref_->swap_stance : false);
}

int AttackScriptGetter::GetSwapStanceBlocked() {
    return (int)(attack_ref_.valid() ? attack_ref_->swap_stance_blocked : false);
}

int AttackScriptGetter::GetUnblockable() {
    return (int)(attack_ref_.valid() ? attack_ref_->unblockable : false);
}

std::string AttackScriptGetter::GetMaterialEvent() {
    return attack_ref_.valid() ? attack_ref_->materialevent : attack_getter_empty_material_event;
}

std::string AttackScriptGetter::GetReactionPath() {
    if (attack_ref_.valid()) {
        if (attack_ref_->direction == _front || !mirror_) {
            return attack_ref_->reaction;
        } else {
            return attack_ref_->reaction + " m";
        }
    } else {
        return attack_getter_empty_path;
    }
}

vec3 AttackScriptGetter::GetImpactDir() {
    if (attack_ref_.valid()) {
        if (!mirror_) {
            return attack_ref_->impact_dir;
        } else {
            return attack_ref_->impact_dir * vec3(-1.0f, 1.0f, 1.0f);
        }
    } else {
        return vec3();
    }
}

float AttackScriptGetter::GetBlockDamage() {
    return attack_ref_.valid() ? RangedRandomFloat(attack_ref_->block_damage[0], attack_ref_->block_damage[1]) : 0.0f;
}

float AttackScriptGetter::GetSharpDamage() {
    return attack_ref_.valid() ? RangedRandomFloat(attack_ref_->sharp_damage[0], attack_ref_->sharp_damage[1]) : 0.0f;
}

float AttackScriptGetter::GetDamage() {
    return attack_ref_.valid() ? RangedRandomFloat(attack_ref_->damage[0], attack_ref_->damage[1]) : 0.0f;
}

float AttackScriptGetter::GetForce() {
    return attack_ref_.valid() ? RangedRandomFloat(attack_ref_->force[0], attack_ref_->force[1]) : 0.0f;
}

bool AttackScriptGetter::HasRangeAdjust() {
    return attack_ref_.valid() ? attack_ref_->has_range_adjust : false;
}

float AttackScriptGetter::GetRangeAdjust() {
    return attack_ref_.valid() ? attack_ref_->range_adjust : 0.0f;
}

std::string AttackScriptGetter::GetPath() {
    return path_;
}

AttackScriptGetter *AttackScriptGetter_Factory() {
    return new AttackScriptGetter();
}

static void ASLoad(AttackScriptGetter *asg, const std::string &path) {
    if (!asg->Load(path)) {
        std::string callstack = active_context_stack.top()->GetCallstack();
        FatalError("Error", "Attempting to load attack with empty path\n Called from:\n%s", callstack.c_str());
    }
}

void AttachAttackScriptGetter(ASContext *as_context) {
    as_context->RegisterObjectType("AttackScriptGetter", 0, asOBJ_REF, "Used to query information from an attack xml file");
    as_context->RegisterObjectBehaviour("AttackScriptGetter", asBEHAVE_FACTORY, "AttackScriptGetter@ f()", asFUNCTION(AttackScriptGetter_Factory), asCALL_CDECL);
    as_context->RegisterObjectBehaviour("AttackScriptGetter", asBEHAVE_ADDREF, "void AttackScriptGetter()", asMETHOD(AttackScriptGetter, AddRef), asCALL_THISCALL);
    as_context->RegisterObjectBehaviour("AttackScriptGetter", asBEHAVE_RELEASE, "void AttackScriptGetter()", asMETHOD(AttackScriptGetter, ReleaseRef), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "void Load(const string &in path)", asFUNCTION(ASLoad), asCALL_CDECL_OBJFIRST, "Load an attack xml file into the attack script getter (e.g. \"Data/Attacks/knifeslash.xml\")");
    as_context->RegisterObjectMethod("AttackScriptGetter", "string GetPath()", asMETHOD(AttackScriptGetter, GetPath), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "string GetSpecial()", asMETHOD(AttackScriptGetter, GetSpecial), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "string GetUnblockedAnimPath()", asMETHOD(AttackScriptGetter, GetUnblockedAnimPath), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "string GetBlockedAnimPath()", asMETHOD(AttackScriptGetter, GetBlockedAnimPath), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "string GetThrowAnimPath()", asMETHOD(AttackScriptGetter, GetThrowAnimPath), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "string GetThrownAnimPath()", asMETHOD(AttackScriptGetter, GetThrownAnimPath), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "string GetThrownCounterAnimPath()", asMETHOD(AttackScriptGetter, GetThrownCounterAnimPath), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "int IsThrow()", asMETHOD(AttackScriptGetter, IsThrow), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "int GetHeight()", asMETHOD(AttackScriptGetter, GetHeight), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "vec3 GetCutPlane()", asMETHOD(AttackScriptGetter, GetCutPlane), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "int GetCutPlaneType()", asMETHOD(AttackScriptGetter, GetCutPlaneType), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "bool HasCutPlane()", asMETHOD(AttackScriptGetter, HasCutPlane), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "vec3 GetStabDir()", asMETHOD(AttackScriptGetter, GetStabDir), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "int GetStabDirType()", asMETHOD(AttackScriptGetter, GetStabDirType), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "bool HasStabDir()", asMETHOD(AttackScriptGetter, HasStabDir), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "int GetDirection()", asMETHOD(AttackScriptGetter, GetDirection), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "int GetSwapStance()", asMETHOD(AttackScriptGetter, GetSwapStance), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "int GetSwapStanceBlocked()", asMETHOD(AttackScriptGetter, GetSwapStanceBlocked), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "int GetUnblockable()", asMETHOD(AttackScriptGetter, GetUnblockable), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "int GetFleshUnblockable()", asMETHOD(AttackScriptGetter, GetFleshUnblockable), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "int GetAsLayer()", asMETHOD(AttackScriptGetter, GetAsLayer), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "string GetAlternate()", asMETHOD(AttackScriptGetter, GetAlternate), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "string GetReactionPath()", asMETHOD(AttackScriptGetter, GetReactionPath), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "string GetMaterialEvent()", asMETHOD(AttackScriptGetter, GetMaterialEvent), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "vec3 GetImpactDir()", asMETHOD(AttackScriptGetter, GetImpactDir), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "float GetBlockDamage()", asMETHOD(AttackScriptGetter, GetBlockDamage), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "float GetSharpDamage()", asMETHOD(AttackScriptGetter, GetSharpDamage), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "float GetDamage()", asMETHOD(AttackScriptGetter, GetDamage), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "float GetForce()", asMETHOD(AttackScriptGetter, GetForce), asCALL_THISCALL);

    // Glimpse - Range Adjust
    as_context->RegisterObjectMethod("AttackScriptGetter", "bool HasRangeAdjust()", asMETHOD(AttackScriptGetter, HasRangeAdjust), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "float GetRangeAdjust()", asMETHOD(AttackScriptGetter, GetRangeAdjust), asCALL_THISCALL);
    // Glimpse - Range Adjust

    as_context->RegisterObjectMethod("AttackScriptGetter", "int GetMirrored()", asMETHOD(AttackScriptGetter, GetMirrored), asCALL_THISCALL);
    as_context->RegisterObjectMethod("AttackScriptGetter", "int GetMobile()", asMETHOD(AttackScriptGetter, GetMobile), asCALL_THISCALL);
    as_context->DocsCloseBrace();

    static const int const_high = _high;
    static const int const_medium = _medium;
    static const int const_low = _low;
    static const int const_right = _right;
    static const int const_front = _front;
    static const int const_left = _left;

    as_context->RegisterGlobalProperty("const int _high", (void *)&const_high);
    as_context->RegisterGlobalProperty("const int _medium", (void *)&const_medium);
    as_context->RegisterGlobalProperty("const int _low", (void *)&const_low);
    as_context->RegisterGlobalProperty("const int _right", (void *)&const_right);
    as_context->RegisterGlobalProperty("const int _front", (void *)&const_front);
    as_context->RegisterGlobalProperty("const int _left", (void *)&const_left);
}

int AttackScriptGetter::GetMirrored() {
    return (int)mirror_;
}

int AttackScriptGetter::GetMobile() {
    return (int)(attack_ref_.valid() ? attack_ref_->mobile : false);
}

int AttackScriptGetter::IsThrow() {
    if (!attack_ref_.valid()) {
        return 0;
    }
    return (int)(!(attack_ref_->throw_anim_path.empty() &&
                   attack_ref_->thrown_anim_path.empty()));
}

vec3 AttackScriptGetter::GetCutPlane() {
    return attack_ref_.valid() ? attack_ref_->cut_plane : vec3();
}

int AttackScriptGetter::GetCutPlaneType() {
    return attack_ref_.valid() ? attack_ref_->cut_plane_type : CutPlaneType::_heavy;
}

bool AttackScriptGetter::HasCutPlane() {
    return attack_ref_.valid() ? attack_ref_->has_cut_plane : false;
}

vec3 AttackScriptGetter::GetStabDir() {
    return attack_ref_.valid() ? attack_ref_->stab_dir : vec3();
}

int AttackScriptGetter::GetStabDirType() {
    return attack_ref_.valid() ? attack_ref_->stab_type : CutPlaneType::_heavy;
}

bool AttackScriptGetter::HasStabDir() {
    return attack_ref_.valid() ? attack_ref_->has_stab_dir : false;
}

int AttackScriptGetter::GetFleshUnblockable() {
    return (int)(attack_ref_.valid() ? attack_ref_->flesh_unblockable : false);
}

int AttackScriptGetter::GetAsLayer() {
    return (int)(attack_ref_.valid() ? attack_ref_->as_layer : false);
}

std::string AttackScriptGetter::GetAlternate() {
    return attack_ref_.valid() ? attack_ref_->alternate : attack_getter_empty_path;
}

std::string AttackScriptGetter::GetThrownCounterAnimPath() {
    return attack_ref_.valid() ? attack_ref_->thrown_counter_anim_path : attack_getter_empty_path;
}

AttackScriptGetter::AttackScriptGetter() : ref_count(1) {}

void AttackScriptGetter::AddRef() {
    ++ref_count;
}

void AttackScriptGetter::ReleaseRef() {
    --ref_count;
    if (ref_count == 0) {
        delete this;
    }
}
