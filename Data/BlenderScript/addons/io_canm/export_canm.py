import bpy
import array
from bpy.props import *
from mathutils import Matrix, Euler, Vector, Quaternion
    
class dfcurvekeyclass:
    def __init__(self):
        self.co = array.array('f')
        self.handle_left = array.array('f')
        self.handle_right = array.array('f')

class fcurveclass:
    def __init__(self):
        self.data_path = ""
        self.fcurvekey = []
        self.array_index = 0
        
class CANMdata:
    def __init__(self):
        self.version = 0
        self.rotation_mode = ""
        self.fcurves = []
        self.fps = 0
        
def WriteFloat(file, val):
    int_loader = array.array('f')
    int_loader.append(val)
    int_loader.tofile(file)
 
def Get4ByteIntArray():
    var = array.array('l')
    if var.itemsize != 4:
        var = array.array('i')
    return var
    
def WriteInt(file, val):
    int_loader = Get4ByteIntArray()
    int_loader.append(val)
    int_loader.tofile(file)
    
def WriteBool(file, val):
    bool_loader = array.array('B')
    bool_loader.append(val)
    bool_loader.tofile(file)
    
def WriteString(file, str):
    string_array = array.array("B")
    string_array.fromstring(str.encode("utf-8"))
    WriteInt(file, len(str))
    string_array.tofile(file)
    
def WriteCANM(filepath, data):
    file = open(filepath, "wb")

    data.version = 1
    num_fcurves = len(data.fcurves)
    WriteInt(file, data.version)
    WriteInt(file, data.fps)
    WriteString(file, data.rotation_mode)
    WriteInt(file, num_fcurves)
    
    for fcurve in data.fcurves:
        WriteString(file, fcurve.data_path)
        WriteInt(file, fcurve.array_index)
        WriteInt(file, len(fcurve.fcurvekey))
        for key in fcurve.fcurvekey:
            key.co.tofile(file)
            key.handle_left.tofile(file)
            key.handle_right.tofile(file)
            
def GetAnimation():
    cam_obj = bpy.context.scene.objects.active
    if cam_obj and cam_obj.type != 'CAMERA':
        print("Camera must be selected")
        return 0
    fcurves = []
    if cam_obj.animation_data.action:
        for fcurve in cam_obj.animation_data.action.fcurves:
            fcurves.append(fcurve)
    if cam_obj.data.animation_data.action:
        for fcurve in cam_obj.data.animation_data.action.fcurves:
            fcurves.append(fcurve)
            
    fcurves_c = []
    for fcurve in fcurves:
        #print(fcurve.data_path)
        fcurve_c = fcurveclass()
        fcurve_c.data_path = fcurve.data_path
        fcurve_c.array_index = fcurve.array_index
        for keyframe in fcurve.keyframe_points:
            keyframe_c = dfcurvekeyclass()
            keyframe_c.co.append(keyframe.co[0])
            keyframe_c.co.append(keyframe.co[1])
            keyframe_c.handle_left.append(keyframe.handle_left[0])
            keyframe_c.handle_left.append(keyframe.handle_left[1])
            keyframe_c.handle_right.append(keyframe.handle_right[0])
            keyframe_c.handle_right.append(keyframe.handle_right[1])
            fcurve_c.fcurvekey.append(keyframe_c)
        fcurves_c.append(fcurve_c)
    
    data = CANMdata()
    data.fcurves = fcurves_c
    data.rotation_mode = cam_obj.rotation_mode
    data.fps = bpy.context.scene.render.fps
    
    return data

def Save(filepath):
    data = GetAnimation()
    if not data:
        return
    WriteCANM(filepath, data)

class CANMExporter(bpy.types.Operator):
    '''Save Phoenix Bones armature'''
    bl_idname = "export_mesh.canm"
    bl_label = "Export CANM"

    filepath = StringProperty(name="File Path", description="Filepath used for exporting the CANM file", maxlen= 1024, default= "")
    
    check_existing = BoolProperty(name="Check Existing", description="Check and warn on overwriting existing files", default=True, options={'HIDDEN'})

    def execute(self, context):
        Save(self.properties.filepath)
        return {'FINISHED'}

    def invoke(self, context, event):
        wm = context.window_manager
        wm.add_fileselect(self)
        return {'RUNNING_MODAL'}
        
#Save("C:\\Users\\David\\Desktop\\export.anm")
#Save("Desktop/export.anm")
#BakeConstraints(bpy.context.scene.objects.active)
data = GetAnimation()
WriteCANM("C:\\Users\\David\\Desktop\\test.canm", data)