//-----------------------------------------------------------------------------
//           Name: im_container.cpp
//      Developer: Wolfire Games LLC
//    Description: Most basic container, fixed size that simply serves to 
//                 contain a single element (and supports a number of 
//                 'floating' elements)
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
#include "im_container.h"

#include <GUI/IMUI/imgui.h>


/*******************************************************************************************/
/**
 * @brief  Constructor
 *
 * @param _defaultSize
 *
 */

SizePolicy::SizePolicy() :
    errorOnOverflow(true),
    defaultSize(UNDEFINEDSIZE),
    maxSize(UNDEFINEDSIZE),
    expansionPolicy(ContainerExpansionStatic)
{
}

SizePolicy::SizePolicy( float _defaultSize ) :
    errorOnOverflow(true),
    expansionPolicy(ContainerExpansionStatic)
{
    defaultSize = _defaultSize;
    maxSize = _defaultSize;
}

SizePolicy SizePolicy::expand( float _maxSize ) {
    expansionPolicy = ContainerExpansionExpand;
    maxSize = _maxSize;
    return *this;
}

SizePolicy SizePolicy::inheritMax() {
    expansionPolicy = ContainerExpansionInheritMax;
    return *this;
}

SizePolicy SizePolicy::staticMax() {
    expansionPolicy = ContainerExpansionStatic;
    return *this;
}

SizePolicy SizePolicy::overflowClip( bool shouldClip ) {
    errorOnOverflow = !shouldClip;
    return *this;
}

AHFloatingElement::AHFloatingElement() :
    element(NULL),
    centerVal( std::make_pair(false, false) )
{}

void AHFloatingElement::setElement( IMElement* _element ) {
    if( element != NULL ) {
        element->Release();
    }

    if( _element != NULL ) {
        _element->AddRef();
    }

    element = _element;
}

AHFloatingElement::~AHFloatingElement() {
    if( element != NULL ) {
        element->Release();
    }
}

/*******************************************************************************************/
/**
 * @brief  Constructor
 *  
 * @param name Element name
 * @param size Size of this container 
 *
 */
IMContainer::IMContainer( std::string const& name, SizePolicy sizeX, SizePolicy sizeY ) :
    IMElement( name ),
    contents(NULL),
    contentsXAlignment(CACenter),
    contentsYAlignment(CACenter),
    backgroundImage(NULL),
    sizePolicyX(sizeX),
    sizePolicyY(sizeY)
{
    IMrefCountTracker.addRefCountObject( getElementTypeName() );
    setSize( vec2( sizePolicyX.defaultSize, sizePolicyY.defaultSize ) );
}

/*******************************************************************************************/
/**
 * @brief  Constructor
 *
 * @param size Size of this container 
 * 
 */
IMContainer::IMContainer( SizePolicy sizeX, SizePolicy sizeY ) :
    contents(NULL),
    contentsXAlignment(CACenter),
    contentsYAlignment(CACenter),
    backgroundImage(NULL),
    sizePolicyX(sizeX),
    sizePolicyY(sizeY)
{
    IMrefCountTracker.addRefCountObject( getElementTypeName() );
    setSize( vec2( sizePolicyX.defaultSize, sizePolicyY.defaultSize ) );
}

/*******************************************************************************************/
/**
 * @brief  Set’s this element’s parent (and does nessesary logic)
 *  
 * @param _parent New parent
 *
 */
void IMContainer::setOwnerParent( IMGUI* _owner, IMElement* _parent ) {
    
    owner = _owner;
    parent = _parent;

    // Simply pass this on to the children
    if( contents != NULL ) {
        contents->setOwnerParent( _owner, this );
    }
    
    for( FEMap::iterator it = floatingContents.begin();
         it != floatingContents.end();
         ++it ) {
        it->second->getElement()->setOwnerParent(_owner, this);
    }

}


/*******************************************************************************************/
/**
 * @brief  Set’s the size policy objects for this container
 * 
 * @param sizeX x dimension policy
 * @param sizeY y dimension policy 
 *
 */
void IMContainer::setSizePolicy( SizePolicy sizeX, SizePolicy sizeY ) {

    sizePolicyX = sizeX;
    sizePolicyY = sizeY;

    setSize( vec2( sizePolicyX.defaultSize, sizePolicyY.defaultSize ) );

}

/*******************************************************************************************/
/**
 * @brief  Gets the name of the type of this element — for autonaming and debugging
 * 
 * @returns name of the element type as a string
 *
 */
std::string IMContainer::getElementTypeName() {
    return "Container";
}

/*******************************************************************************************/
/**
 * @brief  Sets the size of the region (not including padding)
 * 
 * @param _size 2d size vector (-1 element implies undefined - or use UNDEFINEDSIZE)
 *
 */
void IMContainer::setSize( const vec2 _size ) {
    
    //resize the container 
    IMElement::setSize( _size );

    // Now reposition our contents
    positionByAlignment();

}

/*******************************************************************************************/
/**
 * @brief  Sets the x dimension of a region
 * 
 * @param x x dimension size (-1 implies undefined - or use UNDEFINEDSIZE)
 *
 */
void IMContainer::setSizeX( const float x ) {

    //resize the container 
    IMElement::setSize( vec2( x, size.y() ) );

    // Now reposition our contents
    positionByAlignment();

}   

/*******************************************************************************************/
/**
 * @brief  Sets the y dimension of a region
 * 
 * @param y y dimension size (-1 implies undefined - or use UNDEFINEDSIZE)
 * @param resetBoundarySize Should we reset the boundary if it's too small?
 *
 */
void IMContainer::setSizeY( const float y ) {

    //resize the container 
    IMElement::setSize( vec2( size.x(), y ) );

    // Now reposition our contents
    positionByAlignment();
}  


/*******************************************************************************************/
/**
 * @brief  Clear the contents of this container, leaving everything else the same
 * 
 */
void IMContainer::clear() {
    // clear the floating contents
    //
    FEMap deletemap = floatingContents;

    floatingContents.clear();
    
    for( FEMap::iterator it = deletemap.begin();
         it != deletemap.end();
         ++it ) {
        delete it->second;
    }

    // clear the contained element
    if( contents != NULL ) {
        contents->Release();
    }

    contents = NULL; 
    contentsXAlignment = CACenter;
    contentsYAlignment = CACenter;
    contentsOffset = vec2(0,0);

    if( backgroundImage != NULL ) {
        backgroundImage->Release();
    }

    // clear the background
    backgroundImage = NULL;

    setSize(getDefaultSize());
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
void IMContainer::update( uint64_t delta, vec2 drawOffset, GUIState& guistate ) {

    bool mouseOverState = guistate.inheritedMouseOver;
    bool mouseDownState = guistate.inheritedMouseDown;
    IMUIContext::ButtonState currentMouseState = guistate.inheritedMouseState;
    
    // Do whatever the superclass wants
    IMElement::update( delta, drawOffset, guistate );
    
    // Simply pass this on to the children
    if( contents != NULL ) {
        contents->update( delta, drawOffset + drawDisplacement + contentsOffset, guistate);
    }   

    for( FEMap::iterator it = floatingContents.begin();
         it != floatingContents.end();
         ++it ) {
        AHFloatingElement* floatingElement = it->second;
        floatingElement->getElement()->update( delta, drawOffset + drawDisplacement + floatingElement->relativePos, guistate );

        if(floatingContents.empty()) {
            break;
        }
    }
    
    guistate.inheritedMouseOver = mouseOverState;
    guistate.inheritedMouseDown = mouseDownState;
    guistate.inheritedMouseState = currentMouseState;

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
void IMContainer::render( vec2 drawOffset, vec2 clipPos, vec2 clipSize ) {

    // See if we need to adjust clip for this container
    vec2 currentClipPos;
    vec2 currentClipSize;
    
    if( getSizeX() == UNDEFINEDSIZE || getSizeY() == UNDEFINEDSIZE ) {
        currentClipPos = clipPos;
        currentClipSize = clipSize;
    }
    else {
        currentClipPos = drawOffset + drawDisplacement;
        currentClipSize = getSize();
    }

    // Do whatever the superclass wants
    IMElement::render( drawOffset, currentClipPos, currentClipSize );

    // Render the background, if any
    if( backgroundImage != NULL ) {
        backgroundImage->render( drawOffset + drawDisplacement, currentClipPos, currentClipSize );
    }

    // Render the contents 
    if( contents != NULL ) {
        contents->render( drawOffset + drawDisplacement + contentsOffset, currentClipPos, currentClipSize );
    }

    // Render the floating elements
    for( FEMap::iterator it = floatingContents.begin();
         it != floatingContents.end();
         ++it ) {
        AHFloatingElement* floatingElement = it->second;
        floatingElement->getElement()->render( drawOffset + drawDisplacement + floatingElement->relativePos, currentClipPos, currentClipSize );
    }

}


/*******************************************************************************************/
/**
 * @brief  When a resize, move, etc has happened do whatever is necessary
 * 
 */
 void IMContainer::doRelayout() {

    // Invoke the elements relayout
    IMElement::doRelayout();

    // Make sure background image has kept up with any resizing 
    if( backgroundImage != NULL ) { 
        backgroundImage->setSize( getSize() );
    }
     
    // Make sure background image has kept up with any resizing
    if( contents != NULL ) {
        contents->doRelayout();
    }
    
    // Now trigger the children
    for( FEMap::iterator it = floatingContents.begin();
         it != floatingContents.end();
         ++it ) {
        
        AHFloatingElement* felement = it->second;
        
        
        // see if we need to center this element
        if( felement->centering().first &&
            getSizeX() != UNDEFINEDSIZE &&
            felement->getElement()->getSizeX() != UNDEFINEDSIZE ) {
                
            float center = getSizeX()/2.0f;
            felement->relativePos.x() = center - felement->getElement()->getSizeX()/2.0f;
                
        }
            
        if( felement->centering().second &&
            getSizeY() != UNDEFINEDSIZE &&
            felement->getElement()->getSizeY() != UNDEFINEDSIZE ) {
            
            float center = getSizeY()/2.0f;
            felement->relativePos.y() = center - felement->getElement()->getSizeY()/2.0f;
        }
        
        felement->getElement()->doRelayout();
    }

    // now make sure everything is fine with our container
    checkRegion();

 }

/*******************************************************************************************/
/**
 * @brief Rederive the regions for the various orientation containers - for internal use
 * 
 */
void IMContainer::checkRegion() {

    if( contents == NULL ) {
        // we have nothing to do 
        return;
    }
    
    // first reposition our element
    positionByAlignment();

    // now see if we exceed the dimensions of container in any direction 
    vec2 UL = contentsOffset;
    vec2 LR = contentsOffset + contents->getSize();

    // see if we're over in the x dimension 
    if( UL.x() != UNDEFINEDSIZE && ( UL.x() < 0 || LR.x() > getSizeX() ) ) {

        float neededSize = contents->getSizeX();
        float maxSize = UNDEFINEDSIZE;

        if( sizePolicyX.expansionPolicy == ContainerExpansionInheritMax ) {
            if( parent != NULL ) {
                maxSize = parent->getSizeX();
            }
            else {
                maxSize = sizePolicyX.maxSize;
            }
        }

        // if we have no max, just treat it as the size we need
        if( maxSize == UNDEFINEDSIZE ) {
            maxSize = contents->getSizeX();    
        }
        
        float newSize = min( neededSize, maxSize );
        if( newSize != getSizeX() ) {

            setSizeX( newSize );

            // see if we need to (and should) through an overflow error 
            if( getSizeX() > maxSize && sizePolicyX.errorOnOverflow ) {
                onError("Container overflow for object " +  contents->name );
            }
        }

    }

    // see if we're over in the y dimension 
    if( UL.y() != UNDEFINEDSIZE && ( UL.y() < 0 || LR.y() > getSizeY() ) ) {

        float neededSize = contents->getSizeY();
        float maxSize = UNDEFINEDSIZE;

        if( sizePolicyY.expansionPolicy == ContainerExpansionInheritMax ) {
            if( parent != NULL ) {
                maxSize = parent->getSizeY();
            }
            else {
                maxSize = sizePolicyY.maxSize;
            }
        }

        // if we have no max, just treat it as the size we need
        if( maxSize == UNDEFINEDSIZE ) {
            maxSize = contents->getSizeY();    
        }

        float newSize = min( neededSize, maxSize );
        
        if( newSize != getSizeY() ) {

            setSizeY( newSize );

            // see if we need to (and should) through an overflow error 
            if( getSizeY() > maxSize && sizePolicyX.errorOnOverflow ) {
                onError("Container overflow for object " +  contents->name );
            }
        }
    }        
}

/*******************************************************************************************/
/**
 * @brief  Do whatever is necessary when the resolution changes
 *
 */
 void IMContainer::doScreenResize()  {
    
    // Invoke the superclass's method
    IMElement::doScreenResize();
    
    // Now trigger the children
    if( contents != NULL ) {
        contents->doScreenResize();
    }

    for( FEMap::iterator it = floatingContents.begin();
         it != floatingContents.end();
         ++it ) {
        it->second->getElement()->doScreenResize();
    }

 }


/*******************************************************************************************/
/**
 * @brief Adds an arbitrarily placed element to the container 
 *  
 * @param newElement Element to add  
 * @param name Name of the element (this *will* rename the element )
 * @param position Position relative to the upper right of this container (GUI Space) - if
 *         a parameter is undefined, it will center on that axis
 * @param z z ordering for the new element (relative to this container -- optional )
 *
 */
void IMContainer::addFloatingElement( IMElement* element, std::string const& _name, vec2 position, int z ) {
    
    std::pair<bool, bool> centering;
    
    // see if we've got real coordinates and if not, make some
    if( position.x() == UNDEFINEDSIZE ) {
        centering.first = true;
        position.x() = 0.0f;
    }
    else {
        centering.first = false;
    }
    
    if( position.y() == UNDEFINEDSIZE ) {
        centering.second = true;
        position.y() = 0.0f;
    }
    else {
        centering.second = false;
    }

    // Make sure the element has the name 
    element->setName( _name );

    AHFloatingElement* floatingElement = new AHFloatingElement();

    floatingElement->relativePos = position;
    floatingElement->setElement( element );
    floatingElement->centering() = centering;
    
    // See if we're replacing an element 
    FEMap::iterator it = floatingContents.find(_name);
    if( it != floatingContents.end() ) {
        delete it->second;
    }

    floatingContents[ _name ] = floatingElement;
    
    // Link to this element/owning GUI
    element->setOwnerParent( owner, this );

    // Make sure it's in front of us
    if( z < 1 ) {
        element->setZOrdering( getZOrdering() + element->getZOrdering() );
    }
    else {
        element->setZOrdering( getZOrdering() + z );    
    }
    
    // Signal that something new has changed
    onRelayout();

    // release the ref we were given for this method
    element->Release();

}

/*******************************************************************************************/
/**
 * @brief  Sets the contained element
 * 
 * @param element, element to add 
 *
 */
void IMContainer::setElement( IMElement* element ) {
    if(contents) {
        contents->Release();
    }
    contents = element;
    contents->setOwnerParent( owner, this );
    positionByAlignment();
    onRelayout();
}

/*******************************************************************************************/
/**
 * @brief  Adjust the offset position according to the alignments - used internally
 * 
 */
void IMContainer::positionByAlignment() {

    if( contents == NULL ) {
        contentsOffset = vec2(0,0);
        return;
    }

    // compute the horizontal position
    if( contentsXAlignment == CALeft ) {
        contentsOffset.x() = 0;
    }
    else if( contentsXAlignment == CARight ) {
        contentsOffset.x() = getSizeX() - contents->getSizeX();
    }
    else {
        contentsOffset.x() = (getSizeX() / 2)-(contents->getSizeX()/2);
    }

    // figure out the vertical position
    if( contentsYAlignment == CATop ) {
        contentsOffset.y() = 0.0;
    }
    else if( contentsYAlignment == CABottom ) {
        contentsOffset.y() = getSizeY() - contents->getSizeY();
    }
    else {
        contentsOffset.y() = (getSizeY() / 2)-(contents->getSizeY()/2);
    }

}

/*******************************************************************************************/
/**
 * @brief  Set’s the alignment of the contained object
 *  
 * @param xAlignment horizontal alignment 
 * @param yAlignment vertical alignment
 *
 */
void IMContainer::setAlignment( ContainerAlignment xAlignment, ContainerAlignment yAlignment ) {
    contentsXAlignment = xAlignment;
    contentsYAlignment = yAlignment;

    positionByAlignment();
}


/*******************************************************************************************/
/**
 * @brief Removes an element
 * 
 * @param name Name of the element to remove 
 *
 * @returns the element if it's there, NULL otherwise
 * 
 */
 IMElement* IMContainer::removeElement( std::string const&  name ) {

    // make sure it exists
    FEMap::iterator it = floatingContents.find(name);
    if( it != floatingContents.end() ) {

        AHFloatingElement* floatingElement = it->second;
        IMElement* element = floatingElement->getElement();
        
        // since we're returning it
        element->AddRef();
        
        // get rid of the storage
        floatingContents.erase(it);
        delete floatingElement;

        // Signal that something new has changed
        onRelayout();

        return element;
    }
    else {
        // If not, let the caller know
        return NULL;
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
void IMContainer::moveElement( std::string const&  name, vec2 newPos ) {
    // make sure it exists
    FEMap::iterator it = floatingContents.find(name);
    if( it != floatingContents.end() ) {
        AHFloatingElement* floatingElement = it->second;
        floatingElement->relativePos = newPos;
    }
    else {
        // If not, throw an error
        IMDisplayError("GUI Error", "Unable to find element " + name + " to move");
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
void IMContainer::moveElementRelative( std::string const& name, vec2 posChange ) {
    // make sure it exists
    FEMap::iterator it = floatingContents.find(name);
    if( it != floatingContents.end() ) {
        AHFloatingElement* floatingElement = it->second;
        floatingElement->relativePos = floatingElement->relativePos + posChange;
    }
    else {
        // If not, throw an error
        IMDisplayError("GUI Error", "Unable to find element " + name + " to move");
    }
}

vec2 IMContainer::getElementPosition( std::string const& name ) {
    // make sure it exists
    FEMap::iterator it = floatingContents.find(name);
    if( it != floatingContents.end() ) {
        AHFloatingElement* floatingElement = it->second;
        return floatingElement->relativePos;
    }
    else {
        // If not, throw an error
        IMDisplayError("GUI Error", "Unable to find element " + name + " to move");
        return vec2( UNDEFINEDSIZE, UNDEFINEDSIZE );
    }
}


/*******************************************************************************************/
/**
 * @brief  Find an element by name — called internally
 *  
 *
 */
IMElement* IMContainer::findElement( std::string const&  elementName ) {
    // Check if this is the droid we're looking for
    if( name == elementName ) {
        AddRef();
        return this;
    }
    else {
        // If not, look at the children
        FEMap::iterator it = floatingContents.find(name);
        if( it != floatingContents.end() ) {
            AHFloatingElement* floatingElement = it->second;
            IMElement* element = floatingElement->getElement();
            element->AddRef();
            
            return element;
        }
        else {
            // If not, let the caller know
            return NULL;
        }
    }

}

/*******************************************************************************************/
/**
 * @brief  Set a backgound image for this container
 * 
 * @param fileName file name for the image (empty string to clear)
 * @param color 4 component color vector  
 *
 */
void IMContainer::setBackgroundImage( std::string const& fileName, vec4 color  ) {
    // See if we already have a background
    if( backgroundImage != NULL ) {
        backgroundImage->Release();
    }

    if( fileName == "" ) {
        backgroundImage = NULL;
        return;
    }

    if( getSizeX() == UNDEFINEDSIZE || getSizeY() == UNDEFINEDSIZE ) {
        IMDisplayError("GUI Error", "Cannot add a background image to a container with an undefined dimension" );  
    }

    backgroundImage = new IMImage( fileName );

    // Load the image
    backgroundImage->setImageFile( fileName );

    // Set the size to be the same as this container
    backgroundImage->setSize( getSize() );

    // Set the color
    backgroundImage->setColor( color );

    // Set the appropriate rendering order
    backgroundImage->setZOrdering( getZOrdering() );

}

void IMContainer::setPauseBehaviors( bool pause ) {
    IMElement::setPauseBehaviors( pause );
    if( contents != NULL ) {
        contents->setPauseBehaviors( pause );
    }

    for( FEMap::iterator it = floatingContents.begin();
         it != floatingContents.end();
         ++it ) {
        AHFloatingElement* floatingElement = it->second;
        floatingElement->getElement()->setPauseBehaviors( pause );
    }
}

/*******************************************************************************************/
/**
 * @brief  Remove all referenced object without releaseing references
 *
 */
void IMContainer::clense() {

    IMElement::clense();
    
    floatingContents.clear();

    contents = NULL;
    backgroundImage = NULL;
    
}

std::vector<IMElement*> IMContainer::getFloatingContents() {
    std::vector<IMElement*> elements;
    elements.reserve(floatingContents.size());
    for(FEMap::iterator iter = floatingContents.begin(); iter != floatingContents.end(); ++iter) {
        IMElement* element = iter->second->getElement();
        //element->AddRef();
        elements.push_back(element);
    }
    return elements;
}

/*******************************************************************************************/
/**
 * @brief  Destructor
 *
 */
IMContainer::~IMContainer() {
    IMrefCountTracker.removeRefCountObject( getElementTypeName() );
    clear();
}


void IMContainer::DestroyedIMElement( IMElement* element ) {
    if( element == contents ) {
        contents = NULL;
    }

    if( element == backgroundImage ) {
        backgroundImage = NULL;
    }

    FEMap::iterator feit; 

    for( feit = floatingContents.begin(); feit != floatingContents.end(); feit++ ) {
        if( feit->second->getElement() == element ) {
            floatingContents.erase(feit);
            feit = floatingContents.begin();
        }
    }

    IMElement::DestroyedIMElement(element);
}

void IMContainer::DestroyedIMGUI( IMGUI* imgui ) {

    IMElement::DestroyedIMGUI(imgui);
}


