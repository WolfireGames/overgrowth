//-----------------------------------------------------------------------------
//           Name: compat.h
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
#pragma once

#include <vector>
#include <string>

std::string GetAbsPath(const char* full_rel_path);
int initWriteDir(char* writedir, int kPathBufferSize);
void chdirToBasePath(char *argv0);
void WorkingDir(std::string *dir);
void ShortenWindowsPath(std::string &str);
void caseCorrect(char* path);
std::string caseCorrect(const std::string & path );
std::vector<std::string>& getSubdirectories( const char *basepath, std::vector<std::string>& mods  );
std::vector<std::string>& getDeepManifest( const char *basepath, const char* prefix, std::vector<std::string>& files );
bool fileExist( const char *path );
bool fileReadable( const char *path );

int os_copyfile( const char *source, const char *dest );
int os_movefile( const char *source, const char *dest );
int os_deletefile( const char *path );
int os_createfile( const char *path );
int os_fileexists( const char *path );

std::string dumpIntoFile( const void* buf, size_t nbyte );

bool isFile( const char* path );

bool checkFileAccess(const char* path);
void createParentDirs(const char* abs_path);
bool areSame(const char* path1, const char* path2 );

#include <Compat/platform.h>
#if PLATFORM_UNIX == 1  //This is shared on mac and linux.
#include <Compat/UNIX/unix_compat.h>
#endif

#if PLATFORM_LINUX == 1
#include <Compat/Linux/linux_compat.h>
#endif

#if PLATFORM_MACOSX == 1
#include <Compat/Mac/mac_compat.h>
#endif

#if PLATFORM_WINDOWS == 1
#include <Compat/Win/win_compat.h>
#endif
