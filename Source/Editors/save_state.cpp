//-----------------------------------------------------------------------------
//           Name: save_state.cpp
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
#include "save_state.h"

unsigned SavedChunk::global_chunk_id_counter = 1;

SavedChunk::SavedChunk() : global_chunk_id(global_chunk_id_counter++)
{

}

unsigned SavedChunk::GetGlobalID()
{
    return global_chunk_id;
}

void AddChunkToHistory(std::list<SavedChunk> &chunk_list, int state_id, SavedChunk &saved_chunk){
    saved_chunk.state_id = state_id;
    chunk_list.push_back(saved_chunk);
}

void StateHistory::clear() {
    chunks.clear();
    current_state = -1;
    start_state = 0;
    num_states = 0;;
}

std::list<SavedChunk>::iterator StateHistory::GetCurrentChunk()
{
    std::list<SavedChunk>::iterator chunkit = chunks.begin();
    for( ; chunkit != chunks.end(); chunkit++ )
    {
        if( chunkit->state_id == current_state )
        {
            return chunkit;
        }
    }
    return chunks.end();
}
