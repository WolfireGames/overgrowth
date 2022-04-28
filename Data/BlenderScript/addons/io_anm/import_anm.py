import bpy
from bpy.props import *
import array
import mathutils
from mathutils import Vector, Matrix
from io_anm import anm_types

def Get4ByteIntArray():
    var = array.array('l')
    if var.itemsize != 4:
        var = array.array('i')
    return var
    
class BoneNameTranslator():
    def __init__(self, arm_data):
        bone_labels = {}
        self.to_bone_map = {}
        self.from_bone_map = {}
        num = 0
        for bone in arm_data.bones:
            if bone.name == "root" or bone.layers[0] == False:
                continue
            if bone.name.split("_")[0] == "Bone":
                bone_id = int(bone.name.split("_")[-1])
                bone_labels[bone_id] = True
                self.to_bone_map[bone.name] = bone.name
                self.from_bone_map[bone.name] = bone.name
            num = num+1
        
        index = 0
        for bone in arm_data.bones:
            if bone.name == "root" or bone.layers[0] == False:
                continue
            if bone.name.split("_")[0] != "Bone":
                while index in bone_labels:
                    index = index + 1
                self.from_bone_map["Bone_"+str(index)] = bone.name
                self.to_bone_map[bone.name] = "Bone_"+str(index)
                index = index+1
    def ToBoneName(self,name):
        return self.to_bone_map.get(name)
    def FromBoneName(self,name):
        return self.from_bone_map.get(name)

def LoadInt(file):
    int_loader = Get4ByteIntArray()
    int_loader.fromfile(file, 1)
    return int_loader[0]
    
def LoadFloat(file):
    float_loader = array.array('f')
    float_loader.fromfile(file, 1)
    return float_loader[0]
    
def LoadBool(file):
    bool_loader = array.array('B')
    bool_loader.fromfile(file, 1)
    return bool_loader[0]

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
            
def CalcBoneMats(data):
    for bone in data.edit_bones:
        if not bone.get("mat"):
            bone["mat"] = GetBoneMat(bone)
    
def ReadANM(filepath):
    file = open(filepath, "rb")
    
    data = anm_types.ANMdata()
    data.name = filepath.split("\\")[-1]
    data.version = LoadInt(file)
    data.looping = LoadBool(file)
    if data.version > 0:
        data.start = LoadInt(file)
    data.end = LoadInt(file)
    num_keyframes = LoadInt(file)
    
    for i in range(num_keyframes):
        keyframe = anm_types.Keyframe()
        keyframe.time = LoadInt(file)
        if data.version >= 4:
            num_weights = LoadInt(file)
            keyframe.weights.fromfile(file, num_weights)
        num_bone_mats = LoadInt(file)
        for j in range(num_bone_mats):
            mat = array.array('f')
            mat.fromfile(file, 16)
            keyframe.mats.append(mat)
        if data.version >= 2:
            num_events = LoadInt(file)
            for j in range(num_events):
                event = anm_types.Event()
                event.which_bone = LoadInt(file)
                string_size = LoadInt(file)
                string_array = array.array("B")
                string_array.fromfile(file, string_size)
                event.string = string_array.tostring().decode("utf-8")
                keyframe.events.append(event)
        if data.version >= 3:
            num_ik_bones = LoadInt(file)
            for j in range(num_ik_bones):
                ik_bone = anm_types.IKBone()
                ik_bone.start.fromfile(file, 3)
                ik_bone.end.fromfile(file, 3)
                path_length = LoadInt(file)
                ik_bone.bone_path.fromfile(file, path_length)
                string_size = LoadInt(file)
                string_array = array.array("B")
                string_array.fromfile(file, string_size)
                ik_bone.string = string_array.tostring().decode("utf-8")
                keyframe.ik_bones.append(ik_bone)
                #print(ik_bone)
        if data.version >= 5:
            num_shape_keys = LoadInt(file)
            for j in range(num_shape_keys):
                shape_key = anm_types.ShapeKey()
                shape_key.weight = LoadFloat(file)
                string_size = LoadInt(file)
                string_array = array.array("B")
                string_array.fromfile(file, string_size)
                shape_key.string = string_array.tostring().decode("utf-8")
                keyframe.shape_keys.append(shape_key)
        if data.version >= 6:
            num_status_keys = LoadInt(file)
            for j in range(num_status_keys):
                status_key = anm_types.StatusKey()
                status_key.weight = LoadFloat(file)
                string_size = LoadInt(file)
                string_array = array.array("B")
                string_array.fromfile(file, string_size)
                status_key.string = string_array.tostring().decode("utf-8")
                keyframe.status_keys.append(status_key)
        data.keyframes.append(keyframe)
        
    
    '''
    print("Version: ", data.version)
    print("Looping: ", data.looping)
    print("Start: ", data.start)
    print("End: ", data.end)
    print("Num keyframes: ", len(data.keyframes))
    id = 0
    for keyframe in data.keyframes:
        print("Keyframe ", id, ":")
        print("    Time: ", keyframe.time)
        print("    Weights: ", keyframe.weights)
        print("    Mats: ", len(keyframe.mats))
        print("    Events: ", keyframe.events)
        print("    IKBones: ", keyframe.ik_bones)
    '''
    return data

def to_array(array, idarray):
    for item in idarray:
        array.append(item)
       
        
def PoseFromKeyframe(data, key_id, arm_obj):
    arm_data = arm_obj.data
    translation = []
    matrices = []
    for mat in data.keyframes[key_id].mats:
        matrix = Matrix([mat[0], -mat[2], mat[1], mat[3]],
                        [mat[4], -mat[6], mat[5], mat[7]],
                        [mat[8], -mat[10], mat[9], mat[11]],
                        [mat[12], -mat[14], mat[13], mat[15]])
        
        '''bpy.ops.object.add(type='EMPTY', location=(mat[12]+2, -mat[14], mat[13]))
        ob = bpy.context.scene.objects.active
        ob.empty_draw_type = 'ARROWS'
        ob.empty_draw_size = 0.1
        ob.rotation_mode = 'QUATERNION'
        ob.rotation_quaternion = matrix.rotation_part().to_quat()'''
        translation.append(matrix.translation_part())
        matrices.append(matrix.rotation_part())
    
    bpy.context.scene.objects.active = arm_obj
    
    bnt = BoneNameTranslator(arm_data)
    for bone in arm_data.bones:
        if bnt.ToBoneName(bone.name):
            bone.name = bnt.ToBoneName(bone.name)
    
    initial_translation = []
    initial_matrices = []
    bone_matrices = []
    inv_bone_matrices = []
    num = 0
    for iter_bone in arm_data.bones:
        if iter_bone.name == "root" or iter_bone.layers[0] == False:
            continue
        name = "Bone_"+str(num)
        bone = arm_data.bones[name]
        bone.use_hinge
        mat = array.array('f')
        to_array(mat, bone["mat"])
        matrix = Matrix([mat[0], -mat[2], mat[1], mat[3]],
                        [mat[4], -mat[6], mat[5], mat[7]],
                        [mat[8], -mat[10], mat[9], mat[11]],
                        [mat[12], -mat[14], mat[13], mat[15]])
                    
        initial_translation.append(matrix.translation_part())
        initial_matrices.append(matrix.rotation_part())
        bone_matrices.append(bone.matrix_local.rotation_part())
        inv_bone_matrices.append(bone_matrices[-1].copy().invert())
        '''
        bpy.ops.object.add(type='EMPTY', location=(mat[12]+4, -mat[14], mat[13]))
        ob = bpy.context.scene.objects.active
        ob.empty_draw_type = 'ARROWS'
        ob.empty_draw_size = 0.1
        ob.rotation_mode = 'QUATERNION'
        ob.rotation_quaternion = matrix.rotation_part().to_quat()
        
        bpy.ops.object.add(type='EMPTY', location=(mat[12]+6, -mat[14], mat[13]))
        ob = bpy.context.scene.objects.active
        ob.empty_draw_type = 'ARROWS'
        ob.empty_draw_size = 0.1
        ob.rotation_mode = 'QUATERNION'
        ob.rotation_quaternion = (matrices[num]*initial_matrices[num].copy().invert()).rotation_part().to_quat()'''
        num = num + 1
    bpy.context.scene.objects.active = arm_obj
    
    #for num in range(len(matrices)):
        #matrices[num] = inv_bone_matrices[num] * matrices[num] * bone_matrices[num]
        #initial_matrices[num] = inv_abone_matrices[num] * initial_matrices[num] * bone_matrices[num]
        
    initial_inv_matrices = []
    for mat in initial_matrices:
        matrix = mat.copy()
        matrix.invert()
        initial_inv_matrices.append(matrix)
        
    ident = Matrix([1,0,0],[0,1,0],[0,0,1])
    local_matrices = []
    num = 0
    root_translation = Vector((0,0,0))
    for matrix in matrices:
        name = "Bone_"+str(num)
        bone = arm_data.bones[name]
        parent_bone = bone.parent
        if not parent_bone or parent_bone.name == "root":
            local_matrices.append(matrix*initial_inv_matrices[num])
            curr_translation = translation[num] - initial_translation[num]
            #offset = matrix * initial_inv_matrices[num] * inv_bone_matrices[num] * Vector((0,bone.length*0.5,0));
            #curr_translation = curr_translation + Vector((offset[0], offset[1], offset[2]))
            root_translation = curr_translation
            root_translation[1] = root_translation[1]*-1
            root_translation[0] = root_translation[0]*-1
            offset = inv_bone_matrices[num] * local_matrices[num] * bone_matrices[num] * Vector((0,bone.length*0.5,0));
            offset[1],offset[2] = offset[2], offset[1]
            offset[2] *= -1
            offset[1] *= -1
            root_translation += offset
        else:
            parent_id = int(parent_bone.name.split("_")[-1])
            inv_mat = (matrices[parent_id]*initial_inv_matrices[parent_id]).invert()
            local_matrices.append(inv_mat*matrix*initial_inv_matrices[num])
        num = num + 1
      
    #straight up = [1,0,0][0,0,-1],[0,1,0]
    #straight right = [0,-1,0][1,0,0][0,0,1]
    #straight left = [0,1,0][-1,0,0][0,0,1]
    #local_matrices now contains local rotation matrices in world space
    #time to convert them to bone space
    num = 0
    for matrix in local_matrices:
        local_matrices[num] = inv_bone_matrices[num] * local_matrices[num] * bone_matrices[num]
        num = num + 1
   
    local_quats = []
    for matrix in local_matrices:
        local_quats.append(matrix.to_quat())
    
    bpy.ops.object.mode_set(mode='OBJECT', toggle=False)
    bpy.ops.object.mode_set(mode='POSE', toggle=False)

    pose = arm_obj.pose
    pose_bones = pose.bones

    num = 0
    for quat in local_quats:
        name = "Bone_"+str(num)
        bone = pose_bones[name]
        if bone.rotation_mode == 'QUATERNION':   
            bone.rotation_quaternion = quat
        else:
            mode = bone.rotation_mode
            euler = quat.to_euler(mode)
            bone.rotation_euler = euler
        num = num + 1
    
    pose_bones["root"].location = root_translation
    
    for bone in arm_data.bones:
        if bnt.FromBoneName(bone.name):
            bone.name = bnt.FromBoneName(bone.name)

def ApplyAnimation(data):
    arm_obj = bpy.context.scene.objects.active
    if arm_obj.type != 'ARMATURE':
        print("PHXBN armature must be selected")
        return
        
    bpy.ops.object.mode_set(mode='EDIT')
    CalcBoneMats(arm_obj.data)
    bpy.ops.object.mode_set(mode='OBJECT')
       
    action = bpy.data.actions.new(data.name)
    if not arm_obj.animation_data:
        arm_obj.animation_data_create()
    arm_obj.animation_data.action = action
    
    '''pose_bones = arm_obj.pose.bones
    for bone in pose_bones:
        group = action.groups.add(bone.name)
        for j in range(3):
            action.fcurves.new("rotation_euler",j,bone.name)'''
            
    action["Version"] = data.version
    action["Looping"] = data.looping
    action["Start"] = data.start
    action["End"] = data.end
    
    ik_bones = {}
    ik_bone_keys = {}
    for keyframe in data.keyframes:
        for bone in keyframe.ik_bones:
            if not bone.string in ik_bones:
                ik_bones[bone.string] = bone
                ik_bone_keys[bone.string] = {}
            ik_bone_keys[bone.string][keyframe.time] = True
    
    #print("IK Bones:\n",ik_bones)
    
    pose_bones = arm_obj.pose.bones
    index = 0
    for bone in pose_bones:
        for constraint in bone.constraints:
            if constraint.type == 'IK' and not constraint.target:
                bone.constraints.remove(constraint)
            else:
                index = index + 1
    
    ik_constraints = []
    for bone_string in ik_bones:
        bone = ik_bones[bone_string]
        constraint = pose_bones["Bone_"+str(bone.bone_path[-1])].constraints.new('IK')
        constraint.chain_count = len(bone.bone_path)
        constraint.name = bone_string
        ik_constraints.append(constraint)
        
    bone_quat = {}
    for i in range(len(data.keyframes)):
        the_frame = data.keyframes[i].time/1000*bpy.context.scene.render.fps
        PoseFromKeyframe(data, i, arm_obj)
        for bone in pose_bones:
            if bone.rotation_mode == 'QUATERNION':
                if bone.name in bone_quat:
                    if bone.rotation_quaternion.dot(bone_quat[bone.name]) < 0:
                        bone.rotation_quaternion.negate()
                bone.keyframe_insert("rotation_quaternion", frame = the_frame)
                bone_quat[bone.name] = bone.rotation_quaternion.copy()
            else:
                bone.keyframe_insert("rotation_euler", frame = the_frame)
        for constraint in ik_constraints:
            if data.keyframes[i].time in ik_bone_keys[constraint.name]:
                constraint.influence = 1
            else:
                constraint.influence = 0
            constraint.keyframe_insert("influence", frame = the_frame)
        pose_bones["root"].keyframe_insert("location", frame = the_frame)
        for shape_key in data.keyframes[i].shape_keys:
            arm_obj[shape_key.string] = shape_key.weight
            arm_obj.keyframe_insert("[\""+shape_key.string+"\"]", frame = the_frame)
    
def Load(filepath):
    data = ReadANM(filepath)   
    if not data:
        return
    ApplyAnimation(data)
    
class ANMImporter(bpy.types.Operator):
    '''Load Phoenix Animation'''
    bl_idname = "import_armature.anm"
    bl_label = "Import ANM"

    filepath = StringProperty(name="File Path", description="Filepath used for importing the ANM file", maxlen=1024, default="")

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

#Load("C:\\Users\\David\\Desktop\\Wolfire SVN\\Project\\Data\\Animations\\run.anm")
#Load("C:\\Users\\David\\Desktop\\export.anm")
