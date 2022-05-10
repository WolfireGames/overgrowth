//-----------------------------------------------------------------------------
//           Name: assetloaderrors.cpp
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
#include "assetloaderrors.h"

const char* GetLoadErrorString(int& v) {
    switch (v) {
        case kLoadOk:
            return "Load ok";
        case kLoadErrorNoFile:
            return "Path value is empty";
        case kLoadErrorMissingFile:
            return "File doesn't exist on disk";
        case kLoadErrorCouldNotOpen:
            return "Could not open file";
        case kLoadErrorCouldNotRead:
            return "Could not read from opened file";
        case kLoadErrorCorruptFile:
            return "File read has unexpected/corrupt data";
        case kLoadErrorCouldNotOpenXML:
            return "Could not open file as XML";
        case kLoadErrorGeneralFileError:
            return "File data error";
        case kLoadErrorIncompleteXML:
            return "Incomplete data, missing XML elements.";
        case kLoadErrorIncompatibleFileVersion:
            return "File appears to be of an unsupported format version.";
        case kLoadErrorMissingSubFile:
            return "Missing file that top file requires.";
        case kLoadErrorInvalidFileEnding:
            return "Loader demands that the file has a specific file-ending in the path";
        default:
            return "Unknown/Undefined error code";
    }
}
