//-----------------------------------------------------------------------------
//           Name: engine.cpp
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
#include "engine.h"

#include <Game/hardcoded_assets.h>
#include <Game/level.h>
#include <Game/savefile.h>
#include <Game/avatar_control_manager.h>

#include <GUI/gui.h>
#include <GUI/widgetframework.h>
#include <GUI/scriptable_ui.h>
#include <GUI/dimgui/dimgui.h>
#include <GUI/dimgui/modmenu.h>
#include <GUI/dimgui/imgui_impl_sdl_gl3.h>
#include <GUI/IMUI/im_element.h>
#include <GUI/IMUI/im_support.h>

#include <Graphics/cubemap.h>
#include <Graphics/flares.h>
#include <Graphics/models.h>
#include <Graphics/particles.h>
#include <Graphics/pxdebugdraw.h>
#include <Graphics/retargetfile.h>
#include <Graphics/sky.h>
#include <Graphics/camera.h>
#include <Graphics/shaders.h>
#include <Graphics/font_renderer.h>
#include <Graphics/simplify.hpp>
#include <Graphics/text.h>
#include <Graphics/lightprobecollection.hpp>
#include <Graphics/halfedge.h>

#include <XML/level_loader.h>
#include <XML/Parsers/manifestparser.h>

#include <Objects/cameraobject.h>
#include <Objects/hotspot.h>
#include <Objects/terrainobject.h>
#include <Objects/movementobject.h>
#include <Objects/reflectioncaptureobject.h>
#include <Objects/placeholderobject.h>
#include <Objects/envobject.h>
#include <Objects/lightprobeobject.h>
#include <Objects/lightvolume.h>
#include <Objects/decalobject.h>
#include <Objects/itemobject.h>
#include <Objects/dynamiclightobject.h>
#include <Objects/prefab.h>

#include <Editors/map_editor.h>
#include <Editors/sky_editor.h>
#include <Editors/actors_editor.h>

#include <Internal/hardware_specs.h>
#include <Internal/config.h>
#include <Internal/timer.h>
#include <Internal/common.h>
#include <Internal/filesystem.h>
#include <Internal/memwrite.h>
#include <Internal/locale.h>
#include <Internal/filesystem.h>
#include <Internal/modloading.h>
#include <Internal/detect_settings.h>
#include <Internal/dialogues.h>
#include <Internal/stopwatch.h>
#include <Internal/spawneritem.h>
#include <Internal/assetpreload.h>
#include <Internal/referencecounter.h>
#include <Internal/profiler.h>
#include <Internal/zip_util.h>

#include <Asset/Asset/levelinfo.h>
#include <Asset/Asset/levelset.h>
#include <Asset/Asset/syncedanimation.h>
#include <Asset/Asset/attacks.h>
#include <Asset/Asset/reactions.h>
#include <Asset/Asset/animation.h>

#include <Compat/fileio.h>
#include <Compat/filepath.h>

#include <Timing/timingevent.h>
#include <Timing/intel_gl_perf.h>

#include <Memory/stack_allocator.h>
#include <Memory/allocation.h>

#include <Physics/bulletworld.h>
#include <Physics/physics.h>

#include <Math/vec2math.h>
#include <Math/vec3math.h>

#include <Logging/logdata.h>
#include <Logging/ramhandler.h>

#include <Utility/assert.h>
#include <Utility/strings.h>

#include <Online/online.h>
#include <Online/online_utility.h>

#include <UserInput/keyTranslator.h>
#include <UserInput/keycommands.h>

#include <Wrappers/glm.h>
#include <Images/image_export.hpp>
#include <Threading/thread_name.h>
#include <Network/asnetwork.h>
#include <Version/version.h>
#include <Steam/steamworks.h>
#include <JSON/json.h>
#include <Scripting/scriptfile.h>
#include <Sound/sound.h>
#include <Main/debuglevelload.h>

#include <SDL.h>

#include <algorithm>
#include <limits>

extern char imgui_ini_path[kPathSize];
extern bool asdebugger_enabled;
extern bool asprofiler_enabled;

//#define OpenVR
#ifdef OpenVR
#include "openvr.h"
#endif

//#define OculusVR
#ifdef OculusVR
#include "OVR_CAPI_GL.h"
#include "Extras/OVR_Math.h"
bool g_oculus_vr_activated = false;

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(k)
#endif

namespace OVR {

struct DepthBuffer {
    GLuint texId;

    DepthBuffer(Sizei size, int sampleCount) {
        UNREFERENCED_PARAMETER(sampleCount);

        assert(sampleCount <= 1);  // The code doesn't currently handle MSAA textures.

        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        GLenum internalFormat = GL_DEPTH_COMPONENT24;
        GLenum type = GL_UNSIGNED_INT;
        if (GLEW_ARB_depth_buffer_float) {
            internalFormat = GL_DEPTH_COMPONENT32F;
            type = GL_FLOAT;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, size.w, size.h, 0, GL_DEPTH_COMPONENT, type, NULL);
    }
    ~DepthBuffer() {
        if (texId) {
            glDeleteTextures(1, &texId);
            texId = 0;
        }
    }
};

struct TextureBuffer {
    ovrSession Session;
    ovrTextureSwapChain TextureChain;
    GLuint texId;
    GLuint fboId;
    Sizei texSize;

    TextureBuffer(ovrSession session, bool rendertarget, bool displayableOnHmd, Sizei size, int mipLevels, unsigned char* data, int sampleCount) : Session(session),
                                                                                                                                                   TextureChain(NULL),
                                                                                                                                                   texId(0),
                                                                                                                                                   fboId(0),
                                                                                                                                                   texSize(0, 0) {
        UNREFERENCED_PARAMETER(sampleCount);

        assert(sampleCount <= 1);  // The code doesn't currently handle MSAA textures.

        texSize = size;

        if (displayableOnHmd) {
            // This texture isn't necessarily going to be a rendertarget, but it usually is.
            assert(session);           // No HMD? A little odd.
            assert(sampleCount == 1);  // ovr_CreateSwapTextureSetD3D11 doesn't support MSAA.

            ovrTextureSwapChainDesc desc = {};
            desc.Type = ovrTexture_2D;
            desc.ArraySize = 1;
            desc.Width = size.w;
            desc.Height = size.h;
            desc.MipLevels = 1;
            desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
            desc.SampleCount = 1;
            desc.StaticImage = ovrFalse;

            ovrResult result = ovr_CreateTextureSwapChainGL(Session, &desc, &TextureChain);

            int length = 0;
            ovr_GetTextureSwapChainLength(session, TextureChain, &length);

            if (OVR_SUCCESS(result)) {
                for (int i = 0; i < length; ++i) {
                    GLuint chainTexId;
                    ovr_GetTextureSwapChainBufferGL(Session, TextureChain, i, &chainTexId);
                    glBindTexture(GL_TEXTURE_2D, chainTexId);

                    if (rendertarget) {
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    } else {
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    }
                }
            }
        } else {
            glGenTextures(1, &texId);
            glBindTexture(GL_TEXTURE_2D, texId);

            if (rendertarget) {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            } else {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            }

            glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, texSize.w, texSize.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }

        if (mipLevels > 1) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        Graphics::Instance()->genFramebuffers(&fboId, "texture_buffer");
    }

    ~TextureBuffer() {
        if (TextureChain) {
            ovr_DestroyTextureSwapChain(Session, TextureChain);
            TextureChain = NULL;
        }
        if (texId) {
            glDeleteTextures(1, &texId);
            texId = 0;
        }
        if (fboId) {
            Graphics::Instance()->deleteFramebuffer(&fboId);
            fboId = 0;
        }
    }

    Sizei GetSize() const {
        return texSize;
    }

    void SetAndClearRenderSurface(DepthBuffer* dbuffer) {
        GLuint curTexId;
        if (TextureChain) {
            int curIndex;
            ovr_GetTextureSwapChainCurrentIndex(Session, TextureChain, &curIndex);
            ovr_GetTextureSwapChainBufferGL(Session, TextureChain, curIndex, &curTexId);
        } else {
            curTexId = texId;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, fboId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curTexId, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dbuffer->texId, 0);

        glViewport(0, 0, texSize.w, texSize.h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_FRAMEBUFFER_SRGB);
    }

    void UnsetRenderSurface() {
        glBindFramebuffer(GL_FRAMEBUFFER, fboId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
    }

    void Commit() {
        if (TextureChain) {
            ovr_CommitTextureSwapChain(Session, TextureChain);
        }
    }
};

};  // namespace OVR

OVR::TextureBuffer* eyeRenderTexture[2] = {NULL, NULL};
OVR::DepthBuffer* eyeDepthBuffer[2] = {NULL, NULL};
ovrMirrorTexture mirrorTexture = NULL;
GLuint mirrorFBO = 0;
long long frameIndex = 0;
ovrSession session;
ovrHmdDesc hmdDesc;

#endif

extern ASTextContext g_as_text_context;

SceneLight* primary_light = NULL;

// We cannot allow the loading screen to call anything in input unless input is mutex locked
// this is because the loading loops modify the state of the input class.
std::queue<SDL_Event> queued_events;

extern Timer game_timer;
extern Timer ui_timer;
extern TextAtlas g_text_atlas[kNumTextAtlas];
extern TextAtlasRenderer g_text_atlas_renderer;

#define THREADED 1

#ifdef _DEBUG
const int _max_steps_per_frame = 4;
#else
const int _max_steps_per_frame = 4;
#endif

extern const char* new_empty_level_path;
extern Config config;
extern Config default_config;
extern NativeLoadingText native_loading_text;
extern bool g_simple_water;
extern bool g_disable_fog;
extern bool g_simple_shadows;
extern bool g_level_shadows;
extern bool g_albedo_only;
extern bool g_no_reflection_capture;
extern bool g_no_detailmaps;
extern bool g_no_decals;
extern bool g_no_decal_elements;
extern bool g_single_pass_shadow_cascade;
extern bool g_draw_vr;
extern bool g_gamma_correct_final_output;
bool g_draw_collision = false;
bool g_make_invisible_visible = false;

std::string script_dir_path;
static float prev_view_time;

static const float shadow_sizes[] = {20.0f, 60.0f, 250.0f, 1100.0f};  // Dimensions in world space of square for each shadow cascade

static bool runtime_level_load_stress = true;

static bool interrupt_loading;

const char* font_path = "Data/Fonts/Lato-Regular.ttf";

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

//#define USE_NVTX_PROFILER true

#ifdef OpenVR
vr::IVRSystem* m_pHMD;

std::string GetTrackedDeviceString(vr::IVRSystem* pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError = NULL) {
    uint32_t unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
    if (unRequiredBufferLen == 0)
        return "";

    char* pchBuffer = new char[unRequiredBufferLen];
    unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
    std::string sResult = pchBuffer;
    delete[] pchBuffer;
    return sResult;
}

void ProcessVREvent(const vr::VREvent_t& event) {
    switch (event.eventType) {
        case vr::VREvent_TrackedDeviceActivated: {
            // SetupRenderModelForTrackedDevice( event.trackedDeviceIndex );
            LOGI << "Device " << event.trackedDeviceIndex << " attached. Setting up render model." << std::endl;
        } break;
        case vr::VREvent_TrackedDeviceDeactivated: {
            LOGI << "Device " << event.trackedDeviceIndex << " detached." << std::endl;
        } break;
        case vr::VREvent_TrackedDeviceUpdated: {
            LOGI << "Device " << event.trackedDeviceIndex << " updated." << std::endl;
        } break;
    }
}
#endif

VBOContainer quad_vert_vbo;
VBOContainer quad_index_vbo;

void PushGPUProfileRange(const char* cstr) {
#ifdef USE_NVTX_PROFILER
    nvtxRangePushA(cstr);
#endif

#ifdef GLDEBUG
    if (GLAD_GL_KHR_debug) {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, cstr);
    }
#endif  // GLDEBUG
}

void PopGPUProfileRange() {
#ifdef USE_NVTX_PROFILER
    nvtxRangePop();
#endif

#ifdef GLDEBUG
    if (GLAD_GL_KHR_debug) {
        glPopDebugGroup();
    }
#endif  // GLDEBUG
}

int live_update_countdown = 0;

static void RequestLiveUpdate() {
    live_update_countdown = 10;
}

static void LiveUpdate(SceneGraph* scenegraph, AssetManager* asset_manager, ScriptableUI* scriptable_ui) {
    Input* input = Input::Instance();
    Graphics* graphics = Graphics::Instance();
    Engine* engine = Engine::Instance();

    if (config.PrimarySourceModified()) {
        config.Load(config.GetPrimaryPath());
        engine->SetGameSpeed(config["global_time_scale_mult"].toNumber<float>(), true);
        input->SetFromConfig(config);
        graphics->SetFromConfig(config, true);
        ActiveCameras::Get()->SetAutoCamera(config["auto_camera"].toNumber<bool>());
        graphics->InitScreen();
        DebugDrawAux::Instance()->visible_sound_spheres = config["visible_sound_spheres"].toNumber<bool>();
        engine->GetSound()->SetMusicVolume(config["music_volume"].toNumber<float>());
        engine->GetSound()->SetMasterVolume(config["master_volume"].toNumber<float>());
    }

    ModLoading::Instance().Reload();
    AssetPreload::Instance().Reload();

    Textures::Instance()->Reload();
    if (scenegraph) {
        if (scenegraph->sky) {
            scenegraph->sky->Reload();
            scenegraph->sky->LightingChanged(scenegraph->terrain_object_ != NULL);
        }
    }

    asset_manager->Reload<Attack>();
    asset_manager->Reload<Reaction>();
    asset_manager->Reload<SyncedAnimationGroup>();
    asset_manager->Reload<Animation>();
    asset_manager->Reload<SoundGroup>();
    asset_manager->Reload<ParticleType>();
    asset_manager->Reload<Material>();
    asset_manager->Reload<Character>();
    asset_manager->Reload<Animation>();
    asset_manager->Reload<Item>();

    if (scenegraph) {
        scenegraph->SendMessageToAllObjects(OBJECT_MSG::RELOAD);
        scenegraph->level->LiveUpdateCheck();
        scenegraph->particle_system->script_context_.Reload();
    }
    if (scriptable_ui) {
        scriptable_ui->Reload();
    }
    AnimationRetargeter::Instance()->Reload();
    Shaders::Instance()->Reload();

    ReloadImGui();
}

const char* CStrEngineStateType(const EngineStateType& state) {
    switch (state) {
        case kEngineNoState:
            return "EngineNoState";
        case kEngineLevelState:
            return "EngineLevelState";
        case kEngineEditorLevelState:
            return "EngineEditorLevelState";
        case kEngineScriptableUIState:
            return "EngineScriptableUIState";
        case kEngineCampaignState:
            return "EngineCampaignState";
    }
    return "Unknown";
}

EngineState::EngineState(std::string _id, EngineStateType _type, Path _path) : id(_id), type(_type), path(_path), pop_past(false) {
}

EngineState::EngineState(std::string _id, EngineStateType _type) : id(_id), type(_type), pop_past(false) {
}

EngineState::EngineState(std::string _id, ScriptableCampaign* _campaign, Path _path) : id(_id), type(kEngineCampaignState), path(_path), pop_past(true) {
    campaign.Set(_campaign);
}

EngineState::EngineState() : type(kEngineNoState), pop_past(true) {
}

std::ostream& operator<<(std::ostream& out, const EngineState& in) {
    out << "Type(" << in.type << "," << in.path << ")";
    return out;
}

Engine* Engine::instance_ = NULL;

void Engine::Dispose() {
    ModLoading::Instance().DeRegisterCallback(this);
    ModLoading::Instance().Dispose();

    config.GetRef("menu_show_state") = show_state;
    config.GetRef("menu_show_performance") = show_performance;
    config.GetRef("menu_show_log") = show_log;
    config.GetRef("menu_show_warnings") = show_warnings;
    config.GetRef("menu_show_save") = show_save;
    config.GetRef("menu_show_sound") = show_sound;
    config.GetRef("menu_show_mod_menu") = show_mod_menu;
    config.GetRef("menu_show_asdebugger_contexts") = show_asdebugger_contexts;
    config.GetRef("menu_show_asprofiler") = show_asprofiler;
    config.GetRef("menu_show_mp_debug") = show_mp_debug;
    config.GetRef("menu_show_mp_info") = show_mp_info;
    config.GetRef("menu_show_input_debug") = show_input_debug;
    config.GetRef("menu_show_mp_settings") = show_mp_settings;

    if (scenegraph_) {
        scenegraph_->map_editor->gui = NULL;
        scenegraph_->Dispose();
        delete scenegraph_;
        scenegraph_ = NULL;
    }
    if (scriptable_menu_)
        delete scriptable_menu_;

    for (auto& g_text_atla : g_text_atlas) {
        g_text_atla.Dispose();
    }
    Models::Instance()->Dispose();
    ActiveCameras::Get()->SetCameraObject(NULL);
    particle_types.Dispose();
    g_text_atlas_renderer.Dispose();
    Input::Instance()->Dispose();
    DecalTextures::Dispose();
    Textures::Instance()->Dispose();
    Shaders::Instance()->Dispose();
    DebugDraw::Instance()->Dispose();
    DisposeTextAtlases();
    ScriptFileCache::Instance()->Dispose();
    sound.Dispose();
    ActiveCameras::Instance()->Dispose();
    Graphics::Instance()->Dispose();
    asset_manager.Dispose();
    IMrefCountTracker.logSanityCheck();
    Online::Instance()->Dispose();
#if ENABLE_STEAMWORKS
    Steamworks::Instance()->Dispose();
#endif
    as_network.Dispose();
    DisposeImGui();

    SDLNet_Quit();
    SDL_Quit();

    // Save config on shutdown, in case we have some global data stored in the configuration.
    std::string config_path = GetConfigPath();
    if (config.HasChangedSinceLastSave()) {
        LOGI << "Saving the configuration because we're disposing the engine and there are unsaved changes" << std::endl;
        config.Save(config_path);
    }
    Engine::instance_ = NULL;

    AssetPreload::Instance().Dispose();

#ifdef OculusVR
    if (g_oculus_vr_activated) {
        ovr_Shutdown();
    }
#endif
#ifdef OpenVR
    if (m_pHMD) {
        vr::VR_Shutdown();
        m_pHMD = NULL;
    }
#endif
}

Engine* Engine::Instance() {
    return Engine::instance_;
}

ThreadedSound* Engine::GetSound() {
    return &sound;
}

AssetManager* Engine::GetAssetManager() {
    return &asset_manager;
}

AnimationEffectSystem* Engine::GetAnimationEffectSystem() {
    return &particle_types;
}

LipSyncSystem* Engine::GetLipSyncSystem() {
    return &lip_sync_system;
}

ASNetwork* Engine::GetASNetwork() {
    return &as_network;
}

SceneGraph* Engine::GetSceneGraph() {
    return scenegraph_;
}

void Engine::UpdateControls(float timestep, bool loading_screen) {
    if (scenegraph_ && scenegraph_->map_editor && scenegraph_->map_editor->state_ != MapEditor::kInGame && !paused) {
        Input::Instance()->AllowControllerInput(false);
    } else {
        Input::Instance()->AllowControllerInput(true);
    }

    if (loading_screen == true || waiting_for_input_) {
        SDL_Event event;
        // We want to poll events during load screen to allow SDL to handle basic window drag/resize events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                {
                    loading_mutex_.lock();
                    LOGE << "User requested interrupt, shutting down loading" << std::endl;
                    interrupt_loading = true;
                    loading_mutex_.unlock();
                }
            }

            if (SDL_GetWindowFlags(Graphics::Instance()->sdl_window_) & SDL_WINDOW_INPUT_FOCUS) {
                switch (event.type) {
                    case SDL_MOUSEBUTTONDOWN:
                    case SDL_KEYDOWN:
                    case SDL_JOYBUTTONDOWN:
                        if (Online::Instance()->IsActive()) {
                            if (Online::Instance()->IsHosting() && Online::Instance()->AllClientsReady()) {
                                last_loading_input_time = SDL_TS_GetTicks();
                                Online::Instance()->SessionStarted(true);
                            }
                        } else {
                            last_loading_input_time = SDL_TS_GetTicks();
                            Online::Instance()->SessionStarted(true);
                        }
                        break;
                }
            }

            queued_events.push(event);
        }
    } else {
        Input* input = Input::Instance();
        Graphics* graphics = Graphics::Instance();

        // reset deltas on userinterfaces
        input->getMouse().Update();

        if (loading_screen == false) {
            PROFILER_ZONE(g_profiler_ctx, "UpdateKeyboardFocus");
            input->UpdateKeyboardFocus();
        }

        input->getKeyboard().clearKeyPresses();
        KeyCommand::ClearKeyPresses();

        // Process all the events queued during the loading screen if necessary.
        // We never want to miss events because our keyboard/mouse state would get out of sync
        if (!queued_events.empty()) {
            PROFILER_ZONE(g_profiler_ctx, "Push queued events");
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                queued_events.push(event);
            }
            while (!queued_events.empty()) {
                SDL_PushEvent(&queued_events.front());
                queued_events.pop();
            }
        }

        {
            PROFILER_ZONE(g_profiler_ctx, "Process SDL events");
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                {
                    PROFILER_ZONE(g_profiler_ctx, "ImGui_ImplSdlGL3_ProcessEvent");
                    ProcessEventImGui(&event);
                }
                // Redirect input events to the controller
                switch (event.type) {
                    case SDL_WINDOWEVENT:
                        switch (event.window.event) {
                            case SDL_WINDOWEVENT_FOCUS_LOST:
                                graphics->SetWindowGrab(false);
                                cursor.SetVisible(true);
                                input->HandleEvent(event);
                                break;
                            case SDL_WINDOWEVENT_FOCUS_GAINED:
#ifndef WIN32
                                RequestLiveUpdate();
#endif
                                input->HandleEvent(event);
                                break;
                            case SDL_WINDOWEVENT_RESIZED:
                                resize_event_frame_counter = 10;
                                resize_value = ivec2(event.window.data1, event.window.data2);
                                break;
                        }
                        break;
                    case SDL_QUIT:
                        Input::Instance()->RequestQuit();
                        return;
                    case SDL_MOUSEBUTTONDOWN:
                    case SDL_MOUSEWHEEL:
                        if (!WantMouseImGui()) {
                            input->HandleEvent(event);
                        }
                        break;
                    case SDL_KEYDOWN:
                    case SDL_TEXTINPUT:
                        if (!WantKeyboardImGui()) {
                            input->HandleEvent(event);
                        }
                        break;
                    default:
                        input->HandleEvent(event);
                }
            }
        }
        input->ignore_mouse_frame = false;

        Input::Instance()->ProcessControllers(timestep);
        PlayerInput* controller = Input::Instance()->GetController(0);
        static const std::string kSlow = "slow";
        if (Input::Instance()->debug_keys && current_engine_state_.type == kEngineEditorLevelState) {
            if (controller != NULL && controller->key_down[kSlow].count == 1) {
                slow_motion = !slow_motion;
                CommitPause();
            }

            if (KeyCommand::CheckPressed(input->getKeyboard(), KeyCommand::kPause, KIMF_LEVEL_EDITOR_GENERAL)) {
                user_paused = !user_paused;
                CommitPause();
            }
        }

        bool file_menu_active = current_engine_state_.type == kEngineScriptableUIState ||
                                current_engine_state_.type == kEngineCampaignState ||
                                (current_engine_state_.type == kEngineEditorLevelState && Input::Instance()->debug_keys);
        if (file_menu_active) {
            if (KeyCommand::CheckPressed(input->getKeyboard(), KeyCommand::kNewLevel, KIMF_LEVEL_EDITOR_GENERAL)) {
                NewLevel();
            }
            if (KeyCommand::CheckPressed(input->getKeyboard(), KeyCommand::kOpenLevel, KIMF_LEVEL_EDITOR_GENERAL)) {
                ::LoadLevel(false);
            }
        }

        static const std::string kShowTiming = "show_timing";
        if (controller != NULL && controller->key_down[kShowTiming].count == 1) {
#ifdef SLIM_TIMING
            slimTimingEvents->toggleVisualization();
#endif  // SLIM_TIMING
        }
        static const std::string kScreenshot = "screenshot";
        if (controller != NULL && controller->key_down[kScreenshot].count == 1) {
            graphics->queued_screenshot = 1;
            graphics->screenshot_mode = Graphics::kGameplay;
            graphics->pre_screenshot_media_mode_state = graphics->media_mode();
            graphics->SetMediaMode(true);
        }
        static const std::string kTransparentScreenshot = "transparent_screenshot";
        if (controller != NULL && controller->key_down[kTransparentScreenshot].count == 1) {
            graphics->queued_screenshot = 1;
            graphics->screenshot_mode = Graphics::kTransparentGameplay;
            graphics->pre_screenshot_media_mode_state = graphics->media_mode();
            graphics->SetMediaMode(true);
        }

        if (KeyCommand::CheckPressed(input->getKeyboard(), KeyCommand::kQuit, KIMF_MENU)) {
            input->RequestQuit();
        }

        if (KeyCommand::CheckPressed(input->getKeyboard(), KeyCommand::kBack, KIMF_MENU)) {
            ScriptableUICallback("back");
        }

        if (KeyCommand::CheckPressed(input->getKeyboard(), KeyCommand::kPrintObjects, KIMF_MENU | KIMF_LEVEL_EDITOR_GENERAL | KIMF_PLAYING)) {
            if (scenegraph_)
                scenegraph_->PrintCurrentObjects();
        }

        if (KeyCommand::CheckPressed(input->getKeyboard(), KeyCommand::kToggleLevelLoadStress, KIMF_MENU | KIMF_LEVEL_EDITOR_GENERAL | KIMF_PLAYING)) {
            if (config["level_load_stress"].toNumber<bool>()) {
                if (runtime_level_load_stress) {
                    LOGI << "Disabling level load stress." << std::endl;
                    runtime_level_load_stress = false;
                } else {
                    LOGI << "Enabling level load stress." << std::endl;
                    runtime_level_load_stress = true;
                }
            } else {
                LOGI << "level load stress has not been enabled." << std::endl;
            }
        }

        if (current_engine_state_.type == kEngineEditorLevelState && KeyCommand::CheckPressed(input->getKeyboard(), KeyCommand::kLoadItemSearch, KIMF_LEVEL_EDITOR_GENERAL)) {
            show_load_item_search = true;
        }

        if (current_engine_state_.type == kEngineEditorLevelState && KeyCommand::CheckPressed(input->getKeyboard(), KeyCommand::kOpenCustomEditor, KIMF_LEVEL_EDITOR_GENERAL)) {
            LOGI << "Launch custom editor(s)" << std::endl;
            scenegraph_->level->Message("edit_selected_dialogue");

            std::vector<Object*> selected;
            scenegraph_->ReturnSelected(&selected);

            for (auto& selected_i : selected) {
                if (selected_i->GetType() == _hotspot_object) {
                    Hotspot* hotspot = (Hotspot*)selected_i;

                    if (hotspot->HasCustomGUI()) {
                        hotspot->LaunchCustomGUI();
                    }
                }
            }
        }

        if (current_engine_state_.type == kEngineEditorLevelState && KeyCommand::CheckPressed(input->getKeyboard(), KeyCommand::kMakeSelectedCharacterSavedCorpse, KIMF_LEVEL_EDITOR_GENERAL)) {
            LOGI << "Make currently selected characters into saved corpses" << std::endl;
            scenegraph_->level->Message("make_selected_character_saved_corpse");
        }

        if (current_engine_state_.type == kEngineEditorLevelState && KeyCommand::CheckPressed(input->getKeyboard(), KeyCommand::kReviveSelectedCharacterAndUnsaveCorpse, KIMF_LEVEL_EDITOR_GENERAL)) {
            LOGI << "Revive currently selected character corpses, and remove saved corpse state" << std::endl;
            scenegraph_->level->Message("revive_selected_character_and_unsave_corpse");
        }

        if (current_engine_state_.type == kEngineScriptableUIState) {
            if (KeyCommand::CheckPressed(input->getKeyboard(), KeyCommand::kRefresh, KIMF_ANY)) {
                LOGI << "Request refresh of current menu script" << std::endl;
                scriptable_menu_->Reload(true);
            }
        }

        if (resize_event_frame_counter >= 0) {
            if (resize_event_frame_counter == 0) {
                Graphics::Instance()->WindowResized(resize_value);
                if (!loading_screen && scenegraph_) {
                    scenegraph_->level->WindowResized(resize_value);
                    ScriptableCampaign* campaign_script = GetCurrentCampaign();
                    if (campaign_script) {
                        campaign_script->WindowResized(resize_value);
                    }
                }
            }
            resize_event_frame_counter--;
        }
    }

#ifdef OpenVR
    vr::VREvent_t event;
    while (m_pHMD->PollNextEvent(&event, sizeof(event))) {
        ProcessVREvent(event);
    }

    // Process SteamVR controller state
    for (vr::TrackedDeviceIndex_t unDevice = 0; unDevice < vr::k_unMaxTrackedDeviceCount; unDevice++) {
        vr::VRControllerState_t state;
        if (m_pHMD->GetControllerState(unDevice, &state, sizeof(state))) {
            // m_rbShowTrackedDevice[ unDevice ] = state.ulButtonPressed == 0;
        }
    }
#endif
}

void Engine::HandleRabbotToggleControls() {
    Keyboard& keyboard = Input::Instance()->getKeyboard();

    bool rabbot_mode = scenegraph_->map_editor->state_ == MapEditor::kInGame;
    if (!rabbot_mode && keyboard.wasScancodePressed(SDL_SCANCODE_8, KIMF_PLAYING | KIMF_LEVEL_EDITOR_GENERAL)) {
        scenegraph_->map_editor->SendInRabbot();
    }
    int menu_player = Input::Instance()->PlayerOpenedMenu();
    if (menu_player != -1 && (current_menu_player == -1 || current_menu_player == menu_player)) {
        if (rabbot_mode) {
            if (current_engine_state_.type == kEngineEditorLevelState) {
                scenegraph_->map_editor->StopRabbot();
            } else {
                current_menu_player = menu_player;
                scenegraph_->level->Message("open_menu");
                char buffer[48];
                sprintf(buffer, "menu_player%i", menu_player);
                scenegraph_->level->Message(buffer);
            }
        } else {
            scenegraph_->level->Message("open_menu");
            current_menu_player = menu_player;
            char buffer[48];
            sprintf(buffer, "menu_player%i", menu_player);
            scenegraph_->level->Message(buffer);
        }
        Graphics::Instance()->SetMediaMode(false);
    }

    if (Online::Instance()->IsClient() && Online::Instance()->GetHostAllowsClientEditor() == false) {
        if (current_engine_state_.type == kEngineEditorLevelState) {
            current_engine_state_.type = kEngineLevelState;
            scenegraph_->map_editor->SendInRabbot();
        }
    }

    if (keyboard.wasScancodePressed(SDL_SCANCODE_F1, KIMF_PLAYING | KIMF_LEVEL_EDITOR_GENERAL)) {
        if (Online::Instance()->IsClient()) {
            if (Online::Instance()->GetHostAllowsClientEditor() == false && false) {
                return;
            }
        }

        if (current_engine_state_.type == kEngineLevelState) {
            current_engine_state_.type = kEngineEditorLevelState;
            scenegraph_->map_editor->StopRabbot();
            Shaders::Instance()->create_program_warning = false;
        } else if (current_engine_state_.type == kEngineEditorLevelState) {
            current_engine_state_.type = kEngineLevelState;
            scenegraph_->map_editor->SendInRabbot();
        }
    }
}

static void ClipPolyToPlane(std::vector<vec3>* poly_verts, vec3 plane_normal, float plane_distance) {
    std::vector<vec3> clipped_verts;
    // Check each line segment against the plane
    for (size_t i = 0, len = poly_verts->size(); i < len; ++i) {
        vec3 start = poly_verts->at(i);
        vec3 end = poly_verts->at((i + 1) % len);
        float start_dot = dot(start, plane_normal);
        float end_dot = dot(end, plane_normal);
        if (start_dot >= plane_distance) {
            // Preserve start point if it is on the correct side of the plane
            clipped_verts.push_back(start);
        }
        if ((start_dot < plane_distance) != (end_dot < plane_distance)) {
            // Line segment intersects the plane, add intersection point
            // Solve for t: start_dot + t * (end_dot - start_dot) = plane_distance
            // We know (end_dot != start_dot) so no risk of divide by zero
            float t = (plane_distance - start_dot) / (end_dot - start_dot);
            vec3 intersection_point = start + (t * (end - start));
            clipped_verts.push_back(intersection_point);
        }
    }
    *poly_verts = clipped_verts;
}

struct VoxelSpan {
    int height[2];  // start and end height (0 = top, 1 = bottom)
};

bool VoxelSpanHeightSort(const VoxelSpan& a, const VoxelSpan& b) {
    return (a.height[0] > b.height[0]);
}

typedef std::list<VoxelSpan> VoxelSpanList;
typedef std::vector<VoxelSpanList> VoxelSpanField;

struct VoxelField {
    VoxelSpanField spans;
    int voxel_field_dims[3];
    vec3 voxel_field_bounds[2];
    float voxel_size;
    VoxelField() {
        for (int& voxel_field_dim : voxel_field_dims) {
            voxel_field_dim = 0;
        }
        voxel_size = 0.5f;
    }
};

static void RasterizeTrisToVoxelField(const std::vector<vec3>& tri_verts, VoxelField& field) {
    vec3 intersect_bounds[2] = {vec3(FLT_MAX), vec3(-FLT_MAX)};
    for (const auto& tri_vert : tri_verts) {
        for (int j = 0; j < 3; ++j) {
            intersect_bounds[0][j] = min(intersect_bounds[0][j], tri_vert[j]);
            intersect_bounds[1][j] = max(intersect_bounds[1][j], tri_vert[j]);
        }
    }

    static const bool kDrawBounds = false;
    if (kDrawBounds) {
        DebugDraw::Instance()->AddWireBox((intersect_bounds[0] + intersect_bounds[1]) * 0.5f,
                                          (intersect_bounds[1] - intersect_bounds[0]) * 0.5f,
                                          vec4(1.0f), _persistent);
    }

    // Get dimensions of voxel field
    for (int j = 0; j < 3; ++j) {
        int voxel_min = (int)floorf(intersect_bounds[0][j] / field.voxel_size);
        int voxel_max = (int)ceilf(intersect_bounds[1][j] / field.voxel_size);
        field.voxel_field_bounds[0][j] = voxel_min * field.voxel_size;
        field.voxel_field_bounds[1][j] = voxel_max * field.voxel_size;
        field.voxel_field_dims[j] = voxel_max - voxel_min;
    }

    // Create storage for voxel spans
    field.spans.clear();
    field.spans.resize(field.voxel_field_dims[0] * field.voxel_field_dims[2]);

    // Rasterize triangles to voxel field
    for (size_t tri_index = 0, num_tri_verts = tri_verts.size();
         tri_index < num_tri_verts;
         tri_index += 3) {
        // Get bounding box of triangle
        vec3 bounds_min(FLT_MAX), bounds_max(-FLT_MAX);
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                bounds_min[j] = min(bounds_min[j], tri_verts[tri_index + i][j]);
                bounds_max[j] = max(bounds_max[j], tri_verts[tri_index + i][j]);
            }
        }
        static const bool kDrawTriBounds = false;
        if (kDrawTriBounds) {
            DebugDraw::Instance()->AddWireBox((bounds_max + bounds_min) * 0.5f, (bounds_max - bounds_min) * 0.5f, vec4(1.0f), _persistent);
        }

        // Get bounding box of triangle in terms of voxels
        vec3 voxel_bounds_min, voxel_bounds_max;
        int voxel_size[3];
        for (int i = 0; i < 3; ++i) {
            voxel_bounds_min[i] = floorf(bounds_min[i] / field.voxel_size);
            voxel_bounds_max[i] = ceilf(bounds_max[i] / field.voxel_size);
            voxel_size[i] = (int)voxel_bounds_max[i] - (int)voxel_bounds_min[i];
            voxel_bounds_min[i] *= field.voxel_size;
            voxel_bounds_max[i] *= field.voxel_size;
        }

        // Clip poly to each voxel slice
        for (int voxel_x = 0; voxel_x < voxel_size[0]; ++voxel_x) {
            // Start with initial triangle
            std::vector<vec3> poly_verts;
            poly_verts.reserve(3);
            for (int i = 0; i < 3; ++i) {
                poly_verts.push_back(tri_verts[tri_index + i]);
            }
            // Clip to slice bounds
            ClipPolyToPlane(&poly_verts, vec3(1, 0, 0), voxel_bounds_min[0] + voxel_x * field.voxel_size);
            ClipPolyToPlane(&poly_verts, vec3(-1, 0, 0), -(voxel_bounds_min[0] + (voxel_x + 1) * field.voxel_size));
            // Clip poly along other axis
            for (int voxel_z = 0; voxel_z < voxel_size[2]; ++voxel_z) {
                // Start with initial triangle
                std::vector<vec3> new_poly_verts = poly_verts;
                // Clip to slice bounds
                ClipPolyToPlane(&new_poly_verts, vec3(0, 0, 1), voxel_bounds_min[2] + voxel_z * field.voxel_size);
                ClipPolyToPlane(&new_poly_verts, vec3(0, 0, -1), -(voxel_bounds_min[2] + (voxel_z + 1) * field.voxel_size));
                // Rasterize clipped polygon to voxel field
                if (!new_poly_verts.empty()) {
                    vec3 poly_bounds_min(FLT_MAX), poly_bounds_max(-FLT_MAX);
                    for (auto& new_poly_vert : new_poly_verts) {
                        for (int j = 0; j < 3; ++j) {
                            poly_bounds_min[j] = min(poly_bounds_min[j], new_poly_vert[j]);
                            poly_bounds_max[j] = max(poly_bounds_max[j], new_poly_vert[j]);
                        }
                    }
                    int span_top = (int)ceilf(poly_bounds_max[1] / field.voxel_size);
                    int span_bottom = (int)floorf(poly_bounds_min[1] / field.voxel_size);
                    VoxelSpan span;
                    span.height[0] = span_top;
                    span.height[1] = span_bottom;
                    int abs_voxel_x = voxel_x + (int)((voxel_bounds_min[0] - field.voxel_field_bounds[0][0]) / field.voxel_size);
                    int abs_voxel_z = voxel_z + (int)((voxel_bounds_min[2] - field.voxel_field_bounds[0][2]) / field.voxel_size);
                    field.spans[abs_voxel_x * field.voxel_field_dims[2] + abs_voxel_z].push_back(span);

                    static const bool kDrawSpans = false;
                    if (kDrawSpans) {
                        vec3 pos = vec3(voxel_bounds_min[0] + (voxel_x + 0.5f) * field.voxel_size,
                                        (span_top + span_bottom) * 0.5f * field.voxel_size,
                                        voxel_bounds_min[2] + (voxel_z + 0.5f) * field.voxel_size);
                        vec3 box_size = vec3(field.voxel_size * 0.5f,
                                             (span_top - span_bottom) * 0.5f * field.voxel_size,
                                             field.voxel_size * 0.5f);
                        DebugDraw::Instance()->AddWireBox(pos, box_size, vec4(1.0f), _persistent);
                    }
                }
            }
        }
    }

    // Merge overlapping voxel spans
    for (auto& span_list : field.spans) {
        span_list.sort(VoxelSpanHeightSort);
        for (std::list<VoxelSpan>::iterator iter = span_list.begin();
             iter != span_list.end();
             ++iter) {
            bool stop = false;
            while (!stop) {
                std::list<VoxelSpan>::iterator next_iter = iter;
                ++next_iter;
                if (next_iter != span_list.end()) {
                    VoxelSpan& span = *iter;
                    VoxelSpan& next_span = *next_iter;
                    if (span.height[1] <= next_span.height[0] + 1 &&
                        span.height[0] >= next_span.height[1] - 1) {
                        span.height[0] = max(span.height[0], next_span.height[0]);
                        span.height[1] = min(span.height[1], next_span.height[1]);
                        span_list.erase(next_iter);
                    } else {
                        stop = true;
                    }
                } else {
                    stop = true;
                }
            }
        }
    }
}

VoxelField g_voxel_field;

void GetColumnNeighbors(int num_spans[2], VoxelSpan* spans[2], int span_start[2], std::vector<std::list<int> >* span_neighbors) {
    if (num_spans[0] == 0 || num_spans[1] == 0) {
        // If either column has no spans than we will find no new neighbors
        return;
    }
    // Start with the top side of the top span of each column
    int span_id[2] = {0, 0};
    int span_end[2] = {0, 0};
    int active_span[2] = {-1, -1};
    bool done = false;
    while (!done) {
        int next_height[2];
        for (int i = 0; i < 2; ++i) {
            next_height[i] = spans[i][span_id[i]].height[span_end[i]];
        }
        if (next_height[0] > next_height[1]) {
            ++span_end[0];
        } else if (next_height[1] > next_height[0]) {
            ++span_end[1];
        } else {
            ++span_end[0];
            ++span_end[1];
        }
        // If we go off the bottom of a span, move to the top of the next span
        for (int i = 0; i < 2; ++i) {
            if (span_end[i] > 1) {
                active_span[i] = -1;  // We left the span so it is no longer active
                span_end[i] = 0;
                ++span_id[i];
                if (span_id[i] >= num_spans[i]) {
                    done = true;
                }
            } else if (span_end[i] == 1) {
                active_span[i] = span_start[i] + span_id[i];  // We entered a new span
                if (active_span[0] != -1 && active_span[1] != -1) {
                    VoxelSpan test_spans[2];
                    test_spans[0] = spans[0][span_id[0]];
                    test_spans[1] = spans[1][span_id[1]];
                    if (test_spans[0].height[0] < test_spans[1].height[1] ||
                        test_spans[1].height[0] < test_spans[0].height[1]) {
                        LOGE << "False positive!" << std::endl;
                    }
                    span_neighbors->at(active_span[0]).push_back(active_span[1]);
                    span_neighbors->at(active_span[1]).push_back(active_span[0]);
                    // We found a neighbor pair!
                }
            }
        }
    }
}

void PlaceLightProbes(SceneGraph* scenegraph, vec3 translation, quaternion rotation, vec3 scale) {
    // Rasterize triangles in scene to g_voxel_field
    {
        std::vector<vec3> tri_verts;
        {  // Get tris intersecting box
            btTransform transform;
            {  // Calculate box transform matrix
                mat4 rotation_mat = Mat4FromQuaternion(rotation);
                mat4 translation_mat;
                translation_mat.SetTranslation(translation);
                mat4 transform_mat = translation_mat * rotation_mat;
                transform.setFromOpenGLMatrix(transform_mat.entries);
            }
            // Set up box collision shape in Bullet
            btCollisionObject temp_object;
            btBoxShape shape(btVector3(scale[0], scale[1], scale[2]));
            temp_object.setCollisionShape(&shape);
            temp_object.setWorldTransform(transform);
            // Check for collision with each object
            for (auto object : scenegraph->collide_objects_) {
                EntityType object_type = object->GetType();
                if (object_type == _env_object || object_type == _terrain_type) {
                    // Extract model from object
                    const Model* model = NULL;
                    BulletObject* bullet_object = NULL;
                    switch (object_type) {
                        case _env_object:
                            bullet_object = ((EnvObject*)object)->bullet_object_;
                            model = &Models::Instance()->GetModel(((EnvObject*)object)->model_id_);
                            break;
                        case _terrain_type:
                            bullet_object = ((TerrainObject*)object)->bullet_object_;
                            model = &((TerrainObject*)object)->terrain_.GetModel();
                            break;
                        default:
                            break;
                    }
                    if (bullet_object && bullet_object->shape.get() &&
                        (bullet_object->shape->getShapeType() == TRIANGLE_MESH_SHAPE_PROXYTYPE || bullet_object->shape->getShapeType() == SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE)) {
                        // Check for collision between box and object, with triangle list
                        TriListResults tlr;
                        TriListCallback cb(tlr);
                        object->scenegraph_->bullet_world_->GetPairCollisions(temp_object, *bullet_object->body, cb);
                        // Add transformed triangles to list
                        const std::vector<int>& tris = tlr[bullet_object];
                        for (int tri : tris) {
                            vec3 vert[3];
                            int face_index = tri * 3;
                            for (int j = 0; j < 3; ++j) {
                                int vert_index = model->faces[face_index + j] * 3;
                                for (int k = 0; k < 3; ++k) {
                                    vert[j][k] = model->vertices[vert_index + k];
                                }
                                tri_verts.push_back(object->GetTransform() * vert[j]);
                            }
                        }
                    }
                }
            }
        }  // done getting tris intersecting box

        RasterizeTrisToVoxelField(tri_verts, g_voxel_field);
    }

    // Create virtual box mesh, and create a voxel field from that
    VoxelField box_voxel_field;
    {
        std::vector<vec3> tri_verts;
        // Add box faces to voxel field
        mat4 box_transform;
        {
            mat4 rotation_mat = Mat4FromQuaternion(rotation);
            mat4 translation_mat;
            translation_mat.SetTranslation(translation);
            mat4 scale_mat;
            scale_mat.SetScale(scale);
            box_transform = translation_mat * rotation_mat * scale_mat;
        }
        tri_verts.push_back(box_transform * vec3(-1, -1, -1));
        tri_verts.push_back(box_transform * vec3(1, -1, -1));
        tri_verts.push_back(box_transform * vec3(1, 1, -1));

        tri_verts.push_back(box_transform * vec3(-1, -1, -1));
        tri_verts.push_back(box_transform * vec3(1, 1, -1));
        tri_verts.push_back(box_transform * vec3(-1, 1, -1));

        tri_verts.push_back(box_transform * vec3(-1, -1, 1));
        tri_verts.push_back(box_transform * vec3(1, -1, 1));
        tri_verts.push_back(box_transform * vec3(1, 1, 1));

        tri_verts.push_back(box_transform * vec3(-1, -1, 1));
        tri_verts.push_back(box_transform * vec3(1, 1, 1));
        tri_verts.push_back(box_transform * vec3(-1, 1, 1));

        tri_verts.push_back(box_transform * vec3(-1, -1, -1));
        tri_verts.push_back(box_transform * vec3(-1, 1, -1));
        tri_verts.push_back(box_transform * vec3(-1, 1, 1));

        tri_verts.push_back(box_transform * vec3(-1, -1, -1));
        tri_verts.push_back(box_transform * vec3(-1, 1, 1));
        tri_verts.push_back(box_transform * vec3(-1, -1, 1));

        tri_verts.push_back(box_transform * vec3(1, -1, -1));
        tri_verts.push_back(box_transform * vec3(1, 1, -1));
        tri_verts.push_back(box_transform * vec3(1, 1, 1));

        tri_verts.push_back(box_transform * vec3(1, -1, -1));
        tri_verts.push_back(box_transform * vec3(1, 1, 1));
        tri_verts.push_back(box_transform * vec3(1, -1, 1));

        tri_verts.push_back(box_transform * vec3(-1, -1, -1));
        tri_verts.push_back(box_transform * vec3(1, -1, -1));
        tri_verts.push_back(box_transform * vec3(1, -1, 1));

        tri_verts.push_back(box_transform * vec3(-1, -1, -1));
        tri_verts.push_back(box_transform * vec3(1, -1, 1));
        tri_verts.push_back(box_transform * vec3(-1, -1, 1));

        tri_verts.push_back(box_transform * vec3(-1, 1, -1));
        tri_verts.push_back(box_transform * vec3(1, 1, -1));
        tri_verts.push_back(box_transform * vec3(1, 1, 1));

        tri_verts.push_back(box_transform * vec3(-1, 1, -1));
        tri_verts.push_back(box_transform * vec3(1, 1, 1));
        tri_verts.push_back(box_transform * vec3(-1, 1, 1));

        RasterizeTrisToVoxelField(tri_verts, box_voxel_field);
    }

    VoxelSpanField empty_box_spans;
    empty_box_spans.resize(box_voxel_field.spans.size());

    // Get empty spans from box field
    for (size_t column_index = 0, len = box_voxel_field.spans.size(); column_index < len; ++column_index) {
        std::list<VoxelSpan>& span_list = box_voxel_field.spans[column_index];
        std::list<VoxelSpan>& empty_span_list = empty_box_spans[column_index];
        span_list.sort(VoxelSpanHeightSort);
        VoxelSpan* prev_span = NULL;
        for (auto& span : span_list) {
            if (prev_span) {
                VoxelSpan empty_span;
                empty_span.height[0] = prev_span->height[1];
                empty_span.height[1] = span.height[0];
                empty_span_list.push_back(empty_span);
            }
            prev_span = &span;
        }
    }

    int voxel_field_start[2];
    voxel_field_start[0] = (int)(g_voxel_field.voxel_field_bounds[0][0] / g_voxel_field.voxel_size);
    voxel_field_start[1] = (int)(g_voxel_field.voxel_field_bounds[0][2] / g_voxel_field.voxel_size);
    int box_voxel_field_start[2];
    box_voxel_field_start[0] = (int)(box_voxel_field.voxel_field_bounds[0][0] / box_voxel_field.voxel_size);
    box_voxel_field_start[1] = (int)(box_voxel_field.voxel_field_bounds[0][2] / box_voxel_field.voxel_size);

    VoxelSpanField cropped_voxel_field_spans;
    cropped_voxel_field_spans.resize(box_voxel_field.spans.size());
    // Adapt triangle voxel field to box voxel field
    for (size_t column_index = 0, len = box_voxel_field.spans.size(); column_index < len; ++column_index) {
        int world_coord[2];
        world_coord[0] = box_voxel_field_start[0];
        world_coord[0] += (column_index / box_voxel_field.voxel_field_dims[2]);
        world_coord[1] = box_voxel_field_start[1];
        world_coord[1] += column_index % box_voxel_field.voxel_field_dims[2];
        int voxel_field_coord[2];
        voxel_field_coord[0] = world_coord[0] - voxel_field_start[0];
        voxel_field_coord[1] = world_coord[1] - voxel_field_start[1];
        int voxel_field_span_index = voxel_field_coord[0] * g_voxel_field.voxel_field_dims[2] + voxel_field_coord[1];
        std::list<VoxelSpan>& span_list = cropped_voxel_field_spans[column_index];
        std::list<VoxelSpan>* box_span_list = NULL;
        if (voxel_field_coord[0] >= 0 && voxel_field_coord[1] >= 0 &&
            voxel_field_coord[0] < g_voxel_field.voxel_field_dims[0] &&
            voxel_field_coord[1] < g_voxel_field.voxel_field_dims[2]) {
            box_span_list = &g_voxel_field.spans[voxel_field_span_index];
        }
        if (box_span_list) {
            span_list = *box_span_list;
        }
    }

    // Crop tri spans to empty box field spans
    for (size_t column_index = 0, len = cropped_voxel_field_spans.size(); column_index < len; ++column_index) {
        std::list<VoxelSpan>& span_list = cropped_voxel_field_spans[column_index];
        std::list<VoxelSpan>* box_span_list = &empty_box_spans[column_index];
        for (std::list<VoxelSpan>::iterator iter = span_list.begin();
             iter != span_list.end();) {
            VoxelSpan& span = *iter;
            if (!box_span_list || box_span_list->empty()) {
                iter = span_list.erase(iter);
            } else {
                LOG_ASSERT(box_span_list->size() == 1);
                VoxelSpan& empty_span = box_span_list->front();
                if (span.height[1] > empty_span.height[0] || span.height[0] < empty_span.height[1]) {
                    iter = span_list.erase(iter);
                } else {
                    span.height[0] = min(span.height[0], empty_span.height[0]);
                    span.height[1] = max(span.height[1], empty_span.height[1]);
                    ++iter;
                }
            }
        }
    }

    // Get empty spans from voxel field
    VoxelSpanField empty_field_spans;
    empty_field_spans.resize(cropped_voxel_field_spans.size());
    for (size_t column_index = 0, len = cropped_voxel_field_spans.size(); column_index < len; ++column_index) {
        std::list<VoxelSpan>& empty_box_span_list = empty_box_spans[column_index];
        std::list<VoxelSpan>& span_list = cropped_voxel_field_spans[column_index];
        std::list<VoxelSpan>& empty_span_list = empty_field_spans[column_index];
        VoxelSpan* prev_span = NULL;
        for (auto& span : span_list) {
            if (prev_span) {
                VoxelSpan empty_span;
                empty_span.height[0] = prev_span->height[1];
                empty_span.height[1] = span.height[0];
                if (empty_span.height[0] != empty_span.height[1]) {
                    empty_span_list.push_back(empty_span);
                }
            }
            prev_span = &span;
        }
        if (!empty_box_span_list.empty()) {
            if (span_list.empty()) {
                empty_span_list = empty_box_span_list;
            } else {
                VoxelSpan empty_span;
                empty_span.height[0] = empty_box_span_list.front().height[0];
                empty_span.height[1] = span_list.front().height[0];
                if (empty_span.height[0] != empty_span.height[1]) {
                    empty_span_list.push_front(empty_span);
                }
                empty_span.height[0] = span_list.back().height[1];
                empty_span.height[1] = empty_box_span_list.front().height[1];
                if (empty_span.height[0] != empty_span.height[1]) {
                    empty_span_list.push_back(empty_span);
                }
            }
        }
    }

    // Flatten empty spans to contiguous memory
    std::vector<VoxelSpan> span_vector;
    std::vector<int> column_start_index;
    column_start_index.resize(empty_field_spans.size());
    for (size_t column_index = 0, len = empty_field_spans.size();
         column_index < len;
         ++column_index) {
        column_start_index[column_index] = span_vector.size();
        std::list<VoxelSpan>& span_list = empty_field_spans[column_index];
        for (auto& iter : span_list) {
            span_vector.push_back(iter);
        }
    }
    // Cap start index list to avoid special case when finding num spans per column
    column_start_index.push_back((int)span_vector.size());

    // Find neighbors to each span
    std::vector<std::list<int> > span_neighbors;
    span_neighbors.resize(span_vector.size());
    for (size_t column_index = 0, len = empty_field_spans.size();
         column_index < len;
         ++column_index) {
        int num_spans[2];
        num_spans[0] = column_start_index[column_index + 1] - column_start_index[column_index];
        if (num_spans[0] == 0) {
            // No spans in this column, so skip to next
            continue;
        }
        int coord[2];
        coord[0] = (int)(column_index / box_voxel_field.voxel_field_dims[2]);
        coord[1] = column_index % box_voxel_field.voxel_field_dims[2];
        // Check column x+1
        if (coord[0] < box_voxel_field.voxel_field_dims[0] - 1) {
            size_t check_column_index = column_index + 1;
            num_spans[1] = column_start_index[check_column_index + 1] - column_start_index[check_column_index];
            if (num_spans[1] > 0) {
                int span_start[2];
                span_start[0] = column_start_index[column_index];
                span_start[1] = column_start_index[check_column_index];
                VoxelSpan* spans[2];
                spans[0] = &span_vector[span_start[0]];
                spans[1] = &span_vector[span_start[1]];
                GetColumnNeighbors(num_spans, spans, span_start, &span_neighbors);
            }
        }
        // Check column z+1
        if (coord[1] < box_voxel_field.voxel_field_dims[0] - 1) {
            size_t check_column_index = column_index + box_voxel_field.voxel_field_dims[2];
            num_spans[1] = column_start_index[check_column_index + 1] - column_start_index[check_column_index];
            if (num_spans[1] > 0) {
                int span_start[2];
                span_start[0] = column_start_index[column_index];
                span_start[1] = column_start_index[check_column_index];
                VoxelSpan* spans[2];
                spans[0] = &span_vector[span_start[0]];
                spans[1] = &span_vector[span_start[1]];
                GetColumnNeighbors(num_spans, spans, span_start, &span_neighbors);
            }
        }
    }

    // Fill the top middle span, and flood fill from there
    std::vector<int> filled;
    filled.resize(span_vector.size(), 0);
    std::queue<int> to_fill;
    to_fill.push(column_start_index[(box_voxel_field.voxel_field_dims[0] / 2 * box_voxel_field.voxel_field_dims[2]) +
                                    box_voxel_field.voxel_field_dims[2] / 2]);
    filled[to_fill.front()] = 1;
    while (!to_fill.empty()) {
        int span = to_fill.front();
        to_fill.pop();
        for (std::list<int>::iterator iter = span_neighbors[span].begin();
             iter != span_neighbors[span].end();
             ++iter) {
            int neighbor = *iter;
            if (!filled[neighbor]) {
                to_fill.push(neighbor);
                filled[neighbor] = filled[span] + 1;
            }
        }
    }

    int kVoxelThinAmount = 16;
    // Spawn probes in voxels
    for (size_t column_index = 0, len = empty_field_spans.size();
         column_index < len;
         ++column_index) {
        std::list<VoxelSpan>& span_list = empty_field_spans[column_index];
        for (int i = column_start_index[column_index], end = column_start_index[column_index + 1];
             i < end; ++i) {
            int voxel_x = (int)(column_index / box_voxel_field.voxel_field_dims[2]);
            int voxel_z = column_index % box_voxel_field.voxel_field_dims[2];
            if (voxel_x % kVoxelThinAmount == 0 && voxel_z % kVoxelThinAmount == 0) {
                float world_x = box_voxel_field.voxel_field_bounds[0][0] + (voxel_x + 0.5f) * box_voxel_field.voxel_size;
                float world_z = box_voxel_field.voxel_field_bounds[0][2] + (voxel_z + 0.5f) * box_voxel_field.voxel_size;
                VoxelSpan& span = span_vector[i];
                for (int j = span.height[1]; j < span.height[0]; ++j) {
                    if (j % kVoxelThinAmount == 0) {
                        float world_y = (j + 0.5f) * box_voxel_field.voxel_size;
                        vec3 pos(world_x, world_y, world_z);
                        Object* ppo = new LightProbeObject();
                        if (!filled[i]) {
                            ((LightProbeObject*)ppo)->GetScriptParams()->ASSetInt("Negative", 1);
                        }
                        ppo->SetTranslation(pos);
                        if (ActorsEditor_AddEntity(scenegraph, ppo)) {
                        } else {
                            LOGE << "Failed at adding probe to scenegraph" << std::endl;
                            delete ppo;
                        }
                    }
                }
            }
        }
    }

    VoxelSpanField final_field;
    final_field.resize(empty_field_spans.size());
    for (size_t column_index = 0, len = final_field.size();
         column_index < len;
         ++column_index) {
        std::list<VoxelSpan>& span_list = final_field[column_index];
        for (int i = column_start_index[column_index], end = column_start_index[column_index + 1];
             i < end; ++i) {
            if (filled[i] == 0) {
                span_list.push_back(span_vector[i]);
            }
        }
    }

    g_voxel_field = box_voxel_field;
    g_voxel_field.spans = final_field;
}

static int shutdown_timer = 100;

static uint64_t level_load_settle_frame_count = 0;

const char* stress_levels[] = {
    "Data/Levels/og_story/19b_Magma_Arena.xml",
    "Data/Levels/og_story/lukas-test05.xml",
    "Data/Levels/og_story/lukas-test13.xml",
    "Data/Levels/og_story/23a_barracks.xml",
    "Data/Levels/og_story/23b_Rock_Arena_ffa.xml",
    "Data/Levels/og_story/27_Sky_Ark.xml",
    "Data/Levels/og_story/test10.xml",
    "Data/Levels/og_story/lukas-test23.xml",
    "Data/Levels/og_story/test13.xml",
    "Data/Levels/og_story/22c_Waterfall_Arena_1v3.xml",
    "Data/Levels/og_story/lukas-test36.xml",
    "Data/Levels/og_story/test26.xml",
    "Data/Levels/og_story/test25.xml",
    "Data/Levels/og_story/lukas-test09.xml",
    "Data/Levels/og_story/22b_Waterfall_Arena_1v4.xml",
    "Data/Levels/og_story/lukas-test12.xml",
    "Data/Levels/og_story/19_MagmaBarracks_Jill.xml",
    "Data/Levels/og_story/lukas-test33.xml",
    "Data/Levels/og_story/25_Garden_Duel.xml",
    "Data/Levels/og_story/test06.xml",
    "Data/Levels/og_story/11_Rebel_Base.xml",
    "Data/Levels/og_story/test18.xml",
    "Data/Levels/og_story/19c_Magma_Arena_1v1.xml",
    "Data/Levels/og_story/01_Slaver_Camp_Landing.xml",
    "Data/Levels/og_story/lukas-test27.xml",
    "Data/Levels/og_story/23c_Rock_Arena_Wolves.xml",
    "Data/Levels/og_story/cave_artPass.xml",
    "Data/Levels/og_story/lukas-test01.xml",
    "Data/Levels/og_story/14_The_Rats_Cache.xml",
    "Data/Levels/og_story/test02.xml",
    "Data/Levels/og_story/lukas-test32.xml",
    "Data/Levels/og_story/lukas-test10.xml",
    "Data/Levels/og_story/lukas-test03.xml",
    "Data/Levels/og_story/test24.xml",
    "Data/Levels/og_story/19_Magma_Barracks.xml",
    "Data/Levels/og_story/test03.xml",
    "Data/Levels/og_story/18_RudeAwakening_Jill.xml",
    "Data/Levels/og_story/lukas-test02.xml",
    "Data/Levels/og_story/18_RudeAwakening.xml",
    "Data/Levels/og_story/lukas-test24.xml",
    "Data/Levels/og_story/07_Beach_Boat_Assault.xml",
    "Data/Levels/og_story/23_Rock_Arena.xml",
    "Data/Levels/og_story/lukas-test08.xml",
    "Data/Levels/og_story/27_Sky_Ark_David.xml",
    "Data/Levels/og_story/test05.xml",
    "Data/Levels/og_story/test15.xml",
    "Data/Levels/og_story/22_Waterfall_Arena.xml",
    "Data/Levels/og_story/test11.xml",
    "Data/Levels/og_story/16_Tree_Rescue.xml",
    "Data/Levels/og_story/test23.xml",
    "Data/Levels/og_story/lukas-test17.xml",
    "Data/Levels/og_story/12_Canyon_Ambush.xml",
    "Data/Levels/og_story/test37.xml",
    "Data/Levels/og_story/test04.xml",
    "Data/Levels/og_story/lukas-test31.xml",
    "Data/Levels/og_story/lukas-test26.xml",
    "Data/Levels/og_story/20_Volcano_Mine.xml",
    "Data/Levels/og_story/lukas-test16.xml",
    "Data/Levels/og_story/13_Rat_Slavers.xml",
    "Data/Levels/og_story/lukas-test15.xml",
    "Data/Levels/og_story/lukas-test20.xml",
    "Data/Levels/og_story/test14.xml",
    "Data/Levels/og_story/test17.xml",
    "Data/Levels/og_story/test32.xml",
    "Data/Levels/og_story/lukas-test25.xml",
    "Data/Levels/og_story/test09.xml",
    "Data/Levels/og_story/test13_terrain.xml",
    "Data/Levels/og_story/test19.xml",
    "Data/Levels/og_story/21_Waterfall_Barracks.xml",
    "Data/Levels/og_story/02_Slaver_Camp.xml",
    "Data/Levels/og_story/lukas-test11.xml",
    "Data/Levels/og_story/test12.xml",
    "Data/Levels/og_story/test30.xml",
    "Data/Levels/og_story/test16.xml",
    "Data/Levels/og_story/lukas-test35.xml",
    "Data/Levels/og_story/lukas-test04.xml",
    "Data/Levels/og_story/test20.xml",
    "Data/Levels/og_story/test01Stucco.xml",
    "Data/Levels/og_story/test27.xml",
    "Data/Levels/og_story/19e_Magma_Arena_Wolf.xml",
    "Data/Levels/og_story/22b_Waterfall_Barracks_return.xml",
    "Data/Levels/og_story/lukas-test21.xml",
    "Data/Levels/og_story/lukas-test18.xml",
    "Data/Levels/og_story/26_Sky_Ark_Cutscene.xml",
    "Data/Levels/og_story/17_Rat_HQ.xml",
    "Data/Levels/og_story/lukas-test29.xml",
    "Data/Levels/og_story/lukas-test19.xml",
    "Data/Levels/og_story/test35.xml",
    "Data/Levels/og_story/24_Forced_Fight.xml",
    "Data/Levels/og_story/13c_Rat_Slavers.xml",
    "Data/Levels/og_story/test21.xml",
    "Data/Levels/og_story/lukas-test14.xml",
    "Data/Levels/og_story/test08.xml",
    "Data/Levels/og_story/test29.xml",
    "Data/Levels/og_story/09_Dog_Fort_Rescue.xml",
    "Data/Levels/og_story/22_Waterfall_Arena_1v1.xml",
    "Data/Levels/og_story/05_Watchtower_Build_Site_Rebuild.xml",
    "Data/Levels/og_story/19d_Magma_Arena_ffa.xml",
    "Data/Levels/og_story/lukas-test30.xml",
    "Data/Levels/og_story/test22.xml",
    "Data/Levels/og_story/23_Rock_Arena_2v2.xml",
    "Data/Levels/og_story/test28.xml",
    "Data/Levels/og_story/08_Ice_Cliff_Landing.xml",
    "Data/Levels/og_story/13b_Rat_Slavers.xml",
    "Data/Levels/og_story/08_Ice_Cliff_Landing_Crete.xml",
    "Data/Levels/og_story/lukas-test28.xml",
    "Data/Levels/og_story/test33.xml",
    "Data/Levels/og_story/lukas-test07.xml",
    "Data/Levels/og_story/test07.xml",
    "Data/Levels/og_story/lukas-test34.xml",
    "Data/Levels/og_story/test31.xml",
    "Data/Levels/og_story/10_Storm_Troopers.xml",
    "Data/Levels/og_story/test36.xml",
    "Data/Levels/og_story/19b_Magma_Arena_2v2.xml",
    "Data/Levels/og_story/test01.xml",
    "Data/Levels/og_story/06_Occupied_Farm.xml",
    "Data/Levels/og_story/lukas-test06.xml",
    "Data/Levels/og_story/test34.xml",
    "Data/Levels/og_story/04_Main_Slaver_Camp.xml",
    "Data/Levels/og_story/05_Watchtower_Build_Site.xml",
    "Data/Levels/og_story/15_Bayou.xml",
    NULL};

// Update the simulation
void Engine::Update() {
    game_timer.UpdateWallTime();

    PROFILER_ZONE(g_profiler_ctx, "Update");

    if (check_save_level_changes_dialog_is_showing) {
        // Wait for dialog to be complete
        UpdateControls(ui_timer.timestep, false);
        return;
    }

#ifdef WIN32
    {
        PROFILER_ZONE(g_profiler_ctx, "Wait for object updates");
        if (WaitForSingleObject(data_change_notification, 0) == WAIT_OBJECT_0 || WaitForSingleObject(write_change_notification, 0) == WAIT_OBJECT_0) {
            RequestLiveUpdate();
            while (WaitForSingleObject(data_change_notification, 0) == WAIT_OBJECT_0) {
                FindNextChangeNotification(data_change_notification);
            }
            while (write_change_notification != INVALID_HANDLE_VALUE && WaitForSingleObject(write_change_notification, 0) == WAIT_OBJECT_0) {
                FindNextChangeNotification(write_change_notification);
            }
        }
    }
#endif

#if ENABLE_STEAMWORKS
    {
        PROFILER_ZONE(g_profiler_ctx, "Steam update");
        Steamworks::Instance()->Update(current_engine_state_.type != kEngineLevelState && current_engine_state_.type != kEngineEditorLevelState);
    }
#endif

    while (queued_engine_state_.size() > 0) {
        bool continue_state_change = true;

        EngineState new_engine_state;

        bool found = false;
        std::deque<EngineState> history_copy = state_history;
        std::deque<EngineState> final_pop;

        LOGI << "Checking if we actually allow any state changes" << std::endl;

        switch (current_engine_state_.type) {
            case kEngineLevelState:
            case kEngineEditorLevelState:
                if (scenegraph_) {
                    if (!check_save_level_changes_dialog_is_finished && !check_save_level_changes_dialog_is_showing) {
                        // Trigger save dialog
                        Input::Instance()->SetGrabMouse(false);
                        check_save_level_changes_dialog_is_showing = true;
                        check_save_level_changes_dialog_quit_if_not_cancelled = false;
                        check_save_level_changes_dialog_is_finished = false;
                        return;
                    }

                    continue_state_change = check_save_level_changes_last_result;

                    // Clear dialog state
                    check_save_level_changes_dialog_is_showing = false;
                    check_save_level_changes_dialog_quit_if_not_cancelled = false;
                    check_save_level_changes_dialog_is_finished = false;
                    check_save_level_changes_last_result = false;
                }
                break;
            case kEngineScriptableUIState:
                break;
            case kEngineCampaignState:
                break;
            case kEngineNoState:
                break;
        }
        bool pushed_current_to_history = false;

        if (continue_state_change) {
            LOGI << "Performing a state queue request " << std::endl;

            EngineStateAction esa = queued_engine_state_.front();
            queued_engine_state_.pop_front();
            std::deque<EngineState> local_popped_past;

            switch (esa.type) {
                case kEngineStateActionPopState:
                    while (found == false) {
                        EngineState candidate;
                        if (!history_copy.empty()) {
                            candidate = history_copy.front();
                            history_copy.pop_front();
                            if (candidate.pop_past == false) {
                                found = true;
                                LOGI << "Popping state to " << candidate << std::endl;
                                new_engine_state = candidate;
                                state_history = history_copy;
                                final_pop = local_popped_past;
                            } else {
                                LOGI << "Popping and tossing state " << candidate << std::endl;
                                local_popped_past.push_front(candidate);
                            }
                        } else {
                            if (esa.allow_game_exit) {
                                Input::Instance()->RequestQuit();
                            }
                            found = true;
                        }
                    }
                    break;
                case kEngineStateActionPopUntilType:
                    while (found == false) {
                        EngineState candidate;
                        if (!history_copy.empty()) {
                            candidate = history_copy.front();
                            history_copy.pop_front();

                            if (candidate.type == esa.state.type && candidate.pop_past == false) {
                                found = true;
                                LOGI << "Popping state to " << candidate << std::endl;
                                new_engine_state = candidate;
                                state_history = history_copy;
                                final_pop = local_popped_past;
                            } else {
                                LOGI << "Popping and tossing state " << candidate << std::endl;
                                local_popped_past.push_front(candidate);
                            }
                        } else {
                            if (esa.allow_game_exit) {
                                Input::Instance()->RequestQuit();
                            }
                            found = true;
                        }
                    }
                    break;
                case kEngineStateActionPopUntilID:
                    while (found == false) {
                        EngineState candidate;
                        if (!history_copy.empty()) {
                            candidate = history_copy.front();
                            history_copy.pop_front();

                            if (candidate.id == esa.state.id && candidate.pop_past == false) {
                                found = true;
                                LOGI << "Popping state to " << candidate << std::endl;
                                new_engine_state = candidate;
                                state_history = history_copy;
                                final_pop = local_popped_past;
                            } else {
                                LOGI << "Popping and tossing state " << candidate << std::endl;
                                local_popped_past.push_front(candidate);
                            }
                        } else {
                            if (esa.allow_game_exit) {
                                Input::Instance()->RequestQuit();
                            }
                            found = true;
                        }
                    }

                    break;
                case kEngineStateActionPushState:
                    state_history.push_front(current_engine_state_);
                    pushed_current_to_history = true;
                    new_engine_state = esa.state;
                    break;
            }

            if (pushed_current_to_history == false) {
                final_pop.push_front(current_engine_state_);
            }

            // Trigger final destruction of engine state
            while (final_pop.size() > 0) {
                EngineState final_state_pop = final_pop.front();
                LOGI << "There are some final popping to do of " << final_state_pop << std::endl;
                final_pop.pop_front();
                switch (final_state_pop.type) {
                    case kEngineLevelState:
                    case kEngineEditorLevelState:
                        Input::Instance()->SetUpForXPlayers(0);
                        current_menu_player = -1;
                        break;
                    case kEngineScriptableUIState:
                        break;
                    case kEngineCampaignState:
                        final_state_pop.campaign->LeaveCampaign();
                        break;
                    case kEngineNoState:
                        LOGE << "No state" << std::endl;
                        break;
                }
            }

            // We want to load something new!
            if (new_engine_state.type != kEngineNoState) {
                cursor.SetCursor(DEFAULT_CURSOR);
                LOGI << "Switching game state" << std::endl;
                ScriptableCampaign* sc = GetCurrentCampaign();
                // Lets unload what we previously had loaded first.
                switch (current_engine_state_.type) {
                    case kEngineLevelState:
                    case kEngineEditorLevelState:
                        if (sc) {
                            sc->LeaveLevel();
                        }
                        ClearLoadedLevel();
                        Input::Instance()->SetGrabMouse(false);
                        break;
                    case kEngineScriptableUIState:
                        if (scriptable_menu_ != NULL) {
                            delete scriptable_menu_;
                            scriptable_menu_ = NULL;
                            IMElement::DestroyQueuedIMElements();
                            LOGI << "Disposing of scriptable menu" << std::endl;
                            IMrefCountTracker.logSanityCheck();
                        }
                        break;
                    case kEngineCampaignState:
                        break;
                    case kEngineNoState:
                        LOGE << "No state" << std::endl;
                        break;
                }

                current_engine_state_ = new_engine_state;

                // Now load/initialize what is requested.
                switch (current_engine_state_.type) {
                    case kEngineLevelState:
                    case kEngineEditorLevelState:
                        Engine::Instance()->GetSound()->TransitionToSong("overgrowth_silence");
                        if (current_engine_state_.path.isValid()) {
                            QueueLevelToLoad(current_engine_state_.path);
                        } else {
                            ScriptableCampaign* sc = GetCurrentCampaign();
                            if (sc) {
                                std::string campaign_id = sc->GetCampaignID();
                                ModInstance::Campaign camp = ModLoading::Instance().GetCampaign(campaign_id);

                                for (auto& level : camp.levels) {
                                    if (strmtch(level.id, current_engine_state_.id)) {
                                        std::string short_path = std::string("Data/Levels/") + std::string(level.path);

                                        if (FileExists(short_path, kAnyPath)) {
                                            Path path = FindFilePath(short_path, kAnyPath);
                                            QueueLevelToLoad(path);
                                        } else {
                                            LOGE << "Unable to find level: " << short_path << ". Will not change state" << std::endl;
                                            EngineStateAction esa;
                                            esa.type = kEngineStateActionPopState;
                                            esa.allow_game_exit = true;
                                            queued_engine_state_.push_back(esa);
                                        }
                                    }
                                }
                            } else {
                                LOGE << "Currently no campaign is active, can't load level with id: " << current_engine_state_.id << std::endl;
                                EngineStateAction esa;
                                esa.type = kEngineStateActionPopState;
                                esa.allow_game_exit = true;
                                queued_engine_state_.push_back(esa);
                            }
                        }
                        Shaders::Instance()->create_program_warning = false;
                        break;
                    case kEngineScriptableUIState: {
                        UIShowCursor(0);
                        scriptable_menu_ = new ScriptableUI((void*)this, &Engine::StaticScriptableUICallback);
                        ASData as_data;
                        as_data.scenegraph = scenegraph_;
                        as_data.gui = &gui;
                        Path script_path = current_engine_state_.path;
                        scriptable_menu_->Initialize(script_path, as_data, break_on_script_change);
                        latest_menu_path_ = script_path;

                        // Display popups queued while we didn't have a menu
                        while (!popup_pueue.empty()) {
                            scriptable_menu_->QueueBasicPopup(std::get<0>(popup_pueue.front()), std::get<1>(popup_pueue.front()));
                            popup_pueue.pop_front();
                        }
                        break;
                    }
                    case kEngineCampaignState:
                        current_engine_state_.campaign->EnterCampaign();
                        break;
                    case kEngineNoState:
                        LOGE << "No state" << std::endl;
                        break;
                }
            }
        } else {
            queued_engine_state_.clear();
        }
    }

    if (queued_level_.isValid()) {
        LoadLevel(queued_level_);
        queued_level_ = Path();
        PROFILER_NAME_TIMELINE(g_profiler_ctx, "Gameplay");

        if (level_loaded_) {
            if (scenegraph_) {
                if (scenegraph_->map_editor->state_ != MapEditor::kInGame) {
                    cursor.SetVisible(true);
                }
            }

            ScriptableCampaign* sc = GetCurrentCampaign();
            if (sc) {
                sc->EnterLevel();
            }
        } else {
            EngineStateAction esa;
            esa.type = kEngineStateActionPopState;
            esa.allow_game_exit = true;
            queued_engine_state_.push_back(esa);
        }
    }
    int num_ui_timesteps = ui_timer.GetStepsNeeded();
    num_ui_timesteps = min(num_ui_timesteps, _max_steps_per_frame);
    ui_timer.updates_since_last_frame = num_ui_timesteps;

    if (Input::Instance()->WasQuitRequested()) {
        if (scenegraph_) {
            if (!check_save_level_changes_dialog_is_finished && !check_save_level_changes_dialog_is_showing) {
                // Trigger save dialog
                Input::Instance()->SetGrabMouse(false);
                check_save_level_changes_dialog_is_showing = true;
                check_save_level_changes_dialog_quit_if_not_cancelled = true;
                check_save_level_changes_dialog_is_finished = false;
                return;
            }

            quitting_ = check_save_level_changes_last_result;

            // Clear dialog state
            check_save_level_changes_dialog_is_showing = false;
            check_save_level_changes_dialog_quit_if_not_cancelled = false;
            check_save_level_changes_dialog_is_finished = false;
            check_save_level_changes_last_result = false;
        } else {
            quitting_ = true;
        }
        Input::Instance()->ClearQuitRequested();
    }
    sound.UpdateGameTimestep(game_timer.timestep);
    sound.Update();

    if (Online::Instance()->IsAwaitingShutdown()) {
        Online::Instance()->StopMultiplayer();
    }

    if (Online::Instance()->IsActive()) {
        Online::Instance()->CheckPendingMessages();
    }

    if (!level_loaded_ || waiting_for_input_) {
        GetAssetManager()->SetLoadWarningMode(false, "", "");
        // Main menu loop
        for (int curr_step = 0; curr_step < num_ui_timesteps; curr_step++) {
            UpdateControls(ui_timer.timestep, false);
            ui_timer.Update();
            if (scriptable_menu_ && !waiting_for_input_) {
                scriptable_menu_->Update();
                if (scriptable_menu_->IsDeleteScheduled()) {
                    delete scriptable_menu_;
                    scriptable_menu_ = NULL;
                }
            }
        }
        sound.UpdateGameTimescale(1.0f);
        if (waiting_for_input_) {
            waiting_for_input_ = false;
            bool skip_loading_pause = config["skip_loading_pause"].toNumber<bool>();
            bool no_dialogues = config["no_dialogues"].toNumber<bool>();
            if (last_loading_input_time < finished_loading_time &&
                (load_screen_tip[0] != '\0' || Online::Instance()->IsActive()) &&
                !skip_loading_pause &&
                !no_dialogues &&
                !interrupt_loading) {
                waiting_for_input_ = true;
            }

            // Clients should never be allowed to end the loading screen themselves
            // Multiplayer->ForceMapStartOnLoad() is the authority on level loading in Multiplayer (for the client)
            if (Online::Instance()->IsClient()) {
                waiting_for_input_ = !Online::Instance()->ForceMapStartOnLoad();
            }
        }
    } else {
        if (scenegraph_->map_editor->state_ == MapEditor::kInGame) {
            GetAssetManager()->SetLoadWarningMode(true, "Main Game Mode", scenegraph_->level_path_.GetOriginalPath());
        } else {
            GetAssetManager()->SetLoadWarningMode(false, "", "");
        }
        // Fixed time steps
        const std::vector<ASContext*>& active_contexts = ASProfiler::GetActiveContexts();
        if (paused) {
            for (int curr_step = 0; curr_step < num_ui_timesteps; curr_step++) {
                PROFILER_ZONE(g_profiler_ctx, "Timestep");
                ui_timer.Update();
                avatar_control_manager.Update();
                DebugDraw::Instance()->Update(game_timer.timestep);
                DebugDrawAux::Instance()->Update(game_timer.timestep);
                UpdateControls(ui_timer.timestep, false);
                ActiveCameras::Get()->getCameraObject()->Update(ui_timer.timestep);
                if (scenegraph_->map_editor->state_ != MapEditor::kInGame) {
                    PROFILER_ZONE(g_profiler_ctx, "Map editor update");
                    scenegraph_->map_editor->Update(&cursor);
                }
                {
                    PROFILER_ZONE(g_profiler_ctx, "Level update");
                    scenegraph_->level->Update(paused);
                }

                for (auto& movement_object : scenegraph_->movement_objects_) {
                    MovementObject* mo = (MovementObject*)movement_object;
                    if (mo->controlled) {
                        mo->UpdatePaused();
                    }
                }
                HandleRabbotToggleControls();

                for (auto active_context : active_contexts) {
                    active_context->profiler.Update();
                }
            }
        } else {
            int num_game_timesteps = game_timer.GetStepsNeeded();
            num_game_timesteps = min(num_game_timesteps, _max_steps_per_frame);
            game_timer.updates_since_last_frame = num_game_timesteps;

            for (int curr_step = 0; curr_step < num_ui_timesteps; curr_step++) {
                ui_timer.Update();
            }
            for (int curr_step = 0; curr_step < num_game_timesteps; curr_step++) {
                PROFILER_ZONE(g_profiler_ctx, "Timestep");
                {
                    PROFILER_ZONE(g_profiler_ctx, "UpdateControlledModule");
                    avatar_control_manager.Update();
                }
                {
                    PROFILER_ZONE(g_profiler_ctx, "Update debugdraw");
                    DebugDraw::Instance()->Update(game_timer.timestep);
                    DebugDrawAux::Instance()->Update(game_timer.timestep);
                }

                if (!paused) {
                    PROFILER_ZONE(g_profiler_ctx, "Update timer");
                    game_timer.Update();
                }
                {
                    PROFILER_ZONE(g_profiler_ctx, "Update controls");
                    UpdateControls(game_timer.timestep, false);
                }
                {
                    PROFILER_ZONE(g_profiler_ctx, "Update flares");
                    scenegraph_->flares.Update(game_timer.timestep);
                }

                if (!paused) {
                    PROFILER_ZONE(g_profiler_ctx, "Scenegraph update");
                    scenegraph_->Update(game_timer.timestep, game_timer.game_time);
                } else {
                    ActiveCameras::Get()->getCameraObject()->Update(game_timer.timestep);
                }

                // if host, go over all changes and send over socket.
                // client, go over all updates for env objects
                if (Online::Instance()->IsActive()) {
                    Online::Instance()->UpdateObjects(scenegraph_);
                }

                if (Online::Instance()->IsActive()) {
                    for (auto eo : scenegraph_->visible_static_meshes_) {
                        eo->Update(game_timer.timestep);
                    }
                }

                if (scenegraph_->map_editor->state_ != MapEditor::kInGame) {
                    PROFILER_ZONE(g_profiler_ctx, "Map editor update");
                    scenegraph_->map_editor->Update(&cursor);
                }

                if (kLightProbeEnabled) {
                    // TODO: move alongside other editor input processing?
                    if (KeyCommand::CheckPressed(Input::Instance()->getKeyboard(), KeyCommand::kBakeGI, KIMF_MENU | KIMF_LEVEL_EDITOR_GENERAL)) {
                        scenegraph_->map_editor->BakeLightProbes(0);
                    }
                }
                if (!paused) {
                    PROFILER_ZONE(g_profiler_ctx, "Animation effects update");
                    particle_types.Update(game_timer.timestep);
                    // AnimationEffects::Instance()->Update(game_timer.timestep);
                }
                {
                    PROFILER_ZONE(g_profiler_ctx, "Level update");
                    scenegraph_->level->Update(paused);
                }
                HandleRabbotToggleControls();
                // Disposing of an object might in turn queue up more items
                // to delete, so make we loop until we're at the end of
                // object_ids_to_delete
                for (int i : scenegraph_->object_ids_to_delete) {
                    scenegraph_->map_editor->DeleteID(i);
                }
                scenegraph_->object_ids_to_delete.clear();

                for (auto active_context : active_contexts) {
                    active_context->profiler.Update();
                }
            }
            sound.UpdateGameTimescale(powf(game_timer.time_scale / current_global_scale_mult, 0.5f));
        }

        level_updated_ = min(10, level_updated_ + 1);
    }

    ScriptableCampaign* sc = GetCurrentCampaign();
    if (sc) {
        sc->Update();
    }

    if (!IsStateQueued() && config["quit_after_load"].toBool()) {
        if (shutdown_timer < 0) {
            Input::Instance()->RequestQuit();
        }

        shutdown_timer--;
    }

    asset_manager.Update();
    as_network.Update();

    UpdateImGui();
    if (Online::Instance() != nullptr) {
        Online::Instance()->LateUpdate(GetSceneGraph());
    }

    static int next_load_stress_level = 0;
    if (!cache_generation_paths.empty() && level_load_settle_frame_count == 0) {
        back_to_menu = true;

        EngineState newState("IsThisUsed", kEngineLevelState, cache_generation_paths.front());
        QueueState(newState);

        cache_generation_paths.erase(cache_generation_paths.begin());
        level_load_settle_frame_count = 60 * 4;
    } else if (level_load_settle_frame_count > 0) {
        --level_load_settle_frame_count;
    } else if (runtime_level_load_stress) {
        if (!config["level_load_stress"].toNumber<bool>()) {
            runtime_level_load_stress = false;
        } else {
            if (stress_levels[next_load_stress_level] == NULL) {
                next_load_stress_level = 0;
            }
            if (stress_levels[next_load_stress_level] != NULL) {
                if (queued_engine_state_.size() == 0) {
                    Path level = FindFilePath(stress_levels[next_load_stress_level]);
                    if (level.isValid()) {
                        QueueLevelCacheGeneration(level);
                    }
                    next_load_stress_level++;
                }
            }
        }
    } else if (back_to_menu) {
        cache_generation_paths.clear();

        EngineStateAction esa;
        esa.type = kEngineStateActionPopUntilType;
        esa.allow_game_exit = false;
        esa.state.type = kEngineScriptableUIState;
        QueueState(esa);
        back_to_menu = false;
    }

#if MONITOR_MEMORY
    if ((frame_counter % (60)) == 0) {
        LOGI.Format("malloc Calls This Frame: %d\n", GetAndResetMallocAllocationCount());
        LOGI.Format("malloc Current Allocations: %d\n", GetCurrentNumberOfMallocAllocations());
        for (int i = 0; i < OG_MALLOC_TYPE_COUNT; i++) {
            uint64_t mem_cb = GetCurrentTotalMallocAllocation(i);
            if (mem_cb >= 1024) {
                LOGI.Format("malloc Memory Use (%s): %u KiB\n", OgMallocTypeString(i), (unsigned)(mem_cb / 1024));
            } else {
                LOGI.Format("malloc Memory Use (%s): %u B\n", OgMallocTypeString(i), (unsigned)mem_cb);
            }
        }
    }
#endif

    save_file_.ExecuteQueuedWrite();

    if (current_engine_state_.type != kEngineLevelState) {
        if (live_update_countdown > 0 && --live_update_countdown == 0) {
            if (config["enable_live_update"].toBool() && frame_counter > 30) {
                LiveUpdate(scenegraph_, &asset_manager, scriptable_menu_);
            }
        }
    }

    IMElement::DestroyQueuedIMElements();

    frame_counter++;

    DisplayLastQueuedError();

    if (queued_engine_state_.size() > 0) {  // Avoid drawing bad frames, e.g. when loading level from dialogue
        Update();
    }
}

std::vector<vec3> sphere_points;

static TextureRef histogram_tex;
static const int kHistogramBuckets = 256;
static const int kHistogramRes = 128;
static const bool kDrawHistogram = false;

struct FramebufferObject {
    GLuint id;
    TextureRef bound_tex;
};

static void CreateHistogram(const TextureRef& color_tex) {
    Shaders* shaders = Shaders::Instance();
    Textures* textures = Textures::Instance();
    Graphics* graphics = Graphics::Instance();

    static FramebufferObject framebuffer;
    static int shader_id;
    static int vert_attrib_id;
    // static int buckets_uniform_id;
    static VBOContainer pixel_vbo;
    if (!histogram_tex.valid()) {
        histogram_tex = textures->makeTexture(kHistogramBuckets, 1, GL_RGBA16F, GL_RGBA);
        textures->SetTextureName(histogram_tex, "Histogram");
        graphics->genFramebuffers(&framebuffer.id, "histogram");
        CHECK_GL_ERROR();
        graphics->PushFramebuffer();
        graphics->RenderFramebufferToTexture(framebuffer.id, histogram_tex);
        CHECK_GL_ERROR();
        framebuffer.bound_tex = histogram_tex;
        graphics->PopFramebuffer();
        CHECK_GL_ERROR();
        shader_id = shaders->returnProgram("histogram_create");
        shaders->setProgram(shader_id);
        vert_attrib_id = shaders->returnShaderAttrib("pixel_uv", shader_id);
        CHECK_GL_ERROR();
        std::vector<GLfloat> pixel_data;
        pixel_data.resize(kHistogramRes * kHistogramRes * 2);
        for (int i = 0, index = 0; i < kHistogramRes; ++i) {
            for (int j = 0; j < kHistogramRes; ++j) {
                pixel_data[index + 0] = (j + 0.5f) / (float)kHistogramRes;
                pixel_data[index + 1] = (i + 0.5f) / (float)kHistogramRes;
                index += 2;
            }
        }
        pixel_vbo.Fill(kVBOFloat | kVBOStatic, pixel_data.size() * sizeof(GLfloat), &pixel_data[0]);
        CHECK_GL_ERROR();
    }
    CHECK_GL_ERROR();
    graphics->PushViewport();
    CHECK_GL_ERROR();
    graphics->setViewport(0, 0, kHistogramBuckets, 1);
    GLState gl_state;
    gl_state.blend = true;
    gl_state.depth_test = false;
    gl_state.depth_write = false;
    gl_state.cull_face = false;
    gl_state.blend_src = GL_SRC_ALPHA;
    gl_state.blend_dst = GL_ONE;
    graphics->setGLState(gl_state);
    CHECK_GL_ERROR();
    graphics->PushFramebuffer();
    CHECK_GL_ERROR();
    graphics->bindFramebuffer(framebuffer.id);
    CHECK_GL_ERROR();
    shaders->setProgram(shader_id);
    textures->bindTexture(color_tex);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    CHECK_GL_ERROR();
    graphics->EnableVertexAttribArray(vert_attrib_id);
    CHECK_GL_ERROR();
    pixel_vbo.Bind();
    glVertexAttribPointer(vert_attrib_id, 2, GL_FLOAT, false, 2 * sizeof(GLfloat), 0);
    CHECK_GL_ERROR();
    glPointSize(1);
    graphics->DrawArrays(GL_POINTS, 0, kHistogramRes * kHistogramRes);
    graphics->ResetVertexAttribArrays();
    CHECK_GL_ERROR();

    graphics->BindArrayVBO(0);
    graphics->PopFramebuffer();
    graphics->PopViewport();
}

static void DrawHistogram() {
    Shaders* shaders = Shaders::Instance();
    Textures* textures = Textures::Instance();
    Graphics* graphics = Graphics::Instance();

    static int shader_id = -1;
    static VBOContainer vbo;
    if (shader_id == -1) {
        shader_id = shaders->returnProgram("histogram_draw");
        CHECK_GL_ERROR();
        std::vector<GLfloat> data;
        data.resize(kHistogramBuckets * 4);
        for (int i = 0, index = 0; i < kHistogramBuckets; ++i) {
            data[index + 0] = (float)i;
            data[index + 1] = 0.0f;
            data[index + 2] = (float)i;
            data[index + 3] = 1.0f;
            index += 4;
        }
        vbo.Fill(kVBOFloat | kVBOStatic, data.size() * sizeof(GLfloat), &data[0]);
        CHECK_GL_ERROR();
    }

    GLState gl_state;
    gl_state.blend = true;
    gl_state.depth_test = false;
    gl_state.depth_write = false;
    gl_state.cull_face = false;
    graphics->setGLState(gl_state);
    shaders->setProgram(shader_id);
    int vert_attrib_id = shaders->returnShaderAttrib("lines", shader_id);
    int uniform_mvp_id = shaders->returnShaderVariable("mvp", shader_id);
    int uniform_buckets_id = shaders->returnShaderVariable("num_buckets", shader_id);
    glm::mat4 proj_mat;
    proj_mat = glm::ortho(0.0f, (float)graphics->viewport_dim[2], 0.0f, (float)graphics->viewport_dim[3]);
    glm::mat4 mvp_mat = proj_mat;
    shaders->SetUniformInt(uniform_buckets_id, kHistogramBuckets);
    shaders->SetUniformMat4(uniform_mvp_id, (const GLfloat*)&mvp_mat);
    textures->bindTexture(histogram_tex);
    graphics->EnableVertexAttribArray(vert_attrib_id);
    vbo.Bind();
    glVertexAttribPointer(vert_attrib_id, 2, GL_FLOAT, false, 2 * sizeof(GLfloat), 0);
    glLineWidth(1);
    graphics->DrawArrays(GL_LINES, 0, kHistogramBuckets * 2);
    graphics->ResetVertexAttribArrays();
    graphics->BindArrayVBO(0);
}

static void DrawQuad(int shader_id) {
    PROFILER_GPU_ZONE(g_profiler_ctx, "Engine.cpp DrawQuad");
    Shaders* shaders = Shaders::Instance();
    Graphics* graphics = Graphics::Instance();
    int vert_attrib_id = shaders->returnShaderAttrib("vert_attrib", shader_id);
    graphics->EnableVertexAttribArray(vert_attrib_id);
    glVertexAttribPointer(vert_attrib_id, 2, GL_FLOAT, false, 2 * sizeof(GLfloat), 0);
    graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    graphics->ResetVertexAttribArrays();
}

void ApplyPostEffects(std::string post_shader, Engine::DrawingViewport drawing_viewport, vec2 active_screen_start, vec2 active_screen_end, Engine::PostEffectsType post_effects_type, const VBOContainer& quad_vert_vbo, const VBOContainer& quad_index_vbo) {
    ;
    Shaders* shaders = Shaders::Instance();
    Textures* textures = Textures::Instance();
    Graphics* graphics = Graphics::Instance();
    Camera* camera = ActiveCameras::Get();

    PROFILER_GPU_ZONE(g_profiler_ctx, "Apply post effects");
    graphics->PopFramebuffer();

    if (graphics->multisample_framebuffer_exists) {
        PROFILER_GPU_ZONE(g_profiler_ctx, "Blit MSAA color buffer");
        graphics->BlitColorBuffer();
    }

    if (kDrawHistogram) {
        CreateHistogram(graphics->screen_color_tex);
    }

    GLState gl_state;
    gl_state.blend = false;
    gl_state.cull_face = false;
    gl_state.depth_test = false;
    gl_state.depth_write = false;
    graphics->setGLState(gl_state);

    // Blur screen
    quad_vert_vbo.Bind();
    quad_index_vbo.Bind();
    CHECK_GL_ERROR();

    if (post_effects_type == Engine::kStraight) {
        PROFILER_GPU_ZONE(g_profiler_ctx, "Draw unfiltered screen quad");
        // Draw unfiltered HDR scene (e.g. for reflection capture)
        int shader_id = shaders->returnProgram(post_shader);
        graphics->PushViewport();
        if (drawing_viewport != Engine::kViewport) {
            graphics->setViewport(0, 0, graphics->window_dims[0], graphics->window_dims[1]);
        } else {
            glViewport(0, 0, graphics->window_dims[0], graphics->window_dims[1]);
        }
        shaders->setProgram(shader_id);
        textures->bindTexture(graphics->screen_color_tex);
        // glEnable(GL_FRAMEBUFFER_SRGB );
        DrawQuad(shader_id);
        // glDisable(GL_FRAMEBUFFER_SRGB );
        graphics->PopViewport();
        graphics->PopViewport();
    } /*else if(post_effects_type == Engine::kFinal && drawing_viewport == Engine::kViewport){
        PROFILER_GPU_ZONE(g_profiler_ctx, "Tone map only");
        int shader_id = shaders->returnProgram(post_shader + " #TONE_MAP");
        graphics->PushViewport();
        glViewport(0,0,graphics->window_dims[0],graphics->window_dims[1]);
        shaders->setProgram(shader_id);
        textures->bindTexture(graphics->screen_color_tex);
        glEnable(GL_FRAMEBUFFER_SRGB );
        DrawQuad(shader_id);
        glDisable(GL_FRAMEBUFFER_SRGB );
        graphics->PopViewport();
        graphics->PopViewport();
    } */
    else if (post_effects_type == Engine::kFinal) {
        PROFILER_GPU_ZONE(g_profiler_ctx, "Full post effects");
        static const GLbitfield all_buffers = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
        static const GLenum all_attachments[3] = {GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
        graphics->PushViewport();
        graphics->PushFramebuffer();

        graphics->setViewport(0, 0, graphics->render_dims[0], graphics->render_dims[1]);
        graphics->bindFramebuffer(graphics->post_effects.post_framebuffer);

        TextureRef final_texture = graphics->screen_color_tex;
        TextureRef temp_texture = graphics->post_effects.temp_screen_tex;

        graphics->EnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, false, 2 * sizeof(GLfloat), 0);

        float curr_time = game_timer.game_time + game_timer.GetInterpWeight() / 120.0f;
        if (graphics->config_.motion_blur_amount_ > 0.01f) {
            PROFILER_GPU_ZONE(g_profiler_ctx, "Motion blur");
            {
                mat4 temp = camera->GetViewMatrix();
                if (curr_time != prev_view_time) {
                    temp.SetTranslationPart(temp.GetTranslationPart() + camera->GetViewMatrix().GetRotatedvec3(ActiveCameras::Get()->GetCenterVel() * (curr_time - prev_view_time)));
                    float rotation_blur_amount = 0.125f;
                    temp = mix(temp, ActiveCameras::Get()->prev_view_mat, rotation_blur_amount);
                }
                int shader_id = shaders->returnProgram(post_shader + " #CALC_MOTION_BLUR");
                shaders->setProgram(shader_id);
                shaders->SetUniformInt("screen_width", graphics->render_dims[0]);
                shaders->SetUniformInt("screen_height", graphics->render_dims[1]);
                shaders->SetUniformMat4("proj_mat", camera->GetProjMatrix());
                shaders->SetUniformMat4("prev_view_mat", temp);  // ActiveCameras::Get()->prev_view_mat);
                shaders->SetUniformMat4("view_mat", camera->GetViewMatrix());
                if (curr_time != prev_view_time) {
                    shaders->SetUniformFloat("time_offset", curr_time - prev_view_time);
                } else {
                    shaders->SetUniformFloat("time_offset", game_timer.timestep);
                }

                textures->bindTexture(final_texture);
                textures->bindTexture(graphics->screen_vel_tex, 2);
                textures->bindTexture(graphics->pure_noise_ref, TEX_PURE_NOISE);
                textures->bindTexture(graphics->screen_depth_tex, TEX_SCREEN_DEPTH);
                graphics->framebufferColorTexture2D(temp_texture);
                graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }

            {  // Find dominant motion blur directions
                int intermediate_width = graphics->render_dims[0] / 4;
                int intermediate_height = graphics->render_dims[1] / 4;
                graphics->setViewport(0, 0, intermediate_width, intermediate_height);
                {  // Lower resolution of overbright for bloom
                    int shader_id = shaders->returnProgram(post_shader + " #DOWNSAMPLE");
                    shaders->setProgram(shader_id);
                    shaders->SetUniformInt("screen_width", graphics->render_dims[0]);
                    shaders->SetUniformInt("screen_height", graphics->render_dims[1]);
                    shaders->SetUniformFloat("src_lod", 0.0f);
                    textures->bindTexture(temp_texture);
                    graphics->framebufferColorTexture2D(temp_texture, 2);
                    graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                }

                // Downsample bloom again
                int downsampled_width = graphics->render_dims[0] / 16;
                int downsampled_height = graphics->render_dims[1] / 16;
                graphics->setViewport(0, 0, downsampled_width, downsampled_height);
                {
                    int shader_id = shaders->returnProgram(post_shader + " #DOWNSAMPLE");
                    shaders->setProgram(shader_id);
                    shaders->SetUniformInt("screen_width", intermediate_width);
                    shaders->SetUniformInt("screen_height", intermediate_height);
                    shaders->SetUniformFloat("src_lod", 2.0f);
                    textures->bindTexture(temp_texture);
                    graphics->framebufferColorTexture2D(temp_texture, 4);
                    graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                }

                // Apply separable guassian blur to further down-sampled overbright image
                {
                    int shader_id = shaders->returnProgram(post_shader + " #BLUR_HORZ");
                    shaders->setProgram(shader_id);
                    shaders->SetUniformInt("screen_width", downsampled_width);
                    shaders->SetUniformInt("screen_height", downsampled_height);
                    shaders->SetUniformFloat("src_lod", 4.0f);
                    textures->bindTexture(temp_texture);
                    graphics->framebufferColorTexture2D(final_texture, 4);
                    graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                }
                {
                    int shader_id = shaders->returnProgram(post_shader + " #BLUR_VERT");
                    shaders->setProgram(shader_id);
                    shaders->SetUniformInt("screen_width", downsampled_width);
                    shaders->SetUniformInt("screen_height", downsampled_height);
                    shaders->SetUniformFloat("src_lod", 4.0f);
                    textures->bindTexture(final_texture);
                    graphics->framebufferColorTexture2D(temp_texture, 4);
                    graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                }
                graphics->setViewport(0, 0, graphics->render_dims[0], graphics->render_dims[1]);
            }

            {
                int shader_id = shaders->returnProgram(post_shader + " #APPLY_MOTION_BLUR");
                shaders->setProgram(shader_id);
                shaders->SetUniformInt("screen_width", graphics->render_dims[0]);
                shaders->SetUniformInt("screen_height", graphics->render_dims[1]);
                shaders->SetUniformFloat("time_offset", curr_time - prev_view_time);
                shaders->SetUniformFloat("motion_blur_mult", graphics->config_.motion_blur_amount_);

                textures->bindTexture(final_texture);
                textures->bindTexture(temp_texture, 2);
                textures->bindTexture(graphics->pure_noise_ref, TEX_PURE_NOISE);
                textures->bindTexture(graphics->screen_depth_tex, TEX_SCREEN_DEPTH);
                graphics->framebufferColorTexture2D(graphics->screen_vel_tex);
                graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                final_texture = graphics->screen_vel_tex;
            }
        }

        // Depth of field
        if (graphics->config_.depth_of_field() && (camera->far_blur_amount > 0.0f || camera->near_blur_amount > 0.0f)) {
            PROFILER_GPU_ZONE(g_profiler_ctx, "Depth of field");
            int shader_id = graphics->config_.depth_of_field_reduced()
                                ? shaders->returnProgram(post_shader + " #DOF #DOF_LESS")
                                : shaders->returnProgram(post_shader + " #DOF");
            shaders->setProgram(shader_id);
            shaders->SetUniformInt("screen_width", graphics->render_dims[0]);
            shaders->SetUniformInt("screen_height", graphics->render_dims[1]);
            shaders->SetUniformFloat("time", game_timer.game_time);

            shaders->SetUniformFloat("near_blur_amount", camera->near_blur_amount);
            shaders->SetUniformFloat("far_blur_amount", camera->far_blur_amount);
            shaders->SetUniformFloat("near_sharp_dist", camera->near_sharp_dist);
            shaders->SetUniformFloat("far_sharp_dist", camera->far_sharp_dist);
            shaders->SetUniformFloat("near_blur_transition_size", camera->near_blur_transition_size);
            shaders->SetUniformFloat("far_blur_transition_size", camera->far_blur_transition_size);

            textures->bindTexture(final_texture);
            textures->bindTexture(graphics->pure_noise_ref, TEX_PURE_NOISE);
            textures->bindTexture(graphics->screen_depth_tex, TEX_SCREEN_DEPTH);
            graphics->framebufferColorTexture2D(temp_texture);
            graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            shader_id = shaders->returnProgram(post_shader + " #COPY");

            textures->bindTexture(temp_texture);
            graphics->framebufferColorTexture2D(final_texture);
            graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        CHECK_GL_ERROR();
        {  // Apply tone mapping -- compress dynamic range
            PROFILER_GPU_ZONE(g_profiler_ctx, "Tone map");
            int shader_id = shaders->returnProgram(post_shader + " #TONE_MAP");
            shaders->setProgram(shader_id);
            shaders->SetUniformInt("screen_width", graphics->render_dims[0]);
            shaders->SetUniformInt("screen_height", graphics->render_dims[1]);
            shaders->SetUniformFloat("saturation", Engine::Instance()->GetSceneGraph()->level->script_params().ASGetFloat("Saturation"));
            if (g_albedo_only) {
                shaders->SetUniformFloat("black_point", 0.0f);
                shaders->SetUniformFloat("white_point", 1.0f);
            } else {
                shaders->SetUniformFloat("black_point", graphics->hdr_black_point);
                shaders->SetUniformFloat("white_point", graphics->hdr_white_point);
            }
            shaders->SetUniformVec3("tint", camera->tint);
            shaders->SetUniformVec3("vignette_tint", camera->vignette_tint);
            textures->bindTexture(final_texture);
            graphics->framebufferColorTexture2D(graphics->post_effects.tone_mapped_tex);
            graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            final_texture = graphics->post_effects.tone_mapped_tex;
        }

        if (graphics->hdr_bloom_mult > 0.0f) {  // This is all bloom
            PROFILER_GPU_ZONE(g_profiler_ctx, "Bloom");
            CHECK_GL_ERROR();

            int intermediate_width = graphics->render_dims[0] / 4;
            int intermediate_height = graphics->render_dims[1] / 4;
            graphics->setViewport(0, 0, intermediate_width, intermediate_height);
            graphics->bindFramebuffer(graphics->post_effects.post_framebuffer);

            {  // Isolate pixels that are brighter than pure white
                int shader_id = shaders->returnProgram(post_shader + " #DOWNSAMPLE #OVERBRIGHT");
                shaders->setProgram(shader_id);
                shaders->SetUniformInt("screen_width", graphics->render_dims[0]);
                shaders->SetUniformInt("screen_height", graphics->render_dims[1]);
                if (g_albedo_only) {
                    shaders->SetUniformFloat("black_point", 0.0f);
                    shaders->SetUniformFloat("white_point", 1.0f);
                } else {
                    shaders->SetUniformFloat("black_point", graphics->hdr_black_point);
                    shaders->SetUniformFloat("white_point", graphics->hdr_white_point);
                }
                shaders->SetUniformFloat("bloom_mult", graphics->hdr_bloom_mult);
                shaders->SetUniformFloat("src_lod", 0.0f);
                textures->bindTexture(final_texture);
                graphics->framebufferColorTexture2D(temp_texture, 2);
                graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }

            {
                int shader_id = shaders->returnProgram(post_shader + " #BLUR_DIR");
                shaders->setProgram(shader_id);
                shaders->SetUniformInt("screen_width", intermediate_width);
                shaders->SetUniformInt("screen_height", intermediate_width);
                shaders->SetUniformFloat("src_lod", 2.0f);

                shaders->SetUniformInt("horz", 1);
                textures->bindTexture(temp_texture);
                graphics->framebufferColorTexture2D(final_texture, 2);
                graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                shaders->SetUniformInt("horz", 0);
                textures->bindTexture(final_texture);
                graphics->framebufferColorTexture2D(temp_texture, 2);
                graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }

            // Downsample bloom again
            int downsampled_width = graphics->render_dims[0] / 16;
            int downsampled_height = graphics->render_dims[1] / 16;
            graphics->setViewport(0, 0, downsampled_width, downsampled_height);
            graphics->bindFramebuffer(graphics->post_effects.post_framebuffer);
            {
                int shader_id = shaders->returnProgram(post_shader + " #DOWNSAMPLE");
                shaders->setProgram(shader_id);
                shaders->SetUniformInt("screen_width", intermediate_width);
                shaders->SetUniformInt("screen_height", intermediate_height);
                shaders->SetUniformFloat("src_lod", 2.0f);
                textures->bindTexture(temp_texture);
                graphics->framebufferColorTexture2D(temp_texture, 4);
                graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }

            // Apply separable guassian blur to further down-sampled overbright image
            {
                int shader_id = shaders->returnProgram(post_shader + " #BLUR_DIR");
                shaders->setProgram(shader_id);
                shaders->SetUniformInt("screen_width", downsampled_width);
                shaders->SetUniformInt("screen_height", downsampled_height);
                shaders->SetUniformFloat("src_lod", 4.0f);

                shaders->SetUniformInt("horz", 1);
                textures->bindTexture(temp_texture);
                graphics->framebufferColorTexture2D(final_texture, 4);
                graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                shaders->SetUniformInt("horz", 0);
                textures->bindTexture(final_texture);
                graphics->framebufferColorTexture2D(temp_texture, 4);
                graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }

            // Add downsampled and super downsampled bloom to the original tone-mapped image
            graphics->setViewport(0, 0, graphics->render_dims[0], graphics->render_dims[1]);
            {
                int shader_id = shaders->returnProgram(post_shader + " #ADD");
                shaders->setProgram(shader_id);
                textures->bindTexture(final_texture, TEX_TONE_MAPPED);
                textures->bindTexture(temp_texture, TEX_INTERMEDIATE);
                graphics->framebufferColorTexture2D(final_texture);
                graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }
            // final_texture = temp_texture;
        }
        const bool kMinZDepth = false;
        if (kMinZDepth) {
            glColorMask(false, false, false, false);
            graphics->setDepthWrite(true);
            graphics->setDepthTest(true);
            graphics->setDepthFunc(GL_ALWAYS);
            int width = graphics->render_dims[0];
            int height = graphics->render_dims[1];
            int level = 0;
            int shader_id = shaders->returnProgram(post_shader + " #DOWNSAMPLE_DEPTH");
            shaders->setProgram(shader_id);
            textures->bindTexture(graphics->screen_depth_tex);
            while (width > 1 && height > 1) {
                width = max(1, width / 2);
                height = max(1, height / 2);
                ++level;
                graphics->setViewport(0, 0, width, height);
                graphics->bindFramebuffer(graphics->post_effects.post_framebuffer);
                shaders->SetUniformInt("screen_width", width);
                shaders->SetUniformInt("screen_height", height);
                shaders->SetUniformFloat("src_lod", (float)(level - 1));
                graphics->framebufferColorTexture2D(final_texture, level);
                graphics->framebufferDepthTexture2D(graphics->screen_depth_tex, level);
                graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }
            glColorMask(true, true, true, true);
            graphics->setDepthWrite(false);
            graphics->setDepthTest(false);
            graphics->setDepthFunc(GL_LEQUAL);
        }

        graphics->PopFramebuffer();
        graphics->PopViewport();

        // glEnable(GL_SCISSOR_TEST);
        {
            PROFILER_GPU_ZONE(g_profiler_ctx, "Final pass");

            int shader_id;
            if (g_gamma_correct_final_output) {
                shader_id = shaders->returnProgram(post_shader + " #BRIGHTNESS #GAMMA_CORRECT_OUTPUT");
            } else {
                shader_id = shaders->returnProgram(post_shader + " #BRIGHTNESS");
            }

            shaders->setProgram(shader_id);
            graphics->setViewport((GLint)(active_screen_start[0] * graphics->window_dims[0]), (GLint)(active_screen_start[1] * graphics->window_dims[1]), (GLint)(active_screen_end[0] * graphics->window_dims[0]), (GLint)(active_screen_end[1] * graphics->window_dims[1]));
            int temp_viewport_dims[4];
            assert(sizeof(temp_viewport_dims) == sizeof(graphics->viewport_dim));
            memcpy(temp_viewport_dims, graphics->viewport_dim, sizeof(temp_viewport_dims));
            graphics->setViewport((GLint)0.0f, (GLint)0.0f, (GLint)graphics->window_dims[0], (GLint)graphics->window_dims[1]);
            glEnable(GL_SCISSOR_TEST);
            glScissor(temp_viewport_dims[0], temp_viewport_dims[1], temp_viewport_dims[2], temp_viewport_dims[3]);
            shaders->SetUniformInt("screen_width", graphics->config_.screen_width());
            shaders->SetUniformInt("screen_height", graphics->config_.screen_height());
            if (g_albedo_only) {
                shaders->SetUniformFloat("black_point", 0.0f);
                shaders->SetUniformFloat("white_point", 1.0f);
            } else {
                shaders->SetUniformFloat("black_point", graphics->hdr_black_point);
                shaders->SetUniformFloat("white_point", graphics->hdr_white_point);
            }
            shaders->SetUniformFloat("hdr_bloom_mult", graphics->hdr_bloom_mult);
            shaders->SetUniformFloat("time", game_timer.game_time);
            shaders->SetUniformVec3("tint", camera->tint);
            shaders->SetUniformFloat("brightness", config["brightness"].toNumber<float>());

            if (sphere_points.empty()) {
                while (sphere_points.size() < 32) {
                    vec3 point(RangedRandomFloat(-1.0f, 1.0f),
                               RangedRandomFloat(-1.0f, 1.0f),
                               RangedRandomFloat(-1.0f, 1.0f));
                    if (length_squared(point) <= 1.0f) {
                        sphere_points.push_back(normalize(point));
                    }
                }
            }
            shaders->SetUniformVec3Array("sphere_points", sphere_points);
            textures->bindTexture(final_texture);
            textures->bindTexture(graphics->screen_depth_tex, TEX_SCREEN_DEPTH);
            textures->bindTexture(graphics->noise_ref, TEX_NOISE);
            textures->bindTexture(graphics->pure_noise_ref, TEX_PURE_NOISE);
            // It appears this might trigger a crash on intel6XX,
            // so for now, we do gamma correction in the shader
            // via #GAMMA_CORRECT_OUTPUT
            // glEnable(GL_FRAMEBUFFER_SRGB );
            graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            // glDisable(GL_FRAMEBUFFER_SRGB );
        }
        graphics->PopViewport();
        graphics->ResetVertexAttribArrays();
    }
}

void Engine::DrawScene(DrawingViewport drawing_viewport, Engine::PostEffectsType post_effects_type, SceneGraph::SceneDrawType scene_draw_type) {
    PROFILER_GPU_ZONE(g_profiler_ctx, "Draw scene")
    Graphics* graphics = Graphics::Instance();
    PushGPUProfileRange(__FUNCTION__);
    GLenum buffers[] = {GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT};

    {
        PROFILER_GPU_ZONE(g_profiler_ctx, "Setup Post Effects")
        graphics->PushFramebuffer();
        if (graphics->multisample_framebuffer_exists) {
            graphics->bindFramebuffer(graphics->multisample_framebuffer);
        } else {
            graphics->bindFramebuffer(graphics->framebuffer);
        }
        graphics->PushViewport();
    }

    // Prepare for drawing
    {
        PROFILER_GPU_ZONE(g_profiler_ctx, "Clear screen");
        glDrawBuffers(2, buffers);
        glClearColor(0.0f, 0.0f, 0.0f, 0);
        graphics->drawing_shadow = false;
        graphics->Clear(true);
        glDrawBuffers(1, buffers);
    }

    {
        PROFILER_GPU_ZONE(g_profiler_ctx, "Apply viewpoint");
        ActiveCameras::Get()->applyViewpoint();

        std::vector<Camera>& virt_cam = ActiveCameras::Instance()->GetVirtualCameras();

        for (auto& cam : virt_cam) {
            cam.applyViewpoint();
        }
    }

    {  // Perform per-frame, per-camera functions, like character LOD and spawning grass
        PROFILER_GPU_ZONE(g_profiler_ctx, "Pre-draw camera");
        ActiveCameras::Get()->tint = vec3(1.0);
        ActiveCameras::Get()->vignette_tint = vec3(1.0);
        float predraw_time = game_timer.GetRenderTime();
        for (auto obj : scenegraph_->objects_) {
            if (!obj->parent) {
                obj->PreDrawCamera(predraw_time);
            }
        }
    }

    if (!g_no_decals) {
        scenegraph_->PrepareLightsAndDecals(active_screen_start, active_screen_end, vec2(graphics->render_dims[0], graphics->render_dims[1]));
    }

    // Draw everything
    {
        PROFILER_GPU_ZONE(g_profiler_ctx, "Draw scenegraph");
        if (graphics->depth_prepass) {
            PROFILER_GPU_ZONE(g_profiler_ctx, "Draw depth-only pass");
            mat4 proj = ActiveCameras::Get()->GetProjMatrix();
            mat4 view = ActiveCameras::Get()->GetViewMatrix();
            scenegraph_->DrawDepthMap(proj * view, (const vec4*)ActiveCameras::Get()->frustumPlanes, 6, SceneGraph::kDepthPrePass, scene_draw_type);
        }
        PROFILER_GPU_ZONE(g_profiler_ctx, "Draw color pass");
        scenegraph_->Draw(scene_draw_type);
    }

    if (graphics->multisample_framebuffer_exists) {
        graphics->BlitDepthBuffer();
        graphics->bindFramebuffer(graphics->multisample_framebuffer);
    }

    {
        PROFILER_GPU_ZONE(g_profiler_ctx, "Draw misc");
        {
            if (!(graphics->queued_screenshot && graphics->screenshot_mode == 1)) {
                PROFILER_GPU_ZONE(g_profiler_ctx, "Draw flares");
                scenegraph_->flares.Draw(Flares::kSharp);
            }
        }
        if (scenegraph_->map_editor->state_ != MapEditor::kInGame && ActiveCameras::Instance()->Get()->GetFlags() != Camera::kPreviewCamera) {
            {
                PROFILER_GPU_ZONE(g_profiler_ctx, "Draw map editor");
                scenegraph_->map_editor->Draw();
            }
            scenegraph_->light_probe_collection.Draw(*scenegraph_->bullet_world_);
            scenegraph_->dynamic_light_collection.Draw(*scenegraph_->bullet_world_);
        }
        {
            PROFILER_GPU_ZONE(g_profiler_ctx, "Draw debug geometry");
            GLenum buffers[] = {GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT};
            glDrawBuffers(2, buffers);
            DebugDraw::Instance()->Draw();
        }
    }

    float temp_white_point = 0.0f, temp_black_point = 0.0f;
    if (scenegraph_->IsCollisionNavMeshVisible() || g_draw_collision) {
        temp_white_point = graphics->hdr_white_point;
        temp_black_point = graphics->hdr_black_point;
        graphics->hdr_white_point = 1.0f;
        graphics->hdr_black_point = 0.0f;
    }

    {
        std::string post_shader_name = graphics->post_shader_name;
        if (scenegraph_->level->script_params().HasParam("Custom Shader") && config.GetRef("custom_level_shaders").toNumber<bool>()) {
            const std::string& custom_shader = scenegraph_->level->script_params().GetStringVal("Custom Shader");
            if (!custom_shader.empty()) {
                post_shader_name += " " + custom_shader;
            }
        }
        ApplyPostEffects(post_shader_name, drawing_viewport, active_screen_start, active_screen_end, post_effects_type, quad_vert_vbo, quad_index_vbo);
    }
    if (scenegraph_->IsCollisionNavMeshVisible() || g_draw_collision) {
        graphics->hdr_white_point = temp_white_point;
        graphics->hdr_black_point = temp_black_point;
    }
    PopGPUProfileRange();
}

void Engine::LoadScreenLoop(bool loading_in_progress) {
    {
        PROFILER_ZONE_IDLE(g_profiler_ctx, "Load screen 60hz wait");
        SDL_Delay(16);  // Sleep for 16 ms, don't need more than 60 fps!
    }
    PROFILER_ZONE(g_profiler_ctx, "Load screen loop");
    int num_time_steps = game_timer.GetStepsNeeded();
    num_time_steps = min(num_time_steps, _max_steps_per_frame);
    for (int curr_step = 0; curr_step < num_time_steps; curr_step++) {
        PROFILER_ZONE(g_profiler_ctx, "Load screen step");
        game_timer.game_time += game_timer.timestep;
        UpdateControls(game_timer.timestep, true);
    }
    {
        PROFILER_GPU_ZONE(g_profiler_ctx, "Load screen draw");
        DrawLoadScreen(loading_in_progress);
    }
}

void Engine::DrawLoadScreen(bool loading_in_progress) {
#ifndef GLDEBUG
    // don't draw loading screen on gl debug
    // makes traces more useful

    Graphics* graphics = Graphics::Instance();
    Textures* textures = Textures::Instance();
    CHECK_GL_ERROR();
    graphics->setViewport(0, 0, graphics->window_dims[0], graphics->window_dims[1]);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    graphics->Clear(true);

    vec3 base_color = vec3(min(1.0f, (SDL_TS_GetTicks() - started_loading_time) / 1000.0f));

    if (!level_has_screenshot && load_screen_tip[0] == '\0') {
        textures->drawTexture(loading_screen_logo->GetTextureRef(),
                              vec3((float)graphics->window_dims[0] / 2.0f, (float)graphics->window_dims[1] / 2.0f + sinf(game_timer.game_time) * 20.0f, 0),
                              512.0f + (sinf(game_timer.game_time * 2.0f) - 1.0f) * 20);
        CHECK_GL_ERROR();
    } else {
        if (level_has_screenshot) {
            CHECK_GL_ERROR();

            // no coloring
            Graphics* graphics = Graphics::Instance();
            Shaders* shaders = Shaders::Instance();

            int shader_id = shaders->returnProgram("simple_2d #TEXTURE #FLIPPED #LEVEL_SCREENSHOT");
            shaders->createProgram(shader_id);
            int shader_attrib_vert_coord = shaders->returnShaderAttrib("vert_coord", shader_id);
            int shader_attrib_tex_coord = shaders->returnShaderAttrib("tex_coord", shader_id);
            int uniform_mvp_mat = shaders->returnShaderVariable("mvp_mat", shader_id);
            int uniform_color = shaders->returnShaderVariable("color", shader_id);
            shaders->setProgram(shader_id);
            CHECK_GL_ERROR();

            GLState gl_state;
            gl_state.blend = true;
            gl_state.cull_face = false;
            gl_state.depth_write = false;
            gl_state.depth_test = false;
            graphics->setGLState(gl_state);
            CHECK_GL_ERROR();

            glm::mat4 proj_mat;
            proj_mat = glm::ortho(0.0f, (float)graphics->window_dims[0], 0.0f, (float)graphics->window_dims[1]);
            glm::mat4 modelview_mat(1.0f);
            vec2 where;
            vec2 size(Textures::Instance()->getWidth(level_screenshot->GetTextureRef()),
                      Textures::Instance()->getHeight(level_screenshot->GetTextureRef()));
            size /= size[1];
            size *= (float)graphics->window_dims[1];
            where = vec2(graphics->window_dims[0] * 0.5f, graphics->window_dims[1] * 0.5f);
            modelview_mat = glm::translate(modelview_mat, glm::vec3(where[0] - size[0] * 0.5f, where[1] - size[1] * 0.5f, 0));
            modelview_mat = glm::scale(modelview_mat, glm::vec3(size[0], size[1], 1.0f));
            modelview_mat = glm::translate(modelview_mat, glm::vec3(0.5f, 0.5f, 0.5f));
            modelview_mat = glm::translate(modelview_mat, glm::vec3(-0.5f, -0.5f, -0.5f));
            glm::mat4 mvp_mat = proj_mat * modelview_mat;
            CHECK_GL_ERROR();

            graphics->EnableVertexAttribArray(shader_attrib_vert_coord);
            CHECK_GL_ERROR();
            graphics->EnableVertexAttribArray(shader_attrib_tex_coord);
            CHECK_GL_ERROR();
            static const GLfloat verts[] = {
                0, 0, 0, 0,
                1, 0, 1, 0,
                1, 1, 1, 1,
                0, 1, 0, 1};
            static const GLuint indices[] = {
                0, 1, 2,
                0, 2, 3};
            static VBOContainer vert_vbo;
            static VBOContainer index_vbo;
            static bool vbo_filled = false;
            if (!vbo_filled) {
                vert_vbo.Fill(kVBOFloat | kVBOStatic, sizeof(verts), (void*)verts);
                index_vbo.Fill(kVBOElement | kVBOStatic, sizeof(indices), (void*)indices);
                vbo_filled = true;
            }
            vert_vbo.Bind();
            index_vbo.Bind();

            glVertexAttribPointer(shader_attrib_vert_coord, 2, GL_FLOAT, false, 4 * sizeof(GLfloat), 0);
            CHECK_GL_ERROR();
            glVertexAttribPointer(shader_attrib_tex_coord, 2, GL_FLOAT, false, 4 * sizeof(GLfloat), (void*)(sizeof(GL_FLOAT) * 2));
            CHECK_GL_ERROR();

            int num_indices = 6;

            glUniformMatrix4fv(uniform_mvp_mat, 1, false, (GLfloat*)&mvp_mat);
            CHECK_GL_ERROR();
            vec4 color(base_color, 1.0f);
            glUniform4fv(uniform_color, 1, &color[0]);
            CHECK_GL_ERROR();

            Textures::Instance()->bindTexture(level_screenshot, 0);
            CHECK_GL_ERROR();
            graphics->DrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, 0);
            CHECK_GL_ERROR();

            graphics->BindElementVBO(0);
            graphics->BindArrayVBO(0);
            graphics->ResetVertexAttribArrays();
            CHECK_GL_ERROR();
        }

        {  // Draw pulsing Overgrowth logo
            CHECK_GL_ERROR();

            // no coloring
            Graphics* graphics = Graphics::Instance();
            Shaders* shaders = Shaders::Instance();

            int shader_id = shaders->returnProgram("simple_2d #TEXTURE #FLIPPED #LOADING_LOGO");
            shaders->createProgram(shader_id);
            int shader_attrib_vert_coord = shaders->returnShaderAttrib("vert_coord", shader_id);
            int shader_attrib_tex_coord = shaders->returnShaderAttrib("tex_coord", shader_id);
            int uniform_mvp_mat = shaders->returnShaderVariable("mvp_mat", shader_id);
            int uniform_color = shaders->returnShaderVariable("color", shader_id);
            int uniform_time = shaders->returnShaderVariable("time", shader_id);
            shaders->setProgram(shader_id);
            CHECK_GL_ERROR();

            GLState gl_state;
            gl_state.blend = true;
            gl_state.cull_face = false;
            gl_state.depth_write = false;
            gl_state.depth_test = false;
            graphics->setGLState(gl_state);
            CHECK_GL_ERROR();

            glm::mat4 proj_mat;
            proj_mat = glm::ortho(0.0f, (float)graphics->window_dims[0], 0.0f, (float)graphics->window_dims[1]);
            glm::mat4 modelview_mat(1.0f);
            vec2 where;
            vec2 size(graphics->window_dims[1] * 0.1f);
            where = vec2(graphics->window_dims[0] * 0.5f, graphics->window_dims[1] * 0.1f);
            modelview_mat = glm::translate(modelview_mat, glm::vec3(where[0] - size[0] * 0.5f, where[1] - size[1] * 0.5f, 0));
            modelview_mat = glm::scale(modelview_mat, glm::vec3(size[0], size[1], 1.0f));
            modelview_mat = glm::translate(modelview_mat, glm::vec3(0.5f, 0.5f, 0.5f));
            modelview_mat = glm::translate(modelview_mat, glm::vec3(-0.5f, -0.5f, -0.5f));
            glm::mat4 mvp_mat = proj_mat * modelview_mat;
            CHECK_GL_ERROR();

            graphics->EnableVertexAttribArray(shader_attrib_vert_coord);
            CHECK_GL_ERROR();
            graphics->EnableVertexAttribArray(shader_attrib_tex_coord);
            CHECK_GL_ERROR();
            static const GLfloat verts[] = {
                0, 0, 0, 0,
                1, 0, 1, 0,
                1, 1, 1, 1,
                0, 1, 0, 1};
            static const GLuint indices[] = {
                0, 1, 2,
                0, 2, 3};
            static VBOContainer vert_vbo;
            static VBOContainer index_vbo;
            static bool vbo_filled = false;
            if (!vbo_filled) {
                vert_vbo.Fill(kVBOFloat | kVBOStatic, sizeof(verts), (void*)verts);
                index_vbo.Fill(kVBOElement | kVBOStatic, sizeof(indices), (void*)indices);
                vbo_filled = true;
            }
            vert_vbo.Bind();
            index_vbo.Bind();

            glVertexAttribPointer(shader_attrib_vert_coord, 2, GL_FLOAT, false, 4 * sizeof(GLfloat), 0);
            CHECK_GL_ERROR();
            glVertexAttribPointer(shader_attrib_tex_coord, 2, GL_FLOAT, false, 4 * sizeof(GLfloat), (void*)(sizeof(GL_FLOAT) * 2));
            CHECK_GL_ERROR();

            int num_indices = 6;

            glUniformMatrix4fv(uniform_mvp_mat, 1, false, (GLfloat*)&mvp_mat);
            CHECK_GL_ERROR();
            vec4 color(base_color, 1.0f);
            glUniform4fv(uniform_color, 1, &color[0]);
            glUniform1f(uniform_time, SDL_GetTicks() / 1000.0f);
            CHECK_GL_ERROR();

            Textures::Instance()->bindTexture(loading_screen_og_logo->GetTextureRef(), 0);
            CHECK_GL_ERROR();
            graphics->DrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, 0);
            CHECK_GL_ERROR();

            graphics->BindElementVBO(0);
            graphics->BindArrayVBO(0);
            graphics->ResetVertexAttribArrays();
            CHECK_GL_ERROR();
        }
    }

    // Draw technical loading text
    if (!level_has_screenshot && load_screen_tip[0] == '\0' || load_screen_tip[0] == '\0' && Online::Instance()->IsActive()) {
        int font_height = font_renderer.GetFontInfo(0, FontRenderer::INFO_HEIGHT) / 64;
        for (int i = 0; i < NativeLoadingText::kMaxLines; ++i) {
            int pos[] = {20, graphics->window_dims[1] - 20 - font_height * i};
            g_text_atlas_renderer.num_characters = 0;
            CHECK_GL_ERROR();
            g_text_atlas_renderer.AddText(&g_text_atlas[kTextAtlasMono],
                                          &native_loading_text.buf[NativeLoadingText::kMaxCharPerLine * i],
                                          pos, &font_renderer,
                                          UINT32MAX);
            CHECK_GL_ERROR();
            g_text_atlas_renderer.Draw(&g_text_atlas[kTextAtlasMono],
                                       graphics, TextAtlasRenderer::kTextShadow, vec4(1.0f));
            CHECK_GL_ERROR();
            Textures::Instance()->InvalidateBindCache();
        }
    }

    {  // Draw fun text
        int font_size = int(max(18, min(Graphics::Instance()->window_dims[1] / 30, Graphics::Instance()->window_dims[0] / 50)));
        TextMetrics metrics = g_as_text_context.ASGetTextAtlasMetrics(font_path, font_size, 0, load_screen_tip);
        int pos[] = {graphics->window_dims[0] / 2 - metrics.bounds[2] / 2, graphics->window_dims[1] / 2 - font_size / 2};
        g_as_text_context.ASDrawTextAtlas(font_path, font_size, 0, load_screen_tip, pos[0], pos[1], vec4(base_color, 1.0f));
        Textures::Instance()->InvalidateBindCache();
    }

    CHECK_GL_ERROR();
    Textures::Instance()->InvalidateBindCache();
    graphics->SwapToScreen();
    graphics->ClearGLState();
    CHECK_GL_ERROR();

#endif  // GLDEBUG
}

void Engine::SetViewportForCamera(int which_cam, int num_screens, Graphics::ScreenType screen_type) {
    Graphics* graphics = Graphics::Instance();
    switch (which_cam) {
        case 0:  // Draw the first screen, which might take up the whole viewport, or just half
            if (num_screens == 1) {
                active_screen_start = vec2(0.0f, 0.0f);
                active_screen_end = vec2(1.0f, 1.0f);
            } else if (num_screens == 2) {
                active_screen_start = vec2(0.0f, 0.0f);
                active_screen_end = vec2(1.0f, 0.5f);
            } else if (num_screens == 3 || num_screens == 4) {
                active_screen_start = vec2(0.0f, 0.0f);
                active_screen_end = vec2(0.5f, 0.5f);
            }
            break;
        case 1:  // The second screen always just takes half the viewport
            if (num_screens == 2) {
                active_screen_start = vec2(0.0f, 0.5f);
                active_screen_end = vec2(1.0f, 1.0f);
            } else if (num_screens == 3 || num_screens == 4) {
                active_screen_start = vec2(0.5f, 0.0f);
                active_screen_end = vec2(1.0f, 0.5f);
            }
            break;
        case 2:
            active_screen_start = vec2(0.0f, 0.5f);
            active_screen_end = vec2(0.5f, 1.0f);
            break;
        case 3:
            active_screen_start = vec2(0.5f, 0.5f);
            active_screen_end = vec2(1.0f, 1.0f);
            break;
    }

    graphics->startDraw(active_screen_start, active_screen_end, screen_type);
}

void Engine::DrawCubeMap(TextureRef cube_map, const vec3& pos, GLuint cube_map_fbo, SceneGraph::SceneDrawType scene_draw_type) {
    GLuint cube_map_tex_id = Textures::Instance()->returnTexture(cube_map);
    Graphics* graphics = Graphics::Instance();
    CHECK_GL_ERROR();
    // TODO: Why do we have to render the first one twice? Some state is messed up
    for (int temp_i = -1; temp_i < 6; ++temp_i) {
        PROFILER_GPU_ZONE(g_profiler_ctx, "Draw cubemap face")
        int i = max(0, temp_i);
        graphics->startDraw(vec2(0.0f, 0.0f), vec2(128.0f, 128.0f), Graphics::kTexture);
        ActiveCameras::Set(1);
        CHECK_GL_ERROR();
        ActiveCameras::Get()->SetFlags(Camera::kPreviewCamera);
        // Apply camera object settings to camera
        Camera* camera = ActiveCameras::Instance()->Get();
        CHECK_GL_ERROR();
        // Set camera position
        camera->SetPos(pos);
        // Set camera euler angles from rotation matrix
        mat4 rot = GetCubeMapRotation(i);
        vec3 front = rot * vec3(0, 0, 1);
        vec3 up = rot * vec3(0, 1, 0);
        vec3 expected_right = normalize(cross(front, vec3(0, 1, 0)));
        vec3 expected_up = normalize(cross(expected_right, front));
        float cam[3];
        cam[0] = asinf(front[1]) * -180.0f / PI_f;
        cam[1] = atan2f(front[0], front[2]) * 180.0f / PI_f;
        cam[2] = atan2f(dot(up, expected_right), dot(up, expected_up)) * 180.0f / PI_f;
        // Set rotation twice so we don't have to worry about interpolation
        camera->SetXRotation(cam[0]);
        camera->SetXRotation(cam[0]);
        camera->SetYRotation(cam[1]);
        camera->SetYRotation(cam[1]);
        camera->SetZRotation(cam[2]);
        camera->SetZRotation(cam[2]);
        // Force true 90 degree fov
        camera->flexible_fov = false;
        camera->SetFOV(90.0f);
        camera->SetDistance(0.0f);
        // Draw view
        CHECK_GL_ERROR();
        DrawScene(kViewport, Engine::kStraight, SceneGraph::kStaticOnly);
        CHECK_GL_ERROR();
        scenegraph_->level->Draw();

        graphics->PushFramebuffer();
        glBindFramebuffer(GL_FRAMEBUFFER, cube_map_fbo);
        CHECK_GL_ERROR();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cube_map_tex_id, 0);
        CHECK_GL_ERROR();
        glBindFramebuffer(GL_READ_FRAMEBUFFER, graphics->framebuffer);
        CHECK_GL_ERROR();
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, cube_map_fbo);
        CHECK_GL_ERROR();
        glBlitFramebuffer(0, 0, 128, 128, 0, 0, 128, 128, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        CHECK_GL_ERROR();
        graphics->PopFramebuffer();

        camera->flexible_fov = true;
    }
    CHECK_GL_ERROR();
}

std::vector<GLfloat> shadow_cache_verts;
std::vector<GLuint> shadow_cache_indices;
extern const bool kUseShadowCache = false;
bool shadow_cache_dirty = true;
bool shadow_cache_dirty_sun_moved = true;
bool shadow_cache_dirty_level_loaded = true;
VBOContainer shadow_cache_verts_vbo;
VBOContainer shadow_cache_indices_vbo;

static inline vec2 CalculateMulMat4Vec3(const mat4& transform, const vec3& vertex) {
    return {
        transform[0 + 4 * 0] * vertex[0] + transform[0 + 4 * 1] * vertex[1] + transform[0 + 4 * 2] * vertex[2] + transform[0 + 4 * 3] * 1.0f,
        transform[1 + 4 * 0] * vertex[0] + transform[1 + 4 * 1] * vertex[1] + transform[1 + 4 * 2] * vertex[2] + transform[1 + 4 * 3] * 1.0f,
    };
}

static void CalculateMinMax(const mat4& model_to_light_transform, const Model& model, float out_min_max_bounds[4]) {
    float result_min_max_bounds[4] = {
        FLT_MAX,
        FLT_MAX,
        -FLT_MAX,
        -FLT_MAX,
    };  // min_x, min_y, max_x, max_y

    vec3 bounding_corners[8] = {
        model.min_coords,
        vec3(model.max_coords[0], model.min_coords[1], model.min_coords[2]),
        vec3(model.min_coords[0], model.max_coords[1], model.min_coords[2]),
        vec3(model.max_coords[0], model.max_coords[1], model.min_coords[2]),
        vec3(model.min_coords[0], model.min_coords[1], model.max_coords[2]),
        vec3(model.max_coords[0], model.min_coords[1], model.max_coords[2]),
        vec3(model.min_coords[0], model.max_coords[1], model.max_coords[2]),
        model.max_coords,
    };

    for (const auto& bounding_corner : bounding_corners) {
        vec2 light_space_vert = CalculateMulMat4Vec3(model_to_light_transform, bounding_corner);
        result_min_max_bounds[0] = std::min(light_space_vert[0], result_min_max_bounds[0]);
        result_min_max_bounds[1] = std::min(light_space_vert[1], result_min_max_bounds[1]);
        result_min_max_bounds[2] = std::max(light_space_vert[0], result_min_max_bounds[2]);
        result_min_max_bounds[3] = std::max(light_space_vert[1], result_min_max_bounds[3]);
    }

    memcpy(out_min_max_bounds, result_min_max_bounds, sizeof(result_min_max_bounds));
}

void DrawShadowCache(const mat4& proj_view_matrix, const char* special) {
    Graphics* graphics = Graphics::Instance();
    Shaders* shaders = Shaders::Instance();

    GLState gl_state;
    gl_state.depth_test = true;
    gl_state.cull_face = true;
    gl_state.depth_write = true;
    gl_state.blend = false;

    graphics->setGLState(gl_state);

    const int kBufSize = 512;
    char shader_name[kBufSize];
    FormatString(shader_name, kBufSize, "shadow%s", special);
    int shader_id = shaders->returnProgram(shader_name);
    shaders->setProgram(shader_id);
    shaders->SetUniformMat4("projection_view_mat", proj_view_matrix);
    int vert_attrib_id = shaders->returnShaderAttrib("vert", shader_id);

    shadow_cache_verts_vbo.Bind();
    graphics->EnableVertexAttribArray(vert_attrib_id);
    glVertexAttribPointer(vert_attrib_id, 3, GL_FLOAT, false, 0, 0);
    shadow_cache_indices_vbo.Bind();
    graphics->DrawElements(GL_TRIANGLES, shadow_cache_indices.size(), GL_UNSIGNED_INT, 0);
    graphics->ResetVertexAttribArrays();
}

static bool UpdateShadowCache(SceneGraph* scenegraph) {
    Graphics* graphics = Graphics::Instance();
    Textures* textures = Textures::Instance();
    Shaders* shaders = Shaders::Instance();
    PROFILER_ENTER(g_profiler_ctx, "Get bounds");

    // Get basis from light perspective
    vec3 light_dir = scenegraph->primary_light.pos;
    vec3 light_to_world_basis[3];
    light_to_world_basis[2] = light_dir;
    light_to_world_basis[0] = normalize(cross(vec3(0, 1, 0), light_to_world_basis[2]));
    light_to_world_basis[1] = normalize(cross(light_to_world_basis[0], light_to_world_basis[2]));
    mat4 light_to_world_transform = {
        light_to_world_basis[0][0],
        light_to_world_basis[0][1],
        light_to_world_basis[0][2],
        0.0f,
        light_to_world_basis[1][0],
        light_to_world_basis[1][1],
        light_to_world_basis[1][2],
        0.0f,
        light_to_world_basis[2][0],
        light_to_world_basis[2][1],
        light_to_world_basis[2][2],
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
    };
    mat4 world_to_light_transform = transpose(light_to_world_transform);

    // Get bounds of scene in light space
    const int kMaxUpdatesPerFrame = 32;
    bool max_updates_hit = false;
    int current_update_count = 0;
    if (shadow_cache_dirty_sun_moved) {
        // Dirty all objects once, so iterative processing can clean them up
        for (auto& lb : scenegraph->visible_static_meshes_shadow_cache_bounds_) {
            if (!lb.is_ignored) {
                lb.is_calculated = false;
            }
        }
        for (auto& lb : scenegraph->terrain_objects_shadow_cache_bounds_) {
            lb.is_calculated = false;
        }
    }
    float bounds[4] = {FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX};  // min_x, min_y, max_x, max_y
    for (size_t i = 0, len = scenegraph->visible_static_meshes_.size(); i < len && !max_updates_hit; ++i) {
        EnvObject& eo = *scenegraph->visible_static_meshes_[i];
        ShadowCacheObjectLightBounds& lb = scenegraph->visible_static_meshes_shadow_cache_bounds_[i];
        lb.is_ignored = eo.attached_ != NULL;
        if (!lb.is_ignored) {
            if (!lb.is_calculated || shadow_cache_dirty_level_loaded) {
                const Model& env_model = *eo.GetModel();
                const mat4& model_to_world_transform = eo.GetTransform();
                CalculateMinMax(world_to_light_transform * model_to_world_transform, env_model, &lb.min_bounds[0]);
                lb.is_calculated = true;
                ++current_update_count;
            }
        }

        if (!shadow_cache_dirty_level_loaded && current_update_count == kMaxUpdatesPerFrame && i + 1 < len) {
            max_updates_hit = true;
            break;
        }
    }
    for (size_t i = 0, len = scenegraph->terrain_objects_.size(); i < len && !max_updates_hit; ++i) {
        TerrainObject& to = *scenegraph->terrain_objects_[i];
        ShadowCacheObjectLightBounds& lb = scenegraph->terrain_objects_shadow_cache_bounds_[i];
        if (!lb.is_calculated || shadow_cache_dirty_level_loaded) {
            const Model& env_model = *to.GetModel();
            const mat4& model_to_world_transform = to.GetTransform();
            CalculateMinMax(world_to_light_transform * model_to_world_transform, env_model, &lb.min_bounds[0]);
            lb.is_calculated = true;
            ++current_update_count;
        }

        if (!shadow_cache_dirty_level_loaded && current_update_count == kMaxUpdatesPerFrame && i + 1 < len) {
            max_updates_hit = true;
            break;
        }
    }
    if (shadow_cache_dirty_level_loaded || !max_updates_hit) {
        for (auto& lb : scenegraph->visible_static_meshes_shadow_cache_bounds_) {
            if (!lb.is_ignored) {
                bounds[0] = std::min(lb.min_bounds[0], bounds[0]);
                bounds[1] = std::min(lb.min_bounds[1], bounds[1]);
                bounds[2] = std::max(lb.max_bounds[0], bounds[2]);
                bounds[3] = std::max(lb.max_bounds[1], bounds[3]);
            }
        }
        for (auto& lb : scenegraph->terrain_objects_shadow_cache_bounds_) {
            bounds[0] = std::min(lb.min_bounds[0], bounds[0]);
            bounds[1] = std::min(lb.min_bounds[1], bounds[1]);
            bounds[2] = std::max(lb.max_bounds[0], bounds[2]);
            bounds[3] = std::max(lb.max_bounds[1], bounds[3]);
        }
    }
    PROFILER_LEAVE(g_profiler_ctx);

    if (max_updates_hit) {
        return false;
    }

    {  // Draw depth map of the whole level
        Camera* camera = ActiveCameras::Get();
        int num_shadow_view_frustum_planes = 6;
        vec4 shadow_view_frustum_planes[6];
        CHECK_GL_ERROR();
        glColorMask(0, 0, 0, 0);
        graphics->drawing_shadow = true;
        graphics->PushFramebuffer();
        graphics->bindFramebuffer(graphics->static_shadow_fb);
        graphics->PushViewport();
        glEnable(GL_SCISSOR_TEST);
        float shadow_size = max(bounds[2] - bounds[0], bounds[3] - bounds[1]);
        // float shadow_radius = shadow_size / 1.414213563731f / 2.0f; // Get radius of circle inscribed in square with sides 'shadow_size'
        vec3 shadow_center = (bounds[0] + bounds[2]) * 0.5f * light_to_world_basis[0] + (bounds[1] + bounds[3]) * 0.5f * light_to_world_basis[1];
        vec3 light_space_center(dot(shadow_center, light_to_world_basis[0]),
                                dot(shadow_center, light_to_world_basis[1]),
                                dot(shadow_center, light_to_world_basis[2]));  // Note: Dot of vector vs columns is like transpose(mat) * vec. And transpose(rot_only_mat) == inverse(rot_only_mat).
        // Snap center to nearest texel to improve frame-to-frame coherency
        float texel_size = shadow_size * 2.0f / graphics->shadow_res;
        for (int j = 0; j < 3; ++j) {
            light_space_center[j] = floor(light_space_center[j] / texel_size) * texel_size;
        }
        // Update shadow center to new texel-snapped position
        shadow_center = light_space_center[0] * light_to_world_basis[0] +
                        light_space_center[1] * light_to_world_basis[1];

        graphics->setViewport(0, 0, graphics->cascade_shadow_res, graphics->cascade_shadow_res);
        graphics->Clear(true);
        mat4 proj, view;
        camera->applyShadowViewpoint(light_dir, shadow_center, shadow_size, &proj, &view);
        mat4 proj_view_matrix = proj * view;
        graphics->simple_shadow_mat = proj_view_matrix;

        for (int i = 0; i < 6; ++i) {
            for (int j = 0; j < 4; ++j) {
                shadow_view_frustum_planes[num_shadow_view_frustum_planes - 6 + i][j] = camera->frustumPlanes[i][j];
            }
        }
        scenegraph->DrawDepthMap(proj_view_matrix, shadow_view_frustum_planes, num_shadow_view_frustum_planes, SceneGraph::kDepthShadow, SceneGraph::kStaticOnly);
        glDisable(GL_SCISSOR_TEST);
        graphics->PopViewport();
        graphics->PopFramebuffer();
        graphics->drawing_shadow = false;
        glColorMask(1, 1, 1, 1);
    }

    if (false) {  // Create shadow cache VBO of only triangles that affect shadows
        PROFILER_LEAVE(g_profiler_ctx);
        PROFILER_ENTER(g_profiler_ctx, "Fill VBO");
        shadow_cache_verts_vbo.Fill(kVBOStatic | kVBOFloat, shadow_cache_verts.size() * sizeof(GLfloat), &shadow_cache_verts[0]);
        shadow_cache_indices_vbo.Fill(kVBOStatic | kVBOElement, shadow_cache_indices.size() * sizeof(GLuint), &shadow_cache_indices[0]);

        PROFILER_LEAVE(g_profiler_ctx);
        PROFILER_ENTER(g_profiler_ctx, "Render all cascade tiles");
        const int render_size = 2048;
        graphics->PushFramebuffer();
        graphics->PushViewport();
        graphics->SetBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_DST_COLOR);  // Should be equivalent to Photoshop 'lighten' mode
        std::vector<bool> visible_gpu(shadow_cache_indices.size() / 3, false);
        int tri_visible_shader_id = shaders->returnProgram("tri_visible");
        static GLuint cascade_tri_visible_fb = UINT_MAX;
        static TextureRef cascade_tri_visible_texture_ref;
        // Make sure framebuffer exists for accumulating visible triangle ids
        if (cascade_tri_visible_fb == UINT_MAX) {
            graphics->genFramebuffers(&cascade_tri_visible_fb, "cascade_tri_visible_fb");
            graphics->bindFramebuffer(cascade_tri_visible_fb);
            cascade_tri_visible_texture_ref = textures->makeTextureColor(render_size, render_size);
            Textures::Instance()->SetTextureName(cascade_tri_visible_texture_ref, "Cascade Triangle Visible");
            graphics->framebufferColorTexture2D(cascade_tri_visible_texture_ref);
        }
        graphics->bindFramebuffer(cascade_tri_visible_fb);
        graphics->setViewport(0, 0, render_size, render_size);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        graphics->Clear(true);
        // Create VBO that just sends one vec2 for each texel in a render_size*render_size image
        static VBOContainer pixel_vbo;
        if (!pixel_vbo.valid()) {
            std::vector<GLfloat> pixel_data;
            pixel_data.resize(render_size * render_size * 2);
            for (int i = 0, index = 0; i < render_size; ++i) {
                for (int j = 0; j < render_size; ++j) {
                    pixel_data[index + 0] = (j + 0.5f) / (float)render_size;
                    pixel_data[index + 1] = (i + 0.5f) / (float)render_size;
                    index += 2;
                }
            }
            pixel_vbo.Fill(kVBOFloat | kVBOStatic, pixel_data.size() * sizeof(GLfloat), &pixel_data[0]);
        }
        glPointSize(1);
        CHECK_GL_ERROR();
        float size[2] = {bounds[2] - bounds[0], bounds[3] - bounds[1]};
        for (int cascade = 0; cascade < 1; ++cascade) {
            // Convert light-space bounds to texels
            int texel_dim[2];
            for (int j = 0; j < 2; ++j) {
                texel_dim[j] = (int)ceilf(size[j] / (shadow_sizes[cascade]) * graphics->shadow_res);
            }
            // How many tiles we need to render in each direction
            int render_dim[2];
            for (int j = 0; j < 2; ++j) {
                render_dim[j] = (texel_dim[j] + render_size - 1) / render_size;
            }
            Camera* camera = ActiveCameras::Get();
            for (int x = 0; x < render_dim[0]; ++x) {
                for (int y = 0; y < render_dim[1]; ++y) {
                    graphics->bindFramebuffer(graphics->cascade_shadow_color_fb);
                    graphics->setViewport(0, 0, render_size, render_size);
                    PROFILER_GPU_ZONE(g_profiler_ctx, "Cascade tile");
                    graphics->Clear(true);
                    vec3 light_space_center = vec3(bounds[0] + (x + 0.5f) * shadow_sizes[cascade],
                                                   bounds[1] + (y + 0.5f) * shadow_sizes[cascade],
                                                   0.0f);
                    // Snap center to nearest texel to improve frame-to-frame coherency
                    float texel_size = shadow_sizes[cascade] * 2.0f / graphics->shadow_res;
                    for (int j = 0; j < 2; ++j) {
                        light_space_center[j] = floor(light_space_center[j] / texel_size) * texel_size;
                    }
                    // Update shadow center to new texel-snapped position
                    vec3 shadow_center = light_space_center[0] * light_to_world_basis[0] +
                                         light_space_center[1] * light_to_world_basis[1];
                    mat4 proj, view;
                    camera->applyShadowViewpoint(light_dir, shadow_center, shadow_sizes[cascade], &proj, &view);
                    mat4 proj_view_matrix = proj * view;
                    // Draw triangle ids as color to RGB channels
                    glColorMask(1, 1, 1, 0);
                    graphics->setDepthFunc(GL_LESS);
                    DrawShadowCache(proj_view_matrix, " #TRI_COLOR");
                    // Draw to alpha channel if there is another fragment behind the front one
                    glColorMask(0, 0, 0, 1);
                    graphics->setDepthFunc(GL_GREATER);
                    DrawShadowCache(proj_view_matrix, "");
                    // Calculate visible tris on GPU
                    glColorMask(1, 1, 1, 1);
                    graphics->setBlend(true);
                    graphics->bindFramebuffer(cascade_tri_visible_fb);
                    graphics->setViewport(0, 0, render_size, render_size);
                    shaders->setProgram(tri_visible_shader_id);
                    shaders->SetUniformInt("output_width", render_size);
                    shaders->SetUniformInt("output_height", render_size);
                    textures->bindTexture(graphics->cascade_shadow_color_ref);
                    pixel_vbo.Bind();
                    int vert_attrib_id = shaders->returnShaderAttrib("pixel_uv", tri_visible_shader_id);
                    graphics->EnableVertexAttribArray(vert_attrib_id);
                    glVertexAttribPointer(vert_attrib_id, 2, GL_FLOAT, false, 2 * sizeof(GLfloat), 0);
                    glDrawArrays(GL_POINTS, 0, render_size * render_size);
                    graphics->ResetVertexAttribArrays();
                }
            }
            {
                int size = render_size * render_size;
                unsigned char* pixels = new unsigned char[size * 4];
                ::memset(pixels, 0, size * 4);
                {
                    PROFILER_ZONE_STALL(g_profiler_ctx, "glReadPixels");
                    glReadPixels(0, 0, render_size, render_size,
                                 GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, (GLvoid*)pixels);
                }
                // std::string path = ImageExport::FindEmptySequentialFile("Screenshots/TriVisible",".png");
                // ImageExport::SavePNGTransparent(path.c_str(), pixels, render_size, render_size);
                int check_size = min(size, (int)visible_gpu.size());
                int num_tris = 0;
                for (int pixel = 0, index = 0; pixel < check_size; ++pixel, index += 4) {
                    // unsigned char* pixel_ptr = &pixels[index];
                    if (pixels[index + 0] != 0) {
                        ++num_tris;
                        int id = index / 4;
                        visible_gpu[id] = true;
                    }
                }
                delete[] pixels;
                LOGI << "NUM GPU TRIS: " << num_tris << std::endl;
            }
            glColorMask(1, 1, 1, 1);
            std::vector<GLuint> new_shadow_cache_indices;
            for (size_t i = 0, index = 0, len = visible_gpu.size(); i < len; ++i, index += 3) {
                if (visible_gpu[i]) {
                    for (size_t j = 0; j < 3; ++j) {
                        new_shadow_cache_indices.push_back(shadow_cache_indices[index + j]);
                    }
                }
            }

            if (false) {  // Debug wireframe
                std::vector<GLuint> line_indices;
                for (size_t i = 0, len = new_shadow_cache_indices.size(); i < len; i += 3) {
                    for (size_t j = 0; j < 3; ++j) {
                        line_indices.push_back(new_shadow_cache_indices[i + j]);
                        line_indices.push_back(new_shadow_cache_indices[i + (j + 1) % 3]);
                    }
                }
                DebugDraw::Instance()->AddLines(shadow_cache_verts, line_indices, vec4(1.0), _persistent, _DD_XRAY);
            }
            shadow_cache_indices = new_shadow_cache_indices;
        }
        PROFILER_LEAVE(g_profiler_ctx);
        PROFILER_ENTER(g_profiler_ctx, "Finalize");
        graphics->setDepthFunc(GL_LEQUAL);
        graphics->PopViewport();
        graphics->PopFramebuffer();

        if (shadow_cache_indices.empty()) {
            // Add degenerate triangle if shadow cache is empty, so
            // that we don't need a separate branch to deal with this
            for (int i = 0; i < 3; ++i) {
                shadow_cache_indices.push_back(0);
            }
        }

        shadow_cache_verts_vbo.Fill(kVBOStatic | kVBOFloat, shadow_cache_verts.size() * sizeof(GLfloat), &shadow_cache_verts[0]);
        shadow_cache_indices_vbo.Fill(kVBOStatic | kVBOElement, shadow_cache_indices.size() * sizeof(GLuint), &shadow_cache_indices[0]);

        LOGI << "Shadow cache verts: " << shadow_cache_verts.size() / 3 << std::endl;
        LOGI << "Shadow cache tris: " << shadow_cache_indices.size() / 3 << std::endl;
        PROFILER_LEAVE(g_profiler_ctx);
    }

    return true;
}

#define NUM_SHADOW_CASCADES 4

struct uvec4 {
    union {
        unsigned int v[4];
        struct {
            unsigned int x, y, z, w;
        };
    };

    uvec4() {}

    uvec4(unsigned int x_, unsigned int y_, unsigned int z_, unsigned int w_)
        : x(x_), y(y_), z(z_), w(w_) {
    }
};

struct ShadowCascadeParams {
    mat4 proj_view_matrix[NUM_SHADOW_CASCADES];
    vec4 frustum_planes[NUM_SHADOW_CASCADES][6];
    uvec4 viewport[NUM_SHADOW_CASCADES];
};

void GetShadowableTris(SceneGraph* scenegraph) {
    // For each cascade size
    // Divide scene into tiles
    // Draw tiles using shader that renders the triangle and object id to a texture
    // Read the texture and add triangle/object pair to a set
    // Create model for each tile

    Graphics* graphics = Graphics::Instance();
    Camera* camera = ActiveCameras::Get();

    vec3 light_dir = scenegraph->primary_light.pos;
    vec3 basis[3];
    basis[2] = light_dir;
    basis[0] = normalize(cross(vec3(0, 1, 0), basis[2]));
    basis[1] = normalize(cross(basis[0], basis[2]));

    vec3 camera_pos;

    ShadowCascadeParams shadow_params;
    for (int i = 0; i < NUM_SHADOW_CASCADES; ++i) {
        float shadow_size = shadow_sizes[i];
        float shadow_radius = shadow_size / 1.414213563731f / 2.0f;  // Get radius of circle inscribed in square with sides 'shadow_size'
        vec3 shadow_center = camera_pos;
        vec3 light_space_center(
            dot(shadow_center, basis[0]),
            dot(shadow_center, basis[1]),
            dot(shadow_center, basis[2]));
        // Snap center to nearest texel to improve frame-to-frame coherency
        float texel_size = shadow_size * 2.0f / graphics->shadow_res;
        for (int j = 0; j < 3; ++j) {
            light_space_center[j] = floor(light_space_center[j] / texel_size) * texel_size;
        }
        // Update shadow center to new texel-snapped position
        shadow_center = light_space_center[0] * basis[0] +
                        light_space_center[1] * basis[1];

        GLuint res = graphics->cascade_shadow_res / 2;
        uvec4& viewport = shadow_params.viewport[i];
        switch (i) {
            case 0:
                viewport = uvec4(0, 0, res, res);
                break;
            case 1:
                viewport = uvec4(res, 0, res * 2, res);
                break;
            case 2:
                viewport = uvec4(0, res, res, res * 2);
                break;
            case 3:
                viewport = uvec4(res, res, res * 2, res * 2);
                break;
        }
        mat4 proj, view;
        camera->applyShadowViewpoint(light_dir, shadow_center, shadow_size, &proj, &view);
        mat4 proj_view_matrix = proj * view;
        graphics->cascade_shadow_mat[i] = proj_view_matrix;
        shadow_params.proj_view_matrix[i] = proj_view_matrix;
        for (int j = 0; j < 6; ++j) {
            memcpy(&shadow_params.frustum_planes[i][j], &camera->frustumPlanes[j][0], 4 * sizeof(float));
        }
    }

    // Make room for the shadow frustum planes for each cascade
    int num_shadow_view_frustum_planes = 6;
    vec4 shadow_view_frustum_planes[6];
    CHECK_GL_ERROR();
    glColorMask(1, 1, 1, 1);
    graphics->drawing_shadow = true;
    graphics->PushFramebuffer();
    graphics->bindFramebuffer(graphics->cascade_shadow_fb);
    graphics->PushViewport();
    graphics->setViewport(0, 0, graphics->cascade_shadow_res, graphics->cascade_shadow_res);
    glEnable(GL_SCISSOR_TEST);

    for (int i = 0; i < NUM_SHADOW_CASCADES; ++i) {
        const uvec4& viewport = shadow_params.viewport[i];
        graphics->setViewport(viewport.x, viewport.y, viewport.z, viewport.w);
        graphics->Clear(true);
        mat4 proj_view_matrix = shadow_params.proj_view_matrix[i];

        if (kUseShadowCache) {
            DrawShadowCache(proj_view_matrix, "");
        }
        for (int i = 0; i < 6; ++i) {
            for (int j = 0; j < 4; ++j) {
                shadow_view_frustum_planes[num_shadow_view_frustum_planes - 6 + i][j] = camera->frustumPlanes[i][j];
            }
        }
        if (i == 0) {
            scenegraph->DrawDepthMap(proj_view_matrix, shadow_view_frustum_planes, num_shadow_view_frustum_planes, SceneGraph::kDepthShadow, SceneGraph::kStaticAndDynamic);
        } else {
            scenegraph->DrawDepthMap(proj_view_matrix, &shadow_view_frustum_planes[num_shadow_view_frustum_planes - 6], 6, SceneGraph::kDepthShadow, SceneGraph::kStaticAndDynamic);
        }
    }

    glDisable(GL_SCISSOR_TEST);
    graphics->PopViewport();
    graphics->PopFramebuffer();
    graphics->drawing_shadow = false;
    glColorMask(1, 1, 1, 1);
}

float last_shadow_update_time = 0.0f;

static void UpdateShadowCascades(SceneGraph* scenegraph) {
    PROFILER_GPU_ZONE(g_profiler_ctx, "Draw shadow cascades");
    bool update_all = false;
    if (last_shadow_update_time > game_timer.game_time - 1.0f) {
    } else {
        update_all = true;
        last_shadow_update_time = game_timer.game_time;
    }
    Graphics* graphics = Graphics::Instance();
    Camera* camera = ActiveCameras::Get();
    CHECK_GL_ERROR();
    // Get orthographic basis from perspective of directional light source
    vec3 light_dir = scenegraph->primary_light.pos;
    vec3 basis[3];
    basis[2] = light_dir;
    basis[0] = normalize(cross(vec3(0, 1, 0), basis[2]));
    basis[1] = normalize(cross(basis[0], basis[2]));
    // Get player view camera information
    camera->applyViewpoint();
    vec3 camera_facing = camera->GetFacing();
    vec3 camera_pos = camera->GetPos();
    mat4 camera_proj_mat = camera->GetProjMatrix();
    mat4 camera_view_mat = camera->GetViewMatrix();
    mat4 inv_camera_mat = invert(camera_proj_mat * camera_view_mat);
    vec3 camera_corner_dir[4];
    camera_corner_dir[0] = inv_camera_mat * vec3(-1.0f, -1.0f, 1.0f);
    camera_corner_dir[1] = inv_camera_mat * vec3(1.0f, -1.0f, 1.0f);
    camera_corner_dir[2] = inv_camera_mat * vec3(1.0f, 1.0f, 1.0f);
    camera_corner_dir[3] = inv_camera_mat * vec3(-1.0f, 1.0f, 1.0f);
    vec4 view_frustum_planes[6];
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 4; ++j) {
            view_frustum_planes[i][j] = camera->frustumPlanes[i][j];
        }
    }
    int num_shadow_view_frustum_planes = 0;
    vec4 shadow_view_frustum_planes[30];
    // Add camera frustum panes if they are facing the direction the light is coming from
    for (const auto& view_frustum_plane : view_frustum_planes) {
        if (dot(light_dir, view_frustum_plane.xyz()) >= 0.0f) {
            shadow_view_frustum_planes[num_shadow_view_frustum_planes] = view_frustum_plane;
            ++num_shadow_view_frustum_planes;
        }
    }
    // Get convex hull of camera frustum in light space
    {
        PROFILER_GPU_ZONE(g_profiler_ctx, "Convex hull");
        vec2 light_space_points[5];
        // bool points_used[5] = {false};
        for (int i = 0; i < 2; ++i) {
            light_space_points[0][i] = dot(camera_pos, basis[i]);
        }
        const float kDist = 10000.0f;
        for (int point = 0; point < 4; ++point) {
            for (int i = 0; i < 2; ++i) {
                light_space_points[point + 1][i] = dot(camera_pos + normalize(camera_corner_dir[point]) * kDist, basis[i]);
            }
        }
        // Get convex hull using 'string wrapping' counter-clockwise
        int hull_points[5];
        int num_hull_points = 1;
        // Start with leftmost point
        float lowest_x = FLT_MAX;
        for (int i = 0; i < 5; ++i) {
            if (light_space_points[i][0] < lowest_x) {
                lowest_x = light_space_points[i][0];
                hull_points[0] = i;
            }
        }
        // points_used[hull_points[0]] = true;
        float last_angle = -PI_f * 0.5f;  // Start with 'string' pointing straight down
        bool done = false;
        while (!done) {
            float best_angle = 0.0f;
            int last_point = hull_points[num_hull_points - 1];
            float lowest_diff = FLT_MAX;
            int best_guess = -1;
            for (int i = 0; i < 5; ++i) {
                if (i != last_point) {
                    vec2 vec = light_space_points[i] - light_space_points[last_point];
                    float angle = atan2(vec[1], vec[0]);
                    while (angle < last_angle) {
                        angle += PI_f * 2.0f;
                    }
                    float diff = angle - last_angle;
                    if (diff < lowest_diff) {
                        best_guess = i;
                        lowest_diff = diff;
                        best_angle = angle;
                    }
                }
            }
            if (best_guess == hull_points[0]) {
                done = true;
            } else {
                LOG_ASSERT(num_hull_points < 5);
                hull_points[num_hull_points] = best_guess;
                ++num_hull_points;
                last_angle = best_angle;
                last_point = best_guess;
            }
        }
        // Extrude convex hull lines into 3D planes
        for (int i = 0; i < num_hull_points; ++i) {
            int points[2] = {hull_points[i], hull_points[(i + 1) % num_hull_points]};
            vec2 dir = light_space_points[points[1]] - light_space_points[points[0]];
            vec2 normal(-dir[1], dir[0]);
            vec3 world_normal = normalize(basis[0] * normal[0] + basis[1] * normal[1]);
            vec3 pos = basis[0] * light_space_points[points[0]][0] + basis[1] * light_space_points[points[0]][1];
            float d = -dot(world_normal, pos);
            shadow_view_frustum_planes[num_shadow_view_frustum_planes] = vec4(world_normal, d);
            ++num_shadow_view_frustum_planes;
        }
    }

    ShadowCascadeParams shadow_params;
    for (int i = 0; i < NUM_SHADOW_CASCADES; ++i) {
        if (i != 0 && !update_all) {
            continue;
        }
        PROFILER_GPU_ZONE(g_profiler_ctx, "Get shadow cascade params");
        float shadow_size = shadow_sizes[i];
        float shadow_radius = shadow_size / 1.414213563731f / 2.0f;  // Get radius of circle inscribed in square with sides 'shadow_size'
        vec3 shadow_center = camera_pos;
        if (i == 0) {
            shadow_center += camera_facing * shadow_radius;
        }
        vec3 light_space_center(dot(shadow_center, basis[0]),
                                dot(shadow_center, basis[1]),
                                dot(shadow_center, basis[2]));
        // Snap center to nearest texel to improve frame-to-frame coherency
        float texel_size = shadow_size * 2.0f / graphics->shadow_res;
        for (int j = 0; j < 3; ++j) {
            light_space_center[j] = floor(light_space_center[j] / texel_size) * texel_size;
        }
        // Update shadow center to new texel-snapped position
        shadow_center = light_space_center[0] * basis[0] +
                        light_space_center[1] * basis[1];

        GLuint res = graphics->cascade_shadow_res / 2;
        uvec4& viewport = shadow_params.viewport[i];
        switch (i) {
            case 0:
                viewport = uvec4(0, 0, res, res);
                break;
            case 1:
                viewport = uvec4(res, 0, res * 2, res);
                break;
            case 2:
                viewport = uvec4(0, res, res, res * 2);
                break;
            case 3:
                viewport = uvec4(res, res, res * 2, res * 2);
                break;
        }
        mat4 proj, view;
        camera->applyShadowViewpoint(light_dir, shadow_center, shadow_size, &proj, &view);
        mat4 proj_view_matrix = proj * view;
        graphics->cascade_shadow_mat[i] = proj_view_matrix;
        shadow_params.proj_view_matrix[i] = proj_view_matrix;
        for (int j = 0; j < 6; ++j) {
            memcpy(&shadow_params.frustum_planes[i][j], &camera->frustumPlanes[j][0], 4 * sizeof(float));
        }
    }

    // Make room for the shadow frustum planes for each cascade
    num_shadow_view_frustum_planes += 6;
    CHECK_GL_ERROR();
    glColorMask(0, 0, 0, 0);
    graphics->drawing_shadow = true;
    graphics->PushFramebuffer();
    graphics->bindFramebuffer(graphics->cascade_shadow_fb);
    graphics->PushViewport();
    graphics->setViewport(0, 0, graphics->cascade_shadow_res, graphics->cascade_shadow_res);
    glEnable(GL_SCISSOR_TEST);

    if (g_single_pass_shadow_cascade) {
        static GLuint shadow_cascade_ubo_id = 0;
        if (shadow_cascade_ubo_id == 0) {
            glGenBuffers(1, &shadow_cascade_ubo_id);
        }
        glBindBufferBase(GL_UNIFORM_BUFFER, UBO_SHADOW_CASCADES, shadow_cascade_ubo_id);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(ShadowCascadeParams), &shadow_params, GL_DYNAMIC_DRAW);

        {  // Perform per-frame, per-camera functions, like character LOD and spawning grass
            PROFILER_GPU_ZONE(g_profiler_ctx, "Pre-draw camera");
            float predraw_time = game_timer.GetRenderTime();
            for (auto obj : scenegraph->objects_) {
                if (!obj->parent) {
                    obj->PreDrawCamera(predraw_time);
                }
            }
        }

        {
            const uvec4& viewport = shadow_params.viewport[0];
            graphics->setViewport(viewport.x, viewport.y, viewport.z, viewport.w);
        }

        int viewport_dim[4];
        for (int i = 1; i < NUM_SHADOW_CASCADES; ++i) {
            const uvec4& viewport = shadow_params.viewport[i];
            viewport_dim[0] = viewport.x;
            viewport_dim[1] = viewport.y;
            viewport_dim[2] = viewport.z - viewport.x;
            viewport_dim[3] = viewport.w - viewport.y;
            glScissorIndexed(i, viewport_dim[0], viewport_dim[1], viewport_dim[2], viewport_dim[3]);
            glViewportIndexedf(i, (float)viewport_dim[0], (float)viewport_dim[1], (float)viewport_dim[2], (float)viewport_dim[3]);
        }

        mat4 proj_view_matrix = shadow_params.proj_view_matrix[0];

        if (kUseShadowCache) {
            DrawShadowCache(proj_view_matrix, "");
        }

        scenegraph->DrawDepthMap(proj_view_matrix, NULL, 0, SceneGraph::kDepthAllShadowCascades, SceneGraph::kStaticAndDynamic);
    } else {
        {  // Perform per-frame, per-camera functions, like character LOD and spawning grass
            PROFILER_GPU_ZONE(g_profiler_ctx, "Pre-draw camera");
            float predraw_time = game_timer.GetRenderTime();
            for (int i = 0; i < scenegraph->objects_.size(); i++) {
                auto obj = scenegraph->objects_[i];
                if (!obj->parent) {
                    obj->PreDrawCamera(predraw_time);
                }
            }
        }
        for (int i = 0; i < NUM_SHADOW_CASCADES; ++i) {
            if (i != 0 && !update_all) {
                continue;
            }
            PROFILER_GPU_ZONE(g_profiler_ctx, "Draw shadow cascade");
            const uvec4& viewport = shadow_params.viewport[i];
            graphics->setViewport(viewport.x, viewport.y, viewport.z, viewport.w);
            graphics->Clear(true);
            mat4 proj_view_matrix = shadow_params.proj_view_matrix[i];

            if (kUseShadowCache) {
                DrawShadowCache(proj_view_matrix, "");
            }
            int cascade = i;
            for (int i = 0; i < 6; ++i) {
                for (int j = 0; j < 4; ++j) {
                    shadow_view_frustum_planes[num_shadow_view_frustum_planes - 6 + i][j] = shadow_params.frustum_planes[cascade][i][j];
                }
            }
            if (i == 0) {
                scenegraph->DrawDepthMap(proj_view_matrix, shadow_view_frustum_planes, num_shadow_view_frustum_planes, SceneGraph::kDepthShadow, SceneGraph::kStaticAndDynamic);
            } else {  // Don't apply camera frustum to larger cascades unless we're updating it every frame
                scenegraph->DrawDepthMap(proj_view_matrix, &shadow_view_frustum_planes[num_shadow_view_frustum_planes - 6], 6, SceneGraph::kDepthShadow, SceneGraph::kStaticOnly);
            }
        }
    }

    glDisable(GL_SCISSOR_TEST);
    graphics->PopViewport();
    graphics->PopFramebuffer();
    graphics->drawing_shadow = false;
    glColorMask(1, 1, 1, 1);
}

void ReadAverageColorsFromCubemap(vec3* avg_color, TextureRef cubemap) {
    Graphics* graphics = Graphics::Instance();
    for (int i = 0; i < 6; ++i) {
        static GLuint cube_map_read_fbo = UINT_MAX;
        graphics->PushFramebuffer();
        if (cube_map_read_fbo == UINT_MAX) {
            graphics->genFramebuffers(&cube_map_read_fbo, "cube_map_read_fbo");
        }
        graphics->bindFramebuffer(cube_map_read_fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, Textures::Instance()->returnTexture(cubemap), 5);
        GLfloat pixels[4 * 4 * 3];
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        {
            PROFILER_ZONE_STALL(g_profiler_ctx, "Read pixels")
            glReadPixels(0, 0, 4, 4, GL_RGB, GL_FLOAT, (GLfloat*)pixels);
        }
        if (ISNAN(pixels[0])) {
            LOGE << "Light probe read fail" << std::endl;
            LOGE << "cube_map_read_fbo: " << cube_map_read_fbo << std::endl;
            LOGE << "scenegraph_->light_probe_collection.cube_map.id: " << cubemap.id << std::endl;
            LOGE << "pixels[0]: " << pixels[0] << std::endl;
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, Textures::Instance()->returnTexture(cubemap), 5);
            glReadBuffer(GL_COLOR_ATTACHMENT0);
            {
                PROFILER_ZONE_STALL(g_profiler_ctx, "Read pixels")
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, Textures::Instance()->returnTexture(cubemap), 4);
                glReadPixels(0, 0, 4, 4, GL_RGB, GL_FLOAT, (GLfloat*)pixels);
            }
            LOGE << "pixels[0]: " << pixels[0] << std::endl;
        }
        LOG_ASSERT(!ISNAN(pixels[0]));
        // Get average of centermost pixels
        avg_color[i] = vec3(0.0f);
        for (int j = 0, index = 0; j < 16; ++j, index += 3) {
            if (j / 4 > 0 && j / 4 < 3 && j % 4 > 0 && j % 4 < 3) {
                avg_color[i] += vec3(pixels[index + 0], pixels[index + 1], pixels[index + 2]) / 4.0f;
            }
        }
        graphics->PopFramebuffer();
    }
}

extern int num_detail_object_draw_calls;

void Engine::NewLevel() {
    Path p = FindFilePath(new_empty_level_path, kAnyPath);
    Engine::Instance()->QueueState(EngineState("", kEngineEditorLevelState, p));
}

bool Engine::GetSplitScreen() const {
    return forced_split_screen_mode == kForcedModeSplit || (forced_split_screen_mode == kForcedModeNone && Graphics::Instance()->config_.split_screen());
}

static const int kCollisionNormalVersion = 2;
static const char* kCollisionNormalIdentifier = "overgrowth_level_collision_normals";

void SaveCollisionNormals(const SceneGraph* scenegraph) {
    std::string path = scenegraph->level_path_.GetOriginalPathStr() + ".col_norm";
    if (scenegraph->level_path_.source != kAbsPath) {
        if (!config["allow_game_dir_save"].toBool()) {
            path = GetWritePath(scenegraph->level_path_.mod_source) + path;
        }
    }
    std::string temp_path = GetWritePath(CoreGameModID).c_str();
    temp_path += "temp_collision_normals";
    FILE* file = my_fopen(temp_path.c_str(), "wb");
    if (file) {
        fwrite(kCollisionNormalIdentifier, strlen(kCollisionNormalIdentifier), 1, file);
        fwrite(&kCollisionNormalVersion, sizeof(int), 1, file);
        int num_objects = 0;
        for (auto eo : scenegraph->visible_static_meshes_) {
            if (eo->GetCollisionModelID() != -1) {
                ++num_objects;
            }
        }
        fwrite(&num_objects, sizeof(int), 1, file);
        for (auto eo : scenegraph->visible_static_meshes_) {
            if (eo->GetCollisionModelID() != -1) {
                int id = eo->GetID();
                fwrite(&id, sizeof(int), 1, file);
                int num_normals = (int)eo->normal_override_custom.size();
                fwrite(&num_normals, sizeof(int), 1, file);
                if (num_normals > 0) {
                    fwrite(&eo->normal_override_custom[0], sizeof(vec4) * num_normals, 1, file);
                }
                int num_ledge_lines = (int)eo->ledge_lines.size();
                fwrite(&num_ledge_lines, sizeof(int), 1, file);
                if (num_ledge_lines > 0) {
                    fwrite(&eo->ledge_lines[0], sizeof(int) * num_ledge_lines, 1, file);
                }
            }
        }
        fclose(file);

        Zip(temp_path, temp_path + ".zip", "data", _YES_OVERWRITE);
        // void UnZip(const std::string &zip_file_path, ExpandedZipFile &expanded_zip_file);

        std::string src = temp_path + ".zip";
        std::string dst = path + ".zip";
        if (movefile(src.c_str(), dst.c_str())) {
            LOGI << "Creating parent dirs before trying to move file again" << std::endl;
            CreateParentDirs(dst);
            if (movefile(src.c_str(), dst.c_str())) {
#ifndef NO_ERR
                DisplayError("Error",
                             (std::string("Problem moving file \"") + strerror(errno) + "\". From " +
                              src +
                              " to " + dst)
                                 .c_str());
#endif
            }
        }
    }
}

static int ParseCollisionNormalsFile(SceneGraph* scenegraph, const std::vector<char>& file) {
    PROFILER_ZONE(g_profiler_ctx, "ParseCollisionNormalsFile");
    char header_buf[64] = {'\0'};
    int index = 0;
    memread(header_buf, strlen(kCollisionNormalIdentifier), 1, file, index);
    if (strcmp(header_buf, kCollisionNormalIdentifier) != 0) {
        const int kBufSize = 512;
        char buf[kBufSize];
        FormatString(buf, kBufSize, "Collision normal header '%s' does not match expected '%s'", header_buf, kCollisionNormalIdentifier);
        DisplayError("Error", buf);
        return -1;
    }
    int version;
    memread(&version, sizeof(int), 1, file, index);
    if (version <= 0 || version > kCollisionNormalVersion) {
        const int kBufSize = 512;
        char buf[kBufSize];
        FormatString(buf, kBufSize, "Collision normal version '%d' does not match expected '%d'", version, kCollisionNormalVersion);
        DisplayError("Error", buf);
        return -2;
    }
    int num_objects;
    memread(&num_objects, sizeof(int), 1, file, index);
    std::vector<vec4> normal_buf;
    std::vector<int> ledge_line_buf;
    for (int i = 0; i < num_objects; ++i) {
        int id;
        memread(&id, sizeof(int), 1, file, index);
        Object* obj = scenegraph->GetObjectFromID(id);
        int num_normals;
        memread(&num_normals, sizeof(int), 1, file, index);
        normal_buf.resize(num_normals);
        if (num_normals > 0) {
            memread(&normal_buf[0], sizeof(vec4) * num_normals, 1, file, index);
        }
        if (version >= 2) {
            int num_ledge_lines;
            memread(&num_ledge_lines, sizeof(int), 1, file, index);
            if (num_ledge_lines > 0) {
                ledge_line_buf.resize(num_ledge_lines);
                memread(&ledge_line_buf[0], sizeof(int) * num_ledge_lines, 1, file, index);
            }
        }
        if (obj && obj->GetType() == _env_object) {
            EnvObject* eo = (EnvObject*)obj;
            eo->normal_override_custom = normal_buf;
            eo->normal_override_buffer_dirty = true;
            if (version >= 2) {
                eo->ledge_lines = ledge_line_buf;
            }
        }
    }
    return 0;
}

void LoadCollisionNormals(SceneGraph* scenegraph) {
    PROFILER_ZONE(g_profiler_ctx, "LoadCollisionNormals");
    std::string path = scenegraph->level_path_.GetOriginalPathStr() + ".col_norm.zip";
    if (FileExists(path, kAnyPath)) {
        char temp_path[kPathSize];
        FindFilePath(path.c_str(), temp_path, kPathSize, kAnyPath);
        ExpandedZipFile expanded_zip_file;
        {
            PROFILER_ZONE(g_profiler_ctx, "Unzipping file");
            UnZip(temp_path, expanded_zip_file);
        }
        std::vector<char> buffer;
        const char* data;
        const char* filename;
        unsigned size;
        expanded_zip_file.GetEntry(0, filename, data, size);
        buffer.resize(size);
        memcpy(&buffer[0], data, size);
        ParseCollisionNormalsFile(scenegraph, buffer);
    }
}

// Draw the current frame
void Engine::Draw() {
    Graphics* graphics = Graphics::Instance();
    Keyboard& keyboard = Input::Instance()->getKeyboard();

    ++draw_frame;
    num_detail_object_draw_calls = 0;
    PROFILER_GPU_ZONE(g_profiler_ctx, "Draw frame %d", draw_frame);

    if (user_paused) {
        const int kBufSize = 512;
        char buf[kBufSize];
        FormatString(buf, kBufSize, "Time is currently paused, press %s to unpause", config["bind[pause]"].str().c_str());
        gui.AddDebugText("paused", buf, 0.0f);
    }

    if (slow_motion) {
        const int kBufSize = 512;
        char buf[kBufSize];
        FormatString(buf, kBufSize, "Time is currently slowed down, press %s to speed back up", config["key[slow]"].str().c_str());
        gui.AddDebugText("slow", buf, 0.0f);
    }

    DrawImGui(graphics, scenegraph_, &gui, GetAssetManager(), Engine::Instance(), cursor.visible);

    if (check_save_level_changes_dialog_is_showing || !scenegraph_) {
        // No level loaded, so draw menu instead
        graphics->setViewport(0, 0, graphics->window_dims[0], graphics->window_dims[1]);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        graphics->Clear(true);
        if (scriptable_menu_) {
            PROFILER_GPU_ZONE(g_profiler_ctx, "Draw scriptable menu", draw_frame);
            scriptable_menu_->Draw();
        }
        RenderImGui();
        ImGui_ImplSdlGL3_RenderDrawLists(ImGui::GetDrawData());
        if (!WantMouseImGui()) {
            cursor.Draw();
        }

        // Limit FPS in menus, even if limit_fps_in_game is not set to true (and thus timing code in GameMain isn't being called)
        if (!graphics->config_.vSync() && !graphics->config_.limit_fps_in_game()) {
            PROFILER_ZONE_IDLE(g_profiler_ctx, "SDL_Sleep");
            if (!g_draw_vr) {
                static uint32_t last_time = 0;

                int max_frame_rate = graphics->config_.max_frame_rate();
                if (max_frame_rate < 15) {
                    max_frame_rate = 15;
                }
                if (max_frame_rate > 500) {
                    max_frame_rate = 500;
                }

                uint32_t ticks_to_wait = (uint32_t)(1000.0f / max_frame_rate);
                uint32_t diff = ticks_to_wait - (SDL_TS_GetTicks() - last_time) - 1;
                if (diff > ticks_to_wait - 1) {
                    diff = ticks_to_wait - 1;
                }

                SDL_Delay(diff);
                last_time = SDL_TS_GetTicks();
            }
        }
    } else {
        game_timer.ReportFrameForFPSCount();
        PushGPUProfileRange(__FUNCTION__);

        // Get number of screens that we need
        const GraphicsConfig& graphics_config = graphics->config_;
        scenegraph_->GetPlayerCharacterIDs(&num_avatars, avatar_ids, kMaxAvatars);
        unsigned num_screens = clamp(num_avatars, 1, 4);
        if (!Engine::Instance()->GetSplitScreen() || scenegraph_->map_editor->state_ != MapEditor::kInGame) {
            num_screens = 1;
        }
        bool old_simple_water = g_simple_water;
        if (num_screens > 1) {
            g_simple_water = true;
        }

        {  // Perform per-frame calculations (like character shadows or LOD)
            PROFILER_GPU_ZONE(g_profiler_ctx, "Pre-draw frame");
            float predraw_time = game_timer.GetRenderTime();
            for (auto obj : scenegraph_->objects_) {
                if (!obj->parent) {
                    obj->PreDrawFrame(predraw_time);
                }
            }
        }

        {
            if (scenegraph_->reflection_data_loaded == false) {
                scenegraph_->LoadReflectionCaptureCubemaps();
            }

            PROFILER_GPU_ZONE(g_profiler_ctx, "Updating reflection capture cubemaps");
            std::vector<TextureRef> textures;
            for (auto obj : scenegraph_->objects_) {
                ReflectionCaptureObject* reflection_obj = (ReflectionCaptureObject*)obj;
                if (obj->GetType() == _reflection_capture_object) {
                    if (reflection_obj->cube_map_ref.valid()) {
                        if (reflection_obj->GetScriptParams()->ASGetInt("Global") == 1) {
                            scenegraph_->sky->SetSpecularCubeMapTexture(reflection_obj->cube_map_ref);
                        }
                    }
                }
            }
            textures.push_back(scenegraph_->sky->GetSpecularCubeMapTexture());
            scenegraph_->ref_cap_matrix.clear();
            scenegraph_->ref_cap_matrix_inverse.clear();
            for (auto obj : scenegraph_->objects_) {
                ReflectionCaptureObject* reflection_obj = (ReflectionCaptureObject*)obj;
                if (obj->GetType() == _reflection_capture_object) {
                    if (reflection_obj->cube_map_ref.valid()) {
                        if (reflection_obj->GetScriptParams()->ASGetInt("Global") != 1) {
                            textures.push_back(reflection_obj->cube_map_ref);
                            const mat4& refmat = obj->GetTransform();
                            scenegraph_->ref_cap_matrix.push_back(refmat);
                            scenegraph_->ref_cap_matrix_inverse.push_back(invert(refmat));
                        }
                    }
                }
            }

            if (g_no_reflection_capture == false) {
                if (textures.size() > 6) {
                    textures.resize(6);
                    if (scenegraph_->ref_cap_matrix.size() > 6) {
                        scenegraph_->ref_cap_matrix.resize(6);
                        scenegraph_->ref_cap_matrix_inverse.resize(6);
                    }
                    LOGI << "Clamping number of cubemaps to 6" << std::endl;
                }

                bool different = false;
                if (textures.size() != scenegraph_->sub_cubemaps.size()) {
                    different = true;
                } else {
                    for (size_t i = 0, len = textures.size(); i < len; ++i) {
                        if (textures[i] != scenegraph_->sub_cubemaps[i]) {
                            different = true;
                        }
                    }
                }
                if (different || scenegraph_->cubemaps_need_refresh) {
                    scenegraph_->cubemaps_need_refresh = false;
                    {
                        LOGI << "Creating cubemaps texture" << std::endl;
                        PROFILER_GPU_ZONE(g_profiler_ctx, "Create cubemaps texture");
                        // cubemaps = Textures::Instance()->makeCubemapArrayTexture(2, 128, 128, GL_RGBA16F, GL_RGBA, 0);
                        Textures::Instance()->setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
                        Textures::Instance()->setWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
                        scenegraph_->cubemaps = Textures::Instance()->makeFlatCubemapArrayTexture(6, 128, 128, GL_RGBA16F, GL_RGBA, 0);
                        Textures::Instance()->SetTextureName(scenegraph_->cubemaps, "Reflection Capture Cubemap Array");
                        Textures::Instance()->DeleteUnusedTextures();
                    }

                    Graphics* graphics = Graphics::Instance();
                    graphics->PushFramebuffer();
                    static bool initialized_framebuffers = false;
                    static GLuint framebuffers[2] = {0, 0};
                    if (initialized_framebuffers == false) {
                        Graphics::Instance()->genFramebuffers(&framebuffers[0], "cubemap_1");
                        Graphics::Instance()->genFramebuffers(&framebuffers[1], "cubemap_2");
                        initialized_framebuffers = true;
                    }
                    glColorMask(true, true, true, true);
                    graphics->setViewport(0, 0, 128 * 6, 128);
                    for (size_t tex_index = 0, num_tex = textures.size(); tex_index < num_tex; ++tex_index) {
                        TextureRef tex = textures[tex_index];
                        Textures::Instance()->EnsureInVRAM(tex);
                        CHECK_GL_ERROR();
                        int width = 128, height = 128, level = 0;
                        while (width >= 1 && height >= 1) {
                            for (int i = 0; i < 6; ++i) {
                                glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[0]);
                                CHECK_GL_ERROR();
                                int tex_id = Textures::Instance()->returnTexture(tex);
                                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, tex_id, level);
                                CHECK_GL_ERROR();
                                glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[1]);
                                CHECK_GL_ERROR();
                                tex_id = Textures::Instance()->returnTexture(scenegraph_->cubemaps);
                                glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex_id, level, tex_index);
                                CHECK_GL_ERROR();
                                glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffers[0]);
                                CHECK_GL_ERROR();
                                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffers[1]);
                                CHECK_GL_ERROR();
                                glDisable(GL_SCISSOR_TEST);
                                glBlitFramebuffer(0, 0, width, height, width * i, 0, width * i + width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
                                glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
                                CHECK_GL_ERROR();
                                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
                                CHECK_GL_ERROR();
                            }
                            ++level;
                            width /= 2;
                            height /= 2;
                        }
                    }
                    graphics->PopFramebuffer();
                    scenegraph_->sub_cubemaps = textures;
                }
            } else {
                scenegraph_->cubemaps.clear();
                scenegraph_->cubemaps_need_refresh = true;
            }
        }

        if (shadow_cache_dirty) {
            PROFILER_GPU_ZONE(g_profiler_ctx, "Update shadow cache");
            if (UpdateShadowCache(scenegraph_)) {
                shadow_cache_dirty = false;
            }
            shadow_cache_dirty_sun_moved = false;  // Always cleared. All the objects get their internal dirty flags set, so iterative processing can handle just a few per frame
            shadow_cache_dirty_level_loaded = false;
        }

        if (!g_simple_shadows && g_level_shadows) {
            UpdateShadowCascades(scenegraph_);
        }

        // Updated dynamic character cubemap if that is enabled
        if (num_avatars != 0) {
            PROFILER_GPU_ZONE(g_profiler_ctx, "Update dynamic cubemap");
            MovementObject* player_obj = (MovementObject*)scenegraph_->GetObjectFromID(avatar_ids[0]);
            if (graphics_config.dynamic_character_cubemap_) {
                if (player_obj) {
                    bool temp_visible = player_obj->visible;
                    player_obj->visible = false;
                    bool media_mode = graphics->media_mode();
                    graphics->SetMediaMode(true);
                    static TextureRef cube_map;
                    static GLuint cube_map_fbo;
                    if (!cube_map.valid()) {
                        CHECK_GL_ERROR();
                        cube_map = Textures::Instance()->makeCubemapTexture(128, 128, GL_RGBA16F, GL_RGBA, Textures::MIPMAPS);
                        Textures::Instance()->SetTextureName(cube_map, "Dynamic Character Cubemap");
                        graphics->genFramebuffers(&cube_map_fbo, "cube_map_fbo");
                        CHECK_GL_ERROR();
                    }
                    player_obj->rigged_object()->cube_map = cube_map;
                    DrawCubeMap(cube_map, player_obj->position, cube_map_fbo, SceneGraph::kStaticAndDynamic);
                    CubemapMipChain(cube_map, Cubemap::SPHERE, NULL);
                    graphics->SetMediaMode(media_mode);
                    player_obj->visible = temp_visible;
                }
            } else {
                player_obj->rigged_object()->cube_map.clear();
            }
        }

        if (kLightProbeEnabled && !scenegraph_->light_probe_collection.to_process.empty()) {
            LightProbeUpdateEntry entry = scenegraph_->light_probe_collection.to_process.front();
            scenegraph_->light_probe_collection.to_process.pop();
            LightProbe* probe = scenegraph_->light_probe_collection.GetProbeFromID(entry.id);

            if (probe) {
                PROFILER_GPU_ZONE(g_profiler_ctx, "Render light probe cubemap")
                bool media_mode = graphics->media_mode();
                graphics->SetMediaMode(true);
                // Increase light intensity to make shadows pure black
                float temp_intensity = scenegraph_->primary_light.intensity;
                if (entry.pass == 0) {
                    scenegraph_->primary_light.intensity = 3.0f;
                } else {
                    scenegraph_->primary_light.intensity = 1.5f;
                }
                // Draw scene from light probe perspective
                DrawCubeMap(scenegraph_->light_probe_collection.cube_map, probe->pos,
                            scenegraph_->light_probe_collection.cube_map_fbo, SceneGraph::kStaticOnly);
                // Blur cubemap to represent different levels of roughness
                CubemapMipChain(scenegraph_->light_probe_collection.cube_map, Cubemap::SPHERE, NULL);
                // Reset the light intensity
                scenegraph_->primary_light.intensity = temp_intensity;
                graphics->SetMediaMode(media_mode);

                // Read pixels from roughest level of cubemap to get ambient cube
                vec3 avg_color[6];
                ReadAverageColorsFromCubemap(avg_color, scenegraph_->light_probe_collection.cube_map);
                for (int i = 0; i < 6; ++i) {
                    if (entry.pass == 0) {
                        probe->ambient_cube_color[i] = avg_color[i];
                    } else if (entry.pass == 1) {
                        probe->ambient_cube_color_buf[i] = avg_color[i];
                    }
                }
            }
            // Finalize second bounce light probes
            if (kLightProbe2pass && scenegraph_->light_probe_collection.to_process.empty()) {
                for (auto& light_probe : scenegraph_->light_probe_collection.light_probes) {
                    for (int j = 0; j < 6; ++j) {
                        light_probe.ambient_cube_color[j] =
                            light_probe.ambient_cube_color_buf[j];
                    }
                }
                kLightProbe2pass = false;
            }

            scenegraph_->light_probe_collection.UpdateTextureBuffer(*scenegraph_->bullet_world_);
        }

        // Clear background if we have more than one screen, since the screens might not
        // completely cover everything
        if (num_screens > 1 || graphics->render_output_dims[0] != graphics->window_dims[0] || graphics->render_output_dims[1] != graphics->window_dims[1]) {
            graphics->startDraw(vec2(0.0f, 0.0f), vec2(1.0f, 1.0f), Graphics::kWindow);
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            graphics->Clear(true);
        }

        // Draw voxel spans
        static const bool kDrawSpans = false;
        if (kDrawSpans) {
            for (int span_index = 0, len = (int)g_voxel_field.spans.size(); span_index < len; ++span_index) {
                std::list<VoxelSpan>& span_list = g_voxel_field.spans[span_index];
                for (auto span : span_list) {
                    int voxel_x = span_index / g_voxel_field.voxel_field_dims[2];
                    int voxel_z = span_index % g_voxel_field.voxel_field_dims[2];
                    vec3 pos = vec3(
                        g_voxel_field.voxel_field_bounds[0][0] + (voxel_x + 0.5f) * g_voxel_field.voxel_size,
                        (span.height[0] + span.height[1]) * 0.5f * g_voxel_field.voxel_size,
                        g_voxel_field.voxel_field_bounds[0][2] + (voxel_z + 0.5f) * g_voxel_field.voxel_size);
                    vec3 box_size = vec3(
                        g_voxel_field.voxel_size * 0.5f,
                        (span.height[0] - span.height[1]) * 0.5f * g_voxel_field.voxel_size,
                        g_voxel_field.voxel_size * 0.5f);
                    DebugDraw::Instance()->AddWireBox(pos, box_size, vec4(1.0f), _delete_on_draw);
                }
            }
        }

        if (scenegraph_->light_probe_collection.light_volume_enabled) {
            PROFILER_GPU_ZONE(g_profiler_ctx, "Update light volume objects");
            LightVolumeObject* lvo = NULL;
            for (auto& light_volume_object : scenegraph_->light_volume_objects_) {
                lvo = light_volume_object;
            }

            if (lvo && lvo->dirty && !IsBeingMoved(scenegraph_->map_editor, lvo)) {
                int dims[3] = {600, 100, 100};
                float* pixels = (float*)alloc.stack.Alloc(dims[0] * dims[1] * dims[2] * 4 * sizeof(float));
                for (int x = 0; x < dims[0] / 6; ++x) {
                    for (int y = 0; y < dims[1]; ++y) {
                        for (int z = 0; z < dims[2]; ++z) {
                            vec3 pos = vec3(x / ((float)dims[0] / 6.0f), y / (float)dims[1], z / (float)dims[2]);
                            pos -= vec3(0.5f);
                            pos *= 2.0f;
                            pos = lvo->GetTransform() * pos;
                            vec3 color[6];
                            int tet = scenegraph_->light_probe_collection.GetTetrahedron(pos, color, -1);
                            for (int i = 0; i < 6; ++i) {
                                vec3 avg;
                                float opac = 1.0f;
                                if (tet != -1) {
                                    avg = color[i];
                                } else {
                                    avg = vec3(0.0f, 0.0f, 0.0f);
                                    opac = 0.0f;
                                }
                                // avg = pos * 0.01f;
                                int index = (z * (dims[0] * dims[1]) + y * (dims[0]) + x + i * dims[0] / 6) * 4;
                                pixels[index + 0] = avg[2];
                                pixels[index + 1] = avg[1];
                                pixels[index + 2] = avg[0];
                                pixels[index + 3] = opac;
                            }
                        }
                    }
                }
                scenegraph_->light_probe_collection.ambient_3d_tex = Textures::Instance()->make3DTexture(dims[0], dims[1], dims[2], GL_RGBA16F, GL_BGRA, false, &(pixels[0]));
                Textures::Instance()->SetTextureName(scenegraph_->light_probe_collection.ambient_3d_tex, "Ambient Light Volume");
                alloc.stack.Free(pixels);
                Textures::Instance()->DeleteUnusedTextures();
                lvo->dirty = false;
            }
        }
        {
            if (g_no_reflection_capture == false) {
                PROFILER_GPU_ZONE(g_profiler_ctx, "Update reflection capture objects");
                for (auto& object : scenegraph_->objects_) {
                    Object* obj = object;
                    ReflectionCaptureObject* reflection_obj = (ReflectionCaptureObject*)obj;
                    if (obj->GetType() == _reflection_capture_object && reflection_obj->dirty) {
                        bool media_mode = graphics->media_mode();
                        graphics->SetMediaMode(true);
                        // Draw scene from light probe perspective
                        if (!reflection_obj->cube_map_ref.valid()) {
                            reflection_obj->cube_map_ref = Textures::Instance()->makeCubemapTexture(128, 128, GL_RGBA16F, GL_RGBA, Textures::MIPMAPS);
                            Textures::Instance()->SetTextureName(reflection_obj->cube_map_ref, "Reflection Capture Cubemap");
                        }
                        DrawCubeMap(reflection_obj->cube_map_ref, object->GetTranslation(),
                                    scenegraph_->light_probe_collection.cube_map_fbo, SceneGraph::kStaticOnly);
                        // Blur cubemap to represent different levels of roughness
                        CubemapMipChain(reflection_obj->cube_map_ref, Cubemap::SPHERE, NULL);
                        graphics->SetMediaMode(media_mode);
                        ReadAverageColorsFromCubemap(reflection_obj->avg_color, reflection_obj->cube_map_ref);
                        ((ReflectionCaptureObject*)object)->dirty = false;
                        scenegraph_->cubemaps_need_refresh = true;
                    }
                }
            }
        }

        CHECK_GL_ERROR();
        // Iterate through screens and render them
        for (unsigned i = 0; i < num_screens; ++i) {
            PROFILER_GPU_ZONE(g_profiler_ctx, "Draw screen");
            PushGPUProfileRange("Draw Screen");
            SetViewportForCamera(i, num_screens, Graphics::kRender);
            ActiveCameras::Set(i);
            ActiveCameras::Get()->SetAutoCamera(config["auto_camera"].toNumber<bool>());
            if (level_loaded_) {  // Draw the in-game scene if level is loaded
                CHECK_GL_ERROR();
                DrawScene(kScreen, Engine::kFinal, SceneGraph::kStaticAndDynamic);
            }
            PopGPUProfileRange();
        }
        CHECK_GL_ERROR();
        // Draw GUI layer
        {
            PROFILER_GPU_ZONE(g_profiler_ctx, "Draw GUI screen");
            PushGPUProfileRange("Draw GUI Screen");
            CHECK_GL_ERROR();
            graphics->setViewport(0, 0, graphics->window_dims[0], graphics->window_dims[1]);
            CHECK_GL_ERROR();
            {
                scenegraph_->level->Draw();
                CHECK_GL_ERROR();
            }
            if (!graphics->media_mode()) {
                CHECK_GL_ERROR();
                {  // Draw alert string
                    if (scenegraph_->map_editor->state_ != MapEditor::kInGame) {
                        if (scenegraph_->level_path_.isValid()) {
                            gui.AddDebugText("_level", scenegraph_->level_path_.GetOriginalPath(), 0.5);
                        }
                        if (graphics->nav_mesh_out_of_date) {
                            gui.AddDebugText("_nav_update", "Nav mesh needs update", 0.5);
                        }
                    }
                }
            }
#ifdef SLIM_TIMING
            slimTimingEvents->drawVisualization(g_text_atlas_renderer, font_renderer, g_text_atlas[kTextAtlasMono]);
#endif  // SLIM_TIMING
            if (config["fps_label"].toNumber<bool>() && !graphics->media_mode() || Online::Instance()->IsActive()) {
                static float fps_updated_time = 0.0f;
                if (fps_updated_time < ui_timer.game_time - 1.0f) {
                    float current = game_timer.GetFrameTime();
                    float fastest = game_timer.GetFastestFrameTime();
                    float slowest = game_timer.GetSlowestFrameTime();

                    int current_fps = (int)(current == 0.0f ? 0 : (1000.0f / current));
                    int fastest_fps = (int)(fastest < 0.00001 ? 0 : (1000.0f / fastest));
                    int slowest_fps = (int)(slowest < 0.00001 ? 0 : (1000.0f / slowest));

                    FormatString(fps_label_str, kFPSLabelMaxLen, "FPS: [%d, %d] current: %d", slowest_fps, fastest_fps, current_fps);
                    FormatString(frame_time_label_str, kFPSLabelMaxLen, "Frame time: [%.3f, %.3f] ms current: %.3f ms", fastest, slowest, current);
                    fps_updated_time = ui_timer.game_time;
                }
                gui.AddDebugText("_framerate", fps_label_str, 0.5);
                gui.AddDebugText("_frametime", frame_time_label_str, 0.5);
            }

            // Make sure glActiveTexture is synced up -- not sure why this should be necessary but it seems to be
            // Otherwise character shadows break in game mode
            Textures::Instance()->InvalidateBindCache();

            if (kDrawHistogram) {
                DrawHistogram();
            }

            PopGPUProfileRange();
        }

        // Draw camera preview
        if (level_loaded_) {  // Draw the in-game scene if level is loaded
            DrawImGuiCameraPreview(this, scenegraph_, graphics);
        }

        if (!graphics->media_mode()) {
            graphics->setViewport(0, 0, graphics->window_dims[0], graphics->window_dims[1]);
            {
                PROFILER_GPU_ZONE(g_profiler_ctx, "Render Dear ImGui screen");
                RenderImGui();
                ImGui_ImplSdlGL3_RenderDrawLists(ImGui::GetDrawData());
            }
            if (!WantMouseImGui()) {
                PROFILER_GPU_ZONE(g_profiler_ctx, "Draw cursor");
                cursor.Draw();
            }
        }

        const float fade_ticks = 300.0f;
        if (level_has_screenshot && (first_level_drawn == UINT_MAX || SDL_TS_GetTicks() < first_level_drawn + fade_ticks)) {
            CHECK_GL_ERROR();

            if (first_level_drawn == UINT_MAX && !waiting_for_input_) {
                first_level_drawn = SDL_TS_GetTicks();
            }

            // no coloring
            Graphics* graphics = Graphics::Instance();
            Shaders* shaders = Shaders::Instance();

            int shader_id = shaders->returnProgram("simple_2d #TEXTURE #FLIPPED #LEVEL_SCREENSHOT");
            shaders->createProgram(shader_id);
            int shader_attrib_vert_coord = shaders->returnShaderAttrib("vert_coord", shader_id);
            int shader_attrib_tex_coord = shaders->returnShaderAttrib("tex_coord", shader_id);
            int uniform_mvp_mat = shaders->returnShaderVariable("mvp_mat", shader_id);
            int uniform_color = shaders->returnShaderVariable("color", shader_id);
            shaders->setProgram(shader_id);
            CHECK_GL_ERROR();

            GLState gl_state;
            gl_state.blend = true;
            gl_state.cull_face = false;
            gl_state.depth_write = false;
            gl_state.depth_test = false;
            graphics->setGLState(gl_state);
            CHECK_GL_ERROR();

            glm::mat4 proj_mat;
            proj_mat = glm::ortho(0.0f, (float)graphics->window_dims[0], 0.0f, (float)graphics->window_dims[1]);
            glm::mat4 modelview_mat(1.0f);
            vec2 where;
            vec2 size(Textures::Instance()->getWidth(level_screenshot->GetTextureRef()),
                      Textures::Instance()->getHeight(level_screenshot->GetTextureRef()));
            size /= size[1];
            size *= (float)graphics->window_dims[1];
            where = vec2(graphics->window_dims[0] * 0.5f, graphics->window_dims[1] * 0.5f);
            modelview_mat = glm::translate(modelview_mat, glm::vec3(where[0] - size[0] * 0.5f, where[1] - size[1] * 0.5f, 0));
            modelview_mat = glm::scale(modelview_mat, glm::vec3(size[0], size[1], 1.0f));
            modelview_mat = glm::translate(modelview_mat, glm::vec3(0.5f, 0.5f, 0.5f));
            modelview_mat = glm::translate(modelview_mat, glm::vec3(-0.5f, -0.5f, -0.5f));
            glm::mat4 mvp_mat = proj_mat * modelview_mat;
            CHECK_GL_ERROR();

            graphics->EnableVertexAttribArray(shader_attrib_vert_coord);
            CHECK_GL_ERROR();
            graphics->EnableVertexAttribArray(shader_attrib_tex_coord);
            CHECK_GL_ERROR();
            static const GLfloat verts[] = {
                0, 0, 0, 0,
                1, 0, 1, 0,
                1, 1, 1, 1,
                0, 1, 0, 1};
            static const GLuint indices[] = {
                0, 1, 2,
                0, 2, 3};
            static VBOContainer vert_vbo;
            static VBOContainer index_vbo;
            static bool vbo_filled = false;
            if (!vbo_filled) {
                vert_vbo.Fill(kVBOFloat | kVBOStatic, sizeof(verts), (void*)verts);
                index_vbo.Fill(kVBOElement | kVBOStatic, sizeof(indices), (void*)indices);
                vbo_filled = true;
            }
            vert_vbo.Bind();
            index_vbo.Bind();

            glVertexAttribPointer(shader_attrib_vert_coord, 2, GL_FLOAT, false, 4 * sizeof(GLfloat), 0);
            CHECK_GL_ERROR();
            glVertexAttribPointer(shader_attrib_tex_coord, 2, GL_FLOAT, false, 4 * sizeof(GLfloat), (void*)(sizeof(GL_FLOAT) * 2));
            CHECK_GL_ERROR();

            int num_indices = 6;

            glUniformMatrix4fv(uniform_mvp_mat, 1, false, (GLfloat*)&mvp_mat);
            CHECK_GL_ERROR();
            vec4 color(1.0f);
            if (!waiting_for_input_) {
                color[3] = 1.0f - (SDL_TS_GetTicks() - first_level_drawn) / fade_ticks;
            }
            glUniform4fv(uniform_color, 1, &color[0]);
            CHECK_GL_ERROR();

            Textures::Instance()->bindTexture(level_screenshot, 0);
            CHECK_GL_ERROR();
            graphics->DrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, 0);
            CHECK_GL_ERROR();

            graphics->BindElementVBO(0);
            graphics->BindArrayVBO(0);
            graphics->ResetVertexAttribArrays();
            CHECK_GL_ERROR();

            if (waiting_for_input_) {  // Draw fun text
                // Draw loading screen tip
                int font_size = int(max(18, min(Graphics::Instance()->window_dims[1] / 30, Graphics::Instance()->window_dims[0] / 50)));
                TextMetrics metrics = g_as_text_context.ASGetTextAtlasMetrics(font_path, font_size, 0, load_screen_tip);
                int pos[] = {graphics->window_dims[0] / 2 - metrics.bounds[2] / 2, graphics->window_dims[1] / 2 - font_size / 2};
                g_as_text_context.ASDrawTextAtlas(font_path, font_size, 0, load_screen_tip, pos[0], pos[1], vec4(1.0f));

                // Add spacing
                pos[1] += metrics.bounds[3] + font_size;

                // Draw button prompt text
                float pulse_while_waiting_for_input = powf(sinf((SDL_TS_GetTicks() - finished_loading_time) / (0.5f * 1000.0f)), 2.0f) / 2.0f + 0.5f;
                std::string button_prompt_text = "Press any key to continue";
                if (Online::Instance()->IsActive()) {
                    if (Online::Instance()->IsHosting()) {
                        if (Online::Instance()->GetPlayerCount() > 0) {
                            if (Online::Instance()->AllClientsReady()) {
                                button_prompt_text = "All clients are ready. \nPress any key to continue";
                            } else {
                                button_prompt_text = "Waiting for all clients to load";
                            }
                        } else {
                            button_prompt_text = "Press any key to continue";
                        }
                    } else {
                        button_prompt_text = "Waiting for the host to start the game";
                    }
                }

                if (load_screen_tip[0] == '\0') {
                    TextMetrics prompt_metrics = g_as_text_context.ASGetTextAtlasMetrics(font_path, font_size, 0, button_prompt_text);
                    pos[0] = graphics->window_dims[0] / 2 - prompt_metrics.bounds[2] / 2;
                }
                g_as_text_context.ASDrawTextAtlas(font_path, font_size, 0, button_prompt_text, pos[0], pos[1], vec4(vec3(pulse_while_waiting_for_input), 1.0f));

                Textures::Instance()->InvalidateBindCache();
            }
        }

        // scenegraph_->decals->PostDraw();
        ActiveCameras::Set(0);

        PopGPUProfileRange();
        g_simple_water = old_simple_water;
    }

    DecalTextures::Instance()->Draw();

    ActiveCameras::Instance()->UpdatePrevViewMats();
    prev_view_time = game_timer.game_time + game_timer.GetInterpWeight() / 120.0f;

    {
        PROFILER_GPU_ZONE(g_profiler_ctx, "Error check");
        if (printed_rendering_error_message == false) {
            if (CheckGLError(0, "", "General error in OpenGL rendering context. Please report with logfile to bugs@wolfire.com")) {
                printed_rendering_error_message = true;
            }
        } else {
            char err[512];
            if (CheckGLErrorStr(err, 512)) {
                LOGI << "OpenGL Error: " << err << std::endl;
            }
        }
    }

#ifdef OpenVR
    if (m_pHMD) {
        vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
        vr::EVRCompositorError err = vr::VRCompositor()->WaitGetPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);
        if (err != vr::VRCompositorError_None) {
            LOGE << "VR WaitGetPoses error: " << err << std::endl;
        }
        vr::TrackedDevicePose_t* hmd_pose = &m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd];
        if (vr::VRCompositor()->CanRenderScene() && hmd_pose->bPoseIsValid) {
            uint32_t dims[2];

            m_pHMD->GetRecommendedRenderTargetSize(&dims[0], &dims[1]);
            static TextureRef resolve_tex = Textures::Instance()->makeRectangularTexture(dims[0], dims[1], GL_RGBA, GL_RGBA);
            {
                Graphics::Instance()->PushFramebuffer();
                graphics->bindFramebuffer(graphics->post_effects.post_framebuffer);
                graphics->framebufferColorTexture2D(graphics->post_effects.temp_screen_tex);
                graphics->bindFramebuffer(0);
                glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, graphics->post_effects.post_framebuffer);
                GLint w = graphics->render_dims[0];
                GLint h = graphics->render_dims[1];
                glBlitFramebuffer(0, 0, Graphics::Instance()->window_dims[0], Graphics::Instance()->window_dims[1],
                                  0, 0, w, h,
                                  GL_COLOR_BUFFER_BIT, GL_NEAREST);
                Graphics::Instance()->PopFramebuffer();
            }
            // RenderControllerAxes();
            // RenderStereoTargets();
            // RenderCompanionWindow();

            for (int eye = 0; eye < 2; ++eye) {
                vr::EVREye vr_eye = eye ? vr::Eye_Left : vr::Eye_Right;
                vr::HmdMatrix44_t vr_proj_mat = m_pHMD->GetProjectionMatrix(vr_eye, 0.2f, 20000.0f);
                vr::HmdMatrix34_t vr_eye_mat = m_pHMD->GetEyeToHeadTransform(vr_eye);
                vr::HmdMatrix34_t vr_head_mat = hmd_pose->mDeviceToAbsoluteTracking;

                mat4 proj_mat(vr_proj_mat.m[0][0], vr_proj_mat.m[1][0], vr_proj_mat.m[2][0], vr_proj_mat.m[3][0],
                              vr_proj_mat.m[0][1], vr_proj_mat.m[1][1], vr_proj_mat.m[2][1], vr_proj_mat.m[3][1],
                              vr_proj_mat.m[0][2], vr_proj_mat.m[1][2], vr_proj_mat.m[2][2], vr_proj_mat.m[3][2],
                              vr_proj_mat.m[0][3], vr_proj_mat.m[1][3], vr_proj_mat.m[2][3], vr_proj_mat.m[3][3]);

                mat4 eye_mat(vr_eye_mat.m[0][0], vr_eye_mat.m[1][0], vr_eye_mat.m[2][0], 0.0,
                             vr_eye_mat.m[0][1], vr_eye_mat.m[1][1], vr_eye_mat.m[2][1], 0.0,
                             vr_eye_mat.m[0][2], vr_eye_mat.m[1][2], vr_eye_mat.m[2][2], 0.0,
                             vr_eye_mat.m[0][3], vr_eye_mat.m[1][3], vr_eye_mat.m[2][3], 1.0f);

                mat4 head_mat(vr_head_mat.m[0][0], vr_head_mat.m[1][0], vr_head_mat.m[2][0], 0.0,
                              vr_head_mat.m[0][1], vr_head_mat.m[1][1], vr_head_mat.m[2][1], 0.0,
                              vr_head_mat.m[0][2], vr_head_mat.m[1][2], vr_head_mat.m[2][2], 0.0,
                              vr_head_mat.m[0][3], vr_head_mat.m[1][3], vr_head_mat.m[2][3], 1.0f);

                const bool kDrawImaxDisplay = true;
                if (kDrawImaxDisplay) {
                    Graphics::Instance()->PushFramebuffer();
                    graphics->bindFramebuffer(graphics->post_effects.post_framebuffer);
                    graphics->framebufferColorTexture2D(resolve_tex);
                    graphics->setViewport(0, 0, dims[0], dims[1]);
                    graphics->Clear(true);

                    Shaders* shaders = Shaders::Instance();
                    int shader_id = Shaders::Instance()->returnProgram("simple_tex_3d #VR_DISPLAY");
                    shaders->setProgram(shader_id);
                    shaders->SetUniformVec4("color", vec4(1.0f));
                    int vert_coord_id = shaders->returnShaderAttrib("vert_coord", shader_id);
                    int tex_coord_id = shaders->returnShaderAttrib("tex_coord", shader_id);
                    graphics->BindArrayVBO(0);
                    CHECK_GL_ERROR();

                    GLState temp_gl_state;
                    temp_gl_state.blend = false;
                    temp_gl_state.cull_face = false;
                    temp_gl_state.depth_test = false;
                    temp_gl_state.depth_write = false;
                    graphics->setGLState(temp_gl_state);
                    CHECK_GL_ERROR();

                    float aspect = graphics->render_dims[0] / (float)graphics->render_dims[1];

                    mat4 translate_screen;
                    mat4 scale;
                    mat4 translate_offset;

                    translate_screen.SetTranslation(vec3(0, 0, -10.0f));
                    scale.SetScale(vec3(10.0f * aspect, 10.0f, 10.0f));
                    translate_offset.SetTranslation(vec3(-0.5f, -0.5f, 0.0f));

                    mat4 model = translate_screen * scale * translate_offset;

                    mat4 m_mvp = proj_mat * invert(head_mat) * invert(eye_mat) * model;
                    shaders->SetUniformMat4("mvp_mat", m_mvp);
                    Textures::Instance()->bindTexture(graphics->post_effects.temp_screen_tex);

                    graphics->EnableVertexAttribArray(vert_coord_id);
                    graphics->EnableVertexAttribArray(tex_coord_id);
                    quad_vert_vbo.Bind();
                    quad_index_vbo.Bind();
                    glVertexAttribPointer(vert_coord_id, 2, GL_FLOAT, false, 2 * sizeof(GLfloat), 0);
                    glVertexAttribPointer(tex_coord_id, 2, GL_FLOAT, false, 2 * sizeof(GLfloat), 0);
                    graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                    graphics->ResetVertexAttribArrays();
                    CHECK_GL_ERROR();
                    DebugDraw::Instance()->Draw();
                    Graphics::Instance()->PopFramebuffer();
                }

                vr::Texture_t vr_tex;
                vr_tex.eType = vr::TextureType_OpenGL;
                vr_tex.eColorSpace = vr::ColorSpace_Auto;
                vr_tex.handle = (void*)Textures::Instance()->returnTexture(resolve_tex);

                vr::IVRCompositor* compositor = vr::VRCompositor();
                err = vr::VRCompositor()->Submit(vr_eye, &vr_tex);
                if (err != vr::VRCompositorError_None) {
                    LOGE << "VR Submit error: " << err << std::endl;
                }
                glFlush();
            }
            glFlush();
        }
    }
#endif

#ifdef OculusVR
    if (g_oculus_vr_activated) {
        bool old_g_simple_water = g_simple_water;
        g_simple_water = true;
        PROFILER_ZONE(g_profiler_ctx, "VR");
        Graphics::Instance()->PushFramebuffer();
        ovrSessionStatus sessionStatus;
        ovr_GetSessionStatus(session, &sessionStatus);
        if (sessionStatus.ShouldQuit) {
            Input::Instance()->RequestQuit();
        }
        if (sessionStatus.ShouldRecenter)
            ovr_RecenterTrackingOrigin(session);

        if (sessionStatus.IsVisible) {
            PROFILER_ENTER(g_profiler_ctx, "VR Setup");
            {
                graphics->bindFramebuffer(graphics->post_effects.post_framebuffer);
                graphics->framebufferColorTexture2D(Graphics::Instance()->post_effects.temp_screen_tex);
                graphics->bindFramebuffer(0);
                Graphics::Instance()->PushFramebuffer();
                glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, graphics->post_effects.post_framebuffer);
                GLint w = graphics->render_dims[0];
                GLint h = graphics->render_dims[1];
                glBlitFramebuffer(0, 0, Graphics::Instance()->window_dims[0], Graphics::Instance()->window_dims[1],
                                  0, 0, w, h,
                                  GL_COLOR_BUFFER_BIT, GL_NEAREST);
                Graphics::Instance()->PopFramebuffer();
            }

            // Call ovr_GetRenderDesc each frame to get the ovrEyeRenderDesc, as the returned values (e.g. HmdToEyeOffset) may change at runtime.
            ovrEyeRenderDesc eyeRenderDesc[2];
            eyeRenderDesc[0] = ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
            eyeRenderDesc[1] = ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);

            // Get eye poses, feeding in correct IPD offset
            ovrPosef EyeRenderPose[2];
            ovrVector3f HmdToEyeOffset[2] = {eyeRenderDesc[0].HmdToEyeOffset,
                                             eyeRenderDesc[1].HmdToEyeOffset};

            double sensorSampleTime;  // sensorSampleTime is fed into the layer later
            ovr_GetEyePoses(session, frameIndex, ovrTrue, HmdToEyeOffset, EyeRenderPose, &sensorSampleTime);

            PROFILER_LEAVE(g_profiler_ctx);  // VR SETUP

            ovrInputState touchState;
            ovr_GetInputState(session, ovrControllerType_Active, &touchState);
            ovrTrackingState trackingState = ovr_GetTrackingState(session, 0.0, false);

            static std::vector<int> debug_draw_vr;
            for (int i = 0, len = debug_draw_vr.size(); i < len; ++i) {
                DebugDraw::Instance()->Remove(debug_draw_vr[i]);
            }
            debug_draw_vr.clear();

            {
                const int kBufSize = 512;
                char key[kBufSize], val[kBufSize];
                FormatString(val, kBufSize, "touchState.Buttons: %u", touchState.Buttons);
                gui.AddDebugText("Buttons", val, 0.5f);
                FormatString(val, kBufSize, "touchState.Touches: %u", touchState.Touches);
                gui.AddDebugText("Touches", val, 0.5f);
                FormatString(val, kBufSize, "touchState.ControllerType: %#010x", (unsigned)touchState.ControllerType);
                gui.AddDebugText("ControllerType", val, 0.5f);

                for (int hand = 0; hand < 2; ++hand) {
                    if (trackingState.HandStatusFlags[hand] & ovrStatus_PositionTracked) {
                        vec3 pos = *(vec3*)&trackingState.HandPoses[hand].ThePose.Position;
                        FormatString(key, kBufSize, "handpos%d", hand);
                        FormatString(val, kBufSize, "trackingState.HandPoses[%d].ThePose.Position: %f %f %f", hand, pos[0], pos[1], pos[2]);
                        quaternion quat = *(quaternion*)&trackingState.HandPoses[hand].ThePose.Orientation;

                        mat4 scale;
                        scale.SetUniformScale(0.05f);
                        mat4 transform = Mat4FromQuaternion(quat) * scale;
                        transform.SetTranslationPart(ActiveCameras::Get()->GetPos() + pos);
                        debug_draw_vr.push_back(
                            DebugDraw::Instance()->AddTransformedWireScaledSphere(transform,
                                                                                  vec4(1.0f),
                                                                                  _fade));
                        vec3 points[2];
                        points[0] = transform * vec3(0, 0, 0);
                        points[1] = transform * vec3(0, 0, 0) + (transform * vec4(0, 0, -100000, 0)).xyz();
                        debug_draw_vr.push_back(
                            DebugDraw::Instance()->AddLine(points[0], points[1],
                                                           vec4(1.0f),
                                                           _fade));

                        float dist = -10.0;
                        if (points[1][2] < dist) {
                            float t = (dist - points[0][2]) / (points[1][2] - points[0][2]);
                            vec3 pos = points[0] * (1.0 - t) + points[1] * t;
                            vec2 pixel((pos[0]) * graphics->window_dims[1] * 0.1f + graphics->window_dims[0] * 0.5f,
                                       (5.0f - pos[1]) * graphics->window_dims[1] * 0.1f);
                            if (pixel[0] > 0.0f && pixel[1] > 0.0f &&
                                pixel[0] <= graphics->window_dims[0] &&
                                pixel[1] <= graphics->window_dims[1]) {
                                debug_draw_vr.push_back(
                                    DebugDraw::Instance()->AddWireSphere(pos,
                                                                         0.1,
                                                                         vec4(1.0f, 0.0f, 0.0f, 1.0f),
                                                                         _fade));
                                SDL_WarpMouseInWindow(Graphics::Instance()->sdl_window_, pixel[0], pixel[1]);
                                if (touchState.IndexTrigger[hand] > 0.5f) {
                                    SDL_MouseButtonEvent event;
                                    event.type = SDL_MOUSEBUTTONDOWN;
                                    event.button = 1;
                                    event.clicks = 1;
                                    queued_events.push(*(SDL_Event*)&event);
                                } else {
                                    SDL_MouseButtonEvent event;
                                    event.type = SDL_MOUSEBUTTONUP;
                                    event.button = 1;
                                    queued_events.push(*(SDL_Event*)&event);
                                }
                            }
                        }
                    }
                }
            }

            // Render Scene to Eye Buffers
            for (int eye = 0; eye < 2; ++eye) {
                PROFILER_ZONE(g_profiler_ctx, "VR draw eye screen");
                glDisable(GL_SCISSOR_TEST);
                // Switch to eye render target
                graphics->setDepthTest(true);
                graphics->setDepthWrite(true);
                glEnable(GL_DEPTH_TEST);
                eyeRenderTexture[eye]->SetAndClearRenderSurface(eyeDepthBuffer[eye]);
                float Yaw = 0.0f;

                // Get view and projection matrices
                OVR::Matrix4f rollPitchYaw = OVR::Matrix4f::RotationY(Yaw);
                OVR::Matrix4f finalRollPitchYaw = rollPitchYaw * OVR::Matrix4f(EyeRenderPose[eye].Orientation);
                OVR::Vector3f finalUp = finalRollPitchYaw.Transform(OVR::Vector3f(0, 1, 0));
                OVR::Vector3f finalForward = finalRollPitchYaw.Transform(OVR::Vector3f(0, 0, -1));
                OVR::Vector3f shiftedEyePos = rollPitchYaw.Transform(*(OVR::Vector3f*)&((*(vec3*)&EyeRenderPose[eye].Position) + ActiveCameras::Get()->GetPos()));

                OVR::Matrix4f view = OVR::Matrix4f::LookAtRH(shiftedEyePos, shiftedEyePos + finalForward, finalUp);
                OVR::Matrix4f proj = ovrMatrix4f_Projection(hmdDesc.DefaultEyeFov[eye], 0.2f, 20000.0f, ovrProjection_None);

                const bool kDrawImaxDisplay = (!scenegraph_);
                if (kDrawImaxDisplay) {
                    Shaders* shaders = Shaders::Instance();
                    int shader_id = Shaders::Instance()->returnProgram("simple_tex_3d #VR_DISPLAY");
                    shaders->setProgram(shader_id);
                    shaders->SetUniformVec4("color", vec4(1.0f));
                    int vert_coord_id = shaders->returnShaderAttrib("vert_coord", shader_id);
                    int tex_coord_id = shaders->returnShaderAttrib("tex_coord", shader_id);
                    graphics->BindArrayVBO(0);
                    CHECK_GL_ERROR();

                    GLState temp_gl_state;
                    temp_gl_state.blend = false;
                    temp_gl_state.cull_face = false;
                    temp_gl_state.depth_test = true;
                    temp_gl_state.depth_write = true;
                    graphics->setGLState(temp_gl_state);
                    CHECK_GL_ERROR();

                    float aspect = graphics->render_dims[0] / (float)graphics->render_dims[1];
                    OVR::Matrix4f model = OVR::Matrix4f::Translation(0, 0, -10) * OVR::Matrix4f::Scaling(10.0f * aspect, 10.0f, 10.0f) * OVR::Matrix4f::Translation(-0.5, -0.5, 0);

                    mat4 m_proj, m_view, m_model;
                    memcpy(&m_proj, &proj, sizeof(mat4));
                    memcpy(&m_view, &view, sizeof(mat4));
                    memcpy(&m_model, &model, sizeof(mat4));
                    mat4 m_mvp = transpose(m_proj) * transpose(m_view) * transpose(m_model);
                    shaders->SetUniformMat4("mvp_mat", m_mvp);
                    Textures::Instance()->bindTexture(graphics->post_effects.temp_screen_tex);

                    graphics->EnableVertexAttribArray(vert_coord_id);
                    graphics->EnableVertexAttribArray(tex_coord_id);
                    quad_vert_vbo.Bind();
                    quad_index_vbo.Bind();
                    glVertexAttribPointer(vert_coord_id, 2, GL_FLOAT, false, 2 * sizeof(GLfloat), 0);
                    glVertexAttribPointer(tex_coord_id, 2, GL_FLOAT, false, 2 * sizeof(GLfloat), 0);
                    graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                    graphics->ResetVertexAttribArrays();
                    CHECK_GL_ERROR();

                    memcpy(&m_proj, &proj, sizeof(mat4));
                    memcpy(&m_view, &view, sizeof(mat4));
                    m_view = transpose(m_view);
                    m_proj = transpose(m_proj);
                    ActiveCameras::Get()->SetMatrices((float*)&m_view, (float*)&m_proj);
                    DebugDraw::Instance()->Draw();
                }

                const bool kDrawVRScene = (scenegraph_);
                if (kDrawVRScene) {
                    PROFILER_GPU_ZONE(g_profiler_ctx, "Draw screen");
                    PushGPUProfileRange("Draw Screen");
                    if (level_loaded_) {  // Draw the in-game scene if level is loaded
                        mat4 m_proj, m_view, m_model;
                        memcpy(&m_proj, &proj, sizeof(mat4));
                        memcpy(&m_view, &view, sizeof(mat4));
                        m_view = transpose(m_view);
                        m_proj = transpose(m_proj);
                        auto cam = ActiveCameras::Get();
                        vec3 cam_pos;
                        memcpy(&cam_pos, &shiftedEyePos, sizeof(cam_pos));
                        auto old_interp = cam->interp_pos;
                        cam->interp_pos = cam_pos;
                        cam->SetMatrices((float*)&m_view, (float*)&m_proj);

                        graphics->setDepthFunc(GL_LEQUAL);
                        graphics->setDepthTest(true);
                        graphics->setDepthWrite(true);

                        cam->calcFrustumPlanes(m_proj, m_view);

                        if (!g_no_decals) {
                            scenegraph_->PrepareLightsAndDecals(vec2(0, 0), vec2(1, 1), vec2(eyeRenderTexture[eye]->texSize.w, eyeRenderTexture[eye]->texSize.h));
                        }
                        scenegraph_->DrawDepthMap(m_proj * m_view,
                                                  (const vec4*)ActiveCameras::Get()->frustumPlanes,
                                                  6,
                                                  SceneGraph::kDepthPrePass,
                                                  SceneGraph::kStaticAndDynamic);
                        scenegraph_->Draw(SceneGraph::kStaticAndDynamic);
                        DebugDraw::Instance()->Draw();
                        cam->interp_pos = old_interp;
                    }
                    PopGPUProfileRange();
                }

                // Avoids an error when calling SetAndClearRenderSurface during next iteration.
                // Without this, during the next while loop iteration SetAndClearRenderSurface
                // would bind a framebuffer with an invalid COLOR_ATTACHMENT0 because the texture ID
                // associated with COLOR_ATTACHMENT0 had been unlocked by calling wglDXUnlockObjectsNV.
                eyeRenderTexture[eye]->UnsetRenderSurface();

                // Commit changes to the textures so they get picked up frame
                eyeRenderTexture[eye]->Commit();
            }

            // Do distortion rendering, Present and flush/sync

            ovrLayerEyeFov ld;
            ld.Header.Type = ovrLayerType_EyeFov;
            ld.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;  // Because OpenGL.

            for (int eye = 0; eye < 2; ++eye) {
                ld.ColorTexture[eye] = eyeRenderTexture[eye]->TextureChain;
                ld.Viewport[eye] = OVR::Recti(eyeRenderTexture[eye]->GetSize());
                ld.Fov[eye] = hmdDesc.DefaultEyeFov[eye];
                ld.RenderPose[eye] = EyeRenderPose[eye];
                ld.SensorSampleTime = sensorSampleTime;
            }

            ovrLayerHeader* layers = &ld.Header;
            {
                PROFILER_ZONE(g_profiler_ctx, "VR submit frame");
                ovrResult result = ovr_SubmitFrame(session, frameIndex, NULL, &layers, 1);
                // exit the rendering loop if submit returns an error, will retry on ovrError_DisplayLost
                if (!OVR_SUCCESS(result)) {
                    ovrErrorInfo errorInfo;
                    ovr_GetLastErrorInfo(&errorInfo);
                    FatalError("Error", "ovr_SubmitFrame failed: %s", errorInfo.ErrorString);
                }
            }

            frameIndex++;
        }

        // Blit mirror texture to back buffer
        const bool kMirror = false;
        if (kMirror) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBO);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            GLint w = Graphics::Instance()->window_dims[0];
            GLint h = Graphics::Instance()->window_dims[1];
            glBlitFramebuffer(0, h, w, 0,
                              0, 0, w, h,
                              GL_COLOR_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        }
        Graphics::Instance()->PopFramebuffer();

        g_simple_water = old_g_simple_water;
    }
#endif  // OculusVR
}

void Engine::LoadConfigFile() {
    static const int kBufSize = 512;
    char defaults_path[kBufSize];
    if (FindFilePath("Data/defaults.txt", defaults_path, kBufSize, kDataPaths | kModPaths) == -1) {
        FatalError("Error", "Working directory is probably not set correctly -- could not find defaults.txt in Data folder");
    }
    default_config.Load(defaults_path);

    std::string config_path = GetConfigPath();
    if (!config.Load(config_path)) {
        // If no config file, then load defaults file, and create config file from that
        if (!config.Load(defaults_path)) {
            std::string abs_path = AbsPathFromRel("/Data");
            FatalError("Error", "Working directory is probably not set correctly -- could not find config.txt or defaults.txt in Data folder: %s", abs_path.c_str());
        }
        config.Save(config_path);
    } else {
        // If config file is found, then fill in missing values with the defaults
        Config old_config = config;
        config.Load(defaults_path, true);
        if (old_config != config) {
            config.Save(config_path);
        }
    }

    config.GetRef("menu_player_config") = 0;

    Graphics::Instance()->SetFromConfig(config);
    Input::Instance()->SetFromConfig(config);
    // Textures::Instance()->SetProcessPoolsEnabled(config["background_process_pool"].toNumber<bool>());
    ActiveCameras::Get()->SetAutoCamera(config["auto_camera"].toNumber<bool>());
    DebugDrawAux::Instance()->visible_sound_spheres = config["visible_sound_spheres"].toNumber<bool>();
    g_albedo_only = config["albedo_only"].toNumber<bool>();
    g_disable_fog = config["disable_fog"].toNumber<int>();
    g_no_reflection_capture = config["no_reflection_capture"].toNumber<bool>();
    g_no_decals = config["no_decals"].toNumber<bool>();
    g_no_decal_elements = config["no_decal_elements"].toNumber<bool>();
    g_no_detailmaps = config["no_detailmaps"].toNumber<bool>();
    g_single_pass_shadow_cascade = config["single_pass_shadow_cascade"].toNumber<bool>();
    sound.SetMusicVolume(config["music_volume"].toNumber<float>());
    sound.SetMasterVolume(config["master_volume"].toNumber<float>());
    std::string extra_data_path = config["extra_data_path"].str();

    // Only care about data path if it's specified.
    if (extra_data_path.length() > 0) {
        if (CheckFileAccess(extra_data_path.c_str())) {
            AddPath(extra_data_path.c_str(), kDataPaths);
        } else {
            std::stringstream ss;
            ss << "Could not find: " << extra_data_path << std::endl
               << " Check extra_data_path in config file";
            DisplayError("Warning", ss.str().c_str(), _ok, false);
        }
    } else {
        LOGE << "No extra_data_path was specified." << std::endl;
    }

    Shaders::Instance()->shader_dir_path = "Data/GLSL/";
    script_dir_path = "Data/Scripts/";
    std::string mod_path = config["mod_path0"].str();
    if (!mod_path.empty()) {
        if (CheckFileAccess(mod_path.c_str())) {
            AddPath(mod_path.c_str(), kModPaths);
        } else {
            const int kBufSize = 512;
            char err[kBufSize];
            FormatString(err, kBufSize, "Could not open path: %s\n", mod_path.c_str());
            DisplayError("Error", err);
        }
    }
}

void Engine::PreloadAssets(const Path& level_path) {
    // Release currently hold preloaded from previous level so they may get retagged.
    asset_manager.ReleaseAssetHoldLoad(HOLD_LOAD_MASK_PRELOAD);

    std::vector<LevelAssetPreloadParser::Asset>& preload_files = AssetPreload::Instance().GetPreloadFiles();
    LOGI << "Starting Preloading for: " << level_path << std::endl;
    for (auto& preload_file : preload_files) {
        if (preload_file.all_levels || preload_file.level_name == level_path.GetOriginalPathStr()) {
            // LOGI << "Preloading: " << preload_files[i].path << " " << preload_files[i].asset_type << " " << preload_files[i].load_flags << std::endl;
            Path p = FindFilePath(preload_file.path, kAnyPath, false);
            if (p.isValid()) {
                GetAssetManager()->LoadSync(preload_file.asset_type, preload_file.path, preload_file.load_flags, HOLD_LOAD_MASK_PRELOAD);
            }
        }
    }
}

void Engine::LoadLevelData(const Path& level_path) {
    LOGI << "Load Level: \"" << level_path << "\"" << std::endl;
#if THREADED
    PROFILER_NAME_THREAD(g_profiler_ctx, "Level loading thread");
    NameCurrentThread("Level loading thread");
#endif
    PROFILER_ZONE(g_profiler_ctx, "Loading level");

    LOG_ASSERT(scenegraph_->bullet_world_ == NULL);
    scenegraph_->bullet_world_ = new BulletWorld();
    scenegraph_->bullet_world_->Init();
    scenegraph_->bullet_world_->SetGravity(Physics::Instance()->gravity);
    scenegraph_->bullet_world_->CreatePlane(vec3(0.0f, 1.0f, 0.0f), -100.0f);

    scenegraph_->abstract_bullet_world_ = new BulletWorld();
    scenegraph_->abstract_bullet_world_->Init();

    scenegraph_->plant_bullet_world_ = new BulletWorld();
    scenegraph_->plant_bullet_world_->Init();

    // sound.AttachBulletWorld(scenegraph_->bullet_world_);

    AddLoadingText("Loading level...");

    if (LevelLoader::LoadLevel(level_path, *scenegraph_) == false) {
        loading_mutex_.lock();
        finished_loading_time = (float)SDL_TS_GetTicks();
        loading_in_progress_ = false;
        waiting_for_input_ = false;
        loading_mutex_.unlock();
        return;
    }

    AddLoadingText("Preloading assets...");

    PreloadAssets(level_path);

    // Clear all unused assets, after loading new level.
    while (asset_manager.UnloadUnreferenced(0, 0)) {
    }

    loading_mutex_.lock();

    LoadCollisionNormals(scenegraph_);

    {
        PROFILER_ZONE(g_profiler_ctx, "UpdateControlledModule");
        avatar_control_manager.Update();
    }

    finished_loading_time = (float)SDL_TS_GetTicks();
    loading_in_progress_ = false;
    waiting_for_input_ = true;
    scenegraph_->map_editor->QueueSaveHistoryState();

    loading_mutex_.unlock();
}

void Engine::ClearLoadedLevel() {
    // We want music in menues now, levels have to clear their own sounds.
    sound.ClearTransient();
    sound.UpdateGameTimestep(game_timer.timestep);
    sound.UpdateGameTimescale(1.0f);
    sound.Update();
    if (scenegraph_) {
        scenegraph_->Dispose();
        delete scenegraph_;
        scenegraph_ = NULL;
    }
    Models::Instance()->Dispose();
    Online::Instance()->ClearIDTranslations();

    asset_manager.Update();
    asset_manager.DumpLoadWarningData("asset_manager_warnings.xml");
    ActiveCameras::Get()->SetCameraObject(NULL);
    level_loaded_ = false;
    DebugDraw::Instance()->Dispose();

    if (config["full_level_unload"].toBool()) {
        LOGI << "Unloading all unreferenced assets after clearing level [full_level_unload: true]..." << std::endl;
        size_t pre_count = asset_manager.GetLoadedAssetCount();
        size_t tex_pre_count = asset_manager.GetAssetTypeCount(TEXTURE_ASSET);
        while (asset_manager.UnloadUnreferenced(0, 0)) {
        }
        Textures::Instance()->DeleteUnusedTextures();
        LOGI << (pre_count - asset_manager.GetLoadedAssetCount()) << " Assets were unloaded" << std::endl;
        LOGI << (tex_pre_count - asset_manager.GetAssetTypeCount(TEXTURE_ASSET)) << " Texture assets were unloaded" << std::endl;
    } else {
        LOGE << "Skipping post-clear level unload (Increases peak memory usage)[full_level_unload: false]" << std::endl;
    }
}

void Engine::LoadLevel(Path queued_level) {
    GetAssetManager()->SetLoadWarningMode(false, "", "");
    Shaders::Instance()->level_path = queued_level;
    Shaders::Instance()->create_program_warning = false;

    PROFILER_ZONE(g_profiler_ctx, "Loading queued level");
    PROFILER_NAME_TIMELINE(g_profiler_ctx, "Loading level");

    Path level_path = queued_level;

    latest_level_path_ = level_path;

    std::string difficulty = config.GetClosestDifficulty();

    if (difficulty == "Casual") {
        loading_screen_og_logo = loading_screen_og_logo_casual;
    } else if (difficulty == "Hardcore") {
        loading_screen_og_logo = loading_screen_og_logo_hardcore;
    } else if (difficulty == "Expert") {
        loading_screen_og_logo = loading_screen_og_logo_expert;
    }

    if (level_path.isValid()) {
        LevelInfoAssetRef lia = Engine::Instance()->GetAssetManager()->LoadSync<LevelInfoAsset>(level_path.GetOriginalPath());
        if (lia.valid()) {
            Path shortest_path = FindShortestPath2(level_path.GetFullPath());
            const char* shortest_path_orig_folder = shortest_path.GetOriginalPath();

            if (beginswith(shortest_path_orig_folder, "Data/Levels/")) {
                shortest_path_orig_folder = &shortest_path_orig_folder[strlen("Data/Levels/")];
            }

            level_has_screenshot = false;
            level_screenshot.clear();
            char screenshot_path[kPathSize];

            if (lia->GetLoadingScreenImage().empty() == false) {
                strscpy(screenshot_path, lia->GetLoadingScreenImage().c_str(), kPathSize);
            } else {
                LOGI << "No loading screen image, loading normal." << std::endl;

                FormatString(screenshot_path, kPathSize, "Data/LevelLoading/%s_loading.jpg", shortest_path_orig_folder);
            }

            if (FileExists(screenshot_path, kModPaths | kDataPaths)) {
                level_screenshot = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(screenshot_path, PX_NOCONVERT | PX_NOREDUCE | PX_NOMIPMAP, 0x0);
            } else if (Online::Instance()->IsActive()) {
                // Multiplayer needs an image for the "waiting for all clients" dialogue!
                strscpy(screenshot_path, "Data/Images/full_fallback.png", kPathSize);
                if (FileExists(screenshot_path, kModPaths | kDataPaths)) {
                    level_screenshot = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(screenshot_path, PX_NOCONVERT | PX_NOREDUCE | PX_NOMIPMAP, 0x0);
                } else {
                    level_screenshot = loading_screen_logo;  // Won't render correctly! Still better than nothing!
                }
            }

            if (level_screenshot.valid()) {
                level_has_screenshot = true;
                first_level_drawn = UINT_MAX;
            }

            AddLevelPathToRecentLevels(level_path);

            ClearLoadedLevel();
            forced_split_screen_mode = kForcedModeNone;
            paused = false;
            menu_paused = false;
            user_paused = false;
            slow_motion = false;
            check_save_level_changes_dialog_is_showing = false;
            check_save_level_changes_dialog_quit_if_not_cancelled = false;
            check_save_level_changes_dialog_is_finished = false;
            check_save_level_changes_last_result = false;
            shadow_cache_dirty = true;
            shadow_cache_dirty_sun_moved = true;
            shadow_cache_dirty_level_loaded = true;
            LOG_ASSERT(scenegraph_ == NULL);
            scenegraph_ = new SceneGraph();
            scenegraph_->cubemaps_need_refresh = true;
            scenegraph_->light_probe_collection.probe_lighting_enabled = config["tet_mesh_lighting"].toBool();
            scenegraph_->light_probe_collection.light_volume_enabled = config["light_volume_lighting"].toBool();
            ASData as_data;
            as_data.scenegraph = scenegraph_;
            as_data.gui = &gui;
            scenegraph_->particle_system = new ParticleSystem(as_data);
            scenegraph_->particle_system->particle_types = &particle_types;
            scenegraph_->sky = new Sky();
            scenegraph_->flares.scenegraph = scenegraph_;
            scenegraph_->map_editor = new MapEditor();
            scenegraph_->map_editor->gui = &gui;
            scenegraph_->map_editor->Initialize(scenegraph_);
            scenegraph_->map_editor->state_ = MapEditor::kIdle;
            scenegraph_->map_editor->SetTypeEnabled(_light_probe_object, scenegraph_->light_probe_collection.show_probes);
            scenegraph_->level = new Level();
            scenegraph_->level->Initialize(scenegraph_, &gui);
            scenegraph_->level->script_params().SetObjectID(std::numeric_limits<uint32_t>::max());
            primary_light = &scenegraph_->primary_light;

            AddLoadingText("Starting to load \"" + level_path.GetFullPathStr() + "\"");

            Input::Instance()->SetGrabMouse(false);
            UIShowCursor(true);

            started_loading_time = SDL_TS_GetTicks();
            load_screen_tip[0] = '\0';
            LoadScreenLoop(true);

            interrupt_loading = false;

#if THREADED
            loading_in_progress_ = true;
            std::thread load_thread(std::bind(&Engine::LoadLevelData, this, level_path));
            bool keep_looping = loading_in_progress_;
            last_loading_input_time = SDL_TS_GetTicks();
            while (keep_looping) {
                LoadScreenLoop(loading_in_progress_);
                DisplayLastQueuedError();
                loading_mutex_.lock();
                keep_looping = loading_in_progress_;
                loading_mutex_.unlock();
            }
            load_thread.join();
#else
            LoadLevelData(level_path);
#endif
            Graphics::Instance()->setSimpleShadows(config["simple_shadows"].toNumber<bool>());

            if (interrupt_loading) {
                ClearLoadedLevel();
                return;
            }

            if (g_no_reflection_capture == false) {
                scenegraph_->LoadReflectionCaptureCubemaps();
            }

            if (kLightProbeEnabled) {
                PROFILER_ZONE(g_profiler_ctx, "Init light probe collection");
                scenegraph_->light_probe_collection.Init();
            }
            {
                PROFILER_ZONE(g_profiler_ctx, "Init dynamic light collection");
                scenegraph_->dynamic_light_collection.Init();
            }
            {
                PROFILER_ZONE(g_profiler_ctx, "Initialize level");
                scenegraph_->level->Initialize(scenegraph_, &gui);
            }

            {
                PROFILER_ZONE(g_profiler_ctx, "Send 'added_object' messages");
                for (auto obj : scenegraph_->objects_) {
                    static const int kBufSize = 256;
                    char msg[kBufSize];
                    FormatString(msg, kBufSize, "added_object %d", obj->GetID());
                    scenegraph_->level->Message(msg);
                }
            }

            AddLoadingText("Loading sky...");
            DrawLoadScreen(true);

            {
                PROFILER_ZONE(g_profiler_ctx, "Load sky");
                scenegraph_->sky->QueueLoadResources();
                scenegraph_->sky->BakeFirstPass();
                if (!scenegraph_->terrain_object_) {
                    scenegraph_->sky->BakeSecondPass(NULL);
                }
            }

            AddLoadingText("Reinitializing prefabs...");
            scenegraph_->map_editor->ReloadAllPrefabs();

            scenegraph_->map_editor->QueueSaveHistoryState();

            AddLoadingText("Level load completed, initiating game...");
            AddLoadingText("Preloading shaders...");
            {
                PROFILER_ZONE(g_profiler_ctx, "Preload shaders");
                scenegraph_->PreloadShaders();
            }

            DrawLoadScreen(true);

            if (kLightProbeEnabled) {
                PROFILER_ZONE(g_profiler_ctx, "Update light probe texture buffer");
                scenegraph_->light_probe_collection.UpdateTextureBuffer(*scenegraph_->bullet_world_);
            }

            if ((config["editor_mode"].toNumber<bool>() == false && !scenegraph_->movement_objects_.empty()) || current_engine_state_.type != kEngineEditorLevelState) {
                scenegraph_->map_editor->SendInRabbot();
            }

            native_loading_text.Clear();

            level_loaded_ = true;
            UIShowCursor(0);
            level_updated_ = 0;

            if (level_path.GetOriginalPathStr() == new_empty_level_path) {
                scenegraph_->level_path_ = Path();
            }

            if (level_has_screenshot && scenegraph_->level->loading_screen_.image.empty()) {
                scenegraph_->level->loading_screen_.image = screenshot_path;
            }
            scenegraph_->map_editor->UpdateEnabledObjects();
            // level_screenshot.clear();
        } else {
            LOGE << "Unable to load LevelInfoAsset for " << level_path << ". Level file is likely corrupt or missing in some way." << std::endl;
        }
    }

    Online::Instance()->SetLevelLoaded();
    Shaders::Instance()->create_program_warning = true;
}

void Engine::AddLevelPathToRecentLevels(const Path& level_path) {
    std::vector<std::string> level_history;
    level_history.push_back(level_path.GetFullPath());

    for (int i = 0; i < kMaxLevelHistory; ++i) {
        const int kConfigKeyNameSize = 128;
        char configKeyName[kConfigKeyNameSize];
        FormatString(configKeyName, kConfigKeyNameSize, "level_history%d", i + 1);

        if (config.HasKey(configKeyName) && config[configKeyName].str() != level_path.GetFullPathStr() && config[configKeyName].str() != "") {
            level_history.push_back(config[configKeyName].str());
        }
    }

    for (int i = 0; i < kMaxLevelHistory; ++i) {
        const int kConfigKeyNameSize = 128;
        char configKeyName[kConfigKeyNameSize];
        FormatString(configKeyName, kConfigKeyNameSize, "level_history%d", i + 1);

        if (i < (int)level_history.size()) {
            config.GetRef(configKeyName) = level_history[i];
        } else {
            config.GetRef(configKeyName) = "";
        }
    }

    config.Save(GetConfigPath());
}

void Engine::GetAvatarIds(std::vector<ObjectID>& avatars) {
    scenegraph_->GetPlayerCharacterIDs(&num_avatars, avatar_ids, kMaxAvatars);
    avatars.clear();
    for (int i = 0; i < num_avatars; ++i) {
        avatars.push_back(avatar_ids[i]);
    }
}

void Engine::QueueLevelToLoad(const Path& level) {
    if (Online::Instance()->IsHosting()) {
        Online::Instance()->PerformLevelChangeCleanUp();
        Online::Instance()->ChangeLevel(level.GetOriginalPathStr());
    }

    queued_level_ = level;
    UIShowCursor(1);
    cursor.SetVisible(false);
}

void Engine::QueueLevelCacheGeneration(const Path& path) {
    cache_generation_paths.push_back(path);
}

void Engine::GenerateLevelCache(ModInstance* mod_instance) {
    for (auto& level : mod_instance->levels) {
        std::string path = "Data/levels/";
        path += level.path;

        QueueLevelCacheGeneration(FindFilePath(path, kAnyPath));
    }
    for (auto& campaign : mod_instance->campaigns) {
        for (size_t j = 0; j < campaign.levels.size(); ++j) {
            std::string path = "Data/levels/";
            path += campaign.levels[j].path;

            QueueLevelCacheGeneration(FindFilePath(path, kAnyPath));
        }
    }
}

void Engine::Initialize() {
    current_menu_player = -1;
    waiting_for_input_ = false;
    back_to_menu = false;
#ifdef OpenVR
    // Loading the SteamVR Runtime
    vr::EVRInitError eError = vr::VRInitError_None;
    m_pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);

    if (eError != vr::VRInitError_None) {
        m_pHMD = NULL;
        char buf[1024];
        sprintf_s(buf, sizeof(buf), "Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
        FatalError("OpenVR Error", buf);
        return;
    }

    vr::IVRRenderModels* m_pRenderModels;
    m_pRenderModels = (vr::IVRRenderModels*)vr::VR_GetGenericInterface(vr::IVRRenderModels_Version, &eError);
    if (!m_pRenderModels) {
        m_pHMD = NULL;
        vr::VR_Shutdown();

        char buf[1024];
        sprintf_s(buf, sizeof(buf), "Unable to get render model interface: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
        FatalError("OpenVR Error", buf);
    }

    if (!vr::VRCompositor()) {
        FatalError("OpenVR Error", "vr::VRCompositor() failed");
    }

    std::string m_strDriver;
    std::string m_strDisplay;
    m_strDriver = "No Driver";
    m_strDisplay = "No Display";

    m_strDriver = GetTrackedDeviceString(m_pHMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
    m_strDisplay = GetTrackedDeviceString(m_pHMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);
#endif
#ifdef OculusVR
    ovrInitParams initParams = {ovrInit_RequestVersion, OVR_MINOR_VERSION, NULL, 0, 0};
    ovrResult result = ovr_Initialize(&initParams);
    if (OVR_FAILURE(result)) {
        ovrErrorInfo errorInfo;
        ovr_GetLastErrorInfo(&errorInfo);
        LOGI << "ovr_Initialize failed: " << errorInfo.ErrorString << std::endl;
    } else {
        LOGE << "ovr_Initialize succeeded." << std::endl;

        ovrGraphicsLuid luid;
        result = ovr_Create(&session, &luid);
        if (OVR_FAILURE(result)) {
            ovrErrorInfo errorInfo;
            ovr_GetLastErrorInfo(&errorInfo);
            LOGI << "ovr_Create failed: " << errorInfo.ErrorString << std::endl;
        } else {
            LOGE << "ovr_Create succeeded." << std::endl;
            g_oculus_vr_activated = true;
            g_draw_vr = true;
            hmdDesc = ovr_GetHmdDesc(session);
        }
    }
#endif
    AssetPreload::Instance().Initialize();
#if ENABLE_STEAMWORKS
    Steamworks::Instance()->Initialize();
#endif
    // multiplayer.Initialize();

    if (Engine::instance_ != NULL) {
        LOGF << "Engine only supports one instance at any time, if this need changes, refactoring will be needed for all situations that refer to Engine::Instance()" << std::endl;
    } else {
        Engine::instance_ = this;
    }

    LOG_ASSERT(Engine::instance_ != NULL);
    paused = false;
    scriptable_menu_ = NULL;
    level_loaded_ = false;
    quitting_ = false;
    draw_frame = 0;
    scenegraph_ = NULL;
    resize_event_frame_counter = -1;
    printed_rendering_error_message = false;
    forced_split_screen_mode = kForcedModeNone;

    active_screen_start = vec2(0.0f, 0.0f);
    active_screen_end = vec2(1.0f, 1.0f);

    // Check if we can find Data folder
    char data_path[kPathSize];
    if (FindFilePath("Data", data_path, kPathSize, kDataPaths)) {
#ifdef _DEPLOY
        FatalError("Error", "Could not find Data folder; please make sure Overgrowth is fully extracted and installed.");
#else
        FatalError("Error", "Could not find Data folder; please make sure working directory is set correctly.");
#endif
    }

    LOGI << "Initializing SDL" << std::endl;
    if (SDL_Init(0) == 0) {
        LOGI << "SDL Initialized successfully" << std::endl;
    } else {
        LOGE << "SDL Initialization failed" << std::endl;
    }

    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
        LOGE << "Failed when initializing SDL video subsystem" << std::endl;
    }
    if (SDL_InitSubSystem(SDL_INIT_EVENTS) < 0) {
        LOGE << "Failed when initializing SDL events subsystem" << std::endl;
    }
    if (SDL_InitSubSystem(SDL_INIT_TIMER) < 0) {
        LOGE << "Failed when initializing SDL timer subsystem" << std::endl;
    }

    LOGI << "Initializing SDL_net" << std::endl;
    SDLNet_Init();

    as_network.Initialize();
    asset_manager.Initialize();

    KeyCommand::Initialize();
    ActiveCameras::Set(0);
    Input::Instance()->Initialize();
    Input::Instance()->cursor = &cursor;
    LoadConfigFile();
    AnimationRetargeter::Instance()->Load("Data/Animations/retarget.xml");

    ModLoading::Instance().Initialize();
    ModLoading::Instance().InitMods();

    std::string preferred_audio_device = config["preferred_audio_device"].str();
    sound.Initialize(preferred_audio_device.c_str());
    sound.EnableLayeredSoundtrackLimiter(config["use_soundtrack_limiter"].toBool());
    Graphics::Instance()->Initialize();
    Graphics::Instance()->InitScreen();

    if (config["has_detected_settings"].toBool() == false) {
        DetectAndSetSettings();
        config.GetRef("has_detected_settings") = true;
    }

    // Set initial default playername
    if (!config.HasKey("playername") || !OnlineUtility::IsValidPlayerName(config.GetRef("playername").str())) {
        config.GetRef("playername") = OnlineUtility::GetDefaultPlayerName();
    }

#ifdef OculusVR
    if (g_oculus_vr_activated) {
        for (int eye = 0; eye < 2; ++eye) {
            ovrSizei idealTextureSize = ovr_GetFovTextureSize(session, ovrEyeType(eye), hmdDesc.DefaultEyeFov[eye], 1);
            eyeRenderTexture[eye] = new OVR::TextureBuffer(session, true, true, idealTextureSize, 1, NULL, 1);
            eyeDepthBuffer[eye] = new OVR::DepthBuffer(eyeRenderTexture[eye]->GetSize(), 0);

            if (!eyeRenderTexture[eye]->TextureChain) {
                FatalError("Error", "Could not create eye texture");
            }
        }

        ovrMirrorTextureDesc desc;
        memset(&desc, 0, sizeof(desc));
        desc.Width = hmdDesc.Resolution.w / 2;
        desc.Height = hmdDesc.Resolution.h / 2;
        desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
        ovrMirrorTexture mirrorTexture = NULL;
        result = ovr_CreateMirrorTextureGL(session, &desc, &mirrorTexture);
        if (OVR_FAILURE(result)) {
            ovrErrorInfo errorInfo;
            ovr_GetLastErrorInfo(&errorInfo);
            LOGI << "ovr_CreateMirrorTextureGL failed: " << errorInfo.ErrorString << std::endl;
        } else {
            LOGE << "ovr_CreateMirrorTextureGL succeeded." << std::endl;
        }

        // Configure the mirror read buffer
        GLuint texId;
        ovr_GetMirrorTextureBufferGL(session, mirrorTexture, &texId);

        Graphics::Instance()->genFramebuffers(&mirrorFBO, "mirror_fbo");
        glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBO);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0);
        glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

        ovr_SetTrackingOriginType(session, ovrTrackingOrigin_FloorLevel);
    }
#endif

    show_state = config["menu_show_state"].toBool();
    show_performance = config["menu_show_performance"].toBool();
    show_log = config["menu_show_log"].toBool();
    show_warnings = config["menu_show_warnings"].toBool();
    show_save = config["menu_show_save"].toBool();
    show_sound = config["menu_show_sound"].toBool();
    show_mod_menu = config["menu_show_mod_menu"].toBool();
    asdebugger_enabled = config["asdebugger_enabled"].toBool();
    asprofiler_enabled = config["asprofiler_enabled"].toBool();
    show_asdebugger_contexts = config["menu_show_asdebugger_contexts"].toBool();
    show_asprofiler = config["menu_show_asprofiler"].toBool();
    show_mp_debug = config["menu_show_mp_debug"].toBool();
    show_input_debug = config["menu_show_input_debug"].toBool();
    show_mp_info = config["menu_show_mp_info"].toBool();
    show_mp_settings = config["menu_show_mp_settings"].toBool();
    break_on_script_change = config["asdebugger_break_on_script_change"].toBool();

    if (!GLAD_GL_VERSION_4_0 && g_single_pass_shadow_cascade) {
        LOGW << "OpenGL 4.0 not found, disabling single-pass shadow cascade" << std::endl;
        g_single_pass_shadow_cascade = false;
        config.GetRef("single_pass_shadow_cascade") = false;
    }

    game_timer.SetStepFrequency(120);
    ui_timer.SetStepFrequency(120);
    {
        char save[kPathSize];
        for (int i = SaveFile::kCurrentVersion; i >= 1; i--) {
            if (i == 1) {
                FormatString(save, kPathSize, "%ssavefile.sav", GetWritePath(CoreGameModID).c_str());
            } else {
                FormatString(save, kPathSize, "%ssavefile.sav%d", GetWritePath(CoreGameModID).c_str(), i);
            }

            if (FileExists(save, kAbsPath)) {
                if (save_file_.ReadFromFile(save)) {
                    LOGI << "Successfully loaded v" << i << " save file" << std::endl;
                    break;
                } else {
                    LOGE << "Failed to load v" << i << " save file" << std::endl;
                }
            }
        }
        FormatString(save, kPathSize, "%ssavefile.sav%d", GetWritePath(CoreGameModID).c_str(), SaveFile::kCurrentVersion);
        save_file_.SetWriteFile(save);
    }

    CHECK_GL_ERROR();

    cursor.SetCursor(DEFAULT_CURSOR);

    CHECK_GL_ERROR();
    PrintSpecs();
    CHECK_GL_ERROR();

    CHECK_GL_ERROR();
    FontRenderer::Instance(&font_renderer);

    CHECK_GL_ERROR();
    if (config["load_all_levels"].toNumber<bool>()) {
        ManifestXMLParser mp;

        if (!mp.Load(FindFilePath(config["ogda_manifest"].str(), kAnyPath).GetFullPathStr())) {
            LOGF << "Unable to parse ogda manifest for load-all-levels" << std::endl;
        }

        std::vector<ManifestXMLParser::BuilderResult>::iterator resultit;

        for (resultit = mp.manifest.builder_results.begin();
             resultit != mp.manifest.builder_results.end();
             resultit++) {
            if (resultit->type == "level") {
                LevelInfoAssetRef levelinfo = Engine::Instance()->GetAssetManager()->LoadSync<LevelInfoAsset>(std::string("Data/") + resultit->dest);
                Path level_path = FindFilePath(std::string("Data/") + resultit->dest, kAnyPath);
                QueueState(EngineState(levelinfo->GetName(), kEngineEditorLevelState, level_path));
            }
        }
    } else if (config["main_menu"].toNumber<bool>()) {
        CHECK_GL_ERROR();
        Path path = FindFilePath(script_dir_path + std::string("mainmenu.as"), kAnyPath);
        QueueState(EngineState("main_menu", kEngineScriptableUIState, path));
        CHECK_GL_ERROR();
    } else {
        CHECK_GL_ERROR();
        Path path = FindFilePath(script_dir_path + std::string("mainmenu.as"), kAnyPath);
        current_engine_state_ = EngineState("main_menu", kEngineScriptableUIState, path);  // So that "back" takes us back to the main menu

        if (FileExists(DebugLoadLevel(), kAnyPath)) {
            Path path = FindFilePath(DebugLoadLevel(), kAnyPath);
            LevelInfoAssetRef levelinfo = Engine::Instance()->GetAssetManager()->LoadSync<LevelInfoAsset>(path.GetFullPath());
            QueueState(EngineState(levelinfo->GetName(), kEngineEditorLevelState, path));
        } else if (FileExists(std::string("Data/") + DebugLoadLevel(), kAnyPath)) {
            Path path = FindFilePath(std::string("Data/") + DebugLoadLevel(), kAnyPath);
            LevelInfoAssetRef levelinfo = Engine::Instance()->GetAssetManager()->LoadSync<LevelInfoAsset>(path.GetFullPath());
            QueueState(EngineState(levelinfo->GetName(), kEngineEditorLevelState, path));
        } else {
            Path path = FindFilePath(std::string("Data/Levels/") + DebugLoadLevel(), kAnyPath);
            LevelInfoAssetRef levelinfo = Engine::Instance()->GetAssetManager()->LoadSync<LevelInfoAsset>(path.GetFullPath());
            QueueState(EngineState(levelinfo->GetName(), kEngineEditorLevelState, path));
        }

        CHECK_GL_ERROR();
    }

    if (config["global_time_scale_mult"].toNumber<float>() < 0.1f) {
        config.GetRef("global_time_scale_mult") = 0.1f;
    }
    game_timer.target_time_scale = config["global_time_scale_mult"].toNumber<float>();
    current_global_scale_mult = config["global_time_scale_mult"].toNumber<float>();

    CHECK_GL_ERROR();
    g_text_atlas[kTextAtlasMono].Create(HardcodedPaths::paths[HardcodedPaths::kMonoFontPath], 18, &font_renderer, 0);
    CHECK_GL_ERROR();
    g_text_atlas[kTextAtlasDynamic].Create(HardcodedPaths::paths[HardcodedPaths::kDynamicFontPath], 18, &font_renderer, 0);
    CHECK_GL_ERROR();
    g_text_atlas_renderer.Init();

    static const GLfloat quad_data[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f};
    static const GLuint quad_index[] = {
        0, 1, 2, 0, 3, 2};

    quad_vert_vbo.Fill(kVBOFloat | kVBOStatic, sizeof(quad_data), (void*)quad_data);
    quad_index_vbo.Fill(kVBOElement | kVBOStatic, sizeof(quad_index), (void*)quad_index);

    InitImGui();

    ModLoading::Instance().RegisterCallback(this);
#ifdef WIN32
    data_change_notification = FindFirstChangeNotificationW(UTF16fromUTF8(data_path).c_str(), true, FILE_NOTIFY_CHANGE_LAST_WRITE);
    char write_path[kPathSize];
    FindFilePath("Data/Mods/", write_path, kPathSize, kWriteDir);
    write_change_notification = FindFirstChangeNotificationW(UTF16fromUTF8(write_path).c_str(), true, FILE_NOTIFY_CHANGE_LAST_WRITE);
#endif

    loading_screen_logo = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/logo.tga", PX_SRGB | PX_NOREDUCE, 0x0);

    loading_screen_og_logo_casual = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/ui/ogicon_casual.png", PX_SRGB | PX_NOREDUCE, 0x0);
    loading_screen_og_logo_hardcore = loading_screen_og_logo_casual;  //= Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/ui/ogicon_hardcore.png", PX_SRGB | PX_NOREDUCE, 0x0);
    loading_screen_og_logo_expert = loading_screen_og_logo_casual;    //= Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/ui/ogicon_expert.png", PX_SRGB | PX_NOREDUCE, 0x0);

    loading_screen_og_logo = loading_screen_og_logo_casual;
}

void Engine::GetShaderNames(std::map<std::string, int>& preload_shaders) {
    std::string post_shader_name = Graphics::Instance()->post_shader_name;
    if (scenegraph_->level->script_params().HasParam("Custom Shader") && config.GetRef("custom_level_shaders").toNumber<bool>()) {
        const std::string& custom_shader = scenegraph_->level->script_params().GetStringVal("Custom Shader");
        if (!custom_shader.empty()) {
            post_shader_name += " " + custom_shader;
        }
    }

    preload_shaders[post_shader_name] = 0;
    preload_shaders[post_shader_name + " #TONE_MAP"] = 0;
    preload_shaders[post_shader_name + " #CALC_MOTION_BLUR"] = 0;
    preload_shaders[post_shader_name + " #DOWNSAMPLE"] = 0;
    preload_shaders[post_shader_name + " #BLUR_HORZ"] = 0;
    preload_shaders[post_shader_name + " #BLUR_VERT"] = 0;
    preload_shaders[post_shader_name + " #APPLY_MOTION_BLUR"] = 0;
    preload_shaders[post_shader_name + " #DOF"] = 0;
    preload_shaders[post_shader_name + " #COPY"] = 0;
    preload_shaders[post_shader_name + " #DOWNSAMPLE #OVERBRIGHT"] = 0;
    preload_shaders[post_shader_name + " #BLUR_DIR"] = 0;
    preload_shaders[post_shader_name + " #ADD"] = 0;
    preload_shaders[post_shader_name + " #DOWNSAMPLE_DEPTH"] = 0;
    if (g_gamma_correct_final_output) {
        preload_shaders[post_shader_name + " #BRIGHTNESS #GAMMA_CORRECT_OUTPUT"] = 0;
    } else {
        preload_shaders[post_shader_name + " #BRIGHTNESS"] = 0;
    }
}

void Engine::ScriptableUICallback(const std::string& level) {
    LOGW << "Got a callback to switch to: " << level << std::endl;

    if (config["global_time_scale_mult"].toNumber<float>() < 0.1f) {
        config.GetRef("global_time_scale_mult") = 0.1f;
    }
    game_timer.target_time_scale = config["global_time_scale_mult"].toNumber<float>();
    current_global_scale_mult = config["global_time_scale_mult"].toNumber<float>();

    if (hasEnding(level, ".as")) {
        std::string short_path = script_dir_path + level;
        const std::string& long_path = level;

        // First check if a full path was entered.
        if (FileExists(long_path, kAnyPath)) {
            LOGI << "L " << long_path << std::endl;
            Path path = FindFilePath(long_path, kAnyPath);
            QueueState(EngineState("", kEngineScriptableUIState, path));
        }
        // Then check if it a path relative to script_dir_path as root.
        else if (FileExists(short_path, kAnyPath)) {
            LOGI << "S " << long_path << std::endl;
            Path path = FindFilePath(short_path, kAnyPath);
            QueueState(EngineState("", kEngineScriptableUIState, path));
        } else {
            std::string message = "Unable to find script: " + short_path + " or " + long_path + ". Will not change state";

            DisplayError("Unable to find script", message.c_str(), _ok, true);
        }
    } else if (hasEnding(level, ".xml")) {
        std::string short_path = "Data/Levels/" + level;
        const std::string& long_path = level;

        EngineStateType target_state_type = kEngineLevelState;
        if (current_engine_state_.type == kEngineEditorLevelState) {
            target_state_type = kEngineEditorLevelState;
        }

        Path level_path;
        // First check if a full path was entered.
        if (FileExists(long_path, kAnyPath)) {
            level_path = FindFilePath(long_path, kAnyPath);
        }
        // Then check if it a path relative to Data/Levels as root.
        else if (FileExists(short_path, kAnyPath)) {
            level_path = FindFilePath(short_path, kAnyPath);
        }

        if (level_path.isValid()) {
            LevelInfoAssetRef levelinfo = Engine::Instance()->GetAssetManager()->LoadSync<LevelInfoAsset>(level_path.GetFullPath());
            std::string level_id;
            if (levelinfo.valid()) {
                level_id = levelinfo->GetName();
            }

            ScriptableCampaign* sc = GetCurrentCampaign();
            if (sc) {
                std::string campaign_id = sc->GetCampaignID();
                ModInstance::Campaign campaign = ModLoading::Instance().GetCampaign(campaign_id);

                for (auto& level : campaign.levels) {
                    if (level.path == long_path || level.path == short_path) {
                        level_id = level.id;
                    }
                }
            }
            QueueState(EngineState(level_id, target_state_type, level_path));
        } else {
            std::string message = "Unable to find level: " + level + ". Will not change state";

            DisplayError("Unable to find level", message.c_str(), _ok, true);
        }
    } else if (hasBeginning(level, "set_campaign")) {
        LOGI << "Setting campaign " << std::endl;
        LOGI << level << std::endl;
        if (level.size() > strlen("set_campaign ")) {
            std::string campaign_id = level.substr(strlen("set_campaign "));
            if (strlen(ModLoading::Instance().GetCampaign(campaign_id).id) > 0) {
                ModInstance::Campaign mi_campaign = ModLoading::Instance().GetCampaign(campaign_id);
                ScriptableCampaign* campaign = new ScriptableCampaign();

                Path campaign_path = FindFilePath(std::string("Data/Scripts/campaign/") + std::string(mi_campaign.main_script));
                if (campaign_path.isValid() == false) {
                    if (strlen(mi_campaign.main_script) > 0) {
                        LOGE << "Given campaign script path \"" << mi_campaign.main_script << "\"is invalid, using fallback default_campaign.as" << std::endl;
                    }
                    campaign_path = FindFilePath("Data/Scripts/campaign/default_campaign.as");
                }

                if (campaign_path.isValid()) {
                    LOGI << "Initializing campaign state with script " << campaign_path << std::endl;
                    campaign->Initialize(campaign_path, campaign_id);
                    LOGI << "Campaign accepted, loading as state" << std::endl;
                    QueueState(EngineState("", campaign, campaign_path));
                } else {
                    LOGE << "Path to campaign script \"" << campaign_path << "\""
                         << " for campaign_id: " << campaign_id << std::endl;
                }
            } else {
                LOGE << "Campaign id: " << campaign_id << " does not belong to any existing (or active) campaign " << std::endl;
            }
        }
    } else if (hasBeginning(level, "load_campaign_level")) {
        std::string level_id = level.substr(strlen("load_campaign_level "));
        QueueState(EngineState(level_id, kEngineLevelState));
    } else if (level == "back") {
        // These rules should probably be removed from here and moved into scripts.
        bool perform_exit = false;
        switch (current_engine_state_.type) {
            case kEngineNoState:
                LOGW << "Current state is no state, invalid state" << std::endl;
                perform_exit = false;
                break;

            case kEngineLevelState:
            case kEngineEditorLevelState:
                // Ingame, the esc button has very different use,
                // instead spawning the main menu.
                perform_exit = false;
                break;

            case kEngineScriptableUIState:
                if (scriptable_menu_) {
                    perform_exit = scriptable_menu_->CanGoBack();
                } else {
                    LOGE << "No current scriptable_ui_ when there should be." << std::endl;
                    perform_exit = true;
                }
                break;
            case kEngineCampaignState:
                perform_exit = true;
                break;
            default:
                LOGE << "Unknown current state in ask pop queue state stack" << std::endl;
                break;
        }

        if (perform_exit) {
            EngineStateAction esa;
            esa.type = kEngineStateActionPopState;
            esa.allow_game_exit = false;
            queued_engine_state_.push_back(esa);
        } else {
            LOGI << "Ignoring request to back" << std::endl;
        }
    } else if (level == "back_to_menu") {
        EngineStateAction esa;
        esa.type = kEngineStateActionPopUntilType;
        esa.allow_game_exit = false;
        esa.state.type = kEngineScriptableUIState;
        queued_engine_state_.push_back(esa);

        Online::Instance()->StopMultiplayer();
    } else if (level == "back_to_main_menu") {
        EngineStateAction esa;
        esa.type = kEngineStateActionPopUntilID;
        esa.allow_game_exit = false;
        esa.state.id = "main_menu";
        queued_engine_state_.push_back(esa);

        Online::Instance()->StopMultiplayer();
    } else if (level == "exit") {
        EngineStateAction esa;
        esa.type = kEngineStateActionPopState;
        esa.allow_game_exit = true;
        queued_engine_state_.push_back(esa);
    } else if (level == "mods") {
        Path path = FindFilePath(script_dir_path + std::string("modmenu/main.as"), kAnyPath);
        QueueState(EngineState("", kEngineScriptableUIState, path));
    } else {
        LOGE << "Unknown message: " << level << std::endl;
    }
}

bool Engine::IsStateQueued() {
    return (queued_engine_state_.size() > 0);
}

void Engine::QueueState(const EngineState& state) {
    EngineStateAction esa;
    esa.type = kEngineStateActionPushState;
    esa.state = state;
    queued_engine_state_.push_back(esa);
}

void Engine::QueueState(const EngineStateAction& action) {
    queued_engine_state_.push_back(action);
}

void Engine::QueueErrorMessage(const std::string& title, const std::string& message) {
    // Send error directly to scriptable menus if possible (Main menu)
    if (scriptable_menu_ != nullptr) {
        scriptable_menu_->QueueBasicPopup(title, message);
    } else {  // If we don't have a menu, queue it up instead
        popup_pueue.push_back(std::tuple<std::string, std::string>(title, message));
    }
}

/*
void Engine::OpenChallengeMenu() {
    LOG_ASSERT(!scriptable_menu_);
    scriptable_menu_ = new ScriptableUI((void*)this, &Engine::StaticHandleChallengeLevelSelect);
    ASData as_data;
    as_data.scenegraph = scenegraph_;
    as_data.gui = &gui;
    scriptable_menu_->Initialize(script_dir_path + "campaignchallengemenu.as", "void SetChallengeListMode()", as_data);
}
*/

void Engine::StaticScriptableUICallback(void* instance, const std::string& level) {
    ((Engine*)instance)->ScriptableUICallback(level);
}

/*
void Engine::OpenCampaignMenu()
{
    LOG_ASSERT(!scriptable_menu_);
    scriptable_menu_ = new ScriptableUI((void*)this, &Engine::StaticHandleChallengeLevelSelect);
    ASData as_data;
    as_data.scenegraph = scenegraph_;
    as_data.gui = &gui;
    scriptable_menu_->Initialize(script_dir_path + "campaignmenu.as", "void SetCampaignListMode()", as_data);
}

class NewMenuScenegraph: public SceneGraph{
    public:
        NewMenuScenegraph():SceneGraph(){}
        Sound * sound;
};
*/

void Engine::ModActivationChange(const ModInstance* mod) {
    ReloadImGui();
}

Path Engine::GetLatestMenuPath() {
    return latest_menu_path_;
}

Path Engine::GetLatestLevelPath() {
    return latest_level_path_;
}

void Engine::CommitPause() {
    bool was_paused = paused;
    if (!Online::Instance()->IsActive()) {
        paused = user_paused || menu_paused;
    } else {
        paused = false;
    }

    game_timer.target_time_scale = (paused || slow_motion) ? 0.1f : current_global_scale_mult;
    if (was_paused != paused) {
        game_timer.time_scale = paused ? current_global_scale_mult : 0.1f;
    }
    if (!paused) {
        Engine::Instance()->current_menu_player = -1;
    }
}

ScriptableCampaign* Engine::GetCurrentCampaign() {
    if (current_engine_state_.type == kEngineCampaignState) {
        if (current_engine_state_.campaign.Valid()) {
            return current_engine_state_.campaign.GetPtr();
        }
    } else {
        for (auto& i : state_history) {
            if (i.type == kEngineCampaignState) {
                if (i.campaign.Valid()) {
                    return i.campaign.GetPtr();
                }
            }
        }
    }

    return NULL;
}

std::string Engine::GetCurrentLevelID() {
    if (current_engine_state_.type == kEngineLevelState || current_engine_state_.type == kEngineEditorLevelState) {
        return current_engine_state_.id;
    } else {
        for (auto& i : state_history) {
            if (i.type == kEngineLevelState || i.type == kEngineEditorLevelState) {
                return i.id;
            }
        }
    }
    return "";
}

void Engine::InjectWindowResizeEvent(ivec2 size) {
    resize_event_frame_counter = 10;
    resize_value = size;
}

void DumpScenegraphState() {
    SceneGraph* s = Engine::Instance()->GetSceneGraph();
    if (s) {
        s->DumpState();
    }
}

void Engine::SetGameSpeed(float val, bool hard) {
    if (val >= 0.1f) {
        if (hard) {
            game_timer.time_scale /= current_global_scale_mult;
        }
        game_timer.target_time_scale /= current_global_scale_mult;
        config.GetRef("global_time_scale_mult") = val;
        current_global_scale_mult = val;
        game_timer.target_time_scale *= val;
        if (hard) {
            game_timer.time_scale *= val;
        }
    } else {
        LOGE << "Disallowing setting gamespeed below 0.1" << std::endl;
    }
}

bool Engine::RequestedInterruptLoading() {
    loading_mutex_.lock();
    bool val = interrupt_loading;
    loading_mutex_.unlock();
    return val;
}
