///
/// @file DataPak.h
///
/// @brief The header for DataPak utilities.
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
#ifndef AWESOMIUM_DATA_PAK_H_
#define AWESOMIUM_DATA_PAK_H_
#pragma once

#include <Awesomium/Platform.h>
#include <Awesomium/WebString.h>
#include <Awesomium/DataSource.h>

namespace Awesomium {

class DataPakImpl;

///
/// Packs all files in a certain directory into a PAK file. You should probably
/// not call this during run-time; the general idea is to create a standalone
/// app that calls this function to pack your files before-hand and then use
/// DataPakSource to load those during the runtime of your application.
///
/// @param  out_file  The full file-path to the output PAK file (filename
///                   should end with `.pak`).
///
/// @param  in_dir  The path to the input directory.
///
/// @param  ignore_ext  A list of file extensions to ignore (comma-separated
///                     list, eg ".cpp,.mp4,.mp3").
///
/// @param[out]  num_files_written  The number of files written to the PAK.
///
/// @return  Returns true if successful.
///
bool OSM_EXPORT WriteDataPak(const WebString& out_file,
                             const WebString& in_dir,
                             const WebString& ignore_ext,
                             unsigned short& num_files_written);

///
/// @brief  A special DataSource that loads all resources from a PAK file.
///
/// @see WriteDataPak
///
class OSM_EXPORT DataPakSource : public DataSource {
 public:
  ///
  /// Create a DataPakSource from a PAK file.
  ///
  /// @param  pak_path  The file path to the PAK file.
  ///
  DataPakSource(const WebString& pak_path);

  ///
  /// Destroy the DataPakSource.
  ///
  virtual ~DataPakSource();

  ///
  /// This method is inherited from DataSource.
  ///
  virtual void OnRequest(int request_id,
                         const ResourceRequest& request,
                         const WebString& path);

 protected:
  DataPakImpl* pak_impl_;
};
}  // namespace Awesomium

#endif  // AWESOMIUM_DATA_PAK_H_
