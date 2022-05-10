//-----------------------------------------------------------------------------
//           Name: crn.h
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
// crn uses size_t without including headers that contains it on mac
#include <cstdlib>
#include <crn_mipmapped_texture.h>
#include <crn_texture_conversion.h>
#include <crn_buffer_stream.h>

// crnlib pulls in wingdi.h on Windows which defines a GetObject macro in some circumstances.
// This collides with GetObject functions in angelscript, so we undefine it here.
#ifdef GetObject
#undef GetObject
#endif
