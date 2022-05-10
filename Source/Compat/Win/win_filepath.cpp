//-----------------------------------------------------------------------------
//           Name: win_filepath.cpp
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

#include <Compat/compat.h>

#include <Compat/fileio.h>
#include <Internal/error.h>

#define NOMINMAX
#include <direct.h>
#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>

#include <cstdio>
#include <vector>
#include <string>
#include <cctype>

using std::equal;
using std::string;
using std::toupper;
using std::vector;

static const int kPathBufferSize = 1024;

static int getdir(string dir, vector<string>& files) {
    WIN32_FIND_DATA ffd;
    TCHAR szDir[MAX_PATH];
    HANDLE hFind = INVALID_HANDLE_VALUE;
    DWORD dwError = 0;

    // Prepare string for use with FindFile functions.  First, copy the
    // string to a buffer, then append '\*' to the directory name.

    StringCchCopy(szDir, MAX_PATH, dir.c_str());
    StringCchCat(szDir, MAX_PATH, TEXT("\\*"));

    // Find the first file in the directory.

    hFind = FindFirstFile(szDir, &ffd);

    if (INVALID_HANDLE_VALUE == hFind) {
        DisplayError("Error", "Getdir fail");
        return dwError;
    }

    // List all the files in the directory with some info about them.

    do {
        files.push_back(ffd.cFileName);
    } while (FindNextFile(hFind, &ffd) != 0);

    dwError = GetLastError();
    if (dwError != ERROR_NO_MORE_FILES) {
        DisplayError("Error", "Getdir fail 2");
    }

    FindClose(hFind);
    return dwError;
}

// Case insensitive equals compare
bool iequals(string& str1, string& str2) {
    return ((str1.size() == str2.size()) && equal(str1.begin(), str1.end(), str2.begin(), [](char& c1, char& c2) {
                return (c1 == c2 || toupper(c1) == toupper(c2));
            }));
}

string FindPath(string path) {
    //"Data/Skeletons/basic-attached-guard-joints.phxbn"
    string path_so_far = ".";
    string next_dir;
    size_t old_slash_pos = 0;
    size_t slash_pos = 0;
    bool case_problem = false;
    bool found_match;
    do {
        // Get next path segment
        slash_pos = path.find('/', old_slash_pos);
        next_dir = path.substr(old_slash_pos, slash_pos - old_slash_pos);
        old_slash_pos = slash_pos + 1;

        // Compare to current directory
        vector<string> files;
        getdir(path_so_far, files);
        found_match = false;
        for (unsigned i = 0; i < files.size(); i++) {
            if (iequals(files[i], next_dir)) {
                if (files[i] != next_dir) {
                    case_problem = true;
                }
                path_so_far += '/' + files[i];
                found_match = true;
            }
        }
        if (!found_match) {
            return path;
        }
    } while (slash_pos != string::npos);

    if (case_problem) {
        DisplayError("Warning", ("Case discrepency:\nSearched for \n\"./" + path + "\"\nFound \n\"" + path_so_far + "\"").c_str(), _ok_cancel, false);
    }

    return path_so_far;
}

string AbsPathFromRel(string path) {
    string wdir;
    WorkingDir(&wdir);
    return wdir + path;
}

void GetCaseCorrectPath(const char* input_path, char* correct_path) {
    // Correct case by converting to short path and back to long
    char short_path[kPathBufferSize];
    GetShortPathName(input_path, short_path, kPathBufferSize);
    GetLongPathName(short_path, correct_path, kPathBufferSize);
}
