//-----------------------------------------------------------------------------
//           Name: jobxmlparser.h
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

#include <XML/Parsers/xmlparserbase.h>

#include <map>
#include <vector>
#include <string>
#include <iostream>

class JobXMLParser : public XMLParserBase
{
public:
    JobXMLParser();

    uint32_t Load( const std::string& path ) override;
    bool Save( const std::string& path ) override;
    void Clear() override;

    class Searcher
    {
    public:
        int row;
        std::string type_pattern_re;
        std::string path_ending;
        std::string searcher;
    };

    class Item
    {
    public:
        int row;
        std::string type;
        std::string path;
        bool recursive;
        std::string path_ending;
        bool operator<(const Item& rhs) const;
    };

    class Builder
    {
    public:
        int row;
        std::string type_pattern_re;
        std::string path_ending;
        std::string builder;
    };

    class Generator
    {
    public:
        int row;
        std::string generator;
    };

    std::vector<std::string> inputs;
    std::vector<Searcher> searchers;
    std::vector<Item> items;
    std::vector<Builder> builders;
    std::vector<Generator> generators;
};

inline std::ostream& operator<<( std::ostream& out, const JobXMLParser::Searcher& s )
{
    out << "XmlSearcher(" << s.type_pattern_re << "," << s.path_ending << "," << s.searcher << ")";
    return out;
}

inline std::ostream& operator<<( std::ostream& out, const JobXMLParser::Item& item )
{
    out << "XmlItem(" << item.path << ":" << item.row << "," << item.type << "," << item.recursive << "," << item.path_ending  << ")";
    return out;
}

inline std::ostream& operator<<( std::ostream& out, const JobXMLParser::Builder& b )
{
    out << "XmlBuilder(" << b.type_pattern_re << "," << b.path_ending << "," << b.builder << ")";
    return out;
}

inline std::ostream& operator<<(std::ostream& out, const std::vector<JobXMLParser::Searcher>& filters)
{
    std::vector<JobXMLParser::Searcher>::const_iterator filter_it;
    filter_it = filters.begin();
    out << "XmlSearchers(";
    for( ; filter_it != filters.end(); filter_it++ )
    {
        out << *filter_it << ",";
    }
    out << ")";
    return out;
}

inline std::ostream& operator<<(std::ostream& out, const std::vector<JobXMLParser::Item>& items)
{
    out << "XmlItems("; 
    std::vector<JobXMLParser::Item>::const_iterator item_it;
    item_it = items.begin();
    for(; item_it != items.end(); item_it++ )
    {
        out << *item_it << ",";
    }
    out << ")";
    return out;
}

inline std::ostream& operator<<(std::ostream& out, const std::vector<JobXMLParser::Builder>& builders)
{
    out << "XmlBuilders("; 
    std::vector<JobXMLParser::Builder>::const_iterator builder_it;
    builder_it = builders.begin();
    for(; builder_it != builders.end(); builder_it++ )
    {
        out << *builder_it << ",";
    }
    out << ")";
    return out;
}


inline std::ostream& operator<<( std::ostream& out, const JobXMLParser& dat )
{
    out << "JobXMLParser(" << dat.searchers << "," << dat.items << "," << dat.builders << ")";
    return out;
}
