//-----------------------------------------------------------------------------
//           Name: Cursor.cpp
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
#include "Cursor.h"

#include <Graphics/textures.h>
#include <Graphics/graphics.h>

#include <UserInput/input.h>
#include <Main/engine.h>

void GameCursor::Draw() {
    if (Graphics::Instance()->media_mode()) {
        UIShowCursor(true);
        return;
    }

    if (!visible || !m_texture_ref.valid()) return;

    CHECK_GL_ERROR();
    int* mouse_pos = Input::Instance()->getMouse().pos_;
    int* window_dims = Graphics::Instance()->window_dims;
    vec3 draw_pos(
        (float)(mouse_pos[0] + m_offset_x),
        (float)((window_dims[1] - mouse_pos[1]) - m_offset_y),
        0.0f);
    Textures::Instance()->drawTexture(m_texture_ref, draw_pos,
                                      m_size, m_rotation);
    CHECK_GL_ERROR();
}

void GameCursor::SetCursor(_cursor_type t) {
    // std::pair<int, int> dimensions;
    switch (t) {
        case DEFAULT_CURSOR:
            if (!default_cursor_asset.valid()) {
                default_cursor_asset = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/Cursors/cursor_nocompress.png", PX_SRGB | PX_NOREDUCE, 0x0);
            }
            m_texture_ref = default_cursor_asset->GetTextureRef();
            m_type = DEFAULT_CURSOR;
            m_offset_x = 12;
            m_offset_y = 15;
            m_size = 32.0f;
            break;
        case TRANSLATE_CURSOR:
            if (!translate_cursor_asset.valid()) {
                translate_cursor_asset = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/Cursors/translate_nocompress.png", PX_SRGB | PX_NOREDUCE, 0x0);
            }
            m_texture_ref = translate_cursor_asset->GetTextureRef();
            m_type = TRANSLATE_CURSOR;
            m_size = 32.0f;
            m_offset_x = 12;
            m_offset_y = 15;
            break;
        case SCALE_CURSOR:
            if (!scale_cursor_asset.valid()) {
                scale_cursor_asset = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/Cursors/scale_nocompress.png", PX_SRGB | PX_NOREDUCE, 0x0);
            }
            m_texture_ref = scale_cursor_asset->GetTextureRef();
            m_type = SCALE_CURSOR;
            m_size = 32.0f;
            m_offset_x = 12;
            m_offset_y = 15;
            break;
        case ROTATE_CURSOR:
            if (!rotate_cursor_asset.valid()) {
                rotate_cursor_asset = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/Cursors/rotate_nocompress.png", PX_SRGB | PX_NOREDUCE, 0x0);
            }
            m_texture_ref = rotate_cursor_asset->GetTextureRef();
            m_type = ROTATE_CURSOR;
            m_size = 32.0f;
            m_offset_x = 12;
            m_offset_y = 15;
            break;
        case ROTATE_CIRCLE_CURSOR:
            if (!rotate_circle_cursor_asset.valid()) {
                rotate_circle_cursor_asset = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/Cursors/rotate_circle_nocompress.png", PX_SRGB | PX_NOREDUCE, 0x0);
            }
            m_texture_ref = rotate_circle_cursor_asset->GetTextureRef();
            m_type = ROTATE_CIRCLE_CURSOR;
            m_size = 36.0f;
            m_offset_x = m_offset_y = 0;
            break;
        case ADD_CURSOR:
            if (!add_cursor_asset.valid()) {
                add_cursor_asset = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/Cursors/plus_nocompress.png", PX_SRGB | PX_NOREDUCE, 0x0);
            }
            m_texture_ref = add_cursor_asset->GetTextureRef();
            m_type = ADD_CURSOR;
            m_size = 32.0f;
            m_offset_x = 12;
            m_offset_y = 15;
            break;
        case REMOVE_CURSOR:
            if (!remove_cursor_asset.valid()) {
                remove_cursor_asset = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/Cursors/minus_nocompress.png", PX_SRGB | PX_NOREDUCE, 0x0);
            }
            m_texture_ref = remove_cursor_asset->GetTextureRef();
            m_type = REMOVE_CURSOR;
            m_size = 32.0f;
            m_offset_x = 12;
            m_offset_y = 15;
            break;
        case LINK_CURSOR:
            if (!link_cursor_asset.valid()) {
                link_cursor_asset = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/Cursors/link_nocompress.png", PX_SRGB | PX_NOREDUCE, 0x0);
            }
            m_texture_ref = link_cursor_asset->GetTextureRef();
            m_type = LINK_CURSOR;
            m_size = 32.0f;
            m_offset_x = 12;
            m_offset_y = 15;
            break;
        case UNLINK_CURSOR:
            if (!unlink_cursor_asset.valid()) {
                unlink_cursor_asset = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/Cursors/unlink_nocompress.png", PX_SRGB | PX_NOREDUCE, 0x0);
            }
            m_texture_ref = unlink_cursor_asset->GetTextureRef();
            m_type = UNLINK_CURSOR;
            m_size = 32.0f;
            m_offset_x = 12;
            m_offset_y = 15;
            break;
        case HAND_CURSOR:
            if (!hand_cursor_asset.valid()) {
                hand_cursor_asset = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/Cursors/pointer_nocompress.png", PX_SRGB | PX_NOREDUCE, 0x0);
            }
            m_texture_ref = hand_cursor_asset->GetTextureRef();
            m_type = HAND_CURSOR;
            m_size = 32.0f;
            m_offset_x = 12;
            m_offset_y = 15;
            break;
    }
}

void GameCursor::SetVisible(bool _visible) {
    visible = _visible;
}

void GameCursor::SetCursor(const TextureAssetRef& t_id) {
    external_cursor_asset = t_id;
    m_texture_ref = external_cursor_asset->GetTextureRef();
}

void GameCursor::setCursor(std::string texture_name) {
    external_cursor_asset = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(texture_name.c_str(), PX_SRGB | PX_NOREDUCE, 0x0);
    m_texture_ref = external_cursor_asset->GetTextureRef();
}

void GameCursor::Rotate(float angle) {
    m_rotation += angle;
}
