//-----------------------------------------------------------------------------
//           Name: filesystem.h
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

#include <Internal/modid.h>
#include <Internal/path.h>

#include <Global/global_config.h>

#include <string>
#include <vector>

// TODO: On Windows these should be DOS-style short paths for UTF compatibility
struct Paths {
    Paths();
    static const int kMaxPaths = 256;
    char paths[kMaxPaths][kPathSize];
    ModID mod_path_ids[kMaxPaths];
    int num_paths;
    int AddPath(const char* path);
    bool AddModPath(const char* path, ModID modid);
    void RemovePath(const char* path);
};

typedef int PathFlagsBitfield;

Path FindImagePath(const char* path, PathFlagsBitfield flags = kAnyPath, bool is_necessary = true);
Path FindFilePath(const std::string& path, PathFlagsBitfield flags = kAnyPath, bool is_necessary = true);
Path FindFilePath(const char* path, PathFlagsBitfield flags = kAnyPath, bool is_necessary = true);

int FindImagePath(const char* path, char* buf, int buf_size, PathFlagsBitfield flags, bool is_necessary = true, PathFlags* resulting_path = NULL, bool allow_crn = true, bool allow_dds = true, ModID* modsource = NULL);
int FindFilePath(const char* path, char* buf, int buf_size, PathFlagsBitfield flags, bool is_necessary = true, PathFlags* resulting_path = NULL, ModID* modsource = NULL);
int FindFilePaths(const char* path, char* bufs, int buf_size, int num_bufs, PathFlagsBitfield flags, bool is_necessary, PathFlags* resulting_paths, ModID* modsourceids);
void AddPath(const char* path, PathFlags type);
bool AddModPath(const char* path, ModID modid);
void RemovePath(const char* path, PathFlags type);

bool CheckFileAccess(const char* path);
bool FileExists(const Path& path);
bool FileExists(const char* path, PathFlagsBitfield flags);
bool FileExists(const std::string& path, PathFlagsBitfield flags);
void CreateDirsFromPath(const std::string& base, const std::string& path);
void CreateParentDirs(const char* abs_path);
void CreateParentDirs(const std::string& abs_path);
void GenerateManifest(const char* path, std::vector<std::string>& files);

const std::string GetPathFlagsStr(PathFlagsBitfield f);

// Append file to path, adding any missing delimiters.
void AssemblePath(const char* first, const char* second, char* out, size_t outsize);
std::string AssemblePath(const char* first, const char* second);
std::string AssemblePath(const std::string& first, const std::string& second);
std::string AssemblePath(const std::string& first, const std::string& second, const std::string& third);

const char* GetDataPath(int i);

std::string GetWritePath(const ModID& id);
void GetWritePath(const ModID& id, char* dest, size_t len);

std::string GetConfigPath();
void GetConfigPath(char* dest, size_t len);

std::string GetLogfilePath();
void GetLogfilePath(char* dest, size_t len);

void GetScenGraphDumpPath(char* buf, size_t size);

void GetASDumpPath(char* buf, size_t size);

std::string GetHWReportPath();
void GetHWReportPath(char* dest, size_t len);

std::string GetVersionXMLPath();
void GetVersionXMLPath(char* dest, size_t len);

// Will guess mime type based on string, very basic
std::string GuessMime(std::string path);

std::vector<unsigned char> readFile(const char* filename);

std::string StripDriveletter(std::string path);

// Split a fully qualified filename into path and filename components
//  if path seperator is not found -- assumes all filename
std::pair<std::string, std::string> SplitPathFileName(char* fullPath);
std::pair<std::string, std::string> SplitPathFileName(std::string const& fullPath);

extern Paths vanilla_data_paths;
extern Paths mod_paths;
extern char write_path[kPathSize];

int CheckWritePermissions(const char* dir);

int copyfile(const std::string& source, const std::string& dest);
int copyfile(const char* source, const char* dest);

// This would be called DeleteFile, but that is defined in Windows. The rest are just for consistency
// Generally, using Path is preferred, but there are times when it is not possible to create a Path
int movefile(const char* source, const char* dest);
int deletefile(const char* filename);
int createfile(const char* filename);
int fileexists(const char* filename);

std::string DumpIntoFile(const void* buf, size_t nbyte);

std::string CaseCorrect(const std::string& string);

// This function will convert all backslashes to frontslashes which is the games internal
// path separator.

std::string ApplicationPathSeparators(const std::string& string);
void ApplicationPathSeparators(char* string);

// This function will convert either slashes or backslashes to the pathseparator used by the system
//  Unix-likes: slash
//  Windows: backslash
//  This function is used before paths are sent to system functions.
std::string NormalizePathSeparators(const std::string& string);
void NormalizePathSeparators(char* string);

bool AreSame(const char* path1, const char* path2);

std::string FindShortestPath(const std::string& string);
Path FindShortestPath2(const std::string& string);

void ClearCache(bool dry_run);

std::string GetMorphPath(const std::string& base_model, const std::string& morph);

bool IsFile(Path& path);
bool IsImageFile(Path& path);

/*
 * This function will remove duplicate forward slashes in strings and convert it to an internal application format.
 */
std::string SanitizePath(const char* path);
std::string SanitizePath(const std::string& path);
bool IsPathSane(const char* path);

std::string GenerateParallelPath(const char* base, const char* target, const char* postfix, Path asset);

std::string RemoveFileEnding(std::string in);

uint64_t GetFileSize(const Path& file);

void CreateBackup(const char* path, int max_backup_count = 10);
