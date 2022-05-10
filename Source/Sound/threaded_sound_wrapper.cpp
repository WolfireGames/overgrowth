//-----------------------------------------------------------------------------
//           Name: threaded_sound_wrapper.cpp
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
#include "threaded_sound_wrapper.h"

#include <Threading/thread_name.h>
#include <Threading/sdl_wrapper.h>

#include <Utility/assert.h>
#include <Utility/strings.h>

#include <Memory/allocation.h>
#include <Sound/sound.h>
#include <Logging/logdata.h>
#include <Internal/profiler.h>

static ThreadedSoundMessageQueue message_queue_in;
static ThreadedSoundMessageQueue message_queue_out;
static ThreadedSoundDataBridge tsdb;

void ThreadedSoundMessage::Allocate(size_t size) {
    void* new_ptr = NULL;

    // Calling Allocate to shrink the data store might result in truncation, which is not handled.
    LOG_ASSERT(datalen <= size);

    if (size > LOCAL_DATA_SIZE) {
        new_ptr = OG_MALLOC(size);
    } else {
        new_ptr = local_data;
    }

    if (data != NULL) {
        if (data != new_ptr && new_ptr != NULL) {
            memcpy(new_ptr, data, min(datalen, size));
        }

        if (data != local_data) {
            OG_FREE(data);
        }
    }

    data = new_ptr;

    if (size < LOCAL_DATA_SIZE) {
        datalen = LOCAL_DATA_SIZE;
    } else {
        datalen = size;
    }
}

void ThreadedSoundMessage::Free() {
    if (data != NULL) {
        if (datalen > LOCAL_DATA_SIZE) {
            OG_FREE(data);
        } else {
            // Nothing
        }

        data = NULL;
        string_data = false;
    }
}

ThreadedSoundMessage::ThreadedSoundMessage() : type(None),
                                               data(NULL),
                                               datalen(0),
                                               has_handle(false),
                                               wsh(0),
                                               string_data(false) {
}

ThreadedSoundMessage::ThreadedSoundMessage(Type t) : type(t),
                                                     data(NULL),
                                                     datalen(0),
                                                     has_handle(false),
                                                     wsh(0),
                                                     string_data(false) {
}

ThreadedSoundMessage::~ThreadedSoundMessage() {
    Free();
}

ThreadedSoundMessage::ThreadedSoundMessage(const ThreadedSoundMessage& tsm) : type(tsm.type),
                                                                              data(NULL),
                                                                              datalen(0),
                                                                              has_handle(tsm.has_handle),
                                                                              wsh(tsm.wsh),
                                                                              string_data(tsm.string_data) {
    if (tsm.data != NULL) {
        Allocate(tsm.datalen);
        memcpy(data, tsm.data, tsm.datalen);
    }
}

ThreadedSoundMessage& ThreadedSoundMessage::operator=(const ThreadedSoundMessage& rhs) {
    Free();

    type = rhs.type;
    has_handle = rhs.has_handle;
    wsh = rhs.wsh;
    string_data = rhs.string_data;

    Allocate(rhs.datalen);
    memcpy(data, rhs.data, rhs.datalen);

    return *this;
}

ThreadedSoundMessage::Type ThreadedSoundMessage::GetType() {
    return type;
}

void ThreadedSoundMessage::SetHandle(wrapper_sound_handle _wsh) {
    has_handle = true;
    wsh = _wsh;
}

wrapper_sound_handle ThreadedSoundMessage::GetWrapperHandle() {
    LOG_ASSERT(has_handle);
    return wsh;
}

real_sound_handle ThreadedSoundMessage::GetRealHandle() {
    LOG_ASSERT(has_handle);
    return tsdb.GetHandle(wsh);
}

void ThreadedSoundMessage::SetData(const void* _data_ptr, size_t offset, size_t _datalen) {
    if (offset + _datalen > datalen) {
        Allocate(offset + _datalen);
    }
    memcpy((char*)data + offset, _data_ptr, _datalen);
}

void ThreadedSoundMessage::SetData(const void* _data_ptr, size_t _datalen) {
    if (_datalen > datalen) {
        Allocate(_datalen);
    }
    memcpy(data, _data_ptr, _datalen);
}

void ThreadedSoundMessage::GetData(void* _data_ptr, size_t _datalen) {
    LOG_ASSERT(_datalen <= datalen);
    memcpy(_data_ptr, data, _datalen);
}

void ThreadedSoundMessage::GetData(void* _data_ptr, size_t _offset, size_t _datalen) {
    LOG_ASSERT((_offset + _datalen) <= datalen);
    memcpy(_data_ptr, (char*)data + _offset, _datalen);
}

void ThreadedSoundMessage::SetStringData(const std::string& str) {
    size_t str_size = str.size() * sizeof(char) + 1;
    Allocate(str_size);
    memcpy(data, str.c_str(), str_size);
    string_data = true;
}

std::string ThreadedSoundMessage::GetStringData() {
    LOG_ASSERT(string_data);
    std::string s((const char*)data);
    return s;
}

ThreadedSoundDataBridge::ThreadedSoundDataBridge() : handle_counter(0) {
}

wrapper_sound_handle ThreadedSoundDataBridge::CreateWrapperHandle() {
    wrapper_sound_handle val = 0;
    mutex.lock();
    while (val == 0) {
        val = ++handle_counter;
    }
    // Zero is an invalid map name and good default.
    handle_map[val] = 0;
    mutex.unlock();
    return val;
}

void ThreadedSoundDataBridge::SetHandle(wrapper_sound_handle wsh, real_sound_handle rsh) {
    mutex.lock();
    handle_map[wsh] = rsh;
    mutex.unlock();
}

void ThreadedSoundDataBridge::SetValues(wrapper_sound_handle wsh, const char* name, const char* type) {
    mutex.lock();
    if (data_copy_map.find(wsh) != data_copy_map.end()) {
        SoundInstanceDataCopy& d = data_copy_map[wsh];
        strscpy(d.name, name, SoundInstanceDataCopy::NAME_SIZE);
        strscpy(d.type, type, SoundInstanceDataCopy::TYPE_SIZE);
    }
    mutex.unlock();
}

void ThreadedSoundDataBridge::SetIdentifier(wrapper_sound_handle wsh, const char* id) {
    mutex.lock();
    strscpy(data_copy_map[wsh].id, id, SoundInstanceDataCopy::ID_SIZE);
    mutex.unlock();
}

void ThreadedSoundDataBridge::SetPosition(wrapper_sound_handle wsh, const vec3& position) {
    mutex.lock();
    if (data_copy_map.find(wsh) != data_copy_map.end()) {
        data_copy_map[wsh].position = position;
    }
    mutex.unlock();
}

void ThreadedSoundDataBridge::RemoveHandle(wrapper_sound_handle _wsh) {
    mutex.lock();

    {
        std::map<wrapper_sound_handle, real_sound_handle>::iterator hmit = handle_map.find(_wsh);
        if (hmit != handle_map.end())
            handle_map.erase(hmit);
    }

    {
        std::map<wrapper_sound_handle, SoundInstanceDataCopy>::iterator dcpit = data_copy_map.find(_wsh);
        if (dcpit != data_copy_map.end())
            data_copy_map.erase(dcpit);
    }

    mutex.unlock();
}

real_sound_handle ThreadedSoundDataBridge::GetHandle(wrapper_sound_handle wsh) {
    real_sound_handle rsh = 0;
    mutex.lock();
    if (handle_map.find(wsh) != handle_map.end()) {
        rsh = handle_map[wsh];
    }
    mutex.unlock();
    return rsh;
}

bool ThreadedSoundDataBridge::IsHandleValid(wrapper_sound_handle wsh) {
    bool out;
    mutex.lock();
    std::map<wrapper_sound_handle, real_sound_handle>::iterator it = handle_map.find(wsh);

    if (it != handle_map.end())
        out = true;
    else
        out = false;
    mutex.unlock();
    return out;
}

std::map<wrapper_sound_handle, real_sound_handle> ThreadedSoundDataBridge::GetAllHandles() {
    std::map<wrapper_sound_handle, real_sound_handle> v;
    mutex.lock();
    v = handle_map;
    mutex.unlock();
    return v;
}

SoundInstanceDataCopy ThreadedSoundDataBridge::GetHandleData(wrapper_sound_handle wsh) {
    SoundInstanceDataCopy v;
    mutex.lock();
    if (data_copy_map.find(wsh) != data_copy_map.end()) {
        v = data_copy_map[wsh];
    }
    mutex.unlock();
    return v;
}

// void ThreadedSoundDataBridge::SetHandleData( wrapper_sound_handle wsh, const SoundInstanceDataCopy &sidc ) {
//     mutex.lock();
//     data_copy_map[wsh] = sidc;
//     mutex.unlock();
// }

SoundDataCopy ThreadedSoundDataBridge::GetData() {
    SoundDataCopy out;
    mutex.lock();
    out = sound_data;
    mutex.unlock();
    return out;
}

size_t ThreadedSoundDataBridge::GetSoundHandleCount() {
    size_t i;
    mutex.lock();
    i = handle_map.size();
    mutex.unlock();
    return i;
}

size_t ThreadedSoundDataBridge::GetSoundInstanceCount() {
    size_t i;
    mutex.lock();
    i = data_copy_map.size();
    mutex.unlock();
    return i;
}

void ThreadedSoundDataBridge::SetData(const SoundDataCopy& dat) {
    mutex.lock();
    sound_data = dat;
    mutex.unlock();
}

void ThreadedSoundDataBridge::SetPreferredDevice(const std::string& name) {
    mutex.lock();
    preferred_device = name;
    mutex.unlock();
}

std::string ThreadedSoundDataBridge::GetPreferredDevice() {
    std::string return_value;
    mutex.lock();
    return_value = preferred_device;
    mutex.unlock();
    return return_value;
}

void ThreadedSoundDataBridge::SetUsedDevice(const std::string& name) {
    mutex.lock();
    used_device = name;
    mutex.unlock();
}

std::string ThreadedSoundDataBridge::GetUsedDevice() {
    std::string return_value;
    mutex.lock();
    return_value = used_device;
    mutex.unlock();
    return return_value;
}

std::vector<std::string> ThreadedSoundDataBridge::GetAvailableDevices() {
    std::vector<std::string> ret;
    mutex.lock();
    ret = available_devices;
    mutex.unlock();
    return ret;
}

void ThreadedSoundDataBridge::SetAvailableDevices(std::vector<std::string> devices) {
    mutex.lock();
    available_devices = devices;
    mutex.unlock();
}

void ThreadedSoundMessageQueue::Queue(const ThreadedSoundMessage& ev) {
    mutex.lock();
    messages.push(ev);
    mutex.unlock();
}

ThreadedSoundMessage ThreadedSoundMessageQueue::Pop() {
    mutex.lock();
    ThreadedSoundMessage val = messages.front();
    messages.pop();
    mutex.unlock();

    return val;
}

size_t ThreadedSoundMessageQueue::Count() {
    size_t c = 0;
    mutex.lock();
    c = messages.size();
    mutex.unlock();
    return c;
}

static void ThreadedSoundLoop() {
    bool running = true;
    bool initialized = false;
    Sound* sound = NULL;
    const unsigned int minimum_tick = 16;

    PROFILER_NAME_THREAD(g_profiler_ctx, "Sound Thread");

    while (running) {
        PROFILER_ENTER(g_profiler_ctx, "Sound Loop");

        unsigned int start_time = SDL_TS_GetTicks();

        std::queue<ThreadedSoundMessage> reinsertion;

        PROFILER_ENTER(g_profiler_ctx, "Polling events");

        // Run until the message queue is empty
        while (message_queue_in.Count() > 0) {
            ThreadedSoundMessage msg = message_queue_in.Pop();

            if (msg.GetType() == ThreadedSoundMessage::Initialize) {
                if (false == initialized) {
                    NameCurrentThread("Sound thread");
                    std::string preferred_device = tsdb.GetPreferredDevice();
                    sound = new Sound(preferred_device.c_str());
                    initialized = true;
                    tsdb.SetUsedDevice(sound->GetUsedDevice());
                    tsdb.SetAvailableDevices(sound->GetAvailableDevices());
                } else {
                    LOGE << "Trying to double initialize threaded sound system" << std::endl;
                }
            } else if (initialized) {
                if (msg.GetType() == ThreadedSoundMessage::Dispose) {
                    if (sound) {
                        sound->Dispose();
                        delete sound;
                        sound = NULL;
                        running = false;
                        initialized = false;
                    }
                } else if (msg.GetType() == ThreadedSoundMessage::InFUpdateGameTimestep) {
                    float f;
                    msg.GetData(&f, sizeof(float));
                    sound->UpdateGameTimestep(f);
                } else if (msg.GetType() == ThreadedSoundMessage::InFUpdateGameTimescale) {
                    float f;
                    msg.GetData(&f, sizeof(float));
                    sound->UpdateGameTimescale(f);
                } else if (msg.GetType() == ThreadedSoundMessage::InFSetAirWhoosh) {
                    float v[2];
                    msg.GetData(v, sizeof(v));
                    sound->setAirWhoosh(v[0], v[1]);
                } else if (msg.GetType() == ThreadedSoundMessage::InFUpdateListener) {
                    vec3 v[4];
                    msg.GetData(v, sizeof(v));
                    sound->updateListener(v[0], v[1], v[2], v[3]);
                } else if (msg.GetType() == ThreadedSoundMessage::InFPlay) {
                    SoundPlayInfo* v;
                    msg.GetData(&v, sizeof(SoundPlayInfo*));

                    real_sound_handle rsh = msg.GetRealHandle();
                    sound->Play(rsh, *v);

                    delete v;
                } else if (msg.GetType() == ThreadedSoundMessage::InFPlayGroup) {
                    SoundGroupPlayInfo* v;
                    msg.GetData(&v, sizeof(SoundGroupPlayInfo*));

                    sound->PlayGroup(msg.GetRealHandle(), *v);
                    delete v;
                } else if (msg.GetType() == ThreadedSoundMessage::InRCreateHandle) {
                    wrapper_sound_handle wsh;
                    msg.GetData(&wsh, sizeof(wrapper_sound_handle));
                    real_sound_handle rsh = sound->CreateHandle();

                    tsdb.SetHandle(wsh, rsh);
                } else if (msg.GetType() == ThreadedSoundMessage::InFSetPosition) {
                    vec3 v;

                    msg.GetData(v.entries, sizeof(v.entries));

                    sound->SetPosition(msg.GetRealHandle(), v);
                } else if (msg.GetType() == ThreadedSoundMessage::InFTranslatePosition) {
                    vec3 v;

                    msg.GetData(v.entries, sizeof(v.entries));

                    sound->TranslatePosition(msg.GetRealHandle(), v);
                } else if (msg.GetType() == ThreadedSoundMessage::InFSetVelocity) {
                    vec3 v;

                    msg.GetData(v.entries, sizeof(v.entries));

                    sound->SetVelocity(msg.GetRealHandle(), v);
                } else if (msg.GetType() == ThreadedSoundMessage::InFSetPitch) {
                    float v;

                    msg.GetData(&v, sizeof(float));

                    sound->SetPitch(msg.GetRealHandle(), v);
                } else if (msg.GetType() == ThreadedSoundMessage::InFSetVolume) {
                    float v;

                    msg.GetData(&v, sizeof(float));

                    sound->SetVolume(msg.GetRealHandle(), v);
                } else if (msg.GetType() == ThreadedSoundMessage::InFStop) {
                    sound->Stop(msg.GetRealHandle());
                } else if (msg.GetType() == ThreadedSoundMessage::InFAddMusic) {
                    Path* p;
                    msg.GetData(&p, sizeof(Path*));
                    sound->AddMusic(*p);
                    delete p;
                } else if (msg.GetType() == ThreadedSoundMessage::InFRemoveMusic) {
                    Path* p;
                    msg.GetData(&p, sizeof(Path*));
                    sound->RemoveMusic(*p);
                    delete p;
                } else if (msg.GetType() == ThreadedSoundMessage::InFQueueSegment) {
                    sound->QueueSegment(msg.GetStringData());
                } else if (msg.GetType() == ThreadedSoundMessage::InFTransitionToSegment) {
                    sound->TransitionToSegment(msg.GetStringData());
                } else if (msg.GetType() == ThreadedSoundMessage::InFTransitionToSong) {
                    sound->TransitionToSong(msg.GetStringData());
                } else if (msg.GetType() == ThreadedSoundMessage::InFSetSegment) {
                    sound->SetSegment(msg.GetStringData());
                } else if (msg.GetType() == ThreadedSoundMessage::InFSetLayerGain) {
                    char layer_name[MusicXMLParser::NAME_MAX_LENGTH];
                    float layer_gain;

                    msg.GetData(layer_name, 0, MusicXMLParser::NAME_MAX_LENGTH);
                    msg.GetData(&layer_gain, MusicXMLParser::NAME_MAX_LENGTH, sizeof(float));

                    sound->SetLayerGain(std::string(layer_name), layer_gain);
                } else if (msg.GetType() == ThreadedSoundMessage::InFSetSong) {
                    sound->SetSong(msg.GetStringData());
                } else if (msg.GetType() == ThreadedSoundMessage::InFAddAmbientTriangle) {
                    sound->AddAmbientTriangle(msg.GetStringData());
                } else if (msg.GetType() == ThreadedSoundMessage::InFClear) {
                    sound->Clear();
                } else if (msg.GetType() == ThreadedSoundMessage::InFClearTransient) {
                    sound->ClearTransient();
                } else if (msg.GetType() == ThreadedSoundMessage::InFSetMusicVolume) {
                    float v;
                    msg.GetData(&v, sizeof(float));
                    sound->SetMusicVolume(v);
                } else if (msg.GetType() == ThreadedSoundMessage::InFSetMasterVolume) {
                    float v;
                    msg.GetData(&v, sizeof(float));
                    sound->SetMasterVolume(v);
                } else if (msg.GetType() == ThreadedSoundMessage::InFSetOcclusionPosition) {
                    vec3 v;
                    msg.GetData(&v, sizeof(vec3));
                    sound->SetOcclusionPosition(msg.GetRealHandle(), v);
                } else if (msg.GetType() == ThreadedSoundMessage::InFLoadSoundFile) {
                    sound->LoadSoundFile(msg.GetStringData());
                } else if (msg.GetType() == ThreadedSoundMessage::InFEnableLayeredSoundtrackLimiter) {
                    uint8_t d;
                    msg.GetData(&d, sizeof(uint8_t));
                    sound->EnableLayeredSoundtrackLimiter(d);
                } else if (msg.GetType() == ThreadedSoundMessage::None) {
                    LOGI << "Got None. Invalid" << std::endl;
                } else {
                    LOGE << "Function type is not handled in ThreadedSoundWrapper, needs to be implemented: " << msg.GetType() << std::endl;
                }
            } else {
                LOGW << "Sound system isn't initialized yet, putting message into the queue." << std::endl;
                reinsertion.push(msg);
            }
        }

        while (reinsertion.size() > 0) {
            message_queue_in.Queue(reinsertion.front());
            reinsertion.pop();
        }

        PROFILER_LEAVE(g_profiler_ctx);

        if (initialized) {
            PROFILER_ENTER(g_profiler_ctx, "Sound Update");
            sound->Update();
            PROFILER_LEAVE(g_profiler_ctx);

            PROFILER_ENTER(g_profiler_ctx, "Post Data Poll");
            // Now we update the data from sound to a protected structure for some get functions.
            std::map<wrapper_sound_handle, real_sound_handle> sounds = tsdb.GetAllHandles();
            std::map<wrapper_sound_handle, real_sound_handle>::iterator sounds_it = sounds.begin();
            for (; sounds_it != sounds.end(); sounds_it++) {
                if (sounds_it->second != 0) {
                    if (sound->IsHandleValid(sounds_it->second)) {
                        tsdb.SetPosition(sounds_it->first, sound->GetPosition(sounds_it->second));
                    } else {
                        if (sounds_it->second != 0) {
                            tsdb.RemoveHandle(sounds_it->first);
                        }
                    }
                }
            }

            SoundDataCopy sdc;
            sdc.current_segment = sound->GetSegment();

            sdc.current_song_name = sound->GetSongName();
            sdc.current_song_type = sound->GetSongType();
            sdc.current_layer_names = sound->GetLayerNames();
            sdc.current_layer_gains = sound->GetLayerGains();

            sdc.next_song_name = sound->GetNextSongName();
            sdc.next_song_type = sound->GetNextSongType();
            sdc.ambient_triangles = sound->GetAmbientTriangles();

            std::vector<AudioEmitter*> emitters = sound->GetActiveSounds();

            for (auto& emitter : emitters) {
                SoundSourceInfo ss;

                strscpy(ss.name, emitter->display_name.c_str(), sizeof(ss.name));
                ss.pos = emitter->GetPosition();

                sdc.sound_source.push_back(ss);
            }

            tsdb.SetData(sdc);

            PROFILER_LEAVE(g_profiler_ctx);
        }

        unsigned int duration = SDL_TS_GetTicks() - start_time;

        if (running) {
            PROFILER_ZONE_IDLE(g_profiler_ctx, "Sound Loop Sleep");
            if (duration < minimum_tick) {
                std::this_thread::sleep_for(std::chrono::milliseconds(minimum_tick - duration));
            } else {
                std::this_thread::yield();
            }
        }
        PROFILER_LEAVE(g_profiler_ctx);  // Sound Loop;
    }

    if (sound) {
        delete sound;
    }
}

ThreadedSound::ThreadedSound() : initialized(false) {
}

void ThreadedSound::Initialize(const char* preferred_device) {
    tsdb.SetPreferredDevice(preferred_device);
    thread = std::thread(ThreadedSoundLoop);
    ThreadedSoundMessage tsm(ThreadedSoundMessage::Initialize);
    message_queue_in.Queue(tsm);
    initialized = true;
}

ThreadedSound::~ThreadedSound() {
    Dispose();
}

/*** Sound interface message implementation ***/

// Because this function is called every frame, it's a perfect candidate for
// dealing with return messages from the sound system into the main program thread.
void ThreadedSound::Update() {
}

void ThreadedSound::UpdateGameTimestep(float timestep) {
    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFUpdateGameTimestep);
    tsm.SetData(&timestep, sizeof(float));
    message_queue_in.Queue(tsm);
}

void ThreadedSound::UpdateGameTimescale(float time_scale) {
    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFUpdateGameTimescale);
    tsm.SetData(&time_scale, sizeof(float));
    message_queue_in.Queue(tsm);
}

void ThreadedSound::setAirWhoosh(float amount, float pitch) {
    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFSetAirWhoosh);
    float f[2];
    f[0] = amount;
    f[1] = pitch;
    tsm.SetData(f, sizeof(f));
    message_queue_in.Queue(tsm);
}

void ThreadedSound::updateListener(vec3 pos, vec3 vel, vec3 facing, vec3 up) {
    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFUpdateListener);
    vec3 v[4];
    v[0] = pos;
    v[1] = vel;
    v[2] = facing;
    v[3] = up;
    tsm.SetData(v, sizeof(v));
    message_queue_in.Queue(tsm);
}

unsigned long ThreadedSound::CreateHandle(const char* ident) {
    wrapper_sound_handle wsh = tsdb.CreateWrapperHandle();

    tsdb.SetIdentifier(wsh, ident);

    ThreadedSoundMessage tsm(ThreadedSoundMessage::InRCreateHandle);

    tsm.SetData(&wsh, sizeof(wrapper_sound_handle));

    message_queue_in.Queue(tsm);

    return wsh;
}

void ThreadedSound::Play(const unsigned long& handle, SoundPlayInfo spi) {
    tsdb.SetValues(handle, spi.path.c_str(), "Play");

    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFPlay);

    SoundPlayInfo* v = new SoundPlayInfo(spi);

    tsm.SetHandle(handle);
    tsm.SetData(&v, sizeof(SoundPlayInfo*));

    message_queue_in.Queue(tsm);
}

void ThreadedSound::PlayGroup(const unsigned long& handle, const SoundGroupPlayInfo& sgpi) {
    tsdb.SetValues(handle, sgpi.GetPath().c_str(), "Group");

    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFPlayGroup);

    SoundGroupPlayInfo* v;

    v = new SoundGroupPlayInfo(sgpi);

    v->play_past_tick = sgpi.play_past_tick;

    tsm.SetHandle(handle);
    tsm.SetData(&v, sizeof(SoundGroupPlayInfo*));

    message_queue_in.Queue(tsm);
}

const vec3 ThreadedSound::GetPosition(const unsigned long& handle) {
    return tsdb.GetHandleData(handle).position;
}

const std::string ThreadedSound::GetName(const unsigned long& handle) {
    return tsdb.GetHandleData(handle).name;
}

const std::string ThreadedSound::GetType(const unsigned long& handle) {
    return tsdb.GetHandleData(handle).type;
}

const std::string ThreadedSound::GetID(const unsigned long& handle) {
    return tsdb.GetHandleData(handle).id;
}

void ThreadedSound::SetPosition(const unsigned long& handle, const vec3& new_pos) {
    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFSetPosition);
    tsm.SetHandle(handle);
    tsm.SetData(new_pos.entries, sizeof(new_pos.entries));
    message_queue_in.Queue(tsm);
}

void ThreadedSound::TranslatePosition(const unsigned long& handle, const vec3& trans) {
    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFTranslatePosition);
    tsm.SetHandle(handle);
    tsm.SetData(trans.entries, sizeof(trans.entries));
    message_queue_in.Queue(tsm);
}

void ThreadedSound::SetVelocity(const unsigned long& handle, const vec3& new_vel) {
    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFSetVelocity);
    tsm.SetHandle(handle);
    tsm.SetData(new_vel.entries, sizeof(new_vel.entries));
    message_queue_in.Queue(tsm);
}

void ThreadedSound::SetPitch(const unsigned long& handle, float pitch) {
    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFSetPitch);
    tsm.SetHandle(handle);
    tsm.SetData(&pitch, sizeof(float));
    message_queue_in.Queue(tsm);
}

void ThreadedSound::SetVolume(const unsigned long& handle, float volume) {
    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFSetVolume);
    tsm.SetHandle(handle);
    tsm.SetData(&volume, sizeof(float));
    message_queue_in.Queue(tsm);
}

void ThreadedSound::Stop(const unsigned long& handle) {
    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFStop);
    tsm.SetHandle(handle);
    message_queue_in.Queue(tsm);
}

bool ThreadedSound::IsHandleValid(const unsigned long& handle) {
    return tsdb.IsHandleValid(handle);
}

void ThreadedSound::AddMusic(const Path& path) {
    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFAddMusic);

    Path* p = new Path(path);
    tsm.SetData(&p, sizeof(Path*));

    message_queue_in.Queue(tsm);
}

void ThreadedSound::RemoveMusic(const Path& path) {
    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFRemoveMusic);

    Path* p = new Path(path);
    tsm.SetData(&p, sizeof(Path*));

    message_queue_in.Queue(tsm);
}

void ThreadedSound::QueueSegment(const std::string& string) {
    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFQueueSegment);
    tsm.SetStringData(string);
    message_queue_in.Queue(tsm);
}

void ThreadedSound::TransitionToSegment(const std::string& string) {
    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFTransitionToSegment);
    tsm.SetStringData(string);
    message_queue_in.Queue(tsm);
}

void ThreadedSound::TransitionToSong(const std::string& string) {
    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFTransitionToSong);
    tsm.SetStringData(string);
    message_queue_in.Queue(tsm);
}

void ThreadedSound::SetSegment(const std::string& string) {
    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFSetSegment);
    tsm.SetStringData(string);
    message_queue_in.Queue(tsm);
}

void ThreadedSound::SetLayerGain(const std::string& string, float v) {
    // LOGI << "Queueing call to SetLayerGain(" << string << "," << v << ")" <<  std::endl;
    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFSetLayerGain);
    char str[MusicXMLParser::NAME_MAX_LENGTH];
    strscpy(str, string.c_str(), MusicXMLParser::NAME_MAX_LENGTH);
    tsm.SetData(str, 0, MusicXMLParser::NAME_MAX_LENGTH);
    tsm.SetData(&v, MusicXMLParser::NAME_MAX_LENGTH, sizeof(float));
    message_queue_in.Queue(tsm);
}

void ThreadedSound::SetSong(const std::string& string) {
    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFSetSong);
    tsm.SetStringData(string);
    message_queue_in.Queue(tsm);
}

const std::string ThreadedSound::GetSegment() const {
    return tsdb.GetData().current_segment;
}

const std::vector<std::string> ThreadedSound::GetLayerNames() const {
    return tsdb.GetData().current_layer_names;
}

const std::map<std::string, float> ThreadedSound::GetLayerGains() const {
    return tsdb.GetData().current_layer_gains;
}

const float ThreadedSound::GetLayerGain(const std::string& layer) const {
    return tsdb.GetData().current_layer_gains[layer];
}

const std::string ThreadedSound::GetSongName() const {
    return tsdb.GetData().current_song_name;
}

const std::string ThreadedSound::GetSongType() const {
    return tsdb.GetData().current_song_type;
}

const std::string ThreadedSound::GetNextSongName() const {
    return tsdb.GetData().next_song_name;
}

const std::string ThreadedSound::GetNextSongType() const {
    return tsdb.GetData().next_song_type;
}

void ThreadedSound::AddAmbientTriangle(const std::string& path) {
    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFAddAmbientTriangle);
    tsm.SetStringData(path);
    message_queue_in.Queue(tsm);
}

std::vector<AmbientTriangle> ThreadedSound::GetAmbientTriangles() {
    std::vector<AmbientTriangle> d = tsdb.GetData().ambient_triangles;
    return d;
}

void ThreadedSound::Clear() {
    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFClear);
    message_queue_in.Queue(tsm);
}

void ThreadedSound::ClearTransient() {
    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFClearTransient);
    message_queue_in.Queue(tsm);
}

void ThreadedSound::Dispose() {
    if (initialized) {
        ThreadedSoundMessage tsm(ThreadedSoundMessage::Dispose);
        message_queue_in.Queue(tsm);
        thread.join();
        initialized = false;
    }
}

void ThreadedSound::SetMusicVolume(float val) {
    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFSetMusicVolume);
    tsm.SetData(&val, sizeof(float));
    message_queue_in.Queue(tsm);
}

void ThreadedSound::SetMasterVolume(float val) {
    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFSetMasterVolume);
    tsm.SetData(&val, sizeof(float));
    message_queue_in.Queue(tsm);
}

void ThreadedSound::SetOcclusionPosition(const unsigned long& handle, const vec3& new_pos) {
    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFSetOcclusionPosition);
    tsm.SetHandle(handle);
    tsm.SetData(&new_pos, sizeof(vec3));
    message_queue_in.Queue(tsm);
}

void ThreadedSound::LoadSoundFile(const std::string& path) {
    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFLoadSoundFile);
    tsm.SetStringData(path);
    message_queue_in.Queue(tsm);
}

std::vector<SoundSourceInfo> ThreadedSound::GetCurrentSoundSources() {
    SoundDataCopy sdc = tsdb.GetData();
    return sdc.sound_source;
}

size_t ThreadedSound::GetSoundHandleCount() {
    return tsdb.GetSoundHandleCount();
}

size_t ThreadedSound::GetSoundInstanceCount() {
    return tsdb.GetSoundInstanceCount();
}

std::map<wrapper_sound_handle, real_sound_handle> ThreadedSound::GetAllHandles() {
    return tsdb.GetAllHandles();
}

std::string ThreadedSound::GetPreferredDevice() {
    return tsdb.GetPreferredDevice();
}

std::string ThreadedSound::GetUsedDevice() {
    return tsdb.GetUsedDevice();
}

std::vector<std::string> ThreadedSound::GetAvailableDevices() {
    return tsdb.GetAvailableDevices();
}

void ThreadedSound::EnableLayeredSoundtrackLimiter(bool val) {
    ThreadedSoundMessage tsm(ThreadedSoundMessage::InFEnableLayeredSoundtrackLimiter);
    uint8_t d = val ? 1 : 0;
    tsm.SetData(&d, sizeof(uint8_t));
    message_queue_in.Queue(tsm);
}
