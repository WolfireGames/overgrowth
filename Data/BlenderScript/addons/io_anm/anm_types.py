import array

def Get4ByteIntArray():
    var = array.array('l')
    if var.itemsize != 4:
        var = array.array('i')
    return var
    
class ShapeKey:
    def __init__(self):
        self.weight = 0.0
        self.string = "null"
    def __repr__(self):
        return "(\""+self.string+"\", "+str(self.weight)+")"
        
class StatusKey:
    def __init__(self):
        self.weight = 0.0
        self.string = "null"
    def __repr__(self):
        return "(\""+self.string+"\", "+str(self.weight)+")"
        
class IKBone:
    def __init__(self):
        self.start = array.array('f')
        self.end = array.array('f')
        self.bone_path = Get4ByteIntArray()
        self.string = "null"
    def __repr__(self):
        return "(\""+self.string+"\", "+str(self.bone_path)+")"

class Event:
    def __init__(self):
        self.which_bone = 0
        self.string = ""
    def __repr__(self):
        return "(\""+self.string+"\", "+str(self.which_bone)+")"


class Keyframe:
    def __init__(self):
        self.time = 0
        self.weights = array.array('f')
        self.mobility_mat = array.array('f')
        self.mats = []
        self.weapon_mats = []
        self.weap_relative_ids = []
        self.weap_relative_weights = []
        self.events = []
        self.ik_bones = []
        self.shape_keys = []
        self.status_keys = []
    def __repr__(self):
        return "("+str(self.time)+", "+str(len(self.mats))+")"

class ANMdata:
    def __init__(self):
        self.version = 0
        self.looping = 0
        self.start = 0
        self.end = 0
        self.keyframes = []
        self.name = ""
        
