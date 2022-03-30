///
/// @file Surface.h
///
/// @brief The header for the Surface and SurfaceFactory classes.
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
#ifndef AWESOMIUM_SURFACE_H_
#define AWESOMIUM_SURFACE_H_
#pragma once

#include <Awesomium/Platform.h>

namespace Awesomium {

class WebView;

///
/// @brief  This interface can be used to provide your own Surface
///         implementation to directly handle paint and pixel-scroll events
///         for all offscreen WebViews.
///
/// You should use this by defining your own SurfaceFactory that creates
/// your specific Surface implementation.
///
/// @note  See WebView::surface
///
class OSM_EXPORT Surface {
 public:
  virtual ~Surface() = 0;

  ///
  /// This event is called whenever the WebView wants to paint a certain section
  /// of the Surface with a block of pixels. It is your responsibility to copy
  /// src_buffer to the location in this Surface specified by dest_rect.
  ///
  /// @param src_buffer  A pointer to a block of pixels in 32-bit BGRA format.
  ///                    Size of the buffer is `src_row_span * src_rect.height`.
  ///                    Beware that src_buffer points to the beginning of the
  ///                    transport buffer, you should use src_rect to determine
  ///                    the offset to begin copying pixels from.
  ///
  /// @param src_row_span  The number of bytes of each row.
  ///                      (Usually `src_rect.width * 4`)
  ///
  /// @param src_rect  The dimensions of the region of src_buffer to copy from.
  ///                  May have a non-zero origin.
  ///
  /// @param dest_rect The location to copy src_buffer to. Always has same
  ///                  dimensions as src_rect but may have different origin
  ///                  (which specifies the offset of the section to copy to).
  ///
  virtual void Paint(unsigned char* src_buffer,
                     int src_row_span,
                     const Awesomium::Rect& src_rect,
                     const Awesomium::Rect& dest_rect) = 0;

  ///
  /// This event is called whenever the WebView wants to 'scroll' an existing
  /// section of the Surface by a certain offset. It your responsibility to
  /// translate the pixels within the specified clipping rectangle by the
  /// specified offset.
  ///
  /// @param dx  The amount of pixels to offset vertically.
  /// @param dy  The amount of pixels to offset vertically.
  ///
  /// @param clip_rect  The rectangle that this operation should be clipped to.
  ///
  virtual void Scroll(int dx,
                      int dy,
                      const Awesomium::Rect& clip_rect) = 0;
};

///
/// @brief  This interface can be used to provide your own SurfaceFactory so
/// that you can ultimately provide your own Surface implementation.
///
/// @note  See WebCore::set_surface_factory
///
class OSM_EXPORT SurfaceFactory {
 public:
  virtual ~SurfaceFactory() = 0;

  ///
  /// This event will be called whenever a WebView needs to create a new
  /// Surface. You should return your own Surface implementation here.
  ///
  /// @param  The WebView requesting the Surface.
  ///
  /// @param  width  The initial width of the Surface.
  ///
  /// @param  height  The initial height of the Surface.
  ///
  /// @return You should return your own Surface implementation here. The
  ///         WebView will call SurfaceFactory::DestroySurface when it is done
  ///         using the Surface.
  ///
  virtual Awesomium::Surface* CreateSurface(Awesomium::WebView* view,
                                            int width,
                                            int height) = 0;

  ///
  /// This event will be called whenever a WebView needs to destroy a Surface.
  /// You should cast this Surface to your own implementation and destroy it.
  ///
  /// @param  surface  The surface to destroy (you should cast this to your
  ///                  own implementation).
  ///
  virtual void DestroySurface(Awesomium::Surface* surface) = 0;
};

}  // namespace Awesomium

#endif  // AWESOMIUM_SURFACE_H_
