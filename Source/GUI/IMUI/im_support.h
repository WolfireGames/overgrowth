//-----------------------------------------------------------------------------
//           Name: im_support.h
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
#pragma once

#include <Math/vec2.h>
#include <Math/vec4.h>

#include <Internal/error.h>

#include <GUI/IMUI/imui.h>

#include <Graphics/graphics.h>

#include <string>
#include <map>


// All coordinates are specified in terms of a space 2560 x 1440
// (at the moment -- this assumes 16:9 ratio -- other ratios are a TODO)
// when rendering to the screen the projection is done automatically
// coordinates start at the top left of the screen 

extern const float UNDEFINEDSIZE;
extern const int UNDEFINEDSIZEI;

/*******************************************************************************************/
/**
 * @brief  Helper class to derive and contain the scaling factors and offsets between GUI space 
 *         and screen space
 * 
 */
struct ScreenMetrics {

    vec2 mainSize;
    vec2 fourThree;
    vec2 GUISpace;

    vec2 renderOffset; // for extreme resolutions where we have to adjust

    float GUItoScreenXScale; // Scaling factor between screen width and GUI space width
    float GUItoScreenYScale; // Scaling factor between screen height and GUI space height
    vec2 screenSize; // what physical screen resolution are these values based on

    /*******************************************************************************************/
    /**
     * @brief  Constructor, for constructing
     * 
     */
    ScreenMetrics();

    /*******************************************************************************************/
    /**
     * @brief  Gets the current screen dimensions
     *
     * @returns 2d vector of the screen dimenions
     *
     */
    vec2 getMetrics();
    
    /*******************************************************************************************/
    /**
     * @brief  Checks to see if the resolution has changed
     * 
     * @param metrics 2d vector to compare against
     *
     * @returns true if the resolution has changed, false otherwise
     *
     */
    bool checkMetrics( vec2& metrics );
    
    /*******************************************************************************************/
    /**
     * @brief  Computer various values this class is responsible for
     * 
     */
    void computeFactors();

    vec2 GUIToScreen( const vec2 pos );
    
    float getScreenWidth();
    float getScreenHeight();
    
};

extern ScreenMetrics screenMetrics; // Dimension and translation information for the screen

/*******************************************************************************************/
/**
 * @brief Enums for various options
 *
 */
enum DividerOrientation {
    DOVertical,  // self explanatory 
    DOHorizontal // also self explanatory 
};

// When the boundary of an element is bigger than itself, how should it align itself
enum ContainerAlignment {
    CATop = 0,
    CALeft = 0,
    CACenter = 1,
    CARight = 2,
    CABottom = 2
};

vec4 HexColor(std::string const& hex);


// Wrap the internal error function
void IMDisplayError( std::string const& errorTitle, std::string const& errorMessage );
void IMDisplayError( std::string const& errorMessage );

// For sanity checking the reference counting for the GUI subsystem
class IMReferenceCountTracker {

    int totalRefCountObjs;
    std::map<std::string, int> typeCounts;
    
public:
    
    IMReferenceCountTracker() :
        totalRefCountObjs(0)
    {
    }
    
    // Register a new ref counted object
    void addRefCountObject( std::string const& className );
    // Register the destruction of a ref counted object
    void removeRefCountObject( std::string const& className );
    
    // Display a message if there are still live objects
    void logSanityCheck();
    
    virtual ~IMReferenceCountTracker();
    
};

extern IMReferenceCountTracker IMrefCountTracker;
