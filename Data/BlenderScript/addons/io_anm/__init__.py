bl_addon_info = {
    'name': 'Import/Export: Phoenix animation (.anm)...',
    'author': 'David Rosen',
    'version': '0.1',
    'blender': (2, 5, 4),
    'location': 'File > Import/Export > ANM',
    'description': 'Import Phoenix animation (.anm format)',
    'warning': '', # used for warning icon and text in addons panel
    'category': 'Import/Export'}

import bpy

def menu_import(self, context):
    from io_anm import import_anm
    self.layout.operator(import_anm.ANMImporter.bl_idname, text="Phoenix Animation (.anm)").filepath = "*.anm"
    
def menu_export(self, context):
    from io_anm import export_anm
    import os
    default_path = os.path.splitext(bpy.data.filepath)[0] + ".anm"
    self.layout.operator(export_anm.ANMExporter.bl_idname, text="Phoenix Animation (.anm)").filepath = default_path
    
def register():
    from io_anm import import_anm, export_anm
    bpy.types.register(import_anm.ANMImporter)
    bpy.types.register(export_anm.ANMExporter)
    bpy.types.INFO_MT_file_import.append(menu_import)
    bpy.types.INFO_MT_file_export.append(menu_export)

def unregister():
    from io_anm import import_anm, export_anm
    #bpy.types.unregister(import_anm.ANMImporter)
    #bpy.types.unregister(export_anm.ANMExporter)
    bpy.types.INFO_MT_file_import.remove(menu_import)
    bpy.types.INFO_MT_file_export.remove(menu_export)
    
if __name__ == "__main__":
    register()
