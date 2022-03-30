//-----------------------------------------------------------------------------
//           Name: assetloaderrors.h
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
#pragma once 

const int kLoadOk                           = 0; //Load succesful
const int kLoadErrorNoFile                  = 1; //No file specified
const int kLoadErrorMissingFile             = 2; //File given does not exist on disk
const int kLoadErrorCouldNotOpen            = 3; //fopen() failed
const int kLoadErrorCouldNotRead            = 4; //fread() failed
const int kLoadErrorCorruptFile             = 5; //Internal hash mismatch
const int kLoadErrorCouldNotOpenXML         = 6; //XML document could not be parsed/loaded
const int kLoadErrorGeneralFileError        = 7; // Error with loading data from file, unspecific
const int kLoadErrorIncompleteXML           = 8; //Missing expected values in file
const int kLoadErrorIncompatibleFileVersion = 9; //File version mismatch from expectation or capability
const int kLoadErrorMissingSubFile          = 10;//File referenced to by the main file isn't accessible
const int kLoadErrorInvalidFileEnding       = 11;//Loader demands that the file has a specific file-ending in the path

const char* GetLoadErrorString(int& v);
