//-----------------------------------------------------------------------------
//           Name: attributescanner.h
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
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

#include <vector>
#include <utility>
#include <string>

class TiXmlElement;
class Item;

typedef std::pair<const char*, const char*> attribpair;

class AttributeScanner
{
public:
static void Do(
    std::vector<Item>& items, 
    const Item& item, 
    TiXmlElement *eElem, 
    const std::vector<attribpair>& attribs, 
    const std::vector<const char*>& attribs_ignore);

static void DoAllSame(
    std::vector<Item>& items, 
    const Item& item, 
    TiXmlElement *eElem,
    std::string type );
};

