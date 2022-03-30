#include "ui_tools/ui_support.as"
#include "ui_tools/ui_Element.as"
#include "ui_tools/ui_Image.as"

/*******
 *  
 * ui_Container.as
 *
 * Most basic container, fixed size that simply serves to contain a bunch of elements
 *  at specified offsets 
 *
 */

namespace AHGUI {

/*******************************************************************************************/
/**
 * @brief  Hold the information for the contained element
 *
 */
class ContainerElement {
    ivec2 relativePos;  // Where is this element offset from the upper/right of this container
    Element@ element;   // Handle to the element itself
}

class ContainerElementInstance {
    ContainerElementInstance() {
        key = "";
    }

    ContainerElementInstance( string _key, ContainerElement _elem ) {
        key = _key;
        elem = _elem; 
    }

    string key;
    ContainerElement elem;
}

class ContainerElements {
    array<ContainerElementInstance> elems;

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

    ContainerElement@ Get( string key ) {
        for( uint i = 0; i < elems.length(); i++ ) {
            if( elems[i].key == key ) {
                return elems[i].elem;
            }
        }
        return ContainerElement();
    }

    void Set( string key, ContainerElement elem ) {
        bool found = false;
        for( uint i = 0; i < elems.length(); i++ ) {
            if( elems[i].key == key ) {
                found = true;
                elems[i].elem = elem;
            }
        }
        if( found == false ) {
            elems.insertLast(ContainerElementInstance(key,elem));
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

    ContainerElement@ GetI( int i ) {
        return elems[i].elem;
    }

    uint Count() {
        return elems.length();
    }
}

/*******************************************************************************************/
/**
 * @brief  Basic container class, holds other elements
 *
 */
class Container : Element {
    
    ContainerElements floatingContents; // elements in this container
    Image@ backgroundImage = null;
    

    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *  
     * @param name Element name
     * @param size Size of this container 
     *
     */
    Container( string name, ivec2 size = ivec2( AH_UNDEFINEDSIZE,AH_UNDEFINEDSIZE ) ) {
        super(name);
        setSize( size );
    }

    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *
     * @param size Size of this container 
     * 
     */
    Container( ivec2 size = ivec2( AH_UNDEFINEDSIZE,AH_UNDEFINEDSIZE ) ) {
        super();
        setSize( size );
    }

    /*******************************************************************************************/
    /**
     * @brief  Gets the name of the type of this element — for autonaming and debugging
     * 
     * @returns name of the element type as a string
     *
     */
    string getElementTypeName() {
        return "Container";
    }

    /*******************************************************************************************/
    /**
     * @brief  Clear the contents of this container, leaving everything else the same
     * 
     */
    void clear() {
        floatingContents.deleteAll();
        
        boundaryOffset = ivec2(0,0);
        setSize(getDefaultSize());
        setBoundarySize();
        setDisplacement();
        
        onRelayout();
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
        // Do whatever the superclass wants
        Element::update( delta, drawOffset, guistate );

        for( uint k = 0; k < floatingContents.Count(); k++ ) {
            ContainerElement@ element = cast<ContainerElement>(floatingContents.GetI(k));
            element.element.update( delta, drawOffset + boundaryOffset + drawDisplacement + element.relativePos, guistate );
        }
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
    void render( ivec2 drawOffset, ivec2 clipPos, ivec2 clipSize ) {

        // See if we need to adjust clip for this container
        ivec2 currentClipPos = drawOffset + boundaryOffset + drawDisplacement;
        ivec2 currentClipSize;

        if( getSizeX() == AH_UNDEFINEDSIZE || getSizeY() == AH_UNDEFINEDSIZE ) {
            currentClipSize = clipSize;
        }
        else {
            currentClipSize = getSize();
        }

        // Render the background, if any
        if( backgroundImage !is null ) {
            
            backgroundImage.render( drawOffset + boundaryOffset + drawDisplacement, currentClipPos, currentClipSize );
        }

        // Simply pass this on to the children
        for( uint k = 0; k < floatingContents.Count(); k++ ) {
            ContainerElement@ element = cast<ContainerElement>(floatingContents.GetI(k));
            element.element.render( drawOffset + boundaryOffset + drawDisplacement + element.relativePos, currentClipPos, currentClipSize );
        }


        // Do whatever the superclass wants
        Element::render( drawOffset, currentClipPos, currentClipSize );

    }


    /*******************************************************************************************/
    /**
     * @brief  When a resize, move, etc has happened do whatever is necessary
     * 
     */
     void doRelayout() {

        // Invoke the elements relayout
        Element::doRelayout();

        // Make sure background image has kept up with any resizing 
        if( backgroundImage !is null ) {
            backgroundImage.setSize( getSizeX(), getSizeY() );
        }
        
        // Now trigger the children
        for( uint k = 0; k < floatingContents.Count(); k++ ) {
            ContainerElement@ element = cast<ContainerElement>(floatingContents.GetI(k));
            element.element.doRelayout();
        }

     }

    /*******************************************************************************************/
    /**
     * @brief  Do whatever is necessary when the resolution changes
     *
     */
     void doScreenResize()  {
        
        // Invoke the superclass's method
        Element::doScreenResize();
        
        // Now trigger the children
        for( uint k = 0; k < floatingContents.Count(); k++ ) {
            ContainerElement@ element = cast<ContainerElement>(floatingContents.GetI(k));
            element.element.doScreenResize();
        }
     }


    /*******************************************************************************************/
    /**
     * @brief Adds an arbitrarily placed element to the container 
     *  
     * @param newElement Element to add  
     * @param name Name of the element (this *will* rename the element )
     * @param position Position relative to the upper right of this container (GUI Space)
     * @param z z ordering for the new element (relative to this container -- optional )
     *
     */
    void addFloatingElement( Element@ element, string name, ivec2 position, int z = -1 ) {

        // Make sure the element has the name 
        element.setName( name );

        ContainerElement containerElement();

        containerElement.relativePos = position;
        @containerElement.element = element;

        floatingContents.Set(name, containerElement);
        
        // Link to this element/owning GUI
        @element.owner = @owner;
        @element.parent = @this;

        // Make sure it's in front of us
        if( z < 1 ) {
            element.setZOrdering( getZOrdering() + element.getZOrdering() );
        }
        else {
            element.setZOrdering( getZOrdering() + z );    
        }
        
        // Signal that something new has changed
        onRelayout();

    }

    /*******************************************************************************************/
    /**
     * @brief Removes an element
     * 
     * @param name Name of the element to remove 
     *
     * @returns the element if it's there, null otherwise
     * 
     */
     Element@ removeElement( string name ) {

        // make sure it exists
        if( floatingContents.exists(name) ) {
            // and delete it
            ContainerElement@ element = floatingContents.Get(name);
            floatingContents.Remove( name );

            // Signal that something new has changed
            onRelayout();

            return element.element;
        }
        else {
            // If not, let the caller know
            return null;
        }

    }

    /*******************************************************************************************/
    /**
     * @brief  Moves and element to a new position
     * 
     * @param name Name of the element to move
     * @param newPos new position of the element
     *
     */
    void moveElement( string name, ivec2 newPos ) {
        // make sure it exists
        if( floatingContents.exists(name) ) {
            // Change it
            ContainerElement@ elementContainer = floatingContents.Get(name);
            elementContainer.relativePos = newPos;
        }
        else {
            // If not, throw an error
            DisplayError("GUI Error", "Unable to find element " + name + " to move");
        }
    }

    /*******************************************************************************************/
    /**
     * @brief  Moves and element to a new position relative to its old one
     * 
     * @param name Name of the element to move
     * @param posChange change in element position
     *
     */
    void moveElementRelative( string name, ivec2 posChange ) {
        // make sure it exists
        if( floatingContents.exists(name) ) {
            // Change it
            ContainerElement@ elementContainer = floatingContents.Get(name);
            elementContainer.relativePos += posChange;
        }
        else {
            // If not, throw an error
            DisplayError("GUI Error", "Unable to find element " + name + " to move");
        }
    }


    /*******************************************************************************************/
    /**
     * @brief  Find an element by name — called internally
     *  
     *
     */
    Element@ findElement( string elementName ) {
        // Check if this is the droid we're looking for
        if( name == elementName ) {
            return this;
        }
        else {
            // If not, look at the children
            if( floatingContents.exists(elementName) ) {
    
                Element@ element = floatingContents.Get(elementName).element;
            
                return element;
            }
            else {
                // If not, let the caller know
                return null;
            }
        }

    }

    /*******************************************************************************************/
    /**
     * @brief  Set a backgound image for this container
     * 
     * @param fileName file name for the image (empty string to clear)
     *
     */
    void setBackgroundImage( string fileName ) {
        setBackgroundImage( fileName, vec4(1.0) );
    }

    /*******************************************************************************************/
    /**
     * @brief  Set a backgound image for this container
     * 
     * @param fileName file name for the image (empty string to clear)
     * @param color 4 component color vector  
     *
     */
    void setBackgroundImage( string fileName, vec4 color  ) {
        
        if( fileName == "" ) {
            @backgroundImage = null;
            return;
        }

        if( getSizeX() == AH_UNDEFINEDSIZE || getSizeY() == AH_UNDEFINEDSIZE ) {
            DisplayError("GUI Error", "Cannot add a background image to a container with an undefined dimension" );  
        }

        @backgroundImage = @Image( fileName );

        // Load the image
        backgroundImage.setImageFile( fileName );

        // Set the size to be the same as this container
        backgroundImage.setSize( getSizeX(), getSizeY() );

        // Set the color
        backgroundImage.setColor( color );

        // Set the appropriate rendering order
        backgroundImage.setZOrdering( getZOrdering() );

    }

    /*******************************************************************************************/
    /**
     * @brief  Destructor
     *
     */
    ~Container() {
        floatingContents.deleteAll();
    }

}


} // namespace AHGUI

