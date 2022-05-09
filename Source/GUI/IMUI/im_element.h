//-----------------------------------------------------------------------------
//           Name: im_element.h
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
#pragma once

#include <GUI/IMUI/im_support.h>
#include <GUI/IMUI/imui_state.h>
#include <GUI/IMUI/im_message.h>
#include <GUI/IMUI/im_events.h>

#include <Scripting/angelscript/asmodule.h>

#include <map>
#include <string>

class IMElement;  // Forward declaration

/*******************************************************************************************/
/**
 * @brief  Attachable behavior base class - called on update
 *
 */
struct IMUpdateBehavior {
    bool initialized;  // Has this update been run once?
    int refCount;      // for AS reference counting

    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *
     * @param _name Name for this object (incumbent on the programmer to make sure they're unique)
     *
     */
    IMUpdateBehavior() : initialized(false),
                         refCount(1) {
        IMrefCountTracker.addRefCountObject("UpdateBehavior");
    }

    /*******
     *
     * Angelscript factory
     *
     */
    static IMUpdateBehavior* ASFactory() {
        return new IMUpdateBehavior();
    }

    /*******
     *
     * Angelscript memory management boilerplate
     *
     */
    void AddRef() {
        // Increase the reference counter
        refCount++;
    }

    void Release() {
        // Decrease ref count and delete if it reaches 0
        if (--refCount == 0) {
            delete this;
        }
    }

    /*******************************************************************************************/
    /**
     * @brief  Called before the first update
     *
     * @param element The element attached to this behavior
     * @param delta Number of millisecond elapsed since last update
     * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
     * @param guistate The state of the GUI at this update
     *
     * @returns true if this behavior should continue next update, false otherwise
     *
     */
    virtual bool initialize(IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate) {
        return true;
    }

    /*******************************************************************************************/
    /**
     * @brief  Called on update
     *
     * @param element The element attached to this behavior
     * @param delta Number of millisecond elapsed since last update
     * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
     * @param guistate The state of the GUI at this update
     *
     * @returns true if this behavior should continue next update, false otherwise
     *
     */
    virtual bool update(IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate) {
        return true;
    }

    /*******************************************************************************************/
    /**
     * @brief  Called when the behavior ceases, whether by its own indicate or externally
     *
     * @param element The element attached to this behavior
     *
     */
    virtual void cleanUp(IMElement* element) {
    }

    /*******************************************************************************************/
    /**
     * @brief  Create a copy of this object (respecting inheritance)
     *
     */
    virtual IMUpdateBehavior* clone() {
        IMUpdateBehavior* c = new IMUpdateBehavior;
        return c;
    }

    /*******************************************************************************************/
    /**
     * @brief  Destructor
     *
     */
    virtual ~IMUpdateBehavior() {
        IMrefCountTracker.removeRefCountObject("UpdateBehavior");
    }
};

/*******************************************************************************************/
/**
 * @brief  Attachable behavior base class - called on mouse over
 *
 */
struct IMMouseOverBehavior {
    int refCount;  // for AS reference counting

    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *
     */
    IMMouseOverBehavior() : refCount(1) {
        IMrefCountTracker.addRefCountObject("MouseOverBehavior");
    }

    /*******
     *
     * Angelscript factory
     *
     */
    static IMMouseOverBehavior* ASFactory() {
        return new IMMouseOverBehavior();
    }

    /*******
     *
     * Angelscript memory management boilerplate
     *
     */
    void AddRef() {
        // Increase the reference counter
        refCount++;
    }

    void Release() {
        // Decrease ref count and delete if it reaches 0
        if (--refCount == 0) {
            delete this;
        }
    }

    /*******************************************************************************************/
    /**
     * @brief  Called when the mouse enters the element
     *
     * @param element The element attached to this behavior
     * @param delta Number of millisecond elapsed since last update
     * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
     * @param guistate The state of the GUI at this update
     *
     */
    virtual void onStart(IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate) {
    }

    /*******************************************************************************************/
    /**
     * @brief  Called when the mouse is still over the element
     *
     * @param element The element attached to this behavior
     * @param delta Number of millisecond elapsed since last update
     * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
     * @param guistate The state of the GUI at this update
     *
     */
    virtual void onContinue(IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate) {
    }

    /*******************************************************************************************/
    /**
     * @brief  Called when the mouse leaves the element
     *
     * @param element The element attached to this behavior
     * @param delta Number of millisecond elapsed since last update
     * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
     * @param guistate The state of the GUI at this update
     *
     * @return true if this behavior should be retained, false otherwise
     *
     */
    virtual bool onFinish(IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate) {
        return true;
    }

    /*******************************************************************************************/
    /**
     * @brief  Called when the behavior ceases, whether by its own indicate or externally
     *
     * @param element The element attached to this behavior
     *
     */
    virtual void cleanUp(IMElement* element) {
    }

    /*******************************************************************************************/
    /**
     * @brief  Create a copy of this object (respecting inheritance)
     *
     */
    virtual IMMouseOverBehavior* clone() {
        IMMouseOverBehavior* c = new IMMouseOverBehavior;
        return c;
    }

    /*******************************************************************************************/
    /**
     * @brief  Destructor
     *
     */
    virtual ~IMMouseOverBehavior() {
        IMrefCountTracker.removeRefCountObject("MouseOverBehavior");
    }
};

/*******************************************************************************************/
/**
 * @brief  Attachable behavior base class - called on mouse down
 *
 */
struct IMMouseClickBehavior {
    int refCount;  // for AS reference counting

    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *
     */
    IMMouseClickBehavior() : refCount(1) {
        IMrefCountTracker.addRefCountObject("MouseClickBehavior");
    }

    /*******
     *
     * Angelscript factory
     *
     */
    static IMUpdateBehavior* ASFactory() {
        return new IMUpdateBehavior();
    }

    /*******
     *
     * Angelscript memory management boilerplate
     *
     */
    void AddRef() {
        // Increase the reference counter
        refCount++;
    }

    void Release() {
        // Decrease ref count and delete if it reaches 0
        if (--refCount == 0) {
            delete this;
        }
    }

    /*******************************************************************************************/
    /**
     * @brief  Called when the mouse button is pressed on element
     *
     * @param element The element attached to this behavior
     * @param delta Number of millisecond elapsed since last update
     * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
     * @param guistate The state of the GUI at this update
     *
     * @return true if this behavior should be retained, false otherwise
     *
     */
    virtual bool onDown(IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate) {
        return true;
    }

    /*******************************************************************************************/
    /**
     * @brief  Called when the mouse button continues to be pressed on an element
     *
     * @param element The element attached to this behavior
     * @param delta Number of millisecond elapsed since last update
     * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
     * @param guistate The state of the GUI at this update
     *
     * @return true if this behavior should be retained, false otherwise
     *
     */
    virtual bool onStillDown(IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate) {
        return true;
    }

    /*******************************************************************************************/
    /**
     * @brief  Called when the mouse button is released on element
     *
     * @param element The element attached to this behavior
     * @param delta Number of millisecond elapsed since last update
     * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
     * @param guistate The state of the GUI at this update
     *
     * @return true if this behavior should be retained, false otherwise
     *
     */
    virtual bool onUp(IMElement* element, uint64_t delta, vec2 drawOffset, GUIState& guistate) {
        return true;
    }

    /*******************************************************************************************/
    /**
     * @brief  Called when the behavior ceases, whether by its own indicate or externally
     *
     * @param element The element attached to this behavior
     *
     */
    virtual void cleanUp(IMElement* element) {
    }

    /*******************************************************************************************/
    /**
     * @brief  Create a copy of this object (respecting inheritance)
     *
     */
    virtual IMMouseClickBehavior* clone() {
        IMMouseClickBehavior* c = new IMMouseClickBehavior;
        return c;
    }

    /*******************************************************************************************/
    /**
     * @brief  Destructor
     *
     */
    virtual ~IMMouseClickBehavior() {
        IMrefCountTracker.removeRefCountObject("MouseClickBehavior");
    }
};

struct ControllerItem {
    IMMessage* message;
    IMMessage* messageOnSelect;
    IMMessage* messageLeft;
    IMMessage* messageRight;
    IMMessage* messageUp;
    IMMessage* messageDown;
    bool execute_on_select;
    bool skip_show_border;

    int refCount;

    ControllerItem()
        : refCount(1), message(NULL), messageOnSelect(NULL), messageLeft(NULL), messageRight(NULL), messageUp(NULL), messageDown(NULL), skip_show_border(false), execute_on_select(false) {
        IMrefCountTracker.addRefCountObject("CItem");
    }

    ~ControllerItem() {
        IMrefCountTracker.removeRefCountObject("CItem");
        if (message) {
            message->Release();
        }
        if (messageOnSelect) {
            messageOnSelect->Release();
        }
        if (messageLeft) {
            messageLeft->Release();
        }
        if (messageRight) {
            messageRight->Release();
        }
        if (messageUp) {
            messageUp->Release();
        }
        if (messageDown) {
            messageDown->Release();
        }
    }

    static ControllerItem* ASFactory() {
        return new ControllerItem();
    }

    void AddRef() {
        refCount++;
    }

    void Release() {
        if (--refCount == 0) {
            delete this;
        }
    }

    void setMessage(IMMessage* message) {
        if (this->message) {
            this->message->Release();
        }
        this->message = message;
    }
    void setMessageOnSelect(IMMessage* message) {
        if (this->messageOnSelect) {
            this->messageOnSelect->Release();
        }
        this->messageOnSelect = message;
    }
    void setMessages(IMMessage* message, IMMessage* messageOnSelect, IMMessage* messageLeft, IMMessage* messageRight, IMMessage* messageUp, IMMessage* messageDown) {
        if (message) {
            if (this->message) {
                this->message->Release();
            }
            this->message = message;
        }
        if (messageOnSelect) {
            if (this->messageOnSelect) {
                this->messageOnSelect->Release();
            }
            this->messageOnSelect = messageOnSelect;
        }
        if (messageLeft) {
            if (this->messageLeft) {
                this->messageLeft->Release();
            }
            this->messageLeft = messageLeft;
        }
        if (messageRight) {
            if (this->messageRight) {
                this->messageRight->Release();
            }
            this->messageRight = messageRight;
        }
        if (messageUp) {
            if (this->messageUp) {
                this->messageUp->Release();
            }
            this->messageUp = messageUp;
        }
        if (messageDown) {
            if (this->messageDown) {
                this->messageDown->Release();
            }
            this->messageDown = messageDown;
        }
    }

    IMMessage* getMessage() {
        if (message) message->AddRef();
        return message;
    }
    IMMessage* getMessageOnSelect() {
        if (messageOnSelect) messageOnSelect->AddRef();
        return messageOnSelect;
    }
    IMMessage* getMessageLeft() {
        if (messageLeft) messageLeft->AddRef();
        return messageLeft;
    }
    IMMessage* getMessageRight() {
        if (messageRight) messageRight->AddRef();
        return messageRight;
    }
    IMMessage* getMessageUp() {
        if (messageUp) messageUp->AddRef();
        return messageUp;
    }
    IMMessage* getMessageDown() {
        if (messageDown) messageDown->AddRef();
        return messageDown;
    }

    bool isActive() { return message || messageOnSelect || messageLeft || messageRight || messageUp || messageDown; }
};

class IMGUI;  // Forward declaration

class IMElement : public IMEventListener {
   public:
    vec2 size;              // dimensions of the actual region (GUI space)
    vec2 defaultSize;       // What size (if any) should this element become once 'reset'
    vec2 drawDisplacement;  // Is this element being drawn somewhere other than where it 'lives' (mostly for tweening)
    vec2 lastDrawOffset;
    float paddingU;  // (minimum) Padding between the element and the upper boundary
    float paddingD;  // (minimum) Padding between the element and the lower boundary
    float paddingL;  // (minimum) Padding between the element and the left boundary
    float paddingR;  // (minimum) Padding between the element and the right boundary

    ControllerItem controllerItem;

    int zOrdering;  // At what point in the rendering process does this get drawn in

    std::string name;  // name to refer to this object by -- incumbent on the programmer to make sure they're unique

    IMElement* parent;  // NULL if 'root'
    IMGUI* owner;       // what GUI owns this element

    int numBehaviors;  // Counter for unique behavior names

    typedef std::map<std::string, IMUpdateBehavior*> UBMap;
    typedef std::map<std::string, IMMouseOverBehavior*> MOBMap;
    typedef std::map<std::string, IMMouseClickBehavior*> MCBMap;

    UBMap updateBehaviors;           // update behaviors
    MOBMap mouseOverBehaviors;       // mouse over behaviors
    MCBMap leftMouseClickBehaviors;  // mouse up behaviors

    bool show;             // should this element be rendered?
    bool shouldClip;       // should this element be included in container clipping?
    vec4 color;            // if this element is colored, what color is it? -- other elements may define further colors
    vec4 effectColor;      // if the color is temp
    bool isColorEffected;  // is there a temporary color change?
    bool border;           // should this element have a border?
    float borderSize;      // how thick is this border (in GUI space pixels)
    vec4 borderColor;      // color for the border

    bool mouseOver;  // has mouse been over this element
    bool mouse_clicking;

    bool mouseDownForChildren;  // Should this pass on mouse down to all its children
    bool mouseOverForChildren;  // Should this pass on mouse over to all its children

    int refCount;  // for AS reference counting
    static std::vector<IMElement*> deletion_schedule;

    bool pauseBehaviors;  // Don't update any behaviors (mouse exit will still be called to avoid breakage)
    bool scriptMouseOver;

    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *
     * @param _name Name for this object (incumbent on the programmer to make sure they're unique)
     *
     */
    IMElement(std::string const& _name = "");

    /*******
     *
     * Angelscript memory management boilerplate
     *
     */
    void AddRef();
    void Release();

    int getRefCount() { return refCount; }

    /*******************************************************************************************/
    /**
     * @brief  Gets the name of the type of this element — for autonaming and debugging
     *
     * @returns name of the element type as a string
     *
     */
    virtual std::string getElementTypeName();

    /*******************************************************************************************/
    /**
     * @brief  Set’s this element’s parent (and does nessesary logic)
     *
     * @param _parent New parent
     *
     */
    virtual void setOwnerParent(IMGUI* _owner, IMElement* _parent);

    /*******************************************************************************************/
    /**
     * @brief  Set the color for the element
     *
     * @param _color 4 component vector for the color
     *
     */
    virtual void setColor(vec4 _color);

    /*******************************************************************************************/
    /**
     * @brief  Gets the current color
     *
     * If the color is effected, it'll return the effected color
     *
     * @returns 4 component vector of the color
     *
     */
    virtual vec4 getColor();

    /*******************************************************************************************/
    /**
     * @brief  Gets the current color -- ignoring the effect color
     *
     * @returns 4 component vector of the color
     *
     */
    virtual vec4 getBaseColor();

    /*******************************************************************************************/
    /**
     * @brief  Set the effect color for the element
     *
     * @param _color 4 component vector for the color
     *
     */
    virtual void setEffectColor(vec4 _color);

    /*******************************************************************************************/
    /**
     * @brief  Gets the effect current color
     *
     * @returns 4 component vector of the color
     *
     */
    virtual vec4 getEffectColor();

    /*******************************************************************************************/
    /**
     * @brief Clears any effect color (reseting to the base)
     *
     */
    virtual void clearColorEffect();

    /*******************************************************************************************/
    /**
     * @brief  Sets the red value
     *
     * @param value Color value
     *
     */
    virtual void setR(float value);

    /*******************************************************************************************/
    /**
     * @brief  Gets the red value
     *
     * @returns Color value
     *
     */
    virtual float getR();

    /*******************************************************************************************/
    /**
     * @brief Sets the green value
     *
     * @param value Color value
     *
     */
    virtual void setG(float value);

    /*******************************************************************************************/
    /**
     * @brief Gets the green value
     *
     * @returns Color value
     *
     */
    virtual float getG();

    /*******************************************************************************************/
    /**
     * @brief Sets the blue value
     *
     * @param value Color value
     *
     */
    virtual void setB(float value);

    /*******************************************************************************************/
    /**
     * @brief Gets the blue value
     *
     * @returns Color value
     *
     */
    virtual float getB();

    /*******************************************************************************************/
    /**
     * @brief Sets the alpha value
     *
     * @param value Color value
     *
     */
    virtual void setAlpha(float value);

    /*******************************************************************************************/
    /**
     * @brief Gets the alpha value
     *
     * @returns Color value
     *
     */
    virtual float getAlpha();

    /*******************************************************************************************/
    /**
     * @brief  Sets the effect red value
     *
     * @param value Color value
     *
     */
    virtual void setEffectR(float value);

    /*******************************************************************************************/
    /**
     * @brief  Gets the effect red value
     *
     * @returns Color value
     *
     */
    virtual float getEffectR();

    /*******************************************************************************************/
    /**
     * @brief  Clear effect red value
     *
     */
    virtual void clearEffectR();

    /*******************************************************************************************/
    /**
     * @brief Sets the effect green value
     *
     * @param value Color value
     *
     */
    virtual void setEffectG(float value);

    /*******************************************************************************************/
    /**
     * @brief Gets the effect green value
     *
     * @returns Color value
     *
     */
    virtual float getEffectG();

    /*******************************************************************************************/
    /**
     * @brief  Clear effect green value
     *
     */
    virtual void clearEffectG();

    /*******************************************************************************************/
    /**
     * @brief Sets the blue value
     *
     * @param value Color value
     *
     */
    virtual void setEffectB(float value);

    /*******************************************************************************************/
    /**
     * @brief Gets the blue value
     *
     * @returns Color value
     *
     */
    virtual float getEffectB();

    /*******************************************************************************************/
    /**
     * @brief  Clear effect blue value
     *
     */
    virtual void clearEffectB();

    /*******************************************************************************************/
    /**
     * @brief Sets the alpha value
     *
     * @param value Color value
     *
     */
    virtual void setEffectAlpha(float value);

    /*******************************************************************************************/
    /**
     * @brief Gets the alpha value
     *
     * @returns Color value
     *
     */
    virtual float getEffectAlpha();

    /*******************************************************************************************/
    /**
     * @brief  Clear effect alpha value
     *
     */
    virtual void clearEffectAlpha();

    /*******************************************************************************************/
    /**
     * @brief  Should this element have a border
     *
     * @param _border Show this border or not
     *
     */
    virtual void showBorder(bool _border = true);

    /*******************************************************************************************/
    /**
     * @brief  Sets the border thickness
     *
     * @param thickness Thickness of the border in GUI space pixels
     *
     */
    virtual void setBorderSize(float _borderSize);

    /*******************************************************************************************/
    /**
     * @brief  Set the color for the border
     *
     * @param _color 4 component vector for the color
     *
     */
    virtual void setBorderColor(vec4 _color);

    /*******************************************************************************************/
    /**
     * @brief  Gets the current border color
     *
     * @returns 4 component vector of the color
     *
     */
    virtual vec4 getBorderColor();

    /*******************************************************************************************/
    /**
     * @brief  Sets the border red value
     *
     * @param value Color value
     *
     */
    virtual void setBorderR(float value);

    /*******************************************************************************************/
    /**
     * @brief  Gets the border red value
     *
     * @returns Color value
     *
     */
    virtual float getBorderR();

    /*******************************************************************************************/
    /**
     * @brief Sets the border green value
     *
     * @param value Color value
     *
     */
    virtual void setBorderG(float value);

    /*******************************************************************************************/
    /**
     * @brief Gets the border green value
     *
     * @returns Color value
     *
     */
    virtual float getBorderG();

    /*******************************************************************************************/
    /**
     * @brief Sets the border blue value
     *
     * @param value Color value
     *
     */
    virtual void setBorderB(float value);

    /*******************************************************************************************/
    /**
     * @brief Gets the border blue value
     *
     * @returns Color value
     *
     */
    virtual float getBorderB();

    /*******************************************************************************************/
    /**
     * @brief Sets the border alpha value
     *
     * @param value Color value
     *
     */
    virtual void setBorderAlpha(float value);

    /*******************************************************************************************/
    /**
     * @brief Gets the border alpha value
     *
     * @returns Color value
     *
     */
    virtual float getBorderAlpha();

    /*******************************************************************************************/
    /**
     * @brief  Sets the z ordering (order of drawing, higher is drawing on top of lower)
     *
     * @param z new Z ordering value (expected to be greater then 0 and the parent container)
     *
     */
    virtual void setZOrdering(int z);

    /*******************************************************************************************/
    /**
     * @brief  Gets the z ordering (order of drawing - higher is drawing on top of lower)
     *
     * @returns current Z ordering value
     *
     */
    virtual int getZOrdering();

    /*******************************************************************************************/
    /**
     * @brief  Set the z ordering of this element to be higher than the given element
     *
     * @param element Element to be below this one
     *
     */
    virtual void renderAbove(IMElement* element);

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
    virtual void renderBelow(IMElement* element);

    /*******************************************************************************************/
    /**
     * @brief  Show or hide this element
     *
     * @param _show Show this element or not
     *
     */
    virtual void setVisible(bool _show);

    /*******************************************************************************************/
    /**
     * @brief Should this element be including in the container clipping?
     *
     * @param _clip Clip this element or not
     *
     */
    virtual void setClip(bool _clip);

    /*******************************************************************************************/
    /**
     * @brief  Rather counter-intuitively, this draws this object on the screen
     *
     * @param drawOffset Absolute offset from the upper lefthand corner (GUI space)
     * @param clipPos pixel location of upper lefthand corner of clipping region
     * @param clipSize size of clipping region
     *
     */
    virtual void render(vec2 drawOffset, vec2 currentClipPos, vec2 currentClipSize);

    /*******************************************************************************************/
    /**
     * @brief  Get the element position on screen.
     *
     */
    virtual vec2 getScreenPosition();

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
    virtual bool pointInElement(vec2 drawOffset, vec2 point);

    /*******************************************************************************************/
    /**
     * @brief  Add an update behavior
     *
     * @param behavior Handle to behavior in question
     * @param behaviorName name to identify the behavior
     *
     */
    virtual void addUpdateBehavior(IMUpdateBehavior* behavior, std::string const& behaviorName = "");

    /*******************************************************************************************/
    /**
     * @brief  Removes a named update behavior
     *
     * @param behaviorName name to identify the behavior
     *
     * @returns true if there was a behavior to remove, false otherwise
     *
     */
    virtual bool removeUpdateBehavior(std::string const& behaviorName);

    /*******************************************************************************************/
    /**
     * @brief Indicates if a behavior exists, can be used to see if its finished.
     *
     * @param behaviorName name to identify the behavior
     *
     * @returns true if there was a behavior
     *
     */
    virtual bool hasUpdateBehavior(std::string const& behaviorName);

    /*******************************************************************************************/
    /**
     * @brief  Clear update behaviors
     *
     */
    virtual void clearUpdateBehaviors();

    /*******************************************************************************************/
    /**
     * @brief  Add a mouse over behavior
     *
     * @param behavior Handle to behavior in question
     * @param behaviorName name to identify the behavior
     *
     */
    virtual void addMouseOverBehavior(IMMouseOverBehavior* behavior, std::string const& behaviorName = "");

    /*******************************************************************************************/
    /**
     * @brief  Removes a named update behavior
     *
     * @param behaviorName name to identify the behavior
     *
     * @returns true if there was a behavior to remove, false otherwise
     *
     */
    virtual bool removeMouseOverBehavior(std::string const& behaviorName);

    /*******************************************************************************************/
    /**
     * @brief  Clear mouse over behaviors
     *
     */
    virtual void clearMouseOverBehaviors();

    /*******************************************************************************************/
    /**
     * @brief  Add a click behavior
     *
     * @param behavior Handle to behavior in question
     * @param behaviorName name to identify the behavior
     *
     */
    virtual void addLeftMouseClickBehavior(IMMouseClickBehavior* behavior, std::string const& behaviorName = "");

    /*******************************************************************************************/
    /**
     * @brief  Removes a named click behavior
     *
     * @param behaviorName name to identify the behavior
     *
     * @returns true if there was a behavior to remove, false otherwise
     *
     */
    virtual bool removeLeftMouseClickBehavior(std::string const& behaviorName);

    /*******************************************************************************************/
    /**
     * @brief  Clear mouse over behaviors
     *
     */
    virtual void clearLeftMouseClickBehaviors();

    /*******************************************************************************************/
    /**
     * @brief  Should mouse down apply to this elements children?
     *
     * @param send true if should (default)
     *
     */
    void sendMouseDownToChildren(bool send = true) {
        mouseDownForChildren = send;
    }

    /*******************************************************************************************/
    /**
     * @brief  Should mouse over apply to this elements children?
     *
     * @param true if should (default)
     *
     */
    void sendMouseOverToChildren(bool send = true) {
        mouseOverForChildren = send;
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
    virtual void update(uint64_t delta, vec2 drawOffset, GUIState& guistate);

    /*******************************************************************************************/
    /**
     * @brief  When this element is resized, moved, etc propagate this signal upwards
     *
     */
    virtual void onRelayout();

    /*******************************************************************************************/
    /**
     * @brief  When this element has an error, propagate it upwards
     *
     * @param newError Error message
     *
     */
    virtual void onError(std::string const& newError);

    /*******************************************************************************************/
    /**
     * @brief  When a resize, move, etc has happened do whatever is necessary
     *
     */
    virtual void doRelayout();

    /*******************************************************************************************/
    /**
     * @brief  Do whatever is necessary when the resolution changes
     *
     */
    virtual void doScreenResize();

    /*******************************************************************************************/
    /**
     * @brief Set the name of this element
     *
     * @param _name New name (incumbent on the programmer to make sure they're unique)
     *
     */
    virtual void setName(std::string const& _name);

    /*******************************************************************************************/
    /**
     * @brief Gets the name of this element
     *
     * @returns name of this element
     *
     */
    virtual std::string getName();

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
    virtual void setPadding(float U, float D, float L, float R);

    /*******************************************************************************************/
    /**
     * @brief  Set the padding above the element
     *
     * @param paddingSize The number of pixels (in GUI space) for the padding
     *
     */
    virtual void setPaddingU(float paddingSize);

    /*******************************************************************************************/
    /**
     * @brief  Set the padding below the element
     *
     * @param paddingSize The number of pixels (in GUI space) for the padding
     *
     */
    virtual void setPaddingD(float paddingSize);

    /*******************************************************************************************/
    /**
     * @brief  Set the padding to the left of the element
     *
     * @param paddingSize The number of pixels (in GUI space) for the padding
     *
     */
    virtual void setPaddingL(float paddingSize);

    /*******************************************************************************************/
    /**
     * @brief  Set the padding to the right of the element
     *
     * @param paddingSize The number of pixels (in GUI space) for the padding
     *
     */
    virtual void setPaddingR(float paddingSize);

    /*******************************************************************************************/
    /**
     * @brief  Sets the drawing displacement (mostly used for tweening)
     *
     * @param newDisplacement newValues for the displacement
     *
     */
    virtual void setDisplacement(vec2 newDisplacement = vec2(0, 0));

    /*******************************************************************************************/
    /**
     * @brief  Sets the drawing displacement x component (mostly used for tweening)
     *
     * @param newDisplacement newValues for the displacement
     *
     */
    virtual void setDisplacementX(float newDisplacement = 0);

    /*******************************************************************************************/
    /**
     * @brief  Sets the drawing displacement y component (mostly used for tweening)
     *
     * @param newDisplacement newValues for the displacement
     *
     */
    virtual void setDisplacementY(float newDisplacement = 0);

    /*******************************************************************************************/
    /**
     * @brief  Gets the drawing displacement (mostly used for tweening)
     *
     * @returns Displacement vector
     *
     */
    virtual vec2 getDisplacement(vec2 newDisplacement = vec2(0, 0));

    /*******************************************************************************************/
    /**
     * @brief  Gets the drawing displacement x component (mostly used for tweening)
     *
     * @returns Displacement value
     *
     */
    virtual float getDisplacementX();

    /*******************************************************************************************/
    /**
     * @brief  Gets the drawing displacement y component (mostly used for tweening)
     *
     * @returns Displacement value
     *
     */
    virtual float getDisplacementY();

    /*******************************************************************************************/
    /**
     * @brief  Sets the default size
     *
     * @param newDefault the new default size
     *
     */
    virtual void setDefaultSize(vec2 newDefault);

    /*******************************************************************************************/
    /**
     * @brief  Retrieves the default size
     *
     * @returns 2d integer vector of the default size
     *
     */
    virtual vec2 getDefaultSize();

    /*******************************************************************************************/
    /**
     * @brief  For container type classes - resize event, called internally
     *
     */
    virtual void onChildResize(IMElement* child);

    /*******************************************************************************************/
    /**
     * @brief  For container type classes - resize event, called internally
     *
     */
    virtual void onParentResize();

    /*******************************************************************************************/
    /**
     * @brief  Sets the size of the region (not including padding)
     *
     * @param _size 2d size vector (-1 element implies undefined - or use UNDEFINEDSIZE)
     *
     */
    virtual void setSize(const vec2 _size);

    /*******************************************************************************************/
    /**
     * @brief  Sets the x dimension of a region
     *
     * @param x x dimension size (-1 implies undefined - or use UNDEFINEDSIZE)
     *
     */
    virtual void setSizeX(const float x);

    /*******************************************************************************************/
    /**
     * @brief  Sets the y dimension of a region
     *
     * @param y y dimension size (-1 implies undefined - or use UNDEFINEDSIZE)
     *
     */
    virtual void setSizeY(const float y);

    /*******************************************************************************************/
    /**
     * @brief  Gets the size vector
     *
     * @returns The size vector
     *
     */
    virtual vec2 getSize();

    /*******************************************************************************************/
    /**
     * @brief  Gets the size x component
     *
     * @returns The x size
     *
     */
    virtual float getSizeX();

    /*******************************************************************************************/
    /**
     * @brief  Gets the size y component
     *
     * @returns The y size
     *
     */
    virtual float getSizeY();

    /*******************************************************************************************/
    /**
     * @brief  Sends a message to the owning GUI
     *
     * @param theMessage the message
     *
     */
    virtual void sendMessage(IMMessage* theMessage);

    /*******************************************************************************************/
    /**
     * @brief  Finds an element by a given name
     *
     * @param elementName the name of the element
     *
     * @returns handle to the element (NULL if not found)
     *
     */
    virtual IMElement* findElement(std::string const& elementName);

    IMElement* getParent() const {
        if (parent) parent->AddRef();
        return parent;
    }

    /*******************************************************************************************/
    /**
     * @brief  Remove all referenced object without releaseing references
     *
     */
    virtual void clense();

    virtual void setPauseBehaviors(bool pause);

    bool isMouseOver();
    void setScriptMouseOver(bool mouseOver);

    /*******************************************************************************************/
    /**
     * @brief  Destructor
     *
     */
    virtual ~IMElement();

    static void DestroyQueuedIMElements();
    void DestroyedIMElement(IMElement* element) override;
    void DestroyedIMGUI(IMGUI* imgui) override;
};
