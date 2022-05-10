//-----------------------------------------------------------------------------
//           Name: xmlseekerbase.cpp
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
#include "xmlseekerbase.h"

#include <string>
#include <Ogda/jobhandler.h>
#include <Internal/filesystem.h>
#include <Logging/logdata.h>

#include <tinyxml.h>
#include <XML/xml_helper.h>

std::vector<Item> XMLSeekerBase::Search(const Item& item) {
    std::string full_path = item.GetAbsPath();
    TiXmlDocument doc(full_path.c_str());
    doc.LoadFile();

    if (!doc.Error()) {
        return this->SearchXML(item, doc);
    } else {
        return std::vector<Item>();
    }
}

void XMLSeekerBase::HandleElementCallback(std::vector<Item>& items, TiXmlNode* eRoot, TiXmlElement* eElem, const Item& item, void* userdata) {
    LOGE << "Unimplemented HandleElementCallback for " << GetName() << " on " << item << std::endl;
}
