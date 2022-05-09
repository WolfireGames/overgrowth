//-----------------------------------------------------------------------------
//           Name: textfile.h
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
// textfile.h: interface for reading and writing text files
// www.lighthouse3d.com
//
// You may use these functions freely.
// they are provided as is, and no warranties, either implicit,
// or explicit are given
//////////////////////////////////////////////////////////////////////
#pragma once

#include <string>

void textFileRead(const std::string &filename, std::string *dst);
int textFileWrite(char *fn, char *s);
