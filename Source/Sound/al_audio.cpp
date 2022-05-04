//-----------------------------------------------------------------------------
//           Name: al_audio.cpp
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
#include "al_audio.h"

#include <Sound/soundlogging.h>
#include <Sound/Loader/ogg_loader.h>

#include <Internal/error.h>
#include <Internal/timer.h>
#include <Internal/common.h>
#include <Internal/filesystem.h>

#include <Compat/fileio.h>
#include <Memory/allocation.h>
#include <Math/vec3math.h>
#include <Threading/sdl_wrapper.h>
#include <Utility/strings.h>

#ifdef WIN32
#define NOMINMAX
#include <windows.h>
#endif

#ifndef WIN32
#include <unistd.h>
#endif

#include <memory.h>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cmath>
#include <cerrno>


/*******************************************************************
   staticEmitter class - internal class for playing sounds.
*******************************************************************/
class StaticEmitter : public AudioEmitter {
public:    
    StaticEmitter(bool loop, const vec3 &position, float volume, float pitch_mul = 1.0f, uint8_t flags = 0, float _max_distance = 100.0f, unsigned char _priority = _default_priority) : 
        m_looping(loop),
        m_position(position),
        m_velocity(0.0f),
        m_volume(volume),
        m_pitch_mul(pitch_mul),
        m_volume_mult(1.0),
        m_max_distance(_max_distance),
        m_occlusion_position(position),
        m_priority(_priority)
    {flags_ = flags;}

    ~StaticEmitter() override
    {}

    bool KeepPlaying() override {
        return m_looping?false:true;
    }

    bool GetPosition(vec3 &p) override {
        p = m_position;
        return ((flags_ & SoundFlags::kRelative) != 0);
    }

    const vec3 GetPosition() override {
        return m_position;
    }

    const vec3 GetOcclusionPosition() override {
        return m_occlusion_position;
    }

    float GetMaxDistance() override {
        return m_max_distance;
    }

    void GetDirection(vec3 &p) override    {
        p.x() = 0.0f; p.y() = 0.0f; p.z() = 0.0f;
        return;
    }

    const vec3& GetVelocity() override {
        return m_velocity;
    }

    float GetVolume() override {
        return m_volume * m_volume_mult;
    }

    float GetPitchMultiplier() override {
        return m_pitch_mul;
    }

    void SetPitchMultiplier(float mul) override {
        m_pitch_mul = mul;
    }

    void SetVolume(float mul) override {
        if( mul > 1.0f ) 
            mul = 1.0f;
        if( mul < 0.0f )
            mul = 0.0f;

        m_volume = mul;
    }

    void SetVolumeMult(float mul) override {
        m_volume_mult = mul;
    }

    void SetPosition(const vec3 &pos) override {
        m_position = pos;
    }

    void SetOcclusionPosition(const vec3 &pos) override {
        m_occlusion_position = pos;
    }

    void SetVelocity(const vec3 &vel) override {
        m_velocity = vel;
    }

    void Unsubscribe() override {
        // should call the audioEmitter destructor, all should be cleaned.
        delete this;
    }

    unsigned char GetPriority() override {
        return m_priority;
    }

private:
    bool m_looping;
    vec3 m_position;
    vec3 m_velocity;
    float m_volume;
    float m_pitch_mul;
    float m_volume_mult;
    float m_max_distance;
    vec3 m_occlusion_position;
    unsigned char m_priority;
};

void AudioEmitter::link(alAudio &a)
{
    m_audio = &a;
}

void AudioEmitter::Unsubscribe()
{
    if (m_audio)
    {
        m_audio->unsubscribe(*this);
        m_audio = NULL;
    }
}

AudioEmitter::~AudioEmitter()
{
    if (m_audio)
    {
        m_audio->unsubscribe(*this);
        m_audio = NULL;
    }
}

uint8_t AudioEmitter::flags() {
    return flags_;
}

int AudioEmitter::uid_counter = 1;


/*******************************************************************
   audioStreamer class - base interface for stream subscribers
*******************************************************************/
audioStreamer::audioStreamer()
{
}

audioStreamer::~audioStreamer()
{
}

alAudio::alAudio(const char* preferred_device, float volume, float reference_distance) :
    m_device(NULL),
    m_context(NULL),
    priority_levels(0),
    m_handle_ctr(0),
    m_reference_distance(reference_distance),
    m_gain(volume),
    m_pitch(1.0f)
{
    //Sending NULL to alcOpenDevice will result in fallback to default.
    const char* open_device_name = NULL;

    if( preferred_device == NULL ) {
        preferred_device = ""; 
    }

    this->preferred_device = std::string(preferred_device);

    if( alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT") == AL_TRUE ) {
        LOGI << "ALC_ENUMERATION_EXT is available" << std::endl;

        const char* device_list = alcGetString(NULL, ALC_DEVICE_SPECIFIER);

        size_t cur_d_start = 0;
        if( strlen(preferred_device) > 0 ) {
            LOGI << "Trying to open preferred device: " << preferred_device << std::endl;
        }

        LOGI << "=== Available sound devices ===" << std::endl;
        while( device_list ) {
            if( device_list[cur_d_start] == '\0' ) {
                device_list = NULL;
            } else {
                LOGI << &device_list[cur_d_start] << std::endl;
                available_device_list.push_back(std::string(&device_list[cur_d_start]));
                if(strmtch(preferred_device, &device_list[cur_d_start])) {
                    open_device_name = &device_list[cur_d_start];
                }
                cur_d_start += strlen(&device_list[cur_d_start]) + 1;
            }
        }
        LOGI << "===                         ===" << std::endl;

        if( open_device_name == NULL ) {
            open_device_name = alcGetString( NULL, ALC_DEFAULT_DEVICE_SPECIFIER );
            LOGI << "Opening default audio device: " << open_device_name << "." << std::endl;
        } else  {
            LOGI << "Opening preferred audio device: " << open_device_name << "." << std::endl;
        }

        used_device = std::string(open_device_name);
    } else {
        LOGI << "ALC_ENUMERATION_EXT is not available, will open default device." << std::endl;
    }
    
    m_device = alcOpenDevice( open_device_name );   

    if( m_device )
    {
        const char* actual_device = alcGetString(m_device,ALC_DEVICE_SPECIFIER);

        if( actual_device ) {
            LOGI << "Opened and using audio device: " << actual_device << std::endl;
        }
        
        m_context = alcCreateContext( m_device, NULL );

        if( m_context )
        {
            alcMakeContextCurrent( m_context );
            alGetError(); // Clear Error Code

            reset_listener();

            const unsigned long _ideal_max_sources = 32;

            for( unsigned i = 0; i < _ideal_max_sources; i++ )
            {
                rc_alAudioSource as;
                as->Allocate();

                if( as->IsValid() )
                {
                    m_sources.push_back(as);
                }
                else
                {
                    break;
                }
            }

            LOGI << "We have " << m_sources.size() << " audio sources available." << std::endl;
        }
        else
        {
            LOGF << "Failed to load audio context, this means no sound, and probably crashing." << alcErrString(alcGetError(m_device)) << std::endl;
        }
    }
    else
    {
        LOGF << "Failed to load audio device, this means no sound, and probably crashing." <<  std::endl;
    }
}

void alAudio::Dispose() {

    m_sources.clear();

    m_buffers.clear();

    // unsubscribe to all the audioEmitters,
    // this will also allow the staticEmitters that are still running to be deleted
    streamer_subscribers::iterator ssi;
    for (ssi = m_streamers.begin(); ssi != m_streamers.end(); ++ssi)
    {
        if (!ssi->second->get_discard())
            ssi->second->get_audioEmitter()->Unsubscribe();
    }

    for (ssi = m_streamers.begin(); ssi != m_streamers.end(); ++ssi)
    {
        delete ssi->second;
    }

    alcDestroyContext( m_context );

    ALCenum err = alcGetError( m_device );
    m_context = NULL;
    if( err == ALC_NO_ERROR )
    {
        if( alcCloseDevice( m_device ) == false )
        {
            LOGE << "Unable to shut down alc device: \"" << alcErrString(err) << "\"" <<  std::endl;
        }
        
        m_device = NULL;
    }
    else
    {
        LOGE << "Error shutting down alc context: \"" << alcErrString(err) << "\"" << std::endl;
    }
}

void alAudio::reset_listener()
{
    listener_pos = vec3(0.0f);
    ALfloat listenerPos[]={0.0f,0.0f,0.0f};
    ALfloat listenerVel[]={0.0f,0.0f,0.0f};
    ALfloat listenerOri[]={0.0f,0.0f,-1.0f,0.0f,1.0f,0.0f}; // at, then up
    // Set Listener attributes
    alListenerfv(AL_POSITION,listenerPos);
    alListenerfv(AL_VELOCITY,listenerVel);
    alListenerfv(AL_ORIENTATION,listenerOri);
}

void alAudio::set_listener_position(vec3 &position)
{
    listener_pos = position;
    alListener3f(AL_POSITION, position.x(), position.y(), position.z());
}

void alAudio::set_listener_orientation(vec3 &facing, vec3 &up)
{
    ALfloat listenerOri[6];
    listenerOri[0] = facing.x();
    listenerOri[1] = facing.y();
    listenerOri[2] = facing.z();
    listenerOri[3] = up.x();
    listenerOri[4] = up.y();
    listenerOri[5] = up.z();

    alListenerfv(AL_ORIENTATION, listenerOri);
}

void alAudio::set_listener_velocity(vec3 &velocity)
{
    alListener3f(AL_VELOCITY, velocity.x(), velocity.y(), velocity.z());
}

void alAudio::update(float timestep)
{

    update_subscribers(timestep);

    //Disabling because this system is now runnign in separate thread and needs to be fully contained.
    //TODO: This rendering has to be moved to an external site.
    /*
    if( config["visible_sound_spheres"].toBool() )
    {
        static_handles::iterator iter = m_handles.begin();
        for(; iter != m_handles.end(); ++iter){
            DebugDraw::Instance()->AddWireSphere(iter->second->GetPosition(), 
                iter->second->GetMaxDistance()*0.4f, vec4(1.0f), _delete_on_update);
        }
    }
    */

}

void alAudio::subscribe(audioStreamer &streamer)
{
    rc_alAudioSource source;

    if (m_sources.empty())
    {
        //printf("Implement sound prioritization");
        source = find_free_source();
    }
    else
    {
        source = m_sources.front();
        m_sources.pop_front();
    }
    
    if( source->IsValid() )
    {
        streamer.link(*this);

        // set current values in source
        source->Sourcef(AL_REFERENCE_DISTANCE, m_reference_distance);                
        source->Sourcef(AL_GAIN, m_gain * streamer.GetVolume());
        source->Sourcef(AL_PITCH, m_pitch * streamer.GetPitchMultiplier());
        source->Sourcef(AL_MIN_GAIN, 0.0f);
        source->Sourcef(AL_MAX_GAIN, 1.0f);

        streamerLink *link = new streamerLink(source, streamer);
        m_streamers.insert(streamer_subscribers::value_type(&streamer, link));    
    }
}

void alAudio::unsubscribe(AudioEmitter &streamer)
{    
    const unsigned long &handle = streamer.handle;
    m_handles.erase(handle);
    //printf("Erasing %u from handles\n", handle);

    streamer_subscribers::iterator it = m_streamers.find(&streamer);

    if (it != m_streamers.end())
    {
        it->second->signal_discard();
    }
}


void alAudio::update_subscribers(float timestep) {
    unsigned int current_tick = SDL_TS_GetTicks();
    streamer_subscribers::iterator it = m_streamers.begin();
    while(it != m_streamers.end())     {
        if (it->second->get_discard()){
            ++it;        
            continue;
        }

        // set current values in source
        it->second->get_source()->Sourcef(AL_REFERENCE_DISTANCE, m_reference_distance);                
        it->second->get_source()->Sourcef(AL_GAIN, m_gain * it->second->get_audioEmitter()->GetVolume());
        it->second->get_source()->Sourcef(AL_PITCH, m_pitch * it->second->get_audioEmitter()->GetPitchMultiplier());
        const vec3 &vel = it->second->get_audioEmitter()->GetVelocity();
        it->second->get_source()->Source3f(AL_VELOCITY, vel[0], vel[1], vel[2]);
        //alSourcef(it->second->get_source(), AL_MIN_GAIN, 0.0f);
        //alSourcef(it->second->get_source(), AL_MAX_GAIN, 1.0f);
    
        it->second->update(timestep,current_tick);
        
        ++it;        
    }

    // sweep and remove discarded members
    it = m_streamers.begin();
    while(it != m_streamers.end())
    {
        if (it->second->get_discard())
        {
            it->second->get_source()->Stop();
            m_sources.push_front(it->second->get_source());

            delete it->second;
            // verify
            m_streamers.erase(it++);
        }
        else
        {
            ++it;
        }
    }
}

bool alAudio::load(const std::string& rel_path, const FilterInfo &filter_info) {
    // prexisting file found, don't reload
    if (m_buffers.end() != m_buffers.find(std::pair<std::string, std::string>(rel_path, filter_info.path))) {
        return true;
    }

    if (rel_path[rel_path.length() - 1] == 'g' || rel_path[rel_path.length() - 1] == 'G')
        return load_ogg(rel_path, filter_info);
    else
        return load_wav(rel_path, filter_info);
}

bool alAudio::load_ogg(const std::string& rel_path, const FilterInfo &filter_info)
{
    bool retval = true;

    Path abs_path = FindFilePath( rel_path.c_str(), kAnyPath ); 

    oggLoader ol(abs_path);

    char *data = new char[ol.get_sample_count() * 2];

    int max_size = ol.get_sample_count() * 2;
    int size = 0;
    int result = 0;
    while(size < max_size)
    {
        result = ol.stream_buffer_int16(data + size, max_size);
        if (result <= 0) {
            delete [] data;
            return false;
        }

        size += result;
    }

    ALenum format;

    if(ol.get_channels() == 1)
        format = AL_FORMAT_MONO16;
    else
        format = AL_FORMAT_STEREO16;

    if (format != AL_FORMAT_MONO16)
    {
        LOGW << "Stereo format handed to a mono reader" << std::endl;
        retval = false;
    }

    if (retval)
    {
        rc_alAudioBuffer buffer;
        buffer->Allocate();

        // Copy ogg data into AL Buffer
        buffer->BufferData( format, data, ol.get_sample_count() * 2, ol.get_sample_rate());
        
        if (AL_NO_ERROR == alGetError())
        {
            m_buffers.insert(buffer_map::value_type(
                std::pair<std::string, std::string>(rel_path, filter_info.path),
                buffer));
        }
        else
        {
            LOGE << "Failed to buffer data" << std::endl;
            retval = false;
        }
    }

    delete [] data;

    return retval;
}

void LowPassFilter(ALenum format, ALvoid* data, ALsizei size) {
    unsigned bytes_per_channel_sample = 1;
    if(format == AL_FORMAT_MONO16 || format == AL_FORMAT_STEREO16){
        bytes_per_channel_sample = 2;
    }
    
    unsigned channels = 1;
    if(format == AL_FORMAT_STEREO8 || format == AL_FORMAT_STEREO16){
        channels = 2;
    }

    if(bytes_per_channel_sample == 2 && channels == 1){
        int16_t* data_dbyte = (int16_t*)data;
        for(int i=0; i<size/2-3; i++){
            data_dbyte[i] = data_dbyte[i]/4 + data_dbyte[i+1]/4 + data_dbyte[i+2]/4 + data_dbyte[i+3]/4;
        }
    }

    if(bytes_per_channel_sample == 2 && channels == 2){
        int16_t* data_dbyte = (int16_t*)data;
        for(int i=0; i<size/2-6; i++){
            data_dbyte[i] = data_dbyte[i]/4 + data_dbyte[i+2]/4 + data_dbyte[i+4]/4 + data_dbyte[i+6]/4;
        }
    }
}

bool ParseWAV(const char* file_data, ALenum &format, ALvoid* &data, ALsizei &size, ALsizei &freq)
{
    const char* data_index = &file_data[0];
    char xbuffer[5];
    xbuffer[4] = '\0';
    memcpy(xbuffer, data_index, 4);
    data_index += 4;
    if(strcmp(xbuffer, "RIFF") != 0){
        LOGE << "WAV file has invalid header" << std::endl;
        return false;
    }
    data_index += 4;
    memcpy(xbuffer, data_index, 4);
    data_index += 4;
    if(strcmp(xbuffer, "WAVE") != 0){
        LOGE << "WAV file has invalid header" << std::endl;
        return false;
    }
    bool found_format_chunk = false;
    do {
        memcpy(xbuffer, data_index, 4);
        data_index += 4;
        if(strcmp(xbuffer, "fmt ") == 0){
            found_format_chunk = true;
        } else {
            int32_t chunk_size = *((int32_t*)data_index);
            data_index += 4 + chunk_size;
        }
    } while(!found_format_chunk);
    
    data_index += 4;
    int16_t audioFormat = *((int16_t*)data_index);
    data_index += 2;
    int16_t channels = *((int16_t*)data_index);
    data_index += 2;
    int32_t sampleRate = *((int32_t*)data_index);
    data_index += 4;
    //int32_t byteRate = *((int32_t*)data_index);
    data_index += 6;
    int16_t bitsPerSample = *((int16_t*)data_index);
    data_index += 2;

    if (audioFormat != 1) {
        // Note: This early exit was added because the original code assumed there'd be a 16bit int here.
        // In the case of an example 32 bit float file, audioFormat == 3, this was a "fact" chunk instead,
        // and it would convert "fa" to an int16, and then skip 24+kb and hit an access violation
        LOGE << "WAV file has unsupported non-PCM format" << std::endl;
        return false;
    }

    bool found_data_chunk = false;
    do {
        memcpy(xbuffer, data_index, 4);
        data_index += 4;
        if(strcmp(xbuffer, "data") == 0){
            found_data_chunk = true;
        } else {
            int32_t chunk_size = *((int32_t*)data_index);
            data_index += 4 + chunk_size;
        }
    } while(!found_data_chunk);

    int32_t dataChunkSize = *((int32_t*)data_index);
    data_index += 4;
    
    if(channels == 1 && bitsPerSample == 8){
        format = AL_FORMAT_MONO8;
    } else if(channels == 2 && bitsPerSample == 8){
        format = AL_FORMAT_STEREO8;
    } else if(channels == 1 && bitsPerSample == 16){
        format = AL_FORMAT_MONO16;
    } else if(channels == 2 && bitsPerSample == 16){
        format = AL_FORMAT_STEREO16;
    } else {
        LOGE << "WAV file has unknown format" << std::endl;
        return false;
    }
    
    freq = sampleRate;
    data = OG_MALLOC(dataChunkSize);
    size = dataChunkSize;
    memcpy(data, data_index, dataChunkSize);

    return true;
} 

bool GenerateEmptyData(ALenum &format, ALvoid* &data, ALsizei &size, ALsizei &freq) {
    freq = 44100;  
    format = AL_FORMAT_MONO8; 
    size = freq*5;
    data = OG_MALLOC(size); 
    memset(data, 0, size);
    return true;
}

bool LoadWAVFromFile(const char* file_path, ALenum &format, ALvoid* &data, ALsizei &size, ALsizei &freq) {
    FILE* file = my_fopen(file_path, "rb");
    if(!file){
        LOGE << "Could not open WAV file (fopen): " << file_path << " " << strerror(errno) << std::endl;
        return false;
    }

    fseek (file, 0, SEEK_END);
    int file_size = ftell(file);
    rewind (file);

    void* mem = OG_MALLOC(file_size);
    if(!mem){
        LOGE << "Could not allocate memory for file: " << file_path << std::endl;
        fclose(file);
        return false;
    }
    fread(mem, 1, file_size, file);
    fclose(file);
    
    bool ret = ParseWAV((char*)mem, format, data, size, freq);
    
    OG_FREE(mem);
    return ret;
}

bool alAudio::load_wav(const std::string& rel_path, const FilterInfo &filter_info) {
    bool retval = true;

    ALint    error;
    ALsizei size,freq;
    ALenum    format;
    ALvoid    *data = NULL;

    char abs_path[kPathSize];
    if(FindFilePath(rel_path.c_str(), abs_path, kPathSize, kModPaths|kDataPaths) == -1){
        LOGE << "Could not find sound file: " <<  rel_path.c_str() << ". Generating empty placeholder." << std::endl;
        GenerateEmptyData(format,data,size,freq);
    } else {
        if( LoadWAVFromFile(abs_path,format,data,size,freq) == false ) {
            LOGE << "Failed loading data, generating empty placeholder for " << rel_path.c_str() << std::endl; 
            GenerateEmptyData(format,data,size,freq);
        }
    }

    if (data == NULL) {
        retval = false;
    } else {
        rc_alAudioBuffer buffer;
        buffer->Allocate();

        //LowPassFilter(format, data, size);

        AudioBufferData abd;
        abd.bytes_per_channel_sample = 1;
        if(format == AL_FORMAT_MONO16 || format == AL_FORMAT_STEREO16){
            abd.bytes_per_channel_sample = 2;
        }
        abd.channels = 1;
        if(format == AL_FORMAT_STEREO8 || format == AL_FORMAT_STEREO16){
            abd.channels = 2;
        }
        abd.num_bytes = size;
        abd.data = (void*)data;

        if(!filter_info.path.empty()){
            if(filter_info.type == _simple_filter) {
                SimpleFIRFilter filter;
                if( filter.Load(filter_info.path) ) {
                    filter.Apply(abd);
                } else {
                    LOGE << "Failed loading fir filter: " << filter_info.path << " will not apply to sound of " << rel_path << std::endl;
                }
                buffer->BufferData(format,data,size,freq);
            } else if(filter_info.type == _convolution_filter) {
                LOGE << "Game does not currently support _convolution_filter for audio, and the developer never expected it to be used, it was disabled because underlying library was removed after being deemed not in use." << std::endl;

                buffer->BufferData(format,data,size,freq);
                /*
                FFTConvolutionFilter filter;
                std::vector<int16_t> filtered;
                if( filter.Load(filter_info.path) ) {
                    filter.Apply(abd, &filtered);
                } else { 
                    LOGE << "Failed loading FFTConvolution filter: " << filter_info.path << " will not apply to sound of " << rel_path << std::endl;
                }
                if(!filtered.empty()){
                    buffer->BufferData(format,&filtered[0],filtered.size()*2,freq);
                } else {
                    buffer->BufferData(format,data,size,freq);
                }
                */
            }
        } else {
            buffer->BufferData(format,data,size,freq);
        }
        
        if ((error = alGetError()) == AL_NO_ERROR) {
            m_buffers.insert(buffer_map::value_type(
                std::pair<std::string, std::string>(rel_path, filter_info.path),
                buffer));
        } else {
            LOGE << "Failed to buffer data: \"" << alErrString(error) << "\"" <<  std::endl;
            retval = false;
        }        
    }

    OG_FREE(data);

    return retval;
}

void alAudio::set_volume(float volume)
{
    m_gain = volume;

    streamer_subscribers::iterator it = m_streamers.begin();
    while(it != m_streamers.end())
        {
            if (!it->second->get_discard())
            {
                it->second->get_source()->Sourcef(AL_GAIN, m_gain * it->second->get_audioEmitter()->GetVolume());                
            }        
            ++it;
        }
}

void alAudio::set_master_volume( float volume )
{
    if( volume > 1.0f )
        volume = 1.0f;
    if( volume < 0.0f )
        volume = 0.0f;

    alListenerf(AL_GAIN, volume);
}

void alAudio::set_pitch(float pitch)
{
    m_pitch = pitch;

    streamer_subscribers::iterator it = m_streamers.begin();
    while(it != m_streamers.end())
        {
            if (!it->second->get_discard())
            {
                it->second->get_source()->Sourcef(AL_PITCH, m_pitch * it->second->get_audioEmitter()->GetPitchMultiplier());
            }        
            ++it;
        }
}

void alAudio::set_reference_distance(float distance)
{
    m_reference_distance = distance;

    streamer_subscribers::iterator it = m_streamers.begin();
    while(it != m_streamers.end())
        {
            if (!it->second->get_discard())
            {
                it->second->get_source()->Sourcef( AL_REFERENCE_DISTANCE, m_reference_distance);
            }        
            ++it;
        }
}

void alAudio::set_distance_model(ALenum model)
{
    alDistanceModel(model);
}

void alAudio::play(const unsigned long& handle, const SoundPlayInfo& spi, AudioEmitter *owner )
{
    set_sound(handle,spi, owner);
}

std::vector<AudioEmitter*> alAudio::GetActiveSounds() {
    std::vector<AudioEmitter*> active_sounds(m_handles.size());
    alAudio::static_handles::iterator iter;
    unsigned i=0;
    for(iter = m_handles.begin(); iter != m_handles.end(); ++iter){
        active_sounds[i] = iter->second;
        ++i;
    }
    return active_sounds;
}

float GetRolloffFromMaxDistance(float max_distance){
    static const float inv_threshold_vol = 100.0f;
    return (inv_threshold_vol - 1.0f) / (1+max_distance);
}

int alAudio::set_sound(const unsigned long& handle, const SoundPlayInfo& spi, AudioEmitter *owner)
{
    if(!spi.looping && !(spi.flags & SoundFlags::kRelative) && distance_squared(spi.position, listener_pos) > 
        spi.max_distance * spi.max_distance)
    {
        return -1;
    }
    const char *rel_path = spi.path.c_str();

    buffer_map::iterator it = m_buffers.find(
        std::pair<std::string, std::string>(rel_path, spi.filter_info.path));

    // if buffer isn't found, need to load.
    if (m_buffers.end() == it) {
        size_t str_size = strlen(rel_path);
        char *buffer = new char [str_size + 1];
        ::memset(buffer, 0, str_size + 1);
        ::memcpy(buffer, rel_path, str_size);

        if (load(buffer, spi.filter_info)) {
            it = m_buffers.find(
                std::pair<std::string, std::string>(
                rel_path, spi.filter_info.path)); // this can be made faster.
        } else {
            it = m_buffers.end();
        }

        ALenum err = alGetError();

        if( err != AL_NO_ERROR )
        {
            LOGE << alErrString(err)  << std::endl;
        }

        delete [] buffer;
    }

    if (m_buffers.end() != it) {
        //need to manage this better than dropping out.
        rc_alAudioSource source;

        if (m_sources.empty())
        {
            //printf("Implement sound prioritization");
            source = find_free_source();
        }
        else
        {
            source = m_sources.front();
            m_sources.pop_front();
        }

        if( source->IsValid() )
        {
            priority_levels = max(priority_levels, spi.priority);
                    
            AudioEmitter *emitter = NULL;
            if (owner == NULL)
            {
                //printf("Creating %u %s\n",handle, spi.path.c_str()); 
                // this is not a leak due to the unsubscribe deleting the object.
                //staticEmitter *emitter = new staticEmitter(looping, position);            
                emitter = new StaticEmitter(spi.looping, spi.position, spi.volume, spi.pitch, spi.flags, spi.max_distance, spi.priority);
                emitter->handle = handle; 
                m_handles.insert(static_handles::value_type(handle, emitter));
            }
            else
            {
                emitter = owner;
            }
            CheckALError(__LINE__, __FILE__);
            emitter->display_name = spi.path.substr(spi.path.rfind('/')+1);
            emitter->SetOcclusionPosition(spi.occlusion_position);
            emitter->SetVolumeMult(spi.volume_mult);

            // set current values in source
            source->Sourcef(AL_REFERENCE_DISTANCE, m_reference_distance); 
            //alSourcef(source, AL_ROLLOFF_FACTOR, GetRolloffFromMaxDistance(spi.max_distance));                
            source->Sourcef(AL_GAIN, m_gain * emitter->GetVolume());
            source->Sourcef(AL_PITCH, m_pitch * emitter->GetPitchMultiplier());
            source->Sourcef(AL_MIN_GAIN, 0.0f);
            source->Sourcef(AL_MAX_GAIN, spi.max_gain);
            CheckALError(__LINE__, __FILE__);

            vec3 p;        
            source->Sourcei (AL_SOURCE_RELATIVE, emitter->GetPosition(p)?AL_TRUE:AL_FALSE);
            source->Source3f(AL_POSITION, p.x(), p.y(), p.z());
            emitter->GetDirection(p);
            source->Source3f(AL_DIRECTION, p.x(), p.y(), p.z());
            p = emitter->GetVelocity();
            source->Source3f(AL_VELOCITY, p.x(), p.y(), p.z());

            emitter->link(*this);
            staticLink *link = new staticLink(source, *emitter, it->second, spi);
            m_streamers.insert(streamer_subscribers::value_type(emitter, link));

            CheckALError(__LINE__, __FILE__);
        }
    }    
    else
    {
        LOGE << "Unable to load sound file: " << rel_path << std::endl;
    }

    return handle;
}

rc_alAudioSource alAudio::find_free_source()
{    
    int i = priority_levels;
    do {
        streamer_subscribers::iterator it = m_streamers.begin();
        while(it != m_streamers.end())
        {
            if (it->second->get_discard() || (it->second->get_audioEmitter()->GetPriority() == i))
            {
                rc_alAudioSource source = it->second->get_source();
                source->Stop();

                // nuke handles.
                static_handles::iterator it_handle = m_handles.begin();
                while(it_handle != m_handles.end())
                {
                    if (it->second->get_audioEmitter() == it_handle->second) {
                        /*it_handle = */m_handles.erase(it_handle++);
                    } else {
                         ++it_handle;
                    }
                }

                delete it->second;
                m_streamers.erase(it++);

                return source;
            }
            ++it;
        }
        --i;
    } while (i >= 0);

    return rc_alAudioSource();
}

std::string GetALErrorString(ALenum err)
{
    switch(err)
    {
    case AL_NO_ERROR:
        return std::string("AL_NO_ERROR");
        break;

    case AL_INVALID_NAME:
        return std::string("AL_INVALID_NAME");
        break;

    case AL_INVALID_ENUM:
        return std::string("AL_INVALID_ENUM");
        break;

    case AL_INVALID_VALUE:
        return std::string("AL_INVALID_VALUE");
        break;

    case AL_INVALID_OPERATION:
        return std::string("AL_INVALID_OPERATION");
        break;

    case AL_OUT_OF_MEMORY:
        return std::string("AL_OUT_OF_MEMORY");
        break;
    };

    return std::string("Unknown OpenAL error");
}

void CheckALError(int line, const char* file){
    ALenum err;
    err = alGetError();
    if (err != AL_NO_ERROR) {
        char error_msg[1024];
        int i = 0;
        int last_slash = 0;
        while(file[i] != '\0') {
            if(file[i] == '\\' || file[i] == '/') last_slash = i+1;
            i++;
        }
        FormatString(error_msg, 1024, "On line %d of %s: \n%s", line, &file[last_slash], GetALErrorString(err).c_str());
        DisplayError("OpenAL error", error_msg);
    }
}

alAudio::streamerLink::streamerLink(rc_alAudioSource source, audioStreamer &streamer) :
    basicLink(source), m_streamer(&streamer)
{
    unsigned long i = 0;
    for (i = 0; i < streamer.required_buffers(); ++i)
    {
        rc_alAudioBuffer buf;
        buf->Allocate();
        m_buffers.push_back(buf);
    }

    for (i = 0; i < m_buffers.size(); ++i)
    {
        update(m_buffers[i]);
    }
      
    
    CheckALError(__LINE__, __FILE__);
    m_source->QueueBuffers( std::vector<rc_alAudioBuffer>(m_buffers.begin(),m_buffers.begin() + streamer.required_buffers()));
    CheckALError(__LINE__, __FILE__);
    m_source->Play();
    CheckALError(__LINE__, __FILE__);
}

alAudio::streamerLink::~streamerLink()
{
    // may not be a safe assumption in the future
    m_source->Stop();
}

void alAudio::streamerLink::update(rc_alAudioBuffer buffer)
{
    m_streamer->update(buffer);    
    update_position();
}

void alAudio::streamerLink::update(float timestep, unsigned int current_tick)
{
    update_position();

    std::vector<rc_alAudioBuffer> buffers = m_source->DequeueBuffers();

    for( unsigned i = 0; i < buffers.size(); i++ )
    {
        m_streamer->update(buffers[i]);
    }

    m_source->QueueBuffers(buffers);

    // check if playback stopped due to buffer timeout...
    ALint state = m_source->GetSourcei(AL_SOURCE_STATE);

    if (AL_STOPPED == state)
    {
        if (m_streamer->KeepPlaying())
        {
            m_source->Play();
        }
        else
        {
            m_streamer->Unsubscribe();
        }
    }
}

void alAudio::streamerLink::update_position()
{
    vec3 p;

    m_source->Sourcei (AL_SOURCE_RELATIVE, m_streamer->GetPosition(p)?AL_TRUE:AL_FALSE);
    m_source->Source3f(AL_POSITION, p.x(), p.y(), p.z());

    m_streamer->GetDirection(p);
    m_source->Source3f(AL_DIRECTION, p.x(), p.y(), p.z());
    p = m_streamer->GetVelocity();
    m_source->Source3f(AL_VELOCITY, p.x(), p.y(), p.z());
}

void alAudio::streamerLink::stop()
{
    m_streamer->Stop();
    m_source->Stop();

    std::vector<rc_alAudioBuffer> buffers = m_source->DequeueBuffers();

    for( unsigned i = 0; i < buffers.size(); i++ )
    {
        m_streamer->update(buffers[i]);
    }

    m_source->QueueBuffers(buffers);

    m_source->Play();
}


alAudio::staticLink::staticLink( rc_alAudioSource source, AudioEmitter &emitter, rc_alAudioBuffer buffer, const SoundPlayInfo &_spi) : 
    basicLink(source), 
    m_buffer(buffer),
    m_emitter(&emitter),
    played(false),
    play_on_tick(_spi.play_past_tick) 
{
    spi = _spi;
    source->SetBuffer( m_buffer);
    source->Sourcei(AL_LOOPING, spi.looping?AL_TRUE:AL_FALSE);
    m_source->Stop();
}

alAudio::staticLink::~staticLink() {
}

void alAudio::staticLink::update(float timestep, unsigned int current_tick) {    
    update_position();

    vec3 p;
    m_emitter->GetPosition(p);

    /*
    TODO: Move to external routine, calling it from here would break the thread boundery
    if( config["visible_sound_spheres"].toBool() )
    {
        DebugDraw::Instance()->AddLine(p, p+vec3(0.0f,1.0f,0.0f), 0.1f, vec4(1.0f), _delete_on_draw);
    }
    */

    if( current_tick >= play_on_tick && played == false) {
        m_source->Play();
        played = true;
    }

    ALint state = m_source->GetSourcei(AL_SOURCE_STATE);
    if (played == true && AL_STOPPED == state )
    {
        m_emitter->Unsubscribe();
    }    
    // Do not put any code after this spot, unsubscribe WILL destroy this object.
}

void alAudio::staticLink::update_position() {
    vec3 p;

    m_source->Sourcei (AL_SOURCE_RELATIVE, m_emitter->GetPosition(p)?AL_TRUE:AL_FALSE);
    m_source->Source3f(AL_POSITION, p.x(), p.y(), p.z());

    m_emitter->GetDirection(p);
    m_source->Source3f(AL_DIRECTION, p.x(), p.y(), p.z());
    p = m_emitter->GetVelocity();
    m_source->Source3f(AL_VELOCITY, p.x(), p.y(), p.z());
}

void alAudio::staticLink::stop() {
    m_emitter->Unsubscribe();
    // Do not put any code after this spot, unsubscribe WILL destroy this object.
}

unsigned long alAudio::get_next_handle() {
    return ++m_handle_ctr;
}

unsigned long alAudio::get_invalid_handle() {
    return 0;
}

std::string alAudio::GetPreferredDevice() {
    return preferred_device;
}

std::string alAudio::GetUsedDevice() {
    return used_device;
}

std::vector<std::string> alAudio::GetAvailableDevices() {
    return available_device_list;
}

void alAudio::change_pitch(unsigned long handle, float pitch) {
    static_handles::iterator it = m_handles.find(handle);

    if (it != m_handles.end()) {
        it->second->SetPitchMultiplier(pitch);
    }
}

void alAudio::change_volume(unsigned long handle, float volume) {
    static_handles::iterator it = m_handles.find(handle);

    if (it != m_handles.end()) {
        it->second->SetVolume(volume);
    }
}

void alAudio::SetPosition(const unsigned long &handle, const vec3 &new_pos) {
    static_handles::iterator it = m_handles.find(handle);

    if (it != m_handles.end()) {
        it->second->SetPosition(new_pos);
    }
}

void alAudio::TranslatePosition(const unsigned long &handle, const vec3 &new_pos) {
    static_handles::iterator it = m_handles.find(handle);

    if (it != m_handles.end()) {
        it->second->SetPosition(it->second->GetPosition()+new_pos);
    }
}

void alAudio::SetOcclusionPosition(const unsigned long &handle, const vec3 &new_pos) {
    static_handles::iterator it = m_handles.find(handle);

    if (it != m_handles.end()) {
        it->second->SetOcclusionPosition(new_pos);
    }
}

void alAudio::SetVelocity(const unsigned long &handle, const vec3 &new_vel) {
    static_handles::iterator it = m_handles.find(handle);

    if (it != m_handles.end()) {
        it->second->SetVelocity(new_vel);
    }
}

bool alAudio::IsHandleValid(const unsigned long &handle) {
    static_handles::iterator it = m_handles.find(handle);
    if (it != m_handles.end()) {
        return true;
    }
    return false;
}

const vec3 alAudio::GetPosition( const unsigned long &handle ) {
    static_handles::iterator it = m_handles.find(handle);
    if (it != m_handles.end()) {
        return(it->second->GetPosition());
    }
    return vec3(0.0f);
}

void alAudio::SetPitch( const unsigned long &handle, float pitch ) {
    static_handles::iterator it = m_handles.find(handle);
    if (it != m_handles.end()) {
        it->second->SetPitchMultiplier(pitch);
    }
}

void alAudio::SetVolume( const unsigned long &handle, float volume ) {
    static_handles::iterator it = m_handles.find(handle);
    if (it != m_handles.end()) {
        it->second->SetVolume(volume);
    }
}

void alAudio::Stop( const unsigned long & handle ) {
    static_handles::iterator it = m_handles.find(handle);
    if (it != m_handles.end()) {
        //printf("Stopping %u\n",handle);
        it->second->Unsubscribe();
    }
}

void alAudio::StopAll() {
    streamer_subscribers::iterator it = m_streamers.begin();
    while(it != m_streamers.end())     {
        if (it->second->get_discard()){
            ++it;
            continue;
        }

        it->second->stop();

        ++it;
    }
}

void alAudio::StopAllTransient()
{
    streamer_subscribers::iterator it = m_streamers.begin();
    while(it != m_streamers.end())     {
        if (it->second->get_discard()){
            ++it;
            continue;
        }

        if( it->second->is_transient() )
        {
            it->second->stop();
        }

        ++it;
    }
}

bool SimpleFIRFilter::Load( const std::string &path )
{
    ALsizei size,freq;
    ALenum    format;
    ALvoid    *data = NULL;

    char abs_path[kPathSize];
    FindFilePath(path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths);

    if( LoadWAVFromFile(abs_path, format, data, size, freq) ) {
        if( data ) {
            unsigned bytes_per_channel_sample = 1;
            if(format == AL_FORMAT_MONO16 || format == AL_FORMAT_STEREO16){
                bytes_per_channel_sample = 2;
            }

            unsigned channels = 1;
            if(format == AL_FORMAT_STEREO8 || format == AL_FORMAT_STEREO16){
                channels = 2;
            }

            if(bytes_per_channel_sample == 2 && channels == 1){
                int16_t* data_dbyte = (int16_t*)data;
                filter.resize(size/2);
                for(int i=0; i<size/2; i++){
                    filter[i] = data_dbyte[i] / 32767.0f;
                }
            } else {
                OG_FREE(data);
                data = NULL;
                LOGE << "Filter is not 16-bit mono: " <<  path.c_str() << std::endl;
                return false;
            }
        } else {
            LOGE << "Unable to load fir filter from " << path << std::endl;
            return false;
        }
    } else {
        LOGE << "Unable to load wav file for FIR filter" << std::endl;
        return false;
    }
    
    OG_FREE( data );
    return true;
}

void SimpleFIRFilter::Apply( AudioBufferData &abd, std::vector<int16_t> *output_new)
{
    unsigned filter_mid = unsigned(((float)filter.size())*0.5f);
    unsigned filter_size = filter.size();
    unsigned filter_tip = filter_size - filter_mid;

    if(abd.bytes_per_channel_sample == 2){
        for(unsigned k=0; k<abd.channels; ++k){
            int16_t* data_dbyte = (int16_t*)abd.data;
            unsigned end = abd.num_bytes/2/abd.channels;

            std::vector<int16_t> temp(filter_size);
            for(unsigned i=0; i<filter_mid; ++i){
                temp[i] = 0;
            }
            unsigned next_step = min(filter_size,end+filter_mid);
            for(unsigned i=filter_mid; i<next_step; ++i){
                temp[i] = data_dbyte[(i-filter_mid)*abd.channels+k];
            }
            for(unsigned i=end+filter_mid; i<filter_size; ++i){
                temp[i] = 0;
            }

            unsigned offset = 0;
            float total;
            for(unsigned i=0; i<end; i++){
                total = 0.0f;
                for(unsigned j=0; j<filter.size(); ++j){
                    total += (float)temp[(j+offset)%filter_size] * filter[j];
                }
                data_dbyte[i*abd.channels+k] = (int16_t)total;
                if(i+filter_tip < end){
                    temp[offset] = data_dbyte[(i+filter_tip)*abd.channels+k];
                } else {
                    temp[offset] = 0;
                }
                ++offset;
                offset = offset%filter_size;
            }
        }
    }
}

/*
bool FFTConvolutionFilter::Load( const std::string &path )
{
    ALsizei size,freq;
    ALenum    format;
    ALvoid    *data = NULL;

    char abs_path[kPathSize];
    FindFilePath(path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths);

    if( LoadWAVFromFile(abs_path, format, data, size, freq) ) {
        if( data )
        {
            unsigned bytes_per_channel_sample = 1;
            if(format == AL_FORMAT_MONO16 || format == AL_FORMAT_STEREO16){
                bytes_per_channel_sample = 2;
            }

            unsigned channels = 1;
            if(format == AL_FORMAT_STEREO8 || format == AL_FORMAT_STEREO16){
                channels = 2;
            }

            if(bytes_per_channel_sample == 2 && channels == 1){
                int16_t* data_dbyte = (int16_t*)data;
                filter.resize(size/2);
                for(int i=0; i<size/2; i++){
                    filter[i] = data_dbyte[i];// / 32767.0f;
                }
            } else {
                LOGE << "Filter is not 16-bit mono: " << path.c_str() << std::endl;
                OG_FREE(data);
                data = NULL;
                return false;
            }
        } else {
            LOGE << "Unable to load convolution filter from " << path << std::endl;
            return false;
        }
    } else { 
        LOGE << "Unable to load wav file for FFTConvolution filter" << std::endl;
        return false;
    }

    OG_FREE(data);
    return true;
}

void FFTConvolutionFilter::Apply( AudioBufferData &abd, std::vector<int16_t> *output_new)
{
    if(abd.channels > 1){
        return;
    }
    int16_t* samples = (int16_t*)abd.data;
    int length1 = abd.num_bytes/abd.bytes_per_channel_sample;
    int length2 = filter.size();

    float gain = 0.2f;

    int i;
    float *in;
    float *in2;
    int n;
    int nc;
    fftwf_complex *out1;
    fftwf_complex *out2;
    fftwf_plan plan_backward;
    fftwf_plan plan_forward;

    int long_length = length1+length2-1;

    n = long_length;
    nc = ( n / 2 ) + 1;
    in = (float*)fftwf_malloc ( sizeof ( float ) * n );
    for ( i = 0; i < length1; i++ )
    {
        in[i] = samples[i];
    }
    for ( i = length1; i < n; i++ )
    {
        in[i] = 0;
    }
    out1 = (fftwf_complex*)fftwf_malloc ( sizeof ( fftwf_complex ) * nc );
    plan_forward = fftwf_plan_dft_r2c_1d ( n, in, out1, FFTW_ESTIMATE );
    fftwf_execute ( plan_forward );
    fftwf_destroy_plan ( plan_forward );
    fftwf_free ( in );

    in = (float*)fftwf_malloc ( sizeof ( float ) * n );
    for ( i = 0; i < length2; i++ )
    {
        in[i] = filter[i];
    }
    for ( i = length2; i < n; i++ )
    {
        in[i] = 0;
    }
    out2 = (fftwf_complex*)fftwf_malloc ( sizeof ( fftwf_complex ) * nc );
    plan_forward = fftwf_plan_dft_r2c_1d ( n, in, out2, FFTW_ESTIMATE );
    fftwf_execute ( plan_forward );
    fftwf_destroy_plan ( plan_forward );
    fftwf_free ( in );

    float new_r;
    float new_i;
    float scale = 1/(float)n*gain;
    for (i = 0;i<nc;i++){
        new_r = (out1[i][0]*out2[i][0] - out1[i][1]*out2[i][1])*scale;
        new_i = (out1[i][1]*out2[i][0] + out1[i][0]*out2[i][1])*scale;

        out1[i][0] = new_r;
        out1[i][1] = new_i;
    }

    
    //Set up an array to hold the backtransformed data IN2,
    //get a "plan", and execute the plan to backtransform the OUT
    //FFT coefficients to IN2.
    

    in2 = (float*)fftwf_malloc ( sizeof ( float ) * n );
    plan_backward = fftwf_plan_dft_c2r_1d ( n, out1, in2, FFTW_ESTIMATE );
    fftwf_execute ( plan_backward );

    for (i = 0; i < length1; i++ )
    {
        samples[i] = (int16_t)(in2[i]/n);
    }

    if(output_new){
        std::vector<int16_t> &output = (*output_new);
        output.resize(long_length);
        for (i = 0; i < long_length; i++ )
        {
            output[i] = (int16_t)(in2[i]/n);
        }
    }

    fftwf_destroy_plan ( plan_backward );
    fftwf_free ( in2 );
    fftwf_free ( out1 );
    fftwf_free ( out2 );

    return;
}
*/

void alAudio::SetVolumeMult( const unsigned long &handle, float volume )
{
    static_handles::iterator it = m_handles.find(handle);
    if (it != m_handles.end())
    {
        it->second->SetVolumeMult(volume);
    }
}
