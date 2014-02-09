##############################################################################
#   CEED - Unified CEGUI asset editor
#
#   Copyright (C) 2011-2012   Martin Preisler <martin@preisler.me>
#                             and contributing authors (see AUTHORS file)
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
##############################################################################

from xml.etree import cElementTree as ElementTree

from ceed import compatibility
from ceed.compatibility.imageset import cegui

GorillaFile = "Gorilla file"

class GorillaTypeDetector(compatibility.TypeDetector):
    def getType(self):
        return GorillaFile

    def getPossibleExtensions(self):
        return set(["gorilla"])

    def matches(self, data, extension):
        if extension != "gorilla":
            return False

        # todo: we should be at least a bit more precise
        return True

class GorillaToCEGUI1Layer(compatibility.Layer):
    def getSourceType(self):
        return GorillaFile

    def getTargetType(self):
        return cegui.CEGUIImageset1

    def transform(self, data):
        # TODO: very crude and work in progress transformation
        class Sprite(object):
            def __init__(self, name, xpos, ypos, width, height):
                self.name = name

                self.xpos = xpos
                self.ypos = ypos
                self.width = width
                self.height = height

        textureName = None
        sprites = []
        section = None

        for line in data.split("\n"):
            stripped = line.strip()

            if stripped.lower() == "[texture]":
                section = "Texture"
            elif stripped.lower() == "[sprites]":
                section = "Sprites"
            elif stripped.lower().startswith("["):
                # unknown section
                section = None

            else:
                if section == "Texture":
                    if stripped.lower().startswith("file"):
                        split = stripped.split(" ", 1)
                        if textureName is not None:
                            raise RuntimeError("Texture file is defined multiple times, duplicating line '%s'" % (stripped))
                        if len(split) != 2:
                            raise RuntimeError("Invalid texture file line '%s', can't be split into 2 pieces" % (stripped))

                        textureName = split[1]

                    # TODO: we ignore whitepixels for now

                if section == "Sprites":
                    if stripped == "":
                        # empty line
                        continue

                    split = stripped.split(" ")
                    if len(split) != 5:
                        raise RuntimeError("Error, line '%s' can't be split into 5 strings" % (stripped))

                    sprites.append(Sprite(split[0], split[1], split[2], split[3], split[4]))

        if textureName is None:
            raise RuntimeError("Gorilla file doesn't contain any texture info")

        if len(sprites) == 0:
            raise RuntimeError("Gorilla file doesn't contain any sprites (font glyphs are not being converted, that's a TODO)")

        root = ElementTree.Element("Imageset")

        # from what I see, gorilla files don't have names in them, the filename is the
        # name of the resource
        root.set("Name", GorillaFile)
        root.set("Imagefile", textureName)

        for sprite in sprites:
            image = ElementTree.Element("Image")

            image.set("Name", sprite.name)

            image.set("XPos", sprite.xpos)
            image.set("YPos", sprite.ypos)
            image.set("Width", sprite.width)
            image.set("Height", sprite.height)

            root.append(image)

        return ElementTree.tostring(root, "utf-8")

class CEGUI1ToGorillaLayer(compatibility.Layer):
    def getSourceType(self):
        return cegui.CEGUIImageset1

    def getTargetType(self):
        return GorillaFile

    def transform(self, data):
        root = ElementTree.fromstring(data)

        ret = ""
        ret += "[Texture]\n"
        ret += "file %s\n" % (root.get("Imagefile"))
        ret += "\n"

        ret += "[Sprites]\n"
        for image in root.findall("Image"):
            ret += "%s %s %s %s %s\n" % (image.get("Name"), image.get("XPos"), image.get("YPos"), image.get("Width"), image.get("Height"))

        return ret
