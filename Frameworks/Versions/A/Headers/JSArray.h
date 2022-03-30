///
/// @file JSArray.h
///
/// @brief The header for the JSArray class.
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
#ifndef AWESOMIUM_JS_ARRAY_H_
#define AWESOMIUM_JS_ARRAY_H_
#pragma once

#include <Awesomium/Platform.h>

namespace Awesomium {

class JSValue;
template<class T>
class WebVector;

///
/// @brief  This class represents an Array type in JavaScript. It has a
///         flexible size and you can treat it like a stack or list.
///
class OSM_EXPORT JSArray {
 public:
  /// Create an empty JSArray
  JSArray();

  ///
  /// Create a JSArray with a certain number of elements. All elements will
  /// have Undefined type by default.
  ///
  explicit JSArray(unsigned int n);

  JSArray(const JSArray& rhs);
  ~JSArray();

  JSArray& operator=(const JSArray& rhs);

  /// Get the number of items in the array
  unsigned int size() const;

  /// Get the internal capacity of the array
  unsigned int capacity() const;

  ///
  /// Get the value at a certain index. Will assert if the index is out
  /// of bounds.
  ///
  JSValue& At(unsigned int idx);

  ///
  /// Get the value at a certain index. Will assert if the index is out
  /// of bounds.
  ///
  const JSValue& At(unsigned int idx) const;

  JSValue& operator[](unsigned int idx);
  const JSValue& operator[](unsigned int idx) const;

  ///
  /// Push an item onto the back of the array
  ///
  void Push(const JSValue& item);

  ///
  /// Pop an item off the back of the array
  ///
  void Pop();

  ///
  /// Insert an item into the array at a specific index.
  /// The index must be greater than or equal to size().
  ///
  void Insert(const JSValue& item, unsigned int idx);

  ///
  /// Erase an item at a specific index.
  ///
  void Erase(unsigned int idx);

  ///
  /// Clear the entire array.
  ///
  void Clear();

 protected:
  WebVector<JSValue>* vector_;
};

}  // namespace Awesomium

#endif  // AWESOMIUM_JS_ARRAY_H_

