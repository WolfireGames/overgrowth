//-----------------------------------------------------------------------------
//           Name: ugc_id.cpp
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
#include "ugc_id.h"

UGCID::UGCID() : id(-1) {

}

UGCID::UGCID( int _id ) : id(_id) {
}

bool UGCID::Valid() {
    return id != -1;
}

bool UGCID::operator==( const UGCID& other ) const {
    return this->id == other.id; 
}

bool UGCID::operator!=( const UGCID& other ) const {
    return this->id != other.id;
}

std::ostream& operator<<(std::ostream& os, const UGCID &mi ) {
    os << mi.id;
    return os;
}
