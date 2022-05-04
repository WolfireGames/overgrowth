//-----------------------------------------------------------------------------
//           Name: imgui.h
//      Developer: Wolfire Games LLC
//    Description: Main class for creating adhoc GUIs as part of the UI tools  
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

#include <GUI/IMUI/imui.h>
#include <GUI/IMUI/im_element.h>
#include <GUI/IMUI/im_message.h>
#include <GUI/IMUI/im_support.h>
#include <GUI/IMUI/im_container.h>
#include <GUI/IMUI/im_events.h>

/*******************************************************************************************/
/**
 * @brief One thing to rule them all
 *
 */
class IMGUI : public IMEventListener {


    uint64_t lastUpdateTime; // When was this last updated (ms)
    unsigned int elementCount; // Counter for naming unnamed elements
    bool needsRelayout; // has a relayout signal been fired?
    std::string reportedErrors; // have any errors been reported?
    bool showGuides; // overlay some extra lines to aid layout
    
    vec2 mousePosition; // Where is the mouse?
    IMUIContext::ButtonState leftMouseState;    // What's the state of the left mouse button?
    std::vector<IMMessage*> messageQueue;   // Messages waiting to process
    std::vector<IMContainer*> backgrounds;  // Containers for background layers, if any
    std::vector<IMContainer*> foregrounds;  // Containers for foreground layers, if any

    std::vector<IMContainer*> header;  // header panels
    std::vector<IMContainer*> footer;  // footer panels
    std::vector<IMContainer*> main;    // main panels

    float mainOffset;   // y offset for the main panel
    float footerOffset; // y offset for the footer panel

    float headerHeight;   // size of the header (0 if none)
    float footerHeight;   // size of the footer (0 if none)
    float mainHeight;     // size of the main panel (0 if none)
    
    std::vector<float> headerPanelWidths;   // Width of the header panels
    std::vector<float> footerPanelWidths;   // Width of the header panels
    std::vector<float> mainPanelWidths;     // Width of the header panels
    
    float headerPanelGap;   // How much space between header panels
    float footerPanelGap;   // How much space between footer panels
    float mainPanelGap;     // How much space between main panels
    
    unsigned int pendingFGLayers;   // Foreground layers to create on 'setup'
    unsigned int pendingBGLayers;   // Background layers to create on 'setup'
    float pendingHeaderHeight;        // Size of header to create on 'setup'
    float pendingFooterHeight;        // Size of footer to create on 'setup'
    
    std::vector<float> pendingHeaderPanels; // Sizes of header panels
    std::vector<float> pendingMainPanels;   // Sizes of main panels
    std::vector<float> pendingFooterPanels; // Sizes of footer panels
    
    vec2 screenSize;   // for detecting changes

    int refCount; // for AS reference counting
    
    

    
    /*******************************************************************************************/
    /**
     * @brief  If a relayout is requested, perform it
     *
     */
    void doRelayout();
    
    /*******************************************************************************************/
    /**
     * @brief  Figure out where we’re going to render the main panels - used internally
     *
     */
    void derivePanelOffsets();

private: 
    IMGUI( IMGUI const& other );
    
public:
    
    int guiid;
    
    GUIState guistate; // current state of the GUI, updated at 'update'
    
    IMUIContext* IMGUI_IMUIContext; // UI features from the engine

    /*******************************************************************************************/
    /**
     * @brief Constructor
     *  
     * @param mainOrientation The orientation of the container
     *
     */
    IMGUI();
    
    
    /*******
     *
     * Angelscript memory management boilerplate
     *
     */
    void AddRef();
    void Release();
    
    /*******************************************************************************************/
    /**
     * @brief  Set the number of background layers
     *
     *  NOTE: This will take effect when setup is called next
     *
     *  Background layers are rendered highest index value first
     *
     * @param numLayers number of layers required
     *
     */
    void setBackgroundLayers( unsigned int numLayers );
    
    /*******************************************************************************************/
    /**
     * @brief  Set the number of foreground layers
     *
     *  NOTE: This will take effect when setup is called next
     *
     *  Foreground layers are rendered lowest index value first
     *
     * @param numLayers number of layers required
     *
     */
    void setForegroundLayers( unsigned int numLayers );
    
    /*******************************************************************************************/
    /**
     * @brief  Set the size of the header (0 for none)
     *
     *  NOTE: This will take effect when setup is called next
     *
     * @param _headerSize size in GUI space
     *
     */
    void setHeaderHeight( float _headerSize );
    
    /*******************************************************************************************/
    /**
     * @brief  Set the size of the footer (0 for none)
     *
     *  NOTE: This will take effect when setup is called next
     *
     * @param _footerSize size in GUI space
     *
     */
    void setFooterHeight( float _footerSize );
    
    /*******************************************************************************************/
    /**
     * @brief  Sets the sizes of the various header panels
     *
     * If not specified (specified means non-zero) there will be one panel, full width
     * If one is specified, it will be centered 
     * If two are specified, they will be left and right justified
     * Three will be left, center, right justified
     *
     */
    void setHeaderPanels( float first = 0, float second = 0, float third = 0 );
    
    /*******************************************************************************************/
    /**
     * @brief  Sets the sizes of the various main panels
     *
     * If not specified (specified means non-zero) there will be one panel, full width
     * If one is specified, it will be centered
     * If two are specified, they will be left and right justified
     * Three will be left, center, right justified
     *
     */
    void setMainPanels( float first = 0, float second = 0, float third = 0 );
    
    /*******************************************************************************************/
    /**
     * @brief  Sets the sizes of the various footer panels
     *
     * If not specified (specified means non-zero) there will be one panel, full width
     * If one is specified, it will be centered
     * If two are specified, they will be left and right justified
     * Three will be left, center, right justified
     *
     */
    void setFooterPanels( float first = 0, float second = 0, float third = 0 );
    
    
    /*******************************************************************************************/
    /**
     * @brief  Sets up the GUI components: header, footer, main panel and foreground/background
     *
     */
    void setup();
    
    /*******************************************************************************************/
    /**
     * @brief  Clear the GUI - you probably want resetMainLayer though
     * 
     * @param mainOrientation The orientation of the container
     *
     */
    void clear();

    /*******************************************************************************************/
    /**
     * @brief  Turns on (or off) the visual guides
     *  
     * @param setting True to turn on guides, false otherwise
     *
     */
    void setGuides( bool setting );
    
    /*******************************************************************************************/
    /**
     * @brief  Records all the errors reported by child elements
     *  
     * @param newError Newly reported error string
     *
     */
    void reportError( std::string const& newError );
    
    /*******************************************************************************************/
    /**
     * @brief  Receives a message - used internally
     * 
     * @param message The message in question  
     *
     */
    void receiveMessage( IMMessage* message );
    
    /*******************************************************************************************/
    /**
     * @brief  Gets the size of the waiting message queue
     *
     * @returns size of the queue (as an unsigned integer)
     *
     */
    unsigned int getMessageQueueSize();
    
    /*******************************************************************************************/
    /**
     * @brief  Gets the next available message in the queue (NULL if none)
     *
     * @returns A copy of the message (NULL if none)
     *
     */
    IMMessage* getNextMessage();

    /*******************************************************************************************/
    /**
     * @brief  Retrieves a reference to the main panel
     *
     */
    IMContainer* getMain(unsigned int panel = 0) {
        if( panel < main.size() ) {
            main[panel]->AddRef();
            return main[panel];
        }
        else {
            return NULL;
        }
    }
    
    /*******************************************************************************************/
    /**
     * @brief  Retrieves a reference to the footer panel
     *
     */
    IMContainer* getFooter(unsigned int panel = 0) {
        if( panel < footer.size() ) {
            footer[panel]->AddRef();
            return footer[panel];
        }
        else {
            return NULL;
        }
    }
    
    /*******************************************************************************************/
    /**
     * @brief  Retrieves a reference to the header panel
     *
     */
    IMContainer* getHeader(unsigned int panel = 0) {
        if( panel < header.size() ) {
            header[panel]->AddRef();
            return header[panel];
        }
        else {
            return NULL;
        }
    }
    
    /*******************************************************************************************/
    /**
     * @brief  Retrieves a reference to a specified background layer
     *
     * @param layerNum index of the background layer (starting at 0)
     *
     */
    IMContainer* getBackgroundLayer( unsigned int layerNum = 0 );
    
    /*******************************************************************************************/
    /**
     * @brief  Retrieves a reference to a specified foreground layer
     *
     * @param layerNum index of the foreground layer (starting at 0)
     *
     */
    IMContainer* getForegroundLayer( unsigned int layerNum = 0 );
    
    /*******************************************************************************************/
    /**
     * @brief  When this element is resized, moved, etc propagate this signal upwards
     *
     */
    void onRelayout();
    
    /*******************************************************************************************/
    /**
     * @brief  Updates the gui  
     * 
     */
    void update();
    
    /*******************************************************************************************/
    /**
     * @brief  Fired when the screen resizes
     *
     */
    void doScreenResize();

    /*******************************************************************************************/
    /**
     * @brief  Render the GUI
     *
     */
    void render();

    /*******************************************************************************************/
    /**
     * @brief  Draw a box (in *screen* coordinates) -- used internally
     * 
     */
    void drawBox( vec2 boxPos, vec2 boxSize, vec4 boxColor, int zOrder, bool shouldClip = false,
                  vec2 currentClipPos = vec2(UNDEFINEDSIZE, UNDEFINEDSIZE), 
                  vec2 currentClipSize = vec2(UNDEFINEDSIZE, UNDEFINEDSIZE) );
    
    /*******************************************************************************************/
    /**
     * @brief  Finds an element in the gui by a given name
     * 
     * @param elementName the name of the element
     *
     * @returns handle to the element (NULL if not found)  
     *
     */
    IMElement* findElement( std::string const& elementName );
    
    /*******************************************************************************************/
    /**
     * @brief  Gets a unique name for assigning to unnamed elements (used internally)
     * 
     * @returns Unique name as string
     *
     */
    std::string getUniqueName( std::string const& type = "Unkowntype");
    
    /*******************************************************************************************/
    /**
     * @brief  Remove all referenced object without releaseing references
     *
     */
    virtual void clense();
    
    /*******************************************************************************************/
    /**
     * @brief Destructor 
     *  
     */
    virtual ~IMGUI();

    void DestroyedIMElement( IMElement* element ) override;
    void DestroyedIMGUI( IMGUI* IMGUI ) override;
};


