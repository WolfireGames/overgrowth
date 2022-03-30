///
/// @file STLHelpers.h
///
/// @brief The header for all of the STL helper functions. This header is not
///        included in the normal API via WebCore.h on purpose. You should
///        only include this header if you use the STL in your application.
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
#ifndef AWESOMIUM_STL_HELPERS_H_
#define AWESOMIUM_STL_HELPERS_H_

#include <Awesomium/WebString.h>
#include <string>
#include <cstring>
#include <ostream>
#include <istream>

namespace Awesomium {

///
/// Helper function to convert WebString to a std::string.
///
inline std::string ToString(const WebString& str) {
  std::string result;

  if (str.IsEmpty())
    return std::string();

  unsigned int len = str.ToUTF8(NULL, 0);

  char* buffer = new char[len];
  str.ToUTF8(buffer, len);

  result.assign(buffer, len);
  delete[] buffer;

  return result;
}

///
/// Helper function to convert std::string to a WebString.
///
inline WebString ToWebString(const std::string& str) {
  return WebString::CreateFromUTF8(str.data(), str.length());
}

///
/// Stream operator to allow WebString output to an ostream
///
inline std::ostream& operator<<(std::ostream& out, const WebString& str) {
    out << ToString(str);
    return out;
}

///
/// Stream operator to allow WebString to be input from an istream
///
inline std::istream& operator>>(std::istream& in, WebString& out) {
    std::string x;
    in >> x;
    out.Append(WebString::CreateFromUTF8(x.data(), x.length()));
    return in;
}

///
/// Web String Literal (for quick, inline string definitions)
///
/// @note: Here's two examples:
///
///     WebString my_str(WSLit("literal string goes here"));
///
///     WebConfig config;
///     config.user_script = WSLit("window.testVal = 123;");
///
inline WebString WSLit(const char* string_literal) {
  return WebString::CreateFromUTF8(string_literal, strlen(string_literal));
}

}  // namespace Awesomium

#endif  // AWESOMIUM_STL_HELPERS_H_
