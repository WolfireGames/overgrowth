//-----------------------------------------------------------------------------
//           Name: multiplayer_menu_connecting.as
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

#include "save/general.as"
#include "menu_common.as"
#include "music_load.as"

MusicLoad ml("Data/Music/menu.xml");

IMGUI@ imGUI;

bool back_event_queued = false;
float dot_counter = 0;
IMText@ connecting_text;

bool HasFocus() {
    return false;
}

void Initialize() {
    @imGUI = CreateIMGUI();

    // We're going to want a 100 'gui space' pixel header/footer
    imGUI.setHeaderHeight(200);
    imGUI.setFooterHeight(200);

    imGUI.setFooterPanels(200.0f, 1400.0f);
    imGUI.setup();

    BuildUI();
    setBackGround();
}

void BuildUI() {
    IMDivider left_panel("left_panel", DOVertical);
    left_panel.setAlignment(CALeft, CACenter);
    left_panel.append(IMSpacer(DOVertical, 100));

    IMDivider horizontal_buttons_holder(DOHorizontal);
    horizontal_buttons_holder.append(IMSpacer(DOHorizontal, 75));

    IMDivider buttons_holder("buttons_holder", DOVertical);
    buttons_holder.append(IMSpacer(DOHorizontal, 200));
    buttons_holder.setAlignment(CACenter, CACenter);
    horizontal_buttons_holder.append(buttons_holder);
    left_panel.append(horizontal_buttons_holder);


    @connecting_text = IMText("Connecting...", button_font);
    buttons_holder.append(connecting_text);

    // TODO This relies on being able to close a pending connection in the multiplayer system, which is not currently supported. Once it is, commenting this out should be all that is needed to cancel out of this menu
    // AddButton("Cancel", buttons_holder, exit_icon);

    IMDivider mainDiv( "mainDiv", DOHorizontal );
    mainDiv.append(left_panel);
    imGUI.getMain().setElement( @mainDiv );
}

void Dispose() {
    imGUI.clear();
}

bool CanGoBack() {
    return back_event_queued;
}

void Update() {
    UpdateController();
    UpdateKeyboardMouse();

    // Animate connecting text
    UpdateConnectingText();

    // This is a weak check for connection failures.
    // We expect to either move out of this menu on success, or fail connecting with a popup message
    // If the popup doesn't show up for whatever reason, we might end up with a softlock
    if(HasPopup()) {
        back_event_queued = true;
    } else if(back_event_queued) {
        this_ui.SendCallback("back");
    }

    // process any messages produced from the update
    while(imGUI.getMessageQueueSize() > 0) {
        IMMessage@ message = imGUI.getNextMessage();

        if(message.name == "Cancel") {
            Online_Close(); // TODO This is currently not supported by the multiplayer system
            back_event_queued = true;
        }
    }

    // Do the general GUI updating
    imGUI.update();
}

void UpdateConnectingText() {
    dot_counter += time_step * 2;
    uint count = uint(dot_counter % 3 + 1);

    string dots = "";
    for(uint i = 0; i < count; i++) {
        dots += ".";
    }
    connecting_text.setText("Connecting" + dots);
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
