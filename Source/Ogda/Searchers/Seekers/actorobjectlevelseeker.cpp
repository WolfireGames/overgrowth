//-----------------------------------------------------------------------------
//           Name: actorobjectlevelseeker.cpp
//      Developer: Wolfire Games LLC
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
#include "actorobjectlevelseeker.h"

#include <tinyxml.h>
#include <XML/xml_helper.h>

#include <Internal/filesystem.h>
#include <Logging/logdata.h>

#define ARRLEN(arr) sizeof(arr)/sizeof(arr[0])

struct Parameter
{
    const char* name;
    const char* type;
    bool ignored;
    bool into_file; 
    const char* ignored_value;

public:
    Parameter(const char* name, const char* type, bool ignored, bool into_file) : name(name), type(type), ignored(ignored), into_file(into_file), ignored_value(NULL) {};
    Parameter(const char* name, const char* type, bool ignored, bool into_file, const char* ignored_value) : name(name), type(type), ignored(ignored), into_file(into_file), ignored_value(ignored_value){ };

    bool Search(std::vector<Item>& items, const Item& item, TiXmlElement* el )
    {
        const char* name_str = el->Attribute("name");
        const char* val_str = el->Attribute("val");

        if( name_str )
        {
            if(strcmp(name_str, name) == 0 )
            {
                if( !ignored )
                {
                    if( ignored_value == NULL || strcmp(ignored_value,val_str) != 0 ) 
                    {
                        if( into_file )
                        {
                            Item i(item.input_folder, DumpIntoFile( val_str, strlen( val_str ) ), type, item.source);
                            i.SetOnlySearch(true);
                            i.SetDeleteOnExit(true);
                            items.push_back(i);
                        }
                        else
                        {
                            items.push_back(Item(item.input_folder, val_str, type, item.source));
                        }
                    }
                }
             
                return true;
            }
        }
        else
        {
            LOGE << "Missing attribute name on row " << el->Row() << " in " << item << std::endl; 
        }
        return false;
    }
};

struct ActorObject
{
    const char* name;
    const char* type;
    bool ignored;
    std::vector<Parameter> parameters;

public:
    ActorObject( const char* name, const char* type, bool ignored, std::vector<Parameter> parameters ) : name(name), type(type), ignored(ignored), parameters( parameters ){};

    bool Search(std::vector<Item>& items, const Item& item, TiXmlElement* eao)
    {
        if( strcmp(eao->Value(), name) == 0 )
        {
            const char* t = eao->Attribute("type_file"); 

            if( !t ) {
                t = eao->Attribute("prefab_path"); 
            }

            if( !ignored )
            {
                if( t )
                { 
                    items.push_back(Item(item.input_folder, t, type, item.source));
                }
                else 
                {
                    LOGE << "Missing attribute type_file/prefab_path on row " << eao->Row() << " in " << item << std::endl; 
                } 
            }

            TiXmlElement* p = eao->FirstChildElement( "parameters" );
            
            if( p )
            {
                TiXmlElement* el = p->FirstChildElement();

                while( el )
                {
                    bool found = false;
                    for(auto & parameter : parameters)
                    {
                        found |= parameter.Search(
                            items, 
                            item, 
                            el);
                    }

                    if( !found )
                    {
                        const char* name_str = el->Attribute("name");
                        LOGW << "Missing attribute handler for name \"" << name_str << "\" on row " << eao->Row() << " in " << item << std::endl; 
                    }

                    el = el->NextSiblingElement();
                }
            }

            return true;
        }
        else
        {
            return false;
        }
    }
};

void ActorObjectLevelSeeker::SearchGroup( std::vector<Item>& items, const Item& item, TiXmlElement* root )
{
    std::vector<Parameter> global_params;
    
    global_params.push_back( Parameter("Type", "", true, false));
    global_params.push_back( Parameter("Exclamation Character", "", true, false));
    global_params.push_back( Parameter("Hearing Modifier", "", true, false));
    global_params.push_back( Parameter("goal_0_post", "", true, false));
    global_params.push_back( Parameter("goal_1_post", "", true, false));
    global_params.push_back( Parameter("goal_2_post", "", true, false));

    global_params.push_back( Parameter("goal_0_pre", "", true, false));
    global_params.push_back( Parameter("goal_1_pre", "", true, false));
    global_params.push_back( Parameter("goal_2_pre", "", true, false));
    global_params.push_back( Parameter("goal_3_pre", "", true, false));
    global_params.push_back( Parameter("goal_4_pre", "", true, false));
    global_params.push_back( Parameter("goal_5_pre", "", true, false));
    global_params.push_back( Parameter("goal_6_pre", "", true, false));
    global_params.push_back( Parameter("goal_7_pre", "", true, false));
    global_params.push_back( Parameter("goal_8_pre", "", true, false));
    global_params.push_back( Parameter("goal_9_pre", "", true, false));
    global_params.push_back( Parameter("goal_10_pre", "", true, false));
    global_params.push_back( Parameter("goal_11_pre", "", true, false));
    global_params.push_back( Parameter("goal_12_pre", "", true, false));
    global_params.push_back( Parameter("goal_13_pre", "", true, false));
    global_params.push_back( Parameter("goal_14_pre", "", true, false));
    global_params.push_back( Parameter("goal_15_pre", "", true, false));
    global_params.push_back( Parameter("goal_16_pre", "", true, false));
    global_params.push_back( Parameter("goal_17_pre", "", true, false));

    global_params.push_back( Parameter("goal_0_post", "", true, false));
    global_params.push_back( Parameter("goal_1_post", "", true, false));
    global_params.push_back( Parameter("goal_2_post", "", true, false));
    global_params.push_back( Parameter("goal_3_post", "", true, false));
    global_params.push_back( Parameter("goal_4_post", "", true, false));
    global_params.push_back( Parameter("goal_5_post", "", true, false));
    global_params.push_back( Parameter("goal_6_post", "", true, false));
    global_params.push_back( Parameter("goal_7_post", "", true, false));
    global_params.push_back( Parameter("goal_8_post", "", true, false));
    global_params.push_back( Parameter("goal_9_post", "", true, false));
    global_params.push_back( Parameter("goal_10_post", "", true, false));
    global_params.push_back( Parameter("goal_11_post", "", true, false));
    global_params.push_back( Parameter("goal_12_post", "", true, false));
    global_params.push_back( Parameter("goal_13_post", "", true, false));
    global_params.push_back( Parameter("goal_14_post", "", true, false));
    global_params.push_back( Parameter("goal_15_post", "", true, false));
    global_params.push_back( Parameter("goal_16_post", "", true, false));
    global_params.push_back( Parameter("goal_17_post", "", true, false));

    global_params.push_back( Parameter("goal_0", "", true, false));
    global_params.push_back( Parameter("goal_1", "", true, false));
    global_params.push_back( Parameter("goal_2", "", true, false));
    global_params.push_back( Parameter("goal_3", "", true, false));
    global_params.push_back( Parameter("goal_4", "", true, false));
    global_params.push_back( Parameter("goal_5", "", true, false));
    global_params.push_back( Parameter("goal_6", "", true, false));
    global_params.push_back( Parameter("goal_7", "", true, false));
    global_params.push_back( Parameter("goal_8", "", true, false));
    global_params.push_back( Parameter("goal_9", "", true, false));
    global_params.push_back( Parameter("goal_10", "", true, false));
    global_params.push_back( Parameter("goal_11", "", true, false));
    global_params.push_back( Parameter("goal_12", "", true, false));
    global_params.push_back( Parameter("goal_13", "", true, false));
    global_params.push_back( Parameter("goal_14", "", true, false));
    global_params.push_back( Parameter("goal_15", "", true, false));
    global_params.push_back( Parameter("goal_16", "", true, false));
    global_params.push_back( Parameter("goal_17", "", true, false));

    global_params.push_back( Parameter("Stick To Nav Mesh", "", true, false));

    global_params.push_back( Parameter("fall_death", "", true, false));
    global_params.push_back( Parameter("fall_damage_mult", "", true, false));

    global_params.push_back( Parameter("time", "", true, false));

    global_params.push_back( Parameter("Display Text", "", true, false));

    global_params.push_back( Parameter("KillNPC", "", true, false));
    global_params.push_back( Parameter("KillPlayer", "", true, false));
    global_params.push_back( Parameter("characters", "", true, false));

    global_params.push_back( Parameter("Start Disabled", "", true, false));

    global_params.push_back( Parameter("Throw Counter Probability", "", true, false));
    global_params.push_back( Parameter("Throw Trainer", "", true, false));
    global_params.push_back( Parameter("Weapon Catch Skill", "", true, false));

    global_params.push_back( Parameter("SavedTransform", "", true, false));

    global_params.push_back( Parameter("rotation_scale", "", true, false));
    global_params.push_back( Parameter("time_scale", "", true, false));
    global_params.push_back( Parameter("translation_scale", "", true, false));

    global_params.push_back( Parameter("music_layer_override", "", true, false));

    global_params.push_back( Parameter("Invisible When Stationary", "", true, false));

    global_params.push_back( Parameter("No Look Around", "", true, false));

    global_params.push_back( Parameter("Short", "", true, false));
    global_params.push_back( Parameter("Offset", "", true, false));
    global_params.push_back( Parameter("Lethal", "", true, false));

    std::vector<ActorObject> aos;
    {
        std::vector<Parameter> params = global_params;
        params.push_back( Parameter("Dialogue", "dialogue",false,false, "empty"));

        params.push_back( Parameter("Script", "dialogue",false,true));
        params.push_back( Parameter("Battles", "battles",false,true));

        params.push_back( Parameter("Name", "",true,false));
        params.push_back( Parameter("DisplayName", "",true,false));
        params.push_back( Parameter("NumParticipants", "",true,false));
        params.push_back( Parameter("obj_1", "",true,false));
        params.push_back( Parameter("obj_2", "",true,false));
        params.push_back( Parameter("obj_3", "",true,false));
        params.push_back( Parameter("obj_4", "",true,false));
        params.push_back( Parameter("obj_5", "",true,false));
        params.push_back( Parameter("obj_6", "",true,false));
        params.push_back( Parameter("obj_7", "",true,false));
        params.push_back( Parameter("obj_8", "",true,false));
        params.push_back( Parameter("obj_9", "",true,false));
        params.push_back( Parameter("game_type", "",true,false));
        params.push_back( Parameter("team", "",true,false));
        params.push_back( Parameter("LocName", "",true,false));
        params.push_back( Parameter("Visible in game", "",true,false));
        params.push_back( Parameter("Automatic", "",true,false));


        ActorObject ao( "PlaceholderObject", "", true, params  );
        aos.push_back(ao);
    }
    {
        std::vector<Parameter> params = global_params;

        params.push_back( Parameter("Wait", "", true, false));
        
        ActorObject ao( "PathPointObject", "", true, params  );
        aos.push_back(ao);
    }
    {
        std::vector<Parameter> params = global_params;
        ActorObject ao( "CameraObject", "", true, params  );
        aos.push_back(ao);
    }
    {
        std::vector<Parameter> params = global_params;

        ActorObject ao( "parameters", "", true, params  );
        aos.push_back(ao);
    }
    {
        std::vector<Parameter> params = global_params;
        ActorObject ao( "AmbientSoundObject", "ambient_sound_object", false, params  );
        aos.push_back(ao);
    }
    {
        std::vector<Parameter> params = global_params;
        params.push_back( Parameter("Rotating Y","",        true,false));

        ActorObject ao( "EnvObject", "object", false, params  );
        aos.push_back(ao);
    }
    {
        std::vector<Parameter> params = global_params;
        params.push_back( Parameter("Aggression","",        true,false));
        params.push_back( Parameter("Attack Damage","",     true,false));
        params.push_back( Parameter("Attack Knockback" ,"", true,false));
        params.push_back( Parameter("Attack Speed","",      true,false));
        params.push_back( Parameter("Block Follow-up" ,"",  true,false));
        params.push_back( Parameter("Block Skill","",       true,false));
        params.push_back( Parameter("Character Scale","",   true,false));
        params.push_back( Parameter("Damage Resistance" ,"",true,false));
        params.push_back( Parameter("Ear Size","",          true,false));
        params.push_back( Parameter("Fat","",               true,false));
        params.push_back( Parameter("Ground Aggression","", true,false));
        params.push_back( Parameter("Left handed","",       true,false));
        params.push_back( Parameter("Lives","",             true,false));
        params.push_back( Parameter("Movement Speed","",    true,false));
        params.push_back( Parameter("Muscle","",            true,false));
        params.push_back( Parameter("Static","",            true,false));
        params.push_back( Parameter("Teams","",             true,false));
        params.push_back( Parameter("Knockout Shield","",             true,false));
        params.push_back( Parameter("Armor","",             true,false));
        params.push_back( Parameter("dead_body","",             true,false));

        params.push_back( Parameter("Peripheral FOV distance","",             true,false));
        params.push_back( Parameter("Peripheral FOV horizontal","",             true,false));
        params.push_back( Parameter("Peripheral FOV vertical","",             true,false));

        params.push_back( Parameter("Focus FOV distance","",             true,false));
        params.push_back( Parameter("Focus FOV horizontal","",             true,false));
        params.push_back( Parameter("Focus FOV vertical","",             true,false));

        ActorObject ao( "ActorObject", "actor_object", false, params  );
        aos.push_back(ao);
    }
    {
        std::vector<Parameter> params = global_params;
        ActorObject ao( "Decal", "decal_object", false, params  );
        aos.push_back(ao);
    }
    {
        std::vector<Parameter> params = global_params;

        params.push_back(Parameter("Collectables needed", "", true, false));
        params.push_back(Parameter("Damage", "", true, false));
        params.push_back(Parameter("Dialogue", "", true, false));
        params.push_back(Parameter("LightDuration", "", true, false));
        params.push_back(Parameter("ParticleInterval", "", true, false));
        params.push_back(Parameter("TintFlames", "", true, false));
        params.push_back(Parameter("UseFire", "", true, false));
        params.push_back(Parameter("UseLights", "", true, false));
        params.push_back(Parameter("InstantKill", "", true, false));
        params.push_back(Parameter("XVel", "", true, false));
        params.push_back(Parameter("YVel", "", true, false));
        params.push_back(Parameter("ZVel", "", true, false));
        params.push_back(Parameter("light_id", "", true, false));
        params.push_back(Parameter("spawn_point", "", true, false));
        params.push_back(Parameter("LastEnteredTime", "", true, false));
        params.push_back(Parameter("music", "", true, false));
        params.push_back(Parameter("Automatic", "", true, false));
        params.push_back(Parameter("Visible in game", "", true, false));
        params.push_back(Parameter("Armor", "", true, false));
        params.push_back(Parameter("Portals", "", true, false));
        params.push_back(Parameter("player_id", "", true, false));
        params.push_back(Parameter("Delay Max", "", true, false));
        params.push_back(Parameter("Delay Min", "", true, false));
        params.push_back(Parameter("Fade Distance", "", true, false));
        params.push_back(Parameter("Gain", "", true, false));
        params.push_back(Parameter("Global", "", true, false));
        params.push_back(Parameter("Invisible", "", true, false));
        params.push_back(Parameter("Water Fog", "", true, false));
        params.push_back(Parameter("Wave Density", "", true, false));
        params.push_back(Parameter("Wave Height", "", true, false));
        params.push_back(Parameter("Delay Max", "", true, false));
        params.push_back(Parameter("Delay Min", "", true, false));
        params.push_back(Parameter("Fade Distance", "", true, false));
        params.push_back(Parameter("Fire Ribbons", "", true, false));
        params.push_back(Parameter("Ignite Characters", "", true, false));
        params.push_back(Parameter("Light Amplify", "", true, false));
        params.push_back(Parameter("Light Distance", "", true, false));
        params.push_back(Parameter("Light Type", "", true, false));
        params.push_back(Parameter("Question Character", "", true, false));
        params.push_back(Parameter("Visible in game", "", true, false));
        params.push_back(Parameter("Hearing Modifier", "", true, false));
        params.push_back(Parameter("Wind Scale", "", true, false));
        params.push_back(Parameter("player_spawn", "", true, false));
        params.push_back(Parameter("checkpoint_id", "", true, false));
        params.push_back(Parameter("level_hotspot_id", "", true, false));
        params.push_back(Parameter("Hotspot ID", "", true, false));
        params.push_back(Parameter("Objects", "", true, false));

        params.push_back(Parameter("Display Image", "texture", false, false));
        params.push_back(Parameter("Level to load", "level", false, false));
        params.push_back(Parameter("next_level", "level", false, false));
        params.push_back(Parameter("Sound Path", "sound", false, false));

        ActorObject ao( "Hotspot", "hotspot_object", false, params  );
        aos.push_back(ao);
    }
    {
        std::vector<Parameter> params = global_params;
        ActorObject ao( "ItemObject", "item", false, params  );
        aos.push_back(ao);
    }
    {
        std::vector<Parameter> params = global_params;

        params.push_back(Parameter("Negative", "", true, false));

        ActorObject ao( "LightProbeObject", "", true, params  );
        aos.push_back(ao);
    }
    {
        std::vector<Parameter> params = global_params;
        ActorObject ao( "NavmeshRegionObject", "", true, params  );
        aos.push_back(ao);
    }
    {
        std::vector<Parameter> params = global_params;
        ActorObject ao( "NavmeshHintObject", "", true, params  );
        aos.push_back(ao);
    }
    {
        std::vector<Parameter> params = global_params;
        ActorObject ao( "NavmeshConnectionObject", "", true, params  );
        aos.push_back(ao);
    }
    {
        std::vector<Parameter> params = global_params;

        params.push_back(Parameter("Global", "", true, false));
        ActorObject ao( "ReflectionCaptureObject", "", true, params  );
        aos.push_back(ao);
    }
    {
        std::vector<Parameter> params = global_params;
        ActorObject ao( "LightVolumeObject", "", true, params  );
        aos.push_back(ao);
    }
    {
        std::vector<Parameter> params = global_params;

        params.push_back( Parameter("Global", "",true,false));

        ActorObject ao( "ReflectionCaptureObject", "", true, params  );
        aos.push_back(ao);
    }
    {
        std::vector<Parameter> params = global_params;
        params.push_back(Parameter("portal_a_light", "", true, false));

        params.push_back( Parameter("Level to load", "", true,false));

        ActorObject ao( "DynamicLightObject", "", true, params  );
        aos.push_back(ao);
    }
    {
        std::vector<Parameter> params = global_params;

        ActorObject ao( "Group", "", true, params  );
        aos.push_back(ao);
    }
    {
        std::vector<Parameter> params = global_params;

        ActorObject ao( "Prefab", "prefab", true, params  );
        aos.push_back(ao);
    }
    {
        std::vector<Parameter> params = global_params;

        ActorObject ao( "EnvObjectAttachments", "", true, params  );
        aos.push_back(ao);
    }
    {
        std::vector<Parameter> params = global_params;

        ActorObject ao( "Palette", "", true, params  );
        aos.push_back(ao);
    }
    {
        std::vector<Parameter> params = global_params;

        ActorObject ao( "ItemConnections", "", true, params  );
        aos.push_back(ao);
    }
    {
        std::vector<Parameter> params = global_params;

        ActorObject ao( "Connections", "", true, params  );
        aos.push_back(ao);
    }


    TiXmlElement* el = root->FirstChildElement();
    while( el )
    {
        if( strcmp( el->Value(), "Group" ) == 0 )
        {
            SearchGroup(items,item,el);
        }
        else if( strcmp( el->Value(), "Prefab" ) == 0 )
        {
            SearchGroup(items,item,el);
        }
        else if( strcmp( el->Value(), "ActorObject" ) == 0 ) //Actor objects can have sub EnvObjects for attachments
        {
            SearchGroup(items,item,el);
        }

        bool found = false;

        for(auto & ao : aos)
        {
            found |= ao.Search( items, item, el );
        } 

        if( !found )
        {
            LOGE << "Urecognized ActorObject " << el->Value() << " on row " << el->Row() << " in " << item  << std::endl;
        }

        el = el->NextSiblingElement();
    }
}

std::vector<Item> ActorObjectLevelSeeker::SearchLevelRoot( const Item& source, TiXmlHandle& hRoot )
{
    std::vector<Item> items;

    //The standard root now seems to be ActorObjects, but for historic reasons we look for some more than that.
    const char* entity_roots[] = 
    {
        "ActorObjects", //Currently standard collected 
        "Groups",
        "EnvObjects",
        "Decals",
        "Hotspots"
    };

    for(auto & entity_root : entity_roots)
    {
        TiXmlElement* elem = hRoot.FirstChildElement(entity_root).ToElement();

        if( elem )
        {
            SearchGroup( items, source, elem );
        }
        else
        {
            LOGD << source << " Is missing " << entity_root << std::endl;
        }
    }
    return items;
}
