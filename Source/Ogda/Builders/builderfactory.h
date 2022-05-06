//-----------------------------------------------------------------------------
//           Name: builderfactory.h
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
#include "builder.h"

#include <vector>

class BuilderFactory
{
private:
    class ActionFactoryBase
    { 
    public:
        virtual ActionBase* NewInstance() = 0;
        virtual std::string GetActionName() = 0;
        virtual bool StoreResultInDatabase() = 0;
    };

    template<class Action>
    class ActionFactory : public ActionFactoryBase
    {
        ActionBase* NewInstance() override
        {
            return new Action();
        }

        std::string GetActionName() override
        {
            return std::string(Action().GetName());
        }

        bool StoreResultInDatabase() override
        {
            return Action().StoreResultInDatabase(); 
        }
    };

    std::vector<ActionFactoryBase*> actions;
public:
    BuilderFactory();
    ~BuilderFactory();
    bool HasBuilder( const std::string& builder );
    Builder CreateBuilder( const std::string& builder, const std::string& ending, const std::string& type_pattern_re );
    bool StoreResultInDatabase( std::string builder );
};
