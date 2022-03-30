//-----------------------------------------------------------------------------
//           Name: mem_read_entity_description.cpp
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
#include "mem_read_entity_description.h"

#include <Game/EntityDescription.h>
#include <Internal/memwrite.h>

void MemReadItemConnectionData(ItemConnectionData &icd, const std::vector<char> &mem_stream, int &index) {
    memread(&icd.id, sizeof(icd.id), 1, mem_stream, index);
    memread(&icd.mirrored, sizeof(icd.mirrored), 1, mem_stream, index);
    memread(&icd.attachment_type, sizeof(icd.attachment_type), 1, mem_stream, index);
    int32_t str_size;
    memread(&str_size, sizeof(str_size), 1, mem_stream, index);
    icd.attachment_str.resize(str_size);
    memread(&icd.attachment_str[0], str_size, 1, mem_stream, index);
}

void MemWriteItemConnectionData(const ItemConnectionData &icd, std::vector<char> &mem_stream) {
    memwrite(&icd.id, sizeof(icd.id), 1, mem_stream);
    memwrite(&icd.mirrored, sizeof(icd.mirrored), 1, mem_stream);
    memwrite(&icd.attachment_type, sizeof(icd.attachment_type), 1, mem_stream);
    int32_t str_size = icd.attachment_str.size();
    memwrite(&str_size, sizeof(str_size), 1, mem_stream);
    memwrite(&icd.attachment_str[0], str_size, 1, mem_stream);
}

void MemReadNavMeshConnectionData(NavMeshConnectionData &nmcd, const std::vector<char> &mem_stream, int &index)
{
    memread(&nmcd.other_object_id, sizeof(nmcd.other_object_id), 1, mem_stream, index); 
    memread(&nmcd.offmesh_connection_id, sizeof(nmcd.offmesh_connection_id), 1, mem_stream, index);
    memread(&nmcd.poly_area, sizeof(nmcd.poly_area), 1, mem_stream, index);
}

void MemWriteNavMeshConnectionData(const NavMeshConnectionData &nmcd, std::vector<char> &mem_stream)
{
    memwrite(&nmcd.other_object_id, sizeof(nmcd.other_object_id), 1, mem_stream);
    memwrite(&nmcd.offmesh_connection_id, sizeof(nmcd.offmesh_connection_id), 1, mem_stream);
    memwrite(&nmcd.poly_area, sizeof(nmcd.poly_area), 1, mem_stream);
}

