//-----------------------------------------------------------------------------
//           Name: imgui.cpp
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
#include "imgui.h"

#include <Internal/timer.h>

#include <Graphics/hudimage.h>

#include <map>
#include <vector>
#include <string>

/*******************************************************************************************/
/**
 * @brief Constructor
 *
 * @param mainOrientation The orientation of the container
 *
 */
int guicount = 0;

IMGUI::IMGUI() : elementCount(1),
                 header(),
                 footer(),
                 main(),
                 mainOffset(0.0),
                 footerOffset(0.0),
                 headerHeight(0.0),
                 footerHeight(0.0),
                 mainHeight(0.0),
                 headerPanelGap(0.0),
                 footerPanelGap(0.0),
                 mainPanelGap(0.0),
                 pendingFGLayers(1),
                 pendingBGLayers(1),
                 pendingHeaderHeight(0.0f),
                 pendingFooterHeight(0.0f),
                 refCount(1) {
    IMrefCountTracker.addRefCountObject("IMGUI");
    guicount++;

    guiid = guicount;

    screenSize = screenMetrics.getMetrics();

    lastUpdateTime = 0;

    needsRelayout = false;
    showGuides = false;

    IMGUI_IMUIContext = new IMUIContext;
    IMGUI_IMUIContext->Init();

    imevents.RegisterListener(this);
}
/*
IMGUI::IMGUI( IMGUI const& other ) {

    IMrefCountTracker.addRefCountObject( "IMGUI" );

    lastUpdateTime = other.lastUpdateTime;
    elementCount = other.elementCount;
    needsRelayout = other.needsRelayout;
    reportedErrors = other.reportedErrors;
    showGuides = other.showGuides;
    mousePosition = other.mousePosition;
    leftMouseState = other.leftMouseState;
    messageQueue = other.messageQueue;
    backgrounds = other.backgrounds;
    foregrounds = other.foregrounds;
    header = other.header;
    footer = other.footer;
    main = other.main;
    mainOffset = other.mainOffset;
    footerOffset = other.footerOffset;
    headerHeight = other.headerHeight;
    footerHeight = other.footerHeight;
    mainHeight = other.mainHeight;
    headerPanelWidths = other.headerPanelWidths;
    footerPanelWidths = other.footerPanelWidths;
    mainPanelWidths = other.mainPanelWidths;
    headerPanelGap = other.headerPanelGap;
    footerPanelGap = other.footerPanelGap;
    mainPanelGap = other.mainPanelGap;
    pendingFGLayers = other.pendingFGLayers;
    pendingBGLayers = other.pendingBGLayers;
    pendingHeaderHeight = other.pendingHeaderHeight;
    pendingFooterHeight = other.pendingFooterHeight;
    pendingHeaderPanels = other.pendingHeaderPanels;
    pendingMainPanels = other.pendingMainPanels;
    pendingFooterPanels = other.pendingFooterPanels;
    screenSize = other.screenSize;
    refCount = other.refCount;

    IMGUI_IMUIContext = new IMUIContext;
    IMGUI_IMUIContext->Init();

    imevents.RegisterListener(this);
}
*/

void IMGUI::AddRef() {
    // Increase the reference counter
    refCount++;
}

void IMGUI::Release() {
    // Decrease ref count and delete if it reaches 0
    if (--refCount == 0) {
        delete this;
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
void IMGUI::setBackgroundLayers(unsigned int numLayers) {
    pendingBGLayers = numLayers;
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
void IMGUI::setForegroundLayers(unsigned int numLayers) {
    pendingFGLayers = numLayers;
}

/*******************************************************************************************/
/**
 * @brief  Set the size of the header (0 for none)
 *
 *  NOTE: This will take effect when setup is called next
 *
 * @param _headerSize size in GUI space
 *
 */
void IMGUI::setHeaderHeight(float _headerSize) {
    pendingHeaderHeight = _headerSize;
}

/*******************************************************************************************/
/**
 * @brief  Set the size of the footer (0 for none)
 *
 *  NOTE: This will take effect when setup is called next
 *
 * @param _footerSize size in GUI space
 *
 */
void IMGUI::setFooterHeight(float _footerSize) {
    pendingFooterHeight = _footerSize;
}

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
void IMGUI::setHeaderPanels(float first, float second, float third) {
    if (first > 0.0) {
        pendingHeaderPanels.push_back(first);
    }

    if (second > 0.0) {
        pendingHeaderPanels.push_back(second);
    }

    if (third > 0.0) {
        pendingHeaderPanels.push_back(third);
    }
}

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
void IMGUI::setMainPanels(float first, float second, float third) {
    if (first > 0.0) {
        pendingMainPanels.push_back(first);
    }

    if (second > 0.0) {
        pendingMainPanels.push_back(second);
    }

    if (third > 0.0) {
        pendingMainPanels.push_back(third);
    }
}

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
void IMGUI::setFooterPanels(float first, float second, float third) {
    if (first > 0.0) {
        pendingFooterPanels.push_back(first);
    }

    if (second > 0.0) {
        pendingFooterPanels.push_back(second);
    }

    if (third > 0.0) {
        pendingFooterPanels.push_back(first);
    }
}

/*******************************************************************************************/
/**
 * @brief  Sets up main layer with header, footer and primary panel (clears any existing)
 *
 * @param _headerSize y size of the header (0 for none)
 * @param _footerSize y size of the footer (0 for none)
 *
 */
void IMGUI::setup() {
    for (auto& it : header) {
        it->Release();
    }

    for (auto& it : footer) {
        it->Release();
    }

    for (auto& it : main) {
        it->Release();
    }

    header.clear();
    footer.clear();
    main.clear();

    // Make sure we at least some values
    if (pendingMainPanels.size() == 0) {
        pendingMainPanels.push_back(screenMetrics.mainSize.x());
    }

    if (pendingHeaderPanels.size() == 0) {
        pendingHeaderPanels.push_back(screenMetrics.mainSize.x());
    }

    if (pendingFooterPanels.size() == 0) {
        pendingFooterPanels.push_back(screenMetrics.mainSize.x());
    }

    // Sanity check
    if (pendingHeaderHeight + pendingFooterHeight > screenMetrics.mainSize.y()) {
        IMDisplayError("GUI Error", "Header/footer too large");
    }

    const std::string numbering[] = {std::string("0"), std::string("1"), std::string("2")};

    // Now build the panels
    if (pendingHeaderHeight > 0) {
        headerHeight = pendingHeaderHeight;

        float totalWidth = 0;

        // Go through the list of pending panels, create them and total their widths
        int panelCount = 0;
        for (float& pendingHeaderPanel : pendingHeaderPanels) {
            IMContainer* panel = new IMContainer("header" + numbering[panelCount],
                                                 SizePolicy(pendingHeaderPanel),
                                                 SizePolicy(headerHeight));
            panel->setOwnerParent(this, NULL);
            header.push_back(panel);

            totalWidth += pendingHeaderPanel;
        }

        // Sanity check
        if (totalWidth > screenMetrics.mainSize.x()) {
            IMDisplayError("GUI Error", "Header panels too wide");
        }

        // determine the gap size
        if (header.size() == 1 || header.size() == 3) {
            headerPanelGap = (screenMetrics.mainSize.x() - totalWidth) / 2;
        } else {
            headerPanelGap = (screenMetrics.mainSize.x() - totalWidth);
        }
    }

    if (pendingFooterHeight > 0) {
        footerHeight = pendingFooterHeight;

        float totalWidth = 0;

        // Go through the list of pending panels, create them and total their widths
        int panelCount = 0;
        for (float& pendingFooterPanel : pendingFooterPanels) {
            IMContainer* panel = new IMContainer("footer" + numbering[panelCount],
                                                 SizePolicy(pendingFooterPanel),
                                                 SizePolicy(footerHeight));
            panel->setOwnerParent(this, NULL);

            footer.push_back(panel);

            totalWidth += pendingFooterPanel;
        }

        // Sanity check
        if (totalWidth > screenMetrics.mainSize.x()) {
            IMDisplayError("GUI Error", "Footer panels too wide");
        }

        // determine the gap size
        if (footer.size() == 1 || main.size() == 3) {
            footerPanelGap = (screenMetrics.mainSize.x() - totalWidth) / 2;
        } else {
            footerPanelGap = (screenMetrics.mainSize.x() - totalWidth);
        }
    }

    mainHeight = screenMetrics.mainSize.y() - (headerHeight + footerHeight);

    {
        float totalWidth = 0;

        // Go through the list of pending panels, create them and total their widths
        int panelCount = 0;
        for (float& pendingMainPanel : pendingMainPanels) {
            IMContainer* panel = new IMContainer("main" + numbering[panelCount],
                                                 SizePolicy(pendingMainPanel),
                                                 SizePolicy(mainHeight));
            panel->setOwnerParent(this, NULL);

            main.push_back(panel);

            totalWidth += pendingMainPanel;
        }

        // Sanity check
        if (totalWidth > screenMetrics.mainSize.x()) {
            IMDisplayError("GUI Error", "Main panels too wide");
        }

        // determine the gap size for rendering
        if (main.size() == 1 || main.size() == 3) {
            mainPanelGap = (screenMetrics.mainSize.x() - totalWidth) / 2;
        } else {
            mainPanelGap = (screenMetrics.mainSize.x() - totalWidth);
        }
    }

    // setup background layers
    for (unsigned int i = 0; i < pendingFGLayers; ++i) {
        IMContainer* newContainer = new IMContainer(SizePolicy(screenMetrics.GUISpace.x()),
                                                    SizePolicy(screenMetrics.GUISpace.y()));

        newContainer->setOwnerParent(this, NULL);

        foregrounds.push_back(newContainer);
    }

    for (unsigned int i = 0; i < pendingBGLayers; ++i) {
        IMContainer* newContainer = new IMContainer(SizePolicy(screenMetrics.GUISpace.x()),
                                                    SizePolicy(screenMetrics.GUISpace.y()));

        newContainer->setOwnerParent(this, NULL);

        backgrounds.push_back(newContainer);
    }

    derivePanelOffsets();

    doRelayout();

    pendingFGLayers = 1;
    pendingBGLayers = 1;
    pendingHeaderHeight = 0.0f;
    pendingFooterHeight = 0.0f;
    pendingHeaderPanels.clear();
    pendingFooterPanels.clear();
    pendingMainPanels.clear();
}

/*******************************************************************************************/
/**
 * @brief  Figure out where we’re going to render the main panels - used internally
 *
 */
void IMGUI::derivePanelOffsets() {
    float headerEnd = -1;

    if (header.size() != 0) {
        headerEnd = headerHeight - 1;
    }

    footerOffset = screenMetrics.GUISpace.y();

    if (footer.size() != 0) {
        footerOffset = screenMetrics.GUISpace.y() - footerHeight;
    }

    if (main.size() != 0) {
        // see if we have any space
        if (headerHeight + footerHeight + mainHeight < screenMetrics.GUISpace.y()) {
            // first center the main panel
            mainOffset = (screenMetrics.GUISpace.y() / 2.0f) - (mainHeight / 2.0f);

            // now check if we're going over our bounds
            if (mainOffset <= headerEnd) {
                mainOffset += (headerEnd - mainOffset) + 1;
            }

            float mainEnd = mainOffset + mainHeight - 1;

            if (mainEnd >= footerOffset) {
                // we can do these two checks independently as we know there's enough room for all three
                mainOffset -= (mainEnd - footerOffset) + 1;
            }
        } else {
            mainOffset = headerEnd + 1;
        }
    }
}

/*******************************************************************************************/
/**
 * @brief  Clear the GUI - you probably want resetMainLayer though
 *
 * @param mainOrientation The orientation of the container
 *
 */
void IMGUI::clear() {
    std::vector<IMContainer*> containerList;

    containerList.insert(containerList.end(), backgrounds.begin(), backgrounds.end());
    containerList.insert(containerList.end(), foregrounds.begin(), foregrounds.end());
    containerList.insert(containerList.end(), header.begin(), header.end());
    containerList.insert(containerList.end(), footer.begin(), footer.end());
    containerList.insert(containerList.end(), main.begin(), main.end());

    backgrounds.clear();
    foregrounds.clear();
    header.clear();
    footer.clear();
    main.clear();

    for (int i = int(containerList.size()) - 1; i >= 0; --i) {
        containerList[i]->Release();
    }

    for (auto& i : messageQueue) {
        i->Release();
    }
    messageQueue.clear();
}

/*******************************************************************************************/
/**
 * @brief  Turns on (or off) the visual guides
 *
 * @param setting True to turn on guides, false otherwise
 *
 */
void IMGUI::setGuides(bool setting) {
    showGuides = setting;
}

/*******************************************************************************************/
/**
 * @brief  When this element is resized, moved, etc propagate this signal upwards
 *
 */
void IMGUI::onRelayout() {
    // Record this
    needsRelayout = true;
}

/*******************************************************************************************/
/**
 * @brief  Records all the errors reported by child elements
 *
 * @param newError Newly reported error string
 *
 */
void IMGUI::reportError(std::string const& newError) {
    reportedErrors += newError + "\n";
}

/*******************************************************************************************/
/**
 * @brief  Receives a message - used internally
 *
 * Remember to increase the message's ref count if used internally!
 *
 * @param message The message in question
 *
 */
void IMGUI::receiveMessage(IMMessage* message) {
    IMMessage* messageCopy = new IMMessage(*message);
    messageQueue.push_back(messageCopy);
    message->Release();
}

/*******************************************************************************************/
/**
 * @brief  Gets the size of the waiting message queue
 *
 * @returns size of the queue (as an unsigned integer)
 *
 */
unsigned int IMGUI::getMessageQueueSize() {
    return (unsigned int)messageQueue.size();
}

/*******************************************************************************************/
/**
 * @brief  Gets the next available message in the queue (NULL if none)
 *
 * @returns A copy of the message (NULL if none)
 *
 */
IMMessage* IMGUI::getNextMessage() {
    // Again, I'm aware this isn't very efficient, but we're only dealing with at most
    //  a half dozen of these a frame
    IMMessage* message = *messageQueue.begin();
    messageQueue.erase(messageQueue.begin());

    return message;
}

/*******************************************************************************************/
/**
 * @brief  Retrieves a reference to a specified background layer
 *
 * @param layerNum index of the background layer (starting at 0)
 *
 */
IMContainer* IMGUI::getBackgroundLayer(unsigned int layerNum) {
    if (layerNum >= backgrounds.size()) {
        IMDisplayError("GUI Error", "Unknown background layer ");
        return NULL;
    }
    IMContainer* container = backgrounds[layerNum];
    container->AddRef();
    return container;
}

/*******************************************************************************************/
/**
 * @brief  Retrieves a reference to a specified foreground layer
 *
 * @param layerNum index of the foreground layer (starting at 0)
 *
 */
IMContainer* IMGUI::getForegroundLayer(unsigned int layerNum) {
    if (layerNum >= foregrounds.size()) {
        IMDisplayError("GUI Error", "Unknown foreground layer ");
        return NULL;
    }

    IMContainer* container = foregrounds[layerNum];
    container->AddRef();
    return container;
}

/*******************************************************************************************/
/**
 * @brief  If a relayout is requested, perform it
 *
 */
void IMGUI::doRelayout() {
    int relayoutCount = 0;
    while (needsRelayout) {
        relayoutCount++;

        // clear the error string
        reportedErrors = "";

        needsRelayout = false;
        for (int layer = int(backgrounds.size()) - 1; layer >= 0; --layer) {
            backgrounds[layer]->doRelayout();
        }

        for (auto& it : header) {
            it->doRelayout();
        }

        for (auto& it : footer) {
            it->doRelayout();
        }

        for (auto& it : main) {
            it->doRelayout();
        }

        for (auto& foreground : foregrounds) {
            foreground->doRelayout();
        }

        // MJB Note: I'm aware that redoing the layout for the whole structure
        // is rather inefficient, but given that this is targeted at most a few
        // dozen elements, this should not be a problem -- can easily be optimized
        // if so
    }
}

/*******************************************************************************************/
/**
 * @brief  Updates the gui
 *
 */
extern Timer ui_timer;
void IMGUI::update() {
    // Do relayout as necessary
    doRelayout();

    // If we haven't updated yet, set the time
    if (lastUpdateTime == 0) {
        lastUpdateTime = uint64_t(ui_timer.game_time * 1000);
    }

    // Calculate the delta time
    uint64_t delta = uint64_t(ui_timer.game_time * 1000) - lastUpdateTime;

    // Update the time
    lastUpdateTime = uint64_t(ui_timer.game_time * 1000);

    // Get the input from the engine
    IMGUI_IMUIContext->UpdateControls();

    vec2 engineMousePos = IMGUI_IMUIContext->getMousePosition();

    // Translate to GUISpace
    mousePosition.x() = (engineMousePos.x() - screenMetrics.renderOffset.x()) / screenMetrics.GUItoScreenXScale;
    mousePosition.y() = (screenMetrics.getScreenHeight() - engineMousePos.y() + screenMetrics.renderOffset.y()) / screenMetrics.GUItoScreenYScale;
    leftMouseState = IMGUI_IMUIContext->getLeftMouseState();

    // Fill in our GUI structure
    guistate.mousePosition = mousePosition;
    guistate.leftMouseState = leftMouseState;
    guistate.inheritedMouseDown = false;
    guistate.inheritedMouseOver = false;
    guistate.clickHandled = false;

    // Do relayout as necessary
    doRelayout();

    vec2 drawOffset(0, 0);
    // update the backgrounds
    for (int layer = int(backgrounds.size()) - 1; layer >= 0; --layer) {
        backgrounds[layer]->update(delta, drawOffset, guistate);
    }

    if (header.size() == 1) {
        drawOffset.x() = headerPanelGap;
        header[0]->update(delta, drawOffset, guistate);
    } else if (header.size() == 2) {
        header[0]->update(delta, drawOffset, guistate);
        drawOffset.x() = header[0]->getSizeX() + headerPanelGap;
        header[1]->update(delta, drawOffset, guistate);
    } else if (header.size() == 3) {
        header[0]->update(delta, drawOffset, guistate);
        drawOffset.x() = header[0]->getSizeX() + headerPanelGap;
        header[1]->update(delta, drawOffset, guistate);
        drawOffset.x() = header[1]->getSizeX() + headerPanelGap;
        header[2]->update(delta, drawOffset, guistate);
    }

    drawOffset.x() = 0;
    drawOffset.y() = mainOffset;
    if (main.size() == 1) {
        drawOffset.x() = mainPanelGap;
        main[0]->update(delta, drawOffset, guistate);
    } else if (main.size() == 2) {
        main[0]->update(delta, drawOffset, guistate);
        drawOffset.x() = main[0]->getSizeX() + mainPanelGap;
        main[1]->update(delta, drawOffset, guistate);
    } else if (main.size() == 3) {
        main[0]->update(delta, drawOffset, guistate);
        drawOffset.x() = main[0]->getSizeX() + mainPanelGap;
        main[1]->update(delta, drawOffset, guistate);
        drawOffset.x() = main[1]->getSizeX() + mainPanelGap;
        main[2]->update(delta, drawOffset, guistate);
    }

    drawOffset.x() = 0;
    drawOffset.y() = footerOffset;
    if (footer.size() == 1) {
        drawOffset.x() = footerPanelGap;
        footer[0]->update(delta, drawOffset, guistate);
    } else if (footer.size() == 2) {
        footer[0]->update(delta, drawOffset, guistate);
        drawOffset.x() = footer[0]->getSizeX() + footerPanelGap;
        footer[1]->update(delta, drawOffset, guistate);
    } else if (footer.size() == 3) {
        footer[0]->update(delta, drawOffset, guistate);
        drawOffset.x() = footer[0]->getSizeX() + footerPanelGap;
        footer[1]->update(delta, drawOffset, guistate);
        drawOffset.x() = footer[1]->getSizeX() + footerPanelGap;
        footer[2]->update(delta, drawOffset, guistate);
    }

    drawOffset = vec2(0, 0);
    // update the backgrounds
    for (int layer = int(foregrounds.size()) - 1; layer >= 0; --layer) {
        foregrounds[layer]->update(delta, drawOffset, guistate);
    }

    // See if this triggered any relayout events
    doRelayout();

    // If we've got here and still have an error, report it
    if (reportedErrors != "") {
        IMDisplayError("GUI Error", reportedErrors);
    }
}

/*******************************************************************************************/
/**
 * @brief  Fired when the screen resizes
 *
 */
void IMGUI::doScreenResize() {
    screenMetrics.checkMetrics(screenSize);

    // All the font sizes will likely have changed
    IMGUI_IMUIContext->clearTextAtlases();

    // Move our panels around (if necessary)
    derivePanelOffsets();

    // We need to inform the elements
    for (int layer = int(backgrounds.size()) - 1; layer >= 0; --layer) {
        backgrounds[layer]->setSizePolicy(SizePolicy(screenMetrics.GUISpace.x()),
                                          SizePolicy(screenMetrics.GUISpace.y()));

        backgrounds[layer]->setSize(vec2(screenMetrics.GUISpace.x(),
                                         screenMetrics.GUISpace.y()));

        backgrounds[layer]->doScreenResize();
    }

    for (auto& it : header) {
        it->doScreenResize();
    }

    for (auto& it : footer) {
        it->doScreenResize();
    }

    for (auto& it : main) {
        it->doScreenResize();
    }

    for (auto& foreground : foregrounds) {
        // reset the size
        foreground->setSizePolicy(SizePolicy(screenMetrics.GUISpace.x()),
                                  SizePolicy(screenMetrics.GUISpace.y()));

        foreground->doScreenResize();
    }

    doRelayout();
}

/*******************************************************************************************/
/**
 * @brief  Render the GUI
 *
 */
void IMGUI::render() {
    vec2 origin(0, 0);
    // render the backgrounds
    for (int layer = int(backgrounds.size()) - 1; layer >= 0; --layer) {
        backgrounds[layer]->render(origin, origin, vec2(UNDEFINEDSIZE, UNDEFINEDSIZE));
        IMGUI_IMUIContext->render();
    }

    // render the main content
    vec2 drawOffset(0, 0);
    if (header.size() == 1) {
        drawOffset.x() = headerPanelGap;
        header[0]->render(drawOffset, drawOffset, vec2(UNDEFINEDSIZE, UNDEFINEDSIZE));
    } else if (header.size() == 2) {
        header[0]->render(drawOffset, drawOffset, vec2(UNDEFINEDSIZE, UNDEFINEDSIZE));
        drawOffset.x() = header[0]->getSizeX() + headerPanelGap;
        header[1]->render(drawOffset, drawOffset, vec2(UNDEFINEDSIZE, UNDEFINEDSIZE));
    } else if (header.size() == 3) {
        header[0]->render(drawOffset, drawOffset, vec2(UNDEFINEDSIZE, UNDEFINEDSIZE));
        drawOffset.x() = header[0]->getSizeX() + headerPanelGap;
        header[1]->render(drawOffset, drawOffset, vec2(UNDEFINEDSIZE, UNDEFINEDSIZE));
        drawOffset.x() = header[1]->getSizeX() + headerPanelGap;
        header[2]->render(drawOffset, drawOffset, vec2(UNDEFINEDSIZE, UNDEFINEDSIZE));
    }

    drawOffset.x() = 0;
    drawOffset.y() = mainOffset;

    if (main.size() == 1) {
        drawOffset.x() = mainPanelGap;
        main[0]->render(drawOffset, drawOffset, vec2(UNDEFINEDSIZE, UNDEFINEDSIZE));
    } else if (main.size() == 2) {
        main[0]->render(drawOffset, drawOffset, vec2(UNDEFINEDSIZE, UNDEFINEDSIZE));
        drawOffset.x() = main[0]->getSizeX() + mainPanelGap;
        main[1]->render(drawOffset, drawOffset, vec2(UNDEFINEDSIZE, UNDEFINEDSIZE));
    } else if (main.size() == 3) {
        main[0]->render(drawOffset, drawOffset, vec2(UNDEFINEDSIZE, UNDEFINEDSIZE));
        drawOffset.x() = main[0]->getSizeX() + mainPanelGap;
        main[1]->render(drawOffset, drawOffset, vec2(UNDEFINEDSIZE, UNDEFINEDSIZE));
        drawOffset.x() = main[1]->getSizeX() + mainPanelGap;
        main[2]->render(drawOffset, drawOffset, vec2(UNDEFINEDSIZE, UNDEFINEDSIZE));
    }

    drawOffset.x() = 0;
    drawOffset.y() = footerOffset;
    if (footer.size() == 1) {
        drawOffset.x() = footerPanelGap;
        footer[0]->render(drawOffset, drawOffset, vec2(UNDEFINEDSIZE, UNDEFINEDSIZE));
    } else if (footer.size() == 2) {
        footer[0]->render(drawOffset, drawOffset, vec2(UNDEFINEDSIZE, UNDEFINEDSIZE));
        drawOffset.x() = footer[0]->getSizeX() + footerPanelGap;
        footer[1]->render(drawOffset, drawOffset, vec2(UNDEFINEDSIZE, UNDEFINEDSIZE));
    } else if (footer.size() == 3) {
        footer[0]->render(drawOffset, drawOffset, vec2(UNDEFINEDSIZE, UNDEFINEDSIZE));
        drawOffset.x() = footer[0]->getSizeX() + footerPanelGap;
        footer[1]->render(drawOffset, drawOffset, vec2(UNDEFINEDSIZE, UNDEFINEDSIZE));
        drawOffset.x() = footer[1]->getSizeX() + footerPanelGap;
        footer[2]->render(drawOffset, drawOffset, vec2(UNDEFINEDSIZE, UNDEFINEDSIZE));
    }

    IMGUI_IMUIContext->render();

    // render the foregrounds
    for (auto& foreground : foregrounds) {
        foreground->render(origin, origin, vec2(UNDEFINEDSIZE, UNDEFINEDSIZE));
        IMGUI_IMUIContext->render();
    }

    // TODO
    //    if( showGuides ) {
    //
    //        for( int i = 2; i <= 16; i *= 2 ) {
    //
    //            // draw the vertical lines
    //            float increment = screenMetrics.getScreenWidth() / (float)i;
    //            float xpos = increment;
    //
    //            while( xpos < screenMetrics.getScreenWidth() ) {
    //
    //                HUDImage* newimage = hud.AddImage();
    //                newimage->SetImageFromPath("Data/Textures/ui/whiteblock.tga");
    //
    //                vec2 imagepos( xpos-1, 0.0f );
    //
    //                newimage->scale = 1;
    //                newimage->scale.x *= 2;
    //                newimage->scale.y *= screenMetrics.getScreenHeight();
    //
    //                newimage->position.x = imagepos.x;
    //                newimage->position.y() = screenMetrics.getScreenHeight() - imagepos.y() - (newimage.GetWidth() * newimage.scale.y() );
    //                newimage->position.z() = -100.0;// 0.1f;
    //
    //                newimage->color = vec4( 0.0, 0.2, 0.5 + (0.5/i), 0.25 );
    //
    //                xpos += increment;
    //            }
    //
    //            increment = GetScreenHeight() / i;
    //            int ypos = increment;
    //
    //            while( ypos < GetScreenHeight() ) {
    //
    //                HUDImage @newimage = hud.AddImage();
    //                newimage.SetImageFromPath("Data/Textures/ui/whiteblock.tga");
    //
    //                vec2 imagepos( 0, ypos -1 );
    //
    //                newimage.scale = 1;
    //                newimage.scale.x *= GetScreenWidth();
    //                newimage.scale.y *= 2;
    //
    //                newimage.position.x = imagepos.x;
    //                newimage.position.y = GetScreenHeight() - imagepos.y - (newimage.GetWidth() * newimage.scale.y );
    //                newimage.position.z = -100.0;// 0.1f;
    //
    //                newimage.color = vec4( 0.0, 0.2, 0.5 + (0.5/i), 0.25 );
    //
    //                ypos += increment;
    //            }
    //        }
    //    }
    //
    //    hud.Draw();
    IMGUI_IMUIContext->render();
}

/*******************************************************************************************/
/**
 * @brief  Draw a box (in *screen* coordinates) -- used internally
 *
 */
void IMGUI::drawBox(vec2 boxPos, vec2 boxSize, vec4 boxColor, int zOrder, bool shouldClip,
                    vec2 currentClipPos,
                    vec2 currentClipSize) {
    IMUIImage boxImage("Data/Textures/ui/whiteblock.tga");

    boxImage.setPosition(vec3(boxPos.x(), boxPos.y(), float(zOrder)));
    boxImage.setColor(boxColor);
    boxImage.setRenderSize(vec2(boxSize.x(), boxSize.y()));

    if (shouldClip && currentClipSize.x() != UNDEFINEDSIZE && currentClipSize.y() != UNDEFINEDSIZE) {
        vec2 screenClipPos = screenMetrics.GUIToScreen(currentClipPos);

        vec2 screenClipSize((currentClipSize.x() * screenMetrics.GUItoScreenXScale) + 0.5f,
                            (currentClipSize.y() * screenMetrics.GUItoScreenYScale) + 0.5f);

        boxImage.setClipping(vec2(screenClipPos.x(), float(screenClipPos.y())),
                             vec2(screenClipSize.x(), screenClipSize.y()));
    }

    IMGUI_IMUIContext->queueImage(boxImage);
}

/*******************************************************************************************/
/**
 * @brief  Finds an element in the gui by a given name
 *
 * @param elementName the name of the element
 *
 * @returns handle to the element (NULL if not found)
 *
 */
IMElement* IMGUI::findElement(std::string const& elementName) {
    for (auto& it : header) {
        IMElement* foundElement = it->findElement(elementName);

        if (foundElement != NULL) {
            foundElement->AddRef();
            return foundElement;
        }
    }

    for (auto& it : footer) {
        IMElement* foundElement = it->findElement(elementName);

        if (foundElement != NULL) {
            foundElement->AddRef();
            return foundElement;
        }
    }

    for (auto& it : main) {
        IMElement* foundElement = it->findElement(elementName);

        if (foundElement != NULL) {
            foundElement->AddRef();
            return foundElement;
        }
    }

    return NULL;
}

/*******************************************************************************************/
/**
 * @brief  Gets a unique name for assigning to unnamed elements (used internally)
 *
 * @returns Unique name as string
 *
 */
std::string IMGUI::getUniqueName(std::string const& type) {
    elementCount += 1;

    std::ostringstream oss;
    oss << type << elementCount;

    return oss.str();
}

/*******************************************************************************************/
/**
 * @brief  Remove all referenced object without releaseing references
 *
 */
void IMGUI::clense() {
    backgrounds.clear();
    foregrounds.clear();
    header.clear();
    footer.clear();
    main.clear();
}

/*******************************************************************************************/
/**
 * @brief Destructor
 *
 */
IMGUI::~IMGUI() {
    IMrefCountTracker.removeRefCountObject("IMGUI");
    clear();
    delete IMGUI_IMUIContext;

    imevents.DeRegisterListener(this);
    imevents.TriggerDestroyed(this);
}

void IMGUI::DestroyedIMElement(IMElement* element) {
    for (int i = backgrounds.size() - 1; i >= 0; i--) {
        if (backgrounds[i] == element) {
            backgrounds.erase(backgrounds.begin() + i);
        }
    }

    for (int i = foregrounds.size() - 1; i >= 0; i--) {
        if (foregrounds[i] == element) {
            foregrounds.erase(foregrounds.begin() + i);
        }
    }

    for (int i = header.size() - 1; i >= 0; i--) {
        if (header[i] == element) {
            header.erase(header.begin() + i);
        }
    }

    for (int i = footer.size() - 1; i >= 0; i--) {
        if (footer[i] == element) {
            LOGI << "Removing footer " << element << std::endl;
            footer.erase(footer.begin() + i);
        }
    }

    for (int i = main.size() - 1; i >= 0; i--) {
        if (main[i] == element) {
            LOGI << "Removing main " << element << std::endl;
            main.erase(main.begin() + i);
        }
    }
}

void IMGUI::DestroyedIMGUI(IMGUI* IMGUI) {
}
