//-----------------------------------------------------------------------------
//           Name: theriumcredits.as
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
#include "music_load.as"

// Load the music 
MusicLoad ml("Data/Music/menu.xml");

// Constants because I'm not typing this in again
const float postHeader = 40;
const float postName = 20;
const float postGroup = 80;
const float scrollUpdateSize = 1;

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
	PlaySong("credits");

	// Actually setup the GUI -- must do this before we do anything 
    imGUI.setup();

    // The center will hold in this case
    creditDiv.setAlignment( CACenter, CACenter );

	addToTextScroll( "Thank you for playing", true );
	

	creditDiv.append( IMImage( "textures/ui/t2/logo.png" ) );
	creditDiv.appendSpacer( postGroup );
	creditDiv.appendSpacer( postGroup );

	addToTextScroll( "Creator", true );
	addToTextScroll( "Constance P." );

	creditDiv.appendSpacer( postGroup );
	
	addToTextScroll( "Scripts", true);
	addToTextScroll( "Constance P.");
	addToTextScroll( "Halzoid98");
	
	creditDiv.appendSpacer( postGroup );

	addToTextScroll( "Modellers", true);
	addToTextScroll( "Rolands");
	addToTextScroll( "Xweert/Jackerino");
	
	creditDiv.appendSpacer( postGroup );

	addToTextScroll( "Writers", true);
	addToTextScroll( "Constance P.");
	addToTextScroll( "Xweert/Jackerino");
	addToTextScroll( "Sjoormangundr");

	creditDiv.appendSpacer( postGroup );
	
	addToTextScroll( "Beta Testers", true);
	addToTextScroll( "Halzoid98");
	addToTextScroll( "Halt and Catch Fire");
	addToTextScroll( "Charleston");
	addToTextScroll( "Tryhard60");
	addToTextScroll( "Merlyn Morgan Graham");

	creditDiv.appendSpacer( postGroup );

	creditDiv.appendSpacer( postGroup );
	
	addToTextScroll( "Contributors", true);

	creditDiv.appendSpacer( postGroup );
	
	addToTextScroll( "Scripts", true);
	addToTextScroll( "Keril Artemov");
	addToTextScroll( "Thomason");
	addToTextScroll( "Gyrth McMulin");
	addToTextScroll( "Merlyn Morgan Graham");
	
	creditDiv.appendSpacer( postGroup );

	addToTextScroll( "Assets", true);
	addToTextScroll( "Gyrth's Bow and Arrow");
	addToTextScroll( "Twowolves' Assets and Lanterns");
	
	creditDiv.appendSpacer( postGroup );

	addToTextScroll( "Textures", true);
	addToTextScroll( "Xweert/Jackerino");
	addToTextScroll( "CGTextures");
	
	creditDiv.appendSpacer( postGroup );

	addToTextScroll( "Music", true);
	addToTextScroll( "Grindgrain");
	addToTextScroll( "Anton Rhiel");

	creditDiv.appendSpacer( postGroup );
	
	addToTextScroll( "Assistant Writer", true);
	addToTextScroll( "Vanessa");

	creditDiv.appendSpacer( postGroup );
	
	creditDiv.appendSpacer( postGroup );
	
	creditDiv.appendSpacer( postGroup );

	creditDiv.appendSpacer( postGroup );
	
	creditDiv.appendSpacer( postGroup );
	
	creditDiv.appendSpacer( postGroup );
	
	creditDiv.appendSpacer( postGroup );
	
	creditDiv.appendSpacer( postGroup );

	creditDiv.append( IMImage( "textures/ui/t2/credits.png" ) );


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
	return true;
}
void Dispose(){

}
