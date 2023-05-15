//-----------------------------------------------------------------------------
//           Name: credits.as
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

// Load the music 
MusicLoad ml("Data/Music/menu.xml");

// Constants because I'm not typing this in again
const float postHeader = 40;
const float postName = 20;
const float postGroup = 80;
const float scrollUpdateSize = 2;

// Just to avoid some repeated text
// Some structure to hold the credits in 
IMDivider creditDiv( "mainDiv", DOVertical );

void addToTextScroll( string text, bool isHeaer = false ) {
	if( isHeaer ) {
		// New text object
		IMText newText( text, creditFontBig );
		
		// Add it to the divider
		creditDiv.append( newText );

		// Add some space
		creditDiv.appendSpacer( postHeader );
	}
	else {
		// Second verse, same as the first
		IMText newText( text, creditFontSmall );	
		creditDiv.append( newText );	
		creditDiv.appendSpacer( postName );
	}
	
}

// The actual GUI object 
IMGUI@ imGUI;

void Initialize() {
    @imGUI = CreateIMGUI();
	// Kick off some music
	PlaySong("menu-lugaru");

	// Actually setup the GUI -- must do this before we do anything 
    imGUI.setup();

    // The center will hold in this case
    creditDiv.setAlignment( CACenter, CACenter );

    // The following is really showing my PhD in CS to it's maximum advantage
	addToTextScroll( "Thank you for playing:", true );
	

	creditDiv.append( IMImage( "Textures/ui/menus/credits/overgrowth-white.png" ) );
	creditDiv.appendSpacer( postGroup );
	creditDiv.appendSpacer( postGroup );

	addToTextScroll( "Project lead", true );
	addToTextScroll( "David Rosen" );

	creditDiv.appendSpacer( postGroup );
	
	addToTextScroll( "Art", true);
	addToTextScroll( "Aubrey Serr");
	addToTextScroll( "Mark Stockton");
	
	creditDiv.appendSpacer( postGroup );

	addToTextScroll( "Game design", true);
	addToTextScroll( "David Rosen");
	addToTextScroll( "Jillian Ogle");
	
	creditDiv.appendSpacer( postGroup );

	addToTextScroll( "Level design", true);
	addToTextScroll( "Aubrey Serr");
	addToTextScroll( "Josh Goheen");
	addToTextScroll( "Mark Stockton");

	creditDiv.appendSpacer( postGroup );
	
	addToTextScroll( "Music", true);
	addToTextScroll( "Anton Riehl");
	addToTextScroll( "Mikko Tarmia");

	creditDiv.appendSpacer( postGroup );
	
	addToTextScroll( "Producers", true);
	addToTextScroll( "Jillian Ogle");
	addToTextScroll( "Lukas Orsvärn");

	creditDiv.appendSpacer( postGroup );
	
	addToTextScroll( "Programming", true);
	addToTextScroll( "Brian Cronin");
	addToTextScroll( "David Rosen");
	addToTextScroll( "David Lyhed Danielsson");
	addToTextScroll( "Gyrth McMulin");
	addToTextScroll( "Jeffrey Rosen");
	addToTextScroll( "John Graham");
	addToTextScroll( "Max Danielsson");
	addToTextScroll( "Micah J Best");
	addToTextScroll( "Phillip Isola");
	addToTextScroll( "Tuomas Närväinen");
	addToTextScroll( "Turo Lamminen");
	
	creditDiv.appendSpacer( postGroup );

	addToTextScroll( "Public relations", true);
	addToTextScroll( "John Graham");
	
	creditDiv.appendSpacer( postGroup );

	addToTextScroll( "Sound effects", true);
	addToTextScroll( "Tapio Liukkonen");
	
	creditDiv.appendSpacer( postGroup );

	addToTextScroll( "User interface", true);
	addToTextScroll( "Aubrey Serr");
	addToTextScroll( "Jeffrey Rosen");
	addToTextScroll( "Mark Stockton");
	addToTextScroll( "Micah J Best");

	creditDiv.appendSpacer( postGroup );
	
	addToTextScroll( "Special thanks to", true);
	addToTextScroll( "Kylie Allen");
	addToTextScroll( "Ryan Mapa");

	creditDiv.appendSpacer( postGroup );

	creditDiv.append( IMImage( "Textures/ui/menus/credits/logo.tga" ) );


	// Now make this a 'floating component' of the main container
	
	// Actually add it to the container
	// start at the bottom of the screen
	imGUI.getMain().addFloatingElement(	creditDiv, 	
									    "credits", 
									    vec2(
											UNDEFINEDSIZE, // Will center 
											screenMetrics.GUISpace.y
										)
									  );

	// Send a message if somebody clicks anywhere
	imGUI.getMain().addLeftMouseClickBehavior( IMFixedMessageOnClick("click"), "click" );

}

void Update(){

	bool done = false;

	// Check to see if we get an early exit 
	if(GetInputPressed(0,'esc')){
    	done = true;
    }

    // process waiting messages
    while( imGUI.getMessageQueueSize() > 0 ) {
        IMMessage@ message = imGUI.getNextMessage();
        if( message.name == "click" )
        {
        	done = true;
        }
    }

    // Move the credits up
    // Get the position 
    vec2 creditPos = imGUI.getMain().getElementPosition("credits");

    //Check to see if we're off the screen
    if( creditPos.y + creditDiv.getSizeY() < 0.0 ) {
    	done = true;
    }

    creditPos.y -= scrollUpdateSize;

    imGUI.getMain().moveElement( "credits", creditPos );

    if( done ) {
    	this_ui.SendCallback("back");
    }

    imGUI.update();
}

void Resize() {
    imGUI.doScreenResize();
}

void DrawGUI(){
    imGUI.render();
}

void Draw(){
}

void Init(string str){
}

void StartArenaMeta(){

}
bool CanGoBack(){
	return false;
}
void Dispose(){

}
