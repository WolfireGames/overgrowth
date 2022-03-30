///
/// @file WebTouchEvent.h
///
/// @brief The header for the WebTouchEvent struct.
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
#ifndef AWESOMIUM_WEB_TOUCH_EVENT_H_
#define AWESOMIUM_WEB_TOUCH_EVENT_H_
#pragma once

#include <Awesomium/Platform.h>

namespace Awesomium {

/// An enumeration of the different states for each touch-point.
enum WebTouchPointState {
  kWebTouchPointState_Undefined,
  kWebTouchPointState_Released,
  kWebTouchPointState_Pressed,
  kWebTouchPointState_Moved,
  kWebTouchPointState_Stationary,
  kWebTouchPointState_Cancelled,
};

#pragma pack(push)
#pragma pack(1)
///
/// @brief  Represents a single touch-point in a multi-touch event.
///
struct OSM_EXPORT WebTouchPoint {
  WebTouchPoint();

  /// The unique numeric ID of this touch-point (0 through 7)
  int id;

  /// The current state of this touch-point
  WebTouchPointState state;

  /// The horizontal coordinate (in pixels), relative to the screen.
  int screen_position_x;

  /// The vertical coordinate (in pixels), relative to the screen.
  int screen_position_y;

  /// The horizontal coordinate (in pixels), relative to the view.
  int position_x;

  /// The vertical coordinate (in pixels), relative to the view.
  int position_y;

  ///
  /// The radius of the ellipse which most closely circumscribes the touch
  /// area along the x-axis (in pixels). Set this to 1 if not known.
  ///
  int radius_x;

  ///
  /// The radius of the ellipse which most closely circumscribes the touch
  /// area along the y-axis (in pixels). Set this to 1 if not known.
  ///
  int radius_y;

  ///
  /// The angle (in degrees) that the ellipse described by radius_x and
  /// radius_y is rotated clockwise about its center; 0 if no value is known.
  /// The value must be greater than or equal to 0 and less than 90.
  ///
  float rotation_angle;

  ///
  /// A relative value of pressure applied, in the range 0 to 1, where 0 is
  /// no pressure, and 1 is the highest level of pressure the touch device is
  /// capable of sensing. Set this to 0 if not known.
  ///
  float force;
};
#pragma pack(pop)

/// The different types of WebTouchEvents
enum WebTouchEventType {
  /// Indicates a new touch on the surface has occurred 
  kWebTouchEventType_Start,
  /// Indicates one or more fingers have moved
  kWebTouchEventType_Move,
  /// Indicates a finger has been lifted from the surface
  kWebTouchEventType_End,
  /// Indicates a finger has moved into an invalid area
  kWebTouchEventType_Cancel,
};

#pragma pack(push)
#pragma pack(1)
///
/// @brief  A generic multi-touch event.
///
/// @see  WebView::InjectTouchEvent
///
struct OSM_EXPORT WebTouchEvent {
  WebTouchEvent();

  /// The type of this multi-touch event.
  WebTouchEventType type;

  /// The length of the touches array
  unsigned int touches_length;

  /// List of all touches which are currently down.
  WebTouchPoint touches[8];

  /// The length of the changed_touches array
  unsigned int changed_touches_length;

  /// List of all touches whose state has changed since the last WebTouchEvent
  WebTouchPoint changed_touches[8];

  /// The length of the target_touches_length
  unsigned int target_touches_length;

  /// List of all touches which are currently down and are targeting the event
  /// recipient.
  WebTouchPoint target_touches[8];
};
#pragma pack(pop)

}  // namespace Awesomium

#endif // AWESOMIUM_WEB_TOUCH_EVENT_H_
