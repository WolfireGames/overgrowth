//-----------------------------------------------------------------------------
//           Name: arena_meta.as
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

#include "ui_effects.as"
#include "arena_meta_persistence.as"
#include "ui_tools.as"
#include "utility/ticker.as"
#include "music_load.as"

MusicLoad ml("Data/Music/arena.xml");

// enum for the statemachine (Someday I'll write something to make this easier )
enum ArenaGUIState {
    agsInvalidState,
    agsLoadingLevel,
    agsMainMenu,
    agsSelectProfile,
    agsNewProfile,
    agsConfirmDelete,
    agsCharacterIntro,
    agsMetaChoice,
    agsMessage,
    agsMapScreen,
    agsEndScreen,
};

int top_margin = 100;
int left_margin = 300;

int screen_height = 1400;
int left_status_pane_height = 1100;
int status_pane_column_width = 320;
int status_pane_column_padding = 25;
int status_pane_glyph_height = 250;

int node_marker_width = 50;
int node_marker_height = 50;

int title_font_size = 100;
int label_font_size = 75;
int menu_button_font_size = 100;
int infotext_font_size = 75;
int body_font_size = 75;
int query_font_size = 60;
int world_map_font_size = 50;

string title_font = "edosz";
string label_font = "edosz";
string button_font = "edosz";
string body_font = "edosz";
string infotext_font = "edosz";
string query_font = "edosz";
string world_map_font = "edosz";

vec4 title_color = HexColor("#9e0000");
vec4 label_color = HexColor("#fff");
vec4 button_color = HexColor("#fff");
vec4 infotext_color = HexColor("#fff");
//vec4 body_color = HexColor("#000");
vec4 body_color = HexColor("#fff");
vec4 query_color =  HexColor("#000");
vec4 world_map_color = HexColor("#fff");

vec4 activatedButton = HexColor("#ffffff");
vec4 deactivatedButton = HexColor("#888888");

// Some data for the GUI

// Based on the values from GetRandomFurColor
array<vec3> furColorChoices = { vec3(1.0,1.0,1.0), 
                                vec3(34.0/255.0,34.0/255.0,34.0/255.0), 
                                vec3(137.0/255.0, 137.0/255.0, 137.0/255.0), 
                                vec3(105.0/255.0,73.0/255.0,54.0/255.0), 
                                vec3(53.0/255.0,28.0/255.0,10.0/255.0), 
                                vec3(172.0/255.0,124.0/255.0,62.0/255.0) };

array<string> arenaNames = { "Cave", "Waterfall" };
array<string> arenaImages = {"Textures/arenamenu/cave_arena.tga", 
                             "Textures/arenamenu/waterfall_arena.tga"};

AHGUI::MouseOverPulseColor buttonHover( 
                                        HexColor("#ffde00"), 
                                        HexColor("#ffe956"), .25 );



float limitDecimalPoints( float n, int points ) {
    return float( float(int( n * pow( 10, points ) )) / pow( 10, points ) );

}                            

class MouseOverImage : AHGUI::MouseOverBehavior 
{
    string normal;
    string hover;

    MouseOverImage( string _normal, string _hover )
    {
        normal = _normal;
        hover = _hover;
    }

    void onStart( AHGUI::Element@ element, uint64 delta, ivec2 drawOffset, AHGUI::GUIState& guistate ) {
        AHGUI::Image@ img = cast<AHGUI::Image>( element );
        int width = img.getSizeX(); 
        int height = img.getSizeY(); 
        img.setImageFile(hover);
        img.setSize( width,height );
    }

    void onContinue( AHGUI::Element@ element, uint64 delta, ivec2 drawOffset, AHGUI::GUIState& guistate ) {

    }

    bool onFinish( AHGUI::Element@ element, uint64 delta, ivec2 drawOffset, AHGUI::GUIState& guistate ) {
        AHGUI::Image@ img = cast<AHGUI::Image>( element );
        int width = img.getSizeX(); 
        int height = img.getSizeY(); 
        img.setImageFile(normal);
        img.setSize( width,height );
        return true;
    }
}

class MouseClickWorldNode : AHGUI::MouseClickBehavior {
    ArenaGUI@ gui;
    string node_id;

    MouseClickWorldNode( ArenaGUI@ _gui, string _node_id )
    {
        @gui = @_gui;
        node_id = _node_id;
    }

    bool onDown( AHGUI::Element@ element, uint64 delta, ivec2 drawOffset, AHGUI::GUIState& guistate ) {
        global_data.world_map_node_id = node_id;
        gui.RefreshWorldMap();
        return true;
    }

    bool onStillDown( AHGUI::Element@ element, uint64 delta, ivec2 drawOffset, AHGUI::GUIState& guistate ) {
        return true;
    }

    bool onUp( AHGUI::Element@ element, uint64 delta, ivec2 drawOffset, AHGUI::GUIState& guistate ) {
        return true;
    }

    void cleanUp( AHGUI::Element@ element ) {
        
    }
}

class MouseHoverSetTextValue : AHGUI::MouseOverBehavior {
    AHGUI::Text@ text;
    string new_value;

    MouseHoverSetTextValue( AHGUI::Text@ _text, string _new_value ) {
        @text = _text;
        new_value = _new_value;
    }

    void onStart( AHGUI::Element@ element, uint64 delta, ivec2 drawOffset, AHGUI::GUIState& guistate ) {
        string tt = new_value;
        text.setVisible(true);
        text.setText(tt); 
    }

    void onContinue( AHGUI::Element@ element, uint64 delta, ivec2 drawOffset, AHGUI::GUIState& guistate ) {

    }

    bool onFinish( AHGUI::Element@ element, uint64 delta, ivec2 drawOffset, AHGUI::GUIState& guistate ) {
        text.setVisible(false);
        return true;
    }
}

class ArenaGUI : AHGUI::GUI {
    bool forceReloadState = false;
    //New profile
    WrappingTicker characterSelection(1);

    // fancy ribbon background stuff 
    float visible = 0.0f;
    float target_visible = 1.0f;
    RibbonBackground ribbon_background; 
    //TODO: fold this into AHGUI 
    
    ArenaGUIState currentState = agsInvalidState; // Token for our state machine
    ArenaGUIState lastState = agsInvalidState;    // Last seen state, to detect changes

    JSONValue newCharacter; // For storing a character as its being created
    JSONValue profileData;  // All the current profile data (as a copy)

    array<AHGUI::Element@> furColorSelected; // Keep track of which fur color element is selected

    // New selection
    WrappingTicker currentProfile(1);

    // Intro page
    BlockingTicker currentIntroPage(1);

    /*******************************************************************************************/
    /**
     * @brief  Constructor
     * 
     */
    ArenaGUI() {
        // Call the superclass to set things up
        super();

        characterSelection.setMax( global_data.getCharacters().size() );

        // initialize the background
        ribbon_background.Init();

        // Start a new arena session
        global_data.startNewSession();

        // Read the data from the file (this will create it if we don't have it)
        global_data.ReadPersistentInfo();

        // see if we already have some profiles
        profileData = global_data.getProfiles();

        if( global_data.getSessionProfile() == -1 )
        {
            currentState = agsMainMenu;
        }
        else
        {
            UpdateStateBasedOnCurrentWorldNode();
        }
    }

    AHGUI::Divider@ addFooter(bool showback = true, bool showcontinue = true)
    {
        AHGUI::Divider@ footer = root.addDivider( DDBottomRight, DOHorizontal, ivec2( AH_UNDEFINEDSIZE, 200 ) );
        footer.setName("footerdiv");

        footer.addSpacer( 175, DDLeft );
        footer.addSpacer( 175, DDRight );

        if( showback )
        {
            array<string> sub_names = {"backimage", "backtext"};
            AHGUI::MouseOverPulseColorSubElements buttonHoverSubElements( 
                                            HexColor("#ffde00"), 
                                            HexColor("#ffe956"), .25, sub_names );

            AHGUI::Divider@ backDivider = footer.addDivider( DDLeft, DOHorizontal, ivec2( 200, AH_UNDEFINEDSIZE ) );

            backDivider.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("back") );
            backDivider.addMouseOverBehavior( buttonHoverSubElements, "mouseover" );
            backDivider.setName("backdivider");

            AHGUI::Image backImage("Textures/ui/arena_mode/left_arrow.png");
            backImage.scaleToSizeX(75);
            backImage.addUpdateBehavior( AHGUI::FadeIn( 1000, @inSine ) );
            backImage.setName("backimage");

            backDivider.addElement( backImage, DDLeft);  
            backDivider.addSpacer(30, DDLeft);

            AHGUI::Text backText( "Back", title_font, 60, button_color );
            backText.addUpdateBehavior( AHGUI::FadeIn( 1000, @inSine ) );
            backText.setName("backtext");
            
            backDivider.addElement( backText, DDLeft );
        }

        if( showcontinue )
        {
            array<string> sub_names = {"nextimage", "nexttext"};
            AHGUI::MouseOverPulseColorSubElements buttonHoverSubElements( 
                                            HexColor("#ffde00"), 
                                            HexColor("#ffe956"), .25, sub_names );

            AHGUI::Divider@ nextDivider = footer.addDivider( DDRight, DOHorizontal, ivec2( 200, AH_UNDEFINEDSIZE ) );

            nextDivider.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("next"), "leftclick" );
            nextDivider.addMouseOverBehavior( buttonHoverSubElements, "mouseover" );
            nextDivider.setName("nextdivider");

            AHGUI::Image nextImage("Textures/ui/arena_mode/right_arrow.png");
            nextImage.scaleToSizeX(75);
            nextImage.addUpdateBehavior( AHGUI::FadeIn( 1000, @inSine ), "update" );
            nextImage.setName("nextimage");

            nextDivider.addElement(nextImage, DDRight);  
            nextDivider.addSpacer(30, DDRight);

            AHGUI::Text nextText( "Next", title_font, 60, button_color );
            nextText.addUpdateBehavior( AHGUI::FadeIn( 1000, @inSine ), "update" );
            nextText.setName("nexttext");
            
            nextDivider.addElement( nextText, DDRight );
        }

        return footer;
    }

    /*******************************************************************************************/
    /**
     * @brief  Change the contents of the GUI based on the state
     *
     */
    void handleStateChange() 
    {
        //see if anything has changed
        if( not forceReloadState && lastState == currentState ) {
            return;
        }

        if( forceReloadState ) {
            forceReloadState = false;
        }

        // Record the change
        lastState = currentState;

        // First clear the old screen 
        clear();

        ArenaCampaignSanityCheck( global_data );

        // Now we switch on the state
        switch( currentState ) {
            
            case agsInvalidState: {
                // For completeness -- throw an error and move on
                DisplayError("GUI Error", "GUI in invalid state");

            }
            break;

            case agsMainMenu: {

                addFooter(true,false);

                AHGUI::Divider@ mainpane = root.addDivider( DDTop,
                                                            DOHorizontal, 
                                                            ivec2( AH_UNDEFINEDSIZE, 1140 ) );

                mainpane.addSpacer(left_margin, DDLeft);
                AHGUI::Divider@ menulist = mainpane.addDivider( DDTop,
                                                                DOVertical,
                                                                ivec2(400, AH_UNDEFINEDSIZE) );
                menulist.addSpacer(top_margin, DDTop);

                mainpane.setName("mainpane");
                menulist.setName("menulist");

                AHGUI::Text arenaModeTitle( "Arena Mode", title_font, title_font_size, title_color );
                
                menulist.addElement(arenaModeTitle, DDTop);

                AHGUI::Image titleUnderline("Textures/ui/arena_mode/main_menu_title_underline.png");

                menulist.addElement(titleUnderline, DDTop); 

                AHGUI::Text newProfile( "Continue", button_font, 100, activatedButton );

                if( global_data.getProfiles().size() > 0 )
                {
                    newProfile.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("continue") );
                    newProfile.addMouseOverBehavior( buttonHover );
                }
                else
                {
                    newProfile.setColor(deactivatedButton);
                }

                menulist.addSpacer(150, DDTop);
                menulist.addElement( newProfile, DDTop );

                AHGUI::Text newCampaignButton( "New Campaign", button_font, 100, activatedButton );

                // Have it send a message to indicate we should go to create profile state
                newCampaignButton.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("new") );

                // Make it pulse when we mouse over 
                newCampaignButton.addMouseOverBehavior( buttonHover );
                
                // Add this to the main pane  
                menulist.addSpacer(60, DDTop);
                menulist.addElement( newCampaignButton, DDTop );


                AHGUI::Text instantActionButton( "Instant Action", button_font, 100, deactivatedButton );

                // Add this to the main pane  
                menulist.addSpacer(60, DDTop);
                menulist.addElement( instantActionButton, DDTop );

                AHGUI::Text customMatchButton( "Custom Match", button_font, 100, deactivatedButton );

                // Add this to the main pane  
                menulist.addSpacer(60, DDTop);
                menulist.addElement( customMatchButton, DDTop );

                // For fun let's put an image on the screen 
                //AHGUI::Image topImage("Textures/ui/versus_mode/fight_glyph.tga");
                //topImage.scaleToSizeX(400);
                //topImage.addUpdateBehavior( AHGUI::FadeIn( 1000, @inSine ) );

                // Add this to the main pane 
                //mainpane.addElement( topImage, DDTop );    
            }
            break;

            case agsNewProfile: {

                addFooter();

                newCharacter = global_data.generateNewProfile();

                AHGUI::Divider@ mainpane = root.addDivider( DDTop,  
                                                            DOVertical, 
                                                            ivec2( AH_UNDEFINEDSIZE, 1140 ) );

                mainpane.addSpacer( top_margin, DDTop );
                
                AHGUI::Text title = AHGUI::Text( "Select Character", title_font, title_font_size, title_color );

                mainpane.addElement( title, DDTop );

                 
                mainpane.addElement(AHGUI::Image("Textures/ui/arena_mode/main_menu_title_underline.png"), DDTop); 
                
                mainpane.addSpacer(40,DDTop);

                AHGUI::Divider@ profileNamediv = mainpane.addDivider( DDTop,
                                                                      DOHorizontal,
                                                                      ivec2(500, 70) ); 

                AHGUI::Text profileNameLabel = AHGUI::Text( "Profile Name: ", label_font, label_font_size, label_color );
                profileNamediv.addElement( profileNameLabel, DDLeft );

                AHGUI::Text profileNameValue = AHGUI::Text( newCharacter["character_name"].asString(), label_font, label_font_size, label_color );
                profileNameValue.setName("newnametext");
                profileNameValue.addMouseOverBehavior( buttonHover );
                profileNameValue.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("newrandomname") );
                profileNamediv.addElement( profileNameValue, DDLeft );
                
                mainpane.addSpacer( 30, DDTop );


                mainpane.addElement(AHGUI::Image("Textures/ui/arena_mode/select_character_divider.png"), DDTop); 

                AHGUI::Divider@ centerpane = mainpane.addDivider( DDTop,
                                                                  DOHorizontal,
                                                                  ivec2(1700, 700) );

                AHGUI::Divider@ characterdiv = centerpane.addDivider( DDLeft,
                                                                      DOHorizontal,
                                                                      ivec2(700,AH_UNDEFINEDSIZE) );

                
                AHGUI::Image leftarrow = AHGUI::Image("Textures/ui/arena_mode/left_arrow.png");
                leftarrow.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("previous_character") );
                leftarrow.addMouseOverBehavior( buttonHover );
                characterdiv.addElement(leftarrow, DDLeft);

                AHGUI::Divider@ characterImagediv = characterdiv.addDivider( DDCenter,
                                                                             DOVertical,
                                                                             ivec2(250, AH_UNDEFINEDSIZE) );
                
                AHGUI::Image rightarrow = AHGUI::Image("Textures/ui/arena_mode/right_arrow.png");
                rightarrow.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("next_character") );
                rightarrow.addMouseOverBehavior( buttonHover );
                characterdiv.addElement(rightarrow, DDRight);

                centerpane.addSpacer(75, DDRight);
                AHGUI::Divider@ infodiv = centerpane.addDivider( DDRight,
                                                                 DOVertical,
                                                                 ivec2(600, AH_UNDEFINEDSIZE) );

                AHGUI::Image@ portrait = AHGUI::Image();
                portrait.setName("portrait");

                characterImagediv.addElement(portrait,DDCenter);

                infodiv.setSize(900, 700);
                
                infodiv.addSpacer(50, DDTop);

                AHGUI::Text charactername = AHGUI::Text("", label_font, label_font_size, label_color);
                charactername.setName("title");
                infodiv.addElement(charactername, DDTop);

                AHGUI::Text descriptiontext = AHGUI::Text("", infotext_font, infotext_font_size, infotext_color);
                descriptiontext.setName("description");
                infodiv.addElement(descriptiontext, DDTop);

                infodiv.addSpacer(50,DDBottom);
                AHGUI::Divider@ iconListdiv = infodiv.addDivider(DDBottom, DOHorizontal, ivec2(250, AH_UNDEFINEDSIZE));
                iconListdiv.setName("statediv");

                mainpane.addElement(AHGUI::Image("Textures/ui/arena_mode/select_character_divider.png"), DDTop); 

                ChangeToCharacter(int(characterSelection));

                }
            break;
            case agsSelectProfile: {

                currentProfile.setMax( global_data.getProfiles().size() ); 

                AHGUI::Divider@ footer = addFooter();

                AHGUI::Text deleteButton( "Delete Profile", label_font, label_font_size, label_color );
                deleteButton.addMouseOverBehavior( buttonHover );
                deleteButton.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("delete") );
                deleteButton.addUpdateBehavior( AHGUI::FadeIn( 1000, @inSine ) );

                footer.addElement( deleteButton, DDCenter );
                

                AHGUI::Divider@ mainpane = root.addDivider( DDTop,
                                                            DOHorizontal, 
                                                            ivec2( AH_UNDEFINEDSIZE, 1140 ) );

                mainpane.addSpacer(left_margin, DDLeft);
                AHGUI::Divider@ menulist = mainpane.addDivider( DDTop,
                                                                DOVertical,
                                                                ivec2(400, AH_UNDEFINEDSIZE) );
                menulist.addSpacer(top_margin, DDTop);

                mainpane.setName("mainpane");
                menulist.setName("menulist");

                AHGUI::Text arenaModeTitle( "Arena Mode", title_font, title_font_size, title_color );
                
                menulist.addElement(arenaModeTitle, DDTop);

                AHGUI::Image titleUnderline("Textures/ui/arena_mode/main_menu_title_underline.png");

                menulist.addElement(titleUnderline, DDTop); 

                AHGUI::Divider@ characterdiv = menulist.addDivider( DDLeft,
                                                                      DOHorizontal,
                                                                      ivec2(700,AH_UNDEFINEDSIZE) );
                
                AHGUI::Image leftarrow = AHGUI::Image("Textures/ui/arena_mode/left_arrow.png");
                leftarrow.setName( "leftarrow" );
                leftarrow.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("previous_profile") );
                leftarrow.addMouseOverBehavior( buttonHover );
                characterdiv.addElement(leftarrow, DDLeft);

                AHGUI::Divider@ infodiv = characterdiv.addDivider( DDCenter,
                                                                   DOVertical,
                                                                   ivec2(250, AH_UNDEFINEDSIZE) );
                infodiv.addSpacer(100,DDTop);

                AHGUI::Text profileName = AHGUI::Text( "", label_font, label_font_size, label_color );
                profileName.setName("profilename"); 
                infodiv.addElement(profileName, DDTop);

                infodiv.addSpacer(100,DDTop);

                AHGUI::Divider@ battlesFoughtdiv = infodiv.addDivider(  DDTop, 
                                                                        DOHorizontal,
                                                                        ivec2( 0, 0 ) );


                AHGUI::Text battlesFoughtLabel = AHGUI::Text( "Battles Fought: ", label_font, label_font_size, label_color );
                battlesFoughtdiv.addElement( battlesFoughtLabel, DDLeft );

                AHGUI::Text battlesFought = AHGUI::Text( "0", label_font, label_font_size, label_color );
                battlesFought.setName("battlesfought");
                battlesFoughtdiv.addElement( battlesFought, DDLeft );

                infodiv.addSpacer(30,DDTop);
                
                AHGUI::Divider@ totalFansdiv = infodiv.addDivider(  DDTop, 
                                                                        DOHorizontal,
                                                                        ivec2( 0,0 ) );

                AHGUI::Text totalFansLabel = AHGUI::Text( "Total Fans: ", label_font, label_font_size, label_color );
                totalFansdiv.addElement( totalFansLabel, DDLeft );

                AHGUI::Text totalFans = AHGUI::Text( "0", label_font, label_font_size, label_color );
                totalFans.setName("totalfans");
                totalFansdiv.addElement( totalFans, DDLeft );

                infodiv.addSpacer(30,DDTop);

                AHGUI::Divider@ skillAssesmentdiv = infodiv.addDivider(  DDTop, 
                                                                        DOHorizontal,
                                                                        ivec2( 0,0 ) );

                AHGUI::Text skillAssesmentLabel = AHGUI::Text( "Skill Assesment: ", label_font, label_font_size, label_color );
                skillAssesmentdiv.addElement( skillAssesmentLabel, DDLeft );

                AHGUI::Text skillAssesment = AHGUI::Text( "0", label_font, label_font_size, label_color );
                skillAssesment.setName( "skillassesment" );
                skillAssesmentdiv.addElement( skillAssesment, DDLeft );

                infodiv.addSpacer(30,DDTop);
                
                AHGUI::Divider@ statesdiv = infodiv.addDivider(  DDTop, 
                                                                    DOHorizontal,
                                                                    ivec2( 0,100 ) );
                statesdiv.setName( "states" );
                    
                infodiv.addSpacer( 25, DDTop );

                AHGUI::Divider@ statesdiv2 = infodiv.addDivider(  DDTop, 
                                                                    DOHorizontal,
                                                                    ivec2( 0,100 ) );
                statesdiv2.setName( "states2" );

                
                AHGUI::Image rightarrow = AHGUI::Image("Textures/ui/arena_mode/right_arrow.png");
                rightarrow.setName( "rightarrow" );
                rightarrow.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("next_profile") );
                rightarrow.addMouseOverBehavior( buttonHover );
                characterdiv.addElement(rightarrow, DDRight);

                ChangeToProfile( int(currentProfile) ); 
            }
            break;
            case agsCharacterIntro: {

                currentIntroPage = 0;
                currentIntroPage.setMax(global_data.getCurrentCharacter()["intro"]["pages"].size());

                addFooter();

                AHGUI::Divider@ mainpane = root.addDivider( DDTop,  
                                                            DOVertical, 
                                                            ivec2( AH_UNDEFINEDSIZE, 1140 ) );

                mainpane.addSpacer( top_margin, DDTop );

                AHGUI::Text title = AHGUI::Text("", title_font, title_font_size, title_color );
                title.setName( "title" );
                mainpane.addElement( title, DDTop );  

                AHGUI::Image titleUnderline("Textures/ui/arena_mode/main_menu_title_underline.png");

                mainpane.addElement(titleUnderline, DDTop); 
        
                mainpane.addSpacer( 50, DDTop );

                AHGUI::Divider@ bodydiv = mainpane.addDivider( DDTop,
                                             DOHorizontal,
                                             ivec2(1409, 823) ); 

                bodydiv.setBackgroundImage( "Textures/ui/arena_mode/intro_background.png" );

                AHGUI::Divider@ subbodydiv = bodydiv.addDivider( DDCenter,
                                             DOVertical,
                                             ivec2(1409, AH_UNDEFINEDSIZE) ); 
                

                subbodydiv.addSpacer( 100, DDTop );
                AHGUI::Divider@ descriptiondiv = subbodydiv.addDivider( DDTop,
                                                    DOVertical,
                                                    ivec2(1200, 200));
                descriptiondiv.setName("descriptiondiv");

                subbodydiv.addSpacer( 100, DDBottom );
                AHGUI::Image glyph = AHGUI::Image("Textures/ui/arena_mode/black_glyphs/two_characters_chained_by_one.png");
                glyph.setName("glyph");
                subbodydiv.addElement( glyph, DDBottom );

                mainpane.addSpacer(50, DDTop );

                AHGUI::Divider@ pagediv = mainpane.addDivider( DDTop,
                                            DOHorizontal,
                                            ivec2(100, AH_UNDEFINEDSIZE) ); 

                AHGUI::Image prevImage = AHGUI::Image("Textures/ui/arena_mode/left_arrow.png");

                prevImage.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("previous_page") );

                prevImage.scaleToSizeX(75);

                prevImage.addUpdateBehavior( AHGUI::FadeIn( 1000, @inSine ) );

                prevImage.addMouseOverBehavior( buttonHover );
                
                pagediv.addElement(prevImage, DDLeft);

                pagediv.addSpacer(40,DDLeft);

                AHGUI::Divider@ pagecounterwrapper = pagediv.addDivider( DDLeft,
                                                        DOVertical,
                                                        ivec2(200, AH_UNDEFINEDSIZE));  
                AHGUI::Text pagecounter = AHGUI::Text("0/0", label_font, label_font_size, label_color );
                pagecounter.setName( "pagecounter" );
                pagecounterwrapper.addElement(pagecounter, DDLeft);

                pagediv.addSpacer(40,DDLeft);

                AHGUI::Image nextPageImage = AHGUI::Image("Textures/ui/arena_mode/right_arrow.png");

                nextPageImage.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("next_page") );

                nextPageImage.scaleToSizeX(75);

                nextPageImage.addUpdateBehavior( AHGUI::FadeIn( 1000, @inSine ) );

                nextPageImage.addMouseOverBehavior( buttonHover );
                
                pagediv.addElement(nextPageImage, DDLeft);

                RefreshStoryPage(); }
            break;
            case agsMetaChoice: {
                AHGUI::Divider@ toppadwrapper = root.addDivider( DDTop,
                                                                 DOVertical,
                                                                 ivec2( AH_UNDEFINEDSIZE, screen_height ) );
                toppadwrapper.addSpacer( top_margin, DDTop );

                AHGUI::Divider@ wrapperpane = toppadwrapper.addDivider( DDTop,
                                                               DOHorizontal,
                                                               ivec2( AH_UNDEFINEDSIZE, screen_height-top_margin ) );
                wrapperpane.addSpacer( 30, DDLeft );

                AHGUI::Divider@ leftpane = wrapperpane.addDivider( DDLeft, 
                                                                   DOHorizontal,
                                                                   ivec2(status_pane_column_width*2+status_pane_column_padding, left_status_pane_height) );
                leftpane.setName("statuspane");

                wrapperpane.addSpacer( 30, DDLeft );

                AHGUI::Image verticaldivider("Textures/ui/arena_mode/vertical_divider.png");
                wrapperpane.addElement(verticaldivider, DDLeft );

                wrapperpane.addSpacer( 30, DDLeft );

                AHGUI::Divider@ mainpane = wrapperpane.addDivider( DDLeft,  
                                                            DOVertical, 
                                                            ivec2( 1600, screen_height-top_margin ) );

                AHGUI::Text title = AHGUI::Text("Title", title_font, title_font_size, title_color );
                title.setName( "title" );
                mainpane.addElement( title, DDTop );  

                AHGUI::Image titleUnderline("Textures/ui/arena_mode/main_menu_title_underline.png");

                mainpane.addElement(titleUnderline, DDTop); 
        
                mainpane.addSpacer( 100, DDTop );

                AHGUI::Divider@ bodydiv = mainpane.addDivider( DDTop,
                                             DOHorizontal,
                                             ivec2(AH_UNDEFINEDSIZE, 961) ); 
                bodydiv.addSpacer(100, DDLeft);

                AHGUI::Divider@ subbodydiv = bodydiv.addDivider( DDLeft,
                                             DOVertical,
                                             ivec2(1407, 961) ); 
                subbodydiv.setBackgroundImage( "Textures/ui/arena_mode/meta_background.png" );
                subbodydiv.addSpacer(100,DDTop);

                AHGUI::Divider@ descriptiondiv = subbodydiv.addDivider( DDTop,
                                             DOVertical,
                                             ivec2(1207, 500) );

                descriptiondiv.setName("description");

                subbodydiv.addSpacer(100,DDTop);

                AHGUI::Divider@ optionsdiv = subbodydiv.addDivider( DDTop,
                                             DOVertical,
                                             ivec2(800, AH_UNDEFINEDSIZE) ); 
                optionsdiv.setName("options");
                
                RefreshMetaPage();
            }
            break;
            case agsMessage: {

                AHGUI::Divider@ toppadwrapper = root.addDivider( DDTop,
                                                                 DOVertical,
                                                                 ivec2( AH_UNDEFINEDSIZE, screen_height ) );
                toppadwrapper.addSpacer( top_margin, DDTop );

                AHGUI::Divider@ wrapperpane = toppadwrapper.addDivider( DDTop,
                                                               DOHorizontal,
                                                               ivec2( AH_UNDEFINEDSIZE, screen_height-top_margin ) );
                wrapperpane.addSpacer( 30, DDLeft );

                AHGUI::Divider@ leftpane = wrapperpane.addDivider( DDLeft, 
                                                                   DOHorizontal,
                                                                   ivec2(status_pane_column_width*2+status_pane_column_padding, left_status_pane_height) );
                leftpane.setName("statuspane");

                wrapperpane.addSpacer( 30, DDLeft );

                AHGUI::Image verticaldivider("Textures/ui/arena_mode/vertical_divider.png");
                wrapperpane.addElement(verticaldivider, DDLeft );

                wrapperpane.addSpacer( 30, DDLeft );

                AHGUI::Divider@ mainpane = wrapperpane.addDivider( DDLeft,  
                                                            DOVertical, 
                                                            ivec2( 1600, screen_height-top_margin ) );

                AHGUI::Text title = AHGUI::Text("Title", title_font, title_font_size, title_color );
                title.setName( "title" );
                mainpane.addElement( title, DDTop );  

                AHGUI::Image titleUnderline("Textures/ui/arena_mode/main_menu_title_underline.png");

                mainpane.addElement(titleUnderline, DDTop); 
        
                mainpane.addSpacer( 100, DDTop );

                AHGUI::Divider@ bodydiv = mainpane.addDivider( DDTop,
                                             DOHorizontal,
                                             ivec2(AH_UNDEFINEDSIZE, 961) ); 
                bodydiv.addSpacer(100, DDLeft);

                AHGUI::Divider@ subbodydiv = bodydiv.addDivider( DDLeft,
                                             DOVertical,
                                             ivec2(1407, 961) ); 
                subbodydiv.setBackgroundImage( "Textures/ui/arena_mode/meta_background.png" );
                subbodydiv.addSpacer(100,DDTop);

                AHGUI::Divider@ descriptiondiv = subbodydiv.addDivider( DDTop,
                                             DOVertical,
                                             ivec2(1207, 500) );

                descriptiondiv.setName("description");

                subbodydiv.addSpacer(100,DDTop);

                AHGUI::Divider@ optionsdiv = subbodydiv.addDivider( DDTop,
                                             DOVertical,
                                             ivec2(800, AH_UNDEFINEDSIZE) ); 
                optionsdiv.setName("options");
                
                RefreshMessage();
            }
            break;
            case agsEndScreen: {

                AHGUI::Divider@ footer = addFooter(false,false);

                AHGUI::Divider@ backDivider = footer.addDivider( DDCenter, DOHorizontal, ivec2( 0, AH_UNDEFINEDSIZE ) );
                backDivider.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("back") );
                AHGUI::Text backText( "The End", button_font, 100, button_color );
                backText.addUpdateBehavior( AHGUI::FadeIn( 1000, @inSine ) );
                backText.addMouseOverBehavior( buttonHover );
                backDivider.addElement( backText, DDLeft );

                AHGUI::Divider@ mainpane = root.addDivider( DDTop,  
                                                            DOVertical, 
                                                            ivec2( AH_UNDEFINEDSIZE, 1000 ) );

                mainpane.addSpacer( 100, DDTop );

                int tabwidth = 700;

                AHGUI::Text title = AHGUI::Text("Title", title_font, title_font_size, title_color );
                title.setName( "title" );
                mainpane.addElement( title, DDTop );  

                AHGUI::Image titleUnderline("Textures/ui/arena_mode/main_menu_title_underline.png");
                mainpane.addElement(titleUnderline, DDTop); 

                AHGUI::Divider@ subtitleDiv = mainpane.addDivider( DDTop, DOHorizontal, ivec2(0,100) );
                AHGUI::Image grayLeftUnderline = AHGUI::Image("Textures/ui/arena_mode/gray_underline_left.png");
                subtitleDiv.addElement( grayLeftUnderline, DDLeft );  
                subtitleDiv.addSpacer(50,DDLeft);
                AHGUI::Text subtitle = AHGUI::Text("DEFEAT!", body_font, body_font_size, body_color);
                subtitle.setName("subtitle");
                subtitleDiv.addElement( subtitle, DDCenter );
                AHGUI::Image grayRightUnderline = AHGUI::Image("Textures/ui/arena_mode/gray_underline_right.png");
                subtitleDiv.addSpacer(50,DDLeft);
                subtitleDiv.addElement( grayRightUnderline, DDRight );  


                int mainInfoDivHeight = 600;
                AHGUI::Divider@ mainInfoDiv = mainpane.addDivider( DDTop, DOHorizontal, ivec2(0,mainInfoDivHeight) );
                
                {
                    int spacerHeight = 60;
                    AHGUI::Divider@ characterDiv = mainInfoDiv.addDivider(DDLeft, DOVertical, ivec2(tabwidth,mainInfoDivHeight) );
                    AHGUI::Text characterTitle = AHGUI::Text( "Slave", label_font, label_font_size, label_color );
                    characterTitle.setName("charactertitle");
                    characterDiv.addElement( characterTitle, DDTop );
                    characterDiv.addSpacer(spacerHeight,DDTop);
                    AHGUI::Image characterImage = AHGUI::Image("Textures/ui/arena_mode/character_image/slave.png");
                    characterImage.setName("characterimage");
                    characterImage.scaleToSizeY(mainInfoDivHeight-200);
                    characterDiv.addElement( characterImage, DDTop );
                }
                {
                    int spacerHeight = 40;
                    AHGUI::Divider@ killInfoDiv = mainInfoDiv.addDivider( DDCenter, DOVertical, ivec2(tabwidth,mainInfoDivHeight) );
                    killInfoDiv.addSpacer(spacerHeight*2,DDTop); 
                    AHGUI::Text killcount = AHGUI::Text("----", label_font, label_font_size, label_color ); 
                    killcount.setName("killcount");
                    killInfoDiv.addElement(killcount, DDTop);
                    killInfoDiv.addSpacer(spacerHeight,DDTop);
                    AHGUI::Divider@ killsDiv = killInfoDiv.addDivider(DDTop,DOHorizontal,ivec2(AH_UNDEFINEDSIZE,60));
                    killsDiv.setName("killsdiv");
                    killInfoDiv.addSpacer(spacerHeight*2,DDTop);
                    AHGUI::Text kocount = AHGUI::Text("----", label_font, label_font_size, label_color );
                    kocount.setName("kocount");
                    killInfoDiv.addElement(kocount,DDTop);
                    killInfoDiv.addSpacer(spacerHeight,DDTop);
                    AHGUI::Divider@ kodiv = killInfoDiv.addDivider(DDTop,DOHorizontal,ivec2(AH_UNDEFINEDSIZE,60));
                    kodiv.setName("kodiv");
                }
                {
                    int spacerHeight = 100;
                    AHGUI::Divider@ generalStats = mainInfoDiv.addDivider( DDRight, DOVertical, ivec2(tabwidth,mainInfoDivHeight) );
                    generalStats.addSpacer(spacerHeight,DDTop);
                    AHGUI::Text battleswon = AHGUI::Text("Battles Won", label_font, label_font_size, label_color );
                    battleswon.setName("battleswon");
                    generalStats.addElement(battleswon, DDTop);

                    generalStats.addSpacer(spacerHeight,DDTop);
                    AHGUI::Text battleslost = AHGUI::Text("Battles Lost" , label_font, label_font_size, label_color );
                    battleslost.setName("battleslost");
                    generalStats.addElement(battleslost, DDTop);

                    generalStats.addSpacer(spacerHeight,DDTop);
                    AHGUI::Text mortalwounds = AHGUI::Text("Mortal Wounds" , label_font, label_font_size, label_color );
                    mortalwounds.setName("mortalwound");
                    generalStats.addElement(mortalwounds, DDTop);

                    generalStats.addSpacer(spacerHeight,DDTop);
                    AHGUI::Text playtime = AHGUI::Text("Play Time", label_font, label_font_size, label_color );
                    playtime.setName("playtime");
                    generalStats.addElement(playtime, DDTop);

                    /*
                    generalStats.addSpacer(spacerHeight,DDTop);
                    AHGUI::Text wealth = AHGUI::Text("Wealth", label_font, label_font_size, label_color );
                    wealth.setName("wealth");
                    generalStats.addElement(wealth, DDTop);
                    */
                }
                {
                    AHGUI::Divider@ statediv = mainpane.addDivider( DDTop, DOHorizontal, ivec2(0,100) );
                    statediv.setName( "statediv" );
                }
                {
                    AHGUI::Text state_name = AHGUI::Text("State", label_font, label_font_size, label_color );
                    state_name.setName( "state" );
                    state_name.setVisible( false );
                    mainpane.addElement( state_name, DDTop );
                }
                RefreshEndScreen();
            }
            break;
            case agsMapScreen:{

                addFooter(true,true);

                int main_height = left_status_pane_height;

                AHGUI::Divider@ toppadwrapper = root.addDivider( DDTop,
                                                                 DOVertical,
                                                                 ivec2( AH_UNDEFINEDSIZE, main_height) );
                toppadwrapper.addSpacer( top_margin, DDTop );

                AHGUI::Divider@ wrapperpane = toppadwrapper.addDivider( DDTop,
                                                               DOHorizontal,
                                                               ivec2( AH_UNDEFINEDSIZE, main_height) );
                wrapperpane.addSpacer( 30, DDLeft );

                AHGUI::Divider@ leftpane = wrapperpane.addDivider( DDLeft, 
                                                                   DOHorizontal,
                                                                   ivec2(status_pane_column_width*2+status_pane_column_padding, left_status_pane_height) );
                leftpane.setName("statuspane");

                wrapperpane.addSpacer( 30, DDLeft );

                AHGUI::Image verticaldivider("Textures/ui/arena_mode/vertical_divider.png");
                verticaldivider.scaleToSizeY( main_height - 60 );
                wrapperpane.addElement(verticaldivider, DDLeft );

                wrapperpane.addSpacer( 30, DDLeft );

                AHGUI::Divider@ mainpane = wrapperpane.addDivider( DDLeft,  
                                                            DOVertical, 
                                                            ivec2( 1600, main_height-top_margin ) );

                AHGUI::Text title = AHGUI::Text("Title", title_font, title_font_size, title_color );
                title.setName( "title" );
                mainpane.addElement( title, DDTop );  

                AHGUI::Image titleUnderline("Textures/ui/arena_mode/main_menu_title_underline.png");

                mainpane.addElement(titleUnderline, DDTop); 
        
                mainpane.addSpacer( 50, DDTop );

                AHGUI::Divider@ bodydiv = mainpane.addDivider( DDTop,
                                             DOHorizontal,
                                             ivec2(AH_UNDEFINEDSIZE, main_height - 150) ); 
                bodydiv.addSpacer(100, DDLeft);

                AHGUI::Divider@ subbodydiv = bodydiv.addDivider( DDLeft,
                                             DOVertical,
                                             ivec2(1500, main_height - 350) ); 
                //subbodydiv.setBackgroundImage( "Textures/world_map/arena_world_map.png" );
                subbodydiv.addSpacer(100,DDTop);
                subbodydiv.setPadding(100,100,100,100);
                subbodydiv.setName("subbodydiv");
        
                RefreshWorldMap();
            }
            break;
        }
    }


    /*******************************************************************************************/
    /**
     * @brief Called for each message received 
     * 
     * @param message The message in question  
     *
     */
    void processMessage( AHGUI::Message@ message ) {
        Log( info, "Got message: " + message.name + "\n" );
        // Check to see if an exit has been requested 
        if( message.name == "mainmenu" ) {
            global_data.WritePersistentInfo( false );
            global_data.clearSessionProfile();
            global_data.clearArenaSession();
            this_ui.SendCallback("back");
        }

        // switch on the state -- though the messages should be unique
        switch( currentState ) {
            case agsMainMenu: {
                if( message.name == "continue" ) {
                    currentState = agsSelectProfile; 
                }
                else if( message.name == "new" ) {
                    currentState = agsNewProfile;
                }
                else if( message.name == "back" ) {
                    processMessage(AHGUI::Message("mainmenu"));
                }
            }
            break;
            case agsInvalidState: {
                // For completeness -- throw an error and move on
                DisplayError("GUI Error", "GUI in invalid state");
            }
            break;
            case agsNewProfile: {
                if( message.name == "newrandomname") {
                    // Generate a new random name
                    newCharacter["character_name"] = JSONValue( global_data.generateRandomName() );

                    // Find the element in the layout, by name
                    AHGUI::Text@ nameText = cast<AHGUI::Text>(root.findElement("newnametext"));

                    nameText.setText(newCharacter["character_name"].asString());
                }
                else if( message.name == "cancel" ) {
                    // We can throw this all away and go back (depending on if we have profiles)
                    if( profileData.size() != 0 ) {
                        currentState = agsSelectProfile;
                    }
                    else
                    {
                        currentState = agsMainMenu;
                    }
                }
                else if( message.name == "back" ) {
                    currentState = agsMainMenu;
                }
                else if( message.name == "next" ) {
                    JSONValue cur_character = global_data.getCharacters()[int(characterSelection)];

                    newCharacter["character_id"]    = cur_character["id"];
                    newCharacter["states"]          = cur_character["states"];

                    global_data.addProfile( newCharacter );
                    global_data.setDataFrom( newCharacter["id"].asInt() );

                    global_data.queued_world_node_id = cur_character["world_node_id"].asString();
                    global_data.ResolveWorldNode();

                    currentProfile.setMax( global_data.getProfiles().size() );
                    currentProfile = global_data.getProfileIndexFromId(newCharacter["id"].asInt());

                    currentState = agsCharacterIntro;
                }
                else if( message.name == "next_character" ) {
                    characterSelection++;
                    ChangeToCharacter(int(characterSelection));
                }
                else if( message.name == "previous_character" ) {
                    characterSelection--;
                    ChangeToCharacter(int(characterSelection));
                }
            }
            break;
            case agsSelectProfile: {
                if( message.name == "delete" ) {
                    if( global_data.getProfiles().size() > 0 )
                    {
                        global_data.removeProfile( global_data.getProfiles()[int(currentProfile)]["id"].asInt() );
                    }
                
                    if( global_data.getProfiles().size() == 0 )
                    {
                        currentState = agsMainMenu;
                    }

                    currentProfile.setMax( global_data.getProfiles().size() );
                    ChangeToProfile( int(currentProfile) ); 
                }
                else if( message.name == "next" ) {
                    global_data.setDataFrom(  global_data.getProfiles()[int(currentProfile)]["id"].asInt() );
                    global_data.setSessionProfile( global_data.getProfiles()[int(currentProfile)]["id"].asInt() );
                    UpdateStateBasedOnCurrentWorldNode();
                }
                else if( message.name == "back" ) {
                    currentState = agsMainMenu;
                }
                else if( message.name == "next_profile" ) {
                    currentProfile++; 
                    ChangeToProfile( int(currentProfile) ); 
                }
                else if( message.name == "previous_profile" ) {
                    currentProfile--; 
                    ChangeToProfile( int(currentProfile) ); 
                }
            }
            break;
            case agsCharacterIntro: {
                if( message.name == "back" ) {
                    currentState = agsNewProfile;
                }
                else if( message.name == "next" ) {
                    global_data.setDataFrom(  global_data.getProfiles()[int(currentProfile)]["id"].asInt() );
                    global_data.setSessionProfile( global_data.getProfiles()[int(currentProfile)]["id"].asInt() );
                    UpdateStateBasedOnCurrentWorldNode();
                }
                else if( message.name == "next_page" ) {
                    currentIntroPage++;
                    RefreshStoryPage();
                }
                else if( message.name == "previous_page" ) {
                    currentIntroPage--;
                    RefreshStoryPage();
                }
            }
            break;
            case agsMetaChoice:
            {
                if( message.name == "back" ) {
                    currentState = agsSelectProfile;
                }
                else if( message.name == "next" ) {
                    // Start the level!
                }
                else if( message.name == "option" ) {
                    int option = message.intParams[0]; 

                    global_data.meta_choice_option = option;
                    global_data.done_with_current_node = true;
                    global_data.ResolveWorldNode();

                    UpdateStateBasedOnCurrentWorldNode();
                }
            }
            break;
            case agsMessage:
            { 
                if( message.name == "continue" ) {
                    global_data.done_with_current_node = true;
                    global_data.ResolveWorldNode();
                    UpdateStateBasedOnCurrentWorldNode();
                } 
            }
            break;
            case agsEndScreen:
            {
                if( message.name == "back" ) {
                    currentState = agsMainMenu;
                }
            }
            break;
            case agsMapScreen:
            {
                if( message.name == "back" ) {
                    currentState = agsMainMenu;
                }
                else if( message.name == "next" ) {
                    global_data.done_with_current_node = true;
                    global_data.ResolveWorldNode();
                    UpdateStateBasedOnCurrentWorldNode();
                }
            }
            break;
        }
    }

    bool prev_state = true;
    /*******************************************************************************************/
    /**
     * @brief  Update the menu
     *
     */
    void update() {
        if( IsKeyDown( GetCodeForKey( "f9" ) ) ) 
        {
            currentState = agsMapScreen;
            global_data.world_map_id = "fighter_map";
            global_data.world_map_node_id = "cave_arena";

            global_data.states = array<string>();
            global_data.states.insertLast("rabbit");
            global_data.states.insertLast("well_nourished");
            global_data.states.insertLast("contender");

            global_data.world_map_nodes.resize(0);
            global_data.world_map_nodes.insertLast(WorldMapNodeInstance("something_else",false,true));
            global_data.world_map_nodes.insertLast(WorldMapNodeInstance("cave_arena",false,true));
            global_data.world_map_nodes.insertLast(WorldMapNodeInstance("waterfall_arena",true,false));
            global_data.world_map_nodes.insertLast(WorldMapNodeInstance("slave_camp",false,false));
            global_data.world_map_nodes.insertLast(WorldMapNodeInstance("magma_arena",false,false));

            global_data.world_map_connections.resize(0);
            global_data.world_map_connections.insertLast(WorldMapConnectionInstance("something_else_to_cave_arena"));
            global_data.world_map_connections.insertLast(WorldMapConnectionInstance("cave_arena_to_waterfall_arena"));
            global_data.world_map_connections.insertLast(WorldMapConnectionInstance("waterfall_arena_to_slave_camp"));
        }

        if( IsKeyDown( GetCodeForKey("f12") ) )
        {
            JSON r;

            r.getRoot()["profile"] = global_data.serializeCurrentProfile(); 

            Log(info,r.writeString(true));
        }

        if( IsKeyDown( GetCodeForKey("f5") ) ) 
        {
            forceReloadState = true;
            global_data.ReloadJSON();
        }

        handleStateChange();

        // Update the GUI 
        AHGUI::GUI::update();
    }

    /*******************************************************************************************/
    /**
    * @brief  Render the gui
    * 
    */
    void render() {

        // Update the background 
        // TODO: fold this into AHGUI
        ribbon_background.Update();
        visible = UpdateVisible(visible, target_visible);
        ribbon_background.DrawGUI(visible);
        hud.Draw();

        // Update the GUI 
        AHGUI::GUI::render();
    }

    void ChangeToProfile( int id ) {
        if( currentState == agsSelectProfile ) {
            AHGUI::Text@ profilenametext = cast<AHGUI::Text>(root.findElement("profilename"));
            AHGUI::Text@ battlesfought = cast<AHGUI::Text>(root.findElement("battlesfought"));
            AHGUI::Text@ totalfans = cast<AHGUI::Text>(root.findElement("totalfans"));
            AHGUI::Text@ skillassesment = cast<AHGUI::Text>(root.findElement("skillassesment"));
            AHGUI::Divider@ statesdiv = cast <AHGUI::Divider>(root.findElement("states"));
            AHGUI::Divider@ statesdiv2 = cast <AHGUI::Divider>(root.findElement("states2"));
            AHGUI::Image@ rightarrow = cast<AHGUI::Image>(root.findElement("rightarrow"));
            AHGUI::Image@ leftarrow = cast<AHGUI::Image>(root.findElement("leftarrow"));

            JSONValue cur_profile = global_data.getProfiles()[id];

            if( profilenametext !is null
                && battlesfought !is null
                && totalfans !is null
                && skillassesment !is null
                && statesdiv !is null
                && leftarrow !is null
                && rightarrow !is null ) {

                leftarrow.setVisible( global_data.getProfiles().size() > 1 );
                rightarrow.setVisible( global_data.getProfiles().size() > 1 );
                 
                profilenametext.setText(cur_profile["character_name"].asString());
                battlesfought.setText( "" + (cur_profile["player_wins"].asInt() 
                                        + cur_profile["player_loses"].asInt()));
                totalfans.setText( "" + cur_profile["fans"].asInt());
                skillassesment.setText( "" + floor(cur_profile["player_skill"].asDouble() * 1000 + 0.5f)/1000.0f);

                statesdiv.clear();
                statesdiv2.clear();
                for( uint i = 0; i < 6 ; i++ )
                {
                    AHGUI::Divider@ div;
                    if( i < 3 )
                        @div = @statesdiv;
                    else
                        @div = @statesdiv2;
                    
                    if( i != 0 && i != 3 ) div.addSpacer(50,DDLeft);

                    if( i < cur_profile["states"].size() )
                    {
                        JSONValue state = global_data.getState(cur_profile["states"][i].asString());

                        if( state.type() == JSONobjectValue )
                        {
                            AHGUI::Divider@ icon1div = div.addDivider(DDLeft, DOVertical, ivec2(275, AH_UNDEFINEDSIZE));  

                            if( i == 6 )
                            {
                                icon1div.addElement(AHGUI::Text("...", label_font, label_font_size, label_color), DDBottom);
                            }
                            else
                            {
                                icon1div.addElement(AHGUI::Image(state["glyph"].asString()),DDTop);
                                icon1div.addElement(AHGUI::Text(state["title"].asString(), label_font, label_font_size, label_color), DDBottom);
                            }
                        }
                        else
                        {
                            Log( error, "Missing state " + cur_profile["states"][i].asString() );
                        }
                    }
                    else
                    {
                        AHGUI::Divider@ icon1div = div.addDivider(DDLeft, DOVertical, ivec2(275, AH_UNDEFINEDSIZE));  
                        /*
                        icon1div.addElement(AHGUI::Image("Textures/ui/arena_mode/glyphs/skull.png"),DDTop);
                        icon1div.addElement(AHGUI::Text("Food", label_font, label_font_size, label_color), DDBottom);
                        */
                    }
                }
            }
        }
    }

    void ChangeToCharacter(int id) {
        if( currentState == agsNewProfile ) {
            AHGUI::Divider @statediv = cast<AHGUI::Divider>(root.findElement("statediv"));
            AHGUI::Text@ title = cast<AHGUI::Text>(root.findElement("title"));
            AHGUI::Text@ description = cast<AHGUI::Text>(root.findElement("description"));
            AHGUI::Image@ portrait = cast<AHGUI::Image>(root.findElement("portrait"));

            AHGUI::Divider@ nextDivider = cast<AHGUI::Divider>(root.findElement("nextdivider"));
            AHGUI::Image@ nextImage = cast<AHGUI::Image>(root.findElement("nextimage"));
            AHGUI::Text@ nextText = cast<AHGUI::Text>(root.findElement("nexttext"));

            if( statediv !is null 
                && title !is null 
                && description !is null
                && nextDivider !is null
                && nextImage !is null
                && nextText !is null
                && portrait !is null ) {

                statediv.clear();

                JSONValue cur_character = global_data.getCharacters()[id];

                nextDivider.removeLeftMouseClickBehavior( "leftclick" );

                nextImage.removeUpdateBehavior( "update" );
                nextImage.removeMouseOverBehavior( "mouseover" );
        
                nextText.removeUpdateBehavior( "update" );
                nextText.removeMouseOverBehavior( "mouseover" );

                nextText.setColor(deactivatedButton); 
                nextImage.setColor(deactivatedButton); 

                if( cur_character["enabled"].asBool() )
                {
                    nextImage.setColor(activatedButton);
                    nextText.setColor(activatedButton);

                    nextDivider.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("next"), "leftclick" );

                    nextImage.addMouseOverBehavior( buttonHover, "mouseover" );

                    nextText.addMouseOverBehavior( buttonHover, "mouseover" );
                }

                title.setText(cur_character["title"].asString());
                description.setText(cur_character["description"].asString());
                portrait.setImageFile(cur_character["portrait"].asString());

                for( uint i = 0; i < 4; i++ )
                {
                    if( i < cur_character["states"].size() )
                    {
                        JSONValue state = global_data.getState(cur_character["states"][i].asString());

                        if( state.type() == JSONobjectValue )
                        {
                            if( i > 0 ) statediv.addSpacer(50,DDLeft);

                            AHGUI::Divider@ icon1div = statediv.addDivider(DDLeft, DOVertical, ivec2(275, AH_UNDEFINEDSIZE));  
                        
                            icon1div.addElement(AHGUI::Image(state["glyph"].asString()),DDTop);
                            icon1div.addElement(AHGUI::Text(state["title"].asString(), label_font, label_font_size, label_color), DDBottom);
                        }
                        else
                        {
                            Log( error, "Missing state " + cur_character["states"][i].asString() );
                        }
                    }
                    else
                    {
                        AHGUI::Divider@ icon1div = statediv.addDivider(DDLeft, DOVertical, ivec2(275, AH_UNDEFINEDSIZE));  
                    }
                }
            }
        }
    }

    void RefreshStoryPage()
    {
        if( currentState == agsCharacterIntro )
        {
            int page = int(currentIntroPage);
            AHGUI::Text@ title = cast<AHGUI::Text>(root.findElement( "title" ));
            AHGUI::Divider@ descriptiondiv = cast<AHGUI::Divider>(root.findElement( "descriptiondiv" ));
            AHGUI::Image@ glyph = cast<AHGUI::Image>(root.findElement("glyph"));
            AHGUI::Text@ pagecounter = cast<AHGUI::Text>(root.findElement("pagecounter"));

            if( title !is null 
                && descriptiondiv !is null
                && glyph !is null
                && pagecounter !is null )
            {
                JSONValue jtitle = global_data.getCurrentCharacter()["story_title"];
                JSONValue jpage = global_data.getCurrentCharacter()["intro"]["pages"][page];
                title.setText(jtitle.asString());

                descriptiondiv.clear();
                for( uint i = 0; i < jpage["description"].size(); i++ )
                {
                    AHGUI::Text bodytext = AHGUI::Text(global_data.resolveString(jpage["description"][i].asString()), query_font, query_font_size, query_color );
                    AHGUI::Divider@ bodywrapper = descriptiondiv.addDivider( DDTop, DOHorizontal, ivec2( 1200, 0 ) );
                    bodytext.setHorizontalAlignment( BALeft );
                    bodywrapper.addElement( bodytext, DDLeft );
                }
                descriptiondiv.doRelayout();

                glyph.setImageFile(jpage["glyph"].asString());
                pagecounter.setText("" + (page+1) + "/" + currentIntroPage.getMax());
            }
            else
            {
                Log(error, "unable to update intro page, missing element" );
            }
        }
    }

    void RefreshMetaPage()
    {
        if( currentState == agsMetaChoice )
        {
            JSONValue world_map = global_data.getWorldMap(global_data.world_map_id);
            JSONValue world_map_node = global_data.getWorldNode(global_data.world_node_id);
             
            AHGUI::Text@ title = cast<AHGUI::Text>(root.findElement( "title" ));
            AHGUI::Divider@ description = cast<AHGUI::Divider>(root.findElement( "description" ));
            AHGUI::Divider@ options = cast<AHGUI::Divider>(root.findElement( "options" ));

            RefreshStatusPane();
            
            description.clear();
            options.clear();


            int curdivindex = 0;
            JSONValue meta_choice = global_data.getMetaChoice( global_data.meta_choice_id );

            title.setText(meta_choice["title"].asString());

            for( uint j = 0; j < meta_choice["description"].size(); j++ )
            {
                AHGUI::Divider@ descriptionlinewrapper = description.addDivider( DDTop,
                                                                                 DOHorizontal,
                                                                                 ivec2(1200,AH_UNDEFINEDSIZE ));
                AHGUI::Text descriptionline(global_data.resolveString(meta_choice["description"][j].asString()), query_font, query_font_size, query_color);
                descriptionlinewrapper.setHorizontalAlignment( BALeft );
                descriptionlinewrapper.addElement(descriptionline,DDLeft);
            }

            for( uint j = 0; j < meta_choice["options"].size(); j++ )
            {
                AHGUI::Divider@ optionlinewrapper = options.addDivider( DDTop,
                                                                        DOHorizontal,
                                                                        ivec2( 1200, AH_UNDEFINEDSIZE ));
                AHGUI::Text optionline( "" + (j+1) + ". " + meta_choice["options"][j]["description"].asString(), query_font, query_font_size, query_color);
                optionline.addLeftMouseClickBehavior(AHGUI::FixedMessageOnClick("option", j));
                optionline.addMouseOverBehavior( buttonHover );
                optionlinewrapper.setHorizontalAlignment( BALeft );
                optionlinewrapper.addElement(optionline,DDLeft);
            }
        }
    }

    void RefreshMessage()
    {
        if( currentState == agsMessage )
        {
            JSONValue world_map_node = global_data.getWorldNode(global_data.world_node_id);

            AHGUI::Text@ title = cast<AHGUI::Text>(root.findElement( "title" ));
            AHGUI::Divider@ description = cast<AHGUI::Divider>(root.findElement( "description" ));
            AHGUI::Divider@ options = cast<AHGUI::Divider>(root.findElement( "options" ));

            RefreshStatusPane();
            
            description.clear();
            options.clear();

            int curdivindex = 0;

            JSONValue message = global_data.getMessage( global_data.message_id );

            title.setText(message["title"].asString());

            for( uint j = 0; j < message["description"].size(); j++ )
            {
                AHGUI::Divider@ descriptionlinewrapper = description.addDivider( DDTop,
                                                                                 DOHorizontal,
                                                                                 ivec2(1200,AH_UNDEFINEDSIZE ));
                AHGUI::Text descriptionline(global_data.resolveString(message["description"][j].asString()), query_font, query_font_size, query_color);
                descriptionlinewrapper.addElement(descriptionline,DDLeft);
            }

            AHGUI::Divider@ optionlinewrapper = options.addDivider( DDTop,
                                                                    DOHorizontal,
                                                                    ivec2( 1200, AH_UNDEFINEDSIZE ));
            AHGUI::Text optionline( "Continue", query_font, query_font_size, query_color);
            optionline.addLeftMouseClickBehavior(AHGUI::FixedMessageOnClick("continue"));
            optionline.addMouseOverBehavior(buttonHover);
            optionlinewrapper.addElement(optionline,DDLeft);
        }
    }

    void RefreshEndScreen()
    {
        if( currentState == agsEndScreen )
        {
            JSONValue character = global_data.getCurrentCharacter();

            AHGUI::Text@ title = cast<AHGUI::Text>(root.findElement("title"));
            AHGUI::Image@ characterimage = cast<AHGUI::Image>(root.findElement("characterimage"));
            AHGUI::Text@ subtitle = cast<AHGUI::Text>(root.findElement("subtitle"));
            AHGUI::Text@ charactertitle = cast<AHGUI::Text>(root.findElement("charactertitle"));
            AHGUI::Text@ killcount = cast<AHGUI::Text>(root.findElement("killcount"));
            AHGUI::Divider@ killsdiv = cast<AHGUI::Divider>(root.findElement("killsdiv"));
            AHGUI::Text@ kocount = cast<AHGUI::Text>(root.findElement("kocount"));
            AHGUI::Divider@ kodiv = cast<AHGUI::Divider>(root.findElement("kodiv"));
            AHGUI::Text@ battleswon = cast<AHGUI::Text>(root.findElement("battleswon"));
            AHGUI::Text@ battleslost = cast<AHGUI::Text>(root.findElement("battleslost"));
            AHGUI::Text@ mortalwounds = cast<AHGUI::Text>(root.findElement("mortalwound"));
            AHGUI::Text@ playtime = cast<AHGUI::Text>(root.findElement("playtime"));
            //AHGUI::Text@ wealth = cast<AHGUI::Text>(root.findElement("wealth"));
            AHGUI::Divider@ statediv = cast<AHGUI::Divider>(root.findElement("statediv"));
            AHGUI::Text@ state_text = cast<AHGUI::Text>(root.findElement("state"));

            if( title !is null )
            {
                title.setText(character["story_title"].asString());
            }

            if( characterimage !is null )
            {
                characterimage.setImageFile(character["portrait"].asString());
            }

            if( subtitle !is null )
            {
                if( global_data.hasState("won") )
                {
                    subtitle.setText("SUCCESS!");
                }
                else if( global_data.hasState("lost") )
                {
                    subtitle.setText("DEFEAT!");
                }
                else
                {
                    subtitle.setText("NEITHER!");
                }
            }
        
            if( charactertitle !is null )
            {
                charactertitle.setText( character["title"].asString() ); 
            }
       
            if( killcount !is null )
            {
                killcount.setText( global_data.player_kills + " Kills" );
            }
            
            if( killsdiv !is null )
            {
                killsdiv.clear();
                int c = 0;
                while( c < global_data.player_kills )
                {
                    if( c + 10 <= global_data.player_kills )
                    {
                        AHGUI::Image k10 = AHGUI::Image( "Textures/ui/arena_mode/10_kills.png" );
                        killsdiv.addElement( k10, DDLeft );
                        c += 10;    
                    }
                    else
                    {
                        AHGUI::Image k1 = AHGUI::Image( "Textures/ui/arena_mode/1_kills.png" );
                        killsdiv.addElement( k1, DDLeft );
                        c++;
                    }
                }
            }
            
            if( kocount !is null )
            {
                kocount.setText( global_data.player_kos + " Knockouts" );
            }

            if( kodiv !is null )
            {
                kodiv.clear();
                int c = 0;
                while( c < global_data.player_kos )
                {
                    if( c + 10 <= global_data.player_kos )
                    {
                        AHGUI::Image k10 = AHGUI::Image( "Textures/ui/arena_mode/10_kos.png" );
                        kodiv.addElement( k10, DDLeft );
                        c += 10;    
                    }
                    else
                    {
                        AHGUI::Image k1 = AHGUI::Image( "Textures/ui/arena_mode/1_kos.png" );
                        kodiv.addElement( k1, DDLeft );
                        c++;
                    }
                }
            }

            if( battleswon !is null )
            {
                battleswon.setText( "Battles Won: " + global_data.player_wins ); 
            }

            if( battleslost !is null )
            {
                battleslost.setText( "Battles Lost: " + global_data.player_loses );
            }

            if( mortalwounds !is null )
            {
                mortalwounds.setText( "Mortal Wounds: " + global_data.player_deaths );
            }

            if( playtime !is null )
            {
                playtime.setText( "Play Time: " + global_data.play_time/60 + ":" + global_data.play_time%60 );
            }
            //wealth.setText("");

            if( statediv !is null )
            {
                statediv.clear();
                for( uint i = 0; i < global_data.states.length(); i++ )
                {
                    JSONValue state = global_data.getState(global_data.states[i]);
                    if( state.type() == JSONobjectValue )
                    {
                        if( i != 0 )
                            statediv.addSpacer(40,DDLeft);

                        AHGUI::Image glyph = AHGUI::Image( state["glyph"].asString() ); 
                        glyph.scaleToSizeY(100);
                        glyph.addMouseOverBehavior(MouseHoverSetTextValue(@state_text, state["title"].asString()));
                        glyph.addMouseOverBehavior(buttonHover);
                        statediv.addElement( glyph, DDLeft );
                    }
                    else
                    {
                        Log( error, "Invalid state " + global_data.states[i] );
                    }
                }
            }
        }
    }

    void RefreshStatusPane()
    {
        AHGUI::Divider@ statuspane = cast<AHGUI::Divider>(root.findElement("statuspane"));
        statuspane.clear();

        int height = left_status_pane_height;
        statuspane.setSizeY( height );
        statuspane.setSizeX( status_pane_column_width*2+status_pane_column_padding );

        int curdivindex = 0;

        AHGUI::Divider@ leftpane = statuspane.addDivider( DDLeft, DOVertical, ivec2( status_pane_column_width, height ) );
        statuspane.addSpacer(  status_pane_column_padding, DDLeft );
        AHGUI::Divider@ rightpane = statuspane.addDivider( DDLeft, DOVertical, ivec2( status_pane_column_width, height ) );

        for( uint i = 0; i < global_data.states.length(); i++ )
        {
            AHGUI::Divider@ curdivider = @rightpane;
            if( i % 2 == 0 )
            {
                @curdivider = @leftpane; 
            } 

            AHGUI::Divider@ glyphcontainer = curdivider.addDivider( DDTop, DOVertical, ivec2( status_pane_column_width, status_pane_glyph_height ) );

            JSONValue state = global_data.getState(global_data.states[i]);

            if( state.type() == JSONobjectValue )
            {
                AHGUI::Image glyphimg( state["glyph"].asString() );
                glyphcontainer.addElement( glyphimg, DDTop );

                AHGUI::Text glyphtitle( state["title"].asString(), label_font, label_font_size, label_color );
                glyphcontainer.addElement( glyphtitle, DDTop );
            }
            else
            {
                DisplayError("Unknown state", "Unknown state \"" + global_data.states[i] + "\"");
            }
        }
    }

    string GetWorldMapNodeMarkerImage( WorldMapNodeInstance@ node, bool hover = false )
    {
        string name = "Textures/world_map/map_marker";
        if(!node.is_available)
        {
            name += "_unavailable";
        }

        if(node.is_visited)
        {
            name += "_visited";
        }

        if( node.id == global_data.world_map_node_id )
        {
            name += "_current";
        }
    
        if( hover )
        {
            name += "_hover";
        }

        name += ".png";

        return name;
    }

    void RefreshWorldMap()
    {
        if( currentState == agsMapScreen )
        {
            Log( info, "Showing world map: " + global_data.world_map_id);
            JSONValue world_map = global_data.getCurrentWorldMap();

            AHGUI::Text@ title = cast<AHGUI::Text>(root.findElement( "title" ));
            AHGUI::Divider@ subbodydiv = cast<AHGUI::Divider>(root.findElement("subbodydiv"));
            
            AHGUI::Divider@ nextDivider = cast<AHGUI::Divider>(root.findElement("nextdivider"));
            AHGUI::Image@ nextImage = cast<AHGUI::Image>(root.findElement("nextimage"));
            AHGUI::Text@ nextText = cast<AHGUI::Text>(root.findElement("nexttext"));

            nextDivider.removeLeftMouseClickBehavior( "leftclick" );

            nextImage.removeUpdateBehavior( "update" );
            nextImage.removeMouseOverBehavior( "mouseover" );
    
            nextText.removeUpdateBehavior( "update" );
            nextText.removeMouseOverBehavior( "mouseover" );

            nextText.setColor(deactivatedButton); 
            nextImage.setColor(deactivatedButton); 
            
            RefreshStatusPane();

            subbodydiv.setBackgroundImage( world_map["map"].asString() );

            //TODO: Make work with new node system, where they are now a state rather than static data.
            int linecount = 0;
            for( uint i = 0; i < global_data.world_map_nodes.length(); i++ )
            {
                WorldMapNodeInstance@ inst = global_data.world_map_nodes[i];
                JSONValue node = global_data.getWorldMapNode(inst.id);

                //Check if the currently selected node (if there is one) is valid, meaning we want to enable player to continue
                if( inst.id == global_data.world_map_node_id && inst.is_available && not inst.is_visited )
                {
                    nextImage.setColor(activatedButton);
                    nextText.setColor(activatedButton);

                    nextDivider.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("next"), "leftclick" );

                    nextImage.addMouseOverBehavior( buttonHover, "mouseover" );

                    nextText.addMouseOverBehavior( buttonHover, "mouseover" );
                }

                if( node.type() == JSONobjectValue )
                {
                    float xpos = node["pos"]["x"].asFloat() * float(subbodydiv.getSizeX());
                    float ypos = node["pos"]["y"].asFloat() * float(subbodydiv.getSizeY());
            
                    vec2 title_offset = vec2( node["title_offset"]["x"].asFloat(),node["title_offset"]["y"].asFloat()) * float(subbodydiv.getSizeX());

                    AHGUI::Text node_title( node["title"].asString(), world_map_font, world_map_font_size, world_map_color);
                    //subbodydiv.addElement(node_title, DDTop);
                    int node_title_width = node_title.getSizeX();
                    subbodydiv.addFloatingElement(node_title, "world_map_node_title_" + node["id"].asString(), ivec2(title_offset)+ivec2(int(xpos)-node_title_width/2, int(ypos)-75), 5);
            
                    AHGUI::Image node_marker( GetWorldMapNodeMarkerImage( inst ) );

                    node_marker.setSize( node_marker_width,node_marker_height );

                    if( inst.is_available && not inst.is_visited )
                    {
                        node_marker.addMouseOverBehavior( MouseOverImage( GetWorldMapNodeMarkerImage(inst), GetWorldMapNodeMarkerImage(inst,true) ));
                        node_marker.addLeftMouseClickBehavior( MouseClickWorldNode( this, node["id"].asString() ) );
                        node_marker.addMouseOverBehavior( buttonHover );
                        node_marker.addUpdateBehavior( AHGUI::PulseAlpha( 0.5, 1.0, 0.75 ) );
                    }

                    node_marker.setName("world_map_node_image_" + node["id"].asString());
                    //subbodydiv.addElement( node_marker, DDTop);
                    subbodydiv.addFloatingElement( node_marker, "world_map_node_image_" + node["id"].asString(), ivec2(int(xpos)-node_marker_width/2, int(ypos)-node_marker_height/2 ) );
                }
                else
                {
                    DisplayError( "Can't draw invalid node", "Can't draw invalid world_map_node: " + inst.id );
                }
            }

            AHGUI::Image line_( "Textures/world_map/line_segment.png" );
            int line_segment_width = line_.getSizeX();
            float safe_zone = 50;

            for( uint i = 0; i < global_data.world_map_connections.length(); i++ )
            { 
                JSONValue world_map_connection = global_data.getWorldMapConnection(global_data.world_map_connections[i].id);

                if( world_map_connection.type() == JSONobjectValue )
                {
                    JSONValue from_node = global_data.getWorldMapNode(world_map_connection["from"].asString());
                    JSONValue to_node = global_data.getWorldMapNode(world_map_connection["to"].asString());

                    if( from_node.type() == JSONobjectValue && to_node.type() == JSONobjectValue )
                    {
                        vec2 from_node_pos(
                            from_node["pos"]["x"].asFloat() * float(subbodydiv.getSizeX()),
                            from_node["pos"]["y"].asFloat() * float(subbodydiv.getSizeY()) 
                        );
                        vec2 to_node_pos(
                            to_node["pos"]["x"].asFloat() * float(subbodydiv.getSizeX()),
                            to_node["pos"]["y"].asFloat() * float(subbodydiv.getSizeY()) 
                        );

                        float dist = length( from_node_pos - to_node_pos );
                        vec2 dir = normalize( from_node_pos - to_node_pos );

                        const float pi = 3.141592f;
                        float rotation = -atan2(dir.y, dir.x) * (180/pi);

                        float step = 0;
                        float optimal_step = 0;
                        float missing = 3.402823466e+38;
                         
                        for( int k = 100; k < 200; k++ )
                        {
                            step = line_segment_width + k*0.10;

                            float curdist = safe_zone;
                            while( curdist + step < (dist - safe_zone) )
                            {
                                curdist += step;
                            }
                            
                            if( dist-curdist < missing )
                            {
                                missing = dist-curdist;
                                optimal_step = step;
                            }
                        }
                    
                        step = optimal_step;

                        float curdist = safe_zone;

                        while( curdist < (dist - safe_zone) )
                        {
                            vec2 curpos = to_node_pos + dir * curdist;
                                
                            AHGUI::Image line( "Textures/world_map/line_segment.png" );
                            line.setRotation( rotation );
                            subbodydiv.addFloatingElement( line, "mapline" + linecount++, ivec2(curpos)-ivec2(line_segment_width/2,line_segment_width/2) );

                            curdist += step;
                        }
                    }
                }
            }
        }
    }

    void UpdateStateBasedOnCurrentWorldNode()
    {
        JSONValue world_map_node = global_data.getCurrentWorldNode();

        if( world_map_node["type"].asString() == "meta_choice" )
        {
            currentState = agsMetaChoice;
        }
        else if( world_map_node["type"].asString() == "message" )
        {
            currentState = agsMessage;
        } 
        else if( world_map_node["type"].asString() == "arena_instance" )
        {
            JSONValue arena_instance = global_data.getArenaInstance(global_data.arena_instance_id);      
            this_ui.SendCallback( arena_instance["level"].asString() );
            currentState = agsLoadingLevel;
        }
        else if( world_map_node["type"].asString() == "end_game" )
        {
            currentState = agsEndScreen; 
        }
        else if( world_map_node["type"].asString() == "world_map" )
        {
            currentState = agsMapScreen;
        }
        else
        {
            Log( error, "Unknown world map node type: \"" + world_map_node["type"].asString() + "\"\n" );
            currentState = agsMainMenu;
        }

        //If we move to the same screen again, just force the reload.
        forceReloadState = true;
    }
}

/*******
 *  
 * BEGIN NEW FEATURE DISPLAY (Delete this eventually)
 *
 */

class NewFeaturesExampleGUI : AHGUI::GUI {

    // fancy ribbon background coordinates
    ivec2 fgUpper1Position;
    ivec2 fgUpper2Position;
    ivec2 fgLower1Position;
    ivec2 fgLower2Position;
    ivec2 bgRibbonUp1Position;
    ivec2 bgRibbonUp2Position;
    ivec2 bgRibbonDown1Position;
    ivec2 bgRibbonDown2Position;
    AHGUI::Container@ centerContainer;

    int flyingTextY = 0;
    int flyingTextDir = 1;
    array<int> flyingImagesX = {0,0,0,0,0}; 
    array<int> flyingImagesDir = {1,1,1,1,1}; 

    /*******************************************************************************************/
    /**
     * @brief  Constructor
     * 
     */
    NewFeaturesExampleGUI() {
        // Call the superclass to set things up
        super();

        // Fill the screen ( no letterboxing - *not* 16x9 anymore )
        restrict16x9( false );

        // Initialize the extra layers (one in front, one behind)
        setBackgroundLayers(1);
        setForegroundLayers(1);

        // given that this has to fill the whole screen this is one of the few places 
        //  where you should ever have to reference the size of the screen
        fgUpper1Position =      ivec2( 0,    0 );
        fgUpper2Position =      ivec2( 2560, 0 );
        fgLower1Position =      ivec2( 0,    AHGUI::screenMetrics.GUISpaceY/2 );
        fgLower2Position =      ivec2( 2560, AHGUI::screenMetrics.GUISpaceY/2 );
        bgRibbonUp1Position =   ivec2( 0, 0 );
        bgRibbonUp2Position =   ivec2( 0, AHGUI::screenMetrics.GUISpaceY );
        bgRibbonDown1Position = ivec2( 0, 0 );
        bgRibbonDown2Position = ivec2( 0, -AHGUI::screenMetrics.GUISpaceY );

        // get references to the foreground and background containers
        AHGUI::Container@ background = getBackgroundLayer( 0 );
        AHGUI::Container@ foreground = getForegroundLayer( 0 );

        // Make a new image 
        AHGUI::Image blueBackground("Textures/ui/challenge_mode/blue_gradient_c_nocompress.tga");
        // fill the screen
        blueBackground.setSize(2560, AHGUI::screenMetrics.GUISpaceY);
        blueBackground.setColor( 1.0,1.0,1.0,0.8 );
        // Call it blueBG and give it a z value of 1 to put it furthest behind
        background.addFloatingElement( blueBackground, "blueBG", ivec2( 0, 0 ), 1 );  


        // Note (as I didn't do an example of this)
        // AHGUI::Divider now inherits from AHGUI::Container
        // Dividers work the same as they always did, but all the feature of Container
        // (as demonstrated here) are available in Divider as well -- such as addFloatingElement 
        // and setBackgroundImage

        // Make a new image for half the upper image
        AHGUI::Image fgImageUpper1("Textures/ui/challenge_mode/red_gradient_border_c.tga");
        fgImageUpper1.setSize(2560, AHGUI::screenMetrics.GUISpaceY/2);
        fgImageUpper1.setColor( 0.7,0.7,0.7,1.0 );
        // use only the top half(ish) of the image
        fgImageUpper1.setImageOffset( ivec2(0,0), ivec2(1024, 600) );
        // flip it upside down 
        fgImageUpper1.setRotation( 180 );
        // Call it gradientUpper1 
        foreground.addFloatingElement( fgImageUpper1, "gradientUpper1", fgUpper1Position, 2 );

        // repeat for a second image so we can scroll unbrokenly  
        AHGUI::Image fgImageUpper2("Textures/ui/challenge_mode/red_gradient_border_c.tga");
        fgImageUpper2.setSize(2560, AHGUI::screenMetrics.GUISpaceY/2);
        fgImageUpper2.setColor( 0.7,0.7,0.7,1.0 );
        fgImageUpper2.setImageOffset( ivec2(0,0), ivec2(1024, 600) );
        fgImageUpper2.setRotation( 180 );
        foreground.addFloatingElement( fgImageUpper2, "gradientUpper2", fgUpper2Position, 2 );        

        // repeat again for the bottom image(s) (not flipped this time)
        AHGUI::Image bgImageLower1("Textures/ui/challenge_mode/red_gradient_border_c.tga");
        bgImageLower1.setSize(2560, AHGUI::screenMetrics.GUISpaceY/2);
        bgImageLower1.setColor( 0.7,0.7,0.7,1.0 );
        bgImageLower1.setImageOffset( ivec2(0,0), ivec2(1024, 600) );
        foreground.addFloatingElement( bgImageLower1, "gradientLower1", fgLower1Position, 2 ); 

        AHGUI::Image fgImageLower2("Textures/ui/challenge_mode/red_gradient_border_c.tga");
        fgImageLower2.setSize(2560, AHGUI::screenMetrics.GUISpaceY/2);
        fgImageLower2.setColor( 0.7,0.7,0.7,1.0 );
        fgImageLower2.setImageOffset( ivec2(0,0), ivec2(1024, 600) );
        foreground.addFloatingElement( fgImageLower2, "gradientLower2", fgLower2Position, 2 );   

        // Repeat this same process for the two 'ribbons' which will, instead' go up and down
        AHGUI::Image bgRibbonUp1("Textures/ui/challenge_mode/giometric_ribbon_c.tga");
        bgRibbonUp1.setImageOffset( ivec2(256,0), ivec2(768, 1024) );
        // Fill the left half of the screen
        bgRibbonUp1.setSize(1280, AHGUI::screenMetrics.GUISpaceY);
        bgRibbonUp1.setColor( 0.0,0.0,0.0,1.0 );
        // Put this at the front of the blue background (z=3)
        background.addFloatingElement( bgRibbonUp1, "ribbonUp1", bgRibbonUp1Position, 3 );

        AHGUI::Image bgRibbonUp2("Textures/ui/challenge_mode/giometric_ribbon_c.tga");
        bgRibbonUp2.setImageOffset( ivec2(256,0), ivec2(768, 1024) );
        bgRibbonUp2.setSize(1280, AHGUI::screenMetrics.GUISpaceY);
        bgRibbonUp2.setColor( 0.0,0.0,0.0,1.0 );
        background.addFloatingElement( bgRibbonUp2, "ribbonUp2", bgRibbonUp2Position, 3 );
        
        AHGUI::Image bgRibbonDown1("Textures/ui/challenge_mode/giometric_ribbon_c.tga");
        bgRibbonDown1.setImageOffset( ivec2(256,0), ivec2(768, 1024) );
        bgRibbonDown1.setSize(1280, AHGUI::screenMetrics.GUISpaceY);
        bgRibbonDown1.setColor( 0.0,0.0,0.0,1.0 );
        background.addFloatingElement( bgRibbonDown1, "ribbonDown1", bgRibbonDown1Position, 3 );

        AHGUI::Image bgRibbonDown2("Textures/ui/challenge_mode/giometric_ribbon_c.tga");
        bgRibbonDown2.setImageOffset( ivec2(256,0), ivec2(768, 1024) );
        bgRibbonDown2.setSize(1280, AHGUI::screenMetrics.GUISpaceY);
        bgRibbonDown2.setColor( 0.0,0.0,0.0,1.0 );
        background.addFloatingElement( bgRibbonDown2, "ribbonDown2", bgRibbonDown2Position, 3 );


        // Now construct main pane
        
        // First a footer
        AHGUI::Divider@ footer = root.addDivider( DDBottom, DOHorizontal, ivec2( AH_UNDEFINEDSIZE, 300 ) );
        footer.setName("footerdiv");

        // Add some text to put something here
        AHGUI::Text footerText( "Footer", "edosz", 120, HexColor("#fff") );

        // Manually add it to the divider
        footer.addElement( footerText, DDLeft );

        // Repeat for a header
        AHGUI::Divider@ header = root.addDivider( DDTop, DOHorizontal, ivec2( AH_UNDEFINEDSIZE, 300 ) );
        footer.setName("headerdiv");

        // Add some text to put something here
        AHGUI::Text headerText( "Header", "edosz", 120, HexColor("#fff") );

        // Manually add it to the divider
        header.addElement( headerText, DDRight );

        // Now build the main part of the body 
        // Use a special function to avoid having to mess with variable screen dimensions 
        // This will eat up all the space remaining y space (thus having to call it last)
        AHGUI::Divider@ mainPane = root.addDividerFillCenter( DOHorizontal );

        // Add some fun self-descriptive text 
        AHGUI::Text sideText( "This is rotated text", "edosz", 120, HexColor("#fff") );
        sideText.setRotation( 90 );
        mainPane.addElement( sideText, DDRight );
        // (are you having fun yet?)

        // Create a fixed sized container 
        @centerContainer = @AHGUI::Container( "mainpanel", ivec2( 900, 900 ) );

        // Set a background to this container (will be sized to the container automatically)
        centerContainer.setBackgroundImage( "Textures/ui/challenge_mode/background_map.tga" );
        centerContainer.showBorder(true);

        mainPane.addElement( centerContainer, DDCenter );

        // Add some flying images to show off the container clipping
        for( uint i = 0; i < flyingImagesX.length(); ++i ) {

            AHGUI::Image newImage( "Textures/ui/versus_mode/fight_glyph.tga" );
            newImage.setSize( 300,300 );
            // Vary the color 
            newImage.setColor( 1.0/float(i+1), 1.0/float(i+1), 1.0/float(i+1), 1.0 );

            centerContainer.addFloatingElement( newImage, "flyingImage" + i, ivec2(flyingImagesX[i],150*i), int(i*2) );
        }

        // Finally a bit of text 
        AHGUI::Text flyingText( "Z Sorted Text", "edosz", 120, HexColor("#fff") );
        centerContainer.addFloatingElement( flyingText, "flyingText", ivec2(170, flyingTextY ), 5 );

    }


    /*******************************************************************************************/
    /**
     * @brief Called for each message received 
     * 
     * @param message The message in question  
     *
     */
    void processMessage( AHGUI::Message@ message ) {
        Log( info, "Got message: " + message.name + "\n" );
        // Check to see if an exit has been requested 
        if( message.name == "mainmenu" ) {
            global_data.WritePersistentInfo( false );
            global_data.clearSessionProfile();
            this_ui.SendCallback("back");
        }
    }

    /*******************************************************************************************/
    /**
     * @brief  Update the menu
     *
     */
    void update() {

        // Update the background images (we could have made this into a behavior)
    
        // Calculate the new positions
        fgUpper1Position.x -= 2;
        fgUpper2Position.x -= 2;
        fgLower1Position.x -= 1;
        fgLower2Position.x -= 1;
        bgRibbonUp1Position.y -= 1;
        bgRibbonUp2Position.y -= 1;
        bgRibbonDown1Position.y += 1;
        bgRibbonDown2Position.y += 1;

        // wrap the images around
        if( fgUpper1Position.x == 0 ) {
            fgUpper2Position.x = 2560;
        }

        if( fgUpper2Position.x == 0 ) {
            fgUpper1Position.x = 2560;
        }

        if( fgLower1Position.x == 0 ) {
            fgLower2Position.x = 2560;
        }

        if( fgLower2Position.x == 0 ) {
            fgLower1Position.x = 2560;
        }

        if( bgRibbonUp1Position.y == 0 ) {
            bgRibbonUp2Position.y = AHGUI::screenMetrics.GUISpaceY;
        }

        if( bgRibbonUp2Position.y == 0 ) {
            bgRibbonUp1Position.y = AHGUI::screenMetrics.GUISpaceY;
        }

        if( bgRibbonDown1Position.y == 0 ) {
            bgRibbonDown2Position.y = -AHGUI::screenMetrics.GUISpaceY;
        }

        if( bgRibbonDown2Position.y == 0 ) {
            bgRibbonDown1Position.y = -AHGUI::screenMetrics.GUISpaceY;
        }

        // Get a reference to the first (and only) background container
        AHGUI::Container@ background = getBackgroundLayer( 0 );
        AHGUI::Container@ foreground = getForegroundLayer( 0 );

        // Update the images position in the container
        foreground.moveElement( "gradientUpper1", fgUpper1Position );
        foreground.moveElement( "gradientUpper2", fgUpper2Position );
        foreground.moveElement( "gradientLower1", fgLower1Position );
        foreground.moveElement( "gradientLower2", fgLower2Position );
        background.moveElement( "ribbonUp1",      bgRibbonUp1Position );
        background.moveElement( "ribbonUp2",      bgRibbonUp2Position );
        background.moveElement( "ribbonDown1",    bgRibbonDown1Position );
        background.moveElement( "ribbonDown2",    bgRibbonDown2Position );

        // Update our flying things 
        for( uint i = 0; i < flyingImagesX.length(); ++i ) {

            AHGUI::Image@ flyingImage = cast<AHGUI::Image>(centerContainer.findElement("flyingImage"+i));

            // Rotate the image just for fun (you must being having lots of fun by now)
            flyingImage.setRotation( flyingImage.getRotation() + float(i) );

            // Update our x values
            flyingImagesX[i] += (flyingImagesDir[i] * (i+1));

            // Make sure we stay *roughly* in bounds 
            if( flyingImagesX[i] > 950 ) {
                flyingImagesDir[i] = - 1;
            }

            if( flyingImagesX[i] < -250 ) {
                flyingImagesDir[i] = 1;
            }

            centerContainer.moveElement( "flyingImage" + i, ivec2(flyingImagesX[i],int(150*i)) );
        }

        // Finally move our text
        flyingTextY += flyingTextDir * 3;
        if( flyingTextY < -100 ) {
            flyingTextDir = 1;
        }
        if( flyingTextY > 1000 ) {
            flyingTextDir = -1;
        }

        centerContainer.moveElement( "flyingText", ivec2(170, flyingTextY ) );

        // Update the GUI 
        AHGUI::GUI::update();

    }

    /*******************************************************************************************/
    /**
    * @brief  Render the gui
    * 
    */
    void render() {
        // Update the GUI 
        AHGUI::GUI::render();
    }

}



/*******
 *  
 * END NEW FEATURE DISPLAY (Delete this eventually)
 *
 */



ArenaGUI arenaGUI;
// Comment out the above and uncomment to enable the new feature demo
//NewFeaturesExampleGUI exampleGUI;

bool HasFocus(){    
    return false;
}

void Initialize( ) {
    PlaySong("sub-arena-loop");
    PlaySegment("menu");
}

void Dispose() {
    
}

bool CanGoBack()
{
    return true;
}

void Update(){
    arenaGUI.update();
    // Comment out the above and uncomment to enable the new feature demo
    //exampleGUI.update();
}

int counter = 0;
int offset = 0;

void DrawGUI(){
    arenaGUI.render();
    // Comment out the above and uncomment to enable the new feature demo
    //exampleGUI.render();
}

void Draw(){
}

void Init(string str){
}
