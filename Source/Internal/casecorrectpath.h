//-----------------------------------------------------------------------------
//           Name: casecorrectpath.h
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

#include <string>

// Takes an input_path that is known to exist // e.g. "DaTa/fILE.zIP"
// Fills correct_case with the true file path case // e.g. "Data/File.zip"
// Returns whether or not input_path == correct_case
bool IsPathCaseCorrect(const std::string& input_path, std::string *correct_case);
