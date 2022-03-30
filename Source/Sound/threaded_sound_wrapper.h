//-----------------------------------------------------------------------------
//           Name: threaded_sound_wrapper.h
//      Developer: Wolfire Games LLC
//         Author: Max Danielsson
//    Description: Wrapper around the entire sound system, allowing all sound to be run
//                  in a separate thread. This is done using a message queue structure and a 
//                  synchronized data structure for immediate reads.
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

#include <Sound/soundplayinfo.h>
#include <Sound/al_audio.h>
#include <Sound/ambienttriangle.h>

#include <Math/vec3.h>
#include <Internal/filesystem.h>

#include <queue>
#include <thread>
#include <mutex>
#include <chrono>


typedef unsigned long wrapper_sound_handle;
typedef unsigned long real_sound_handle;

class ThreadedSoundMessage {

public:
    static const unsigned char LOCAL_DATA_SIZE = 64;

    enum Type
    {
        None,
        Initialize,
        Dispose,

        InFUpdateGameTimestep,
        InFUpdateGameTimescale,
        InFSetAirWhoosh,
        InFUpdateListener,
        InFPlay,
        InFPlayGroup,
        InFSetPosition,
        InFTranslatePosition,
        InFSetVelocity,
        InFSetPitch,
        InFSetVolume,
        InFStop,
        InFAddMusic,
        InFRemoveMusic,
        InFQueueSegment,
        InFTransitionToSegment,
        InFTransitionToSong,
        InFSetSegment,
        InFSetLayerGain,
        InFSetSong,
        InFAddAmbientTriangle,
        InFClear,
        InFClearTransient,
        InFSetMusicVolume,
        InFSetMasterVolume,
        InFSetOcclusionPosition,
        InFLoadSoundFile,
        InFEnableLayeredSoundtrackLimiter,

        InRCreateHandle
    };

private: 
    Type type;

    unsigned char local_data[LOCAL_DATA_SIZE];

    void Free();

    void *data;
    size_t datalen;

    bool has_handle;
    wrapper_sound_handle wsh;

    bool string_data;
public:
    void Allocate(size_t size);

    ThreadedSoundMessage();
    ThreadedSoundMessage( Type t );
    ~ThreadedSoundMessage();

    ThreadedSoundMessage( const ThreadedSoundMessage& tsm );
    ThreadedSoundMessage& operator=(const ThreadedSoundMessage& tsm);

    Type GetType();

    void SetHandle( wrapper_sound_handle _wsh );
    wrapper_sound_handle GetWrapperHandle();
    real_sound_handle GetRealHandle();

    //Different ways of storing data in same memory chunk.
    void SetData( const void* mem_ptr, size_t data_len ); 
    void SetData( const void* mem_ptr, size_t offset, size_t data_len );
    void GetData( void* mem_ptr, size_t max_len );
    void GetData( void* mem_ptr, size_t offset, size_t data_len );

    void SetStringData( const std::string &str );
    std::string GetStringData( );
};

class ThreadedSoundMessageQueue {
    std::queue<ThreadedSoundMessage> messages;
    std::mutex mutex;
public:

    void Queue( const ThreadedSoundMessage &ev );
    ThreadedSoundMessage Pop( );
    size_t Count( );
};

struct SoundSourceInfo {
    vec3 pos;
    char name[128];
};

struct SoundDataCopy {
    std::string current_segment;
    std::string current_song_name;
    std::string current_song_type;

    std::string next_song_name;
    std::string next_song_type;

    std::vector<AmbientTriangle> ambient_triangles;

    std::map<std::string,float> current_layer_gains;
    std::vector<std::string> current_layer_names;

    std::vector<SoundSourceInfo> sound_source;
}; 

struct SoundInstanceDataCopy {
    static const int ID_SIZE = 8;
    static const int NAME_SIZE = 32;
    static const int TYPE_SIZE = 8;
    char id[ID_SIZE];
    char name[NAME_SIZE];
    char type[TYPE_SIZE];
    vec3 position;
};

/*
 * Class containing data that is used for copies, reads and mapping internal to external handles.
 */
class ThreadedSoundDataBridge {

private:
    std::mutex mutex;

    wrapper_sound_handle handle_counter;
    std::map<wrapper_sound_handle,real_sound_handle> handle_map;
    std::map<wrapper_sound_handle,SoundInstanceDataCopy> data_copy_map;

    SoundDataCopy sound_data;

    std::vector<std::string> available_devices;
    std::string preferred_device;
    std::string used_device;
public: 
    ThreadedSoundDataBridge();

    wrapper_sound_handle CreateWrapperHandle();

    void SetHandle( wrapper_sound_handle wsh, real_sound_handle rsh );
    void SetIdentifier( wrapper_sound_handle wsh, const char* id );
    void SetValues( wrapper_sound_handle wsh, const char* name, const char* type );
    void SetPosition( wrapper_sound_handle wsh, const vec3& position );

    void RemoveHandle( wrapper_sound_handle wsh );
    real_sound_handle GetHandle( wrapper_sound_handle wsh );
    bool IsHandleValid( wrapper_sound_handle wsh );

    std::map<wrapper_sound_handle,real_sound_handle> GetAllHandles();
    SoundInstanceDataCopy GetHandleData( wrapper_sound_handle wsh );
    //void SetHandleData( wrapper_sound_handle wsh, const SoundInstanceDataCopy &sidc );

    SoundDataCopy GetData();
    size_t GetSoundHandleCount();
    size_t GetSoundInstanceCount();
    void SetData( const SoundDataCopy& dat );

    void SetPreferredDevice(const std::string& name);
    std::string GetPreferredDevice();
    
    void SetUsedDevice(const std::string& name);
    std::string GetUsedDevice();

    void SetAvailableDevices(std::vector<std::string> devices);
    std::vector<std::string> GetAvailableDevices();
};

class ThreadedSound {
private:

    std::thread thread;
    bool initialized;

public:
    ThreadedSound();
    ~ThreadedSound();

    void Initialize(const char* preferred_device);

    //Following is a copy of the Sound interface
    void Update();
    void UpdateGameTimestep( float timestep );
    void UpdateGameTimescale( float time_scale );

    void setAirWhoosh(float amount, float pitch = 1.0f);

    void updateListener(vec3 pos, vec3 vel, vec3 facing, vec3 up);

    unsigned long CreateHandle(const char* ident);

    void Play(const unsigned long &handle, SoundPlayInfo spi);
    
    void PlayGroup(const unsigned long &handle, const SoundGroupPlayInfo& sgpi);
    const vec3 GetPosition(const unsigned long &handle);
    const std::string GetName(const unsigned long& handle);
    const std::string GetType(const unsigned long& handle);
    const std::string GetID(const unsigned long& handle);
    void SetPosition(const unsigned long &handle, const vec3 &new_pos);
    void TranslatePosition(const unsigned long& handle, const vec3 &movement);
    void SetVelocity(const unsigned long &handle, const vec3 &new_vel);
    void SetPitch(const unsigned long &handle, float pitch);
    void SetVolume(const unsigned long &handle, float volume);
    void Stop(const unsigned long &handle);
    bool IsHandleValid(const unsigned long &handle);

    void AddMusic(const Path& path);
    void RemoveMusic(const Path& path);

    void QueueSegment(const std::string& string);
    void TransitionToSegment(const std::string& string);
    void TransitionToSong(const std::string& string);

    void SetSegment(const std::string& string);
    void SetLayerGain(const std::string& layer, float gain);
    void SetSong(const std::string& string);

    const std::string GetSegment() const;
    const std::map<std::string,float> GetLayerGains() const;
    const std::vector<std::string> GetLayerNames() const;
    const float GetLayerGain( const std::string& layer ) const;
    const std::string GetSongName() const;
    const std::string GetSongType() const;

    const std::string GetNextSongName() const;
    const std::string GetNextSongType() const;

    void AddAmbientTriangle(const std::string &path);
    std::vector<AmbientTriangle> GetAmbientTriangles();
    void Clear();
    //Clear all sounds that are considered transient (that is sound effects in a level and not music)
    //This is called when a level is unloaded, or loaded. Basically between major state changes.
    void ClearTransient();

    //std::vector<AudioEmitter*> GetActiveSounds();
    //alAudio &getAlAudio();
    //void changeSoundtrack(char *name, char *type)

    void Dispose();
    void SetMusicVolume( float val );
    void SetMasterVolume( float val );

    //void UpdateSoundOcclusion( );
    //void SetSoundOcclusionState( int id, bool occluded );
    void SetOcclusionPosition( const unsigned long &handle, const vec3 &new_pos );
    void LoadSoundFile( const std::string & path );
    //End of Sound interface.
    
    std::vector<SoundSourceInfo> GetCurrentSoundSources();
    size_t GetSoundHandleCount();
    size_t GetSoundInstanceCount();

    std::map<wrapper_sound_handle,real_sound_handle> GetAllHandles();

    std::string GetPreferredDevice();
    std::string GetUsedDevice();
    std::vector<std::string> GetAvailableDevices();

    void EnableLayeredSoundtrackLimiter(bool val);
};
