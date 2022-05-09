//-----------------------------------------------------------------------------
//           Name: sky_editor.cpp
//      Developer: Wolfire Games LLC
//         Author: Phillip Isola
//    Description: Editor for the sky
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
#include "sky_editor.h"

#include <Editors/map_editor.h>
#include <Editors/editor_utilities.h>

#include <Graphics/sky.h>
#include <Graphics/flares.h>
#include <Graphics/camera.h>
#include <Graphics/camera.h>
#include <Graphics/graphics.h>
#include <Graphics/shaders.h>
#include <Graphics/Cursor.h>

#include <Math/vec3math.h>
#include <Math/vec4math.h>

#include <Internal/datemodified.h>
#include <Internal/memwrite.h>

#include <UserInput/input.h>
#include <Objects/cameraobject.h>
#include <Main/scenegraph.h>
#include <XML/xml_helper.h>

#include <tinyxml.h>

#include <cmath>

static const float _initial_sun_angular_rad = PI_f / 30.0f;  // must not be zero
static const float _sun_brightness_constant = 1.0f / ((float)pow(tanf(_initial_sun_angular_rad), 2.0f));
static const float _min_sun_angular_rad = PI_f / 100.0f;
static const float _max_sun_angular_rad = PI_f / 10.0f;
static const float _color_orb_scale = tanf(_initial_sun_angular_rad) * 0.1f;

static const float _min_sun_brightness = tanf(_min_sun_angular_rad) * tanf(_min_sun_angular_rad) * _sun_brightness_constant;
static const float _max_sun_brightness = tanf(_max_sun_angular_rad) * tanf(_max_sun_angular_rad) * _sun_brightness_constant;

SkyEditor::SkyEditor(SceneGraph* s) : sun_dir_(1.0f, 0.0f, 0.0f),
                                      translation_offset_(0.0f, 0.0f, 0.0f, 0.0f),
                                      rotation_zero_(1.0f, 0.0f, 0.0f),
                                      scenegraph_(s) {
    m_sun_selected = false;

    m_tool = EditorTypes::NO_TOOL;

    m_sun_angular_rad = _initial_sun_angular_rad;
    CalcBrightnessFromAngularRad();

    m_sun_color_angle = 0.0f;

    m_sun_translating = false;
    m_sun_scaling = false;
    m_sun_rotating = false;

    m_lighting_changed = false;

    m_gl_state.depth_test = false;
    m_gl_state.cull_face = false;
    m_gl_state.blend = true;
    m_gl_state.depth_write = false;
}

void SkyEditor::Draw() {
    if (m_sun_selected) {
        Camera* cam = ActiveCameras::Get();
        Graphics* graphics = Graphics::Instance();
        Shaders* shaders = Shaders::Instance();
        // align with sun ray
        vec3 origin_dir(0, 0, 1);
        float angle = GetAngleBetween(sun_dir_, origin_dir);
        vec3 around = cross(origin_dir, sun_dir_);
        quaternion quat_a(vec4(normalize(around), angle * PI_f / 180.0f));

        // align with rotation around z so up y is maintained as (0,1,0)
        vec3 curr_up(cam->GetUpVector());
        curr_up = normalize(invert(quat_a) * curr_up);
        vec3 sun_up_vector(0.0f, 1.0f, 0.0f);
        curr_up[2] = sun_up_vector[2];  // forces rotation around z only (any rotation not around z would violate this equality
        angle = GetAngleBetween(sun_up_vector, curr_up);
        around = cross(curr_up, sun_up_vector);
        quaternion quat_b(vec4(normalize(around), -angle * PI_f / 180.0f));
        m_viewing_transform = Mat4FromQuaternion(quat_a * quat_b);

        vec3 camera_pos(cam->GetPos());
        m_viewing_transform.AddTranslation(camera_pos);

        graphics->setGLState(m_gl_state);
        graphics->setDepthFunc(GL_LEQUAL);

        mat4 mvp = cam->GetProjMatrix() * cam->GetViewMatrix() * m_viewing_transform;

        vec3 rgb;
        float scale = tanf(m_sun_angular_rad);

        mat4 circle_scale;
        circle_scale.SetScale(vec3(scale, scale, 1.0f));
        mat4 circle_mvp = mvp * circle_scale;

        // Draw circle
        int shader_id = shaders->returnProgram("3d_color #COLOR_ATTRIB #NO_VELOCITY_BUF");
        shaders->setProgram(shader_id);
        shaders->SetUniformMat4("mvp", circle_mvp);
        GLfloat data[37 * 7];
        int index = 0;
        for (int i = 0; i <= 36; i++) {
            float angle = (float)i * (float)PI / 18.0f;
            if (m_sun_rotating) {
                rgb = CalcColorFromAngle((float)rad2deg * angle);
                data[index + 0] = rgb[0];
                data[index + 1] = rgb[1];
                data[index + 2] = rgb[2];
                data[index + 3] = 1.0f;
            } else {
                data[index + 0] = 0.4f;
                data[index + 1] = 0.4f;
                data[index + 2] = 0.4f;
                data[index + 3] = 0.3f;
            }
            data[index + 4] = cosf(angle);
            data[index + 5] = sinf(angle);
            data[index + 6] = 1.0f;
            index += 7;
        }
        static VBOContainer data_vbo;
        data_vbo.Fill(kVBODynamic | kVBOFloat, sizeof(data), data);
        data_vbo.Bind();
        int vert_attrib_id = shaders->returnShaderAttrib("vert_attrib", shader_id);
        int color_attrib_id = shaders->returnShaderAttrib("color_attrib", shader_id);
        graphics->EnableVertexAttribArray(vert_attrib_id);
        graphics->EnableVertexAttribArray(color_attrib_id);
        glVertexAttribPointer(vert_attrib_id, 3, GL_FLOAT, false, 7 * sizeof(GLfloat), (const void*)(sizeof(GLfloat) * 4));
        glVertexAttribPointer(color_attrib_id, 4, GL_FLOAT, false, 7 * sizeof(GLfloat), 0);
        glDrawArrays(GL_LINE_STRIP, 0, 37);
        graphics->ResetVertexAttribArrays();

        // Draw handle
        {
            shader_id = shaders->returnProgram("3d_color #COLOR_UNIFORM #NO_VELOCITY_BUF");
            shaders->setProgram(shader_id);
            mat4 translate_mat;
            translate_mat.SetTranslation(vec3(cosf((float)deg2rad * m_sun_color_angle) * scale, sinf((float)deg2rad * m_sun_color_angle) * scale, 0.0f));
            mat4 handle_mvp = mvp * translate_mat;
            shaders->SetUniformMat4("mvp", handle_mvp);
            rgb = CalcColorFromAngle(m_sun_color_angle);
            shaders->SetUniformVec4("color_uniform", vec4(rgb, 1.0f));
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            float data[30];
            data[0] = 0.0f;
            data[1] = 0.0f;
            data[2] = 1.0f;
            int index = 3;
            for (float angle = (float)PI * 2; angle >= 0; angle -= (float)PI / 4.0f) {
                data[index + 0] = cosf(angle) * _color_orb_scale;
                data[index + 1] = sinf(angle) * _color_orb_scale;
                data[index + 2] = 1.0f;
                index += 3;
            }
            data_vbo.Fill(kVBODynamic | kVBOFloat, sizeof(data), data);
            data_vbo.Bind();
            int vert_attrib_id = shaders->returnShaderAttrib("vert_attrib", shader_id);
            graphics->EnableVertexAttribArray(vert_attrib_id);
            glVertexAttribPointer(vert_attrib_id, 3, GL_FLOAT, false, 3 * sizeof(GLfloat), 0);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 10);
            shaders->SetUniformVec4("color_uniform", vec4(vec3(0.4f), 0.3f));
            glDrawArrays(GL_LINE_STRIP, 1, 9);
            graphics->ResetVertexAttribArrays();
        }
    }
}

void SkyEditor::UpdateCursor(GameCursor* cursor) {
    switch (m_tool) {
        case EditorTypes::TRANSLATE:
            cursor->SetCursor(TRANSLATE_CURSOR);
            break;
        case EditorTypes::SCALE:
            cursor->SetCursor(SCALE_CURSOR);
            break;
        case EditorTypes::ROTATE:
            cursor->SetCursor(ROTATE_CURSOR);
            break;
        default:
            cursor->SetCursor(DEFAULT_CURSOR);
            break;
    }
}

bool SkyEditor::MouseOverColorOrb(const LineSegment& mouseray) {
    float scale = tanf(m_sun_angular_rad);
    vec3 mouseray_dir = normalize(mouseray.end - mouseray.start);

    vec3 p = m_viewing_transform * vec3(cosf((float)deg2rad * m_sun_color_angle) * scale, sinf((float)deg2rad * m_sun_color_angle) * scale, 1.0f);
    vec3 n = -sun_dir_;
    float r = _color_orb_scale;

    float to_dist = RayPlaneIntersection(mouseray.start, mouseray_dir, p, n);
    if (to_dist < 0) return false;
    vec3 hit_point = mouseray.start + to_dist * mouseray_dir;

    if (length(hit_point - p) < r)
        return true;
    else
        return false;
}

EditorTypes::Tool SkyEditor::OmniGetTool(float angle_from_sun,
                                         const LineSegment& mouseray) {
    if (!m_sun_selected) {
        return EditorTypes::NO_TOOL;
    } else if (MouseOverColorOrb(mouseray)) {
        return EditorTypes::ROTATE;
    } else if (angle_from_sun <= m_sun_angular_rad * 0.8f) {
        return EditorTypes::TRANSLATE;
    } else if (angle_from_sun <= m_sun_angular_rad * 1.4f) {
        return EditorTypes::SCALE;
    } else {
        return EditorTypes::NO_TOOL;
    }
}

bool SkyEditor::HandleSelect(float angle_from_sun) {
    Mouse* mouse = &(Input::Instance()->getMouse());
    if (mouse->mouse_double_click_[Mouse::LEFT]) {
        if (angle_from_sun < PI / 50) {
            m_sun_selected = true;
            return true;
        } else {
            m_sun_selected = false;
            return true;
        }
    }
    return false;
}

extern bool shadow_cache_dirty;
extern bool shadow_cache_dirty_sun_moved;

void SkyEditor::HandleSunTranslate(float angle_from_sun) {
    Mouse* mouse = &(Input::Instance()->getMouse());

    if (!m_sun_translating                                                         // && angle_from_sun <= m_sun_angular_rad
        && mouse->mouse_down_[Mouse::LEFT] == Mouse::CLICKED && m_sun_selected) {  // start
        HandleTransformationStarted();
        m_sun_translating = true;

        vec4 sun_center = m_viewing_transform * vec4(0.0f, 0.0f, 1.0f, 1.0f);
        vec3 ray_through_sun_center = normalize(sun_center.xyz() - ActiveCameras::Get()->GetPos());

        translation_offset_ = vec4(ray_through_sun_center - ActiveCameras::Get()->GetMouseRay(), 0.0f);
        translation_offset_ = invert(m_viewing_transform) * translation_offset_;
    } else if (m_sun_translating) {
        if (mouse->mouse_down_[Mouse::LEFT]) {  // continue
            PlaceSun(ActiveCameras::Get()->GetMouseRay() + (m_viewing_transform * translation_offset_).xyz());
        } else {  // stop
            PlaceSun(ActiveCameras::Get()->GetMouseRay() + (m_viewing_transform * translation_offset_).xyz());
            m_sun_translating = false;

            HandleTransformationStopped();
        }
    }

    shadow_cache_dirty = true;
    shadow_cache_dirty_sun_moved = true;
}

void SkyEditor::HandleSunScale(float angle_from_sun) {
    Mouse* mouse = &(Input::Instance()->getMouse());

    if (!m_sun_scaling                                                             // && angle_from_sun > m_sun_angular_rad && angle_from_sun < m_sun_angular_rad*1.2f
        && mouse->mouse_down_[Mouse::LEFT] == Mouse::CLICKED && m_sun_selected) {  // start
        HandleTransformationStarted();
        m_sun_scaling = true;
        scale_angle_zero_ = angle_from_sun;

        if (ActiveCameras::Get()->m_camera_object != NULL) {
            ActiveCameras::Get()->m_camera_object->IgnoreMouseInput(true);
        }
    } else if (m_sun_scaling) {
        if (mouse->mouse_down_[Mouse::LEFT]) {  // continue
            if (scale_angle_zero_ != 0) {
                float factor = angle_from_sun / scale_angle_zero_;
                ScaleSun(factor);
                scale_angle_zero_ = angle_from_sun;
            }
        } else {  // stop
            if (scale_angle_zero_ != 0) {
                float factor = angle_from_sun / scale_angle_zero_;
                ScaleSun(factor);
                scale_angle_zero_ = angle_from_sun;
            }
            m_sun_scaling = false;

            if (ActiveCameras::Get()->m_camera_object != NULL) {
                ActiveCameras::Get()->m_camera_object->IgnoreMouseInput(false);
            }

            HandleTransformationStopped();
        }
    }
}

static float GetSignedAngleBetween(const vec3& n, const vec3& v1, const vec3& v2) {
    const vec3 a = normalize(v1);
    const vec3 b = normalize(v2);
    float d = dot(a, b);
    d = clamp(d, -1.0f, 1.0f);
    if (d != 0.0f) {
        vec4 c = cross(a, b);
        return rad2degf * atan2f(dot(normalize(n), c), d);
    } else {
        return 0.0f;
    }
}

void SkyEditor::HandleSunRotate(float angle_from_sun, const vec3& mouseray, GameCursor* cursor) {
    Mouse* mouse = &(Input::Instance()->getMouse());
    if (!m_sun_rotating                                                            // && angle_from_sun > m_sun_angular_rad && angle_from_sun < m_sun_angular_rad*1.2f
        && mouse->mouse_down_[Mouse::LEFT] == Mouse::CLICKED && m_sun_selected) {  // start
        HandleTransformationStarted();
        m_sun_rotating = true;

        vec3 dir = normalize(mouseray - sun_dir_);
        cursor->SetCursor(ROTATE_CIRCLE_CURSOR);
        cursor->SetRotation(90 - m_sun_color_angle);
        rotation_zero_ = dir;

        if (ActiveCameras::Get()->m_camera_object != NULL) {
            ActiveCameras::Get()->m_camera_object->IgnoreMouseInput(true);
        }
    } else if (m_sun_rotating) {
        if (mouse->mouse_down_[Mouse::LEFT]) {  // continue
            vec3 dir = normalize(mouseray - sun_dir_);
            float angle = -GetSignedAngleBetween(sun_dir_, dir, rotation_zero_);
            RotateSun(angle);
            cursor->SetRotation(90 - m_sun_color_angle);
            rotation_zero_ = dir;
        } else {  // stop
            vec3 dir = normalize(mouseray - sun_dir_);
            float angle = -GetSignedAngleBetween(sun_dir_, dir, rotation_zero_);
            RotateSun(angle);
            cursor->SetRotation(0);
            cursor->SetCursor(ROTATE_CIRCLE_CURSOR);
            rotation_zero_ = dir;

            m_sun_rotating = false;

            if (ActiveCameras::Get()->m_camera_object != NULL) {
                ActiveCameras::Get()->m_camera_object->IgnoreMouseInput(false);
            }

            HandleTransformationStopped();
        }
    }
}

void SkyEditor::HandleTransformationStarted() {
    scenegraph_->map_editor->state_ = MapEditor::kSkyDrag;
}

void SkyEditor::HandleTransformationStopped() {
    scenegraph_->map_editor->state_ = MapEditor::kIdle;
    scenegraph_->sky->LightingChanged(scenegraph_->terrain_object_ != NULL);
    scenegraph_->map_editor->QueueSaveHistoryState();
}

void SkyEditor::SaveSky(TiXmlNode* root) {
    TiXmlElement* sky = new TiXmlElement("Sky");
    root->LinkEndChild(sky);
    TiXmlElement* sky_el;
    sky_el = new TiXmlElement("DomeTexture");
    sky_el->LinkEndChild(new TiXmlText(scenegraph_->sky->dome_texture_name.c_str()));
    sky->LinkEndChild(sky_el);

    std::stringstream num1;
    std::string num_string;

    num1 << m_sun_angular_rad;
    sky_el = new TiXmlElement("SunAngularRad");
    num_string = num1.str();
    sky_el->LinkEndChild(new TiXmlText(num_string.c_str()));
    sky->LinkEndChild(sky_el);

    std::stringstream num2;
    num2 << ConvertToFirstCycle(m_sun_color_angle, 360.0f);
    sky_el = new TiXmlElement("SunColorAngle");
    num_string = num2.str();
    sky_el->LinkEndChild(new TiXmlText(num_string.c_str()));
    sky->LinkEndChild(sky_el);

    sky_el = new TiXmlElement("RayToSun");
    sky_el->SetDoubleAttribute("r0", sun_dir_[0]);
    sky_el->SetDoubleAttribute("r1", sun_dir_[1]);
    sky_el->SetDoubleAttribute("r2", sun_dir_[2]);
    sky->LinkEndChild(sky_el);
}

// Transforms

void SkyEditor::PlaceSun(const vec3& dir) {
    sun_dir_ = normalize(dir);
    scenegraph_->primary_light.pos = sun_dir_;
    flare->position = sun_dir_;
    m_lighting_changed = true;
}

void SkyEditor::TranslateSun(const vec3& trans) {
    sun_dir_ = normalize(sun_dir_ + trans);
    scenegraph_->primary_light.pos = sun_dir_;
    flare->position = sun_dir_;
    m_lighting_changed = true;
}

void SkyEditor::ScaleSun(float factor) {
    float new_angular_rad = m_sun_angular_rad * factor;
    new_angular_rad = clamp(new_angular_rad,
                            _min_sun_angular_rad,
                            _max_sun_angular_rad);
    m_sun_angular_rad = new_angular_rad;
    // scale brightness to be proportional to sun's surface area
    CalcBrightnessFromAngularRad();
    flare->diffuse = m_sun_brightness;
    scenegraph_->primary_light.intensity = 1.0f;
    if (m_sun_brightness > _no_glare_threshold) {
        float range = _max_sun_brightness - _no_glare_threshold;
        float intensity = max(0.0f, 1.0f - (m_sun_brightness - _no_glare_threshold) / range);
        scenegraph_->primary_light.intensity = intensity;
    }
    if (m_sun_brightness < _sharp_glare_threshold) {
        float range = _sharp_glare_threshold - _min_sun_brightness;
        float intensity = max(0.0f, 1.0f + (_sharp_glare_threshold - m_sun_brightness) * 2.0f / range);
        scenegraph_->primary_light.intensity = intensity;
    }
    // printf("brightness = %g\n", m_sun_brightness);
}

void SkyEditor::RotateSun(float angle) {
    // RotateHueHSV(m_sun_color_angle, angle);
    // vec4 rgb = HSVtoRGB(m_sun_color_angle,0.25f,1.0f);
    m_sun_color_angle += angle;
    vec3 rgb = CalcColorFromAngle(m_sun_color_angle);
    scenegraph_->primary_light.color = rgb;
    flare->color = rgb;
}

void SkyEditor::CalcBrightnessFromAngularRad() {
    float size = tan(m_sun_angular_rad);
    m_sun_brightness = size * size * _sun_brightness_constant;
}

// white -> yellow -> orange -> red -> violet -> blue-white
static const vec3 WHITE(1.0f, 1.0f, 1.0f);
static const vec3 YELLOW(252.0f / 255.0f, 1.0f, 178.0f / 255.0f);
static const vec3 ORANGE(1.0f, 223.0f / 255.0f, 178.0f / 255.0f);
static const vec3 RED(1.0f, 178.0f / 255.0f, 178.0f / 255.0f);
static const vec3 VIOLET(1.0f, 178.0f / 255.0f, 232.0f / 255.0f);
static const vec3 BLUE_WHITE(225.0f / 255.0f, 248.0f / 255.0f, 1.0f);

vec3 SkyEditor::CalcColorFromAngle(float angle) {
    angle = ConvertToFirstCycle(angle, 360.0f);
    if (angle < 60) {
        return Interpolate(WHITE, YELLOW, angle / 60.0f);
    } else if (angle < 120) {
        return Interpolate(YELLOW, ORANGE, (angle - 60.0f) / 60.0f);
    } else if (angle < 180) {
        return Interpolate(ORANGE, RED, (angle - 120.0f) / 60.0f);
    } else if (angle < 240) {
        return Interpolate(RED, VIOLET, (angle - 180.0f) / 60.0f);
    } else if (angle < 300) {
        return Interpolate(VIOLET, BLUE_WHITE, (angle - 240.0f) / 60.0f);
    } else {
        return Interpolate(BLUE_WHITE, WHITE, (angle - 300.0f) / 60.0f);
    }
}

void SkyEditor::SaveHistoryState(std::list<SavedChunk>& chunk_list, int state_id) {
    SavedChunk saved_chunk;
    saved_chunk.obj_id = 0;
    saved_chunk.type = ChunkType::SKY_EDITOR;

    saved_chunk.desc.AddFloat(EDF_SUN_RADIUS, m_sun_angular_rad);
    saved_chunk.desc.AddFloat(EDF_SUN_COLOR_ANGLE, m_sun_color_angle);
    saved_chunk.desc.AddVec3(EDF_SUN_DIRECTION, sun_dir_);

    AddChunkToHistory(chunk_list, state_id, saved_chunk);
}

bool SkyEditor::ReadChunk(SavedChunk& the_chunk) {
    bool something_changed = false;
    EntityDescription& desc = the_chunk.desc;
    for (auto& field : desc.fields) {
        switch (field.type) {
            case EDF_SUN_RADIUS: {
                float old_m_sun_angular_rad = m_sun_angular_rad;
                memread(&m_sun_angular_rad, sizeof(float), 1, field.data);
                if (m_sun_angular_rad != old_m_sun_angular_rad) {
                    something_changed = true;
                }
                break;
            }
            case EDF_SUN_COLOR_ANGLE: {
                float old_m_sun_color_angle = m_sun_color_angle;
                memread(&m_sun_color_angle, sizeof(float), 1, field.data);
                if (m_sun_color_angle != old_m_sun_color_angle) {
                    something_changed = true;
                }
                break;
            }
            case EDF_SUN_DIRECTION: {
                vec3 old_m_sun_ray = sun_dir_;
                memread(&sun_dir_.entries, sizeof(float), 3, field.data);
                if (sun_dir_ != old_m_sun_ray) {
                    something_changed = true;
                }
                break;
            }
        }
    }
    if (something_changed) {
        PlaceSun(sun_dir_);
        ScaleSun(1.0f);
        RotateSun(0.0f);
    }
    return something_changed;
}

void SkyEditor::ApplySkyInfo(const SkyInfo& si) {
    scenegraph_->sky->dome_texture_name = si.dome_texture_path;
    PlaceSun(si.ray_to_sun);
    m_sun_angular_rad = si.sun_angular_rad;
    ScaleSun(1.0f);
    RotateSun(si.sun_color_angle);
    scenegraph_->sky->level_name = scenegraph_->level_name_;
    m_lighting_changed = false;
}
