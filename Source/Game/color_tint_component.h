//-----------------------------------------------------------------------------
//           Name: color_tint_component.h
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

#include <Game/component.h>
#include <Math/vec3.h>

class ColorTintComponent : public Phoenix::Component {
   public:
    vec3 tint_;
    float overbright_;
    ColorTintComponent() : tint_(1.0f), overbright_(0.0f) {}
    static void LoadDescriptionFromXML(EntityDescription& desc, const TiXmlElement* el);
    void SetFromDescription(const EntityDescription& desc) override;
    virtual void AddToDescription(EntityDescription& desc) const;
    void SaveToXML(TiXmlElement* el) override;
    void ReceiveObjectMessageVAList(OBJECT_MSG::Type type, va_list args) override;
};
