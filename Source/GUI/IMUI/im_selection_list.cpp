//-----------------------------------------------------------------------------
//           Name: im_selection_list.cpp
//      Developer: Wolfire Games LLC
//    Description: Divider types for making selection lists
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
#include "im_selection_list.h"

/*******************************************************************************************/
/**
 * @brief  Constructor
 *
 *
 */
IMTextSelectionList::IMTextSelectionList(std::string const& name,
                                         FontSetup _font,
                                         float _betweenSpace,
                                         IMMouseOverBehavior* _mouseOver) : IMDivider(name),
                                                                            itemBehavior(NULL),
                                                                            itemXAlignment(CACenter),
                                                                            itemYAlignment(CACenter) {
    IMrefCountTracker.addRefCountObject(getElementTypeName());

    // let our superclass set us up
    font = _font;
    betweenSpace = _betweenSpace;
    mouseOver = _mouseOver;
}

/*******************************************************************************************/
/**
 * @brief  Set’s the alignment of the list objects
 *
 * @param xAlignment horizontal alignment
 * @param yAlignment vertical alignment
 *
 */
void IMTextSelectionList::setAlignment(ContainerAlignment xAlignment, ContainerAlignment yAlignment) {
    itemXAlignment = xAlignment;
    itemYAlignment = yAlignment;
}

/*******************************************************************************************/
/**
 * @brief  Add an update behavior to each subsequent item
 *
 * @param behavior Handle to behavior in question
 *
 */
void IMTextSelectionList::setItemUpdateBehaviour(IMUpdateBehavior* behavior) {
    if (itemBehavior != NULL) {
        itemBehavior->Release();
    }

    itemBehavior = behavior->clone();

    // release the reference we were given
    behavior->Release();
}

/*******************************************************************************************/
/**
 * @brief  Add a new entry to this list
 *
 * @param name GUI name for this text
 * @param text Text to display
 * @param name Text to send as a message when this item is selected
 *
 */
void IMTextSelectionList::addEntry(std::string const& name, std::string const& text, std::string const& message) {
    std::string empty = "";
    addEntryParam(name, text, message, empty);
}

void IMTextSelectionList::addEntryParam(std::string const& name, std::string const& text, std::string const& message, std::string const& param) {
    // build the new object
    IMText* newText = new IMText(text, font);
    newText->setName(name);
    IMMouseClickBehavior* clickBehavior = NULL;
    if (param == "") {
        clickBehavior = new IMFixedMessageOnClick(message);
    } else {
        clickBehavior = new IMFixedMessageOnClick(message, param);
    }

    clickBehavior->AddRef();
    newText->addLeftMouseClickBehavior(clickBehavior);
    clickBehavior->Release();

    if (mouseOver != NULL) {
        // make a new copy of the mouseover behaviour
        IMMouseOverBehavior* newBehavior = mouseOver->clone();

        newBehavior->AddRef();
        newText->addMouseOverBehavior(newBehavior, "");
        newBehavior->Release();
    }

    if (itemBehavior != NULL) {
        IMUpdateBehavior* newBehavior = itemBehavior->clone();
        newBehavior->AddRef();
        newText->addUpdateBehavior(newBehavior, "");
        newBehavior->Release();
    }

    // if we have been given space and we have at least on element, add the space
    if ((betweenSpace > 0) && (getContainerCount() > 0)) {
        IMSpacer* newSpacer = appendSpacer(betweenSpace);
        newSpacer->Release();
    }

    // .. and add it to the list
    IMContainer* newContainer = append(newText);
    newContainer->Release();

    // finally, remove the reference as we're done with it (and append will have addrefed it)
    // newText->Release();
}

/*******************************************************************************************/
/**
 * @brief  Destructor
 *
 */
IMTextSelectionList::~IMTextSelectionList() {
    IMrefCountTracker.removeRefCountObject(getElementTypeName());

    if (mouseOver != NULL) {
        mouseOver->Release();
    }

    if (itemBehavior != NULL) {
        itemBehavior->Release();
    }
}
