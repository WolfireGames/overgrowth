///
/// @file WebURL.h
///
/// @brief The header for the WebURL class.
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
#ifndef AWESOMIUM_WEB_URL_H_
#define AWESOMIUM_WEB_URL_H_
#pragma once

#include <Awesomium/Platform.h>
#include <Awesomium/WebString.h>

namespace Awesomium {

///
/// @brief  This class represents a parsed URL. It provides convenience methods
///         to access parsed details of the url.
///
class OSM_EXPORT WebURL {
 public:
  ///
  /// Create an empty WebURL.
  ///
  WebURL();

  ///
  /// Create a WebURL from a string.
  ///
  /// @param  url_string  A properly-formatted URL string. For example,
  ///                     http://www.google.com is valid, www.google.com is not
  ///
  explicit WebURL(const WebString& url_string);
  WebURL(const WebURL& rhs);

  ~WebURL();

  WebURL& operator=(const WebURL& rhs);

  /// Whether or not this URL is valid (was parsed successfully).
  bool IsValid() const;

  /// Whether or not this URL is empty.
  bool IsEmpty() const;

  /// The actual URL string.
  WebString spec() const;

  /// The parsed scheme (ex, 'http')
  WebString scheme() const;

  /// The parsed username (if any)
  WebString username() const;

  /// The parsed password (if any)
  WebString password() const;

  /// The parsed hostname, IPv4 Address, or IPv6 Address
  WebString host() const;

  /// The parsed port (if any)
  WebString port() const;

  /// The parsed path
  WebString path() const;

  /// The parsed query portion (anything following '?')
  WebString query() const;

  /// The parsed anchor portion (anything following '#')
  WebString anchor() const;

  /// The filename of the path (if any)
  WebString filename() const;

  bool operator==(const WebURL& other) const;
  bool operator!=(const WebURL& other) const;
  bool operator<(const WebURL& other) const;

 private:
  explicit WebURL(const void* internal_instance);
  void* instance_;
  friend class InternalHelper;
};

}  // namespace Awesomium

#endif  // AWESOMIUM_WEB_URL_H_
