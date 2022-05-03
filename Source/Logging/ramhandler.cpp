//-----------------------------------------------------------------------------
//           Name: ramhandler.cpp
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
#include <Logging/ramhandler.h>

#include <iostream>
#include <cassert>

#include <Utility/strings.h>

RamHandler::RamHandler() : is_locked(false) {
    data = malloc(64 * 2048);
    mem.Init(data, 2048, 64);
    mem.no_log = true;
}

RamHandler::~RamHandler() {
    free(data);
    data = NULL;
}

void RamHandler::DeleteMessage(uint32_t index) {
    if( row_instances[index].data != NULL ) {
        mem.Free(row_instances[index].data);
        row_instances[index].data = NULL;
        row_instance_count--;
    }
}

void RamHandler::Log( LogSystem::LogType type, int row, const char* filename, const char* cat, const char* message_prefix, const char* message ) {
	log_mutex_.lock();

    unsigned next_index = (row_instance_pos + 1) % ROW_INSTANCE_SIZE;
    unsigned delete_index_offset = 0;

    DeleteMessage(next_index);

    size_t msg_size = strlen(message)+1;
    void* new_mem = NULL; 

    while(new_mem == NULL) {
        new_mem = mem.Alloc(msg_size);

        if(new_mem) {
            memcpy(new_mem,message,msg_size);
            row_instances[next_index].data = (uint8_t*)new_mem;  
            row_instances[next_index].type = type;
            strscpy((char*)row_instances[next_index].prefix,cat,PREFIX_LENGTH);
            strscpy((char*)row_instances[next_index].filename,filename,FILENAME_LENGTH);
            row_instances[next_index].row = row;

            row_instance_count++;
            row_instance_pos = next_index;
        } else if(row_instance_count > 0){
            delete_index_offset++;
            DeleteMessage((next_index + delete_index_offset) % ROW_INSTANCE_SIZE);
        } else {
            std::cerr << "Unable to allocate memory for log-line" << std::endl;
			break;
        }
    } 

	log_mutex_.unlock();
}

void RamHandler::Flush() {

}

void RamHandler::Lock() {
    assert(is_locked == false);
    is_locked = true;
    log_mutex_.lock(); 
}

void RamHandler::Unlock() {
    assert(is_locked == true);
    is_locked = false;
    log_mutex_.unlock();
}

RamHandlerLogRow RamHandler::GetLog( unsigned index ) {
    assert(is_locked);
    return row_instances[(row_instance_pos - row_instance_count + 1 + index) % ROW_INSTANCE_SIZE];
}

unsigned RamHandler::GetCount() {
    assert(is_locked);
    return row_instance_count; 
}
