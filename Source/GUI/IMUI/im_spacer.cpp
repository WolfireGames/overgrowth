//-----------------------------------------------------------------------------
//           Name: im_spacer.cpp
//      Developer: Wolfire Games LLC
//    Description: Blank space element class for creating adhoc GUIs as part 
//                 of the UI tools. You probably don't want to create these 
//                 yourself, instead use the method in Divider
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
#include "im_spacer.h"

/*******************************************************************************************/
/**
 * @brief  Constructor - static divider
 *
 */
IMSpacer::IMSpacer( DividerOrientation _orientation, float size ) {
    
    IMrefCountTracker.addRefCountObject( getElementTypeName() );
    
    orientation = _orientation;
    
    if( orientation == DOVertical ) {
        setSize( vec2(UNDEFINEDSIZE, size) );
    }
    else {
        setSize( vec2(size, UNDEFINEDSIZE ) );
    }
    
    isStatic = true;
}

/*******************************************************************************************/
/**
 * @brief  Constructor - expanding divider
 *
 */
IMSpacer::IMSpacer( DividerOrientation _orientation ) {
    
    IMrefCountTracker.addRefCountObject( getElementTypeName() );
    
    // Start with 0 size, we'll
    if( orientation == DOVertical ) {
        setSize( vec2(UNDEFINEDSIZE, 0.0f) );
    }
    else {
        setSize( vec2(0.0f, UNDEFINEDSIZE) );
    }
    
    orientation = _orientation;
    
    isStatic = false;
}

IMSpacer::IMSpacer() {
    IMrefCountTracker.addRefCountObject( getElementTypeName() );
}

/*******************************************************************************************/
/**
 * @brief  Gets the name of the type of this element — for autonaming and debugging
 *
 * @returns name of the element type as a string
 *
 */
std::string IMSpacer::getElementTypeName() {
    return "Spacer";
}

/*******************************************************************************************/
/**
 * @brief  Destructor
 *
 */
IMSpacer::~IMSpacer() {
    IMrefCountTracker.removeRefCountObject( getElementTypeName() );
}




