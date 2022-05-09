//-----------------------------------------------------------------------------
//           Name: imui.cpp
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

#include <Graphics/graphics.h>
#include <Graphics/shaders.h>
#include <Graphics/vbocontainer.h>

#include <Timing/timingevent.h>
#include <Timing/intel_gl_perf.h>

#include <UserInput/input.h>
#include <GUI/IMUI/imui.h>
#include <Wrappers/glm.h>
#include <Internal/profiler.h>
#include <Main/engine.h>

#include <utf8/utf8.h>

#include <cmath>
#include <algorithm>

namespace {
GLState gl_state;
}

/**
 * IMUIRenderable
 **/

void IMUIRenderable::setPosition(vec3 newPos) {
    pos = newPos;
}

void IMUIRenderable::setColor(vec4 newColor) {
    color = newColor;
}

void IMUIRenderable::setClipping(vec2 offset, vec2 size) {
    enableClip = true;
    clipPosition = offset;
    clipSize = size;
}

void IMUIRenderable::disableClipping() {
    enableClip = false;
}

void IMUIRenderable::setRotation(float newRotation) {
    rotation = newRotation;
}

/**
 * IMUIImage
 **/

IMUIImage::IMUIImage(std::string filename) : IMUIRenderable(IMIURimage) {
    loadImage(filename);
}

bool IMUIImage::loadImage(std::string filename) {
    textureRef = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(filename, PX_NOMIPMAP | PX_NOREDUCE | PX_NOCONVERT, 0x0);

    // If this was loaded then we can populate some attributes
    if (textureRef.valid()) {
        // Record the original texture dimensions
        textureSize[0] = (float)(Textures::Instance()->getReducedWidth(textureRef));

        textureSize[1] = (float)(Textures::Instance()->getReducedHeight(textureRef));

        // Assume that we're going to render the whole thing unless otherwise told
        textureOffset = vec2(0.0);
        textureSourceSize = textureSize;

        // No scaling by default
        renderSize = textureSize;

        return true;
    } else {
        // We're not good, tell the caller
        return false;
    }
}

bool IMUIImage::isValid() const {
    return textureRef.valid();
}

float IMUIImage::getTextureWidth() const {
    return textureSize[0];
}

float IMUIImage::getTextureHeight() const {
    return textureSize[1];
}

void IMUIImage::setRenderOffset(vec2 offset, vec2 size) {
    // Make sure we have a valid texture
    if (!isValid()) return;

    // If the offset is greater than the texture size, let's wrap around
    textureOffset = vec2((float)fmod(offset[0], textureSize[0]),
                         (float)fmod(offset[1], textureSize[1]));

    // Make sure the requested size doesn't go over the size of the texture
    textureSourceSize = vec2(min(size[0], textureSize[0] - textureOffset[0]),
                             min(size[1], textureSize[1] - textureOffset[1]));
}

void IMUIImage::setRenderSize(vec2 size) {
    renderSize = size;
}

/**
 * IMUIText
 **/

void IMUIText::setText(std::string& _text) {
    text = _text;

    // Make sure we've been properly initialized with a font
    if (owner != NULL) {
        owner->deriveFontDimensions(*this);
    } else {
        dimensions = vec2(0.0);
        boundingBox = vec2(0.0);
    }
}

void IMUIText::setRotation(float newRotation) {
    rotation = newRotation;

    // Make sure we've been properly initialized with a font
    if (owner != NULL && text != "") {
        owner->deriveFontDimensions(*this);
    } else {
        dimensions = vec2(0.0);
        boundingBox = vec2(0.0);
    }
}

IMUIContext::IMUIContext() : image_index_vbo(new VBOContainer()),
                             image_data_vbo(new VBORingContainer(sizeof(GLfloat) * 4 * 4, kVBOFloat | kVBODynamic)),
                             character_data_vbo(new VBORingContainer(sizeof(GLfloat) * kMaxCharacters * 4 * kFloatsPerVert, kVBOFloat | kVBODynamic)),
                             character_index_vbo(new VBORingContainer(sizeof(GLuint) * kMaxCharacters * 6, kVBOElement | kVBODynamic)) {
}

void IMUIContext::Init() {
    hot_ = -1;
    active_ = -1;
    lmb_state = kMouseStillUp;

    gl_state.blend = true;
    gl_state.cull_face = false;
    gl_state.depth_test = false;
    gl_state.depth_write = false;
}

bool IMUIContext::DoButton(int id, vec2 top_left, vec2 bottom_right, UIState& ui_state) {
    bool mouse_over = false;
    if (mouse_pos[0] > top_left[0] && mouse_pos[0] < bottom_right[0] &&
        mouse_pos[1] > top_left[1] && mouse_pos[1] < bottom_right[1]) {
        mouse_over = true;
    }

    return DoButtonMouseOver(id, mouse_over, ui_state);
}

bool IMUIContext::DoButtonMouseOver(int id, bool mouse_over, UIState& ui_state) {
    bool result = false;
    if (active_ == id && lmb_state == kMouseUp) {
        if (hot_ == id) {
            result = true;
        }
        active_ = -1;
    } else if (hot_ == id && lmb_state == kMouseDown) {
        active_ = id;
    }
    if (mouse_over) {
        if (active_ == -1 && hot_ == -1) {
            hot_ = id;
        }
    } else if (hot_ == id) {
        hot_ = -1;
    }
    if (active_ == id) {
        ui_state = kActive;
    } else if (hot_ == id) {
        ui_state = kHot;
    } else {
        ui_state = kNothing;
    }
    return result;
}

void IMUIContext::ClearHot() {
    hot_ = -1;
}

void IMUIContext::UpdateControls() {
    Input* user_input = Input::Instance();
    const Mouse& mouse = user_input->getMouse();
    mouse_pos[0] = (float)mouse.pos_[0];
    mouse_pos[1] = (float)(Graphics::Instance()->window_dims[1] - mouse.pos_[1]);
    switch (lmb_state) {
        case kMouseUp:
            lmb_state = kMouseStillUp;
            // printf("Mouse: Still up\n");
        case kMouseStillUp:
            if (mouse.mouse_down_[Mouse::LEFT]) {
                lmb_state = kMouseDown;
                // printf("Mouse: Down\n");
            }
            break;
        case kMouseDown:
            lmb_state = kMouseStillDown;
            // printf("Mouse: Still down\n");
        case kMouseStillDown:
            if (!mouse.mouse_down_[Mouse::LEFT]) {
                lmb_state = kMouseUp;
                // printf("Mouse: up\n");
            }
            break;
    }
}

vec2 IMUIContext::getMousePosition() {
    return mouse_pos;
}

IMUIContext::ButtonState IMUIContext::getLeftMouseState() {
    return lmb_state;
}

IMUIContext::~IMUIContext() {
    // Free up our resources
    clearTextAtlases();
}

void IMUIContext::queueImage(IMUIImage& newImage) {
    IMUIImage* queuedImage = new IMUIImage(newImage);
    renderQueue.push_back(queuedImage);
}

IMUIText IMUIContext::makeText(std::string const& fontName, int size, int fontFlags, int renderFlags) {
    IMUIText newText;

    newText.fontName = fontName;
    newText.size = size;
    newText.fontFlags = fontFlags;
    newText.renderFlags = renderFlags;

    newText.owner = this;

    return newText;
}

void IMUIContext::deriveFontDimensions(IMUIText& text) {
    // Make sure we have a meaningful text object
    if (text.owner == NULL || text.text == "" || text.fontName == "") {
        // If not make sure that the dimensions are sensible
        text.dimensions = vec2(0.0);
        text.boundingBox = vec2(0.0);
        return;
    }

    TextAtlas* atlas = getTextAtlas(text.fontName, text.size, text.fontFlags);

    if (atlas == NULL) {
        std::string errorMessage = "Unable to find font " + text.fontName;
        text.dimensions = vec2(0.0);
        text.boundingBox = vec2(0.0);
        DisplayError("Error", errorMessage.c_str());
        return;
    }

    // Now that we're sure everything is ok, we can finally figure out how big our text is
    vec2 dimensions(0, atlas->pixel_height);

    // Get our singleton
    FontRenderer* font_renderer = FontRenderer::Instance();

    int last_c = -1;
    int length = text.text.length();
    int currentLineX = 0;

    std::vector<uint32_t> utf32_string;
    try {
        utf8::utf8to32(text.text.begin(), text.text.end(), std::back_inserter(utf32_string));
    } catch (const utf8::not_enough_room& ner) {
        LOGE << "Got utf8 exception \"" << ner.what() << "\" this might indicate invalid utf-8 data" << std::endl;
    }

    for (unsigned int c : utf32_string) {
        if (c == '\n') {
            if (currentLineX > dimensions[0]) {
                dimensions[0] = (float)currentLineX;
            }

            dimensions[1] += atlas->pixel_height;

            // Reset our x position
            currentLineX = 0;
            last_c = -1;

            // Go onto the next character
            continue;
        }

        std::vector<CharacterInfo>::iterator character;
        for (character = atlas->alphabet.begin();
             character != atlas->alphabet.end();
             character++) {
            if (character->codepoint == c)
                break;
        }

        // Make sure it's a character we can render
        if (character != atlas->alphabet.end()) {
            // Put in the approriate ammount of space
            int kerning_x = 0;
            if (last_c != -1) {
                FT_Vector vec;
                font_renderer->GetKerning(0, last_c, c, &vec);
                kerning_x = vec.x / 64;
            }

            currentLineX += kerning_x;

            currentLineX += character->advance_x;

            last_c = c;
        }
    }

    if (currentLineX > dimensions[0]) {
        dimensions[0] = (float)currentLineX;
    }

    text.dimensions = dimensions;

    // Now we compute the dimensions under a given rotation
    if (text.rotation == 0) {
        text.boundingBox = dimensions;
    } else {
        float halfX = text.dimensions[0] / 2.0f;
        float halfY = text.dimensions[1] / 2.0f;

        // Define four coordinates
        vec2 UL(-halfX, halfY);
        vec2 UR(halfX, halfY);
        vec2 LL(-halfX, -halfY);
        vec2 LR(halfX, -halfY);

        // Get the rotation in radians
        float rotation = text.rotation * (PI_f / 180);

        vec2 ULP((UL[0] * cosf(rotation)) - (UL[1] * sinf(rotation)),
                 (UL[0] * sinf(rotation)) + (UL[1] * cosf(rotation)));

        vec2 URP((UR[0] * cosf(rotation)) - (UR[1] * sinf(rotation)),
                 (UR[0] * sinf(rotation)) + (UR[1] * cosf(rotation)));

        vec2 LLP((LL[0] * cosf(rotation)) - (LL[1] * sinf(rotation)),
                 (LL[0] * sinf(rotation)) + (LL[1] * cosf(rotation)));

        vec2 LRP((LR[0] * cosf(rotation)) - (LR[1] * sinf(rotation)),
                 (LR[0] * sinf(rotation)) + (LR[1] * cosf(rotation)));

        // Now find the extremal points
        float maxX = max(max(ULP[0], URP[0]), max(LLP[0], LRP[0]));
        float minX = min(min(ULP[0], URP[0]), min(LLP[0], LRP[0]));
        float maxY = max(max(ULP[1], URP[1]), max(LLP[1], LRP[1]));
        float minY = min(min(ULP[1], URP[1]), min(LLP[1], LRP[1]));

        // Finally write out our bouding values
        text.boundingBox = vec2(fabsf(maxX) + fabsf(minX), fabsf(maxY) + fabsf(minY));
    }
}

void IMUIContext::queueText(IMUIText& newText) {
    IMUIText* queuedText = new IMUIText(newText);
    renderQueue.push_back(queuedText);
}

void IMUIContext::clearTextAtlases() {
    for (int i = 0; i < atlases.num_atlases; ++i) {
        atlases.cached[i].atlas.Dispose();
    }
    // Reset the cached atlas counter
    atlases.num_atlases = 0;
}

class RenderableZCompare {
   public:
    bool operator()(const IMUIRenderable* a, const IMUIRenderable* b) {
        return a->pos[2] > b->pos[2];
    }
};

struct HUDImageGLState {
    GLState gl_state;
    HUDImageGLState() {
        gl_state.blend = true;
        gl_state.blend_src = GL_SRC_ALPHA;
        gl_state.blend_dst = GL_ONE_MINUS_SRC_ALPHA;
        gl_state.cull_face = false;
        gl_state.depth_test = false;
        gl_state.depth_write = false;
    }
};

static const HUDImageGLState hud_gl_state;

void IMUIContext::render() {
    PROFILER_GPU_ZONE(g_profiler_ctx, "IMUIContext::render");

    Graphics* graphics = Graphics::Instance();
    Shaders* shaders = Shaders::Instance();
    Textures* textures = Textures::Instance();

    // Sort renderables so we can draw them in order of depth
    std::sort(renderQueue.begin(), renderQueue.end(), RenderableZCompare());

    int shader_id = shaders->returnProgram("simple_2d #TEXTURE #FLIPPED");
    shaders->setProgram(shader_id);
    int vert_coord_id = shaders->returnShaderAttrib("vert_coord", shader_id);
    int tex_coord_id = shaders->returnShaderAttrib("tex_coord", shader_id);
    int mvp_mat_id = shaders->returnShaderVariable("mvp_mat", shader_id);
    int color_id = shaders->returnShaderVariable("color", shader_id);
    graphics->setGLState(hud_gl_state.gl_state);
    glm::mat4 proj_mat;

    const float winWidth = (float)graphics->window_dims[0];
    const float winHeight = (float)graphics->window_dims[1];

    const float max_aspect_ratio = 16.0f / 9.0f;
    float aspect_ratio = winWidth / winHeight;
    float xOffset = 0.0f;

    while (renderQueue.size() > 0) {
        IMUIRenderable* renderable = renderQueue[renderQueue.size() - 1];
        renderQueue.resize(renderQueue.size() - 1);

        xOffset = 0.0f;
        if (renderable->skip_aspect_fitting == false) {
            if (aspect_ratio > max_aspect_ratio) {
                xOffset = (winWidth - winHeight * max_aspect_ratio) / 2.0f;
            }
        }

        proj_mat = glm::translate(glm::ortho(0.0f, winWidth, 0.0f, winHeight), glm::vec3(xOffset, 0.0f, 0.0f));

        if (renderable->enableClip) {
            glScissor((GLint)(renderable->clipPosition[0] + xOffset),
                      (GLint)(winHeight - (renderable->clipPosition[1] + renderable->clipSize[1])),
                      (GLsizei)(renderable->clipSize[0]),
                      (GLsizei)(renderable->clipSize[1]));

            glEnable(GL_SCISSOR_TEST);
        }

        switch (renderable->type) {
            case IMIURimage: {
                // Cast this to an image
                IMUIImage* image = (IMUIImage*)renderable;

                // If we don't have a valid texture or we're completely transparent, skip this
                if (!image->isValid() || image->color[3] <= 0.0f) {
                    break;
                }

                // Get our texture dimensions
                float width = image->textureSize[0];
                float height = image->textureSize[1];

                // Image need to be scaled up to compensate for texture reduction
                width = (float)textures->getReducedWidth(image->textureRef);
                height = (float)textures->getReducedHeight(image->textureRef);

                // Image use non-premultiplied alpha
                graphics->SetBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                // Bind the texture
                textures->bindTexture(image->textureRef);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -1.0f);

                //  Setup our texture parameters
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                // Inform the shader of the color
                glUniform4fv(color_id, 1, (GLfloat*)&(image->color));

                // Now we get to build some triangles and attach some texture coordinates
                const vec2 to = vec2(image->textureOffset[0] / width,
                                     (height - (image->textureOffset[1] + image->textureSourceSize[1])) / height);
                const vec2 tss = vec2(image->textureSourceSize[0] / width,
                                      image->textureSourceSize[1] / height);
                const vec2 rs = image->renderSize;

                GLfloat data[] = {
                    to[0], to[1], 0.0, 0.0,
                    to[0] + tss[0], to[1], 1.0, 0.0,
                    to[0] + tss[0], to[1] + tss[1], 1.0, 1.0,
                    to[0], to[1] + tss[1], 0.0, 1.0};

                static const GLuint indices[] = {0, 1, 2, 0, 2, 3};

                // Set up the model matrix
                glm::mat4 model_mat;
                model_mat = glm::translate(model_mat,
                                           glm::vec3(image->pos[0],
                                                     winHeight - (image->pos[1] + rs[1]),
                                                     0.0f));

                model_mat = glm::translate(model_mat, glm::vec3(0.5f * image->renderSize[0],
                                                                0.5f * image->renderSize[1],
                                                                0.0f));

                model_mat = glm::rotate(model_mat, image->rotation, glm::vec3(0.0f, 0.0f, 1.0f));
                model_mat = glm::translate(model_mat, glm::vec3(-0.5f * image->renderSize[0],
                                                                -0.5f * image->renderSize[1],
                                                                0.0f));

                model_mat = glm::scale(model_mat, glm::vec3(image->renderSize[0],
                                                            image->renderSize[1],
                                                            1.0f));

                glm::mat4 mvp_mat = proj_mat * model_mat;

                glUniformMatrix4fv(mvp_mat_id, 1, false, (GLfloat*)&mvp_mat);
                graphics->SetClientActiveTexture(0);
                graphics->EnableVertexAttribArray(vert_coord_id);
                graphics->EnableVertexAttribArray(tex_coord_id);

                if (!image_index_vbo->valid()) {
                    image_index_vbo->Fill(kVBOElement | kVBOStatic, sizeof(indices), (void*)indices);
                }

                image_data_vbo->Fill(sizeof(data), (void*)data);
                image_data_vbo->Bind();
                image_index_vbo->Bind();

                glVertexAttribPointer(vert_coord_id, 2, GL_FLOAT, false, 4 * sizeof(GLfloat), (const void*)(image_data_vbo->offset() + 2 * sizeof(GLfloat)));
                glVertexAttribPointer(tex_coord_id, 2, GL_FLOAT, false, 4 * sizeof(GLfloat), (const void*)(image_data_vbo->offset()));

                graphics->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)image_index_vbo->offset());

                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.0f);

            } break;

            case IMIURtext: {
                // Storage for character mapping in the texture atlas
                unsigned num_characters = 0;
                unsigned indices[kMaxCharacters * 6];
                float verts[kMaxCharacters * 4 * kFloatsPerVert];  // 2v2t

                // Cast this to a text object
                IMUIText* text = (IMUIText*)renderable;

                // Make sure there's anything to do
                if (text->owner == NULL || text->text == "" || text->fontName == "") {
                    return;
                }

                // Make sure we have our text atlas
                TextAtlas* atlas = getTextAtlas(text->fontName, text->size, text->fontFlags);

                if (atlas == NULL) {
                    std::string errorMessage = "Unable to find font " + text->fontName;
                    DisplayError("Error", errorMessage.c_str());
                    return;
                }

                // Now that we're sure everything is ok, we can finally figure out how big our text is
                vec2 dimensions(0, atlas->pixel_height);

                // Get our singleton
                FontRenderer* font_renderer = FontRenderer::Instance();

                int last_c = -1;
                int length = text->text.length();

                int line_height = font_renderer->GetFontInfo(0, FontRenderer::INFO_HEIGHT) / 64;
                int ascender_height = font_renderer->GetFontInfo(0, FontRenderer::INFO_ASCENDER) / 64;

                float ascender_size = ((float)abs(ascender_height) / (float)abs(line_height)) * text->size;

                // Since we have the size already caclulated we can center text on the origin
                // (for rotational purposes)
                float startX = (float)(-1.0 * text->dimensions[0] / 2.0);
                vec2 currentPos(startX, (text->dimensions[1]) / 2.0f - ascender_size);

                std::vector<uint32_t> utf32_string;

                try {
                    utf8::utf8to32(text->text.begin(), text->text.end(), std::back_inserter(utf32_string));
                } catch (const utf8::not_enough_room& ner) {
                    LOGE << "Got utf8 exception \"" << ner.what() << "\" this might indicate invalid utf-8 data" << std::endl;
                }

                for (unsigned i = 0; i < utf32_string.size() && i < kMaxCharacters; ++i) {
                    if (num_characters < kMaxCharacters) {
                        uint32_t c = utf32_string[i];

                        if (c == '\n') {
                            currentPos[1] += atlas->pixel_height;
                            currentPos[0] = startX;
                        }

                        std::vector<CharacterInfo>::iterator character;
                        for (character = atlas->alphabet.begin();
                             character != atlas->alphabet.end();
                             character++) {
                            if (character->codepoint == c)
                                break;
                        }

                        if (character != atlas->alphabet.end()) {
                            int kerning_x = 0;
                            if (last_c != -1) {
                                FT_Vector vec;
                                font_renderer->GetKerning(0, last_c, c, &vec);
                                kerning_x = vec.x / 64;
                            }
                            currentPos[0] += kerning_x;

                            int vert_index = num_characters * 4 * TextAtlasRenderer::kFloatsPerVert;
                            verts[vert_index++] = (float)currentPos[0] + character->bearing[0];
                            verts[vert_index++] = (float)currentPos[1] + character->bearing[1];
                            verts[vert_index++] = (character->pos[0]) / (float)atlas->atlas_dims[0];
                            verts[vert_index++] = 1.0f - ((character->pos[1]) / (float)atlas->atlas_dims[1]);

                            verts[vert_index++] = (float)currentPos[0] + character->width + character->bearing[0];
                            verts[vert_index++] = (float)currentPos[1] + character->bearing[1];
                            verts[vert_index++] = (character->pos[0] + character->width) / (float)atlas->atlas_dims[0];
                            verts[vert_index++] = 1.0f - ((character->pos[1]) / (float)atlas->atlas_dims[1]);

                            verts[vert_index++] = (float)currentPos[0] + character->width + character->bearing[0];
                            verts[vert_index++] = (float)currentPos[1] - (character->height - character->bearing[1]);
                            verts[vert_index++] = (character->pos[0] + character->width) / (float)atlas->atlas_dims[0];
                            verts[vert_index++] = 1.0f - ((character->pos[1] + character->height) / (float)atlas->atlas_dims[1]);

                            verts[vert_index++] = (float)currentPos[0] + character->bearing[0];
                            verts[vert_index++] = (float)currentPos[1] - (character->height - character->bearing[1]);
                            verts[vert_index++] = (character->pos[0]) / (float)atlas->atlas_dims[0];
                            verts[vert_index++] = 1.0f - ((character->pos[1] + character->height) / (float)atlas->atlas_dims[1]);

                            unsigned indices_index = num_characters * 6;
                            unsigned vert_start = num_characters * 4;
                            indices[indices_index++] = vert_start + 0;
                            indices[indices_index++] = vert_start + 1;
                            indices[indices_index++] = vert_start + 2;
                            indices[indices_index++] = vert_start + 0;
                            indices[indices_index++] = vert_start + 2;
                            indices[indices_index++] = vert_start + 3;

                            ++num_characters;
                            currentPos[0] += character->advance_x;
                            last_c = c;
                        }
                    }
                }

                character_data_vbo->Fill(sizeof(verts) * num_characters / kMaxCharacters, verts);
                character_index_vbo->Fill(sizeof(indices) * num_characters / kMaxCharacters, indices);

                character_data_vbo->Bind();
                character_index_vbo->Bind();

                graphics->EnableVertexAttribArray(vert_coord_id);
                graphics->EnableVertexAttribArray(tex_coord_id);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, atlas->tex);
                glVertexAttribPointer(vert_coord_id, 2, GL_FLOAT, false, 4 * sizeof(GLfloat), (const void*)(character_data_vbo->offset()));
                glVertexAttribPointer(tex_coord_id, 2, GL_FLOAT, false, 4 * sizeof(GLfloat), (const void*)(character_data_vbo->offset() + sizeof(GL_FLOAT) * 2));

                int num_indices = sizeof(indices) * num_characters / kMaxCharacters / sizeof(float);

                // Set up the model matrix
                glm::mat4 model_mat;
                model_mat = glm::translate(model_mat,
                                           glm::vec3(text->pos[0] + 0.5f * text->boundingBox[0],
                                                     winHeight - (text->pos[1] + (0.5 * text->boundingBox[1])),
                                                     0.0f));

                model_mat = glm::rotate(model_mat, text->rotation, glm::vec3(0.0f, 0.0f, 1.0f));

                glm::mat4 mvp_mat = proj_mat * model_mat;

                if (text->renderFlags & TextAtlasRenderer::kTextShadow) {
                    glm::mat4 offset_model_mat;
                    offset_model_mat = glm::translate(offset_model_mat,
                                                      glm::vec3(text->pos[0] + 0.5f * text->boundingBox[0] + 2.0,
                                                                winHeight - (text->pos[1] + (0.5 * text->boundingBox[1]) + 2.0),
                                                                0.0f));
                    glm::mat4 mat = proj_mat * offset_model_mat;
                    glUniformMatrix4fv(mvp_mat_id, 1, false, (GLfloat*)&mat);
                    glUniform4f(color_id, 0.0f, 0.0f, 0.0f, text->color.a());
                    graphics->DrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, (const void*)character_index_vbo->offset());
                }

                glUniformMatrix4fv(mvp_mat_id, 1, false, (GLfloat*)&mvp_mat);
                glUniform4fv(color_id, 1, &(text->color[0]));
                graphics->DrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, (const void*)character_index_vbo->offset());

                graphics->BindElementVBO(0);
                graphics->BindArrayVBO(0);
                graphics->ResetVertexAttribArrays();
            } break;

            case IMUIRinvalid:
            default:
                LOGE << "Unknown renderable type: " << renderable->type << std::endl;
        }

        if (renderable->enableClip) {
            glDisable(GL_SCISSOR_TEST);
        }

        // Clean up the copy
        delete renderable;
    }
    graphics->ResetVertexAttribArrays();

    renderQueue.clear();
}

TextAtlas* IMUIContext::getTextAtlas(std::string fontName, int size, int flags) {
    TextAtlas* atlas = NULL;

    if (fontName.length() >= CachedTextAtlas::kPathSize) {
        DisplayError("Error", "Font path is too long");
    } else {
        int found = -1;
        // See if we've chached this font atlas
        for (int i = 0; i < atlases.num_atlases; ++i) {
            const CachedTextAtlas& cached = atlases.cached[i];
            if (strcmp(cached.path, fontName.c_str()) == 0 &&
                size == cached.pixel_height &&
                flags == cached.flags) {
                found = i;
            }
        }
        if (found == -1) {
            // If we don't have this font cached, try to load it
            if (atlases.num_atlases >= CachedTextAtlases::kMaxAtlases) {
                DisplayError("Error", "Too many cached text atlases");
            } else {
                found = atlases.num_atlases++;
                CachedTextAtlas* new_atlas = &atlases.cached[found];
                strncpy(new_atlas->path, fontName.c_str(), CachedTextAtlas::kPathSize);
                new_atlas->pixel_height = size;
                new_atlas->flags = flags;
                new_atlas->atlas.Create(fontName.c_str(), size,
                                        FontRenderer::Instance(), flags);
            }
        }

        // See if, one way or another, we have a texture atlas
        if (found != -1) {
            atlas = &atlases.cached[found].atlas;
        }
    }

    return atlas;
}
