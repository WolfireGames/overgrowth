//-----------------------------------------------------------------------------
//           Name: soundtrack.h
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

#include <Sound/al_audio.h>
#include <Sound/Loader/base_loader.h>
#include <Sound/high_res_buffer_segment.h>
#include <Sound/AudioFilters/transition_mixer.h>
#include <Sound/AudioFilters/limiter_audio_filter.h>

#include <XML/Parsers/musicxmlparser.h>
#include <Sound/buffer_segment.h>

#include <Internal/filesystem.h>
#include <Internal/referencecounter.h>

#include <map>
#include <set>
#include <deque>

class Soundtrack;

extern bool g_sound_enable_layered_soundtrack_limiter;

class PlayedInterface
{
public:
    PlayedInterface( Soundtrack* _owner );
    virtual ~PlayedInterface() {};
    virtual void update( HighResBufferSegment* buffer ) = 0;

    virtual int64_t  GetPCMPos() = 0;
    virtual void SetPCMPos( int64_t pos ) = 0;
    virtual int64_t GetPCMCount() = 0;

    virtual int SampleRate() = 0;
    virtual int Channels() = 0;

protected:
    Soundtrack* owner; 
};

class PlayedSongInterface : public PlayedInterface 
{
public:
    PlayedSongInterface( Soundtrack* _owner );
    PlayedSongInterface( Soundtrack* _owner, const MusicXMLParser::Song& song);
    ~PlayedSongInterface() override {};

    virtual void Rewind() = 0;
    virtual bool IsAtEnd() = 0;

    virtual void SetGain(float v) = 0;
    virtual float GetGain() = 0;

    const std::string GetSongName() const;
    const std::string GetSongType() const;
    const MusicXMLParser::Song& GetSong() const;

protected:
    MusicXMLParser::Song song;
};

typedef ReferenceCounter<PlayedSongInterface> rc_PlayedSongInterface;

class Soundtrack : public audioStreamer
{
private:

    class PlayedSegment : public PlayedInterface
    {
    public:
        PlayedSegment( Soundtrack* _owner );
        PlayedSegment( Soundtrack* _owner, const MusicXMLParser::Segment& segment, bool overlapped_transition );
        ~PlayedSegment() override {};
        void update( HighResBufferSegment* buffer ) override;
        const MusicXMLParser::Segment& GetSegment();
        bool IsAtEnd();
        void Rewind();

        int64_t  GetPCMPos() override;
        void SetPCMPos( int64_t pos ) override;
        int64_t GetPCMCount() override;
    
        int SampleRate() override;
        int Channels() override;

        const std::string GetSegmentName() const;
    private:
        rc_baseLoader data;
        MusicXMLParser::Segment segment;
    public:
        bool overlapped_transition;
        bool started;
    };

    class PlayedSegmentedSong : public PlayedSongInterface
    {
    public:
        PlayedSegmentedSong( Soundtrack* _owner, const MusicXMLParser::Song& _song );
        ~PlayedSegmentedSong( ) override;
        void update( HighResBufferSegment* buffer ) override;
        bool QueueSegment( const MusicXMLParser::Segment& nextSeg );
        bool TransitionIntoSegment( const MusicXMLParser::Segment& newSeg );
        bool SetSegment( const MusicXMLParser::Segment& nextSeg );

        bool IsAtEnd() override;
        void Rewind() override;
        int SampleRate() override;
        int Channels() override;

        void SetGain(float v) override;
        float GetGain() override;

        const std::string GetSegmentName() const;

        void SetPCMPos( int64_t c ) override;
        int64_t GetPCMPos() override;
        int64_t GetPCMCount() override;

        friend bool operator==( const Soundtrack::PlayedSegmentedSong &lhs, const Soundtrack::PlayedSegmentedSong &rhs );
    private:
        PlayedSegment currentSegment;

        std::deque<PlayedSegment> segmentQueue;

        TransitionMixer mixer;
    };

    class PlayedSingleSong : public PlayedSongInterface
    {
    public:
        PlayedSingleSong( Soundtrack* _owner );
        PlayedSingleSong( Soundtrack* _owner, const MusicXMLParser::Song& song );
        ~PlayedSingleSong() override {};
        void update( HighResBufferSegment* buffer ) override;
        bool IsAtEnd() override;
        void Rewind() override;

        int64_t  GetPCMPos() override;
        void SetPCMPos( int64_t pos ) override;
        int64_t GetPCMCount() override;

        void SetGain(float v) override;
        float GetGain() override;
    
        int SampleRate() override;
        int Channels() override;
    private:
        rc_baseLoader data;
    public:
        bool started;

        int current_volume_step;
        float volume_change_per_second;

        float target_gain;
        float from_gain;
        float current_gain;
    };

    class PlayedLayeredSong : public PlayedSongInterface
    {
    public:
        PlayedLayeredSong( Soundtrack* _owner );
        PlayedLayeredSong( Soundtrack* _owner, const MusicXMLParser::Song& song );
        ~PlayedLayeredSong() override;
        void update( HighResBufferSegment* buffer ) override;
        bool IsAtEnd() override;
        void Rewind() override;

        int64_t  GetPCMPos() override;
        void SetPCMPos( int64_t pos ) override;
        int64_t GetPCMCount() override;

        void SetGain(float v) override;
        float GetGain() override;
    
        int SampleRate() override;
        int Channels() override;

        //Specialized
        bool SetLayerGain(const std::string& name, float gain);
        float GetLayerGain(const std::string& name);
        std::vector<std::string> GetLayerNames() const;
        const std::map<std::string,float> GetLayerGains();
    private:
        std::vector<rc_PlayedSongInterface> subsongs;

        int64_t pcm_count;
        int64_t pcm_pos;
        int sample_rate;
        int channels;
        bool started;
    };

    class TransitionPlayer : public PlayedInterface
    {
    public:
        TransitionPlayer(Soundtrack* _owner);
        ~TransitionPlayer() override;

        void update( HighResBufferSegment* buffer ) override;
        void SetTransitionPeriod( float sec );
        
        bool TransitionToSong( const MusicXMLParser::Song& nextSong );
        bool SetSong( const MusicXMLParser::Song& nextSong );

        bool QueueSegment( const std::string& segment_name );
        bool TransitionIntoSegment( const std::string& segment_name );
        bool SetSegment( const std::string& segment_name );
        const std::string GetSegmentName( ) const;

        bool SetLayerGain( const std::string& layer, float gain );
        float GetLayerGain( const std::string& layer );
        std::vector<std::string> GetLayerNames() const;
        const std::map<std::string,float> GetLayerGains();

        void SetPCMPos( int64_t c ) override;
        int64_t GetPCMPos() override;
        int64_t GetPCMCount() override;

        int SampleRate() override;
        int Channels() override;

        const std::string GetSongName() const;
        const std::string GetSongType() const;

        const std::string GetNextSongName() const;
        const std::string GetNextSongType() const;

    private:
        std::map<std::string, int64_t> stored_pcm_pos;
        bool playing;
        TransitionMixer mixer;

        rc_PlayedSongInterface currentSong;
        
        std::deque<rc_PlayedSongInterface> songQueue;
    };

    friend bool operator==( const Soundtrack::PlayedSegmentedSong &lhs, const Soundtrack::PlayedSegmentedSong &rhs );
    friend bool operator!=( const Soundtrack::PlayedSegmentedSong &lhs, const Soundtrack::PlayedSegmentedSong &rhs );

    std::map<std::string,MusicXMLParser::Music> music;

    LimiterAudioFilter limiter;
    TransitionPlayer transitionPlayer;

    int current_volume_step;
    float volume_change_per_second;

    float volume_start;
    float volume_target;

    void PostProcessVolume(HighResBufferSegment* buffer);
public:

    void Dispose();

    //Audiostreamer API
    /// Request to fill a buffer with data
    void update(rc_alAudioBuffer buffer) override;

    /// Return the number of buffers this streaming class requires (usually 2)
    unsigned long required_buffers() override;

	/// Stop the current stream (reset to empty)
	void Stop() override;

    //AudioEmitter API
    //
    /// Set to false to allow the emitter to time out (useful only on non-looping sounds)
    bool KeepPlaying() override;
    /// if true is returned, this will be a relative-to-listener sound
    bool GetPosition(vec3 &p) override;
    void GetDirection(vec3 &p) override;
    const vec3& GetVelocity() override;
    const vec3 GetPosition() override;
    const vec3 GetOcclusionPosition() override;

    /// Indicate the priority of this effect to make room for newer/higher priority effects
    unsigned char GetPriority() override;

    void SetVolume(float vol) override;

    bool IsTransient() override;
    
public: //Control API
    Soundtrack( float volume );

    ~Soundtrack() override;

    void AddMusic( const Path& file );
    void RemoveMusic( const Path& file );

    //Set what segment to play after current is finished
    bool QueueSegment( const std::string& name );
    bool TransitionIntoSegment( const std::string& name );
    //Abruptly change segment
    bool SetSegment( const std::string& name );

    const std::string GetSegment( ) const;
    const std::map<std::string,float> GetLayerGains();

    const std::vector<std::string> GetLayerNames() const;
    const std::string GetSongName() const;
    const std::string GetSongType() const;
    const std::string GetNextSongName() const;
    const std::string GetNextSongType() const;
    const MusicXMLParser::Song GetSong(const std::string& name);

    //Make a soft transition to a different song.
    bool TransitionToSong( const std::string& name );
    //Abruptly change song.
    bool SetSong( const std::string& name );

    void SetLayerGain(const std::string& layer, float v);
};


inline bool operator==( const Soundtrack::PlayedSegmentedSong &lhs, const Soundtrack::PlayedSegmentedSong &rhs )
{
    return lhs.song == rhs.song;
}

inline bool operator!=( const Soundtrack::PlayedSegmentedSong &lhs, const Soundtrack::PlayedSegmentedSong &rhs )
{
    return !(lhs == rhs);
}
