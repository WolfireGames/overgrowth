///
/// @file WebCore.h
///
/// @brief The main header for the Awesomium C++ API. This header includes most
///        of the common API functions you will need.
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
#ifndef AWESOMIUM_WEB_CORE_H_
#define AWESOMIUM_WEB_CORE_H_
#pragma once

#include <Awesomium/Platform.h>
#include <Awesomium/WebConfig.h>
#include <Awesomium/WebPreferences.h>
#include <Awesomium/WebSession.h>
#include <Awesomium/WebView.h>
#include <Awesomium/Surface.h>
#include <Awesomium/ResourceInterceptor.h>

namespace Awesomium {

///
/// The severity level for a log message. See WebCore::Log
///
enum LogSeverity {
  kLogSeverity_Info = 0,    ///< Info message
  kLogSeverity_Warning,     ///< Warning message
  kLogSeverity_Error,       ///< Error message
  kLogSeverity_ErrorReport, ///< Error report message
  kLogSeverity_Fatal        ///< Fatal error message, terminates application
};

///
/// @brief  The core of Awesomium. You should initialize it before doing
///         anything else.
///
/// This singleton class manages the lifetime of all WebViews (see WebView)
/// and maintains useful services like the network stack, inter-process
/// messaging, and Surface creation.
///
class OSM_EXPORT WebCore {
 public:
  ///
  /// Creates the WebCore with a certain configuration. You can access the
  /// singleton via instance() later. You can only do this once per-process.
  ///
  /// @param  config  Global configuration settings.
  ///
  /// @return  Returns a pointer to the singleton instance.
  ///
  static WebCore* Initialize(const WebConfig& config);

  ///
  /// Destroys the WebCore singleton and cleans up any active WebViews.
  ///
  static void Shutdown();

  ///
  /// Get a pointer to the WebCore singleton.
  ///
  /// @note  Will return 0 if the WebCore has not been initialized.
  ///
  static WebCore* instance();

  ///
  /// Create a WebSession which will be used to store all user-data (such as
  /// cookies, cache, certificates, local databases, etc).
  ///
  /// @param  path  The directory path to store the data (will create the path
  ///               if it doesn't exist, or load it if it already exists).
  ///               Specify an empty string to use an in-memory store.
  ///
  /// @return  Returns a new WebSession instance that you can use with
  ///          any number of WebViews. You are responsible for calling
  ///          WebSession::Release when you are done using the session.
  ///
  virtual WebSession* CreateWebSession(const WebString& path,
                                       const WebPreferences& prefs) = 0;
  ///
  /// Creates a new WebView.
  ///
  /// @param  width    The initial width, in pixels.
  /// @param  height   The initial height, in pixels.
  ///
  /// @param  session  The session to use for this WebView. Pass 0 to use
  ///                  a default, global, in-memory session.
  ///
  /// @param  type     The type of WebView to create. See WebViewType for more
  ///                  information.
  ///
  /// @return  Returns a pointer to a new WebView instance. You should call
  ///          WebView::Destroy when you are done with the instance.
  ///
  virtual WebView* CreateWebView(int width,
                                 int height,
                                 WebSession* session = 0,
                                 WebViewType type = kWebViewType_Offscreen) = 0;

  ///
  /// Set the SurfaceFactory to be used to create Surfaces for all offscreen
  /// WebViews from this point forward. If you call this, you are responsible
  /// for destroying the passed instance after you Shutdown the WebCore.
  ///
  /// @param  factory  The factory to be used to create all Surfaces. You are
  ///                  responsible for destroying this instance after you
  ///                  call Shutdown.
  ///
  /// @note: If you never call this, a default BitmapSurfaceFactory will be
  ///        used and all Surfaces will be of type 'BitmapSurface'.
  ///
  virtual void set_surface_factory(SurfaceFactory* factory) = 0;

  ///
  /// Get the current SurfaceFactory instance.
  ///
  virtual SurfaceFactory* surface_factory() const = 0;

  ///
  /// Set the ResourceInterceptor instance.
  ///
  /// @param  interceptor  The instance that will be used to intercept all
  ///                      resources. You are responsible for destroying this
  ///                      instance after you call Shutdown.
  ///
  virtual void set_resource_interceptor(ResourceInterceptor* interceptor) = 0;

  ///
  /// Get the current ResourceInterceptor instance.
  ///
  virtual ResourceInterceptor* resource_interceptor() const = 0;

  ///
  /// Updates the WebCore, you must call this periodically within your
  /// application's main update loop. This method allows the WebCore to
  /// conduct operations such as updating the Surface of each WebView,
  /// destroying any WebViews that are queued for destruction, and dispatching
  /// any queued WebViewListener events.
  ///
  virtual void Update() = 0;

  ///
  /// Log a message to the log (print to the console and log file).
  ///
  virtual void Log(const WebString& message,
                   LogSeverity severity,
                   const WebString& file,
                   int line) = 0;

  ///
  /// Get the version for this build of Awesomium.
  ///
  virtual const char* version_string() const = 0;

  ///
  /// Get the number of bytes actually used by our allocator (TCMalloc).
  ///
  /// @note: Only implemented on Windows.
  ///
  static unsigned int used_memory();

  ///
  /// Get the number of bytes cached by our allocator (TCMalloc).
  ///
  /// @note: Only implemented on Windows.
  ///
  static unsigned int allocated_memory();

  ///
  /// Force TCMalloc to release as much unused memory as possible. Reducing
  /// the size of the memory pool may cause a small performance hit.
  ///
  /// @note: Only implemented on Windows.
  ///
  static void release_memory();

 protected:
  virtual ~WebCore() {}

 private:
  static WebCore* instance_;
};

}  // namespace Awesomium

#endif  // AWESOMIUM_WEB_CORE_H_
