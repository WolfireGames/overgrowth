//-----------------------------------------------------------------------------
//           Name: al_audio.h
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

#ifdef WIN32
#pragma warning(disable:4786)
#endif 

#include <Sound/soundplayinfo.h>
#include <Sound/al_audio_buffer.h>
#include <Sound/al_audio_source.h>

#include <Internal/integer.h>
#include <Internal/referencecounter.h>

#include <Wrappers/openal.h>
#include <Math/enginemath.h>

#include <map>
#include <string>
#include <vector>
#include <deque>
#include <list>
#include <algorithm>
#include <string>

namespace SoundFlags {
    enum {
        kNoOcclusion = 1<<0,
        kRelative = 1<<1
    };
}

struct AudioBufferData {
    unsigned channels;
    unsigned bytes_per_channel_sample;
    void *data;
    unsigned num_bytes;
};

class AudioFilter {
public:
    virtual bool Load(const std::string &path)=0;
    virtual void Apply( AudioBufferData &abd, std::vector<int16_t> *output_new = NULL)=0;
    virtual ~AudioFilter() {}
};

class SimpleFIRFilter : public AudioFilter {
    std::vector<float> filter;
public:
    bool Load(const std::string &path) override;
    void Apply( AudioBufferData &abd, std::vector<int16_t> *output_new = NULL) override;
};

/*
class FFTConvolutionFilter : public AudioFilter {
    std::vector<float> filter;
public:
    bool Load(const std::string &path);
    void Apply( AudioBufferData &abd, std::vector<int16_t> *output_new = NULL);
};
*/

class alAudio;

/**
* Base class for all subscribers
*/
class AudioEmitter
{
public:
    //Unique id for this emitter instance, used to reference between undefined slices of time.
    int uid;

    uint8_t flags_;
    unsigned long handle;
    std::string display_name;

    AudioEmitter() : uid(++uid_counter), m_audio(NULL), handle(0) {}
    virtual ~AudioEmitter();

    /// Set to false to allow the emitter to time out (useful only on non-looping sounds)
    virtual bool KeepPlaying() = 0;
    /// if true is returned, this will be a relative-to-listener sound
    virtual bool GetPosition(vec3 &p) = 0;
    virtual void GetDirection(vec3 &p) = 0;
    virtual const vec3& GetVelocity() = 0;
    virtual const vec3 GetPosition() = 0;
    virtual const vec3 GetOcclusionPosition() = 0;
    virtual float GetMaxDistance() {return 0.0f;}
    uint8_t flags();

    /// allows you to change the global alAudio pitch multiplier for this sound
    virtual float GetPitchMultiplier() {return 1.0f;}

    /// allows you to change the global alAudio gain multiplier for this sound
    virtual float GetVolume() {return 1.0f;}

    /// For use by alAudio to build subscriber chains
    void link(alAudio &a);
    /// For use by alAudio to eliminate this object from the subscriber chain
    virtual void Unsubscribe();

    /// Indicate the priority of this effect to make room for newer/higher priority effects
    virtual unsigned char GetPriority() = 0;

    virtual void SetPitchMultiplier(float mul) {;}
    virtual void SetVolume(float mul) {;}
    virtual void SetVolumeMult(float mul) {;}
    virtual void SetVelocity(const vec3 &vel) {;}
    virtual void SetPosition(const vec3 &pos) {;}
    virtual void SetOcclusionPosition(const vec3 &pos) {;}
    virtual void SetFlags(uint8_t flags) {flags_ = flags;}
    //Am i a transient sound? That means, short lived and shouldn't remain between major state changes.
    virtual bool IsTransient() { return true; }

protected:
    alAudio *m_audio;
    static int uid_counter;
};




/**
* Subscriber specifically for streaming audio (oggSoundtracks, network voice, etc)
*/
class audioStreamer : public AudioEmitter
{
public:
    audioStreamer();
    ~audioStreamer() override;    

    /// Request to fill a buffer with data
    virtual void update(rc_alAudioBuffer buffer) = 0;

    /// Return the number of buffers this streaming class requires (usually 2)
    virtual unsigned long required_buffers() = 0;

	/// Stop the current stream (reset to empty)
	virtual void Stop() = 0;
};


/**
* Interface to OpenAL
*/
class alAudio
{
public:
    alAudio(const char* preferred_device, float volume, float reference_distance = 2000.0f);
    
    /// Add a streaming subscriber to the alAudio subscriber chain
    void subscribe(audioStreamer &streamer);

    /// Remove any subscriber from the alAudio subscriber chain
    void unsubscribe(AudioEmitter &streamer);

    void set_listener_position(vec3 &position);
    void set_listener_orientation(vec3 &facing, vec3 &up);
    void set_listener_velocity(vec3 &velocity);

    /// Preload a sound into the buffer. Accepts OGG or WAV.
    bool load(const std::string& file, const FilterInfo &filter_info);

    void play(const unsigned long& handle, const SoundPlayInfo& spi, AudioEmitter *owner = NULL);

    /// This must be called frequently, it is responsible for updating every source in the link chain - optimally called once a frame.
    void update(float timestep);

    /// Set global volume
    void set_volume(float volume);

    void set_master_volume( float volume );

    /// Set global pitch
    void set_pitch(float pitch);

    /// Set reference distance for sound cutoff
    void set_reference_distance(float distance);

    void set_distance_model(ALenum model);

private:
    ALCdevice* m_device;
    ALCcontext* m_context;
    unsigned char priority_levels;

    int set_sound(const unsigned long& handle, const SoundPlayInfo& spi, AudioEmitter *owner);
    void update_subscribers(float timestep);
    void reset_listener();

    bool load_wav(const std::string& file, const FilterInfo &filter_info);
    bool load_ogg(const std::string& file, const FilterInfo &filter_info);

    rc_alAudioSource find_free_source();
public:

    typedef std::deque<rc_alAudioSource> source_deque;
    source_deque m_sources;

    typedef std::map<std::pair<std::string, std::string>, rc_alAudioBuffer> buffer_map;
    buffer_map m_buffers;

    /**
    * Base class of the subscriber chain
    */
    class basicLink
    {
    protected:
        rc_alAudioSource m_source;

    public:
        basicLink(rc_alAudioSource source) : m_source(source), m_discard(false) {}
        virtual ~basicLink() {}
        virtual void update(float timestep, unsigned int current_tick) = 0;
        virtual void update_position() = 0;
        virtual void stop() = 0;
        virtual bool is_transient() { return get_audioEmitter()->IsTransient(); };

        rc_alAudioSource get_source() {return m_source;}

        bool get_discard() { return m_discard; }
        void signal_discard() { m_discard = true; }

        virtual AudioEmitter *get_audioEmitter() = 0;        

        SoundPlayInfo spi;
    private:
        bool        m_discard;
    };

    /**
    * Static audio subscriber link
    */
    class staticLink : public basicLink
    {
    private:
        //alAudio m_audio;
        rc_alAudioBuffer m_buffer;
        AudioEmitter *m_emitter;
        unsigned int play_on_tick;  
        bool played;

    public:
        staticLink(rc_alAudioSource source, AudioEmitter &emitter, rc_alAudioBuffer buffer, const SoundPlayInfo& spi);
        ~staticLink() override;

        void update(float timestep,unsigned int current_tick) override;
        void update_position() override;
        void stop() override;
        AudioEmitter *get_audioEmitter() override {return m_emitter;}
    };


    /**
    * Streaming audio subscriber link
    */
    class streamerLink : public basicLink
    {
    private:
        typedef std::vector<rc_alAudioBuffer> buffer_vector;
        buffer_vector m_buffers;
        audioStreamer *m_streamer;

    public:
        streamerLink(rc_alAudioSource source, audioStreamer &streamer);
        ~streamerLink() override;

        void update(float timestep, unsigned int current_tick) override;
        void update(rc_alAudioBuffer buffer);
        void update_position() override;
        void stop() override;

        rc_alAudioSource get_source() {return m_source;}
        AudioEmitter *get_audioEmitter() override {return m_streamer;}
    };

    typedef std::map<AudioEmitter *, basicLink *> streamer_subscribers;

    typedef std::map<unsigned long, AudioEmitter *> static_handles;

    std::vector<std::string> available_device_list;
    std::string used_device;
    std::string preferred_device;

    unsigned long get_next_handle();
    unsigned long get_invalid_handle();
private:
    unsigned long m_handle_ctr;
public:
    static_handles m_handles;

    std::string GetUsedDevice();
    std::vector<std::string> GetAvailableDevices();
    std::string GetPreferredDevice();

    void change_pitch(unsigned long handle, float pitch);
    void change_volume(unsigned long handle, float volume);
    void Dispose();
    bool IsHandleValid(const unsigned long &handle);
    void SetPosition(const unsigned long &handle, const vec3 &new_pos);
    void TranslatePosition( const unsigned long &handle, const vec3 &trans);
    void SetVelocity(const unsigned long &handle, const vec3 &new_vel);
    void SetPitch(const unsigned long &handle, float pitch);
    void SetVolume(const unsigned long &handle, float volume);
    void SetVolumeMult(const unsigned long &handle, float volume);
    const vec3 GetPosition(const unsigned long &handle);
    std::vector<AudioEmitter*> GetActiveSounds();
    void Stop( const unsigned long & handle );
    void StopAll();
    void StopAllTransient();
    void SetOcclusionPosition(const unsigned long &handle, const vec3 &new_pos);
    streamer_subscribers m_streamers;
    float    m_reference_distance;
    float    m_gain;
    float    m_pitch;
    vec3 listener_pos;
};

void CheckALError(int line, const char* file);

