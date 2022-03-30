//-----------------------------------------------------------------------------
//           Name: controller.as
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

// Common items for all menus in the system

bool list_created = false; // Backwards compat. Will keep old scripts compiling

float border_size = 10.0f;
vec4 border_color = vec4(0.75f,0.75f,0.75f,0.75f);
vec4 border_color_locked = vec4(1.0f,1.0f,1.0f,1.0f);
bool controller_active = GetControllerActive();
vec2 mouse_position;
bool controller_wraparound = false;
float input_interval_timer = 0.0f;
float input_longpress_timer = 0.0f;
float input_interval = 0.1f;
float input_longpress_threshold = 0.5f;
bool first_run = true;
int old_controller_index = -1;

// This has only been kept for backwards compatibility, since these are now
// found in IMElement::controllerItem
class ControllerItem
{
	IMElement@ element;
	IMMessage@ message = null;
	IMMessage@ message_left = null;
	IMMessage@ message_right = null;
	IMMessage@ message_up = null;
	IMMessage@ message_down = null;
	IMMessage@ message_on_select = null;

	bool execute_on_select = false; // Backwards compat

	ControllerItem(){}
};

// The GUI has 3 main areas, these are used to make it possible to switch between them
enum Area { HEADER, MAIN, FOOTER };
Area current_area = MAIN;

enum Direction {
	UP,
	DOWN,
	LEFT,
	RIGHT
};
enum FirstLast { FIRST, LAST };
enum Component { X, Y, XY };
enum CheckDirection { FORWARDS, BACKWARDS };

IMElement@ current_item = null;
bool current_item_locked = false; // If the user has pressed enter on a controller item without a message

// When removing elements, the last active controller is stored.
// Often, elements are removed and then added back (e.g. the settings menu).
// When this happens, we want the user's selection to be restored
bool controller_stored = false;
string last_active_controller;

// Mostly kept for backwards compat
const int kMainController = 1;
const int kSubmenuControllerItems = 2;
int current_controller_item_state = kMainController;

array<IMElement@> controller_items;
array<IMElement@> sub_controller_items;

bool controller_paused = false;

bool GetControllerActive() {
	return GetInterlevelData("controller_active") == "true";
}

int controller_player = 0;
void SetControllerPlayer(int player) {
	controller_player = player;
}

// Used for dropdown menus. Restricts the selectable controller items to
// anything added after this function has been called
void EnableControllerSubmenu() {
	StoreActiveController();
	current_controller_item_state = kSubmenuControllerItems;
}

void DisableControllerSubmenu() {
	//Log(info, "Disabling submenu");
	if(controller_stored) {
		//Log(info, "Disabling and restoring " + last_active_controller);
		SetCurrentControllerItem(last_active_controller);
	}
	controller_stored = false;
	current_controller_item_state = kMainController;
	sub_controller_items.resize(0);
}

void ResetController() {
	//Log(info, "ResetController");
	@current_item = null;
	current_item_locked = false;
	controller_active = false;
	controller_stored = false;
	controller_items.resize(0);
	sub_controller_items.resize(0);
	current_controller_item_state = kMainController;
	current_area = MAIN;
	first_run = true;
}

// If "message" isn't given, message_<direction> can be used to control what
// message is sent when menu keys are pressed after the user has selected this
// controller item by pressing enter.
// Signature is kept for backwards compat
void AddControllerItem(IMElement@ element, IMMessage@ message, IMMessage@ message_left = null, IMMessage@ message_right = null, IMMessage@ message_up = null, IMMessage@ message_down = null){
	//Log(info, "Adding controller item");
	element.controllerItem.setMessages(message, null, message_left, message_right, message_up, message_down);
	if(current_controller_item_state == kMainController) {
		controller_items.insertLast(element);
	} else {
		sub_controller_items.insertLast(element);
	}
	// Restore last active element if it was stored
	if(controller_stored && last_active_controller == element.getName()) {
		//Log(info, "Restoring previous active controller");
		SetItemActive(element, false);
	} else {
		if(controller_stored) {
			//Log(info, element.getName() + " is not old stored element " + last_active_controller);
		}
	}
}

void AddControllerItem(ControllerItem @item){
	AddControllerItem(item.element, item.message, item.message_left, item.message_right, item.message_up, item.message_down);
	item.element.controllerItem.setMessageOnSelect(item.message_on_select);
}

// Also stores the currently active controller item if it is removed
void ClearControllerItems(int start_at = 0){
	//Log(info, "Clearing controller items starting at " + start_at);
	if(current_item !is null && current_controller_item_state == kMainController) {
		bool store_active = true;
		for(int i = 0; i < start_at; ++i) {
			if(controller_items[i].getName() == current_item.getName()) {
				//Log(info, "NOT storing currently active item");
				store_active = false;
			}
		}

		if(store_active) {
			StoreActiveController();
		} else {
			controller_stored = false;
		}
	} else {
		controller_stored = false;
	}
	if(start_at < int(controller_items.size())) {
		controller_items.removeRange(start_at, controller_items.size());
	} else {
		Log(warning, "Tried clearing controller items starting at index " + start_at + ", but there are only " + controller_items.size() + " controller items available");
	}
}

void StoreActiveController() {
	if(controller_active && current_item !is null) {
		last_active_controller = current_item.getName();
		if(last_active_controller.length() > 0) {
			controller_stored = true;
			//Log(info, "Storing last active controller \"" + last_active_controller + "\"");
		} else {
			controller_stored = false;
			//Log(info, "Was going to store active controller, but its name is empty");
		}

		@current_item = null;
	}
}

// Kept for backwards compat
void SetControllerItemBeforeShift(){
    ClearControllerItems();
}

// Kept for backwards compat. Not needed since this is now handled automatically
void SetControllerItemAfterShift(int direction){ }

void SetCurrentControllerItem(uint index){
	if(controller_active) {
		current_item_locked = false;
		if(current_controller_item_state == kMainController) {
			//Log(info, "Setting current controller item to index " + index + " in main items");
			SetItemActive(controller_items[index], false);
		} else {
			//Log(info, "Setting current controller item to index " + index + " in sub items");
			SetItemActive(sub_controller_items[index], false);
		}
	}
}

void SetCurrentControllerItem(string name){
	if(controller_active) {
		for(uint i = 0; i < controller_items.size(); ++i) {
			if(controller_items[i].getName() == name) {
				SetItemActive(controller_items[i], false);
				break;
			}
		}
	}
}

// Kept for backwards compat
int GetCurrentControllerItemIndex(){
	return current_controller_item_state;
}

// Kept for backwards compat. This seems to be seldom used
string GetCurrentControllerItemName(){
	if(current_item !is null) {
		return current_item.getName();
	} else {
		return "";
	}
}

void SetItemActive(IMElement@ element, bool trigger_on_select = true){
	@current_item = @element;
	current_item.setMouseOver(true);
	if(current_item_locked) {
		element.setBorderColor(border_color_locked);
	} else {
		element.setBorderColor(border_color);
	}
	element.setBorderSize(border_size);
	if(!element.controllerItem.skip_show_border) {
		element.showBorder(true);
	}
	if(trigger_on_select && element.controllerItem.getMessageOnSelect() !is null) {
		//Log(info, "Message on select");
		element.sendMessage(element.controllerItem.getMessageOnSelect());
	}
}

// Note that this does not set current_item to null
void DeactivateCurrentItem(){
	current_item.setMouseOver(false);
	if(current_item !is null) {
		if(!current_item.controllerItem.skip_show_border){
			current_item.showBorder(false);
		}
	}
}

// Draws boxes around controller items. Good for debugging
void DrawBoxes() {
    for(uint i = 0; i < controller_items.size(); i++){
        vec2 new_position = controller_items[i].getScreenPosition();
		float scaleX = screenMetrics.GUItoScreenXScale;
		float scaleY = screenMetrics.GUItoScreenYScale;
		vec2 size = controller_items[i].getSize();
		size.x *= scaleX;
		size.y *= scaleY;
        imGUI.drawBox(new_position, size, vec4(1,0,0,1), 9);
        //current_controller_items[i].position = vec2(new_position.x + (current_controller_items[i].element.getSizeX() / 2.0f), new_position.y + (current_controller_items[i].element.getSizeY() / 2.0f));
    }
}

// Called when the user has pressed enter on an IMElement without a message,
// "locking" it. Useful for sliders
void UpdateCurrentItem() {
	if(GetInputPressed(controller_player, "menu_return")) {
		//Log(info, "Return was pressed");
		current_item_locked = false;
		current_item.setBorderColor(border_color);
	} else {
		if(input_longpress_timer > input_longpress_threshold) {
			if(input_interval_timer > 0.0f) {
				input_interval_timer -= time_step;
				return;
			}
			
			if(GetInputDown(controller_player, "menu_up")) {
				if(current_item.controllerItem.getMessageUp() !is null) {
					current_item.sendMessage(current_item.controllerItem.getMessageUp());
				} else {
					current_item_locked = false;
					GetNextItem(Direction::UP);
				}
				input_interval_timer = input_interval;
			} else if(GetInputDown(controller_player, "menu_down")) {
				if(current_item.controllerItem.getMessageDown() !is null) {
					current_item.sendMessage(current_item.controllerItem.getMessageDown());
				} else {
					current_item_locked = false;
					GetNextItem(Direction::DOWN);
				}
				input_interval_timer = input_interval;
			} else if(GetInputDown(controller_player, "menu_left")) {
				if(current_item.controllerItem.getMessageLeft() !is null) {
					current_item.sendMessage(current_item.controllerItem.getMessageLeft());
				} else {
					current_item_locked = false;
					GetNextItem(Direction::LEFT);
				}
				input_interval_timer = input_interval;
			} else if(GetInputDown(controller_player, "menu_right")) {
				if(current_item.controllerItem.getMessageRight() !is null) {
					current_item.sendMessage(current_item.controllerItem.getMessageRight());
				} else {
					current_item_locked = false;
					GetNextItem(Direction::RIGHT);
				}
				input_interval_timer = input_interval;
			} else {
				input_longpress_timer = 0.0f;
			}
		} else {
			input_longpress_timer += time_step;
			if(GetInputPressed(controller_player, "menu_up")) {
				if(current_item.controllerItem.getMessageUp() !is null) {
					current_item.sendMessage(current_item.controllerItem.getMessageUp());
				} else {
					current_item_locked = false;
					GetNextItem(Direction::UP);
				}
				input_interval_timer = input_interval;
			} else if(GetInputPressed(controller_player, "menu_down")) {
				if(current_item.controllerItem.getMessageDown() !is null) {
					current_item.sendMessage(current_item.controllerItem.getMessageDown());
				} else {
					current_item_locked = false;
					GetNextItem(Direction::DOWN);
				}
				input_interval_timer = input_interval;
			} else if(GetInputPressed(controller_player, "menu_left")) {
				if(current_item.controllerItem.getMessageLeft() !is null) {
					current_item.sendMessage(current_item.controllerItem.getMessageLeft());
				} else {
					current_item_locked = false;
					GetNextItem(Direction::LEFT);
				}
				input_interval_timer = input_interval;
			} else if(GetInputPressed(controller_player, "menu_right")) {
				if(current_item.controllerItem.getMessageRight() !is null) {
					current_item.sendMessage(current_item.controllerItem.getMessageRight());
				} else {
					current_item_locked = false;
					GetNextItem(Direction::RIGHT);
				}
				input_interval_timer = input_interval;
			} else if(!GetInputDown(controller_player, "menu_up") && !GetInputDown(controller_player, "menu_down") && !GetInputDown(controller_player, "menu_left") && !GetInputDown(controller_player, "menu_right")) {
				input_longpress_timer = 0.0f;
			}
		}
	}
}

// General update function. Called when the user hasn't "locked in" an element
void UpdateInput() {
	if(GetInputPressed(controller_player, "menu_return")) {
		//Log(info, "Return was pressed");
		if(current_item !is null && current_item.controllerItem.isActive()) {
			if(current_item.controllerItem.getMessage() !is null) {
				//Log(info, "Sending message");
				current_item.sendMessage(current_item.controllerItem.getMessage());
			} else {
				//Log(info, "Locking item");
				current_item_locked = true;
				current_item.setBorderColor(border_color_locked);
			}
		}
	} else {
		if(input_longpress_timer > input_longpress_threshold) {
			if(input_interval_timer > 0.0f) {
				input_interval_timer -= time_step;
				return;
			}
			
			if(GetInputDown(controller_player, "menu_up")) {
				GetNextItem(Direction::UP);
				input_interval_timer = input_interval;
			} else if(GetInputDown(controller_player, "menu_down")) {
				GetNextItem(Direction::DOWN);
				input_interval_timer = input_interval;
			} else if(GetInputDown(controller_player, "menu_left")) {
				GetNextItem(Direction::LEFT);
				input_interval_timer = input_interval;
			} else if(GetInputDown(controller_player, "menu_right")) {
				GetNextItem(Direction::RIGHT);
				input_interval_timer = input_interval;
			} else {
				input_longpress_timer = 0.0f;
			}
		} else {
			input_longpress_timer += time_step;

			if(GetInputPressed(controller_player, "menu_up")) {
				GetNextItem(Direction::UP);
				input_interval_timer = input_interval;
			} else if(GetInputPressed(controller_player, "menu_down")) {
				GetNextItem(Direction::DOWN);
				input_interval_timer = input_interval;
			} else if(GetInputPressed(controller_player, "menu_left")) {
				GetNextItem(Direction::LEFT);
				input_interval_timer = input_interval;
			} else if(GetInputPressed(controller_player, "menu_right")) {
				GetNextItem(Direction::RIGHT);
				input_interval_timer = input_interval;
			} else if(!GetInputDown(controller_player, "menu_up") && !GetInputDown(controller_player, "menu_down") && !GetInputDown(controller_player, "menu_left") && !GetInputDown(controller_player, "menu_right")) {
				input_longpress_timer = 0.0f;
			}
		}
	}
}

void UpdateController() {
	if(first_run) {
		mouse_position = imGUI.guistate.mousePosition;
		controller_active = GetInterlevelData("controller_active") == "true";
		if(controller_active) {
			GetNextItem(Direction::DOWN);
		}
		first_run = false;
	}
	if(controller_paused) {
		return;
	}
	if(controller_active) {
		if(mouse_position != imGUI.guistate.mousePosition){
			DeactivateCurrentItem();
			if(current_controller_item_state != kMainController) {
				SetCurrentControllerItem(last_active_controller);
				DeactivateCurrentItem();
			}
			SetGrabMouse(false);
			SetInterlevelData("controller_active", "false");
			current_item_locked = false;
			controller_stored = false;
			@current_item = null;
			controller_active = false;
		} else {
			if(current_item_locked) {
				UpdateCurrentItem();
			} else {
				UpdateInput();
			}
		}
	} else if(GetInputPressed(controller_player, "menu_up") || GetInputPressed(controller_player, "menu_down") || GetInputPressed(controller_player, "menu_left") || GetInputPressed(controller_player, "menu_right")) {
		imGUI.receiveMessage(IMMessage("close_all_open_menus"));
		if(current_controller_item_state != kMainController) {
			DisableControllerSubmenu();
		}
		if(current_item !is null) {
			SetItemActive(current_item);
		}
		mouse_position = imGUI.guistate.mousePosition;
		controller_active = true;
		SetInterlevelData("controller_active", "true");
		SetGrabMouse(true);

		if(current_item_locked) {
			UpdateCurrentItem();
		} else {
			UpdateInput();
		}
	}
}

// Recursively goes through element and finds the first or last element inside it.
// First means any dividers will be gone throuh from 0 to the max size, and that
// floating elements will be looking for the element closest to (0, 0)
// Last means any dividers will be gone through from max to 0, and that floating
// elements will be looking for the element closest to (0, 0)
IMElement@ GetClosestElement(IMElement@ element, vec2 position) {
	if(element is null)
		return null;
	string type = element.getElementTypeName();

	if(element.controllerItem.isActive()) {
		//Log(info, "Found element " + element.getName() + " (" + type + "): " + element.controllerItem.getMessage().name);
		return element;
	}

	if(type == "Divider") {
		IMDivider@ divider = cast<IMDivider>(element);
		/*if(divider.getOrientation() == DOVertical) {
			Log(info, "Getting first element of " + divider.getName() + " (vertical divider) with " + divider.getContainerCount() + " children");
		} else {
			Log(info, "Getting first element of " + divider.getName() + " (horizontal divider) with " + divider.getContainerCount() + " children");
		}*/

		array<IMContainer@> @containers = divider.getContainers();
		IMElement@ closest = null;
		for(uint i = 0; i < containers.size(); ++i) {
			IMElement@ found = GetClosestElement(containers[i].getContents(), position);
			if(!LHSClosest(position, closest, found, XY)) {
				@closest = found;
			}
		}

		return closest;
	} else if(type == "Container") {
		IMContainer@ container = cast<IMContainer>(element);
		IMElement@ contents = container.getContents(); 
		IMElement@ potentialNext = null;
		if(contents !is null) {
			//Log(info, "Getting first element of " + container.getContents().getName() + " (container)");
			@potentialNext = GetClosestElement(contents, position);
		}/* else {
			Log(info, "Getting first element of " + container.getName() + " (container without contents)");
		}*/

		if(potentialNext is null) {
			array<IMElement@> @floatingChildren = container.getFloatingContents();
			//Log(info, "Container has " + floatingChildren.size() + " floating elements");
			if(floatingChildren.size() > 0) {
				while(floatingChildren.size() > 0) {
					IMElement@ closest = floatingChildren[0];
					uint found_index = 0;
					for(uint i = 1; i < floatingChildren.size(); ++i) {
						if(!LHSClosest(position, closest, floatingChildren[i], XY)) {
							@closest = floatingChildren[i];
							found_index = i;
						}
					}
					//Log(info, "Closest is at " + closest.getScreenPosition().x + ", " + closest.getScreenPosition().y);
					@potentialNext = GetClosestElement(closest, position);
					if(potentialNext !is null)
						return potentialNext;

					floatingChildren.removeAt(found_index);
				}
			}
		}

		return potentialNext;
	} else if(type == "Spacer") {
		//Log(info, "Found spacer, skipping");
		return null;
	} else {
		//Log(info, "Leaf without controller item: " + type);
		return null;
	}
}

// Checks whether or not lhs is closed than rhs to position.
// component controls which component is checked
bool LHSClosest(vec2 position, IMElement@ lhs, IMElement@ rhs, Component component) {
	if(rhs is null)
		return true;
	if(lhs is null)
		return false;

	if(component == X) {
		if(abs(lhs.getScreenPosition().x - position.x) <= abs(rhs.getScreenPosition().x - position.x)) {
			return true;
		} else {
			return false;
		}
	} else if (component == Y) {
		if(abs(lhs.getScreenPosition().y - position.y) <= abs(rhs.getScreenPosition().y - position.y)) {
			return true;
		} else {
			return false;
		}
	} else {
		if(length_squared(lhs.getScreenPosition() - position) <= length_squared(rhs.getScreenPosition() - position)) {
			return true;
		} else {
			return false;
		}
	}
}

// Checks if lhs has a higher "score" than rhs
// This is used when jumping between floating elements. A higher score means
// the element was a better fit in the given direction.
// I just made this up and it seems to work fine for our current GUI.
bool LHSHighestScore(Direction direction, vec2 position, IMElement@ lhs, IMElement@ rhs) {
	if(rhs is null)
		return true;
	if(lhs is null)
		return false;

	vec2 direction_vector(0.0f);
	switch(direction) {
		case Direction::UP: direction_vector.y = -1; break;
		case Direction::DOWN: direction_vector.y = 1; break;
		case Direction::LEFT: direction_vector.x = -1; break;
		case Direction::RIGHT: direction_vector.x = 1; break;
	}

	vec2 lhs_dir = lhs.getScreenPosition() - position;
	vec2 rhs_dir = rhs.getScreenPosition() - position;

	float lhs_dist = length_squared(lhs_dir);
	float rhs_dist = length_squared(rhs_dir);

	float lhs_dot = dot(normalize(lhs_dir), direction_vector);
	float rhs_dot = dot(normalize(rhs_dir), direction_vector);

	// Square to make slight offsets even larger
	lhs_dot = lhs_dot * lhs_dot;
	rhs_dot = rhs_dot * rhs_dot;

	//Log(info, "lhs: " + lhs.getName() + "lhs_dot = " + lhs_dot + ", rhs: " + rhs.getName() + "rhs_dot = " + rhs_dot);

	if(lhs_dot / lhs_dist >= rhs_dot / rhs_dist) {
		return true;
	} else {
		return false;
	}
}

// Recursively goes through element and find the element which is closest to pos,
// constrained by component.
// check_direction controls which direction elements are iterated over. See
// GetFirstLastElements for further comments.
IMElement@ GetElementMatchComponent(IMElement@ element, vec2 pos, Component component, CheckDirection check_direction) {
	if(element is null)
		return null;
	string type = element.getElementTypeName();

	if(element.controllerItem.isActive()) {
		//Log(info, "Found element at x = " + element.getScreenPosition().x + ", y = " + element.getScreenPosition().y + " " + element.getName() + " (" + type + ")");
		return element;
	}

	if(type == "Divider") {
		IMDivider@ divider = cast<IMDivider>(element);
		/*if(divider.getOrientation() == DOVertical) {
			Log(info, "Getting element matching pos " + divider.getName() + " (vertical divider) with " + divider.getContainerCount() + " children");
		} else {
			Log(info, "Getting element matching pos " + divider.getName() + " (horizontal divider) with " + divider.getContainerCount() + " children");
		}*/

		IMElement@ closest_child = null;
		int start_index = 0;
		int end_index = 0;
		int inc_value = 1;
		if(check_direction == BACKWARDS) {
			start_index = divider.getContainerCount() - 1;
			end_index = -1;
			inc_value = -1;
		} else if(check_direction == FORWARDS) {
			end_index = divider.getContainerCount();
			inc_value = 1;
		}
		for(int i = start_index; i != end_index; i += inc_value) {
			IMElement@ potential_child = GetElementMatchComponent(divider.getContainerAt(i), pos, component, check_direction);
			if(!LHSClosest(pos, closest_child, potential_child, component)) {
				@closest_child = @potential_child;
			}
		}
		return closest_child;
	} else if(type == "Container") {
		IMContainer@ container = cast<IMContainer>(element);
		IMElement@ contents = container.getContents(); 
		IMElement@ potentialNext = null;
		// Prioritize contents over floating elements. Arbitrary choice, could perhaps be swapped if needed
		if(contents !is null) {
			//Log(info, "Getting element matching pos " + container.getContents().getName() + " (container)");
			@potentialNext = GetElementMatchComponent(container.getContents(), pos, component, check_direction);
		}/* else {
			Log(info, "Getting element matching pos " + container.getName() + " (container without contents)");
		}*/

		if(potentialNext is null) {
			array<IMElement@> floating_contents = container.getFloatingContents();
			//Log(info, "Container has " + floating_contents.size() + " floating elements");
			IMElement@ closest_child = null;
			while(floating_contents.size() > 0) {
				// Find the IMElement closest to pos
				IMElement@ closest = floating_contents[0];
				uint found_index = 0;
				for(uint i = 1; i < floating_contents.size(); ++i) {
					IMElement@ candidate = floating_contents[i];
					//Log(info, "Comparing closest to " + floating_contents[i].getName());
					if(!LHSClosest(pos, closest, floating_contents[i], XY)) {
						@closest = floating_contents[i];
						found_index = i;
					}
				}

				// It was found, so check if there is a controller item 
				//Log(info, "Closest is " + closest.getName());
				IMElement@ potential_child = GetElementMatchComponent(closest, pos, component, check_direction);
				if(potential_child !is null) {
					//Log(info, "Returning potential child");
					return potential_child;
				}
				//Log(info, "Potential child was null");

				// No element found, so remove it from the list and find the
				// next closest
				floating_contents.removeAt(found_index);
			}
		}

		return potentialNext;
	} else if(type == "Spacer") {
		//Log(info, "Found spacer, skipping");
		return null;
	} else {
		//Log(info, "Leaf without controller item: " + type);
		return null;
	}
}

// Recursively gets the next element in the given direction.
// target is the element to look for, and when found we find whatever is next of
//  it.
// current is where we're looking for target (probably a divder or container)
// target_position is where the first target position was, it never changes.
// However, target and current change many times
IMElement@ GetNextElement(Direction direction, IMElement@ target, IMElement@ current, vec2 target_position) {
	if(current is null)
		return null;
	
	//Log(info, "Searching for " + target.getName());

	string type = current.getElementTypeName();
	if(type == "Divider") {
		// If the divider orientation matches the direction we're going, look
		// through it until we find our target, then go to the next element and
		// try to find a controller item inside of it.
		// If none can be found, we simply go to current's parent and look there
		IMDivider@ divider = cast<IMDivider>(current);
		if(divider.getOrientation() == DOVertical) {
			//Log(info, "Searching in " + current.getName() + " (vertical divider) with " + divider.getContainerCount() + " children");
			if(direction == UP || direction == DOWN) {
				for(int i = 0; i < int(divider.getContainerCount()); ++i) {
					IMContainer@ child = divider.getContainerAt(i);
					string name = child.getContents().getName();
					//Log(info, "child" + i + ": " + name);

					if(name == target.getName()) {
						//Log(info, "child" + i + " matches target");
						if(direction == UP) {
							for(--i; i >= 0; --i) {
								IMElement@ potential_next = divider.getContainerAt(i).getContents();
								//Log(info, "Potential next type: " + potential_next.getElementTypeName());
								if(potential_next.getElementTypeName() != "Spacer") {
									@potential_next = GetElementMatchComponent(potential_next, target_position, Component::X, BACKWARDS);
									if(potential_next !is null)
										return potential_next;
								}
							}
							break; // Break since i is reset to 0, avoid infinite loop
						} else if(direction == DOWN) {
							for(++i; i < int(divider.getContainerCount()); ++i) {
								IMElement@ potential_next = divider.getContainerAt(i).getContents();
								//Log(info, "Potential next type: " + potential_next.getElementTypeName());
								if(potential_next.getElementTypeName() != "Spacer") {
									@potential_next = GetElementMatchComponent(potential_next, target_position, Component::X, FORWARDS);
									if(potential_next !is null)
										return potential_next;
								}
							}
						}
					}
				}
			}
		} else {
			//Log(info, "Searching in " + current.getName() + " (horizontal divider) with " + divider.getContainerCount() + " children");
			if(direction == LEFT || direction == RIGHT) {
				for(int i = 0; i < int(divider.getContainerCount()); ++i) {
					IMContainer@ child = divider.getContainerAt(i);
					string name = child.getContents().getName();
					//Log(info, "child" + i + ": " + name);

					if(name == target.getName()) {
						//Log(info, "child" + i + " matches target");
						if(direction == LEFT) {
							for(--i; i >= 0; --i) {
								IMElement@ potential_next = divider.getContainerAt(i).getContents();
								//Log(info, "Potential next type: " + potential_next.getElementTypeName());
								if(potential_next.getElementTypeName() != "Spacer") {
									@potential_next = GetElementMatchComponent(potential_next, target_position, Component::Y, BACKWARDS); // TODO: This works when BACKWARDS, but logic suggests it should be FORWARDS
									if(potential_next !is null)
										return potential_next;
								}
							}
							break; // Break since i is reset to 0, avoid infinite loop
						} else if(direction == RIGHT) {
							for(++i; i < int(divider.getContainerCount()); ++i) {
								IMElement@ potential_next = divider.getContainerAt(i).getContents();
								//Log(info, "Potential next type: " + potential_next.getElementTypeName());
								if(potential_next.getElementTypeName() != "Spacer") {
									@potential_next = GetElementMatchComponent(potential_next, target_position, Component::Y, FORWARDS);
									if(potential_next !is null)
										return potential_next;
								}
							}
						}
					}
				}
			}
		}

		IMElement@ potential_parent = current.getParent();
		if(potential_parent is null)
			return null;

		return GetNextElement(direction, current, current.getParent(), target_position);
	} else if (type == "Container") {
		IMContainer@ container = cast<IMContainer>(current);
		// Go through all floating elements and try to find elements.
		// LHSHighestScore is used here.
		array<IMElement@> floating_contents = container.getFloatingContents();
		while(floating_contents.size() > 0) {
			IMElement@ closest = null;
			uint found_index = uint(-1);
			for(uint i = 0; i < floating_contents.size(); ++i) {
				IMElement@ candidate = floating_contents[i];
				bool valid = false;
				// I think LHSIsClosest might remove the need to do this
				// filtering, but it's implemented now, so whatever
				switch(direction) {
					case Direction::LEFT:
						if(candidate.getScreenPosition().x < target_position.x)
							valid = true;
						break;
					case Direction::RIGHT:
						if(candidate.getScreenPosition().x > target_position.x)
							valid = true;
						break;
					case Direction::UP:
						if(candidate.getScreenPosition().y < target_position.y)
							valid = true;
						break;
					case Direction::DOWN:
						if(candidate.getScreenPosition().y > target_position.y)
							valid = true;
						break;
				}
				if(valid) {
					if(!LHSHighestScore(direction, target_position, closest, floating_contents[i])) {
						@closest = floating_contents[i];
						found_index = i;
					}
				} else {
					floating_contents.removeAt(i);
					i--;
				}
			}

			if(found_index != uint(-1)) {
				//Log(info, "Closest is at " + closest.getScreenPosition().x + ", " + closest.getScreenPosition().y);
				IMElement@ potentialNext = GetClosestElement(closest, vec2(0.0f, 0.0f));
				if(potentialNext !is null)
					return potentialNext;

				floating_contents.removeAt(found_index);
			} else {
				break;
			}
		}
		if(container.getName().length() > 0) {
			//Log(info, "Searching in " + container.getName() + " (container with contents)");
			// A lot of elements are added to container and given an anonymous
			// container, so just ignore that container
			return GetNextElement(direction, current, container.getParent(), target_position);
		} else {
			//Log(warning, "Searching in container without name and with contents");
			return GetNextElement(direction, target, container.getParent(), target_position);
		}
	} else {
		//Log(info, "Ignoring type " + type);
	}

	return null;
}

Area GetNextArea() {
	switch(current_area) {
		case HEADER: return MAIN;
		case MAIN: return FOOTER;
		case FOOTER: return FOOTER;
		default: return MAIN;
	}
	return MAIN;
}

Area GetPreviousArea() {
	switch(current_area) {
		case HEADER: return HEADER;
		case MAIN: return HEADER;
		case FOOTER: return MAIN;
		default: return MAIN;
	}
	return MAIN;
}

IMElement@ AreaToElement(Area area) {
	switch(area) {
		case HEADER: return imGUI.getHeader();
		case MAIN: return imGUI.getMain();
		case FOOTER: return imGUI.getFooter();
		default: return imGUI.getMain();
	}
	return imGUI.getMain();;
}

void GetNextItem(Direction direction){
	if(current_item is null) {
		//Log(info, "Current_item is null");
		current_area = MAIN;
		IMElement@ next = GetClosestElement(imGUI.getMain(), vec2(0.0f, 0.0f));
		if(next !is null) {
			SetItemActive(next);
		}
	} else {
		//Log(info, "##############################");
		//Log(info, "Getting item closest to " + current_item.getName() + " (" + current_item.getElementTypeName() + ")");
		/*switch(direction) {
			case Direction::UP: Log(info, "Looking up"); break;
			case Direction::DOWN: Log(info, "Looking down"); break;
			case Direction::LEFT: Log(info, "Looking left"); break;
			case Direction::RIGHT: Log(info, "Looking right"); break;
		}*/
		IMElement@ parent = current_item.getParent();
		if(parent !is null) {
			//Log(info, "Parent is not null");
			IMElement@ next = GetNextElement(direction, current_item, current_item.getParent(), current_item.getScreenPosition());
			if(next is null) {
				Area next_area;
				// Header/main/footer are vertically stacked, so only up/down
				// can be used to cycle between them
				if(direction == UP) {
					//Log(info, "No element found, looking in previous area");
					next_area = GetPreviousArea();
				} else if(direction == DOWN) {
					//Log(info, "No element found, looking in next area");
					next_area = GetNextArea();
				}
				if(current_area != next_area) {
					@next = GetClosestElement(AreaToElement(next_area), current_item.getScreenPosition());
					if(@next !is null) {
						current_area = next_area;
					}
				}
			}
			if(next !is null) {
				//Log(info, "Found a next match: " + next.getName());
				bool allow = false;
				if(current_controller_item_state == kSubmenuControllerItems) {
					for(uint i = 0; i < sub_controller_items.size(); ++i) {
						if(@next == @sub_controller_items[i]) {
							allow = true;
							//Log(info, "match will be used");
							break;
						}
					}
				} else {
					allow = true;
				}
				if(allow) {
					DeactivateCurrentItem();
					SetItemActive(next);
				} else {
					//Log(info, "Current item is NOT in matching control group");
				}
			}
		}/* else {
			//Log(info, "Parent is null");
		}*/
	}
}

// Backwards compat, this is handled automatically now
void ExecuteOnSelect(){ }
