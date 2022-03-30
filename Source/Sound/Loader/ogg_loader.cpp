//-----------------------------------------------------------------------------
//           Name: ogg_loader.cpp
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

#include <Internal/error.h>
#include <Internal/filesystem.h>

#include <Sound/Loader/ogg_loader.h>
#include <Compat/fileio.h>
#include <Logging/logdata.h>

#include <cstring>
#include <cerrno>

#ifdef WIN32
static int _fseek64_wrap(FILE *f,ogg_int64_t off,int whence){
    if(f==NULL)return(-1);
    return fseek(f,(long)off,whence);
}

static size_t do_read(void *a, size_t b, size_t c, void *d)
{
    return fread(a, b, c, (FILE *)d);
}

#endif

oggLoader::oggLoader(Path path) :
    m_ogg_file(NULL),
    m_vorbis_info(NULL),
    m_vorbis_comment(NULL)
{
    int result;

    ended = false;

    if(!(m_ogg_file = my_fopen(path.GetFullPath(), "rb"))) {
        DisplayError("Error",("Could not open file: " + path.GetFullPathStr() + " " + strerror(errno) ).c_str());
        ended = true;
        return;
    }


#ifdef WIN32
    ov_callbacks callbacks = {
        (size_t (*)(void *, size_t, size_t, void *))  fread,
        (int (*)(void *, ogg_int64_t, int))           _fseek64_wrap,
        (int (*)(void *))                             fclose,
        (long (*)(void *))                            ftell};

    if((result = ov_open_callbacks(m_ogg_file, &m_ogg_stream, NULL, 0, callbacks)) < 0)
#else
    if((result = ov_open(m_ogg_file, &m_ogg_stream, NULL, 0)) < 0)
#endif
    {
        DisplayError("Error","Problem with ogg streaming (ov_open failed)");
        // only close file pointers if ov_open fails.
        fclose(m_ogg_file);
        m_ogg_file = NULL;
        ended = true;
        return;
    }

    m_vorbis_info = ov_info(&m_ogg_stream, -1);
    m_vorbis_comment = ov_comment(&m_ogg_stream, -1);
}

oggLoader::~oggLoader()
{
    if (m_ogg_file != NULL)
        ov_clear(&m_ogg_stream);
}

int oggLoader::stream_buffer_int16(char *buffer, int size)
{
    // ehh, error condition...
    if (m_vorbis_info == NULL)
    {
        ::memset(buffer, 0, size);
        return 0;
    }

    int section;

#ifdef __BIG_ENDIAN__
            int bigendianp = 1;
#else
            int bigendianp = 0;
#endif

    int length =  ov_read(&m_ogg_stream, buffer, size, bigendianp, 2, 1, &section);
    if( length == 0 )
        ended = true;
    return length;
}

unsigned long oggLoader::get_sample_count()
{
    if (m_ogg_file != NULL)
        return (unsigned long)ov_pcm_total(&m_ogg_stream, 0);
    else
        return 0;
}

unsigned long oggLoader::get_channels()
{
    if (m_vorbis_info)
        return m_vorbis_info->channels;
    else
        return 0;
}

int oggLoader::get_sample_rate()
{
    if (m_vorbis_info)
        return m_vorbis_info->rate;
    else
        return 0;
}

int oggLoader::rewind()
{
    if (m_ogg_file != NULL)
    {
        ended = false;
        return ov_raw_seek_lap(&m_ogg_stream, 0);
    }

    return -1;
}

bool oggLoader::is_at_end()
{
    return ended;
}

int64_t oggLoader::get_pcm_pos()
{
   return ov_pcm_tell( &m_ogg_stream );
}

void oggLoader::set_pcm_pos( int64_t pos )
{
    int ret = ov_pcm_seek( &m_ogg_stream, pos );    
    if( ret != 0 )
    {
        LOGE << "Error seeking in ogg stream " << std::endl;
    }
}
