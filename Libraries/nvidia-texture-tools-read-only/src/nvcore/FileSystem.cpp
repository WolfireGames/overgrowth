// This code is in the public domain -- castano@gmail.com

#include "FileSystem.h"
#include <nvcore/nvcore.h>

#if NV_OS_WIN32
//#include <shlwapi.h> // PathFileExists
#include <windows.h> // GetFileAttributes
#include <direct.h> // _mkdir
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

using namespace nv;


bool FileSystem::exists(const char * path)
{
#if NV_OS_UNIX
	return access(path, F_OK|R_OK) == 0;
	//struct stat buf;
	//return stat(path, &buf) == 0;
#elif NV_OS_WIN32
    // PathFileExists requires linking to shlwapi.lib
    //return PathFileExists(path) != 0;
    return GetFileAttributes(path) != 0xFFFFFFFF;
#else
	if (FILE * fp = fopen(path, "r"))
	{
		fclose(fp);
		return true;
	}
	return false;
#endif
}

bool FileSystem::createDirectory(const char * path)
{
#if NV_OS_WIN32
    return _mkdir(path) != -1;
#else
    return mkdir(path, 0777) != -1;
#endif
}

