//-----------------------------------------------------------------------------
//           Name: online.h
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

#include <Online/online_datastructures.h>
#include <Online/online_session.h>
#include <Online/online_peer.h>
#include <Online/online_file_transfer_handler.h>
#include <Online/online_message_handler.h>
#include <Online/state_machine.h>

#include <Online/Message/audio_play_sound_group_relative_gain_message.h>
#include <Online/Message/audio_play_sound_group_gain_message.h>
#include <Online/Message/audio_play_sound_loop_message.h>
#include <Online/Message/audio_play_sound_loop_at_location_message.h>
#include <Online/Message/whoosh_sound_message.h>
#include <Online/Message/audio_play_group_priority_message.h>
#include <Online/Message/audio_play_sound_message.h>
#include <Online/Message/audio_play_sound_location_message.h>
#include <Online/Message/audio_play_sound_group_relative_message.h>
#include <Online/Message/audio_play_sound_group_message.h>
#include <Online/Message/audio_play_sound_group_voice_message.h>
#include <Online/Message/audio_play_sound_impact_item_message.h>
#include <Online/Message/pcs_build_version_request_message.h>
#include <Online/Message/pcs_build_version_message.h>
#include <Online/Message/pcs_loading_completed_message.h>
#include <Online/Message/pcs_client_parameters_message.h>
#include <Online/Message/pcs_session_parameters_message.h>
#include <Online/Message/pcs_assign_player_id.h>
#include <Online/Message/pcs_file_transfer_metadata_message.h>
#include <Online/Message/set_player_state.h>
#include <Online/Message/remove_player_state.h>
#include <Online/Message/sp_string_message.h>
#include <Online/Message/sp_union_message.h>
#include <Online/Message/sp_remove_message.h>
#include <Online/Message/sp_rename_message.h>
#include <Online/Message/chat_entry_message.h>
#include <Online/Message/set_object_enabled_message.h>
#include <Online/Message/player_input_message.h>
#include <Online/Message/attach_to_message.h>
#include <Online/Message/camera_transform_message.h>
#include <Online/Message/test_message.h>
#include <Online/Message/movement_object_update.h>
#include <Online/Message/morph_target_update.h>
#include <Online/Message/item_update.h>
#include <Online/Message/env_object_update.h>
#include <Online/Message/cut_line.h>
#include <Online/Message/angelscript_data.h>
#include <Online/Message/angelscript_object_data.h>
#include <Online/Message/material_sound_event.h>
#include <Online/Message/host_session_flag.h>
#include <Online/Message/timed_slow_motion.h>
#include <Online/Message/create_entity.h>
#include <Online/Message/remove_object.h>
#include <Online/Message/set_avatar_palette.h>
#include <Online/Message/send_level_message.h>
#include <Online/Message/editor_transform_change.h>
#include <Online/Message/ping.h>
#include <Online/Message/pong.h>

#include <Math/vec3.h>
#include <Math/mat4.h>

#include <Game/connection_closed_reason.h>
#include <Game/levelinfo.h>

#include <UserInput/input.h>
#include <Math/quaternions.h>
#include <Utility/block_allocator.h>
#include <Internal/filesystem.h>
#include <Network/net_framework.h>

#include <cstdint>
#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>
#include <array>
#include <list>
#include <thread>
#include <mutex>

class MovementObject;
class SceneGraph;
class EnvObject;
class Object;
class ItemObject;
class Engine;
struct PlayerInput;

struct ZSTD_CCtx_s;
typedef struct ZSTD_CCtx_s ZSTD_CCtx;
struct ZSTD_DCtx_s;
typedef struct ZSTD_DCtx_s ZSTD_DCtx;

using std::array;
using std::endl;
using std::list;
using std::lock_guard;
using std::map;
using std::mutex;
using std::numeric_limits;
using std::pair;
using std::runtime_error;
using std::string;
using std::stringstream;
using std::thread;
using std::to_string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

using namespace OnlineMessages;

typedef OnlineMessageHandlerTemplate<
    TestMessage,
    AudioPlayGroupPriorityMessage,
    AudioPlaySoundLoopMessage,
    AudioPlaySoundGroupGainMessage,
    AudioPlaySoundLocationMessage,
    AudioPlaySoundMessage,
    AudioPlaySoundLoopAtLocationMessage,
    AudioPlaySoundGroupRelativeMessage,
    AudioPlaySoundGroupMessage,
    AudioPlaySoundGroupVoiceMessage,
    AudioPlaySoundImpactItemMessage,
    AudioPlaySoundGroupRelativeGainMessage,
    WhooshSoundMessage,
    ChatEntryMessage,
    SetObjectEnabledMessage,
    PlayerInputMessage,
    AttachToMessage,
    CameraTransformMessage,
    MovementObjectUpdate,
    PCSBuildVersionRequestMessage,
    PCSBuildVersionMessage,
    PCSLoadingCompletedMessage,
    PCSSessionParametersMessage,
    PCSClientParametersMessage,
    PCSAssignPlayerID,
    PCSFileTransferMetadataMessage,
    SetPlayerState,
    RemovePlayerState,
    SPStringMessage,
    SPUnionMessage,
    SPRemoveMessage,
    SPRenameMessage,
    MorphTargetUpdate,
    ItemUpdate,
    EnvObjectUpdate,
    CutLine,
    AngelscriptData,
    AngelscriptObjectData,
    MaterialSoundEvent,
    HostSessionFlag,
    TimedSlowMotion,
    CreateEntity,
    RemoveObject,
    SetAvatarPalette,
    SendLevelMessage,
    EditorTransformChange,
    Ping,
    Pong>
    OnlineMessageHandler;

extern OnlineMessageHandler message_handler;

class Online {
   public:
    int compression_level;

    string level_name;
    string campaign_id;
    bool host_started_level;

    OnlineSession* online_session;

   private:
    NetFramework net;

    OnlineFileTransferHandler online_file_transfer_handler;

    vector<char> compressed_serialization_buffer;
    thread socket_thread;
    unordered_map<string, uint32_t> states;  // This is angelscript states, these need to exist independent of Multiplayer to allow for states to be set while not hosting to transfer when starting to host
    unordered_map<CommonObjectID, ObjectID> to_object_id;
    unordered_map<ObjectID, CommonObjectID> to_common_id;

    bool attach_avatar_camera;
    MultiplayerMode mode;
    uint64_t initial_ts;
    uint32_t next_available_state_id;
    bool loading;

    PeerID next_free_peer_id;
    uint32_t tick_period;
    bool p2p_sockets_active = false;
    unordered_set<NetListenSocketID> listen_sockets;

    float no_data_interpstep_override = 0.5f;
    uint32_t last_update_wall_ticks = 0;
    uint32_t last_quarter_update_wall_ticks = 0;
    string default_hot_join_characte_path;

    ZSTD_CCtx* zstdCContext;
    ZSTD_DCtx* zstdDContext;

    map<string, KeyState> client_key_down_map_state;

   public:
    static Online* Instance() {
        static Online online;
        return &online;
    }

    Online();
    void Dispose();

    bool IsClient() const;
    bool IsActive() const;
    bool IsHosting() const;
    bool IsAwaitingShutdown() const;
    uint32_t GetPlayerCount();

    void StartMultiplayer(MultiplayerMode multiplayerMode);
    void StartHostingMultiplayer();

    void QueueStopMultiplayer();
    void StopMultiplayer();
    void StopListening();

    vector<PlayerState> GetPlayerStates();

    void CloseConnection(NetConnectionID conn_id, ConnectionClosedReason reason);

    bool TryGetPlayerState(PlayerState& player_state, ObjectID object_id) const;

    void AssignNewControllerForPlayer(PlayerID player_id);
    void ClearOnlineSession();

    template <typename Message, typename... Args>
    OnlineMessageRef& CreateMessage(Args... args);
    void Send(const OnlineMessageRef& message);
    void SendTo(NetConnectionID target, const OnlineMessageRef& message);
    template <typename Message, typename... Args>
    void Send(Args... args);
    template <typename Message, typename... Args>
    void SendTo(NetConnectionID target, Args... args);

    void SendIntFloatScriptParam(uint32_t i, const string& key, const ScriptParam& param);
    void SendStringScriptParam(uint32_t i, const string& key, const ScriptParam& param);
    void SendScriptParam(uint32_t id, const string& key, const ScriptParam& param);
    void SendScriptParamMap(uint32_t id, const ScriptParamMap& param);
    bool GetHostAllowsClientEditor() const;
    void SetIfHostAllowsClientEditor(bool mode);
    void SetDefaultHotJoinCharacter(const string& path);
    string GetDefaultHotJoinCharacter() const;
    void SessionStarted(bool host_started_the_level);
    bool ForceMapStartOnLoad() const;
    bool AllClientsReady();
    ScriptParams* GetScriptParamsFromID(ObjectID id);
    void UpdateMovementObjectFromID(uint32_t id);
    void SocketMessageApplyLevelChange();
    bool SetAvatarCameraAttachedMode(bool mode);
    bool IsAvatarCameraAttached() const;
    void SendLevelMessage(const string& msg);
    void SendAvatarPaletteChange(const OGPalette& palette, ObjectID object_id);
    bool IsAvatarPossessed(ObjectID avatar_id);
    void PossessAvatar(PlayerID player_id, ObjectID object_id);
    ObjectID CreateCharacter();
    void SetNoDataInterpStepOverRide(float override);
    float GetNoDataInterpStepOverRide() const;
    void LateUpdate(SceneGraph* scenegraph);

    void SyncHostSessionFlags();
    void GenerateEnvObjectSyncPackages(NetConnectionID conn, SceneGraph* graph);
    void GenerateMovementSyncPackages(NetConnectionID conn, SceneGraph* graph);
    void GenerateStateForNewConnection(NetConnectionID conn);

    bool CheckLoadedMapAndCampaignState(const string& campagin, const string& level_name);
    void SetLevelLoaded();
    uint32_t IncomingMessageCount() const;
    uint32_t OutgoingMessageCount() const;
    bool GetIfPendingAngelScriptUpdates();
    bool GetIfPendingAngelScriptStates();
    AngelScriptUpdate GetAngelScriptUpdate();
    void MoveAngelScriptQueueForward();
    void MoveAngelStateQueueForward();
    AngelScriptUpdate GetAngelScriptStates();
    uint32_t AddSyncState(uint32_t state, const vector<char>& data);
    uint32_t RegisterMPState(const string& state);
    void SetTickperiod(const uint32_t& tickperiod);
    void GetConnectionStatus(const Peer& peer, ConnectionStatus* status) const;

    void RegisterHostObjectID(CommonObjectID hostid, ObjectID clientid);
    void DeRegisterHostClientIDTranslation(ObjectID clientid);
    ObjectID GetObjectID(CommonObjectID hostid) const;
    CommonObjectID GetOriginalID(ObjectID clientid) const;
    void ClearIDTranslations();

    bool SendingRemovePackages() const;
    void RemoveObject(Object* o, ObjectID my_id);
    bool NetworkRemoveableType(Object* o) const;
    void ChangeLevel(const string& id);
    void PerformLevelChangeCleanUp();

    const vector<Peer> GetPeers();
    vector<Peer>::iterator GetPeerIt(NetConnectionID conn_id);
    bool IsLocalAvatar(const ObjectID avatar) const;
    vector<ObjectID> GetLocalAvatarIDs() const;
    PlayerState GenerateHostPlayerState() const;

    void OnConnectionChange(NetFrameworkConnectionStatusChanged* data);

    void BroadcastChatMessage(const string& chat_message);
    void SendRawChatMessage(const string& raw_chat_entry);
    void AddLocalChatMessage(string chat_message);
    void RemoveOldChatMessages(float threshold);
    const vector<ChatMessage>& GetChatMessages() const;

    void AddPeer(NetFrameworkConnectionStatusChanged* data);
    Peer* GetPeerFromConnection(NetConnectionID conn_id);
    Peer* GetPeerFromID(PeerID peer_id);

    bool HasFreeAvatars() const;
    void UpdateObjects(SceneGraph* scenegraph_);
    uint32_t GetNumberOfFreeAvatars() const;
    uint8_t GetPlayerLimit();
    bool IsLobbyFull();
    void AddFreeAvatarsIds();
    void RemoveAllAvatarIds();
    void RemoveFreeAvatarId(uint32_t id);
    ObjectID GetFreeAvatarID();
    void AddFreeAvatarId(uint32_t id);
    void CreateListenSocketIP(const string& localAddress);
    void ConnectByIPAddress(const string& address);

    bool NetFrameworkHasFriendInviteOverlay() const;
    void ActivateGameOverlayInviteDialog();

    void CheckPendingMessages();
    void ApplyPlayerInput();

    PlayerState GetOwnPlayerState();

    // Host and Client use, used to convert a PlayerInput binding name to a shared integer representation.
    uint8_t GetBindID(string bind_name);

   private:
    // Explicitally disable copy constructors and assignment to prevent misuse.
    Online(const Online&) = delete;
    Online& operator=(const Online&) = delete;
    Online(Online&&) = delete;
    Online& operator=(Online&&) = delete;

    // Host only function, clients get their bindings from the host.
    void AssignBindID(string bind_name);

    // Client side cache, not used on the server.
    void SendPlayerInputState();
    void SendPlayerCameraState();

    void CloseConnectionImmediate(NetConnectionID conn, ConnectionClosedReason reason);

    // Functions run inside a separate socket communication thread.
    void NetworkDataThread();
    void SendMessages();
    void SendMessageObjects();
    void HandleConnectionsToBeClosed();

    void SendMessageToConnection(NetConnectionID conn_id, char* buffer, uint32_t bytes, bool reliable, bool flush);
    void SendStateMessages();

    void CheckNewMessages();

    size_t CompressData(vector<char>& target_buffer, const void* source_buffer, uint32_t source_size);
    size_t DecompressData(vector<char>& target_buffer, const void* source_buffer, size_t source_size);
};

template <typename Message, typename... Args>
void Online::Send(Args... args) {
    Send(message_handler.Create<Message>(args...));
}

template <typename Message, typename... Args>
void Online::SendTo(NetConnectionID target, Args... args) {
    SendTo(target, message_handler.Create<Message>(args...));
}

template <typename Message, typename... Args>
OnlineMessageRef& Online::CreateMessage(Args... args) {
    return message_handler.Create<Message>(args...);
}
