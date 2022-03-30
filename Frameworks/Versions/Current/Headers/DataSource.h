///
/// @file DataSource.h
///
/// @brief The header for the DataSource class.
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
#ifndef AWESOMIUM_DATA_SOURCE_H_
#define AWESOMIUM_DATA_SOURCE_H_
#pragma once

#include <Awesomium/Platform.h>
#include <Awesomium/WebString.h>
#include <Awesomium/ResourceInterceptor.h>

namespace Awesomium {

class WebSession;

///
/// @brief  This is an interface that you can use to provide your own
///         resource loader for `asset://` requests.
///
/// @see  WebSession::AddDataSource
///
class OSM_EXPORT DataSource {
 public:
  virtual ~DataSource();
  ///
  /// This event is called whenever a request is made for a certain path
  /// on the asset URL that this DataSource is bound to. It is your
  /// responsibility to call DataSource::SendResponse with a response
  /// to this request.
  ///
  /// @param request_id  The unique ID for the request.
  ///
  /// @param request     The details of the URL request.
  ///
  /// @param path        The path for the request. If your DataSource was
  ///                    registered with an asset_host of **MyHost**, path will
  ///                    contain whatever follows `asset://MyHost/`.
  ///
  /// @note  This event is always called on the main thread. To avoid potential
  ///        deadlocks, you should not make any calls to WebView, WebCore, or
  ///        JSObject within this callback.
  ///
  virtual void OnRequest(int request_id,
                         const ResourceRequest& request,
                         const WebString& path) = 0;

  ///
  /// Send the response for a previous request. You can call this immediately
  /// within OnRequest or asynchronously (once you've finished loading the
  /// resource). This method can be called from any thread.
  ///
  /// @param  request_id   The unique ID for the request.
  ///
  /// @param  buffer_size  The size of the buffer.
  ///
  /// @param  buffer       The bytes for the resource (will be copied, you
  ///                      retain ownership).
  ///
  /// @param  mime_type    The mime-type of the resource.
  ///
  /// @note  You should pass 0 for buffer_size and NULL for buffer to signify
  ///        that the request failed.
  ///
  void SendResponse(int request_id,
                    unsigned int buffer_size,
                    const unsigned char* buffer,
                    const WebString& mime_type);

 protected:
  DataSource();

  void set_session(WebSession* session, int data_source_id);

  WebSession* session_;
  int data_source_id_;

  friend class WebSessionImpl;
};
}  // namespace Awesomium

#endif  // AWESOMIUM_DATA_SOURCE_H_
