//-----------------------------------------------------------------------------
//           Name: theriummainmenu.as
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

#include "ui_effects_t2.as"
#include "ui_tools.as"
#include "music_load.as"

AHGUI::FontSetup labelFont("Underdog-Regular", 70,HexColor("#fff"));
AHGUI::FontSetup versionFont("Underdog-Regular", 65, HexColor("#fff"));

int title_spacing = 100;
int menu_item_spacing = 20;

MusicLoad ml("Data/Music/theriummenu.xml");

AHGUI::MouseOverPulseColor buttonHover(
                                        HexColor("#ff4516"),
                                        HexColor("#ff8216"), 2 );

bool draw_settings = false;

class MainMenuGUI : AHGUI::GUI {
    RibbonBackground ribbon_background;

    MainMenuGUI()
    {
        //restrict16x9(false);

        super();

        ribbon_background.Init();

        Init();
    }

    void Init()
    {
        AHGUI::Divider@ mainpane = root.addDivider( DDTop,
                                                    DOVertical,
                                                    ivec2( UNDEFINEDSIZEI, 1140 ) );

        /*
        AHGUI::Image alphasticker = AHGUI::Image("Textures/ui/main_menu/alphasticker.png");
        alphasticker.scaleToSizeX( 350 );
        mainpane.addFloatingElement( alphasticker, "alphasticker", ivec2( 2100, 100 ));
        */

        /* 
        // TODO: Why is this making it crash on MAC?? -David
        AHGUI::Text alphaversion = AHGUI::Text( GetBuildVersionShort().split("-")[0], versionFont );
        mainpane.addFloatingElement( alphaversion, "alphaversion", ivec2( 1800, 300 ));
        */

        AHGUI::Image titleImage = AHGUI::Image("textures/ui/t2/logo.png");
        mainpane.addElement(titleImage,DDTop);

        mainpane.addSpacer(title_spacing,DDTop);
/*
        {
            AHGUI::Text buttonText = AHGUI::Text("BEGIN", labelFont);
            buttonText.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("begin") );
            buttonText.addMouseOverBehavior( buttonHover );
            mainpane.addElement(buttonText, DDTop);

            mainpane.addSpacer( menu_item_spacing, DDTop ) ;
        }
*/
        {
            AHGUI::Text buttonText = AHGUI::Text("BEGIN", labelFont);
            buttonText.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("prologue") );
            buttonText.addMouseOverBehavior( buttonHover );
            mainpane.addElement(buttonText, DDTop);

            mainpane.addSpacer( menu_item_spacing, DDTop ) ;
        }

        {
            AHGUI::Text buttonText = AHGUI::Text("S2 HUB", labelFont);
            buttonText.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("hub") );
            buttonText.addMouseOverBehavior( buttonHover );
            mainpane.addElement(buttonText, DDTop);

            mainpane.addSpacer( menu_item_spacing, DDTop ) ;
        }
		
        {
            AHGUI::Text buttonText = AHGUI::Text("Levels", labelFont);
            buttonText.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("play") );
            buttonText.addMouseOverBehavior( buttonHover );
            mainpane.addElement(buttonText, DDTop);

            mainpane.addSpacer( menu_item_spacing, DDTop ) ;
        }
/*
        {
            AHGUI::Text buttonText = AHGUI::Text("VERSUS", labelFont);
            buttonText.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("versus") );
            buttonText.addMouseOverBehavior( buttonHover );
            mainpane.addElement(buttonText, DDTop);

            mainpane.addSpacer( menu_item_spacing, DDTop ) ;
        }
*/
/*
        {
            AHGUI::Text buttonText = AHGUI::Text("Editor", labelFont);
            buttonText.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("old_alpha_menu") );
            buttonText.addMouseOverBehavior( buttonHover );
            mainpane.addElement(buttonText, DDTop);

            mainpane.addSpacer( menu_item_spacing, DDTop ) ;
        }
*/

        {
            AHGUI::Text buttonText = AHGUI::Text("Credits", labelFont);
            buttonText.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("credits") );
            buttonText.addMouseOverBehavior( buttonHover );
            mainpane.addElement(buttonText, DDTop);

            mainpane.addSpacer( menu_item_spacing, DDTop ) ;
        }

        {
            AHGUI::Text buttonText = AHGUI::Text("Back", labelFont);
            buttonText.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("exit") );
            buttonText.addMouseOverBehavior( buttonHover );
            mainpane.addElement(buttonText, DDTop);

            mainpane.addSpacer( menu_item_spacing, DDTop ) ;
        }
    }

    void processMessage( AHGUI::Message@ message )
    {
        Log( info, "Got processMessage " + message.name );
        if( message.name == "arena" )
        {
        }
        else if( message.name == "begin" )
        {
            this_ui.SendCallback("t2/begin.xml");
        }
        else if( message.name == "prologue" )
        {
            this_ui.SendCallback("t2/tutorial.xml");
        }
        else if( message.name == "hub" )
        {
            this_ui.SendCallback("t2/hub.xml");
        }
		else if( message.name == "credits" )
        {
			this_ui.SendCallback( "theriumcredits.as" );
        }
		else if( message.name == "play" )
        {
			this_ui.SendCallback( "theriumplay_menu.as" );
        }
        else if( message.name == "mods" )
        {
            this_ui.SendCallback( "mods" );
        }
        else if( message.name == "exit" )
        {
            this_ui.SendCallback( "exit" );
        }
        else if( message.name == "settings" )
        {
            this_ui.SendCallback( "main_menu_settings.as" );
        }
    }

    void update()
    {
        //Other things here, before

        AHGUI::GUI::update();
    }
	
string GetRandomBackground(){
	array<string> background_paths;
	int counter = 0;
	while(true){
		string path = "Textures/ui/menus/main/background_" + counter + ".jpg";
		if(FileExists("Data/" + path)){
	    	background_paths.insertLast(path);
	    	counter++;
		}else{
	    	break;
		}
	}
	if(background_paths.size() < 1){
		return "Textures/error.tga";
	}else{
		return background_paths[rand()%background_paths.size()];
	}
}


    void render() {
        EnterTelemetryZone("MainMenuGUI::render()");

        EnterTelemetryZone("ribbon_background.Update()");
        ribbon_background.Update();
        LeaveTelemetryZone();

        EnterTelemetryZone("ribbon_background.DrawGUI");
        ribbon_background.DrawGUI(1.1f);
        LeaveTelemetryZone();

        EnterTelemetryZone("hud.Draw()");
        hud.Draw();
        LeaveTelemetryZone();

        EnterTelemetryZone("AHGUI::GUI::render()");
        AHGUI::GUI::render();
        LeaveTelemetryZone();

        LeaveTelemetryZone();

        if(draw_settings){
            ImGui_Begin("Settings", draw_settings);
            ImGui_DrawSettings();
            ImGui_End();
        }
    }

}


MainMenuGUI@ mainmenuGUI = @MainMenuGUI();
// Comment out the above and uncomment to enable the new feature demo
//NewFeaturesExampleGUI exampleGUI;

bool HasFocus() {
    return false;
}

void Initialize() {
    PlaySong("overgrowth_main");
}

void Dispose() {
}

bool CanGoBack() {
    return false;
}

void Update() {

    mainmenuGUI.update();
}

void DrawGUI() {
    EnterTelemetryZone("DrawGUI");
    mainmenuGUI.render();
    LeaveTelemetryZone();
}

void Draw() {
}

void Init(string str) {
}

void StartMainMenu() {

}
