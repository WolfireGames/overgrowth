//-----------------------------------------------------------------------------
//           Name: im_image.cpp
//      Developer: Wolfire Games LLC
//    Description: Image element class for creating adhoc GUIs as part of 
//                 the UI tools  
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

#include <Internal/filesystem.h>

#include <string>
/*******************************************************************************************/
/**
 * @brief Any styled text element 
 *
 */
class IMImage : public IMElement
{

public:
    bool skip_aspect_fitting;
    bool center;

    std::string imageFileName; 	//Filename for the image
	IMUIImage imuiImage;	//Engine image description

    vec2 originalImageSize;//What is the base size of the image before transformation

    float rotation; // how much is this image rotated

    vec2 textureOffset; // Where to start drawing the texture from  
    vec2 textureSize; 	 // How much of the image to use

    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *
     */
    IMImage();
    
    /*******************************************************************************************/
    /**
     * @brief  Copy constructor
     *
     */
    IMImage( IMImage const& other );
    
    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *
     * @param imageName Filename for the image
     *
     */
    IMImage(std::string const& imageName);

    IMImage(std::string const& imageName, bool abs_path);

    /*******
     *  
     * Angelscript factory 
     *
     */
     static IMImage* ASFactory( std::string const& imageName ) {
        return new IMImage( imageName );
     }
    
     static IMImage* ASFactoryPath( Path& path ) {
        return new IMImage( path.GetAbsPathStr(), true );
     }
    
    /*******************************************************************************************/
    /**
     * @brief  Gets the name of the type of this element — for autonaming and debugging
     *
     * @returns name of the element type as a string
     *
     */
    std::string getElementTypeName();

    void setSkipAspectFitting(bool val);
    void setCenter(bool val);
    
    /*******************************************************************************************/
    /**
     * @brief  Sets the source for the image
     *
     * @param _fileName
     *
     */
    void setImageFile( std::string const& _fileName );
    void setImageFileAbs( std::string const& _fileName );
    
    /*******************************************************************************************/
    /**
     * @brief  Rescales the image to a specified width
     *
     * @param newSize new x size
     *
     */
    void scaleToSizeX( float newSize );
    
    /*******************************************************************************************/
    /**
     * @brief  Rescales the image to a specified height
     *
     * @param newSize new y size
     *
     */
    void scaleToSizeY( float newSize );
    
    /*******************************************************************************************/
    /**
     * @brief  Rather counter-intuitively, this draws this object on the screen
     *
     * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
     * @param clipPos pixel location of upper lefthand corner of clipping region
     * @param clipSize size of clipping region
     *
     */
    void render( vec2 drawOffset, vec2 currentClipPos, vec2 currentClipSize );
    
    /*******************************************************************************************/
    /**
     * @brief  Updates the element
     *
     * @param delta Number of millisecond elapsed since last update
     * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
     * @param guistate The state of the GUI at this update
     *
     */
    void update( uint64_t delta, vec2 drawOffset, GUIState& guistate );
    
    /*******************************************************************************************/
    /**
     * @brief Sets the rotation for this image
     *
     * @param Rotation (in degrees)
     *
     */
    void setRotation( float _rotation );
    
    /*******************************************************************************************/
    /**
     * @brief Gets the rotation for this image
     *
     * @returns current rotation (in degrees)
     *
     */
    float getRotation();
    
    /*******************************************************************************************/
    /**
     * @brief  Render only part of this image 
     * 
     * (all coordinates are based on the original size of the image)
     * 
     * @param offset position of the upper lefthand coordinate to source the rendering from 
     * @param size size of the rectangle to use for the rendering 
     *
     */
    void setImageOffset( vec2 offset, vec2 size );
    
    /*******************************************************************************************/
    /**
     * @brief  Remove all referenced object without releaseing references
     *
     */
    virtual void clense();
    
    /*******************************************************************************************/
    /**
     * @brief  Destructor
     *
     */
    virtual ~IMImage();
    
};

