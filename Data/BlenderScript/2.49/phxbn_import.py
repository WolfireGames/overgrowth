#!BPY

"""
Name: 'Phoenix bones (.phxbn)...'
Blender: 249
Group: 'Import'
Tip: 'Import a (.phxbn) skeleton file'
"""

__author__ = "Campbell Barton"
__url__ = ("blender.org", "blenderartists.org")
__version__ = "1.90 06/08/01"

__bpydoc__ = """\
This script imports PHXBN skeleton files to Blender.
"""

import Blender
import bpy
import BPyMessages
import array
Vector= Blender.Mathutils.Vector
Euler= Blender.Mathutils.Euler
Matrix= Blender.Mathutils.Matrix
RotationMatrix = Blender.Mathutils.RotationMatrix
TranslationMatrix= Blender.Mathutils.TranslationMatrix

DEG2RAD = 0.017453292519943295

class PHXBNdata:
	def __init__(self):
		self.vertices = []
		self.bones = []
		self.parents = []
		self.bone_parents = []
		self.bone_weights = array.array('f')
		self.bone_ids = array.array('f')

def ReadPHXBN(file_path, mesh_obj):
	# File loading stuff
	# Open the file for importing
	file = open(file_path, 'rb')	
	
	print "\nLoading phxbn file: ", file_path
	version = array.array('l')
	version.fromfile(file, 1)
	print 'Version: ', version[0], '\n'
	
	rigging_stage = array.array('l')
	rigging_stage.fromfile(file, 1)
	#print 'Rigging stage: ', rigging_stage[0], '\n'
	
	num_points = array.array('l')
	num_points.fromfile(file, 1)
	#print 'Num points: ', num_points[0], '\n'
	
	data = PHXBNdata();
	
	for i in range(0,num_points[0]):
		vertex = array.array('f')
		vertex.fromfile(file, 3)
		data.vertices.append(vertex)
	
	for i in range(0,num_points[0]):
		parent = array.array('l')
		parent.fromfile(file, 1)
		data.parents.append(parent[0])
	
	num_bones = array.array('l')
	num_bones.fromfile(file, 1)
	#print 'Num bones: ', num_bones[0], '\n'
	
	for i in range(0,num_bones[0]):
		bone = array.array('l')
		bone.fromfile(file, 2)
		data.bones.append(bone)
		
	for i in range(0,num_bones[0]):
		bone_parent = array.array('l')
		bone_parent.fromfile(file, 1)
		data.bone_parents.append(bone_parent[0])
	
	bone_mass = array.array('f')
	bone_mass.fromfile(file, num_bones[0])
	
	bone_com = array.array('f')
	bone_com.fromfile(file, num_bones[0]*3)
	
	print bone_com[0]
	
	mesh = mesh_obj.getData()
	
	if rigging_stage[0] == 1:
		num_faces = len(mesh.faces)
		num_verts = num_faces * 3
		print "Loading ", num_verts, " bone weights and ids"
		file_bone_weights = array.array('f')
		file_bone_ids = array.array('f')
		file_bone_weights.fromfile(file, num_verts*4)
		file_bone_ids.fromfile(file, num_verts*4)
		
		num_verts = len(mesh.verts)
		data.bone_weights = [0 for i in xrange(num_verts*4)]
		data.bone_ids = [0 for i in xrange(num_verts*4)]
		for face_id in xrange(num_faces):
			for face_vert_num in xrange(3):
				for i in xrange(4):
					data.bone_weights[mesh.faces[face_id].v[face_vert_num].index*4+i] =file_bone_weights[face_id*12 + face_vert_num * 4 + i]
					data.bone_ids[mesh.faces[face_id].v[face_vert_num].index*4+i] =file_bone_ids[face_id*12 + face_vert_num * 4 + i]
		
	#print 'Vertices: ', data.vertices, '\n'
	
	min_vec = Vector(mesh.verts[0].co)
	max_vec = Vector(mesh.verts[0].co)
	for vert in mesh.verts:
		min_vec[0] = min(min_vec[0], vert.co[0])
		min_vec[1] = min(min_vec[1], vert.co[1])
		min_vec[2] = min(min_vec[2], vert.co[2])
		max_vec[0] = max(max_vec[0], vert.co[0])
		max_vec[1] = max(max_vec[1], vert.co[1])
		max_vec[2] = max(max_vec[2], vert.co[2])
	
	Blender.Window.EditMode(0)
	center = (min_vec + max_vec)*0.5
	print min_vec
	print max_vec
	print center
	for vert in mesh.verts:
		vert.co[0] = vert.co[0] - center[0]
		vert.co[1] = vert.co[1] - center[1]
		vert.co[2] = vert.co[2] - center[2]
	mesh.update()
	
	return data


#=============#
# TESTING     #
#=============#

#('/metavr/mocap/bvh/boxer.bvh')
#('/d/staggered_walk.bvh')
#('/metavr/mocap/bvh/dg-306-g.bvh') # Incompleate EOF
#('/metavr/mocap/bvh/wa8lk.bvh') # duplicate joint names, \r line endings.
#('/metavr/mocap/bvh/walk4.bvh') # 0 channels

'''
import os
DIR = '/metavr/mocap/bvh/'
for f in ('/d/staggered_walk.bvh',):
	#for f in os.listdir(DIR)[5:6]:
	#for f in os.listdir(DIR):
	if f.endswith('.bvh'):
		s = Blender.Scene.New(f)
		s.makeCurrent()
		#file= DIR + f
		file= f
		print f
		bvh_nodes= read_bvh(file, 1.0)
		bvh_node_dict2armature(bvh_nodes, 1)
'''

def UI(file, PREF_UI= True):
	
	if BPyMessages.Error_NoFile(file):
		return
	
	Draw= Blender.Draw
	
	print 'Attempting import PHXBN:\n', file, '\n'
	
	Blender.Window.WaitCursor(1)
	# Get the BVH data and act on it.
	t1= Blender.sys.time()
	print 'Parsing phxbn...\n',
	data = ReadPHXBN(file)
	print '%.4f' % (Blender.sys.time()-t1)
	t1= Blender.sys.time()
	'''
	print '\timporting to blender...',
	if IMPORT_AS_ARMATURE:	bvh_node_dict2armature(bvh_nodes, IMPORT_START_FRAME, IMPORT_LOOP)
	if IMPORT_AS_EMPTIES:	bvh_node_dict2objects(bvh_nodes,  IMPORT_START_FRAME, IMPORT_LOOP)
	'''
	print 'Done in %.4f\n' % (Blender.sys.time()-t1)
	Blender.Window.WaitCursor(0)

def AddArmature(data, mesh_obj):
	scn = bpy.data.scenes.active
	arm_data = bpy.data.armatures.new()
	arm_obj = scn.objects.new(arm_data)
	arm_data.makeEditable()
	
	num = 0
	for data_bone in data.bones:
		bone = Blender.Armature.Editbone()
		bone.head = Vector(data.vertices[data_bone[0]][0],
					 data.vertices[data_bone[0]][1],
					 data.vertices[data_bone[0]][2])
		bone.tail = Vector(data.vertices[data_bone[1]][0],
					 data.vertices[data_bone[1]][1],
					 data.vertices[data_bone[1]][2])
		arm_data.bones["Bone_"+str(num)] = bone
		num = num + 1
		
		
	num = 0
	for bone_parent in data.bone_parents:
		name = "Bone_"+str(num);
		parent_name = "Bone_"+str(bone_parent)
		if bone_parent != -1:
			arm_data.bones[name].parent = arm_data.bones[parent_name]
		num = num + 1
		
	arm_data.update()
	arm_obj.link(arm_data)
	
	vertgroup_created=[]
	for bone in data.bones:
		vertgroup_created.append(0)

	mesh = mesh_obj.getData()
	index = 0
	vert_index = 0
	for vert in mesh.verts:
		for bone_num in range(0,4):
			bone_id = int(data.bone_ids[index])
			bone_weight = data.bone_weights[index]
			name = "Bone_"+str(bone_id);
			if vertgroup_created[bone_id]==0:
				vertgroup_created[bone_id]=1
				mesh.addVertGroup(name)
			#assign the weight for this vertex
			mesh.assignVertsToGroup(name, [vert_index], bone_weight, 'replace')
			index = index + 1
		vert_index = vert_index + 1
	mesh.update()
		
	arm_obj.makeParentDeform([mesh_obj], 0, 0)
	arm_obj.drawMode = Blender.Object.DrawModes.XRAY
		
def ReadObj(path):
	name = path.split('\\')[-1].split('/')[-1]
	mesh = Blender.NMesh.New( name )
	file = open(path, 'r')
	num_verts = 0;
	for line in file:
		words = line.split()
		if len(words) == 0 or words[0].startswith('#'):
			pass
		elif words[0] == 'v':
			x, y, z = float(words[1]), float(words[2]), float(words[3])
			mesh.verts.append(Blender.NMesh.Vert(x, y, z))
			num_verts = num_verts + 1
		elif words[0] == 'f':
			is_good = True;
			faceVertList = []
			for index_group in words[1:]:
				index = index_group.split('/')[0]
				if(int(index) > num_verts):
					is_good = False
				else:
					faceVert = mesh.verts[int(index)-1]
					faceVertList.append(faceVert)
			if is_good:
				newFace = Blender.NMesh.Face(faceVertList)
				mesh.addFace(newFace)
	ob = Blender.Object.New('Mesh', name)
	ob.link(mesh) # tell the object to use the mesh we just made
	scn = Blender.Scene.GetCurrent()	
	scn.link(ob)
	
	return ob
	
def main():
	mesh_obj = ReadObj("C:\\Users\\David\\Desktop\\Wolfire SVN\\Project\\Data\\Models\\Characters\\IGF_Guard\\IGF_Guard.obj");
	phxbn_data = ReadPHXBN("C:\\Users\\David\\Desktop\\Wolfire SVN\\Project\\Data\\Skeletons\\test.phxbn", mesh_obj);
	AddArmature(phxbn_data, mesh_obj)
	
	Blender.Redraw()
	#Blender.Window.FileSelector(UI, 'Import PHXBN', '*.phxbn')

if __name__ == '__main__':
	#def foo():
	main()