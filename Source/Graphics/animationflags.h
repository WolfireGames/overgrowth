//-----------------------------------------------------------------------------
//           Name: animationflags.h
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

enum AnimationClientFlags{
    _ANM_MOBILE = (1<<0),
    _ANM_MIRRORED = (1<<1),
    _ANM_SWAP = (1<<2),
    _ANM_SUPER_MOBILE = (1<<3),
    _ANM_FROM_START = (1<<4)
};
