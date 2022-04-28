bl_addon_info = {
    'name': 'Import/Export: Objects as shape keys(.obj)...',
    'author': 'David Rosen',
    'version': '0.1',
    'blender': (2, 5, 4),
    'location': 'File > Import/Export > OBJ shape key',
    'description': 'Import shape key (.obj format)',
    'warning': '', # used for warning icon and text in addons panel
    'category': 'Import/Export'}

import bpy

def menu_import(self, context):
    from io_obj_shape import import_obj_shape
    self.layout.operator(import_obj_shape.ObjShapeImporter.bl_idname, text="Shape key (.obj)").filepath = "*.obj"
    
def menu_export(self, context):
    from io_obj_shape import export_obj_shape
    import os
    default_path = os.path.splitext(bpy.data.filepath)[0] + ".obj"
    self.layout.operator(export_obj_shape.ObjShapeExporter.bl_idname, text="Shape key (.obj)").filepath = default_path
    
def register():
    from io_obj_shape import import_obj_shape, export_obj_shape
    bpy.types.register(import_obj_shape.ObjShapeImporter)
    #bpy.types.register(export_obj_shape.ObjShapeExporter)
    bpy.types.INFO_MT_file_import.append(menu_import)
    #bpy.types.INFO_MT_file_export.append(menu_export)

def unregister():
    from io_obj_shape import import_obj_shape, export_obj_shape
    #bpy.types.unregister(import_obj_shape.ObjShapeImporter)
    #bpy.types.unregister(export_obj_shape.ObjShapeExporter)
    bpy.types.INFO_MT_file_import.remove(menu_import)
    #bpy.types.INFO_MT_file_export.remove(menu_export)
    
if __name__ == "__main__":
    register()
