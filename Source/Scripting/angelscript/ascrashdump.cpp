//-----------------------------------------------------------------------------
//           Name: ascrashdump.cpp
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
#include "ascrashdump.h"

#include <Scripting/angelscript/ascontext.h>
#include <Compat/fileio.h>
#include <Logging/logdata.h>
#include <Internal/filesystem.h>

struct AngelScriptContext
{
    AngelScriptContext();
    bool allocated;
    char name[64];
    ASContext *context;
};

AngelScriptContext::AngelScriptContext() : allocated(false)
{

}

static const int CONTEXT_COUNT = 16;
static AngelScriptContext contexts[CONTEXT_COUNT];

void RegisterAngelscriptContext( const char* name, ASContext* ascontext )
{
    for(auto & context : contexts)
    {
        if( context.allocated == false )
        {
            strncpy( context.name, name, 64 );
            context.name[63] = '\0';
            context.allocated = true;
            context.context = ascontext;
            return;
        }
    }    
}

void DeregisterAngelscriptContext( ASContext* ascontext )
{
    for(auto & context : contexts)
    {
        if( context.context == ascontext )
        {
            context.allocated = false;
        }
    }
}

void DumpAngelscriptStates()
{
    std::ofstream output;
    char dump_path[kPathSize];
    GetASDumpPath(dump_path, kPathSize);
    
    my_ofstream_open(output,dump_path);

    if( output.is_open() ) {
        LOGI << "Dumping angelscript states to: " << dump_path << std::endl;
        for(auto & context : contexts)
        {
            if( context.allocated )
            {
                LOGI << "Dumping angelscript state for: " << context.name << std::endl;
                if( output.good() ) {
                    context.context->DumpCallstack(output);
                }
            }
        }
    } else {
        LOGE << "Unable to dump angelscript states to: " << dump_path << std::endl;
    }
}
