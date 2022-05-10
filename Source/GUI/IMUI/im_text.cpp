//-----------------------------------------------------------------------------
//           Name: im_text.cpp
//      Developer: Wolfire Games LLC
//    Description: Text element class for creating adhoc GUIs as part of the
//                 UI tools
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
#include "im_text.h"

#include <GUI/IMUI/imgui.h>

/*******************************************************************************************/
/**
 * @brief  Constructor
 *
 */
IMText::IMText() : fontObjDirty(false),
                   fontObjInit(false) {
    IMrefCountTracker.addRefCountObject(getElementTypeName());
}

/*******************************************************************************************/
/**
 * @brief  Constructor
 *
 * @param _name Name for this object (incumbent on the programmer to make sure they're unique)
 *
 */
IMText::IMText(std::string const& _name) : IMElement(_name) {
    IMrefCountTracker.addRefCountObject(getElementTypeName());
}

/*******************************************************************************************/
/**
 * @brief  Gets the name of the type of this element — for autonaming and debugging
 *
 * @returns name of the element type as a string
 *
 */
std::string IMText::getElementTypeName() {
    return "Text";
}

/*******************************************************************************************/
/**
 * @brief  Constructor
 *
 * @param _text String for the text
 * @param _fontName name of the font (assumed to be in Data/Fonts)
 * @param _fontSize height of the font
 * @param _color vec4 color rgba
 *
 */
IMText::IMText(std::string const& _text, std::string const& _fontName, int _fontSize, vec4 _color) {
    IMrefCountTracker.addRefCountObject(getElementTypeName());

    setText(_text);
    setFontByName(_fontName, _fontSize);
    setColor(_color);
}

/*******************************************************************************************/
/**
 * @brief  Constructor
 *
 * @param _text String for the text
 * @param _fontSetup Font parameter structure
 *
 */
IMText::IMText(std::string const& _text, FontSetup _fontSetup) {
    IMrefCountTracker.addRefCountObject(getElementTypeName());

    setText(_text);
    setFont(_fontSetup);
}

/*******************************************************************************************/
/**
 * @brief  Copy constructor
 *
 */
IMText::IMText(IMText const& other) {
    IMrefCountTracker.addRefCountObject(getElementTypeName());

    fontObjDirty = other.fontObjDirty;
    fontObjInit = other.fontObjInit;
    fontSetup = other.fontSetup;
    text = other.text;
    screenFontSize = other.screenFontSize;
    screenSize = other.screenSize;
}

/*******************************************************************************************/
/**
 * @brief  Derives the various metrics for this text element
 *
 */
void IMText::updateEngineTextObject() {
    // only bother if we have text, an owner and we haven't done it yet
    if (text != "" && owner != NULL && fontObjDirty) {
        // update the actual screen resize
        screenFontSize = (int)(screenMetrics.GUItoScreenYScale * float(fontSetup.size));

        // get a new text object
        int flags = 0;
        if (fontSetup.fontName == "edosz") {
            flags = TextAtlas::kSmallLowercase;
        }
        imuiText = owner->IMGUI_IMUIContext->makeText(std::string("Data/Fonts/" + fontSetup.fontName + ".ttf"),
                                                      screenFontSize, flags);
        fontObjInit = true;

        // update its attributes
        imuiText.setText(text);

        if (fontSetup.shadowed) {
            imuiText.setRenderFlags(TextAtlasRenderer::kTextShadow);
        }

        imuiText.setRotation(fontSetup.rotation);

        // ask the engine for the text dimensions
        vec2 boundingBox = imuiText.getBoundingBoxDimensions();

        screenSize.x() = boundingBox.x() + 0.5f;
        screenSize.y() = boundingBox.y() + 0.5f;

        vec2 newSize = vec2((screenSize.x() / screenMetrics.GUItoScreenXScale) + 0.5f,
                            (screenSize.y() / screenMetrics.GUItoScreenYScale) + 0.5f);

        setSize(newSize);

        fontObjDirty = false;
        onRelayout();
    }
}

/*******************************************************************************************/
/**
 * @brief  When a resize, move, etc has happened do whatever is necessary
 *
 */
void IMText::doRelayout() {
    if (fontObjDirty) {
        updateEngineTextObject();
    }
}

/*******************************************************************************************/
/**
 * @brief  Do whatever is necessary when the resolution changes
 *
 */
void IMText::doScreenResize() {
    screenFontSize = int(screenMetrics.GUItoScreenYScale * float(fontSetup.size));
    fontObjDirty = true;
}

/*******************************************************************************************/
/**
 * @brief  Sets the font attributes
 *
 * @param _fontSetup font description object
 *
 */
void IMText::setFont(FontSetup _fontSetup) {
    fontSetup = _fontSetup;
    setColor(fontSetup.color);
    fontObjDirty = true;
}

/*******************************************************************************************/
/**
 * @brief  Sets the font attributes
 *
 * @param _fontName name of the font (assumed to be in Data/Fonts)
 * @param _fontSize height of the font
 *
 */
void IMText::setFontByName(std::string const& _fontName, int _fontSize) {
    fontSetup.fontName = _fontName;
    fontSetup.size = _fontSize;
    fontObjDirty = true;
}

/*******************************************************************************************/
/**
 * @brief  Sets the actual text
 *
 * @param _text String for the text
 *
 */
void IMText::setText(std::string const& _text) {
    text = _text;
    fontObjDirty = true;
}

/*******************************************************************************************/
/**
 * @brief  Gets the current text
 *
 * @returns String for the text
 *
 */
std::string IMText::getText() {
    return text;
}

/*******************************************************************************************/
/**
 * @brief  Sets the text to be shadowed
 *
 * @param shadow true (default) if the text should have a shadow, false otherwise
 *
 */
void IMText::setShadowed(bool shouldShadow) {
    fontSetup.shadowed = shouldShadow;
    fontObjDirty = true;
}

/*******************************************************************************************/
/**
 * @brief Sets the rotation for this image
 *
 * @param Rotation (in degrees)
 *
 */
void IMText::setRotation(float _rotation) {
    fontSetup.rotation = _rotation;
    fontObjDirty = true;
}

/*******************************************************************************************/
/**
 * @brief Gets the rotation for this text
 *
 * @returns current rotation (in degrees)
 *
 */
float IMText::getRotation() {
    return fontSetup.rotation;
}

/*******************************************************************************************/
/**
 * @brief  Updates the element
 *
 * @param delta Number of millisecond elapsed since last update
 * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
 * @param guistate The state of the GUI at this update
 *
 */
void IMText::update(uint64_t delta, vec2 drawOffset, GUIState& guistate) {
    IMElement::update(delta, drawOffset, guistate);

    // update the text object, if any
    updateEngineTextObject();
}

/*******************************************************************************************/
/**
 * @brief  Rather counter-intuitively, this draws this object on the screen
 *
 * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
 * @param clipPos pixel location of upper lefthand corner of clipping region
 * @param clipSize size of clipping region
 *
 */
void IMText::render(vec2 drawOffset, vec2 currentClipPos, vec2 currentClipSize) {
    // Make sure we're supposed draw (and have something to draw)
    if (show && text != "" && fontObjInit) {
        vec2 GUIRenderPos = drawOffset + vec2(paddingL, paddingU) + drawDisplacement;

        vec2 screenRenderPos = screenMetrics.GUIToScreen(GUIRenderPos);

        imuiText.setPosition(vec3(screenRenderPos.x(), screenRenderPos.y(), (float)getZOrdering()));

        if (isColorEffected) {
            imuiText.setColor(effectColor);
        } else {
            imuiText.setColor(color);
        }

        if (shouldClip && currentClipSize.x() != UNDEFINEDSIZE && currentClipSize.y() != UNDEFINEDSIZE) {
            vec2 adjustedClipPos = screenMetrics.GUIToScreen(currentClipPos + drawDisplacement);

            vec2 screenClipPos(vec2(adjustedClipPos.x(),
                                    adjustedClipPos.y()));
            vec2 screenClipSize(vec2((currentClipSize.x() * screenMetrics.GUItoScreenXScale),
                                     (currentClipSize.y() * screenMetrics.GUItoScreenYScale)));

            imuiText.setClipping(screenClipPos, screenClipSize);
        }

        if (fontSetup.shadowed) {
            imuiText.setRenderFlags(TextAtlasRenderer::kTextShadow);
        }

        if (owner != NULL) {
            owner->IMGUI_IMUIContext->queueText(imuiText);
        }
    }

    // Call the superclass to make sure any element specific rendering is done
    IMElement::render(drawOffset, currentClipPos, currentClipSize);
}

/*******************************************************************************************/
/**
 * @brief  Remove all referenced object without releaseing references
 *
 */
void IMText::clense() {
}

IMText::~IMText() {
    IMrefCountTracker.removeRefCountObject(getElementTypeName());
}
