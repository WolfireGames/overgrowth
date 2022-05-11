//-----------------------------------------------------------------------------
//           Name: main.cpp
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
#ifndef __PS4__
#include <Internal/config.h>
#include <Internal/crashreport.h>
#include <Internal/error.h>
#include <Internal/filesystem.h>
#include <Internal/profiler.h>
#include <Internal/dialogues.h>

#include <Logging/consolehandler.h>
#include <Logging/filehandler.h>
#include <Logging/logdata.h>
#include <Logging/ramhandler.h>

#include <Main/engine.h>
#include <Main/altmain.h>

#include <Threading/rand.h>
#include <Threading/thread_sanity.h>

#include <Graphics/graphics.h>
#include <Memory/allocation.h>
#include <Version/version.h>
#include <Compat/platformsetup.h>
#include <Timing/timingevent.h>

#include <SDL.h>
#include <RecastAlloc.h>
#include <DetourAlloc.h>
#include <tclap/CmdLine.h>
#if ENABLE_FPU_SIGNALS == 1
#include <fenv.h>
#endif
#ifdef UNIT_TESTS
#include <UnitTests/testmain.h>
#endif
#include <sstream>

extern Config config;
extern bool mem_track_enable;
extern bool g_draw_vr;
ProfilerContext* g_profiler_ctx;

static bool debug_output = false;
static bool spam_output = false;
static bool clear_log = false;
static bool quit_after_load = false;
static bool no_dialogues = false;
static bool disable_rendering = false;
static bool load_all_levels = false;
static bool clear_cache = false;
static bool clear_cache_dry_run = false;
static bool level_load_stress = false;

static std::string overloadedWriteDir;
static std::string overloadedWorkingDir;

Allocation alloc;

RamHandler ram_handler;

static void* rcAllocReplacement(size_t size, rcAllocHint) {
    return og_malloc(size, OG_MALLOC_RC);
}

static void rcFreeReplacement(void* ptr) {
    og_free(ptr);
}

static void* dtAllocReplacement(size_t size, dtAllocHint) {
    return og_malloc(size, OG_MALLOC_DT);
}

static void dtFreeReplacement(void* ptr) {
    og_free(ptr);
}

#if defined PLATFORM_WINDOWS
#include <windows.h>
typedef enum PROCESS_DPI_AWARENESS {
    PROCESS_DPI_UNAWARE = 0,
    PROCESS_SYSTEM_DPI_AWARE = 1,
    PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS;
// typedef BOOL (WINAPI * SETPROCESSDPIAWARE_T)(void);
typedef HRESULT(WINAPI* SETPROCESSDPIAWARENESS_T)(PROCESS_DPI_AWARENESS);
#ifdef OG_DEBUG
#include <DbgHelp.h>
#endif
#endif

int GameMain(int argc, char* argv[]) {
    RegisterMainThreadID();

#ifdef PLATFORM_WINDOWS
    HMODULE shcore = LoadLibraryA("Shcore.dll");
    SETPROCESSDPIAWARENESS_T SetProcessDpiAwareness = NULL;
    if (shcore) {
        SetProcessDpiAwareness = (SETPROCESSDPIAWARENESS_T)GetProcAddress(shcore, "SetProcessDpiAwareness");
    }
    // HMODULE user32 = LoadLibraryA("User32.dll");
    // SETPROCESSDPIAWARE_T SetProcessDPIAware = NULL;
    // if (user32) {
    //     SetProcessDPIAware = (SETPROCESSDPIAWARE_T) GetProcAddress(user32, "SetProcessDPIAware");
    // }

    if (SetProcessDpiAwareness) {
        if (SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE) != S_OK) {
            LOGW << "Couldn't set process dpi awareness (per monitor)" << std::endl;
        }
    } else if (SetProcessDPIAware != nullptr) {
        if (!SetProcessDPIAware()) {
            LOGW << "Couldn't set process dpi awareness (system)" << std::endl;
        }
    }

    // if (user32) {
    //     FreeLibrary(user32);
    // }
    if (shcore) {
        FreeLibrary(shcore);
    }
#ifdef OG_DEBUG
    SymSetOptions(SYMOPT_LOAD_LINES);
    HANDLE handle = GetCurrentProcess();
    SymInitialize(handle, NULL, TRUE);
#endif  // OG_DEBUG
#endif

    rand_ts_seed((unsigned int)time(NULL));

    alloc.Init();

    dtAllocSetCustom(dtAllocReplacement, dtFreeReplacement);
    rcAllocSetCustom(rcAllocReplacement, rcFreeReplacement);

    // Initialize profiler
    ProfilerContext profiler_context;
    profiler_context.Init(&alloc.stack);
    g_profiler_ctx = &profiler_context;

    PROFILER_ENTER(&profiler_context, "SetUpEnvironment");
    SetUpEnvironment(argv[0], overloadedWriteDir.c_str(), overloadedWorkingDir.c_str());
    PROFILER_LEAVE(&profiler_context);

    // Have to wait until we SetUpEnvironment before we can get write dir.
    FileHandler fileLogHandler(GetLogfilePath(), clear_log ? 0 : 10 * 1024 * 1024, 40 * 1024 * 1024);

    STIMING_INIT(GetWritePath(CoreGameModID).c_str());

    STIMING_ADDVISUALIZATION(STUpdate, vec3(0.7, 0.3, 0.3),     // Redish
                             STDraw, vec3(0.3, 0.7, 0.3),       // Greenish
                             STDrawSwap, vec3(0.3, 0.3, 0.7));  // Blueish

    STIMING_SETVISUALIZATIONSCALE(16, 32);

    STIMING_INITVISUALIZATION();

    LogTypeMask level =
        LogSystem::info | LogSystem::warning | LogSystem::error | LogSystem::fatal;
    if (debug_output) {
        level |= LogSystem::debug;
    }
    if (spam_output) {
        level |= LogSystem::spam;
    }
    LogSystem::RegisterLogHandler(level, &fileLogHandler);

    LOGI << "Starting program. Version " << GetBuildVersion() << "_" << GetBuildIDString() << " " << GetBuildTimestamp() << " " << GetArch() << " " << GetPlatform() << std::endl;

#ifdef NDEBUG
    LOGI << "Deploy (Release)" << std::endl;
#else
    LOGI << "Debug" << std::endl;
#endif

    PROFILER_ENTER(&profiler_context, "Engine initialize");
    // Engine* engine = (Engine*)alloc.stack.Alloc(sizeof(Engine));
    // new(engine) Engine;
    Engine* engine = new Engine();
    engine->Initialize();
    Dialog::Initialize();
    PROFILER_LEAVE(&profiler_context);

    if (clear_cache) {
        ClearCache(false);
    }
    if (clear_cache_dry_run) {
        ClearCache(true);
    }

    // Main loop
    while (!engine->quitting_) {
        STIMING_STARTFRAME();

        STIMING_START_COARSE(STUpdate);
        engine->Update();
        STIMING_END_COARSE(STUpdate);

        bool time_to_draw = true;
        static uint32_t last_time = 0;

        if (!engine->quitting_ && disable_rendering == false) {
            Graphics* graphics = Graphics::Instance();
            if (!graphics->config_.vSync() && graphics->config_.limit_fps_in_game()) {
                PROFILER_ZONE_IDLE(g_profiler_ctx, "SDL_Sleep");
                if (!g_draw_vr) {
                    time_to_draw = false;

                    int max_frame_rate = graphics->config_.max_frame_rate();
                    if (max_frame_rate < 15) {
                        max_frame_rate = 15;
                    }
                    if (max_frame_rate > 500) {
                        max_frame_rate = 500;
                    }

                    int ticks_to_wait = 1000 / max_frame_rate;
                    int diff = ticks_to_wait - (SDL_TS_GetTicks() - last_time) - 1;
                    if (diff > ticks_to_wait - 1) {
                        diff = ticks_to_wait - 1;
                    }

                    if (diff > 1) {
                        SDL_Delay(1);
                    } else if (diff < 1) {
                        time_to_draw = true;
                    }
                }
            }

            if (time_to_draw) {
                STIMING_START_COARSE(STDraw);
                engine->Draw();
                last_time = SDL_TS_GetTicks();
                STIMING_END_COARSE(STDraw);
            }
        }

        if (time_to_draw) {
            STIMING_START_COARSE(STDrawSwap);
            Graphics::Instance()->SwapToScreen();
            STIMING_END_COARSE(STDrawSwap);

            Graphics::Instance()->ClearGLState();
        }

        STIMING_ENDFRAME();

        SDL_Delay(0);  // Allow other threads to run
        PROFILER_TICK(g_profiler_ctx);
    }

    STIMING_FINALIZE();

    LOGI << "Final check if savefile needs to be written..." << std::endl;
    Engine::Instance()->save_file_.ExecuteQueuedWrite();

    LOGI << "Cleanly disposing of loaded assets..." << std::endl;
    engine->Dispose();
    // engine->~Engine();
    delete engine;
    // alloc.stack.Free(engine);

    profiler_context.Dispose(&alloc.stack);

    LOGI << "Program terminated successfully." << std::endl;
    DisposeEnvironment();

    LogSystem::DeregisterLogHandler(&fileLogHandler);

    alloc.Dispose();

#if defined PLATFORM_WINDOWS && defined OG_DEBUG
    SymCleanup(handle);
#endif

    LOGE << "Shutting down" << std::endl;
    LogSystem::Flush();
    return 0;
}

int main(int argc, char* argv[]) {
#if ENABLE_FPU_SIGNALS == 1
    feenableexcept(FE_INVALID | FE_OVERFLOW);
#endif

    //_CrtSetDbgFlag(_CRTDBG_CHECK_ALWAYS_DF);
    // Check for the command-line arg that XCode automatically adds when starting through the debugger,
    // and remove it so as not to confuse the parser
    for (int i = 0; i < argc - 1; i++) {
        if (strcmp(argv[i], "-NSDocumentRevisionsDebugMode") == 0) {
            strcpy(argv[i], "");
            strcpy(argv[i + 1], "");
        }
    };

    try {
        std::string full_version = std::string(GetBuildVersion()) + "_" + GetBuildIDString();
        // Set up command-line-parser
        TCLAP::CmdLine cmd("Overgrowth", ' ', full_version.c_str());

        TCLAP::ValueArg<std::string> configurationArg("c", "config", "Configuration string", false, "", "string");
        cmd.add(configurationArg);

        TCLAP::ValueArg<std::string> levelArg("l", "level", "Load level on startup", false, "", "string");
        cmd.add(levelArg);

        TCLAP::ValueArg<std::string> writeDirArg("", "write-dir", "Force set write directory to something else than system default.", false, "", "string");
        cmd.add(writeDirArg);

        TCLAP::ValueArg<std::string> workingDirArg("", "working-dir", "Force set the working dir for the application.", false, "", "string");
        cmd.add(workingDirArg);

        TCLAP::ValueArg<std::string> ogdaManifest("", "ogda-manifest", "Ogda generated manifest of game assets.", false, "", "string");
        cmd.add(ogdaManifest);

        TCLAP::SwitchArg ddsconvertSwitch("", "ddsconvert", "Start game with DDSConvert.", cmd, false);
        TCLAP::SwitchArg debugOutput("d", "debug-output", "Start game with debug output", cmd, false);
        TCLAP::SwitchArg spamOutput("s", "spam-output", "Start game with spammy debug output", cmd, false);
        TCLAP::SwitchArg quitAfterLoad("", "quit-after-load", "Turn of the game after level load is performed", cmd, false);
        TCLAP::SwitchArg noDialogues("", "no-dialogues", "Skip creating dialogues and instead do automatic responses", cmd, false);
        TCLAP::SwitchArg clearLog("", "clear-log", "Empty the log instead of appending to it", cmd, false);
        TCLAP::SwitchArg disableRendering("", "disable-rendering", "Disable the draw loop", cmd, false);
        TCLAP::SwitchArg loadAllLevels("", "load-all-levels", "Load all levels specified in the ogda build manifest", cmd, false);
        TCLAP::SwitchArg clearCache("", "clear-cache", "Clear the write folder of known cache files", cmd, false);
        TCLAP::SwitchArg clearCacheDryRun("", "clear-cache-dry-run", "Clear the write folder of known cache files (dry run)", cmd, false);
        TCLAP::SwitchArg levelLoadStress("", "level-load-stress", "Load levels in a loop", cmd, false);
#ifdef UNIT_TESTS
        TCLAP::SwitchArg runUnitTests("", "run-unit-tests", "Run all unit tests", cmd, false);
#endif

        // Actually parse the command line
        cmd.parse(argc, argv);

        // Extract information from command-line-parser
        bool runWithDDSConvert = ddsconvertSwitch.getValue();
        std::string levelname = levelArg.getValue();
        std::string configuration = configurationArg.getValue();
        std::string manifest = ogdaManifest.getValue();

        overloadedWriteDir = writeDirArg.getValue();
        overloadedWorkingDir = workingDirArg.getValue();
        debug_output = debugOutput.getValue();
        spam_output = spamOutput.getValue();
        clear_log = clearLog.getValue();
        quit_after_load = quitAfterLoad.getValue();
        no_dialogues = noDialogues.getValue();
        disable_rendering = disableRendering.getValue();
        load_all_levels = loadAllLevels.getValue();
        clear_cache = clearCache.getValue();
        clear_cache_dry_run = clearCacheDryRun.getValue();
        level_load_stress = levelLoadStress.getValue();

        std::stringstream configurationStream(configuration);
        config.Load(configurationStream, false, true);

        // If command line specified a level, skip main menu and jump to loading that level
        if (!levelname.empty()) {
            std::stringstream ss;
            ss << "debug_load_level: " << levelname << std::endl;
            ss << "main_menu: false" << std::endl;
            config.Load(ss, false, true);
        }

        if (!manifest.empty()) {
            std::stringstream ss;
            ss << "ogda_manifest: " << manifest << std::endl;
            config.Load(ss, false, true);
        }

        if (quit_after_load) {
            std::stringstream ss;
            ss << "quit_after_load: true" << std::endl;
            config.Load(ss, false, true);
        }

        if (no_dialogues) {
            std::stringstream ss;
            ss << "no_dialogues: true" << std::endl;
            config.Load(ss, false, true);
        }

        if (load_all_levels) {
            std::stringstream ss;
            ss << "load_all_levels: true" << std::endl;
            config.Load(ss, false, true);
        }

        if (level_load_stress) {
            std::stringstream ss;
            ss << "level_load_stress: true" << std::endl;
            config.Load(ss, false, true);
        }

        /******************************************************/
        // Register logging handlers
        ConsoleHandler consoleHandler;
        LogTypeMask level =
            LogSystem::info | LogSystem::warning | LogSystem::error | LogSystem::fatal;

        if (debug_output) {
            level |= LogSystem::debug;
        }

        if (spam_output) {
            level |= LogSystem::spam;
        }

        // Choose which main function to run
        int ret;
        if (runWithDDSConvert) {
            LogSystem::RegisterLogHandler(level, &consoleHandler);
            ret = DDSConvertMain(argc, argv, overloadedWriteDir.c_str(), overloadedWorkingDir.c_str());
            LogSystem::DeregisterLogHandler(&consoleHandler);
        } else {
            LogSystem::RegisterLogHandler(level, &consoleHandler);
            LogSystem::RegisterLogHandler(level, &ram_handler);
            ret = RunWithCrashReport(argc, argv, &GameMain);
            LogSystem::DeregisterLogHandler(&ram_handler);
            LogSystem::DeregisterLogHandler(&consoleHandler);
        }

        return ret;
    } catch (TCLAP::ArgException& e) {
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    }
    return 0;
}

#else
#include <cstdio>
#include <sys/stat.h>

struct PS4Engine {
    void Initialize();
};

void PS4Engine::Initialize() {
    FILE* file = fopen("/app0/Data/", "rb");
}

int main(int argc, char* argv[]) {
    ConsoleHandler consoleHandler;
    LogTypeMask level =
        LogSystem::info | LogSystem::warning | LogSystem::error | LogSystem::fatal;

    if (debug_output) {
        level |= LogSystem::debug;
    }
    if (spam_output) {
        level |= LogSystem::spam;
    }

    PS4Engine engine;
    engine.Initialize();

    LogSystem::DeregisterLogHandler(&consoleHandler);
    return 0;
}
#endif
