//-----------------------------------------------------------------------------
//           Name: modid.cpp
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
#include "modid.h"

#include <Logging/logdata.h>

using std::ostream;
using std::endl;

const ModID CoreGameModID(-2);

ModID::ModID(): id(-1) {
}

ModID::ModID(int id) : id(id) {
}

bool ModID::Valid() const {
    return id != -1;
}

bool ModID::operator==( const ModID& modid ) const {
    return this->id == modid.id;
}

bool ModID::operator!=( const ModID& modid ) const {
    return this->id != modid.id;
}

ModValidity::ModValidity() : upper(0ULL), lower(0ULL) {
    
}

ModValidity::ModValidity(uint16_t bit) {  
    if( bit < 64 ) { 
        upper = 0ULL;
        lower |= 1ULL << bit; 
    } else if( bit < 128 ) {
        upper |= 1ULL << (bit-64);
        lower = 0ULL;
    } else {
        LOGE << "Bit too high" << endl;
    }
}

ModValidity::ModValidity(uint64_t upper, uint64_t lower) : upper(upper), lower(lower) {

}

ModValidity ModValidity::Intersection(const ModValidity& other) const {
    return ModValidity(other.upper & upper, other.lower & lower);
}

ModValidity ModValidity::Union(const ModValidity& other) const {
    return ModValidity(other.upper | upper, other.lower | lower);
}

ModValidity& ModValidity::Append(const ModValidity& other) {
    upper |= other.upper;
    lower |= other.lower;
    return *this;
}

bool ModValidity::Empty() const {
    return upper == 0ULL && lower == 0ULL;
}

bool ModValidity::NotEmpty() const {
    return !Empty();
}

bool ModValidity::Intersects(const ModValidity &other) const {
    return (*this & other).NotEmpty();
}

ModValidity ModValidity::operator&(const ModValidity& rhs) const {
    return Intersection(rhs);
}

ModValidity ModValidity::operator|(const ModValidity& rhs) const {
    return Union(rhs);
}

ModValidity& ModValidity::operator|=(const ModValidity& rhs) {
    Append(rhs); 
    return *this;
}

ModValidity ModValidity::operator~() const {
    return ModValidity(~upper,~lower);   
}

bool ModValidity::operator!=(const ModValidity& rhs) const {
    return upper != rhs.upper || lower != rhs.lower;
}

ostream& operator<<(ostream& os, const ModID &mi ) {
    os << mi.id;
    return os;
}

ostream& operator<<(ostream& os, const ModValidity &mi ) {
    os << "ModValidity(" << mi.upper << "," << mi.lower << ")";
    return os;
}
