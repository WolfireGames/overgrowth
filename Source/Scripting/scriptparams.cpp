//-----------------------------------------------------------------------------
//           Name: scriptparams.cpp
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
#include "scriptparams.h"

#include <Scripting/angelscript/ascontext.h>
#include <Internal/memwrite.h>
#include <Logging/logdata.h>
#include <Utility/assert.h>
#include <Main/engine.h>
#include <Online/online.h>

#include <tinyxml.h>

#include <sstream>

void ScriptParams::RegisterScriptInstance( ASContext* context )
{
    context->RegisterGlobalProperty("ScriptParams params", this);
}

void ScriptParams::RegisterScriptType( ASContext* context )
{
    if(context->TypeExists("ScriptParams")){
        return;
    }
    context->RegisterObjectType("ScriptParams", 0, asOBJ_REF | asOBJ_NOCOUNT);
    context->RegisterObjectMethod("ScriptParams", "const string &GetString (const string &in key)", asMETHOD(ScriptParams,GetStringVal), asCALL_THISCALL);
    context->RegisterObjectMethod("ScriptParams", "float GetFloat (const string &in key)", asMETHOD(ScriptParams,ASGetFloat), asCALL_THISCALL);
    context->RegisterObjectMethod("ScriptParams", "int GetInt (const string &in key)", asMETHOD(ScriptParams,ASGetInt), asCALL_THISCALL);
    context->RegisterObjectMethod("ScriptParams", "float SetFloat (const string &in key, float val)", asMETHOD(ScriptParams,ASSetFloat), asCALL_THISCALL);
    context->RegisterObjectMethod("ScriptParams", "int SetInt (const string &in key, int)", asMETHOD(ScriptParams,ASSetInt), asCALL_THISCALL);
    context->RegisterObjectMethod("ScriptParams", "void SetString (const string &in key, const string &in val)", asMETHOD(ScriptParams,ASSetString), asCALL_THISCALL);
    context->RegisterObjectMethod("ScriptParams", "void AddInt (const string &in key, int default_val)", asMETHOD(ScriptParams,ASAddInt), asCALL_THISCALL);
    context->RegisterObjectMethod("ScriptParams", "void AddIntSlider (const string &in key, int default_val, const string &in bounds)", asMETHOD(ScriptParams,ASAddIntSlider), asCALL_THISCALL);
    context->RegisterObjectMethod("ScriptParams", "void AddFloatSlider (const string &in key, float default_val, const string &in bounds)", asMETHOD(ScriptParams,ASAddFloatSlider), asCALL_THISCALL);
    context->RegisterObjectMethod("ScriptParams", "void AddIntCheckbox (const string &in key, bool default_val)", asMETHOD(ScriptParams,ASAddIntCheckbox), asCALL_THISCALL);
    context->RegisterObjectMethod("ScriptParams", "void AddFloat (const string &in key, float default_val)", asMETHOD(ScriptParams,ASAddFloat), asCALL_THISCALL);
    context->RegisterObjectMethod("ScriptParams", "void AddString (const string &in key, const string &in default_val)", asMETHOD(ScriptParams,ASAddString), asCALL_THISCALL);

    context->RegisterObjectMethod("ScriptParams", "void ASAddJSONFromString (const string &in key, const string &in default_val)", asMETHOD(ScriptParams,ASAddJSONFromString), asCALL_THISCALL);
    context->RegisterObjectMethod("ScriptParams", "void AddJSON (const string &in key, JSON &in default_val)", asMETHOD(ScriptParams,ASAddJSON), asCALL_THISCALL);
    context->RegisterObjectMethod("ScriptParams", "JSON GetJSON (const string &in key)", asMETHOD(ScriptParams,ASGetJSON), asCALL_THISCALL);


    context->RegisterObjectMethod("ScriptParams", "void Remove (const string &in key)", asMETHOD(ScriptParams,ASRemove), asCALL_THISCALL);
    context->RegisterObjectMethod("ScriptParams", "bool HasParam (const string &in key)", asMETHOD(ScriptParams,HasParam), asCALL_THISCALL);
    context->RegisterObjectMethod("ScriptParams", "bool IsParamInt(const string &in key)", asMETHOD(ScriptParams, IsParamInt), asCALL_THISCALL);
    context->RegisterObjectMethod("ScriptParams", "bool IsParamFloat(const string &in key)", asMETHOD(ScriptParams, IsParamFloat), asCALL_THISCALL);
    context->RegisterObjectMethod("ScriptParams", "bool IsParamString(const string &in key)", asMETHOD(ScriptParams, IsParamString), asCALL_THISCALL);
    context->RegisterObjectMethod("ScriptParams", "bool IsParamJSON(const string &in key)", asMETHOD(ScriptParams, IsParamJSON), asCALL_THISCALL);
    context->DocsCloseBrace();
}

namespace {
    std::string null_str;
}

const std::string &ScriptParams::GetStringVal(const std::string &val) const
{
    ScriptParamMap::const_iterator iter = parameter_map_.find(val);
    if(iter != parameter_map_.end()) {
        return iter->second.GetString();
    } else {
        DisplayError("Error",("No parameter \""+val+"\" of correct type.").c_str());
        return null_str;
    }
}

const std::string &ScriptParams::GetJSONValAsString(const std::string &val) const
{
    ScriptParamMap::const_iterator iter = parameter_map_.find(val);
    if(iter != parameter_map_.end() && iter->second.IsJSON() ) {
        return iter->second.GetString();
    } else {
        DisplayError("Error",("No parameter \""+val+"\" of correct type.").c_str());
        return null_str;
    }
}

SimpleJSONWrapper ScriptParams::GetJSONVal(const std::string &val)
{
    ScriptParamMap::iterator iter = parameter_map_.find(val);
    if(iter != parameter_map_.end() && iter->second.IsJSON() ) {
        return iter->second.GetJSON();
    } else {
        DisplayError("Error",("No parameter \""+val+"\" of correct type.").c_str());
        SimpleJSONWrapper nullWrapper;
        return nullWrapper;
    }

}

bool ScriptParams::HasParam(const std::string &val) const {
    return parameter_map_.find(val) != parameter_map_.end();
}

bool ScriptParams::IsParamString(const std::string &val) const {

    ScriptParamMap::const_iterator iter = parameter_map_.find(val);

    if(iter == parameter_map_.end() || !(iter->second.IsString()) )
    {
        return false;
    }
    else {
        return true;
    }

}

bool ScriptParams::IsParamFloat(const std::string &val) const {

    ScriptParamMap::const_iterator iter = parameter_map_.find(val);

    if(iter == parameter_map_.end() || !(iter->second.IsFloat()) )
    {
        return false;
    }
    else {
        return true;
    }

}

bool ScriptParams::IsParamInt(const std::string &val) const {

    ScriptParamMap::const_iterator iter = parameter_map_.find(val);

    if(iter == parameter_map_.end() || !(iter->second.IsInt()) )
    {
        return false;
    }
    else {
        return true;
    }

}

bool ScriptParams::IsParamJSON(const std::string &val) const {

    ScriptParamMap::const_iterator iter = parameter_map_.find(val);

    if(iter == parameter_map_.end() || !(iter->second.IsJSON()) )
    {
        return false;
    }
    else {
        return true;
    }
}

float ScriptParams::ASGetFloat(const std::string &key)
{
    ScriptParamMap::iterator iter = parameter_map_.find(key);
    float ret_val = -1.0f;
    if(iter != parameter_map_.end()){
        const ScriptParam &sp = iter->second;
        ret_val = sp.GetFloat();
    } else {
        DisplayError("Error",("No parameter \""+key+"\" of correct type.").c_str());
    }
    return ret_val;
}

int ScriptParams::ASGetInt(const std::string &key)
{
    ScriptParamMap::iterator iter = parameter_map_.find(key);
    int ret_val = -1;
    if(iter != parameter_map_.end()) {
        const ScriptParam &sp = iter->second;
        ret_val = sp.GetInt();
    } else {
        DisplayError("Error",("No parameter \""+key+"\" of correct type.").c_str());
    }
    return ret_val;
}

void ScriptParams::ASRemove(const std::string &key) {
    ScriptParamMap::iterator iter = parameter_map_.find(key);
    parameter_map_.erase(key);
}

void ScriptParams::SetObjectID(uint32_t id) {
	obj_id = id;
}

uint32_t ScriptParams::GetObjectID() const {
	return obj_id;
}

bool ScriptParams::RenameParameterKey(const std::string & curr_name, const std::string & next_name) {
	if (HasParam(curr_name)) {

		ScriptParam  sp = parameter_map_[curr_name];
		parameter_map_.erase(curr_name);
		parameter_map_[next_name] = sp;

		return true;
	}
	return false;
}

void ScriptParams::ASSetFloat(const std::string &key, float num)
{
	parameter_map_[key].SetFloat(num);

	if (Online::Instance()->IsActive()) {
		Online::Instance()->SendIntFloatScriptParam(obj_id, key, parameter_map_[key]);
	}
	
}

void ScriptParams::ASSetInt(const std::string &key, int num)
{
    parameter_map_[key].SetInt(num);

	if (Online::Instance()->IsActive()) {
		Online::Instance()->SendIntFloatScriptParam(obj_id, key, parameter_map_[key]);
	}
}

void ScriptParams::ASAddInt(const std::string &key, int default_val)
{
    if(parameter_map_.find(key) != parameter_map_.end()){
        return;
    }
    ScriptParam sp;
    sp.SetInt(default_val);
    parameter_map_.insert(std::pair<std::string, ScriptParam>(key, sp));
}

void ScriptParams::ASAddIntSlider(const std::string &key, int default_val, const std::string &details)
{
    ASAddInt(key, default_val);
    ScriptParam &sp = parameter_map_[key];
    sp.editor().SetDisplaySlider(details);
}

void ScriptParams::ASAddFloatSlider(const std::string &key, float default_val, const std::string &details)
{
    ASAddFloat(key, default_val);
    ScriptParam &sp = parameter_map_[key];
    sp.editor().SetDisplaySlider(details);
}

void ScriptParams::ASAddIntCheckbox(const std::string &key, bool default_val)
{
    ASAddInt(key, default_val);
    ScriptParam &sp = parameter_map_[key];
    sp.editor().SetCheckbox();
}

void ScriptParams::ASAddFloat(const std::string &key, float default_val)
{
    if(parameter_map_.find(key) != parameter_map_.end()){
        return;
    }
    ScriptParam sp;
    sp.SetFloat(default_val);
    parameter_map_.insert(std::pair<std::string, ScriptParam>(key, sp));
}

void ScriptParams::ASAddString(const std::string &key, const std::string &default_val) {
    if(parameter_map_.find(key) != parameter_map_.end()){
        return;
    }
    ScriptParam sp;
    sp.SetString(default_val);
    parameter_map_.insert(std::pair<std::string, ScriptParam>(key, sp));
}

void ScriptParams::InsertScriptParam(const std::string & key, ScriptParam sp) {
	if (parameter_map_.find(key) != parameter_map_.end()) {
		parameter_map_[key].UpdateValuesFromSocket(sp);
	} else { 
		parameter_map_[key] = sp;
	}
}
 
void ScriptParams::InsertNewScriptParam(const std::string& key, ScriptParam& param) {
	parameter_map_[key] = param;
	if (Online::Instance()->IsActive()) {
		Online::Instance()->SendStringScriptParam(obj_id, key, parameter_map_[key]);
	}
}

void ScriptParams::ASAddJSONFromString(const std::string &key, const std::string &default_val) {
    if(parameter_map_.find(key) != parameter_map_.end()){
        return;
    }
    ScriptParam sp;
    sp.SetJSONFromString(default_val);
    parameter_map_.insert(std::pair<std::string, ScriptParam>(key, sp));
}

void ScriptParams::ASAddJSON(const std::string &key, SimpleJSONWrapper& details) {
    if(parameter_map_.find(key) != parameter_map_.end()){
        return;
    }
    ScriptParam sp;
    sp.SetJSON(details);
    parameter_map_.insert(std::pair<std::string, ScriptParam>(key, sp));
}

SimpleJSONWrapper ScriptParams::ASGetJSON(const std::string &key) {

    ScriptParamMap::iterator iter = parameter_map_.find(key);
    SimpleJSONWrapper ret_val;
    if(iter != parameter_map_.end()) {
        ScriptParam &sp = iter->second;
        ret_val = sp.GetJSON();
    } else {
        DisplayError("Error",("No parameter \""+key+"\" of correct type.").c_str());
    }
    return ret_val;
}

const ScriptParamMap & ScriptParams::GetParameterMap() const
{
    return parameter_map_;
}

void ScriptParams::SetParameterMap( const ScriptParamMap& spm )
{
    parameter_map_ = spm;
}

void ScriptParams::ASSetString( const std::string &key, const std::string& val ) {
    parameter_map_[key].SetString(val);

	if (Online::Instance()->IsActive()) {
		Online::Instance()->SendStringScriptParam(obj_id, key, parameter_map_[key]);
	}
}

std::ostream & operator<<(std::ostream & os, const ScriptParam & data) {
	os << "float " << data.f_val_ << " int: " << data.i_val_ << " string: " << data.str_val_ << std::endl;
	return os;
}

void ReadScriptParametersFromXML( ScriptParamMap &spm, const TiXmlElement* params )
{
    LOG_ASSERT(params);
    const TiXmlElement* param = params->FirstChildElement("parameter");
    while(param){
        std::string name = param->Attribute("name");
        std::string type_str = param->Attribute("type");
        ScriptParam sp;
        if(type_str == "int"){
            int val;
            param->QueryIntAttribute("val", &val);
            sp.SetInt(val);
        } else if(type_str == "float"){
            float val = 0.0f;
            param->QueryFloatAttribute("val", &val);
            sp.SetFloat(val);
        } else if(type_str == "string"){
            sp.SetString(param->Attribute("val"));
        } else if(type_str == "json"){
            sp.SetJSONFromString(param->Attribute("val"));
        }
        spm[name] = sp;
        param = param->NextSiblingElement();
    }
}

void WriteScriptParamsToXML( const ScriptParamMap & pm, TiXmlElement* params )
{
    for(const auto & iter : pm){
        TiXmlElement* param = new TiXmlElement("parameter");
        param->SetAttribute("name",iter.first.c_str());
        const ScriptParam &sp = iter.second;
        sp.WriteToXML(param);
        params->LinkEndChild(param);
    }
}

void ReadScriptParametersFromRAM( ScriptParamMap &spm, const std::vector<char> &data )
{
    int index = 0;
    int num_params;
    memread(&num_params, sizeof(int), 1, data, index);
    for(int i=0; i<num_params; ++i){
        std::string name;
        {
            int str_len;
            memread(&str_len, sizeof(int), 1, data, index);
            name.resize(str_len);
            memread(&name[0], sizeof(char), str_len, data, index);
        }
        ScriptParam sp;
        sp.ReadFromRAM(data, index);
        spm[name] = sp;
    }
}

void WriteScriptParamsToRAM( const ScriptParamMap& spm, std::vector<char> &data )
{
    int num_params = spm.size();
    memwrite(&num_params, sizeof(int), 1, data);
    ScriptParamMap::const_iterator iter = spm.begin();
    for(; iter != spm.end(); ++iter){
        {
            const std::string &str = iter->first;
            int str_len = str.size();
            memwrite(&str_len, sizeof(int), 1, data);
            memwrite(&str[0], sizeof(char), str_len, data);
        }
        const ScriptParam& sp = iter->second;
        sp.WriteToRAM(data);
    }
}

const std::string& ScriptParam::GetString() const {
    if(type_ != STRING && type_ != JSON ){
        DisplayError("Error", "Calling GetString() on parameter of invalid type.");
    }
    return str_val_;
}

float ScriptParam::GetFloat() const {
    switch(type_){
        case FLOAT:
            return f_val_;
        case INT:
            return (float)i_val_;
        case STRING:
            return (float)atof(str_val_.c_str());
        default:
            DisplayError("Error", "Calling GetFloat() on parameter of invalid type.");
            return -1.0f;
    }
}

int ScriptParam::GetInt() const {
    switch(type_){
        case FLOAT:
            return (int)f_val_;
        case INT:
            return i_val_;
        case STRING:
            if(str_val_ == "true"){
                return 1;
            } else if(str_val_ == "false"){
                return 0;
            }
            return atoi(str_val_.c_str());
        default:
            DisplayError("Error", "Calling GetInt() on parameter of invalid type.");
            return -1;
    }
}

void ScriptParam::SetInt( int val ) {
    type_ = INT;
    i_val_ = val;
    editor_.SetTextField();
}

void ScriptParam::SetFloat( float val ) {
    type_ = FLOAT;
    f_val_ = val;
    editor_.SetTextField();
}

void ScriptParam::SetString( const std::string &val ) {
    type_ = STRING;
    str_val_ = val;
    editor_.SetTextField();
}

void ScriptParam::SetJSONFromString( const std::string &val ) {
    type_ = JSON;
    str_val_ = val;
    editor_.SetCustomWindowLauncher();
}

void ScriptParam::SetJSON( SimpleJSONWrapper& JSONVal ) {
    type_ = JSON;
    str_val_ = JSONVal.writeString(false);
    editor_.SetCustomWindowLauncher();
}

SimpleJSONWrapper ScriptParam::GetJSON() {
    if(type_ != JSON ){
        DisplayError("Error", "Calling GetJSON() on parameter of invalid type.");
    }

    SimpleJSONWrapper JSONVal;

    if( JSONVal.parseString( str_val_ ) ) {
        return JSONVal;
    }
    else {
        DisplayError("Error", "Cannot parse JSON value.");
        return JSONVal;
    }
}

void ScriptParam::WriteToXML( TiXmlElement* param ) const {
    switch(type_){
        case INT:
            param->SetAttribute("type", "int");
            param->SetAttribute("val", i_val_);
            break;
        case FLOAT:
            param->SetAttribute("type", "float");
            param->SetDoubleAttribute("val", f_val_);
            break;
        case STRING:
            param->SetAttribute("type", "string");
            param->SetAttribute("val", str_val_.c_str());
            break;
        case JSON:
            param->SetAttribute("type", "json");
            param->SetAttribute("val", str_val_.c_str());
            break;
        case OTHER:
            LOGW << "Got unhandled type OTHER" <<std::endl;
            break;
    }
}

void ScriptParam::ReadFromRAM( const std::vector<char> & data, int &index ) {
    memread(&type_, sizeof(int), 1, data, index);
    switch(type_){
        case INT:
            memread(&i_val_, sizeof(int), 1, data, index);
            break;
        case FLOAT:
            memread(&f_val_, sizeof(float), 1, data, index);
            break;
        case STRING:
        case JSON:
            int str_len;
            memread(&str_len, sizeof(int), 1, data, index);
            str_val_.resize(str_len);
            memread(&str_val_[0], sizeof(char), str_len, data, index);
            break;
        case OTHER:
            LOGW << "Got unhandled type OTHER" <<std::endl;
            break;

    }
    editor_.ReadFromRAM(data, index);
}

void ScriptParam::WriteToRAM( std::vector<char> & data ) const {
    int int_type = type_;
    memwrite(&int_type, sizeof(int), 1, data);
    switch(int_type){
        case INT:
            memwrite(&i_val_, sizeof(int), 1, data);
            break;
        case FLOAT:
            memwrite(&f_val_, sizeof(float), 1, data);
            break;
        case STRING:
        case JSON:
            int str_len = str_val_.size();
            memwrite(&str_len, sizeof(int), 1, data);
            memwrite(&str_val_[0], sizeof(char), str_len, data);
            break;
    }
    editor_.WriteToRAM(data);
}

std::string ScriptParam::AsString() const {
    std::ostringstream oss;
    switch(type_){
        case INT:
            oss << i_val_;
            break;
        case FLOAT:
            oss << f_val_;
            break;
        case STRING:
            oss << "\"" << str_val_ << "\"";
            break;
        case JSON:
            oss  << str_val_;
            break;
        case OTHER:
            LOGW << "Got unhandled type OTHER" <<std::endl;
            break;
    }
    return oss.str();
}

std::string ScriptParam::GetStringForSocket() const {
	return str_val_;
}

void ScriptParam::UpdateValuesFromSocket(const ScriptParam & data) {
	this->f_val_ = data.f_val_;
	this->str_val_ = data.str_val_;
	this->type_ = data.type_;  
}

ScriptParam::ScriptParam():
    type_(OTHER), i_val_(0)
{}

bool ScriptParam::operator==( const ScriptParam& other ) const {
    return ( type_ == other.type_ ) && ( i_val_ == other.i_val_ ) &&
           ( f_val_ == other.f_val_ ) && ( str_val_ == other.str_val_ ) &&
    ( editor_ == other.editor_ );
}

bool ScriptParam::operator!=( const ScriptParam& other ) const {
    return !(*this == other);
}

void ScriptParamParts::Editor::SetTextField() {
    type_ = ScriptParamEditorType::TEXTFIELD;
}

void ScriptParamParts::Editor::SetDisplaySlider( const std::string &details ) {
    type_ = ScriptParamEditorType::DISPLAY_SLIDER;
    details_ = details;
}

void ScriptParamParts::Editor::SetColorPicker( ) {
    type_ = ScriptParamEditorType::COLOR_PICKER;
}

void ScriptParamParts::Editor::SetCheckbox() {
    type_ = ScriptParamEditorType::CHECKBOX;
}

void ScriptParamParts::Editor::SetDropdown( const std::string &details ) {
    type_ = ScriptParamEditorType::DROPDOWN_MENU;
    details_ = details;
}

void ScriptParamParts::Editor::SetMultiSelect( const std::string &details ) {
    type_ = ScriptParamEditorType::MULTI_SELECT;
    details_ = details;
}

void ScriptParamParts::Editor::SetDetails(const std::string & details) {
	this->details_ = details;
}

void ScriptParamParts::Editor::SetCustomWindowLauncher(){
    type_ = ScriptParamEditorType::CUSTOM_WINDOW_LAUNCHER;
}

ScriptParamEditorType::Type ScriptParamParts::Editor::type() const {
    return type_;
}

bool ScriptParamParts::Editor::operator==( const Editor& other ) const {
    return ( type_ == other.type_ ) && ( details_ == other.details_ );
}

bool ScriptParamParts::Editor::operator!=( const Editor& other ) const {
    return !(*this == other);
}


void ScriptParamParts::Editor::WriteToRAM( std::vector<char> & data ) const {
    int int_type = type_;
    memwrite(&int_type, sizeof(int), 1, data);
    int str_len = details_.size();
    memwrite(&str_len, sizeof(int), 1, data);
    memwrite(&details_[0], sizeof(char), str_len, data);
}

void ScriptParamParts::Editor::ReadFromRAM( const std::vector<char> & data, int & index ) {
    memread(&type_, sizeof(int), 1, data, index);
    int str_len;
    memread(&str_len, sizeof(int), 1, data, index);
    details_.resize(str_len);
    memread(&details_[0], sizeof(char), str_len, data, index);
}


bool testScriptParamsEqual( const ScriptParamMap& spmA, const ScriptParamMap& spmB ) {

    //proceed to compare maps here
    if(spmA.size() != spmB.size())
        return false;  // differing sizes, they are not the same

    ScriptParamMap::const_iterator i, j;
    for(i = spmA.begin(), j = spmB.begin(); i != spmA.end(); ++i, ++j)
    {
        if(*i != *j)
            return false;
    }

    return true;

}
