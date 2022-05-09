//-----------------------------------------------------------------------------
//           Name: scriptparams.h
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
#include <JSON/jsonhelper.h>

#include <map>
#include <string>
#include <vector>
#include <ostream>
#include <iostream>

class TiXmlElement;

namespace ScriptParamEditorType {
enum Type {
    UNDEFINED,
    TEXTFIELD,
    DISPLAY_SLIDER,
    COLOR_PICKER,
    CHECKBOX,
    DROPDOWN_MENU,
    MULTI_SELECT,
    CUSTOM_WINDOW_LAUNCHER
};
}

namespace ScriptParamParts {
class Editor {
   public:
    void SetTextField();
    void SetDisplaySlider(const std::string &details);
    void SetColorPicker();
    void SetCheckbox();
    void SetCustomWindowLauncher();
    void SetDropdown(const std::string &details);
    ScriptParamEditorType::Type type() const;
    Editor() : type_(ScriptParamEditorType::UNDEFINED) {}
    void SetType(ScriptParamEditorType::Type type) { type_ = type; };
    void WriteToRAM(std::vector<char> &data) const;
    void ReadFromRAM(const std::vector<char> &data, int &index);
    const std::string &GetDetails() const { return details_; }
    void SetMultiSelect(const std::string &details);
    void SetDetails(const std::string &details);
    bool operator==(const Editor &) const;
    bool operator!=(const Editor &) const;

   private:
    ScriptParamEditorType::Type type_;
    std::string details_;
};
}  // namespace ScriptParamParts

class ScriptParam {
   public:
    int GetInt() const;
    void SetInt(int val);
    float GetFloat() const;
    void SetFloat(float default_val);
    const std::string &GetString() const;
    void SetString(const std::string &default_val);
    void SetJSONFromString(const std::string &default_val);
    void SetJSON(SimpleJSONWrapper &JSON);
    SimpleJSONWrapper GetJSON();
    void WriteToXML(TiXmlElement *params) const;
    void ReadFromRAM(const std::vector<char> &data, int &index);
    void WriteToRAM(std::vector<char> &data) const;
    const ScriptParamParts::Editor &editor() const { return editor_; }
    ScriptParamParts::Editor &editor() { return editor_; }
    std::string AsString() const;
    std::string GetStringForSocket() const;
    void UpdateValuesFromSocket(const ScriptParam &data);
    ScriptParam();

    bool IsString() const { return type_ == STRING; }
    bool IsFloat() const { return type_ == FLOAT; }
    bool IsInt() const { return type_ == INT; }
    bool IsJSON() const { return type_ == JSON; }
    friend std::ostream &operator<<(std::ostream &os, const ScriptParam &data);
    bool operator==(const ScriptParam &) const;
    bool operator!=(const ScriptParam &) const;

    enum ScriptParamType {
        STRING,
        FLOAT,
        INT,
        JSON,
        OTHER
    };

   private:
    ScriptParamParts::Editor editor_;
    ScriptParamType type_;
    std::string str_val_;
    union {
        float f_val_;
        int32_t i_val_;
    };
};

typedef std::map<std::string, ScriptParam> ScriptParamMap;

class ASContext;
class ScriptParams {
   public:
    const ScriptParamMap &GetParameterMap() const;
    void SetParameterMap(const ScriptParamMap &spm);
    const std::string &GetStringVal(const std::string &val) const;
    const std::string &GetJSONValAsString(const std::string &val) const;
    SimpleJSONWrapper GetJSONVal(const std::string &val);
    static void RegisterScriptType(ASContext *context);
    void RegisterScriptInstance(ASContext *context);
    bool HasParam(const std::string &val) const;
    bool IsParamString(const std::string &val) const;
    bool IsParamFloat(const std::string &val) const;
    bool IsParamInt(const std::string &val) const;
    bool IsParamJSON(const std::string &val) const;
    float ASGetFloat(const std::string &key);
    int ASGetInt(const std::string &key);
    void ASAddInt(const std::string &key, int default_val);
    void ASAddFloat(const std::string &key, float default_val);
    void ASAddString(const std::string &key, const std::string &default_val);
    void InsertScriptParam(const std::string &key, ScriptParam sp);
    void InsertNewScriptParam(const std::string &key, ScriptParam &param);
    void ASAddJSONFromString(const std::string &key, const std::string &details);
    void ASAddJSON(const std::string &key, SimpleJSONWrapper &details);
    SimpleJSONWrapper ASGetJSON(const std::string &key);

    void ASAddIntSlider(const std::string &key, int default_val, const std::string &details);
    void ASAddIntCheckbox(const std::string &key, bool default_val);
    void ASAddFloatSlider(const std::string &key, float default_val, const std::string &details);

    void ASSetInt(const std::string &key, int num);
    void ASSetFloat(const std::string &key, float num);
    void ASSetString(const std::string &key, const std::string &val);
    void ASRemove(const std::string &key);
    void SetObjectID(uint32_t id);
    uint32_t GetObjectID() const;
    bool RenameParameterKey(const std::string &curr_name, const std::string &new_name);

   private:
    uint32_t obj_id;
    ScriptParamMap parameter_map_;
};

class TiXmlElement;
void ReadScriptParametersFromXML(ScriptParamMap &spm, const TiXmlElement *params);
void WriteScriptParamsToXML(const ScriptParamMap &pm, TiXmlElement *params);
void ReadScriptParametersFromRAM(ScriptParamMap &spm, const std::vector<char> &data);
void WriteScriptParamsToRAM(const ScriptParamMap &spm, std::vector<char> &data);
bool testScriptParamsEqual(const ScriptParamMap &spmA, const ScriptParamMap &spmB);
