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
# modified by James Sherratt for Blender 2.90
#
# Started Date: 07 June 2005  Put Public 20 June 2005
#  Distributed under the GNU PUBLIC LICENSE
# """
# Here an explanation of the V4 Format found at www.glest.org
# ================================
# 1. DATA TYPES
# ================================
# G3D files use the following data types:
# uint8: 8 bit unsigned integer
# uint16: 16 bit unsigned integer
# uint32: 32 bit unsigned integer
# float32: 32 bit floating point
# ================================
# 2. OVERALL STRUCTURE
# ================================
# - File header
# - Model header
# - Mesh header
# - Texture names
# - Mesh data
# ================================
# 2. FILE HEADER
# ================================
# Code:
# struct FileHeader{
#   uint8 id[3];
#   uint8 version;
# };
# This header is shared among all the versions of G3D, it identifies this file as a G3D model and provides information of the version.
# id: must be "G3D"
# version: must be 4, in binary (not '4')
# ================================
# 3. MODEL HEADER
# ================================
# Code:
# struct ModelHeader{
#   uint16 meshCount;
#   uint8 type;
# };
# meshCount: number of meshes in this model
# type: must be 0
# ================================
# 4. MESH HEADER
# ================================
# There is a mesh header for each mesh, there must be "meshCount" headers in a file but they are not consecutive, texture names and mesh data are stored in between.
# Code:
# struct MeshHeader{
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
# };
# name: name of the mesh
# frameCount: number of keyframes in this mesh
# vertexCount: number of vertices in each frame
# indexCount: number of indices in this mesh (the number of triangles is indexCount/3)
# diffuseColor: RGB diffuse color
# specularColor: RGB specular color (currently unused)
# specularPower: specular power (currently unused)
# properties: property flags
# Code:
# enum MeshPropertyFlag{
#        mpfCustomColor= 1,
#        mpfTwoSided= 2,
#        mpfNoSelect= 4
# };
# mpfTwoSided: meshes in this mesh are rendered by both sides, if this flag is not present only "counter clockwise" faces are rendered
# mpfCustomColor: alpha in this model is replaced by a custom color, usually the player color
# textures: texture flags
# Code:
# enum MeshTexture{
#        diffuse = 1,
#        specular = 2,
#        normal = 4
# };
# ================================
# 4. TEXTURE NAMES
# ================================
# A list of uint8[64] texture name values. One for each texture in the mesh. If there are no textures in the mesh no texture names are present.
# ================================
# 5. MESH DATA
# ================================
# After each mesh header and texture names the mesh data is placed.
# vertices: frameCount * vertexCount * 3, float32 values representing the x, y, z vertex coords for all frames
# normals: frameCount * vertexCount * 3, float32 values representing the x, y, z normal coords for all frames
# texture coords: vertexCount * 2, float32 values representing the s, t tex coords for all frames (only present if the mesh has 1 texture at least)
# indices: indexCount, uint32 values representing the indices
###########################################################################

import types
import string
import struct
import sys
from math import radians
from mathutils import Matrix
import subprocess
from os.path import dirname, abspath
from os import path
import os
from types import *
import bmesh
from bpy_extras.io_utils import ImportHelper, ExportHelper
from bpy_extras.image_utils import load_image
from bpy.props import StringProperty
import bpy
bl_info = {
    "name": "G3D Mesh Import/Export",
    "description": "Import/Export .g3d file (Glest 3D)",
    "author": "various, see head of script",
    "version": (0, 12, 1),
    "blender": (2, 90, 0),
    "location": "File > Import-Export",
    "warning": "always keep .blend files",
    "wiki_url": "http://glest.wikia.com/wiki/G3D_support",
    "tracker_url": "https://forum.megaglest.org/index.php?topic=6596",
    "category": "Import-Export"
}
###########################################################################
# Importing Structures needed (must later verify if i need them really all)
###########################################################################

###########################################################################
# Variables that are better Global to handle
###########################################################################
imported = []  # List of all imported Objects
toexport = []  # List of Objects to export (actually only meshes)
sceneID = None  # Points to the active Blender Scene


def unpack_list(list_of_tuples):
    l = []
    for t in list_of_tuples:
        l.extend(t)
    return l


###########################################################################
# Declaring Structures of G3D Format
###########################################################################


class G3DHeader:  # Read first 4 Bytes of file should be G3D + Versionnumber
    binary_format = "<3cB"

    def __init__(self, fileID):
        temp = fileID.read(struct.calcsize(self.binary_format))
        data = struct.unpack(self.binary_format, temp)
        self.id = str(data[0] + data[1] + data[2], "utf-8")
        self.version = data[3]


class G3DModelHeaderv3:  # Read Modelheader in V3 there is only the number of Meshes in file
    binary_format = "<I"

    def __init__(self, fileID):
        temp = fileID.read(struct.calcsize(self.binary_format))
        data = struct.unpack(self.binary_format, temp)
        self.meshcount = data[0]


# Read Modelheader: Number of Meshes and Meshtype (must be 0)
class G3DModelHeaderv4:
    binary_format = "<HB"

    def __init__(self, fileID):
        temp = fileID.read(struct.calcsize(self.binary_format))
        data = struct.unpack(self.binary_format, temp)
        self.meshcount = data[0]
        self.mtype = data[1]


class G3DMeshHeaderv3:  # Read Meshheader
    binary_format = "<7I64c"

    def __init__(self, fileID):
        temp = fileID.read(struct.calcsize(self.binary_format))
        data = struct.unpack(self.binary_format, temp)
        self.framecount = data[0]  # Framecount = Number of Animationsteps
        # Number of Normal Frames actualli equal to Framecount
        self.normalframecount = data[1]
        # Number of Frames of Texturecoordinates seems everytime to be 1
        self.texturecoordframecount = data[2]
        # Number of Frames of Colors seems everytime to be 1
        self.colorframecount = data[3]
        self.vertexcount = data[4]  # Number of Vertices in each Frame
        # Number of Indices in Mesh (Triangles = Indexcount/3)
        self.indexcount = data[5]
        self.properties = data[6]  # Property flags
        if self.properties & 1:  # PropertyBit is Mesh Textured ?
            self.hastexture = False
            self.diffusetexture = None
        else:
            self.diffusetexture = "".join(
                [str(x, "ascii") for x in data[7:-1] if x[0] < 127])
            self.hastexture = True
        if self.properties & 2:  # PropertyBit is Mesh TwoSided ?
            self.istwosided = True
        else:
            self.istwosided = False
        if self.properties & 4:  # PropertyBit is Mesh Alpha Channel custom Color in Game ?
            self.customalpha = True
        else:
            self.customalpha = False


class G3DMeshHeaderv4:  # Read Meshheader
    binary_format = "<64c3I8f2I"
    texname_format = "<64c"

    def _readtexname(self, fileID):
        temp = fileID.read(struct.calcsize(self.texname_format))
        data = struct.unpack(self.texname_format, temp)
        return "".join([str(x, "ascii") for x in data[0:-1] if x[0] < 127])

    def __init__(self, fileID):
        temp = fileID.read(struct.calcsize(self.binary_format))
        data = struct.unpack(self.binary_format, temp)
        # Name of Mesh every Char is a  String on his own
        self.meshname = "".join(
            [str(x, "ascii") for x in data[0:64] if x[0] < 127])
        self.framecount = data[64]  # Framecount = Number of Animationsteps
        self.vertexcount = data[65]  # Number of Vertices in each Frame
        # Number of Indices in Mesh (Triangles = Indexcount/3)
        self.indexcount = data[66]
        self.diffusecolor = data[67:70]  # RGB diffuse color
        self.specularcolor = data[70:73]  # RGB specular color (unused)
        self.specularpower = data[73]  # Specular power (unused)
        self.opacity = data[74]  # Opacity
        self.properties = data[75]  # Property flags
        self.textures = data[76]  # Texture flags

        self.customalpha = bool(self.properties & 1)
        self.istwosided = bool(self.properties & 2)
        self.noselect = bool(self.properties & 4)
        self.glow = bool(self.properties & 8)

        self.hastexture = False
        self.diffusetexture = None
        self.speculartexture = None
        self.normaltexture = None
        if self.textures:  # PropertyBit is Mesh Textured ?
            if self.textures & 1:  # diffuse
                self.diffusetexture = self._readtexname(fileID)
            if self.textures & 2:  # specular
                self.speculartexture = self._readtexname(fileID)
            if self.textures & 4:  # normal
                self.normaltexture = self._readtexname(fileID)

            self.hastexture = True
            # read all texture slots, otherwise it's read as data
            tex = self.textures >> 3
            while tex:
                tex &= tex - 1  # set rightmost 1-bit to 0
                # discard texture name, as we don't know what to do with it
                fileID.seek(struct.calcsize(self.texname_format))
                print("warning: ignored texture in undefined texture slot")


class G3DMeshdataV3:  # Calculate and read the Mesh Datapack
    def __init__(self, fileID, header):
        # Calculation of the Meshdatasize to load because its variable
        # Animationframes * Vertices per Animation * 3 (Each Point are 3 Float X Y Z Coordinates)
        vertex_format = "<%if" % int(
            header.framecount * header.vertexcount * 3)
        # The same for Normals
        normals_format = "<%if" % int(
            header.normalframecount * header.vertexcount * 3)
        # Same here but Textures are 2D so only 2 Floats needed for Position inside Texture Bitmap
        texturecoords_format = "<%if" % int(
            header.texturecoordframecount * header.vertexcount * 2)
        # Colors in format RGBA
        colors_format = "<%if" % int(header.colorframecount * 4)
        # Indices
        indices_format = "<%iI" % int(header.indexcount)
        # Load the Meshdata as calculated above
        self.vertices = struct.unpack(
            vertex_format, fileID.read(struct.calcsize(vertex_format)))
        self.normals = struct.unpack(
            normals_format, fileID.read(struct.calcsize(normals_format)))
        self.texturecoords = struct.unpack(
            texturecoords_format,
            fileID.read(struct.calcsize(texturecoords_format)))
        self.colors = struct.unpack(
            colors_format, fileID.read(struct.calcsize(colors_format)))
        self.indices = struct.unpack(
            indices_format, fileID.read(struct.calcsize(indices_format)))


class G3DMeshdataV4:  # Calculate and read the Mesh Datapack
    def __init__(self, fileID, header):
        # Calculation of the Meshdatasize to load because its variable
        # Animationframes * Points (Vertex) per Animation * 3 (Each Point are 3 Float X Y Z Coordinates)
        vertex_format = "<%if" % int(
            header.framecount * header.vertexcount * 3)
        # The same for Normals
        normals_format = "<%if" % int(
            header.framecount * header.vertexcount * 3)
        # Same here but Textures are 2D so only 2 Floats needed for Position inside Texture Bitmap
        texturecoords_format = "<%if" % int(header.vertexcount * 2)
        # Indices
        indices_format = "<%iI" % int(header.indexcount)
        # Load the Meshdata as calculated above
        self.vertices = struct.unpack(
            vertex_format, fileID.read(struct.calcsize(vertex_format)))
        self.normals = struct.unpack(
            normals_format, fileID.read(struct.calcsize(normals_format)))
        if header.hastexture:
            self.texturecoords = struct.unpack(
                texturecoords_format,
                fileID.read(struct.calcsize(texturecoords_format)))
        self.indices = struct.unpack(
            indices_format, fileID.read(struct.calcsize(indices_format)))


# Create a Mesh inside Blender


def createMesh(filename, header, data, toblender, operator):
    def tex_path(tex_filename):
        tex_file = os.path.join(dirname(abspath(filename)), tex_filename).replace('\00', '')
        if os.path.exists(tex_file):
            return tex_file
        else:
            operator.report({'WARNING'},
                "Bad texture filename found in g3d: {}".format(tex_file))
            tex_file = tex_file[:tex_file.rfind(".")]
            for fmt in ('png', 'jpeg', 'tga', 'bmp'):
                new_filename = "{}.{}".format(tex_file, fmt)
                print(new_filename)
                if os.path.exists(new_filename):
                    return new_filename
        raise IOError("Could not find image texture file.")

    mesh = bpy.data.meshes.new(header.meshname)  # New Mesh
    # New Object for the new Mesh
    meshobj = bpy.data.objects.new(header.meshname + 'Object', mesh)
    # scene = bpy.context.scene
    # scene.objects.link(meshobj)
    # scene.update()
    col = bpy.data.collections.get("Collection")
    col.objects.link(meshobj)
    uvcoords = []
    img_diffuse = None
    img_specular = None
    img_normal = None
    if header.hastexture:  # Load Texture when assigned
        try:
            texturefile = tex_path(header.diffusetexture)
            img_diffuse = bpy.data.images.load(texturefile)
            for x in range(0, len(data.texturecoords), 2):  # Prepare the UV
                uvcoords.append(
                    [data.texturecoords[x], data.texturecoords[x + 1]])

            if header.isv4:
                if header.speculartexture:
                    texturefile = tex_path(header.speculartexture)
                    img_specular = bpy.data.images.load(texturefile)
                if header.normaltexture:
                    texturefile = tex_path(header.normaltexture)
                    img_normal = bpy.data.images.load(texturefile)
        except:
            import traceback
            traceback.print_exc()

            header.hastexture = False
            operator.report({'WARNING'},
                            "Couldn't load texture. See console for details.")

    vertsCO = []
    vertsNormal = []

    # Get the Vertices into an empty mesh. This is difficult because
    # we have to pick a frame in the mesh where all the verts are
    # different values. Otherwise it loses verts.
    # TODO: It's possible to create a mesh where all the verts aren't
    # different values in any frame. That could still cause problems.
    # Figure out a better solution :p (e.g. moving verts to random
    # different positions).
    # (We tried putting the verts in a line, to prevent merging.
    # This was a problem because there are some verts that need merging.)
    header_most_diff = 0 # This is the header we will use to define the mesh.
    n_max_diff = 0 # Max number of "different" vertices found in the frame.
    for x in range(0, header.framecount):
        n_diff_verts = 0 # Current number of "different" vertices.
        # Find nubmer of different verts
        diff_verts = set()
        n_diff_verts = 0
        for i in range(0, header.vertexcount * 3, 3):
            vert = tuple(data.vertices[x * header.vertexcount * 3 + i:x * header.vertexcount * 3 + i + 3])
            if (vert not in diff_verts):
                n_diff_verts += 1
                diff_verts.add(vert)

        if n_diff_verts == header.vertexcount:
            # if we've found a frame where all the verts are different, we don't need to do anymore searching.
            header_most_diff = x
            n_max_diff = n_diff_verts # Currently not needed, may be useful when improving this code.
            break
        
        if n_diff_verts > n_max_diff:
            # If the number of different verts is greater than
            # the previous match, this frame is the new max.
            n_max_diff = n_diff_verts
            header_most_diff = x
        
    # Get the Vertices and Normals into empty Mesh
    for x in range(0, header.vertexcount * 3, 3):
        vertsCO.append(tuple(data.vertices[header_most_diff*header.vertexcount*3+x:header_most_diff*header.vertexcount*3+x+3]))
        vertsNormal.extend([(data.normals[x], data.normals[x + 1],
                            data.normals[x + 2])])

    faces = []
    faceuv = []
    for i in range(0, len(data.indices), 3):  # Build Faces into Mesh
        faces.append(
            [data.indices[i], data.indices[i + 1], data.indices[i + 2]])
        if header.hastexture:
            uv = []
            u0 = uvcoords[data.indices[i]][0]
            v0 = uvcoords[data.indices[i]][1]
            uv.append([u0, v0])
            u1 = uvcoords[data.indices[i + 1]][0]
            v1 = uvcoords[data.indices[i + 1]][1]
            uv.append([u1, v1])
            u2 = uvcoords[data.indices[i + 2]][0]
            v2 = uvcoords[data.indices[i + 2]][1]
            uv.append([u2, v2])
            faceuv.append([uv, 0, 0, 0])
        else:
            uv = []
            uv.append([0, 0])
            uv.append([0, 0])
            uv.append([0, 0])
            faceuv.append([uv, 0, 0, 0])
    mesh.from_pydata(vertsCO, [], faces)
    mesh.vertices.foreach_set("co", unpack_list(vertsCO))
    mesh.vertices.foreach_set("normal", unpack_list(vertsNormal))
    # mesh.tessfaces.add(len(faces) // 4)
    # mesh.tessfaces.foreach_set("vertices_raw", faces)
    # mesh.tessfaces.foreach_set("use_smooth", [True] * len(mesh.tessfaces))
    mesh.polygons.foreach_set(
        "use_smooth", (True,)*len(mesh.polygons.data.polygons))
    mesh.g3d_customColor = header.customalpha
    mesh.show_double_sided = header.istwosided
    if header.isv4:
        mesh.g3d_noSelect = header.noselect
        mesh.g3d_glow = header.glow
    else:
        mesh.g3d_noSelect = False
        mesh.glow = False

    # ===================================================================================================
    # Material Setup
    # ===================================================================================================

    def addtexslot(matdata, index, name, img):
        texture = bpy.data.textures.new(name=name, type='IMAGE')
        texture.image = img
        slot = matdata.texture_slots.create(index)
        slot.texture = texture
        slot.texture_coords = 'UV'

    if header.hastexture:
        materialname = "pskmat"
        materials = []
        matdata = bpy.data.materials.new(materialname + '1')
        matdata.use_backface_culling = not header.istwosided
        # Show alpha.
        matdata.blend_method = 'BLEND'
        # Shader node tree.
        matdata.use_nodes = True
        node_tree = matdata.node_tree
        shader_node = node_tree.nodes['Principled BSDF']
        # Diffuse image
        diff_img_node = node_tree.nodes.new('ShaderNodeTexImage')
        node_tree.links.new(
            diff_img_node.outputs['Color'], shader_node.inputs['Base Color'])
        node_tree.links.new(
            diff_img_node.outputs['Alpha'], shader_node.inputs['Alpha'])
        diff_img_node.image = img_diffuse

        # addtexslot(matdata, 0, 'diffusetexture', img_diffuse)
        if img_specular:
            spec_img_node = node_tree.nodes.new('ShaderNodeTexImage')
            node_tree.links.new(
                spec_img_node.outputs['Alpha'], diff_node.inputs['Specular'])
            spec_img_node.image = img_specular
        if img_normal:
            norm_node = node_tree.nodes.new('ShaderNodeNormalMap')
            node_tree.links.new(
                norm_node.outputs['Normal'], diff_node.inputs['Normal'])
            norm_img_node = node_tree.nodes.new('ShaderNodeTexImage')
            node_tree.links.new(
                norm_img_node.outputs['Color'], norm_node.inputs['Color'])
            norm_img_node.image = img_normal

        if header.isv4:
            shader_node.inputs['Subsurface Color'].default_value = header.diffusecolor + (1,)
            shader_node.inputs['Emission'].default_value = header.specularcolor + (1,)
            shader_node.inputs['Alpha'].default_value = header.opacity
        materials.append(matdata)

        for material in materials:
            # add material to the mesh list of materials
            mesh.materials.append(material)

        countm = 0
        psktexname = "psk" + str(countm)
        mesh.uv_layers.new(name=psktexname)
        if (len(faceuv) > 0):
            uv_dat = mesh.uv_layers[psktexname].data
            for i in range(len(faceuv)):
                for j in range(3):
                    uv_dat[i*3+j].uv = faceuv[i][0][j]

    imported.append(meshobj)  # Add to Imported Objects
    sk = meshobj.shape_key_add()
    for x in range(0, header.framecount): # Put in Vertex Positions for Keyanimation
        sk = meshobj.shape_key_add()
        for i in range(0, header.vertexcount * 3, 3):
            sk.data[i // 3].co[0] = data.vertices[x * header.vertexcount * 3 +
                                                  i]
            sk.data[i // 3].co[1] = data.vertices[x * header.vertexcount * 3 +
                                                  i + 1]
            sk.data[i // 3].co[2] = data.vertices[x * header.vertexcount * 3 +
                                                  i + 2]

    # activate one shapekey per frame
    for i in range(0, header.framecount):
        shape = mesh.shape_keys.key_blocks[i+1]
        shape.value = 0.0
        shape.keyframe_insert("value", frame=i)
        shape.value = 1.0
        shape.keyframe_insert("value", frame=(i + 1))
        shape.value = 0.0
        shape.keyframe_insert("value", frame=(i + 2))

    meshobj.active_shape_key_index = 0

    if toblender:
        # rotate from glest to blender orientation
        # mesh.transform( Matrix( ((1,0,0,0),(0,0,-1,0),(0,1,0,0),(0,0,0,1)) ) )
        # doesn't work, maybe because of shape keys
        # use object transformation instead
        meshobj.rotation_euler = (radians(90), 0, 0)

    # update polygon structures from tessfaces
    mesh.update()
    mesh.update_tag()

    # remove duplicates
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.0001)
    bm.to_mesh(mesh)
    bm.free()

    return


###########################################################################
# Import
###########################################################################


def G3DLoader(filepath, toblender, operator):  # Main Import Routine
    global imported, sceneID
    print("\nNow Importing File: " + filepath)
    fileID = open(filepath, "rb")
    header = G3DHeader(fileID)
    print("\nHeader ID         : " + header.id)
    print("Version           : " + str(header.version))
    if header.id != "G3D":
        print("ERROR: This is Not a G3D Model File")
        operator.report({'ERROR'}, "This is Not a G3D Model File")
        fileID.close
        return
    if header.version not in (3, 4):
        print("ERROR: The Version of this G3D File is not Supported")
        operator.report({'ERROR'},
                        "The Version of this G3D File is not Supported")
        fileID.close
        return
    # in_editmode = Blender.Window.EditMode()             #Must leave Editmode when active
    # if in_editmode: Blender.Window.EditMode(0)
    sceneID = bpy.context.scene  # Get active Scene
    # scenecontext=sceneID.getRenderingContext()          #To Access the Start/Endframe its so hidden i searched till i got angry :-)
    # Generate the Base Filename without Path + extension
    basename = os.path.basename(filepath).split('.')[0]
    imported = []
    maxframe = 0
    if header.version == 3:
        modelheader = G3DModelHeaderv3(fileID)
        print("Number of Meshes  : " + str(modelheader.meshcount))
        for x in range(modelheader.meshcount):
            meshheader = G3DMeshHeaderv3(fileID)
            meshheader.isv4 = False
            print("\nMesh Number         : " + str(x + 1))
            print("framecount            : " + str(meshheader.framecount))
            print("normalframecount      : " +
                  str(meshheader.normalframecount))
            print("texturecoordframecount: " +
                  str(meshheader.texturecoordframecount))
            print("colorframecount       : " + str(meshheader.colorframecount))
            print("pointcount            : " + str(meshheader.vertexcount))
            print("indexcount            : " + str(meshheader.indexcount))
            print("texturename           : " + str(meshheader.diffusetexture))
            print("hastexture            : " + str(meshheader.hastexture))
            print("istwosided            : " + str(meshheader.istwosided))
            print("customalpha           : " + str(meshheader.customalpha))
            # Generate Meshname because V3 has none
            meshheader.meshname = basename + str(x + 1)
            if meshheader.framecount > maxframe:
                maxframe = meshheader.framecount  # Evaluate the maximal animationsteps
            meshdata = G3DMeshdataV3(fileID, meshheader)
            createMesh(filepath, meshheader, meshdata, toblender, operator)
        fileID.close
        bpy.context.scene.frame_start = 1
        bpy.context.scene.frame_end = maxframe
        bpy.context.scene.frame_current = 1

        anchor = bpy.data.objects.new('Empty', None)
        col = bpy.data.collections.get("Collection")
        col.objects.link(anchor)
        anchor.select_set(True)

        for ob in imported:
            ob.parent = anchor
        bpy.context.view_layer.update()

        return

    if header.version == 4:
        modelheader = G3DModelHeaderv4(fileID)
        print("Number of Meshes  : " + str(modelheader.meshcount))
        for x in range(modelheader.meshcount):
            meshheader = G3DMeshHeaderv4(fileID)
            meshheader.isv4 = True
            print("\nMesh Number   : " + str(x + 1))
            print("meshname        : " + str(meshheader.meshname))
            print("framecount      : " + str(meshheader.framecount))
            print("vertexcount     : " + str(meshheader.vertexcount))
            print("indexcount      : " + str(meshheader.indexcount))
            print("diffusecolor    : %1.6f %1.6f %1.6f" %
                  meshheader.diffusecolor)
            print("specularcolor   : %1.6f %1.6f %1.6f" %
                  meshheader.specularcolor)
            print("specularpower   : %1.6f" % meshheader.specularpower)
            print("opacity         : %1.6f" % meshheader.opacity)
            print("properties      : " + str(meshheader.properties))
            print("textures        : " + str(meshheader.textures))
            print("texturename     : " + str(meshheader.diffusetexture))
            print("istwosided      : " + str(meshheader.istwosided))
            if len(meshheader.meshname) == 0:  # When no Meshname in File Generate one
                meshheader.meshname = basename + str(x + 1)
            if meshheader.framecount > maxframe:
                maxframe = meshheader.framecount  # Evaluate the maximal animationsteps
            meshdata = G3DMeshdataV4(fileID, meshheader)
            createMesh(filepath, meshheader, meshdata, toblender, operator)
        fileID.close

        bpy.context.scene.frame_start = 1
        bpy.context.scene.frame_end = maxframe
        bpy.context.scene.frame_current = 1
        anchor = bpy.data.objects.new('Empty', None)
        col = bpy.data.collections.get("Collection")
        col.objects.link(anchor)
        anchor.select_set(True)
        for ob in imported:
            ob.parent = anchor
        bpy.context.view_layer.update()

        def tex_all_view3d_area():
            '''Gets the 3d viewport. It is relative to whatever area the python script runs in.
            Returns None if there's no area.'''
            for screen in bpy.data.screens:
                for ar in screen.areas.values():
                    if ar.type == 'VIEW_3D':
                        tex_view3d(ar)
        
        
        def tex_view3d(area):
            '''Sets the viewport colour to '(unshaded) texture'.'''
            # Check there is a 3d view.
            if area is not None:
                area.spaces[0].shading.type = 'SOLID'
                area.spaces[0].shading.color_type = 'TEXTURE'
                area.spaces[0].shading.show_specular_highlight = False

        tex_all_view3d_area()
        print(
            "Created a empty Object as 'Grip' where all imported Objects are parented to"
        )
        print(
            "To move the complete Meshes only select this empty Object and move it"
        )
        print("All Done, have a good Day :-)\n\n")
        return


def G3DSaver(filepath, context, toglest, operator):
    print("\nNow Exporting File: " + filepath)

    objs = context.selected_objects
    if len(objs) == 0:
        objs = bpy.data.objects

    # get real meshcount as len(bpy.data.meshes) holds also old meshes
    meshCount = 0
    for obj in objs:
        if obj.type == 'MESH':
            meshCount += 1
            if obj.mode != 'OBJECT':  # we want to be in object mode
                print("ERROR: mesh not in object mode")
                operator.report({'ERROR'}, "mesh not in object mode")
                return -1

    if meshCount == 0:
        print("ERROR: no meshes found")
        operator.report({'ERROR'}, "no meshes found")
        return -1

    fileID = open(filepath, "wb")
    # G3DHeader v4
    fileID.write(struct.pack("<3cB", b'G', b'3', b'D', 4))
    # G3DModelHeaderv4
    fileID.write(struct.pack("<HB", meshCount, 0))
    # meshes
    # for mesh in bpy.data.meshes:
    for obj in objs:
        context.view_layer.objects.active = obj
        if obj.type != 'MESH':
            continue
        mesh = obj.data
        diffuseColor = [1.0, 1.0, 1.0]
        specularColor = [0.9, 0.9, 0.9]
        opacity = 1.0
        textures = 0
        texnames = []
        if len(mesh.materials) > 0:
            try:
                # we have a texture, hopefully
                material = mesh.materials[0].node_tree.nodes['Principled BSDF']
                diff_tex = material.inputs['Base Color'].links[0].from_node
                # only look for other textures when we have diffuse
                # if slot and slot.texture.type == 'IMAGE' and len(mesh.uv_textures) > 0:
                diffuseColor = material.inputs['Subsurface Color'].default_value[:3]
                specularColor = material.inputs['Emission'].default_value[:3]
                opacity = material.inputs['Alpha'].default_value
                textures = 1
                texnames.append(bpy.path.basename(diff_tex.image.filepath))
                # specular and normal
            except IndexError:
                print("Could not find diffuse texture.")
            else:
                try:
                    spec_tex = material.inputs['Specular'].links[0].from_node
                    texnames.append(bpy.path.basename(spec_tex.image.filepath))
                    textures |= 1 << 1
                except IndexError:
                    print("No specular texture found.")
                try:
                    norm_tex = material.inputs['Normal'].links[0].from_node.inputs['Color'].links[0].from_node
                    texnames.append(bpy.path.basename(norm_tex.image.filepath))
                    textures |= 1 << 2
                except IndexError:
                    print("No normal texture found.")

            # else:
            #     print(
            #         "WARNING: first texture slot in first material isn't of type IMAGE or it's not unwrapped, texture ignored"
            #     )
            #     operator.report({
            #         'WARNING'
            #     }, "first texture slot in first material isn't of type IMAGE or it's not unwrapped, texture ignored"
            #     )
                # continue without texture

        meshname = mesh.name
        frameCount = context.scene.frame_end - context.scene.frame_start + 1
        realFaceCount = 0  # real face count (triangles)
        indices = []  # list of indices
        newverts = []  # list of vertex indices which need to be duplicated
        uvlist = []  # list of texcoords
        # tesselate n-polygons to triangles & quads
        bpy.ops.object.editmode_toggle()
        bpy.ops.mesh.select_all(action='SELECT')
        bpy.ops.mesh.quads_convert_to_tris()
        bpy.ops.object.editmode_toggle()

        # We have to update the mesh reference after any mesh modification (e.g. quads_to_tris()).
        mesh = obj.data
        # mesh.update(calc_tessface=True)
        if textures:
            # uvtex = mesh.tessface_uv_textures[0]
            uvlist[:] = [[0] * 2 for i in range(len(mesh.vertices))]
            # blender allows to have multiple texcoords per vertex,
            # in g3d format every vertex can only have one texcoord
            # -> duplicate vertex
            # the dictionary/map vdict collects all the stuff
            # index to "unique" vertices from blender
            #   -> tuple( list of texcoords, list of indices to the duplicated vertex )
            vdict = dict()
            nextIndex = len(mesh.vertices)
            uvdata = mesh.uv_layers[0].data
            uvindex = 0

            # for face in mesh.tessfaces:
            for face in mesh.polygons:  
                # when a vertex is duplicated it gets a new index, so the
                # triple of indices describing the face is different too
                faceindices = []
                realFaceCount += 1
                # uvdata = uvtex.data[face.index]
                # for i in range(3):
                for vindex in face.vertices:
                    # closure, got rid of copy&paste, still looking weird
                    def getTexCoords():
                        nonlocal nextIndex, vdict, uvlist, newverts, vindex
                        # vindex = face.vertices[i]
                        if vindex not in vdict:  # new vertex -> add it
                            vdict[vindex] = [uvdata[uvindex].uv], [vindex]
                            # that's a (s,t)-pair
                            uvlist[vindex] = uvdata[uvindex].uv
                        else:
                            found = False
                            idx = 0
                            for ele in vdict[vindex][0]:
                                if uvdata[uvindex].uv[0] == ele[0] and uvdata[uvindex].uv[1] == ele[1]:
                                    found = True
                                    break
                                idx += 1
                            if found:  # same vertex and texcoord before
                                # it could be a different index now, the index of a new
                                # duplicated vertex
                                # vindex = vdict[vindex][1][ vdict[vindex][0].index(uvdata.uv[i]) ]
                                vindex = vdict[vindex][1][idx]
                            else:  # same vertex as before but with different texcoord -> duplicate
                                vdict[vindex][0].append(uvdata[uvindex].uv)
                                vdict[vindex][1].append(nextIndex)

                                # duplicate vertex because it takes part in different faces
                                # with different texcoords
                                newverts.append(vindex)
                                uvlist.append(uvdata[uvindex].uv)
                                # new index for the duplicated vertex
                                vindex = nextIndex
                                nextIndex += 1

                        faceindices.append(vindex)

                    getTexCoords()
                    uvindex += 1
                indices.extend(faceindices)

                if len(face.vertices) == 4:
                    faceindices = []
                    realFaceCount += 1
                    for i in [0, 2, 3]:
                        getTexCoords()
                    indices.extend(faceindices)
        else:
            for face in mesh.polygons:
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
            fileID.close()
            return -1
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
        if mesh.g3d_glow:
            properties |= 8

        #MeshData
        vertices = []
        normals = []
        fcurrent = context.scene.frame_current
        depsgraph = bpy.context.evaluated_depsgraph_get()

        for i in range(context.scene.frame_start, context.scene.frame_end+1):
            context.scene.frame_set(i)
            #FIXME: not sure what's better: PREVIEW or RENDER settings
            bm = bmesh.new()
            bm.from_object(obj, depsgraph=depsgraph)
            bm.transform(obj.matrix_world)  # apply object-mode transformations

            if toglest:
                # rotate from blender to glest orientation
                bm.transform( Matrix( ((1,0,0,0),(0,0,1,0),(0,-1,0,0),(0,0,0,1)) ) )
                # transform normals too
                bm.normal_update()

            for vertex in bm.verts:
                vertices.extend(vertex.co)
                normals.extend(vertex.normal)

            # duplicate vertices and corresponding normals, for every frame
            for nv in newverts:
                verts = [v for v in bm.verts]
                vertices.extend(verts[nv].co)
                normals.extend(verts[nv].normal)

        context.scene.frame_set(fcurrent)


        # MeshHeader
        fileID.write(
            struct.pack("<64s3I8f2I", bytes(meshname, "ascii"), frameCount,
                        vertexCount, indexCount, diffuseColor[0],
                        diffuseColor[1], diffuseColor[2], specularColor[0],
                        specularColor[1], specularColor[2], specularPower,
                        opacity, properties, textures))
        # Texture names
        if textures:  # only when we have textures
            for i in range(len(texnames)):
                fileID.write(struct.pack("<64s", bytes(texnames[i], "ascii")))

        # see G3DMeshdataV4
        vertex_format = "<%if" % int(frameCount * vertexCount * 3)
        normals_format = "<%if" % int(frameCount * vertexCount * 3)
        texturecoords_format = "<%if" % int(vertexCount * 2)
        indices_format = "<%iI" % int(indexCount)

        fileID.write(struct.pack(vertex_format, *vertices))
        fileID.write(struct.pack(normals_format, *normals))

        # texcoords
        if textures:  # only when we have textures
            texcoords = []
            for uv in uvlist:
                texcoords.extend(uv)
            fileID.write(struct.pack(texturecoords_format, *texcoords))

        fileID.write(struct.pack(indices_format, *indices))

    fileID.close()
    return 0


# ---=== Register ===
class G3DPanel(bpy.types.Panel):
    # bl_idname = "OBJECT_PT_G3DPanel"
    bl_label = "G3D properties"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "data"

    @classmethod
    def poll(cls, context):
        return (context.object is not None and context.object.type == 'MESH')

    def draw(self, context):
        self.layout.prop(context.object.data, "g3d_customColor")
        self.layout.prop(context.object.data,
                         "show_double_sided",
                         text="double sided")
        self.layout.prop(context.object.data, "g3d_noSelect")
        self.layout.prop(context.object.data, "g3d_glow")


class ImportG3D(bpy.types.Operator, ImportHelper):
    '''Load a G3D file'''
    bl_idname = "importg3d.g3d"
    bl_label = "Import G3D"

    filename_ext = ".g3d"
    filter_glob = StringProperty(default="*.g3d", options={'HIDDEN'})

    toblender = bpy.props.BoolProperty(
        name="rotate to Blender orientation",
        description="Rotate meshes from Glest to Blender orientation",
        default=True)

    def execute(self, context):
        try:
            G3DLoader(self.filepath, self.toblender, self)
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

    # export options
    showg3d = bpy.props.BoolProperty(
        name="show G3D afterwards",
        description=("Run g3dviewer to show G3D after export. "
                     "g3dviewer needs to be in the scripts directory, "
                     "otherwise the associated program of .g3d is run."),
        default=False)
    toglest = bpy.props.BoolProperty(
        name="rotate to glest orientation",
        description="Rotate meshes from Blender to Glest orientation",
        default=True)

    def execute(self, context):
        try:
            res = G3DSaver(self.filepath, context, self.toglest, self)
            if res == 0 and self.showg3d:
                print("opening g3dviewer with " + self.filepath)
                scriptsdir = bpy.utils.script_path_user()
                dname = os.path.dirname(self.filepath)
                found = False
                for f in os.listdir(scriptsdir):
                    if "g3dviewer" in f:
                        f = os.path.join(scriptsdir, f)
                        if os.path.isfile(f) and os.access(f, os.X_OK):
                            cmd = [f, self.filepath]
                            print(cmd)
                            subprocess.Popen(cmd, cwd=dname)
                            found = True

                # try default associated program
                if not found:
                    if os.name == 'posix':
                        # xdg-open is only a shell script which delegates the job to a
                        # desktop specific program, e.g. if DE=kde than kde-open
                        # needs DE environment variable set, otherwise it just throws it
                        # at the browser, which is not very helpful
                        print("running xdg-open " + self.filepath)
                        subprocess.Popen(['xdg-open', self.filepath],
                                         cwd=dname)
                    elif os.name == 'mac':
                        subprocess.Popen(['open', self.filepath], cwd=dname)
                    elif os.name == 'nt':
                        # os.startfile(self.filepath) # no way to change dir
                        subprocess.Popen(['cmd', '/C', 'start', self.filepath],
                                         cwd=dname)

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
    bpy.types.Mesh.g3d_customColor = bpy.props.BoolProperty(
        name="team color",
        description="replace alpha channel of texture with team color")
    bpy.types.Mesh.g3d_noSelect = bpy.props.BoolProperty(
        name="non-selectable", description="click on mesh doesn't select unit")
    bpy.types.Mesh.g3d_glow = bpy.props.BoolProperty(
        name="glow", description="let objects glow like particles")
    bpy.types.Mesh.show_double_sided = bpy.props.BoolProperty(
        name="double_sided", description="render the object double sided")

    for classes in (G3DPanel, ImportG3D, ExportG3D):
        bpy.utils.register_class(classes)

    bpy.types.TOPBAR_MT_file_import.append(menu_func_import)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)


def unregister():
    for classes in (G3DPanel, ImportG3D, ExportG3D):
        bpy.utils.register_class(classes)

    bpy.types.TOPBAR_MT_file_import.remove(menu_func_import)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)


if __name__ == '__main__':
    register()
#    main()

# for obj in bpy.data.objects:
#    if obj.type == 'MESH':
#        obj.select = True
#        bpy.ops.object.delete()
# G3DLoader("import.g3d", True, None)

# for obj in bpy.context.selected_objects:
#    obj.select = False  # deselect everything, so we get it all
# G3DSaver("test.g3d", bpy.context)
