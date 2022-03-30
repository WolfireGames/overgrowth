//-----------------------------------------------------------------------------
//           Name: im_text.h
//      Developer: Wolfire Games LLC
//    Description: Text element class for creating adhoc GUIs as part of the 
//                 UI tools  
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

struct FontSetup
{
    std::string fontName;   // name for the font
    int size;               // size in GUI pixels
    vec4 color;             // color for the font
    float rotation; // Rotation for the text
    bool shadowed;  // should this text have a shadow?

    FontSetup() :
        rotation(0.0),
        shadowed(false)
    {
        fontName = "OpenSans-Regular";
        size = 60;
        color = HexColor("#fff");
        
    }

    FontSetup( std::string const& _name, int _size, vec4 _color, bool _shadowed = false ) :
        rotation(0.0)
    {
        fontName = _name;
        size = _size;
        color = _color;
        shadowed = _shadowed;
    }
    
    ~FontSetup() {}
};


static void FontSetup_ASconstructor_default(FontSetup *self) {
    new(self) FontSetup();
}

static void FontSetup_ASconstructor_params(FontSetup *self, std::string const& _name, int32_t _size, vec4 _color, bool _shadowed = false ) {
    new(self) FontSetup(  _name, _size, _color, _shadowed );
}

static void FontSetup_AScopy(FontSetup *self, const FontSetup& other ) {
    new(self) FontSetup( other );
}

static void FontSetup_ASdestructor(FontSetup *self ) {
    self->~FontSetup();
}

static const FontSetup& FontSetup_ASassign(FontSetup *self, const FontSetup& other) {
    return (*self) = other;
}




/*******************************************************************************************/
/**
 * @brief Any styled text element 
 *
 */
class IMText : public IMElement
{
public:
    IMUIText imuiText;  // Engine object for the text
    bool fontObjDirty;  // Does the engine object need updating
    bool fontObjInit;   // Has the font object been initialized, at least once
    FontSetup fontSetup;// Attributes for the current font
    std::string text;   // Actual text to render
    int screenFontSize; // Height of the text (screen space) 
    vec2 screenSize;   // Bit of a hack as we need the screen height for getting the right metrics
    
    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *
     */
    IMText();
    
    /*******************************************************************************************/
    /**
     * @brief  Copy constructor
     *
     */
    IMText( IMText const& other );
    
    /*******************************************************************************************/
    /**
     * @brief  Constructor
     * 
     * @param _name Name for this object (incumbent on the programmer to make sure they're unique)
     *
     */
    IMText(std::string const& _name);
    
    /*******************************************************************************************/
    /**
     * @brief  Constructor
     * 
     * @param _text String for the text
     * @param _fontName name of the font (assumed to be in Data/Fonts)
     * @param _fontSize height of the font
     * @param _color vec4 color rgba
     *
     */
    IMText(std::string const& _text, std::string const& _fontName, int _fontSize, vec4 _color = vec4(1.0f) );
    
    /*******************************************************************************************/
    /**
     * @brief  Constructor
     * 
     * @param _text String for the text
     * @param _fontSetup Font parameter structure
     *
     */
    IMText( std::string const& _text, FontSetup _fontSetup );
    
    /*******
     *
     * Angelscript factory
     *
     */
    static IMText* ASFactory_named( std::string const& name ) {
        return new IMText( name );
    }
    
    static IMText* ASFactory_unnamed( std::string const& _text, std::string const& _fontName, int _fontSize, vec4 _color = vec4(1.0f) ) {
        return new IMText( _text, _fontName, _fontSize, _color );
    }
    
    static IMText* ASFactory_fontsetup( std::string const& _text, FontSetup _fontSetup ) {
        return new IMText( _text, _fontSetup );
    }
    
    
    /*******************************************************************************************/
    /**
     * @brief  Gets the name of the type of this element — for autonaming and debugging
     *
     * @returns name of the element type as a string
     *
     */
    std::string getElementTypeName();
    
    /*******************************************************************************************/
    /**
     * @brief  Derives the various metrics for this text element
     * 
     */
    void updateEngineTextObject();
    
    /*******************************************************************************************/
    /**
     * @brief  When a resize, move, etc has happened do whatever is necessary
     * 
     */
    void doRelayout();

    /*******************************************************************************************/
    /**
     * @brief  Do whatever is necessary when the resolution changes
     *
     */
    void doScreenResize();

    /*******************************************************************************************/
    /**
     * @brief  Sets the font attributes 
     * 
     * @param _fontSetup font description object
     *
     */
    void setFont( FontSetup _fontSetup );

    /*******************************************************************************************/
    /**
     * @brief  Sets the font attributes 
     * 
     * @param _fontName name of the font (assumed to be in Data/Fonts)
     * @param _fontSize height of the font
     *
     */
    void setFontByName( std::string const& _fontName, int _fontSize );

    /*******************************************************************************************/
    /**
     * @brief  Sets the actual text 
     * 
     * @param _text String for the text
     *
     */
    void setText( std::string const& _text );
    
    /*******************************************************************************************/
    /**
     * @brief  Gets the current text
     * 
     * @returns String for the text
     *
     */
    std::string getText();
    
    /*******************************************************************************************/
    /**
     * @brief  Sets the text to be shadowed
     *  
     * @param shadow true (default) if the text should have a shadow, false otherwise
     *
     */
    void setShadowed( bool shouldShadow = true );
    
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
     * @brief Gets the rotation for this text
     * 
     * @returns current rotation (in degrees)
     *
     */
    float getRotation();
    
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
     * @brief  Remove all referenced object without releaseing references
     *
     */
    virtual void clense();
    
    virtual ~IMText();
};
