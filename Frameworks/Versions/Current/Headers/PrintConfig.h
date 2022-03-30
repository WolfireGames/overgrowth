///
/// @file PrintConfig.h
///
/// @brief The header for the PrintConfig class.
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
#ifndef AWESOMIUM_PRINT_CONFIG_H_
#define AWESOMIUM_PRINT_CONFIG_H_

#include <Awesomium/Platform.h>
#include <Awesomium/WebString.h>

namespace Awesomium {

///
/// @brief Use this class to specify print-to-file settings.
///
/// @see WebView::PrintToFile
///
#pragma pack(push)
#pragma pack(1)
struct OSM_EXPORT PrintConfig {
  ///
  /// Create the default Printing Configuration
  ///
  PrintConfig();

  /// The dimensions (width/height) of the page, in points.
  Awesomium::Rect page_size;

  /// Number of dots per inch.
  double dpi;

  /// Whether or not we should make a new file for each page.
  bool split_pages_into_multiple_files;

  /// Whether or not we should only print the selection.
  bool print_selection_only;
};
#pragma pack(pop)

}  // namespace Awesomium

#endif  // AWESOMIUM_PRINT_CONFIG_H_
