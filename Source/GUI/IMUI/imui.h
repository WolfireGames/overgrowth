//-----------------------------------------------------------------------------
//           Name: imui.h
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

#include <Logging/logdata.h>
#include <Logging/loghandler.h>
#include <Logging/consolehandler.h>

#include <Math/vec2.h>
#include <Math/vec3.h>
#include <Math/vec4.h>

#include <Graphics/textureref.h>
#include <Graphics/shaders.h>
#include <Graphics/graphics.h>
#include <Graphics/text.h>
#include <Graphics/vboringcontainer.h>

#include <vector>
#include <string>
#include <ostream>

enum IMUIRenderableType {
    IMUIRinvalid = 0,
    IMIURimage = 1,
    IMIURtext = 2
};

struct IMUIRenderable {
    bool skip_aspect_fitting; /**Ignore constraints used for ensuring UI sanity **/

    vec3 pos; /**< Position (including z ordering) */
    vec4 color; /**< Color for this element */
    
    float rotation; /**< Rotation for the element ( in degrees ) */
    bool enableClip; /**< Should we clip while rendering? */
    vec2 clipPosition; /**< offset from the upper left corner of the screen to clip */
    vec2 clipSize; /**< width and height of the clip */
    
    IMUIRenderableType type; /**< Type for quick and dirty RTTI */
    
    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *
     */
    IMUIRenderable( IMUIRenderableType _type = IMUIRinvalid ) :
        pos( 0.0 ),
        color( 1.0 ),
        rotation(0),
        enableClip( false ),
        type(_type),
        skip_aspect_fitting(false)
    {}
    
    /*******************************************************************************************/
    /**
     * @brief  Copy constructor
     *
     */
    IMUIRenderable( const IMUIRenderable& other ) {
        
        pos = other.pos;
        color = other.color;
        
        rotation = other.rotation;
        enableClip = other.enableClip;
        clipPosition = other.clipPosition;
        clipSize = clipSize = other.clipSize;
        
        type = other.type;

        skip_aspect_fitting = other.skip_aspect_fitting;
    }

    virtual ~IMUIRenderable(){};
    
    /*******************************************************************************************/
    /**
     * @brief  Sets the position (z will be used for drawing order, no perspective)
     *
     * @param newPos new position for the renderable
     *
     */
    void setPosition( vec3 newPos );
    
    /*******************************************************************************************/
    /**
     * @brief  Sets the color
     *
     * @param newColor new color for the renderable
     *
     */
    void setColor( vec4 newColor );
 
    /*******************************************************************************************/
    /**
     * @brief  Sets the rotation
     *
     * @param newRotation new rotation for the renderable
     *
     */
    void setRotation( float newRotation );
    
    
    /*******************************************************************************************/
    /**
     * @brief  Sets the clipping for rendering this object
     *
     * @param offset position in pixels (from upper left) for the clipping box
     * @param size width and height of the clipping box
     *
     */
    void setClipping( vec2 offset, vec2 size );
    
    /*******************************************************************************************/
    /**
     * @brief  Disables clipping for this renderable
     *
     */
    void disableClipping();
    
};

struct IMUIImage : public IMUIRenderable {
    TextureAssetRef textureRef; /**< Texture associated with this image (if any) */
    
    vec2 renderSize; /**< what size in screen pixels should we be rendering at */
    vec2 textureSize; /**< what size is the texture as loaded */
    vec2 textureOffset; /**< where do we start rendering in the texture */
    vec2 textureSourceSize; /**< How much of the texture are we rendering? */
    
    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *
     */
    IMUIImage() :
        IMUIRenderable( IMIURimage )
    {}

    
    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *
     *
     * @param filename Takes the filename to populate this iamge
     */
    
    IMUIImage( std::string filename );
    
    /*******************************************************************************************/
    /**
     * @brief  Copy constructor
     *
     */
    IMUIImage( const IMUIImage& other ) :
        IMUIRenderable( (IMUIRenderable)other ),
        textureRef( other.textureRef ),
        renderSize( other.renderSize ),
        textureSize( other.textureSize ),
        textureOffset( other.textureOffset ),
        textureSourceSize( other.textureSourceSize )
    {
    }
    
    /*******************************************************************************************/
    /**
     * @brief  Destructor
     *
     */
    ~IMUIImage() override {
    }
    
    /*******************************************************************************************/
    /**
     * @brief  Set the texture from a file
     *
     * @param filename Where to find the texture
     *
     * @returns true if the file was loaded, false otherwise
     *
     */
    bool loadImage( std::string filename );
    
    /*******************************************************************************************/
    /**
     * @brief  Determines if this image is valid for display
     *
     * @returns true if valid, false otherwise
     *
     */
    bool isValid() const;
    
    /*******************************************************************************************/
    /**
     * @brief  Gets the width of the attached texture
     *
     * @returns The width (as float)
     *
     */
    float getTextureWidth() const;
    
    /*******************************************************************************************/
    /**
     * @brief  Gets the height of the attached texture
     *
     * @returns The height (as float)
     *
     */
    float getTextureHeight() const;
    
    
    /*******************************************************************************************/
    /**
     * @brief  Sets the offset of where to start drawing the image
     *
     * @param offset Offset from the upperlefthand 
     * @param size How big an area are we going to render (clamps to make sure the
     *
     */
    void setRenderOffset( vec2 offset, vec2 size );
    
    
    /*******************************************************************************************/
    /**
     * @brief  Sets the size to render on the screen
     *
     * @param size new render dimensions
     *
     */
    void setRenderSize( vec2 size );
    
};


class IMUIContext;

struct IMUIText : public IMUIRenderable {
    
    std::string text; /**< actual text for this object */
    
    IMUIContext* owner; /**< what context created this object */
    
    // font information (filled by IMUIContext)
    int size; /**< size of font, in pixels */
    std::string fontName; /**< fontName (including full path) */
    int fontFlags; /**< font options flags */
    int renderFlags; /**< render options flags */
    
    vec2 dimensions; /**< dimensions of the text  */
    vec2 boundingBox; /**< dimensions of the text under rotation (if any) */
    
    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *
     */
    IMUIText() :
        IMUIRenderable( IMIURtext ),
        text( "" ),
        owner( NULL ),
        size( 0 ),
        fontName(""),
        fontFlags( 0 ),
        renderFlags( 0 ),
        dimensions( 0.0 ),
        boundingBox( 0.0 )
    {}
    
    /*******************************************************************************************/
    /**
     * @brief  Copy constructor
     *
     */
    IMUIText( const IMUIText & other ) :
        IMUIRenderable( (IMUIRenderable)other ),
        text( other.text ),
        owner( other.owner ),
        size( other.size ),
        fontName( other.fontName ),
        fontFlags( other.fontFlags ),
        renderFlags( other.renderFlags ),
        dimensions( other.dimensions ),
        boundingBox( other.boundingBox )
    {
    }
    
    /*******************************************************************************************/
    /**
     * @brief  Destructor
     *
     */
    ~IMUIText() override {
    }
    
    /*******************************************************************************************/
    /**
     * @brief  Gets the hight and width of the bounding box around the text
     *
     * @returns Size of the text as a 2d vector
     *
     */
    vec2 getDimensions() const { return dimensions; }
    
    /*******************************************************************************************/
    /**
     * @brief  Gets the hight and width of the bounding box around the text (under rotation, if any)
     *
     * @returns Size of the text as a 2d vector
     *
     */
    vec2 getBoundingBoxDimensions() const { return boundingBox; }
    
    /*******************************************************************************************/
    /**
     * @brief  Sets the text for this instance
     *
     * @param text Text for this instance
     *
     */
    void setText( std::string& _text );
    
    /*******************************************************************************************/
    /**
     * @brief  Sets the rotation
     *
     * @param newRotation new rotation for the renderable
     *
     */
    void setRotation( float newRotation );
    
    /*******************************************************************************************/
    /**
     * @brief  Sets the render flags for this text
     *
     * (currently only kTextShadow )
     *
     * @param newFlags flags for rendering
     *
     */
    void setRenderFlags( int newFlags ) {
        renderFlags = newFlags;
    }
    
};

class IMUIContext {
public:

    enum UIState{kNothing, kHot, kActive};
    enum ButtonState{kMouseUp, kMouseDown, kMouseStillUp, kMouseStillDown};
    
    void UpdateControls();
    bool DoButton(int id, vec2 top_left, vec2 bottom_right, UIState &ui_state);//, UIState* ui_state = NULL);
    bool DoButtonMouseOver( int id, bool mouse_over, UIState& ui_state);
    void ClearHot();
    void Init();
    vec2 getMousePosition();
    ButtonState getLeftMouseState();
    
    // Text and image support
    std::vector<IMUIRenderable*> renderQueue; /**< queue for objects to render this frame */

    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *
     */
    IMUIContext();
    
    /*******************************************************************************************/
    /**
     * @brief  Destructor
     *
     */
    ~IMUIContext();
    
    /*******************************************************************************************/
    /**
     * @brief Queue an image for rendering
     *
     * @param newImage pointer to an image description
     *
     */
    void queueImage( IMUIImage& newImage );
    
    /*******************************************************************************************/
    /**
     * @brief  Create a new text object with given font characteristics
     *
     * @param fontName filename for this font (including path)
     * @param size size in pixels
     * @param flags style flags
     *
     */
    IMUIText makeText( std::string const& fontName, int size, int fontFlags, int renderFlags = 0 );
    
    /*******************************************************************************************/
    /**
     * @brief  Adds the dimensions to a given text object
     *
     * @param text Text object (reference)
     *
     */
    void deriveFontDimensions( IMUIText& text );
    
    /*******************************************************************************************/
    /**
     * @brief Queue some text for rendering
     *
     * @param newImage
     *
     */
    void queueText( IMUIText& newText );
    
    /*******************************************************************************************/
    /**
     * @brief  Clear all the cached texts
     *
     */
    void clearTextAtlases();
    
    /*******************************************************************************************/
    /**
     * @brief  Render all queued elements and clear the render queue
     *
     */
    void render();
    
    
private:
    static const unsigned kFloatsPerVert = 4;
    static const unsigned kMaxCharacters = 1024;

    RC_VBORingContainer image_data_vbo;
    RC_VBOContainer image_index_vbo;

    RC_VBORingContainer character_data_vbo;
    RC_VBORingContainer character_index_vbo;
    
    CachedTextAtlases atlases; /**< text atlases owned by this instance */
    
    struct DebugVizSquare {
        UIState ui_state;
        vec2 top_left, bottom_right;
    };
    
    int hot_;
    int active_;
    vec2 mouse_pos;
    
    ButtonState lmb_state;
    std::vector<IMUIRenderableType*> renderables;
    
    void Draw();
    
    /*******************************************************************************************/
    /**
     * @brief Get the atlas for the specified font (loaded if not already cached)
     *
     * @param fontName filename for this font (including path)
     * @param size size in pixels
     * @param flags style flags
     *
     * @returns Pointer to the atlas structure, null if not found
     *
     */
    TextAtlas* getTextAtlas( std::string fontName, int size, int flags );

};
