#!BPY
"""
Name: 'G3D Fileformat Im/Exporter'
Blender: 237
Group: 'Wizards'
Tooltip: 'Imports and Exports the Glest fileformat V3/V4 (.g3d)'
"""
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
import Blender
from Blender import NMesh, Object, Scene
from Blender.BGL import *
from Blender.Draw import *
from Blender.Window import *
from Blender.Image import *
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

###########################################################################
# Declaring Structures of G3D Format
###########################################################################
class G3DHeader:                            #Read first 4 Bytes of file should be G3D + Versionnumber
    binary_format = "<3cB"
    def __init__(self, fileID):
        temp = fileID.read(struct.calcsize(self.binary_format))
	data = struct.unpack(self.binary_format,temp)
	self.id = "".join(data[0:3])
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
            self.texturefilename = None
        else:
            self.texturefilename = "".join([x for x in data[7:-1] if x in string.printable])
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
 	self.meshname = "".join([x for x in data[0:64] if x in string.printable]) #Name of Mesh every Char is a  String on his own
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
            self.texturefilename = "".join([x for x in data[0:-1] if x in string.printable])
            self.hastexture = True
        else:
            self.hastexture = False
            self.texturefilename = None

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

def createMesh(filename,header,data):               #Create a Mesh inside Blender
    mesh = NMesh.New()                              #New Mesh
    meshobj = Object.New("Mesh",header.meshname)    #New Object for the new Mesh
    meshobj.link(mesh)                              #Link the Mesh to the Object
    sceneID.link (meshobj)                          #Link the Object to the actual Scene
    uvcoords = []
    if header.hastexture:                           #Load Texture when assigned
        texturefile = dirname(filename)+os.sep+header.texturefilename
        texture = Load(texturefile)
        mesh.hasFaceUV(1)                           #Because has Texture must have UV Coordinates
        mesh.hasVertexUV(1)                         #UV for each Vertex not only for Face
        for x in xrange(0,len(data.texturecoords),2): #Prepare the UVs
            uvcoords.append((data.texturecoords[x],data.texturecoords[x+1]))
    else:                                           #Has no Texture
        texture = None
        mesh.hasFaceUV(0)
    if header.isv4:
        mesh.col(int(255*header.diffusecolor[0]),int(255*header.diffusecolor[1]),int(255*header.diffusecolor[2]),int(255*header.opacity))
    if header.istwosided:
        mesh.setMode("TwoSided","AutoSmooth")       #Added Autosmooth because i think it does no harm
    else:                                           #I wonder why TwoSided here takes no effect on Faces ??
        mesh.setMode("AutoSmooth")                  #Same here
    y=0
    for x in xrange(0,header.vertexcount*3,3):      #Get the Vertices and Normals into empty Mesh
        vertex=NMesh.Vert()                         #and load UV coords when needed per Vertex
        vertex.co[0] = data.vertices[x]
        vertex.co[1] = data.vertices[x+1]
        vertex.co[2] = data.vertices[x+2]
        vertex.no[0] = data.normals[x]
        vertex.no[1] = data.normals[x+1]
        vertex.no[2] = data.normals[x+2]
        if header.hastexture:                       ###Have to Check this per Vertex UV### may not be needed
            vertex.uvco[0] = data.texturecoords[y]  ###but seems to do no harm :-)###
            vertex.uvco[1] = data.texturecoords[y+1]
        mesh.verts.append(vertex)			
        y= y+2
    for i in xrange(0,len(data.indices),3):         #Build Faces into Mesh
        face = NMesh.Face()
        face.smooth = 1
	face.v.append(mesh.verts[data.indices[i]])
	face.v.append(mesh.verts[data.indices[i+1]])
	face.v.append(mesh.verts[data.indices[i+2]])
        if header.hastexture:                       #Put UV in Faces too and assign Texture when textured
            face.uv.append(uvcoords[data.indices[i]])
            face.uv.append(uvcoords[data.indices[i+1]])
            face.uv.append(uvcoords[data.indices[i+2]])
            face.image = texture
        if header.istwosided:                       #Finaly found out how it works :-)
            face.mode |= NMesh.FaceModes['TWOSIDE'] #Really Bad documented this works to set all modes
	mesh.faces.append(face)
    mesh.update()                                   #Update changes to Mesh
    objdata = meshobj.getData()                     #Access Data of the Meshobject only the Obj has Key/IPOData
    imported.append(meshobj)                        #Add to Imported Objects
    for x in xrange(header.framecount):             #Put in Vertex Positions for Keyanimation
	for i in xrange(0,header.vertexcount*3,3):
	    objdata.verts[i/3].co[0]= data.vertices[x*header.vertexcount*3 + i]
            objdata.verts[i/3].co[1]= data.vertices[x*header.vertexcount*3 + i +1]
	    objdata.verts[i/3].co[2]= data.vertices[x*header.vertexcount*3 + i +2 ]
    	objdata.update()
        mesh.insertKey(x+1,"absolute")
	Blender.Set("curframe",x)

    # Make the keys animate in the 3d view.
    key = mesh.key
    key.relative = False

    # Add an IPO to teh Key
    ipo = Blender.Ipo.New('Key', 'md2')
    key.ipo = ipo
    # Add a curve to the IPO
    curve = ipo.addCurve('Basis')

    # Add 2 points to cycle through the frames.
    curve.append((1, 0))
    curve.append((header.framecount, (header.framecount-1)/10.0))
    curve.interpolation = Blender.IpoCurve.InterpTypes.LINEAR

    return
###########################################################################
# Import
###########################################################################
def G3DLoader(filename):            #Main Import Routine
    global imported, sceneID
    print "\nNow Importing File    : " + filename
    fileID = open(filename,"rb")
    header = G3DHeader(fileID)
    print "\nHeader ID             : " + header.id
    print "Version               : " + str(header.version)
    if header.id != "G3D":
        print "This is Not a G3D Model File"
        fileID.close
        return
    if header.version not in (3, 4):
        print "The Version of this G3D File is not Supported"
        fileID.close
        return
    in_editmode = Blender.Window.EditMode()             #Must leave Editmode when active
    if in_editmode: Blender.Window.EditMode(0)
    sceneID=Scene.getCurrent()                          #Get active Scene
    scenecontext=sceneID.getRenderingContext()          #To Access the Start/Endframe its so hidden i searched till i got angry :-)
    basename=str(Blender.sys.makename(filename,"",1))   #Generate the Base Filename without Path + extension
    imported = []
    maxframe=0
    if header.version == 3:
        modelheader = G3DModelHeaderv3(fileID)
        print "Number of Meshes      : " + str(modelheader.meshcount)
        for x in xrange(modelheader.meshcount):
            meshheader = G3DMeshHeaderv3(fileID)
            meshheader.isv4 = False    
            print "\nMesh Number           : " + str(x+1)
            print "framecount            : " + str(meshheader.framecount)
	    print "normalframecount      : " + str(meshheader.normalframecount)
	    print "texturecoordframecount: " + str(meshheader.texturecoordframecount)
	    print "colorframecount       : " + str(meshheader.colorframecount)
	    print "pointcount            : " + str(meshheader.vertexcount)
	    print "indexcount            : " + str(meshheader.indexcount)
	    print "texturename           : " + str(meshheader.texturefilename)
            print "hastexture            : " + str(meshheader.hastexture)
            print "istwosided            : " + str(meshheader.istwosided)
            print "customalpha           : " + str(meshheader.customalpha)
            meshheader.meshname = basename+str(x+1)     #Generate Meshname because V3 has none
            if meshheader.framecount > maxframe: maxframe = meshheader.framecount #Evaluate the maximal animationsteps
	    meshdata = G3DMeshdataV3(fileID,meshheader)
	    createMesh(filename,meshheader,meshdata)
        fileID.close
        print "Imported Objects: ",imported
        scenecontext.startFrame(1)      #Yeah finally found this Options :-)
        scenecontext.endFrame(maxframe) #Set it correctly to the last Animationstep :-))))
        Blender.Set("curframe",1)       #Why the Heck are the above Options not here accessible ????
        anchor = Object.New("Empty",basename) #Build an "empty" to Parent all meshes to it for easy handling
        sceneID.link(anchor)                #Link it to current Scene
        anchor.makeParent(imported)         #Make it Parent for all imported Meshes
        anchor.sel = 1                      #Select it
        if in_editmode: Blender.Window.EditMode(1) # Be nice and restore Editmode when was active
        return
    if header.version == 4:
        modelheader = G3DModelHeaderv4(fileID)
        print "Number of Meshes      : " + str(modelheader.meshcount)
        for x in xrange(modelheader.meshcount):
            meshheader = G3DMeshHeaderv4(fileID)
            meshheader.isv4 = False    
            print "\nMesh Number           : " + str(x+1)
	    print "meshname              : " + str(meshheader.meshname)
	    print "framecount            : " + str(meshheader.framecount)
	    print "vertexcount           : " + str(meshheader.vertexcount)
	    print "indexcount            : " + str(meshheader.indexcount)
	    print "diffusecolor          : %1.6f %1.6f %1.6f" %meshheader.diffusecolor
	    print "specularcolor         : %1.6f %1.6f %1.6f" %meshheader.specularcolor
	    print "specularpower         : %1.6f" %meshheader.specularpower
	    print "opacity               : %1.6f" %meshheader.opacity
	    print "properties            : " + str(meshheader.properties)
	    print "textures              : " + str(meshheader.textures)
	    print "texturename           : " + str(meshheader.texturefilename)
            if len(meshheader.meshname) ==0:    #When no Meshname in File Generate one
                meshheader.meshname = basename+str(x+1)
            if meshheader.framecount > maxframe: maxframe = meshheader.framecount #Evaluate the maximal animationsteps
   	    meshdata = G3DMeshdataV4(fileID,meshheader)
	    createMesh(filename,meshheader,meshdata)
        fileID.close
        scenecontext.startFrame(1)      #Yeah finally found this Options :-)
        scenecontext.endFrame(maxframe) #Set it correctly to the last Animationstep :-))))
        Blender.Set("curframe",1)       #Why the Heck are the above Options not here accessible ????
        anchor = Object.New("Empty",basename) #Build an "empty" to Parent all meshes to it for easy handling
        sceneID.link(anchor)                #Link it to current Scene
        anchor.makeParent(imported)         #Make it Parent for all imported Meshes
        anchor.sel = 1                      #Select it
        if in_editmode: Blender.Window.EditMode(1) # Be nice and restore Editmode when was active
        print "Created a empty Object as 'Grip' where all imported Objects are parented to"
        print "To move the complete Meshes only select this empty Object and move it"
        print "All Done, have a good Day :-)\n\n"
        return
###########################################################################
# Export (Ripped out its work in Progress will be added soon
###########################################################################
def G3DSaverV3(filename):
    return

def G3DSaverV4(filename):
    return
###########################################################################
# Complete GUI Part of The Wizard
###########################################################################
# Events that can occure by pressing the Buttons
EVENT_LOADG3D=2
EVENT_SAVEG3DV3=3
EVENT_SAVEG3DV4=4
EVENT_EXIT=100
###########################################################################
# GUI eventhandler
###########################################################################
def event(evt, val):
	if evt == Blender.Draw.ESCKEY and not val: Blender.Draw.Exit()
def button_event(evt):
    global EVENT_LOAD_G3D,EVENT_SAVE_G3DV3,EVENT_SAVE_G3DV4,EVENT_EXIT
    if evt == EVENT_EXIT: Exit()
    if evt == EVENT_LOADG3D:
            defaultname = os.sep + ".g3d"
#I use this defaultnames only for development to have short ways to models
#if u want make a path to your models here and comment the above defaultname out.
#Be careful to not move the start of the Line because it matters in python
#            defaultname="I:/glest/glest_game/techs/magitech/factions/magic/units/magic_armor/models/magic_armor_walking.g3d"
#            defaultname="I:/glest/glest_game/techs/magitech/factions/.g3d"
#            defaultname="I:/entwicklung/blenderexport/test/swordman_sample/swordman.g3d"
            FileSelector(G3DLoader, "G3D to load", defaultname)
            Blender.Redraw()
    if evt==EVENT_SAVEG3DV3:
            defaultname = Blender.sys.makename(Blender.Get("filename"),".g3d",1)
            FileSelector(G3DSaverV3, "G3D V3 to save", defaultname)
    if evt==EVENT_SAVEG3DV4:
            defaultname = Blender.sys.makename(Blender.Get("filename"),".g3d",1)
            FileSelector(G3DSaverV4, "G3D V4 to save", defaultname)
###########################################################################
# GUI Screenmask as a nice looking feature i Center the Mask
###########################################################################
def gui():
    global EVENT_LOADG3D,EVENT_SAVEG3DV3,EVENT_SAVEG3DV4,EVENT_EXIT
    xmiddle= GetScreenSize()[0] / 2      #calculate horizontal middle of the screen
    #Clearing the Screen all here needs imported BGL + DRAW
    glClearColor(0.0, 0.0, 0.0,1)
    glClear(GL_COLOR_BUFFER_BIT)
    # logo
    homedir = Blender.Get("homedir")
    Logo = Load(homedir+ os.sep + "scripts" + os.sep + "g3d_logo.png") # Load Logo file
    glEnable(GL_BLEND)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
    Image(Logo,(xmiddle-(Logo.getSize()[0]/2)),20)
    glDisable(GL_BLEND)
    #Title
    tbuffer= "G3D Import Wizard supporting V3 / V4 models"
    xpos = xmiddle - (GetStringWidth(tbuffer) /2)
    glColor3f(1, 0, 0)
    glRasterPos2d(xpos, 70)
    Text(tbuffer)
    tbuffer= "Please view any Errors / Info in the Blender Console"
    xpos = xmiddle - (GetStringWidth(tbuffer) /2)
    glColor3f(1, 1, 0)
    glRasterPos2d(xpos, 55)
    Text(tbuffer)
    #Buttons
    Button("Load G3D",EVENT_LOADG3D , xmiddle-140, 10, 100, 40)
    #Button("Save G3D V3",EVENT_SAVEG3DV3 , xmiddle-100, 10, 100, 40)
    #Button("Save G3D V4",EVENT_SAVEG3DV4 , xmiddle, 10, 100, 40)
    Button("Exit",EVENT_EXIT , xmiddle+40, 10, 100, 40)
###########################################################################
# Registering GUI events (Activating GUI)  THE END 
###########################################################################
Register(gui, event, button_event)
