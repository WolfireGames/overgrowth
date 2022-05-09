//-----------------------------------------------------------------------------
//           Name: detect_settings.cpp
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
#include "detect_settings.h"

#include <Internal/hardware_specs.h>
#include <Internal/config.h>

#include <Graphics/graphics.h>
#include <Logging/logdata.h>
#include <Main/engine.h>

extern bool g_no_reflection_capture;
extern bool g_s3tc_dxt5_support;

void DetectAndSetOpenGLFeatureRestrictions() {
    if (HasHardwareS3TCSupport()) {
        LOGI << "Verified support for S3TC DXT5 compressed textures" << std::endl;
        g_s3tc_dxt5_support = true;
    } else {
        LOGI << "Could not Verify support for S3TC DXT5 compressed textures, disabling compressed texture loads." << std::endl;
        g_s3tc_dxt5_support = false;
    }
}

void DetectAndSetSettings() {
    LOGI << "Detecting hardware and setting graphics settings to match hardware" << std::endl;
    std::map<std::string, int> gfx_int = GetHardwareLimitationsInt();
    // Disable reflections if we don't have enough image units to allow it to fit.
    if (gfx_int["GL_MAX_TEXTURE_IMAGE_UNITS"] <= 16) {
        g_no_reflection_capture = true;
        config.GetRef("no_reflection_capture") = true;
        LOGI << "Limited texture samplers, disabling reflection capture" << std::endl;
    } else {
        LOGI << "Sufficient amount of samplers to support all features." << std::endl;
    }
}
