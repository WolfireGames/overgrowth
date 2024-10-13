//-----------------------------------------------------------------------------
//           Name: destination_trail.as
//      Developer: Wolfire Games LLC
//    Script Type: Hotspot
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

array<int> points;
string placeholder = "Data/Objects/placeholder/empty_placeholder.xml";
int point_index = 0;
int trail_index = 0;
bool post_init_done = false;
vec3 trail_pos = vec3(0);
float trail_distance = 0.0f;
const vec3 origin_offset = vec3(0, -0.5f, 0);

void SetParameters() {
    params.AddInt("NumPoints", 4);
}

void Init() {
    // No additional initialization needed
}

void HandleEventItem(string event, ItemObject@ obj) {
    // No item event handling needed
}

void Reset() {
    point_index = 0;
    trail_index = point_index;
}

void Update() {
    if (!post_init_done) {
        InitializePoints();
        post_init_done = true;
    }
    if (points.size() < 2) {
        AddPoint();
    }
    if (EditorModeActive()) {
        CheckForDeletedPoints();
        CheckForNewPoints();
        DrawConnectionLines();
        DrawBoxes();
    } else {
        CheckForPreviousPoint();
        CheckForNextPoint();
        UpdateTrail();
    }
}

void InitializePoints() {
    Object@ hotspot_obj = ReadObjectFromID(hotspot.GetID());
    hotspot_obj.SetScale(vec3(0.125f));
    hotspot_obj.SetCopyable(false);
    hotspot_obj.SetDeletable(true);
    hotspot_obj.SetSelectable(true);
    hotspot_obj.SetTranslatable(true);
    hotspot_obj.SetScalable(false);
    hotspot_obj.SetRotatable(false);
    points.insertLast(hotspot.GetID());
    CheckForNewPoints();
}

void UpdateTrail() {
    MovementObject@ player = ReadCharacter(0);
    Object@ closest_point_obj = ReadObjectFromID(points[trail_index]);

    if (trail_pos == vec3(0) || trail_distance > 10.0f) {
        trail_pos = player.position + origin_offset;
        trail_index = point_index;
        trail_distance = 0.0f;
    }
    vec3 target_pos = closest_point_obj.GetTranslation();
    vec3 direction = normalize(target_pos - trail_pos);
    MakeParticle("Data/Particles/destination_particle.xml", trail_pos, direction);
    trail_pos += (direction * 0.05f);

    float radius = 0.1f;
    if (distance(trail_pos, target_pos) < radius) {
        if (int(points.size()) > (trail_index + 1)) {
            trail_index++;
        } else {
            trail_pos = player.position + origin_offset;
            trail_index = point_index;
            trail_distance = 0.0f;
        }
    }
}

void CheckForNextPoint() {
    if (int(points.size()) <= point_index + 1) {
        return;
    }
    MovementObject@ player = ReadCharacter(0);
    Object@ next_point = ReadObjectFromID(points[point_index + 1]);
    if (distance(player.position, next_point.GetTranslation()) < DistanceBetweenPoints(point_index, point_index + 1)) {
        point_index++;
    }
}

void CheckForPreviousPoint() {
    if (point_index - 1 < 0) {
        return;
    }
    MovementObject@ player = ReadCharacter(0);
    Object@ previous_point = ReadObjectFromID(points[point_index - 1]);
    if (distance(player.position, previous_point.GetTranslation()) < DistanceBetweenPoints(point_index, point_index - 1)) {
        point_index--;
    }
}

float DistanceBetweenPoints(int index_a, int index_b) {
    Object@ point_a = ReadObjectFromID(points[index_a]);
    Object@ point_b = ReadObjectFromID(points[index_b]);
    return distance(point_a.GetTranslation(), point_b.GetTranslation());
}

void CheckForNewPoints() {
    array<int> placeholder_ids = GetObjectIDsType(35);
    for (uint o = 0; o < placeholder_ids.size(); o++) {
        if (points.find(placeholder_ids[o]) == -1) {
            Object@ placeholder = ReadObjectFromID(placeholder_ids[o]);
            ScriptParams@ placeholder_params = placeholder.GetScriptParams();
            if (placeholder_params.HasParam("belongsto") && placeholder_params.GetString("belongsto") == ("" + hotspot.GetID())) {
                points.insertLast(placeholder_ids[o]);
            }
        }
    }
}

void CheckForDeletedPoints() {
    for (uint i = 0; i < points.size(); i++) {
        if (!ObjectExists(points[i])) {
            points.removeAt(i);
            i--;
        }
    }
}

void DrawConnectionLines() {
    Object@ hotspot_obj = ReadObjectFromID(hotspot.GetID());
    vec3 last_pos = hotspot_obj.GetTranslation();
    for (uint i = 0; i < points.size(); i++) {
        Object@ this_point = ReadObjectFromID(points[i]);
        DebugDrawLine(last_pos, this_point.GetTranslation(), vec3(1), _delete_on_update);
        last_pos = this_point.GetTranslation();
    }
}

void DrawBoxes() {
    Object@ hotspot_obj = ReadObjectFromID(hotspot.GetID());
    Object@ last_point = ReadObjectFromID(points[points.size() - 1]);
    DebugDrawWireBox(hotspot_obj.GetTranslation(), vec3(0.5f), vec3(1, 0, 0), _delete_on_update);
    DebugDrawWireBox(last_point.GetTranslation(), vec3(0.5f), vec3(0, 1, 0), _delete_on_update);
}

void AddPoint() {
    int object_id = CreateObject(placeholder);
    points.insertLast(object_id);
    Object@ obj = ReadObjectFromID(object_id);
    obj.SetScale(vec3(0.5f));
    obj.SetCopyable(true);
    obj.SetDeletable(true);
    obj.SetSelectable(true);
    obj.SetTranslatable(true);
    obj.SetScalable(false);
    obj.SetRotatable(false);
    obj.SetTranslation(ReadObjectFromID(hotspot.GetID()).GetTranslation() + vec3(0, 1, 0));
    ScriptParams@ placeholder_params = obj.GetScriptParams();
    placeholder_params.AddString("belongsto", "" + hotspot.GetID());
}
