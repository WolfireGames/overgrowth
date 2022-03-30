//-----------------------------------------------------------------------------
//           Name: mainmenu_micah_new.as
//      Developer: Wolfire Games LLC
//    Script Type:
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
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

#include "menu_common.as"
#include "arena_meta_persistence.as"
#include "music_load.as"

MusicLoad ml("Data/Music/menu.xml");

// keep track of the math for our backgrounds
class BackgroundObject {
    int z;          // z ordering
    vec2 startPos; // where does it start rendering?
    float sizeX;     // how big is this in the x direction?
    vec2 shiftSize; // how much to shift it by for parallax 
    string filename;// what file to load for this
    string GUIname; // name used to find this in the GUI
    float alpha;    // alpha value
    bool fadeIn;    // should we fade in

    BackgroundObject( string _fileName, string _GUIname, int _z, 
                      vec2 _startPos, float _sizeX, vec2 _shiftSize, float _alpha,
                      bool _fadeIn ) {
        z = _z;         
        startPos = _startPos;
        sizeX = _sizeX;    
        shiftSize = _shiftSize;
        filename = _fileName;
        GUIname = _GUIname;
        alpha = _alpha;
        fadeIn = _fadeIn;
    }

    void addToGUI( IMGUI@ theGUI ) {
        
        // Set it to our background image
        IMImage backgroundImage( filename );
        
        // Make sure that no matter the aspect ratio, it covers the whole screen top to bottom
        backgroundImage.scaleToSizeX(sizeX);

        backgroundImage.setAlpha(alpha);

        if( fadeIn ) {
            backgroundImage.addUpdateBehavior( IMFadeIn( 2000, inSineTween ), filename + "-fadeIn" );    
        }

        // Now set this as the element in the background container, this will center it
        theGUI.getBackgroundLayer().addFloatingElement( backgroundImage, 
                                                           GUIname, 
                                                           startPos, 
                                                           z );

    }

    void adjustPositionByMouse( IMGUI@ theGUI ) {

        vec2 mouseRatio = vec2( theGUI.guistate.mousePosition.x/screenMetrics.GUISpace.x,
                                theGUI.guistate.mousePosition.y/screenMetrics.GUISpace.y );
        
        vec2 shiftPosition = vec2( shiftSize.x * mouseRatio.x,
                                   0 );//int( float(shiftSize.y) * mouseRatio.y ) );
        
        theGUI.getBackgroundLayer().moveElement( GUIname, startPos + shiftPosition );
    
    }

}

IMGUI@ imGUI;
array<BackgroundObject@> bgobjects;

bool HasFocus() {
    return false;
}

// Draw the picture for the background
void setBackGround() {
    // Clear the current background 
    imGUI.getBackgroundLayer( 0 ).clear();
        
    for( uint i = 0; i < bgobjects.length(); ++i ) {
        bgobjects[i].addToGUI( imGUI );
    }

}

void Initialize() {
    @imGUI = CreateIMGUI();
    // Start playing some music
    PlaySong("menu-lugaru"); 

    // We're going to want a 100 'gui space' pixel header/footer
    imGUI.setHeaderHeight(100);
    imGUI.setFooterHeight(100);

    // Actually setup the GUI -- must do this before we do anything 
    imGUI.setup();

    bgobjects.resize(0);

    bgobjects.insertLast(BackgroundObject( "Textures/ui/menus/main/twistedpaths-sky.tga", 
                                           "BG-sky", 
                                           1, 
                                           vec2(0,0), 
                                           screenMetrics.GUISpace.x, 
                                           vec2(0,0), 
                                           1.0, 
                                           false ) );

    bgobjects.insertLast( BackgroundObject( "Textures/ui/menus/main/twistedpaths-bushybg.tga", 
                                            "BG-bushybg", 
                                            2, 
                                            vec2(-10,150), 
                                            screenMetrics.GUISpace.x + 10, 
                                            vec2(10,10), 
                                            0.7, 
                                            false ) );

    bgobjects.insertLast( BackgroundObject( "Textures/ui/menus/main/twistedpaths-frontbush.tga", 
                                            "BG-frontbush1", 
                                            3, 
                                            vec2(1000,150), 
                                            2000 + 20, 
                                            vec2(20,20), 
                                            1.0, 
                                            false ) );
    
    bgobjects.insertLast( BackgroundObject( "Textures/ui/menus/main/twistedpaths-frontbush.tga", 
                                            "BG-frontbush2", 
                                            4, 
                                            vec2(-200,150), 
                                            1800 + 20, 
                                            vec2(26,26), 
                                            1.0, 
                                            false ) );

    bgobjects.insertLast( BackgroundObject( "Textures/ui/menus/main/twistedpaths-siderock-left.tga", 
                                            "BG-siderock-left", 
                                            5, 
                                            vec2(-50,400), 
                                            500, 
                                            vec2(30,30), 
                                            1.0, 
                                            false ) );

    bgobjects.insertLast( BackgroundObject( "Textures/ui/menus/main/twistedpaths-talltree.tga", 
                                            "BG-talltree", 
                                            6, 
                                            vec2(-200,0), 
                                            800 + 70, 
                                            vec2(40,40), 
                                            1.0, 
                                            false ) );

    bgobjects.insertLast( BackgroundObject( "Textures/ui/menus/main/twistedpaths-treebg.tga", 
                                            "BG-treebg", 
                                            7, 
                                            vec2(-80,0), 
                                            screenMetrics.GUISpace.x + 80, 
                                            vec2(50,50), 
                                            1.0, 
                                            false ) );

    bgobjects.insertLast( BackgroundObject( "Textures/ui/menus/main/overgrowth-small.png", 
                                            "BG-og-small", 
                                            9, 
                                            vec2(1200,0), 
                                            1200, 
                                            vec2(45,45), 
                                            1.0,
                                            true ) );

    bgobjects.insertLast( BackgroundObject( "Textures/ui/menus/main/twistedpaths-main.tga", 
                                            "BG-main", 
                                            10, 
                                            vec2(-80,0), 
                                            screenMetrics.GUISpace.x + 80, 
                                            vec2(80,80), 
                                            1.0, 
                                            false ) );

    // setup our background
    setBackGround();

    // build our text menu
    IMTextSelectionList menu("mainmenu", selectionListFont, 10, selectionListButtonHover);

    menu.setAlignment( CALeft, CACenter );

    // Add our various entries
    int fadeInTime = 2000;
    int fadeInDiff = 250;

    menu.setItemUpdateBehaviour(IMFadeIn( fadeInTime, inSineTween ));
    menu.addEntry( "arena", "Arena", "arena" );
    fadeInTime += fadeInDiff;
    menu.setItemUpdateBehaviour(IMFadeIn( fadeInTime, inSineTween ));
    menu.addEntry( "versus", "Versus", "versus" );
    fadeInTime += fadeInDiff;
    menu.setItemUpdateBehaviour(IMFadeIn( fadeInTime, inSineTween ));
    menu.addEntry( "editor", "Editor", "old_alpha_menu" );
    fadeInTime += fadeInDiff;
    menu.setItemUpdateBehaviour(IMFadeIn( fadeInTime, inSineTween ));
    menu.addEntry( "lugaru", "Lugaru", "lugaru" );
    fadeInTime += fadeInDiff;
    menu.setItemUpdateBehaviour(IMFadeIn( fadeInTime, inSineTween ));
    menu.addEntry( "tutorial", "Tutorial", "tutorial" );
    fadeInTime += fadeInDiff;
    menu.setItemUpdateBehaviour(IMFadeIn( fadeInTime, inSineTween ));
    menu.addEntry( "settings", "Settings", "settings" );
    fadeInTime += fadeInDiff;
    menu.setItemUpdateBehaviour(IMFadeIn( fadeInTime, inSineTween ));
    menu.addEntry( "mods", "Mods", "mods" );
    fadeInTime += fadeInDiff;
    menu.setItemUpdateBehaviour(IMFadeIn( fadeInTime, inSineTween ));
    menu.addEntry( "credits", "Credits", "credits" );
    fadeInTime += fadeInDiff;
    menu.setItemUpdateBehaviour(IMFadeIn( fadeInTime, inSineTween ));
    menu.addEntry( "exit", "Exit", "exit" );

    // Build some structure to put this in 
    IMDivider mainDiv( "mainDiv", DOHorizontal );
    // add some space
    mainDiv.appendSpacer( 475 );

    // add the menu

    Log(info, "About to append");
    mainDiv.append( menu );
    Log(info, "Done append");

    // Align the contained element to the left
    imGUI.getMain().setAlignment( CALeft, CACenter );

    // Add it to the main panel of the GUI
    imGUI.getMain().setElement( @mainDiv );

    // Put the alpha version in the footer 
    IMText alphaversion( GetBuildVersionShort().split("-")[0], noteFont );
    // Add some extra space 
    alphaversion.setPaddingR( 100 ); 
    // push this over to the right
    imGUI.getFooter().setAlignment( CARight, CACenter );
    // add it to the container
    imGUI.getFooter().setElement( alphaversion );

}

void Dispose() {
    CloseSettings();
}

bool CanGoBack() {
    return not CloseSettings();
}

void Update() {

    // Update the background
    // update our background objects
    for( uint i = 0; i <  bgobjects.length(); ++i ) {
        bgobjects[i].adjustPositionByMouse( imGUI );
    }

    // Do the general GUI updating 
    imGUI.update();

    // process any messages produced from the update
    while( imGUI.getMessageQueueSize() > 0 ) {
        IMMessage@ message = imGUI.getNextMessage();

        Log( info, "Got processMessage " + message.name );
        if( message.name == "arena" )
        {
            global_data.clearSessionProfile();
            global_data.clearArenaSession();
            this_ui.SendCallback( "arena_simple.as" );
        }
        else if( message.name == "versus" )
        {
            this_ui.SendCallback("Project60/22_grass_beach.xml");
        }
        else if( message.name == "lugaru" )
        {
            this_ui.SendCallback( "lugaru_menu_simple.as" );
        }
        else if( message.name == "tutorial" )
        {
            this_ui.SendCallback("tutorial.xml");
        }
        else if( message.name == "credits" )
        {
            this_ui.SendCallback( "credits.as" );
        }
        else if( message.name == "mods" )
        {
            this_ui.SendCallback( "mods" );
        }
        else if( message.name == "old_alpha_menu" )
        {
            this_ui.SendCallback( "old_alpha_menu" );
        }
        else if( message.name == "exit" )
        {
            this_ui.SendCallback( "exit" );
        }
        else if( message.name == "settings" )
        {
            OpenSettings(context);
        }
    }

}


void Resize() {
    imGUI.doScreenResize(); // This must be called first
    setBackGround();
}

void ScriptReloaded() {
    // Clear the old GUI 
    imGUI.clear();
    // Rebuild it
    Initialize();

}

void DrawGUI() {
    imGUI.render();
}

void Draw() {
}

void Init(string str) {
}

