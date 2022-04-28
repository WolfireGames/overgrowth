"""
This script makes meshes symmetrical if they are nearly symmetrical
"""

bl_addon_info = {
    'name': 'Mesh: Symmetricalize',
    'author': 'David Rosen',
    'version': '1',
    'blender': (2, 5, 4),
    'location': 'View3D > Specials > Symmetricalize',
    'description': 'This script makes meshes symmetrical if they are nearly symmetrical',
    'warning': '', # used for warning icon and text in addons panel
    'wiki_url': '',
    'tracker_url': '',
    'category': 'Mesh'}
    
import bpy

def SymmetricalizeFunc(context):
    print("Symmetricalizing...")
    obj = context.scene.objects.active
    mesh = obj.data
    
    bpy.ops.object.mode_set(mode='OBJECT')

    min_coord = mesh.vertices[0].co[0]
    for vert in mesh.vertices:
        if(abs(vert.co[0])<abs(min_coord)):
            min_coord = vert.co[0]
        
    print("Old center point was: ", min_coord)

    for vert in mesh.vertices:
        vert.co[0] = vert.co[0] - min_coord

    bucket_scale = 100000

    vert_buckets = {}
    for vert in mesh.vertices:
        if vert.co[0] > 0:
            vert_buckets[int(round(vert.co[0] * bucket_scale))] = vert.index
            
    for vert in mesh.vertices:
        if(vert.co[0] < 0):
            index = int(round(-vert.co[0] * bucket_scale))
            if index in vert_buckets:
                vert.co[0] = mesh.vertices[vert_buckets[index]].co[0] * -1
    
    bpy.ops.object.mode_set(mode='EDIT')

class Symmetricalize(bpy.types.Operator):
    '''Makes meshes symmetrical if they are nearly symmetrical'''
    bl_idname = 'mesh.symmetricalize'
    bl_label = 'Symmetricalize'
    bl_options = {'REGISTER', 'UNDO'}

    @classmethod
    def poll(self, context):
        obj = context.active_object
        return (obj and obj.type == 'MESH')

    def execute(self, context):
        SymmetricalizeFunc(context)
        return {'FINISHED'}

menu_func = (lambda self, context: self.layout.operator(Symmetricalize.bl_idname, text="Symmetricalize"))

def register():
    #bpy.types.register(Symmetricalize)
    bpy.types.VIEW3D_MT_edit_mesh_specials.append(menu_func)
    bpy.types.VIEW3D_MT_edit_mesh_vertices.append(menu_func)

def unregister():
    #bpy.types.unregister(Symmetricalize)
    bpy.types.VIEW3D_MT_edit_mesh_specials.remove(menu_func)
    bpy.types.VIEW3D_MT_edit_mesh_vertices.remove(menu_func)

if __name__ == "__main__":
    register()
