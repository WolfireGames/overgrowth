//-----------------------------------------------------------------------------
//           Name: textureref.h
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
#pragma once

#include <Asset/Asset/texturedummy.h>

class TextureRef {
   public:
    int id;
    void clear();

    bool valid() const;

    TextureRef();
    TextureRef(int id);
    TextureRef(int id, TextureDummyRef td);
    TextureRef(const TextureRef& id);
    ~TextureRef();

    const TextureRef& operator=(const TextureRef& other);
    bool operator!=(const TextureRef& other) const;
    bool operator<(const TextureRef& other) const;
};
