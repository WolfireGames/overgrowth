//-----------------------------------------------------------------------------
//           Name: im_support.cpp
//      Developer: Wolfire Games LLC
//    Description: A set of support function and defines for putting together
//                 `ad hoc'/overlay GUIs
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
#include "im_support.h"

const float UNDEFINEDSIZE = -1.0;
const int UNDEFINEDSIZEI = -1;

/*******************************************************************************************/
/**
 * @brief  Helper class to derive and contain the scaling factors and offsets between GUI space
 *         and screen space
 *
 */

/*******************************************************************************************/
/**
 * @brief  Constructor, for constructing
 *
 */
ScreenMetrics::ScreenMetrics() : mainSize(2560, 1440),
                                 fourThree(2560, 1920),
                                 GUISpace(2560, 1440),
                                 renderOffset(0, 0) {
    computeFactors();
}

float ScreenMetrics::getScreenWidth() {
    return (float)Graphics::Instance()->window_dims[0];
}

float ScreenMetrics::getScreenHeight() {
    return (float)Graphics::Instance()->window_dims[1];
}

/*******************************************************************************************/
/**
 * @brief  Gets the current screen dimensions
 *
 * @returns 2d vector of the screen dimenions
 *
 */
vec2 ScreenMetrics::getMetrics() {
    computeFactors();

    return screenSize;
}

/*******************************************************************************************/
/**
 * @brief  Checks to see if the resolution has changed and if so rederive the values
 *
 * @returns true if the resolution has changed, false otherwise
 *
 */
bool ScreenMetrics::checkMetrics(vec2& metrics) {
    if (screenSize.x() != getScreenWidth() || screenSize.y() != getScreenHeight()) {
        computeFactors();
    }

    if (screenSize.x() != metrics.x() || screenSize.y() != metrics.y()) {
        metrics = screenSize;
        return true;
    } else {
        return false;
    }
}

/*******************************************************************************************/
/**
 * @brief  Compute various values this class is responsible for
 *
 */
void ScreenMetrics::computeFactors() {
    const float max_aspect_ratio = 16.0f / 9.0f;
    float aspect_ratio = getScreenWidth() / getScreenHeight();

    if (aspect_ratio > max_aspect_ratio) {
        aspect_ratio = max_aspect_ratio;
    }

    GUISpace.x() = 2560;
    GUISpace.y() = 2560 * (1.0f / aspect_ratio);

    screenSize = vec2(getScreenHeight() * aspect_ratio, getScreenHeight());

    GUItoScreenXScale = (screenSize.x() / GUISpace.x());
    GUItoScreenYScale = (screenSize.y() / GUISpace.y());

    renderOffset = vec2((getScreenWidth() - getScreenHeight() * aspect_ratio) / 2.0f, 0.0f);
}

vec2 ScreenMetrics::GUIToScreen(const vec2 pos) {
    return vec2((pos.x() * GUItoScreenXScale),
                (pos.y() * GUItoScreenYScale));
}

ScreenMetrics screenMetrics;  // Dimension and translation information for the screen

// Scooping this from the Angelscript source to maintain compatibilty and avoid having to expose it
int64_t parseInt(const std::string val, unsigned int base) {
    // Only accept base 10 and 16
    if (base != 10 && base != 16) {
        return 0;
    }

    const char* end = &val[0];

    // Determine the sign
    bool sign = false;
    if (*end == '-') {
        sign = true;
        end++;
    } else if (*end == '+')
        end++;

    int64_t res = 0;
    if (base == 10) {
        while (*end >= '0' && *end <= '9') {
            res *= 10;
            res += *end++ - '0';
        }
    } else if (base == 16) {
        while ((*end >= '0' && *end <= '9') ||
               (*end >= 'a' && *end <= 'f') ||
               (*end >= 'A' && *end <= 'F')) {
            res *= 16;
            if (*end >= '0' && *end <= '9')
                res += *end++ - '0';
            else if (*end >= 'a' && *end <= 'f')
                res += *end++ - 'a' + 10;
            else if (*end >= 'A' && *end <= 'F')
                res += *end++ - 'A' + 10;
        }
    }

    if (sign)
        res = -res;

    return res;
}

vec4 HexColor(std::string const& hex) {
    if (hex.substr(0, 1) == "#" && hex.length() == 7) {
        float c1 = (float)parseInt(hex.substr(1, 2), 16);
        float c2 = (float)parseInt(hex.substr(3, 2), 16);
        float c3 = (float)parseInt(hex.substr(5, 2), 16);

        return vec4(c1 / 255.0f, c2 / 255.0f, c3 / 255.0f, 1.0f);
    }
    if (hex.substr(0, 1) == "#" && hex.length() == 4) {
        float c1 = (float)parseInt(hex.substr(1, 1), 16);
        float c2 = (float)parseInt(hex.substr(2, 1), 16);
        float c3 = (float)parseInt(hex.substr(3, 1), 16);

        return vec4(c1 / 16.0f, c2 / 16.0f, c3 / 16.0f, 1.0f);
    } else {
        return vec4(1.0f);
    }
}

void IMDisplayError(std::string const& errorTitle, std::string const& errorMessage) {
    DisplayError(errorTitle.c_str(), errorMessage.c_str(), _ok);
}

void IMDisplayError(std::string const& errorMessage) {
    IMDisplayError("GUI Error", errorMessage.c_str());
}

// Register a new ref counted object
void IMReferenceCountTracker::addRefCountObject(std::string const& className) {
    totalRefCountObjs++;

    std::map<std::string, int>::iterator findIt = typeCounts.find(className);

    if (findIt == typeCounts.end()) {
        typeCounts[className] = 1;
    } else {
        typeCounts[className] += 1;
    }
}

// Register the destruction of a ref counted object
void IMReferenceCountTracker::removeRefCountObject(std::string const& className) {
    totalRefCountObjs--;

    std::map<std::string, int>::iterator findIt = typeCounts.find(className);

    if (findIt == typeCounts.end()) {
        LOGI << "removeRefCountObject with an object never registered" << std::endl;
    } else {
        typeCounts[className] -= 1;
    }
}

// Display a message if there are still live objects
void IMReferenceCountTracker::logSanityCheck() {
    if (totalRefCountObjs == 0) {
        LOGI << "No live IMGUI elements" << std::endl;
    } else {
        LOGI << "Error: There still " << totalRefCountObjs << " LIVE IMGUI elements" << std::endl;

        for (auto& typeCount : typeCounts) {
            if (typeCount.second != 0) {
                LOGI << typeCount.first << ": " << typeCount.second << std::endl;
            }
        }
    }
}

IMReferenceCountTracker::~IMReferenceCountTracker() {
    logSanityCheck();
}

IMReferenceCountTracker IMrefCountTracker;
