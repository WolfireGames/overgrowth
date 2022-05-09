//-----------------------------------------------------------------------------
//           Name: im_divider.h
//      Developer: Wolfire Games LLC
//    Description: Specialized container element class for creating adhoc GUIs
//                 as part of the UI tools
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
#include <GUI/IMUI/im_container.h>
#include <GUI/IMUI/im_spacer.h>

/*******************************************************************************************/
/**
 * @brief  Basic container class, holds other elements
 *
 */
class IMDivider : public IMElement {
   public:
    std::vector<IMContainer*> containers;  // Whats in this divider
    DividerOrientation orientation;        // vertical by default
    int contentsNum;                       // just a counter for making unique names

    ContainerAlignment contentsXAlignment;  // horizontal alignment of new elements
    ContainerAlignment contentsYAlignment;  // vertical alignment of new elements

    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *
     * @param name IMElement name
     * @param _orientation The orientation of the container
     *
     */
    IMDivider(std::string const& name, DividerOrientation _orientation = DOVertical);

    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *
     * @param name IMElement name
     * @param _orientation The orientation of the container
     *
     */
    IMDivider(DividerOrientation _orientation = DOVertical);

    /*******
     *
     * Angelscript factory
     *
     */
    static IMDivider* ASFactory_named(std::string const& name, DividerOrientation _orientation = DOVertical) {
        return new IMDivider(name, _orientation);
    }

    static IMDivider* ASFactory_unnamed(DividerOrientation _orientation = DOVertical) {
        return new IMDivider(_orientation);
    }

    /*******************************************************************************************/
    /**
     * @brief  Gets the name of the type of this element — for autonaming and debugging
     *
     * @returns name of the element type as a string
     *
     */
    std::string getElementTypeName() override;

    /*******************************************************************************************/
    /**
     * @brief  Set’s this element’s parent (and does nessesary logic)
     *
     * @param _parent New parent
     *
     */
    void setOwnerParent(IMGUI* _owner, IMElement* _parent) override;

    /*******************************************************************************************/
    /**
     * @brief  Set’s the alignment of the contained object
     *
     * @param xAlignment horizontal alignment
     * @param yAlignment vertical alignment
     * @param reposition should we go through and reposition existing objects
     *
     */
    void setAlignment(ContainerAlignment xAlignment, ContainerAlignment yAlignment, bool reposition = true);

    /*******************************************************************************************/
    /**
     * @brief  Clear the contents of this divider, leaving everything else the same
     *
     */
    void clear();

    /*******************************************************************************************/
    /**
     * @brief  Updates the element
     *
     * @param delta Number of millisecond elapsed since last update
     * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
     * @param guistate The state of the GUI at this update
     *
     */
    void update(uint64_t delta, vec2 drawOffset, GUIState& guistate) override;

    /*******************************************************************************************/
    /**
     * @brief  Rather counter-intuitively, this draws this object on the screen
     *
     * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
     * @param clipPos pixel location of upper lefthand corner of clipping region
     * @param clipSize size of clipping region
     *
     */
    void render(vec2 drawOffset, vec2 clipPos, vec2 clipSize) override;

    /*******************************************************************************************/
    /**
     * @brief Rederive the regions for the various orientation containers - for internal use
     *
     */
    void checkRegions();

    /*******************************************************************************************/
    /**
     * @brief  When a resize, move, etc has happened do whatever is necessary
     *
     */
    void doRelayout() override;

    /*******************************************************************************************/
    /**
     * @brief  Do whatever is necessary when the resolution changes
     *
     */
    void doScreenResize() override;

    /*******************************************************************************************/
    /**
     * @brief Convenience function to add a spacer element to this divider
     *
     * @param size Size of the element in terms of GUI space pixels
     *
     * @returns the space object created, just in case you need it
     *
     */
    IMSpacer* appendSpacer(float _size);

    /*******************************************************************************************/
    /**
     * @brief Convenience function to add a dynamic spacer element to this divider
     *         A dynamic spacer will eat up any extra space in the divider
     *
     * @param size Size of the element in terms of GUI space pixels
     *
     * @returns the space object created, just in case you need it
     *
     */
    IMSpacer* appendDynamicSpacer();

    /*******************************************************************************************/
    /**
     * @brief  Get the number of containers in this divider
     *
     * @returns count of the containers
     *
     */
    unsigned int getContainerCount();

    /*******************************************************************************************/
    /**
     * @brief  Fetch the container at the given index
     *
     *
     * @param i index of the container
     *
     */
    IMContainer* getContainerAt(unsigned int i);

    /*******************************************************************************************/
    /**
     * @brief  Gets the container of a named element
     *
     * @param _name Name of the element
     *
     * @returns container of the element (NULL if none)
     *
     */
    IMContainer* getContainerOf(std::string const& _name);

    /*******************************************************************************************/
    /**
     * @brief Adds an element to the divider
     *
     * @param newElement IMElement to add
     * @param direction Portion of the divider to add to (default top/left)
     *
     */
    IMContainer* append(IMElement* newElement, float containerSize = UNDEFINEDSIZE);

    /*******************************************************************************************/
    /**
     * @brief  Find an element by name — called internally
     *
     *
     */
    IMElement* findElement(std::string const& elementName) override;

    void setPauseBehaviors(bool pause) override;

    DividerOrientation getOrientation() const { return orientation; }

    /*******************************************************************************************/
    /**
     * @brief  Remove all referenced object without releaseing references
     *
     */
    void clense() override;

    /*******************************************************************************************/
    /**
     * @brief  Destructor
     *
     */
    ~IMDivider() override;

    void DestroyedIMElement(IMElement* element) override;
    void DestroyedIMGUI(IMGUI* imgui) override;
};
