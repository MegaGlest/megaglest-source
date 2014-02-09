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

from xml.etree import cElementTree as ElementTree

CEGUIImageset1 = "CEGUI imageset 1"
CEGUIImageset2 = "CEGUI imageset 2"

class Imageset1TypeDetector(compatibility.TypeDetector):
    def getType(self):
        return CEGUIImageset1

    def getPossibleExtensions(self):
        return set(["imageset"])

    def matches(self, data, extension):
        if extension not in ["", "imageset"]:
            return False

        # todo: we should be at least a bit more precise
        # (implement XSD based TypeDetector?)
        return ceguihelpers.checkDataVersion("Imageset", None, data)

class Imageset2TypeDetector(compatibility.TypeDetector):
    def getType(self):
        return CEGUIImageset2

    def getPossibleExtensions(self):
        return set(["imageset"])

    def matches(self, data, extension):
        if extension not in ["", "imageset"]:
            return False

        return ceguihelpers.checkDataVersion("Imageset", "2", data)

class CEGUI1ToCEGUI2Layer(compatibility.Layer):
    def getSourceType(self):
        return CEGUIImageset1

    def getTargetType(self):
        return CEGUIImageset2

    def transform(self, data):
        root = ElementTree.fromstring(data)
        root.set("version", "2")

        root.set("imagefile", root.get("Imagefile", ""))
        del root.attrib["Imagefile"]

        if root.get("ResourceGroup") is not None:
            root.set("resourceGroup", root.get("ResourceGroup", ""))
            del root.attrib["ResourceGroup"]

        root.set("name", root.get("Name", ""))
        del root.attrib["Name"]

        if root.get("NativeHorzRes") is not None:
            root.set("nativeHorzRes", root.get("NativeHorzRes", "640"))
            del root.attrib["NativeHorzRes"]

        if root.get("NativeVertRes") is not None:
            root.set("nativeVertRes", root.get("NativeVertRes", "480"))
            del root.attrib["NativeVertRes"]

        if root.get("AutoScaled") is not None:
            root.set("autoScaled", root.get("AutoScaled", "false"))
            del root.attrib["AutoScaled"]

        for image in root.findall("Image"):
            image.set("name", image.get("Name", ""))
            del image.attrib["Name"]

            # TODO: We only deal with basic image here
            if image.get("type", "BasicImage") == "BasicImage":
                if image.get("XPos") is not None:
                    image.set("xPos", image.get("XPos", "0"))
                    del image.attrib["XPos"]

                if image.get("YPos") is not None:
                    image.set("yPos", image.get("YPos", "0"))
                    del image.attrib["YPos"]

                if image.get("Width") is not None:
                    image.set("width", image.get("Width", "1"))
                    del image.attrib["Width"]

                if image.get("Height") is not None:
                    image.set("height", image.get("Height", "1"))
                    del image.attrib["Height"]

                if image.get("XOffset") is not None:
                    image.set("xOffset", image.get("XOffset", "0"))
                    del image.attrib["xOffset"]

                if image.get("YOffset") is not None:
                    image.set("yOffset", image.get("YOffset", "0"))
                    del image.attrib["yOffset"]

        return ElementTree.tostring(root, "utf-8")

class CEGUI2ToCEGUI1Layer(compatibility.Layer):
    def getSourceType(self):
        return CEGUIImageset2

    def getTargetType(self):
        return CEGUIImageset1

    @classmethod
    def autoScaledToBoolean(cls, value):
        if value in ["true", "vertical", "horizontal", "min", "max"]:
            return "true"
        else:
            return "false"

    def transform(self, data):
        root = ElementTree.fromstring(data)
        del root.attrib["version"] # imageset version 1 has no version attribute!

        root.set("Imagefile", root.get("imagefile", ""))
        del root.attrib["imagefile"]

        if root.get("resourceGroup") is not None:
            root.set("ResourceGroup", root.get("resourceGroup", ""))
            del root.attrib["resourceGroup"]

        root.set("Name", root.get("name", ""))
        del root.attrib["name"]

        if root.get("nativeHorzRes") is not None:
            root.set("NativeHorzRes", root.get("nativeHorzRes", "640"))
            del root.attrib["nativeHorzRes"]

        if root.get("nativeVertRes") is not None:
            root.set("NativeVertRes", root.get("nativeVertRes", "480"))
            del root.attrib["nativeVertRes"]

        if root.get("autoScaled") is not None:
            root.set("AutoScaled", CEGUI2ToCEGUI1Layer.autoScaledToBoolean(root.get("autoScaled", "false")))
            del root.attrib["autoScaled"]

        for image in root.findall("Image"):
            image.set("Name", image.get("name", ""))
            del image.attrib["name"]

            if image.get("type", "BasicImage") != "BasicImage":
                raise NotImplementedError("Can't convert non-BasicImage in imageset version 2 (1.0+) to older version, such stuff wasn't supported in imagesets version 1 (everything up to 0.7)")

            # TODO: We only deal with basic image here
            if image.get("Type", "BasicImage") == "BasicImage":
                if image.get("xPos") is not None:
                    image.set("XPos", image.get("xPos", "0"))
                    del image.attrib["xPos"]

                if image.get("yPos") is not None:
                    image.set("YPos", image.get("yPos", "0"))
                    del image.attrib["yPos"]

                if image.get("width") is not None:
                    image.set("Width", image.get("width", "1"))
                    del image.attrib["width"]

                if image.get("height") is not None:
                    image.set("Height", image.get("height", "1"))
                    del image.attrib["height"]

                if image.get("xOffset") is not None:
                    image.set("XOffset", image.get("xOffset", "0"))
                    del image.attrib["XOffset"]

                if image.get("yOffset") is not None:
                    image.set("YOffset", image.get("yOffset", "0"))
                    del image.attrib["YOffset"]

        return ElementTree.tostring(root, "utf-8")
