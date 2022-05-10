//-----------------------------------------------------------------------------
//           Name: im_selection_list.h
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
#pragma once

#include <GUI/IMUI/im_support.h>
#include <GUI/IMUI/im_element.h>
#include <GUI/IMUI/im_text.h>
#include <GUI/IMUI/im_divider.h>
#include <GUI/IMUI/im_behaviors.h>

/*******************************************************************************************/
/**
 * @brief  A selection list of text elements
 *
 *
 */
class IMTextSelectionList : public IMDivider {
    IMUpdateBehavior* itemBehavior;

   public:
    FontSetup font;                     // what font to use for this selection list
    float betweenSpace;                 // how much space to put between items?
    IMMouseOverBehavior* mouseOver;     // what behaviour for mousing over?
    ContainerAlignment itemXAlignment;  // list item x alignment
    ContainerAlignment itemYAlignment;  // list item y alignment

    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *
     *
     */
    IMTextSelectionList(std::string const& name,
                        FontSetup _font,
                        float _betweenSpace,
                        IMMouseOverBehavior* _mouseOver = NULL);

    IMTextSelectionList() {}

    /*******
     *
     * Angelscript factory
     *
     */
    static IMTextSelectionList* ASFactory(std::string const& _name,
                                          FontSetup _fontSetup,
                                          float _betweenSpace,
                                          IMMouseOverBehavior* _mouseOver = NULL) {
        return new IMTextSelectionList(_name, _fontSetup, _betweenSpace, _mouseOver);
    }

    /*******************************************************************************************/
    /**
     * @brief  Set’s the alignment of the list objects
     *
     * @param xAlignment horizontal alignment
     * @param yAlignment vertical alignment
     *
     */
    void setAlignment(ContainerAlignment xAlignment, ContainerAlignment yAlignment);

    /*******************************************************************************************/
    /**
     * @brief  Gets the name of the type of this element — for autonaming and debugging
     *
     * @returns name of the element type as a string
     *
     */
    std::string getElementTypeName() override {
        return "TextSelectionList";
    }

    /*******************************************************************************************/
    /**
     * @brief  Add an update behavior to each subsequent item
     *
     * @param behavior Handle to behavior in question
     *
     */
    void setItemUpdateBehaviour(IMUpdateBehavior* behavior);

    /*******************************************************************************************/
    /**
     * @brief  Add a new entry to this list
     *
     * @param name GUI name for this text
     * @param text Text to display
     * @param name Text to send as a message when this item is selected
     *
     */
    void addEntry(std::string const& name, std::string const& text, std::string const& message);
    void addEntryParam(std::string const& name, std::string const& text, std::string const& message, std::string const& param);

    /*******************************************************************************************/
    /**
     * @brief  Destructor
     *
     */
    ~IMTextSelectionList() override;
};
