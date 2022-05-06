//-----------------------------------------------------------------------------
//           Name: im_spacer.h
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
#pragma once

#include <GUI/IMUI/im_support.h>
#include <GUI/IMUI/im_element.h>

/*******************************************************************************************/
/**
 * @brief Blank space
 *
 */
class IMSpacer : public IMElement
{

public:
    
    DividerOrientation orientation; // what direction is the host divider
    bool isStatic;  // should this grow and shrink by the needs of its container?

    /*******************************************************************************************/
    /**
     * @brief  Constructor - static divider 
     *
     */
    IMSpacer( DividerOrientation _orientation, float size );
    
    /*******************************************************************************************/
    /**
     * @brief  Constructor - expanding divider 
     *
     */
    IMSpacer( DividerOrientation _orientation );
    IMSpacer();
    
    /*******
     *
     * Angelscript factory
     *
     */
    static IMSpacer* ASFactory_static( DividerOrientation _orientation, float size ) {
        return new IMSpacer( _orientation, size );
    }
    
    static IMSpacer* ASFactory_dynamic( DividerOrientation _orientation ) {
        return new IMSpacer( _orientation );
    }
    
    /*******************************************************************************************/
    /**
     * @brief  Gets the name of the type of this element — for autonaming and debugging
     * 
     * @returns name of the element type as a string
     *
     */
    std::string getElementTypeName() override;
    
    /*******************************************************************************************/
    /**
     * @brief  Destructor
     *
     */
    ~IMSpacer() override;
};


