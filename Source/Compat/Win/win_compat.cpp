//-----------------------------------------------------------------------------
//           Name: win_compat.cpp
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
#include <Compat/platform.h>
#if !PLATFORM_WINDOWS
#error Do not compile this.
#endif
#include "win_compat.h"

#include <Compat/compat.h>
#include <Compat/fileio.h>

#include <Internal/integer.h>
#include <Internal/error.h>
#include <Internal/filesystem.h>

#include <Logging/logdata.h>
#include <Memory/allocation.h>

#include <utf8/utf8.h>

#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#define NOMINMAX
#include <Windows.h>
#include <direct.h>
#include <tchar.h> 
#include <strsafe.h>
#include <conio.h>  

#include <cassert>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

/// here's where things start getting ugly, replacements for stuff windows doesn't have.
#include <ShlObj.h>

// X_OK should be execute bit, but non exists in io.h? Windows doesn't have execute rights?
#define X_OK 0x0
#define F_OK 0x0
#define W_OK 0x02
#define R_OK 0x04
#define RW_OK 0x06

using std::string;
using std::wstring;
using std::basic_string;
using std::vector;
using std::endl;

int UTF8Access(const string &path, int access_mode){
	return _waccess(UTF16fromUTF8(path).c_str(), access_mode);
} 

bool chdir(char *dir) {
	return (SetCurrentDirectoryW(UTF16fromUTF8(dir).c_str()) == TRUE)?true:false;
}

#define S_IRWXU 0x0000000

#if 1
#define dprintf printf
#else
static inline void dprintf(const char *fmt, ...) {}
#endif

char* strtok_r(
    char *str, 
    const char *delim, 
    char **nextp)
{
    char *ret;

    if (str == NULL)
    {
        str = *nextp;
    }

    str += strspn(str, delim);

    if (*str == '\0')
    {
        return NULL;
    }

    ret = str;

    str += strcspn(str, delim);

    if (*str)
    {
        *str++ = '\0';
    }

    *nextp = str;

    return ret;
}

string GetAbsPath(const char* full_rel_path)
{
	wstring w_full_rel_path = UTF16fromUTF8(full_rel_path);
	wchar_t w_full_path[PATH_MAX];
	_wfullpath(w_full_path, w_full_rel_path.c_str(), PATH_MAX);
	return UTF8fromUTF16(wstring(w_full_path));
}

// Get the Windows write directory 
// ("$VOL:/$USERNAME/Documents/Wolfire/Overgrowth")
int initWriteDir(char* writedir, int kPathBufferSize) {
    // Read documents path into UTF16 char buffer
    wchar_t writedir_utf16_buf[MAX_PATH];
    HRESULT result = SHGetFolderPathW(
        NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, 
        writedir_utf16_buf);
    if(result != S_OK){
        return 1;
    }

    /*wchar_t short_writedir_utf16_buf[MAX_PATH];
    DWORD err = GetShortPathNameW(writedir_utf16_buf, short_writedir_utf16_buf, MAX_PATH);
	
	if(err == 0) {
		LPVOID lpMsgBuf;
		LOGF << "Failed at retrieving short path for write dir" << endl;
		DWORD dw = GetLastError();
		FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
			(LPTSTR) &lpMsgBuf,
			0, NULL );

		DisplayError("Error init writedir", (const char*)lpMsgBuf, _ok, true);
		LOGF << lpMsgBuf << endl;

		LocalFree(lpMsgBuf);
		return 4;
	} else if( err > MAX_PATH ) {
		LOGF << "Short path of write dir could not fit in buffer" << endl;
		return 5;
	}

    if(!WideCharToMultiByte(CP_UTF8, 0, short_writedir_utf16_buf, -1, 
        writedir, kPathBufferSize, NULL, NULL))*/
    if(!WideCharToMultiByte(CP_UTF8, 0, writedir_utf16_buf, -1, 
        writedir, kPathBufferSize, NULL, NULL))
    {
        return 2;
    }    
    int len = strlen(writedir);
    // Replace backslashes with forward slashes
    for(int i=0; i<len; ++i){
        if(writedir[i] == '\\'){
            writedir[i] = '/';
        }
    }    
    // Append slash to the end if there isn't one already
    if(writedir[len-1] != '/'){
        if(len == kPathBufferSize){
            return 3;
        }
        strcat(writedir, "/");
    }
    // Add program identifier
    const char* to_append = "Wolfire/Overgrowth/";
    if(len + (int)strlen(to_append) >= kPathBufferSize){
        return 3;
    }
    strcat(writedir, to_append);
    return 0;
}

static char *findBinaryInPath(const char *bin, char *envr) {
    size_t alloc_size = 0;
    char *exe = NULL;
    char *start = envr;
    char *ptr;
    do {
        size_t size;
        ptr = strchr(start, ':');  // find next $PATH separator.
        if (ptr)
            *ptr = '\0';

        size = strlen(start) + strlen(bin) + 2;
        if (size > alloc_size) {
            char *x = (char *) realloc(exe, size);
            if (x == NULL) {
                if (exe != NULL) {
                    free(exe);
                }
                return NULL;
            }
            alloc_size = size;
            exe = x;
        }
        // build full binary path...
        strcpy_s(exe, size, start);
        if ((exe[0] == '\0') || (exe[strlen(exe) - 1] != '/'))
            strcat_s(exe, size, "/");
        strcat_s(exe, size, bin);
        if (UTF8Access(exe, X_OK) == 0) { // Exists as executable? We're done.
            strcpy_s(exe, size, start);  // i'm lazy. piss off.
            return(exe);
        } /* if */
        start = ptr + 1;  // start points to beginning of next element.
    } while (ptr != NULL);

    if (exe != NULL) {
        OG_FREE(exe);
    }
    return(NULL);  // doesn't exist in path.
}

// Set home directory so file paths work properly when running from shortcuts
void chdirToBasePath(char *argv0) {
    basic_string<uint32_t> path_utf32;
    {
        wchar_t path_utf16_buf[kPathSize];
        GetModuleFileNameW( NULL, path_utf16_buf, kPathSize);

        // Convert path from UTF16 to UTF32 by way of UTF8
        string path_utf8;
        utf8::utf16to8(path_utf16_buf, &path_utf16_buf[wcsnlen(path_utf16_buf, kPathSize)-1], back_inserter(path_utf8));
        utf8::utf8to32(path_utf8.begin(), path_utf8.end(), back_inserter(path_utf32));
    }
    
    // Eliminate everything after final '\\'
    path_utf32 = path_utf32.substr(0, path_utf32.rfind('\\'));
    
    wstring path_utf16;
    {
        // Convert path from UTF32 to UTF16 by way of UTF8
        string path_utf8;
        utf8::utf32to8(path_utf32.begin(), path_utf32.end(), back_inserter(path_utf8));
        utf8::utf8to16(path_utf8.begin(), path_utf8.end(), back_inserter(path_utf16));
    }
    
    _wchdir(path_utf16.c_str());
}

void SetWorkingDir(const char* path) {
    _wchdir(UTF16fromUTF8(path).c_str());
}

void caseCorrect(char* path) {
    return;
}

string caseCorrect(const string & path ) {
    return path;
}

static void caseCorrectFilename(char *path, bool inwritedir) {
    if (*path == '\0') {
        return;  // this is the root directory ("/"), so just return.
    }
    if (UTF8Access(path, F_OK) == 0) {
        return;  // fast path: it definitely exists in acceptable case.
    }
    char *base = strrchr(path, '/');
    if (base == path) {
	} else if (base == NULL) {
        base = path-1;
    } else {
        *base = '\0';
		CreateDirectoryW(UTF16fromUTF8(path).c_str(), NULL);
        *base = '/';
    }
    base++;
}

void ShortenWindowsPath(string &str) {
	wstring path_utf16_buf;
	wstring extra;
	utf8::utf8to16(str.begin(), str.end(), back_inserter(path_utf16_buf));
	struct _stat buf;
	while(_wstat( &path_utf16_buf[0], &buf )!=0){
		unsigned offset = path_utf16_buf.rfind('/');
		if(offset == string::npos){
			offset = path_utf16_buf.rfind('\\');
        }
        if(offset == string::npos){
            return;
        }
		extra = path_utf16_buf.substr(offset, path_utf16_buf.size()-offset) + extra;
        path_utf16_buf = path_utf16_buf.substr(0, offset);
	}
	wstring short_buf;
	DWORD len = GetShortPathNameW(&path_utf16_buf[0], NULL, 0);
	
    if(len == 0) {
		LPVOID lpMsgBuf;
		
		DWORD dw = GetLastError();
		FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
			(LPTSTR) &lpMsgBuf,
			0, NULL );

		DisplayError("Error init writedir", (const char*)lpMsgBuf, _ok, true);

		LocalFree(lpMsgBuf);
	}
	short_buf.resize(len);
	len = GetShortPathNameW(&path_utf16_buf[0], &short_buf[0], len);
	if(len == 0) {
		LPVOID lpMsgBuf;
		
		DWORD dw = GetLastError();
		FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
			(LPTSTR) &lpMsgBuf,
			0, NULL );

		DisplayError("Error init writedir", (const char*)lpMsgBuf, _ok, true);

		LocalFree(lpMsgBuf);
	}
	short_buf.resize(short_buf.size()-1);
	short_buf += extra;
	str.clear();
	utf8::utf16to8(&short_buf[0], &short_buf[wcsnlen(&short_buf[0], kPathSize)], back_inserter(str));
}

void WorkingDir( string *dir ) {
	wchar_t *buf = _wgetcwd(NULL, NULL);
	string path_utf8;
	utf8::utf16to8(buf, &buf[wcsnlen(buf, kPathSize)], back_inserter(path_utf8));
	OG_FREE(buf);
	*dir = path_utf8;
}

vector<string>& getSubdirectories( const char *basepath, vector<string>& mods  )
{
	wstring path_utf16_buf;
	string path(basepath);

	path += "\\*";

	utf8::utf8to16(path.begin(), path.end(), back_inserter(path_utf16_buf));

   WIN32_FIND_DATAW ffd;
   size_t length_of_arg;
   HANDLE hFind = INVALID_HANDLE_VALUE;
   DWORD dwError=0;
   
    // Check that the input path plus 1 is not longer than MAX_PATH.
   StringCchLengthW(path_utf16_buf.c_str(), MAX_PATH, &length_of_arg);

	if (length_of_arg > (MAX_PATH-1))
	{
		LOGE <<  "Directory path is too long." << endl;
	}
	else
	{
	// Find the first file in the directory.
		hFind = FindFirstFileW(path_utf16_buf.c_str(), &ffd);

		if (hFind == INVALID_HANDLE_VALUE) 
		{
			LOGE <<  "FindFirstFile failed " <<  GetLastError() << endl;
		} 
		else
		{
			do
			{
				if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					wstring output_utf16( ffd.cFileName );
					string output;
					utf8::utf16to8(output_utf16.begin(), output_utf16.end(), back_inserter(output));
					//printf("modpath:%s\n", output.c_str() );
					if( output != string(".") && output != string("..") )
					{
						mods.push_back(string(basepath) + "/" + output);
					}
				}
			}
			while (FindNextFileW(hFind, &ffd) != 0);
 
			dwError = GetLastError();
			if (dwError != ERROR_NO_MORE_FILES) 
			{
				LOGE << "Error gettings subdirs: " << dwError << endl;
			}
			FindClose(hFind);			
		}
	}
   return mods;
}

vector<string>& getDeepManifest( const char *basepath, const char* prefix, vector<string>& files )
{
	string s_prefix( prefix );
	wstring path_utf16_buf;
	string path(basepath);

	path += "\\*";

	utf8::utf8to16(path.begin(), path.end(), back_inserter(path_utf16_buf));

   WIN32_FIND_DATAW ffd;
   size_t length_of_arg;
   HANDLE hFind = INVALID_HANDLE_VALUE;
   DWORD dwError=0;
   
    // Check that the input path plus 1 is not longer than MAX_PATH.
   StringCchLengthW(path_utf16_buf.c_str(), MAX_PATH, &length_of_arg);

	if (length_of_arg > (MAX_PATH-1))
	{
		LOGE << "Directory path is too long." << endl;
	}
	else
	{
	// Find the first file in the directory.
		hFind = FindFirstFileW(path_utf16_buf.c_str(), &ffd);

		if (hFind == INVALID_HANDLE_VALUE) 
		{
			LOGE << "FindFirstFile failed " << GetLastError() << endl;
		} 
		else
		{
			do
			{
				
				wstring output_utf16( ffd.cFileName );
				string output;
				utf8::utf16to8(output_utf16.begin(), output_utf16.end(), back_inserter(output));

				string sub_prefix;

				if( s_prefix.length() == 0 )
				{
					sub_prefix = output;
				}
				else
				{
					sub_prefix = s_prefix + "/" + output;
				}

				if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					//printf("modpath:%s\n", output.c_str() );
					if( output != string(".") && output != string("..") )
					{
						string fullpath = string(basepath) + "/" + output;
						
						getDeepManifest( fullpath.c_str(), sub_prefix.c_str(), files );		
					}
				}
				else
				{
					files.push_back( sub_prefix );
				}
			}
			while (FindNextFileW(hFind, &ffd) != 0);
 
			dwError = GetLastError();
			if (dwError != ERROR_NO_MORE_FILES) 
			{
				LOGE << "Error gettings subdirs: " << dwError << endl;
			}
			FindClose(hFind);			
		}
	}
   return files;
}

bool fileExist( const char *path )
{
    return UTF8Access(string(path), F_OK) == 0;
}

bool fileReadable( const char* path )
{
    return UTF8Access(string(path), R_OK) == 0;
}

int os_copyfile( const char *source, const char *dest )
{
    return CopyFileW(UTF16fromUTF8(source).c_str(), UTF16fromUTF8(dest).c_str(), FALSE) == 0;
}

int os_movefile( const char *source, const char *dest )
{
    return MoveFileExW(UTF16fromUTF8(source).c_str(), UTF16fromUTF8(dest).c_str(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == 0;
}

int os_deletefile( const char *path )
{
    return _wremove(UTF16fromUTF8(path).c_str());
}

int os_createfile( const char *path )
{
    FILE* file = _wfopen(UTF16fromUTF8(path).c_str(), L"w");

    if(file) {
        fclose(file);
        return 0;
    } else {
        return 1;
    }
}

int os_fileexists( const char *path )
{
    return _waccess(UTF16fromUTF8(path).c_str(), F_OK);
}

string dumpIntoFile( const void* buf, size_t nbyte )
{
    LOGF << "dumpIntoFile PLACEHOLDER" << endl;
    exit(1);
}

bool isFile( const char* path )
{
    struct _stat buf;
    if(_wstat( UTF16fromUTF8(path).c_str(), &buf )==0){
        if(_S_IFREG & buf.st_mode){
            return true; // It is a regular file
        } else {
            return false; // It might be a directory or pipe
        }
    }  else {
        return false; //No such file
    }
}

bool checkFileAccess(const char* path)
{
    return _waccess(UTF16fromUTF8(path).c_str(), 04) != -1;
}

void createParentDirs(const char* abs_path) {
    char build_path[kPathSize] = {'\0'};
    for(int i=0; abs_path[i] != '\0'; ++i){
        if(abs_path[i] == '/' || abs_path[i] == '\\') {
            if(!CheckFileAccess(build_path)){
                CreateDirectoryW(UTF16fromUTF8(build_path).c_str(), NULL);
            }
        }
        build_path[i] = abs_path[i];
    }
}

bool areSame(const char* path1, const char* path2 )
{
	wstring w_path1 = UTF16fromUTF8(path1);
	wstring w_path2 = UTF16fromUTF8(path2);

	LPCWSTR szPath1 = w_path1.c_str();
	LPCWSTR szPath2 = w_path2.c_str();

    //Validate the input
    _ASSERT(szPath1 != NULL);
    _ASSERT(szPath2 != NULL);

    //Get file handles
    HANDLE handle1 = ::CreateFileW(szPath1, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); 
    HANDLE handle2 = ::CreateFileW(szPath2, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    bool bResult = false;

    //if we could open both paths...
    if (handle1 != INVALID_HANDLE_VALUE && handle2 != INVALID_HANDLE_VALUE)
    {
        BY_HANDLE_FILE_INFORMATION fileInfo1;
        BY_HANDLE_FILE_INFORMATION fileInfo2;
        if (::GetFileInformationByHandle(handle1, &fileInfo1) && ::GetFileInformationByHandle(handle2, &fileInfo2))
        {
            //the paths are the same if they refer to the same file (fileindex) on the same volume (volume serial number)
            bResult = fileInfo1.dwVolumeSerialNumber == fileInfo2.dwVolumeSerialNumber &&
                      fileInfo1.nFileIndexHigh == fileInfo2.nFileIndexHigh &&
                      fileInfo1.nFileIndexLow == fileInfo2.nFileIndexLow;
        }
    }

    //free the handles
    if (handle1 != INVALID_HANDLE_VALUE )
    {
        ::CloseHandle(handle1);
    }

    if (handle2 != INVALID_HANDLE_VALUE )
    {
        ::CloseHandle(handle2);
    }

    //return the result
    return bResult;
}
