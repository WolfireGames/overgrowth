//-----------------------------------------------------------------------------
//           Name: color_tint_component.cpp
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

#include <Game/color_tint_component.h>
#include <Game/EntityDescription.h>

#include <Internal/memwrite.h>
#include <Editors/save_state.h>

#include <tinyxml.h>

void ColorTintComponent::LoadDescriptionFromXML( EntityDescription& desc, const TiXmlElement* el ) {
    double dval;
    vec3 color_tint(1.0f);
    float overbright = 0.0f;
    if(el->QueryDoubleAttribute("color_r",&dval)==TIXML_SUCCESS) color_tint[0] = (float)dval;
    if(el->QueryDoubleAttribute("color_g",&dval)==TIXML_SUCCESS) color_tint[1] = (float)dval;
    if(el->QueryDoubleAttribute("color_b",&dval)==TIXML_SUCCESS) color_tint[2] = (float)dval;
    if(el->QueryDoubleAttribute("overbright",&dval)==TIXML_SUCCESS) overbright = (float)dval;

    float max_val = 0.0f;
    for(int i=0; i<3; ++i){
        if(color_tint[i] > max_val){
            max_val = color_tint[i];
        }
    }

    if(max_val > 1.0f){
        for(int i=0; i<3; ++i){
            color_tint[i] /= max_val;
        }
        overbright += (max_val - 1.0f)/0.3f;
    }

    desc.AddVec3(EDF_COLOR, color_tint);
    desc.AddFloat(EDF_OVERBRIGHT, overbright);
}

void ColorTintComponent::SetFromDescription( const EntityDescription& desc ) {
    for(const auto & field : desc.fields){
        switch(field.type){
            case EDF_COLOR:
                memread(tint_.entries, sizeof(float), 3, field.data);
                break;
            case EDF_OVERBRIGHT:
                memread(&overbright_, sizeof(float), 1, field.data);
                break;
        }
    }
}

void ColorTintComponent::SaveToXML( TiXmlElement* el ) {
    el->SetDoubleAttribute("color_r", tint_[0]);
    el->SetDoubleAttribute("color_g", tint_[1]);
    el->SetDoubleAttribute("color_b", tint_[2]);        
    el->SetDoubleAttribute("overbright", overbright_);
}

void ColorTintComponent::AddToDescription( EntityDescription& desc ) const {
    desc.AddVec3(EDF_COLOR, tint_);
    desc.AddFloat(EDF_OVERBRIGHT, overbright_);
}

void ColorTintComponent::ReceiveObjectMessageVAList( OBJECT_MSG::Type type, va_list args ) {
    switch(type){
    case OBJECT_MSG::SET_COLOR:
        tint_ = *va_arg(args, vec3*);
        break;
    case OBJECT_MSG::SET_OVERBRIGHT:
        overbright_ = *va_arg(args, float*);
        break;
    default:
        break;
    }
}

