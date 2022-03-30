///
/// @file ResourceInterceptor.h
///
/// @brief The header for the ResourceInterceptor events.
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
#ifndef AWESOMIUM_RESOURCE_INTERCEPTOR_H_
#define AWESOMIUM_RESOURCE_INTERCEPTOR_H_
#pragma once

#include <Awesomium/Platform.h>
#include <Awesomium/WebString.h>

namespace Awesomium {

class WebURL;
class WebString;
class WebView;

class ResourceResponse;
class ResourceRequest;
class UploadElement;

///
/// @brief The ResourceInterceptor class is used to intercept requests
/// and responses for resources via WebView::set_resource_interceptor
///
class OSM_EXPORT ResourceInterceptor {
 public:
  ///
  /// Override this method to intercept requests for resources. You can use
  /// this to modify requests before they are sent, respond to requests using
  /// your own custom resource-loading back-end, or to monitor requests for
  /// tracking purposes.
  ///
  /// @param  request  The resource request.
  ///
  /// @return  Return a new ResourceResponse (see ResourceResponse::Create)
  ///          to override the response, otherwise, return NULL to allow
  ///          normal behavior.
  ///
  /// @note WARNING: This method is called on the IO Thread, you should not
  ///       make any calls to WebView or WebCore (they are not threadsafe).
  ///
  virtual Awesomium::ResourceResponse* OnRequest(
                                          Awesomium::ResourceRequest* request) {
    return 0;
  }

  ///
  /// Override this method to intercept frame navigations. You can use this to
  /// block or log navigations for each frame of a WebView.
  ///
  /// @param  origin_process_id   The unique process id of the origin WebView.
  ///                             See WebView::process_id()
  ///
  /// @param  origin_routing_id   The unique routing id of the origin WebView.
  ///                             See WebView::routing_id()
  ///
  /// @param  method   The HTTP method (usually GET or POST) of the request.
  ///
  /// @param  url    The URL that the frame wants to navigate to.
  ///
  /// @param  is_main_frame   Whether or not this navigation involves the main
  ///                         frame (eg, the whole page is navigating away).
  ///
  /// @return  Return True to block a navigation. Return False to let it continue.
  ///
  /// @note WARNING: This method is called on the IO Thread, you should not
  ///       make any calls to WebView or WebCore (they are not threadsafe).
  ///
  virtual bool OnFilterNavigation(int origin_process_id,
                                  int origin_routing_id,
                                  const Awesomium::WebString& method,
                                  const Awesomium::WebURL& url,
                                  bool is_main_frame) {
    return false;
  }

  ///
  /// Override this method to intercept download events (usually triggered
  /// when a server response indicates the content should be downloaded, eg:
  /// Content-Disposition: attachment). You can use this to log these
  /// type of responses.
  ///
  /// @param  origin_process_id   The unique process id of the origin WebView.
  ///                             See WebView::process_id()
  ///
  /// @param  origin_routing_id   The unique routing id of the origin WebView.
  ///                             See WebView::routing_id()
  ///
  /// @param  url    The URL of the request that initiated the download.
  ///
  /// @note WARNING: This method is called on the IO Thread, you should not
  ///       make any calls to WebView or WebCore (they are not threadsafe).
  ///
  virtual void OnWillDownload(int origin_process_id,
                              int origin_routing_id,
                                const Awesomium::WebURL& url) {
  }

  virtual ~ResourceInterceptor() {}
};

///
/// The ResourceRequest class represents a request for a URL resource. You can
/// get information about the request or modify it (change GET to POST, modify
/// headers, etc.).
///
class OSM_EXPORT ResourceRequest {
 public:
  /// Cancel this request.
  virtual void Cancel() = 0;

  /// The process ID where this request originated from. This corresponds
  /// to WebView::process_id().
  virtual int origin_process_id() const = 0;

  /// The routing ID where this request originated from. This corresponds
  /// to WebView::routing_id().
  virtual int origin_routing_id() const = 0;

  /// Get the URL associated with this request.
  virtual WebURL url() const = 0;

  /// Get the HTTP method (usually "GET" or "POST")
  virtual WebString method() const = 0;

  /// Set the HTTP method
  virtual void set_method(const WebString& method) = 0;

  /// Get the referrer
  virtual WebString referrer() const = 0;

  /// Set the referrer
  virtual void set_referrer(const WebString& referrer) = 0;

  /// Get extra headers for the request
  virtual WebString extra_headers() const = 0;

  ///
  /// Add a list of HTTP header strings, each delimited by "\r\n". Each
  /// individual header string should be in the format defined at the
  /// following link: http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html
  ///
  virtual void set_extra_headers(const WebString& headers) = 0;

  /// Add a single HTTP header key/value pair.
  virtual void AppendExtraHeader(const WebString& name,
                                 const WebString& value) = 0;

  /// Get the number of upload elements (essentially, batches of POST data).
  virtual unsigned int num_upload_elements() const = 0;

  /// Get a certain upload element (returned instance is owned by this class)
  virtual const UploadElement* GetUploadElement(unsigned int idx) const = 0;

  /// Clear all upload elements
  virtual void ClearUploadElements() = 0;

  /// Append a file for POST data (adds a new UploadElement)
  virtual void AppendUploadFilePath(const WebString& path) = 0;

  /// Append a string of bytes for POST data (adds a new UploadElement)
  virtual void AppendUploadBytes(const char* bytes,
                                 unsigned int num_bytes) = 0;
  ///
  /// Make this request ignore any DataSource handlers.
  ///
  /// This is useful when you are using DataSource to override default
  /// protocols such as HTTP or FTP (see WebConfig::asset_protocol) and you
  /// only want a subset of requests to be handled by DataSource. The request
  /// will be handled by the default handler if this request ignores the
  /// DataSource handler. This is set to FALSE by default.
  ///
  virtual void set_ignore_data_source_handler(bool ignore) = 0;

 protected:
  virtual ~ResourceRequest() {}
};

///
/// @brief The ResourceResponse class is simply a wrapper around a raw block
///        of data and a specified mime-type. It can be used with
///        ResourceInterceptor::onRequest to return a custom resource for a
///        certain resource request.
///
class OSM_EXPORT ResourceResponse {
 public:
  ///
  /// Create a ResourceResponse from a raw block of data. (Data is not owned,
  /// a copy is made of the supplied buffer.)
  ///
  /// @param  num_bytes  Size (in bytes) of the memory buffer.
  ///
  /// @param  buffer     Raw memory buffer to be copied.
  ///
  /// @param  mime_type  The mime-type of the data.
  ///                    See <http://en.wikipedia.org/wiki/Internet_media_type>
  ///
  static ResourceResponse* Create(unsigned int num_bytes,
                                  unsigned char* buffer,
                                  const WebString& mime_type);

  ///
  /// Create a ResourceResponse from a file on disk.
  ///
  /// @param  file_path  The path to the file.
  ///
  static ResourceResponse* Create(const WebString& file_path);

 protected:
  ResourceResponse(unsigned int num_bytes,
                   unsigned char* buffer,
                   const WebString& mime_type);
  ResourceResponse(const WebString& file_path);

  ~ResourceResponse();

  unsigned int num_bytes_;
  unsigned char* buffer_;
  WebString mime_type_;
  WebString file_path_;

  friend class WebCoreImpl;
};

class OSM_EXPORT UploadElement {
 public:
  /// Whether or not this UploadElement is a file
  virtual bool IsFilePath() const = 0;

  /// Whether or not this UploadElement is a string of bytes
  virtual bool IsBytes() const = 0;

  virtual unsigned int num_bytes() const = 0;

  /// Get the string of bytes associated with this UploadElement
  virtual const unsigned char* bytes() const = 0;

  /// Get the file path associated with this UploadElement
  virtual WebString file_path() const = 0;

 protected:
  virtual ~UploadElement() {}
};

}  // namespace Awesomium

#endif  // AWESOMIUM_RESOURCE_INTERCEPTOR_H_
