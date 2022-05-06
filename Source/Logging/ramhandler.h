//-----------------------------------------------------------------------------
//           Name: ramhandler.h
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

#include <mutex>

#include <Logging/loghandler.h>
#include <Internal/integer.h>
#include <Memory/block_allocator.h>

struct RamHandlerLogRow {
    uint8_t* data;
    uint8_t type;
    uint8_t prefix[PREFIX_LENGTH];
    uint8_t filename[FILENAME_LENGTH];
    uint32_t row;
};
/*!
	Potential child to LogHandler, logging output to an array for later rendering.
*/
class RamHandler : public LogHandler
{
private:
    static const unsigned ROW_INSTANCE_SIZE = 256;
    RamHandlerLogRow row_instances[ROW_INSTANCE_SIZE];
    unsigned row_instance_pos;
    unsigned row_instance_count;
    
    void* data;
    BlockAllocator mem;
    bool is_locked;

    std::mutex log_mutex_;

    void DeleteMessage(uint32_t index);
public:
	RamHandler();

	~RamHandler() override;

	/*! \param message will be printed to std::cout */
	void Log( LogSystem::LogType type, int row, const char* filename, const char* cat, const char* message_prefix, const char* message ) override;
    void Flush() override;

    void Lock();
    void Unlock();
    RamHandlerLogRow GetLog( unsigned index );
    unsigned GetCount();
};

