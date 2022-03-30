///
/// @file BitmapSurface.h
///
/// @brief The header for the BitmapSurface class.
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
#ifndef AWESOMIUM_BITMAP_SURFACE_H_
#define AWESOMIUM_BITMAP_SURFACE_H_
#pragma once

#include <Awesomium/Platform.h>
#include <Awesomium/Surface.h>
#include <Awesomium/WebString.h>

namespace Awesomium {

class WebView;

///
/// @brief  This is the default Surface for WebView. It is a basic bitmap
///         that keeps track of whether or not it has changed since the
///         last time you called BitmapSurface::CopyTo.
///
class OSM_EXPORT BitmapSurface : public Awesomium::Surface {
 public:
  BitmapSurface(int width, int height);
  ~BitmapSurface();

  /// A pointer to the raw pixel buffer (32-bit BGRA format, 4 bpp)
  const unsigned char* buffer() const { return buffer_; }

  /// Get the width of the bitmap (in pixels)
  int width() const { return width_; }

  /// Get the height of the bitmap (in pixels)
  int height() const { return height_; }

  /// The number of bytes per row (this is usually `width * 4`)
  int row_span() const { return row_span_; }

  ///
  /// Manually set this bitmap as dirty (useful if you copied the
  /// bitmap buffer without the CopyTo method).
  ///
  void set_is_dirty(bool is_dirty);

  ///
  /// Whether or not the bitmap has changed since the last
  /// time CopyTo was called.
  ///
  bool is_dirty() const { return is_dirty_; }

  ///
  /// Copy this bitmap to a certain destination. Will also set the dirty
  /// bit to False.
  ///
  /// @param  dest_buffer  A pointer to the destination pixel buffer.
  ///
  /// @param  dest_row_span  The number of bytes per-row of the destination.
  ///
  /// @param  dest_depth  The depth (number of bytes per pixel, is usually 4
  ///                     for BGRA surfaces and 3 for BGR surfaces).
  ///
  /// @param  convert_to_rgba  Whether or not we should convert BGRA to RGBA.
  ///
  /// @param  flip_y  Whether or not we should invert the bitmap vertically.
  ///
  void CopyTo(unsigned char* dest_buffer,
        int dest_row_span,
        int dest_depth,
        bool convert_to_rgba,
        bool flip_y) const;

  ///
  /// Save this bitmap to a PNG image on disk.
  ///
  /// @param  file_path  The file path (should have filename ending in .png).
  ///
  /// @param  preserve_transparency  Whether or not we should preserve the
  ///                                alpha channel.
  ///
  /// @return  Returns true if the operation succeeded.
  ///
  bool SaveToPNG(const Awesomium::WebString& file_path,
                 bool preserve_transparency = false) const;

  ///
  /// Save this bitmap to a JPEG image on disk.
  ///
  /// @param  file_path  The file path (should have filename ending in .jpg).
  ///
  /// @param  quality  The compression quality (1-100, with 100 being best
  ///                  picture quality but highest file size).
  ///
  /// @return  Returns true if the operation succeeded.
  ///
  bool SaveToJPEG(const Awesomium::WebString& file_path,
                  int quality = 90) const;

  /// Get the opacity (0-255) of a certain point (in pixels).
  unsigned char GetAlphaAtPoint(int x, int y) const;

  ///
  /// This method is inherited from Surface, it is called by the WebView
  /// whenever a block of pixels needs to be painted to the bitmap.
  ///
  void Paint(unsigned char* src_buffer,
             int src_row_span,
             const Awesomium::Rect& src_rect,
             const Awesomium::Rect& dest_rect);

  ///
  /// This method is inherited from Surface, it is called by the WebView
  /// whenever a block of pixels needs to be translated on the bitmap.
  ///
  void Scroll(int dx, int dy,
              const Awesomium::Rect& clip_rect);

 private:
  unsigned char* buffer_;
  int width_;
  int height_;
  int row_span_;
  bool is_dirty_;
};

/// Helper function for copying pixel buffers.
void OSM_EXPORT CopyBuffers(int width,
                            int height,
                            unsigned char* src,
                            int src_row_span,
                            unsigned char* dest,
                            int dest_row_span,
                            int dest_depth,
                            bool convert_to_rgba,
                            bool flip_y);

/// @brief  The default SurfaceFactory for WebCore. Creates a BitmapSurface.
class OSM_EXPORT BitmapSurfaceFactory : public Awesomium::SurfaceFactory {
 public:
  BitmapSurfaceFactory();
  ~BitmapSurfaceFactory();
  Awesomium::Surface* CreateSurface(WebView* view, int width, int height);
  void DestroySurface(Awesomium::Surface* surface);
};

}  // namespace Awesomium

#endif  // AWESOMIUM_BITMAP_SURFACE_H_
