///
/// @file Platform.h
///
/// @brief The header for the base platform definitions.
///
/// @author
///
/// This file is a part of Awesomium, a Web UI bridge for native apps.
///
/// Website: <http://www.awesomium.com>
///
/// Copyright (C) 2015 Awesomium Technologies LLC. All rights reserved.
/// Awesomium is a trademark of Awesomium Technologies LLC.
///
#ifndef AWESOMIUM_PLATFORM_H_
#define AWESOMIUM_PLATFORM_H_

#if defined(__WIN32__) || defined(_WIN32)
#  if defined(OSM_IMPLEMENTATION)
#    define OSM_EXPORT __declspec(dllexport)
#  else
#    define OSM_EXPORT __declspec(dllimport)
#  endif
#else
#  define OSM_EXPORT __attribute__((visibility("default")))
#endif

#if defined(_WIN32)
#include <Windows.h>
#elif defined(__linux__)
#include <unistd.h>
#elif defined(__APPLE__)
#include <unistd.h>
#ifdef __OBJC__
@class NSView;
#else
class NSView;
#endif
#endif

#if __LP64__
typedef long int64;
#else
typedef long long int64;
#endif

typedef unsigned short wchar16;

/// The current version of Awesomium. This will be included in the user-agent.
#define OSM_VERSION "1.7.5.1"

/// The namespace for the Awesomium API.
namespace Awesomium {

/// Represents a generic error.
enum Error {
  kError_None = 0,        ///< No error (everything is cool!)
  kError_BadParameters,   ///< Bad parameters were supplied.
  kError_ObjectGone,      ///< The object no longer exists.
  kError_ConnectionGone,  ///< The IPC connection no longest exists.
  kError_TimedOut,        ///< The operation timed out.
  kError_WebViewGone,     ///< The WebView no longer exists.
  kError_Generic,         ///< A generic error was encountered.
};

#if defined(_WIN32)
typedef HWND NativeWindow;
#elif defined(__APPLE__)
typedef NSView* NativeWindow;
#else
typedef void* NativeWindow;
#endif

#if defined(_WIN32)
typedef HANDLE ProcessHandle;
#elif defined(__linux__) || defined(__APPLE__)
typedef pid_t ProcessHandle;
#endif

#pragma pack(push)
#pragma pack(1)
/// Represents a generic rectangle with pixel dimensions
struct OSM_EXPORT Rect {
  /// The x-coordinate of the origin of the rectangle
  int x;

  /// The y-coordinate of the origin of the rectangle
  int y;

  /// The width of the rectangle
  int width;

  /// The height of the rectangle
  int height;

  /// Create an empty Rect
  Rect();

  /// Create a Rect with certain dimensions
  Rect(int x, int y, int width, int height);

  /// Check whether or not this Rect is empty (width and height == 0)
  bool IsEmpty() const;
};
#pragma pack(pop)

}  // namespace Awesomium

///
/// @mainpage Awesomium C++ API
///
/// @section intro_sec Introduction
///
/// Hi there, welcome to the Awesomium C++ API docs!
///
/// Awesomium is a Web UI bridge for native applications.
///
/// If this is your first time exploring the API, we recommend
/// starting with Awesomium::WebCore and Awesomium::WebView.
///
/// For an introduction to basic API concepts, please see:
///   http://wiki.awesomium.com/getting-started/basic-concepts.html
///
///
/// @section usefullinks_sec Useful Links
/// - Awesomium Main: <http://www.awesomium.com>
/// - Support: <http://answers.awesomium.com>
/// - Wiki: <http://wiki.awesomium.com>
///
/// @section copyright_sec Copyright
/// This documentation is copyright (C) 2015 Awesomium Technologies LLC. All rights reserved.
/// Awesomium is a trademark of Awesomium Technologies LLC.
///

#endif  // AWESOMIUM_PLATFORM_H_
