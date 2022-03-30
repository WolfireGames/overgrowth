///
/// @file WebString.h
///
/// @brief The header for the WebString class.
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
#ifndef AWESOMIUM_WEB_STRING_H_
#define AWESOMIUM_WEB_STRING_H_
#pragma once

#include <Awesomium/Platform.h>

namespace Awesomium {

template<class T>
class WebVector;

///
/// @brief  This class represents a UTF-16 String.
///
class OSM_EXPORT WebString {
 public:
  /// Create an empty string
  explicit WebString();

  /// Create a string by copying it from another substring
  explicit WebString(const WebString& src, unsigned int pos, unsigned int n);

  /// Create a string by copying it from a UTF-16 buffer
  explicit WebString(const wchar16* data);

  /// Create a string by copying it from a UTF-16 buffer
  explicit WebString(const wchar16* data, unsigned int len);

  WebString(const WebString& src);

  ~WebString();

  WebString& operator=(const WebString& rhs);

  /// Create a WebString from a UTF8 buffer
  static WebString CreateFromUTF8(const char* data, unsigned int len);

  /// Get the internal UTF-16 buffer
  const wchar16* data() const;

  /// Get the length of this string
  unsigned int length() const;

  /// Whether or not this string is empty.
  bool IsEmpty() const;

  /// Compare this string with another string.
  int Compare(const WebString& src) const;

  WebString& Assign(const WebString& src);
  WebString& Assign(const WebString& src, unsigned int pos, unsigned int n);
  WebString& Assign(const wchar16* data);
  WebString& Assign(const wchar16* data, unsigned int len);

  /// Append a string to the end of this string.
  WebString& Append(const WebString& src);

  /// Swap the contents of this string with another.
  void Swap(WebString& src);

  /// Clear the contents of this string.
  void Clear();

  ///
  /// Convert this WebString to a UTF8 string. To determine the length of
  /// the destination buffer, you should call this with NULL dest and 0 len
  /// first. Returns the length of the final conversion.
  ///
  unsigned int ToUTF8(char* dest, unsigned int len) const;

  bool operator==(const WebString& other) const;
  bool operator!=(const WebString& other) const;
  bool operator<(const WebString& other) const;

 private:
  explicit WebString(const void* internal_instance);
  void* instance_;
  friend class InternalHelper;
};

}  // namespace Awesomium

#endif  // AWESOMIUM_WEB_STRING_H_
