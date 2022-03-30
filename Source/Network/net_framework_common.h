//-----------------------------------------------------------------------------
//           Name: net_framework_common.h
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

#include <cstdint>
#include <string>
#include <iostream>

using std::string;
using std::ostream;

struct ConnectionStatus {
	int ms_ping;
	float connection_quality_local;
	float connection_quality_remote;
	float out_packets_per_sec;
	float out_bytes_per_sec;
	float in_packets_per_sec;
	float in_bytes_per_sec;
	int send_rate_bytes_per_second;
	int pending_unreliable;
	int pending_reliable;
	int sent_unacked_reliable;
	int64_t usec_queue_time;
};

enum class NetFrameworkConnectionState {
    Unknown,
    Connecting,
    ClosedByPeer,
    ProblemDetectedLocally,
    FindingRoute,
    Connected,
};

string to_string(const NetFrameworkConnectionState& state);
ostream& operator<<(ostream& out, const NetFrameworkConnectionState& in);
