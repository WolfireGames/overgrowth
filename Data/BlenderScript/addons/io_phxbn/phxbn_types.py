import array

_hinge_joint = 0
_amotor_joint = 1
_fixed_joint = 2

class IKBone:
    def __init__(self):
        self.name = ""
        self.bone = 0
        self.chain = 0

def Get4ByteIntArray():
    var = array.array('l')
    if var.itemsize != 4:
        var = array.array('i')
    return var

class Joint:
    def __init__(self):
        self.type = Get4ByteIntArray()
        self.stop_angles = array.array('f')
        self.bone_ids = Get4ByteIntArray()
        self.axis = array.array('f')

class PHXBNdata:
    def __init__(self):
        self.vertices = []
        self.bones = []
        self.parents = Get4ByteIntArray()
        self.bone_parents = Get4ByteIntArray()
        self.bone_weights = array.array('f')
        self.bone_ids = array.array('f')
        self.bone_swap = Get4ByteIntArray()
        self.bone_mass = array.array('f')
        self.bone_com = array.array('f')
        self.rigging_stage = Get4ByteIntArray()
        self.version = Get4ByteIntArray()
        self.parent_ids = Get4ByteIntArray()
        self.joints = []
        self.bone_mat = array.array('f')
        self.ik_bones = []
