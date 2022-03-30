//-----------------------------------------------------------------------------
//           Name: online_datastructures.h
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

#include <Math/vec3.h>
#include <Math/mat4.h>
#include <Math/quaternions.h>

#include <Scripting/scriptparams.h>
#include <Editors/entity_type.h>
#include <Objects/object.h>
#include <Network/net_framework.h>
#include <Utility/binn_util.h>

#include <vector>
#include <string>
#include <utility>
#include <array>

using std::array;

typedef uint32_t OnlineMessageID;
typedef uint16_t OnlineMessageType;
typedef uint8_t PeerID;
typedef uint8_t PlayerID;
typedef int32_t CommonObjectID;

class OnlineMessageRef {
    OnlineMessageID message_id;
public:
    OnlineMessageRef();
    OnlineMessageRef(const OnlineMessageID message_id);
    OnlineMessageRef(const OnlineMessageRef& rhs);
    ~OnlineMessageRef();
    OnlineMessageRef& operator=(const OnlineMessageRef& rhs);
    void* GetData() const;
    OnlineMessageID GetID() const;
    bool IsValid() const;
};

enum class OnlineFlags {
    INVALID,
    ALLOWSEDITOR
};

enum class MultiplayerMode : uint8_t {
    NoMultiplayer,
    Host,
    Client,
    CleanUp,
    AwaitingShutdown,
};

struct PlayerState {
    std::string playername = "UNKNOWN";
    ObjectID object_id = 1;
    uint16_t ping = 0;
    int32_t controller_id = -1; // Incoming inputs are copied into this controller.

    // Invalid on clients
    int32_t camera_id = -1; // Virtual camera id, used for character logic, some data may be syned to/from this camera from the client.
};

struct MorphTargetStateStorage {
    float disp_weight;
    float mod_weight;
    bool dirty;
    std::string name;
};

struct NetworkBone {
    float scale;
    vec3 translation0;
    vec3 translation1;
    quaternion rotation0;
    quaternion rotation1;
    float model_char_scale;

    binn* Serialize();
    void Deserialize(binn* l);
};

struct RiggedObjectFrame {
    float host_walltime;
    uint8_t bone_count;
    array<NetworkBone,64> bones;

    binn* Serialize();
    void Deserialize(binn* l);
};

struct MovementObjectFrame {
    float host_walltime;
    vec3 position;
    vec3 velocity;
    vec3 facing;
    RiggedObjectFrame rigged_object_frame;
};

struct ItemObjectFrame {
    float host_walltime;
    mat4 transform;
};

struct AngelScriptUpdate {
    uint32_t state;
    vector<char> data;
};

struct ChatMessage {
    float time;
    std::string message;
};
