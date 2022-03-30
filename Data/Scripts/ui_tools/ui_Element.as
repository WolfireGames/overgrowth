#include "ui_tools/ui_support.as"
#include "ui_tools/ui_guistate.as"

/*******
 *  
 * ui_element.as
 *
 * Root element class for creating adhoc GUIs as part of the UI tools   
 *
 */

namespace AHGUI {

/*******************************************************************************************/
/**
 * @brief  Attachable behavior base class - called on update
 * 
 */
class UpdateBehavior {
    string key;

    bool initialized = false;   // Has this update been run once?

    /*******************************************************************************************/
    /**
     * @brief  Called before the first update
     * 
     * @param element The element attached to this behavior 
     * @param delta Number of millisecond elapsed since last update
     * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
     * @param guistate The state of the GUI at this update
     *
     * @returns true if this behavior should continue next update, false otherwise
     *
     */
    bool initialize( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
        return true;
    }

    /*******************************************************************************************/
    /**
     * @brief  Called on update
     * 
     * @param element The element attached to this behavior 
     * @param delta Number of millisecond elapsed since last update
     * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
     * @param guistate The state of the GUI at this update
     *
     * @returns true if this behavior should continue next update, false otherwise
     *
     */
    bool update( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
        return true;
    }

    /*******************************************************************************************/
    /**
     * @brief  Called when the behavior ceases, whether by its own indicate or externally
     * 
     * @param element The element attached to this behavior 
     *
     */
    void cleanUp( Element@ element ) {
        
    }

}

/*******************************************************************************************/
/**
 * @brief  Attachable behavior base class - called on mouse over 
 * 
 */
class MouseOverBehavior {

    /*******************************************************************************************/
    /**
     * @brief  Called when the mouse enters the element
     * 
     * @param element The element attached to this behavior 
     * @param delta Number of millisecond elapsed since last update
     * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
     * @param guistate The state of the GUI at this update
     *
     */
    void onStart( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {

    }

    /*******************************************************************************************/
    /**
     * @brief  Called when the mouse is still over the element 
     * 
     * @param element The element attached to this behavior 
     * @param delta Number of millisecond elapsed since last update
     * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
     * @param guistate The state of the GUI at this update
     *
     */
    void onContinue( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {

    }

    /*******************************************************************************************/
    /**
     * @brief  Called when the mouse leaves the element
     * 
     * @param element The element attached to this behavior 
     * @param delta Number of millisecond elapsed since last update
     * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
     * @param guistate The state of the GUI at this update
     *
     * @return true if this behavior should be retained, false otherwise
     *
     */
    bool onFinish( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
        return true;
    }

    /*******************************************************************************************/
    /**
     * @brief  Called when the behavior ceases, whether by its own indicate or externally
     * 
     * @param element The element attached to this behavior 
     *
     */
    void cleanUp( Element@ element ) {
        
    }

}

/*******************************************************************************************/
/**
 * @brief  Attachable behavior base class - called on mouse down 
 * 
 */
class MouseClickBehavior {

    /*******************************************************************************************/
    /**
     * @brief  Called when the mouse button is pressed on element
     * 
     * @param element The element attached to this behavior 
     * @param delta Number of millisecond elapsed since last update
     * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
     * @param guistate The state of the GUI at this update
     *
     * @return true if this behavior should be retained, false otherwise
     *
     */
    bool onDown( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
        return true;
    }

    /*******************************************************************************************/
    /**
     * @brief  Called when the mouse button continues to be pressed on an element
     * 
     * @param element The element attached to this behavior 
     * @param delta Number of millisecond elapsed since last update
     * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
     * @param guistate The state of the GUI at this update
     *
     * @return true if this behavior should be retained, false otherwise
     *
     */
    bool onStillDown( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
        return true;
    }

    /*******************************************************************************************/
    /**
     * @brief  Called when the mouse button is released on element
     * 
     * @param element The element attached to this behavior 
     * @param delta Number of millisecond elapsed since last update
     * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
     * @param guistate The state of the GUI at this update
     *
     * @return true if this behavior should be retained, false otherwise
     *
     */
    bool onUp( Element@ element, uint64 delta, ivec2 drawOffset, GUIState& guistate ) {
        return true;
    }

    /*******************************************************************************************/
    /**
     * @brief  Called when the behavior ceases, whether by its own indicate or externally
     * 
     * @param element The element attached to this behavior 
     *
     */
    void cleanUp( Element@ element ) {
        
    }

}

class UpdateBehaviorInstance { 
    UpdateBehaviorInstance() {
        key = "";
    }

    UpdateBehaviorInstance( string _key, UpdateBehavior@ _elem ) {
        key = _key;
        @elem = @_elem; 
    }

    string key;
    UpdateBehavior@ elem;
}

class UpdateBehaviors {
    array<UpdateBehaviorInstance@> elems;

    void deleteAll() {
        elems.removeRange(0,elems.length());
    }

    bool exists( string key ) {
        for( uint i = 0; i < elems.length(); i++ ) {
            if( elems[i].key == key ) {
                return true;
            }
        }
        return false;
    }

    array<string> getKeys() {
        array<string> keys;
        for( uint i = 0; i < elems.length(); i++ ) {
            keys.insertLast(elems[i].key);
        }
        return keys;
    }

    UpdateBehavior@ Get( string key ) {
        for( uint i = 0; i < elems.length(); i++ ) {
            if( elems[i].key == key ) {
                return elems[i].elem;
            }
        }
        return null;
    }

    void Set( string key, UpdateBehavior@ elem ) {
        bool found = false;
        for( uint i = 0; i < elems.length(); i++ ) {
            if( elems[i].key == key ) {
                found = true;
                elems[i].elem = elem;
            }
        }
        if( found == false ) {
            elems.insertLast(UpdateBehaviorInstance(key,elem));
        }
    }

    void Remove( string key ) {
        for( uint i = 0; i < elems.length(); i++ ) {
            if( elems[i].key == key ) {
                elems.removeAt(i);
                i--;
            } 
        }
    }

    UpdateBehavior@ GetI( int i ) {
        return elems[i].elem;
    }

    string GetKeyI( int i ) {
        return elems[i].key;
    }

    uint Count() {
        return elems.length();
    }
}

class MouseOverBehaviorInstance { 
    MouseOverBehaviorInstance() {
        key = "";
    }
    
    MouseOverBehaviorInstance(string _key, MouseOverBehavior@ _elem) {
        key = _key;
        @elem = @_elem; 
    }

    string key;
    MouseOverBehavior@ elem;
}

class MouseOverBehaviors {
    array<MouseOverBehaviorInstance@> elems;

    void deleteAll() {
        elems.removeRange(0,elems.length());
    }

    bool exists( string key ) {
        for( uint i = 0; i < elems.length(); i++ ) {
            if( elems[i].key == key ) {
                return true;
            }
        }
        return false;
    }

    array<string> getKeys() {
        array<string> keys;
        for( uint i = 0; i < elems.length(); i++ ) {
            keys.insertLast(elems[i].key);
        }
        return keys;
    }

    MouseOverBehavior@ Get( string key ) {
        for( uint i = 0; i < elems.length(); i++ ) {
            if( elems[i].key == key ) {
                return elems[i].elem;
            }
        }
        return null;
    }

    void Set( string key, MouseOverBehavior@ elem ) {
        bool found = false;
        for( uint i = 0; i < elems.length(); i++ ) {
            if( elems[i].key == key ) {
                found = true;
                elems[i].elem = elem;
            }
        }
        if( found == false ) {
            elems.insertLast(MouseOverBehaviorInstance(key,elem));
        }
    }

    void Remove( string key ) {
        for( uint i = 0; i < elems.length(); i++ ) {
            if( elems[i].key == key ) {
                elems.removeAt(i);
                i--;
            } 
        }
    }

    MouseOverBehavior@ GetI( int i ) {
        return elems[i].elem;
    }

    string GetKeyI( int i ) {
        return elems[i].key;
    }

    uint Count() {
        return elems.length();
    }
}

class MouseClickBehaviorInstance { 
    MouseClickBehaviorInstance() { 
        key = "";
    }

    MouseClickBehaviorInstance( string _key, MouseClickBehavior@ _elem ) { 
        key = _key;
        @elem = @_elem;
    }

    string key;
    MouseClickBehavior@ elem;
}

class LeftMouseClickBehaviors {
    array<MouseClickBehaviorInstance@> elems;

    void deleteAll() {
        elems.removeRange(0,elems.length());
    }

    bool exists( string key ) {
        for( uint i = 0; i < elems.length(); i++ ) {
            if( elems[i].key == key ) {
                return true;
            }
        }
        return false;
    }

    array<string> getKeys() {
        array<string> keys;
        for( uint i = 0; i < elems.length(); i++ ) {
            keys.insertLast(elems[i].key);
        }
        return keys;
    }

    MouseClickBehavior@ Get( string key ) {
        for( uint i = 0; i < elems.length(); i++ ) {
            if( elems[i].key == key ) {
                return elems[i].elem;
            }
        }
        return null;
    }

    void Set( string key, MouseClickBehavior@ elem ) {
        bool found = false;
        for( uint i = 0; i < elems.length(); i++ ) {
            if( elems[i].key == key ) {
                found = true;
                elems[i].elem = elem;
            }
        }
        if( found == false ) {
            elems.insertLast(MouseClickBehaviorInstance(key,elem));
        }
    }

    void Remove( string key ) {
        for( uint i = 0; i < elems.length(); i++ ) {
            if( elems[i].key == key ) {
                elems.removeAt(i);
                i--;
            } 
        }
    }

    MouseClickBehavior@ GetI( int i ) {
        return elems[i].elem;
    }

    string GetKeyI( int i ) {
        return elems[i].key;
    }

    uint Count() {
        return elems.length();
    }
}

/*******************************************************************************************/
/**
 * @brief  Base class for all AdHoc Gui elements
 *
 */
class Element {

    ivec2 size;             // dimensions of the actual region (GUI space)    
    ivec2 defaultSize;      // What size (if any) should this element become once 'reset'
    ivec2 boundarySize;     // length and width of the maximum extent of this element (GUI Space) 
    ivec2 boundaryOffset;   // upper left coordinate relative to the containing boundary (GUI space)
    ivec2 drawDisplacement; // Is this element being drawn somewhere other than where it 'lives' (mostly for tweening)
    BoundaryAlignment alignmentX; // How to position this element versus a boundary that's bigger than the size
    BoundaryAlignment alignmentY; // How to position this element versus a boundary that's bigger than the size
    int paddingU;           // (minimum) Padding between the element and the upper boundary 
    int paddingD;           // (minimum) Padding between the element and the lower boundary 
    int paddingL;           // (minimum) Padding between the element and the left boundary 
    int paddingR;           // (minimum) Padding between the element and the right boundary 

    int zOrdering = 1;      // At what point in the rendering process does this get drawn in

    string name;            // name to refer to this object by -- incumbent on the programmer to make sure they're unique
    
    Element@ parent;        // null if 'root'
    GUI@ owner;             // what GUI owns this element

    int numBehaviors = 0; // Counter for unique behavior names

    UpdateBehaviors                  updateBehaviors;            // update behaviors
    MouseOverBehaviors               mouseOverBehaviors;         // mouse over behaviors
    LeftMouseClickBehaviors          leftMouseClickBehaviors;    // mouse up behaviors

    bool show;              // should this element be rendered?
    vec4 color;             // if this element is colored, what color is it? -- other elements may define further colors
    bool isColorEffected;   // is there a temporary color change?
    vec4 effectColor;       // if the color is temp
    bool border;            // should this element have a border?
    int borderSize;         // how thick is this border (in GUI space pixels)
    vec4 borderColor;       // color for the border 

    bool mouseOver;         // has mouse been over this element

    /*******************************************************************************************/
    /**
     * @brief  Initializes the element (called internally)
     * 
     */
     void init() {
        
        name = "";

        size = ivec2( AH_UNDEFINEDSIZE,AH_UNDEFINEDSIZE );
        defaultSize = ivec2( AH_UNDEFINEDSIZE,AH_UNDEFINEDSIZE );
        boundarySize = ivec2( AH_UNDEFINEDSIZE,AH_UNDEFINEDSIZE );
        boundaryOffset = ivec2( 0, 0 );
        drawDisplacement = ivec2( 0, 0);

        @parent = null;
        @owner = null;

        setPadding( 0 );

        show = true;
        color = vec4(1.0,1.0,1.0,1.0);

        isColorEffected = false;
        effectColor = vec4(1.0,1.0,1.0,1.0);
        border = false;
        borderSize = 1;
        borderColor = vec4(1.0,1.0,1.0,1.0);

        mouseOver = false;

        // By default every element is in the center of its container
        alignmentX = BACenter;
        alignmentY = BACenter;

     }

    /*******************************************************************************************/
    /**
     * @brief  Constructor
     * 
     * @param _name Name for this object (incumbent on the programmer to make sure they're unique)
     *
     */
    Element( string _name ) {
        init();
        name = _name;
    }

    /*******************************************************************************************/
    /**
     * @brief  Constructor
     * 
     */
    Element() {
        init();
    }
    
    /*******************************************************************************************/
    /**
     * @brief  Gets the name of the type of this element — for autonaming and debugging
     * 
     * @returns name of the element type as a string
     *
     */
    string getElementTypeName() {
        return "Element";
    }

    /*******************************************************************************************/
    /**
     * @brief  Set the color for the element
     *  
     * @param _R Red 
     * @param _G Green
     * @param _B Blue
     * @param _A Alpha
     *
     */
    void setColor( float _R, float _G, float _B, float _A = 1.0f ) {
        color = vec4( _R, _G, _B, _A );
    } 

    /*******************************************************************************************/
    /**
     * @brief  Set the color for the element
     *  
     * @param _color 4 component vector for the color
     *
     */
    void setColor( vec4 _color ) {
        color = _color; 
    } 

    /*******************************************************************************************/
    /**
     * @brief  Gets the current color
     * 
     * If the color is effected, it'll return the effected color
     *
     * @returns 4 component vector of the color
     *
     */
     vec4 getColor() {
        if(isColorEffected ) {
            return effectColor;    
        }
        else {
            return color;
        }
     }

     /*******************************************************************************************/
    /**
     * @brief  Gets the current color -- ignoring the effect color
     *
     * @returns 4 component vector of the color
     *
     */
     vec4 getBaseColor() {
        return color;
     }

    /*******************************************************************************************/
    /**
     * @brief  Set the effect color for the element
     *  
     * @param _R Red 
     * @param _G Green
     * @param _B Blue
     * @param _A Alpha
     *
     */
    void setEffectColor( float _R, float _G, float _B, float _A = 1.0f ) {
        effectColor = vec4( _R, _G, _B, _A );
    } 

    /*******************************************************************************************/
    /**
     * @brief  Set the effect color for the element
     *  
     * @param _color 4 component vector for the color
     *
     */
    void setEffectColor( vec4 _color ) {
        isColorEffected = true;
        effectColor = _color;
    } 

    /*******************************************************************************************/
    /**
     * @brief  Gets the effect current color
     * 
     * @returns 4 component vector of the color
     *
     */
     vec4 getEffectColor() {
        if( !isColorEffected ) {
            return color;
        }
        return effectColor;
     }

    /*******************************************************************************************/
    /**
     * @brief Clears any effect color (reseting to the base)
     *
     */
     void clearColorEffect() {
        isColorEffected = false;
     }

    /*******************************************************************************************/
    /**
     * @brief  Sets the red value
     * 
     * @param value Color value  
     *
     */
     void setR( float value ) {
        color.x = value;
     }

    /*******************************************************************************************/
    /**
     * @brief  Gets the red value
     * 
     * @returns Color value
     *
     */
     float getR() {
        return color.x;
     }

    /*******************************************************************************************/
    /**
     * @brief Sets the green value
     * 
     * @param value Color value  
     *
     */
     void setG( float value ) {
        color.y = value;
     }

    /*******************************************************************************************/
    /**
     * @brief Gets the green value
     * 
     * @returns Color value
     *
     */
     float getG() {
        return color.y;
     }

    /*******************************************************************************************/
    /**
     * @brief Sets the blue value
     * 
     * @param value Color value  
     *
     */
     void setB( float value ) {
        color.z = value;
     }

    /*******************************************************************************************/
    /**
     * @brief Gets the blue value
     * 
     * @returns Color value
     *
     */
     float getB() {
        return color.y;
     }

    /*******************************************************************************************/
    /**
     * @brief Sets the alpha value
     * 
     * @param value Color value  
     *
     */
     void setAlpha( float value ) {
        color.a = value;
     }

    /*******************************************************************************************/
    /**
     * @brief Gets the alpha value
     * 
     * @returns Color value
     *
     */
     float getAlpha() {
        return color.a;
     }

    /*******************************************************************************************/
    /**
     * @brief  Sets the effect red value
     * 
     * @param value Color value  
     *
     */
     void setEffectR( float value ) {
        if( !isColorEffected ) {
            effectColor = color;
        }
        isColorEffected = true;
        effectColor.x = value;
     }

    /*******************************************************************************************/
    /**
     * @brief  Gets the effect red value
     * 
     * @returns Color value
     *
     */
    float getEffectR() {
        if( !isColorEffected ) {
            return color.x;
        }
        else {
            return effectColor.x;
        }
    }

    /*******************************************************************************************/
    /**
     * @brief  Clear effect red value
     * 
     */
    void clearEffectR() {
        isColorEffected = false;
        effectColor.x = color.x;
    }

    /*******************************************************************************************/
    /**
     * @brief Sets the effect green value
     * 
     * @param value Color value  
     *
     */
     void setEffectG( float value ) {
        if( !isColorEffected ) {
            effectColor = color;
        }
        isColorEffected = true;
        effectColor.y = value;
     }

    /*******************************************************************************************/
    /**
     * @brief Gets the effect green value
     * 
     * @returns Color value
     *
     */
     float getEffectG() {
        if( !isColorEffected ) {
            return color.y;
        }
        else {
            return effectColor.y;
        }
     }
    
    /*******************************************************************************************/
    /**
     * @brief  Clear effect green value
     * 
     */
    void clearEffectG() {
        isColorEffected = false;
        effectColor.y = color.y;
    }

    /*******************************************************************************************/
    /**
     * @brief Sets the blue value
     * 
     * @param value Color value  
     *
     */
     void setEffectB( float value ) {
        if( !isColorEffected ) {
            effectColor = color;
        }
        isColorEffected = true;
        effectColor.z = value;
     }

    /*******************************************************************************************/
    /**
     * @brief Gets the blue value
     * 
     * @returns Color value
     *
     */
     float getEffectB() {
        if( !isColorEffected ) {
            return color.z;
        }
        else {
            return effectColor.z;
        }
     }

    /*******************************************************************************************/
    /**
     * @brief  Clear effect blue value
     * 
     */
    void clearEffectB() {
        isColorEffected = false;
        effectColor.z = color.z;
    }

    /*******************************************************************************************/
    /**
     * @brief Sets the alpha value
     * 
     * @param value Color value  
     *
     */
     void setEffectAlpha( float value ) {
        if( !isColorEffected ) {
            effectColor = color;
        }
        isColorEffected = true;
        effectColor.a = value;
     }

    /*******************************************************************************************/
    /**
     * @brief Gets the alpha value
     * 
     * @returns Color value
     *
     */
     float getEffectAlpha() {
        if( !isColorEffected ) {
            return color.z;
        }
        else {
            return effectColor.z;
        }
     }

    /*******************************************************************************************/
    /**
     * @brief  Clear effect alpha value
     * 
     */
    void clearEffectAlpha() {
        isColorEffected = false;
        effectColor = color;
    }


    /*******************************************************************************************/
    /**
     * @brief  Should this element have a border
     *  
     * @param _border Show this border or not
     *
     */
     void showBorder( bool _border = true ) {
        border = _border;
     }

    /*******************************************************************************************/
    /**
     * @brief  Sets the border thickness
     * 
     * @param thickness Thickness of the border in GUI space pixels 
     *
     */
     void setBorderSize( int _borderSize ) {
        borderSize = _borderSize;
     }


    /*******************************************************************************************/
    /**
     * @brief  Set the color for the border
     *  
     * @param _R Red 
     * @param _G Green
     * @param _B Blue
     * @param _A Alpha
     *
     */
    void setBorderColor( float _R, float _G, float _B, float _A = 1.0f ) {
        borderColor = vec4( _R, _G, _B, _A );
    } 

    /*******************************************************************************************/
    /**
     * @brief  Set the color for the border
     *  
     * @param _color 4 component vector for the color
     *
     */
    void setBorderColor( vec4 _color ) {
        borderColor = _color;
    } 

    /*******************************************************************************************/
    /**
     * @brief  Gets the current border color
     * 
     * @returns 4 component vector of the color
     *
     */
     vec4 getBorderColor() {
        return borderColor;
     }

    /*******************************************************************************************/
    /**
     * @brief  Sets the border red value
     * 
     * @param value Color value  
     *
     */
     void setBorderR( float value ) {
        borderColor.x = value;
     }

    /*******************************************************************************************/
    /**
     * @brief  Gets the border red value
     * 
     * @returns Color value
     *
     */
     float getBorderR() {
        return borderColor.x;
     }

    /*******************************************************************************************/
    /**
     * @brief Sets the border green value
     * 
     * @param value Color value  
     *
     */
     void setBorderG( float value ) {
        borderColor.y = value;
     }

    /*******************************************************************************************/
    /**
     * @brief Gets the border green value
     * 
     * @returns Color value
     *
     */
     float getBorderG() {
        return borderColor.y;
     }

    /*******************************************************************************************/
    /**
     * @brief Sets the border blue value
     * 
     * @param value Color value  
     *
     */
     void setBorderB( float value ) {
        borderColor.z = value;
     }

    /*******************************************************************************************/
    /**
     * @brief Gets the border blue value
     * 
     * @returns Color value
     *
     */
     float getBorderB() {
        return borderColor.y;
     }

    /*******************************************************************************************/
    /**
     * @brief Sets the border alpha value
     * 
     * @param value Color value  
     *
     */
     void setBorderAlpha( float value ) {
        borderColor.a = value;
     }

    /*******************************************************************************************/
    /**
     * @brief Gets the border alpha value
     * 
     * @returns Color value
     *
     */
     float getBorderAlpha() {
        return borderColor.a;
     }

    /*******************************************************************************************/
    /**
     * @brief  Sets the z ordering (order of drawing, higher is drawing on top of lower)
     * 
     * @param z new Z ordering value (expected to be greater then 0 and the parent container)
     *
     */
     void setZOrdering( int z ) {
        zOrdering = z;
     }

    /*******************************************************************************************/
    /**
     * @brief  Gets the z ordering (order of drawing - higher is drawing on top of lower)
     * 
     * @returns current Z ordering value
     *
     */
     int getZOrdering() {
        return zOrdering;
     }

    /*******************************************************************************************/
    /**
     * @brief  Set the z ordering of this element to be higher than the given element
     *  
     * @param element Element to be below this one
     *
     */
     void renderAbove( Element@ element ) {
        setZOrdering( element.getZOrdering() + 1 );
     }

    /*******************************************************************************************/
    /**
     * @brief  Set the z ordering of this element to be lower than the given element
     *  
     * (note that if the element parameter has a z value within 1 of the parent container
     *  this element will be assigned to the same value, which may not look nice )
     *
     * @param element Element to be below this one
     *
     */
     void renderBelow( Element@ element ) {

        int minZ;
        // See if we're a root container
        if( parent !is null ) {
            // not a root
            minZ = parent.getZOrdering();
        }
        else {
            // we're a root
            minZ = 1;            
        }

        // set the z ordering, respecting the derived minimum
        setZOrdering( max( minZ, element.getZOrdering() - 1 ) );

     }


    /*******************************************************************************************/
    /**
     * @brief  Show or hide this element
     *  
     * @param _show Show this element or not
     *
     */
     void setVisible( bool _show ) {
        show = _show;
     }

    /*******************************************************************************************/
    /**
     * @brief  Draw a box (in *screen* coordinates) -- used internally
     * 
     */
    void drawBox( ivec2 boxPos, ivec2 boxSize, vec4 boxColor, int zOrder, ivec2 currentClipPos, ivec2 currentClipSize ) {

        IMUIImage boxImage("Data/Textures/ui/whiteblock.tga");

        boxImage.setPosition( vec3( float(boxPos.x), float(boxPos.y), float(zOrder) ) );
        boxImage.setColor( boxColor );
        boxImage.setRenderSize( vec2( float(boxSize.x), float(boxSize.y) ) );

        if( currentClipSize.x != AH_UNDEFINEDSIZE && currentClipSize.y != AH_UNDEFINEDSIZE ){

            ivec2 screenClipPos = screenMetrics.GUIToScreen( currentClipPos );

            ivec2 screenClipSize( (int(float(currentClipSize.x)*screenMetrics.GUItoScreenXScale + 0.5)), 
                                  (int(float(currentClipSize.y)*screenMetrics.GUItoScreenYScale + 0.5)));

            boxImage.setClipping( vec2( float(screenClipPos.x), float(screenClipPos.y) ), 
                                  vec2( float(screenClipSize.x), float(screenClipSize.y) ) );
        }

        AHGUI_IMUIContext.Get().queueImage( boxImage );
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

        // see if we're visible 
        if( !show ) return;

        // See if we're supposed to draw a border
        if( border ) {

            ivec2 borderCornerUL = drawOffset + drawDisplacement + boundaryOffset - ivec2( paddingL, paddingU );
            ivec2 borderCornerLR = drawOffset + drawDisplacement + boundaryOffset + size - ivec2(1,1) + ivec2( paddingR, paddingD );

            ivec2 screenCornerUL = screenMetrics.GUIToScreen( borderCornerUL );
            ivec2 screenCornerLR = screenMetrics.GUIToScreen( borderCornerLR );

            // figure out the thickness in screen pixels (minimum 1)
            int thickness = max( int( float( borderSize ) * screenMetrics.GUItoScreenXScale ), 1 );

            // top 
            drawBox( screenCornerUL, 
                     ivec2( screenCornerLR.x - screenCornerUL.x, thickness ),
                     borderColor,
                     zOrdering + 1, 
                     currentClipPos, 
                     currentClipSize );

            // bottom 
            drawBox( ivec2( screenCornerUL.x, screenCornerLR.y - thickness ), 
                     ivec2( screenCornerLR.x - screenCornerUL.x, thickness ),
                     borderColor,
                     zOrdering + 1,  
                     currentClipPos, 
                     currentClipSize );

            // left
            drawBox( ivec2( screenCornerUL.x, screenCornerUL.y + thickness ), 
                     ivec2( thickness, screenCornerLR.y - screenCornerUL.y - (2 * thickness) ),
                     borderColor,
                     zOrdering + 1,  
                     currentClipPos, 
                     currentClipSize );
            // right
            drawBox( ivec2( screenCornerLR.x - thickness, screenCornerUL.y + thickness ), 
                     ivec2( thickness, screenCornerLR.y - screenCornerUL.y - (2 * thickness) ),
                     borderColor,
                     zOrdering + 1,  
                     currentClipPos, 
                     currentClipSize );       

        }

    }

    /*******************************************************************************************/
    /**
     * @brief  Checks to see if a point is inside this element
     * 
     * @param drawOffset The upper left hand corner of where the boundary is drawn
     * @param point point in question
     *
     * @returns true if inside, false otherwise
     *
     */
    bool pointInElement( ivec2 drawOffset, ivec2 point ) {

        ivec2 UL = drawOffset + boundaryOffset;
        ivec2 LR = UL + size;

        if( UL.x <= point.x && UL.y <= point.y &&
            LR.x >  point.x && LR.y > point.y ) {
            return true;
        }
        else {
            return false;
        }

    }

    /*******************************************************************************************/
    /**
     * @brief  Add an update behavior
     *  
     * @param behavior Handle to behavior in question
     * @param behaviorName name to identify the behavior
     *
     */
     void addUpdateBehavior( UpdateBehavior@ behavior, string behaviorName = "" ) {
        // if they haven't given us a name, generate one
        if( behaviorName == "" ) {
            behaviorName = "behavior" + numBehaviors;
            numBehaviors++;
        }
        else {
            removeUpdateBehavior( behaviorName );
        }

        updateBehaviors.Set(behaviorName, @behavior);
     }

    /*******************************************************************************************/
    /**
     * @brief  Removes a named update behavior
     * 
     * @param behaviorName name to identify the behavior
     *
     * @returns true if there was a behavior to remove, false otherwise
     *
     */
     bool removeUpdateBehavior( string behaviorName ) {
        // see if there is already a behavior with this name
        if( updateBehaviors.exists(behaviorName) ) {
            // if so clean it up if its been initialized 

            UpdateBehavior@ updater = updateBehaviors.Get(behaviorName);
            
            if( updater.initialized ) {
                updater.cleanUp( this );
            }

            // and delete it
            updateBehaviors.Remove( behaviorName );

            return true;
        }
        else {
            // Let the caller know
            return false;
        }

     }

    /*******************************************************************************************/
    /**
     * @brief Indicates if a behavior exists, can be used to see if its finished.
     * 
     * @param behaviorName name to identify the behavior
     *
     * @returns true if there was a behavior
     *
     */
    bool hasUpdateBehavior(string behaviorName)
    {
        return updateBehaviors.exists(behaviorName);
    }
    

    /*******************************************************************************************/
    /**
     * @brief  Clear update behaviors
     *
     */
     void clearUpdateBehaviors() {

        // iterate through all the behaviors and clean them up
        for( uint k = 0; k < updateBehaviors.Count(); k++ ) {
            UpdateBehavior@ updater = updateBehaviors.GetI(k);
            updater.cleanUp( this );
        }

        updateBehaviors.deleteAll();

     }

    /*******************************************************************************************/
    /**
     * @brief  Add a mouse over behavior
     *  
     * @param behavior Handle to behavior in question
     * @param behaviorName name to identify the behavior
     *
     */
     void addMouseOverBehavior( MouseOverBehavior@ behavior, string behaviorName = "" ) {
        
        // if they haven't given us a name, generate one
        if( behaviorName == "" ) {
            behaviorName = "behavior" + numBehaviors;
            numBehaviors++;
        }
        else {
            removeMouseOverBehavior( behaviorName );
        }

        mouseOverBehaviors.Set(behaviorName, @behavior);
     }

    /*******************************************************************************************/
    /**
     * @brief  Removes a named update behavior
     * 
     * @param behaviorName name to identify the behavior
     *
     * @returns true if there was a behavior to remove, false otherwise
     *
     */
     bool removeMouseOverBehavior( string behaviorName ) {
        // see if there is already a behavior with this name
        if( mouseOverBehaviors.exists(behaviorName) ) {
            // if so clean it up 
            MouseOverBehavior@ updater = mouseOverBehaviors.Get(behaviorName);
            updater.cleanUp( this );

            // and delete it
            mouseOverBehaviors.Remove( behaviorName );

            return true;
        }
        else {
            // Let the caller know
            return false;
        }
     }

    /*******************************************************************************************/
    /**
     * @brief  Clear mouse over behaviors
     *
     */
     void clearMouseOverBehaviors() {

        // iterate through all the behaviors and clean them up
        for( uint k = 0; k < mouseOverBehaviors.Count(); k++ ) {
            MouseOverBehavior@ updater = mouseOverBehaviors.GetI(k);
            updater.cleanUp( this );
        }

        mouseOverBehaviors.deleteAll();

     }

    /*******************************************************************************************/
    /**
     * @brief  Add a click behavior
     *  
     * @param behavior Handle to behavior in question
     * @param behaviorName name to identify the behavior
     *
     */
    void addLeftMouseClickBehavior( MouseClickBehavior@ behavior, string behaviorName = "" ) {
        
        // if they haven't given us a name, generate one
        if( behaviorName == "" ) {
            behaviorName = "behavior" + numBehaviors;
            numBehaviors++;
        }
        else {
            removeLeftMouseClickBehavior( behaviorName );
        }

        leftMouseClickBehaviors.Set(behaviorName, @behavior);
        
    }



    /*******************************************************************************************/
    /**
     * @brief  Removes a named click behavior
     * 
     * @param behaviorName name to identify the behavior
     *
     * @returns true if there was a behavior to remove, false otherwise
     *
     */
     bool removeLeftMouseClickBehavior( string behaviorName ) {
        // see if there is already a behavior with this name
        if( leftMouseClickBehaviors.exists(behaviorName) ) {
            // if so clean it up 
            MouseClickBehavior@ updater = leftMouseClickBehaviors.Get(behaviorName);
            updater.cleanUp( this );

            // and delete it
            leftMouseClickBehaviors.Remove( behaviorName );

            return true;
        }
        else {
            // Let the caller know
            return false;
        }

     }

    /*******************************************************************************************/
    /**
     * @brief  Clear mouse over behaviors
     *
     */
     void clearLeftMouseClickBehaviors() {
        for( uint k = 0; k < leftMouseClickBehaviors.Count(); k++ ) {
            MouseClickBehavior@ updater = leftMouseClickBehaviors.GetI(k);
            updater.cleanUp( this );
        }
        leftMouseClickBehaviors.deleteAll(); 
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
        // Update behaviors
        {
            for( uint k = 0; k < updateBehaviors.Count(); k++ ) {
                UpdateBehavior@ updater = updateBehaviors.GetI(k);
                
                // See if this behavior has been initialized
                if( !updater.initialized ) {

                    if( !updater.initialize( this, delta, drawOffset, guistate ) ) {
                        // If the behavior has indicated it should not begin remove it
                        removeUpdateBehavior( updateBehaviors.GetKeyI(k) );    
                        continue;
                    }
                    else {
                        updater.initialized = true;
                    }
                }

                if( !updater.update( this, delta, drawOffset, guistate ) ) {
                    // If the behavior has indicated it is done
                    removeUpdateBehavior( updateBehaviors.GetKeyI(k) );
                }
            }
        }

        // Now do mouse behaviors 

        // Mouse overs

        if( pointInElement( drawOffset, guistate.mousePosition ) ) {
            if( !mouseOver ) {
                mouseOver = true;

                // Update behaviors 
                for( uint k = 0; k < mouseOverBehaviors.Count(); k++ ) {
                    MouseOverBehavior@ behavior = mouseOverBehaviors.GetI(k);
                    behavior.onStart( this, delta, drawOffset, guistate );
                }
            } else {
                for( uint k = 0; k < mouseOverBehaviors.Count(); k++ ) {
                    MouseOverBehavior@ behavior = mouseOverBehaviors.GetI(k);
                    behavior.onContinue( this, delta, drawOffset, guistate );
                }
            }

            // Mouse click status
            switch( guistate.leftMouseState ) {
            
            case kMouseDown: {
                for( uint k = 0; k < leftMouseClickBehaviors.Count(); k++ ) {
                    MouseClickBehavior@ behavior = leftMouseClickBehaviors.GetI(k);

                    if( !behavior.onDown( this, delta, drawOffset, guistate ) ) {
                        // If the behavior has indicated it is done
                        removeLeftMouseClickBehavior( leftMouseClickBehaviors.GetKeyI(k) );
                    }
                }
            }
            break; 
            
            case kMouseStillDown: {
                for( uint k = 0; k < leftMouseClickBehaviors.Count(); k++ ) {
                    MouseClickBehavior@ behavior = leftMouseClickBehaviors.GetI(k);

                    if( !behavior.onStillDown( this, delta, drawOffset, guistate ) ) {
                        // If the behavior has indicated it is done
                        removeLeftMouseClickBehavior( leftMouseClickBehaviors.GetKeyI(k) );
                    }
                }
            }
            break;

            case kMouseUp: {
                for( uint k = 0; k < leftMouseClickBehaviors.Count(); k++ ) {
                    MouseClickBehavior@ behavior = leftMouseClickBehaviors.GetI(k);

                    if( !behavior.onUp( this, delta, drawOffset, guistate ) ) {
                        // If the behavior has indicated it is done
                        removeLeftMouseClickBehavior( leftMouseClickBehaviors.GetKeyI(k) );
                    }
                }

                // Consider this no longer hovering
                mouseOver = false;
                {
                    for( uint k = 0; k < mouseOverBehaviors.Count(); k++ ) {
                        MouseOverBehavior@ behavior = mouseOverBehaviors.GetI(k);

                        if( !behavior.onFinish( this, delta, drawOffset, guistate ) ) {
                            // If the behavior has indicated it is done
                            removeMouseOverBehavior( mouseOverBehaviors.GetKeyI(k) );
                        }
                    }
                }
            }
            break; 

            case kMouseStillUp: 
            default:

            break;
            
            }

        }
        else {
            // See if this is an 'exit'
            if( mouseOver )
            {
                for( uint k = 0; k < mouseOverBehaviors.Count(); k++ ) {
                    MouseOverBehavior@ behavior = mouseOverBehaviors.GetI(k);

                    if( !behavior.onFinish( this, delta, drawOffset, guistate ) ) {
                        // If the behavior has indicated it is done
                        removeMouseOverBehavior( mouseOverBehaviors.GetKeyI(k) );
                    }
                }
                mouseOver = false;
            }
        }
    }

    /*******************************************************************************************/
    /**
     * @brief  When this element is resized, moved, etc propagate this signal upwards
     * 
     */
     void onRelayout() {

        if( owner !is null ) {
            owner.onRelayout();
        }
     
     }

    /*******************************************************************************************/
    /**
     * @brief  When a resize, move, etc has happened do whatever is necessary
     * 
     */
     void doRelayout() {
        
        // Make sure the boundary is up to date and sensible
        checkBoundary();
        resetAlignmentInBoundary();

     }

    /*******************************************************************************************/
    /**
     * @brief  Do whatever is necessary when the resolution changes
     *
     */
     void doScreenResize()  {
        // Nothing to do in the base class
     }

    /*******************************************************************************************/
    /**
     * @brief Set the name of this element
     * 
     * @param _name New name (incumbent on the programmer to make sure they're unique)
     *
     */
     void setName( string _name ) {
        
        name = _name;
     
     }

    /*******************************************************************************************/
    /**
     * @brief Gets the name of this element
     * 
     * @returns name of this element
     *
     */
     string getName() {
        return name;
     }


    /*******************************************************************************************/
    /**
     * @brief  Set the padding for each direction on the element
     *
     * AH_UNDEFINEDSIZE will cause no change 
     * 
     * @param U (minimum) Padding between the element and the upper boundary  
     * @param D (minimum) Padding between the element and the lower boundary  
     * @param L (minimum) Padding between the element and the left boundary  
     * @param R (minimum) Padding between the element and the right boundary  
     *
     */
    void setPadding( int U, int D, int L, int R) {

        if( U != AH_UNDEFINEDSIZE ) { paddingU = U; }
        if( D != AH_UNDEFINEDSIZE ) { paddingD = D; }
        if( L != AH_UNDEFINEDSIZE ) { paddingL = L; }
        if( R != AH_UNDEFINEDSIZE ) { paddingR = R; }

        onRelayout();

    }

    /*******************************************************************************************/
    /**
     * @brief  Set the padding for all directions on the element
     *
     * @param paddingSize The number of pixels (in GUI space) to add to the padding on all sides
     *
     */
    void setPadding( int paddingSize ) {

        paddingU = paddingSize; 
        paddingD = paddingSize; 
        paddingL = paddingSize; 
        paddingR = paddingSize; 

        onRelayout();

    }

    /*******************************************************************************************/
    /**
     * @brief  Sets the drawing displacement (mostly used for tweening)
     * 
     * @param newDisplacement newValues for the displacement
     *
     */
     void setDisplacement( ivec2 newDisplacement = ivec2(0,0) ) {
        drawDisplacement = newDisplacement;
     }

    /*******************************************************************************************/
    /**
     * @brief  Sets the drawing displacement x component (mostly used for tweening)
     * 
     * @param newDisplacement newValues for the displacement
     *
     */
     void setDisplacementX( int newDisplacement = 0 ) {
        drawDisplacement.x = newDisplacement;
     }

    /*******************************************************************************************/
    /**
     * @brief  Sets the drawing displacement y component (mostly used for tweening)
     * 
     * @param newDisplacement newValues for the displacement
     *
     */
     void setDisplacementY( int newDisplacement = 0 ) {
        drawDisplacement.y = newDisplacement;
     }

    /*******************************************************************************************/
    /**
     * @brief  Gets the drawing displacement (mostly used for tweening)
     * 
     * @returns Displacement vector
     *
     */
     ivec2 getDisplacement( ivec2 newDisplacement = ivec2(0,0) ) {
        return drawDisplacement;
     }

    /*******************************************************************************************/
    /**
     * @brief  Gets the drawing displacement x component (mostly used for tweening)
     * 
     * @returns Displacement value
     *
     */
     int getDisplacementX() {
        return drawDisplacement.x;
     }

    /*******************************************************************************************/
    /**
     * @brief  Gets the drawing displacement y component (mostly used for tweening)
     * 
     * @returns Displacement value
     *
     */
     int getDisplacementY() {
        return drawDisplacement.y;
     }

    /*******************************************************************************************/
    /**
     * @brief  Make sure that the element isn't too big for the max boundary - called internally
     * 
     * @param throwErorr Should this throw an error if the boundary is exceeded
     *
     */
     bool checkBoundary( bool throwErorr = true ) {
        
        // first check if we have undefined boundary sizes (probably shouldn't happen)
        if( boundarySize.x == AH_UNDEFINEDSIZE ) boundarySize.x = size.x + paddingL + paddingR;
        if( boundarySize.y == AH_UNDEFINEDSIZE ) boundarySize.y = size.y + paddingU + paddingD;

        // The boundary is defined to always be at least as big as the element it contains
        // Make sure this is the case
        if( size.x + paddingL + paddingR > boundarySize.x ) {
            boundarySize.x = size.x + paddingL + paddingR;
        }

        if( size.y + paddingU + paddingD > boundarySize.y ) {
            boundarySize.y = size.y + paddingU + paddingD;
        }
     
        return true;

     }

    /*******************************************************************************************/
    /**
     * @brief Resizes the boundary (based solely on the element dimension)
     * 
     */
     void setBoundarySize() {

        boundarySize = size;
        boundarySize.x += paddingL + paddingR;
        boundarySize.y += paddingU + paddingD;
        onRelayout();

     }

    /*******************************************************************************************/
    /**
     * @brief  Sets the alignment versus the container
     *  
     * @param v vertical alignment 
     *
     */
     void setVeritcalAlignment( BoundaryAlignment v = BACenter) {
        alignmentY = v;
        onRelayout();
     }

    /*******************************************************************************************/
    /**
     * @brief  Sets the alignment versus the container
     *  
     * @param h horizontal alignment
     *
     */
     void setHorizontalAlignment( BoundaryAlignment h = BACenter) {
        alignmentX = h;
        onRelayout();
     }

     /*******************************************************************************************/
    /**
     * @brief  Sets the alignment versus the container
     *  
     * @param h vertical alignment 
     * @param v horizontal alignment
     *
     */
     void setAlignment( BoundaryAlignment h = BACenter, BoundaryAlignment v = BACenter) {
        alignmentX = h;
        alignmentY = v;
        onRelayout();
     }
    
    /*******************************************************************************************/
    /**
     * @brief  Sets the elements position in the boundary appropriately
     * 
     */
     void resetAlignmentInBoundary() {
        
        // Compute the boundary offset based on the alignment

        // x position
        switch( alignmentX ) {
            case BALeft:
                boundaryOffset.x = paddingL;
            break;
            case BACenter:
                boundaryOffset.x = (boundarySize.x - (size.x + paddingL + paddingR ))/2;
            break;
            case BARight:
                boundaryOffset.x = boundarySize.x - (size.x + paddingL + paddingR );
            break;
            default:
            break;
        }

        // y position
        switch( alignmentY ) {
            case BALeft:
                boundaryOffset.y = paddingU;
            break;
            case BACenter:
                boundaryOffset.y = (boundarySize.y - (size.y + paddingU + paddingD ))/2;
            break;
            case BARight:
                boundaryOffset.y = boundarySize.y - (size.y + paddingU + paddingD );
            break;
            default:
            break;
        }
    
     }


    /*******************************************************************************************/
    /**
     * @brief  Gets the offset (in GUI space) of the element vs the boundary of the element
     * 
     * @returns The position vector 
     *
     */
    ivec2 getBoundaryOffset() {
        
        return boundaryOffset;
    
    }

    /*******************************************************************************************/
    /**
     * @brief  Sets the default size
     * 
     * @param newDefault the new default size  
     *
     */
    void setDefaultSize( ivec2 newDefault ) {

        defaultSize = newDefault;

    }

    /*******************************************************************************************/
    /**
     * @brief  Retrieves the default size
     * 
     * @returns 2d integer vector of the default size
     *
     */
     ivec2 getDefaultSize() {
        return defaultSize;
     }

    /*******************************************************************************************/
    /**
     * @brief  Sets the size of the region  
     * 
     * @param _size 2d size vector (-1 element implies undefined - or use AH_UNDEFINEDSIZE)
     * @param resetBoundarySize Should we reset the boundary if it's too small?
     *
     */
    void setSize( const ivec2 _size, bool resetBoundarySize = true ) {
        
        size = _size;
        if( resetBoundarySize ) {
            setBoundarySize( size );
        }
        onRelayout();

    }

    /*******************************************************************************************/
    /**
     * @brief  Sets the size of the region 
     * 
     * @param x x dimension size (-1 implies undefined - or use AH_UNDEFINEDSIZE)
     * @param y y dimension size (-1 implies undefined - or use AH_UNDEFINEDSIZE)
     * @param resetBoundarySize Should we reset the boundary if it's too small?
     *
     */
    void setSize( const int x, const int y, bool resetBoundarySize = true  ) {
                
        ivec2 newSize( x, y );
        setSize( newSize, resetBoundarySize );

    }

    /*******************************************************************************************/
    /**
     * @brief  Sets the x dimension of a region
     * 
     * @param x x dimension size (-1 implies undefined - or use AH_UNDEFINEDSIZE)
     * @param resetBoundarySize Should we reset the boundary if it's too small?
     *
     */
    void setSizeX( const int x, bool resetBoundarySize = true ) {
    
        setSize( ivec2( x, size.y ), resetBoundarySize );
    
    }   

    /*******************************************************************************************/
    /**
     * @brief  Sets the y dimension of a region
     * 
     * @param y y dimension size (-1 implies undefined - or use AH_UNDEFINEDSIZE)
     * @param resetBoundarySize Should we reset the boundary if it's too small?
     *
     */
    void setSizeY( const int y, bool resetBoundarySize = true ) {
            
        setSize( ivec2( size.x, y ), resetBoundarySize );

    }  

    /*******************************************************************************************/
    /**
     * @brief  Gets the size vector
     * 
     * @returns The size vector 
     *
     */
    ivec2 getSize() {
        return size;
    }

    /*******************************************************************************************/
    /**
     * @brief  Gets the size x component
     * 
     * @returns The x size
     *
     */
    int getSizeX() {
        return size.x;
    }

    /*******************************************************************************************/
    /**
     * @brief  Gets the size y component
     * 
     * @returns The y size
     *
     */
    int getSizeY() {
        return size.y;
    }

    /*******************************************************************************************/
    /**
     * @brief  Gets the boundary size vector
     * 
     * @returns The size vector 
     *
     */
    ivec2 getBoundarySize() {
        return boundarySize;
    }

    /*******************************************************************************************/
    /**
     * @brief  Gets the boundary size x component
     * 
     * @returns The x size
     *
     */
    int getBoundarySizeX() {
        return boundarySize.x;
    }

    /*******************************************************************************************/
    /**
     * @brief  Gets the boundary size y component
     * 
     * @returns The y size
     *
     */
    int getBoundarySizeY() {
        return boundarySize.y;
    }

    /*******************************************************************************************/
    /**
     * @brief Resizes the boundary 
     * 
     * @param newSize 2d size vector
     *
     */
     void setBoundarySize( ivec2 newSize ) {

        boundarySize = newSize;
        onRelayout();

     }

    /*******************************************************************************************/
    /**
     * @brief Resizes the boundary in the x dimension 
     * 
     * @param x the new size
     *
     */
     void setBoundarySizeX( int x ) {

        boundarySize.x = x;
        onRelayout();

     }


    /*******************************************************************************************/
    /**
     * @brief Resizes the boundary in the y dimension 
     * 
     * @param y the new size
     *
     */
     void setBoundarySizeY( int y ) {

        boundarySize.y = y;
        onRelayout();

     }


    /*******************************************************************************************/
    /**
     * @brief  Sends a message to the owning GUI
     * 
     * @param theMessage the message
     *
     */
     void sendMessage( Message@ theMessage ) {
        
        @theMessage.sender = this;
        
        if( owner !is null  ) {
            owner.receiveMessage( theMessage );
        }

     }

    /*******************************************************************************************/
    /**
     * @brief  Finds an element by a given name
     * 
     * @param elementName the name of the element
     *
     * @returns handle to the element (null if not found)  
     *
     */
    Element@ findElement( string elementName ) {
        // Check if this is the droid we're looking for
        if( name == elementName ) {
            return @this;
        }
        else {
            return null;
        }
    }

};

} // namespace AHGUI

