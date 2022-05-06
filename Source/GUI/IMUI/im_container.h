//-----------------------------------------------------------------------------
//           Name: im_container.h
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
#pragma once

#include <GUI/IMUI/im_support.h>
#include <GUI/IMUI/im_element.h>
#include <GUI/IMUI/im_image.h>

#include <map>

// How does this subdivision grow when the internal size gets bigger
enum ExpansionPolicy {
    ContainerExpansionStatic,  // Stay at the given size
    ContainerExpansionExpand,  // Expand with the contents (if optional )
    ContainerExpansionInheritMax  // Max out at the size of the owning container
};


/*******************************************************************************************/
/**
 * @brief  Base class for the sizing/resizing policy for a subdivider
 *
 */
struct SizePolicy {
    bool errorOnOverflow; // should an error be thrown if the container is overfull
    float defaultSize;    // what is in the initial(default) size
    float maxSize;        // what is the biggest this container can get
    ExpansionPolicy expansionPolicy;  // see
    
    
    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *
     * @param _defaultSize
     *
     */
    SizePolicy();
    SizePolicy( float _defaultSize );
    
    /*******************************************************************************************/
    /**
     * @brief  Copy constructor
     *
     */
    SizePolicy( const SizePolicy& other ) {
        errorOnOverflow = other.errorOnOverflow;
        defaultSize = other.defaultSize;
        maxSize = other.maxSize;
        expansionPolicy = other.expansionPolicy;
    }
    
    
    SizePolicy expand( float _maxSize = UNDEFINEDSIZE );
    SizePolicy inheritMax();
    SizePolicy staticMax();
    SizePolicy overflowClip( bool shouldClip );
    
    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *
     */
    virtual ~SizePolicy() {}
    
};

static void SizePolicy_ASfactory_noparams( SizePolicy *self ) {
    new(self) SizePolicy();
}

static void SizePolicy_ASfactory_params( SizePolicy *self, float _defaultSize ) {
    new(self) SizePolicy( _defaultSize );
}

static void SizePolicy_ASdestructor( SizePolicy *self) {
    self->~SizePolicy();
}

static void SizePolicy_ASCopyConstructor( SizePolicy *self, const SizePolicy& other ) {
    new(self) SizePolicy( other );
}

static const SizePolicy& SizePolicy_ASAssign( SizePolicy *self, const SizePolicy& other) {
    return (*self) = other;
}



/*******************************************************************************************/
/**
 * @brief  Hold the information for the contained element
 *
 */
struct AHFloatingElement {
private:
    IMElement* element;             // Handle to the element itself
    std::pair<bool, bool> centerVal; // Is x, y centered in the container?

public:
    AHFloatingElement();
    void setElement( IMElement* _element = NULL );
    IMElement* getElement() { return element; }
    std::pair<bool, bool>& centering() { return centerVal; }
    vec2 relativePos;  // Where is this element offset from the upper/right of this container
    virtual ~AHFloatingElement();
};

typedef std::map<std::string, AHFloatingElement*> FEMap;

/*******************************************************************************************/
/**
 * @brief  Basic container class, holds other elements
 *
 */
class IMContainer : public IMElement {
    
public:
    
    FEMap floatingContents; // floating elements in this container
    IMElement* contents; // what's contained in this container
    vec2 contentsOffset;
    ContainerAlignment contentsXAlignment; // horizontal alignment of element
    ContainerAlignment contentsYAlignment; // vertical alignment of element
    IMImage* backgroundImage; // image to be put on the background

    SizePolicy sizePolicyX;     // how should this container grow in the x direction
    SizePolicy sizePolicyY;     // how should this container grow in the y direction
    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *  
     * @param name Element name
     * @param size Size of this container 
     *
     */
    IMContainer( std::string const& name, SizePolicy sizeX = SizePolicy(), SizePolicy sizeY  = SizePolicy() );

    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *
     * @param size Size of this container 
     * 
     */
    IMContainer( SizePolicy sizeX = SizePolicy(), SizePolicy sizeY  = SizePolicy() );

    /*******
     *  
     * Angelscript factory 
     *
     */
     static IMContainer* ASFactory_named( std::string const& name, SizePolicy sizeX = SizePolicy(), SizePolicy sizeY  = SizePolicy() ) {
        return new IMContainer( name, sizeX, sizeY );
     }

     static IMContainer* ASFactory_unnamed( SizePolicy sizeX = SizePolicy(), SizePolicy sizeY  = SizePolicy() ) {
        return new IMContainer( sizeX, sizeY );
     }

    /*******************************************************************************************/
    /**
     * @brief  Set’s this element’s parent (and does nessesary logic)
     *  
     * @param _parent New parent
     *
     */
    void setOwnerParent( IMGUI* _owner, IMElement* _parent ) override;

    /*******************************************************************************************/
    /**
     * @brief  Set’s the size policy objects for this container
     * 
     * @param sizeX x dimension policy
     * @param sizeY y dimension policy 
     *
     */
    void setSizePolicy( SizePolicy sizeX, SizePolicy sizeY );

    /*******************************************************************************************/
    /**
     * @brief  Gets the name of the type of this element — for autonaming and debugging
     * 
     * @returns name of the element type as a string
     *
     */
    std::string getElementTypeName() override;

    /*******************************************************************************************/
    /**
     * @brief  Sets the size of the region (not including padding)
     * 
     * @param _size 2d size vector (-1 element implies undefined - or use UNDEFINEDSIZE)
     *
     */
    void setSize( const vec2 _size ) override;

    /*******************************************************************************************/
    /**
     * @brief  Sets the x dimension of a region
     * 
     * @param x x dimension size (-1 implies undefined - or use UNDEFINEDSIZE)
     *
     */
    void setSizeX( const float x ) override;

    /*******************************************************************************************/
    /**
     * @brief  Sets the y dimension of a region
     * 
     * @param y y dimension size (-1 implies undefined - or use UNDEFINEDSIZE)
     * @param resetBoundarySize Should we reset the boundary if it's too small?
     *
     */
    void setSizeY( const float y ) override;

    /*******************************************************************************************/
    /**
     * @brief  Clear the contents of this container, leaving everything else the same
     * 
     */
    void clear();

    /*******************************************************************************************/
    /**
     * @brief  Updates the element  
     * 
     * @param delta Number of millisecond elapsed since last update
     * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
     * @param guistate The state of the GUI at this update
     *
     */
    void update( uint64_t delta, vec2 drawOffset, GUIState& guistate ) override;

    /*******************************************************************************************/
    /**
     * @brief  Rather counter-intuitively, this draws this object on the screen
     *
     * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
     * @param clipPos pixel location of upper lefthand corner of clipping region
     * @param clipSize size of clipping region
     *
     */
    void render( vec2 drawOffset, vec2 clipPos, vec2 clipSize ) override;

    /*******************************************************************************************/
    /**
     * @brief  When a resize, move, etc has happened do whatever is necessary
     * 
     */
     void doRelayout() override;

    /*******************************************************************************************/
    /**
     * @brief Rederive the regions for the various orientation containers - for internal use
     * 
     */
    void checkRegion();


    /*******************************************************************************************/
    /**
     * @brief  Do whatever is necessary when the resolution changes
     *
     */
     void doScreenResize() override;

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
    void addFloatingElement( IMElement* element, std::string const& name, vec2 position, int z = -1 );

    /*******************************************************************************************/
    /**
     * @brief  Sets the contained element
     * 
     * @param element, element to add 
     *
     */
    void setElement( IMElement* element );

    /*******************************************************************************************/
    /**
     * @brief  Adjust the offset position according to the alignments - used internally
     * 
     */
    void positionByAlignment();

    /*******************************************************************************************/
    /**
     * @brief  Set’s the alignment of the contained object
     *  
     * @param xAlignment horizontal alignment 
     * @param yAlignment vertical alignment
     *
     */
    void setAlignment( ContainerAlignment xAlignment, ContainerAlignment yAlignment );

    /*******************************************************************************************/
    /**
     * @brief Removes an element
     * 
     * @param name Name of the element to remove 
     *
     * @returns the element if it's there, NULL otherwise
     * 
     */
     IMElement* removeElement( std::string const& name );

    /*******************************************************************************************/
    /**
     * @brief  Moves and element to a new position
     * 
     * @param name Name of the element to move
     * @param newPos new position of the element
     *
     */
    void moveElement( std::string const& name, vec2 newPos );

    /*******************************************************************************************/
    /**
     * @brief  Moves and element to a new position relative to its old one
     * 
     * @param name Name of the element to move
     * @param posChange change in element position
     *
     */
    void moveElementRelative( std::string const& name, vec2 posChange );
    
    /*******************************************************************************************/
    /**
     * @brief  Get floating element position by name
     *
     * @param name Name of the element to move
     *
     * @returns position of the element (UNDEFINEDSIZE, UNDEFINEDSIZE) if not found (and an error)
     *
     */
    vec2 getElementPosition( std::string const& name );

    /*******************************************************************************************/
    /**
     * @brief  Find an element by name — called internally
     *  
     *
     */
    IMElement* findElement( std::string const& elementName ) override;

    /*******************************************************************************************/
    /**
     * @brief  Set a backgound image for this container
     * 
     * @param fileName file name for the image (empty string to clear)
     *
     */
    void setBackgroundImage( std::string const& fileName, vec4 color = vec4(1.0) );

    void setPauseBehaviors( bool pause ) override;
    
    /*******************************************************************************************/
    /**
     * @brief  Remove all referenced object without releaseing references
     *
     */
    void clense() override;

    IMElement* getContents() { if(contents) contents->AddRef(); return contents; }
    std::vector<IMElement*> getFloatingContents();

    /*******************************************************************************************/
    /**
     * @brief  Destructor
     *
     */
    ~IMContainer() override;
    
    void DestroyedIMElement( IMElement* element ) override;
    void DestroyedIMGUI( IMGUI* imgui ) override;
};


