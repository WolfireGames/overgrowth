//-----------------------------------------------------------------------------
//           Name: platformsetup.cpp
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
#include "platformsetup.h"

#include <Internal/filesystem.h>
#include <Internal/common.h>
#include <Internal/error.h>

#include <Compat/filepath.h>
#include <Logging/logdata.h>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#include <csignal>
static BOOL WINAPI ConsoleHandlerRoutine(DWORD dwCtrlType) {
    switch(dwCtrlType){
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT: 
        raise(SIGTERM);
        break;
    }
    return FALSE;
}
#endif

#if PLATFORM_UNIX
#include <cstring>
#include <libgen.h>
#endif

// Set up working directory, file paths, Windows console
void SetUpEnvironment(char* program_path, const char* overloaded_write_dir, const char* overloaded_working_dir) {
	#if PLATFORM_UNIX
        // Force numeric locale so numbers parse correctly
        setenv("LC_NUMERIC", "en_US.utf8", 1);
        if( strlen(overloaded_working_dir) > 0 ) {
            chdir(overloaded_working_dir);
        }
        char* dirn = dirname( program_path );
        AddPath(dirn, kDataPaths);
        char cwd[kPathSize];
        getcwd(cwd, kPathSize);
        int len = strlen(cwd);
        if(len < kPathSize - 1){
            cwd[len] = '/';
            cwd[len+1] = '\0';
        }

        if( false == areSame( cwd, dirn ) )
        {
            AddPath(cwd, kDataPaths);
        }
        else
        {
            LOGI << "Not adding " << cwd << " to FindPath list as it's the same as ./" << std::endl;
        }
        char write_dir[kPathSize];
        initWriteDir(write_dir, kPathSize);

        if( strcmp(overloaded_write_dir,"") != 0 )
        {
            AddPath(overloaded_write_dir, kWriteDir);
        }
        else
        {
            AddPath(write_dir, kWriteDir);
        }

		FreeImage_Initialise();
    #elif defined(_WIN32)
		std::string cwd;
        if( strlen(overloaded_working_dir) > 0 ) {
            SetWorkingDir(overloaded_working_dir);
            WorkingDir(&cwd);
            AddPath(ApplicationPathSeparators(cwd).c_str(), kDataPaths);
        } else {
            //TODO: Consider if we actually want to include the default working dir as a data path, it seems unecessary
            WorkingDir(&cwd);
            AddPath(ApplicationPathSeparators(cwd).c_str(), kDataPaths);
            #ifdef _DEPLOY	
                char path[] = "";
                chdirToBasePath(path); // Set working directory to .exe location (so shortcuts work)
                WorkingDir(&cwd);
                AddPath(ApplicationPathSeparators(cwd).c_str(), kDataPaths);
            #endif
        }
        char write_path[kPathSize];
        int err = initWriteDir(write_path, kPathSize);
        if(err != 0){
            FatalError("Error", win_compat_err_str[err]);
        }

        if( strcmp(overloaded_write_dir,"") != 0 )
        {
            AddPath(overloaded_write_dir, kWriteDir);
        }
        else
        {
            AddPath(write_path, kWriteDir);
        }

        { // Add working directory to data paths
            wchar_t long_utf16_buf[kPathSize];
            if(!_wgetcwd(long_utf16_buf, kPathSize)){
                DisplayError("Error", "_wgetcwd failed in SetUpEnvironment");
            }
            char short_utf8_buf[kPathSize];
            if(!WideCharToMultiByte(CP_UTF8, 0, long_utf16_buf, -1, 
                short_utf8_buf, kPathSize, NULL, NULL))
            {
                DisplayError("Error", "WideCharToMultiByte failed in SetUpEnvironment");                
            }    
            for(int i=0; i<kPathSize; ++i){
                if(short_utf8_buf[i] == '\\'){
                    short_utf8_buf[i] = '/';
                }
                if(short_utf8_buf[i] == '\0' && i<kPathSize-1){
                    short_utf8_buf[i] = '/';
                    short_utf8_buf[i+1] = '\0';
                    break;
                }
            }
            AddPath(short_utf8_buf, kDataPaths);
        }
        // Set up Windows console
		#if OPEN_WIN32_CONSOLE
			AllocConsole();
			freopen("CONIN$","rb",stdin);
			freopen("CONOUT$","wb",stdout);
			freopen("CONOUT$","wb",stderr);		
			SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE);
		#endif
	#endif
             
    CreateParentDirs((std::string(GetWritePath(CoreGameModID).c_str())+"placeholder").c_str());

    string working_dir;
    WorkingDir(&working_dir);
    working_dir += "/Auxiliary";

    AddPath(working_dir.c_str(), kDataPaths);

#ifdef AUX_DATA
    AddPath(AUX_DATA, kDataPaths);
#endif

    if(CheckWritePermissions(write_path) != 0) {
        #ifdef _WIN32
            FatalError("Error", "Couldn't write to \"%s\", make sure write permissions are enabled for that folder. If you are using Windows 10, make sure Overgrowth is added to the Controlled Folder Access whitelist", write_path);
        #else
            FatalError("Error", "Couldn't write to \"%s\", make sure write permissions are enabled for that folder", write_path);
        #endif
    }
}

void DisposeEnvironment(){
	#if PLATFORM_UNIX
		FreeImage_DeInitialise();
	#elif defined(_WIN32)
		FreeConsole();
	#endif
}
