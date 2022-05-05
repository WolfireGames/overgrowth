#-----------------------------------------------------------------------------
#   Copyright 2022 Wolfire Games LLC
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#-----------------------------------------------------------------------------
CMAKE_MINIMUM_REQUIRED(VERSION 3.5) #Version of cmake in steam-runtime

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

PROJECT(Overgrowth)        

OPTION(FATAL_LOG_ASSERTS "Have LOG_ASSERTS* trigger a normal assert() call" OFF)

IF(FATAL_LOG_ASSERTS)
    ADD_DEFINITIONS(-DFATAL_LOG_ASSERTS=1)
    MESSAGE( STATUS "Compiling with fatal log asserts")
ELSE()
    MESSAGE( STATUS "Not compiling with fatal log asserts")
ENDIF()

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

INCLUDE(GetGitRevisionDescription)
INCLUDE(ExternalProject)

git_describe(GIT_VERSION --always --tag)
git_rev_list(GIT_REV_LIST --count HEAD) # Count what revision we're on
SET(BUILD_ID "-1" CACHE STRING "Build ID value from automated build system")
#Option to build with breakpad, removes some compiler warnings, so it's best to go without for normal development.
OPTION(BREAKPAD "Build with breakpad linked into the binary and with exception handling?" OFF)
OPTION(DO_DIST_ARCHIVE "Generate a package containing all build data relevant for final release, excluding game data." OFF)
OPTION(BUILD_SERVER "Building on a buildserver, this means that additional steps are included which generally only occur on a build server" OFF)
OPTION(SLIM_TIMING "Build with slim timing system activated" OFF)
OPTION(ENABLE_RAD_TELEMETRY "Enable RAD Game Tools Telemetry Support" OFF)
OPTION(ENABLE_GUI "Enable GUI" ON)
OPTION(GPU_MARKERS "Add GPU markers" OFF)
OPTION(GL_TIMING_QUERY "Build with ARB_timing_query support used for timing OpenGL GPU execution times" OFF)
OPTION(GL_INTEL_TIMING "Build with ARB_performance_query support used for timing OpenGL performance specifically on intel GPU's" OFF)
OPTION(BUILD_OGDA "Build OGDA" ON)
OPTION(BUILD_OVERGROWTH "Build the main game" ON)
OPTION(MONITOR_MEMORY "Monitor memory usage and make avaiable ingame" Off)
OPTION(OPEN_CONSOLE "Open a console on startup on windows" On)
OPTION(FORCE_OPENGL_ERROR_CHECKING "Force OpenGL error checking in Release" Off)
OPTION(DEPLOY "Set the codebase to compile for Deploy, this means compiling for final release to end-user, in practice, by setting the _DEPLOY define it activates a working directory change on start to force it to be the path of the binary. It also impacts some error messages. This is automatically turned on if BUILD_SERVER is On" Off)
OPTION(ENABLE_STEAMWORKS "Build with Steamworks Support" Off)
SET(AUX_DATA "" CACHE PATH "Auxiliary Data sourced path, used when using a Data folder from an additional path, like an already installed version of the game")

#Assume we want steamworks support if we're building on the build server, until we start supporting more targets
IF(BUILD_SERVER)
    SET(ENABLE_STEAMWORKS ON)
ENDIF()

IF(ENABLE_STEAMWORKS)
    ADD_DEFINITIONS(-DENABLE_STEAMWORKS=1)
    ADD_DEFINITIONS(-DUSE_STEAM_NETWORK_FRAMEWORK)
ELSE()
    ADD_DEFINITIONS(-DUSE_BASIC_NETWORK_FRAMEWORK)
ENDIF()

IF(GPU_MARKERS)
    IF(APPLE)
        message(FATAL_ERROR "Mac doesn't support GL_KHR_debug")
    ENDIF()

    ADD_DEFINITIONS(-DGPU_MARKERS=1)

    # GPU markers and telemetry are currently exclusive, TODO fix it
    SET(ENABLE_RAD_TELEMETRY OFF)
ENDIF()

IF(SLIM_TIMING)
	ADD_DEFINITIONS(-DSLIM_TIMING=1)
ENDIF()

IF(GL_TIMING_QUERY)
    ADD_DEFINITIONS(-DTIMER_QUERY_TIMING=1)
ENDIF()

IF(GL_INTEL_TIMING)
    ADD_DEFINITIONS(-DINTEL_TIMING=1)
ENDIF()
	

#Activate flags that are related to running on the build server.
IF(BUILD_SERVER)
    IF(MSVC_VERSION GREATER 1910)
        SET(BREAKPAD Off)
    ELSE()
        SET(BREAKPAD Off)#Disable breakpad for all situations for now, build server lacks support
    ENDIF()
    SET(DO_DIST_ARCHIVE On)
    SET(ENABLE_RAD_TELEMTRY OFF)
    SET(SINGLE_SHOT_ASSERTS On)
    SET(OPEN_CONSOLE Off)
    SET(DEPLOY On)
ENDIF()

IF(APPLE)
    # assume built-in pthreads on MacOS
    set(CMAKE_THREAD_LIBS_INIT "-lpthread")
    set(CMAKE_HAVE_THREADS_LIBRARY 1)
    set(CMAKE_USE_WIN32_THREADS_INIT 0)
    set(CMAKE_USE_PTHREADS_INIT 1)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    #-------


    SET(ENABLE_RAD_TELEMETRY OFF)
ENDIF()

IF(ENABLE_RAD_TELEMETRY)

ELSE()
	ADD_DEFINITIONS(-DNTELEMETRY)
ENDIF()

IF(MONITOR_MEMORY)
    ADD_DEFINITIONS(-DMONITOR_MEMORY=1)
ENDIF()

IF(SINGLE_SHOT_ASSERTS)
    ADD_DEFINITIONS(-DSINGLE_SHOT_ASSERTS=1)
ENDIF()

IF(OPEN_CONSOLE)
    ADD_DEFINITIONS(-DOPEN_WIN32_CONSOLE=1)
ENDIF()

MESSAGE( STATUS "Type: ${CMAKE_BUILD_TYPE}" )
IF(CMAKE_BUILD_TYPE)
    SET(WIN32_DIST_FOLDER "${CMAKE_BUILD_TYPE}")
ELSE()
    SET(WIN32_DIST_FOLDER "Release")
ENDIF()
MESSAGE(STATUS "Path: ${CMAKE_BINARY_DIR}/${WIN32_DIST_FOLDER}")
MESSAGE(STATUS "")
MESSAGE(STATUS "Build Server: ${BUILD_SERVER}")
MESSAGE(STATUS "Build Ogda: ${BUILD_OGDA}")
MESSAGE(STATUS "")
MESSAGE(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
MESSAGE(STATUS "")
MESSAGE(STATUS "Monitor Memory: ${MONITOR_MEMORY}")
MESSAGE(STATUS "Generator: ${CMAKE_GENERATOR}")
MESSAGE(STATUS "Breakpad: ${BREAKPAD}")
MESSAGE(STATUS "Slim Timing: ${SLIM_TIMING}")
MESSAGE(STATUS "GL Timing Query: ${GL_TIMING_QUERY}")
MESSAGE(STATUS "Intel Timing: ${GL_INTEL_TIMING}")
MESSAGE(STATUS "Asset Manager Perf: ${ASSET_MANAGER_PERF}")
MESSAGE(STATUS "Enable Rad Telemetry: ${ENABLE_RAD_TELEMETRY}")
MESSAGE(STATUS "Force OpenGL Error Checking: ${FORCE_OPENGL_ERROR_CHECKING}")
MESSAGE(STATUS "Git version: ${GIT_VERSION}")
MESSAGE(STATUS "Git rev-list: ${GIT_REV_LIST}")
MESSAGE(STATUS "Build ID: ${BUILD_ID}")

MATH(EXPR OVERGROWTH_VERSION_NR "${GIT_REV_LIST}+30000") #This version number has to be higher than the old system, but we start fresh on 30000

SET(BUILD_TIMESTAMP "UNKNOWN")
IF(${CMAKE_VERSION} VERSION_GREATER 2.8.12)
    string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S UTC" UTC)
ELSE( ) #In case we are in the steam runtime, otherwise, install newer version of cmake.
    EXECUTE_PROCESS(COMMAND "date" "+%Y-%m-%d %H:%M:%S UTC" OUTPUT_VARIABLE tmp)
    STRING(REPLACE "\n" " " BUILD_TIMESTAMP ${tmp})
ENDIF( )

MESSAGE(STATUS ${out})

SET(ARCH_OUT "ix86")
SET(OS_OUT "Unknown")

SET(OVERGROWTH_OUTPUT_NAME "Overgrowth")
SET(OVERGROWTH_WORKER_OUTPUT_NAME "OvergrowthWorker")

IF(APPLE)
    IF(BUILD_SERVER)
        SET(CMAKE_OSX_DEPLOYMENT_TARGET "10.9")
        SET(CMAKE_OSX_ARCHITECTURES "x86_64")
        SET(ARCH_OUT "x86_64")
    ENDIF()
    SET(OS_OUT "MacOSX")
    IF( "${CMAKE_GENERATOR}" STREQUAL "Xcode" )
        SET(CMAKE_OSX_SYSROOT "macosx")
        SET(CMAKE_CONFIGURATION_TYPES "Release;Debug;MinSizeRel;RelWithDebInfo")	
    ELSEIF( "${CMAKE_GENERATOR}" STREQUAL "Unix Makefiles" )
        SET(CMAKE_OSX_SYSROOT "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX${CMAKE_OSX_DEPLOYMENT_TARGET}.sdk")
    ENDIF()
ENDIF()

#define platform variables
IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    SET(LINUX TRUE)
ENDIF()

IF(BREAKPAD)
    ADD_DEFINITIONS(-DBREAKPAD=1)
ENDIF()


if(LINUX)
    OPTION(FORCE32 "Force a 32build on a 64bit host" OFF)
    SET(OS_OUT "Linux")

    if(FORCE32)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
    endif()

    IF(CMAKE_SIZEOF_VOID_P MATCHES "8" AND NOT(FORCE32))
        SET(64BIT_LINUX true)
    ENDIF()

    IF(64BIT_LINUX)
        SET(LIB_SUFFIX "lib64")
        SET(BIN_SUFFIX ".bin.x86_64")
        SET(ARCH_OUT "amd64")
        SET(TOOL_BIN_NAME "bin64")
        set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS ON)
    ELSE()
        SET(LIB_SUFFIX "lib")
        SET(BIN_SUFFIX ".bin.x86")
        SET(ARCH_OUT "ix86")
        SET(TOOL_BIN_NAME "bin")
        set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF)

        ### Ensure LargeFileSupport on 32bit linux
        set(CMAKE_C_FLAGS           "${CMAKE_C_FLAGS} -D_FILE_OFFSET_BITS=64")
        set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} -D_FILE_OFFSET_BITS=64")
    ENDIF()
endif()

IF(WIN32)
    SET(OS_OUT "Windows")
	
	IF(CMAKE_SIZEOF_VOID_P MATCHES "8" AND NOT(FORCE32))
        SET(64BIT_WINDOWS true)
    ENDIF()
    
    IF(64BIT_WINDOWS)
        SET(ARCH_OUT "amd64")
        set(WINDOWS_LIB_DIR "x64")
    ELSE()
        SET(ARCH_OUT "ix86")
        set(WINDOWS_LIB_DIR "x86")
    ENDIF()
ENDIF()

IF(LINUX)
    FIND_PACKAGE(GTK2 REQUIRED)
    FIND_PACKAGE(SDL2 REQUIRED)
    FIND_PACKAGE(SDL2NET REQUIRED)
    FIND_PACKAGE(OpenAL REQUIRED)
ENDIF()

#Build static libraries only for FFTW
SET(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
#Enable floating point for FFTW
SET(ENABLE_FLOAT ON CACHE BOOL "" FORCE)
#Enable SSE2 for FFTW
SET(ENABLE_SSE2 ON CACHE BOOL "" FORCE)
#Disable bench tests for FFTW
SET(BUILD_TESTS OFF CACHE BOOL "" FORCE)

#Create subprojects that have their own CMakeLists
ADD_SUBDIRECTORY(../Libraries/crunch-r319 crnlib)
ADD_SUBDIRECTORY(../Libraries/glad glad)
set(ZSTD_BUILD_PROGRAMS Off)
set(ZSTD_BUILD_STATIC ON)
set(ZSTD_BUILD_SHARED Off)
ADD_SUBDIRECTORY(../Libraries/zstd-1.5.0/build/cmake zstd)
ADD_SUBDIRECTORY(../Libraries/freetype-2.12.1 freetype)

IF(BUILD_OVERGROWTH) 
    #Create subprojects that have their own CMakeLists
    ADD_SUBDIRECTORY(triangle triangle)
    ADD_SUBDIRECTORY(minizip minizip)
    ADD_SUBDIRECTORY(murmurhash3 murmurhash3)
    ADD_SUBDIRECTORY(binn binn)
    ADD_SUBDIRECTORY(TheoraPlayer TheoraPlayer)
    ADD_SUBDIRECTORY(recastnavigation recastnavigation)
    # disable all demos
    SET(BUILD_DEMOS OFF CACHE BOOL "Set when you want to build the demos for underlying library")
    # enable USE_MSVC_RUNTIME_LIBRARY_DLL
    SET(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "Use MSVC Runtime Library DLL (/MD or /MDd)")
    SET(USE_GLUT OFF CACHE BOOL "Use Glut")
    ADD_SUBDIRECTORY(bullet3-2.89 bullet3-2.89)
    ADD_SUBDIRECTORY(angelscript_2_32_0_sdk/angelscript/projects/cmake angelscript_2_32_0_sdk)

    #Put subprojects in the "Support" folder
    SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)
    SET_PROPERTY(TARGET murmurhash3 PROPERTY FOLDER "Support")
    SET_PROPERTY(TARGET binn PROPERTY FOLDER "Support")
    SET_PROPERTY(TARGET angelscript PROPERTY FOLDER "Support/Angelscript")
    SET_PROPERTY(TARGET triangle PROPERTY FOLDER "Support")
    SET_PROPERTY(TARGET minizip PROPERTY FOLDER "Support")
    SET_PROPERTY(TARGET TheoraPlayer PROPERTY FOLDER "Support")
    SET_PROPERTY(TARGET glad PROPERTY FOLDER "Support")
    SET_PROPERTY(TARGET BulletCollision PROPERTY FOLDER "Support/BulletPhysics")
    SET_PROPERTY(TARGET BulletDynamics PROPERTY FOLDER "Support/BulletPhysics")
    SET_PROPERTY(TARGET BulletFileLoader PROPERTY FOLDER "Support/BulletPhysics")
    SET_PROPERTY(TARGET BulletSoftBody PROPERTY FOLDER "Support/BulletPhysics")
    SET_PROPERTY(TARGET BulletWorldImporter PROPERTY FOLDER "Support/BulletPhysics")
    SET_PROPERTY(TARGET BulletXmlWorldImporter PROPERTY FOLDER "Support/BulletPhysics")
    SET_PROPERTY(TARGET ConvexDecomposition PROPERTY FOLDER "Support/BulletPhysics")
    SET_PROPERTY(TARGET GIMPACTUtils PROPERTY FOLDER "Support/BulletPhysics")
    SET_PROPERTY(TARGET HACD PROPERTY FOLDER "Support/BulletPhysics")
    SET_PROPERTY(TARGET LinearMath PROPERTY FOLDER "Support/BulletPhysics")
    IF(WIN32)
 	    SET_PROPERTY(TARGET App_obj2sdf PROPERTY FOLDER "Support/BulletPhysics")
	    SET_PROPERTY(TARGET Bullet2FileLoader PROPERTY FOLDER "Support/BulletPhysics")
        SET_PROPERTY(TARGET Bullet3Collision PROPERTY FOLDER "Support/BulletPhysics")
        SET_PROPERTY(TARGET Bullet3Common PROPERTY FOLDER "Support/BulletPhysics")
        SET_PROPERTY(TARGET Bullet3Dynamics PROPERTY FOLDER "Support/BulletPhysics")
        SET_PROPERTY(TARGET Bullet3Geometry PROPERTY FOLDER "Support/BulletPhysics")
        SET_PROPERTY(TARGET Bullet3OpenCL_clew PROPERTY FOLDER "Support/BulletPhysics")
        SET_PROPERTY(TARGET BulletInverseDynamics PROPERTY FOLDER "Support/BulletPhysics")
        SET_PROPERTY(TARGET BulletInverseDynamicsUtils PROPERTY FOLDER "Support/BulletPhysics")
        SET_PROPERTY(TARGET BulletRobotics PROPERTY FOLDER "Support/BulletPhysics")
    ENDIF()

    SET_PROPERTY(TARGET crnlib PROPERTY FOLDER "Support")

    SET_PROPERTY(TARGET Recast PROPERTY FOLDER "Support/Recast")
    SET_PROPERTY(TARGET Detour PROPERTY FOLDER "Support/Recast")
    SET_PROPERTY(TARGET DetourCrowd PROPERTY FOLDER "Support/Recast")
    SET_PROPERTY(TARGET DetourTileCache PROPERTY FOLDER "Support/Recast")
    SET_PROPERTY(TARGET HACD PROPERTY FOLDER "Support/BulletPhysics")
ENDIF()

#Build external projects that don't have a CMakeLists file.
IF(BREAKPAD)
    IF(LINUX)
        ExternalProject_Add(breakpad
            SOURCE_DIR ${CMAKE_SOURCE_DIR}/../Libraries/breakpad/
            CONFIGURE_COMMAND ${CMAKE_SOURCE_DIR}/../Libraries/breakpad/configure
            BUILD_COMMAND make
            INSTALL_COMMAND "echo"
        )
    ENDIF()
ENDIF()

IF(APPLE)
    SET(PLATFORM_PREFIX             "macosx")

    ## NOTE setting the rpath this way only works with CMAKE 2.8.12+
    # A workaround for 2.8.11 is do also set 
    # set(CMAKE_XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS ${BIN_RPATH})
    ##
    
    SET(BIN_RPATH "@executable_path/../Frameworks")

    SET(CMAKE_SKIP_BUILD_RPATH              TRUE)
    SET(CMAKE_BUILD_WITH_INSTALL_RPATH      TRUE)
    SET(CMAKE_INSTALL_RPATH                 ${BIN_RPATH})
    SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH   FALSE)
ENDIF()

IF(FORCE_OPENGL_ERROR_CHECKING)
ELSE()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNO_GL_ERROR_CHECKING=1")
ENDIF()

# Add preprocessor definitions so that the code can use platform #ifdefs
IF(LINUX)
    ADD_DEFINITIONS(${GTK2_DEFINITIONS})

    #-no-strict-aliasing for the sake of angelscript
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -rdynamic -g -fno-omit-frame-pointer") #We add G because we want to be able to debug the coredump
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -rdynamic -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -rdynamic -fno-omit-frame-pointer")

ENDIF()

IF(APPLE)
    #-no-strict-aliasing for the sake of angelscript
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -fno-strict-aliasing")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
ENDIF()

#define _DEPLOY for Release target
IF(DEPLOY)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_DEPLOY")
ELSE()
  # if not building release target, export compile commands for use with a
  # language server
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
ENDIF()

SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")

#enable .pdb symbol generation for Release target on Windows
IF(WIN32) 
    #Disable warnings when using standard c functions in visual studio
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GF -D_CRT_SECURE_NO_WARNINGS -DNEED_SNPRINTF_ONLY /EHsc")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /debug")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO}")

    IF(BUILD_SERVER)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL")
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /GL")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
        set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /LTCG")
    ENDIF()
ENDIF()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DOG_DEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DOG_DEBUG")

SET(SRCDIR "${CMAKE_SOURCE_DIR}/../Source")
SET(LIBDIR "${CMAKE_SOURCE_DIR}/../Libraries")
SET(PLIBDIR "${CMAKE_SOURCE_DIR}/../Proprietary/Libraries")
SET(DATADIR "${CMAKE_SOURCE_DIR}/../Data")

FILE(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/Data)

IF(UNIX)
    ADD_CUSTOM_COMMAND( 
        OUTPUT always
        COMMAND echo > /dev/null
    )
    ADD_CUSTOM_COMMAND( 
        OUTPUT ${SRCDIR}/Version/git_version.cpp
        DEPENDS always
        COMMAND printf 'const int none=-1\;\\nextern const int phoenix_build_id=%s\;\\nconst char* phoenix_platform=\"%s\"\;\\nconst char* phoenix_arch=\"%s\"\;\\nconst char* phoenix_git_version_string=\"%s\"\;\\nconst char* phoenix_build_timestamp=\"%s\"\;\\n' "${BUILD_ID}" "${OS_OUT}" "${ARCH_OUT}" "${GIT_VERSION}" "${BUILD_TIMESTAMP}" > '${SRCDIR}/Version/git_version.cpp'
    )
ELSEIF(WIN32)
    ADD_CUSTOM_COMMAND( 
        OUTPUT always
        COMMAND echo
    )
    ADD_CUSTOM_COMMAND( 
        OUTPUT ${SRCDIR}/Version/git_version.cpp
        DEPENDS always
        COMMAND echo const int none=-1\; extern const int phoenix_build_id=${BUILD_ID}\; const char* phoenix_platform=\"${OS_OUT}\"\; const char* phoenix_arch=\"${ARCH_OUT}\"\; const char* phoenix_git_version_string=\"${GIT_VERSION}\"\; const char* phoenix_build_timestamp=\"${BUILD_TIMESTAMP}\"\; > ${SRCDIR}/Version/git_version.cpp
    )
ELSE()
    ADD_CUSTOM_COMMAND( 
        OUTPUT ${SRCDIR}/Version/git_version.cpp
        COMMAND ${CMAKE_COMMAND} -E copy "${SRCDIR}/Version/fallback_version.c" "${SRCDIR}/Version/git_version.cpp"
    )
ENDIF()

IF(LINUX)
    SET(BIN_RPATH "\$ORIGIN/${LIB_SUFFIX}")
    SET(LIB_ROOT ${LIBDIR}/Linux/${LIB_SUFFIX})

    #Only use the built in library folder if we are on the build server and using the 
    #steam runtime environment.
    IF(BUILD_SERVER) 
        LINK_DIRECTORIES(
            ${LIB_ROOT}
        )
    ENDIF()

    # Don't use -rpath.
    SET(CMAKE_SKIP_BUILD_RPATH TRUE)
    SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    SET(CMAKE_INSTALL_RPATH ${BIN_RPATH})
    SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

    SET(CMAKE_EXECUTABLE_SUFFIX ${BIN_SUFFIX})
    
    IF(64BIT_LINUX)
        SET(LINUX_LIB_DIR "linux64")
    ELSE()
        SET(LINUX_LIB_DIR "linux32")
    ENDIF()

    LINK_DIRECTORIES(
        ${SDL2_LIBRARY}
        ${SDL2NET_LIBRARY}
    ) 

    IF(ENABLE_RAD_TELEMETRY)
	    LINK_DIRECTORIES(
		${PLIBDIR}/Telemetry2SDKLinux_20P3/lib
	    ) 
    ENDIF()


    IF(BREAKPAD)
        LINK_DIRECTORIES( 
            "${CMAKE_BINARY_DIR}/breakpad-prefix/src/breakpad-build/src/client/linux/"
        )
    ENDIF()

    IF(ENABLE_STEAMWORKS)
        LINK_DIRECTORIES( 
            "${PLIBDIR}/steamworks_1.51/sdk/redistributable_bin/${LINUX_LIB_DIR}"
        )
    ENDIF()
ENDIF()

IF(WIN32)
	IF(MSVC_VERSION GREATER 1910)
		set(VERSION_DIR "17")
		MESSAGE(STATUS "Building for VS2017 or later, using MSVC1911 libraries where needed")
	ELSE()
		set(VERSION_DIR "12")
	ENDIF()
    
    LINK_DIRECTORIES(
        ${LIBDIR}/FreeImage/lib/${VERSION_DIR}/${WINDOWS_LIB_DIR}
        ${LIBDIR}/SDL2-2.0.20/lib/${WINDOWS_LIB_DIR}
        ${LIBDIR}/SDL2_net-2.0.1/lib/${WINDOWS_LIB_DIR}
        ${LIBDIR}/openal-soft-1.18.0-bin/libs/${WINDOWS_LIB_DIR}
        ${LIBDIR}/ogg/lib/${WINDOWS_LIB_DIR}
        ${LIBDIR}/vorbis/lib/${WINDOWS_LIB_DIR}
        ${LIBDIR}/nvapi/${WINDOWS_LIB_DIR}
        ${LIBDIR}/zlib1.2.7/lib/${WINDOWS_LIB_DIR}
        ${LIBDIR}/theora/lib/${WINDOWS_LIB_DIR}
        ${LIBDIR}/openvr-master/lib/win/${WINDOWS_LIB_DIR}
	    ${LIBDIR}/breakpad/src/client/windows/lib/${WINDOWS_LIB_DIR}
        ${LIBDIR}/xinput/Lib/x86
        ${LIBDIR}/NvToolsExt/lib/Win32
    )

    IF(ENABLE_RAD_TELEMETRY)
        LINK_DIRECTORIES(
            ${PLIBDIR}/Telemetry2SDKWin32_20P3/lib/${WINDOWS_LIB_DIR}
        )
    ENDIF()
ENDIF()


IF(APPLE)    
    LINK_DIRECTORIES(
        ${LIBDIR}/FreeImage/maclib/64
        ${LIBDIR}/libogg-1.3.3/lib/mac64
        ${LIBDIR}/libvorbis-1.3.6/lib/mac64
        ${LIBDIR}/libtheora-1.1.1/lib/mac64
    )

    IF(ENABLE_STEAMWORKS)
        LINK_DIRECTORIES(
            ${PLIBDIR}/steamworks_1.51/sdk/redistributable_bin/osx
        )
    ENDIF()

    FIND_LIBRARY(SDL_FRAMEWORK SDL2 PATHS ${LIBDIR}/SDL2-2.0.20 NO_DEFAULT_PATH)
    FIND_LIBRARY(SDL2NET_FRAMEWORK SDL2_net PATHS ${LIBDIR}/SDL2_net-2.0.1 NO_DEFAULT_PATH)
    FIND_PATH(SDL2_INCLUDE_DIR NAMES SDL.h PATHS ${SDL_FRAMEWORK}/Headers
    NO_DEFAULT_PATH
)
    FIND_LIBRARY(OPENAL_FRAMEWORK OpenAL)
    FIND_LIBRARY(CARBON_FRAMEWORK Carbon)
    FIND_LIBRARY(AU_FRAMEWORK AudioUnit)
    FIND_LIBRARY(FORCE_FEEDBACK_FRAMEWORK ForceFeedback)
    FIND_LIBRARY(CORE_AUDIO_FRAMEWORK CoreAudio)
    FIND_LIBRARY(OPENGL_FRAMEWORK OpenGL)
    FIND_LIBRARY(IOKIT_FRAMEWORK IOKit)
    FIND_LIBRARY(APPKIT_FRAMEWORK AppKit)
    SET(FRAMEWORKS
        ${FORCE_FEEDBACK_FRAMEWORK}
        ${AU_FRAMEWORK}
        ${CORE_AUDIO_FRAMEWORK}
        ${OPENAL_FRAMEWORK}
        ${CARBON_FRAMEWORK}
        ${OPENGL_FRAMEWORK}
        ${IOKIT_FRAMEWORK}
        ${APPKIT_FRAMEWORK}
        ${SDL_FRAMEWORK}
	${SDL2NET_FRAMEWORK}
    )
ENDIF()

ADD_DEFINITIONS(
    -DHAVE_MIXCFG_H
    -DTIXML_USE_STL
    -DIMGUI_ENABLE_FREETYPE
) 

INCLUDE_DIRECTORIES(SYSTEM
    ${LIBDIR}/crunch-r319/crnlib
    ${LIBDIR}/crunch-r319/inc
)

INCLUDE_DIRECTORIES(
    ${SRCDIR}
    ${LIBDIR}
    ${LIBDIR}/imgui-1.85
    ${LIBDIR}/imgui-1.85/misc/freetype
    ${LIBDIR}/ovr_sdk_win_1.12.0_public/OculusSDK/LibOVR/Include
    ${LIBDIR}/openvr-master/headers
    ${LIBDIR}/glm-0.9.4.2/
    bullet3-2.89/src
    angelscript_2_32_0_sdk/angelscript/include
    angelscript_2_32_0_sdk
    /angelscript/source
    ${LIBDIR}/tinyxml
    ${LIBDIR}/nvapi
    ${LIBDIR}/tclap/include
    ${LIBDIR}/tut-framework/include
    ${LIBDIR}/recastnavigation/Recast/Include
    ${LIBDIR}/recastnavigation/Detour/Include
    ${LIBDIR}/recastnavigation/DetourTileCache/Include
    ${LIBDIR}/recastnavigation/DetourCrowd/Include
    ${LIBDIR}/zstd-1.5.0/lib
    ${LIBDIR}/binn-3.0/src
    ${LIBDIR}/TheoraPlayer/Source
    ${LIBDIR}/freetype-2.12.1/include
)

IF(ENABLE_STEAMWORKS)
    INCLUDE_DIRECTORIES(
        ${PLIBDIR}/steamworks_1.51/sdk/public/steam
    )
ELSEIF(ENABLE_GAMENETWORKINGSOCKETS)
    INCLUDE_DIRECTORIES(
        ${LIBDIR}/GameNetworkingSockets-1.4.0/include/steam
    )
ENDIF()

IF(BREAKPAD)
    INCLUDE_DIRECTORIES(
        ${LIBDIR}/breakpad/src
    )
ENDIF()

IF(LINUX)
    MESSAGE(STATUS "SDL2_INCLUDE_DIR: ${SDL2_INCLUDE_DIR}")
    
    INCLUDE_DIRECTORIES(
        LinuxInclude
        ${GTK2_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIR}
        ${SDL2NET_INCLUDE_DIR}
        ${LIBDIR}/Linux/include
        ${LIBDIR}/Linux/include/AL
        ${LIBDIR}/${FFTW_DIR}
    	${LIBDIR}/FreeImage/linuxinclude
    )

    IF(ENABLE_RAD_TELEMETRY)
    INCLUDE_DIRECTORIES(
        ${PLIBDIR}/Telemetry2SDKLinux_20P3/include
	)
    ENDIF()
ENDIF() 

if(WIN32)
    INCLUDE_DIRECTORIES(
      ${LIBDIR}/SDL2-2.0.20/include
      ${LIBDIR}/SDL2_net-2.0.1/include
      ${LIBDIR}/zlib1.2.7/include
      ${LIBDIR}/ogg/include
      ${LIBDIR}/vorbis/include
      ${LIBDIR}/theora/include
      ${LIBDIR}/NvToolsExt/include
      ${SDL2_SOURCE_DIR}/include
      "${LIBDIR}/openal-soft-1.18.0-bin/include/AL"
      ${LIBDIR}/${FFTW_DIR}
      ${LIBDIR}/FreeImage/wininclude/${VERSION_DIR}
    )

    IF(ENABLE_RAD_TELEMETRY)
        INCLUDE_DIRECTORIES(
            ${PLIBDIR}/Telemetry2SDKWin32_20P3/include
            ${PLIBDIR}/Telemetry2SDKWin32_20P3/src/gpu
       )
    ENDIF()
ENDIF()

if(APPLE)
    INCLUDE_DIRECTORIES(
        #${LIBDIR}/../MacInterfaces/
        ${LIBDIR}/libogg-1.3.3/include
        ${LIBDIR}/libvorbis-1.3.6/include
        ${LIBDIR}/libtheora-1.1.1/include
        ${LIBDIR}/SDL2-2.0.20/SDL2.framework/Headers
        ${LIBDIR}/SDL2_net-2.0.1/SDL2_net.framework/Headers
        ${SDL2_SOURCE_DIR}/include
        ${SDL2_INCLUDE_DIR}
    	${LIBDIR}/FreeImage/macinclude
	/opt/X11/include
    )

    IF(ENABLE_RAD_TELEMETRY)
        INCLUDE_DIRECTORIES(
            ${PLIBDIR}/Telemetry2SDKMac_20P3/include
            ${PLIBDIR}/Telemetry2SDKMac_20P3/src/gpu
        )
    ENDIF()
ENDIF()


SET(IMGUI_SRCS
    ${LIBDIR}/imgui-1.85/imgui.cpp
    ${LIBDIR}/imgui-1.85/imgui_demo.cpp
    ${LIBDIR}/imgui-1.85/imgui_draw.cpp
    ${LIBDIR}/imgui-1.85/imgui_widgets.cpp
    ${LIBDIR}/imgui-1.85/imgui_tables.cpp
    ${LIBDIR}/imgui-1.85/imgui_color_picker.cpp
    ${LIBDIR}/imgui-1.85/implot.cpp
    ${LIBDIR}/imgui-1.85/implot_items.cpp
    ${LIBDIR}/imgui-1.85/misc/freetype/imgui_freetype.cpp
)

ADD_LIBRARY( imgui STATIC
    ${IMGUI_SRCS}
)

TARGET_LINK_LIBRARIES( imgui PUBLIC 
    freetype    
)

SET_PROPERTY(TARGET imgui PROPERTY FOLDER "Support")

FILE(GLOB TINYXML_SRCS
    ${LIBDIR}/tinyxml/tinystr.cpp
    ${LIBDIR}/tinyxml/tinyxml.cpp
    ${LIBDIR}/tinyxml/tinyxmlerror.cpp
    ${LIBDIR}/tinyxml/tinyxmlparser.cpp
    ${LIBDIR}/tinyxml/tinystr.h
    ${LIBDIR}/tinyxml/tinyxml.h
)

ADD_LIBRARY( tinyxml STATIC
    ${TINYXML_SRCS}
) 

SET_PROPERTY(TARGET tinyxml PROPERTY FOLDER "Support")

file(GLOB TREX_SRCS
    ${LIBDIR}/trex/trex.cpp
    ${LIBDIR}/trex/trex.h
)

ADD_LIBRARY( trex STATIC
    ${TREX_SRCS}
)

SET_PROPERTY(TARGET trex PROPERTY FOLDER "Support")

SET(LOG_LEVEL 4 CACHE INTEGER "Set Logging level from 1=Fatal to 6=Spam, default 4=Info")
MESSAGE(STATUS "Log level: ${LOG_LEVEL}")
ADD_DEFINITIONS(-DLOG_LEVEL=${LOG_LEVEL})

IF(NOT AUX_DATA STREQUAL "")
    FILE(TO_CMAKE_PATH "${AUX_DATA}" AUX_DATA_FP)
    ADD_DEFINITIONS(-DAUX_DATA=\"${AUX_DATA_FP}\")
ENDIF()

include(SourceUtils)

ADD_LIBRARY( OgLogging STATIC)
target_add_grouped_source(OgLogging Logging)

ADD_LIBRARY( OgCompat STATIC
    ${SRCDIR}/Compat/compat.h
    ${SRCDIR}/Compat/fileio.cpp
    ${SRCDIR}/Compat/fileio.h 
    ${SRCDIR}/Compat/filepath.h  
)

IF(UNIX)
    TARGET_SOURCES(OgCompat PRIVATE
        ${SRCDIR}/Compat/UNIX/unix_filepath.cpp
        ${SRCDIR}/Compat/UNIX/unix_compat.cpp
        ${SRCDIR}/Compat/UNIX/unix_compat.h
    )
ENDIF()

IF(LINUX)
    TARGET_SOURCES(OgCompat PRIVATE
        ${SRCDIR}/Compat/Linux/linux_compat.cpp
        ${SRCDIR}/Compat/Linux/linux_compat.h
    )
ENDIF()

IF(APPLE)
    TARGET_SOURCES(OgCompat PRIVATE
        ${SRCDIR}/Compat/Mac/mac_compat.mm
        ${SRCDIR}/Compat/Mac/mac_compat.h
    )
ENDIF()

IF(WIN32)
    TARGET_SOURCES(OgCompat PRIVATE
        ${SRCDIR}/Compat/Win/win_compat.cpp 
        ${SRCDIR}/Compat/Win/win_compat.h
        ${SRCDIR}/Compat/Win/win_filepath.cpp
    )
ENDIF()

IF(LINUX AND BUILD_OGDA) #Making Ogda linux only for now.
    FILE(GLOB OGDA_SRCS RELATIVE ${CMAKE_SOURCE_DIR}
        ${SRCDIR}/Threading/thread_sanity.cpp
        ${SRCDIR}/Threading/thread_sanity.h
        ${SRCDIR}/Asset/assetbase.h
        ${SRCDIR}/Asset/assetbase.cpp
        ${SRCDIR}/Asset/assetmanager.h
        ${SRCDIR}/Asset/assetmanager.cpp
        ${SRCDIR}/Asset/assetloaderrors.h
        ${SRCDIR}/Asset/assetloaderrors.cpp
        ${SRCDIR}/Asset/assetmanagerthreadhandler.h
        ${SRCDIR}/Asset/assetmanagerthreadhandler.cpp
        ${SRCDIR}/Ogda/*.h
        ${SRCDIR}/Ogda/*.cpp
        ${SRCDIR}/Ogda/Searchers/*.h
        ${SRCDIR}/Ogda/Searchers/*.cpp
        ${SRCDIR}/Ogda/Searchers/Seekers/*.h
        ${SRCDIR}/Ogda/Searchers/Seekers/*.cpp
        ${SRCDIR}/Ogda/Searchers/Seekers/SeekerTools/*.h
        ${SRCDIR}/Ogda/Searchers/Seekers/SeekerTools/*.cpp
        ${SRCDIR}/Ogda/Builders/*.h
        ${SRCDIR}/Ogda/Builders/*.cpp
        ${SRCDIR}/Ogda/Generators/*.h
        ${SRCDIR}/Ogda/Generators/*.cpp
        ${SRCDIR}/Images/*.h
        ${SRCDIR}/Images/*.hpp
        ${SRCDIR}/Images/*.cpp
        ${SRCDIR}/Version/version.cpp
        ${SRCDIR}/Version/version.h
        ${SRCDIR}/XML/Parsers/jobxmlparser.cpp
        ${SRCDIR}/XML/Parsers/jobxmlparser.h
        ${SRCDIR}/XML/Parsers/xmlparserbase.cpp
        ${SRCDIR}/XML/Parsers/xmlparserbase.h
        ${SRCDIR}/XML/Parsers/assetloadwarningparser.cpp
        ${SRCDIR}/XML/Parsers/assetloadwarningparser.h
        ${SRCDIR}/Utility/serialize.cpp
        ${SRCDIR}/Utility/serialize.h
        ${SRCDIR}/Utility/strings.cpp
        ${SRCDIR}/Utility/strings.h
        ${SRCDIR}/Utility/commonregex.cpp
        ${SRCDIR}/Utility/commonregex.h
        ${SRCDIR}/Utility/hash.cpp
        ${SRCDIR}/Utility/hash.h
        ${SRCDIR}/Utility/pcg_basic.cpp
        ${SRCDIR}/Utility/pcg_basic.h
        ${SRCDIR}/Internal/filesystem.h
        ${SRCDIR}/Internal/filesystem.cpp
        ${SRCDIR}/Internal/path.h
        ${SRCDIR}/Internal/path.cpp
        ${SRCDIR}/Internal/common.cpp
        ${SRCDIR}/Internal/common.h
        ${SRCDIR}/Internal/cachefile.cpp
        ${SRCDIR}/Internal/cachefile.h
        ${SRCDIR}/Internal/datemodified.h
        ${SRCDIR}/Internal/datemodified.cpp
        ${SRCDIR}/Internal/checksum.h
        ${SRCDIR}/Internal/checksum.cpp
        ${SRCDIR}/Internal/modid.cpp
        ${SRCDIR}/Graphics/converttexture.cpp
        ${SRCDIR}/JSON/*.cpp
        ${SRCDIR}/JSON/*.h
        ${SRCDIR}/Memory/*.cpp
        ${SRCDIR}/Memory/*.h
    )

    IF(WIN32)
        IF(MSVC_VERSION LESS 1911)
            LIST(APPEND OGDA_SRCS ${SRCDIR}/Internal/snprintf.c)
        ENDIF()

        IF(ENABLE_RAD_TELEMETRY)
            LIST(APPEND OGDA_SRCS
                ${PLIBDIR}/Telemetry2SDKWin32_20P3/src/gpu/gpu_opengl.cpp
            )
        ENDIF()
    ENDIF()

    ADD_EXECUTABLE(Ogda ${SRCDIR}/Version/git_version.cpp ${OGDA_SRCS} )

    TARGET_LINK_LIBRARIES(Ogda
        OgCompat
        OgLogging
        tinyxml
        trex
        murmurhash3
        freeimage
        pthread
        crnlib
        glad
    )

    SET_TARGET_PROPERTIES(Ogda PROPERTIES
        COMPILE_DEFINITIONS OGDA=1
        COMPILE_FLAGS "-w"
    )
ENDIF()

ADD_DEFINITIONS(-DUNIT_TESTS)

SET(OVERGROWTH_INTERNAL_SRCS
    ${LIBDIR}/ovr_sdk_win_1.12.0_public/OculusSDK/LibOVR/Src/OVR_StereoProjection.cpp
    ${LIBDIR}/ovr_sdk_win_1.12.0_public/OculusSDK/LibOVR/Src/OVR_CAPI_Util.cpp
    ${LIBDIR}/ovr_sdk_win_1.12.0_public/OculusSDK/LibOVR/Src/OVR_CAPIShim.c
    ${SRCDIR}/Internal/checksum.cpp
    ${SRCDIR}/Internal/crashreport.cpp
    ${SRCDIR}/Internal/collisiondetection.cpp
    ${SRCDIR}/Internal/config.cpp
    ${SRCDIR}/Internal/datemodified.cpp
    ${SRCDIR}/Internal/dialogues.cpp
    ${SRCDIR}/Internal/error.cpp
    ${SRCDIR}/Internal/hardware_specs.cpp
    ${SRCDIR}/Internal/memwrite.cpp
    ${SRCDIR}/Internal/stopwatch.cpp
    ${SRCDIR}/Internal/textfile.cpp
    ${SRCDIR}/Internal/timer.cpp
    ${SRCDIR}/Internal/treestructure.cpp
    ${SRCDIR}/Internal/cachefile.cpp
    ${SRCDIR}/Internal/varstring.cpp
    ${SRCDIR}/Internal/levelxml.cpp
    ${SRCDIR}/Internal/levelxml_script.cpp
    ${SRCDIR}/Internal/zip_util.cpp
    ${SRCDIR}/Internal/returnpathutil.cpp
    ${SRCDIR}/Internal/casecorrectpath.cpp
    ${SRCDIR}/Internal/file_descriptor.cpp
    ${SRCDIR}/Internal/path_set.cpp
    ${SRCDIR}/Internal/comma_separated_list.h
    ${SRCDIR}/Internal/config.h
    ${SRCDIR}/Internal/crashreport.h
    ${SRCDIR}/Internal/datemodified.h
    ${SRCDIR}/Internal/dialogues.h
    ${SRCDIR}/Internal/error.h
    ${SRCDIR}/Internal/file_descriptor.h
    ${SRCDIR}/Internal/hardware_specs.h
    ${SRCDIR}/Internal/integer.h
    ${SRCDIR}/Internal/levelxml.h
    ${SRCDIR}/Internal/levelxml_script.h
    ${SRCDIR}/Internal/memwrite.h
    ${SRCDIR}/Internal/message.h
    ${SRCDIR}/Internal/path_set.h
    ${SRCDIR}/Internal/returnpathutil.h
    ${SRCDIR}/Internal/scoped_buffer.h
    ${SRCDIR}/Internal/SIMD.h
    ${SRCDIR}/Internal/stopwatch.h
    ${SRCDIR}/Internal/textfile.h
    ${SRCDIR}/Internal/timer.h
    ${SRCDIR}/Internal/treestructure.h
    ${SRCDIR}/Internal/varstring.h
    ${SRCDIR}/Internal/cachefile.h
    ${SRCDIR}/Internal/callstack.h
    ${SRCDIR}/Internal/casecorrectpath.h
    ${SRCDIR}/Internal/checksum.h
    ${SRCDIR}/Internal/collisiondetection.h
    ${SRCDIR}/Internal/win_mem_track.h
    ${SRCDIR}/Internal/win_mem_track.cpp
    ${SRCDIR}/Internal/common.h
    ${SRCDIR}/Internal/common.cpp
    ${SRCDIR}/Internal/profiler.h
    ${SRCDIR}/Internal/profiler.cpp
    ${SRCDIR}/Internal/filesystem.h
    ${SRCDIR}/Internal/filesystem.cpp
    ${SRCDIR}/Internal/modloading.cpp
    ${SRCDIR}/Internal/modloading.h
    ${SRCDIR}/Internal/modid.cpp
    ${SRCDIR}/Internal/modid.h
    ${SRCDIR}/Internal/detect_settings.cpp
    ${SRCDIR}/Internal/detect_settings.h
    ${SRCDIR}/Internal/spawneritem.cpp
    ${SRCDIR}/Internal/spawneritem.h
    ${SRCDIR}/Internal/path.cpp
    ${SRCDIR}/Internal/path.h
    ${SRCDIR}/Internal/assetpreload.cpp
    ${SRCDIR}/Internal/assetpreload.h
    ${SRCDIR}/Internal/locale.cpp
    ${SRCDIR}/Internal/locale.h

    ${SRCDIR}/Compat/hardware_info.h
    ${SRCDIR}/Compat/time.cpp
    ${SRCDIR}/Compat/time.h
    ${SRCDIR}/Compat/os_dialogs.h
    ${SRCDIR}/Compat/platformsetup.cpp
    ${SRCDIR}/Compat/platformsetup.h
    ${SRCDIR}/Compat/processpool.cpp
    ${SRCDIR}/Compat/processpool.h
)

IF(LINUX)
    LIST(APPEND OVERGROWTH_INTERNAL_SRCS 
        ${SRCDIR}/Compat/Linux/os_dialogs_linux.cpp
        ${SRCDIR}/Compat/Linux/linux_hardware_info.cpp
    )
ENDIF()


IF(APPLE)
    LIST(APPEND OVERGROWTH_INTERNAL_SRCS 
        ${SRCDIR}/Compat/Mac/os_dialogs_mac.mm
        ${SRCDIR}/Compat/Mac/os_file_dialogs_mac.mm
        ${SRCDIR}/Compat/Mac/os_file_dialogs_mac.h
        ${SRCDIR}/Compat/Mac/mac_hardware_info.cpp
    )
ENDIF()

IF(WIN32)
	LIST(APPEND OVERGROWTH_INTERNAL_SRCS 
            ${SRCDIR}/Compat/Win/os_dialogs_win.cpp
            ${SRCDIR}/Compat/Win/win_filepath.cpp
            ${SRCDIR}/Compat/Win/win_hardware_info.cpp
	)

	IF(MSVC_VERSION LESS 1911)
		LIST(APPEND OVERGROWTH_INTERNAL_SRCS ${SRCDIR}/Internal/snprintf.c)
	ENDIF()    

	IF(ENABLE_RAD_TELEMETRY)
	    LIST(APPEND OVERGROWTH_INTERNAL_SRCS 
                ${PLIBDIR}/Telemetry2SDKWin32_20P3/src/gpu/gpu_opengl.cpp
            )
	ENDIF()
ENDIF()

LIST(APPEND OVERGROWTH_SRCS
    ${OVERGROWTH_INTERNAL_SRCS}
    ${LIBDIR}/opengl.h
    ${SRCDIR}/Version/version.h
    ${SRCDIR}/Version/version.cpp
)

SOURCE_GROUP(Internal FILES ${OVERGROWTH_INTERNAL_SRCS})
SOURCE_GROUP(ImGui FILES ${IMGUI_SRCS})
SOURCE_GROUP(TinyXML FILES ${TINYXML_SRCS})

IF(WIN32) 
    SET(STATIC_LIBS
        ${STATIC_LIBS}
        zlib
        xinput
        openvr_api
        nvapi #For NVIDIA video card queries
        comdlg32 #For WIN32 dialog boxes
#        dbghelp #For Minidump crash logs
    )
    IF(BREAKPAD)
        SET(STATIC_LIBS
            ${STATIC_LIBS}
	    debug exception_handler_D
            optimized exception_handler
	    debug common_D
	    optimized common
	    debug crash_generation_client_D
	    optimized crash_generation_client
	    debug crash_report_sender_D
	    optimized crash_report_sender
        )
    ENDIF()
	IF(ENABLE_RAD_TELEMETRY)
		SET(STATIC_LIBS
            ${STATIC_LIBS}
			telemetry.link
		)
	ENDIF()
ENDIF()

IF(LINUX)
    SET(STATIC_LIBS
        ${STATIC_LIBS}
    )
    IF(ENABLE_RAD_TELEMETRY)
        SET(STATIC_LIBS
            ${STATIC_LIBS}
            TelemetryX64.link
        )
    ENDIF()
ENDIF()

IF(APPLE)
    SET(SSE_FLAGS "-msse -msse2")
ENDIF()

IF(LINUX)
    if(64BIT_LINUX)
        SET(SSE_FLAGS " ")
    else()
        SET(SSE_FLAGS "-msse -msse2")
    endif()
ENDIF()

IF(WIN32)
    SET(SSE_FLAGS "/w44267")
ENDIF()

IF(WIN32)
    string (REPLACE "/DWIN32" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    string (REPLACE "/D_WIN32" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
ENDIF()

SET(OVERGROWTHWORKER_SRCS
    ${SRCDIR}/Images/freeimage_wrapper.cpp
    ${SRCDIR}/Images/texture_data.cpp
    ${SRCDIR}/Internal/datemodified.cpp
    ${SRCDIR}/Internal/filesystem.cpp
    ${SRCDIR}/Internal/path.cpp
    ${SRCDIR}/Internal/common.cpp
    ${SRCDIR}/Internal/worker.cpp
    ${SRCDIR}/Internal/checksum.cpp
    ${SRCDIR}/Internal/modid.cpp
    ${SRCDIR}/Graphics/converttexture.cpp
    ${SRCDIR}/Internal/casecorrectpath.cpp
    ${SRCDIR}/Utility/strings.cpp
    ${SRCDIR}/Memory/allocation.cpp
    ${SRCDIR}/Memory/blockallocation.cpp
    ${SRCDIR}/Memory/block_allocator.cpp
    ${SRCDIR}/Memory/stack_allocator.cpp
    ${SRCDIR}/Memory/bitarray.cpp
    ${SRCDIR}/Threading/thread_sanity.cpp
    ${SRCDIR}/Compat/filepath.h
    ${SRCDIR}/Compat/processpool.cpp
    ${SRCDIR}/Compat/processpool.h
)

IF(WIN32)
    SET(WIN_RESOURCE_FILE icon.rc)
ENDIF()

IF(BUILD_OVERGROWTH)
    
    ADD_EXECUTABLE(OvergrowthWorker ${OVERGROWTHWORKER_SRCS} ${WIN_RESOURCE_FILE})
    SET_TARGET_PROPERTIES(OvergrowthWorker PROPERTIES
        OUTPUT_NAME ${OVERGROWTH_WORKER_OUTPUT_NAME}
        COMPILE_FLAGS ${SSE_FLAGS}
        COMPILE_DEFINITIONS NO_ERR=1
        COMPILE_DEFINITIONS OG_WORKER=1
    )

    TARGET_LINK_LIBRARIES(OvergrowthWorker
        OgCompat
        OgLogging
        freeimage
        tinyxml
        trex
        glad
        crnlib
    )

    IF(MSVC)
        ADD_COMPILE_OPTIONS(/permissive-)
    ENDIF()

    IF(WIN32)
        SET(CMAKE_CXX_STACK_SIZE "4194304")

        TARGET_LINK_LIBRARIES(OvergrowthWorker
            dbghelp
        )
    ENDIF()

    IF(APPLE)
        #TARGET_LINK_LIBRARIES(OvergrowthWorker)
    ENDIF()

    IF(LINUX)
        TARGET_LINK_LIBRARIES(OvergrowthWorker
	        #nvcore nvmath nvimage squish posh
            dl #needed by nvcore
        )
    ENDIF()

    IF(UNIX)
        TARGET_LINK_LIBRARIES(OvergrowthWorker pthread)
    ENDIF()

    ADD_EXECUTABLE(Overgrowth ${SRCDIR}/Version/git_version.cpp ${OVERGROWTH_SRCS} ${WIN_RESOURCE_FILE} )
    SET_TARGET_PROPERTIES(Overgrowth PROPERTIES
        OUTPUT_NAME ${OVERGROWTH_OUTPUT_NAME}
    )

    target_add_grouped_source(Overgrowth AI)
    target_add_grouped_source(Overgrowth Asset)
    target_add_grouped_source(Overgrowth Asset/Asset)
    target_add_grouped_source(Overgrowth Asset/AssetLoader)
    target_add_grouped_source(Overgrowth Editors)
    target_add_grouped_source(Overgrowth Game)
    target_add_grouped_source(Overgrowth Graphics)
    target_add_grouped_source(Overgrowth GUI)
    target_add_grouped_source(Overgrowth GUI/IMUI)
    target_add_grouped_source(Overgrowth GUI/dimgui)
    target_add_grouped_source(Overgrowth Images)
    target_add_grouped_source(Overgrowth Main)
    target_add_grouped_source(Overgrowth Math)
    target_add_grouped_source(Overgrowth Objects)
    target_add_grouped_source(Overgrowth Physics)
    target_add_grouped_source(Overgrowth Scripting)
    target_add_grouped_source(Overgrowth Scripting/angelscript)
    target_add_grouped_source(Overgrowth Scripting/angelscript/add_on/scriptarray)
    target_add_grouped_source(Overgrowth Scripting/angelscript/add_on/scriptdictionary)
    target_add_grouped_source(Overgrowth Scripting/angelscript/add_on/scripthelper)
    target_add_grouped_source(Overgrowth Scripting/angelscript/add_on/scriptstdstring)
    target_add_grouped_source(Overgrowth Scripting/angelscript/add_on/debugger)
    target_add_grouped_source(Overgrowth Sound)
    target_add_grouped_source(Overgrowth Sound/Music)
    target_add_grouped_source(Overgrowth Sound/Loader)
    target_add_grouped_source(Overgrowth Sound/AudioFilters)
    target_add_grouped_source(Overgrowth Threading)
    target_add_grouped_source(Overgrowth UserInput)
    target_add_grouped_source(Overgrowth XML)
    target_add_grouped_source(Overgrowth XML/Parsers)
    target_add_grouped_source(Overgrowth JSON)
    target_add_grouped_source(Overgrowth Memory)
    target_add_grouped_source(Overgrowth Network)
    #In the future, this should only be activated if we're building for the steam target
    target_add_grouped_source(Overgrowth Network/Steam)
    target_add_grouped_source(Overgrowth Network/Basic)
    target_add_grouped_source(Overgrowth Utility)
    target_add_grouped_source(Overgrowth Wrappers)
    target_add_grouped_source(Overgrowth Timing)
    target_add_grouped_source(Overgrowth Steam)
    target_add_grouped_source(Overgrowth Online)
    target_add_grouped_source(Overgrowth Online/Message)

    # TODO: Make another target called "DeployRelease" that doesn't include UNIT_TESTS
    target_add_grouped_source(Overgrowth UnitTests)

    target_add_grouped_script(Overgrowth Scripts)
    target_add_grouped_script(Overgrowth "Scripts/hotspots")
    target_add_grouped_shaders(Overgrowth "GLSL") 

    IF(WIN32)
        SET_TARGET_PROPERTIES(Overgrowth PROPERTIES
            COMPILE_FLAGS "${SSE_FLAGS} /DWIN32"
        )
        IF(MSVC)
            SET_TARGET_PROPERTIES(Overgrowth PROPERTIES 
                COMPILE_FLAGS "${SSE_FLAGS} /DWIN32 /MP"
            )
            SET_TARGET_PROPERTIES(Overgrowth PROPERTIES
	        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/.."
            )
	    SET_PROPERTY(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Overgrowth)
        ENDIF()
        
        function(CopyDLL start_path end_path)
            add_custom_command( TARGET Overgrowth POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${start_path}"
                    "\"$<TARGET_FILE_DIR:Overgrowth>/${end_path}\""
            )
        endfunction(CopyDLL)
        
        CopyDLL("${CMAKE_SOURCE_DIR}/steam_appid.txt" "")
        CopyDLL("${LIBDIR}/SDL2-2.0.20/lib/${WINDOWS_LIB_DIR}/SDL2.dll" "")
        CopyDLL("${LIBDIR}/SDL2_net-2.0.1/lib/${WINDOWS_LIB_DIR}/SDL2_net.dll" "")

        IF(ENABLE_STEAMWORKS)
                if(64BIT_WINDOWS)
                    CopyDLL("${PLIBDIR}/steamworks_1.51/sdk/redistributable_bin/win64/steam_api64.dll" "")
                else()
                    CopyDLL("${PLIBDIR}/steamworks_1.51/sdk/redistributable_bin/${WINDOWS_LIB_DIR}/steam_api.dll" "")
                endif()
            ELSEIF(ENABLE_GAMENETWORKINGSOCKETS)
                CopyDLL("${LIBDIR}/GameNetworkingSockets-1.4.0/win64_bin/GameNetworkingSockets.dll" "")
                CopyDLL("${LIBDIR}/protobuf/win64_bin/libprotobuf.dll" "")
                CopyDLL("${LIBDIR}/protobuf/win64_bin/libprotoc.dll" "")
                CopyDLL("${LIBDIR}/openssl/win64_bin/libssl-1_1-x64" "")
                CopyDLL("${LIBDIR}/openssl/win64_bin/libcrypto-1_1-x64" "")
        ENDIF()

        CopyDLL("${LIBDIR}/NvToolsExt/bin/Win32/nvToolsExt32_1.dll" "")
        
        CopyDLL("${LIBDIR}/openvr-master/bin/win/${WINDOWS_LIB_DIR}/openvr_api.dll" "")

        CopyDLL("${LIBDIR}/msvc11redist/${WINDOWS_LIB_DIR}/msvcp110.dll" "")
        CopyDLL("${LIBDIR}/msvc11redist/${WINDOWS_LIB_DIR}/msvcr110.dll" "")
        CopyDLL("${LIBDIR}/msvc11redist/${WINDOWS_LIB_DIR}/vccorlib110.dll" "")
        CopyDLL("${LIBDIR}/msvc11redist/${WINDOWS_LIB_DIR}/vcomp110.dll" "")

        CopyDLL("${LIBDIR}/openal-soft-1.18.2-bin/bin/${WINDOWS_LIB_DIR}/OpenAL32.dll" "")
        #CopyDLL("${LIBDIR}/openal-soft-1.18.0-bin/bin/wrap_oal.dll" "")
        
        CopyDLL("${LIBDIR}/ogg/bin/${WINDOWS_LIB_DIR}/libogg.dll" "")
        CopyDLL("${LIBDIR}/vorbis/bin/${WINDOWS_LIB_DIR}/libvorbis.dll" "")
        CopyDLL("${LIBDIR}/vorbis/bin/${WINDOWS_LIB_DIR}/libvorbisfile.dll" "")
        CopyDLL("${LIBDIR}/theora/bin/${WINDOWS_LIB_DIR}/libtheora.dll" "")
        CopyDLL("${LIBDIR}/zlib1.2.7/bin/${WINDOWS_LIB_DIR}/zlib1.dll" "zlib.dll")
        CopyDLL("${LIBDIR}/FreeImage/bin/${VERSION_DIR}/${WINDOWS_LIB_DIR}/FreeImage.dll" "")
        IF(ENABLE_RAD_TELEMETRY)
            #IF(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebIn")
            IF(64BIT_WINDOWS)
                CopyDLL("${PLIBDIR}/Telemetry2SDKWin32_20P3/redist/telemetry64.dll" "")
            ELSE()
                CopyDLL("${PLIBDIR}/Telemetry2SDKWin32_20P3/redist/telemetry32.dll" "")
            ENDIF()
        ENDIF()

        ##Dump symbol file if building with breakpad support.
        IF(BREAKPAD)
            FIND_PROGRAM(DUMP_SYMS dump_syms "${LIBDIR}/breakpad/src/tools/windows/binaries")
            IF(DUMP_SYMS)
                ADD_CUSTOM_COMMAND(TARGET Overgrowth POST_BUILD
                    COMMAND ${DUMP_SYMS} "\"$<TARGET_FILE_DIR:Overgrowth>/${OVERGROWTH_OUTPUT_NAME}.exe\"" ">" "\"$<TARGET_FILE_DIR:Overgrowth>/${OVERGROWTH_OUTPUT_NAME}.exe.sym\""
                )
                ADD_CUSTOM_COMMAND(TARGET OvergrowthWorker POST_BUILD
                    COMMAND ${DUMP_SYMS} "\"$<TARGET_FILE_DIR:OvergrowthWorker>/${OVERGROWTH_WORKER_OUTPUT_NAME}.exe\"" ">" "\"$<TARGET_FILE_DIR:OvergrowthWorker>/${OVERGROWTH_WORKER_OUTPUT_NAME}.exe.sym\""
                )
            ELSE()
                MESSAGE(STATUS "You're missing the program dump_syms from the google breakpad project in your PATH's, this means the .sym files are not being generated after build.")
            ENDIF()
        ENDIF()
    ENDIF()

    IF(UNIX)
        SET_TARGET_PROPERTIES(Overgrowth PROPERTIES
            COMPILE_FLAGS "${SSE_FLAGS}"
        )
    ENDIF()

    IF(LINUX)
        if(BREAKPAD)
            ADD_CUSTOM_COMMAND(TARGET Overgrowth POST_BUILD
                COMMAND "${CMAKE_BINARY_DIR}/breakpad-prefix/src/breakpad-build/src/tools/linux/dump_syms/dump_syms" "${CMAKE_BINARY_DIR}/${OVERGROWTH_OUTPUT_NAME}${BIN_SUFFIX}" ">" "${CMAKE_BINARY_DIR}/${OVERGROWTH_OUTPUT_NAME}${BIN_SUFFIX}.sym"
            )
            ADD_CUSTOM_COMMAND(TARGET OvergrowthWorker POST_BUILD
                COMMAND "${CMAKE_BINARY_DIR}/breakpad-prefix/src/breakpad-build/src/tools/linux/dump_syms/dump_syms" "${CMAKE_BINARY_DIR}/${OVERGROWTH_WORKER_OUTPUT_NAME}${BIN_SUFFIX}" ">" "${CMAKE_BINARY_DIR}/${OVERGROWTH_WORKER_OUTPUT_NAME}${BIN_SUFFIX}.sym"
            )

            #BREAKPAD required c++11 and that pendantic-errors is turned off.
            SET(BREAKPAD_FLAGS "")
        ENDIF()

        # Disabling strict-aliasing, which is default on gcc because angelscript interface has an api function that requires a return of *void.
        SET_TARGET_PROPERTIES(Overgrowth PROPERTIES COMPILE_FLAGS "-Wall -Wno-long-long -Wno-variadic-macros -Wno-trigraphs -Wno-unknown-pragmas -Wno-deprecated-declarations -Wno-unused-variable -Wno-unused-function -Wno-reorder -Werror=return-type ${SSE_FLAGS} ${BREAKPAD_FLAGS} ${NON_BREAKPAD_FLAGS}")
    ENDIF()
        
    # Supress all compile warnings because we shouldn't have to care about external code cleanlyness
    SET(EXTERNAL_LIB_COMPILE_FLAGS "${COMPILE_FLAGS} -w")

    # Use "-noover" to surpress warnings about overriding the library's warning level in MSVC
    IF(MSVC)
        SET(EXTERNAL_LIB_COMPILE_FLAGS "${EXTERNAL_LIB_COMPILE_FLAGS} -noover")
    ENDIF()

    SET_PROPERTY(TARGET murmurhash3             PROPERTY COMPILE_FLAGS "${EXTERNAL_LIB_COMPILE_FLAGS}")
    SET_PROPERTY(TARGET binn                    PROPERTY COMPILE_FLAGS "${EXTERNAL_LIB_COMPILE_FLAGS}")
    SET_PROPERTY(TARGET angelscript             PROPERTY COMPILE_FLAGS "${EXTERNAL_LIB_COMPILE_FLAGS}")
#   SET_PROPERTY(TARGET game                    PROPERTY COMPILE_FLAGS "${EXTERNAL_LIB_COMPILE_FLAGS}")
    SET_PROPERTY(TARGET triangle                PROPERTY COMPILE_FLAGS "${EXTERNAL_LIB_COMPILE_FLAGS}")
    SET_PROPERTY(TARGET minizip                 PROPERTY COMPILE_FLAGS "${EXTERNAL_LIB_COMPILE_FLAGS}")
    SET_PROPERTY(TARGET TheoraPlayer            PROPERTY COMPILE_FLAGS "${EXTERNAL_LIB_COMPILE_FLAGS}")
    SET_PROPERTY(TARGET glad                    PROPERTY COMPILE_FLAGS "${EXTERNAL_LIB_COMPILE_FLAGS}")
    SET_PROPERTY(TARGET trex                    PROPERTY COMPILE_FLAGS "${EXTERNAL_LIB_COMPILE_FLAGS}")
    SET_PROPERTY(TARGET BulletCollision         PROPERTY COMPILE_FLAGS "${EXTERNAL_LIB_COMPILE_FLAGS}")
    SET_PROPERTY(TARGET BulletDynamics          PROPERTY COMPILE_FLAGS "${EXTERNAL_LIB_COMPILE_FLAGS}")
    SET_PROPERTY(TARGET BulletFileLoader        PROPERTY COMPILE_FLAGS "${EXTERNAL_LIB_COMPILE_FLAGS}")
    SET_PROPERTY(TARGET BulletSoftBody          PROPERTY COMPILE_FLAGS "${EXTERNAL_LIB_COMPILE_FLAGS}")
    SET_PROPERTY(TARGET BulletWorldImporter     PROPERTY COMPILE_FLAGS "${EXTERNAL_LIB_COMPILE_FLAGS}")
    SET_PROPERTY(TARGET BulletXmlWorldImporter  PROPERTY COMPILE_FLAGS "${EXTERNAL_LIB_COMPILE_FLAGS}")
    SET_PROPERTY(TARGET ConvexDecomposition     PROPERTY COMPILE_FLAGS "${EXTERNAL_LIB_COMPILE_FLAGS}")
    SET_PROPERTY(TARGET GIMPACTUtils            PROPERTY COMPILE_FLAGS "${EXTERNAL_LIB_COMPILE_FLAGS}")
    SET_PROPERTY(TARGET HACD                    PROPERTY COMPILE_FLAGS "${EXTERNAL_LIB_COMPILE_FLAGS}")
    SET_PROPERTY(TARGET LinearMath              PROPERTY COMPILE_FLAGS "${EXTERNAL_LIB_COMPILE_FLAGS}")
    SET_PROPERTY(TARGET crnlib                  PROPERTY COMPILE_FLAGS "${EXTERNAL_LIB_COMPILE_FLAGS}")
    SET_PROPERTY(TARGET OvergrowthWorker        PROPERTY COMPILE_FLAGS "${EXTERNAL_LIB_COMPILE_FLAGS}")

    IF(APPLE)
        # Setting -g on angelscript prevents clang from crashing due to a bug in the compiler
        SET_TARGET_PROPERTIES(angelscript PROPERTIES COMPILE_FLAGS "-g ${BREAKPAD_FLAGS} ${NON_BREAKPAD_FLAGS}")
        SET_TARGET_PROPERTIES(Overgrowth PROPERTIES COMPILE_FLAGS "${SSE_FLAGS} ${BREAKPAD_FLAGS} ${NON_BREAKPAD_FLAGS}")
        SET_TARGET_PROPERTIES(OvergrowthWorker PROPERTIES COMPILE_FLAGS "${SSE_FLAGS} ${BREAKPAD_FLAGS} ${NON_BREAKPAD_FLAGS}")
    ENDIF()

    TARGET_LINK_LIBRARIES(Overgrowth
        OgCompat
        OgLogging
        BulletDynamics
        BulletCollision
        BulletWorldImporter
        BulletFileLoader
        LinearMath
        minizip
        murmurhash3
        binn
        glad
        TheoraPlayer
        freeimage
        triangle
        angelscript
        imgui
        tinyxml
        trex
        Recast
        DetourCrowd
        DetourTileCache
        Detour
        crnlib
	    freetype
        libzstd_static
        ${STATIC_LIBS}
        ${FLTK_LIBS}
    )

    IF(WIN32)
        IF(64BIT_WINDOWS)
            SET(STEAM_LIB_PATH "${PLIBDIR}/steamworks_1.51/sdk/redistributable_bin/win64/steam_api64.lib")
        ELSE()
            SET(STEAM_LIB_PATH "${PLIBDIR}/steamworks_1.51/sdk/redistributable_bin/${WINDOWS_LIB_DIR}/steam_api.lib")
        ENDIF()
        TARGET_LINK_LIBRARIES(Overgrowth
            openal32
            opengl32
            libogg
            libvorbis
            libvorbisfile
            libtheora
            glu32
            SDL2
            SDL2main
            SDL2_net
            dbghelp
            nvToolsExt32_1
        )
        
        IF(ENABLE_STEAMWORKS)
            TARGET_LINK_LIBRARIES(Overgrowth
                ${STEAM_LIB_PATH}
            )
	ELSEIF(ENABLE_GAMENETWORKINGSOCKETS)
	    TARGET_LINK_LIBRARIES(Overgrowth
		${LIBDIR}/GameNetworkingSockets-1.4.0/win64_lib/GameNetworkingSockets.lib
		${LIBDIR}/protobuf/win64_lib/libprotobuf.lib
		${LIBDIR}/protobuf/win64_lib/libprotoc.lib
		${LIBDIR}/openssl/win64_lib/libssl.lib
		${LIBDIR}/openssl/win64_lib/libcrypto.lib
            )
	ENDIF()
    ENDIF()

    IF(LINUX)
        TARGET_LINK_LIBRARIES(Overgrowth
            SDL2
            ${SDL2NET_LIBRARY}
            openal
            vorbisfile
            vorbis
            ogg
            theoradec
            GL
            GLU
            pthread
            bz2
            png
            jpeg
	        #nvcore nvmath nvimage squish posh
            z
            dl
            ${GTK2_LIBRARIES}
        )

	IF(ENABLE_STEAMWORKS)
            TARGET_LINK_LIBRARIES(Overgrowth
		    steam_api
	    )
	ENDIF()

        IF(BREAKPAD)
            TARGET_LINK_LIBRARIES(Overgrowth
                breakpad_client
            )
            #Make sure that we build breakpad first.
            ADD_DEPENDENCIES(Overgrowth breakpad)
            ADD_DEPENDENCIES(OvergrowthWorker breakpad)
        ENDIF()
    ENDIF()

    IF(APPLE)
        TARGET_LINK_LIBRARIES(Overgrowth
            ${FRAMEWORKS}
            #${PLIBDIR}/Telemetry2SDKMac_20P3/lib/libTelemetryMacX86.link.a
            ogg
            vorbis
            vorbisfile
            theoradec
            bz2
        )
	IF(ENABLE_STEAMWORKS)
            TARGET_LINK_LIBRARIES(Overgrowth
		    steam_api
	    )
	ENDIF()
            
        SET_TARGET_PROPERTIES(Overgrowth PROPERTIES LINK_FLAGS "-F${LIBDIR}/libogg-1.3.3/include")
        SET_TARGET_PROPERTIES(Overgrowth PROPERTIES LINK_FLAGS "-F${LIBDIR}/libvorbis-1.3.6/include")
        #SET_TARGET_PROPERTIES(Overgrowth PROPERTIES LINK_FLAGS "-F${CMAKE_SOURCE_DIR}/../MacInterfaces")
    ENDIF()
ENDIF()

IF(APPLE)
    function(CreateBundleDir path)
        add_custom_command( TARGET Overgrowth POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Overgrowth>/Overgrowth.app${path}"
        )
    endfunction(CreateBundleDir)

    function(CopyFileToBundle start_path end_path)
        add_custom_command( TARGET Overgrowth POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "\"${start_path}\""
                "\"$<TARGET_FILE_DIR:Overgrowth>/Overgrowth.app/Contents/${end_path}\""
        )
    endfunction(CopyFileToBundle)

    function(CopyFolderToBundle start_path end_path)
        add_custom_command( TARGET Overgrowth POST_BUILD
            COMMAND cp -af
                "\"${start_path}\""
                "\"$<TARGET_FILE_DIR:Overgrowth>/Overgrowth.app/Contents/${end_path}\""
        )
    endfunction(CopyFolderToBundle)

    #add_custom_command( TARGET Overgrowth POST_BUILD
    #    COMMAND rm -R "\"$<TARGET_FILE_DIR:Overgrowth>/Overgrowth.app\""
    #)

    add_custom_command( TARGET Overgrowth POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory
            "$<TARGET_FILE_DIR:Overgrowth>/Overgrowth.app"
    )

    CreateBundleDir("")
    CreateBundleDir("/Contents")
    CreateBundleDir("/Contents/MacOS")
    CreateBundleDir("/Contents/Resources")
    CreateBundleDir("/Contents/Frameworks")
    CopyFileToBundle("$<TARGET_FILE:Overgrowth>" "MacOS/${OVERGROWTH_OUTPUT_NAME}")
    CopyFileToBundle("$<TARGET_FILE:OvergrowthWorker>" "MacOS/${OVERGROWTH_WORKER_OUTPUT_NAME}")
    CopyFileToBundle("${CMAKE_SOURCE_DIR}/lugaru2.icns" "Resources/lugaru2.icns")
    CopyFileToBundle("${CMAKE_SOURCE_DIR}/FinalInfo.plist" "Info.plist")
    CopyFileToBundle("${CMAKE_SOURCE_DIR}/steam_appid.txt" "MacOS/steam_appid.txt")

    IF(ENABLE_STEAMWORKS)
        CopyFileToBundle("${PLIBDIR}/steamworks_1.51/sdk/redistributable_bin/osx/libsteam_api.dylib" "MacOS/libsteam_api.dylib")
    ENDIF()

    #CopyFileToBundle("${PLIBDIR}/Telemetry2SDKMac_20P3/redist/libTelemetryMacX86.dylib" "MacOS/libTelemetryMacX86.dylib")
        
    CopyFileToBundle("${CMAKE_SOURCE_DIR}/steam_appid.txt" "Resources/steam_appid.txt")
    CopyFolderToBundle("${LIBDIR}/SDL2-2.0.20/SDL2.framework" "Frameworks/SDL2.framework")
    CopyFolderToBundle("${LIBDIR}/SDL2_net-2.0.1/SDL2_net.framework" "Frameworks/SDL2_net.framework")
ENDIF()


##POST BUILD PACKAGING


IF(DO_DIST_ARCHIVE)
    SET(DIST_ARCHIVE "${CMAKE_BINARY_DIR}/Overgrowth.tar" CACHE STRING "Name of packaged file containing game binaries")
    IF(LINUX)
        ADD_CUSTOM_TARGET( GenerateDistArchive ALL 
            COMMAND "ln" "-s" "\"${LIB_ROOT}/\"" "${LIB_SUFFIX}"
	    #Replace next command with this when renabling BREAKPAD support COMMAND "tar" "-chf" "\"${DIST_ARCHIVE}\"" "${OVERGROWTH_OUTPUT_NAME}${BIN_SUFFIX}.sym" "${OVERGROWTH_WORKER_OUTPUT_NAME}${BIN_SUFFIX}.sym" "${OVERGROWTH_OUTPUT_NAME}${BIN_SUFFIX}" "${OVERGROWTH_WORKER_OUTPUT_NAME}${BIN_SUFFIX}" "${LIB_SUFFIX}"
            COMMAND "tar" "-chf" "\"${DIST_ARCHIVE}\"" "${OVERGROWTH_OUTPUT_NAME}${BIN_SUFFIX}" "${OVERGROWTH_WORKER_OUTPUT_NAME}${BIN_SUFFIX}" "${LIB_SUFFIX}"
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            COMMAND "rm" "${LIB_SUFFIX}"
        )
    ENDIF()
    IF(APPLE)
        ADD_CUSTOM_TARGET( CreateDistArchive ALL 
            COMMAND "mkdir" "-p" "Package"
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        )
        ADD_CUSTOM_TARGET( CopyDistArchive ALL 
            COMMAND "cp" "-av" "\"$<TARGET_FILE_DIR:Overgrowth>/Overgrowth.app\"" "Package/"
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        )
        ADD_DEPENDENCIES( CopyDistArchive
            CreateDistArchive
        )
        ADD_CUSTOM_TARGET( GenerateDistArchive ALL 
            COMMAND "tar" "-cvhf" "\"${DIST_ARCHIVE}\"" "Overgrowth.app"
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/Package"
        )
        ADD_DEPENDENCIES( GenerateDistArchive
            CopyDistArchive
        )
        ADD_CUSTOM_TARGET( ClearDistArchive ALL 
            COMMAND "rm" "-rf" "Package/Overgrowth.app"
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        )
        ADD_DEPENDENCIES( ClearDistArchive
            GenerateDistArchive
        )
    ENDIF()
    IF(WIN32)
	    FIND_PROGRAM(SEVEN_ZIP NAMES 7z PATHS "C:/Program Files/7-Zip")
        ADD_CUSTOM_TARGET( GenerateDistArchive ALL 
            COMMAND ${SEVEN_ZIP} "a" "-ttar" "\"${DIST_ARCHIVE}\"" "./"
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/${WIN32_DIST_FOLDER}"
        )
    ENDIF()
    ADD_DEPENDENCIES( GenerateDistArchive
        Overgrowth
        OvergrowthWorker
    )
ENDIF()

# end of CMakeLists.txt ...
