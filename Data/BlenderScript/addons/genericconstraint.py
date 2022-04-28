from mathutils import Matrix, Vector
import math

"""
This script adds a Phoenix generic rotation constraint
"""

bl_addon_info = {
    'name': 'Armature: Add generic constraint',
    'author': 'David Rosen',
    'version': '1',
    'blender': (2, 5, 4),
    'location': 'Armature > Generic constraint',
    'description': 'This script adds a Phoenix generic rotation constraint',
    'warning': '', # used for warning icon and text in addons panel
    'wiki_url': '',
    'tracker_url': '',
    'category': 'Armature'}
    
import bpy

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
            
def FixBoneRoll(context):
    obj = bpy.context.scene.objects.active
    bone = obj.data.edit_bones.active    
    if not bone:
        print("Need to select a bone first...")
        return
        
    name = bone.name 

    bpy.ops.object.mode_set(mode='POSE')
    bpy.ops.object.mode_set(mode='EDIT')

    bone = obj.data.edit_bones[name]
    
    matrix = GetD6Mat(bone)
    joint_axis = Vector((matrix[0], -matrix[2], matrix[1]))
    
    pose_bone = obj.pose.bones[bone.name]
    
    mat = obj.matrix_world * pose_bone.matrix
    x_axis = Vector((mat[0][0], mat[0][1], mat[0][2])).normalize()
    y_axis = Vector((mat[2][0], mat[2][1], mat[2][2])).normalize()
    joint_conv = Vector((-x_axis.dot(joint_axis), -y_axis.dot(joint_axis), 0)).normalize()
    roll = math.atan2(joint_conv[0], joint_conv[1])
    
    print("Changing "+bone.name+" roll from "+str(bone.roll)+" to "+str(bone.roll+roll))
    bone.roll += roll
    
    pose_bone.rotation_mode = 'YXZ'
    
    constraint = pose_bone.constraints.new('LIMIT_ROTATION')
    constraint.min_x = -0.01745
    constraint.max_x = 0.01745
    constraint.min_y = -0.01745
    constraint.max_y = 0.01745
    constraint.min_z = -0.01745
    constraint.max_z = 0.01745
    constraint.name = "RGDL_Limit Rotation" 
    constraint.use_limit_x = True
    constraint.use_limit_y = True
    constraint.use_limit_z = True
    constraint.influence = 0.0
    constraint.owner_space = 'LOCAL'

class BoneRollOp(bpy.types.Operator):
    '''Add generic rotation constraint'''
    bl_idname = 'armature.genericconstraint'
    bl_label = 'Add generic rotation constraint'
    bl_options = {'REGISTER', 'UNDO'}

    @classmethod
    def poll(self, context):
        obj = context.active_object
        return (obj and obj.type == 'ARMATURE')

    def execute(self, context):
        FixBoneRoll(context)
        return {'FINISHED'}

menu_func = (lambda self, context: self.layout.operator(BoneRollOp.bl_idname, text="Rotation constraint"))

def register():
    bpy.types.VIEW3D_MT_edit_armature.append(menu_func)

def unregister():
    bpy.types.VIEW3D_MT_edit_armature.remove(menu_func)

if __name__ == "__main__":
    register()
