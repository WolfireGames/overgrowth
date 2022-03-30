//-----------------------------------------------------------------------------
//           Name: vboenums.h
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
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

enum {
    kVBOElement = 1 << 0,
    kVBOFloat = 1 << 1,
    kVBOStatic = 1 << 2,
    kVBODynamic = 1 << 3,
    kVBOStream = 1 << 4,
	kVBOForceReBufferData = 1 << 5
};
