from mathutils import Matrix, Vector
import math

"""
This script adds a Phoenix hinge constraint
"""

bl_addon_info = {
    'name': 'Armature: Add hinge constraint',
    'author': 'David Rosen',
    'version': '1',
    'blender': (2, 5, 4),
    'location': 'Armature > Hinge constraint',
    'description': 'This script adds a Phoenix hinge constraint',
    'warning': '', # used for warning icon and text in addons panel
    'wiki_url': '',
    'tracker_url': '',
    'category': 'Armature'}
    
import bpy
            
def FixBoneRoll(context):
    obj = bpy.context.scene.objects.active
    bone = obj.data.edit_bones.active    
    if not bone:
        print("Need to select a bone first...")
        return
        
    name = bone.name 

    bone = obj.data.edit_bones[name]
    pose_bone = obj.pose.bones[bone.name]
    
    pose_bone.rotation_mode = 'XYZ'
    
    constraint = pose_bone.constraints.new('LIMIT_ROTATION')
    constraint.min_x = -0.01745
    constraint.max_x = 0.01745
    constraint.min_y = 0
    constraint.max_y = 0
    constraint.min_z = 0
    constraint.max_z = 0
    constraint.name = "RGDL_Limit Rotation" 
    constraint.use_limit_x = True
    constraint.use_limit_y = True
    constraint.use_limit_z = True
    constraint.influence = 0.0
    constraint.owner_space = 'LOCAL'

class HingeConstraintOp(bpy.types.Operator):
    '''Add hinge constraint'''
    bl_idname = 'armature.hingeconstraint'
    bl_label = 'Add hinge constraint'
    bl_options = {'REGISTER', 'UNDO'}

    @classmethod
    def poll(self, context):
        obj = context.active_object
        return (obj and obj.type == 'ARMATURE')

    def execute(self, context):
        FixBoneRoll(context)
        return {'FINISHED'}

menu_func = (lambda self, context: self.layout.operator(HingeConstraintOp.bl_idname, text="Hinge constraint"))

def register():
    bpy.types.VIEW3D_MT_edit_armature.append(menu_func)

def unregister():
    bpy.types.VIEW3D_MT_edit_armature.remove(menu_func)

if __name__ == "__main__":
    register()
