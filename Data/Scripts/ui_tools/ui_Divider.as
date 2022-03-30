#include "ui_tools/ui_support.as"
#include "ui_tools/ui_Element.as"
#include "ui_tools/ui_Container.as"


/*******
 *  
 * ui_Divider.as
 *
 * Specialized container element class for creating adhoc GUIs as part of the UI tools  
 *
 */

namespace AHGUI {

/*******************************************************************************************/
/**
 * @brief  Basic container class, holds other elements
 *
 */
class Divider : Container {

    array<Element@> topLeftContents; // elements in this container top/left
    array<Element@> bottomRightContents; // elements in this container topLeft
    Element@ centeredElement; // element in this container centered (only one allowed)

    int topLeftBoundStart;  // Start coordinate for the top/left container
    int topLeftBoundEnd;    // End coordinate for the top/left container
    int centerBoundStart;   // Start coordinate for the center element
    int centerBoundEnd;     // End coordinate for the center element
    int bottomRightBoundStart;  // Start coordinate for the center element
    int bottomRightBoundEnd;    // End coordinate for the center element
    int bottomRightSize = 0;    // Size of of the bottom/right elements  
    int centerSize = 0;         // Size of the center element
    int topLeftSize = 0;        // Size of the top/left elements

    DividerOrientation orientation; // vertical by default
    
    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *  
     * @param name Element name
     * @param _orientation The orientation of the container
     *
     */
    Divider( string name, DividerOrientation _orientation = DOVertical ) {
        orientation = _orientation;
        @centeredElement = null;
        super(name);
    }

    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *  
     * @param name Element name
     * @param _orientation The orientation of the container
     *
     */
    Divider( DividerOrientation _orientation = DOVertical ) {
        orientation = _orientation;
        @centeredElement = null;
        super();
    }

    /*******************************************************************************************/
    /**
     * @brief  Gets the name of the type of this element — for autonaming and debugging
     * 
     * @returns name of the element type as a string
     *
     */
    string getElementTypeName() {
        return "Divider";
    }

    /*******************************************************************************************/
    /**
     * @brief  Clear the contents of this divider, leaving everything else the same
     * 
     */
    void clear() {

        topLeftContents.resize(0);
        @centeredElement = null;
        bottomRightContents.resize(0);

        // Reset the region tracking
        topLeftBoundStart = AH_UNDEFINEDSIZE;   
        topLeftBoundEnd = AH_UNDEFINEDSIZE; 
        topLeftSize = 0;  
        centerBoundStart = AH_UNDEFINEDSIZE;   
        centerBoundEnd = AH_UNDEFINEDSIZE;     
        centerSize = 0;
        bottomRightBoundStart = AH_UNDEFINEDSIZE;
        bottomRightBoundEnd = AH_UNDEFINEDSIZE;
        bottomRightSize = 0;

        Container::clear();

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
        Container::update( delta, drawOffset, guistate );

        ivec2 currentDrawOffset = boundaryOffset + drawOffset + drawDisplacement;

        // Simply pass this on to the children
        for( uint i = 0; i < topLeftContents.length(); i++ ) {
            
            topLeftContents[i].update( delta, currentDrawOffset, guistate );
            
            if( orientation == DOVertical ) {
                currentDrawOffset.y += topLeftContents[i].getBoundarySizeY();
            }
            else {
                currentDrawOffset.x += topLeftContents[i].getBoundarySizeX();   
            }
        }

        if( centeredElement !is null ) {

            currentDrawOffset = boundaryOffset + drawOffset;

            if( orientation == DOVertical ) {
                currentDrawOffset.y += centerBoundStart;
            }
            else {
                currentDrawOffset.x += centerBoundStart;   
            }

            centeredElement.update( delta, currentDrawOffset, guistate );
        }

        currentDrawOffset = boundaryOffset + drawOffset;

        if( orientation == DOVertical ) {
            currentDrawOffset.y += bottomRightBoundStart;
        }
        else {
            currentDrawOffset.x += bottomRightBoundStart;   
        }

        for( uint i = 0; i < bottomRightContents.length(); i++ ) {

            bottomRightContents[i].update( delta, currentDrawOffset, guistate );

            if( orientation == DOVertical ) {
                currentDrawOffset.y += bottomRightContents[i].getBoundarySizeY();
            }
            else {
                currentDrawOffset.x += bottomRightContents[i].getBoundarySizeX();   
            }
        }
    }

    /*******************************************************************************************/
    /**
     * @brief This draws this object on the screen
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
            currentClipSize = getBoundarySize();
        }

        // See if the superclass wants to do anything
        Container::render( drawOffset, currentClipPos, currentClipSize );

        // Simply pass this on to the children
        ivec2 currentDrawOffset = boundaryOffset + drawOffset + drawDisplacement;
        for( uint i = 0; i < topLeftContents.length(); i++ ) {
            
            topLeftContents[i].render( currentDrawOffset, currentClipPos, currentClipSize );
            
            if( orientation == DOVertical ) {
                currentDrawOffset.y += topLeftContents[i].getBoundarySizeY();
            }
            else {
                currentDrawOffset.x += topLeftContents[i].getBoundarySizeX();   
            }
        }

        if( centeredElement !is null ) {

            currentDrawOffset = boundaryOffset + drawOffset + drawDisplacement;

            if( orientation == DOVertical ) {
                currentDrawOffset.y += centerBoundStart;
            }
            else {
                currentDrawOffset.x += centerBoundStart;   
            }

            centeredElement.render( currentDrawOffset, currentClipPos, currentClipSize );
        }

        currentDrawOffset = boundaryOffset + drawOffset + drawDisplacement;

        if( orientation == DOVertical ) {
            currentDrawOffset.y += bottomRightBoundStart;
        }
        else {
            currentDrawOffset.x += bottomRightBoundStart;   
        }

        for( uint i = 0; i < bottomRightContents.length(); i++ ) {

            bottomRightContents[i].render( currentDrawOffset, currentClipPos, currentClipSize );

            if( orientation == DOVertical ) {
                currentDrawOffset.y += bottomRightContents[i].getBoundarySizeY();
            }
            else {
                currentDrawOffset.x += bottomRightContents[i].getBoundarySizeX();   
            }
        }

        // Do whatever the superclass wants 
        Element::render( drawOffset, currentClipPos, currentClipSize );

    }

    /*******************************************************************************************/
    /**
     * @brief Rederive the regions for the various orientation containers - for internal use
     * 
     */
     void checkRegions() {

        // Reset the region tracking
        topLeftBoundStart = AH_UNDEFINEDSIZE;   
        topLeftBoundEnd = AH_UNDEFINEDSIZE; 
        topLeftSize = 0;  
        centerBoundStart = AH_UNDEFINEDSIZE;   
        centerBoundEnd = AH_UNDEFINEDSIZE;     
        centerSize = 0;
        bottomRightBoundStart = AH_UNDEFINEDSIZE;
        bottomRightBoundEnd = AH_UNDEFINEDSIZE;
        bottomRightSize = 0;
        
        // see which direction we're going
        if( orientation == DOVertical ) {

            // sum the top contents
            for( uint i = 0; i < topLeftContents.length(); i++ ) {

                if( topLeftBoundStart == AH_UNDEFINEDSIZE ) {
                    topLeftBoundStart = 0;
                    topLeftBoundEnd = -1;
                }

                if( topLeftContents[i].getBoundarySizeY() != AH_UNDEFINEDSIZE ) {
                    // update the totals
                    topLeftSize += topLeftContents[i].getBoundarySizeY();
                    topLeftBoundEnd += topLeftContents[i].getBoundarySizeY() - 1;

                    // check to see if this element pushes the boundary of the divider
                    if( topLeftContents[i].getBoundarySizeX() > getBoundarySizeX() ) {
                        setSizeX( topLeftContents[i].getBoundarySizeX() );
                    }

                    // check to make sure the element boundary is the same as this container
                    if( topLeftContents[i].getBoundarySizeX() < getBoundarySizeX() ) {
                        topLeftContents[i].setBoundarySizeX( getBoundarySizeX() );   
                    }
                }   

            }

            // As the center is one element, we just need to calculate from it
            if( centeredElement !is null && centeredElement.getBoundarySizeY() != AH_UNDEFINEDSIZE ) {
                
                int dividerCenter = ((getSizeY() - 1)/2);

                centerBoundStart = dividerCenter - (centeredElement.getBoundarySizeY()/2);
                centerBoundEnd   = centerBoundStart  + ( centeredElement.getBoundarySizeY() - 1 );

                centerSize = centeredElement.getBoundarySizeY();

                // check to see if this element pushes the boundary of the divider
                if( centeredElement.getBoundarySizeX() > getBoundarySizeX() ) {
                    setSizeX( centeredElement.getBoundarySizeX() );
                }

                // check to make sure the element boundary is the same as this container
                if( centeredElement.getBoundarySizeX() < getBoundarySizeX() ) {
                    centeredElement.setBoundarySizeX( getBoundarySizeX() );
                }
            }

            
            // sum the bottom contents
            for( int i = int(bottomRightContents.length())-1; i >= 0 ; i-- ) {

                if( bottomRightBoundStart == AH_UNDEFINEDSIZE ) {
                    bottomRightBoundEnd = getSizeY() - 1;
                    bottomRightBoundStart = bottomRightBoundEnd + 1;
                }

                if( bottomRightContents[i].getBoundarySizeY() != AH_UNDEFINEDSIZE ) {
                    
                    // update the totals
                    bottomRightBoundStart -= (bottomRightContents[i].getSizeY() - 1);
                    bottomRightSize += bottomRightContents[i].getBoundarySizeY();

                    // check to see if this element pushes the boundary of the divider
                    if( bottomRightContents[i].getBoundarySizeX() > getBoundarySizeX() ) {
                        setSizeX( bottomRightContents[i].getBoundarySizeX() );
                    }

                    // check to make sure the element boundary is the same as this container
                    if( bottomRightContents[i].getBoundarySizeX() < getBoundarySizeX() ) {
                        bottomRightContents[i].setBoundarySizeX( getBoundarySizeX() );
                    }
                }
            }

            // Now we check that this all fits in the divider 
            int neededSize = 0;
            // The math is different if we have a center element 
            if( centeredElement !is null && centerSize > 0  ) {
                // we have a centered element
                // figure out how much size we need for this divider

                int halfCenter = centerSize / 2;
                int topLeftHalf = halfCenter + topLeftSize;
                int bottomRightHalf = halfCenter + bottomRightSize;

                neededSize = max( topLeftHalf * 2, bottomRightHalf * 2 );

            }
            else {
                // we don't have a centered element
                neededSize = topLeftSize + bottomRightSize;
            }

            // Check to see if we have enough size in this divider
            if( neededSize > getSizeY() ) {
                // Try changing the size of the divider
                //  This is likely going to fail and throw an error, but let's
                //  let the system decide
                setSizeY( neededSize );
            }

        }
        else {

            // horizontal layout 

            // sum the left contents
            for( uint i = 0; i < topLeftContents.length(); i++ ) {

                if( topLeftBoundStart == AH_UNDEFINEDSIZE ) {
                    topLeftBoundStart = 0;
                    topLeftBoundEnd = -1;
                }

                if( topLeftContents[i].getBoundarySizeX() != AH_UNDEFINEDSIZE ) {
                    // update the totals
                    topLeftBoundEnd += topLeftContents[i].getBoundarySizeX()- 1;
                    topLeftSize += topLeftContents[i].getBoundarySizeX();

                    // check to see if this element pushes the boundary of the divider
                    if( topLeftContents[i].getBoundarySizeY() > getBoundarySizeY() ) {
                        setSizeY( topLeftContents[i].getBoundarySizeY() );
                    }

                    // check to make sure the element boundary is the same as this container
                    if( topLeftContents[i].getBoundarySizeY() < getBoundarySizeY() ) {
                        topLeftContents[i].setBoundarySizeY( getBoundarySizeY() );
                    }
                }
            }

            // As the center is one element, we just need to calculate from it
            if( centeredElement !is null && centeredElement.getBoundarySizeX() != AH_UNDEFINEDSIZE ) {
                
                int dividerCenter = ((getSizeX()- 1)/2);
                centerBoundStart = dividerCenter - (centeredElement.getBoundarySizeX()/2);
                centerBoundEnd = centeredElement.getBoundarySizeX() - 1;
                centerSize = centeredElement.getBoundarySizeX();

                // check to see if this element pushes the boundary of the divider
                if( centeredElement.getBoundarySizeY() > getBoundarySizeY() ) {
                    setSizeY( centeredElement.getBoundarySizeY() );
                }

                // check to make sure the element boundary is the same as this container
                if( centeredElement.getBoundarySizeY() < getBoundarySizeY() ) {
                    centeredElement.setBoundarySizeY( getBoundarySizeY() );
                }
            }

            // sum the bottom/right contents
            for( int i = int(bottomRightContents.length())-1; i >= 0 ; i-- ) {

                if( bottomRightBoundStart == AH_UNDEFINEDSIZE ) {
                    bottomRightBoundEnd = getSizeX() - 1;
                    bottomRightBoundStart = bottomRightBoundEnd + 1;
                }                

                if( bottomRightContents[i].getBoundarySizeX() != AH_UNDEFINEDSIZE ) {

                    // update the totals
                    bottomRightBoundStart -= (bottomRightContents[i].getBoundarySizeX() - 1);
                    bottomRightSize += bottomRightContents[i].getBoundarySizeX();

                    // check to see if this element pushes the boundary of the divider
                    if( bottomRightContents[i].getBoundarySizeY() > getBoundarySizeY() ) {
                        setSizeY( bottomRightContents[i].getBoundarySizeY() );
                    }

                    // check to make sure the element boundary is the same as this container
                    if( bottomRightContents[i].getBoundarySizeY() < getBoundarySizeY() ) {
                        bottomRightContents[i].setBoundarySizeY( getBoundarySizeY() );
                    }
                }
            }

            // Now we check that this all fits in the divider 
            int neededSize = 0;
            // The math is different if we have a center element 
            if( centeredElement !is null && centerSize > 0  ) {
                // we have a centered element
                // figure out how much size we need for this divider
                int halfCenter = centerSize / 2;
                int topLeftHalf = halfCenter + topLeftSize;
                int bottomRightHalf = halfCenter + bottomRightSize;

                neededSize = max( topLeftHalf * 2, bottomRightHalf * 2 );
            }
            else {
                // we don't have a centered element
                neededSize = topLeftSize + bottomRightSize;
            }

            // Check to see if we have enough size in this divider
            if( neededSize > getSizeX() ) {
                // Try changing the size of the divider
                //  This is likely going to fail and throw an error, but let's
                //  let the system decide
                setSizeX( neededSize );
            }

        }

        //Print( "Done checkRegions -- topLeftSize: " + topLeftSize + "\n" );
     }


    /*******************************************************************************************/
    /**
     * @brief  When a resize, move, etc has happened do whatever is necessary
     * 
     */
     void doRelayout() {

        // Invoke the elements relayout
        Element::doRelayout();
        
        // First pass this down to the children
        for( uint i = 0; i < topLeftContents.length(); i++ ) {
            topLeftContents[i].doRelayout();
        }

        if( centeredElement !is null ) {
            centeredElement.doRelayout();
        }

        for( uint i = 0; i < bottomRightContents.length(); i++ ) {
            bottomRightContents[i].doRelayout();
        }

        checkRegions();

     }

    /*******************************************************************************************/
    /**
     * @brief  Do whatever is necessary when the resolution changes
     *
     */
     void doScreenResize()  {
        
        // Invoke the superclass's method
        Container::doScreenResize();
        
        // Pass this down to the children
        for( uint i = 0; i < topLeftContents.length(); i++ ) {
            topLeftContents[i].doScreenResize();
        }

        if( centeredElement !is null ) {
            centeredElement.doScreenResize();
        }

        for( uint i = 0; i < bottomRightContents.length(); i++ ) {
            bottomRightContents[i].doScreenResize();
        }

     }


    /*******************************************************************************************/
    /**
     * @brief Convenience function to add a spacer element to this divider
     *  
     * @param size Size of the element in terms of GUI space pixels
     * @param direction Side of the container to add to 
     *
     * @returns the space object created, just in case you need it
     *
     */
    Spacer@ addSpacer( int _size, DividerDirection direction = DDTopLeft ) {
        
        // Create a new spacer object
        Spacer@ newSpacer = Spacer(); 
        
        // Set the coordinates based on the orientation
        if( orientation == DOVertical ) {
            newSpacer.setSize( AH_UNDEFINEDSIZE, _size );
        }
        else {
            newSpacer.setSize( _size, AH_UNDEFINEDSIZE );
        }

        // Add this to the divider
        addElement( newSpacer, direction );

        // return a reference to this object in case the
        //  user needs to reference it (get the name, etc)
        return newSpacer;

    }

    /*******************************************************************************************/
    /**
     * @brief Convenience function to add a sub-divider
     *  
     * @param direction Side of the container to add to 
     * @param newOrientation Orientation of the new divider ( defaults to opposite of the host )
     * @param size Size of the element in terms of GUI space pixels (optional if in opposite direction of the host)
     *
     * @returns the space object created, just in case you need it
     *
     */
    Divider@ addDivider( DividerDirection direction, DividerOrientation newOrientation, ivec2 size = ivec2( AH_UNDEFINEDSIZE, AH_UNDEFINEDSIZE ) ) {
        
        // Create a new spacer object
        Divider@ newDivider = Divider(newOrientation); 
        
        // Set the coordinates based on the orientation
        if( orientation == DOVertical ) {
                
            // If the user hasn't specified a size, set it to the width of the container
            if( size.x == AH_UNDEFINEDSIZE ) {
                size.x = getSizeX();
            }

            newDivider.setSize( size );

        }
        else {
            
            // If the user hasn't specified a size, set it to the height of the container
            if( size.y == AH_UNDEFINEDSIZE ) {
                size.y = getSizeY();
            }

            newDivider.setSize( size );
        
        }

        // Add this to the divider
        addElement( newDivider, direction );

        // return a reference to this object in case the
        //  user needs to reference it (get the name, etc)
        return newDivider;

    }

    /*******************************************************************************************/
    /**
     * @brief  Fills the center of a divider with another divider 
     * 
     *  This is a bit of a hack, but useful for unconstrained mode
     *  Make sure that you've added all your other fixed size dividers first
     *   as this will eat up the rest of the space
     *
     * @param newOrientation orientation for the new divider
     *
     */
     Divider@ addDividerFillCenter( DividerOrientation newOrientation ) {

        // Check to make sure that there is no center element already
        if( centeredElement !is null ) {
            DisplayError("GUI Error", "Cannot fill center element with center element already defined in divider " + getName() );  
        }

        // Make sure the totals are up to date 
        checkRegions();

        // Create a new spacer object
        Divider@ newDivider = Divider(newOrientation); 

        ivec2 size;
        
        // Set the coordinates based on the orientation
        if( orientation == DOVertical ) {
            
            size.x = getSizeX();
            size.y = getSizeY() - ( 2 * max(topLeftSize, bottomRightSize) );

        }
        else {
            size.x = getSizeX() - ( 2 * max(topLeftSize, bottomRightSize) );
            size.y = getSizeY();
        }

        newDivider.setSize( size );

        // Add this to the divider
        addElement( newDivider, DDCenter );

        // return a reference to this object in case the
        //  user needs to reference it (set the name, etc)
        return newDivider;

     }


    /*******************************************************************************************/
    /**
     * @brief Adds an element to the divider 
     *  
     * @param newElement Element to add  
     * @param direction Portion of the divider to add to (default top/left)
     *
     */
    void addElement( Element@ newElement, DividerDirection direction = DDCenter ) {

        // Make sure the element has a name 
        if( newElement.name == "" ) {
            newElement.name = owner.getUniqueName( getElementTypeName() );
        }

        // Which orientation is this container?
        if( orientation == DOVertical ) {

            switch( direction ) {
                
                case DDTopLeft: {

                    topLeftContents.insertLast( newElement );
                    break;    
                }

                case DDBottomRight: {

                    bottomRightContents.insertAt( 0, newElement );
                    break;
                }

                case DDCenter: {

                    if( centeredElement !is null ) {
                        DisplayError("GUI Error", "Multiple centered elements added to divider");  
                    }

                    @centeredElement = @newElement;

                    break;
                }
                default:

            }   
        }
        else {

            switch( direction ) {

                case DDTopLeft: {

                    topLeftContents.insertLast( newElement );
                    break;    
                }

                case DDBottomRight: {

                    bottomRightContents.insertAt( 0, newElement );
                    break;
                }

                case DDCenter: {

                    if( centeredElement !is null ) {
                        DisplayError("GUI Error", "Multiple centered elements added to divider");  
                    }

                    @centeredElement = @newElement;

                    break;
                }
                default:

            }   
        }

        // If we don't have a default size already and we have a size, set the default size
        if( newElement.getDefaultSize().x == AH_UNDEFINEDSIZE &&
            newElement.getDefaultSize().y == AH_UNDEFINEDSIZE  ) {
            newElement.setDefaultSize( newElement.getSize() );
        }

        // Link to this element/owning GUI
        @newElement.owner = @owner;
        @newElement.parent = @this;

        // Make sure it's in front of us 
        newElement.setZOrdering( getZOrdering() + 1 );

        // Signal that something new has changed
        onRelayout();

    }


    /*******************************************************************************************/
    /**
     * @brief  Find an element by name — called internally
     *  
     *
     */
    Element@ findElement( string elementName ) {

        // See if the superclass owns this
        Element@ foundElement = Container::findElement( elementName );

        if( foundElement !is null ) {
            return foundElement;
        }

        // Check if this is the droid we're looking for
        if( name == elementName ) {
            return @this;
        }
        else {
            // If not, pass the request onto the children
        
            for( uint i = 0; i < topLeftContents.length(); i++ ) {
            
                Element@ results = topLeftContents[i].findElement( elementName );

                if( results !is null ) {
                    return results;
                }

            }

            if( centeredElement !is null ) {

                Element@ results = centeredElement.findElement( elementName );

                if( results !is null ) {
                    return results;
                }
            }


            for( uint i = 0; i < bottomRightContents.length(); i++ ) {

                Element@ results = bottomRightContents[i].findElement( elementName );

                if( results !is null ) {
                    return results;
                }

            }

            // if we've got this, far we don't have it and so report
            return null;
        }
    }


    /*******************************************************************************************/
    /**
     * @brief  Destructor
     *
     */
    ~Divider() {
        topLeftContents.resize(0);
        bottomRightContents.resize(0);
        @centeredElement = null;
    }
}
} // namespace AHGUI

