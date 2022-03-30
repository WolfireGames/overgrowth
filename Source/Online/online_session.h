//-----------------------------------------------------------------------------
//           Name: online_session.h
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
#include <Online/online_peer.h>
#include <Online/online_client_connection_manager.h>

#include <Network/net_framework.h>

#if ENABLE_STEAMWORKS || ENABLE_GAMENETWORKINGSOCKETS
#include <steamnetworkingtypes.h> // TODO this should be removed once steam is abstracted out
#endif

#include <unordered_map>
#include <vector>
#include <list>
#include <queue>
#include <string>
#include <mutex>
#include <utility>

using std::mutex;
using std::queue;
using std::vector;
using std::string;
using std::pair;
using std::unordered_map;
using std::list;

class OnlineMessageBase;

/// This class is designed to be "current session" data only.
/// Once leaving multiplayer this object will be deallocated.
class OnlineSession {
public:
    PlayerID local_player_id = 0; // TODO this will have to become an array to support multiple people per connection

    queue<AngelScriptUpdate> peer_queued_level_updates;
    queue<AngelScriptUpdate> peer_queued_sync_updates;

    mutex connected_peers_lock;
    vector<Peer> connected_peers; // will replace peers - Shared in steam and socket threads - should be protected
    ClientConnectionManager client_connection_manager;

    mutex state_packages_lock;
    unordered_map<int, AngelScriptUpdate> state_packages; // hold updates needed when new clients join

    mutex chat_messages_lock;
    vector<ChatMessage> chat_messages;

    unordered_map<PlayerID, PlayerState> player_states;
    unordered_map<PlayerID, list<OnlineMessageRef>> player_inputs;

    struct OutgoingMessage {
        //Broadcast means send to all peers.
        //If broadcast is false target has to be set.
        bool broadcast;
        NetConnectionID target;
        OnlineMessageRef message;
    };

    //New outgoing message queue to replace all other examples of sending data from the main thread 
    //and queue into the networking thread.
    mutex outgoing_messages_lock;
    vector<OutgoingMessage> outgoing_messages;

    //These are persistent outgoing messages from the host, 
    //messages that are sent to all current peers but are persistent are moved
    //over to this list after being sent.
    //When a new peer joins the game, all of these are queued up 
    //for the peer before the normal outgoing_messages queue is consumed for them.
    vector<OnlineMessageRef> persistent_outgoing_messages;

    struct IncomingMessage {
        PeerID from;
        OnlineMessageRef message;
    };

    //New incoming message queue to replace all other examples of messages coming in from the network socket to be excuted on the main thread.
    mutex incoming_messages_lock;
    vector<IncomingMessage> incoming_messages;
    

    queue<ConnectionToBeClosed> connections_waiting_to_be_closed;
    vector<ObjectID> free_avatars;
	vector<ObjectID> taken_avatars;

    map<OnlineFlags, bool> host_session_flags;

    //ID Map between input-label strings and number id's.
    unsigned binding_id_map_counter_ = 0;
    map<string, uint8_t> binding_id_map_;
    map<uint8_t, string> binding_name_map_;

    float last_ping_time = 0.0f;
    uint32_t ping_counter = 0;
    uint32_t last_ping_id = 0;
};
