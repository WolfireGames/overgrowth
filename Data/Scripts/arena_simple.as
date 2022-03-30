//-----------------------------------------------------------------------------
//           Name: arena_simple.as
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

#include "ui_tools.as"
#include "arena_meta_persistence.as"
#include "music_load.as"

AHGUI::FontSetup titleFont("edosz", 125, HexColor("#fff"));
AHGUI::FontSetup labelFont("Inconsolata", 80 , HexColor("#fff"));
AHGUI::FontSetup backFont("edosz", 60 , HexColor("#fff"));


int title_spacing = 150;
int menu_item_spacing = 40;

MusicLoad ml("Data/Music/menu.xml");

AHGUI::MouseOverPulseColor buttonHover(
                                        HexColor("#ffde00"),
                                        HexColor("#ffe956"), .25 );

class SimpleArenaGUI : AHGUI::GUI {
    // fancy ribbon background coordinates
    ivec2 fgUpper1Position;
    ivec2 fgUpper2Position;
    ivec2 fgLower1Position;
    ivec2 fgLower2Position;
    ivec2 bgRibbonUp1Position;
    ivec2 bgRibbonUp2Position;
    ivec2 bgRibbonDown1Position;
    ivec2 bgRibbonDown2Position;
    
    SimpleArenaGUI()
    {
        super();
        Init();
    }

    void Init()
    {

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

        vec4 fgColor( 0.7,0.7,0.7,0.7 );

        // Make a new image for half the upper image
        AHGUI::Image fgImageUpper1("Textures/ui/challenge_mode/red_gradient_border_c.tga");
        fgImageUpper1.setSize(2560, AHGUI::screenMetrics.GUISpaceY/2);
        fgImageUpper1.setColor( fgColor );
        // use only the top half(ish) of the image
        fgImageUpper1.setImageOffset( ivec2(0,0), ivec2(1024, 600) );
        // flip it upside down 
        fgImageUpper1.setRotation( 180 );
        // Call it gradientUpper1 
        foreground.addFloatingElement( fgImageUpper1, "gradientUpper1", fgUpper1Position, 2 );

        // repeat for a second image so we can scroll unbrokenly  
        AHGUI::Image fgImageUpper2("Textures/ui/challenge_mode/red_gradient_border_c.tga");
        fgImageUpper2.setSize(2560, AHGUI::screenMetrics.GUISpaceY/2);
        fgImageUpper2.setColor( fgColor );
        fgImageUpper2.setImageOffset( ivec2(0,0), ivec2(1024, 600) );
        fgImageUpper2.setRotation( 180 );
        foreground.addFloatingElement( fgImageUpper2, "gradientUpper2", fgUpper2Position, 2 );        

        // repeat again for the bottom image(s) (not flipped this time)
        AHGUI::Image bgImageLower1("Textures/ui/challenge_mode/red_gradient_border_c.tga");
        bgImageLower1.setSize(2560, AHGUI::screenMetrics.GUISpaceY/2);
        bgImageLower1.setColor( fgColor );
        bgImageLower1.setImageOffset( ivec2(0,0), ivec2(1024, 600) );
        foreground.addFloatingElement( bgImageLower1, "gradientLower1", fgLower1Position, 2 ); 

        AHGUI::Image fgImageLower2("Textures/ui/challenge_mode/red_gradient_border_c.tga");
        fgImageLower2.setSize(2560, AHGUI::screenMetrics.GUISpaceY/2);
        fgImageLower2.setColor( fgColor );
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


        AHGUI::Divider@ header = root.addDivider( DDTop, DOHorizontal, ivec2( 2560, 200 ) );
        header.setName("headerdiv");

        AHGUI::Divider@ mainpane = root.addDivider( DDTop,
                                                    DOVertical,
                                                    ivec2( 2560, 1040 ) );
        mainpane.addSpacer(title_spacing,DDTop);

        AHGUI::Text titleText = AHGUI::Text("Select Arena", titleFont);
        mainpane.addElement(titleText,DDTop);

        mainpane.addSpacer(title_spacing,DDTop);

        {
            AHGUI::Text buttonText = AHGUI::Text("Waterfall Cave", labelFont);
            buttonText.addMouseOverBehavior( buttonHover );
            buttonText.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("waterfall_arena.xml") );
            mainpane.addElement(buttonText, DDTop);

            mainpane.addSpacer( menu_item_spacing, DDTop ) ;
        }

        {
            AHGUI::Text buttonText = AHGUI::Text("Magma Arena", labelFont);
            buttonText.addMouseOverBehavior( buttonHover );
            buttonText.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("Magma_Arena.xml") );
            mainpane.addElement(buttonText, DDTop);

            mainpane.addSpacer( menu_item_spacing, DDTop ) ;
        }

        {
            AHGUI::Text buttonText = AHGUI::Text("Rock Cave", labelFont);
            buttonText.addMouseOverBehavior( buttonHover );
            buttonText.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("Cave_Arena.xml") );
            mainpane.addElement(buttonText, DDTop);

            mainpane.addSpacer( menu_item_spacing, DDTop ) ;
        }

        {
            AHGUI::Text buttonText = AHGUI::Text("Stucco Courtyard", labelFont);
            buttonText.addMouseOverBehavior( buttonHover );
            buttonText.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("stucco_courtyard_arena.xml") );
            mainpane.addElement(buttonText, DDTop);

            mainpane.addSpacer( menu_item_spacing, DDTop ) ;
        }

        AHGUI::Divider@ footer = root.addDivider( DDBottom, DOHorizontal, ivec2( 2560, 200 ) );
        footer.setName("footerdiv");

        footer.addSpacer( 175, DDLeft );
        footer.addSpacer( 175, DDRight );


        array<string> sub_names = {"backimage", "backtext"};
        AHGUI::MouseOverPulseColorSubElements buttonHoverSubElements( 
                                        HexColor("#ffde00"), 
                                        HexColor("#ffe956"), .25, sub_names );

        AHGUI::Divider@ backDivider = footer.addDivider( DDLeft, DOHorizontal, ivec2( 200, UNDEFINEDSIZEI ) );

        backDivider.addLeftMouseClickBehavior( AHGUI::FixedMessageOnClick("back") );
        backDivider.addMouseOverBehavior( buttonHoverSubElements, "mouseover" );
        backDivider.setName("backdivider");

        AHGUI::Image backImage("Textures/ui/arena_mode/left_arrow.png");
        backImage.scaleToSizeX(75);
        backImage.addUpdateBehavior( AHGUI::FadeIn( 1000, @inSine ) );
        backImage.setName("backimage");

        backDivider.addElement( backImage, DDLeft);  
        backDivider.addSpacer(30, DDLeft);

        AHGUI::Text backText( "Back", backFont );
        backText.addUpdateBehavior( AHGUI::FadeIn( 1000, @inSine ) );
        backText.setName("backtext");
        
        backDivider.addElement( backText, DDLeft );
        



    }

    void processMessage( AHGUI::Message@ message )
    {
        
        if( message.name == "back" )
        {
            global_data.WritePersistentInfo( false );
            global_data.clearSessionProfile();
            global_data.clearArenaSession();

            this_ui.SendCallback( "back" );
        }
        else 
        {
            // Hack for this simplified arena menu
            global_data.clearArenaSession();
            this_ui.SendCallback( "arenas/" + message.name );
        }
    }

    void update()
    {
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

        // Update the GUI 
        AHGUI::GUI::update();
    }

    void render() {
        // hud.Draw();

        AHGUI::GUI::render();
    }

}

SimpleArenaGUI simpleArenaGUI;

bool HasFocus() {
    return false;
}

void Initialize() {
    PlaySong("menu-lugaru");
}

void Dispose() {
}

bool CanGoBack() {
    return true;
}

void Update() {

    simpleArenaGUI.update();
}

void DrawGUI() {
    simpleArenaGUI.render();
}

void Draw() {
}

void Init(string str) {
}

void StartMainMenu() {

}
