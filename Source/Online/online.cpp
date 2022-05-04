//-----------------------------------------------------------------------------
//           Name: online.cpp
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
#include "online.h"

#include <Game/level.h>
#include <Game/connection_closed_reason.h>

#include <Math/vec3math.h>
#include <Math/vec4math.h>

#include <Steam/steamworks.h>
#include <Steam/steamworks_util.h>

#include <Objects/object.h>
#include <Objects/envobject.h>
#include <Objects/itemobject.h>
#include <Objects/movementobject.h>
#include <Objects/envobjectattach.h>
#include <Objects/riggedobject.h>
#include <Objects/cameraobject.h>

#include <Editors/map_editor.h>
#include <Editors/actors_editor.h>

#include <Online/online_file_transfer_handler.h>
#include <Online/online_utility.h>
#include <Online/package_header.h>

#include <Compat/compat.h>
#include <Compat/time.h>

#include <Graphics/pxdebugdraw.h>
#include <Main/scenegraph.h>
#include <Logging/logdata.h>

#if ENABLE_STEAMWORKS
#include <isteamnetworkingutils.h>
#include <isteamfriends.h>
#endif

#include <zstd.h>
#include <zstd-1.5.0/lib/common/zstd_common.c>

#include <limits>

extern Timer game_timer;

OnlineMessageHandler message_handler;

OnlineMessageRef::OnlineMessageRef() : message_id(std::numeric_limits<OnlineMessageID>::max()) {
}

OnlineMessageRef::OnlineMessageRef(const OnlineMessageID message_id) : message_id(message_id) {
    message_handler.Acquire(message_id);
}

OnlineMessageRef::OnlineMessageRef(const OnlineMessageRef& rhs) {
    //Debug the fact that this RHS is corrupt sometimes when loading in debug.
    message_id = rhs.message_id;
    message_handler.Acquire(message_id);
}

OnlineMessageRef::~OnlineMessageRef() {
    if(IsValid()) {
        message_handler.Release(message_id);
    }
}

OnlineMessageRef& OnlineMessageRef::operator=(const OnlineMessageRef& rhs){
    if(IsValid()) {
        message_handler.Release(message_id);
    }

    message_id = rhs.message_id;

    message_handler.Acquire(message_id);
    return *this;
}

OnlineMessageID OnlineMessageRef::GetID() const {
    return this->message_id;
}

void* OnlineMessageRef::GetData() const {
    return message_handler.GetMessageData(*this);
}

bool OnlineMessageRef::IsValid() const {
    return message_id != std::numeric_limits<OnlineMessageID>::max();
}

void Online::GetConnectionStatus(const Peer& peer, ConnectionStatus* status) const {
    return net.GetConnectionStatus(peer.conn_id, status);
}

void Online::RegisterHostObjectID(CommonObjectID hostid, ObjectID clientid) {
    to_object_id[hostid] = clientid;
    to_common_id[clientid] = hostid;
}

void Online::DeRegisterHostClientIDTranslation(ObjectID clientid) {
    // check if we need to remove a translation
    auto translation = GetOriginalID(clientid);

    if (translation) {
        to_object_id.erase(translation);
        to_common_id.erase(clientid);
    }
}

ObjectID Online::GetObjectID(CommonObjectID hostid) const {
    auto it = to_object_id.find(hostid);
    if (it != to_object_id.end()) {
        return it->second;
    } else {
        return hostid;
    }
}

CommonObjectID Online::GetOriginalID(ObjectID clientid) const {
    auto it = to_common_id.find(clientid);
    if (it != to_common_id.end()) {
        return it->second;
    } else {
        return clientid;
    }
}

void Online::ClearIDTranslations() {
    to_object_id.clear();
    to_common_id.clear();
}

bool Online::SendingRemovePackages() const {
    return !loading;
}

void Online::RemoveObject(Object * o, ObjectID my_id) {
    if (NetworkRemoveableType(o) && SendingRemovePackages()) {
        Send<OnlineMessages::RemoveObject>(my_id, o->GetType());
    }
}

bool Online::NetworkRemoveableType(Object * o) const {
    EntityType type = o->GetType();
    return type == _env_object || type == _movement_object || type == _item_object;
}

void Online::ChangeLevel(const string & id) {
    campaign_id = ModLoading::Instance().WhichCampaignLevelBelongsTo(id);
    host_started_level = false;
    level_name = id;

    RemoveAllAvatarIds();
    loading = true;

    online_session->outgoing_messages_lock.lock();
    // Clear out Level persistent messages
    for (int i = online_session->persistent_outgoing_messages.size() - 1; i >= 0; i--) {
        OnlineMessageBase* message_base = (OnlineMessageBase*) online_session->persistent_outgoing_messages[i].GetData();
        if (message_base->cat == OnlineMessageCategory::LEVEL_PERSISTENT) {
            online_session->persistent_outgoing_messages.erase(online_session->persistent_outgoing_messages.begin() + i);
        }
    }
    online_session->outgoing_messages_lock.unlock();

    // Reset every peer to a pending connection state.
    online_session->client_connection_manager.ResetConnections();

    // Reset every player's possession
    online_session->connected_peers_lock.lock();
    for (auto& it : online_session->player_states) {
        it.second.object_id = -1;
    }
    online_session->connected_peers_lock.unlock();
}

Online::Online() : mode(MultiplayerMode::NoMultiplayer),
                                initial_ts(0), next_available_state_id(1), next_free_peer_id(1),
                                attach_avatar_camera(true), loading(false),
                                default_hot_join_characte_path("Data/Objects/IGF_Characters/IGF_TurnerActor.xml"),
                                compression_level(10), net(this), host_started_level(false)
{

    zstdCContext = ZSTD_createCCtx();
    zstdDContext = ZSTD_createDCtx();
}

void Online::Dispose() {
    StopMultiplayer();
}

bool Online::IsClient() const {
    return mode == MultiplayerMode::Client;
}

void Online::SendAvatarPaletteChange(const OGPalette & palette, ObjectID object_id) {
    for (const auto& it : palette) {
        Send<OnlineMessages::SetAvatarPalette>(object_id, it.color, it.channel);
    }
}

bool Online::IsAvatarPossessed(ObjectID object_id) {
    Object* object = Engine::Instance()->GetSceneGraph()->GetObjectFromID(object_id);
    if (object != nullptr && object->GetType() == EntityType::_movement_object) {
        MovementObject* mov = (MovementObject*) object;
        return mov->controlled;
    }

    LOGW << "IsAvatarPossessed was called with invalid id: " << object_id << endl;
    return false;
}

void Online::PossessAvatar(PlayerID player_id, ObjectID object_id) {
    // LOGI << std::to_string(player_id) << " possessed " << std::to_string(object_id) << std::endl;
    online_session->player_states[player_id].object_id = object_id;
    Send<OnlineMessages::SetPlayerState>(player_id, &online_session->player_states[player_id]);
}

ObjectID Online::CreateCharacter() {
    SceneGraph * scenegraph = Engine::Instance()->GetSceneGraph();

    ObjectID id = scenegraph->map_editor->CreateObject(default_hot_join_characte_path);
    std::string team = "";
    vec3 position = vec3(0.0f);

    // Attempt to move character close to an existing one.
    vector<ObjectID> local_avatar_ids = GetLocalAvatarIDs();
    if(local_avatar_ids.size() > 0) {
        MovementObject* mymo = static_cast<MovementObject*>(scenegraph->GetObjectFromID(local_avatar_ids[0]));
        team = mymo->GetScriptParams()->GetStringVal("Teams");
        position = mymo->GetTranslation();
    } else {
        LOGW << "Unable to find locally controlled character while creating character for new connection, using fallback parameters" << std::endl;
    }

    MovementObject* mo = static_cast<MovementObject*>(scenegraph->GetObjectFromID(id));
    mo->SetTranslation(position);
    mo->is_player = true;
    mo->created_on_the_fly = true;

    ScriptParams * sp = mo->GetScriptParams();
    sp->ASSetString("Teams", team);

    Send<OnlineMessages::CreateEntity>(default_hot_join_characte_path, position, id);

    // Set player color for new character
    OGPalette* palettes = mo->GetPalette();
    for (auto& palette : *palettes) {
        if (palette.label == "Cloth") {
            palette.color = vec3(1.0f, 0.0f, 0.0f);
        }
    }
    mo->ApplyPalette(*palettes);
    return id;
}

void Online::SetNoDataInterpStepOverRide(float override) {
    no_data_interpstep_override = override;
}

float Online::GetNoDataInterpStepOverRide() const {
    return no_data_interpstep_override;
}

//Client side sending camera info to host
void Online::SendPlayerCameraState() {
    Camera* camera = ActiveCameras::Instance()->GetCamera(0);
    Send<OnlineMessages::CameraTransformMessage>(camera->GetPos(), camera->GetFlatFacing(), camera->GetFacing(), camera->GetUpVector(), camera->GetXRotation(), camera->GetYRotation());
}

void Online::SendPlayerInputState() {
    //Assume player one control. For split screen multiplayer we have to start dealing with multiplayer controllers here.
    PlayerInput* player_input = Input::Instance()->GetController(0);

    for(const string& binding : Input::Instance()->GetAllAvailableBindings()) {

        // TODO Can we make it so these bindings don't even show up in available bindings?
        if(binding == "look_left" || binding == "look_right" || binding == "look_up" || binding == "look_down") {
            continue;
        }

        KeyState local_keystate = player_input->key_down[binding];
        KeyState remote_keystate = client_key_down_map_state[binding];

        // if we have something that the remote does not have, update
        if(local_keystate != remote_keystate) {
            Send<OnlineMessages::PlayerInputMessage>(local_keystate.depth, local_keystate.depth_count, local_keystate.count, online_session->binding_id_map_[binding]);
            client_key_down_map_state[binding] = local_keystate;
        }
    }
}

void Online::LateUpdate(SceneGraph * scenegraph) {
    if (IsActive()) {
        //Send a ping every second.
        if(online_session != nullptr && game_timer.wall_time > online_session->last_ping_time + 1.0f) {
            online_session->last_ping_id = online_session->ping_counter++;
            online_session->last_ping_time = game_timer.wall_time;
            Send<OnlineMessages::Ping>(online_session->last_ping_id);
        }

        online_session->client_connection_manager.Update();

        if (IsHosting()) {
            SyncHostSessionFlags();
        }
    }
}

void Online::SyncHostSessionFlags() {
    lock_guard<mutex> guard(online_session->connected_peers_lock);
    for(auto& conn : online_session->connected_peers) {
        for(auto& flag : online_session->host_session_flags) {
            if(conn.host_session_flags.find(flag.first) == conn.host_session_flags.end() || conn.host_session_flags.find(flag.first)->second != flag.second) {
                SendTo<OnlineMessages::HostSessionFlag>(conn.conn_id, flag.first, flag.second);
                conn.host_session_flags[flag.first] = flag.second;
            }
        }
    }
}

void Online::GenerateStateForNewConnection(NetConnectionID conn) {
    SceneGraph* graph = Engine::Instance()->GetSceneGraph();
    if (graph != nullptr) {
        GenerateMovementSyncPackages(conn, graph);
        GenerateEnvObjectSyncPackages(conn, graph);
    }
}

void Online::GenerateEnvObjectSyncPackages(NetConnectionID conn, SceneGraph* graph) {
    std::vector<Object*> envobjects = graph->GetObjectsOfType(EntityType::_env_object);

    for (const auto& it : envobjects) {
        EnvObject* eo = static_cast<EnvObject*>(it);
        SendTo<OnlineMessages::EnvObjectUpdate>(conn, eo);
    }
}

void Online::GenerateMovementSyncPackages(NetConnectionID conn, SceneGraph * graph) {
    std::vector<Object*> movobjects = graph->GetObjectsOfType(EntityType::_movement_object);

    for (const auto& it : movobjects) {
        MovementObject * mov = static_cast<MovementObject*>(it);
        ObjectID avatarid = mov->GetID();

        // riggedbody blood?
        std::vector<MorphTargetStateStorage> network_morphs = mov->rigged_object()->network_morphs;

        for (const auto& it : network_morphs) {
            SendTo<OnlineMessages::MorphTargetUpdate>(conn, avatarid, it);
        }
    }
}

bool Online::CheckLoadedMapAndCampaignState(const string & campagin, const string & level_name) {
    Engine * engine = Engine::Instance();
    SceneGraph * graph = engine->GetSceneGraph();

    ScriptableCampaign * loaded_campagin = engine->GetCurrentCampaign();
    if (loaded_campagin == nullptr || graph == nullptr) {
        return false;
    }

    string current_campaign = loaded_campagin->GetCampaignID();
    string campaignId = engine->GetCurrentCampaign()->GetCampaignID();
    string current_level = graph->level_path_.GetOriginalPathStr();
    return (current_campaign == campagin) && (level_name == current_level);
}

void Online::SetLevelLoaded() {
    if (IsClient()) {
        // We notify the host about having loaded, the host can only finish loading if we have.
        // From the client's perspective! The host might already be ingame, client doesn't know.
        Send<OnlineMessages::PCSLoadingCompletedMessage>();
    }

    // Host finished loading
    if (IsHosting()) {
        loading = false;
        RemoveAllAvatarIds();
        AddFreeAvatarsIds();
        ObjectID avatar = GetFreeAvatarID();

        campaign_id = ModLoading::Instance().WhichCampaignLevelBelongsTo(level_name);
        LOG_ASSERT(level_name == Engine::Instance()->GetSceneGraph()->level_path_.GetOriginalPathStr());
    }
}

uint32_t Online::IncomingMessageCount() const {
    if(IsActive()) {
        return online_session->incoming_messages.size();
    }
    return 0;
}

uint32_t Online::OutgoingMessageCount() const {
    if(IsActive()) {
        return online_session->outgoing_messages.size();
    }
    return 0;
}

bool Online::GetIfPendingAngelScriptUpdates() {
    return online_session->peer_queued_level_updates.empty() == false;
}

bool Online::GetIfPendingAngelScriptStates() {
    return online_session->peer_queued_sync_updates.empty() == false;
}

AngelScriptUpdate Online::GetAngelScriptUpdate() {
    if (online_session->peer_queued_level_updates.empty() == false) {
        return online_session->peer_queued_level_updates.front();
    }
    return AngelScriptUpdate();
}

void Online::MoveAngelScriptQueueForward() {
    online_session->peer_queued_level_updates.pop();
}

void Online::MoveAngelStateQueueForward() {
    online_session->peer_queued_sync_updates.pop();
}

AngelScriptUpdate Online::GetAngelScriptStates() {
    if (online_session->peer_queued_sync_updates.empty() == false) {
        return online_session->peer_queued_sync_updates.front();
    }
    return AngelScriptUpdate();
}

uint32_t Online::AddSyncState(uint32_t state, const vector<char>& data) {
    AngelScriptUpdate temp;

    temp.state = state;
    temp.data = data;

    lock_guard<mutex> guard(online_session->state_packages_lock);
    online_session->state_packages[state] = temp;

    return state;
}

uint32_t Online::RegisterMPState(const string& state) {
    if (states.find(state) != states.end()) {
        return states[state];
    }

    states[state] = next_available_state_id++;
    return states[state];
}

void Online::SetTickperiod(const uint32_t & tickperiod) {
    tick_period = tickperiod;
}

bool Online::IsActive() const {
    return (IsHosting() || IsClient()) && online_session != nullptr;
}

bool Online::IsHosting() const {
    return mode == MultiplayerMode::Host;
}

bool Online::IsAwaitingShutdown() const {
    return mode == MultiplayerMode::AwaitingShutdown;
}

void Online::StartMultiplayer(MultiplayerMode multiplayerMode) {
    //for(string binding : Input::Instance()->GetAllAvailableBindings()) {
    //    LOGI << binding << " " << (int)Input::Instance()->GetBindID(binding) << endl;
    //}

    //for(string binding_cat : Input::Instance()->GetAvailableBindingCategories()) {
    //    LOGI << binding_cat << endl;
    //}

    if(online_session != nullptr) {
        LOGE << "Attempting to start multiplayer while the previous session is still valid!" << endl;
        // TODO can we close the application here with a big error? This should be a fatal issue!
    }

    if(OnlineUtility::HasActiveIncompatibleMods()) {
        LOGE << "Online multiplayer was enabled despite using unsupported mods, this might cause issues!" << std::endl;
        LOGE << "Incompatible mods: " << OnlineUtility::GetActiveIncompatibleModsString() << std::endl;
    }

    online_session = new OnlineSession();

    host_started_level = false;
    online_session->host_session_flags[OnlineFlags::ALLOWSEDITOR] = false;

    mode = multiplayerMode;

    Engine * engine = Engine::Instance();
    SceneGraph * graph = engine->GetSceneGraph();

    string mpcachedir = string(write_path) + string("multiplayercache/");

    CreateParentDirs(mpcachedir);

    AddPath(mpcachedir.c_str(), PathFlags::kDataPaths);

    if (mode == MultiplayerMode::Host) {
        // assert(scenegraph);
        // TODO this might never be true due to how we handle starting multiplayer now
        if (engine->GetSceneGraph() != nullptr) {
            RemoveAllAvatarIds();
            AddFreeAvatarsIds();
            host_started_level = true;

            // index 0 is probably not correct
            ScriptableCampaign* sc = engine->GetCurrentCampaign();
            if (sc != nullptr) {
                campaign_id = sc->GetCampaignID();
            } else {
                campaign_id = -1;
            }

            // you can check with scenegraph
            level_name = graph->level_path_.GetOriginalPathStr();
        }

        //Assign mappings for input bindings.
        for(const string& binding : Input::Instance()->GetAllAvailableBindings()) {
            AssignBindID(binding);
        }

        online_session->local_player_id = -1;
        online_session->player_states[online_session->local_player_id] = GenerateHostPlayerState();
    }

    net.Initialize();

    engine->menu_paused = false;
    engine->CommitPause();

    tick_period = 30;
    socket_thread = (thread(&Online::NetworkDataThread, this));
}

void Online::StartHostingMultiplayer() {
    if(!IsActive()) {
        StartMultiplayer(MultiplayerMode::Host);
        CreateListenSocketIP("");
    }
}

void Online::PerformLevelChangeCleanUp() {
    lock_guard<mutex> guard(online_session->state_packages_lock);
    online_session->state_packages.clear();
}

const vector<Peer> Online::GetPeers()  {
    lock_guard<mutex> guard(online_session->connected_peers_lock);
    return online_session->connected_peers;
}

Peer* Online::GetPeerFromConnection(NetConnectionID conn_id) {
    for(auto & connected_peer : online_session->connected_peers) {
        if(connected_peer.conn_id == conn_id) {
            return &connected_peer;
        }
    }
    return nullptr;
}

Peer* Online::GetPeerFromID(PeerID peer_id) {
    for(auto & connected_peer : online_session->connected_peers) {
        if(connected_peer.peer_id == peer_id) {
            return &connected_peer;
        }
    }
    return nullptr;
}

vector<Peer>::iterator Online::GetPeerIt(NetConnectionID conn_id) {
    for(int i = 0; i < online_session->connected_peers.size(); i++) {
        if(online_session->connected_peers[i].conn_id == conn_id) {
            return online_session->connected_peers.begin() + i;
        }
    }
    return online_session->connected_peers.end();
}

void Online::QueueStopMultiplayer() {
    if(IsActive()) {
        mode = MultiplayerMode::AwaitingShutdown;
    }
}

void Online::StopMultiplayer() {
    if(IsActive() || mode == MultiplayerMode::AwaitingShutdown) {
        mode = MultiplayerMode::CleanUp;

        { // Immediately disconnect all clients
            lock_guard<mutex> guard(online_session->connected_peers_lock);
            for (auto& it : online_session->connected_peers) {
                // We kill the connection on the socket directly, we no longer want to keep track of any connection changes
                // We can't call CloseConnectionImmediate here, as it modifies the array we are iterating over and uses the same lock
                net.CloseConnection(it.conn_id, ConnectionClosedReason::HOST_STOPPED_HOSTING);
            }
            online_session->connected_peers.clear(); // We shouldn't have to clear this, since we plan on getting rid of online_session shortyly
            StopListening();
        }

        // Clear up on the fly
        SceneGraph* graph = Engine::Instance()->GetSceneGraph();
        if (graph != nullptr) {
            vector<Object *> objects = graph->GetObjectsOfType(EntityType::_movement_object);

            for (auto it : objects) {
                if (it->created_on_the_fly) {
                    graph->map_editor->RemoveObject(it, graph, true);
                    DeRegisterHostClientIDTranslation(it->GetID());
                }
            }
        }

        next_available_state_id = 1;
        next_free_peer_id = 1;

        host_started_level = false;
        loading = false;
        p2p_sockets_active = false;

        if (socket_thread.joinable()) {
            socket_thread.join();
        }

        // Make sure we get booted into the main menu, if we aren't there already.
        if(Engine::Instance()->current_engine_state_.type != EngineStateType::kEngineScriptableUIState) {
            Engine::Instance()->ScriptableUICallback("back_to_main_menu");
        }

        mode = MultiplayerMode::NoMultiplayer;

        delete online_session;
        online_session = nullptr;
    }
}

bool Online::IsLocalAvatar(const ObjectID avatar_id) const {
    if (online_session == nullptr) {
        // Everything is local when playing offline
        return true;
    }

    // Is this avatar controlled by a playerstate? If yes, is it us?
    for (const auto& it : online_session->player_states) {
        if (it.second.object_id == avatar_id) {
            return it.first == online_session->local_player_id;
        }
    }

    // As a host, everything not convered in a playerstate is local
    // As a client, everything not controlled by us playerstate is remote
    return IsHosting();
}

vector<ObjectID> Online::GetLocalAvatarIDs() const {
    vector<ObjectID> local_avatar_ids;
    SceneGraph* sg = Engine::Instance()->GetSceneGraph();
    if(sg != nullptr) {
        for (const auto& it : sg->GetControllableMovementObjects()) {
            if (IsLocalAvatar(it->GetID())) {
                local_avatar_ids.push_back(it->GetID());
            }
        }
    }
    return local_avatar_ids;
}

PlayerState Online::GenerateHostPlayerState() const {
    if(!IsHosting()) {
        LOGW << "Called GenerateHostPlayerState(), but wasn't hosting. Only the host should generate playerstates, especially his own!" << std::endl;
        PlayerState player_state;
        return player_state;
    }

    PlayerState host_player_state;
    host_player_state.ping = 0;
    host_player_state.playername = OnlineUtility::GetPlayerName();
    host_player_state.object_id = -1;
    return host_player_state;
}

void Online::AddLocalChatMessage(string chat_message) {
    lock_guard<mutex> guard(online_session->chat_messages_lock);
    online_session->chat_messages.push_back({game_timer.game_time, chat_message});
}

void Online::RemoveOldChatMessages(float threshold) {
    lock_guard<mutex> guard(online_session->chat_messages_lock);
    online_session->chat_messages.erase(remove_if(online_session->chat_messages.begin(), online_session->chat_messages.end(),
        [threshold](ChatMessage i) { return game_timer.game_time - i.time > threshold; }), online_session->chat_messages.end());
 }

const vector<ChatMessage>& Online::GetChatMessages() const {
    return online_session->chat_messages;
}

void Online::AddPeer(NetFrameworkConnectionStatusChanged *data) {
    if (IsHosting()) {
        if (IsLobbyFull()) {
            CloseConnection(data->conn_id, ConnectionClosedReason::LOBBY_FULL);
            AddLocalChatMessage(string("Somebody tried connecting but we don't have any slots free"));
        } else {
            Peer peer;
            peer.conn_id = data->conn_id;
            peer.peer_id = next_free_peer_id++;
            peer.m_info = data->conn_info;
            {
                lock_guard<mutex> guard(online_session->connected_peers_lock);
                online_session->connected_peers.push_back(peer);
                online_session->client_connection_manager.AddConnection(peer.peer_id, data->conn_id);
            }
        }
    } else {
        Peer peer;
        peer.conn_id = data->conn_id;
        peer.peer_id = 0;
        peer.m_info = data->conn_info;
        {
            lock_guard<mutex> guard(online_session->connected_peers_lock);
            online_session->connected_peers.push_back(peer);
        }
    }
}

/// Queue termination of a connection
void Online::CloseConnection(NetConnectionID conn_id, ConnectionClosedReason reason) {
    if(online_session != nullptr) {
        ConnectionToBeClosed temp;
        temp.conn_id = conn_id;
        temp.reason = reason;

        online_session->connections_waiting_to_be_closed.push(temp);
    } else {
        LOGW << "Attempted to close a connection without a valid online session" << endl;
        assert(online_session != nullptr); // Kill the application for now, this should not be happening!
    }
}

/// Close a connection right away, possibly unsafe to call from anywhere
void Online::CloseConnectionImmediate(NetConnectionID conn_id, ConnectionClosedReason reason) {
    lock_guard<mutex> guard(online_session->connected_peers_lock);

    net.CloseConnection(conn_id,reason);

    vector<Peer>::iterator peer = GetPeerIt(conn_id);
    if (peer != online_session->connected_peers.end()) {
        PlayerID player_id = peer->peer_id; // TODO We assume peer_id == player_id
        if (online_session->player_states.find(player_id) != online_session->player_states.end()) {
            auto& player_state = online_session->player_states[player_id];

            SendRawChatMessage(player_state.playername + " disconnected!");

            // Clean up objects assigned to disconnected player
            SceneGraph * graph = Engine::Instance()->GetSceneGraph();
            if (graph != nullptr) {
                Object * o = graph->GetObjectFromID(player_state.object_id);
                if (o != nullptr) {
                    if (o->created_on_the_fly) {
                        graph->map_editor->RemoveObject(o, graph);
                    } else {
                        online_session->free_avatars.push_back(player_state.object_id);
                    }
                    ((MovementObject*)o)->remote = false;
                }
            }

            // Remove playerstate
            online_session->player_states.erase(player_id);
            Send<OnlineMessages::RemovePlayerState>(player_id);

            // TODO do we need to clean the camera and controller of the player?
        }

        online_session->client_connection_manager.RemoveConnection(peer->peer_id);
        online_session->connected_peers.erase(peer);
    } else {
        LOGW << "Tried closing a connection, but there is no peer assigned to NetConnectionID: " << conn_id << std::endl;
    }
}

void Online::CheckPendingMessages() {
    //Locking online_session->connected_peers_lock here because it's used in some arrays here, and if done more specifically
    //theres a risk for overlapping locks with CheckNewMessages(), causing a full lockup.
    //TODO: Reduce number of locks in multiplayer.
    lock_guard<mutex> guard(online_session->connected_peers_lock);

    {
        lock_guard<mutex> guard(online_session->incoming_messages_lock);

        for(auto& it : online_session->incoming_messages) {
            message_handler.Execute(it.message, it.from);
        }

        online_session->incoming_messages.clear();
    }

    if (host_started_level) {
        ApplyPlayerInput();
    }
}

void Online::ApplyPlayerInput() {
    for(auto& it : online_session->player_inputs) {
        // We don't want to apply two inputs of the same key on the same frame
        // Releasing and pressing down the key should not be processed on the same frame
        unordered_map<uint8_t, int> depth;

        for (list<OnlineMessageRef>::iterator i = it.second.begin(); i != it.second.end();) {
            PlayerInputMessage* piks = static_cast<PlayerInputMessage*>(it.second.front().GetData());

            // Get player involved in the input, discard input if missing
            if (online_session->player_states.find(it.first) == online_session->player_states.end()) {
                break;
            }

            int controller_id = online_session->player_states[it.first].controller_id;
            PlayerInput* player_input = Input::Instance()->GetController(controller_id);
            if (player_input == nullptr) {
                break;
            }

            if (depth.find(piks->id) != depth.end()) {
                int velocity = piks->count - depth[piks->id];

                // a negative velocity mean't that we pressed and release
                // process this later.
                if (velocity < 0) {
                    i++;
                    continue;
                }

                // this is a first frame down, process all other inputs on the next frame
                if (depth[piks->id] == 1) {
                    i++;
                    continue;
                }
            }

            // process the input
            KeyState& key_state = player_input->key_down[online_session->binding_name_map_[piks->id]];
            depth[piks->id] = piks->count;
            key_state.count = piks->count;
            key_state.depth_count = piks->depth_count;
            key_state.depth = piks->depth;

            // we are done with this input
            it.second.erase(i++);
        }
    }
}

PlayerState Online::GetOwnPlayerState() {
    std::lock_guard<std::mutex> guard (online_session->connected_peers_lock);

    auto it = online_session->player_states.find(online_session->local_player_id);
    if (it != online_session->player_states.end()) {
        return it->second;
    }

    LOGW << "Tried to get own players state, but there is no state registered for player id: " << std::to_string(online_session->local_player_id) << std::endl;
    PlayerState fallback;
    return fallback;
}

bool Online::TryGetPlayerState(PlayerState& player_state, ObjectID object_id) const {
    if (IsActive()) {
        for (auto& it : online_session->player_states) {
            if (it.second.object_id == object_id) {
                player_state = it.second;
                return true;
            }
        }
    }

    return false;
}

void Online::AssignNewControllerForPlayer(PlayerID player_id) {
    online_session->player_states[player_id].controller_id = Input::Instance()->AllocateRemotePlayerInput();
    online_session->player_states[player_id].camera_id = ActiveCameras::Instance()->CreateVirtualCameraInstance();
}

size_t Online::CompressData(vector<char>& target_buffer, const void* source_buffer, uint32_t source_size) {
    size_t maxSize = ZSTD_compressBound(source_size);

    if(target_buffer.size() < maxSize) {
        target_buffer.resize(maxSize);
    }

    size_t compressedSize = ZSTD_compressCCtx(zstdCContext,
        target_buffer.data(),
        target_buffer.size(),
        source_buffer,
        source_size,
        compression_level);

    if(compressedSize > maxSize) {
        LOGE << "Failed to compress data, assumed size was smaller than the real size" << endl;
    }

    target_buffer.resize(compressedSize);

    return compressedSize;
}

size_t Online::DecompressData(vector<char>& target_buffer, const void* source_buffer, size_t source_size) {
    size_t decompressed_size = ZSTD_getFrameContentSize(source_buffer, source_size);

    if(decompressed_size == ZSTD_CONTENTSIZE_UNKNOWN) {
        LOGW << "Unable to determine uncompressed package size" << std::endl;
    } else if (decompressed_size == ZSTD_CONTENTSIZE_ERROR) {
        LOGE << "Error while trying to determine content size" << std::endl;
    } else if(target_buffer.size() < decompressed_size) {
        target_buffer.resize(decompressed_size);
    }

    size_t final_decompressed_size = 0;
    while(true) {
        size_t final_decompressed_size = ZSTD_decompressDCtx(zstdDContext, target_buffer.data(), target_buffer.size(), source_buffer, source_size);
        if (ZSTD_isError(final_decompressed_size)) {
            LOGE << ZSTD_getErrorString(ZSTD_getErrorCode(final_decompressed_size)) << endl;
        } else if(final_decompressed_size > target_buffer.size()) {
            //If the final decompressed size is larger than the target buffer, re-do the decompression after resizing.
            target_buffer.resize(final_decompressed_size);
            continue;
        }
        break;
    }

    return final_decompressed_size;
}

uint32_t Online::GetPlayerCount() {
    lock_guard<mutex> guard(online_session->connected_peers_lock);
    return online_session->player_states.size();
}

/// Hosts will add their name here, clients will have their name added by the host later
void Online::BroadcastChatMessage(const string& chat_message) {
    if(IsActive()) {
        if(IsHosting()) {
            if (OnlineUtility::IsCommand(chat_message)) {
                OnlineUtility::HandleCommand(online_session->local_player_id, chat_message);
                AddLocalChatMessage(OnlineUtility::GetPlayerName() + ": " + chat_message);
            } else {
                SendRawChatMessage(OnlineUtility::GetPlayerName() + ": " + chat_message);
            }
        } else {
            SendRawChatMessage(chat_message);
        }
    }
}

void Online::SendRawChatMessage(const string& raw_chat_entry) {
    if(IsActive()) {
        Send<OnlineMessages::ChatEntryMessage>(raw_chat_entry);
        if (IsHosting()) {
            AddLocalChatMessage(raw_chat_entry);
        }
    }
}

vector<PlayerState> Online::GetPlayerStates() {
    if (IsActive()) {
        vector<PlayerState> player_states;
        for (const auto& it : online_session->player_states) {
            player_states.push_back(it.second);
        }
        return player_states;
    }

    vector<PlayerState> empty;
    return empty;
}

void Online::Send(const OnlineMessageRef& message_ref) {
    if(IsActive()) {
        lock_guard<mutex> guard(online_session->outgoing_messages_lock);
        online_session->outgoing_messages.push_back({true, 0, message_ref});
    }
}

void Online::SendTo(NetConnectionID target, const OnlineMessageRef& message_ref) {
    if(IsActive()) {
        lock_guard<mutex> guard(online_session->outgoing_messages_lock);
        online_session->outgoing_messages.push_back({false, target, message_ref});
    }
}

void Online::SendIntFloatScriptParam(uint32_t id, const string& key, const ScriptParam& param) {
    if (param.IsFloat()) {
        Send<OnlineMessages::SPUnionMessage>(id, key, param.GetFloat(), ScriptParam::ScriptParamType::FLOAT, param.editor().type(), param.editor().GetDetails());
    } else {
        Send<OnlineMessages::SPUnionMessage>(id, key, param.GetInt(), ScriptParam::ScriptParamType::INT, param.editor().type(), param.editor().GetDetails());
    }
}

void Online::SendStringScriptParam(uint32_t id, const string& key, const ScriptParam& param) {
    Send<OnlineMessages::SPStringMessage>(id, key, param.GetStringForSocket(), param.editor().type(), param.editor().GetDetails());
}

void Online::SendScriptParam(uint32_t id, const string& key, const ScriptParam& param) {
    if (param.IsString()) {
        SendStringScriptParam(id, key, param);
    } else {
        SendIntFloatScriptParam(id, key, param);
    }
}

void Online::SendScriptParamMap(uint32_t id, const ScriptParamMap& param){
    for (auto& it : param) {
        SendScriptParam(id, it.first, it.second);
    }
}

bool Online::GetHostAllowsClientEditor() const {
    if (online_session == nullptr)
        return false;
    return online_session->host_session_flags[OnlineFlags::ALLOWSEDITOR];
}

void Online::SetIfHostAllowsClientEditor(bool mode) {
    if (online_session != nullptr) {
        online_session->host_session_flags[OnlineFlags::ALLOWSEDITOR] = mode;
    }
}

void Online::SetDefaultHotJoinCharacter(const string & path) {
    default_hot_join_characte_path = path;
}

string Online::GetDefaultHotJoinCharacter() const {
    return default_hot_join_characte_path;
}

void Online::SessionStarted(bool host_started_the_level) {
    host_started_level = host_started_the_level;
}

bool Online::ForceMapStartOnLoad() const {
    return host_started_level;
}

bool Online::AllClientsReady() {
    lock_guard<mutex> guard(online_session->connected_peers_lock);
    return online_session->client_connection_manager.IsEveryClientLoaded();
}

ScriptParams* Online::GetScriptParamsFromID(ObjectID id) {
    if(Engine::Instance()->GetSceneGraph() != nullptr) {
        if(id == numeric_limits<uint32_t>::max())
            return &Engine::Instance()->GetSceneGraph()->level->script_params();

        Object* o = Engine::Instance()->GetSceneGraph()->GetObjectFromID(id);
        if (o != nullptr)
            return o->GetScriptParams();
    }
    return nullptr;
}

void Online::UpdateMovementObjectFromID(uint32_t id) {
    if(id != numeric_limits<uint32_t>::max()) {
        Object * o = Engine::Instance()->GetSceneGraph()->GetObjectFromID(id);
        if(o != nullptr && o->GetType() == EntityType::_movement_object) {
            ((MovementObject*)o)->UpdateScriptParams();
        }
    }
}

bool Online::SetAvatarCameraAttachedMode(bool mode) {
    return attach_avatar_camera = mode;
}

bool Online::IsAvatarCameraAttached() const {
    return attach_avatar_camera && Engine::Instance()->GetSceneGraph()->map_editor->state_ == MapEditor::kInGame;
}

void Online::SendLevelMessage(const string& msg) {
    // we currently only want start_dialogue
    if (msg.find("reset") == string::npos)
        return; // tutorial is very spammy

    Send<OnlineMessages::SendLevelMessage>(msg);
}

void Online::UpdateObjects(SceneGraph *scenegraph_) {
    uint32_t wall_ticks = game_timer.GetWallTicks();
    uint32_t tick_delta = wall_ticks - last_update_wall_ticks;
    uint32_t quarter_tick_delta = wall_ticks - last_quarter_update_wall_ticks;

    bool is_quarter_tick = quarter_tick_delta >= tick_period/4;
    bool is_tick = tick_delta >= tick_period;

    if (IsHosting()) {
        for(Object *o : scenegraph_->movement_objects_) {
            MovementObject* mo = static_cast<MovementObject*>(o);
            RiggedObject* rigged_object = mo->rigged_object();

            // TODO: Loop over peers and do a more frequent update of their own character.
            // Increse the rate further whenever there's a recent input package from that peer tied to the character.
            // We want to send a package the same frame that the object's movement intention has changed to keep it up-to-date.
            // We check when the movementobject was last updated. There is no reason to send updates if it hasn't updated
            static std::unordered_map<MovementObject*, float> last_timestamps;
            if (is_quarter_tick && last_timestamps[mo] != mo->walltime_last_update) {
                last_timestamps[mo] = mo->walltime_last_update;
                Send<OnlineMessages::MovementObjectUpdate>(mo);

                vector<MorphTargetStateStorage>& network_morphs = rigged_object->network_morphs;
                for (auto& it : network_morphs) {
                    if (it.dirty) {
                        Send<OnlineMessages::MorphTargetUpdate>(mo->GetID(), it);
                        it.dirty = false;
                    }
                }
            }
        }

        if (is_tick) {
            for (Object *o : scenegraph_->objects_) {
                switch (o->GetType()) {
                    case _env_object:
                        if (o->online_transform_dirty) {
                            Send<OnlineMessages::EnvObjectUpdate>(static_cast<EnvObject*>(o));
                        }
                        break;

                    case _item_object:
                        Send<OnlineMessages::ItemUpdate>(static_cast<ItemObject*>(o));
                        break;
                }
                o->online_transform_dirty = false;
            }
        }
    } else if(IsClient()) {
        if (is_tick) {
            SendPlayerCameraState();
        }
        //Send input information every frame, we want to make sure we get the latest.
        SendPlayerInputState();
    }

    if(is_tick) {
        last_update_wall_ticks += tick_delta;
    }

    if (is_quarter_tick) {
        last_quarter_update_wall_ticks += quarter_tick_delta;
    }
}

bool Online::HasFreeAvatars() const {
    return !online_session->free_avatars.empty();
}

uint32_t Online::GetNumberOfFreeAvatars() const {
    return online_session->free_avatars.size();
}

uint8_t Online::GetPlayerLimit() {
    if(Engine::Instance()->GetSceneGraph() != nullptr && Engine::Instance()->GetSceneGraph()->level != nullptr) {
        if(Engine::Instance()->GetSceneGraph()->level->script_params().HasParam("Player Limit")) {
            return Engine::Instance()->GetSceneGraph()->level->script_params().ASGetInt("Player Limit");
        }
        return 8; // Default to 8 players (arbitrary limit)
    }
    return 1; // No level, no players
}

bool Online::IsLobbyFull() {
    return GetPlayerLimit() <= GetPlayerCount();
}

void Online::AddFreeAvatarsIds() {
    Engine::Instance()->GetAvatarIds(online_session->free_avatars);
}

void Online::RemoveAllAvatarIds() {
    online_session->free_avatars.clear();
    online_session->taken_avatars.clear();
}

void Online::RemoveFreeAvatarId(uint32_t id) {
    vector<ObjectID>::iterator it = find(online_session->free_avatars.begin(), online_session->free_avatars.end(), id);

    if (it != online_session->free_avatars.end()) {
        online_session->free_avatars.erase(it);
    }
}

void Online::AddFreeAvatarId(uint32_t id) {
    vector<ObjectID>::iterator it = find(online_session->taken_avatars.begin(), online_session->taken_avatars.end(), id);

    if (it == online_session->taken_avatars.end()) {
        online_session->free_avatars.push_back(id);
    }
}

ObjectID Online::GetFreeAvatarID() {
    ObjectID avatarID = -1;
    if (!online_session->free_avatars.empty()) {
        auto front = online_session->free_avatars.begin();
        avatarID = *front;
        online_session->taken_avatars.push_back(avatarID);
        online_session->free_avatars.erase(front);
    }

    return avatarID;
}

void Online::OnConnectionChange(NetFrameworkConnectionStatusChanged *data) {
    switch (data->conn_info.connection_state) {
        case NetFrameworkConnectionState::Connecting: {
            if (IsHosting()) {
                bool accept_connection = true;
                if(accept_connection) {
                    int accept_connection_result;
                    bool is_ok = net.AcceptConnection(data->conn_id, &accept_connection_result);

                    if (is_ok) {
                        AddLocalChatMessage(string("Accepted connection: ") + to_string(data->conn_id));
                    } else {
                        string error_string = to_string(data->conn_id) + " failed to accept connection: (" + net.GetResultString(accept_connection_result) + ")";
                        AddLocalChatMessage(error_string);
                        LOGW << error_string << endl;
                    }
                } else {
                    net.CloseConnection(data->conn_id, ConnectionClosedReason::UNSPECIFIED);
                }
            } else {
                //We come here when we have an outward connection from the client.
            }
            break;
        }

        case NetFrameworkConnectionState::ClosedByPeer:
        {
            ConnectionClosedReason reason = data->conn_info.end_reason;
            CloseConnection(data->conn_id, reason);
            if (IsClient()) {
                StopMultiplayer();

                if(ConnectionClosedReasonUtil::IsUnusualReason(reason)) {
                    string error = ConnectionClosedReasonUtil::GetErrorMessage(reason);
                    Engine::Instance()->QueueErrorMessage("Connection Closed", error);
                }
            }
            break;
        }

        case NetFrameworkConnectionState::ProblemDetectedLocally:
        {
            ConnectionClosedReason reason = data->conn_info.end_reason;
            if (IsClient()) {
                if(ConnectionClosedReasonUtil::IsUnusualReason(reason)) {
                    string error = ConnectionClosedReasonUtil::GetErrorMessage(reason);
                    Engine::Instance()->QueueErrorMessage("Connection Lost", error);
                }
            }

            CloseConnection(data->conn_id, reason);
            break;
        }

        case NetFrameworkConnectionState::FindingRoute: {
            // packages will be dropped in here
            break;
        }

        case NetFrameworkConnectionState::Connected: {
            AddPeer(data);
            break;
        }

        default: {
            LOGE << "Online::OnConnectionChange: Connection has unhandeld state" << endl;
            LOGE << data->conn_info.connection_state << endl;
        }
    }
}

void Online::StopListening() {
    for (auto& socket : listen_sockets) {
        net.CloseListenSocket(socket);
    }

    listen_sockets.clear();
}

bool Online::NetFrameworkHasFriendInviteOverlay() const {
    return net.HasFriendInviteOverlay();
}

void Online::ActivateGameOverlayInviteDialog() {
    if (!IsActive()) {
        LOGW << "Attempted to show Game Overlay Invite dialogue while offline!" << std::endl;
        return;
    }

#if ENABLE_STEAMWORKS
    // TODO this is steam specific code, it needs to be moved to SteamNetFramework
    // Preferably called "ActivateFriendInviteOverlay()" to match HasFriendInviteOverlay()
    SteamworksMatchmaking *matchmaking = Steamworks::Instance()->GetMatchmaking();
    if (matchmaking) {
        if (!p2p_sockets_active) {
            HSteamListenSocket listen = matchmaking->ActivateGameOverlayInviteDialog();
            listen_sockets.insert(listen);
            p2p_sockets_active = true;
        } else {
            matchmaking->OpenInviteDialog();
        }
    } else {
        LOGE << "Couldn't open invite overlay page" << endl;
    }
#else 
    LOGW << "Tried opening a game invite overlay, but we haven't compiled in any support for one." << endl;
#endif
}

void Online::CreateListenSocketIP(const string &local_address) {
    NetListenSocketID listen_socket = net.CreateListenSocketIP(local_address);
    listen_sockets.insert(listen_socket);
}

void Online::ConnectByIPAddress(const string &address) {
    if (!IsActive()) {
        StartMultiplayer(MultiplayerMode::Client);

        NetConnectionID conn_id;

        if(net.ConnectByIPAddress(address, &conn_id) == false) {
            StopMultiplayer();
        }
    }
}

uint8_t Online::GetBindID(string binding_name) {
    return online_session->binding_id_map_[binding_name];
}

void Online::AssignBindID(string binding_name) {
    if(online_session->binding_id_map_.find(binding_name) == online_session->binding_id_map_.end()) {
        online_session->binding_id_map_[binding_name] = ++online_session->binding_id_map_counter_;
        online_session->binding_name_map_[online_session->binding_id_map_[binding_name]] = binding_name;
    } else {
        LOGW << "Binding \"" << binding_name << "\" has been assigned multiple times!" << std::endl;
    }
}

void Online::NetworkDataThread() {
    while (IsActive()) {
        net.Update();
        CheckNewMessages();
        SendMessages();

        if(IsHosting()) {
            HandleConnectionsToBeClosed();
        }

        message_handler.StepFreeUnreferencedMessage();
    }
}

void Online::ClearOnlineSession() {
    delete online_session;
    online_session = new OnlineSession();
}

void Online::SendMessages() {
    SendMessageObjects();
    if (IsHosting()) {
        SendStateMessages();
    }
}

void Online::SendMessageObjects() {
    online_session->connected_peers_lock.lock();

    online_session->outgoing_messages_lock.lock();

    //First, send persistent packages, if we see there's a new client who isn't up to date.
    if(host_started_level && IsHosting()) {
        for(auto & peer : online_session->connected_peers) {
            if(online_session->client_connection_manager.IsClientLoaded(peer.peer_id) && online_session->client_connection_manager.HasClientGottenPersistentQueue(peer.peer_id) == false) {
                for(auto & persistent_outgoing_message : online_session->persistent_outgoing_messages)  {
                    OnlineMessageBase* omb = static_cast<OnlineMessageBase*>(message_handler.GetMessageData(persistent_outgoing_message));

                    if(omb != nullptr) {
                        PackageHeader package_header;

                        package_header.package_type = PackageHeader::Type::MESSAGE_OBJECT;
                        package_header.message_ref = persistent_outgoing_message;

                        binn* l = package_header.Serialize();

                        size_t compressed_size = CompressData(compressed_serialization_buffer, binn_ptr(l), binn_size(l));

                        binn_free(l);

                        SendMessageToConnection(peer.conn_id, compressed_serialization_buffer.data(), compressed_size, true, false);
                    } else {
                        LOGE << "Package data pointer was null, this is unexpected and odd." << endl;
                    }
                }
                online_session->client_connection_manager.SetClientGottenPersistentQueue(peer.peer_id);
            }
        }
    }

    //Send the standard message queues. Store away persistent messages for future client peers if we are host.
    for(int i = 0; i < online_session->outgoing_messages.size(); i++) {
        OnlineSession::OutgoingMessage& outgoing_message = online_session->outgoing_messages[i];
        OnlineMessageBase* omb = static_cast<OnlineMessageBase*>(message_handler.GetMessageData(outgoing_message.message));

        if(omb != nullptr) {
            PackageHeader package_header;

            package_header.package_type = PackageHeader::Type::MESSAGE_OBJECT;
            package_header.message_ref = outgoing_message.message;

            binn* l = package_header.Serialize();

            size_t compressed_size = CompressData(compressed_serialization_buffer, binn_ptr(l), binn_size(l));

            binn_free(l);

            for(auto & peer : online_session->connected_peers) {
                bool send_message = true;

                if(outgoing_message.broadcast == false && outgoing_message.target != peer.conn_id) {
                    send_message = false;
                //Don't send packags to client peers who have recently connected, but hasn't loaded the level, or finished getting the persistent queue yet.
                } else if(IsHosting() && omb->cat != OnlineMessageCategory::TRANSIENT && online_session->client_connection_manager.HasClientGottenPersistentQueue(peer.peer_id) == false) {
                    send_message = false; 
                }

                if(send_message) {
                    SendMessageToConnection(peer.conn_id, compressed_serialization_buffer.data(), compressed_size, omb->reliable_delivery, i+1 == online_session->outgoing_messages.size());
                }
            }

            if(IsHosting()) {
                if(omb->cat == OnlineMessageCategory::LEVEL_PERSISTENT) {
                    online_session->persistent_outgoing_messages.push_back(outgoing_message.message);
                }
            }
        } else {
            LOGE << "Package data pointer was null, this is unexpected and odd." << endl;
        }
    }

    online_session->outgoing_messages.clear();

    online_session->outgoing_messages_lock.unlock();

    online_session->connected_peers_lock.unlock();
}

void Online::HandleConnectionsToBeClosed() {
    while (online_session->connections_waiting_to_be_closed.empty() == false) {
        ConnectionToBeClosed& connection = online_session->connections_waiting_to_be_closed.front();
        CloseConnectionImmediate(connection.conn_id, connection.reason);
        online_session->connections_waiting_to_be_closed.pop();
    }
}

void Online::SendMessageToConnection(NetConnectionID conn_id, char* buffer, uint32_t bytes, bool reliable, bool flush) {
    net.SendMessageToConnection(conn_id, buffer, bytes, reliable, flush);
}

void Online::SendStateMessages() {
    lock_guard<mutex> guard(online_session->connected_peers_lock);
    for (auto& conn : online_session->connected_peers) {
        if(conn.should_send_state_messages) {
            {
                lock_guard<mutex> guard(online_session->state_packages_lock);
                for (auto& package : online_session->state_packages) {
                    SendTo<OnlineMessages::AngelscriptData>(conn.conn_id, package.first, package.second.data, true);
                }
            }

            conn.should_send_state_messages = false;
        }
    }
}

void Online::CheckNewMessages() {
    // we are locking over a big segment and I/O, but it's not dangerous
    // calls that could be waiting are not time sensitive.
    lock_guard<mutex> guard(online_session->connected_peers_lock);

    //static SteamNetworkingMessage_t ** msg = (SteamNetworkingMessage_t **)malloc(sizeof(SteamNetworkingMessage_t*) * 1);
    NetworkingMessage msg;

    for (auto& it : online_session->connected_peers) {
        int nrOfMessages = net.ReceiveMessageOnConnection(it.conn_id, &msg);

        if (nrOfMessages == -1) {
            LOGE << "Invalid connection handle: " << it.conn_id << endl;
        }

        for (int i = 0; i < nrOfMessages; i++) {
            PackageHeader package_header;
            static vector<char> data;
            DecompressData(data, msg.GetData(), msg.GetSize());
            package_header.Deserialize(data.data());

            if (package_header.package_type == PackageHeader::Type::MESSAGE_OBJECT) {
                online_session->incoming_messages_lock.lock();
                online_session->incoming_messages.push_back({it.peer_id, package_header.message_ref});
                online_session->incoming_messages_lock.unlock();
            } else {
                LOGE << "Unrecognized package type: " << (int)package_header.package_type << endl;
            }
            msg.Release();

        }
    }
}

binn* PackageHeader::Serialize() {
    binn* l = binn_list();

    binn_list_add_int8(l, (int8_t)package_type);
    if(package_type == Type::MESSAGE_OBJECT) {
        binn_list_add_int8(l, (int8_t)message_handler.GetMessageType(message_ref));
        binn* serialized_object = message_handler.Serialize(message_ref);
        if(serialized_object != nullptr) {
            binn_list_add_object(l,serialized_object);
        } else {
            LOGE << "Serialization of PackageHeader message resulted in a nullptr" << endl;
        }
        binn_free(serialized_object);
    }

    return l;
}

void PackageHeader::Deserialize(void* data) {
    //Do a raw read, don't worry about copying the data first.
    binn* l = (binn*)data;

    int pos = 1;

    int8_t v_package_type;
    if(binn_list_get_int8(l, pos++, &v_package_type) == false) {
        LOGE << "Failed to read package type from binn message" << endl;
    }
    this->package_type = (Type)v_package_type;

    if(package_type == Type::MESSAGE_OBJECT) {
        int8_t message_type;
        binn_list_get_int8(l, pos++, &message_type);

        void* serialized_object;
        binn_list_get_object(l, pos++, &serialized_object);

        if(serialized_object != nullptr) {
            message_ref = message_handler.Deserialize((OnlineMessageType)message_type, (binn*)serialized_object);
        } else {
            LOGE << "Got a nullptr when trying to fetch a MESSAGE_OBJECT binn from a PackageHeader" << endl;
        }
    }
}
