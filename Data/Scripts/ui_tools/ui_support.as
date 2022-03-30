/*******
 *  
 * ui_support.as
 *
 * A set of support function and defines for putting together `ad hoc'/overlay GUIs  
 *
 */

// All coordinates are specified in terms of a space 2560 x 1440
// (at the moment -- this assumes 16:9 ratio -- other ratios are a TODO)
// when rendering to the screen the projection is done automatically
// coordinates start at the top left of the screen 



const int AH_UNDEFINEDSIZE = -1;

namespace AHGUI {





/*******************************************************************************************/
/**
 * @brief  Helper class to derive and contain the scaling factors and offsets between GUI space 
 *         and screen space
 * 
 */
class ScreenMetrics {
    
    int GUISpaceX = 2560;   // GUI space size X 
    int GUISpaceY = 1440;   // GUI space size Y

    bool GUISpace16x9 = true; // is the GUI forced to 16 x 9? 

    float GUItoScreenXScale; // Scaling factor between screen width and GUI space width
    float GUItoScreenYScale; // Scaling factor between screen height and GUI space height
    ivec2 renderOffset; // Where to start rendering to get a 16x9 picture
    ivec2 screenSize; // what physical screen resolution are these values based on

    /*******************************************************************************************/
    /**
     * @brief  Constructor, for constructing
     * 
     */
    ScreenMetrics( bool do16x9 = true ) {
        restrict16x9( do16x9 );
    }

    /*******************************************************************************************/
    /**
     * @brief  Determine whether or not this is going to letterbox to 16:9
     * 
     * @param _GUISpace16x9
     *
     */
     void restrict16x9( bool _GUISpace16x9 ) {
        GUISpace16x9 = _GUISpace16x9;

        computeFactors();

     } 


    /*******************************************************************************************/
    /**
     * @brief  Checks to see if the resolution has changed and if so rederive the values
     * 
     * @returns true if the resolution has changed, false otherwise
     *
     */
     bool checkMetrics() {
        if( screenSize.x != GetScreenWidth() || screenSize.y != GetScreenHeight() ) {
            computeFactors();
            return true;
        }
        else {
            return false;
        }
     }

    /*******************************************************************************************/
    /**
     * @brief  Computer various values this class is responsible for
     * 
     */
    void computeFactors() {
       
        GUISpaceX = 2560;
        if( GUISpace16x9 ) {
            GUISpaceY = 1440;
        }
        else {
            GUISpaceY = int((float(GUISpaceX)/float(GetScreenWidth()))*float(GetScreenHeight()));
        }

        GUItoScreenXScale = GUItoScreenX();
        GUItoScreenYScale = GUItoScreenY();
        renderOffset = getRenderOffset();
        screenSize = ivec2( GetScreenWidth(), GetScreenHeight() );
    }

    /*******************************************************************************************/
    /**
     * @brief  Figure out effectives pixels we're going to use on the screen
     * 
     */
    ivec2 getEffectiveScreenPixels() {

        ivec2 adjScreenSize;

        if( GUISpace16x9 ) {

            // see if we're wider than 16x9
            if( float(GetScreenWidth())/16.0 > (GetScreenHeight())/9.0 ) {
                // if so, we'll black out the sides
                adjScreenSize.y = GetScreenHeight();
                adjScreenSize.x = int((float(adjScreenSize.y)/ 9.0 ) * 16.0);
            }
            else {
                // otherwise we 'letterbox' up top (if needed)
                adjScreenSize.x = GetScreenWidth();
                adjScreenSize.y = int((float(adjScreenSize.x)/ 16.0 ) * 9.0);
            }
        }
        else {
            adjScreenSize = ivec2( GetScreenWidth(), GetScreenHeight() );
        }

        return adjScreenSize;
    }


    float GUItoScreenX() {
        ivec2 screenSpace = getEffectiveScreenPixels();
        return ( float(screenSpace.x) / float(GUISpaceX) );
    }

    float GUItoScreenY() {
        ivec2 screenSpace = getEffectiveScreenPixels();
        return ( float(screenSpace.y) / float(GUISpaceY) );
    }

    ivec2 getRenderOffset() {
        ivec2 screenSpace = getEffectiveScreenPixels();
        return ivec2( (GetScreenWidth() - screenSpace.x )/2, (GetScreenHeight() - screenSpace.y )/2 );
    }

    ivec2 GUIToScreen( const ivec2 pos ) {
        return ivec2( int( floor( (float(pos.x) * GUItoScreenXScale ) + 0.5 ) + renderOffset.x ),
                      int( floor( (float(pos.y) * GUItoScreenYScale ) + 0.5 ) + renderOffset.y ) );
    }

    ivec2 GUIToScreen( const int x, const int y ) {
        return ivec2( int( floor( (float(x) * GUItoScreenXScale ) + 0.5 ) + renderOffset.x ),
                      int( floor( (float(y) * GUItoScreenYScale ) + 0.5 ) + renderOffset.y ) );
    }

}

// make a global pseudo-singleton
ScreenMetrics screenMetrics;

/*******************************************************************************************/
/**
 * @brief  Helper to draw a monocromatic box 
 * 
 *  
 *
 */
void drawDebugBox( bool GUISpace, ivec2 pos, ivec2 size, float R = 1.0f, float G = 1.0f, float B = 1.0f, float A = 1.0f ) {

    // Check to see if the coordinates are in GUI Space (if not it's in screen space)
    if( GUISpace ) {
        pos = screenMetrics.GUIToScreen( pos );
        size.x = int( float(size.x) * screenMetrics.GUItoScreenXScale );
        size.y = int( float(size.y) * screenMetrics.GUItoScreenYScale );
    }

    HUDImage @boximage = hud.AddImage();
    boximage.SetImageFromPath("Data/Textures/ui/whiteblock.tga");

    boximage.scale = 1;
    boximage.scale.x *= size.x;
    boximage.scale.y *= size.y;

    boximage.position.x = pos.x;
    boximage.position.y = GetScreenHeight() - pos.y - (boximage.GetWidth() * boximage.scale.y );
    boximage.position.z = 1.0;

    boximage.color = vec4( R, G, B, A );  

}


} // namespace AHGUI


/*******************************************************************************************/
/**
 * @brief Enums for various options, out of the namespace for sanities sake, they should 
 *        be easily unique
 *
 */
// enum DividerOrientation {
//     DOVertical,  // self explanatory 
//     DOHorizontal // also self explanatory 
// }

// When placing an element in a divider, which direction is it coming from
//  right now a container can only have one centered element
enum DividerDirection {
    DDTopLeft = 0,
    DDTop = 0,
    DDLeft = 0,
    DDCenter = 1,
    DDBottomRight = 2,
    DDBottom = 2,
    DDRight = 2
}

// When the boundary of an element is bigger than itself, how should it align itself
enum BoundaryAlignment {
    BATop = 0,
    BALeft = 0,
    BACenter = 1,
    BARight = 2,
    BABottom = 2

}

vec4 HexColor(string hex)
{
    if( hex.substr(0,1) == "#" && hex.length() == 7 )
    {
        int c1 = parseInt(hex.substr(1,2), 16);
        int c2 = parseInt(hex.substr(3,2), 16);
        int c3 = parseInt(hex.substr(5,2), 16);

        return vec4( c1/255.0f,c2/255.0f,c3/255.0f,1.0f);
    }
    if( hex.substr(0,1) == "#" && hex.length() == 4 )
    {
        int c1 = parseInt(hex.substr(1,1), 16);
        int c2 = parseInt(hex.substr(2,1), 16);
        int c3 = parseInt(hex.substr(3,1), 16);

        return vec4( c1/16.0f,c2/16.0f,c3/16.0f,1.0f);
    }
    else
    {
        return vec4(1.0f);
    }
}
