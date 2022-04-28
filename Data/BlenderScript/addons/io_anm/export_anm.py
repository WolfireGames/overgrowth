import array
import bpy
from bpy.props import *
from io_anm import anm_types
import operator
from mathutils import Matrix, Euler, Vector, Quaternion

BONE_DEF_LAYER = 29
MOBILITY_LAYER = 25
WEAPON_DEF_LAYER = 24
WEAPON_VIEW_LAYER = 16
NUM_LAYERS = 32

def Get4ByteIntArray():     # Creates an array with 4-byte components, 
    var = array.array('l')  # regardless of whether that means 'l' or 'i'
    if var.itemsize != 4:
        var = array.array('i')
    return var
    
def AddConnectingBones(arm_data):     # Add a bone between disconnected parent-child pairs,
    for bone in arm_data.edit_bones:  # such as the head and the ears.
        if not bone.parent or \
           bone.name == "root" or \
           bone.parent.name == "root" or \
           bone.layers[BONE_DEF_LAYER] == False or \
           bone.parent.layers[BONE_DEF_LAYER] == False or \
           bone.head == bone.parent.tail:
            continue
        bpy.ops.armature.bone_primitive_add(name="connector")
        new_bone = arm_data.edit_bones[-1]
        new_bone.head = bone.parent.tail
        new_bone.tail = bone.head
        new_bone.parent = bone.parent
        for i in range(NUM_LAYERS):
            new_bone.layers[i] = False
        new_bone.layers[BONE_DEF_LAYER] = True
        bone.parent = new_bone
    
    bpy.context.scene.update()
    bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.object.mode_set(mode='EDIT')
    
class BoneNameTranslator():
    def __init__(self, arm_data):
        bone_labels = {}        # {int bone_id, bool exists} 
        self.to_bone_map = {}   # {string desc_label, string bone_id_label}
        self.from_bone_map = {} # {string bone_id_label, string desc_label}
        self.num_weapons = 0
        
        for bone in arm_data.bones: # Handle bones that are already named "Bone_#"
            if bone.name == "root" or bone.layers[BONE_DEF_LAYER] == False:
                continue
            if bone.name.split("_")[0] == "Bone":
                bone_id = int(bone.name.split("_")[-1])
                bone_labels[bone_id] = True
                self.to_bone_map[bone.name] = bone.name
                self.from_bone_map[bone.name] = bone.name
        
        unused_bone_id = 0
        for bone in arm_data.bones: # Rename and record all skeleton bones
            if bone.name == "root" or bone.layers[BONE_DEF_LAYER] == False:
                continue
            if bone.name.split("_")[0] != "Bone":
                while unused_bone_id in bone_labels:
                    unused_bone_id = unused_bone_id + 1
                self.from_bone_map["Bone_"+str(unused_bone_id)] = bone.name
                self.to_bone_map[bone.name] = "Bone_"+str(unused_bone_id)
                bone.name = "Bone_"+str(unused_bone_id)
                unused_bone_id = unused_bone_id+1
        
        unused_weapon_id = 0
        for bone in arm_data.bones: # Rename and record all weapon bones
            if bone.name == "root" or \
               bone.layers[WEAPON_DEF_LAYER] == False or \
               bone.name.split('.')[0] == "connector":
                continue
            if bone.name.split("_")[0] != "Weap":
                while unused_weapon_id in bone_labels:
                    unused_weapon_id = unused_weapon_id + 1
                self.from_bone_map["Weap_"+str(unused_weapon_id)] = bone.name
                self.to_bone_map[bone.name] = "Weap_"+str(unused_weapon_id)
                bone.name = "Weap_"+str(unused_weapon_id)
                unused_weapon_id += 1
                self.num_weapons += 1
    def ToBoneName(self,name):
        return self.to_bone_map.get(name)
    def FromBoneName(self,name):
        return self.from_bone_map.get(name)

def to_array(array, idarray): #Populate an array with elements from a list or dictionary
    for item in idarray:
        array.append(item)

def GetInitialBoneMatZ(edit_bone): #Get initial bone matrix that is facing the z axis
    z_vec = (edit_bone.tail - edit_bone.head).normalize()
    x_vec = Vector((0,0,1))
    y_vec = x_vec.cross(z_vec).normalize()
    x_vec = y_vec.cross(z_vec).normalize()

    return [x_vec[0], x_vec[2], -x_vec[1], 0,
            y_vec[0], y_vec[2], -y_vec[1], 0,
            z_vec[0], z_vec[2], -z_vec[1], 0,
            0,0,0,1]


def GetInitialBoneMat(edit_bone): #Get initial bone matrix that is facing the y axis (or z if needed)
    z_vec = (edit_bone.tail - edit_bone.head).normalize()
    y_vec = Vector((0,1,0))
    if abs(z_vec.dot(y_vec)) > 0.95:
        return GetInitialBoneMatZ(edit_bone)
    x_vec = z_vec.cross(y_vec).normalize()
    y_vec = z_vec.cross(x_vec).normalize()

    return [x_vec[0], x_vec[2], -x_vec[1], 0,
            y_vec[0], y_vec[2], -y_vec[1], 0,
            z_vec[0], z_vec[2], -z_vec[1], 0,
            0,0,0,1]
            
initial_bone_mats = {} # { string name, float mat[16] }
def CalcInitialBoneMats(data): # Calculate initial bone matrices if they are not already provided
    for bone in data.edit_bones:
        if not bone.get("mat"):
            initial_bone_mats[bone.name] = GetInitialBoneMat(bone)
        else:
            initial_bone_mats[bone.name] = bone["mat"]
            
def GetBoneMatrixRotation(arm_data, num, rotations, rotation_modes):
    bone = arm_data.bones["Bone_"+str(num)]
    if rotation_modes[num] == 'QUATERNION':
        matrix = Quaternion(rotations[num]).normalize().to_matrix()
    else:
        matrix = Euler(rotations[num][0:3],rotation_modes[num]).to_matrix()

    # Convert matrix to bone space
    matrix = bone.matrix_local.to_3x3() * matrix * bone.matrix_local.copy().to_3x3().invert()
    
    # Convert matrix to parent space by applying initial bone rotation
    if initial_bone_mats[bone.name]:
        mat = array.array('f')
        to_array(mat, initial_bone_mats[bone.name])
        initial_matrix = Matrix([mat[0], -mat[2], mat[1], mat[3]],
                                [mat[4], -mat[6], mat[5], mat[7]],
                                [mat[8], -mat[10], mat[9], mat[11]],
                                [mat[12], -mat[14], mat[13], mat[15]])
        matrix = matrix.to_4x4() * initial_matrix
    else:
        print("Could not find: " + bone.name)
    
    # Convert matrix to world space by recursively applying all parent rotations
    parent = bone.parent
    if not parent or parent.name == "root" or parent.layers[BONE_DEF_LAYER] == False or bone.layers[BONE_DEF_LAYER] == False:
        return matrix
    if initial_bone_mats.get(parent.name):
        mat = array.array('f')
        to_array(mat, initial_bone_mats[parent.name])
        parent_initial_matrix = Matrix([mat[0], -mat[2], mat[1], mat[3]],
                                       [mat[4], -mat[6], mat[5], mat[7]],
                                       [mat[8], -mat[10], mat[9], mat[11]],
                                       [mat[12], -mat[14], mat[13], mat[15]])
        
    parent_num = int(parent.name.split("_")[-1])
    parent_matrix = GetBoneMatrixRotation(arm_data, parent_num, rotations, rotation_modes)
    
    matrix = parent_matrix * parent_initial_matrix.copy().invert() * matrix    
    return matrix
    
def GetBoneMatrixTranslation(arm_data, num, rotations, rotation_modes):
    bone = arm_data.bones["Bone_"+str(num)]
    
    if rotation_modes[num] == 'QUATERNION':
        matrix = Quaternion(rotations[num]).normalize().to_matrix()
    else:
        matrix = Euler(rotations[num][0:3],rotation_modes[num]).to_matrix()
    matrix = bone.matrix_local * matrix.resize4x4()

    # Convert matrix to world space by recursively applying all parent rotations
    parent = bone.parent        
    if not parent or parent.name == "root" or parent.layers[BONE_DEF_LAYER] == False or bone.layers[BONE_DEF_LAYER] == False:
        return matrix
    
    parent_num = int(parent.name.split("_")[-1])
    parent_matrix = GetBoneMatrixTranslation(arm_data, parent_num, rotations, rotation_modes) * parent.matrix_local.copy().invert()
    
    matrix = parent_matrix * matrix
    
    return matrix
    
def GetNumJoints(arm_obj):
    num = 0
    for bone in arm_obj.pose.bones:
        for constraint in bone.constraints:
            if constraint.type == 'LIMIT_ROTATION' and constraint.owner_space == 'LOCAL':
                num += 1
    return num
    
def KeyframeFromPose(arm_obj, rotations, rotation_modes, translation, num_joints):
    arm_data = arm_obj.data
    matrices = []
   
    keyframe = anm_types.Keyframe()
    for num in range(len(rotations)):
        # Get rotation matrix, then set translation
        matrix = GetBoneMatrixRotation(arm_data, num, rotations, rotation_modes)
        trans_matrix = GetBoneMatrixTranslation(arm_data, num, rotations, rotation_modes)
            
        bone = arm_data.bones["Bone_"+str(num)]
        matrix[3] = trans_matrix * Vector((0,bone.length*0.5,0,1));
        #matrix[3] = matrix[3] + Vector((translation[1], -translation[0], translation[2], 0))
        matrix[3] = matrix[3] + Vector((translation[0], translation[1], translation[2], 0))
        '''bpy.ops.object.add(type='EMPTY', location=(matrix[3][0]+3,matrix[3][1],matrix[3][2]))
        ob = bpy.context.scene.objects.active
        ob.empty_draw_type = 'ARROWS'
        ob.empty_draw_size = 0.1
        ob.rotation_mode = 'QUATERNION'
        ob.rotation_quaternion = matrix.rotation_part().to_quat()'''
        
        mat = array.array('f')
        mat.fromlist([matrix[0][0], matrix[0][2], -matrix[0][1], matrix[0][3],
                      matrix[1][0], matrix[1][2], -matrix[1][1], matrix[1][3],
                      matrix[2][0], matrix[2][2], -matrix[2][1], matrix[2][3],
                      matrix[3][0], matrix[3][2], -matrix[3][1], matrix[3][3]])
        keyframe.mats.append(mat)
        
    return keyframe
    

class KeyframeInfo:
    def __init__(self):
        self.weap_relative_weight = {}
        self.weap_relative_id = {}
        self.weap_translations = {}
        self.weap_rotations = {}
        self.mobility_translation = {}
        self.mobility_rotation = {}
        self.rotations = {}
        self.rotation_modes = {}
        self.weights = {}
        self.translation = array.array('f')
        self.translation.fromlist([0,0,0])
        self.translation_offset = array.array('f') 
        self.translation_offset.fromlist([0,0,0])
        self.ik_bones = []
        self.shape_keys = []
        self.status_keys = []
        self.events = []
    def __repr__(self):
        string = ""
        for bone_id in self.rotations:
            string = string + "Bone " + str(bone_id) + ": \n"
            string = string + str(self.rotations[bone_id])
            string = string + "\n"
        return string
        
old_trans = {} # {string, Vector()} Initial translation of each bone

# We are at a specific frame, so read back the bone matrix info
def BakeBoneConstraints(bone,obj,old_obj,parent_matrix):
    pose_bone = old_obj.pose.bones[bone.name]
    
    inv_bone_matrix = bone.matrix_local.copy().invert()
    inv_parent_matrix = parent_matrix.copy().invert()
         
    matrix_local = inv_bone_matrix * inv_parent_matrix * pose_bone.matrix
    
    # Set keyframes for local translation and rotation values
    new_pose_bone = obj.pose.bones[bone.name]
    new_pose_bone.location = matrix_local.translation_part()
    new_pose_bone.keyframe_insert("location")
    
    if new_pose_bone.rotation_mode == 'QUATERNION':
        new_pose_bone.rotation_quaternion = matrix_local.to_quat().normalize()
        new_pose_bone.keyframe_insert("rotation_quaternion")
    else:
        new_pose_bone.rotation_euler = matrix_local.to_euler(pose_bone.rotation_mode)
        new_pose_bone.keyframe_insert("rotation_euler")
    
    matrix = parent_matrix * bone.matrix_local * new_pose_bone.matrix_local * inv_bone_matrix
    
    if bone.name != "root" and bone.layers[BONE_DEF_LAYER] == True:
        mat = pose_bone.matrix 
        new_trans = mat.translation_part()
        new_pose_bone.location = new_trans - old_trans[bone.name]*2
        new_pose_bone.keyframe_insert("location")
        
    if bone.layers[WEAPON_DEF_LAYER] == True or bone.name == "mobility":
        old_pose_bone = old_obj.pose.bones[bone.name]
        new_pose_bone = obj.pose.bones[bone.name]
        new_pose_bone.location = (old_pose_bone.matrix[3][0],old_pose_bone.matrix[3][1],old_pose_bone.matrix[3][2])
        new_pose_bone.keyframe_insert("location")
        new_pose_bone.rotation_quaternion = old_pose_bone.matrix.to_quat().normalize()
        new_pose_bone.keyframe_insert("rotation_quaternion")
        
    for child in bone.children:
        BakeBoneConstraints(child,obj,old_obj,matrix)

#stretch_amount = 600
stretch_amount = 1

# Walk through each keyframe and bake all constraints
def BakeConstraints(obj):
    action = obj.animation_data.action
    
    times = {}
    
    for fcurve in action.fcurves:
        for key in fcurve.keyframe_points:
            key.co[0] = int(key.co[0]*stretch_amount)
            key.handle_left[0] = int(key.handle_left[0]*stretch_amount)
            key.handle_right[0] = int(key.handle_right[0]*stretch_amount)
            times[key.co[0]] = True
            
    sorted_times = []
    for time in times:
        sorted_times.append(time)
    sorted_times.sort()
    
    bpy.ops.object.duplicate()
    new_obj = bpy.context.scene.objects.active
    
    # Go to each frame in sequence so Blender calculates the bone matrices
    frame = bpy.context.scene.frame_current
    for time in sorted_times:
        num = int(time)
        bpy.context.scene.frame_set(num)
        BakeBoneConstraints(obj.data.bones["root"], obj, new_obj, Matrix())
    bpy.context.scene.frame_set(frame)
    '''
    action = new_obj.animation_data.action
    
    bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.object.select_all(action = 'DESELECT')
    bpy.context.scene.objects.active = new_obj
    bpy.context.scene.objects.active.select = True'''
    bpy.ops.object.delete()
    bpy.context.scene.objects.active = obj
    
    action.user_clear()
    
    for fcurve in action.fcurves:
        for key in fcurve.keyframe_points:
            key.co[0] /= stretch_amount
            key.handle_left[0] /= stretch_amount
            key.handle_right[0] /= stretch_amount
    
    bpy.ops.object.mode_set(mode='EDIT')
    for bone in obj.data.edit_bones:
        if bone.layers[BONE_DEF_LAYER] == False:
            continue
        while bone.parent and bone.parent.layers[BONE_DEF_LAYER] == False and bone.parent.parent:
            bone.parent = bone.parent.parent
    bpy.context.scene.update()
    bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.object.mode_set(mode='EDIT')

def GetAnimation(filepath):
    old_arm_obj = bpy.context.scene.objects.active
       
    if old_arm_obj.type != 'ARMATURE':
        print("PHXBN armature must be selected")
        return
        
    #Create a copy of the armature object so we don't mess up the original
    bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.object.duplicate()
    arm_obj = bpy.context.scene.objects.active
    
    #Add connecting bones between parent-child pairs that are not connected
    bpy.ops.object.mode_set(mode='EDIT')
    AddConnectingBones(arm_obj.data)
    
    #Rename relevant bones to Bone_0, Bone_1, etc
    tbr = BoneNameTranslator(arm_obj.data) 
    
    #txt_file = open(filepath+".txt", "w")
    #for num in range(100):
    #    name = "Bone_"+str(num)
    #    if tbr.from_bone_map.get(name):
    #        txt_file.write(name+": "+tbr.from_bone_map.get(name)+'\n')
    
    #Get initial root translation of each bone
    for bone in arm_obj.data.edit_bones:
        old_trans[bone.name] = bone.head - arm_obj.data.edit_bones["root"].head
    
    #Get initial bone rotation of each bone (using OG Bullet physics orientations)
    CalcInitialBoneMats(arm_obj.data)
    
    #Bake the constraints into the bone matrices
    bpy.ops.object.mode_set(mode='OBJECT')
    BakeConstraints(arm_obj)
        
    action = arm_obj.animation_data.action
    if not action:
        print("Armature must have an action")
        return

    #Create animation data structure and start extracting the easy data
    data = anm_types.ANMdata()
 
    if action.get("Looping"):
        data.looping = action["Looping"]
    else:
        data.looping = False
        
    if action.get("Start"):
        data.start = action["Start"]
    else:
        data.start = int(bpy.context.scene.frame_start*1000/bpy.context.scene.render.fps)
        
    if action.get("End"):
        data.end = action["End"]
    else:
        data.end = int(bpy.context.scene.frame_end*1000/bpy.context.scene.render.fps)
        
    if action.get("Version"):
        data.version = action["Version"]
    else:
        data.version = 4
    
    keyframes = {}
    
    # Record all of the times that have keyframes on them
    pose_bones = arm_obj.pose.bones
    for fcurve in action.fcurves:
        for keyframe in fcurve.keyframe_points:
            time = round(keyframe.co[0]*1000/bpy.context.scene.render.fps)
            if not time in keyframes:
                keyframes[time] = KeyframeInfo()
            
    # Get an ordered list of all the keyframe times
    keyframe_times = []
    for time in keyframes:
        keyframe_times.append(time)
    keyframe_times.sort()
   
    # Sample each fcurve at each keyframe time
    for fcurve in action.fcurves:
        quote_split = fcurve.data_path.split("\"")
        if len(quote_split) < 2:
            continue
        event_split = fcurve.data_path.split("event_")
        if len(event_split) == 2:
            event_name = event_split[1].split("\"")[0]
            for keyframe in fcurve.keyframe_points:
                time = round(keyframe.co[0]*1000/bpy.context.scene.render.fps)
                key_info = keyframes[time]
                event = []
                event.append(event_name)
                bone_name = quote_split[1]
                bone_id = int(bone_name.split("_")[-1])
                event.append(bone_id)
                key_info.events.append(event)
            continue
        weight_split = fcurve.data_path.split("weight")
        if len(weight_split) == 2:
            for time in keyframes:
                key_info = keyframes[time]
                bone_name = quote_split[1]
                bone_id = int(bone_name.split("_")[-1])
                key_info.weights[bone_id] = fcurve.evaluate(time/1000*bpy.context.scene.render.fps)
            continue
        shape_split = fcurve.data_path.split("shape_")
        if len(shape_split) == 2:
            shape_name = shape_split[1].split("\"")[0]
            for time in keyframes:
                key_info = keyframes[time]
                shape_key = []
                shape_key.append(shape_name)
                shape_key.append(fcurve.evaluate(time/1000*bpy.context.scene.render.fps))
                key_info.shape_keys.append(shape_key)
            continue
        status_split = fcurve.data_path.split("status_")
        if len(status_split) == 2:
            status_name = status_split[1].split("\"")[0]
            for time in keyframes:
                key_info = keyframes[time]
                status = []
                status.append(status_name)
                status.append(fcurve.evaluate(time/1000*bpy.context.scene.render.fps))
                key_info.status_keys.append(status)
            continue
        if quote_split[1] == "root":
            if fcurve.data_path.split(".")[-1] == "location":
                array_index = fcurve.array_index
                for time in keyframes:
                    key_info = keyframes[time]
                    key_info.translation[array_index] = fcurve.evaluate(time/1000*bpy.context.scene.render.fps)
            continue
        bone_name =quote_split[1]
        if bone_name.split("_")[0] == "Weap":
            weap_id = int(bone_name.split("_")[-1])
            array_index = fcurve.array_index
            path_end = fcurve.data_path.split(".")[-1]
            if path_end == "location":
                print("Handling weap location "+str(weap_id))
                for time in keyframe_times:
                    val = fcurve.evaluate(time/1000*bpy.context.scene.render.fps)
                    key_info = keyframes[time]
                    if not weap_id in key_info.weap_translations:
                        key_info.weap_translations[weap_id] = [0,0,0]
                    key_info.weap_translations[weap_id][array_index] = val
                continue
            if path_end == "influence":
                name = quote_split[3]
                constraint = pose_bones[bone_name].constraints[name]
                if constraint.type == 'CHILD_OF':
                    #print("Child of detected")
                    #print(fcurve.data_path)
                    #print(constraint.target)
                    #print(constraint.subtarget)
                   
                    for time in keyframe_times:
                        val = fcurve.evaluate(time/1000*bpy.context.scene.render.fps)
                        key_info = keyframes[time]
                        key_info.weap_relative_id[weap_id] = int(constraint.subtarget.split("_")[-1])
                        key_info.weap_relative_weight[weap_id] = val
                        #print(str(time) + " " + str(key_info.weap_relative_id[weap_id]) + " " + str(key_info.weap_relative_weight[weap_id]))
                    continue
            if path_end != "rotation_quaternion":
                continue
            for time in keyframe_times:
                val = fcurve.evaluate(time/1000*bpy.context.scene.render.fps)
                key_info = keyframes[time]
                if not weap_id in key_info.weap_rotations:
                    key_info.weap_rotations[weap_id] = [0,0,0,0]
                key_info.weap_rotations[weap_id][array_index] = val
        if bone_name == "mobility":
            array_index = fcurve.array_index
            path_end = fcurve.data_path.split(".")[-1]
            if path_end == "location":
                print("Handling mobility location "+str(array_index))
                for time in keyframe_times:
                    val = fcurve.evaluate(time/1000*bpy.context.scene.render.fps)
                    key_info = keyframes[time]
                    key_info.mobility_translation[array_index] = val
                continue
            if path_end == "rotation_quaternion":
                print("Handling mobility rotation "+str(array_index))
                for time in keyframe_times:
                    val = fcurve.evaluate(time/1000*bpy.context.scene.render.fps)
                    key_info = keyframes[time]
                    key_info.mobility_rotation[array_index] = val
        if not bone_name.split("_")[0] == "Bone":
            continue
        bone_id = int(bone_name.split("_")[-1])
        array_index = fcurve.array_index
        path_end = fcurve.data_path.split(".")[-1]
        if path_end == "influence":
            name = quote_split[3]
            constraint = pose_bones["Bone_"+str(bone_id)].constraints[name]
            if constraint.type != 'IK' or constraint.target:
                continue
            ik_bone_data = []
            ik_bone_data.append(name)
            ik_bone_data.append(bone_id)
            for time in keyframe_times:
                if fcurve.evaluate(time/1000*bpy.context.scene.render.fps)>0.5 :
                    key_info = keyframes[time]
                    key_info.ik_bones.append(ik_bone_data)
        if path_end == "location":
            if pose_bones["Bone_"+str(bone_id)].parent.name == "root":
                for time in keyframe_times:
                    key_info = keyframes[time]
                    key_info.translation_offset[array_index] = fcurve.evaluate(time/1000*bpy.context.scene.render.fps)
                continue
        if path_end != "rotation_euler" and path_end != "rotation_quaternion":
            continue
        for time in keyframe_times:
            key_info = keyframes[time]
            key_info.rotation_modes[bone_id] = pose_bones["Bone_"+str(bone_id)].rotation_mode
            if not bone_id in key_info.rotations:
                key_info.rotations[bone_id] = []
            while array_index >= len(key_info.rotations[bone_id]):
                key_info.rotations[bone_id].append(0)
            key_info.rotations[bone_id][array_index] = fcurve.evaluate(time/1000*bpy.context.scene.render.fps)
          
    num_joints = GetNumJoints(arm_obj)
          
    if not arm_obj.data.layers[WEAPON_VIEW_LAYER]:
        print("Not exporting weapons because weapon layer is hidden")
        tbr.num_weapons = 0
    print("Num weapons: "+str(tbr.num_weapons))   
    for time in keyframe_times:
        rotations = keyframes[time].rotations
        rotation_modes = keyframes[time].rotation_modes
        keyframes[time].translation[0] = keyframes[time].translation_offset[0]
        keyframes[time].translation[1] = keyframes[time].translation_offset[1]
        keyframes[time].translation[2] = keyframes[time].translation_offset[2]
        translation = keyframes[time].translation
        # Get bone matrices from joint rotations and root translation
        keyframe = KeyframeFromPose(arm_obj, rotations, rotation_modes, translation, num_joints)
        keyframe.time = time
        
        if len(keyframes[time].mobility_translation) == 3 and \
           len(keyframes[time].mobility_rotation) == 4:
            m_translation = keyframes[time].mobility_translation
            m_rotation = keyframes[time].mobility_rotation
            quat = Quaternion()
            quat[0] = m_rotation[0]
            quat[1] = m_rotation[1]
            quat[2] = m_rotation[2]
            quat[3] = m_rotation[3]
            matrix = quat.to_matrix().to_4x4()
            matrix[3][0] = m_translation[0]
            matrix[3][1] = m_translation[1]
            matrix[3][2] = m_translation[2]
            mat = array.array('f')
            mat.fromlist([matrix[0][0], matrix[0][2], -matrix[0][1], matrix[0][3],
                          matrix[1][0], matrix[1][2], -matrix[1][1], matrix[1][3],
                          matrix[2][0], matrix[2][2], -matrix[2][1], matrix[2][3],
                          matrix[3][0], matrix[3][2], -matrix[3][1], matrix[3][3]])
            keyframe.mobility_mat = mat
        
        for i in range(len(rotations)):
            if i in keyframes[time].weights:
                keyframe.weights.append(keyframes[time].weights[i])
            else:
                keyframe.weights.append(0.0)
        for i in range(tbr.num_weapons):
            w_translation = keyframes[time].weap_translations[i]
            rotation = keyframes[time].weap_rotations[i]
            quat = Quaternion()
            quat[0] = rotation[0]
            quat[1] = rotation[1]
            quat[2] = rotation[2]
            quat[3] = rotation[3]
            matrix = quat.to_matrix().to_4x4()
            matrix[3][0] = w_translation[0]
            matrix[3][1] = w_translation[1] - 0.03
            matrix[3][2] = w_translation[2] - 0.69
            mat = array.array('f')
            mat.fromlist([matrix[0][0], matrix[0][2], -matrix[0][1], matrix[0][3],
                          matrix[1][0], matrix[1][2], -matrix[1][1], matrix[1][3],
                          matrix[2][0], matrix[2][2], -matrix[2][1], matrix[2][3],
                          matrix[3][0], matrix[3][2], -matrix[3][1], matrix[3][3]])
            keyframe.weapon_mats.append(mat)
            if i in keyframes[time].weap_relative_id:
                keyframe.weap_relative_ids.append(keyframes[time].weap_relative_id[i])
            else:
                keyframe.weap_relative_ids.append(-1)
            if i in keyframes[time].weap_relative_weight:
                keyframe.weap_relative_weights.append(keyframes[time].weap_relative_weight[i])
            else:
                keyframe.weap_relative_weights.append(0.0)
        for shape_key_data in keyframes[time].shape_keys:
            shape_key = anm_types.ShapeKey()
            shape_key.string = shape_key_data[0]
            shape_key.weight = shape_key_data[1]
            keyframe.shape_keys.append(shape_key)
        for status_key_data in keyframes[time].status_keys:
            status_key = anm_types.StatusKey()
            status_key.string = status_key_data[0]
            status_key.weight = status_key_data[1]
            keyframe.status_keys.append(status_key)
        for event_data in keyframes[time].events:
            event = anm_types.Event()
            event.string = event_data[0]
            event.which_bone = event_data[1]
            keyframe.events.append(event)
        for ik_bone_data in keyframes[time].ik_bones:
            #print("Adding an IK bone")
            ik_bone = anm_types.IKBone()
            ik_bone.string = ik_bone_data[0]
            bone_id = ik_bone_data[1]
            constraint = pose_bones["Bone_"+str(bone_id)].constraints[ik_bone.string]
            path_length = constraint.chain_count
            ik_bone.bone_path.append(bone_id)
            index = bone_id
            for i in range(path_length-1):
                index = int(pose_bones["Bone_"+str(index)].parent.name.split("_")[-1])
                ik_bone.bone_path.append(index)
            ik_bone.bone_path.reverse()
            vec = pose_bones["Bone_"+str(ik_bone.bone_path[0])].head
            ik_bone.start.fromlist([vec[0],vec[1],vec[2]])
            vec = pose_bones["Bone_"+str(ik_bone.bone_path[-1])].tail
            ik_bone.end.fromlist([vec[0],vec[1],vec[2]])
            keyframe.ik_bones.append(ik_bone)
                
        data.keyframes.append(keyframe)
    
    bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.object.select_all(action = 'DESELECT')
    bpy.context.scene.objects.active = arm_obj
    bpy.context.scene.objects.active.select = True
    bpy.ops.object.delete()
    bpy.context.scene.objects.active = old_arm_obj
    
    action.user_clear()
    
    #data.keyframes.sort(key=operator.attrgetter('time'))
    #print(data.keyframes)
    
    return data
 
def WriteFloat(file, val):
    int_loader = array.array('f')
    int_loader.append(val)
    int_loader.tofile(file)
 
def WriteInt(file, val):
    int_loader = Get4ByteIntArray()
    int_loader.append(val)
    int_loader.tofile(file)
    
def WriteBool(file, val):
    bool_loader = array.array('B')
    bool_loader.append(val)
    bool_loader.tofile(file)
 
def WriteANM(filepath, data):

    #print(data.keyframes[0])
    file = open(filepath, "wb")

    data.version = 10
    num_keyframes = len(data.keyframes)
    WriteInt(file, data.version)
    WriteBool(file, False)
    WriteBool(file, data.looping)
    WriteInt(file, int(data.start))
    WriteInt(file, int(data.end))
    WriteInt(file, num_keyframes)
    
    '''print("\nItem sizes:")
    print(array.array('B').itemsize)
    print(array.array('l').itemsize)
    print(array.array('h').itemsize)
    print(array.array('i').itemsize)
    print(array.array('L').itemsize)
    print(array.array('f').itemsize)'''
    '''
    print("\n")
    print(data.version)
    print(data.looping)
    print(data.start)
    print(data.end)
    print(num_keyframes)'''
    
    for i in range(num_keyframes):
        keyframe = data.keyframes[i]
        WriteInt(file, keyframe.time)
        num_weights = len(keyframe.weights)
        WriteInt(file, num_weights)
        keyframe.weights.tofile(file)
        num_bone_mats = len(keyframe.mats)
        WriteInt(file, num_bone_mats)
        for j in range(num_bone_mats):
            keyframe.mats[j].tofile(file)
        num_weapon_mats = len(keyframe.weapon_mats)
        WriteInt(file, num_weapon_mats)
        for j in range(num_weapon_mats):
            keyframe.weapon_mats[j].tofile(file)
            WriteInt(file,keyframe.weap_relative_ids[j])
            WriteFloat(file,keyframe.weap_relative_weights[j])
        if len(keyframe.mobility_mat) > 0:
            WriteBool(file, True)
            keyframe.mobility_mat.tofile(file)
        else :
            WriteBool(file, False)
        num_events = len(keyframe.events)
        WriteInt(file, num_events)
        for event in keyframe.events:
            WriteInt(file, event.which_bone)
            string_array = array.array("B")
            string_array.fromstring(event.string.encode("utf-8"))
            WriteInt(file, len(string_array))
            string_array.tofile(file)
        num_ik_bones = len(keyframe.ik_bones)
        WriteInt(file, num_ik_bones)
        for ik_bone in keyframe.ik_bones:
            ik_bone.start.tofile(file)
            ik_bone.end.tofile(file)
            WriteInt(file, len(ik_bone.bone_path))
            ik_bone.bone_path.tofile(file)
            string_array = array.array("B")
            string_array.fromstring(ik_bone.string.encode("utf-8"))
            WriteInt(file, len(string_array))
            string_array.tofile(file)
        num_shape_keys = len(keyframe.shape_keys)
        WriteInt(file, num_shape_keys)
        for shape_key in keyframe.shape_keys:
            WriteFloat(file, shape_key.weight)
            string_array = array.array("B")
            string_array.fromstring(shape_key.string.encode("utf-8"))
            WriteInt(file, len(string_array))
            string_array.tofile(file)
        num_status_keys = len(keyframe.status_keys)
        WriteInt(file, num_status_keys)
        for status_key in keyframe.status_keys:
            WriteFloat(file, status_key.weight)
            string_array = array.array("B")
            string_array.fromstring(status_key.string.encode("utf-8"))
            WriteInt(file, len(string_array))
            string_array.tofile(file)
 
def Save(filepath):
    data = GetAnimation(filepath)
    if not data:
        return
    WriteANM(filepath, data)

class ANMExporter(bpy.types.Operator):
    '''Save Phoenix Bones armature'''
    bl_idname = "export_mesh.anm"
    bl_label = "Export ANM"

    filepath = StringProperty(name="File Path", description="Filepath used for exporting the ANM file", maxlen= 1024, default= "")
    
    check_existing = BoolProperty(name="Check Existing", description="Check and warn on overwriting existing files", default=True, options={'HIDDEN'})

    def execute(self, context):
        Save(self.properties.filepath)
        return {'FINISHED'}

    def invoke(self, context, event):
        wm = context.window_manager
        wm.add_fileselect(self)
        return {'RUNNING_MODAL'}
        
#Save("C:\\Users\\David\\Desktop\\export.anm")
#Save("Desktop/export.anm")
#BakeConstraints(bpy.context.scene.objects.active)
#Save("C:\\Users\\David\\Desktop\\WolfireSVN\\Project\\Data\\Animations\\r_bigdogswordattackover.anm")
