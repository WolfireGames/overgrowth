//-----------------------------------------------------------------------------
//           Name: sky_editor.h
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
#pragma once

#include <Editors/editor_types.h>
#include <Editors/save_state.h>

#include <Graphics/glstate.h>
#include <Internal/levelxml.h>
#include <Math/mat4.h>

struct LineSegment;
class SceneGraph;
class TiXmlDocument;
class TiXmlNode;
struct Flare;
class GameCursor;

/// The SkyEditor class allows the player to drag, tint, and resize the sun.
class SkyEditor {
   public:
    /**
     * Initialize a new SkyEditor.
     * @param s A pointer to the active scenegraph.
     */
    SkyEditor(SceneGraph* s);

    /**
     * Draw the controls for the sky editor.
     * Namely, the sun radius circle and the color picker circle.
     */
    void Draw();

    /**
     *  Set the cursor to reflect the value of m_tool.
     */
    void UpdateCursor(GameCursor* cursor);

    /**
     * Load the sky data from the level xml file.
     * @param filename The path to the level xml file.
     */
    void ApplySkyInfo(const SkyInfo& si);

    /**
     * Save the sky data to a level xml file.
     * @param doc A reference to the level xml file.
     */
    void SaveSky(TiXmlNode* root);

    void SaveHistoryState(std::list<SavedChunk>& chunk_list, int state_id);
    bool ReadChunk(SavedChunk& saved_chunk);

    // Control and tool handlers
    EditorTypes::Tool OmniGetTool(float angle_from_sun, const LineSegment& mouseray);
    bool HandleSelect(float angle_from_sun);  // returns true is something happened
    void HandleSunTranslate(float angle_from_sun);
    void HandleSunScale(float angle_from_sun);
    void HandleSunRotate(float angle_from_sun, const vec3& mouseray, GameCursor* cursor);
    void HandleTransformationStarted();
    void HandleTransformationStopped();

    // Transformations
    void PlaceSun(const vec3& dir);
    void TranslateSun(const vec3& trans);
    void ScaleSun(float factor);
    void RotateSun(float angle);

    bool m_sun_selected;  /// Whether the sun is selected or not.

    EditorTypes::Tool m_tool;  /// Which tool is active.

    Flare* flare;
    vec3 sun_dir_;  /// ray >>to<< sun.

    vec4 translation_offset_;  /// Used to drag the sun from points other than the center.
    float scale_angle_zero_;   /// The original value of m_sun_angular_rad when scaling begins.
    vec3 rotation_zero_;       /// The original rotation value when rotation begins.

    float m_sun_angular_rad;  /// Angle between the center of the sun and a point on the ring.
    float m_sun_brightness;   /// The 'brightness' (size) of the sun.
    float m_sun_color_angle;  /// Angle of the sun on the color wheel (in degrees).

    bool m_sun_translating;  /// Currently dragging sun to translate.
    bool m_sun_scaling;      /// Currently dragging scale in and out.
    bool m_sun_rotating;     /// Currently dragging on color wheel.

    GLState m_gl_state;  /// GLState for drawing the editor lines

    mat4 m_viewing_transform;  /// Transformation from world space to editor space.

    SceneGraph* scenegraph_;  /// Pointer to relevant scenegraph.

    bool m_lighting_changed;  /// Whether the sun has changed or not.

    /**
     * Takes a mouseray and returns whether it intersects the color wheel orb.
     * @param mouseray The mouse ray vector.
     * @return True if the mouseray intersects the orb.
     */
    bool MouseOverColorOrb(const LineSegment& mouseray);

    /**
     * Calculates m_sun_brightness from m_sun_angular_rad.
     */
    void CalcBrightnessFromAngularRad();

    /**
     * Calculates the sun color from an angle on the color wheel.
     * @param angle The angle on the color wheel.
     * @return A vec3 representing the color.
     */
    vec3 CalcColorFromAngle(float angle);  // angle in degrees
};
