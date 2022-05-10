//-----------------------------------------------------------------------------
//           Name: object_sanity_state.h
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

#include <Editors/entity_type.h>
#include <Internal/integer.h>

static const int kObjectSanity_PO_UnsetConnectID = (1UL << 0);

static const int kObjectSanity_G_NullChild = (1UL << 0);
static const int kObjectSanity_G_ChildHasSanityIssue = (1UL << 1);
static const int kObjectSanity_G_Empty = (1UL << 2);
static const int kObjectSanity_G_ChildHasExternalConnection = (1UL << 3);

class ObjectSanityState {
   public:
    EntityType type;

   private:
    int32_t id;
    uint32_t state_flags;

   public:
    ObjectSanityState();
    ObjectSanityState(EntityType type, int32_t id, uint32_t state_flags);
    bool Valid();
    bool Ok();

    int32_t GetID();
    uint32_t GetStateFlags();

    uint32_t GetErrorCount();
    void GetError(uint32_t errindex, char* outbuf, uint32_t size);
};
