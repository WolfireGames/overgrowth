//-----------------------------------------------------------------------------
//           Name: string_test.cpp
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

#include <Utility/strings.h>
#include <Utility/serialize.h>

#include <tut/tut.hpp>

#include <cmath>
#include <cstdlib>
#include <set>

static uint32_t hexflip( uint32_t v ) {
    char str[9];
    uint32_t vout;
    flags_to_string(str,v);  
    string_flags_to_uint32(&vout, str);
    return vout;
}

namespace tut 
{ 
    struct datastrings //
    { 
        int placeholder;
    };

    typedef test_group<datastrings> tg;
    tg test_group_k("Strings");
    
    typedef tg::object strings;

    template<> 
    template<> 
    void strings::test<1>() 
    { 
        ensure( "ending test", endswith("my/cool/string.png", ".png") == true );
        ensure( "ending test", endswith("my/cool/string.png", ".pnk") == false );
        
        char buffer[6];
        ensure( "strscpy exact match", strscpy(buffer,"false",6) == 0);
        ensure( "strscpy source too long", strscpy(buffer,"false",5) == SOURCE_TOO_LONG);
        ensure( "strscpy source is null", strscpy(buffer,NULL,5) == SOURCE_IS_NULL);
        ensure( "strscpy zero length, null source", strscpy(buffer,NULL,0) == DESTINATION_IS_ZERO_LENGTH);

        ensure( "saysTrue valid true", saysTrue("true") == 1 );
        ensure( "saysTrue valid false", saysTrue("false") == 0 );
        ensure( "saysTrue invalid", saysTrue("falseeeee") == -1 );
        ensure( "saysTrue null", saysTrue(NULL) == -2 );
    }

    template<> 
    template<> 
    void strings::test<2>() 
    { 
        ensure( "hex code 0", hexflip(0) == 0);
        for( uint32_t i = 1; i < 32; i++ ) {
            ensure( "hex code powers", hexflip(1U << i) == (1U << i));
        }

        for( uint32_t i = 1; i < 0xFF; i++ ) {
            ensure( "hex code powers", hexflip(i) == i);
        }

        ensure( "hex code radom", hexflip(0x12345678) == 0x12345678);
        ensure( "hex code radom", hexflip(0xFEDCBA98) == 0xFEDCBA98);
        ensure( "hex code radom", hexflip(0xDEADBEEF) == 0xDEADBEEF);
    }
}
