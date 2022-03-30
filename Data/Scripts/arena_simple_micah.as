//-----------------------------------------------------------------------------
//           Name: arena_simple_micah.as
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

float title_spacing = 150;
float menu_item_spacing = 40;

MusicLoad ml("Data/Music/menu.xml");

IMGUI@ imGUI;
RibbonEffect ribbonEffect( imGUI ); 

bool HasFocus() {
    return false;
}

void Initialize() {
    @imGUI = CreateIMGUI();
    /**
     * Setup the GUI 
     **/

    // Start playing some music
    PlaySong("menu-lugaru"); 

    // We're going to want a 200 'gui space' pixel header/footer
    imGUI.setHeaderHeight(200);
    imGUI.setFooterHeight(200);
    imGUI.setFooterPanels(400,400);

    // Actually setup the GUI -- must do this before we do anything 
    imGUI.setup();

    /**
     * Background/Foreground 
     **/

    // setup our background/forground
    ribbonEffect.reset();

    /**
     * Main body 
     **/

    // Build some structure to put this in 
    IMDivider mainDiv( "selectionDiv", DOVertical );

    // Add the title
    mainDiv.append(IMText("Select Arena", titleFont));

    // Add some space
    mainDiv.appendSpacer( title_spacing );

    // build our actual selection menu
    IMTextSelectionList menu("arenamenu", 
                             selectionListFont, 
                             menu_item_spacing, 
                             selectionListButtonHover);

    menu.setAlignment( CACenter, CACenter );

    // Add our various entries
    float moveInX = 1500;
    int moveInTime = 1500;
    int moveInDir = 1;

    menu.setItemUpdateBehaviour(IMMoveIn( moveInTime, vec2(moveInX * moveInDir, 0), inQuartTween ));
    menu.addEntry( "WaterfallCave", "Waterfall Cave", "waterfall_arena.xml" );
    moveInDir *= -1;
    menu.setItemUpdateBehaviour(IMMoveIn( moveInTime, vec2(moveInX * moveInDir, 0), inQuartTween ));
    menu.addEntry( "MagmaArena", "Magma Arena", "Magma_Arena.xml" );
    moveInDir *= -1;
    menu.setItemUpdateBehaviour(IMMoveIn( moveInTime, vec2(moveInX * moveInDir, 0), inQuartTween ));
    menu.addEntry( "RockCave", "Rock Cave", "Cave_Arena.xml" );
    moveInDir *= -1;
    menu.setItemUpdateBehaviour(IMMoveIn( moveInTime, vec2(moveInX * moveInDir, 0), inQuartTween ));
    menu.addEntry( "StuccoCourtyard", "Stucco Courtyard", "stucco_courtyard_arena.xml" );
    
    // add to the divider
    mainDiv.append( menu );

    // Align the contained element
    imGUI.getMain().setAlignment( CACenter, CACenter );

    // Add it to the main panel of the GUI
    imGUI.getMain().setElement( @mainDiv );

    /**
     * Header
     **/

    /**
     * Footer
     **/ 

    
    IMDivider backDiv( "backDiv", DOHorizontal );
    backDiv.sendMouseOverToChildren();
    backDiv.addLeftMouseClickBehavior( IMFixedMessageOnClick("back"), "" );

    IMImage backImage("Textures/ui/arena_mode/left_arrow.png");
    backImage.scaleToSizeX(75);
    backImage.addUpdateBehavior( IMFadeIn( 1000, inSineTween ), "" );
    backImage.addMouseOverBehavior( selectionListButtonHover, "" );
    backImage.setName("backimage");

    backDiv.append( backImage );

    backDiv.appendSpacer( 30 );

    IMText backText( "Back", backFont );
    backText.addUpdateBehavior( IMFadeIn( 1000, inSineTween ), "" );
    backText.addMouseOverBehavior( selectionListButtonHover, "" );
    backText.setName("backtext");

    backDiv.append( backText );

    IMDivider LLdiv( "LLdiv", DOHorizontal );
    LLdiv.appendSpacer( 75 );
    LLdiv.append( backDiv );

    imGUI.getFooter(0).setAlignment( CALeft, CACenter );
    imGUI.getFooter(0).setElement( LLdiv );

}

void Dispose() {
}

bool CanGoBack() {
    return true;
}

void Update() {

    // Update the background/foreground
    ribbonEffect.update();

    // Do the general GUI updating 
    imGUI.update();

    // process any messages produced from the update
    while( imGUI.getMessageQueueSize() > 0 ) {
        IMMessage@ message = imGUI.getNextMessage();

        Log( info, "Got processMessage " + message.name );
                if( message.name == "back" )
        {
            global_data.WritePersistentInfo( false );
            global_data.clearSessionProfile();
            global_data.clearArenaSession();

            this_ui.SendCallback( "back" );
        } 
        else {
            // Hack for this simplified arena menu
            global_data.clearArenaSession();
            this_ui.SendCallback( "arenas/" + message.name );
        }
    }

}


void Resize() {

    // Pass the rest of the work onto the GUI
    imGUI.doScreenResize(); // This must be called first

    // restart our ribbon effect
    ribbonEffect.reset();

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

