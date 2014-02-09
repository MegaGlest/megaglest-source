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

"""Implements the more advanced SVG input of metaimageset (using Inkscape).
"""

from ceed.metaimageset import inputs

from PySide import QtGui

import os.path
from xml.etree import cElementTree as ElementTree
import tempfile
import subprocess
import cStringIO as StringIO

INKSCAPE_PATH = "inkscape"

def getAllSVGLayers(svgPath):
    """Retrieves all Inkscape layers defined in given SVG.

    Note: I couldn't figure out how to do this with inkscape CLI
    """

    ret = []

    doc = ElementTree.ElementTree(file = svgPath)
    for g in doc.findall(".//{http://www.w3.org/2000/svg}g"):
        if g.get("{http://www.inkscape.org/namespaces/inkscape}groupmode") == "layer":
            ret.append(g.get("{http://www.inkscape.org/namespaces/inkscape}label"))

    return ret

def showOnlySVGLayers(svgPath, layers, targetSvg):
    doc = ElementTree.ElementTree(file = svgPath)
    for g in doc.findall(".//{http://www.w3.org/2000/svg}g"):
        if g.get("{http://www.inkscape.org/namespaces/inkscape}groupmode") == "layer":
            if g.get("{http://www.inkscape.org/namespaces/inkscape}label") in layers:
                g.set("style", "display:inline")
            else:
                g.set("style", "display:none")

    doc.write(targetSvg, encoding = "utf-8")

def exportSVG(svgPath, layers, targetPngPath):
    allLayers = set(getAllSVGLayers(svgPath))
    for layer in layers:
        if not layer in allLayers:
            raise RuntimeError("Can't export with layer \"%s\", it isn't defined in the SVG \"%s\"!" % (layer, svgPath))

    temporarySvg = tempfile.NamedTemporaryFile(suffix = ".svg")
    showOnlySVGLayers(svgPath, layers, temporarySvg.name)

    cmdLine = [INKSCAPE_PATH, "--file=%s" % (temporarySvg.name), "--export-png=%s" % (targetPngPath)]
    stdout = subprocess.check_output(cmdLine, stderr = subprocess.STDOUT)
    # FIXME: debug logging of stdout?

class Component(object):
    def __init__(self, svg, name = "", x = 0, y = 0, width = 1, height = 1, layers = "", xOffset = 0, yOffset = 0):
        self.svg = svg

        self.name = name

        self.x = x
        self.y = y
        self.width = width
        self.height = height

        self.xOffset = xOffset
        self.yOffset = yOffset

        self.layers = layers.split(" ")

        self.cachedImages = None

    def loadFromElement(self, element):
        self.name = element.get("name", "")

        self.x = int(element.get("x", "0"))
        self.y = int(element.get("y", "0"))
        self.width = int(element.get("width", "1"))
        self.height = int(element.get("height", "1"))

        self.xOffset = int(element.get("xOffset", "0"))
        self.yOffset = int(element.get("yOffset", "0"))

        self.layers = element.get("layers", "").split(" ")

    def saveToElement(self):
        ret = ElementTree.Element("Component")

        ret.set("name", self.name)

        ret.set("x", str(self.x))
        ret.set("y", str(self.y))
        ret.set("width", str(self.width))
        ret.set("height", str(self.height))

        ret.set("xOffset", str(self.xOffset))
        ret.set("yOffset", str(self.yOffset))

        ret.set("layers", " ".join(self.layers))

        return ret

    def generateQImage(self):
        full_path = os.path.join(os.path.dirname(self.svg.metaImageset.filePath), self.svg.path)

        temporaryPng = tempfile.NamedTemporaryFile(suffix = ".png")
        exportSVG(full_path, self.layers, temporaryPng.name)

        qimage = QtGui.QImage(temporaryPng.name)
        return qimage.copy(self.x, self.y, self.width, self.height)

    def buildImages(self):
        # FIXME: This is a really nasty optimisation, it can be done way better
        #        but would probably require a slight redesign of inputs.Input
        if self.cachedImages is None:
            self.cachedImages = [inputs.Image(self.name, self.generateQImage(), self.xOffset, self.yOffset)]

        return self.cachedImages

class FrameComponent(object):
    def __init__(self, svg, name = "", x = 0, y = 0, width = 1, height = 1, cornerWidth = 1, cornerHeight = 1, layers = "", skip = ""):
        self.svg = svg

        self.name = name

        self.x = x
        self.y = y
        self.width = width
        self.height = height

        self.cornerWidth = cornerWidth
        self.cornerHeight = cornerHeight

        self.layers = layers.split(" ")
        self.skip = skip.split(" ")

        self.cachedImages = None
        self.cachedQImage = None

    def loadFromElement(self, element):
        self.name = element.get("name", "")

        self.x = int(element.get("x", "0"))
        self.y = int(element.get("y", "0"))
        self.width = int(element.get("width", "1"))
        self.height = int(element.get("height", "1"))

        self.cornerWidth = int(element.get("cornerWidth", "1"))
        self.cornerHeight = int(element.get("cornerHeight", "1"))

        self.layers = element.get("layers", "").split(" ")
        self.skip = element.get("skip", "").split(" ")

    def saveToElement(self):
        ret = ElementTree.Element("Component")

        ret.set("name", self.name)

        ret.set("x", str(self.x))
        ret.set("y", str(self.y))
        ret.set("width", str(self.width))
        ret.set("height", str(self.height))

        ret.set("cornerWidth", str(self.cornerWidth))
        ret.set("cornerHeight", str(self.cornerHeight))

        ret.set("layers", " ".join(self.layers))
        ret.set("skip", " ".join(self.layers))

        return ret

    def generateQImage(self, x, y, width, height):
        if self.cachedQImage is None:
            full_path = os.path.join(os.path.dirname(self.svg.metaImageset.filePath), self.svg.path)

            temporaryPng = tempfile.NamedTemporaryFile(suffix = ".png")
            exportSVG(full_path, self.layers, temporaryPng.name)

            self.cachedQImage = QtGui.QImage(temporaryPng.name)

        return self.cachedQImage.copy(x, y, width, height)

    def buildImages(self):
        # FIXME: This is a really nasty optimisation, it can be done way better
        #        but would probably require a slight redesign of inputs.Input
        if self.cachedImages is None:
            self.cachedImages = []

            if "centre" not in self.skip:
                self.cachedImages.append(
                    inputs.Image("%sCentre" % (self.name),
                        self.generateQImage(
                            self.x + self.cornerWidth + 1, self.y + self.cornerHeight + 1,
                            self.width - 2 * self.cornerWidth - 2, self.height - 2 * self.cornerHeight - 2),
                        0, 0)
                    )

            if "top" not in self.skip:
                self.cachedImages.append(
                    inputs.Image("%sTop" % (self.name),
                        self.generateQImage(
                            self.x + self.cornerWidth + 1, self.y,
                            self.width - 2 * self.cornerWidth - 2, self.cornerHeight),
                        0, 0)
                    )

            if "bottom" not in self.skip:
                self.cachedImages.append(
                    inputs.Image("%sBottom" % (self.name),
                        self.generateQImage(
                            self.x + self.cornerWidth + 1, self.y + self.height - self.cornerHeight,
                            self.width - 2 * self.cornerWidth - 2, self.cornerHeight),
                        0, 0)
                    )

            if "left" not in self.skip:
                self.cachedImages.append(
                    inputs.Image("%sLeft" % (self.name),
                        self.generateQImage(
                            self.x, self.y + self.cornerHeight + 1,
                            self.cornerWidth, self.height - 2 * self.cornerHeight - 2),
                        0, 0)
                    )

            if "right" not in self.skip:
                self.cachedImages.append(
                    inputs.Image("%sRight" % (self.name),
                        self.generateQImage(
                            self.x + self.width - self.cornerWidth, self.y + self.cornerHeight + 1,
                            self.cornerWidth, self.height - 2 * self.cornerHeight - 2),
                        0, 0)
                    )

            if "topLeft" not in self.skip:
                self.cachedImages.append(
                    inputs.Image("%sTopLeft" % (self.name),
                        self.generateQImage(
                            self.x, self.y,
                            self.cornerWidth, self.cornerHeight),
                        0, 0)
                    )

            if "topRight" not in self.skip:
                self.cachedImages.append(
                    inputs.Image("%sTopRight" % (self.name),
                        self.generateQImage(
                            self.x + self.width - self.cornerWidth, self.y,
                            self.cornerWidth, self.cornerHeight),
                        0, 0)
                    )

            if "bottomLeft" not in self.skip:
                self.cachedImages.append(
                    inputs.Image("%sBottomLeft" % (self.name),
                        self.generateQImage(
                            self.x, self.y + self.height - self.cornerHeight,
                            self.cornerWidth, self.cornerHeight),
                        0, 0)
                    )

            if "bottomRight" not in self.skip:
                self.cachedImages.append(
                    inputs.Image("%sBottomRight" % (self.name),
                        self.generateQImage(
                            self.x + self.width - self.cornerWidth, self.y + self.height - self.cornerHeight,
                            self.cornerWidth, self.cornerHeight),
                        0, 0)
                    )

        return self.cachedImages

class InkscapeSVG(inputs.Input):
    """Just one particular SVGs, support advanced features and renders everything
    using Inkscape, the output should be of higher quality than the SVGTiny renderer
    above. Requires Inkscape to be installed and the inkscape binary to be in $PATH.

    Each component is one "image" to be passed to the metaimageset compiler.
    """

    def __init__(self, metaImageset):
        super(InkscapeSVG, self).__init__(metaImageset)

        self.path = ""
        self.components = []

    def loadFromElement(self, element):
        self.path = element.get("path", "")

        for componentElement in element.findall("Component"):
            component = Component(self)
            component.loadFromElement(componentElement)

            self.components.append(component)

        # FrameComponent is a shortcut to avoid having to type out 9 components
        for componentElement in element.findall("FrameComponent"):
            component = FrameComponent(self)
            component.loadFromElement(componentElement)

            self.components.append(component)

    def saveToElement(self):
        ret = ElementTree.Element("InkscapeSVG")
        ret.set("path", self.path)

        for component in self.components:
            ret.append(component.saveToElement())

        return ret

    def getDescription(self):
        return "Inkscape SVG '%s' with %i components" % (self.path, len(self.components))

    def buildImages(self):
        ret = []

        for component in self.components:
            ret.extend(component.buildImages())

        return ret
