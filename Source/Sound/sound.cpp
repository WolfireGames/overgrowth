//-----------------------------------------------------------------------------
//           Name: sound.cpp
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

#include <Math/enginemath.h>
#include <Math/vec3math.h>

#include <Internal/datemodified.h>
#include <Internal/timer.h>
#include <Internal/error.h>
#include <Internal/filesystem.h>
#include <Internal/common.h>

#include <Sound/sound.h>
#include <Asset/Asset/soundgroup.h>
#include <Physics/bulletworld.h>
#include <GUI/gui.h>
#include <Logging/logdata.h>
#include <Threading/sdl_wrapper.h>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include <sstream>

const bool kSoundOcclusionEnabled = false;  // Disabled for now because of a problem with ambient triangles getting occluded (e.g. crowd on stucco arena)

bool g_sound_enable_layered_soundtrack_limiter = false;

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

#define REF_DISTANCE 1.0f
//#define REF_DISTANCE 8.0f

Sound::Sound(const char *preferred_device) : m_last_game_time_scale(1.0f),
                                             m_last_game_timestep(0.0f),
                                             m_accum_game_timestep(0.0f),
                                             m_audio(preferred_device, 1.0f, REF_DISTANCE),
                                             m_soundtrack(0.5f),
                                             whoosh_active_countdown(0) {
    m_audio.set_distance_model(AL_INVERSE_DISTANCE_CLAMPED);
    m_audio.subscribe(m_soundtrack);
    pcg32_srandom_r(&play_group_rand_state, 0xDEADBEEF, 0xFACEFEED);
}

void Sound::Dispose() {
    m_audio.Dispose();
    m_soundtrack.Dispose();
}

void Sound::Update() {
    for (auto &a_t : ambient_triangles) {
        for (unsigned j = 0; j < 3; ++j) {
            a_t.vol[j] = min(1.0f, a_t.vol[j] + m_accum_game_timestep);
            SetVolume(a_t.handles[j], a_t.vol[j]);
        }
    }

    if (m_last_game_time_scale == 1.0f) {
        m_audio.set_pitch(1.0f);
    } else {
        m_audio.set_pitch(m_last_game_time_scale);
    }

    m_audio.update(m_accum_game_timestep);

    m_accum_game_timestep = 0.0f;

    if (whoosh_active_countdown > 0) {
        whoosh_active_countdown--;
        if (whoosh_active_countdown == 0) {
            setAirWhoosh(0);
        }
    }
}

void Sound::UpdateGameTimestep(float timestep) {
    m_last_game_timestep = timestep;
    m_accum_game_timestep += timestep;
}

void Sound::UpdateGameTimescale(float time_scale) {
    m_last_game_time_scale = time_scale;
}

void Sound::updateListener(vec3 pos, vec3 vel, vec3 facing, vec3 up) {
    m_audio.set_listener_position(pos);
    m_audio.set_listener_velocity(vel);
    m_audio.set_listener_orientation(facing, up);

    // std::ostringstream oss;
    // oss << "Ambient triangles: " << ambient_triangles.size();
    // GUI::Instance()->AddDebugText("ambient_triangles", oss.str(), 0.5f);

    for (auto &a_t : ambient_triangles) {
        for (unsigned j = 0; j < 3; ++j) {
            // vec3 new_dir(dot(a_t.rel_dir[j], cross(facing, up)),
            //              dot(a_t.rel_dir[j], up),
            //              dot(a_t.rel_dir[j], facing));
            // SetPosition(a_t.handles[j],
            //             new_dir);
            // printf("%f %f %f\n", new_dir[0], new_dir[1], new_dir[2]);
            SetPosition(a_t.handles[j],
                        pos + a_t.rel_dir[j]);
            SetVelocity(a_t.handles[j],
                        vel);
            // DebugDraw::Instance()->AddWireSphere(pos+a_t.rel_dir[j], 0.3f, vec3(1.0f), _delete_on_update);
        }
    }
}

unsigned long Sound::CreateHandle() {
    return m_audio.get_next_handle();
}

void Sound::Play(const unsigned long &handle, SoundPlayInfo spi) {
    char abs_path[kPathSize];
    if (FindFilePath(spi.path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths) == -1) {
        std::string error_text = "Could not find sound file: " + std::string(spi.path);
        DisplayError("Error", error_text.c_str());
    }

    /*
     * Disabled until we're done threading and can move this out of this class.
    if(kSoundOcclusionEnabled){
        if(!(spi.flags & SoundFlags::kRelative)){
            const vec3 &listener_pos = m_audio.listener_pos;
            bool occluded = false;
            if(bullet_world){
                occluded = bullet_world->CheckRayCollision(listener_pos, spi.occlusion_position);
            }
            if(occluded){
                spi.volume_mult *= _occluded_sound_mult;
            }
        }
    }
    */

    if (spi.volume >= 0.0) {
        m_audio.play(handle, spi);
    } else {
        const int kBufSize = 256;
        char buf[kBufSize];
        FormatString(buf, kBufSize, "Playing sound with negative volume: %s", spi.path.c_str());
        DisplayError("Error", buf);
    }
}

void Sound::PlayGroup(const unsigned long &handle, SoundGroupPlayInfo &sgpi) {
    // DebugDraw::Instance()->AddWireSphere(pos, 0.1f, vec4(1.0f), _persistent);
    // SoundGroupRef sgr = SoundGroups::Instance()->ReturnRef(sgpi.path);
    SoundPlayInfo sound_info;
    if (sgpi.specific_path.empty()) {
        sound_info.path = sgpi.GetARandomOrderedSoundPath(&play_group_rand_state);
    } else {
        sound_info.path = sgpi.specific_path;
    }
    sound_info.position = sgpi.position;
    sound_info.occlusion_position = sgpi.occlusion_position;
    sound_info.volume = sgpi.volume * sgpi.gain;
    sound_info.pitch = 1.0f * sgpi.pitch_shift;
    sound_info.created_by_id = sgpi.created_by_id;
    sound_info.filter_info = sgpi.filter_info;
    sound_info.max_gain = sgpi.max_gain;
    sound_info.priority = sgpi.priority;
    sound_info.flags = sgpi.flags;
    sound_info.max_distance = sgpi.max_distance;
    sound_info.play_past_tick = sgpi.play_past_tick;
    return Play(handle, sound_info);
}

void Sound::setAirWhoosh(float amount, float pitch) {
    whoosh_active_countdown = 60;
    amount = max(0.0f, min(1.0f, amount));

    if (m_air_whoosh_sound.get() == NULL) {
        m_air_whoosh_sound = ambient_ptr(new ambientSound(amount));
        SoundPlayInfo spi;
        spi.path = "Data/Sounds/fall_whoosh.wav";
        spi.looping = true;
        unsigned long handle = m_audio.get_next_handle();
        m_audio.play(handle, spi, m_air_whoosh_sound.get());
    } else {
        m_air_whoosh_sound->SetVolume(amount);
        m_air_whoosh_sound->SetPitch(pitch);
    }
}

std::vector<AudioEmitter *> Sound::GetActiveSounds() {
    return m_audio.GetActiveSounds();
}

/*
std::string Sound::GetGroupPath(const std::string &path){
    SoundGroupRef sgr = SoundGroups::Instance()->ReturnRef(path);
    return sgr->GetSoundPath();
}
*/

bool Sound::IsHandleValid(const unsigned long &handle) {
    return m_audio.IsHandleValid(handle);
}

void Sound::SetPosition(const unsigned long &handle, const vec3 &new_pos) {
    m_audio.SetPosition(handle, new_pos);
}

void Sound::TranslatePosition(const unsigned long &handle, const vec3 &trans) {
    m_audio.TranslatePosition(handle, trans);
}

void Sound::SetOcclusionPosition(const unsigned long &handle, const vec3 &new_pos) {
    m_audio.SetOcclusionPosition(handle, new_pos);
}

void Sound::SetVelocity(const unsigned long &handle, const vec3 &new_vel) {
    m_audio.SetVelocity(handle, new_vel);
}

const vec3 Sound::GetPosition(const unsigned long &handle) {
    return m_audio.GetPosition(handle);
}

void Sound::SetPitch(const unsigned long &handle, float pitch) {
    m_audio.SetPitch(handle, pitch);
}

void Sound::SetVolume(const unsigned long &handle, float volume) {
    m_audio.SetVolume(handle, volume);
}

void Sound::Stop(const unsigned long &handle) {
    m_audio.Stop(handle);
}

void Sound::AddMusic(const Path &path) {
    m_soundtrack.AddMusic(path);
}

void Sound::RemoveMusic(const Path &path) {
    m_soundtrack.RemoveMusic(path);
}

void Sound::QueueSegment(const std::string &string) {
    m_soundtrack.QueueSegment(string);
}

void Sound::TransitionToSegment(const std::string &string) {
    m_soundtrack.TransitionIntoSegment(string);
}

void Sound::SetSegment(const std::string &string) {
    m_soundtrack.SetSegment(string);
}

void Sound::TransitionToSong(const std::string &string) {
    m_soundtrack.TransitionToSong(string);
}

void Sound::SetSong(const std::string &string) {
    m_soundtrack.SetSong(string);
}

void Sound::SetLayerGain(const std::string &layer, float v) {
    m_soundtrack.SetLayerGain(layer, v);
}

const std::string Sound::GetSegment() const {
    return m_soundtrack.GetSegment();
}

const std::map<std::string, float> Sound::GetLayerGains() {
    return m_soundtrack.GetLayerGains();
}

const std::vector<std::string> Sound::GetLayerNames() const {
    return m_soundtrack.GetLayerNames();
}

const std::string Sound::GetSongName() const {
    return m_soundtrack.GetSongName();
}

const std::string Sound::GetSongType() const {
    return m_soundtrack.GetSongType();
}

const std::string Sound::GetNextSongName() const {
    return m_soundtrack.GetNextSongName();
}

const std::string Sound::GetNextSongType() const {
    return m_soundtrack.GetNextSongType();
}

void Sound::SetMusicVolume(float val) {
    m_soundtrack.SetVolume(val);
}

void Sound::SetMasterVolume(float val) {
    m_audio.set_master_volume(val);
}

void Sound::AddAmbientTriangle(const std::string &path) {
    char paths[3][kPathSize];
    for (unsigned i = 0; i < 3; ++i) {
        char abs_path[kPathSize];
        FormatString(paths[i], kPathSize, "%s_%d.wav", path.c_str(), i + 1);
        if (FindFilePath(paths[i], abs_path, kPathSize, kDataPaths | kModPaths) == -1) {
            FatalError("Error", "Could not find sound: %s", paths[i]);
        }
    }
    ambient_triangles.resize(ambient_triangles.size() + 1);
    AmbientTriangle &ambient_triangle = ambient_triangles.back();
    ambient_triangle.path = path;
    ambient_triangle.rel_dir[0] = vec3(1.0f, 0.0f, 0.0f);
    ambient_triangle.rel_dir[1] = vec3(-0.5f, 0.0f, 0.866f);
    ambient_triangle.rel_dir[2] = vec3(-0.5f, 0.0f, -0.866f);
    SoundPlayInfo spi;
    spi.looping = true;
    spi.volume = 0.0f;
    spi.flags = spi.flags | SoundFlags::kNoOcclusion;
    for (unsigned i = 0; i < 3; ++i) {
        spi.path = paths[i];
        spi.position = ambient_triangle.rel_dir[i];
        spi.occlusion_position = spi.position;
        unsigned long handle = CreateHandle();
        Play(handle, spi);
        ambient_triangle.handles[i] = handle;
        ambient_triangle.vol[i] = 0.0f;
    }
}

const std::vector<AmbientTriangle> &Sound::GetAmbientTriangles() {
    return ambient_triangles;
}

void Sound::ClearTransient() {
    m_audio.StopAllTransient();
    ambient_triangles.clear();
}

void Sound::Clear() {
    m_audio.StopAll();
    ambient_triangles.clear();
}

/*
 * For this function to work correctly in the future it'll have to use the bullet_world reference indirectly
 * (as the sound system is run in a separate thread).
 *
void Sound::UpdateSoundOcclusion(void (OcclusionCheck*) (int sound_uid, vec3 from, vec3 to)) {
    if(kSoundOcclusionEnabled){
        const vec3 &listener_pos = m_audio.listener_pos;

        std::vector<AudioEmitter*> sounds = m_audio.GetActiveSounds();

        for(unsigned i=0; i<sounds.size(); ++i){
            if(!(sounds[i]->flags() & SoundFlags::kNoOcclusion)){
                const vec3& sound_pos = sounds[i]->GetOcclusionPosition();
                bool occluded = bullet_world->CheckRayCollision(listener_pos, sound_pos);
                if(occluded){
                    sounds[i]->SetVolumeMult(_occluded_sound_mult);
                } else {
                    sounds[i]->SetVolumeMult(1.0f);
                }
            }
        }
    }
}
*/

void Sound::LoadSoundFile(const std::string &path) {
    FilterInfo filter_info;
    m_audio.load(path, filter_info);
}

std::string Sound::GetPreferredDevice() {
    return m_audio.GetPreferredDevice();
}

std::string Sound::GetUsedDevice() {
    return m_audio.GetUsedDevice();
}

std::vector<std::string> Sound::GetAvailableDevices() {
    return m_audio.GetAvailableDevices();
}

void Sound::EnableLayeredSoundtrackLimiter(bool val) {
    g_sound_enable_layered_soundtrack_limiter = val;
}
