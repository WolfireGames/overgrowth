bl_addon_info = {
    'name': 'Import/Export: Phoenix camera animation (.canm)...',
    'author': 'David Rosen',
    'version': '0.1',
    'blender': (2, 5, 4),
    'location': 'File > Import/Export > CANM',
    'description': 'Export Phoenix camera animation (.canm format)',
    'warning': '', # used for warning icon and text in addons panel
    'category': 'Import/Export'}

import bpy

def menu_export(self, context):
    from io_canm import export_canm
    import os
    default_path = os.path.splitext(bpy.data.filepath)[0] + ".canm"
    self.layout.operator(export_canm.CANMExporter.bl_idname, text="Phoenix Camera Animation (.canm)").filepath = default_path
    
def register():
    from io_canm import export_canm
    bpy.types.register(export_canm.CANMExporter)
    bpy.types.INFO_MT_file_export.append(menu_export)

def unregister():
    from io_canm import export_canm
    bpy.types.INFO_MT_file_export.remove(menu_export)
    
if __name__ == "__main__":
    register()
