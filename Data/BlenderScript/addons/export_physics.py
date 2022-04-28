import bpy
import struct
from bpy.props import *

def GetWriteStream():
    file_bytes = []
    for obj in bpy.data.objects:
        if obj.select:
            obj_bytes = bytes()
            obj_bytes += struct.pack("i",len(obj.name))
            for c in obj.name:
                obj_bytes += struct.pack("c", c)
            obj.rotation_mode = 'QUATERNION'
            q = obj.rotation_quaternion
            obj_bytes += struct.pack("ffff", q[0], q[1], q[2], q[3])
            l = obj.location
            obj_bytes += struct.pack("fff", l[0], l[1], l[2])
            s = obj.scale
            obj_bytes += struct.pack("fff", s[0], s[1], s[2])
            file_bytes.append(obj_bytes)
    
    write_file_bytes = bytes()
    write_file_bytes += struct.pack("BBBBBBBB", 211, ord('F'), ord('Z'), ord('X'), ord('\r'), ord('\n'), 32, ord('\n')) #Add file identification
    write_file_bytes += struct.pack("i", 1) #Add version
    write_file_bytes += struct.pack("i", len(file_bytes)) #Add number of objects
    for obj_bytes in file_bytes:
        write_file_bytes += obj_bytes #Add contents
    write_file_bytes += struct.pack("i", len(write_file_bytes)) #Add size of file
    write_file_bytes += struct.pack("BBB", ord('F'), ord('Z'), ord('X')) #Add file end marker
    return write_file_bytes

def WriteFZX(filepath, data):
    file = open(filepath, "wb")
    file.write(data)
    file.close()

def Save(filepath):
    data = GetWriteStream()
    if not data:
        return
    WriteFZX(filepath, data)

class WolfireFZXExporter(bpy.types.Operator):
    '''Save Phoenix bone physics primitives'''
    bl_idname = "export_physics.fzx"
    bl_label = "Export FZX"

    filepath = StringProperty(name="File Path", description="Filepath used for exporting the FZX file", maxlen= 1024, default= "")
    
    check_existing = BoolProperty(name="Check Existing", description="Check and warn on overwriting existing files", default=True, options={'HIDDEN'})

    def execute(self, context):
        Save(self.properties.filepath)
        return {'FINISHED'}

    def invoke(self, context, event):
        wm = context.window_manager
        wm.add_fileselect(self)
        return {'RUNNING_MODAL'}

bl_addon_info = {
    'name': 'Import/Export: Phoenix collision physics (.fzx)...',
    'author': 'David Rosen',
    'version': '0.1',
    'blender': (2, 5, 5),
    'location': 'File > Import/Export > FZX',
    'description': 'Export Phoenix collision physics (.fzx format)',
    'warning': '', # used for warning icon and text in addons panel
    'category': 'Import/Export'}

import bpy

def menu_export(self, context):
    import os
    default_path = os.path.splitext(bpy.data.filepath)[0] + ".fzx"
    self.layout.operator(WolfireFZXExporter.bl_idname, text="Phoenix Collision Physics (.fzx)").filepath = default_path
    
def register():
    bpy.types.INFO_MT_file_export.append(menu_export)

def unregister():
    bpy.types.INFO_MT_file_export.remove(menu_export)
    
if __name__ == "__main__":
    register()
