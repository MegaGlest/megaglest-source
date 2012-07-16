# -*- coding: utf-8 -*-

###########################################################################
# Glest Model / Texture / UV / Animation Importer and Exporter
# for the Game Glest that u can find http://www.glest.org
# copyright 2005 By Andreas Becker (seltsamuel@yahoo.de)
#
# 2011/05/25: v0.1 alpha1
# modified by William Zheng for Blender 2.57(loveheaven_zhengwei@hotmail.com)
#
# corrected by MrPostiga for Blender 2.58
#
# extended by Yggdrasil
#
# Started Date: 07 June 2005  Put Public 20 June 2005   
#  Distributed under the GNU PUBLIC LICENSE
#"""
#Here an explanation of the V4 Format found at www.glest.org
#================================
#1. DATA TYPES
#================================
#G3D files use the following data types:
#uint8: 8 bit unsigned integer
#uint16: 16 bit unsigned integer
#uint32: 32 bit unsigned integer
#float32: 32 bit floating point
#================================
#2. OVERALL STRUCTURE
#================================
#- File header
#- Model header
#- Mesh header
#- Texture names
#- Mesh data
#================================
#2. FILE HEADER
#================================
#Code:
#struct FileHeader{
#   uint8 id[3];
#   uint8 version;
#};
#This header is shared among all the versions of G3D, it identifies this file as a G3D model and provides information of the version.
#id: must be "G3D"
#version: must be 4, in binary (not '4')
#================================
#3. MODEL HEADER
#================================
#Code:
#struct ModelHeader{
#   uint16 meshCount;
#   uint8 type;
#};		   
#meshCount: number of meshes in this model
#type: must be 0
#================================
#4. MESH HEADER
#================================
#There is a mesh header for each mesh, there must be "meshCount" headers in a file but they are not consecutive, texture names and mesh data are stored in between.
#Code:
#struct MeshHeader{
#   uint8 name[64];
#   uint32 frameCount;
#   uint32 vertexCount;
#   uint32 indexCount;
#   float32 diffuseColor[3];
#   float32 specularColor[3];
#   float32 specularPower;
#   float32 opacity;
#   uint32 properties;
#   uint32 textures;
#};
#name: name of the mesh
#frameCount: number of keyframes in this mesh
#vertexCount: number of vertices in each frame
#indexCount: number of indices in this mesh (the number of triangles is indexCount/3)
#diffuseColor: RGB diffuse color
#specularColor: RGB specular color (currently unused)
#specularPower: specular power (currently unused)
##properties: property flags
#Code:
#enum MeshPropertyFlag{
#		mpfCustomColor= 1,
#		mpfTwoSided= 2
#};
#mpfTwoSided: meshes in this mesh are rendered by both sides, if this flag is not present only "counter clockwise" faces are rendered
#mpfCustomColor: alpha in this model is replaced by a custom color, usually the player color
#textures: texture flags, only 0x1 is currently used, indicating that there is a diffuse texture in this mesh.
#================================
#4. TEXTURE NAMES
#================================
#A list of uint8[64] texture name values. One for each texture in the mesh. If there are no textures in the mesh no texture names are present. In practice since Glest only uses 1 texture for each mesh the number of texture names should be 0 or 1.
#================================
#5. MESH DATA
#================================
#After each mesh header and texture names the mesh data is placed.
#vertices: frameCount * vertexCount * 3, float32 values representing the x, y, z vertex coords for all frames
#normals: frameCount * vertexCount * 3, float32 values representing the x, y, z normal coords for all frames
#texture coords: vertexCount * 2, float32 values representing the s, t tex coords for all frames (only present if the mesh has 1 texture at least)
#indices: indexCount, uint32 values representing the indices
###########################################################################

bl_info = {
	"name": "G3D Mesh Import/Export",
	"author": "various, see head of script",
	"version": (0, 5, 1),
	"blender": (2, 63, 0),
	#"api": 36079,
	"location": "File > Import-Export",
	"description": "Import/Export .g3d file",
	"warning": "",
	"wiki_url": "",
	"tracker_url": "",
	"category": "Import-Export"}
###########################################################################
# Importing Structures needed (must later verify if i need them really all)
###########################################################################
import bpy
from bpy.props import StringProperty
from bpy_extras.image_utils import load_image
from bpy_extras.io_utils import ImportHelper, ExportHelper

import sys, struct, string, types
from types import *
import os
from os import path
from os.path import dirname
###########################################################################
# Variables that are better Global to handle
###########################################################################
imported = []   #List of all imported Objects
toexport = []   #List of Objects to export (actually only meshes)
sceneID  = None #Points to the active Blender Scene
def unpack_list(list_of_tuples):
	l = []
	for t in list_of_tuples:
		l.extend(t)
	return l
###########################################################################
# Declaring Structures of G3D Format
###########################################################################
class G3DHeader:							#Read first 4 Bytes of file should be G3D + Versionnumber
	binary_format = "<3cB"
	def __init__(self, fileID):
		temp = fileID.read(struct.calcsize(self.binary_format))
		data = struct.unpack(self.binary_format,temp)
		self.id = str(data[0]+data[1]+data[2], "utf-8")
		self.version = data[3]

class G3DModelHeaderv3:					 #Read Modelheader in V3 there is only the number of Meshes in file
	binary_format = "<I"
	def __init__(self, fileID):
		temp = fileID.read(struct.calcsize(self.binary_format))
		data = struct.unpack(self.binary_format,temp)
		self.meshcount = data[0]

class G3DModelHeaderv4:					 #Read Modelheader: Number of Meshes and Meshtype (must be 0)
	binary_format = "<HB"
	def __init__(self, fileID):
		temp = fileID.read(struct.calcsize(self.binary_format))
		data = struct.unpack(self.binary_format,temp)
		self.meshcount = data[0]
		self.mtype = data[1]
		
class G3DMeshHeaderv3:										 #Read Meshheader
	binary_format = "<7I64c"
	def __init__(self,fileID):
		temp = fileID.read(struct.calcsize(self.binary_format))
		data = struct.unpack(self.binary_format,temp)
		self.framecount = data[0]				 #Framecount = Number of Animationsteps
		self.normalframecount= data[1]		 #Number of Normal Frames actualli equal to Framecount
		self.texturecoordframecount= data[2]#Number of Frames of Texturecoordinates seems everytime to be 1
		self.colorframecount= data[3]		  #Number of Frames of Colors seems everytime to be 1
		self.vertexcount= data[4]				  #Number of Vertices in each Frame
		self.indexcount= data[5]					   #Number of Indices in Mesh (Triangles = Indexcount/3)
		self.properties= data[6]					   #Property flags
		if self.properties & 1:					  #PropertyBit is Mesh Textured ?
			self.hastexture = False
			self.texturefilename = None
		else:
			self.texturefilename = "".join([str(x, "ascii") for x in data[7:-1] if x[0]< 127 ])
			self.hastexture = True
		if self.properties & 2:					  #PropertyBit is Mesh TwoSided ?
			self.istwosided = True
		else:
			self.istwosided = False
		if self.properties & 4:					  #PropertyBit is Mesh Alpha Channel custom Color in Game ?
			self.customalpha = True
		else:
			self.customalpha = False
		
class G3DMeshHeaderv4:										 #Read Meshheader
	binary_format = "<64c3I8f2I"
	texname_format = "<64c"
	def __init__(self,fileID):
		temp = fileID.read(struct.calcsize(self.binary_format))
		data = struct.unpack(self.binary_format,temp)
		self.meshname = "".join([str(x, "ascii") for x in data[0:64] if x[0] < 127 ]) #Name of Mesh every Char is a  String on his own
		self.framecount = data[64]				#Framecount = Number of Animationsteps
		self.vertexcount = data[65]			   #Number of Vertices in each Frame
		self.indexcount = data[66]				#Number of Indices in Mesh (Triangles = Indexcount/3)
		self.diffusecolor = data[67:70]	   #RGB diffuse color
		self.specularcolor = data[70:73]	  #RGB specular color (unused)
		self.specularpower = data[73]		 #Specular power (unused)
		self.opacity = data[74]					   #Opacity
		self.properties= data[75]				  #Property flags
		self.textures = data[76]					  #Texture flag
		if self.properties & 1:					  #PropertyBit is Mesh Alpha Channel custom Color in Game ?
			self.customalpha = True
		else:
			self.customalpha = False
		if self.properties & 2:					  #PropertyBit is Mesh TwoSided ?
			self.istwosided = True
		else:
			self.istwosided = False
		if self.textures & 1:						#PropertyBit is Mesh Textured ?
			temp = fileID.read(struct.calcsize(self.texname_format))
			data = struct.unpack(self.texname_format,temp)
			self.texturefilename = "".join([str(x, "ascii") for x in data[0:-1] if x[0] < 127 ])
			self.hastexture = True
		else:
			self.hastexture = False
			self.texturefilename = None

class G3DMeshdataV3:											   #Calculate and read the Mesh Datapack
	def __init__(self,fileID,header):
		#Calculation of the Meshdatasize to load because its variable
		#Animationframes * Vertices per Animation * 3 (Each Point are 3 Float X Y Z Coordinates)
		vertex_format = "<%if" % int(header.framecount * header.vertexcount * 3)
		#The same for Normals
		normals_format = "<%if" % int(header.normalframecount * header.vertexcount * 3)
		#Same here but Textures are 2D so only 2 Floats needed for Position inside Texture Bitmap
		texturecoords_format = "<%if" % int(header.texturecoordframecount * header.vertexcount * 2)
		#Colors in format RGBA
		colors_format = "<%if" % int(header.colorframecount * 4)
		#Indices
		indices_format = "<%iI" % int(header.indexcount)
		#Load the Meshdata as calculated above
		self.vertices = struct.unpack(vertex_format,fileID.read(struct.calcsize(vertex_format)))
		self.normals = struct.unpack(normals_format,fileID.read(struct.calcsize(normals_format)))
		self.texturecoords = struct.unpack(texturecoords_format,fileID.read(struct.calcsize(texturecoords_format)))
		self.colors = struct.unpack(colors_format,fileID.read(struct.calcsize(colors_format)))
		self.indices = struct.unpack(indices_format ,fileID.read(struct.calcsize(indices_format)))

class G3DMeshdataV4:											   #Calculate and read the Mesh Datapack
	def __init__(self,fileID,header):
		#Calculation of the Meshdatasize to load because its variable
		#Animationframes * Points (Vertex) per Animation * 3 (Each Point are 3 Float X Y Z Coordinates)
		vertex_format = "<%if" % int(header.framecount * header.vertexcount * 3)
		#The same for Normals
		normals_format = "<%if" % int(header.framecount * header.vertexcount * 3)
		#Same here but Textures are 2D so only 2 Floats needed for Position inside Texture Bitmap
		texturecoords_format = "<%if" % int(header.vertexcount * 2)
		#Indices
		indices_format = "<%iI" % int(header.indexcount)
		#Load the Meshdata as calculated above
		self.vertices = struct.unpack(vertex_format,fileID.read(struct.calcsize(vertex_format)))
		self.normals = struct.unpack(normals_format,fileID.read(struct.calcsize(normals_format)))
		if header.hastexture:
			self.texturecoords = struct.unpack(texturecoords_format,fileID.read(struct.calcsize(texturecoords_format)))
		self.indices = struct.unpack(indices_format ,fileID.read(struct.calcsize(indices_format)))

def createMesh(filename,header,data):						  #Create a Mesh inside Blender
	mesh = bpy.data.meshes.new(header.meshname)		#New Mesh
	meshobj = bpy.data.objects.new(header.meshname+'Object', mesh)	 #New Object for the new Mesh
	scene = bpy.context.scene
	scene.objects.link(meshobj)
	scene.update()
	uvcoords = []
	image = None
	if header.hastexture:												  #Load Texture when assigned
		texturefile = dirname(filename)+os.sep+header.texturefilename
		image = bpy.data.images.load(texturefile) #load_image(texturefile, dirname(filename))
		for x in range(0,len(data.texturecoords),2): #Prepare the UV
			uvcoords.append([data.texturecoords[x],data.texturecoords[x+1]])
	
	vertsCO = []
	vertsNormal = []
	for x in range(0,header.vertexcount*3,3):	   #Get the Vertices and Normals into empty Mesh
		vertsCO.extend([(data.vertices[x],data.vertices[x+1],data.vertices[x+2])])
		vertsNormal.extend([(data.normals[x],data.normals[x+1],data.normals[x+2])])
		#vertsCO.extend([(data.vertices[x+(header.framecount-1)*header.vertexcount*3],data.vertices[x+(header.framecount-1)*header.vertexcount*3+1],data.vertices[x+(header.framecount-1)*header.vertexcount*3+2])])
		#vertsNormal.extend([(data.normals[x+(header.framecount-1)*header.vertexcount*3],data.normals[x+(header.framecount-1)*header.vertexcount*3+1],data.normals[x+(header.framecount-1)*header.vertexcount*3+2])])
	mesh.vertices.add(len(vertsCO))
	mesh.vertices.foreach_set("co", unpack_list(vertsCO))
	mesh.vertices.foreach_set("normal", unpack_list(vertsNormal))
	
	faces = []
	faceuv = []
	for i in range(0,len(data.indices),3):			  #Build Faces into Mesh
		faces.extend([data.indices[i], data.indices[i+1], data.indices[i+2], 0])
		if header.hastexture:	   
			uv = []
			u0 = uvcoords[data.indices[i]][0]
			v0 = uvcoords[data.indices[i]][1]
			uv.append([u0,v0])
			u1 = uvcoords[data.indices[i+1]][0]
			v1 = uvcoords[data.indices[i+1]][1]
			uv.append([u1,v1])
			u2 = uvcoords[data.indices[i+2]][0]
			v2 = uvcoords[data.indices[i+2]][1]
			uv.append([u2,v2])
			faceuv.append([uv,0,0,0])
		else:
			uv = []
			uv.append([0,0])
			uv.append([0,0])
			uv.append([0,0])
			faceuv.append([uv,0,0,0])
	mesh.faces.add(len(faces)//4)
	mesh.faces.foreach_set("vertices_raw", faces)
	mesh.faces.foreach_set("use_smooth", [True] * len(mesh.faces))
	mesh.show_double_sided = header.istwosided
	mesh.update_tag()
	mesh.update()   #Update changes to Mesh
	#===================================================================================================
	#Material Setup
	#===================================================================================================
	if header.hastexture:	   
		materialname = "pskmat"
		materials = []
		texture = bpy.data.textures.new(name=header.texturefilename,type='IMAGE')
		texture.image = image
		matdata = bpy.data.materials.new(materialname + '1')
		slot = matdata.texture_slots.add()
		slot.texture = texture
		slot.texture_coords = 'UV'
		if header.isv4:
			matdata.diffuse_color = (header.diffusecolor[0], header.diffusecolor[1],header.diffusecolor[2])
			matdata.alpha = header.opacity
			matdata.specular_color = (header.specularcolor[0], header.specularcolor[1],header.specularcolor[2])
		matdata.use_face_texture_alpha = header.customalpha
		materials.append(matdata)

		for material in materials:
			#add material to the mesh list of materials
			mesh.materials.append(material)

		countm = 0
		psktexname="psk" + str(countm)
		mesh.uv_textures.new(name=psktexname)
		if (len(faceuv) > 0):
			for countm in range(len(mesh.uv_textures)):
				mesh.update()
				uvtex = mesh.uv_textures[countm] #add one uv texture
				mesh.update()
				for i, face in enumerate(mesh.faces):
					blender_tface = uvtex.data[i] #face
					mfaceuv = faceuv[i]
					if countm == faceuv[i][1]:
						face.material_index = faceuv[i][1]
						blender_tface.uv1 = mfaceuv[0][0] #uv = (0,0)
						blender_tface.uv2 = mfaceuv[0][1] #uv = (0,0)
						blender_tface.uv3 = mfaceuv[0][2] #uv = (0,0)
						blender_tface.image = image
					else:
						blender_tface.uv1 = [0,0]
						blender_tface.uv2 = [0,0]
						blender_tface.uv3 = [0,0]
	mesh.update()
	imported.append(meshobj)			#Add to Imported Objects
	sk = meshobj.shape_key_add()
	#mesh.shape_keys.use_relative = False
	#sk.interpolation = 'KEY_LINEAR'
	for x in range(1,header.framecount):	#Put in Vertex Positions for Keyanimation
		#print("Frame"+str(x))
		sk = meshobj.shape_key_add()
		for i in range(0,header.vertexcount*3,3):
			sk.data[i//3].co[0]= data.vertices[x*header.vertexcount*3 + i]
			sk.data[i//3].co[1]= data.vertices[x*header.vertexcount*3 + i +1]
			sk.data[i//3].co[2]= data.vertices[x*header.vertexcount*3 + i +2]
		#meshobj.keyframe_insert("location",-1, frame=(x+1))
		#meshobj.keyframe_insert(data_path="data.vertices", frame=(x+1))
	mesh.update()
	return
###########################################################################
# Import
###########################################################################
def G3DLoader(filepath):			#Main Import Routine
	global imported, sceneID
	print ("\nNow Importing File: " + filepath)
	fileID = open(filepath,"rb")
	header = G3DHeader(fileID)
	print ("\nHeader ID         : " + header.id)
	print ("Version           : " + str(header.version))
	if header.id != "G3D":
		print ("This is Not a G3D Model File")
		fileID.close
		return
	if header.version not in (3, 4):
		print ("The Version of this G3D File is not Supported")
		fileID.close
		return
	#in_editmode = Blender.Window.EditMode()			 #Must leave Editmode when active
	#if in_editmode: Blender.Window.EditMode(0)
	sceneID = bpy.context.scene						  #Get active Scene
	#scenecontext=sceneID.getRenderingContext()		  #To Access the Start/Endframe its so hidden i searched till i got angry :-)
	basename=os.path.basename(filepath).split('.')[0]   #Generate the Base Filename without Path + extension
	imported = []
	maxframe=0
	if header.version == 3:
		modelheader = G3DModelHeaderv3(fileID)
		print ("Number of Meshes  : " + str(modelheader.meshcount))
		for x in range(modelheader.meshcount):
			meshheader = G3DMeshHeaderv3(fileID)
			meshheader.isv4 = False	
			print ("\nMesh Number         : " + str(x+1))
			print ("framecount            : " + str(meshheader.framecount))
			print ("normalframecount      : " + str(meshheader.normalframecount))
			print ("texturecoordframecount: " + str(meshheader.texturecoordframecount))
			print ("colorframecount       : " + str(meshheader.colorframecount))
			print ("pointcount            : " + str(meshheader.vertexcount))
			print ("indexcount            : " + str(meshheader.indexcount))
			print ("texturename           : " + str(meshheader.texturefilename))
			print ("hastexture            : " + str(meshheader.hastexture))
			print ("istwosided            : " + str(meshheader.istwosided))
			print ("customalpha           : " + str(meshheader.customalpha))
			meshheader.meshname = basename+str(x+1)	 #Generate Meshname because V3 has none
			if meshheader.framecount > maxframe: maxframe = meshheader.framecount #Evaluate the maximal animationsteps
			meshdata = G3DMeshdataV3(fileID,meshheader)
			createMesh(filepath,meshheader,meshdata)
		fileID.close
		bpy.context.scene.frame_start=1
		bpy.context.scene.frame_end=maxframe
		bpy.context.scene.frame_current=1
		anchor = bpy.data.objects.new('Empty', None)
		anchor.select = True
		bpy.context.scene.objects.link(anchor)
		for ob in imported:
				ob.parent = anchor
		bpy.context.scene.update()
		return
	if header.version == 4:
		modelheader = G3DModelHeaderv4(fileID)
		print ("Number of Meshes  : " + str(modelheader.meshcount))
		for x in range(modelheader.meshcount):
			meshheader = G3DMeshHeaderv4(fileID)
			meshheader.isv4 = True	  
			print ("\nMesh Number   : " + str(x+1))
			print ("meshname        : " + str(meshheader.meshname))
			print ("framecount      : " + str(meshheader.framecount))
			print ("vertexcount     : " + str(meshheader.vertexcount))
			print ("indexcount      : " + str(meshheader.indexcount))
			print ("diffusecolor    : %1.6f %1.6f %1.6f" %meshheader.diffusecolor)
			print ("specularcolor   : %1.6f %1.6f %1.6f" %meshheader.specularcolor)
			print ("specularpower   : %1.6f" %meshheader.specularpower)
			print ("opacity         : %1.6f" %meshheader.opacity)
			print ("properties      : " + str(meshheader.properties))
			print ("textures        : " + str(meshheader.textures))
			print ("texturename     : " + str(meshheader.texturefilename))
			if len(meshheader.meshname) ==0:	#When no Meshname in File Generate one
					meshheader.meshname = basename+str(x+1)
			if meshheader.framecount > maxframe: maxframe = meshheader.framecount #Evaluate the maximal animationsteps
			meshdata = G3DMeshdataV4(fileID,meshheader)
			createMesh(filepath,meshheader,meshdata)
		fileID.close
		
		bpy.context.scene.frame_start=1
		bpy.context.scene.frame_end=maxframe
		bpy.context.scene.frame_current=1
		anchor = bpy.data.objects.new('Empty', None)
		anchor.select = True
		bpy.context.scene.objects.link(anchor)
		for ob in imported:
				ob.parent = anchor
		bpy.context.scene.update()
		print ("Created a empty Object as 'Grip' where all imported Objects are parented to")
		print ("To move the complete Meshes only select this empty Object and move it")
		print ("All Done, have a good Day :-)\n\n")
		return

def G3DSaver(filepath, context, operator):
	print ("\nNow Exporting File: " + filepath)

	objs = context.selected_objects
	if len(objs) == 0:
		objs = bpy.data.objects

	#get real meshcount as len(bpy.data.meshes) holds also old meshes
	meshCount = 0
	for obj in objs:
		if obj.type == 'MESH':
			meshCount += 1
			if obj.mode != 'OBJECT': # we want to be in object mode
				print("ERROR: mesh not in object mode")
				operator.report({'ERROR'}, "mesh not in object mode")
				return

	if meshCount == 0:
		print("ERROR: no meshes found")
		operator.report({'ERROR'}, "no meshes found")
		return

	fileID = open(filepath,"wb")
	# G3DHeader v4
	fileID.write(struct.pack("<3cB", b'G', b'3', b'D', 4))
	# G3DModelHeaderv4
	fileID.write(struct.pack("<HB", meshCount, 0))
	# meshes
	#for mesh in bpy.data.meshes:
	for obj in objs:
		if obj.type != 'MESH':
			continue
		mesh = obj.data.copy()
		diffuseColor = [1.0, 1.0, 1.0]
		specularColor = [0.9, 0.9, 0.9]
		opacity = 1.0
		textures = 0
		if len(mesh.materials) > 0:
			# we have a texture, hopefully
			material = mesh.materials[0]
			if material.active_texture.type=='IMAGE' and len(mesh.uv_textures)>0:
				diffuseColor = material.diffuse_color
				specularColor = material.specular_color
				opacity = material.alpha
				textures = 1
				texname = bpy.path.basename(material.active_texture.image.filepath)
			else:
				print("WARNING: active texture in first material isn't of type IMAGE or it's not unwrapped, texture ignored")
				operator.report({'WARNING'}, "active texture in first material isn't of type IMAGE or it's not unwrapped, texture ignored")
				#continue without texture

		meshname = mesh.name
		frameCount = context.scene.frame_end - context.scene.frame_start +1
		#Real face count (triangles)
		realFaceCount = 0
		indices=[]
		newverts=[]
		mesh.calc_tessface() # tesselate n-polygons to triangles and quads
		if textures == 1:
			uvtex = mesh.tessface_uv_textures[0]
			uvlist = []
			uvlist[:] = [[0]*2 for i in range(len(mesh.vertices))]
			s = set()
			for face in mesh.tessfaces:
				faceindices = [] # we create new faces when duplicating vertices
				realFaceCount += 1
				uvdata = uvtex.data[face.index]
				for i in range(3):
					vindex = face.vertices[i]
					if vindex not in s:
						s.add(vindex)
						uvlist[vindex] = uvdata.uv[i]
					elif uvlist[vindex] != uvdata.uv[i]:
						# duplicate vertex because it takes part in different faces
						# with different texcoords
						newverts.append(vindex)
						uvlist.append(uvdata.uv[i])
						#FIXME: probably better with some counter
						vindex = len(mesh.vertices) + len(newverts) -1

					faceindices.append(vindex)
				indices.extend(faceindices)

				#FIXME: stupid copy&paste
				if len(face.vertices) == 4:
					faceindices = []
					realFaceCount += 1
					for i in [0,2,3]:
						vindex = face.vertices[i]
						if vindex not in s:
							s.add(vindex)
							uvlist[vindex] = uvdata.uv[i]
						elif uvlist[vindex] != uvdata.uv[i]:
							# duplicate vertex because it takes part in different faces
							# with different texcoords
							newverts.append(vindex)
							uvlist.append(uvdata.uv[i])
							#FIXME: probably better with some counter
							vindex = len(mesh.vertices) + len(newverts) -1

						faceindices.append(vindex)
					indices.extend(faceindices)
		else:
			for face in mesh.tessfaces:
				realFaceCount += 1
				indices.extend(face.vertices[0:3])
				if len(face.vertices) == 4:
					realFaceCount += 1
					# new face because quad got split
					indices.append(face.vertices[0])
					indices.append(face.vertices[2])
					indices.append(face.vertices[3])


		# abort when no triangles as it crashs g3dviewer
		if realFaceCount == 0:
			print("ERROR: no triangles found")
			operator.report({'ERROR'}, "no triangles found")
			return
		indexCount = realFaceCount * 3
		vertexCount = len(mesh.vertices) + len(newverts)
		specularPower = 9.999999  # unused, same as old exporter
		properties = 0
		if mesh.g3d_customColor:
			properties |= 1
		if mesh.show_double_sided:
			properties |= 2
		if mesh.g3d_noSelect:
			properties |= 4

		#MeshData
		vertices = []
		normals = []
		fcurrent = context.scene.frame_current
		for i in range(context.scene.frame_start, context.scene.frame_end+1):
			context.scene.frame_set(i)
			#FIXME: not sure what's better: PREVIEW or RENDER settings
			m = obj.to_mesh(context.scene, True, 'RENDER')
			m.transform(obj.matrix_world)  # apply object-mode transformations
			for vertex in m.vertices:
				vertices.extend(vertex.co)
				normals.extend(vertex.normal)

			for nv in newverts:
				vertices.extend(m.vertices[nv].co)
				normals.extend(m.vertices[nv].normal)

		context.scene.frame_set(fcurrent)

		# MeshHeader
		fileID.write(struct.pack("<64s3I8f2I",
			bytes(meshname, "ascii"),
			frameCount, vertexCount, indexCount,
			diffuseColor[0], diffuseColor[1], diffuseColor[2],
			specularColor[0], specularColor[1], specularColor[2],
			specularPower, opacity,
			properties, textures
		))
		#Texture names
		if textures == 1: # only when we have one
			fileID.write(struct.pack("<64s", bytes(texname, "ascii")))

		# see G3DMeshdataV4
		vertex_format = "<%if" % int(frameCount * vertexCount * 3)
		normals_format = "<%if" % int(frameCount * vertexCount * 3)
		texturecoords_format = "<%if" % int(vertexCount * 2)
		indices_format = "<%iI" % int(indexCount)

		fileID.write(struct.pack(vertex_format, *vertices))
		fileID.write(struct.pack(normals_format, *normals))

		# texcoords
		if textures == 1: # only when we have one
			texcoords = []
			for uv in uvlist:
				texcoords.extend(uv)
			fileID.write(struct.pack(texturecoords_format, *texcoords))

		fileID.write(struct.pack(indices_format, *indices))

	fileID.close()
	return


#---=== Register ===
class G3DPanel(bpy.types.Panel):
	#bl_idname = "OBJECT_PT_G3DPanel"
	bl_label = "G3D properties"
	bl_space_type = 'PROPERTIES'
	bl_region_type = 'WINDOW'
	bl_context = "data"

	@classmethod
	def poll(cls, context):
		return (context.object is not None and context.object.type == 'MESH')

	def draw(self, context):
		self.layout.prop(context.object.data, "g3d_customColor", text="team color")
		self.layout.prop(context.object.data, "show_double_sided", text="double sided")
		self.layout.prop(context.object.data, "g3d_noSelect", text="non-selectable")

class ImportG3D(bpy.types.Operator, ImportHelper):
	'''Load a G3D file'''
	bl_idname = "importg3d.g3d"
	bl_label = "Import G3D"

	filename_ext = ".g3d"
	filter_glob = StringProperty(default="*.g3d", options={'HIDDEN'})

	def execute(self, context):
		try:
			G3DLoader(**self.as_keywords(ignore=("filter_glob",)))
		except:
			import traceback
			traceback.print_exc()

			return {'CANCELLED'}

		return {'FINISHED'}

class ExportG3D(bpy.types.Operator, ExportHelper):
	'''Save a G3D file'''
	bl_idname = "exportg3d.g3d"
	bl_label = "Export G3D"

	filename_ext = ".g3d"
	filter_glob = StringProperty(default="*.g3d", options={'HIDDEN'})

	def execute(self, context):
		try:
			G3DSaver(self.filepath, context, self)
		except:
			import traceback
			traceback.print_exc()

			return {'CANCELLED'}

		return {'FINISHED'}

def menu_func_import(self, context):
	self.layout.operator(ImportG3D.bl_idname, text="Glest 3D File (.g3d)")

def menu_func_export(self, context):
	self.layout.operator(ExportG3D.bl_idname, text="Glest 3D File (.g3d)")

def register():
	# custom mesh properties
	bpy.types.Mesh.g3d_customColor = bpy.props.BoolProperty()
	bpy.types.Mesh.g3d_noSelect = bpy.props.BoolProperty()

	bpy.utils.register_module(__name__)

	bpy.types.INFO_MT_file_import.append(menu_func_import)
	bpy.types.INFO_MT_file_export.append(menu_func_export)


def unregister():
	bpy.utils.unregister_module(__name__)

	bpy.types.INFO_MT_file_import.remove(menu_func_import)
	bpy.types.INFO_MT_file_export.remove(menu_func_export)

if __name__ == '__main__':
	register()
#	main()

	#G3DLoader("import.g3d")
	#for obj in bpy.context.selected_objects:
	#	obj.select = False  # deselect everything, so we get it all
	#G3DSaver("test.g3d", bpy.context)

