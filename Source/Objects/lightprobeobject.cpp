//-----------------------------------------------------------------------------
//           Name: lightprobeobject.cpp
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
#include "lightprobeobject.h"

#include <Game/EntityDescription.h>
#include <Main/scenegraph.h>
#include <Utility/assert.h>

bool LightProbeObject::Initialize() {
    LOG_ASSERT(probe_id_ == -1);
    sp.ASAddIntCheckbox("Negative", false);
    bool negative = (GetScriptParams()->ASGetInt("Negative") == 1);
    probe_id_ = scenegraph_->light_probe_collection.AddProbe(GetTranslation(), negative, stored_coefficients);

    if (stored_coefficients) {
        delete[] stored_coefficients;
        stored_coefficients = NULL;
    }
    obj_file = "light_probe_object";
    return true;
}

void LightProbeObject::SetScriptParams(const ScriptParamMap& spm) {
    Object::SetScriptParams(spm);
    if (scenegraph_) {
        bool negative = (GetScriptParams()->ASGetInt("Negative") == 1);
        bool success = scenegraph_->light_probe_collection.SetNegative(probe_id_, negative);
        LOG_ASSERT(success);
    }
}

void LightProbeObject::Moved(Object::MoveType type) {
    Object::Moved(type);
    if (scenegraph_) {
        bool success = scenegraph_->light_probe_collection.MoveProbe(probe_id_, GetTranslation());
        LOG_ASSERT(success);
    }
}

void LightProbeObject::Dispose() {
    Object::Dispose();
    if (scenegraph_) {
        bool success = scenegraph_->light_probe_collection.DeleteProbe(probe_id_);
        LOG_ASSERT(success);
        probe_id_ = -1;
    }
}

LightProbeObject::LightProbeObject() {
    box_.dims = vec3(1.0f);
    probe_id_ = -1;
    stored_coefficients = NULL;
}

LightProbeObject::~LightProbeObject() {
    LOG_ASSERT(probe_id_ == -1);
}

void LightProbeObject::GetDesc(EntityDescription& desc) const {
    LOG_ASSERT(probe_id_ != -1);
    LOG_ASSERT(scenegraph_);
    Object::GetDesc(desc);

    const size_t data_size = kLightProbeNumCoeffs * sizeof(float);
    std::vector<char> data(data_size);

    memcpy(&data[0], &scenegraph_->light_probe_collection.GetProbeFromID(probe_id_)->ambient_cube_color, data_size);
    desc.AddData(EDF_GI_COEFFICIENTS, data);
}

bool LightProbeObject::SetFromDesc(const EntityDescription& desc) {
    LOG_ASSERT(probe_id_ == -1);
    LOG_ASSERT(!stored_coefficients);
    bool ret = Object::SetFromDesc(desc);
    if (ret) {
        for (const auto& field : desc.fields) {
            switch (field.type) {
                case EDF_NEGATIVE_LIGHT_PROBE: {
                    int negative_int;
                    field.ReadInt(&negative_int);
                    sp.ASAddIntCheckbox("Negative", false);
                    sp.ASSetInt("Negative", negative_int);
                    break;
                }

                case EDF_GI_COEFFICIENTS: {
                    LOG_ASSERT(!stored_coefficients);
                    stored_coefficients = new float[kLightProbeNumCoeffs];
                    memcpy(stored_coefficients, &field.data[0], kLightProbeNumCoeffs * sizeof(float));
                    break;
                }
            }
        }
    }

    return ret;
}
