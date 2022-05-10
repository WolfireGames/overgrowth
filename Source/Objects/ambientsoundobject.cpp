//-----------------------------------------------------------------------------
//           Name: ambientsoundobject.cpp
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
#include "ambientsoundobject.h"

#include <Math/vec3math.h>
#include <Main/engine.h>

#include <Graphics/pxdebugdraw.h>
#include <Graphics/camera.h>

#include <Internal/memwrite.h>
#include <Internal/timer.h>

#include <Objects/group.h>

#include <Sound/sound.h>
#include <Asset/Asset/ambientsounds.h>
#include <Main/scenegraph.h>

#include <tinyxml.h>

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

bool AmbientSoundObject::Initialize() {
    if (as_ref->GetSoundType() == _continuous) {
        SoundPlayInfo spi;
        spi.path = as_ref->GetPath();
        spi.looping = true;
        spi.position = GetTranslation();
        spi.occlusion_position = spi.position;
        sound_handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
        Engine::Instance()->GetSound()->Play(sound_handle, spi);
    }
    return true;
}

AmbientSoundObject::AmbientSoundObject() : sound_handle(0) {
    box_.dims = vec3(1.0f);
}

AmbientSoundObject::~AmbientSoundObject() {
}

bool AmbientSoundObject::SetFromDesc(const EntityDescription& desc) {
    bool ret = Object::SetFromDesc(desc);
    if (ret) {
        for (const auto& field : desc.fields) {
            switch (field.type) {
                case EDF_FILE_PATH: {
                    std::string type_file;
                    field.ReadString(&type_file);
                    if (!as_ref.valid() || as_ref->path_ != obj_file) {
                        // as_ref = AmbientSounds::Instance()->ReturnRef(type_file);
                        as_ref = Engine::Instance()->GetAssetManager()->LoadSync<AmbientSound>(type_file);
                        if (as_ref->GetSoundType() == _occasional) {
                            delay = as_ref->GetDelayNoLower();
                        }
                    }
                    break;
                }
            }
        }
    }
    return ret;
}

void AmbientSoundObject::Moved(Object::MoveType type) {
    Object::Moved(type);
    if (type & kTranslate && scenegraph_) {
        Engine::Instance()->GetSound()->SetPosition(sound_handle, GetTranslation());
    }
}

bool AmbientSoundObject::InCameraRange() {
    const float _threshold_distance = 10.0f;
    const float _threshold_distance_squared = square(_threshold_distance);

    const float dist_sqrd =
        distance_squared(ActiveCameras::Get()->GetPos(), GetTranslation());
    return dist_sqrd < _threshold_distance_squared;
}

void AmbientSoundObject::Update(float timestep) {
    if (as_ref->GetSoundType() == _occasional && InCameraRange()) {
        delay -= timestep;
        if (delay <= 0.0f) {
            // SoundGroupRef sgr = SoundGroups::Instance()->ReturnRef(as_ref->GetPath());
            SoundGroupRef sgr = Engine::Instance()->GetAssetManager()->LoadSync<SoundGroup>(as_ref->GetPath());
            SoundGroupPlayInfo sgpi(*sgr, GetTranslation());
            unsigned long handle = Engine::Instance()->GetSound()->CreateHandle(__FUNCTION__);
            Engine::Instance()->GetSound()->PlayGroup(handle, sgpi);
            delay = as_ref->GetDelay();
        }
    }
}

void AmbientSoundObject::Draw() {
    /*ppolist::iterator iter = connections.begin();
    for(;iter != connections.end(); ++iter){
        DebugDraw::Instance()->AddLine(position,
                                       (*iter)->position,
                                       vec4(1.0f),
                                       _delete_on_draw);
    }*/
}

void AmbientSoundObject::Copied() {
    sound_handle = 0;
}

void AmbientSoundObject::GetDesc(EntityDescription& desc) const {
    Object::GetDesc(desc);
    desc.AddString(EDF_FILE_PATH, as_ref->path_);
}

const std::string& AmbientSoundObject::GetPath() {
    return as_ref->path_;
}
