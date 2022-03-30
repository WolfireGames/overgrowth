//-----------------------------------------------------------------------------
//           Name: world_sim.as
//      Developer: Wolfire Games LLC
//    Script Type:
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

const int kNumGoodTypes = 5;

class Town {
    int faction;
    int wealth;
    int population;
    string name;
    array<int> goods;
    int vis_id;
    array<int> goods_vis_id;
};

class Caravan {
    int town_destination;
    int vis_id;
};

enum RoadType {
    trail = 0,
    dirt = 1,
    paved = 2
};

class Road {
    float length;
    RoadType type;
    int start;
    int end;
};

array<Town> towns;
array<Caravan> caravans;

void Init(string str){
}

void Dispose(){
    int num_towns = towns.size();
    for(int i=0; i<num_towns; ++i){
        Log(info, "Test");
        DeleteObjectID(towns[i].vis_id);
        int len=towns[i].goods_vis_id.size();
        for(int j=0; j<len; ++j){
            Log(info, "Test");
            DeleteObjectID(towns[i].goods_vis_id[j]);
        }
    }
    towns.resize(0);
    
    int num_caravans = caravans.size();
    for(int i=0; i<num_caravans; ++i){
        Log(info, "Test");
        DeleteObjectID(caravans[i].vis_id);
    }
    caravans.resize(0);
}

bool HasFocus() {
    return false;
}

void Update() {
    SetPlaceholderPreviews();

    if(GetInputPressed(0,"b") && GetInputDown(0,"ctrl")){
        array<Object@> town_spawns;
        array<int> @object_ids = GetObjectIDs();
        int num_objects = object_ids.length();
        for(int i=0; i<num_objects; ++i){
            Object @obj = ReadObjectFromID(object_ids[i]);
            if(obj.IsSelected()){
                if(obj.GetType() == _placeholder_object){
                    ScriptParams@ params = obj.GetScriptParams();
                    if(params.GetString("type") == "town_spawn"){
                        Log(info,"Town selected with ID: "+object_ids[i]);
                    }
                }
            }
        }
    }

    int num_caravans = caravans.size();
    for(int i=0; i<num_caravans; ++i){
        Caravan @caravan = caravans[i];
        if(caravan.town_destination < int(towns.size())){
            vec3 dest = ReadObjectFromID(towns[caravan.town_destination].vis_id).GetTranslation();
            Object@ caravan_obj = ReadObjectFromID(caravans[i].vis_id);
            vec3 curr_pos = caravan_obj.GetTranslation();
            float dist = xz_distance(dest, curr_pos);
            float speed = time_step;
            vec3 dir = dest - curr_pos;
            vec3 flat_dir = dir;
            flat_dir.y = 0;
            flat_dir = normalize(flat_dir);
            if(dist < speed){
                float old_height = curr_pos.y;
                curr_pos = dest;
                curr_pos.y = old_height;
                caravan.town_destination = (caravan.town_destination + 1) % towns.size();
            } else {
                curr_pos += flat_dir * speed;
            }
            caravan_obj.SetTranslation(curr_pos);
            quaternion rotation_x(vec4(1,0,0,3.1415f));
            quaternion rotation_y(vec4(0,1,0,atan2(flat_dir.x, flat_dir.z)));
            caravan_obj.SetRotation(rotation_y);
        }
    }
}

void DrawGUI() {
}

void ReceiveMessage(string msg) {
    TokenIterator token_iter;
    token_iter.Init();
    if(!token_iter.FindNextToken(msg)){
        return;
    }
    string token = token_iter.GetToken(msg);
    if(token == "reset"){
        Dispose();
        SetupSim();    
    } else if(token == "dispose_level"){
    }
}

void SetPlaceholderPreviews() {
    array<int> @object_ids = GetObjectIDs();
    int num_objects = object_ids.length();
    for(int i=0; i<num_objects; ++i){
        Object @obj = ReadObjectFromID(object_ids[i]);
        ScriptParams@ params = obj.GetScriptParams();
        if(params.HasParam("Name")){
            string name_str = params.GetString("Name");
            if("town_spawn" == name_str){
                PlaceholderObject@ placeholder_object = cast<PlaceholderObject@>(obj);
                placeholder_object.SetPreview("Data/Objects/world_map/city.xml");
            }
        }
    }
}


void SetupSim(){
    array<Object@> town_spawns;
    array<int> @object_ids = GetObjectIDs();
    array<int> city_markers;

    int num_objects = object_ids.length();
    for(int i=0; i<num_objects; ++i){
       Object @obj = ReadObjectFromID(object_ids[i]);
        ScriptParams@ params = obj.GetScriptParams();
        if(params.HasParam("Name")){
            string name_str = params.GetString("Name");
            if("town_spawn" == name_str){
                city_markers.push_back(object_ids[i]);
            }
        }
    }
    
    int num_city_markers = city_markers.size();
    towns.resize(num_city_markers);
    Log(info,"Num city markers: "+num_city_markers);
    for(int i=0; i<num_city_markers; ++i){
        int city_marker_id = city_markers[i];
        Object@ city_marker = ReadObjectFromID(city_marker_id);
        int new_city_id = CreateObject("Data/Objects/world_map/city_prefab.xml", true);
        Object@ new_city = ReadObjectFromID(new_city_id);
        new_city.SetTranslation(city_marker.GetTranslation());

        towns[i].vis_id = new_city_id;
        towns[i].wealth = rand()%5000;
        towns[i].population = rand()%5000;
        towns[i].goods.resize(kNumGoodTypes);
        towns[i].faction = rand()%4;
        vec3 color = vec3(1,1,1);
        switch(towns[i].faction){
        case 0:
            color = vec3(0,1,1);
            break;
        case 1:
            color = vec3(1,1,0);
            break;
        case 2:
            color = vec3(1,0,1);
            break;
        case 3:
            color = vec3(1,0,1);
            break;
        }
        //new_flag.SetTint(color);
    }
    

    caravans.resize(1);
    caravans[0].vis_id = CreateObject("Data/Objects/world_map/caravan.xml", true);
    Object@ new_obj = ReadObjectFromID(caravans[0].vis_id);
    vec3 town_pos = ReadObjectFromID(towns[0].vis_id).GetTranslation();
    new_obj.SetTranslation(vec3(town_pos.x,new_obj.GetTranslation().y,town_pos.z));
    ScriptParams@ params = new_obj.GetScriptParams();
    params.AddIntCheckbox("No Save", true);
    caravans[0].town_destination = 1;
}
