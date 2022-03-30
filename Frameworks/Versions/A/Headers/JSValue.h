///
/// @file JSValue.h
///
/// @brief The header for the JSValue class.
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
#ifndef AWESOMIUM_JS_VALUE_H_
#define AWESOMIUM_JS_VALUE_H_
#pragma once

#include <Awesomium/Platform.h>
#include <Awesomium/WebString.h>
#include <Awesomium/JSObject.h>
#include <Awesomium/JSArray.h>

namespace Awesomium {

struct VariantValue;

///
/// @brief  Represents a value in JavaScript.
///
class OSM_EXPORT JSValue {
 public:
  /// Create an empty JSValue ('undefined' by default).
  JSValue();

  /// Create a JSValue initialized with a boolean.
  explicit JSValue(bool value);

  /// Create a JSValue initialized with an integer.
  explicit JSValue(int value);

  /// Create a JSValue initialized with a double.
  explicit JSValue(double value);

  /// Create a JSValue initialized with a string.
  JSValue(const WebString& value);

  /// Create a JSValue initialized with an object.
  JSValue(const JSObject& value);

  /// Create a JSValue initialized with an array.
  JSValue(const JSArray& value);

  JSValue(const JSValue& original);

  ~JSValue();

  JSValue& operator=(const JSValue& rhs);

  /// Get the global Undefined JSValue instance
  static const JSValue& Undefined();

  /// Get the global Null JSValue instance
  static const JSValue& Null();

  /// Returns whether or not this JSValue is a boolean.
  bool IsBoolean() const;

  /// Returns whether or not this JSValue is an integer.
  bool IsInteger() const;

  /// Returns whether or not this JSValue is a double.
  bool IsDouble() const;

  /// Returns whether or not this JSValue is a number (integer or double).
  bool IsNumber() const;

  /// Returns whether or not this JSValue is a string.
  bool IsString() const;

  /// Returns whether or not this JSValue is an array.
  bool IsArray() const;

  /// Returns whether or not this JSValue is an object.
  bool IsObject() const;

  /// Returns whether or not this JSValue is null.
  bool IsNull() const;

  /// Returns whether or not this JSValue is undefined.
  bool IsUndefined() const;

  /// Returns this JSValue as a string (converting if necessary).
  WebString ToString() const;

  /// Returns this JSValue as an integer (converting if necessary).
  int ToInteger() const;

  /// Returns this JSValue as a double (converting if necessary).
  double ToDouble() const;

  /// Returns this JSValue as a boolean (converting if necessary).
  bool ToBoolean() const;

  ///
  /// Gets a reference to this JSValue's array value (will assert if not
  /// an array type)
  ///
  JSArray& ToArray();

  ///
  /// Gets a constant reference to this JSValue's array value (will assert
  /// if not an array type)
  ///
  const JSArray& ToArray() const;

  ///
  /// Gets a reference to this JSValue's object value (will assert if not
  /// an object type)
  ///
  JSObject& ToObject();

  ///
  /// Gets a constant reference to this JSValue's object value (will
  /// assert if not an object type)
  ///
  const JSObject& ToObject() const;

 protected:
  VariantValue* value_;
};

}  // namespace Awesomium

#endif  // AWESOMIUM_JS_VALUE_H_
