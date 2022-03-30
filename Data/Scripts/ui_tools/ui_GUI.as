#include "ui_tools/ui_Element.as"
#include "ui_tools/ui_Message.as"
#include "ui_tools/ui_support.as"
#include "ui_tools/ui_Container.as"
#include "ui_tools/ui_IMUIContext.as"

/*******
 *  
 * ui_GUI.as
 *
 * Main class for creating adhoc GUIs as part of the UI tools  
 *
 */

namespace AHGUI {

// Make this global to avoid duplicate resource use 
IMUIContextWrapper AHGUI_IMUIContext; // UI features from the engine
/*******************************************************************************************/
/**
 * @brief One thing to rule them all
 *
 */
class GUI {

    Divider@ root;  // Root of the UI, holds all the contents 
    uint64 lastUpdateTime; // When was this last updated (ms)
    uint elementCount; // Counter for naming unnamed elements
    bool needsRelayout; // has a relayout signal been fired?
    bool showGuides; // overlay some extra lines to aid layout
    bool isVisible;
    
    ivec2 mousePosition; // Where is the mouse?
    UIMouseState leftMouseState;    // What's the state of the left mouse button?
    array<Message@> messageQueue;   // Messages waiting to process
    array<Container@> backgrounds;  // Containers for background layers, if any
    array<Container@> foregrounds;  // Containers for foreground layers, if any

    /*******************************************************************************************/
    /**
     * @brief Constructor
     *  
     * @param mainOrientation The orientation of the container
     *
     */
    GUI( DividerOrientation mainOrientation = DividerOrientation::DOVertical ) {
        
        Divider newRoot( "root", mainOrientation );
        newRoot.setSize( screenMetrics.GUISpaceX, screenMetrics.GUISpaceY );

        @newRoot.owner = @this;
        @newRoot.parent = null;
        @root = @newRoot;
        lastUpdateTime = 0;

        needsRelayout = false;
        showGuides = false;
        isVisible = true;
    }

    /*******************************************************************************************/
    /**
     * @brief  Should this GUI restrict to 16x9 aspect ratio?
     * 
     * (note: this will completely clear the contents of the GUI)
     *
     * @param restrict true if 16x9, false otherwise
     * @param mainOrientation The orientation of the main container 
     *
     */
    void restrict16x9( bool restrict, 
                       DividerOrientation mainOrientation = DividerOrientation::DOVertical ) {

        screenMetrics.restrict16x9( restrict );

        Divider newRoot( "root", mainOrientation );
        newRoot.setSize( screenMetrics.GUISpaceX, screenMetrics.GUISpaceY );

        @newRoot.owner = @this;
        @newRoot.parent = null;
        @root = @newRoot;

        needsRelayout = false;

    }

    /*******************************************************************************************/
    /**
     * @brief Destructor 
     *  
     */
    ~GUI() {
        @root = null;
    }

    /*******************************************************************************************/
    /**
     * @brief  Clear the GUI
     * 
     * @param mainOrientation The orientation of the container
     *
     */
     void clear( DividerOrientation mainOrientation = DividerOrientation::DOVertical ) {
        
        Divider newRoot( "root", mainOrientation );
        newRoot.setSize( screenMetrics.GUISpaceX, screenMetrics.GUISpaceY );

        backgrounds.resize(0);
        foregrounds.resize(0);

        @newRoot.owner = @this;
        @newRoot.parent = null;
        @root = @newRoot;

     }

    /*******************************************************************************************/
    /**
     * @brief  Turns on (or off) the visual guides
     *  
     * @param setting True to turn on guides, false otherwise
     *
     */
     void setGuides( bool setting ) {
        showGuides = setting;
     }

    void setVisible( bool v ) {
        isVisible = v;
    }

    /*******************************************************************************************/
    /**
     * @brief  When this element is resized, moved, etc propagate this signal upwards
     * 
     */
     void onRelayout() {

        // Record this
        needsRelayout = true;
     
     }

    /*******************************************************************************************/
    /**
     * @brief  Receives a message - used internally
     * 
     * @param message The message in question  
     *
     */
     void receiveMessage( Message@ message ) {
        messageQueue.insertLast( message );
     }

    /*******************************************************************************************/
    /**
     * @brief Called for each message received -- override this one in your subclass to 
     *        deal with messages from the GUI
     * 
     * @param message The message in question  
     *
     */
     void processMessage( Message@ message ) {
        
     }

    /*******************************************************************************************/
    /**
     * @brief  Set the number and initializes the background layers
     * 
     *  NOTE: This will destroy any existing layers
     *
     *  Background layers are rendered highest index value first 
     *
     * @param numLayers number of layers required
     *
     */
     void setBackgroundLayers( int numLayers ) {
        for( int i = 0; i < numLayers; ++i ) {
            Container@ newContainer = Container( ivec2( screenMetrics.GUISpaceX, screenMetrics.GUISpaceY ) );
            backgrounds.insertLast( newContainer );
        }
     }

    /*******************************************************************************************/
    /**
     * @brief  Set the number and initializes the background layers
     * 
     *  NOTE: This will destroy any existing layers
     *
     *  Background layers are rendered highest index value first 
     *
     * @param numLayers number of layers required
     *
     */
     void setForegroundLayers( int numLayers ) {
        for( int i = 0; i < numLayers; ++i ) {
            Container@ newContainer = Container( ivec2( screenMetrics.GUISpaceX, screenMetrics.GUISpaceY ) );
            foregrounds.insertLast( @newContainer );
        }
     }

    /*******************************************************************************************/
    /**
     * @brief  Retrieves a reference to a specified background layer
     * 
     * @param layerNum index of the background layer (starting at 0) 
     *
     */
     Container@ getBackgroundLayer( uint layerNum ) {
        if( layerNum >= backgrounds.length() ) {
            DisplayError("GUI Error", "Unknown background layer " + layerNum);
        }
        return backgrounds[ layerNum ];
     }

    /*******************************************************************************************/
    /**
     * @brief  Retrieves a reference to a specified foreground layer
     * 
     * @param layerNum index of the foreground layer (starting at 0) 
     *
     */
     Container@ getForegroundLayer( uint layerNum ) {
        if( layerNum >= foregrounds.length() ) {
            DisplayError("GUI Error", "Unknown foreground layer " + layerNum);
        }
        return foregrounds[ layerNum ];
     }



    /*******************************************************************************************/
    /**
     * @brief  Updates the gui  
     * 
     */
    void update() {
        if( !isVisible )
            return;

        // If we haven't updated yet, set the time
        if( lastUpdateTime == 0 ) {
            lastUpdateTime = uint64( ui_time * 1000 );
        }

        // Calculate the delta time 
        uint64 delta = uint64( ui_time * 1000 ) - lastUpdateTime;

        // Update the time
        lastUpdateTime = uint64( ui_time * 1000 );

        // Get the input from the engine
        AHGUI_IMUIContext.Get().UpdateControls();

        vec2 engineMousePos = AHGUI_IMUIContext.Get().getMousePosition();

        // Translate to GUISpace
        mousePosition.x = int( (engineMousePos.x - screenMetrics.renderOffset.x ) / screenMetrics.GUItoScreenXScale );
        mousePosition.y = int(float( GetScreenHeight() - int( engineMousePos.y + screenMetrics.renderOffset.y ) ) / screenMetrics.GUItoScreenYScale);  
        leftMouseState = AHGUI_IMUIContext.Get().getLeftMouseState();

        // Do relayout as necessary 
        while( needsRelayout ) {
            needsRelayout = false;
            root.doRelayout();

            // MJB Note: I'm aware that redoing the layout for the whole structure
            // is rather inefficient, but given that this is targeted at most a few
            // dozen elements, this should not be a problem -- can easily be optimized
            // if so
        }

        
        // Now pass this on to the children

        GUIState guistate;

        guistate.mousePosition = mousePosition;
        guistate.leftMouseState = leftMouseState;

        ivec2 origin(0,0);
        root.update( delta, origin, guistate );

        while( needsRelayout ) {
            needsRelayout = false;
            root.doRelayout();
        }

        // Finally process all of our message
        for( uint i = 0; i < messageQueue.length(); i++ ) {
            processMessage( messageQueue[i] );
        }

        messageQueue.resize(0);

        // See if this triggered any resize events
        while( needsRelayout ) {
            needsRelayout = false;
            root.doRelayout();
        }

    }

    /*******************************************************************************************/
    /**
     * @brief  Render the GUI
     *
     */
     void render() {
        if( !isVisible ) return;

        // Check to see if the screen size has changed
        if( screenMetrics.checkMetrics() ) {
                
            // All the font sizes will likely have changed
            AHGUI_IMUIContext.Get().clearTextAtlases();
            // We need to inform the elements
            root.doScreenResize();
            root.doRelayout();
            while( needsRelayout ) {
                needsRelayout = false;
                root.doRelayout();
            }
        }

        ivec2 origin(0,0);
        // render the backgrounds
        for( int layer = int(backgrounds.length())-1; layer >= 0; --layer ) {
            backgrounds[ layer ].render( origin, origin, ivec2( AH_UNDEFINEDSIZE, AH_UNDEFINEDSIZE ) );
            AHGUI_IMUIContext.Get().render();
        }

        // render the main content
        root.render( origin, origin, ivec2( AH_UNDEFINEDSIZE, AH_UNDEFINEDSIZE ) );
        AHGUI_IMUIContext.Get().render();

        // render the foregrounds
        for( uint layer = 0; layer < foregrounds.length(); ++layer ) {
            foregrounds[ layer ].render( origin, origin, ivec2( AH_UNDEFINEDSIZE, AH_UNDEFINEDSIZE ) );
            AHGUI_IMUIContext.Get().render();
        }

        if( showGuides ) {

            for( int i = 2; i <= 16; i *= 2 ) {

                // draw the vertical lines 
                int increment = GetScreenWidth() / i;
                int xpos = increment;

                while( xpos < GetScreenWidth() ) {

                    HUDImage @newimage = hud.AddImage();
                    newimage.SetImageFromPath("Data/Textures/ui/whiteblock.tga");

                    ivec2 imagepos( xpos-1, 0 );
                    
                    newimage.scale = 1;
                    newimage.scale.x *= 2;
                    newimage.scale.y *= GetScreenHeight();

                    newimage.position.x = imagepos.x;
                    newimage.position.y = GetScreenHeight() - imagepos.y - (newimage.GetWidth() * newimage.scale.y );
                    newimage.position.z = -100.0;// 0.1f;

                    newimage.color = vec4( 0.0, 0.2, 0.5 + (0.5/i), 0.25 );

                    xpos += increment;
                }

                increment = GetScreenHeight() / i;
                int ypos = increment;

                while( ypos < GetScreenHeight() ) {

                    HUDImage @newimage = hud.AddImage();
                    newimage.SetImageFromPath("Data/Textures/ui/whiteblock.tga");

                    ivec2 imagepos( 0, ypos -1 );
                    
                    newimage.scale = 1;
                    newimage.scale.x *= GetScreenWidth();
                    newimage.scale.y *= 2;

                    newimage.position.x = imagepos.x;
                    newimage.position.y = GetScreenHeight() - imagepos.y - (newimage.GetWidth() * newimage.scale.y );
                    newimage.position.z = -100.0;// 0.1f;

                    newimage.color = vec4( 0.0, 0.2, 0.5 + (0.5/i), 0.25 );

                    ypos += increment;
                }
            }
        }

        // Blackout the top and bottom if we're 'letterboxing' 
        if( screenMetrics.renderOffset.y > 0 ) {

            // Top bar
            HUDImage @topbar = hud.AddImage();
            topbar.SetImageFromPath("Data/Textures/ui/whiteblock.tga");

            ivec2 imagepos( 0, 0 );
            
            topbar.scale = 1;
            topbar.scale.x *= GetScreenWidth();
            topbar.scale.y *= screenMetrics.renderOffset.y;

            topbar.position.x = imagepos.x;
            topbar.position.y = GetScreenHeight() - imagepos.y - (topbar.GetHeight() * topbar.scale.y );
            topbar.position.z = 10.0;// 0.1f;

            topbar.color = vec4( 0.0, 0.0, 0.0, 1.0f ); 

            // Bottom bar
            HUDImage @bottombar = hud.AddImage();
            bottombar.SetImageFromPath("Data/Textures/ui/whiteblock.tga");

            imagepos = ivec2( 0, GetScreenHeight() - screenMetrics.renderOffset.y );
            
            bottombar.scale = 1;
            bottombar.scale.x *= GetScreenWidth();
            bottombar.scale.y *= screenMetrics.renderOffset.y;

            bottombar.position.x = imagepos.x;
            bottombar.position.y = GetScreenHeight() - imagepos.y - (bottombar.GetHeight() * bottombar.scale.y );
            bottombar.position.z = 10.0;// 0.1f;

            bottombar.color = vec4( 0.0, 0.0, 0.0, 1.0f );   
        }

        if( screenMetrics.renderOffset.x > 0 ) {

            // Left bar
            HUDImage @leftbar = hud.AddImage();
            leftbar.SetImageFromPath("Data/Textures/ui/whiteblock.tga");

            ivec2 imagepos( 0, 0 );
            
            leftbar.scale = 1;
            leftbar.scale.x *= screenMetrics.renderOffset.x;
            leftbar.scale.y *= GetScreenHeight();

            leftbar.position.x = GetScreenWidth() - imagepos.x - (leftbar.GetWidth() * leftbar.scale.x );
            leftbar.position.y = imagepos.y;
            leftbar.position.z = 10.0;// 0.1f;

            leftbar.color = vec4( 0.0, 0.0, 0.0, 1.0f ); 

            // Bottom bar
            HUDImage @rightbar = hud.AddImage();
            rightbar.SetImageFromPath("Data/Textures/ui/whiteblock.tga");

            imagepos = ivec2( GetScreenWidth() - screenMetrics.renderOffset.x, 0 );
            
            rightbar.scale = 1;
            rightbar.scale.x *= screenMetrics.renderOffset.x;
            rightbar.scale.y *= GetScreenHeight();

            rightbar.position.x = GetScreenWidth() - imagepos.x - (rightbar.GetWidth() * rightbar.scale.x );
            rightbar.position.y = imagepos.y;
            rightbar.position.z = 10.0;// 0.1f;

            rightbar.color = vec4( 0.0, 0.0, 0.0, 1.0f );   
        }

        hud.Draw();
        AHGUI_IMUIContext.Get().render();
     }

    /*******************************************************************************************/
    /**
     * @brief  Finds an element in the gui by a given name
     * 
     * @param elementName the name of the element
     *
     * @returns handle to the element (null if not found)  
     *
     */
    Element@ findElement( string elementName ) {
        return root.findElement( elementName );
    }

    /*******************************************************************************************/
    /**
     * @brief  Gets a unique name for assigning to unnamed elements (used internally)
     * 
     * @returns Unique name as string
     *
     */
    string getUniqueName( string type = "Unkowntype") {
        elementCount += 1;
        return type + elementCount;
    }

}
} // namespace AHGUI

