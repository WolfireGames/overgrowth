//-----------------------------------------------------------------------------
//           Name: xml_helper.cpp
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

#include <Internal/error.h>
#include <Internal/common.h>
#include <Internal/filesystem.h>

#include <Memory/allocation.h>
#include <XML/xml_helper.h>
#include <Compat/fileio.h>

TiXmlElement *XmlHelper::findNode(TiXmlDocument &doc, std::string &item, TiXmlElement *element) {
    // TiXmlElement* element = NULL;
    while (item.size()) {
        if (element == NULL) {
            element = doc.FirstChildElement(item.substr(0, item.find('/')));
        } else {
            element = element->FirstChildElement(item.substr(0, item.find('/')));
        }

        if (element == NULL)
            return NULL;

        if (std::string::npos != item.find('/'))
            item = item.substr(item.find('/') + 1);
        else
            item = "";
    }

    return element;
}

TiXmlElement *XmlHelper::findNode(TiXmlDocument &doc, const char *item, TiXmlElement *element) {
    std::string i = item;
    return findNode(doc, i, element);
}

bool XmlHelper::getNodeValue(TiXmlDocument &doc, const char *item, std::string &text) {
    std::string istr(item);
    TiXmlElement *element = findNode(doc, istr);

    if (element != NULL) {
        text = element->GetText();
        return true;
    }

    return false;
}

bool XmlHelper::getNodeValue(TiXmlDocument &doc, const char *item, double &d) {
    std::string text;
    bool retval = getNodeValue(doc, item, text);

    if (retval) {
        d = ::atof(text.c_str());
    }

    return retval;
}

bool XmlHelper::getNodeValue(TiXmlDocument &doc, const char *item, float &f) {
    std::string text;
    bool retval = getNodeValue(doc, item, text);

    if (retval) {
        f = (float)(::atof(text.c_str()));
    }

    return retval;
}

bool XmlHelper::getNodeValue(TiXmlDocument &doc, TiXmlElement *element, const char *item, std::string &text) {
    std::string istr(item);
    element = findNode(doc, istr, element);

    if (element != NULL) {
        text = element->GetText();
        return true;
    }

    return false;
}

bool XmlHelper::getNodeValue(TiXmlDocument &doc, TiXmlElement *element, const char *item, double &d) {
    std::string text;
    bool retval = getNodeValue(doc, element, item, text);

    if (retval) {
        d = ::atof(text.c_str());
    }

    return retval;
}

bool XmlHelper::getNodeValue(TiXmlDocument &doc, TiXmlElement *element, const char *item, float &f) {
    std::string text;
    bool retval = getNodeValue(doc, element, item, text);

    if (retval) {
        f = (float)(::atof(text.c_str()));
    }

    return retval;
}

void GetRange(const TiXmlElement *el,
              const std::string &val_str,
              const std::string &min_str,
              const std::string &max_str,
              float &min_val,
              float &max_val) {
    int result = el->QueryFloatAttribute(val_str.c_str(), &min_val);
    if (result != TIXML_SUCCESS) {
        result = el->QueryFloatAttribute(min_str.c_str(), &min_val);
        result = el->QueryFloatAttribute(max_str.c_str(), &max_val);
    } else {
        max_val = min_val;
    }
}

bool LoadXML(TiXmlDocument &doc,
             const std::string &path,
             const std::string type) {
    char abs_path[kPathSize];
    bool retry = true;
    if (FindFilePath(path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths | kAbsPath) == -1) {
        return false;
    }
    doc.LoadFile(abs_path);
    return true;
}

bool LoadXMLRetryable(TiXmlDocument &doc,
                      const std::string &path,
                      const std::string type) {
    char abs_path[kPathSize];
    bool retry = true;
    if (FindFilePath(path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths | kAbsPath) == -1) {
        ErrorResponse err;
        while (retry) {
            err = DisplayError("Error",
                               (type + " file \"" + path + "\" did not load correctly.").c_str(),
                               _ok_cancel_retry);
            if (err != _retry) {
                return false;
            }
            if (FindFilePath(path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths | kAbsPath) != -1) {
                retry = false;
            }
        }
    }
    doc.LoadFile(abs_path);
    return true;
}

uint8_t *StackLoadText(const char *path, size_t *size_out) {
    uint8_t *mem = NULL;
    long file_size = 0;
    char abs_path[kPathSize];
    bool retry = true;
    if (FindFilePath(path, abs_path, kPathSize, kDataPaths | kModPaths | kAbsPath) != -1) {
        FILE *file = my_fopen(abs_path, "rb");
        if (file) {
            fseek(file, 0, SEEK_END);
            file_size = ftell(file);
            if (file_size > 0) {
                rewind(file);

                LOG_ASSERT(file_size < (1024 * 1024));

                mem = (uint8_t *)alloc.stack.Alloc(file_size + 1);
                if (mem) {
                    size_t count = fread(mem, 1, file_size, file);
                    if ((long)count == file_size) {
                        mem[file_size] = '\0';
                    } else {
                        LOGE << "Did not read expected amount of data from file: " << abs_path << std::endl;
                        alloc.stack.Free(mem);
                        mem = NULL;
                    }
                } else {
                    LOGF << "Could not allocate " << file_size + 1 << " bytes on stack for file " << abs_path << std::endl;
                }
            }
            fclose(file);
        }
    }

    if (size_out) {
        *size_out = file_size;
    }
    return mem;
}

bool LoadText(void *&mem, const char *path) {
    FILE *file = my_fopen(path, "rb");
    if (!file) {
        return false;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    mem = OG_MALLOC(file_size + 1);
    if (!mem) {
        FatalError("Error", "Could not allocate memory for file: %s", path);
    }
    size_t read = fread(mem, 1, file_size, file);

    if (read != file_size) {
        LOGW << "Read less than ftell told us we would!" << std::endl;
    }

    ((char *)mem)[read] = '\0';

    fclose(file);

    return true;
}

bool LoadTextRetryable(void *&mem,
                       const std::string &path,
                       const std::string type) {
    char abs_path[kPathSize];
    bool retry = true;
    if (FindFilePath(path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths | kAbsPath) == -1) {
        ErrorResponse err;
        while (retry) {
            err = DisplayError("Error",
                               (type + " file \"" + path + "\" did not load correctly.").c_str(),
                               _ok_cancel_retry);
            if (err != _retry) {
                return false;
            }
            if (FindFilePath(path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths | kAbsPath) != -1) {
                retry = false;
            }
        }
    }
    LoadText(mem, abs_path);
    return true;
}

void LoadAttribIntoString(TiXmlElement *el, const char *attrib, std::string &str) {
    const char *label = el->Attribute(attrib);
    if (label) {
        str = label;
    }
}
