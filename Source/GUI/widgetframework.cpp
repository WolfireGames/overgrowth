//-----------------------------------------------------------------------------
//           Name: widgetframework.cpp
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
//
//   Copyright 2022 Wolfire Games LLC
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//-----------------------------------------------------------------------------
#include "widgetframework.h"

#include <Logging/logdata.h>

#ifdef WIN32
#define snprintf sprintf_s
#endif

NativeLoadingText native_loading_text;

void AddLoadingText(const std::string& text) {
    native_loading_text.AddLine(text.c_str());
    LOGI << text.c_str() << std::endl;
}

void NativeLoadingText::AddLine(const char* msg) {
    buf_mutex.lock();
    // Shift all lines up
    for (int i = kMaxLines * kMaxCharPerLine - 1, index = (kMaxLines - 1) * kMaxCharPerLine - 1;
         i >= kMaxCharPerLine;) {
        buf[i--] = buf[index--];
    }
    // Add new line
    snprintf(buf, kMaxCharPerLine, "%s", msg);
    buf_mutex.unlock();
}

void NativeLoadingText::Clear() {
    buf_mutex.lock();
    memset(buf, 0, sizeof(buf));
    buf_mutex.unlock();
};
