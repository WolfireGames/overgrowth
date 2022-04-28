//-----------------------------------------------------------------------------
//           Name: unix_filepath.cpp
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
#if !PLATFORM_UNIX
#error Do not compile this.
#endif

#include <Compat/filepath.h>
#include <Compat/compat.h>
#include <Global/global_config.h>
#include <Utility/strings.h>

#include <unistd.h>
#include <cstdlib>
#include <string>

std::string FindPath(std::string path) {
    return path;
}

std::string AbsPathFromRel(std::string path){
		const int max = 512;
		char curr[max];
		getcwd(curr, max);
		return std::string(curr)+path;
}

void GetCaseCorrectPath(const char* input_path, char* correct_path) {
    // Correct case using realpath() and cut off working directory
    int num_dirs = CountCharsInString(input_path, '/');
    char path[kPathSize];
    realpath(input_path, path);
    char *cut_path = &path[FindNthCharFromBack(path,'/',num_dirs+1)+1];
    strcpy(correct_path, cut_path);
}
