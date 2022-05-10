//-----------------------------------------------------------------------------
//           Name: object_sanity_state.cpp
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
#include "object_sanity_state.h"

#include <Internal/common.h>

ObjectSanityState::ObjectSanityState() : id(-1), type(_no_type), state_flags(~0UL) {
}

ObjectSanityState::ObjectSanityState(EntityType type, int32_t id, uint32_t state_flags) : type(type), id(id), state_flags(state_flags) {
}

bool ObjectSanityState::Valid() {
    return id >= 0;
}

bool ObjectSanityState::Ok() {
    return state_flags == 0;
}

int32_t ObjectSanityState::GetID() {
    return id;
}

uint32_t ObjectSanityState::GetStateFlags() {
    return state_flags;
}

uint32_t ObjectSanityState::GetErrorCount() {
    int count = 0;
    for (int i = 0; i < 32; i++) {
        if (state_flags & (1UL << i)) {
            count++;
        }
    }
    return count;
}

void ObjectSanityState::GetError(uint32_t errindex, char* outbuf, uint32_t size) {
    int index_count = -1;
    uint32_t curr_flag = 0;
    for (int i = 0; i < 32; i++) {
        if (state_flags & (1UL << i)) {
            index_count++;
            if (index_count == errindex) {
                curr_flag = (1UL << i);
            }
        }
    }

    if (curr_flag == 0) {
        FormatString(outbuf, size, "Out of bounds error index %d\n", errindex);
    } else if (type == _placeholder_object) {
        if (curr_flag == kObjectSanity_PO_UnsetConnectID) {
            FormatString(outbuf, size, "Connect ID is unset (no character tied to dialogue?)");
        } else {
            FormatString(outbuf, size, "Unknown error flag for _placeholder_object %x\n", curr_flag);
        }
    } else if (type == _group) {
        switch (curr_flag) {
            case kObjectSanity_G_NullChild:
                FormatString(outbuf, size, "There is a child object which is a NULL pointer");
                break;
            case kObjectSanity_G_ChildHasSanityIssue:
                FormatString(outbuf, size, "Some object in the group fails a sanity check");
                break;
            case kObjectSanity_G_Empty:
                FormatString(outbuf, size, "Group is empty");
                break;
            case kObjectSanity_G_ChildHasExternalConnection:
                FormatString(outbuf, size, "There is a connection from inside the group to the outside world");
                break;
            default:
                FormatString(outbuf, size, "Unknown error flag for _group %x\n", curr_flag);
                break;
        }
    } else if (type == _prefab) {
        switch (curr_flag) {
            case kObjectSanity_G_NullChild:
                FormatString(outbuf, size, "There is a child object which is a NULL pointer");
                break;
            case kObjectSanity_G_ChildHasSanityIssue:
                FormatString(outbuf, size, "Some object in the prefab fails a sanity check");
                break;
            case kObjectSanity_G_Empty:
                FormatString(outbuf, size, "Prefab is empty");
                break;
            case kObjectSanity_G_ChildHasExternalConnection:
                FormatString(outbuf, size, "There is a connection from inside the prefab to the outside world");
                break;
            default:
                FormatString(outbuf, size, "Unknown error flag for _prefab %x\n", curr_flag);
                break;
        }
    } else if (curr_flag) {
        FormatString(outbuf, size, "Unknown error flag %x\n", curr_flag);
    }
}
