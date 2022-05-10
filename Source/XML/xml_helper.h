//-----------------------------------------------------------------------------
//           Name: xml_helper.h
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

#include <Internal/integer.h>

#include <tinyxml.h>

#include <string>

bool LoadXML(TiXmlDocument &doc,
             const std::string &path,
             const std::string type);
bool LoadXMLRetryable(TiXmlDocument &doc,
                      const std::string &path,
                      const std::string type);

void GetRange(const TiXmlElement *el,
              const std::string &val_str,
              const std::string &min_str,
              const std::string &max_str,
              float &min_val,
              float &max_val);

bool LoadText(void *&mem, const char *path);

uint8_t *StackLoadText(const char *path, size_t *size_out);

bool LoadTextRetryable(void *&mem, const std::string &path, const std::string type);

class XmlHelper {
   public:
    static TiXmlElement *findNode(TiXmlDocument &doc, std::string &item, TiXmlElement *element = NULL);
    static TiXmlElement *findNode(TiXmlDocument &doc, const char *item, TiXmlElement *element = NULL);
    static bool getNodeValue(TiXmlDocument &doc, const char *item, std::string &text);
    static bool getNodeValue(TiXmlDocument &doc, const char *item, double &d);
    static bool getNodeValue(TiXmlDocument &doc, const char *item, float &f);
    static bool getNodeValue(TiXmlDocument &doc, TiXmlElement *element, const char *item, std::string &text);
    static bool getNodeValue(TiXmlDocument &doc, TiXmlElement *element, const char *item, double &d);
    static bool getNodeValue(TiXmlDocument &doc, TiXmlElement *element, const char *item, float &f);
};

void LoadAttribIntoString(TiXmlElement *el, const char *attrib, std::string &str);
