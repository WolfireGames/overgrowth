//-----------------------------------------------------------------------------
//           Name: im_divider.cpp
//      Developer: Wolfire Games LLC
//    Description: Specialized container element class for creating adhoc GUIs 
//                 as part of the UI tools  
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
#include "im_divider.h"

#include <sstream>

/*******************************************************************************************/
/**
 * @brief  Constructor
 *
 * @param name IMElement name
 * @param _orientation The orientation of the container
 *
 */
IMDivider::IMDivider( std::string const& name, DividerOrientation _orientation ) :
    IMElement(name),
    contentsNum(0),
    contentsXAlignment(CACenter),
    contentsYAlignment(CACenter),
    orientation(DOVertical) // vertical by default
{
    IMrefCountTracker.addRefCountObject( getElementTypeName() );
    orientation = _orientation;
}

/*******************************************************************************************/
/**
 * @brief  Constructor
 *
 * @param name IMElement name
 * @param _orientation The orientation of the container
 *
 */
IMDivider::IMDivider( DividerOrientation _orientation ) : 
    IMElement(),
    contentsNum(0),
    contentsXAlignment(CACenter),
    contentsYAlignment(CACenter),
    orientation(DOVertical) // vertical by default
{
    IMrefCountTracker.addRefCountObject( getElementTypeName() );
    orientation = _orientation;
}

/*******************************************************************************************/
/**
 * @brief  Gets the name of the type of this element — for autonaming and debugging
 *
 * @returns name of the element type as a string
 *
 */
std::string IMDivider::getElementTypeName() {
    return "Divider";
}

/*******************************************************************************************/
/**
 * @brief  Set’s this element’s parent (and does nessesary logic)
 *
 * @param _parent New parent
 *
 */
void IMDivider::setOwnerParent( IMGUI* _owner, IMElement* _parent ) {
    owner = _owner;
    parent = _parent;
    
    // Simply pass this on to the children
    for(auto & container : containers) {
        container->setOwnerParent( owner, this );
    }
}

/*******************************************************************************************/
/**
 * @brief  Set’s the alignment of the contained object
 *
 * @param xAlignment horizontal alignment
 * @param yAlignment vertical alignment
 * @param reposition should we go through and reposition existing objects
 *
 */
void IMDivider::setAlignment( ContainerAlignment xAlignment, ContainerAlignment yAlignment, bool reposition ) {
    
    contentsXAlignment = xAlignment;
    contentsYAlignment = yAlignment;
    
    if( reposition ) {
        for(auto & container : containers) {
            container->setAlignment( xAlignment, yAlignment );
        }
    }
    
}

/*******************************************************************************************/
/**
 * @brief  Clear the contents of this divider, leaving everything else the same
 *
 */
void IMDivider::clear() {
    std::vector<IMContainer*> containers_copy = containers;
    for(auto & iter : containers_copy) {
            iter->Release();
    }
    containers.resize(0);
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
void IMDivider::update( uint64_t delta, vec2 drawOffset, GUIState& guistate ) {
    
    bool mouseOverState = guistate.inheritedMouseOver;
    bool mouseDownState = guistate.inheritedMouseDown;
    IMUIContext::ButtonState currentMouseState = guistate.inheritedMouseState;
    
    // Do whatever the superclass wants
    IMElement::update( delta, drawOffset, guistate );
    
    vec2 currentDrawOffset = drawOffset + drawDisplacement;
    
    // Simply pass this on to the children
    for(auto & container : containers) {
        
        container->update( delta, currentDrawOffset, guistate );
        
        if( orientation == DOVertical ) {
            currentDrawOffset.y() += container->getSizeY();
        }
        else {
            currentDrawOffset.x() += container->getSizeX();
        }
    }
    
    guistate.inheritedMouseOver = mouseOverState;
    guistate.inheritedMouseDown = mouseDownState;
    guistate.inheritedMouseState = currentMouseState;

    
}

/*******************************************************************************************/
/**
 * @brief Draw this object on the screen
 *
 * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
 * @param clipPos pixel location of upper lefthand corner of clipping region
 * @param clipSize size of clipping region
 *
 */
void IMDivider::render( vec2 drawOffset, vec2 clipPos, vec2 clipSize ) {
        
    // See if we need to adjust clip for this container
    vec2 currentClipPos = drawOffset + drawDisplacement;
    vec2 currentClipSize;
    
    if( getSizeX() == UNDEFINEDSIZE || getSizeY() == UNDEFINEDSIZE ) {
        currentClipSize = clipSize;
    }
    else {
        currentClipSize = getSize();
    }
    
    // See if the superclass wants to do anything
    IMElement::render( drawOffset, currentClipPos, currentClipSize );
    
    // Simply pass this on to the children
    vec2 currentDrawOffset = drawOffset + drawDisplacement;
    for(auto & container : containers) {
        
        container->render( currentDrawOffset, currentClipPos, currentClipSize );
        
        if( orientation == DOVertical ) {
            
            if( container->getSizeY() > 0 ) {
                currentDrawOffset.y() += container->getSizeY();
            }
        }
        else {
            
            if( container->getSizeX() > 0 ) {
                currentDrawOffset.x() += container->getSizeX();
            }
        }
    }
    
}

/*******************************************************************************************/
/**
 * @brief Rederive the regions for the various orientation containers - for internal use
 *
 */
void IMDivider::checkRegions() {
    
    // keep track of the dynamic spacers so we can resize them when we're done
    std::vector<IMSpacer*> dynamicSpacers;
    float totalSize = 0;
    
    if( orientation == DOVertical ) {
        
        // First calculate the height
        for(auto & container : containers) {
            
            // see if this element is a dynamic spacer
            // first check if there is an element in this container
            if( container->contents != NULL ) {
                // now see if we can cast it
                if( IMSpacer* spacer = dynamic_cast<IMSpacer*>( container->contents ) ) {
                    if( spacer != NULL && spacer->isStatic == false ) {
                        // queue this up
                        dynamicSpacers.push_back( spacer );
                        // skip the rest of the loop so we don't add this to the total
                        continue;
                    }
                }
            }
            
            // add to our total
            totalSize += container->getSizeY();
            
        }
        
        float addedSpace = 0;
        // see if we've got dynamic spacers and spare space
        if( dynamicSpacers.size() > 0 && totalSize < parent->getSizeY() ) {
            float spacerSize = (parent->getSizeY() - totalSize )/((float) dynamicSpacers.size() );
            for(auto & dynamicSpacer : dynamicSpacers) {
                dynamicSpacer->setSizeY( spacerSize );
                addedSpace += spacerSize;
            }
        }
        
        totalSize += addedSpace;
        
        setSizeY( totalSize );
        
        // Now just fit the divider to the widest element
        float maxSize = UNDEFINEDSIZE;
        for(auto & container : containers) {
            if( container->getSizeX() > maxSize ) {
                maxSize = container->getSizeX();
            }
        }
        
        // if we've grown then resize the container and its elements
        for(auto & container : containers) {
            if( container->getSizeX() != maxSize ) {
                container->setSizeX( maxSize );
            }
        }
        
        // finally, set our own dimensions
        setSizeX( maxSize );
        
        
    }
    else {
        
        // First calculate the height
        for(auto & container : containers) {
            
            // see if this element is a dynamic spacer
            // first check if there is an element in this container
            if( container->contents != NULL ) {
                // now see if we can cast it
                IMSpacer* spacer = dynamic_cast<IMSpacer*>( container->contents );
                
                if( spacer != NULL && spacer->isStatic == false ) {
                    // queue this up
                    dynamicSpacers.push_back( spacer );
                    // skip the rest of the loop so we don't add this to the total
                    continue;
                }
            }
            
            // add to our total
            totalSize += container->getSizeX();
            
        }
        
        float addedSpace = 0;
        // see if we've got dynamic spacers and spare space
        if( dynamicSpacers.size() > 0 && totalSize < parent->getSizeX() ) {
            float spacerSize = (parent->getSizeX() - totalSize )/((float)dynamicSpacers.size() );
            for(auto & dynamicSpacer : dynamicSpacers) {
                dynamicSpacer->setSizeX( spacerSize );
                addedSpace += spacerSize;
            }
        }
        
        totalSize += addedSpace;
        
        setSizeX( totalSize );
        
        // Now just fit the divider to the widest element
        float maxSize = UNDEFINEDSIZE;
        for(auto & container : containers) {
            if( container->getSizeY() > maxSize ) {
                maxSize = container->getSizeY();
            }
        }
        
        // if we've grown then resize the container and its elements
        for(auto & container : containers) {
            if( container->getSizeY() != maxSize ) {
                container->setSizeY( maxSize );
            }
        }
        
        // finally, set our own dimensions
        setSizeY( maxSize );
        
    }
    
}

/*******************************************************************************************/
/**
 * @brief  When a resize, move, etc has happened do whatever is necessary
 *
 */
void IMDivider::doRelayout() {
    
    // Invoke the parents relayout
    IMElement::doRelayout();
    
    // First pass this down to the children
    for(auto & container : containers) {
        container->doRelayout();
    }
    
    checkRegions();
    
}

/*******************************************************************************************/
/**
 * @brief  Do whatever is necessary when the resolution changes
 *
 */
void IMDivider::doScreenResize()  {
    
    // Invoke the superclass's method
    IMElement::doScreenResize();
    
    // Pass this down to the children
    for(auto & container : containers) {
        container->doScreenResize();
    }
    
    onRelayout();
    
}


/*******************************************************************************************/
/**
 * @brief Convenience function to add a spacer element to this divider
 *
 * @param size Size of the element in terms of GUI space pixels
 *
 * @returns the space object created, just in case you need it
 *
 */
IMSpacer* IMDivider::appendSpacer( float _size ) {
    
    // Create a new spacer object
    IMSpacer* newSpacer = new IMSpacer( orientation, _size );
    
    // Add this to the divider (with a referene)
    newSpacer->AddRef();
    
    IMContainer* newContainer = append( newSpacer );
    newContainer->Release();
    
    // return a reference to this object in case the
    //  user needs to reference it (get the name, etc)
    return newSpacer;
    
}

/*******************************************************************************************/
/**
 * @brief Convenience function to add a dynamic spacer element to this divider
 *         A dynamic spacer will eat up any extra space in the divider
 *
 * @param size Size of the element in terms of GUI space pixels
 *
 * @returns the space object created, just in case you need it
 *
 */
IMSpacer* IMDivider::appendDynamicSpacer() {
    // Create a new spacer object
    IMSpacer* newSpacer = new IMSpacer( orientation );
    
    // Add this to the divider
    append( newSpacer );
    
    // return a reference to this object in case the
    //  user needs to reference it (get the name, etc)
    return newSpacer;
}

/*******************************************************************************************/
/**
 * @brief  Get the number of containers in this divider
 *
 * @returns count of the containers
 *
 */
unsigned int IMDivider::getContainerCount() {
    return containers.size();
}

/*******************************************************************************************/
/**
 * @brief  Fetch the container at the given index
 *
 *
 * @param i index of the container
 *
 */
IMContainer* IMDivider::getContainerAt( unsigned int i ) {
    if( i >= containers.size() ) {
        return NULL;
    }
    else {
        containers[i]->AddRef();
        return containers[i];
    }
}

/*******************************************************************************************/
/**
 * @brief  Gets the container of a named element
 *
 * @param _name Name of the element
 *
 * @returns container of the element (NULL if none)
 *
 */
IMContainer* IMDivider::getContainerOf( std::string const& _name ) {
    for(auto & container : containers) {
        
        if( container->contents != NULL && container->contents->getName() == _name ) {
            return container;
        }
    }
    return NULL;
}

/*******************************************************************************************/
/**
 * @brief Adds an element to the divider
 *
 * @param newElement IMElement to add
 * @param direction Portion of the divider to add to (default top/left)
 *
 */
IMContainer* IMDivider::append( IMElement* newElement, float containerSize ) {

    // Link to this element/owning GUI
    newElement->setOwnerParent( owner, this );
    
    // Make sure it's in front of us
    newElement->setZOrdering( getZOrdering() + 1 );
    
    // Make sure the element has a name
    if( newElement->getName() == "" ) {
        // build a new name
        contentsNum++;
        
        std::ostringstream oss;
        oss << name << "_container_" << contentsNum;
        newElement->name = oss.str();
    }

    // Make a new container for it
    IMContainer* newContainer;
    
    if( orientation == DOVertical ) {
        newContainer = new IMContainer( SizePolicy( UNDEFINEDSIZE ).expand().overflowClip(true),
                                        SizePolicy( containerSize ).expand().overflowClip(true) );
    }
    else {
        newContainer = new IMContainer( SizePolicy( containerSize ).expand().overflowClip(true),
                                        SizePolicy( UNDEFINEDSIZE ).expand().overflowClip(true) );
    }
    
    // Add a reference as setElement is expecting it
    newElement->AddRef();
    newContainer->setElement( newElement );
    newContainer->setZOrdering( getZOrdering() );
    
    // Link to this element/owning GUI
    newContainer->setOwnerParent( owner, this );
    
    vec2 newSize;
    
    if( orientation == DOHorizontal ) {
        // If we're not given a size, fit to the element size
        if( containerSize == UNDEFINEDSIZE ) {
            newSize.x() = newElement->getSizeX();
        }
        else {
            newSize.x() = containerSize;
        }
        
        // Base the height on our current one
        newSize.y() = getSizeY();
    }
    else {
        // If we're not given a size, fit to the element size
        if( containerSize == UNDEFINEDSIZE ) {
            newSize.y() = newElement->getSizeY();
        }
        else {
            newSize.y() = containerSize;
        }
        
        // Base the width on our current one
        newSize.x() = getSizeX();
    }
    
    newContainer->setSize( newSize );
    newContainer->setAlignment( contentsXAlignment, contentsYAlignment );
    
    // Make sure we keep a reference to this
    newContainer->AddRef();

    containers.push_back( newContainer );
    
    // Signal that something new has changed
    onRelayout();

    // get rid of the reference we were given 
    newElement->Release();
    
    return newContainer;
    
}


/*******************************************************************************************/
/**
 * @brief  Find an element by name — called internally
 *
 *
 */
IMElement* IMDivider::findElement( std::string const& elementName ) {
    
    // Check if this is the droid we're looking for
    if( name == elementName ) {
        // Up our reference count
        AddRef();
        return this;
    }
    else {
        // If not, pass the request onto the children
        
        for(auto & container : containers) {
            
            IMElement* results = container->findElement( elementName );
            
            if( results != NULL ) {
                return results;
            }
        }
        
        // if we've got this, far we don't have it and so report
        return NULL;
    }
}

void IMDivider::setPauseBehaviors( bool pause ) {
    IMElement::setPauseBehaviors( pause );
    for(auto & container : containers) {
        container->setPauseBehaviors( pause );
    }
}

/*******************************************************************************************/
/**
 * @brief  Remove all referenced object without releaseing references
 *
 */
void IMDivider::clense() {
    IMElement::clense();
    containers.clear();
    
}

/*******************************************************************************************/
/**
 * @brief  Destructor
 *
 */
IMDivider::~IMDivider() {
    IMrefCountTracker.removeRefCountObject( getElementTypeName() );

    std::vector<IMContainer*> deletelist = containers;
    containers.clear();

    for(auto & it : deletelist) {
        it->Release();
    }
}

void IMDivider::DestroyedIMElement( IMElement* element ) {
    for( int i = containers.size()-1; i >= 0; i-- ) {
        if( containers[i] == element ) {
            containers.erase(containers.begin()+i);
        }
    }

    IMElement::DestroyedIMElement(element);
}

void IMDivider::DestroyedIMGUI( IMGUI* imgui ) {

    IMElement::DestroyedIMGUI(imgui);
}
