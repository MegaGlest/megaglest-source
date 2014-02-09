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

from ceed import compatibility
from ceed.compatibility import ceguihelpers
from ceed.compatibility.imageset import cegui as imageset_cegui_compat

from xml.etree import cElementTree as ElementTree

CEGUIFont1 = "CEGUI Font 1"
CEGUIFont2 = "CEGUI Font 2"
CEGUIFont3 = "CEGUI Font 3"

class Font2TypeDetector(compatibility.TypeDetector):
    def getType(self):
        return CEGUIFont2

    def getPossibleExtensions(self):
        return set(["font"])

    def matches(self, data, extension):
        if extension not in ["", "font"]:
            return False

        # todo: we should be at least a bit more precise
        # (implement XSD based TypeDetector?)

        return ceguihelpers.checkDataVersion("Font", None, data)

class Font3TypeDetector(compatibility.TypeDetector):
    def getType(self):
        return CEGUIFont3

    def getPossibleExtensions(self):
        return set(["font"])

    def matches(self, data, extension):
        if extension not in ["", "font"]:
            return False

        return ceguihelpers.checkDataVersion("Font", "3", data)

class Font2ToFont3Layer(compatibility.Layer):
    def getSourceType(self):
        return CEGUIFont2

    def getTargetType(self):
        return CEGUIFont3

    def transformAttribute(self, element, attribute):
        sourceAttributeName = attribute[0].upper() + attribute[1:]
        targetAttributeName = sourceAttributeName[0].lower() + sourceAttributeName[1:]

        if element.get(sourceAttributeName) is not None:
            element.set(targetAttributeName, element.get(sourceAttributeName))
            del element.attrib[sourceAttributeName]

    def transform(self, data):
        root = ElementTree.fromstring(data)
        root.set("version", "3")

        for attr in ["name", "filename", "resourceGroup", "type", "size", "nativeHorzRes", "nativeVertRes", "autoScaled", "antiAlias", "lineScaling"]:
            self.transformAttribute(root, attr)

        for mapping in root.findall("Mapping"):
            for attr in ["codepoint", "image", "horzAdvance"]:
                self.transformAttribute(mapping, attr)

        return ElementTree.tostring(root, "utf-8")

class Font3ToFont2Layer(compatibility.Layer):
    def getSourceType(self):
        return CEGUIFont3

    def getTargetType(self):
        return CEGUIFont2

    def transformAttribute(self, element, attribute):
        targetAttributeName = attribute[0].upper() + attribute[1:]
        sourceAttributeName = targetAttributeName[0].lower() + targetAttributeName[1:]

        if element.get(sourceAttributeName) is not None:
            element.set(targetAttributeName, element.get(sourceAttributeName))
            del element.attrib[sourceAttributeName]

    def transform(self, data):
        root = ElementTree.fromstring(data)
        del root.attrib["version"]

        for attr in ["name", "filename", "resourceGroup", "type", "size", "nativeHorzRes", "nativeVertRes", "autoScaled", "antiAlias", "lineScaling"]:
            self.transformAttribute(root, attr)

        if root.get("AutoScaled") is not None:
            root.set("AutoScaled", imageset_cegui_compat.CEGUI2ToCEGUI1Layer.autoScaledToBoolean(root.get("AutoScaled")))

        for mapping in root.findall("Mapping"):
            for attr in ["codepoint", "image", "horzAdvance"]:
                self.transformAttribute(mapping, attr)

        return ElementTree.tostring(root, "utf-8")
