//-----------------------------------------------------------------------------
//           Name: searcherfactory.h
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
#include "searcher.h"
#include <vector>

class SearcherFactory
{
private:
    class SeekerFactoryBase
    { 
    public:
        virtual SeekerBase* NewInstance() = 0;
        virtual std::string GetSeekerName() = 0;
    };

    template<class Seeker>
    class SeekerFactory : public SeekerFactoryBase
    {
        virtual SeekerBase* NewInstance()
        {
            return new Seeker();
        }

        virtual std::string GetSeekerName()
        {
            return std::string(Seeker().GetName());
        }
    };

    std::vector<SeekerFactoryBase*> seekers;
public:
    SearcherFactory();
    ~SearcherFactory();
    bool HasSearcher( const std::string& searcher );
    Searcher CreateSearcher( const std::string& searcher, const std::string& path_ending, const std::string& type_pattern_re );
};
