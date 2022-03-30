//-----------------------------------------------------------------------------
//           Name: skyinfo.cpp
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
#include "skyinfo.h"

#include <Internal/levelxml.h>
#include <Internal/comma_separated_list.h>
#include <Internal/filesystem.h>
#include <Internal/returnpathutil.h>

#include <Asset/Asset/material.h>
#include <Logging/logdata.h>
#include <Game/detailobjectlayer.h>

#include <tinyxml.h>

void SkyInfo::Print()
{
    LOGI << "DomeTexture: " << dome_texture_path << std::endl;
    LOGI << "Sun angular rad: " << sun_angular_rad << std::endl;
    LOGI << "Sun color angle: " << sun_color_angle << std::endl;
    LOGI << "Ray to sun: " << ray_to_sun << std::endl;
}

void SkyInfo::SetDefaults() {
    dome_texture_path.clear();
}
