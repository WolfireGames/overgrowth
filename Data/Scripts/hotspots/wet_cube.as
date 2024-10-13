//-----------------------------------------------------------------------------
//           Name: wet_cube.as
//      Developer: Wolfire Games LLC
//    Script Type: Hotspot
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

int water_surface_id = -1;
int water_decal_id = -1;

void SetParameters() {
    params.AddFloatSlider("Wave Density", 0.25f, "min:0,max:1,step:0.01");
    params.AddFloatSlider("Wave Height", 0.5f, "min:0,max:1,step:0.01");
    params.AddFloatSlider("Water Fog", 1.0f, "min:0,max:1,step:0.01");
}

void Dispose() {
    DeleteObjectById(water_decal_id);
    DeleteObjectById(water_surface_id);
}

void Update() {
    UpdateWaterSurface();
    UpdateWaterDecal();
    HandleCollisions();
}

void UpdateWaterSurface() {
    if (params.HasParam("Invisible")) {
        return;
    }
    if (water_surface_id == -1) {
        water_surface_id = CreateObject("Data/Objects/water_test.xml", true);
    }
    Object@ water_surface_obj = ReadObjectFromID(water_surface_id);
    Object@ hotspot_obj = ReadObjectFromID(hotspot.GetID());
    water_surface_obj.SetTranslation(hotspot_obj.GetTranslation());
    water_surface_obj.SetRotation(hotspot_obj.GetRotation());
    water_surface_obj.SetScale(hotspot_obj.GetScale() * 2.0f);

    vec3 tint = vec3(
        params.GetFloat("Wave Height"),
        params.GetFloat("Wave Density"),
        params.GetFloat("Water Fog")
    );
    water_surface_obj.SetTint(tint);
}

void UpdateWaterDecal() {
    if (water_decal_id == -1) {
        water_decal_id = CreateObject("Data/Objects/Decals/water_fog.xml", true);
    }
    Object@ water_decal_obj = ReadObjectFromID(water_decal_id);
    Object@ hotspot_obj = ReadObjectFromID(hotspot.GetID());
    water_decal_obj.SetTranslation(hotspot_obj.GetTranslation());
    water_decal_obj.SetRotation(hotspot_obj.GetRotation());
    water_decal_obj.SetScale(hotspot_obj.GetScale() * 4.0f);
}

void HandleCollisions() {
    array<int> colliding_objects;
    level.GetCollidingObjects(hotspot.GetID(), colliding_objects);
    for (uint i = 0; i < colliding_objects.length(); ++i) {
        int object_id = colliding_objects[i];
        if (!ObjectExists(object_id)) {
            continue;
        }
        Object@ obj = ReadObjectFromID(object_id);
        if (obj.GetType() != _movement_object) {
            continue;
        }
        MovementObject@ mo = ReadCharacterID(object_id);
        mo.Execute("WaterIntersect(" + hotspot.GetID() + ");");
        if (params.HasParam("Lethal")) {
            mo.Execute("zone_killed=1; TakeDamage(1.0f);");
        }
    }
}

void DeleteObjectById(int& inout object_id) {
    if (object_id != -1) {
        QueueDeleteObjectID(object_id);
        object_id = -1;
    }
}
