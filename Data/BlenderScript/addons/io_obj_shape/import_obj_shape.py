import bpy
from bpy.props import *
from io_scene_obj.import_obj import load as load_obj

def Load(filepath):
    obj = bpy.context.scene.objects.active

    obj_dict = {}
    for object in bpy.context.scene.objects:
        obj_dict[object.name] = True

    load_obj(None, bpy.context, filepath)

    new_obj = []
    for object in bpy.context.scene.objects:
        if not object.name in obj_dict:
            new_obj.append(object)

    new_obj[0].name = filepath.split('\\')[-1].split('.')[0]
    new_obj[0].select = True
    bpy.ops.object.join_shapes()

    bpy.ops.object.select_all(action = 'DESELECT')
    for object in new_obj:
        object.select = True
    bpy.ops.object.delete()
    
class ObjShapeImporter(bpy.types.Operator):
    '''Load a Wavefront OBJ file as a shape key'''
    bl_idname = "import_obj_shape.obj"
    bl_label = "Import OBJ shape key"

    filepath = StringProperty(name="File Path", description="Filepath used for importing the OBJ file", maxlen=1024, default="")

    def execute(self, context):
        print("Filepath:",self.properties.filepath)
        Load(self.properties.filepath)

        return {'FINISHED'}

    def invoke(self, context, event):
        wm = context.window_manager
        wm.add_fileselect(self)
        return {'RUNNING_MODAL'}

#Load("C:\\Users\\David\\Desktop\\Wolfire SVN\\Project\\Data\\Animations\\run.anm")
#Load("C:\\Users\\David\\Desktop\\export.anm")
