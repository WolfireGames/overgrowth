//-----------------------------------------------------------------------------
//           Name: Cursor.h
//      Developer: Wolfire Games LLC
//         Author: Phillip Isola
//    Description: Singleton graphical cursor.
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

#include <Asset/Asset/texture.h>

#include <string>

enum _cursor_type { DEFAULT_CURSOR, TRANSLATE_CURSOR, SCALE_CURSOR, ROTATE_CURSOR, ROTATE_CIRCLE_CURSOR, ADD_CURSOR, REMOVE_CURSOR, LINK_CURSOR, UNLINK_CURSOR, HAND_CURSOR };

class GameCursor {
public:
    GameCursor() { m_type = DEFAULT_CURSOR; m_size = 40.0f; m_width = 32; m_height = 32; m_rotation = 0;
                m_offset_x = m_offset_y = 0; visible = true;}
    void Draw();
    void SetVisible(bool _visible);
    void SetCursor(_cursor_type t);
    void SetCursor(const TextureAssetRef &t_id);
    void setCursor(std::string texture_name);
    void Rotate(float angle);
    inline void SetRotation(float angle) { m_rotation = angle; }
    inline void SetSize(int width, int height) { m_width = width; m_height = height; }

    bool visible;

private:
    TextureAssetRef hand_cursor_asset;
    TextureAssetRef link_cursor_asset;
    TextureAssetRef unlink_cursor_asset;
    TextureAssetRef remove_cursor_asset;
    TextureAssetRef add_cursor_asset;
    TextureAssetRef rotate_circle_cursor_asset;
    TextureAssetRef rotate_cursor_asset;
    TextureAssetRef scale_cursor_asset;
    TextureAssetRef translate_cursor_asset;
    TextureAssetRef default_cursor_asset;

    TextureAssetRef external_cursor_asset;

    _cursor_type m_type;
    TextureRef m_texture_ref;
    float m_rotation;
    float m_size;
    int m_width, m_height;
    int m_offset_x, m_offset_y;
};
