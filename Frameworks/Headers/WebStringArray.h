///
/// @file WebStringArray.h
///
/// @brief The header for the WebStringArray class.
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
#ifndef AWESOMIUM_WEB_STRING_ARRAY_H_
#define AWESOMIUM_WEB_STRING_ARRAY_H_
#pragma once

#include <Awesomium/Platform.h>
#include <Awesomium/WebString.h>

namespace Awesomium {

template<class T>
class WebVector;

/// An array of WebStrings
class OSM_EXPORT WebStringArray {
 public:
  WebStringArray();
  explicit WebStringArray(unsigned int n);
  WebStringArray(const WebStringArray& rhs);
  ~WebStringArray();

  WebStringArray& operator=(const WebStringArray& rhs);

  /// The size of the array
  unsigned int size() const;

  /// Get the item at a specific index
  WebString& At(unsigned int idx);

  /// Get the item at a specific index
  const WebString& At(unsigned int idx) const;

  /// Get the item at a specific index
  WebString& operator[](unsigned int idx);

  /// Get the item at a specific index
  const WebString& operator[](unsigned int idx) const;

  /// Add an item to the end of the array
  void Push(const WebString& item);

 protected:
  WebVector<WebString>* vector_;
};

}  // namespace Awesomium

#endif  // AWESOMIUM_WEB_STRING_ARRAY_H_
