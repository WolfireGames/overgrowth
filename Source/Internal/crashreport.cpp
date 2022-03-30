//-----------------------------------------------------------------------------
//           Name: crashreport.cpp
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
#include "crashreport.h"

#include <Internal/filesystem.h>
#include <Internal/common.h>
#include <Internal/error.h>

#include <Utility/strings.h>
#include <Version/version.h>
#include <Logging/logdata.h>
#include <Compat/fileio.h>
#include <Scripting/angelscript/ascrashdump.h>

void DumpScenegraphState();

#if PLATFORM_LINUX
#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#if PLATFORM_MACOSX
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#endif

#if PLATFORM_WINDOWS
#include <tchar.h>
#include <strsafe.h>
#endif

#if BREAKPAD && PLATFORM_MACOSX
#include "client/mac/handler/exception_handler.h"
#endif
#if BREAKPAD && PLATFORM_LINUX
#include "client/linux/handler/exception_handler.h"
#endif
#if BREAKPAD && PLATFORM_WINDOWS
#include "client/windows/handler/exception_handler.h"
#endif

#include <sstream>


/* 
 * The following is the old windows minidump routine, currently replaced with breakpad
#if PLATFORM_WINDOWS
#include <windows.h>
#include <dbghelp.h>
#include <shellapi.h>
#include <shlobj.h>
#include <tchar.h>
#include <strsafe.h>
#include <string>
#include "tinyxml.h"
#include "Version/version.h"
#include "Internal/filesystem.h"
#include "Logging/logdata.h"

int GenerateDump(EXCEPTION_POINTERS* pExceptionPointers) {
    // Get temp directory path
    TCHAR szPath[MAX_PATH]; 
    DWORD dwBufferSize = MAX_PATH;
    GetTempPath( dwBufferSize, szPath );
    // Create a folder in the temp directory with the name of the application
    TCHAR szFileName[MAX_PATH]; 
    TCHAR szFileNameDir[MAX_PATH]; 
    LPTSTR szAppName = _T("Overgrowth");
    StringCchPrintf( szFileName, MAX_PATH, _T("%s%s"), szPath, szAppName );
    StringCchPrintf( szFileNameDir, MAX_PATH, _T("%s%s"), szPath, szAppName );
    CreateDirectory( szFileName, NULL );
    // Create an informative file name including the application name and date of error
    SYSTEMTIME stLocalTime;
    GetLocalTime( &stLocalTime );
    StringCchPrintf( szFileName, MAX_PATH, _T("%s%s\\%s-%04d%02d%02d-%02d%02d%02d-%ld-%ld.dmp"), 
		szPath, szAppName, TEXT(GetShortBuildTag().c_str()), 
        stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay, 
        stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond, 
        GetCurrentProcessId(), GetCurrentThreadId());
    // Open the file and write the minidump to it
    HANDLE hDumpFile = CreateFile(szFileName, GENERIC_READ|GENERIC_WRITE, 
        FILE_SHARE_WRITE|FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
    MINIDUMP_EXCEPTION_INFORMATION ExpParam;
    ExpParam.ThreadId = GetCurrentThreadId();
    ExpParam.ExceptionPointers = pExceptionPointers;
    ExpParam.ClientPointers = TRUE;
    BOOL bMiniDumpSuccessful = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), 
        hDumpFile, MiniDumpWithDataSegs, &ExpParam, NULL, NULL);
	// Copy logfile.txt to same location as minidump
	LogSystem::Flush();
	StringCchPrintf(szFileName, MAX_PATH, _T("%s%s\\logfile.txt"), szPath, szAppName);
	TCHAR szLogFileName[MAX_PATH];
	StringCchPrintf(szLogFileName, MAX_PATH, _T("%slogfile.txt"), GetWritePath(CoreGameModID).c_str());
	CopyFile(szLogFileName, szFileName, false);
    // Display error message with instructions   
    MessageBox(NULL,
            "A fatal error occured! Please send the most recent .dmp and logfile.txt files to bugs@wolfire.com, along with the what version of the game you were playing and a description of what exactly was happening in the game when this error occured.", 
            "Fatal Error", 
            MB_OK | 
            MB_ICONHAND | 
            MB_TOPMOST | 
            MB_SETFOREGROUND);
    // In Windows Explorer, open the folder containing the .dmp
    std::string nav_command = "explorer "+std::string(szFileNameDir);
    system(nav_command.c_str());
    return EXCEPTION_EXECUTE_HANDLER;
}
#elif defined(PLATFORM_LINUX) && defined(__GNUG__) //We use GNUG for the backtrace

#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <Version/version.h>
#include "unix_compat.h"

#include "Internal/filesystem.h"
#include "Logging/logdata.h"


const static bool do_coredump_on_crash = false;

//Coredump routine crashes on linux.
//General signal handler for anything that usually cause a coredump
static void segfault_coredump( int sig )
{
    char time_string[256];
    char final_folder[256];
    time_t t = time(NULL);
    struct tm *l_t = localtime(&t);

    //Reset meaning for later.
    signal( sig, SIG_DFL );

    char full_message[2048];
    snprintf( full_message, 2048, "A fatal error occured!" );

    LOGF << "Changing directory for core dump" << std::endl; //Change dir for the core dump
    chdir(GetWritePath(CoreGameModID).c_str());

    strftime( time_string, 256, "%y-%m-%d_%H-%M-%S_%z", l_t );
    snprintf( final_folder, 256, "crash_%s_%s_%s", GetBuildVersion(), GetBuildIDString(), time_string );
    mkdir( final_folder, S_IRWXU );
    chdir( final_folder );

    //Get a backtrace
    {
        void *array[10];
        size_t size;

        // get void*'s for all entries on the stack
        size = backtrace(array, 10);

        int backtrace_file = creat( "backtrace", S_IRUSR | S_IWUSR );
        // print out all the frames to stderr
        LOGF << "Error: signal " <<  sig << std::endl;
        backtrace_symbols_fd(array, size, backtrace_file);
    }

    snprintf( full_message, 2048, 
        "A fatal error occured! Please send the most recent crash folder \"%s%s\" to bugs@wolfire.com, along with a description of what exactly was happening in the game when this error occured.", 
        GetWritePath(CoreGameModID).c_str(), 
        final_folder );


    LOGF << "Game crashed, flushing the log systems." << std::endl;
    LogSystem::Flush();

    MessageBox(NULL,
            full_message,
            "Fatal Error", 
            MB_OK | 
            MB_ICONHAND | 
            MB_TOPMOST | 
            MB_SETFOREGROUND);
    

    //We resend ourselves the signal so the default handler will dump the core. and get the correct exit code.
    kill(getpid(),sig); 
}

int RunWithCrashReport( int argc, char* argv[], int (*func) (int, char*[])) {
    #if PLATFORM_WINDOWS && !defined(_DEBUG) && defined(_DEPLOY)
	
        __try
        {
            return func(argc, argv);
        }
        __except(GenerateDump(GetExceptionInformation()))
        {
            LogSystem::Flush();
            return 1;
        }
}
*/


void GetCrashFolder( char* destpath, size_t maxsize )
{
	char time_string[256];
#if PLATFORM_LINUX || PLATFORM_MACOSX
    time_t t = time(NULL);
    struct tm *l_t = localtime(&t);
	strftime( time_string, 256, "%y-%m-%d_%H-%M-%S", l_t );
#elif PLATFORM_WINDOWS
	SYSTEMTIME stLocalTime;
    GetLocalTime( &stLocalTime );
    FormatString( time_string, 256, "%04d-%02d-%02d_%02d-%02d-%02d", 
        stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay, 
		stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond );
#endif
	FormatString( destpath, maxsize, "crash_%s_%s_%s", GetBuildVersion(), GetBuildIDString(), time_string );
}

#if BREAKPAD
#if PLATFORM_LINUX
static bool dumpCallback(const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool succeeded) 
{ 
    char tempPath[kPathSize];
    char sourcePath[kPathSize];
	char destPath[kPathSize];
    
    char crashFolder[kPathSize];

    LOGF << "Game crashed, flushing the log systems. DumpFile: " << descriptor.path() << std::endl;
    DumpScenegraphState();
    DumpAngelscriptStates();
    LogSystem::Flush();

    GetCrashFolder( crashFolder, kPathSize );

    //Create crash folder.
    FormatString( destPath, kPathSize, "%s/%s", GetWritePath(CoreGameModID).c_str(), crashFolder );
    mkdir( destPath, S_IRUSR | S_IWUSR | S_IXUSR );

    //Get the name of the dump.
    FormatString( tempPath, kPathSize, "%s", descriptor.path() );
    char* last_component = tempPath;
    for( unsigned i = 0; i < strlen( tempPath ) - 1; i++ )
    {
        if( tempPath[i] == '/' )
            last_component = &tempPath[i+1];
    }

    //Copy the dump into the crash folder.
    FormatString( sourcePath, kPathSize, "%s", descriptor.path() );
    FormatString( destPath, kPathSize, "%s/%s/%s", GetWritePath(CoreGameModID).c_str(), crashFolder, last_component );

    LOGF << sourcePath << " " << destPath << std::endl;
    LogSystem::Flush();
    copyfile( sourcePath, destPath );

    GetLogfilePath(sourcePath,kPathSize);
    FormatString( destPath, kPathSize, "%s/%s/logfile.txt", GetWritePath(CoreGameModID).c_str(), crashFolder);
    copyfile( sourcePath, destPath );

    GetHWReportPath(sourcePath, kPathSize);
    FormatString( destPath, kPathSize, "%s/%s/hwreport.txt", GetWritePath(CoreGameModID).c_str(), crashFolder);
    copyfile( sourcePath, destPath );

    GetScenGraphDumpPath(sourcePath, kPathSize);
    FormatString( destPath, kPathSize, "%s/%s/scene_dump.txt", GetWritePath(CoreGameModID).c_str(), crashFolder);
    copyfile( sourcePath, destPath );

    GetASDumpPath(sourcePath, kPathSize);
    FormatString( destPath, kPathSize, "%s/%s/as_dump.txt", GetWritePath(CoreGameModID).c_str(), crashFolder);
    copyfile( sourcePath, destPath );

    FormatString( destPath, kPathSize, "%s/%s/version_info.txt", GetWritePath(CoreGameModID).c_str(), crashFolder);
    FILE* f = my_fopen(destPath, "a");

    if( f )
    {
		fwrite(GetBuildIDString(), strlen(GetBuildIDString()), 1,f );
		fwrite("\n", 1, 1, f );
		fwrite(GetPlatform(), strlen(GetPlatform()), 1,f );
		fwrite("\n", 1, 1, f );
		fwrite(GetArch(), strlen(GetArch()), 1,f );
		fwrite("\n", 1, 1, f );
		fwrite(GetBuildVersion(), strlen(GetBuildVersion()), 1, f );
		fwrite("\n", 1, 1, f );
		fwrite(GetBuildTimestamp(), strlen(GetBuildTimestamp()), 1, f );
		fwrite("\n", 1, 1, f );

        fclose(f);
    }

	 DisplayError(
            "Fatal Error", 
            "A fatal error occured! Please compress the latest crash folder into a .zip and send to bugs@wolfire.com, please include a description of what exactly was happening in the game when this error occured.",
            _ok);

    return succeeded; 
}
#elif PLATFORM_MACOSX

static bool minidumpCallback( const char *dump_dir,
                                    const char *minidump_id,
                                    void *context, 
                                    bool succeeded)
{
    LOGF << "Game crashed, flushing the log systems. DumpFile: " << dump_dir << " " << minidump_id << std::endl;
    DumpScenegraphState();
    DumpAngelscriptStates();
    LogSystem::Flush();
}

static bool filterCallback(void* context)
{
    return true;
}

#elif PLATFORM_WINDOWS
static bool dumpCallback(const wchar_t* dump_path,
                                   const wchar_t* minidump_id,
                                   void* context,
                                   EXCEPTION_POINTERS* exinfo,
                                   MDRawAssertionInfo* assertion,
                                   bool succeeded)
{
     char tempPath[kPathSize];
	 char destPath[kPathSize];

	 WCHAR szWritePath[kPathSize];
	 WCHAR szCrashFolder[kPathSize];
	 WCHAR szSource[kPathSize];
	 WCHAR szDest[kPathSize];

	 LOGF << "Game crashed, flushing the log systems." << std::endl;
     if( succeeded == false ) {
        LOGF << "Program failed at generating a minidump" << std::endl;
     }
     DumpScenegraphState();
     DumpAngelscriptStates();
     LogSystem::Flush();

	 //Convert write path to windows UTF16 format
	 GetWritePath(CoreGameModID, tempPath, kPathSize);
	 NormalizePathSeparators(tempPath); //Ensure write path has correct slashes
	 MultiByteToWideChar(CP_UTF8, 0, tempPath, -1, szWritePath, kPathSize);

	 //Convert crash folder name to windows UTF16 format
	 GetCrashFolder(tempPath,kPathSize);
	 MultiByteToWideChar(CP_UTF8, 0, tempPath, -1, szCrashFolder, kPathSize);

	 //Create the crash folder.
	 StringCchPrintfW( szDest , kPathSize, L"%s\\%s", szWritePath, szCrashFolder );
	 CreateDirectoryW( szDest, NULL );

	 //Copy the dump into the crash folder
	 StringCchPrintfW( szSource, kPathSize, L"%s\\%s.dmp", dump_path, minidump_id );
	 StringCchPrintfW( szDest, kPathSize, L"%s\\%s\\%s.dmp", szWritePath, szCrashFolder, minidump_id );	 
	 MoveFileW( szSource, szDest );

	 //Copy the logfile into the crash folder
	 GetLogfilePath( tempPath, kPathSize );
	 NormalizePathSeparators(tempPath);
	 MultiByteToWideChar(CP_UTF8, 0, tempPath, -1, szSource, kPathSize);
	 StringCchPrintfW( szDest, kPathSize, L"%s\\%s\\logfile.txt", szWritePath, szCrashFolder );
	 CopyFileW( szSource, szDest, FALSE );

	 //Copy the hwreport into the crash folder
	 GetHWReportPath( tempPath, kPathSize );
	 NormalizePathSeparators(tempPath);
	 MultiByteToWideChar(CP_UTF8, 0, tempPath, -1, szSource, kPathSize);
	 StringCchPrintfW( szDest, kPathSize, L"%s\\%s\\hwreport.txt", szWritePath, szCrashFolder );
	 CopyFileW( szSource, szDest, FALSE );

	 //Write version information to file
	 StringCchPrintfW( szDest, kPathSize, L"%s\\%s\\version_info.txt", szWritePath, szCrashFolder );
	 HANDLE versionfile = CreateFileW( szDest, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );

	 //Copy the scene_dump.txt into the crash folder
	 GetScenGraphDumpPath( tempPath, kPathSize );
	 NormalizePathSeparators(tempPath);
	 MultiByteToWideChar(CP_UTF8, 0, tempPath, -1, szSource, kPathSize);
	 StringCchPrintfW( szDest, kPathSize, L"%s\\%s\\scene_dump.txt", szWritePath, szCrashFolder );
	 CopyFileW( szSource, szDest, FALSE );

	 //Copy the as_dump.txt into the crash folder
	 GetASDumpPath( tempPath, kPathSize );
	 NormalizePathSeparators(tempPath);
	 MultiByteToWideChar(CP_UTF8, 0, tempPath, -1, szSource, kPathSize);
	 StringCchPrintfW( szDest, kPathSize, L"%s\\%s\\as_dump.txt", szWritePath, szCrashFolder );
	 CopyFileW( szSource, szDest, FALSE );

	 if( versionfile != INVALID_HANDLE_VALUE )
	 {
		DWORD os;
		WriteFile(versionfile, GetBuildIDString(), strlen(GetBuildIDString()), &os, NULL );
		WriteFile(versionfile, "\r\n", 2, &os, NULL );
		WriteFile(versionfile, GetPlatform(), strlen(GetPlatform()), &os, NULL );
		WriteFile(versionfile, "\r\n", 2, &os, NULL );
		WriteFile(versionfile, GetArch(), strlen(GetArch()), &os, NULL );
		WriteFile(versionfile, "\r\n", 2, &os, NULL );
		WriteFile(versionfile, GetBuildVersion(), strlen(GetBuildVersion()), &os, NULL );
		WriteFile(versionfile, "\r\n", 2, &os, NULL );
		WriteFile(versionfile, GetBuildTimestamp(), strlen(GetBuildTimestamp()), &os, NULL );
		WriteFile(versionfile, "\r\n", 2, &os, NULL );
		CloseHandle(versionfile);
	 }

	 DisplayError(
            "Fatal Error", 
            "A fatal error occured! Please compress the latest crash folder into a .zip and send to bugs@wolfire.com, please include a description of what exactly was happening in the game when this error occured.",
            _ok );

     // In Windows Explorer, open the folder containing the .dmp
	 GetWritePath(CoreGameModID, tempPath, kPathSize);
	 NormalizePathSeparators(tempPath);
	 FormatString( destPath, kPathSize, "explorer %s", tempPath );
     system(destPath);

	 return succeeded;
}

static bool filterCallback(void* context, EXCEPTION_POINTERS* exinfo,
                                 MDRawAssertionInfo* assertion)
{
	return true;
}
#endif
#endif

int RunWithCrashReport( int argc, char* argv[], int (*func) (int, char*[])) {
#if BREAKPAD
#if PLATFORM_LINUX
    google_breakpad::MinidumpDescriptor descriptor(P_tmpdir); 
    google_breakpad::ExceptionHandler eh(descriptor, NULL, dumpCallback, NULL, true, -1);
#elif PLATFORM_MACOSX
    //google_breakpad::ExceptionHandler eh( "/tmp/", filterCallback, minidumpCallback, NULL, true, NULL );
#elif PLATFORM_WINDOWS
	
	fprintf( stderr, "Registering things\n" );
	std::wstring dumpPathStr = L".\\";

	wchar_t dumpPath[MAX_PATH];
	if( GetTempPathW(MAX_PATH,dumpPath) )
		dumpPathStr = std::wstring(dumpPath);

	google_breakpad::ExceptionHandler *eh = new google_breakpad::ExceptionHandler(  
		dumpPathStr, 
		filterCallback, 
		dumpCallback, 
		0, 
		google_breakpad::ExceptionHandler::HANDLER_ALL, 
		MiniDumpNormal, 
		L"", 
		0 );
#endif
	//crash();
#endif
	return func(argc, argv);

#if BREAKPAD
#if PLATFORM_WINDOWS
	delete eh;
#endif
#endif
}
