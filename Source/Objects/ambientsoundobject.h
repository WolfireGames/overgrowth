//-----------------------------------------------------------------------------
//           Name: ambientsoundobject.h
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

#include <Objects/object.h>
#include <Asset/Asset/ambientsounds.h>

#include <vector>
#include <string>
#include <list>

//-----------------------------------------------------------------------------
// Class Definition
//-----------------------------------------------------------------------------

class AmbientSoundObject: public Object
{
public:
    virtual EntityType GetType() const { return _ambient_sound_object; }
    AmbientSoundObject();
    virtual ~AmbientSoundObject();

    bool Initialize();

    void Update(float timestep);
    void Draw();
    void Copied();

    virtual void Moved(Object::MoveType type);
    const std::string & GetPath();
    bool InCameraRange();
    virtual void GetDesc(EntityDescription &desc) const;
    virtual bool SetFromDesc( const EntityDescription& desc );

private:
    float delay;
    AmbientSoundRef as_ref;
    int sound_handle;
};
