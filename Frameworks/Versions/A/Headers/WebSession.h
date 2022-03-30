///
/// @file WebSession.h
///
/// @brief The header for the WebSession class.
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
#ifndef AWESOMIUM_WEB_SESSION_H_
#define AWESOMIUM_WEB_SESSION_H_
#pragma once

#include <Awesomium/Platform.h>
#include <Awesomium/WebString.h>
#include <Awesomium/WebPreferences.h>
#include <Awesomium/DataSource.h>

namespace Awesomium {

///
/// @brief  A WebSession is responsible for storing all user-generated data
///         (cookies, cache, authentication, etc) and for providing custom
///         user data-sources (eg, resolving `asset://` requests).
///
/// Sessions can either be purely in-memory or saved to disk to be restored
/// later (you will need to provide a writeable path to store the data at
/// runtime).
///
/// @see WebCore::CreateWebSession
/// @see WebCore::CreateWebView
///
class OSM_EXPORT WebSession {
 public:
  ///
  /// Release the WebSession, you are responsible for calling this once you are
  /// done using the session. (Call this before shutting down WebCore).
  ///
  virtual void Release() const = 0;

  ///
  /// Whether or not this session is being synchronized to disk (else, it's
  /// in-memory only and all data will be lost at exit).
  ///
  virtual bool IsOnDisk() const = 0;

  ///
  /// The disk path of this session, if any.
  ///
  virtual WebString data_path() const = 0;

  ///
  /// The preferences for this session.
  ///
  virtual const WebPreferences& preferences() const = 0;

  ///
  /// Register a custom DataSource to handle "asset://" requests matching a
  /// certain hostname. This is useful for providing your own resource loader
  /// for local assets.
  ///
  /// @note  You should not add a single DataSource instance to multiple
  ///        WebSessions. The DataSource should outlive this WebSession.
  ///
  /// @param  asset_host  The asset hostname that this DataSource will be used
  ///                     for, (eg, asset://asset_host_goes_here/foobar.html).
  ///                     Specify "catch-all" to catch any unmatched requests.
  ///
  /// @param  source      The DataSource that will handle requests. You retain
  ///                     ownership of the DataSource. This instance should
  ///                     outlive all associated WebSessions and WebViews.
  ///
  virtual void AddDataSource(const WebString& asset_host,
                             DataSource* source) = 0;

  ///
  /// Sets a cookie for a certain URL asynchronously.
  ///
  /// @param  url  The URL to set the cookie on.
  ///
  /// @param  cookie_string  The cookie string, for example:
  ///                        <pre> "key1=value1; key2=value2" </pre>
  ///
  /// @param  is_http_only   Whether or not this cookie is HTTP-only.
  ///
  /// @param  force_session_cookie  Whether or not to force this as a session
  ///                               cookie. (Will not be saved to disk)
  ///
  virtual void SetCookie(const WebURL& url,
                         const WebString& cookie_string,
                         bool is_http_only,
                         bool force_session_cookie) = 0;

  ///
  /// Clears all cookies asynchronously.
  ///
  virtual void ClearCookies() = 0;

  ///
  /// Clears the cache asynchronously.
  ///
  virtual void ClearCache() = 0;

  ///
  /// Gets the saved zoom amount for a certain URL host (in percent).
  /// Zoom amounts are saved per-hostname.
  ///
  /// @see WebView::GetZoom
  /// @see WebView::SetZoom
  ///
  virtual int GetZoomForURL(const WebURL& url) = 0;

 protected:
  virtual ~WebSession() {}
};

}  // namespace Awesomium

#endif  // AWESOMIUM_WEB_SESSION_H_
