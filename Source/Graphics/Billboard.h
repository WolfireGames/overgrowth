//-----------------------------------------------------------------------------
//           Name: Billboard.h
//      Developer: Wolfire Games LLC
//         Author: Phillip Isola
//    Description: Images that rotate to face the camera
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

#include <Math/vec3.h>
#include <Math/vec4.h>

#include <Asset/Asset/texture.h>
#include <Graphics/glstate.h>

#include <string>

void DrawBillboard(const TextureRef& texture_ref, const vec3& pos, float scale, const vec4& color, AlphaMode alpha_mode);
