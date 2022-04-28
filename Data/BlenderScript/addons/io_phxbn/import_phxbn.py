import bpy
from bpy.props import *
import array
import mathutils
import math
from mathutils import Vector
from io_phxbn import phxbn_types
   
def Get4ByteIntArray():
    var = array.array('l')
    if var.itemsize != 4:
        var = array.array('i')
    return var
   
def WriteToTextFile(file_path, data):
    file = open(file_path, 'w')    
    file.write('Version: '+str(data.version[0])+'\n')
    file.write('Rigging stage: '+str(data.rigging_stage[0])+'\n')
    file.write('Num points: '+str(len(data.vertices))+'\n')
    
    file.write('\nVertices:\n')
    for vertex in data.vertices:
        file.write(str(vertex)+'\n')
    
    file.write('\nParents:\n')
    for parent in data.parents:
        file.write(str(parent)+'\n')
        
    file.write('\nNum bones: '+str(len(data.bones))+'\n')
    
    file.write('\nBones:\n')
    for bone in data.bones:
        file.write(str(bone)+'\n')
        
    file.write('\nBone parents:\n')
    for bone_parent in data.bone_parents:
        file.write(str(bone_parent)+'\n')
    
    file.write('\nBone mass:\n')
    for bone_mass in data.bone_mass:
        file.write(str(bone_mass)+'\n')

    file.write('\nBone COM:\n')
    for bone_com in data.bone_com:
        file.write(str(bone_com)+'\n')
    
    file.write('Parent IDs:\n')
    for parent_id in data.parent_ids:
        file.write(str(parent_id)+'\n')
        
    file.write('\nNum joints: '+str(len(data.joints))+'\n')
    
    i = 0
    for joint in data.joints:
        file.write('\nJoint '+str(i)+':\n')
        file.write('Joint type: '+str(joint.type)+'\n')
        file.write('Stop angles: '+str(joint.stop_angles)+'\n')
        file.write('Bone ids: '+str(joint.bone_ids)+'\n')
        file.write('Axis: '+str(joint.axis)+'\n')
        i = i + 1
        
    file.close()
   
def ReadPHXBN(file_path, mesh_obj):
    # File loading stuff
    # Open the file for importing
    file = open(file_path, 'rb')    
    
    data = phxbn_types.PHXBNdata();
    
    print ("\nLoading phxbn file: ", file_path)
    data.version.fromfile(file, 1)
    print ('Version: ', data.version[0], '\n')
    
    if data.version[0] < 8:
        print('PHXBN must be at least version 8.')
        return
    
    data.rigging_stage.fromfile(file, 1)
    print ('Rigging stage: ', data.rigging_stage[0], '\n')
   
    if data.rigging_stage[0] != 1:
        print('Rigging stage should be 1')
        return
        
    num_points = Get4ByteIntArray()
    num_points.fromfile(file, 1)
    print ('Num points: ', num_points[0], '\n')
    
    for i in range(0,num_points[0]):
        vertex = array.array('f')
        vertex.fromfile(file, 3)
        #print(vertex)
        #Convert Phoenix coordinates to Blender coordinates
        vertex[1], vertex[2] = -vertex[2], vertex[1]
        data.vertices.append(vertex)
    
    for i in range(0,num_points[0]):
        parent = Get4ByteIntArray()
        parent.fromfile(file, 1)
        data.parents.append(parent[0])
    
    num_bones = Get4ByteIntArray()
    num_bones.fromfile(file, 1)
    print ('Num bones: ', num_bones[0], '\n')
    
    for i in range(0,num_bones[0]):
        bone = Get4ByteIntArray()
        bone.fromfile(file, 2)
        if data.parents[bone[0]]==bone[1]:
            temp = bone[0]
            bone[0] = bone[1]
            bone[1] = temp
            data.bone_swap.append(1)
        else: 
            data.bone_swap.append(0)
        data.bones.append(bone)
        
    for i in range(0,num_bones[0]):
        bone_parent = Get4ByteIntArray()
        bone_parent.fromfile(file, 1)
        data.bone_parents.append(bone_parent[0])
    
    data.bone_mass.fromfile(file, num_bones[0])
    data.bone_com.fromfile(file, num_bones[0]*3)
    data.bone_mat.fromfile(file, num_bones[0]*16)
    
    mesh = mesh_obj.data
    
    num_faces = len(mesh.faces)
    num_verts = num_faces * 3
    #print ("Loading ", num_verts, " bone weights and ids")
    file_bone_weights = array.array('f')
    file_bone_ids = array.array('f')
    file_bone_weights.fromfile(file, num_verts*4)
    file_bone_ids.fromfile(file, num_verts*4)
    
    num_verts = len(mesh.vertices)
    data.bone_weights = [0 for i in range(num_verts*4)]
    data.bone_ids = [0 for i in range(num_verts*4)]
    for face_id in range(num_faces):
        for face_vert_num in range(3):
            for i in range(4):
                data.bone_weights[mesh.faces[face_id].vertices[face_vert_num]*4+i] =file_bone_weights[face_id*12 + face_vert_num * 4 + i]
                data.bone_ids[mesh.faces[face_id].vertices[face_vert_num]*4+i] =file_bone_ids[face_id*12 + face_vert_num * 4 + i]
     
    data.parent_ids.fromfile(file, num_bones[0])
        
    num_joints = Get4ByteIntArray()
    num_joints.fromfile(file, 1)
    
    #print("Loading ", num_joints, " joints")
    
    for i in range(num_joints[0]):
        joint = phxbn_types.Joint()
        joint.type.fromfile(file, 1)
        if joint.type[0] == phxbn_types._amotor_joint:
            joint.stop_angles.fromfile(file, 6)
        elif joint.type[0] == phxbn_types._hinge_joint:
            joint.stop_angles.fromfile(file, 2)
        joint.bone_ids.fromfile(file, 2)
        if joint.type[0] == phxbn_types._hinge_joint:
            joint.axis.fromfile(file, 3)
        data.joints.append(joint)

    file.close()
    
    #WriteToTextFile(file_path+"_text.txt", data)
    return data

def GetMeshObj():
     for obj in bpy.context.selected_objects:
        if obj.type == 'MESH':
           return obj
                
def GetMeshMidpoint(data, mesh_obj):
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
        
    mid_point = array.array('f')
    for i in range(3):
        mid_point.append((min_point[i] + max_point[i]) * 0.5)
    
    return mid_point
    #for vert in data.vertices:
    #    for i in range(3):
    #        vert[i] = vert[i] + mid_point[i]

def GetD6Mat(edit_bone):
    vec = (edit_bone.tail - edit_bone.head).normalize()
    vec = Vector((vec[0], vec[2], -vec[1]))
    right = Vector((0.0001,1.0001,0.000003))
    up = vec.cross(right).normalize()
    right = up.cross(vec).normalize()

    return [right[0], right[1], right[2], 0,
            up[0], up[1], up[2], 0,
            vec[0],vec[1], vec[2], 0,
            0,0,0,1]
    
def AddFixedJoint(arm_obj,joint):
    joint_bones = []
    joint_bones.append(arm_obj.pose.bones["Bone_"+str(joint.bone_ids[0])])
    joint_bones.append(arm_obj.pose.bones["Bone_"+str(joint.bone_ids[1])])
    
    if joint_bones[0].parent == joint_bones[1]:
        child = joint_bones[0]
        parented = True
    elif joint_bones[1].parent == joint_bones[0]:
        child = joint_bones[1]
        parented = True
    else:
        child = joint_bones[1]
        parented = False
        
    constraint = child.constraints.new('LIMIT_ROTATION')
    constraint.use_limit_x = True
    constraint.use_limit_y = True
    constraint.use_limit_z = True
    constraint.influence = 0.0
    constraint.owner_space = 'LOCAL'
    
    if parented == False:
        constraint.name = joint_bones[0].name
    
def AddAmotorJoint(arm_obj,joint):
    joint_bones = []
    joint_bones.append(arm_obj.pose.bones["Bone_"+str(joint.bone_ids[0])])
    joint_bones.append(arm_obj.pose.bones["Bone_"+str(joint.bone_ids[1])])
    
    if joint_bones[0].parent == joint_bones[1]:
        child = joint_bones[0]
        parented = True
    elif joint_bones[1].parent == joint_bones[0]:
        child = joint_bones[1]
        parented = True
    else:
        child = joint_bones[1]
        parented = False
       
    constraint = child.constraints.new('LIMIT_ROTATION')
    constraint.min_x = joint.stop_angles[2]
    constraint.max_x = joint.stop_angles[3]
    constraint.min_y = joint.stop_angles[0]
    constraint.max_y = joint.stop_angles[1]
    constraint.min_z = joint.stop_angles[4]
    constraint.max_z = joint.stop_angles[5]
    constraint.use_limit_x = True
    constraint.use_limit_y = True
    constraint.use_limit_z = True
    constraint.influence = 0.0
    constraint.owner_space = 'LOCAL'
    
    if parented == False:
        constraint.name = joint_bones[0].name
    
    matrix = GetD6Mat(arm_obj.data.edit_bones[child.name])
    joint_axis = Vector((matrix[0], -matrix[2], matrix[1]))
    
    mat = arm_obj.matrix_world * child.matrix
    x_axis = Vector((mat[0][0], mat[0][1], mat[0][2])).normalize()
    y_axis = Vector((mat[2][0], mat[2][1], mat[2][2])).normalize()
    joint_conv = Vector((-x_axis.dot(joint_axis), -y_axis.dot(joint_axis), 0)).normalize()
    roll = math.atan2(joint_conv[0], joint_conv[1])
    arm_obj.data.edit_bones[child.name].roll += roll
    
    child.rotation_mode = 'YXZ'
    
    
    '''mat = arm_obj.matrix_world * child.matrix
    bpy.ops.object.add(type='EMPTY', location=(mat[3][0], mat[3][1], mat[3][2]))
    ob = bpy.context.scene.objects.active
    ob.empty_draw_type = 'ARROWS'
    ob.empty_draw_size = 0.1
    
    mat = arm_obj.matrix_world * child.matrix
    bpy.ops.object.add(type='EMPTY', location=(mat[3][0] + y_axis[0]*0.1, mat[3][1] + y_axis[1]*0.1, mat[3][2] + y_axis[2]*0.1))
    ob = bpy.context.scene.objects.active
    ob.empty_draw_type = 'ARROWS'
    ob.empty_draw_size = 0.1
    
    bpy.context.scene.objects.active = arm_obj
    bpy.ops.object.mode_set(mode='EDIT')'''
    
def AddHingeJoint(arm_obj,joint):
    joint_bones = []
    joint_bones.append(arm_obj.pose.bones["Bone_"+str(joint.bone_ids[0])])
    joint_bones.append(arm_obj.pose.bones["Bone_"+str(joint.bone_ids[1])])
    joint_axis = Vector((joint.axis[0], -joint.axis[2], joint.axis[1]))
    
    if joint_bones[0].parent == joint_bones[1]:
        child = joint_bones[0]
        joint_axis *= -1
        parented = True
    elif joint_bones[1].parent == joint_bones[0]:
        child = joint_bones[1]
        parented = True
    else:
        child = joint_bones[1]
        parented = False
        
    constraint = child.constraints.new('LIMIT_ROTATION')
    constraint.min_x = joint.stop_angles[0]
    constraint.max_x = joint.stop_angles[1]
    constraint.use_limit_x = True
    constraint.use_limit_y = True
    constraint.use_limit_z = True
    constraint.influence = 0.0
    constraint.owner_space = 'LOCAL'
    
    if parented == False:
        constraint.name = joint_bones[0].name
    
    mat = arm_obj.matrix_world * child.matrix
    x_axis = Vector((mat[0][0], mat[0][1], mat[0][2])).normalize()
    y_axis = Vector((mat[2][0], mat[2][1], mat[2][2])).normalize()
    joint_conv = Vector((x_axis.dot(joint_axis), y_axis.dot(joint_axis), 0)).normalize()
    roll = math.atan2(joint_conv[0], joint_conv[1]) + math.pi / 2
    arm_obj.data.edit_bones[child.name].roll += roll
    
    child.rotation_mode = 'XYZ'
    
    '''mat = arm_obj.matrix_world * child.matrix
    bpy.ops.object.add(type='EMPTY', location=(mat[3][0], mat[3][1], mat[3][2]))
    ob = bpy.context.scene.objects.active
    ob.empty_draw_type = 'ARROWS'
    ob.empty_draw_size = 0.1'''
    
    '''mat = arm_obj.matrix_world * child.matrix
    bpy.ops.object.add(type='EMPTY', location=(mat[3][0] + y_axis[0]*0.1, mat[3][1] + y_axis[1]*0.1, mat[3][2] + y_axis[2]*0.1))
    ob = bpy.context.scene.objects.active
    ob.empty_draw_type = 'ARROWS'
    ob.empty_draw_size = 0.1
    
    mat = arm_obj.matrix_world * child.matrix
    x_axis = Vector((mat[0][0], mat[0][1], mat[0][2])) * 0.1
    bpy.ops.object.add(type='EMPTY', location=(mat[3][0]+x_axis[0], mat[3][1]+x_axis[1], mat[3][2]+x_axis[2]))
    ob = bpy.context.scene.objects.active
    ob.empty_draw_type = 'ARROWS'
    ob.empty_draw_size = 0.1'''
    
    '''mat = arm_obj.matrix_world * child.matrix
    x_axis = Vector((joint.axis[0], -joint.axis[2], joint.axis[1])) * 0.1
    bpy.ops.object.add(type='EMPTY', location=(mat[3][0]+x_axis[0], mat[3][1]+x_axis[1], mat[3][2]+x_axis[2]))
    ob = bpy.context.scene.objects.active
    ob.empty_draw_type = 'ARROWS'
    ob.empty_draw_size = 0.1
    
    bpy.context.scene.objects.active = arm_obj
    bpy.ops.object.mode_set(mode='EDIT')'''
    
def AddArmature(data, mesh_obj):
    scn = bpy.context.scene
    for ob in scn.objects:
        ob.select = False
    
    mid_point = GetMeshMidpoint(data, mesh_obj)
    
    arm_data = bpy.data.armatures.new("MyPHXBN")
    arm_data.use_auto_ik = True
    arm_obj = bpy.data.objects.new("MyPHXBN",arm_data)
    scn.objects.link(arm_obj)
    arm_obj.select = True
    arm_obj.location = mid_point
    scn.objects.active = arm_obj
    
    arm_data["version"] = data.version[0]
    arm_data["rigging_stage"] = data.rigging_stage[0]
    
    bpy.ops.object.mode_set(mode='EDIT')

    bpy.ops.armature.delete()
    bpy.ops.armature.select_all()
    bpy.ops.armature.delete()
    
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
            
    num = 0
    for data_bone in data.bones:
        bpy.ops.armature.bone_primitive_add(name="Bone_"+str(num))
        bone = arm_data.edit_bones[-1]
        bone.use_connect = True
        bone["Point IDs"] = data_bone
        bone["Swap"] = data.bone_swap[num]
        #print (data_bone)
        bone.head = Vector((data.vertices[data_bone[0]][0],
                     data.vertices[data_bone[0]][1],
                     data.vertices[data_bone[0]][2]))
        bone.tail = Vector((data.vertices[data_bone[1]][0],
                     data.vertices[data_bone[1]][1],
                     data.vertices[data_bone[1]][2]))
        bone["Mass"] = data.bone_mass[num]
        bone["COM"] = [data.bone_com[num*3+0],
                       data.bone_com[num*3+1],
                       data.bone_com[num*3+2]]
        bone["mat"] = data.bone_mat[num*16:num*16+16]
        num = num + 1
        
    #print("Bones: ", data.bones)
    
    
    bpy.ops.armature.bone_primitive_add(name="root")
    bone = arm_data.edit_bones[-1]
    bone.head = Vector(((min_point[0]+max_point[0])*0.5,
                        (min_point[1]+max_point[1])*0.5+0.5,
                        -mid_point[2]))
    bone.tail = Vector(((min_point[0]+max_point[0])*0.5,
                        (min_point[1]+max_point[1])*0.5+0.25,
                        -mid_point[2]))
                        
    num = 0
    for bone_parent in data.bone_parents:
        name = "Bone_"+str(num);
        parent_name = "Bone_"+str(bone_parent)
        if bone_parent != -1:
            arm_data.edit_bones[name].parent = arm_data.edit_bones[bone_parent]
        else:
            arm_data.edit_bones[name].use_connect = False
            arm_data.edit_bones[name].parent = arm_data.edit_bones["root"]
        num = num + 1
        
    bpy.context.scene.update()
    
    bpy.ops.object.mode_set(mode='OBJECT')
    bpy.context.scene.objects.active = mesh_obj
    bpy.ops.object.vertex_group_remove(all=True)
    
    vertgroup_created=[]
    for bone in data.bones:
        vertgroup_created.append(0)

    mesh = mesh_obj.data
    index = 0
    vert_index = 0
    for vert in mesh.vertices:
        for bone_num in range(0,4):
            bone_id = int(data.bone_ids[index])
            bone_weight = data.bone_weights[index]
            name = "Bone_"+str(bone_id);
            if vertgroup_created[bone_id]==0:
                vertgroup_created[bone_id]=1
                mesh_obj.vertex_groups.new(name)
            #assign the weight for this vertex
            mesh_obj.vertex_groups.assign([vert_index], 
                                     mesh_obj.vertex_groups[name], 
                                     bone_weight, 
                                     'REPLACE')
            index = index + 1
        vert_index = vert_index + 1
    mesh.update()
        
    mesh_obj.select = True
    scn.objects.active = arm_obj
    bpy.ops.object.parent_set(type='ARMATURE')
    #arm_obj.makeParentDeform([mesh_obj], 0, 0)
    arm_obj.show_x_ray = True
    
    def ncr(n, r):
        return math.factorial(n) / \
              (math.factorial(r) * math.factorial(n - r))

    def CountJoints(bone):
        num_children = len(bone.children)
        if num_children == 0:
            return 0
        if bone.name != "root":
            num_children += 1
        count = ncr(num_children, 2)
        for child in bone.children:
            count += CountJoints(child)
        return int(count)
    
    '''arm_data["num_joints"] = CountJoints(arm_data.bones["root"])
    arm_data["num_special_joints"] = len(data.joints)
    num = 0
    for joint in data.joints:
        name = "joint_" + str(num)
        arm_data[name + "_type"] = joint.type[0]
        if joint.type[0] == phxbn_types._amotor_joint:
            arm_data[name + "_angles"] = joint.stop_angles
        elif joint.type[0] == phxbn_types._hinge_joint:
            arm_data[name + "_angles"] = joint.stop_angles
        arm_data[name + "_bone_ids"] = joint.bone_ids
        if joint.type[0] == phxbn_types._hinge_joint:
            arm_data[name+"_axis"] = joint.axis
        num = num + 1'''
    arm_data["parents"] = data.parents
    arm_data["parent_ids"] = data.parent_ids
    
    bpy.ops.object.mode_set(mode='EDIT')
    for joint in data.joints:
        if joint.type[0] == phxbn_types._hinge_joint:
            AddHingeJoint(arm_obj, joint)
        if joint.type[0] == phxbn_types._amotor_joint:
            AddAmotorJoint(arm_obj, joint)
        if joint.type[0] == phxbn_types._fixed_joint:
            AddFixedJoint(arm_obj, joint)
              
def Load(filepath):
    mesh_obj = GetMeshObj()
    if not mesh_obj:
        print("No mesh is selected")
        return
       
    data = ReadPHXBN(filepath, mesh_obj)   
    if not data:
        return
   
    AddArmature(data, mesh_obj)
    
class PHXBNImporter(bpy.types.Operator):
    '''Load Phoenix Bones armature'''
    bl_idname = "import_armature.phxbn"
    bl_label = "Import PHXBN"

    filepath = StringProperty(name="File Path", description="Filepath used for importing the PHXBN file", maxlen=1024, default="")

    def execute(self, context):
        print("Filepath:",self.properties.filepath)
        Load(self.properties.filepath)
        
        filename = self.properties.filepath.split("\\")[-1]
        #convert the filename to an object name
        objName = bpy.path.display_name(filename)
        print("Filename:",filename)

        #mesh = readMesh(self.properties.filepath, objName)
        #addMeshObj(mesh, objName)

        return {'FINISHED'}

    def invoke(self, context, event):
        wm = context.window_manager
        wm.add_fileselect(self)
        return {'RUNNING_MODAL'}
        
#Load("C:\\Users\\David\\Desktop\\Wolfire SVN\\Project\\Data\\Skeletons\\basic-attached-guard-joints.phxbn")
#Load("C:\\Users\\David\\Desktop\\export.phxbn")
