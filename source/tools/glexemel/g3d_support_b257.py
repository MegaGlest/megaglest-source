###########################################################################
# Glest Model / Texture / UV / Animation Importer and Exporter
# for the Game Glest that u can find http://www.glest.org
# copyright 2005 By Andreas Becker (seltsamuel@yahoo.de)
#
# Updated Jan 2011 by Mark Vejvoda (SoftCoder) to properly import animations
# from G3D into Blender
#
# Started Date: 07 June 2005  Put Public 20 June 2005   
# Ver: 0.01 Beta  Exporter ripped off because work in Progress
#  Distributed under the GNU PUBLIC LICENSE for www.megaglest.org and glest.org
###########################################################################
#NOTE:
#       Copy this Script AND g3d_logo.png into .Blender\scripts
#       directory then start inside blender Scripts window
#       "Update Menus" after that this Script here is accesible 
#       as 'Wizards' G3d Fileformat Im/Exporter
#ToDo:
#Exporter Bughunt he will be rejoined next release
#Maybe Integrate UV Painter to Generate UVMaps from Blender Material and procedural Textures
#will be nice to paint wireframe too, so that one can easyly load into Paintprogram and see where to paint
#(Already possible through Blender functions so at the end of the list :-) )
###########################################################################

bl_info = {
    "name": "MegaGlest G3D Fileformat Import/Exporter",
    "description": "Imports and Exports the Glest fileformat V3/V4 (.g3d)",
    "author": "Andreas Becker, Mark Vejvoda, William Zheng",
    "version": (1,1),
    "blender": (2, 5, 7),
    "api": 35622,
    'location': 'File > Import-Export',
    "warning": '',
    "wiki_url": "http://wiki.blender.org/index.php/Extensions:2.5/Py/"\
    "Scripts/Import-Export/MegaGlest_G3D",
    "tracker_url": "http://projects.blender.org/tracker/index.php?"\
    "func=detail&aid=<number>",
    "category": "Import-Export"
}

"""
Here an explanation of the V4 Format found at www.glest.org
================================
1. DATA TYPES
================================
G3D files use the following data types:
uint8: 8 bit unsigned integer
uint16: 16 bit unsigned integer
uint32: 32 bit unsigned integer
float32: 32 bit floating point
================================
2. OVERALL STRUCTURE
================================
- File header
- Model header
- Mesh header
- Texture names
- Mesh data
================================
2. FILE HEADER
================================
Code:
struct FileHeader{
   uint8 id[3];
   uint8 version;
};
This header is shared among all the versions of G3D, it identifies this file as a G3D model and provides information of the version.
id: must be "G3D"
version: must be 4, in binary (not '4')
================================
3. MODEL HEADER
================================
Code:
struct ModelHeader{
   uint16 meshCount;
   uint8 type;
};           
meshCount: number of meshes in this model
type: must be 0
================================
4. MESH HEADER
================================
There is a mesh header for each mesh, there must be "meshCount" headers in a file but they are not consecutive, texture names and mesh data are stored in between.
Code:
struct MeshHeader{
   uint8 name[64];
   uint32 frameCount;
   uint32 vertexCount;
   uint32 indexCount;
   float32 diffuseColor[3];
   float32 specularColor[3];
   float32 specularPower;
   float32 opacity;
   uint32 properties;
   uint32 textures;
};
name: name of the mesh
frameCount: number of keyframes in this mesh
vertexCount: number of vertices in each frame
indexCount: number of indices in this mesh (the number of triangles is indexCount/3)
diffuseColor: RGB diffuse color
specularColor: RGB specular color (currently unused)
specularPower: specular power (currently unused)
properties: property flags
Code:
enum MeshPropertyFlag{
   mpfTwoSided= 1,
        mpfCustomColor= 2,
};
mpfTwoSided: meshes in this mesh are rendered by both sides, if this flag is not present only "counter clockwise" faces are rendered
mpfCustomColor: alpha in this model is replaced by a custom color, usually the player color
textures: texture flags, only 0x1 is currently used, indicating that there is a diffuse texture in this mesh.
================================
4. TEXTURE NAMES
================================
A list of uint8[64] texture name values. One for each texture in the mesh. If there are no textures in the mesh no texture names are present. In practice since Glest only uses 1 texture for each mesh the number of texture names should be 0 or 1.
================================
5. MESH DATA
================================
After each mesh header and texture names the mesh data is placed.
vertices: frameCount * vertexCount * 3, float32 values representing the x, y, z vertex coords for all frames
normals: frameCount * vertexCount * 3, float32 values representing the x, y, z normal coords for all frames
texture coords: vertexCount * 2, float32 values representing the s, t tex coords for all frames (only present if the mesh has 1 texture at least)
indices: indexCount, uint32 values representing the indices
"""
###########################################################################
# Importing Structures needed (must later verify if i need them really all)
###########################################################################
import sys, struct, string, types
from types import *
import os
from os import path
from os.path import dirname

import bpy
from bpy.props import StringProperty
from io_utils import ImportHelper, ExportHelper
import mathutils
import math

import re
from string import *
from struct import *


###########################################################################
# Variables that are better Global to handle
###########################################################################
imported = []   #List of all imported Objects
toexport = []   #List of Objects to export (actually only meshes)
sceneID  = None #Points to the active Blender Scene

###########################################################################
# Declaring Structures of G3D Format
###########################################################################
class G3DHeader:                            #Read first 4 Bytes of file should be G3D + Versionnumber
    binary_format = "<3cB"
    def __init__(self, fileID):
        temp = fileID.read(struct.calcsize(self.binary_format))
        data = struct.unpack(self.binary_format,temp)
        
        #print(type(data)); print(repr(data))
        #self.id = bytes(data[0]).decode() + bytes(data[1]).decode() + bytes(data[2]).decode()
        self.id = ''.join(item.decode() for item in data[0:3])
        #self.id = data[0:3]
        self.version = data[3]

class G3DModelHeaderv3:                     #Read Modelheader in V3 there is only the number of Meshes in file
    binary_format = "<I"
    def __init__(self, fileID):
        temp = fileID.read(struct.calcsize(self.binary_format))
        data = struct.unpack(self.binary_format,temp)
        self.meshcount = data[0]

class G3DModelHeaderv4:                     #Read Modelheader: Number of Meshes and Meshtype (must be 0)
    binary_format = "<HB"
    def __init__(self, fileID):
        temp = fileID.read(struct.calcsize(self.binary_format))
        data = struct.unpack(self.binary_format,temp)
        self.meshcount = data[0]
        self.mtype = data[1]
		
class G3DMeshHeaderv3:                      #Read Meshheader
    binary_format = "<7I64c"
    def __init__(self,fileID):
        temp = fileID.read(struct.calcsize(self.binary_format))
        data = struct.unpack(self.binary_format,temp)
        self.framecount = data[0]           #Framecount = Number of Animationsteps
        self.normalframecount= data[1]      #Number of Normal Frames actualli equal to Framecount
        self.texturecoordframecount= data[2]#Number of Frames of Texturecoordinates seems everytime to be 1
        self.colorframecount= data[3]       #Number of Frames of Colors seems everytime to be 1
        self.vertexcount= data[4]           #Number of Vertices in each Frame
        self.indexcount= data[5]            #Number of Indices in Mesh (Triangles = Indexcount/3)
        self.properties= data[6]            #Property flags
        if self.properties & 1:             #PropertyBit is Mesh Textured ?
            self.hastexture = False
            self.texturefilepath = None
        else:
            self.texturefilepath = "".join([x.decode("ascii", "ignore") for x in data[7:-1] if x.decode("ascii", "ignore") in string.printable])
            self.hastexture = True
        if self.properties & 2:             #PropertyBit is Mesh TwoSided ?
            self.istwosided = True
        else:
            self.istwosided = False
        if self.properties & 4:             #PropertyBit is Mesh Alpha Channel custom Color in Game ?
            self.customalpha = True
        else:
            self.customalpha = False
        
class G3DMeshHeaderv4:                      #Read Meshheader
    binary_format = "<64c3I8f2I"
    texname_format = "<64c"
    def __init__(self,fileID):
        temp = fileID.read(struct.calcsize(self.binary_format))
        data = struct.unpack(self.binary_format,temp)
        self.meshname = "".join([x.decode("ascii", "ignore") for x in data[0:64] if x.decode("ascii", "ignore") in string.printable]) #Name of Mesh every Char is a  String on his own
        self.framecount = data[64]          #Framecount = Number of Animationsteps
        self.vertexcount = data[65]         #Number of Vertices in each Frame
        self.indexcount = data[66]          #Number of Indices in Mesh (Triangles = Indexcount/3)
        self.diffusecolor = data[67:70]     #RGB diffuse color
        self.specularcolor = data[70:73]    #RGB specular color (unused)
        self.specularpower = data[73]       #Specular power (unused)
        self.opacity = data[74]             #Opacity
        self.properties= data[75]           #Property flags
        self.textures = data[76]            #Texture flag
        if self.properties & 1:             #PropertyBit is Mesh TwoSided ?
            self.istwosided = True
        else:
            self.istwosided = False
        if self.properties & 2:             #PropertyBit is Mesh Alpha Channel custom Color in Game ?
            self.customalpha = True
        else:
            self.customalpha = False
        if self.textures & 1:               #PropertyBit is Mesh Textured ?
            temp = fileID.read(struct.calcsize(self.texname_format))
            data = struct.unpack(self.texname_format,temp)
            self.texturefilepath = "".join([x.decode("ascii", "ignore") for x in data[0:-1] if x.decode("ascii", "ignore") in string.printable])
            self.hastexture = True
        else:
            self.hastexture = False
            self.texturefilepath = None

class G3DMeshdataV3:                        #Calculate and read the Mesh Datapack
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

class G3DMeshdataV4:                        #Calculate and read the Mesh Datapack
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

def createMesh(filepath,header,data):               #Create a Mesh inside Blender
    mesh =  bpy.data.meshes.new(header.meshname)                              #New Mesh
    meshobj = bpy.data.objects.new(header.meshname,mesh)    #New Object for the new Mesh
    #meshobj.link(mesh)                              #Link the Mesh to the Object
    sceneID.objects.link (meshobj)                          #Link the Object to the actual Scene
    uvcoords = []
    if header.hastexture:                           #Load Texture when assigned
        texturefile = dirname(filepath)+os.sep+header.texturefilepath
        texture = bpy.data.images.load(texturefile)
        #mesh.hasFaceUV(1)                           #Because has Texture must have UV Coordinates
        #mesh.hasVertexUV(1)                         #UV for each Vertex not only for Face
        for x in range(0,len(data.texturecoords),2): #Prepare the UVs
            uvcoords.append((data.texturecoords[x],data.texturecoords[x+1]))
    else:                                           #Has no Texture
        texture = None
        #mesh.hasFaceUV(0)
    if header.isv4:
        mesh.col(int(255*header.diffusecolor[0]),int(255*header.diffusecolor[1]),int(255*header.diffusecolor[2]),int(255*header.opacity))
    if header.istwosided:
        #mesh.setMode("TwoSided","AutoSmooth")       #Added Autosmooth because i think it does no harm
        mesh.show_double_sided = True
        mesh.use_auto_smooth = True
    else:                                           #I wonder why TwoSided here takes no effect on Faces ??
        mesh.use_auto_smooth = True                  #Same here
    y=0
    
    mesh.vertices.add(header.vertexcount)
    vertexIndex=0;
    
    for x in range(0,header.vertexcount*3,3):      #Get the Vertices and Normals into empty Mesh
                         #and load UV coords when needed per Vertex
        mesh.vertices[vertexIndex].co[0] = data.vertices[x]
        mesh.vertices[vertexIndex].co[1] = data.vertices[x+1]
        mesh.vertices[vertexIndex].co[2] = data.vertices[x+2]
        mesh.vertices[vertexIndex].normal[0] = data.normals[x]
        mesh.vertices[vertexIndex].normal[1] = data.normals[x+1]
        mesh.vertices[vertexIndex].normal[2] = data.normals[x+2]
        #if header.hastexture:                       ###Have to Check this per Vertex UV### may not be needed
        #            vertex.uvco[0] = data.texturecoords[y]  ###but seems to do no harm :-)###
        #            vertex.uvco[1] = data.texturecoords[y+1]
        #mesh.verts.append(vertex)			
        y= y+2
        vertexIndex = vertexIndex + 1

    mesh.faces.add(len(data.indices))
    faceIndex=0;
        
    for i in range(0,len(data.indices),3):         #Build Faces into Mesh
        #        face = NMesh.Face()
        mesh.faces[faceIndex].use_smooth = 1
        
        #print(type(data.indices[i])); print(repr(data.indices[i]))
        #print(type(mesh.vertices[data.indices[i]])); print(repr(mesh.vertices[data.indices[i]]))
        #print(type(mesh.vertices[data.indices[i]].co[0])); print(repr(mesh.vertices[data.indices[i]].co[0]))
        #print(type(mesh.faces[faceIndex].vertices[0])); print(repr(mesh.faces[faceIndex].vertices[0]))
        
        mesh.faces[faceIndex].vertices[0] = mesh.vertices[data.indices[i]].index
        mesh.faces[faceIndex].vertices[1] = mesh.vertices[data.indices[i+1]].index
        mesh.faces[faceIndex].vertices[2] = mesh.vertices[data.indices[i+2]].index
        if header.hastexture:                       #Put UV in Faces too and assign Texture when textured
             mesh.uv_textures.new()
             tface       = mesh.uv_textures[0].data[faceIndex]
             tface.uv1   = uvcoords[data.indices[i]]
             tface.uv2   = uvcoords[data.indices[i+1]]
             tface.uv3   = uvcoords[data.indices[i+2]]
             tface.image = texture
             tface.use_image = True
             if header.istwosided:                       #Finaly found out how it works :-)
                tface.use_twoside = True
                    
        #            face.uv.append(uvcoords[data.indices[i]])
        #            face.uv.append(uvcoords[data.indices[i+1]])
        #            face.uv.append(uvcoords[data.indices[i+2]])
        #            face.image = texture
        #if header.istwosided:                       #Finaly found out how it works :-)
        #            face.mode |= NMesh.FaceModes['TWOSIDE'] #Really Bad documented this works to set all modes
        #        mesh.faces.append(face)
        faceIndex = faceIndex + 1
        
    mesh.update()                                   #Update changes to Mesh
    #objdata = meshobj.data                     #Access Data of the Meshobject only the Obj has Key/IPOData
    imported.append(meshobj)                        #Add to Imported Objects
    for x in range(header.framecount):             #Put in Vertex Positions for Keyanimation
        for i in range(0,header.vertexcount*3,3):
            vertexIndex = int(i/3)
            dataIndex = x*header.vertexcount*3 + i
            
            #print(type(vertexIndex)); print(repr(vertexIndex))
            #print(type(dataIndex)); print(repr(dataIndex))
            #print(type(mesh.vertices[vertexIndex].co[0])); print(repr(mesh.vertices[vertexIndex].co[0]))
            #print(type(data.vertices[dataIndex])); print(repr(data.vertices[dataIndex]))
            
            mesh.vertices[vertexIndex].co[0] = data.vertices[dataIndex]
            mesh.vertices[vertexIndex].co[1] = data.vertices[dataIndex +1]
            mesh.vertices[vertexIndex].co[2] = data.vertices[dataIndex +2]
            #objdata.verts[i/3].co[0]= data.vertices[x*header.vertexcount*3 + i]
            #objdata.verts[i/3].co[1]= data.vertices[x*header.vertexcount*3 + i +1]
            #objdata.verts[i/3].co[2]= data.vertices[x*header.vertexcount*3 + i +2 ]
        #objdata.update()
        mesh.update()
        #mesh.insertKey(x+1,"absolute")
        shape_key = meshobj.shape_key_add()
                
        #Blender.Set("curframe",x)
        sceneID.frame_current = x

    # Make the keys animate in the 3d view.
    #key = mesh.key
    #key.relative = False
    bpy.data.shape_keys[0].use_relative = False

    # Add an IPO to teh Key
    #ipo = Blender.Ipo.New('Key', 'md2')
    #key.ipo = ipo
    # Add a curve to the IPO
    #curve = ipo.addCurve('Basis')
    #curve = bpy.data.curves.new('Key',CURVE)

    # Add 2 points to cycle through the frames.
    #curve.append((1, 0))
    #curve.append((header.framecount, (header.framecount-1)/10.0))
    #curve.interpolation = Blender.IpoCurve.InterpTypes.LINEAR

    return
###########################################################################
# Import
###########################################################################
def G3DLoader(filepath):            #Main Import Routine
    global imported, sceneID
    print ("\nNow Importing File    : ", filepath)
    fileID = open(filepath,"rb")
    header = G3DHeader(fileID)
    print("\nHeader ID             : ", header.id)
    print("Version               : ", str(header.version))
    if header.id != "G3D":
        print("This is Not a G3D Model File")
        fileID.close
        return
    if header.version not in (3, 4):
        print("The Version of this G3D File is not Supported")
        fileID.close
        return
    in_editmode = 0             #Must leave Editmode when active
    if bpy.context.mode == 'EDIT': in_editmode = 1
    if in_editmode: bpy.ops.object.editmode_toggle()
    sceneID=bpy.context.scene                          #Get active Scene
    #scenecontext=sceneID.getRenderingContext()          #To Access the Start/Endframe its so hidden i searched till i got angry :-)
    #basename=str(Blender.sys.makename(filepath,"",1))   #Generate the Base filepath without Path + extension
    basename="g3d_mesh"
    imported = []
    maxframe=0
    if header.version == 3:
        modelheader = G3DModelHeaderv3(fileID)
        print("Number of Meshes      : ", str(modelheader.meshcount))
        for x in range(modelheader.meshcount):
            meshheader = G3DMeshHeaderv3(fileID)
            meshheader.isv4 = False    
            print("\nMesh Number           : ", str(x+1))
            print("framecount            : ", str(meshheader.framecount))
            print("normalframecount      : ", str(meshheader.normalframecount))
            print("texturecoordframecount: ", str(meshheader.texturecoordframecount))
            print("colorframecount       : ", str(meshheader.colorframecount))
            print("pointcount            : ", str(meshheader.vertexcount))
            print("indexcount            : ", str(meshheader.indexcount))
            print("texturename           : ", str(meshheader.texturefilepath))
            print("hastexture            : ", str(meshheader.hastexture))
            print("istwosided            : ", str(meshheader.istwosided))
            print("customalpha           : ", str(meshheader.customalpha))
            meshheader.meshname = basename+str(x+1)     #Generate Meshname because V3 has none
            if meshheader.framecount > maxframe: maxframe = meshheader.framecount #Evaluate the maximal animationsteps
            meshdata = G3DMeshdataV3(fileID,meshheader)
            createMesh(filepath,meshheader,meshdata)
        fileID.close
        print("Imported Objects: ", imported)
        sceneID.frame_start = 1      #Yeah finally found this Options :-)
        sceneID.frame_end = maxframe #Set it correctly to the last Animationstep :-))))
        sceneID.frame_current = 1       #Why the Heck are the above Options not here accessible ????
        anchor = bpy.data.objects.new(basename,None) #Build an "empty" to Parent all meshes to it for easy handling
        sceneID.objects.link(anchor)                #Link it to current Scene
        #anchor.makeParent(imported)         #Make it Parent for all imported Meshes
        anchor.select = True                      #Select it
        if in_editmode: bpy.ops.object.editmode_toggle() # Be nice and restore Editmode when was active
        return
    
    if header.version == 4:
        modelheader = G3DModelHeaderv4(fileID)
        print("Number of Meshes      : ", str(modelheader.meshcount))
        for x in range(modelheader.meshcount):
            meshheader = G3DMeshHeaderv4(fileID)
            meshheader.isv4 = False    
            print("\nMesh Number           : ", str(x+1))
            print("meshname              : ", str(meshheader.meshname))
            print("framecount            : ", str(meshheader.framecount))
            print("vertexcount           : ", str(meshheader.vertexcount))
            print("indexcount            : ", str(meshheader.indexcount))
            print("diffusecolor          : ", meshheader.diffusecolor)
            print("specularcolor         : ", meshheader.specularcolor)
            print("specularpower         : ", meshheader.specularpower)
            print("opacity               : ", meshheader.opacity)
            print("properties            : ",  str(meshheader.properties))
            print("textures              : ",  str(meshheader.textures))
            print("texturename           : ",  str(meshheader.texturefilepath))
            if len(meshheader.meshname) ==0:    #When no Meshname in File Generate one
                meshheader.meshname = basename+str(x+1)
            if meshheader.framecount > maxframe: maxframe = meshheader.framecount #Evaluate the maximal animationsteps
            meshdata = G3DMeshdataV4(fileID,meshheader)
            createMesh(filepath,meshheader,meshdata)
        fileID.close
        sceneID.frame_start = 1      #Yeah finally found this Options :-)
        sceneID.frame_end = maxframe #Set it correctly to the last Animationstep :-))))
        sceneID.frame_current = 1       #Why the Heck are the above Options not here accessible ????
        anchor = bpy.data.objects.new(basename,None) #Build an "empty" to Parent all meshes to it for easy handling
        sceneID.objects.link(anchor)                #Link it to current Scene
        #anchor.parent = imported         #Make it Parent for all imported Meshes
        anchor.select = True                      #Select it
        if in_editmode: bpy.ops.object.editmode_toggle() # Be nice and restore Editmode when was active
        print("Created a empty Object as 'Grip' where all imported Objects are parented to")
        print("To move the complete Meshes only select this empty Object and move it")
        print("All Done, have a good Day :-)\n\n")
        return


def G3DSaver(filepath, context):
	print ("\nNow Exporting File: " + filepath)
	fileID = open(filepath,"wb")

	# G3DHeader v4
	fileID.write(struct.pack("<3cB", b'G', b'3', b'D', 4))
	#get real meshcount as len(bpy.data.meshes) holds also old meshes
	meshCount = 0
	for obj in bpy.data.objects:#context.selected_objects:
		if obj.type == 'MESH':
			meshCount += 1
	# G3DModelHeaderv4
	fileID.write(struct.pack("<HB", meshCount, 0))
	# meshes
	#for mesh in bpy.data.meshes:
	for obj in bpy.data.objects:#context.selected_objects:
		if obj.type != 'MESH':
			continue
		mesh = obj.data
		diffuseColor = [1.0, 1.0, 1.0]
		specularColor = [0.9, 0.9, 0.9]
		opacity = 1.0
		textures = 0
		if len(mesh.materials) > 0:
			# we have a texture, hopefully
			material = mesh.materials[0]
			if material.active_texture.type == 'IMAGE':
				diffuseColor = material.diffuse_color
				specularColor = material.specular_color
				opacity = material.alpha
				textures = 1
				texname = bpy.path.basename(material.active_texture.image.filepath)
			else:
				#FIXME: needs warning in gui
				print("active texture in first material isn't of type IMAGE")

		meshname = mesh.name
		frameCount = context.scene.frame_end - context.scene.frame_start +1
		#Real face count (only use triangle)
		realFaceCount = 0
		indices=[]
		newverts=[]
		if textures == 1:
			uvtex = mesh.uv_textures[0]
			uvlist = []
			uvlist[:] = [[0]*2 for i in range(len(mesh.vertices))]
			s = set()
			for face in mesh.faces:
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
			for face in mesh.faces:
				realFaceCount += 1
				indices.extend(face.vertices[0:3])
				if len(face.vertices) == 4:
					realFaceCount += 1
					# new face because quad got split
					indices.append(face.vertices[0])
					indices.append(face.vertices[2])
					indices.append(face.vertices[3])


		#FIXME: abort when no triangles as it crashs g3dviewer
		if realFaceCount == 0:
			print("no triangles found")
		indexCount = realFaceCount * 3
		vertexCount = len(mesh.vertices) + len(newverts)
		specularPower = 9.999999  # unused, same as old exporter
		properties = 0
		if mesh.show_double_sided:
			properties |= 1
		if textures==1 and mesh.materials[0].use_face_texture_alpha:
			properties |= 2

		#MeshData
		vertices = []
		normals = []
		fcurrent = context.scene.frame_current
		for i in range(context.scene.frame_start, context.scene.frame_end+1):
			context.scene.frame_set(i)
			#FIXME: this is hacky
			if obj.find_armature() == None:
				m = mesh.copy()
				m.transform(obj.matrix_world)
			else:
				#FIXME: not sure what's better: PREVIEW or RENDER settings
				m = obj.to_mesh(context.scene, True, 'RENDER')
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
class ImportG3D(bpy.types.Operator, ImportHelper):
	'''Import from G3D file format (.g3d)'''
	bl_idname = "import_mg.g3d"
	bl_label = "Import G3D files"

	filepath_ext = ".g3d"
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
	'''Export to G3D file format (.g3d)'''
	bl_idname = "export_mg.g3d"
	bl_label = "Export G3D files"

	filename_ext = ".g3d"
	filter_glob = StringProperty(default="*.g3d", options={'HIDDEN'})

	def execute(self, context):
		try:
			G3DSaver(self.filepath, context)
		except:
			import traceback
			traceback.print_exc()

			return {'CANCELLED'}

		return {'FINISHED'}

def menu_func_import(self, context):
	self.layout.operator(ImportG3D.bl_idname, text="MegaGlest 3D File (.g3d)")

def menu_func_export(self, context):
	self.layout.operator(ExportG3D.bl_idname, text="MegaGlest 3D File (.g3d)")

def register():
	bpy.utils.register_module(__name__)

	bpy.types.INFO_MT_file_import.append(menu_func_import)
	bpy.types.INFO_MT_file_export.append(menu_func_export)


def unregister():
	bpy.utils.unregister_module(__name__)

	bpy.types.INFO_MT_file_import.remove(menu_func_import)
	bpy.types.INFO_MT_file_export.remove(menu_func_export)

if __name__ == '__main__':
	register()

