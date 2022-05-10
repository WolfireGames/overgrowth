//-----------------------------------------------------------------------------
//           Name: elementscanner.h
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

class TiXmlElement;
class TiXmlNode;
class Item;
class XMLSeekerBase;

typedef std::pair<const char*, const char*> elempair;

class ElementScanner {
   public:
    static void Do(
        std::vector<Item>& items,
        const Item& item,
        TiXmlNode* eRoot,
        const std::vector<elempair>& elems,
        const std::vector<const char*>& elems_ignore,
        XMLSeekerBase* callback,
        void* userdata);
};
