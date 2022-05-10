//-----------------------------------------------------------------------------
//           Name: ogg_loader.h
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
#include <Sound/Loader/base_loader.h>

#include <Wrappers/vorbis.h>
#include <Internal/filesystem.h>

#ifdef __APPLE__
#include <OpenAL/al.h>
#else
#include <al.h>
#endif

#include <string>

class oggLoader : public baseLoader {
   private:
    FILE* m_ogg_file;
    OggVorbis_File m_ogg_stream;
    vorbis_info* m_vorbis_info;
    vorbis_comment* m_vorbis_comment;
    std::string m_path;
    unsigned long m_channels;
    bool ended;

   public:
    oggLoader(Path rel_path);
    ~oggLoader() override;

    int stream_buffer_int16(char* buffer, int size) override;
    unsigned long get_sample_count() override;
    unsigned long get_channels() override;
    int get_sample_rate() override;
    int rewind() override;
    bool is_at_end() override;

    int64_t get_pcm_pos() override;
    void set_pcm_pos(int64_t pos) override;
};
