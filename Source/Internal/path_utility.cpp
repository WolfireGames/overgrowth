//-----------------------------------------------------------------------------
//           Name: pathutility.cpp
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
#include "path_utility.h"

#ifdef WIN32
#define NOMINMAX
#include <windows.h>
#include <direct.h>
#endif

std::string pathUtility::localPathToGlobal(const std::string &path)
{
    return localPathToGlobal(path.c_str());
}

std::string pathUtility::localPathToGlobal(const char *path)
{
#ifdef _DEPLOY
#ifdef WIN32    
     char filepath[4096];
    GetModuleFileName( NULL, filepath, 4096);
    std::string dirpath = filepath;

    dirpath = dirpath.substr(0, dirpath.rfind("\\"));
    dirpath += "\\";
    dirpath += path;
    
    return dirpath;
#endif
#endif

    return std::string(path);
}

void pathUtility::changeWorkingDirToAppDir()
{
#ifdef _DEPLOY
#ifdef WIN32    
     char filepath[4096];
    GetModuleFileName( NULL, filepath, 4096);
    std::string dirpath = filepath;

    _chdir(dirpath.substr(0, dirpath.rfind("\\")).c_str());

#endif
#endif
}
