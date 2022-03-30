///
/// @file JSObject.h
///
/// @brief The header for the JSObject class.
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
#ifndef AWESOMIUM_JS_OBJECT_H_
#define AWESOMIUM_JS_OBJECT_H_
#pragma once

#include <Awesomium/Platform.h>
#include <Awesomium/WebString.h>

namespace Awesomium {

class JSValue;
class JSArray;
class JSObjectBridge;
class JSMethodHandler;
class WebView;

///
/// An enumeration of JSObject types.
///
enum JSObjectType {
  /// Local object, can only contain properties.
  kJSObjectType_Local,

  /// Remote object, actually stored in a remote process. All API calls will
  /// be proxied to the remote process and may fail (see GetLastError()).
  kJSObjectType_Remote,

  /// Same as above except that the object will persist between all page loads.
  kJSObjectType_RemoteGlobal,
};

///
/// @brief This class represents an Object type in JavaScript. You can add
///        set, get, and remove named Properties and invoke named Methods.
///
/// @note There are two types of JSObjects, **Local** and **Remote**. Local
///       objects only have Properties and can be accessed without error
///       in the main process. Users can only create Local objects. Remote
///       objects are defined within the V8 engine in a separate process
///       and can have both Properties and Methods. Method calls on Remote
///       objects are proxied to the process and execute synchronously but
///       may fail for various reasons (see JSObject::last_error()).
///
class OSM_EXPORT JSObject {
 public:
  ///
  /// Create a Local object (you can only define Properties).
  ///
  JSObject();

  JSObject(const JSObject& obj);

  ~JSObject();

  JSObject& operator=(const JSObject& rhs);

  ///
  /// Get the remote ID for this JSObject (will be 0 if this is Local).
  ///
  /// @see JSMethodHandler::OnMethodCall
  /// @see JSMethodHandler::OnMethodCallWithReturnValue
  ///
  unsigned int remote_id() const;

  ///
  /// Get the remote reference count for this JSObject (will be 0 for Local).
  ///
  int ref_count() const;

  /// Get this object's type.
  JSObjectType type() const;

  ///
  /// Get this object's owner.
  ///
  /// @note  This is only valid for Remote objects. Local objects will return
  ///        NULL. If the connection has gone away, this will also return NULL.
  ///
  WebView* owner() const;

  ///
  /// Get a list of this object's properties.
  ///
  JSArray GetPropertyNames() const;

  ///
  /// Check whether or not this object has a certain property synchronously.
  ///
  /// @param  name  The name of the property to check for existence.
  ///
  bool HasProperty(const WebString& name) const;

  ///
  /// Get the value of a certain property synchronously.
  ///
  /// @param  name  The name of the property to retrieve.
  ///
  JSValue GetProperty(const WebString& name) const;

  ///
  /// Add or update a certain property synchronously.
  ///
  /// @param  name  The name of the property to set.
  ///
  /// @param  value  The value to set.
  ///
  void SetProperty(const WebString& name, const JSValue& value);

  ///
  /// Add or update a certain property asynchronously. This is useful when you
  /// need to define a large set of properties at once and you would like the
  /// whole operation to be queued together during the next update instead of
  /// making lots of individual, synchronous IPC calls to the child-process
  /// with JSObject::SetProperty.
  ///
  /// @param  name  The name of the property to set.
  ///
  /// @param  value  The value to set.
  ///
  void SetPropertyAsync(const WebString& name, const JSValue& value);

  ///
  /// Remove a certain property synchronously.
  ///
  /// @param  name  The name of the property to remove.
  ///
  void RemoveProperty(const WebString& name);

  ///
  /// Get a list of this object's methods synchronously.
  ///
  /// @note  Only valid for Remote objects.
  ///
  JSArray GetMethodNames() const;

  ///
  /// Check whether or not this object has a certain method.
  ///
  /// @note  Only valid for Remote objects.
  ///
  bool HasMethod(const WebString& name) const;

  ///
  /// Invoke a method with a set of arguments and return a result synchronously.
  ///
  /// @note: Only valid for Remote objects.
  ///
  /// @param  name  The name of the method to call.
  ///
  /// @param  args  The arguments to pass.
  ///
  /// @return  Returns the result of the method call.
  ///
  JSValue Invoke(const WebString& name, const JSArray& args);

  ///
  /// Invoke a method asynchronously with a set of arguments, ignoring the
  /// result.
  ///
  /// @note: Only valid for Remote objects.
  ///
  /// @param  name  The name of the method to call.
  ///
  /// @param  args  The arguments to pass.
  ///
  void InvokeAsync(const WebString& name, const JSArray& args);

  ///
  /// Get this object as a string.
  ///
  WebString ToString() const;

  ///
  /// Declare a method to be handled via a custom callback. The callback
  /// can either be declared with a return value or not.
  ///
  /// @param  name  The name of the method as it will appear in JavaScript.
  ///
  /// @param  has_return_value  Whether or not the method will return a value.
  ///                           We will use a separate method handler based
  ///                           on whether or not the method returns a value.
  ///
  /// @see WebView::set_js_method_handler
  ///
  /// @see JSMethodHandler
  ///
  /// @note  Warning: Methods that are declared with return values will be
  ///        invoked synchronously and may degrade performance.
  ///
  void SetCustomMethod(const WebString& name, bool has_return_value);

  ///
  /// Each of the above methods may fail if this JSObject is Remote.
  /// You should check if there was an error via this method.
  ///
  Error last_error() const;

 protected:
  JSObject(unsigned int jso_bridge_id, unsigned int handle);
  JSObjectBridge* GetBridge(unsigned int id) const;
  void TranslateError(JSObjectBridge* bridge, bool result) const;
  friend class InternalHelper;
  bool is_local_;
  union {
    void* local;
    void* remote;
  } instance_;
  Error last_error_;
};

///
/// @brief  This is an interface that you can use to handle custom method calls
///         from JavaScript.
///
/// @see  JSObject::SetCustomMethod
///
class JSMethodHandler {
 public:
  ///
  /// This event occurs whenever a custom JSObject method (with no return
  /// value) is called from JavaScript.
  ///
  /// @param  caller  The WebView that dispatched this event.
  ///
  /// @param  remote_object_id  The unique ID of the JS Object that contains
  ///                           this method.
  ///
  /// @param  method_name  The name of the method being called.
  ///
  /// @param  args  The arguments passed to the method.
  ///
  virtual void OnMethodCall(Awesomium::WebView* caller,
                            unsigned int remote_object_id,
                            const Awesomium::WebString& method_name,
                            const Awesomium::JSArray& args) = 0;

  ///
  /// This event occurs whenever a custom JSObject method (with a return
  /// value) is called from JavaScript.
  ///
  /// @note  This event is synchronous (the WebView process is blocked until
  ///        this method returns). You must be careful not to invoke any
  ///        synchronous API calls at the risk of causing deadlock.
  ///
  ///        This event does not support objects being passed as arguments
  ///        (all JS objects are replaced with undefined in the args array).
  ///
  /// @param  caller  The WebView that dispatched this event.
  ///
  /// @param  remote_object_id  The unique ID of the JS Object that contains
  ///                           this method.
  ///
  /// @param  method_name  The name of the method being called.
  ///
  /// @param  args  The arguments passed to the method.
  ///
  /// @return  You should handle this method call and return the result.
  ///
  virtual Awesomium::JSValue OnMethodCallWithReturnValue(Awesomium::WebView* caller,
                                              unsigned int remote_object_id,
                                              const Awesomium::WebString& method_name,
                                              const Awesomium::JSArray& args) = 0;

  virtual ~JSMethodHandler() {}
};

}  // namespace Awesomium

#endif  // AWESOMIUM_JS_OBJECT_H_

