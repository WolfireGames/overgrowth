import bpy
from xml.etree import cElementTree as ET

print("\nRunning character loader:")

working_dir = "C:/Users/David/Desktop/WolfireSVN/"

actor_xml_path = working_dir + 'Data/Objects/IGF_Characters/IGF_GuardActor.xml'
print("Loading actor file: "+actor_xml_path)
xml_root = ET.parse(actor_xml_path).getroot()
 
character_xml_path = None
for element in xml_root:
    if element.tag == "Character":
        character_xml_path = element.text

object_xml_path = None
skeleton_xml_path = None
if character_xml_path:
    print("Loading character file: "+working_dir+character_xml_path)
    xml_root = ET.parse(working_dir+character_xml_path).getroot()
    for element in xml_root:
        if(element.tag == "appearance"):
            object_xml_path = element.get("obj_path")
            skeleton_xml_path = element.get("skeleton")

model_path = None
color_path = None
normal_path = None
palette_map_path = None
shader_name = None
if object_xml_path:
    print("Loading object file: "+working_dir+object_xml_path)
    xml_root = ET.parse(working_dir+object_xml_path).getroot()
    for element in xml_root:
        if(element.tag == "Model"):
            model_path = element.text
        if(element.tag == "ColorMap"):
            color_path = element.text
        if(element.tag == "NormalMap"):
            normal_path = element.text
        if(element.tag == "PaletteMap"):
            palette_map_path = element.text
        if(element.tag == "ShaderName"):
            shader_name = element.text
            
bone_path = None
if skeleton_xml_path:
    print("Loading skeleton file: "+working_dir+skeleton_xml_path)
    xml_root = ET.parse(working_dir+skeleton_xml_path).getroot()
    print(xml_root)
    model_path = xml_root.get("model_path")
    bone_path = xml_root.get("bone_path")

'''if model_path:
    print("Model path: "+working_dir+model_path)
    bpy.ops.import_scene.obj(filepath=(working_dir+model_path))
'''

_min_skeleton_version = 6

import struct
if bone_path:
    print("Bone path: "+working_dir+bone_path)
    with open(working_dir+bone_path, mode='rb') as file:
        contents = file.read()
        file.close()
        cursor = 0;
        temp_read = struct.unpack("i", contents[cursor:cursor+4])
        cursor += 4
        version = 5
        if temp_read[0] >= _min_skeleton_version:
            version = temp_read[0];
            temp_read = struct.unpack("i", contents[cursor:cursor+4])
            cursor += 4   
        print("Version: "+str(version))
        print("Rigging stage: "+str(temp_read[0]))
        num_points = struct.unpack("i", contents[cursor:cursor+4])[0]
        cursor += 4
        print("Num points: "+str(num_points))
        points = []
        for i in range(0, num_points):
            points += struct.unpack("fff", contents[cursor:cursor+12])
            cursor += 12

        point_parents = []
        if version >= 8:
            for i in range(0, num_points):
                point_parents += struct.unpack("i", contents[cursor:cursor+4])
                cursor += 4

        num_bones = struct.unpack("i", contents[cursor:cursor+4])[0]
        cursor += 4
        print("Num bones: "+str(num_bones))
        bone_ends = []
        bone_mats = []
        for i in range(0, num_bones):
            bone_ends += struct.unpack("ii", contents[cursor:cursor+8])
            cursor += 8

        print(bone_ends)