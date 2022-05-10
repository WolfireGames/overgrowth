//-----------------------------------------------------------------------------
//           Name: textureref.cpp
//      Developer: Wolfire Games LLC
//    Description: TextureRef is used for generated textures. TextureAssetRef
//                 for data-related loaded textures.
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
#include "textureref.h"

#include <Graphics/textures.h>

using namespace PhoenixTextures;

void TextureRef::clear() {
    Textures::Instance()->DecrementRefCount(id);

    id = INVALID_ID;
}

bool TextureRef::valid() const {
    return id != (int)INVALID_ID;
}

TextureRef::TextureRef() {
    id = INVALID_ID;
}

TextureRef::TextureRef(int _id) {
    id = _id;
    Textures::Instance()->IncrementRefCount(id);
}

TextureRef::TextureRef(const TextureRef& _id) {
    id = _id.id;
    Textures::Instance()->IncrementRefCount(id);
}

TextureRef::~TextureRef() {
    if (id != (int)INVALID_ID) {
        Textures::Instance()->DecrementRefCount(id);
    }
}

const TextureRef& TextureRef::operator=(const TextureRef& other) {
    clear();
    id = other.id;
    Textures::Instance()->IncrementRefCount(id);
    return *this;
}

bool TextureRef::operator!=(const TextureRef& other) const {
    return id != other.id;
}

bool TextureRef::operator<(const TextureRef& other) const {
    return id < other.id;
}
