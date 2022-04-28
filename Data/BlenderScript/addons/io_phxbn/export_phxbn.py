import array
import bpy
from bpy.props import *
from io_phxbn import phxbn_types
from mathutils import Vector, Matrix
import operator

def Get4ByteIntArray():
    var = array.array('l')
    if var.itemsize != 4:
        var = array.array('i')
    return var
    
def UnCenterArmatureInMesh(data, mesh_obj):
    mesh = mesh_obj.data
    
    min_point = array.array('f')
    max_point = array.array('f')
    
    for i in range(3):
        min_point.append(mesh.vertices[0].co[i])
        max_point.append(mesh.vertices[0].co[i])
    
    for vert in mesh.vertices:
        for i in range(3):
            min_point[i] = min(min_point[i], vert.co[i])
            max_point[i] = max(max_point[i], vert.co[i])
        
    mid_point = Vector();
    for i in range(3):
        mid_point[i] = ((min_point[i] + max_point[i]) * 0.5)
    
    mid_point = mesh_obj.matrix_world * mid_point;
    
    #print("Midpoint: ",mid_point)
    
    for vert in data.vertices:
        for i in range(3):
            vert[i] = vert[i] - mid_point[i]

def to_array(array, idarray):
    for item in idarray:
        array.append(item)

class BoneWeight:
    def __init__(self, id, weight):
        self.id = id
        self.weight = weight
 
def AddConnectingBones(arm_obj):
    new_bones = [] 
    arm_data = arm_obj.data 
    for bone in arm_data.edit_bones:
        if bone.name == "root" or bone.layers[29] == False or not bone.parent or bone.parent.name == "root" or bone.parent.layers[29] == False:
            continue
        if bone.head != bone.parent.tail:
            bpy.ops.armature.bone_primitive_add(name="connector")
            new_bone = arm_data.edit_bones[-1]
            new_bone.head = bone.parent.tail
            new_bone.tail = bone.head
            new_bone.parent = bone.parent
            new_bone["Mass"] = 0.01
            for i in range(32):
                new_bone.layers[i] = False
            new_bone.layers[29] = True
            bone.parent = new_bone
            new_bones.append(new_bone.name) 
    
    bpy.context.scene.update()
    bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.object.mode_set(mode='EDIT')
 
def EnforceBoneNaming(arm_data):
    name_shift = {}
    bone_labels = {}
    for bone in arm_data.edit_bones:
        if bone.name == "root" or bone.layers[29] == False:
            continue
        if bone.name.split("_")[0] == "Bone":
            bone_id = int(bone.name.split("_")[-1])
            bone_labels[bone_id] = True
            name_shift[bone.name] = bone.name
    
    index = 0
    for bone in arm_data.edit_bones:
        if bone.name == "root" or bone.layers[29] == False:
            continue
        if bone.name.split("_")[0] != "Bone":
            while index in bone_labels:
                index = index + 1
            name_shift[bone.name] = "Bone_"+str(index)
            bone.name = name_shift[bone.name]
            index = index+1
    return name_shift

            
def GetClosestRealChildren(bone):
    list = []
    for child in bone.children:
        if not child.layers[29]:
            new_list = GetClosestRealChildren(child)
            for line in new_list:
                list.append(line)
            continue
        list.append(child)
    return list
    
class Point():
    def __init__(self):
        self.head_of = []
        self.tail_of = []
        self.id = -1
        
bone_dict = {}

#Create a dictionary like this: bone_dict["Bone_0"] = {"Point IDs",[point, point]}
def CreateBonePointDictionary(bones):
    point_head = {}
    point_tail = {}
    
    first_point = Point()
    first_point.head_of = GetClosestRealChildren(bones["root"])
        
    points = [first_point]
    
    to_process = GetClosestRealChildren(bones["root"])
        
    while len(to_process) > 0:
        bone = to_process.pop(0)
        point = Point()
        point.tail_of.append(bone)
        new_list = GetClosestRealChildren(bone)
        for line in new_list:
            point.head_of.append(line)
            to_process.append(line)
        points.append(point)
                       
    for point in points:
        for bone in point.head_of:
            point_head[bone] = point
        for bone in point.tail_of:
            point_tail[bone] = point
            
    index = 0
    for point in points:
        if point.id != -1:
            continue
        point.id = index
        index += 1
    
    for bone in bones:
        if not bone in point_head or not bone in point_tail:
            continue
        point_ids = Get4ByteIntArray()
        point_ids.append(point_head[bone].id)
        point_ids.append(point_tail[bone].id)
        if not bone.name in bone_dict:
            bone_dict[bone.name] = {}
            #print("Added ", bone.name)
        bone_dict[bone.name]["Point IDs"] = point_ids
    
    #print("Created bone dict")
    #print(bone_dict)
        
def GetParentIDs(data):
    parent_ids = Get4ByteIntArray()
    for bone in data.edit_bones:
        if bone.name.split("_")[0] != "Bone":
            continue
        bone_id = int(bone.name.split("_")[-1])
        while bone_id >= len(parent_ids):
            parent_ids.append(-1)
        if not bone.parent or bone.parent.name == "root" or bone.parent.layers[29] == False:
            continue
        parent_bone_id = int(bone.parent.name.split("_")[-1])
        parent_ids[bone_id] = parent_bone_id
    
    data["parent_ids"] = parent_ids

def GetParents(data):
    parents = Get4ByteIntArray()
    for bone in data.edit_bones:
        if not bone_dict.get(bone.name) or not bone_dict[bone.name].get("Point IDs"):
            continue    
        point_ids = bone_dict[bone.name]["Point IDs"]
        while len(parents) <= point_ids[1]:
            parents.append(-1)
        parents[point_ids[1]] = point_ids[0]
    data["parents"] = parents
    
def GetBoneMatZ(edit_bone):
    z_vec = (edit_bone.tail - edit_bone.head).normalize()
    x_vec = Vector((0,0,1))
    y_vec = x_vec.cross(z_vec).normalize()
    x_vec = y_vec.cross(z_vec).normalize()

    return [x_vec[0], x_vec[2], -x_vec[1], 0,
            y_vec[0], y_vec[2], -y_vec[1], 0,
            z_vec[0], z_vec[2], -z_vec[1], 0,
            0,0,0,1]


def GetBoneMat(edit_bone):
    z_vec = (edit_bone.tail - edit_bone.head).normalize()
    y_vec = Vector((0,1,0))
    if abs(z_vec.dot(y_vec)) > 0.95:
        return GetBoneMatZ(edit_bone)
    x_vec = z_vec.cross(y_vec).normalize()
    y_vec = z_vec.cross(x_vec).normalize()

    return [x_vec[0], x_vec[2], -x_vec[1], 0,
            y_vec[0], y_vec[2], -y_vec[1], 0,
            z_vec[0], z_vec[2], -z_vec[1], 0,
            0,0,0,1]
    
def GetJoints(arm_obj, name_shift):
    joints = []
    for bone in arm_obj.pose.bones:
        for constraint in bone.constraints:
            name_parts = constraint.name.split("_") 
            if constraint.type == 'LIMIT_ROTATION' and constraint.owner_space == 'LOCAL' and name_parts[0] == "RGDL":
                freedom = 0
                if constraint.min_x != constraint.max_x:
                    freedom += 1
                if constraint.min_y != constraint.max_y:
                    freedom += 1
                if constraint.min_z != constraint.max_z:
                    freedom += 1
                    
                joint = phxbn_types.Joint()
                joint.bone_ids.append(int(bone.name.split("_")[1]))
                if bone.parent and len(bone.parent.name.split("_")) > 1:
                    joint.bone_ids.append(int(bone.parent.name.split("_")[1]))
                else:
                    joint.bone_ids.append(0)
                   
                if name_parts[1] != "Limit Rotation": 
                    #print("Trying to find: "+name_parts[1]) 
                    if name_shift.get(name_parts[1]) and \
                    arm_obj.pose.bones.get(name_shift[name_parts[1]]): 
                        #print("Constraint name: "+name_parts[1]) 
                        #print("Constraint name shift: "+name_shift[name_parts[1]]) 
                        #print("Constraint name suffix: "+name_shift[name_parts[1]].split("_")[1]) 
                        joint.bone_ids[1] = int(name_shift[name_parts[1]].split("_")[1]) 
                    else : 
                        print("Could not find: "+name_parts[1]) 

                if freedom == 0:
                    joint.type.append(phxbn_types._fixed_joint)
                    joints.append(joint)
                if freedom == 1:
                    joint.type.append(phxbn_types._hinge_joint)
                    mat = arm_obj.matrix_world * bone.matrix
                    axis = Vector((mat[0][0], mat[0][1], mat[0][2])).normalize()
                    joint.axis.append(axis[0])
                    joint.axis.append(axis[2])
                    joint.axis.append(-axis[1])
                    joint.stop_angles.append(constraint.min_x)
                    joint.stop_angles.append(constraint.max_x)
                    joints.append(joint)
                if freedom > 1:
                    joint.type.append(phxbn_types._amotor_joint)
                    joint.stop_angles.append(constraint.min_y)
                    joint.stop_angles.append(constraint.max_y)
                    joint.stop_angles.append(constraint.min_x)
                    joint.stop_angles.append(constraint.max_x)
                    joint.stop_angles.append(constraint.min_z)
                    joint.stop_angles.append(constraint.max_z)
                    joints.append(joint)
   
    return joints
    '''if not arm_data.get("num_special_joints"):
        arm_data["num_special_joints"] = 0
    num_joints = arm_data["num_special_joints"]
    for num in range(num_joints):
        joint = phxbn_types.Joint()
        name = "joint_" + str(num)
        joint.type.append(arm_data[name + "_type"])
        if joint.type[0] == phxbn_types._amotor_joint:
            to_array(joint.stop_angles, arm_data[name + "_angles"])
        elif joint.type[0] == phxbn_types._hinge_joint:
            to_array(joint.stop_angles, arm_data[name + "_angles"])
        to_array(joint.bone_ids, arm_data[name + "_bone_ids"])
        if joint.type[0] == phxbn_types._hinge_joint:
            to_array(joint.axis, arm_data[name+"_axis"])
        data.joints.append(joint)'''
    
def GetIKBones(arm_obj, name_shift):
    ik_bones = []
    for bone in arm_obj.pose.bones:
        if arm_obj.data.edit_bones[bone.name].layers[29] == False:
            continue
        for constraint in bone.constraints:
            if constraint.type == 'IK' and not constraint.target:
                bone_id = int(bone.name.split("_")[-1])
                ik_length = constraint.chain_count
                #print("IK bone "+constraint.name+": bone_"+str(bone_id)+" with chain count: "+str(ik_length))
                ik_bone = phxbn_types.IKBone()
                ik_bone.name = constraint.name
                ik_bone.bone = bone_id
                ik_bone.chain = ik_length
                ik_bones.append(ik_bone)
    return ik_bones
    
def PHXBNFromArmature():
    scene = bpy.context.scene
    old_armature_object = scene.objects.active
    
    if not old_armature_object or old_armature_object.type != 'ARMATURE':
        print ("Must select armature before exporting PHXBN")
        return
        
    if len(old_armature_object.children) == 0 or old_armature_object.children[0].type != 'MESH':
        print("Armature must be the parent of a mesh.")
        return
      
    #Make a copy of the object so we don't mess up the original
    bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.object.duplicate()
    arm_obj = scene.objects.active
    print ("Reading PHXBN from armature: ", arm_obj.name)
    
    #Set to edit mode so we can add connecting bones
    bpy.ops.object.mode_set(mode='EDIT')
    AddConnectingBones(arm_obj) 
    
    #Extract metadata from armature object
    arm_data = arm_obj.data
    data = phxbn_types.PHXBNdata()
    
    if arm_data.get("version"):
        data.version.append(arm_data["version"])
    else:
        data.version.append(11)
        
    if arm_data.get("rigging_stage"):
        data.rigging_stage.append(arm_data["rigging_stage"])
    else:
        data.rigging_stage.append(1)

    #Rename all bones to "root" or "Bone_X" where X is bone number
    name_shift = EnforceBoneNaming(arm_data)
        
    #Make an easy way to look up the points attached to each bone
    CreateBonePointDictionary(arm_data.edit_bones)
        
    #Count the number of points and bones
    num_points = 0
    num_bones = 0
    for bone in arm_data.edit_bones:
        if bone.name == "root" or bone.layers[29] == False:
            continue
        points = Get4ByteIntArray()
        to_array(points, bone_dict[bone.name]["Point IDs"])
        num_points = max(num_points, points[0]+1)
        num_points = max(num_points, points[1]+1)
        num_bones = num_bones + 1
       
    #Create storage space for bone information       
    for i in range(num_bones):
        points = Get4ByteIntArray()
        data.bones.append(points)
        data.bone_mass.append(0)
        data.bone_com.append(0)
        data.bone_com.append(0)
        data.bone_com.append(0)
        data.bone_parents.append(-1)
        data.bone_swap.append(0)
        for j in range(16):
            data.bone_mat.append(0)
    
    #Create space for point spatial information
    vertices = array.array('f')
    for i in range(num_points*3):
        vertices.append(0)
      
    #Fill in the bone information
    num = 0
    for bone in arm_data.edit_bones:
        if bone.name == "root" or bone.layers[29] == False:
                continue
        points = Get4ByteIntArray()
        to_array(points, bone_dict[bone.name]["Point IDs"])
        bone_id = int(bone.name.split('_')[-1])
        data.bones[bone_id].append(points[0])
        data.bones[bone_id].append(points[1])
        vertices[points[0]*3+0] = bone.head[0]
        vertices[points[0]*3+1] = bone.head[1]
        vertices[points[0]*3+2] = bone.head[2]
        vertices[points[1]*3+0] = bone.tail[0]
        vertices[points[1]*3+1] = bone.tail[1]
        vertices[points[1]*3+2] = bone.tail[2]
        mass = 0.1
        if bone.get("Mass"):
            mass = bone["Mass"]
        data.bone_mass[bone_id] = mass
        COM = [0.0,0.0,0.0]
        if bone.get("COM"):
            COM = bone["COM"]
        data.bone_com[bone_id*3+0] = COM[0]
        data.bone_com[bone_id*3+1] = COM[1]
        data.bone_com[bone_id*3+2] = COM[2]
        mat = GetBoneMat(bone)
        if bone.get("mat"):
            mat = bone["mat"]
        for j in range(16):
            data.bone_mat[bone_id*16+j] = mat[j]
        swap = False
        if bone.get("Swap"):
            swap = bone["Swap"]
        data.bone_swap[bone_id] = swap
        if(data.bone_swap[bone_id]):
            data.bones[bone_id][0], data.bones[bone_id][1] = \
                data.bones[bone_id][1], data.bones[bone_id][0]
        if not bone.parent or bone.parent.name == "root" or bone.parent.layers[29] == False:
            data.bone_parents[bone_id] = -1
        else:
            parent = int(bone.parent.name.split('_')[-1])
            data.bone_parents[bone_id] = parent
        num = num + 1
    
    #Store the position of each point
    for i in range(num_points):
        vertex = array.array('f')
        vertex.append(vertices[i*3+0])
        vertex.append(vertices[i*3+1])
        vertex.append(vertices[i*3+2])
        data.vertices.append(vertex)
    
    #Extract joint and IK bone info
    data.joints = GetJoints(arm_obj, name_shift)
    data.ik_bones = GetIKBones(arm_obj, name_shift)
    
    #Get the mesh object and subtract its midpoint from each skeleton point position
    mesh_obj = old_armature_object.children[0]
    print("Uncentering armature in mesh")
    UnCenterArmatureInMesh(data, mesh_obj)
    
    #Convert Blender coordinates back to Phoenix coordinates
    for vertex in data.vertices:
        vertex[1], vertex[2] = vertex[2], -vertex[1]
        
    #Calculate the parent hierarchies for each bone and point
    if not arm_data.get("parent_ids") or len(arm_data["parent_ids"]) != num_bones:
        GetParentIDs(arm_data)
    to_array(data.parent_ids, arm_data["parent_ids"])
    if not arm_data.get("parents") or len(arm_data["parents"]) != num_points:
       GetParents(arm_data)
    to_array(data.parents, arm_data["parents"])
    
    #Extract the bone ids and weights for each mesh vertex
    mesh = mesh_obj.data
    num_verts = len(mesh.vertices)
    bone_ids = []
    bone_weights = []
    count = 0;
    for vert in mesh.vertices:
        new_bone_weights = []
        #For each vertex group, check if that bone is a real deformation bone.
        #If so, record bone id and weight for this vertex
        for i in range(len(vert.groups)):
            temp_name = mesh_obj.vertex_groups[vert.groups[i].group].name
            if not temp_name in name_shift:
                continue
            group_name = name_shift[temp_name]
            new_bone_weights.append(BoneWeight(int(group_name.split('_')[-1]), \
                                               vert.groups[i].weight))
        #Sort bone weights, and keep the four greatest
        new_bone_weights.sort(key=operator.attrgetter('weight'), reverse=True)
        new_bone_weights = new_bone_weights[0:4]
        while len(new_bone_weights)<4:
            new_bone_weights.append(BoneWeight(0,0.0))
        #Normalize the bone weights
        total = 0.0
        for i in range(4):
            total = total + new_bone_weights[i].weight
        for i in range(4):
            if total != 0:
                new_bone_weights[i].weight = new_bone_weights[i].weight / total
            else:
                print("Error: vertex ",count," has no weights!")
        #Record weights and id in list
        for bone_weight in new_bone_weights:
            bone_ids.append(bone_weight.id)
            bone_weights.append(bone_weight.weight)
        #if count > 100 and count < 130:
        #    print("Vertex ",count,": ",new_bone_weights[0].weight,", ",new_bone_weights[1].weight,", ",new_bone_weights[2].weight,", ",new_bone_weights[3].weight)
        count = count + 1
    
    #Store the vertex weights in the form of an array of face vertices
    for face in mesh.faces:
        for j in range(3):
            for i in range(4):
                index = face.vertices[j]*4+i
                data.bone_ids.append(bone_ids[index])
                data.bone_weights.append(bone_weights[index])
    
    print("Num faces: ", len(mesh.faces))
    print("Num bone weights: ", len(data.bone_weights))
    
    bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.object.delete()
    scene.objects.active = old_armature_object
            
    return data
    
def WriteInt(file, val):
    int_loader = Get4ByteIntArray()
    int_loader.append(val)
    int_loader.tofile(file)
 
def WritePHXBN(file_path, data):
    file = open(file_path, 'wb')    
    
    print ("Saving phxbn file: ", file_path)
    data.version.tofile(file)
    print ('Version: ', data.version[0])
    
    data.rigging_stage.tofile(file)
    print ('Rigging stage: ', data.rigging_stage[0])
        
    num_points = Get4ByteIntArray()
    num_points.append(len(data.vertices))
    num_points.tofile(file)
    print ('Num points: ', num_points[0])
    
    for vertex in data.vertices:
        #print(vertex)
        vertex.tofile(file)
    
    data.parents.tofile(file)
    
    num_bones = Get4ByteIntArray()
    num_bones.append(len(data.bones))
    num_bones.tofile(file)
    print ('Num bones: ', num_bones[0])
    
    for bone in data.bones:
        bone.tofile(file)
        
    data.bone_parents.tofile(file)

    data.bone_mass.tofile(file)
    data.bone_com.tofile(file)
    data.bone_mat.tofile(file)
    
    num_vertices = Get4ByteIntArray()
    num_vertices.append(len(data.bone_weights)//4)
    num_vertices.tofile(file)
    
    data.bone_weights.tofile(file)
    data.bone_ids.tofile(file)
    
    data.parent_ids.tofile(file)
    
    num_joints = Get4ByteIntArray()
    num_joints.append(len(data.joints))
    num_joints.tofile(file)
    
    print("Saving ", num_joints[0], " joints")
    
    for joint in data.joints:
        joint.type.tofile(file)
        if joint.type[0] == phxbn_types._amotor_joint:
            joint.stop_angles.tofile(file)
        elif joint.type[0] == phxbn_types._hinge_joint:
            joint.stop_angles.tofile(file)
        joint.bone_ids.tofile(file)
        if joint.type[0] == phxbn_types._hinge_joint:
            joint.axis.tofile(file)

    num_ik_bones = len(data.ik_bones)
    WriteInt(file, num_ik_bones)
    for ik_bone in data.ik_bones:
        WriteInt(file, ik_bone.bone)
        WriteInt(file, ik_bone.chain)
        string_array = array.array("B")
        string_array.fromstring(ik_bone.name.encode("utf-8"))
        WriteInt(file, len(string_array))
        string_array.tofile(file)
        
    file.close()
    
def Save(filepath):
    data = PHXBNFromArmature()
    if not data:
        return
    WritePHXBN(filepath, data)
    
class PHXBNExporter(bpy.types.Operator):
    '''Save Phoenix Bones armature'''
    bl_idname = "export_mesh.phxbn"
    bl_label = "Export PHXBN"

    filepath = StringProperty(name="File Path", description="Filepath used for exporting the PHXBN file", maxlen= 1024, default= "")
    
    check_existing = BoolProperty(name="Check Existing", description="Check and warn on overwriting existing files", default=True, options={'HIDDEN'})

    def execute(self, context):
        Save(self.properties.filepath)
        return {'FINISHED'}

    def invoke(self, context, event):
        wm = context.window_manager
        wm.add_fileselect(self)
        return {'RUNNING_MODAL'}

#Save("C:\\Users\\David\\Desktop\\export.phxbn")
