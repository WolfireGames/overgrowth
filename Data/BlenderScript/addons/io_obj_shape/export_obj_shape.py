import bpy
from bpy.props import *

def Save(filepath):
    return

class ObjShapeExporter(bpy.types.Operator):
    '''Save shape key as OBJ'''
    bl_idname = "export_obj_shape.obj"
    bl_label = "Export shape key as OBJ"

    filepath = StringProperty(name="File Path", description="Filepath used for exporting the OBJ file", maxlen= 1024, default= "")
    
    check_existing = BoolProperty(name="Check Existing", description="Check and warn on overwriting existing files", default=True, options={'HIDDEN'})

    def execute(self, context):
        Save(self.properties.filepath)
        return {'FINISHED'}

    def invoke(self, context, event):
        wm = context.window_manager
        wm.add_fileselect(self)
        return {'RUNNING_MODAL'}
        
#Save("C:\\Users\\David\\Desktop\\export.anm")
#BakeConstraints(bpy.context.scene.objects.active)
