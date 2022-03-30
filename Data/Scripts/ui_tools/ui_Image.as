#include "ui_tools/ui_support.as"
#include "ui_tools/ui_Element.as"

/*******
 *  
 * ui_Image.as
 *
 *Image element class for creating adhoc GUIs as part of the UI tools  
 *
 */

namespace AHGUI {

/*******************************************************************************************/
/**
 * @brief Any styled text element 
 *
 */
class Image : Element 
{

	string imageFileName; 	//Filename for the image
	IMUIImage imuiImage;	//Engine image description

    ivec2 originalImageSize;//What is the base size of the image before transformation

    float rotation; // how much is this image rotated

    ivec2 textureOffset; // Where to start drawing the texture from  
    ivec2 textureSize; 	 // How much of the image to use

    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *
     */
    Image() {
        super();
        setColor( 1.0, 1.0, 1.0, 1.0 );
    }

    /*******************************************************************************************/
    /**
     * @brief  Constructor
     * 
     * @param imageName Filename for the image
     *
     */
    Image(string imageName) {
        super();
        setImageFile( imageName );
        setColor( 1.0, 1.0, 1.0, 1.0 );
    }

    /*******************************************************************************************/
    /**
     * @brief  Gets the name of the type of this element — for autonaming and debugging
     * 
     * @returns name of the element type as a string
     *
     */
    string getElementTypeName() {
        return "Image";
    }

    /*******************************************************************************************/
    /**
     * @brief  Sets the source for the image
     * 
     * @param _fileName 
     *
     */
    void setImageFile( string _fileName ) {
        
        if( _fileName == "" )
        {
            Log(info, "Sent in empty string, will print callstack");
            PrintCallstack();
        }
        
        imageFileName = "Data/" + _fileName;
    	if( !imuiImage.loadImage( imageFileName ) ) {
    		DisplayError("Error", "Unable to locate image " + imageFileName );
    		return;
    	}

    	//Get the size
        originalImageSize = ivec2( int( imuiImage.getTextureWidth() ), 
        					       int( imuiImage.getTextureHeight() ) );

        setSize( originalImageSize );

    }

    /*******************************************************************************************/
    /**
     * @brief  Rescales the image to a specified width
     * 
     * @param newSize new x size   
     *
     */
    void scaleToSizeX( int newSize ) {
    	int newYSize = int( float(originalImageSize.y)/float(originalImageSize.x) * float(newSize) );
     	setSize( newSize, newYSize );
    }

    /*******************************************************************************************/
    /**
     * @brief  Rescales the image to a specified height
     * 
     * @param newSize new y size   
     *
     */
    void scaleToSizeY( int newSize ) {
    	int newXSize = int( float(originalImageSize.x)/float(originalImageSize.y) * float(newSize) );
     	setSize( newXSize, newSize );
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

    	// Make sure we have an an image and we're supposed draw it
    	if( imageFileName != "" && show ) {

	        ivec2 GUIRenderPos = drawOffset + boundaryOffset + drawDisplacement;

			ivec2 screenRenderPos = screenMetrics.GUIToScreen( GUIRenderPos );

			imuiImage.setRenderSize( vec2( (float(getSizeX())*screenMetrics.GUItoScreenXScale), 
				 						   (float(getSizeY())*screenMetrics.GUItoScreenYScale) ) );

			imuiImage.setPosition( vec3( screenRenderPos.x, screenRenderPos.y, getZOrdering() ) );
			
	        if( isColorEffected ) {
                imuiImage.setColor( effectColor );
            }
            else {
                imuiImage.setColor( color );
            }

			imuiImage.setRotation( rotation );


			if( textureSize.x != 0 && textureSize.y != 0 ) {
				imuiImage.setRenderOffset( vec2( textureOffset.x, textureOffset.y ),
									   vec2( textureSize.x, textureSize.y ) );	
			}
			
			// Add clipping (if we need it)
			if( currentClipSize.x != AH_UNDEFINEDSIZE && currentClipSize.y != AH_UNDEFINEDSIZE ) {

				ivec2 adjustedClipPos = screenMetrics.GUIToScreen( currentClipPos );
	            imuiImage.setClipping( vec2((float(adjustedClipPos.x)), 
	                                        (float(adjustedClipPos.y))), 
	                                   vec2((float(currentClipSize.x)*screenMetrics.GUItoScreenXScale), 
	                                        (float(currentClipSize.y)*screenMetrics.GUItoScreenYScale)) );
        	}

			AHGUI_IMUIContext.Get().queueImage( imuiImage );
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
    	imuiImage.setRotation( rotation );
    }

    /*******************************************************************************************/
    /**
     * @brief Gets the rotation for this image
     * 
     * @returns current rotation (in degrees)
     *
     */
    float getRotation() {
    	return rotation;
    }

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
    void setImageOffset( ivec2 offset, ivec2 size ) {
    	textureOffset = offset;
    	textureSize = size;
    }

}

} // namespace AHGUI

