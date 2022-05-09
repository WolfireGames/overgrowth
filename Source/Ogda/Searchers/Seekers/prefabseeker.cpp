//-----------------------------------------------------------------------------
//           Name: prefabseeker.cpp
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
#include "prefabseeker.h"

#include <cassert>

#include <tinyxml.h>

#include <Utility/strings.h>
#include <Logging/logdata.h>

enum {
    ROOT = 1,
    GROUP,
    PARAMETER
};

std::vector<Item> PrefabSeeker::SearchXML(const Item& item, TiXmlDocument& doc) {
    std::vector<Item> items;

    std::vector<elempair> elems;
    elems.push_back(elempair("Prefab", ""));
    elems.push_back(elempair("Groups", ""));
    elems.push_back(elempair("ActorObjects", ""));
    elems.push_back(elempair("EnvObjects", ""));
    elems.push_back(elempair("Decals", ""));
    elems.push_back(elempair("Hotspots", ""));

    std::vector<const char*> elems_ignore;
    elems_ignore.push_back("Type");

    ElementScanner::Do(items, item, &doc, elems, elems_ignore, this, (void*)ROOT);

    return items;
}

void PrefabSeeker::HandleElementCallback(std::vector<Item>& items, TiXmlNode* eRoot, TiXmlElement* eElem, const Item& item, void* userdata) {
    if (userdata == (void*)ROOT) {
        std::vector<elempair> elems;
        elems.push_back(elempair("parameters", ""));
        elems.push_back(elempair("Prefab", ""));
        elems.push_back(elempair("Group", ""));
        elems.push_back(elempair("ActorObject", ""));
        elems.push_back(elempair("EnvObject", ""));
        elems.push_back(elempair("Decal", ""));
        elems.push_back(elempair("Hotspot", ""));
        elems.push_back(elempair("PlaceholderObject", ""));
        elems.push_back(elempair("PathPointObject", ""));
        elems.push_back(elempair("LightProbeObject", ""));
        elems.push_back(elempair("DynamicLightObject", ""));
        elems.push_back(elempair("NavmeshHintObject", ""));
        elems.push_back(elempair("NavmeshConnectionObject", ""));
        elems.push_back(elempair("NavmeshRegionObject", ""));
        elems.push_back(elempair("ReflectionCaptureObject", ""));
        elems.push_back(elempair("LightVolumeObject", ""));

        std::vector<const char*> elems_ignore;

        ElementScanner::Do(items, item, eElem, elems, elems_ignore, this, (void*)GROUP);
    } else if (userdata == (void*)GROUP) {
        if (strmtch(eElem->Value(), "Group")) {
            std::vector<attribpair> attribs;

            std::vector<const char*> attribs_ignore;

            attribs_ignore.push_back("id");
            attribs_ignore.push_back("group_id");

            attribs_ignore.push_back("s0");
            attribs_ignore.push_back("s1");
            attribs_ignore.push_back("s2");

            attribs_ignore.push_back("t0");
            attribs_ignore.push_back("t1");
            attribs_ignore.push_back("t2");

            attribs_ignore.push_back("color_r");
            attribs_ignore.push_back("color_b");
            attribs_ignore.push_back("color_g");

            attribs_ignore.push_back("overbright");

            attribs_ignore.push_back("r0");
            attribs_ignore.push_back("r1");
            attribs_ignore.push_back("r2");
            attribs_ignore.push_back("r3");
            attribs_ignore.push_back("r4");
            attribs_ignore.push_back("r5");
            attribs_ignore.push_back("r6");
            attribs_ignore.push_back("r7");
            attribs_ignore.push_back("r8");
            attribs_ignore.push_back("r9");
            attribs_ignore.push_back("r10");
            attribs_ignore.push_back("r11");
            attribs_ignore.push_back("r12");
            attribs_ignore.push_back("r13");
            attribs_ignore.push_back("r14");
            attribs_ignore.push_back("r15");

            attribs_ignore.push_back("uses_imposter");

            attribs_ignore.push_back("d0");
            attribs_ignore.push_back("d1");
            attribs_ignore.push_back("d2");

            attribs_ignore.push_back("c0");
            attribs_ignore.push_back("c1");
            attribs_ignore.push_back("c2");

            attribs_ignore.push_back("q0");
            attribs_ignore.push_back("q1");
            attribs_ignore.push_back("q2");
            attribs_ignore.push_back("q3");

            attribs_ignore.push_back("version");

            AttributeScanner::Do(items, item, eElem, attribs, attribs_ignore);

            std::vector<elempair> elems;
            elems.push_back(elempair("parameters", ""));
            elems.push_back(elempair("Prefab", ""));
            elems.push_back(elempair("Group", ""));
            elems.push_back(elempair("ActorObject", ""));
            elems.push_back(elempair("EnvObject", ""));
            elems.push_back(elempair("Decal", ""));
            elems.push_back(elempair("Hotspot", ""));
            elems.push_back(elempair("PlaceholderObject", ""));
            elems.push_back(elempair("PathPointObject", ""));
            elems.push_back(elempair("LightProbeObject", ""));
            elems.push_back(elempair("DynamicLightObject", ""));
            elems.push_back(elempair("NavmeshHintObject", ""));
            elems.push_back(elempair("NavmeshConnectionObject", ""));
            elems.push_back(elempair("NavmeshRegionObject", ""));
            elems.push_back(elempair("ReflectionCaptureObject", ""));
            elems.push_back(elempair("LightVolumeObject", ""));

            std::vector<const char*> elems_ignore;

            ElementScanner::Do(items, item, eElem, elems, elems_ignore, this, (void*)GROUP);
        } else if (strmtch(eElem->Value(), "Prefab")) {
            std::vector<attribpair> attribs;

            std::vector<const char*> attribs_ignore;

            attribs_ignore.push_back("id");
            attribs_ignore.push_back("group_id");

            attribs_ignore.push_back("s0");
            attribs_ignore.push_back("s1");
            attribs_ignore.push_back("s2");

            attribs_ignore.push_back("t0");
            attribs_ignore.push_back("t1");
            attribs_ignore.push_back("t2");

            attribs_ignore.push_back("color_r");
            attribs_ignore.push_back("color_b");
            attribs_ignore.push_back("color_g");

            attribs_ignore.push_back("overbright");

            attribs_ignore.push_back("r0");
            attribs_ignore.push_back("r1");
            attribs_ignore.push_back("r2");
            attribs_ignore.push_back("r3");
            attribs_ignore.push_back("r4");
            attribs_ignore.push_back("r5");
            attribs_ignore.push_back("r6");
            attribs_ignore.push_back("r7");
            attribs_ignore.push_back("r8");
            attribs_ignore.push_back("r9");
            attribs_ignore.push_back("r10");
            attribs_ignore.push_back("r11");
            attribs_ignore.push_back("r12");
            attribs_ignore.push_back("r13");
            attribs_ignore.push_back("r14");
            attribs_ignore.push_back("r15");

            attribs_ignore.push_back("uses_imposter");

            attribs_ignore.push_back("d0");
            attribs_ignore.push_back("d1");
            attribs_ignore.push_back("d2");

            attribs_ignore.push_back("c0");
            attribs_ignore.push_back("c1");
            attribs_ignore.push_back("c2");

            attribs_ignore.push_back("q0");
            attribs_ignore.push_back("q1");
            attribs_ignore.push_back("q2");
            attribs_ignore.push_back("q3");

            attribs_ignore.push_back("version");

            AttributeScanner::Do(items, item, eElem, attribs, attribs_ignore);

            std::vector<elempair> elems;
            elems.push_back(elempair("parameters", ""));
            elems.push_back(elempair("Prefab", ""));
            elems.push_back(elempair("Group", ""));
            elems.push_back(elempair("ActorObject", ""));
            elems.push_back(elempair("EnvObject", ""));
            elems.push_back(elempair("Decal", ""));
            elems.push_back(elempair("Hotspot", ""));
            elems.push_back(elempair("PlaceholderObject", ""));
            elems.push_back(elempair("PathPointObject", ""));
            elems.push_back(elempair("LightProbeObject", ""));
            elems.push_back(elempair("DynamicLightObject", ""));
            elems.push_back(elempair("NavmeshHintObject", ""));
            elems.push_back(elempair("NavmeshConnectionObject", ""));
            elems.push_back(elempair("NavmeshRegionObject", ""));
            elems.push_back(elempair("ReflectionCaptureObject", ""));
            elems.push_back(elempair("LightVolumeObject", ""));

            std::vector<const char*> elems_ignore;

            ElementScanner::Do(items, item, eElem, elems, elems_ignore, this, (void*)GROUP);
        } else if (strmtch(eElem->Value(), "PlaceholderObject")) {
            std::vector<attribpair> attribs;
            attribs.push_back(attribpair("type_file", "object"));

            std::vector<const char*> attribs_ignore;
            attribs_ignore.push_back("id");
            attribs_ignore.push_back("group_id");
            attribs_ignore.push_back("s0");
            attribs_ignore.push_back("s1");
            attribs_ignore.push_back("s2");

            attribs_ignore.push_back("t0");
            attribs_ignore.push_back("t1");
            attribs_ignore.push_back("t2");

            attribs_ignore.push_back("color_r");
            attribs_ignore.push_back("color_b");
            attribs_ignore.push_back("color_g");

            attribs_ignore.push_back("overbright");

            attribs_ignore.push_back("r0");
            attribs_ignore.push_back("r1");
            attribs_ignore.push_back("r2");
            attribs_ignore.push_back("r3");
            attribs_ignore.push_back("r4");
            attribs_ignore.push_back("r5");
            attribs_ignore.push_back("r6");
            attribs_ignore.push_back("r7");
            attribs_ignore.push_back("r8");
            attribs_ignore.push_back("r9");
            attribs_ignore.push_back("r10");
            attribs_ignore.push_back("r11");
            attribs_ignore.push_back("r12");
            attribs_ignore.push_back("r13");
            attribs_ignore.push_back("r14");
            attribs_ignore.push_back("r15");

            attribs_ignore.push_back("q0");
            attribs_ignore.push_back("q1");
            attribs_ignore.push_back("q2");
            attribs_ignore.push_back("q3");

            attribs_ignore.push_back("special_type");

            AttributeScanner::Do(items, item, eElem, attribs, attribs_ignore);
            std::vector<elempair> elems;
            elems.push_back(elempair("parameters", ""));

            std::vector<const char*> elems_ignore;

            ElementScanner::Do(items, item, eElem, elems, elems_ignore, this, (void*)GROUP);
        } else if (strmtch(eElem->Value(), "EnvObject")) {
            std::vector<attribpair> attribs;
            attribs.push_back(attribpair("type_file", "object"));

            std::vector<const char*> attribs_ignore;
            attribs_ignore.push_back("id");
            attribs_ignore.push_back("group_id");
            attribs_ignore.push_back("s0");
            attribs_ignore.push_back("s1");
            attribs_ignore.push_back("s2");

            attribs_ignore.push_back("t0");
            attribs_ignore.push_back("t1");
            attribs_ignore.push_back("t2");

            attribs_ignore.push_back("color_r");
            attribs_ignore.push_back("color_b");
            attribs_ignore.push_back("color_g");

            attribs_ignore.push_back("overbright");

            attribs_ignore.push_back("r0");
            attribs_ignore.push_back("r1");
            attribs_ignore.push_back("r2");
            attribs_ignore.push_back("r3");
            attribs_ignore.push_back("r4");
            attribs_ignore.push_back("r5");
            attribs_ignore.push_back("r6");
            attribs_ignore.push_back("r7");
            attribs_ignore.push_back("r8");
            attribs_ignore.push_back("r9");
            attribs_ignore.push_back("r10");
            attribs_ignore.push_back("r11");
            attribs_ignore.push_back("r12");
            attribs_ignore.push_back("r13");
            attribs_ignore.push_back("r14");
            attribs_ignore.push_back("r15");

            attribs_ignore.push_back("q0");
            attribs_ignore.push_back("q1");
            attribs_ignore.push_back("q2");
            attribs_ignore.push_back("q3");

            AttributeScanner::Do(items, item, eElem, attribs, attribs_ignore);
            std::vector<elempair> elems;
            elems.push_back(elempair("parameters", ""));

            std::vector<const char*> elems_ignore;

            ElementScanner::Do(items, item, eElem, elems, elems_ignore, this, (void*)GROUP);
        } else if (strmtch(eElem->Value(), "Decal")) {
            std::vector<attribpair> attribs;
            attribs.push_back(attribpair("type_file", "decal_object"));

            std::vector<const char*> attribs_ignore;
            attribs_ignore.push_back("i");
            attribs_ignore.push_back("id");
            attribs_ignore.push_back("group_id");
            attribs_ignore.push_back("s0");
            attribs_ignore.push_back("s1");
            attribs_ignore.push_back("s2");
            attribs_ignore.push_back("r0");
            attribs_ignore.push_back("r1");
            attribs_ignore.push_back("r2");
            attribs_ignore.push_back("r3");
            attribs_ignore.push_back("r4");
            attribs_ignore.push_back("r5");
            attribs_ignore.push_back("r6");
            attribs_ignore.push_back("r7");
            attribs_ignore.push_back("r8");
            attribs_ignore.push_back("r9");
            attribs_ignore.push_back("r10");
            attribs_ignore.push_back("r11");
            attribs_ignore.push_back("r12");
            attribs_ignore.push_back("r13");
            attribs_ignore.push_back("r14");
            attribs_ignore.push_back("r15");

            attribs_ignore.push_back("color_r");
            attribs_ignore.push_back("color_b");
            attribs_ignore.push_back("color_g");
            attribs_ignore.push_back("overbright");

            attribs_ignore.push_back("q0");
            attribs_ignore.push_back("q1");
            attribs_ignore.push_back("q2");
            attribs_ignore.push_back("q3");

            attribs_ignore.push_back("p0");
            attribs_ignore.push_back("p1");
            attribs_ignore.push_back("p2");

            attribs_ignore.push_back("p_mode");

            attribs_ignore.push_back("t0");
            attribs_ignore.push_back("t1");
            attribs_ignore.push_back("t2");

            AttributeScanner::Do(items, item, eElem, attribs, attribs_ignore);

            std::vector<elempair> elems;
            elems.push_back(elempair("parameters", ""));

            std::vector<const char*> elems_ignore;

            ElementScanner::Do(items, item, eElem, elems, elems_ignore, this, (void*)GROUP);
        } else if (strmtch(eElem->Value(), "ActorObject")) {
            std::vector<attribpair> attribs;
            attribs.push_back(attribpair("type_file", "actor_object"));

            std::vector<const char*> attribs_ignore;
            attribs_ignore.push_back("id");
            attribs_ignore.push_back("group_id");
            attribs_ignore.push_back("s0");
            attribs_ignore.push_back("s1");
            attribs_ignore.push_back("s2");

            attribs_ignore.push_back("t0");
            attribs_ignore.push_back("t1");
            attribs_ignore.push_back("t2");

            attribs_ignore.push_back("color_r");
            attribs_ignore.push_back("color_b");
            attribs_ignore.push_back("color_g");
            attribs_ignore.push_back("overbright");

            attribs_ignore.push_back("r0");
            attribs_ignore.push_back("r1");
            attribs_ignore.push_back("r2");
            attribs_ignore.push_back("r3");
            attribs_ignore.push_back("r4");
            attribs_ignore.push_back("r5");
            attribs_ignore.push_back("r6");
            attribs_ignore.push_back("r7");
            attribs_ignore.push_back("r8");
            attribs_ignore.push_back("r9");
            attribs_ignore.push_back("r10");
            attribs_ignore.push_back("r11");
            attribs_ignore.push_back("r12");
            attribs_ignore.push_back("r13");
            attribs_ignore.push_back("r14");
            attribs_ignore.push_back("r15");

            attribs_ignore.push_back("color_r");
            attribs_ignore.push_back("color_b");
            attribs_ignore.push_back("color_g");

            attribs_ignore.push_back("overbright");

            attribs_ignore.push_back("q0");
            attribs_ignore.push_back("q1");
            attribs_ignore.push_back("q2");
            attribs_ignore.push_back("q3");

            attribs_ignore.push_back("is_player");

            AttributeScanner::Do(items, item, eElem, attribs, attribs_ignore);

            std::vector<elempair> elems;
            elems.push_back(elempair("parameters", ""));

            std::vector<const char*> elems_ignore;

            ElementScanner::Do(items, item, eElem, elems, elems_ignore, this, (void*)GROUP);
        } else if (strmtch(eElem->Value(), "PathPointObject")) {
            std::vector<attribpair> attribs;
            attribs.push_back(attribpair("type_file", "path_point_object"));

            std::vector<const char*> attribs_ignore;
            attribs_ignore.push_back("id");
            attribs_ignore.push_back("group_id");
            attribs_ignore.push_back("s0");
            attribs_ignore.push_back("s1");
            attribs_ignore.push_back("s2");

            attribs_ignore.push_back("t0");
            attribs_ignore.push_back("t1");
            attribs_ignore.push_back("t2");

            attribs_ignore.push_back("color_r");
            attribs_ignore.push_back("color_b");
            attribs_ignore.push_back("color_g");

            attribs_ignore.push_back("overbright");

            attribs_ignore.push_back("r0");
            attribs_ignore.push_back("r1");
            attribs_ignore.push_back("r2");
            attribs_ignore.push_back("r3");
            attribs_ignore.push_back("r4");
            attribs_ignore.push_back("r5");
            attribs_ignore.push_back("r6");
            attribs_ignore.push_back("r7");
            attribs_ignore.push_back("r8");
            attribs_ignore.push_back("r9");
            attribs_ignore.push_back("r10");
            attribs_ignore.push_back("r11");
            attribs_ignore.push_back("r12");
            attribs_ignore.push_back("r13");
            attribs_ignore.push_back("r14");
            attribs_ignore.push_back("r15");

            attribs_ignore.push_back("q0");
            attribs_ignore.push_back("q1");
            attribs_ignore.push_back("q2");
            attribs_ignore.push_back("q3");

            AttributeScanner::Do(items, item, eElem, attribs, attribs_ignore);

            std::vector<elempair> elems;
            elems.push_back(elempair("parameters", ""));

            std::vector<const char*> elems_ignore;
            elems_ignore.push_back("Connections");

            ElementScanner::Do(items, item, eElem, elems, elems_ignore, this, (void*)GROUP);
        } else if (strmtch(eElem->Value(), "LightProbeObject")) {
            std::vector<attribpair> attribs;

            std::vector<const char*> attribs_ignore;
            attribs_ignore.push_back("id");
            attribs_ignore.push_back("group_id");
            attribs_ignore.push_back("s0");
            attribs_ignore.push_back("s1");
            attribs_ignore.push_back("s2");

            attribs_ignore.push_back("t0");
            attribs_ignore.push_back("t1");
            attribs_ignore.push_back("t2");

            attribs_ignore.push_back("color_r");
            attribs_ignore.push_back("color_b");
            attribs_ignore.push_back("color_g");

            attribs_ignore.push_back("overbright");

            attribs_ignore.push_back("r0");
            attribs_ignore.push_back("r1");
            attribs_ignore.push_back("r2");
            attribs_ignore.push_back("r3");
            attribs_ignore.push_back("r4");
            attribs_ignore.push_back("r5");
            attribs_ignore.push_back("r6");
            attribs_ignore.push_back("r7");
            attribs_ignore.push_back("r8");
            attribs_ignore.push_back("r9");
            attribs_ignore.push_back("r10");
            attribs_ignore.push_back("r11");
            attribs_ignore.push_back("r12");
            attribs_ignore.push_back("r13");
            attribs_ignore.push_back("r14");
            attribs_ignore.push_back("r15");

            attribs_ignore.push_back("q0");
            attribs_ignore.push_back("q1");
            attribs_ignore.push_back("q2");
            attribs_ignore.push_back("q3");

            attribs_ignore.push_back("special_type");

            AttributeScanner::Do(items, item, eElem, attribs, attribs_ignore);
        } else if (strmtch(eElem->Value(), "DynamicLightObject")) {
            std::vector<attribpair> attribs;

            std::vector<const char*> attribs_ignore;
            attribs_ignore.push_back("id");
            attribs_ignore.push_back("group_id");
            attribs_ignore.push_back("s0");
            attribs_ignore.push_back("s1");
            attribs_ignore.push_back("s2");

            attribs_ignore.push_back("t0");
            attribs_ignore.push_back("t1");
            attribs_ignore.push_back("t2");

            attribs_ignore.push_back("color_r");
            attribs_ignore.push_back("color_b");
            attribs_ignore.push_back("color_g");

            attribs_ignore.push_back("overbright");

            attribs_ignore.push_back("r0");
            attribs_ignore.push_back("r1");
            attribs_ignore.push_back("r2");
            attribs_ignore.push_back("r3");
            attribs_ignore.push_back("r4");
            attribs_ignore.push_back("r5");
            attribs_ignore.push_back("r6");
            attribs_ignore.push_back("r7");
            attribs_ignore.push_back("r8");
            attribs_ignore.push_back("r9");
            attribs_ignore.push_back("r10");
            attribs_ignore.push_back("r11");
            attribs_ignore.push_back("r12");
            attribs_ignore.push_back("r13");
            attribs_ignore.push_back("r14");
            attribs_ignore.push_back("r15");

            attribs_ignore.push_back("q0");
            attribs_ignore.push_back("q1");
            attribs_ignore.push_back("q2");
            attribs_ignore.push_back("q3");

            attribs_ignore.push_back("special_type");

            AttributeScanner::Do(items, item, eElem, attribs, attribs_ignore);
        } else if (strmtch(eElem->Value(), "NavmeshHintObject")) {
            std::vector<attribpair> attribs;

            std::vector<const char*> attribs_ignore;
            attribs_ignore.push_back("id");
            attribs_ignore.push_back("group_id");
            attribs_ignore.push_back("s0");
            attribs_ignore.push_back("s1");
            attribs_ignore.push_back("s2");

            attribs_ignore.push_back("t0");
            attribs_ignore.push_back("t1");
            attribs_ignore.push_back("t2");

            attribs_ignore.push_back("color_r");
            attribs_ignore.push_back("color_b");
            attribs_ignore.push_back("color_g");

            attribs_ignore.push_back("overbright");

            attribs_ignore.push_back("r0");
            attribs_ignore.push_back("r1");
            attribs_ignore.push_back("r2");
            attribs_ignore.push_back("r3");
            attribs_ignore.push_back("r4");
            attribs_ignore.push_back("r5");
            attribs_ignore.push_back("r6");
            attribs_ignore.push_back("r7");
            attribs_ignore.push_back("r8");
            attribs_ignore.push_back("r9");
            attribs_ignore.push_back("r10");
            attribs_ignore.push_back("r11");
            attribs_ignore.push_back("r12");
            attribs_ignore.push_back("r13");
            attribs_ignore.push_back("r14");
            attribs_ignore.push_back("r15");

            attribs_ignore.push_back("q0");
            attribs_ignore.push_back("q1");
            attribs_ignore.push_back("q2");
            attribs_ignore.push_back("q3");

            attribs_ignore.push_back("special_type");

            AttributeScanner::Do(items, item, eElem, attribs, attribs_ignore);
        } else if (strmtch(eElem->Value(), "NavmeshConnectionObject")) {
            std::vector<attribpair> attribs;

            std::vector<const char*> attribs_ignore;
            attribs_ignore.push_back("id");
            attribs_ignore.push_back("group_id");
            attribs_ignore.push_back("s0");
            attribs_ignore.push_back("s1");
            attribs_ignore.push_back("s2");

            attribs_ignore.push_back("t0");
            attribs_ignore.push_back("t1");
            attribs_ignore.push_back("t2");

            attribs_ignore.push_back("color_r");
            attribs_ignore.push_back("color_b");
            attribs_ignore.push_back("color_g");

            attribs_ignore.push_back("overbright");

            attribs_ignore.push_back("r0");
            attribs_ignore.push_back("r1");
            attribs_ignore.push_back("r2");
            attribs_ignore.push_back("r3");
            attribs_ignore.push_back("r4");
            attribs_ignore.push_back("r5");
            attribs_ignore.push_back("r6");
            attribs_ignore.push_back("r7");
            attribs_ignore.push_back("r8");
            attribs_ignore.push_back("r9");
            attribs_ignore.push_back("r10");
            attribs_ignore.push_back("r11");
            attribs_ignore.push_back("r12");
            attribs_ignore.push_back("r13");
            attribs_ignore.push_back("r14");
            attribs_ignore.push_back("r15");

            attribs_ignore.push_back("q0");
            attribs_ignore.push_back("q1");
            attribs_ignore.push_back("q2");
            attribs_ignore.push_back("q3");

            attribs_ignore.push_back("special_type");

            AttributeScanner::Do(items, item, eElem, attribs, attribs_ignore);
        } else if (strmtch(eElem->Value(), "NavmeshRegionObject")) {
            std::vector<attribpair> attribs;

            std::vector<const char*> attribs_ignore;
            attribs_ignore.push_back("id");
            attribs_ignore.push_back("group_id");
            attribs_ignore.push_back("s0");
            attribs_ignore.push_back("s1");
            attribs_ignore.push_back("s2");

            attribs_ignore.push_back("t0");
            attribs_ignore.push_back("t1");
            attribs_ignore.push_back("t2");

            attribs_ignore.push_back("color_r");
            attribs_ignore.push_back("color_b");
            attribs_ignore.push_back("color_g");

            attribs_ignore.push_back("overbright");

            attribs_ignore.push_back("r0");
            attribs_ignore.push_back("r1");
            attribs_ignore.push_back("r2");
            attribs_ignore.push_back("r3");
            attribs_ignore.push_back("r4");
            attribs_ignore.push_back("r5");
            attribs_ignore.push_back("r6");
            attribs_ignore.push_back("r7");
            attribs_ignore.push_back("r8");
            attribs_ignore.push_back("r9");
            attribs_ignore.push_back("r10");
            attribs_ignore.push_back("r11");
            attribs_ignore.push_back("r12");
            attribs_ignore.push_back("r13");
            attribs_ignore.push_back("r14");
            attribs_ignore.push_back("r15");

            attribs_ignore.push_back("q0");
            attribs_ignore.push_back("q1");
            attribs_ignore.push_back("q2");
            attribs_ignore.push_back("q3");

            attribs_ignore.push_back("special_type");

            AttributeScanner::Do(items, item, eElem, attribs, attribs_ignore);
        } else if (strmtch(eElem->Value(), "ReflectionCaptureObject")) {
            std::vector<attribpair> attribs;

            std::vector<const char*> attribs_ignore;
            attribs_ignore.push_back("id");
            attribs_ignore.push_back("group_id");
            attribs_ignore.push_back("s0");
            attribs_ignore.push_back("s1");
            attribs_ignore.push_back("s2");

            attribs_ignore.push_back("t0");
            attribs_ignore.push_back("t1");
            attribs_ignore.push_back("t2");

            attribs_ignore.push_back("color_r");
            attribs_ignore.push_back("color_b");
            attribs_ignore.push_back("color_g");

            attribs_ignore.push_back("overbright");

            attribs_ignore.push_back("r0");
            attribs_ignore.push_back("r1");
            attribs_ignore.push_back("r2");
            attribs_ignore.push_back("r3");
            attribs_ignore.push_back("r4");
            attribs_ignore.push_back("r5");
            attribs_ignore.push_back("r6");
            attribs_ignore.push_back("r7");
            attribs_ignore.push_back("r8");
            attribs_ignore.push_back("r9");
            attribs_ignore.push_back("r10");
            attribs_ignore.push_back("r11");
            attribs_ignore.push_back("r12");
            attribs_ignore.push_back("r13");
            attribs_ignore.push_back("r14");
            attribs_ignore.push_back("r15");

            attribs_ignore.push_back("q0");
            attribs_ignore.push_back("q1");
            attribs_ignore.push_back("q2");
            attribs_ignore.push_back("q3");

            attribs_ignore.push_back("special_type");

            AttributeScanner::Do(items, item, eElem, attribs, attribs_ignore);
        } else if (strmtch(eElem->Value(), "Hotspot")) {
            std::vector<attribpair> attribs;
            attribs.push_back(attribpair("type_file", "hotspot_object"));

            std::vector<const char*> attribs_ignore;
            attribs_ignore.push_back("id");
            attribs_ignore.push_back("group_id");
            attribs_ignore.push_back("s0");
            attribs_ignore.push_back("s1");
            attribs_ignore.push_back("s2");

            attribs_ignore.push_back("t0");
            attribs_ignore.push_back("t1");
            attribs_ignore.push_back("t2");

            attribs_ignore.push_back("color_r");
            attribs_ignore.push_back("color_b");
            attribs_ignore.push_back("color_g");

            attribs_ignore.push_back("overbright");

            attribs_ignore.push_back("r0");
            attribs_ignore.push_back("r1");
            attribs_ignore.push_back("r2");
            attribs_ignore.push_back("r3");
            attribs_ignore.push_back("r4");
            attribs_ignore.push_back("r5");
            attribs_ignore.push_back("r6");
            attribs_ignore.push_back("r7");
            attribs_ignore.push_back("r8");
            attribs_ignore.push_back("r9");
            attribs_ignore.push_back("r10");
            attribs_ignore.push_back("r11");
            attribs_ignore.push_back("r12");
            attribs_ignore.push_back("r13");
            attribs_ignore.push_back("r14");
            attribs_ignore.push_back("r15");

            attribs_ignore.push_back("q0");
            attribs_ignore.push_back("q1");
            attribs_ignore.push_back("q2");
            attribs_ignore.push_back("q3");

            attribs_ignore.push_back("special_type");

            AttributeScanner::Do(items, item, eElem, attribs, attribs_ignore);
        } else if (strmtch(eElem->Value(), "LightVolumeObject")) {
            std::vector<attribpair> attribs;

            std::vector<const char*> attribs_ignore;
            attribs_ignore.push_back("id");
            attribs_ignore.push_back("s0");
            attribs_ignore.push_back("s1");
            attribs_ignore.push_back("s2");

            attribs_ignore.push_back("t0");
            attribs_ignore.push_back("t1");
            attribs_ignore.push_back("t2");

            attribs_ignore.push_back("color_r");
            attribs_ignore.push_back("color_b");
            attribs_ignore.push_back("color_g");

            attribs_ignore.push_back("overbright");

            attribs_ignore.push_back("r0");
            attribs_ignore.push_back("r1");
            attribs_ignore.push_back("r2");
            attribs_ignore.push_back("r3");
            attribs_ignore.push_back("r4");
            attribs_ignore.push_back("r5");
            attribs_ignore.push_back("r6");
            attribs_ignore.push_back("r7");
            attribs_ignore.push_back("r8");
            attribs_ignore.push_back("r9");
            attribs_ignore.push_back("r10");
            attribs_ignore.push_back("r11");
            attribs_ignore.push_back("r12");
            attribs_ignore.push_back("r13");
            attribs_ignore.push_back("r14");
            attribs_ignore.push_back("r15");

            attribs_ignore.push_back("q0");
            attribs_ignore.push_back("q1");
            attribs_ignore.push_back("q2");
            attribs_ignore.push_back("q3");

            AttributeScanner::Do(items, item, eElem, attribs, attribs_ignore);
        } else if (strmtch(eElem->Value(), "parameters")) {
            std::vector<elempair> elems;
            elems.push_back(elempair("parameter", ""));

            std::vector<const char*> elems_ignore;

            ElementScanner::Do(items, item, eElem, elems, elems_ignore, this, (void*)PARAMETER);
        } else {
            LOGE << "Unknown item sub " << eElem->Value() << " " << item << std::endl;
        }

    } else if (userdata == (void*)PARAMETER) {
        std::vector<parampair> params;

        std::vector<const char*> params_ignore;
        params_ignore.push_back("Name");

        ParameterScanner::Do(items, item, eElem, params, params_ignore);
    } else {
        XMLSeekerBase::HandleElementCallback(items, eRoot, eElem, item, userdata);
    }
}
