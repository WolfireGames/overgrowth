//-----------------------------------------------------------------------------
//           Name: error.h
//      Developer: Wolfire Games LLC
//         Author: David Rosen
//    Description: This is a simple wrapper for displaying error messages
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

#include <Utility/compiler_macros.h>
#include <Internal/error_response.h>

#include <string>

/**
 *Displays an error message.
 *@param title The title of the message box.
 *@param contents The contents of the message box.
 *@param type Which buttons are available
 *@param allow_repetition If false, skips error if exact content has already
 *                         been displayed
 */
ErrorResponse DisplayError(const char* title,
                           const char* contents,
                           ErrorType type = _ok_cancel,
                           bool allow_repetition = true);

ErrorResponse DisplayFormatError(ErrorType type,
                                 bool allow_repetition,
                                 const char* title,
                                 const char* fmtcontents,
                                 ...);

// Display a message with an OK button only
void DisplayMessage(const char* title,
                    const char* contents);
void DisplayFormatMessage(const char* title,
                          const char* fmtcontents,
                          ...);

ErrorResponse DisplayLastQueuedError();

/**
 *Displays an error message and then exits the program.
 *@param title The title of the message box.
 *@param contents The contents of the message box.
 */
void FatalError(const char* title, const char* fmt, ...) __attribute__((noreturn));

// bool Query(std::string title, std::string contents);

// std::string QueryString(std::string title, std::string contents);
