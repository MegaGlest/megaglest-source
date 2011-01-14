#!BPY
# coding: utf-8
"""
Name: 'G3D XML Exporter Ver:1.1'
Blender: 2.43
Group: 'Export'
Tooltip: 'Exports directly to G3D(if xml2g is installed next to this script) or XML (which can be converted to .g3d using xml2g)'
"""
###########################################################################
# Glest Model / Texture / Animation Exporter
# for the game Glest that you can find at http://www.glest.org
# copyright 2005-2006 By Vincent Gadoury
# Started Date: December 18 2005  Put Public Decembre 20 2005
# Ver: 1.0        (Jan 30 2009)
# Distributed under the GNU PUBLIC LICENSE
# Based on g3d_support.py by Andreas Becker
#     and on glexemel by Jonathan Merritt
###########################################################################
#       (I'm sorry for the poor quality of this text,
#                   I'm not a native English speaker)
#
#INSTALLATION :
#       Copy this Script into your .blender\scripts directory.
#       if you want direct .g3d support unpack all needed glexemel files to this directory too:
#       LINUX:
#       In case of linux its only: xml2g and g3d.dtd ( included is a binary for linux32 bit).
#       The sourcecode can be found here:
#       http://www.glest.org/files/contrib/tools/glexemel-0.1a.zip
#       WINDOWS:
#       If you are using windows, all files from the "bin" directory from the 
#       following zip files are needed:
#       http://www.glest.org/files/contrib/tools/glexemel_win32_0.1a.zip
#
#       In Blender, open a Script Window and in the menu choose "Update Menus"
#       Export with File->Export->G3D XML Exporter
#
#PREPARE THE MODEL :
#       Each of your meshes' faces must be a triangle (3 vertex)
#       In Edit mode, you can press Space, edit>faces>Convert to Triangles
#       Create and place all your meshes before texturing or animating.
#       The X axe is the floor and the Y axe is the elevation.  The center
#       of the Blender's 3D space will be the center of you object, at the
#       ground level.  Before exporting your model, apply the transformations
#       to each of your mesh (In the 3D space window's menu :
#                           Object->Clear/Apply->Apply ... )
#       This will place the center of your mesh (the yellow or pink dot)
#       at the center of the Blender 3D space.
#
#       !!!!!!!!!!!!!!       
#       !!!new since 0.1.4: !!!
#       If you have a non animating model, you have to set the start frame 
#       and the end frame to '1' in blenders 'timeline'-window. Otherwise a non 
#       animating animation will be exported, containing a lot of frames and your
#       model gets really big.
#       !!!!!!!!!!!!!!
#
#       Before doing any texturing or animation, try to export your model and
#       check your python console (the terminal which opens Blender) to see
#       if there is a warning about the number of used faces in the summary.
#       If there is, try to delete "false" faces (which use only 2 vertices)
#       and transform all your faces in triangle.
#       
#EXPORTING :
#       You MUST select the meshes you want to export.
#       Only meshes are exported among the selected objects.
#
#       The diffuse color of the mesh will be it's (first) material's color.
#       The opacity of the mesh will be it's material's alpha.
#       Double sided property of the mesh is exported (see F9).
#       If you want to use custom color in the texture, your mesh must be
#       double sided. If you don't want custom color to be activated for the
#       mesh, add a new boolean property to it (F4) named customColor and with
#       the false value.  Custom Color set if the alpha of the texture is
#       replaced for the color of the player.
#       
#TEXTURING :
#       To use a texture for the mesh, you must add a texture at the FIRST
#       position to the mesh's material. The texture type must be 'image'
#       and you should insert the real image.  In fact, only the filename
#       of the image is used.  After having mapped the image on the
#       mesh with UV Face selection, you don't need anymore to "stick"
#       the coordinates to the vertices.
#       Your texture's format must be TGA, without compression and with the origin
#       at the bottom left position.  The side of the image must be a multiple
#       of 2.  In practice, you must use only one image for all your meshes.
#
#ANIMATING :
#       You can export all animations made with blenders armature system.
#       You have to set the start frame and the end frame in the 'timeline'-window
#       to define the animation which will be exported. 
#       If you animate without using the armatures it will not work!!
#
#       LINKS :
#       Animating with armatures
#               http://www.blender.org/documentation/htmlI/x1829.html
#       Map an image on your mesh with UV Face selection :
#               http://en.wikibooks.org/wiki/Blender_3D:_Noob_to_Pro/UV_Map_Basics
#               http://download.blender.org/documentation/htmlI/ch11s05.html
#
#CONVERT TO .G3D
#       The following is no longer needed if you copied g3d.dtd and xml2g(.exe) next 
#       to this script:
#
#       Use the xml2g program of the glexemel tool at
#       http://www.glest.org/files/contrib/tools/
#       Syntax : ./xml2g ExportedFile.xml DesiredFile.g3d
#       
#       WARNING : The current version of this program don't seems to work...
#                 You'll have to add the lines (if they aren't there) :
#                       #pragma pack(push, 1)
#                   near of the beginning of the file "g3dv4.h" of glexemel and
#                       #pragma pack(pop)
#                   before the #endif
#
#
#ChangeLog :
#       1.1: by Titus Tscharntke (titi)
#            fixed bug in windows ( problems with file sparator char ) 
#       1.0: Big thanks go to Frank Tetzel (Yggdrasil) who finally made a 
#            direct export to .g3d possible. If you copy g3d.dtd and 
#            xml2g(.exe) next to this script it will export to .g3d. 
#            If not, it will export to .xml like before.
#       
#       0.1.7 : by Titus Tscharntke (titi)
#           removed non 7-bit ascii characters for newer versions of python
#	0.1.6 : by justWeedy(weedkiller)
#	    Basic colorsettings for maps are often unused but may cause 
#           models look "bright". 
#           There are Hardcoded defauld values to avoid this. 
#           Turn On/Off due the boolean <useHardCodedColor>
#       0.1.5 : by weedkiller 
#           You don't have to apply scale and rotation anymore
#       0.1.4 : by weedkiller and Titus Tscharntke ( www.titusgames.de )
#           New implementation of the animation export ( including some usage changes!!! )
#           Exporting animations is now possible with newer Blender versions ( tested with 2.43 )
#
#       0.1.3 : by Titus Tscharntke ( www.titusgames.de )
#           Fixed a bug with getProperty() in blender 2.43
#
#       0.1.2 :
#           Correcting an execution bug
#           Adding the "frameCount" property for each object
#           Using only the triangle faces for the index count
#               (problems of linking between index and vertex may occur)
#           Adding the summary display in the python console
#           Adding a better documentation
#       0.1.1b :
#           Adding a vertex duplication method to correct the vertex UV problem.
#           Correcting NMVert.uvco misuse
#
#ToDo:
#       More tests...
#       More validations...
#       Warn the user if there is something wrong, e.g. :
#                   - no mesh among the selected objects;
#                   - texture without uvco;
#                   - no material on a mesh.
#       Test and correct the theoric linking problem if faces are ignored...
#
#Contributions :
#       pseudonym404 : NMVert.uvco issue and solutions
#
###############################################################################

import Blender
from Blender import NMesh
from Blender import sys as bsys

import subprocess, os


# part Hardcoded Color >>
HardCodedDiffuse=0.588235
HardCodedSpecular=0.900000
useHardCodedColor='true'  #'false' if not wanted
# part Hardcoded Color <<






seenindex=set()
uvlist=[]

# This list will contain the associative "real" vertex of each duplicated vertex
# in ascendant order.
# The fisrt "vertex" of the list (newvertices[0]) has the index len(mesh.verts)
newvertices=[]
currentnewvertex=0

#This list will contain all the "new" faces' indices
newindices=[]




def icmp(x,y):
    return cmp(x,y)

def notseenindex(index):
    seen = index in seenindex
    if not seen: seenindex.add(index)
    return not seen

# this will return the properties data if it exists, else the given defaultvalue
def getPropertyIfExists(blenderobject,propertyName,defaultvalue):
    propertiesList = blenderobject.getAllProperties()
    lpropertiesList = range( 0 , len(propertiesList) )
    for iprop in lpropertiesList:
        objectProperty = propertiesList[iprop]
        currentPropertyName=objectProperty.getName()
        if currentPropertyName == propertyName: 
            return objectProperty.getData()
    return defaultvalue


def write_obj(filepath):
    out = file(filepath, 'w')
   
    print("-----------------------------------------------")
   
    #Header
    out.write( '<?xml version="1.0" encoding="ASCII" ?>\n' )
    out.write( '<!DOCTYPE G3D SYSTEM "g3d.dtd">\n' )
    out.write( '<!-- \n' )
    out.write( '    This file is an XML encoding of a G3D binary\n' )
    out.write( '    file.  The XML format is by Jonathan Merritt\n' )
    out.write( '    (not yet accepted as part of Glest!).\n' )
    out.write( '    The file was exported from Blender with the\n' )
    out.write( '    G3D-XML Exporter script by Vincent Gadoury.\n' )
    out.write( '-->\n' )
    out.write( '<G3D version="4">\n' )
   
    objects = Blender.Object.GetSelected()
   
    print("Exporting %i selected objects to XML-G3D format..." %(len(objects)))

    #FOR EACH MESH
    lobjects = range( 0 , len(objects) )
    for iobj in lobjects:
        object = objects[iobj]
       
        mesh = object.getData()
        # Skip the object if it's not a mesh
        if type(mesh) != Blender.Types.NMeshType :
            continue
       
        #Clear the lists
        seenindex.clear()
        uvlist[:]=[]
        newvertices[:]=[]
        newindices[:]=[]
           
        currentnewvertex=(len(mesh.verts))
       
        #Find some properties of the mesh
       
        image = None
        textureName = ''
        diffuseTexture = 'false'
        opacity = 1
        diffuseColor = [ 1.0, 1.0, 1.0 ]
       
        #Find if the mesh has a material and a texture
        #   (opacity, diffuseColor, diffuseTexture, textureName)
        if len(mesh.materials) > 0:
            material = mesh.materials[0]
           
            opacity = material.alpha
            diffuseColor[0] = material.rgbCol[0]
            diffuseColor[1] = material.rgbCol[1]
            diffuseColor[2] = material.rgbCol[2]
	    
	    # part Hardcoded Color >>
	    if useHardCodedColor :
		    diffuseColor[0] = HardCodedDiffuse
		    diffuseColor[1] = HardCodedDiffuse
		    diffuseColor[2] = HardCodedDiffuse
            # part Hardcoded Color <<
	    
            if material.getTextures()[0]:
                image = material.getTextures()[0].tex.getImage()
            if image:
                textureName = image.getFilename()
                textureName = textureName.split(os.path.normcase('/'))[-1]  #get only the filename
                diffuseTexture = 'true'
        #End material and texture
       
        #TwoSided
        if mesh.mode & NMesh.Modes['TWOSIDED']:
            twoSided='true'
        else:
            twoSided='false'
       
        #CustomColor
        customColor = 1
        customColor = getPropertyIfExists(object,'customColor',customColor)
        if customColor:
            customColor = 'true'
        else:
            customColor = 'false'
       
        #Real face count (only use triangle)
        realFaceCount = 0
        for face in mesh.faces:
            if (len(face.v) == 3):
                realFaceCount += 1
       
        #Frames number
        frameCount = 1
        #frameCount = getPropertyIfExists(object,'frameCount',frameCount)
        startFrame = Blender.Get('staframe')
        endFrame = Blender.Get('endframe')
        frameCount = endFrame-startFrame+1

        # TRANSFERING FACE TEXTURE COORD. TO VERTEX TEXTURE COORD.
        #  and duplicating vertex associated to different uvco
       
        if textureName == '':    #THERE IS NO TEXTURE
            #create the index list, don't care of newvertices
            for face in mesh.faces:
                faceindices = []
                if (len(face.v) == 3):
                    for vert in face.v:
                        faceindices.append(vert.index)
                    newindices.append(faceindices[0:3])
           
        else:               # THERE IS A TEXTURE
           
            uvlist[:] = [[0]*3 for i in range( len(mesh.verts) )]
           
            for face in mesh.faces:
                faceindices = []
                if (len(face.v) == 3):
                    for i in range(len(face.uv)):
                        vindex = face.v[i].index
                       
                        if notseenindex(vindex):
                            uvlist[vindex] = [vindex, face.uv[i][0], face.uv[i][1]]
                           
                        elif uvlist[vindex][1] != face.uv[i][0] or \
                          uvlist[vindex][2] != face.uv[i][1]:
                            #debug: print("dif: [%f,%f] et [%f,%f]" %( uvlist[vindex][1], face.uv[i][0], uvlist[vindex][2], face.uv[i][1] ))
                            #Create a new "entry" for an existing vertex
                            newvertices.append(vindex)
                            uvlist.append([currentnewvertex, face.uv[i][0], face.uv[i][1]])
                            vindex = currentnewvertex
                            currentnewvertex += 1
                           
                        faceindices.append(vindex)
                    newindices.append(faceindices[0:3])
           
        #End texture and vertex copy
       
       
        # ---- BEGINNING OF THE WRITING OF THE FILE ----
       
        #MESH HEADER
        out.write( '\n<Mesh \n' )
        out.write( '        name="%s" \n' % (mesh.name) )
        out.write( '        frameCount="%i" \n' % ( frameCount ) )
        out.write( '        vertexCount="%i" \n' % (len(mesh.verts) + len(newvertices)) )
        out.write( '        indexCount="%i" \n' % ( realFaceCount * 3 ) )
        out.write( '        specularPower="9.999999" \n' )
        out.write( '        opacity="%f" \n' % (opacity) )
        out.write( '        twoSided="%s" \n' % ( twoSided ) )
        out.write( '        customColor="%s" \n' % ( customColor ) )
        out.write( '        diffuseTexture="%s" > \n' % ( diffuseTexture ) )
       
        #DIFFUSE
        out.write( '    <Diffuse>\n        <Color r="%f" g="%f" b="%f" />\n     </Diffuse>\n' % ( diffuseColor[0], diffuseColor[1], diffuseColor[2] ) )
       
        #SPECULAR # part Hardcoded Color: THIS WAS ALREADY HARDCODED ...
        out.write( '    <Specular><Color r="%f" g="%f" b="%f" /></Specular>\n' % (HardCodedSpecular, HardCodedSpecular, HardCodedSpecular) )
      
        #TEXTURE
        if textureName == '':
            out.write( '    <!-- NO TEXTURE -->\n' )
        else:
            out.write( '    <Texture name="%s" />\n' % (textureName) )
       
        #For each FRAME
       
        #VERTICES
        framelist={}
        l = range( startFrame , startFrame+frameCount )
        for frame in l:
            # set the right frame
            Blender.Set('curframe', frame)
            curObject=object
            curMatrix= curObject.getMatrix('worldspace') 
            
            fmesh = curObject.getData()

            # after creation of new Object, Selection is no longer reliable (new obj. are selected, too), already corrected in code (see: curObject= ...)
            defmesh=Blender.Mesh.New(curObject.name+'F'+str(frame))
            defmesh.getFromObject(curObject.name)
            
            framelist[frame]=defmesh  

            # just to avoid work
            fmesh=defmesh

            out.write( '    <Vertices frame="%i">\n' % ( frame-startFrame ) )
            # "real" vertex
            for vert in fmesh.verts:
                vert.co= vert.co * curMatrix
                out.write( '        <Vertex x="%f" y="%f" z="%f" />\n' % (vert.co.x,vert.co.y,vert.co.z) )
            # "linked" new vertex
            for ind in newvertices:
                out.write( '        <Vertex x="%f" y="%f" z="%f" />\n' % \
                    (fmesh.verts[ind].co.x, fmesh.verts[ind].co.y, fmesh.verts[ind].co.z) )
            out.write( '    </Vertices>\n' )

        #NORMALS
        l = range( startFrame , startFrame+frameCount )
        for frame in l:
            Blender.Set('curframe', frame)

            # getting the created meshes
            fmesh = framelist[frame] #.getData()

            out.write( '    <Normals frame="%i">\n' % ( frame-startFrame ) )
            # "real" vertex
            for vert in fmesh.verts:
                out.write( '        <Normal x="%f" y="%f" z="%f" />\n' % (vert.no.x, vert.no.y, vert.no.z) )
            # "linked" new vertex
            for ind in newvertices:
                out.write( '        <Normal x="%f" y="%f" z="%f" />\n' % \
                    (fmesh.verts[ind].no.x, fmesh.verts[ind].no.y, fmesh.verts[ind].no.z) )
            out.write( '    </Normals>\n' )
       
        #End FRAMES


        #TEXTURES COORDS
        if textureName == '':
            out.write( '    <!-- NO TEXTURE COORDS -->\n' )
        else:
            out.write( '    <TexCoords>\n' )
            #write list
            for uv in uvlist:
                out.write( '        <ST s="%f" t="%f" />\n' % (uv[1], uv[2]) )
            out.write( '    </TexCoords>\n' )
       
        #INDICES
        out.write( '    <Indices>\n' )
        for face in newindices:
            for vert in face:
                out.write( '        <Ix i="%i" />\n' % (vert) )
#            out.write('\n')
        out.write( '    </Indices>\n' )
       
        #END MESH
        out.write( '</Mesh>\n' )
       
        #Printing a summary of the mesh to the output
        print("\nObject : %s" %(object.getName() ) )
        print("Mesh : %s" %(mesh.name) )
        print("%i frames" % ( frameCount ) )
        print("%i vertices" % (len(mesh.verts) + len(newvertices)) )
        print("%i exported faces (%i real faces)" % (realFaceCount, len(mesh.faces) ) )
        if realFaceCount != len(mesh.faces) :
            print("WARNING : some faces have been ignored (not triangle)\n" +
            "       Errors may occur with faces and indices linking..." )
            Blender.Draw.PupMenu("export warning:%t|WARNING : some faces have been ignored (not triangle)|" +
            "       Errors may occur with faces and indices linking...")
        print("%i indices" % (realFaceCount * 3) )
        print("opacity : %f" % (opacity) )
        print("two sided : %s" % (twoSided) )
        print("custom color : %s" % (customColor) )
        print("Use a diffuse texture : %s" % (diffuseTexture) )
        if textureName != '':
            print( "    texture name : %s" % (textureName) )
        print("diffuse color : %f,%f,%f" % ( diffuseColor[0], diffuseColor[1], diffuseColor[2] )  )
        print("Number of new vertices to fit uv mapping : %i\n" % ( len(newvertices) ) )

    #FOOTER
    out.write( '</G3D>\n' )
   
    out.close()
#END write_obj

# file selector based on ac3d_export.py by Willian P. Germano
# File Selector callback:
def fs_callback(filename):
    global cmd, extension, scriptsdir
    if not filename.endswith(extension): filename = '%s%s' % (filename, extension)
    if bsys.exists(filename):
        if Blender.Draw.PupMenu('OVERWRITE?%t|File exists') != 1:
            return
    if cmd:
        tmp = bsys.join(scriptsdir, "tmp.xml")
        write_obj(tmp)
        
        print "running glexemel"
        cmd = [cmd, tmp, filename]
        print cmd
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out = p.stdout.read()
        err = p.stderr.read()
        res = p.wait()
        if res != 0 or err:
            s = ''
            if out:
                s += out.replace('\n', '|')
            if err:
                s += err.replace('\n', '|')
            Blender.Draw.PupMenu('glexemel error: see console%t|' + s)
        print out
        print err
        print res
    else:
        write_obj(filename)
        print "glexemel not found"


scriptsdir = Blender.Get('scriptsdir')
files = os.listdir(scriptsdir)
cmd = ''
extension=".xml"
for fname in files:
    if fname.startswith("xml2g"):
        cmd = bsys.join(scriptsdir, fname)
        extension = ".g3d"
        break

OBJS = Blender.Object.GetSelected()
if not OBJS:
    Blender.Draw.PupMenu('ERROR: no objects selected')
else:
    fname = bsys.makename(ext=extension)
    Blender.Window.FileSelector(fs_callback, "Export XML-G3D", fname)
#Blender.Window.FileSelector(write_obj, "Export") 
