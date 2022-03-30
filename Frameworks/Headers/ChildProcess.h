///
/// @file ChildProcess.h
///
/// @brief The header for child process functions.
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
#ifndef AWESOMIUM_CHILD_PROCESS_H_
#define AWESOMIUM_CHILD_PROCESS_H_
#pragma once

#include <Awesomium/Platform.h>
#if defined(__WIN32__) || defined(_WIN32)
#include <Windows.h>
#endif

namespace Awesomium {

#if defined(__WIN32__) || defined(_WIN32)
  ///
  /// Returns whether or not this process is an Awesomium Child Process.
  ///
  /// @see WebConfig::child_process_path
  ///
  bool OSM_EXPORT IsChildProcess(HINSTANCE hInstance);

  ///
  /// Invokes the main method for an Awesomium Child Process.
  ///
  /// @see WebConfig::child_process_path
  ///
  int OSM_EXPORT ChildProcessMain(HINSTANCE hInstance);
#else
  ///
  /// Returns whether or not this process is an Awesomium Child Process.
  ///
  /// @see WebConfig::child_process_path
  ///
  bool OSM_EXPORT IsChildProcess(int argc, char **argv);

  ///
  /// Invokes the main method for an Awesomium Child Process.
  ///
  /// @see WebConfig::child_process_path
  ///
  int OSM_EXPORT ChildProcessMain(int argc, char **argv);
#endif

}  // namespace Awesomium

#endif  // AWESOMIUM_CHILD_PROCESS_H_
