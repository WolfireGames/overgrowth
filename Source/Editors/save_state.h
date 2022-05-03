//-----------------------------------------------------------------------------
//           Name: save_state.h
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

#include <Game/EntityDescription.h>

#include <list>

namespace ChunkType {
    enum ChunkType {
        LEVEL,
        SKY_EDITOR,
        OBJECT,
        GROUP
    };
}

struct SavedChunk {
private:
    static unsigned global_chunk_id_counter;
    unsigned global_chunk_id;
public:
    SavedChunk();
    int state_id;
    ChunkType::ChunkType type;
    int obj_id;
    EntityDescription desc;
    unsigned GetGlobalID();
};

// StateHistory stores all of the saved state chunks, as well as information
// about where we are in the undo/redo timeline
class StateHistory {
public:
    std::list<SavedChunk> chunks;
    int current_state;
    int start_state;
    int num_states;
    
    void clear();

    StateHistory(){clear();}

    std::list<SavedChunk>::iterator GetCurrentChunk();
};

void AddChunkToHistory(std::list<SavedChunk> &chunk_list, int state_id, SavedChunk &saved_chunk);
