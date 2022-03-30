///
/// @file WebMenuItem.h
///
/// @brief The header for the WebMenuItem class.
///
/// @author
///
/// This file is a part of Awesomium, a Web UI bridge for native apps.
///
/// Website: <http://www.awesomium.com>
///
/// Copyright (C) 2014 Awesomium Technologies LLC. All rights reserved.
/// Awesomium is a trademark of Awesomium Technologies LLC.
///
#ifndef AWESOMIUM_WEB_MENU_ITEM_H_
#define AWESOMIUM_WEB_MENU_ITEM_H_
#pragma once

#include <Awesomium/Platform.h>
#include <Awesomium/WebString.h>

namespace Awesomium {

///
/// Enumeration of the different menu item types.
/// For popup (dropdown) menus, you will usually only
/// need to handle kTypeOption and kTypeGroup.
///
enum WebMenuItemType {
  kWebMenuItemType_Option,          ///< Generic Option eg, <option>
  kWebMenuItemType_CheckableOption, ///< Option with a Checkbox
  kWebMenuItemType_Group,           ///< Group Label eg, <optgroup label="xxx">
  kWebMenuItemType_Separator        ///< Separator
};

///
/// @brief  Represents an item in a menu. This is used for Popup Menus.
///
#pragma pack(push)
#pragma pack(1)
struct OSM_EXPORT WebMenuItem {
  WebMenuItem();

  /// The type of this item
  WebMenuItemType type;

  /// The label to display to users
  WebString label;

  /// The text to display when users hover over this item
  WebString tooltip;

  /// Action ID (used only for ContextMenu)
  unsigned int action;

  /// Whether or not text should be displayed right-to-left
  bool right_to_left;

  /// Whether or not text direction has been overriden.
  bool has_directional_override;

  /// Whether or not this menu option is enabled.
  bool enabled;

  /// Whether or not this menu option is checked (only used with
  /// kTypeCheckableOption).
  bool checked;
};
#pragma pack(pop)

template<class T>
class WebVector;

///
/// @brief  An array of WebMenuItems
///
class OSM_EXPORT WebMenuItemArray {
 public:
  WebMenuItemArray();
  explicit WebMenuItemArray(unsigned int n);
  WebMenuItemArray(const WebMenuItemArray& rhs);
  ~WebMenuItemArray();

  WebMenuItemArray& operator=(const WebMenuItemArray& rhs);

  /// The size of the array
  unsigned int size() const;

  /// Get the item at a specific index
  WebMenuItem& At(unsigned int idx);

  /// Get the item at a specific index
  const WebMenuItem& At(unsigned int idx) const;

  /// Get the item at a specific index
  WebMenuItem& operator[](unsigned int idx);

  /// Get the item at a specific index
  const WebMenuItem& operator[](unsigned int idx) const;

  /// Add an item to the end of the array
  void Push(const WebMenuItem& item);

 protected:
  WebVector<WebMenuItem>* vector_;
};
}

#endif  // AWESOMIUM_WEB_MENU_ITEM_H_
