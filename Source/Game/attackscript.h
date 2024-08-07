//-----------------------------------------------------------------------------
//           Name: attackscript.h
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
#pragma once

#include <Asset/Asset/attacks.h>

class ASContext;

class AttackScriptGetter {
   public:
    AttackScriptGetter();

    AttackRef attack_ref_;
    bool mirror_;
    std::string path_;
    int ref_count;

    std::string GetUnblockedAnimPath();
    std::string GetBlockedAnimPath();
    int GetHeight();
    int GetDirection();
    int GetSwapStance();
    int GetMirrored();
    std::string GetReactionPath();
    std::string GetMaterialEvent();
    vec3 GetImpactDir();
    float GetBlockDamage();
    float GetSharpDamage();
    float GetDamage();
    float GetForce();

    // Glimpse - Range Adjust
    bool HasRangeAdjust();
    float GetRangeAdjust();
    // Glimpse - Range Adjust

    std::string GetPath();
    std::string GetSpecial();
    int GetUnblockable();
    int GetFleshUnblockable();
    int GetMobile();
    int GetSwapStanceBlocked();
    std::string GetThrowAnimPath();
    std::string GetThrownAnimPath();
    std::string GetThrownCounterAnimPath();
    int IsThrow();
    vec3 GetCutPlane();
    bool HasCutPlane();
    int GetCutPlaneType();
    vec3 GetStabDir();
    int GetStabDirType();
    bool HasStabDir();
    int GetAsLayer();
    std::string GetAlternate();

    void AddRef();
    void ReleaseRef();
    const AttackRef& attack_ref() const { return attack_ref_; }
    bool mirror() const { return mirror_; }
    bool Load(std::string path);
};

void AttachAttackScriptGetter(ASContext* as_context);
