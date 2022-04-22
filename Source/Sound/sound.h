//-----------------------------------------------------------------------------
//           Name: sound.h
//      Developer: Wolfire Games LLC
//         Author: David Rosen
//    Description: The sound class loads, manages, and plays sound effects
//                 Not that this class might be run in a separate thread from the main
//                 application, meaning thay any modifications have to be made with case, ensuring
//                 that all calls to the outside application or third part libraries are thread-safe.
//                  
//                 Additionally, this class, and system takes ownership of OpenAL. Meaning that no other
//                 Subsystem is allowed to directly communicate with the OpenAL API as it's not thread-safe.
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

#include <Math/enginemath.h>

#include <Sound/al_audio.h>
#include <Sound/ambient_sound.h>
#include <Sound/soundtrack.h>
#include <Asset/Asset/soundgroup.h>
#include <Sound/ambienttriangle.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <map>

//-----------------------------------------------------------------------------
// Class Definition
//-----------------------------------------------------------------------------

class BulletWorld;

const float _occluded_sound_mult = 0.5f;

extern bool g_sound_enable_layered_soundtrack_limiter;

class Sound {
    public:    
        Sound(const char* preferred_device);

        void Update();
        void UpdateGameTimestep( float timestep );
        void UpdateGameTimescale( float time_scale );

        void setAirWhoosh(float amount, float pitch = 1.0f);

        void updateListener(vec3 pos, vec3 vel, vec3 facing, vec3 up);

        unsigned long CreateHandle();

        void Play(const unsigned long &handle, SoundPlayInfo spi);
        
        void PlayGroup(const unsigned long &handle, SoundGroupPlayInfo& sgpi);
        const vec3 GetPosition(const unsigned long &handle);
        void SetPosition(const unsigned long &handle, const vec3 &new_pos);
        //Safer variant of SetPosition(GetPosition()+translation) as sound can be threaded and
        //there's a potential delay of the position being updated.
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
        void SetSong(const std::string& string);
        void SetLayerGain(const std::string& layer, float v);

        const std::string GetSegment() const;
        const std::map<std::string,float> GetLayerGains();
        const std::vector<std::string> GetLayerNames() const;
        const std::string GetSongName() const;
        const std::string GetSongType() const;

        const std::string GetNextSongName() const;
        const std::string GetNextSongType() const;

        void AddAmbientTriangle(const std::string &path);
        const std::vector<AmbientTriangle> &GetAmbientTriangles();
        void Clear();
        //Clear all sounds that are considered transient (that is sound effects in a level and not music)
        //This is called when a level is unloaded, or loaded. Basically between major state changes.
        void ClearTransient();

        std::vector<AudioEmitter*> GetActiveSounds();
        //alAudio &getAlAudio();
        //void changeSoundtrack(char *name, char *type)

        void Dispose();
        void SetMusicVolume( float val );
        void SetMasterVolume( float val );

        //void UpdateSoundOcclusion( );
        //void SetSoundOcclusionState( int id, bool occluded );
        void SetOcclusionPosition( const unsigned long &handle, const vec3 &new_pos );
        void LoadSoundFile( const std::string & path );

        void SetPreferredDevice(const std::string& name);
        std::string GetPreferredDevice();
        
        void SetUsedDevice(const std::string& name);
        std::string GetUsedDevice();

        void SetAvailableDevices(std::vector<std::string> devices);
        std::vector<std::string> GetAvailableDevices();

        void EnableLayeredSoundtrackLimiter(bool val);
    private:
    
        //Value containint the games time_scale value.
        float m_last_game_time_scale;
        //Value containing the latests game thread update timestep.
        float m_last_game_timestep;
        //In case the game does multiple frames on one audio frame, we will know the total amount
        //of time passed since last sound update in the main game thread.
        float m_accum_game_timestep;
        
        alAudio m_audio;

        Soundtrack m_soundtrack;

        typedef std::unique_ptr<ambientSound> ambient_ptr;
        ambient_ptr m_air_whoosh_sound;
        int whoosh_active_countdown; //Kill whoosh if not updated in given frameperiod.

        std::vector<AmbientTriangle> ambient_triangles;

        pcg32_random_t play_group_rand_state;
};
