from mathutils import Matrix, Vector
import math

"""
This script adds a Phoenix fixed constraint
"""

bl_addon_info = {
    'name': 'Armature: Add fixed constraint',
    'author': 'David Rosen',
    'version': '1',
    'blender': (2, 5, 4),
    'location': 'Armature > Fixed constraint',
    'description': 'This script adds a Phoenix fixed constraint',
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
    
    constraint = pose_bone.constraints.new('LIMIT_ROTATION')
    constraint.min_x = 0
    constraint.max_x = 0
    constraint.min_y = 0
    constraint.max_y = 0
    constraint.min_z = 0
    constraint.max_z = 0
    constraint.use_limit_x = True
    constraint.use_limit_y = True
    constraint.use_limit_z = True
    constraint.name = "RGDL_Limit Rotation" 
    constraint.influence = 0.0
    constraint.owner_space = 'LOCAL'

class FixedConstraintOp(bpy.types.Operator):
    '''Add fixed constraint'''
    bl_idname = 'armature.fixedconstraint'
    bl_label = 'Add fixed constraint'
    bl_options = {'REGISTER', 'UNDO'}

    @classmethod
    def poll(self, context):
        obj = context.active_object
        return (obj and obj.type == 'ARMATURE')

    def execute(self, context):
        FixBoneRoll(context)
        return {'FINISHED'}

menu_func = (lambda self, context: self.layout.operator(FixedConstraintOp.bl_idname, text="Fixed constraint"))

def register():
    bpy.types.VIEW3D_MT_edit_armature.append(menu_func)

def unregister():
    bpy.types.VIEW3D_MT_edit_armature.remove(menu_func)

if __name__ == "__main__":
    register()
