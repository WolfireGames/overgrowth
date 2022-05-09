//-----------------------------------------------------------------------------
//           Name: im_element.cpp
//      Developer: Wolfire Games LLC
//    Description: Base class for all AdHoc Gui elements
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
#include "im_element.h"

#include <GUI/IMUI/imgui.h>

#include <algorithm>
#include <map>
#include <cmath>

/*******************************************************************************************/
/**
 * @brief  Constructor
 *
 * @param _name Name for this object (incumbent on the programmer to make sure they're unique)
 *
 */
IMElement::IMElement(std::string const& _name) : size(UNDEFINEDSIZE, UNDEFINEDSIZE),
                                                 defaultSize(UNDEFINEDSIZE, UNDEFINEDSIZE),
                                                 drawDisplacement(0.0f, 0.0f),
                                                 zOrdering(1),
                                                 parent(NULL),
                                                 owner(NULL),
                                                 show(true),
                                                 shouldClip(true),
                                                 color(1.0, 1.0, 1.0, 1.0),
                                                 effectColor(1.0, 1.0, 1.0, 1.0),
                                                 isColorEffected(false),
                                                 border(false),
                                                 borderSize(1),
                                                 borderColor(1.0, 1.0, 1.0, 1.0),
                                                 mouseOver(false),
                                                 mouse_clicking(false),
                                                 mouseDownForChildren(false),
                                                 mouseOverForChildren(false),
                                                 refCount(1),
                                                 numBehaviors(0),
                                                 pauseBehaviors(false),
                                                 scriptMouseOver(false) {
    name = _name;
    setPadding(0.0, 0.0, 0.0, 0.0);

    imevents.RegisterListener(this);
}

std::vector<IMElement*> IMElement::deletion_schedule;

/*******
 *
 * Angelscript memory management boilerplate
 *
 */
void IMElement::AddRef() {
    // Increase the reference counter
    refCount++;
}

void IMElement::Release() {
    refCount--;
    // Decrease ref count and delete if it reaches 0
    if (refCount == 0) {
        for (IMElement* element : IMElement::deletion_schedule) {
            if (element == this) {
                return;
            }
        }
        IMElement::deletion_schedule.push_back(this);
    } else if (refCount < 0) {
        LOGE << "Negative refcount" << std::endl;
    }
}

/*******************************************************************************************/
/**
 * @brief  Gets the name of the type of this element — for autonaming and debugging
 *
 * @returns name of the element type as a string
 *
 */
std::string IMElement::getElementTypeName() {
    return "Element";
}

/*******************************************************************************************/
/**
 * @brief  Set’s this element’s parent (and does nessesary logic)
 *
 * @param _parent New parent
 *
 */
void IMElement::setOwnerParent(IMGUI* _owner, IMElement* _parent) {
    owner = _owner;
    parent = _parent;
}

/*******************************************************************************************/
/**
 * @brief  Set the color for the element
 *
 * @param _color 4 component vector for the color
 *
 */
void IMElement::setColor(vec4 _color) {
    color = _color;
}

/*******************************************************************************************/
/**
 * @brief  Gets the current color
 *
 * If the color is effected, it'll return the effected color
 *
 * @returns 4 component vector of the color
 *
 */
vec4 IMElement::getColor() {
    if (isColorEffected) {
        return effectColor;
    } else {
        return color;
    }
}

/*******************************************************************************************/
/**
 * @brief  Gets the current color -- ignoring the effect color
 *
 * @returns 4 component vector of the color
 *
 */
vec4 IMElement::getBaseColor() {
    return color;
}

/*******************************************************************************************/
/**
 * @brief  Set the effect color for the element
 *
 * @param _color 4 component vector for the color
 *
 */
void IMElement::setEffectColor(vec4 _color) {
    isColorEffected = true;
    effectColor = _color;
}

/*******************************************************************************************/
/**
 * @brief  Gets the effect current color
 *
 * @returns 4 component vector of the color
 *
 */
vec4 IMElement::getEffectColor() {
    if (!isColorEffected) {
        return color;
    }
    return effectColor;
}

/*******************************************************************************************/
/**
 * @brief Clears any effect color (reseting to the base)
 *
 */
void IMElement::clearColorEffect() {
    isColorEffected = false;
}

/*******************************************************************************************/
/**
 * @brief  Sets the red value
 *
 * @param value Color value
 *
 */
void IMElement::setR(float value) {
    color.x() = value;
}

/*******************************************************************************************/
/**
 * @brief  Gets the red value
 *
 * @returns Color value
 *
 */
float IMElement::getR() {
    return color.x();
}

/*******************************************************************************************/
/**
 * @brief Sets the green value
 *
 * @param value Color value
 *
 */
void IMElement::setG(float value) {
    color.y() = value;
}

/*******************************************************************************************/
/**
 * @brief Gets the green value
 *
 * @returns Color value
 *
 */
float IMElement::getG() {
    return color.y();
}

/*******************************************************************************************/
/**
 * @brief Sets the blue value
 *
 * @param value Color value
 *
 */
void IMElement::setB(float value) {
    color.z() = value;
}

/*******************************************************************************************/
/**
 * @brief Gets the blue value
 *
 * @returns Color value
 *
 */
float IMElement::getB() {
    return color.y();
}

/*******************************************************************************************/
/**
 * @brief Sets the alpha value
 *
 * @param value Color value
 *
 */
void IMElement::setAlpha(float value) {
    color.a() = value;
}

/*******************************************************************************************/
/**
 * @brief Gets the alpha value
 *
 * @returns Color value
 *
 */
float IMElement::getAlpha() {
    return color.a();
}

/*******************************************************************************************/
/**
 * @brief  Sets the effect red value
 *
 * @param value Color value
 *
 */
void IMElement::setEffectR(float value) {
    if (!isColorEffected) {
        effectColor = color;
    }
    isColorEffected = true;
    effectColor.x() = value;
}

/*******************************************************************************************/
/**
 * @brief  Gets the effect red value
 *
 * @returns Color value
 *
 */
float IMElement::getEffectR() {
    if (!isColorEffected) {
        return color.x();
    } else {
        return effectColor.x();
    }
}

/*******************************************************************************************/
/**
 * @brief  Clear effect red value
 *
 */
void IMElement::clearEffectR() {
    isColorEffected = false;
    effectColor.x() = color.x();
}

/*******************************************************************************************/
/**
 * @brief Sets the effect green value
 *
 * @param value Color value
 *
 */
void IMElement::setEffectG(float value) {
    if (!isColorEffected) {
        effectColor = color;
    }
    isColorEffected = true;
    effectColor.y() = value;
}

/*******************************************************************************************/
/**
 * @brief Gets the effect green value
 *
 * @returns Color value
 *
 */
float IMElement::getEffectG() {
    if (!isColorEffected) {
        return color.y();
    } else {
        return effectColor.y();
    }
}

/*******************************************************************************************/
/**
 * @brief  Clear effect green value
 *
 */
void IMElement::clearEffectG() {
    isColorEffected = false;
    effectColor.y() = color.y();
}

/*******************************************************************************************/
/**
 * @brief Sets the blue value
 *
 * @param value Color value
 *
 */
void IMElement::setEffectB(float value) {
    if (!isColorEffected) {
        effectColor = color;
    }
    isColorEffected = true;
    effectColor.z() = value;
}

/*******************************************************************************************/
/**
 * @brief Gets the blue value
 *
 * @returns Color value
 *
 */
float IMElement::getEffectB() {
    if (!isColorEffected) {
        return color.z();
    } else {
        return effectColor.z();
    }
}

/*******************************************************************************************/
/**
 * @brief  Clear effect blue value
 *
 */
void IMElement::clearEffectB() {
    isColorEffected = false;
    effectColor.z() = color.z();
}

/*******************************************************************************************/
/**
 * @brief Sets the alpha value
 *
 * @param value Color value
 *
 */
void IMElement::setEffectAlpha(float value) {
    if (!isColorEffected) {
        effectColor = color;
    }
    isColorEffected = true;
    effectColor.a() = value;
}

/*******************************************************************************************/
/**
 * @brief Gets the alpha value
 *
 * @returns Color value
 *
 */
float IMElement::getEffectAlpha() {
    if (!isColorEffected) {
        return color.z();
    } else {
        return effectColor.z();
    }
}

/*******************************************************************************************/
/**
 * @brief  Clear effect alpha value
 *
 */
void IMElement::clearEffectAlpha() {
    isColorEffected = false;
    effectColor = color;
}

/*******************************************************************************************/
/**
 * @brief  Should this element have a border
 *
 * @param _border Show this border or not
 *
 */
void IMElement::showBorder(bool _border) {
    border = _border;
}

/*******************************************************************************************/
/**
 * @brief  Sets the border thickness
 *
 * @param thickness Thickness of the border in GUI space pixels
 *
 */
void IMElement::setBorderSize(float _borderSize) {
    borderSize = _borderSize;
}

/*******************************************************************************************/
/**
 * @brief  Set the color for the border
 *
 * @param _color 4 component vector for the color
 *
 */
void IMElement::setBorderColor(vec4 _color) {
    borderColor = _color;
}

/*******************************************************************************************/
/**
 * @brief  Gets the current border color
 *
 * @returns 4 component vector of the color
 *
 */
vec4 IMElement::getBorderColor() {
    return borderColor;
}

/*******************************************************************************************/
/**
 * @brief  Sets the border red value
 *
 * @param value Color value
 *
 */
void IMElement::setBorderR(float value) {
    borderColor.x() = value;
}

/*******************************************************************************************/
/**
 * @brief  Gets the border red value
 *
 * @returns Color value
 *
 */
float IMElement::getBorderR() {
    return borderColor.x();
}

/*******************************************************************************************/
/**
 * @brief Sets the border green value
 *
 * @param value Color value
 *
 */
void IMElement::setBorderG(float value) {
    borderColor.y() = value;
}

/*******************************************************************************************/
/**
 * @brief Gets the border green value
 *
 * @returns Color value
 *
 */
float IMElement::getBorderG() {
    return borderColor.y();
}

/*******************************************************************************************/
/**
 * @brief Sets the border blue value
 *
 * @param value Color value
 *
 */
void IMElement::setBorderB(float value) {
    borderColor.z() = value;
}

/*******************************************************************************************/
/**
 * @brief Gets the border blue value
 *
 * @returns Color value
 *
 */
float IMElement::getBorderB() {
    return borderColor.y();
}

/*******************************************************************************************/
/**
 * @brief Sets the border alpha value
 *
 * @param value Color value
 *
 */
void IMElement::setBorderAlpha(float value) {
    borderColor.a() = value;
}

/*******************************************************************************************/
/**
 * @brief Gets the border alpha value
 *
 * @returns Color value
 *
 */
float IMElement::getBorderAlpha() {
    return borderColor.a();
}

/*******************************************************************************************/
/**
 * @brief  Sets the z ordering (order of drawing, higher is drawing on top of lower)
 *
 * @param z new Z ordering value (expected to be greater then 0 and the parent container)
 *
 */
void IMElement::setZOrdering(int z) {
    zOrdering = z;
}

/*******************************************************************************************/
/**
 * @brief  Gets the z ordering (order of drawing - higher is drawing on top of lower)
 *
 * @returns current Z ordering value
 *
 */
int IMElement::getZOrdering() {
    return zOrdering;
}

/*******************************************************************************************/
/**
 * @brief  Set the z ordering of this element to be higher than the given element
 *
 * @param element Element to be below this one
 *
 */
void IMElement::renderAbove(IMElement* element) {
    setZOrdering(element->getZOrdering() + 1);
    element->Release();
}

/*******************************************************************************************/
/**
 * @brief  Set the z ordering of this element to be lower than the given element
 *
 * (note that if the element parameter has a z value within 1 of the parent container
 *  this element will be assigned to the same value, which may not look nice )
 *
 * @param element Element to be below this one
 *
 */
void IMElement::renderBelow(IMElement* element) {
    int minZ;
    // See if we're a root container
    if (parent != NULL) {
        // not a root
        minZ = parent->getZOrdering();
    } else {
        // we're a root
        minZ = 1;
    }

    // set the z ordering, respecting the derived minimum
    setZOrdering(max(minZ, element->getZOrdering() - 1));

    element->Release();
}

/*******************************************************************************************/
/**
 * @brief  Show or hide this element
 *
 * @param _show Show this element or not
 *
 */
void IMElement::setVisible(bool _show) {
    show = _show;
}

/*******************************************************************************************/
/**
 * @brief Should this element be including in the container clipping?
 *
 * @param _clip Clip this element or not
 *
 */
void IMElement::setClip(bool _clip) {
    shouldClip = _clip;
}

/*******************************************************************************************/
/**
 * @brief Draw this object on the screen
 *
 * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
 * @param clipPos pixel location of upper lefthand corner of clipping region
 * @param clipSize size of clipping region
 *
 */
void IMElement::render(vec2 drawOffset, vec2 currentClipPos, vec2 currentClipSize) {
    // see if we're visible
    if (!show) return;

    // See if we're supposed to draw a border
    if (border) {
        vec2 borderCornerUL = drawOffset + drawDisplacement + vec2(2, 2) + vec2(paddingL, paddingU);
        vec2 borderCornerLR = drawOffset + drawDisplacement + size - vec2(1, 1) + vec2(paddingR, paddingD);

        vec2 screenCornerUL = screenMetrics.GUIToScreen(borderCornerUL);
        vec2 screenCornerLR = screenMetrics.GUIToScreen(borderCornerLR);

        // figure out the thickness in screen pixels (minimum 1)
        float thickness = max(borderSize * screenMetrics.GUItoScreenXScale, 1.0f);

        // top
        owner->drawBox(screenCornerUL,
                       vec2(screenCornerLR.x() - screenCornerUL.x(), thickness),
                       borderColor,
                       zOrdering + 1,
                       shouldClip,
                       currentClipPos,
                       currentClipSize);

        // bottom
        owner->drawBox(vec2(screenCornerUL.x(), screenCornerLR.y() - thickness),
                       vec2(screenCornerLR.x() - screenCornerUL.x(), thickness),
                       borderColor,
                       zOrdering + 1,
                       shouldClip,
                       currentClipPos,
                       currentClipSize);

        // left
        owner->drawBox(vec2(screenCornerUL.x(), screenCornerUL.y() + thickness),
                       vec2(thickness, screenCornerLR.y() - screenCornerUL.y() - (2 * thickness)),
                       borderColor,
                       zOrdering + 1,
                       shouldClip,
                       currentClipPos,
                       currentClipSize);
        // right
        owner->drawBox(vec2(screenCornerLR.x() - thickness, screenCornerUL.y() + thickness),
                       vec2(thickness, screenCornerLR.y() - screenCornerUL.y() - (2 * thickness)),
                       borderColor,
                       zOrdering + 1,
                       shouldClip,
                       currentClipPos,
                       currentClipSize);
    }
}

vec2 IMElement::getScreenPosition() {
    vec2 GUIRenderPos = lastDrawOffset + vec2(paddingL, paddingU) + drawDisplacement;

    vec2 screenRenderPos = screenMetrics.GUIToScreen(GUIRenderPos);

    return vec2(screenRenderPos.x(), screenRenderPos.y());
}

/*******************************************************************************************/
/**
 * @brief  Checks to see if a point is inside this element
 *
 * @param drawOffset The upper left hand corner of where the boundary is drawn
 * @param point point in question
 *
 * @returns true if inside, false otherwise
 *
 */
bool IMElement::pointInElement(vec2 drawOffset, vec2 point) {
    vec2 UL = drawOffset + drawDisplacement;
    vec2 LR = UL + size;

    if (UL.x() <= point.x() && UL.y() <= point.y() &&
        LR.x() > point.x() && LR.y() > point.y()) {
        return true;
    } else {
        return false;
    }
}

/*******************************************************************************************/
/**
 * @brief  Add an update behavior
 *
 * @param behavior Handle to behavior in question
 * @param behaviorName name to identify the behavior
 *
 */
void IMElement::addUpdateBehavior(IMUpdateBehavior* behavior, std::string const& behaviorName) {
    std::string bName = behaviorName;

    // if they haven't given us a name, generate one
    if (bName == "") {
        std::ostringstream oss;
        oss << "behavior" << numBehaviors;
        bName = oss.str();
    } else if (updateBehaviors.find(bName) != updateBehaviors.end()) {
        removeUpdateBehavior(bName);
    }

    // add a reference as we're storing it
    behavior->AddRef();

    updateBehaviors[bName] = behavior;

    // get rid of the reference we were given
    behavior->Release();
}

/*******************************************************************************************/
/**
 * @brief  Removes a named update behavior
 *
 * @param behaviorName name to identify the behavior
 *
 * @returns true if there was a behavior to remove, false otherwise
 *
 */
bool IMElement::removeUpdateBehavior(std::string const& behaviorName) {
    // see if there is already a behavior with this name
    UBMap::iterator it = updateBehaviors.find(behaviorName);
    if (it != updateBehaviors.end()) {
        // if so clean it up if its been initialized
        IMUpdateBehavior* updater = it->second;

        if (updater->initialized) {
            updater->cleanUp(this);
        }

        // and remove it, deref it
        updater->Release();
        updateBehaviors.erase(it);

        return true;
    } else {
        // Let the caller know
        return false;
    }
}

/*******************************************************************************************/
/**
 * @brief Indicates if a behavior exists, can be used to see if its finished.
 *
 * @param behaviorName name to identify the behavior
 *
 * @returns true if there was a behavior
 *
 */
bool IMElement::hasUpdateBehavior(std::string const& behaviorName) {
    UBMap::iterator it = updateBehaviors.find(behaviorName);
    if (it != updateBehaviors.end()) {
        return true;
    } else {
        // Let the caller know
        return false;
    }
}

/*******************************************************************************************/
/**
 * @brief  Clear update behaviors
 *
 */
void IMElement::clearUpdateBehaviors() {
    // iterate through all the behaviors and clean them up
    for (auto& updateBehavior : updateBehaviors) {
        IMUpdateBehavior* updater = updateBehavior.second;
        updater->cleanUp(this);
        updater->Release();
    }

    updateBehaviors.clear();
}

/*******************************************************************************************/
/**
 * @brief  Add a mouse over behavior
 *
 * @param behavior Handle to behavior in question
 * @param behaviorName name to identify the behavior
 *
 */
void IMElement::addMouseOverBehavior(IMMouseOverBehavior* behavior, std::string const& behaviorName) {
    std::string bName = behaviorName;

    // if they haven't given us a name, generate one
    if (bName == "") {
        std::ostringstream oss;
        oss << "behavior" << numBehaviors;
        bName = oss.str();
        numBehaviors++;

    } else if (mouseOverBehaviors.find(bName) != mouseOverBehaviors.end()) {
        // Remove old behavior with this name (if any)
        removeMouseOverBehavior(bName);
    }

    // add a reference as we're storing it
    behavior->AddRef();

    mouseOverBehaviors[bName] = behavior;

    // get rid of the reference we were given
    behavior->Release();
}

/*******************************************************************************************/
/**
 * @brief  Removes a named update behavior
 *
 * @param behaviorName name to identify the behavior
 *
 * @returns true if there was a behavior to remove, false otherwise
 *
 */
bool IMElement::removeMouseOverBehavior(std::string const& behaviorName) {
    // see if there is already a behavior with this name
    MOBMap::iterator it = mouseOverBehaviors.find(behaviorName);
    if (it != mouseOverBehaviors.end()) {
        // if so clean it up
        IMMouseOverBehavior* updater = it->second;
        updater->cleanUp(this);

        // and remove/deref it
        updater->Release();
        mouseOverBehaviors.erase(it);

        return true;
    } else {
        // Let the caller know
        return false;
    }
}

/*******************************************************************************************/
/**
 * @brief  Clear mouse over behaviors
 *
 */
void IMElement::clearMouseOverBehaviors() {
    // iterate through all the behaviors and clean them up
    for (auto& mouseOverBehavior : mouseOverBehaviors) {
        IMMouseOverBehavior* updater = mouseOverBehavior.second;
        updater->Release();
        updater->cleanUp(this);
    }

    mouseOverBehaviors.clear();
}

/*******************************************************************************************/
/**
 * @brief  Add a click behavior
 *
 * @param behavior Handle to behavior in question
 * @param behaviorName name to identify the behavior
 *
 */
void IMElement::addLeftMouseClickBehavior(IMMouseClickBehavior* behavior, std::string const& behaviorName) {
    std::string bName = behaviorName;

    // if they haven't given us a name, generate one
    if (bName == "") {
        std::ostringstream oss;
        oss << "behavior" << numBehaviors;
        bName = oss.str();
        numBehaviors++;
    } else if (leftMouseClickBehaviors.find(bName) != leftMouseClickBehaviors.end()) {
        removeLeftMouseClickBehavior(bName);
    }

    // add a reference as we're storing it
    behavior->AddRef();

    leftMouseClickBehaviors[bName] = behavior;

    // get rid of the reference we were given
    behavior->Release();
}

/*******************************************************************************************/
/**
 * @brief  Removes a named click behavior
 *
 * @param behaviorName name to identify the behavior
 *
 * @returns true if there was a behavior to remove, false otherwise
 *
 */
bool IMElement::removeLeftMouseClickBehavior(std::string const& behaviorName) {
    // see if there is already a behavior with this name
    MCBMap::iterator it = leftMouseClickBehaviors.find(behaviorName);
    if (it != leftMouseClickBehaviors.end()) {
        // if so clean it up
        IMMouseClickBehavior* updater = it->second;
        updater->cleanUp(this);

        // and remove/deref it
        updater->Release();
        leftMouseClickBehaviors.erase(it);

        return true;
    } else {
        // Let the caller know
        return false;
    }
}

/*******************************************************************************************/
/**
 * @brief  Clear mouse over behaviors
 *
 */
void IMElement::clearLeftMouseClickBehaviors() {
    // iterate through all the behaviors and clean them up
    for (auto& leftMouseClickBehavior : leftMouseClickBehaviors) {
        IMMouseClickBehavior* updater = leftMouseClickBehavior.second;
        updater->cleanUp(this);
        updater->Release();
    }

    leftMouseClickBehaviors.clear();
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
void IMElement::update(uint64_t delta, vec2 drawOffset, GUIState& guistate) {
    // Update behaviors
    if (!pauseBehaviors) {
        std::vector<std::string> remove_list;

        for (auto& updateBehavior : updateBehaviors) {
            IMUpdateBehavior* updater = updateBehavior.second;

            // See if this behavior has been initialized
            if (!updater->initialized) {
                if (!updater->initialize(this, delta, drawOffset, guistate)) {
                    // If the behavior has indicated it should not begin remove it
                    remove_list.push_back(updateBehavior.first);
                    continue;
                } else {
                    updater->initialized = true;
                }
            }

            if (!updater->update(this, delta, drawOffset, guistate)) {
                // If the behavior has indicated it is done
                remove_list.push_back(updateBehavior.first);
            }
        }

        std::vector<std::string>::iterator rmit;
        for (rmit = remove_list.begin(); rmit != remove_list.end(); rmit++) {
            removeUpdateBehavior(*rmit);
        }
        lastDrawOffset = drawOffset;
    }

    // Now do mouse behaviors

    // Mouse overs

    if ((scriptMouseOver || (pointInElement(drawOffset, guistate.mousePosition) || guistate.inheritedMouseOver)) && !pauseBehaviors) {
        if (mouseOverForChildren) {
            guistate.inheritedMouseOver = true;
        }

        if (!mouseOver) {
            mouseOver = true;

            // Update behaviors
            for (auto& mouseOverBehavior : mouseOverBehaviors) {
                IMMouseOverBehavior* behavior = mouseOverBehavior.second;
                behavior->onStart(this, delta, drawOffset, guistate);
            }
        } else {
            // Update behaviors
            for (auto& mouseOverBehavior : mouseOverBehaviors) {
                IMMouseOverBehavior* behavior = mouseOverBehavior.second;
                behavior->onContinue(this, delta, drawOffset, guistate);
            }
        }
    } else {
        // See if this is an 'exit'
        if (mouseOver) {
            std::vector<std::string> remove_list;
            for (auto& mouseOverBehavior : mouseOverBehaviors) {
                IMMouseOverBehavior* behavior = mouseOverBehavior.second;

                if (!behavior->onFinish(this, delta, drawOffset, guistate)) {
                    // If the behavior has indicated it is done
                    remove_list.push_back(mouseOverBehavior.first);
                }
            }
            mouseOver = false;
            std::vector<std::string>::iterator rm_it;
            for (rm_it = remove_list.begin(); rm_it != remove_list.end(); rm_it++) {
                removeMouseOverBehavior(*rm_it);
            }
        }
    }

    if (mouse_clicking && guistate.leftMouseState == IMUIContext::kMouseStillUp) {
        mouse_clicking = false;
    }

    // Mouse click status
    if (!pauseBehaviors && (!guistate.clickHandled && (pointInElement(drawOffset, guistate.mousePosition) || guistate.inheritedMouseDown))) {
        IMUIContext::ButtonState effectiveState = guistate.leftMouseState;

        // See if we have an override from our parent
        if (guistate.inheritedMouseDown) {
            effectiveState = guistate.inheritedMouseState;
        }

        switch (effectiveState) {
            case IMUIContext::kMouseDown: {
                if (mouseDownForChildren) {
                    guistate.inheritedMouseDown = true;
                }

                mouse_clicking = true;

                std::vector<std::string> remove_list;
                // Update behaviors
                for (auto& leftMouseClickBehavior : leftMouseClickBehaviors) {
                    guistate.clickHandled = true;

                    IMMouseClickBehavior* behavior = leftMouseClickBehavior.second;
                    if (!behavior->onDown(this, delta, drawOffset, guistate)) {
                        // If the behavior has indicated it is done
                        remove_list.push_back(leftMouseClickBehavior.first);
                    }
                }

                std::vector<std::string>::iterator rm_it;
                for (rm_it = remove_list.begin(); rm_it != remove_list.end(); rm_it++) {
                    removeLeftMouseClickBehavior(*rm_it);
                }

            } break;

            case IMUIContext::kMouseStillDown: {
                if (mouseDownForChildren) {
                    guistate.inheritedMouseDown = true;
                }

                std::vector<std::string> remove_list;
                for (auto& leftMouseClickBehavior : leftMouseClickBehaviors) {
                    guistate.clickHandled = true;

                    IMMouseClickBehavior* behavior = leftMouseClickBehavior.second;

                    if (!behavior->onStillDown(this, delta, drawOffset, guistate)) {
                        // If the behavior has indicated it is done
                        remove_list.push_back(leftMouseClickBehavior.first);
                    }
                }

                std::vector<std::string>::iterator rm_it;
                for (rm_it = remove_list.begin(); rm_it != remove_list.end(); rm_it++) {
                    removeLeftMouseClickBehavior(*rm_it);
                }

            } break;

            case IMUIContext::kMouseUp: {
                if (mouse_clicking) {
                    for (auto& leftMouseClickBehavior : leftMouseClickBehaviors) {
                        guistate.clickHandled = true;

                        IMMouseClickBehavior* behavior = leftMouseClickBehavior.second;

                        if (!behavior->onUp(this, delta, drawOffset, guistate)) {
                            // If the behavior has indicated it is done
                            removeLeftMouseClickBehavior(leftMouseClickBehavior.first);
                        }
                    }
                }
                mouse_clicking = false;

                // Consider this no longer hovering
                mouseOver = false;
                {
                    std::vector<std::string> remove_list;
                    for (auto& mouseOverBehavior : mouseOverBehaviors) {
                        guistate.clickHandled = true;

                        IMMouseOverBehavior* behavior = mouseOverBehavior.second;
                        if (!behavior->onFinish(this, delta, drawOffset, guistate)) {
                            // If the behavior has indicated it is done
                            remove_list.push_back(mouseOverBehavior.first);
                        }
                    }

                    std::vector<std::string>::iterator rm_it;
                    for (rm_it = remove_list.begin(); rm_it != remove_list.end(); rm_it++) {
                        removeMouseOverBehavior(*rm_it);
                    }
                }
            } break;

            case IMUIContext::kMouseStillUp:
            default:

                break;
        }
    }
}

/*******************************************************************************************/
/**
 * @brief  When this element is resized, moved, etc propagate this signal upwards
 *
 */
void IMElement::onRelayout() {
    if (owner != NULL) {
        owner->onRelayout();
    }
}

/*******************************************************************************************/
/**
 * @brief  When this element has an error, propagate it upwards
 *
 * @param newError Error message
 *
 */
void IMElement::onError(std::string const& newError) {
    if (owner != NULL) {
        owner->reportError(newError);
    }
}

/*******************************************************************************************/
/**
 * @brief  When a resize, move, etc has happened do whatever is necessary
 *
 */
void IMElement::doRelayout() {
    // Nothing to do in the base class
}

/*******************************************************************************************/
/**
 * @brief  Do whatever is necessary when the resolution changes
 *
 */
void IMElement::doScreenResize() {
    // Nothing to do in the base class
}

/*******************************************************************************************/
/**
 * @brief Set the name of this element
 *
 * @param _name New name (incumbent on the programmer to make sure they're unique)
 *
 */
void IMElement::setName(std::string const& _name) {
    name = _name;
}

/*******************************************************************************************/
/**
 * @brief Gets the name of this element
 *
 * @returns name of this element
 *
 */
std::string IMElement::getName() {
    return name;
}

/*******************************************************************************************/
/**
 * @brief  Set the padding for each direction on the element
 *
 * UNDEFINEDSIZE will cause no change
 *
 * @param U (minimum) Padding between the element and the upper boundary
 * @param D (minimum) Padding between the element and the lower boundary
 * @param L (minimum) Padding between the element and the left boundary
 * @param R (minimum) Padding between the element and the right boundary
 *
 */
void IMElement::setPadding(float U, float D, float L, float R) {
    if (U != UNDEFINEDSIZE) {
        paddingU = U;
    }
    if (D != UNDEFINEDSIZE) {
        paddingD = D;
    }
    if (L != UNDEFINEDSIZE) {
        paddingL = L;
    }
    if (R != UNDEFINEDSIZE) {
        paddingR = R;
    }

    onRelayout();
}

/*******************************************************************************************/
/**
 * @brief  Set the padding above the element
 *
 * @param paddingSize The number of pixels (in GUI space) for the padding
 *
 */
void IMElement::setPaddingU(float paddingSize) {
    paddingU = paddingSize;
    onRelayout();
}

/*******************************************************************************************/
/**
 * @brief  Set the padding below the element
 *
 * @param paddingSize The number of pixels (in GUI space) for the padding
 *
 */
void IMElement::setPaddingD(float paddingSize) {
    paddingD = paddingSize;
    onRelayout();
}

/*******************************************************************************************/
/**
 * @brief  Set the padding to the left of the element
 *
 * @param paddingSize The number of pixels (in GUI space) for the padding
 *
 */
void IMElement::setPaddingL(float paddingSize) {
    paddingL = paddingSize;
    onRelayout();
}

/*******************************************************************************************/
/**
 * @brief  Set the padding to the right of the element
 *
 * @param paddingSize The number of pixels (in GUI space) for the padding
 *
 */
void IMElement::setPaddingR(float paddingSize) {
    paddingR = paddingSize;
    onRelayout();
}

/*******************************************************************************************/
/**
 * @brief  Sets the drawing displacement (mostly used for tweening)
 *
 * @param newDisplacement newValues for the displacement
 *
 */
void IMElement::setDisplacement(vec2 newDisplacement) {
    drawDisplacement = newDisplacement;
}

/*******************************************************************************************/
/**
 * @brief  Sets the drawing displacement x component (mostly used for tweening)
 *
 * @param newDisplacement newValues for the displacement
 *
 */
void IMElement::setDisplacementX(float newDisplacement) {
    drawDisplacement.x() = newDisplacement;
}

/*******************************************************************************************/
/**
 * @brief  Sets the drawing displacement y component (mostly used for tweening)
 *
 * @param newDisplacement newValues for the displacement
 *
 */
void IMElement::setDisplacementY(float newDisplacement) {
    drawDisplacement.y() = newDisplacement;
}

/*******************************************************************************************/
/**
 * @brief  Gets the drawing displacement (mostly used for tweening)
 *
 * @returns Displacement vector
 *
 */
vec2 IMElement::getDisplacement(vec2 newDisplacement) {
    return drawDisplacement;
}

/*******************************************************************************************/
/**
 * @brief  Gets the drawing displacement x component (mostly used for tweening)
 *
 * @returns Displacement value
 *
 */
float IMElement::getDisplacementX() {
    return drawDisplacement.x();
}

/*******************************************************************************************/
/**
 * @brief  Gets the drawing displacement y component (mostly used for tweening)
 *
 * @returns Displacement value
 *
 */
float IMElement::getDisplacementY() {
    return drawDisplacement.y();
}

/*******************************************************************************************/
/**
 * @brief  Sets the default size
 *
 * @param newDefault the new default size
 *
 */
void IMElement::setDefaultSize(vec2 newDefault) {
    defaultSize = newDefault;
}

/*******************************************************************************************/
/**
 * @brief  Retrieves the default size
 *
 * @returns 2d integer vector of the default size
 *
 */
vec2 IMElement::getDefaultSize() {
    return defaultSize;
}

/*******************************************************************************************/
/**
 * @brief  For container type classes - resize event, called internally
 *
 */
void IMElement::onChildResize(IMElement* child) {
    // nothing to do in the base class
    child->Release();
}

/*******************************************************************************************/
/**
 * @brief  For container type classes - resize event, called internally
 *
 */

void IMElement::onParentResize() {
    // nothing to do in the base class
}

/*******************************************************************************************/
/**
 * @brief  Sets the size of the region (not including padding)
 *
 * @param _size 2d size vector (-1 element implies undefined - or use UNDEFINEDSIZE)
 *
 */
void IMElement::setSize(const vec2 _size) {
    vec2 oldSize = size;

    size = _size;

    size.x() += paddingL + paddingR;
    size.y() += paddingU + paddingD;

    // make sure the size has actually changed
    if (size.x() != oldSize.x() || size.y() != oldSize.y()) {
        // Signal that something has changed
        onRelayout();
    }
}

/*******************************************************************************************/
/**
 * @brief  Sets the x dimension of a region
 *
 * @param x x dimension size (-1 implies undefined - or use UNDEFINEDSIZE)
 *
 */
void IMElement::setSizeX(const float x) {
    setSize(vec2(x, size.y()));
}

/*******************************************************************************************/
/**
 * @brief  Sets the y dimension of a region
 *
 * @param y y dimension size (-1 implies undefined - or use UNDEFINEDSIZE)
 *
 */
void IMElement::setSizeY(const float y) {
    setSize(vec2(size.x(), y));
}

/*******************************************************************************************/
/**
 * @brief  Gets the size vector
 *
 * @returns The size vector
 *
 */
vec2 IMElement::getSize() {
    return size;
}

/*******************************************************************************************/
/**
 * @brief  Gets the size x component
 *
 * @returns The x size
 *
 */
float IMElement::getSizeX() {
    return size.x();
}

/*******************************************************************************************/
/**
 * @brief  Gets the size y component
 *
 * @returns The y size
 *
 */
float IMElement::getSizeY() {
    return size.y();
}

/*******************************************************************************************/
/**
 * @brief  Sends a message to the owning GUI
 *
 * @param theMessage the message
 *
 */
void IMElement::sendMessage(IMMessage* theMessage) {
    if (owner != NULL) {
        // receiveMessage releases the message for us
        owner->receiveMessage(theMessage);
    } else {
        theMessage->Release();
    }
}

/*******************************************************************************************/
/**
 * @brief  Finds an element by a given name
 *
 * @param elementName the name of the element
 *
 * @returns handle to the element (NULL if not found)
 *
 */
IMElement* IMElement::findElement(std::string const& elementName) {
    // Check if this is the droid we're looking for
    if (name == elementName) {
        AddRef();
        return this;
    } else {
        return NULL;
    }
}

/*******************************************************************************************/
/**
 * @brief  Remove all referenced object without releaseing references
 *
 */
void IMElement::clense() {
    owner = NULL;
    parent = NULL;

    updateBehaviors.clear();
    mouseOverBehaviors.clear();
    leftMouseClickBehaviors.clear();
}

void IMElement::setPauseBehaviors(bool pause) {
    pauseBehaviors = pause;
}

bool IMElement::isMouseOver() {
    return mouseOver;
}

void IMElement::setScriptMouseOver(bool mouseOver) {
    scriptMouseOver = mouseOver;
}

/*******************************************************************************************/
/**
 * @brief  Destructor
 *
 */
IMElement::~IMElement() {
    LOG_ASSERT(refCount == 0);
    for (auto& updateBehavior : updateBehaviors) {
        updateBehavior.second->Release();
    }
    updateBehaviors.clear();

    for (auto& mouseOverBehavior : mouseOverBehaviors) {
        mouseOverBehavior.second->Release();
    }
    mouseOverBehaviors.clear();

    for (auto& leftMouseClickBehavior : leftMouseClickBehaviors) {
        leftMouseClickBehavior.second->Release();
    }
    leftMouseClickBehaviors.clear();

    imevents.DeRegisterListener(this);
    imevents.TriggerDestroyed(this);
}

void IMElement::DestroyQueuedIMElements() {
    while (!IMElement::deletion_schedule.empty()) {
        IMElement* element = IMElement::deletion_schedule[0];
        if (element != nullptr) {
            delete element;
            element = nullptr;
        }

        IMElement::deletion_schedule.erase(IMElement::deletion_schedule.begin());
    }
}

void IMElement::DestroyedIMElement(IMElement* element) {
    if (element == parent) {
        parent = NULL;
    }
}

void IMElement::DestroyedIMGUI(IMGUI* imgui) {
    if (owner == imgui) {
        owner = NULL;
    }
}
