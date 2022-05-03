//-----------------------------------------------------------------------------
//           Name: assetbase.cpp
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
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
#include "assetbase.h"

#include <Asset/assetmanager.h>
#include <Logging/logdata.h>

#if defined(__GNUG__) && !defined(NDEBUG)
#include <cxxabi.h>
#endif

Asset::Asset(AssetManager* owner, uint32_t asset_id): 
asset_id(asset_id),
owner(owner) {

}

Asset::~Asset() 
{
    /*
    if( ref_count_ != 0 )
    {
        #if defined(__GNUG__) && !defined(NDEBUG)
        int     status;
        char   *realname;

        // typeid
        const std::type_info  &ti = typeid(*this);

        realname = abi::__cxa_demangle(ti.name(), 0, 0, &status);
        LOGE << "For some reason the ref_count in asset \"" << realname << "\" isn't 0 it's: " << ref_count_ << std::endl;
        OG_FREE(realname);
        #else

        LOGE << "For some reason the ref_count in asset \"" << GetName() << "\" isn't 0 it's: " << ref_count_ << std::endl;
        #endif
    }
    */
}

void Asset::IncrementRefCount()
{
    if( owner ) {
        owner->IncrementAsset(asset_id);
    } else {
        LOGE << "This asset was orphaned before it was destroyed, someone was holding a reference too long" << std::endl;
    }
}

void Asset::DecrementRefCount()
{
    if( owner ) {
        owner->DecrementAsset(asset_id);
    } else {
        LOGE << "This asset was orphaned before it was destroyed, someone was holding a reference too long" << std::endl;
    }
}
