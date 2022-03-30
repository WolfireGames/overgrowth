#include "ui_tools/ui_support.as"
#include "ui_tools/ui_Element.as"

/*******
 *  
 * ui_Text.as
 *
 * Text element class for creating adhoc GUIs as part of the UI tools  
 *
 */

namespace AHGUI {

class FontSetup
{
    string name;
    int size;
    vec4 color;

    FontSetup()
    {
        name = "edosz";
        size = 70;
        color = HexColor("#fff");
    }

    FontSetup( string _name, int _size, vec4 _color )
    {
        name = _name;
        size = _size;
        color = _color;
    }
}

/*******************************************************************************************/
/**
 * @brief Any styled text element 
 *
 */
class Text : Element 
{
    IMUIText imuiText;  // Engine object for the text
    string text;        // Actual text to render
    int GUIfontSize;    // Height of the text (GUI space)
    int screenFontSize; // Height of the text (screen space)
    string fontName;    // Name for the font 
    float rotation;     // Rotation for the text 
    ivec2 screenSize;   // Bit of a hack as we need the screen height for getting the right metrics
    bool shadowed;      // Should the text be shadowed?

    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *
     */
    Text() {
        super();
        setColor( 1.0, 1.0, 1.0, 1.0 );
        rotation = 0;
        shadowed = false;
    }

    /*******************************************************************************************/
    /**
     * @brief  Constructor
     * 
     * @param _name Name for this object (incumbent on the programmer to make sure they're unique)
     *
     */
    Text(string _name) {
        super(name);
        setColor( 1.0, 1.0, 1.0, 1.0 );
        rotation = 0;
        shadowed = false;
    }

    /*******************************************************************************************/
    /**
     * @brief  Gets the name of the type of this element — for autonaming and debugging
     * 
     * @returns name of the element type as a string
     *
     */
    string getElementTypeName() {
        return "Text";
    }

    /*******************************************************************************************/
    /**
     * @brief  Constructor
     * 
     * @param _text String for the text
     * @param _fontName name of the font (assumed to be in Data/Fonts)
     * @param _fontSize height of the font
     * @param _R Red 
     * @param _G Green
     * @param _B Blue
     * @param _A Alpha
     *
     */
    Text(string _text, string _fontName, int _fontSize, float _R = 1.0, float _G = 1.0, float _B = 1.0, float _A = 1.0 ) {
        setText( _text );
        setFont( _fontName, _fontSize );
        setColor( _R, _G, _B, _A );
    }

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
    Text(string _text, string _fontName, int _fontSize, vec4 _color = vec4(1.0f) ) {
        setText( _text );
        setFont( _fontName, _fontSize );
        setColor( _color.x, _color.y, _color.z, _color.a );
    }

    /*******************************************************************************************/
    /**
     * @brief  Constructor
     * 
     * @param _text String for the text
     * @param _fontSetup Font parameter structure
     *
     */
    Text( string _text, FontSetup _fontSetup )
    {
        setText( _text );
        setFont( _fontSetup.name, _fontSetup.size );
        setColor( _fontSetup.color.x, _fontSetup.color.y, _fontSetup.color.z, _fontSetup.color.a );
    }

    /*******************************************************************************************/
    /**
     * @brief  Derives the various metrics for this text element
     * 
     */
    void deriveMetrics() {

        // only bother if we have text
        if( text != "" && fontName != "" ) {

            vec2 boundingBox = imuiText.getBoundingBoxDimensions();

            screenSize.x = int(boundingBox.x);
            screenSize.y = int(boundingBox.y);

            setSize( int(float(screenSize.x) / screenMetrics.GUItoScreenX()), 
                     int(float(screenSize.y) / screenMetrics.GUItoScreenX()) );

            // Reset the boundary to the size
            setBoundarySize();

        }

    }

    /*******************************************************************************************/
    /**
     * @brief  Do whatever is necessary when the resolution changes
     *
     */
    void doScreenResize()  {
        screenFontSize = int(screenMetrics.GUItoScreenY() * float(GUIfontSize));
        deriveMetrics();
        
        setFont( fontName, GUIfontSize );
    }

    /*******************************************************************************************/
    /**
     * @brief  Sets the font attributes 
     * 
     * @param _fontName name of the font (assumed to be in Data/Fonts)
     * @param _fontSize height of the font
     *
     */
    void setFont( string _fontName, int _fontSize ) {
        
        fontName = _fontName;
        GUIfontSize = _fontSize;
        screenFontSize = int(screenMetrics.GUItoScreenY() * float(_fontSize));

        imuiText = AHGUI_IMUIContext.Get().makeText( "Data/Fonts/" + fontName + ".ttf", 
                                               screenFontSize, kSmallLowercase );

        if( text != "" ) {
            imuiText.setText( text );
        }

        if( shadowed ) {
            imuiText.setRenderFlags( kTextShadow );
        }

        imuiText.setRotation( rotation );

        deriveMetrics();
        onRelayout();

    }

    /*******************************************************************************************/
    /**
     * @brief  Sets the actual text 
     * 
     * @param _text String for the text
     *
     */
    void setText( string _text ) {
        
        text = _text;
        imuiText.setText( text );
        deriveMetrics();
        onRelayout();

    }

    /*******************************************************************************************/
    /**
     * @brief  Gets the current text
     * 
     * @returns String for the text
     *
     */
    string getText() {
        return text;
    }

    /*******************************************************************************************/
    /**
     * @brief  Sets the text to be shadowed
     *  
     * @param shadow true (default) if the text should have a shadow, false otherwise
     *
     */
    void setShadowed( bool shouldShadow = true ) {
        shadowed = shouldShadow;
    }

    /*******************************************************************************************/
    /**
     * @brief  Rather counter-intuitively, this draws this object on the screen
     *
     * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
     * @param clipPos pixel location of upper lefthand corner of clipping region
     * @param clipSize size of clipping region
     *
     */
    void render( ivec2 drawOffset, ivec2 currentClipPos, ivec2 currentClipSize ) {

        // Make sure we're supposed draw 
        if( show ) {

            ivec2 GUIRenderPos = drawOffset + boundaryOffset + ivec2( paddingL, paddingU ) + drawDisplacement;

            ivec2 screenRenderPos = screenMetrics.GUIToScreen( GUIRenderPos );

            imuiText.setPosition( vec3( screenRenderPos.x, screenRenderPos.y, getZOrdering() ) );
            
            if( isColorEffected ) {
                imuiText.setColor( effectColor );
            }
            else {
                imuiText.setColor( color );
            }

            if( currentClipSize.x != AH_UNDEFINEDSIZE && currentClipSize.y != AH_UNDEFINEDSIZE ){
                
                ivec2 adjustedClipPos = screenMetrics.GUIToScreen( currentClipPos );

                vec2 screenClipPos( vec2((float(adjustedClipPos.x)), 
                                     (float(adjustedClipPos.y)) ) );
                vec2 screenClipSize(vec2((float(currentClipSize.x)*screenMetrics.GUItoScreenXScale), 
                                     (float(currentClipSize.y)*screenMetrics.GUItoScreenYScale)) );

                imuiText.setClipping( screenClipPos, screenClipSize );
            }

            if( shadowed ) {
               imuiText.setRenderFlags( kTextShadow );
            }

            AHGUI_IMUIContext.Get().queueText( imuiText );

        }

        // Call the superclass to make sure any element specific rendering is done
        Element::render( drawOffset, currentClipPos, currentClipSize );

    }

    /*******************************************************************************************/
    /**
     * @brief  Updates the element  
     * 
     * @param delta Number of millisecond elapsed since last update
     * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
     * @param guistate The state of the GUI at this update
     *
     */
    void update( uint64 delta, ivec2 drawOffset, GUIState& guistate ) {

        Element::update( delta, drawOffset, guistate );
    
    }

    /*******************************************************************************************/
    /**
     * @brief Sets the rotation for this image
     * 
     * @param Rotation (in degrees)
     *
     */
    void setRotation( float _rotation ) {
        rotation = _rotation;
        imuiText.setRotation( rotation );
        deriveMetrics();
        onRelayout();
    }

    /*******************************************************************************************/
    /**
     * @brief Gets the rotation for this text
     * 
     * @returns current rotation (in degrees)
     *
     */
    float getRotation() {
        return rotation;
    }

}

} // namespace AHGUI

