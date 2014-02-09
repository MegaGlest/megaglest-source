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

"""Lightweight CEGUI property value types that can parse and write text."""

import abc
import re
import math

from collections import OrderedDict

from ceed.propertytree import properties
from ceed.propertytree import parsers

from PySide import QtGui

class Base(object):
    """Abstract base class for all value types."""

    __metaclass__ = abc.ABCMeta

    floatPattern = '\s*(-?\d+(?:\.\d+)?(?:e[-+]\d+)?)\s*'

    @classmethod
    def tryParse(cls, strValue, target=None):
        """Parse the specified string value and return
        a tuple with the parsed value and a boolean
        specifying success or failure.

        If 'target' is not None, update its attributes
        with the parse value.
        """
        raise NotImplementedError("'tryParse()' not implemented for class '%s'" % cls.__name__)

    @classmethod
    def fromString(cls, strValue):
        """Parse the specified string value and return
        a new instance of this type.
        Raise a ValueError if the string can't be parsed.
        """
        value, valid = cls.tryParse(strValue)
        if not valid:
            raise ValueError("Could not convert string to %s: '%s'." % (cls.__name__, strValue))
        return value

    @classmethod
    def getPropertyType(cls):
        raise NotImplementedError("'getPropertyType()' not implemented for class '%s'" % cls.__name__)

    def parse(self, strValue):
        return self.__class__.tryParse(strValue, self)[1]

    @abc.abstractmethod
    def __hash__(self):
        pass

    @abc.abstractmethod
    def __eq__(self, other):
        pass

    @abc.abstractmethod
    def __repr__(self):
        pass

    def __ne__(self, other):
        """Default implementation of __ne__, negates __eq__!"""
        return not (self.__eq__(other))

class UDim(Base):
    """UDim"""

    pattern = '\s*\{' + \
              Base.floatPattern + \
              ',' + \
              Base.floatPattern + \
              '\}\s*'
    rex = re.compile(pattern)

    @classmethod
    def tryParse(cls, strValue, target=None):
        match = re.match(cls.rex, strValue)
        if match is None:
            return None, False

        scale = float(match.group(1))
        offset = float(match.group(2))

        if target is None:
            target = cls(scale, offset)
        else:
            target.scale = scale
            target.offset = offset

        return target, True

    def __init__(self, scale=0.0, offset=0.0):
        super(UDim, self).__init__()
        self.scale = float(scale)
        self.offset = float(offset)

    def __hash__(self):
        return hash((self.scale, self.offset))

    def __eq__(self, other):
        if isinstance(other, self.__class__):
            return self.scale == other.scale and self.offset == other.offset
        return False

    def __repr__(self):
        def fmt(value):
            # no scientific notation, 16 digits precision, remove trailing zeroes
            return "{:.16f}".format(value).rstrip("0").rstrip(".")
        return "{{{}, {}}}".format(fmt(self.scale), fmt(self.offset))

    @classmethod
    def getPropertyType(cls):
        return UDimProperty

class USize(Base):
    """USize (uses UDim)"""

    pattern = '\s*\{' + \
              UDim.pattern + \
              ',' + \
              UDim.pattern + \
              '\}\s*'
    rex = re.compile(pattern)

    @classmethod
    def tryParse(cls, strValue, target=None):
        match = re.match(cls.rex, strValue)
        if match is None:
            return None, False

        widthScale = float(match.group(1))
        widthOffset = float(match.group(2))
        heightScale = float(match.group(3))
        heightOffset = float(match.group(4))

        if target is None:
            target = cls(UDim(widthScale, widthOffset), UDim(heightScale, heightOffset))
        else:
            target.width.scale = widthScale
            target.width.offset = widthOffset
            target.height.scale = heightScale
            target.height.offset = heightOffset

        return target, True

    def __init__(self, width=UDim(), height=UDim()):
        super(USize, self).__init__()
        self.width = width
        self.height = height

    def __hash__(self):
        return hash((hash(self.width), hash(self.height)))

    def __eq__(self, other):
        if isinstance(other, self.__class__):
            return self.width == other.width and self.height == other.height
        return False

    def __repr__(self):
        return "{{{}, {}}}".format(self.width, self.height)

    @classmethod
    def getPropertyType(cls):
        return USizeProperty

class UVector2(Base):
    """UVector2 (uses UDim)

    Very similar to USize.
    """

    pattern = '\s*\{' + \
              UDim.pattern + \
              ',' + \
              UDim.pattern + \
              '\}\s*'
    rex = re.compile(pattern)

    @classmethod
    def tryParse(cls, strValue, target=None):
        match = re.match(cls.rex, strValue)
        if match is None:
            return None, False

        xScale = float(match.group(1))
        xOffset = float(match.group(2))
        yScale = float(match.group(3))
        yOffset = float(match.group(4))

        if target is None:
            target = cls(UDim(xScale, xOffset), UDim(yScale, yOffset))
        else:
            target.x.scale = xScale
            target.x.offset = xOffset
            target.y.scale = yScale
            target.y.offset = yOffset

        return target, True

    def __init__(self, x=UDim(), y=UDim()):
        #pylint: disable-msg=C0103
        # invalid name x and y - we need x and y here
        super(UVector2, self).__init__()
        self.x = x
        self.y = y

    def __hash__(self):
        return hash((hash(self.x), hash(self.y)))

    def __eq__(self, other):
        if isinstance(other, self.__class__):
            return self.x == other.x and self.y == other.y
        return False

    def __repr__(self):
        return "{{{}, {}}}".format(self.x, self.y)

    @classmethod
    def getPropertyType(cls):
        return UVector2Property

class URect(Base):
    """URect (uses UDim)"""

    pattern = '\s*\{' + \
              UDim.pattern + \
              ',' + \
              UDim.pattern + \
              ',' + \
              UDim.pattern + \
              ',' + \
              UDim.pattern + \
              '\}\s*'
    rex = re.compile(pattern)

    @classmethod
    def tryParse(cls, strValue, target=None):
        match = re.match(cls.rex, strValue)
        if match is None:
            return None, False

        leftScale = float(match.group(1))
        leftOffset = float(match.group(2))
        topScale = float(match.group(3))
        topOffset = float(match.group(4))
        rightScale = float(match.group(5))
        rightOffset = float(match.group(6))
        bottomScale = float(match.group(7))
        bottomOffset = float(match.group(8))

        if target is None:
            target = cls(UDim(leftScale, leftOffset), UDim(topScale, topOffset), UDim(rightScale, rightOffset), UDim(bottomScale, bottomOffset))
        else:
            target.left.scale = leftScale
            target.left.offset = leftOffset
            target.top.scale = topScale
            target.top.offset = topOffset
            target.right.scale = rightScale
            target.right.offset = rightOffset
            target.bottom.scale = bottomScale
            target.bottom.offset = bottomOffset

        return target, True

    def __init__(self, left=UDim(), top=UDim(), right=UDim(), bottom=UDim()):
        super(URect, self).__init__()
        self.left = left
        self.top = top
        self.right = right
        self.bottom = bottom

    def __hash__(self):
        return hash((hash(self.left), hash(self.top), hash(self.right), hash(self.bottom)))

    def __eq__(self, other):
        if isinstance(other, self.__class__):
            return self.left == other.left and self.top == other.top and self.right == other.right and self.bottom == other.bottom
        return False

    def __repr__(self):
        return "{{{}, {}, {}, {}}}".format(self.left, self.top, self.right, self.bottom)

    @classmethod
    def getPropertyType(cls):
        return URectProperty

class EnumBase(Base, properties.EnumValue):
    """Base class for types that have a predetermined list of possible values."""

    # key-value pairs of allowed values
    # the key should be the value and the value should be the display name.
    enumValues = dict()

    @classmethod
    def tryParse(cls, strValue, target=None):
        if not strValue in cls.enumValues:
            return None, False

        value = strValue

        if target is None:
            target = cls(value)
        else:
            target.value = value

        return target, True

    def __init__(self, value=""):
        super(EnumBase, self).__init__()
        self.value = value

    def __hash__(self):
        return hash(self.value)

    def __eq__(self, other):
        if isinstance(other, self.__class__):
            return self.value == other.value
        return False

    def __repr__(self):
        return str(self.value)

    @classmethod
    def getPropertyType(cls):
        return BaseProperty

    def getEnumValues(self):
        return self.enumValues

class AspectMode(EnumBase):
    """AspectMode"""

    enumValues = OrderedDict([ ("Ignore", "Ignore"), ("Shrink", "Shrink"), ("Expand", "Expand") ])

    def __init__(self, value="Ignore"):
        super(AspectMode, self).__init__(value)

class HorizontalAlignment(EnumBase):
    """HorizontalAlignment"""

    enumValues = OrderedDict([ ("Left", "Left"), ("Centre", "Centre"), ("Right", "Right") ])

    def __init__(self, value="Left"):
        super(HorizontalAlignment, self).__init__(value)

class VerticalAlignment(EnumBase):
    """VerticalAlignment"""

    enumValues = OrderedDict([ ("Top", "Top"), ("Centre", "Centre"), ("Bottom", "Bottom") ])

    def __init__(self, value="Top"):
        super(VerticalAlignment, self).__init__(value)

class HorizontalTextFormatting(EnumBase):
    """HorizontalTextFormatting"""

    enumValues = OrderedDict([("LeftAligned", "Left"),
                              ("CentreAligned", "Centre"),
                              ("RightAligned", "Right"),
                              ("Justified", "Justified"),
                              ("WordWrapLeftAligned", "Word-Wrap Left"),
                              ("WordWrapCentreAligned", "Word-Wrap Centre"),
                              ("WordWrapRightAligned", "Word-Wrap Right"),
                              ("WordWrapJustified", "Word-Wrap Justified") ])

    def __init__(self, value="Left"):
        super(HorizontalTextFormatting, self).__init__(value)

class VerticalTextFormatting(EnumBase):
    """VerticalTextFormatting"""

    enumValues = OrderedDict([("TopAligned", "Top"), ("CentreAligned", "Centre"), ("BottomAligned", "Bottom") ])

    def __init__(self, value="Top"):
        super(VerticalTextFormatting, self).__init__(value)

class WindowUpdateMode(EnumBase):
    """WindowUpdateMode"""

    enumValues = OrderedDict([ ("Always", "Always"), ("Visible", "Visible"), ("Never", "Never") ])

    def __init__(self, value="Always"):
        super(WindowUpdateMode, self).__init__(value)

class SortMode(EnumBase):
    """ItemListBase::SortMode"""

    enumValues = OrderedDict([ ("Ascending", "Ascending"), ("Descending", "Descending"), ("UserSort", "UserSort") ])

    def __init__(self, value="Ascending"):
        super(SortMode, self).__init__(value)

class Quaternion(Base):
    """Quaternion"""
    #pylint: disable-msg=C0103
    # invalid name x,y,z etc.

    pattern = '(?:\s*w\s*:' + Base.floatPattern + '\s+)?' + \
              '\s*x\s*:' + Base.floatPattern + \
              '\s+' + \
              '\s*y\s*:' + Base.floatPattern + \
              '\s+' + \
              '\s*z\s*:' + Base.floatPattern
    rex = re.compile(pattern, re.IGNORECASE)

    @classmethod
    def tryParse(cls, strValue, target=None):
        match = re.match(cls.rex, strValue)
        if match is None:
            return None, False

        w = None
        if match.group(1) is not None:
            # we have the 'w' component
            w = float(match.group(1))
        x = float(match.group(2))
        y = float(match.group(3))
        z = float(match.group(4))

        if target is None:
            target = cls(x, y, z, w)
        else:
            if w is not None:
                target.x = x
                target.y = y
                target.z = z
                target.w = w
            else:
                (target.w, target.x, target.y, target.z) = Quaternion.convertEulerDegreesToQuaternion(x, y, z)

        return target, True

    @staticmethod
    def convertEulerRadiansToQuaternion(x, y, z):
        """Convert the x, y, z angles (in radians) to w, x, y, z (quaternion).

        The order of rotation: 1) around Z 2) around Y 3) around X

        Copied from CEGUI, Quaternion.cpp
        """

        sin_z_2 = math.sin(0.5 * z)
        sin_y_2 = math.sin(0.5 * y)
        sin_x_2 = math.sin(0.5 * x)

        cos_z_2 = math.cos(0.5 * z)
        cos_y_2 = math.cos(0.5 * y)
        cos_x_2 = math.cos(0.5 * x)

        return (cos_z_2 * cos_y_2 * cos_x_2 + sin_z_2 * sin_y_2 * sin_x_2,
                cos_z_2 * cos_y_2 * sin_x_2 - sin_z_2 * sin_y_2 * cos_x_2,
                cos_z_2 * sin_y_2 * cos_x_2 + sin_z_2 * cos_y_2 * sin_x_2,
                sin_z_2 * cos_y_2 * cos_x_2 - cos_z_2 * sin_y_2 * sin_x_2)

    @staticmethod
    def convertEulerDegreesToQuaternion(x, y, z):
        d2r = math.pi / 180.0

        return Quaternion.convertEulerRadiansToQuaternion(x * d2r, y * d2r, z * d2r)

    @staticmethod
    def machineEpsilon(func=float):
        """http://en.wikipedia.org/wiki/Machine_epsilon#Approximation_using_Python"""
        machine_epsilon = func(1)
        while func(1)+func(machine_epsilon) != func(1):
            machine_epsilon_last = machine_epsilon
            machine_epsilon = func(machine_epsilon) / func(2)
        return machine_epsilon_last

    @staticmethod
    def convertQuaternionToYPR(w, x, y, z):
        """Return a tuple of yaw, pitch, roll.

        Ported from http://stackoverflow.com/a/1031235

        Note: This is probably wrong but it's a start.
        """
        # FIXME: Please fix me! (See above)
        w2 = w * w
        x2 = x * x
        y2 = y * y
        z2 = z * z
        unitLength = w2 + x2 + y2 + z2      # Normalised == 1, otherwise correction divisor.
        abcd = w * x + y * z
        eps = Quaternion.machineEpsilon()
        pi = math.pi

        yaw = 0
        pitch = 0
        roll = 0

        if abcd > (0.5 - eps) * unitLength:
            yaw = 2 * math.atan2(y, w)
            pitch = pi
            roll = 0
        elif abcd < (-0.5 + eps) * unitLength:
            yaw = -2 * math.atan2(y, w)
            pitch = -pi
            roll = 0
        else:
            adbc = w * z - x * y
            acbd = w * y - x * z
            yaw = math.atan2(2 * adbc, 1 - 2 * (z2 + x2))
            pitch = math.asin(2 * abcd / unitLength)
            roll = math.atan2(2 * acbd, 1 - 2 * (y2 + x2))

        return (yaw, pitch, roll)

    @staticmethod
    def convertQuaternionToDegrees(w, x, y, z):
        r2d = 180.0 / math.pi

        (z, x, y) = Quaternion.convertQuaternionToYPR(w, x, y, z)
        return (x * r2d, y * r2d, z * r2d)

    def __init__(self, x=0.0, y=0.0, z=0.0, w=1.0):
        super(Quaternion, self).__init__()
        if w is not None:
            self.x = float(x)
            self.y = float(y)
            self.z = float(z)
            self.w = float(w)
        else:
            (self.w, self.x, self.y, self.z) = self.convertEulerDegreesToQuaternion(x, y, z)

    def __hash__(self):
        return hash((self.x, self.y, self.z, self.w))

    def __eq__(self, other):
        if isinstance(other, self.__class__):
            return self.x == other.x and self.y == other.y and self.z == other.z and self.w == other.w
        return False

    def __repr__(self):
        def fmt(value):
            # no scientific notation, 16 digits precision, remove trailing zeroes
            return "{:.16f}".format(value).rstrip("0").rstrip(".")
        return "w:{} x:{} y:{} z:{}".format(fmt(self.w), fmt(self.x), fmt(self.y), fmt(self.z))

    def toDegrees(self):
        return self.convertQuaternionToDegrees(self.w, self.x, self.y, self.z)

    @classmethod
    def getPropertyType(cls):
        return QuaternionProperty

class XYZRotation(Base):
    #pylint: disable-msg=C0103
    # invalid name x,y,z etc.

    pattern = '\s*x\s*:' + Base.floatPattern + \
              '\s+' + \
              '\s*y\s*:' + Base.floatPattern + \
              '\s+' + \
              '\s*z\s*:' + Base.floatPattern
    rex = re.compile(pattern, re.IGNORECASE)

    @classmethod
    def tryParse(cls, strValue, target=None):
        match = re.match(cls.rex, strValue)
        if match is None:
            return None, False

        x = float(match.group(1))
        y = float(match.group(2))
        z = float(match.group(3))

        if target is None:
            target = cls(x, y, z)
        else:
            target.x = x
            target.y = y
            target.z = z

        return target, True

    @classmethod
    def fromQuaternion(cls, quat):
        x, y, z = quat.toDegrees()
        return cls(x, y, z)

    def __init__(self, x=0.0, y=0.0, z=0.0):
        super(XYZRotation, self).__init__()
        self.x = float(x)
        self.y = float(y)
        self.z = float(z)

    def __hash__(self):
        return hash((self.x, self.y, self.z))

    def __eq__(self, other):
        if isinstance(other, self.__class__):
            return self.x == other.x and self.y == other.y and self.z == other.z
        return False

    def __repr__(self):
        def fmt(value):
            # no scientific notation, 16 digits precision, remove trailing zeroes
            return "{:.16f}".format(value).rstrip("0").rstrip(".")
        return "x:{} y:{} z:{}".format(fmt(self.x), fmt(self.y), fmt(self.z))

    @classmethod
    def getPropertyType(cls):
        return XYZRotationProperty

class Colour(Base):
    """Colour

    Can parse hex strings like:
        [#]RGB
        [#]RRGGBB
        [#]AARRGGBB
    and named colors like 'green', 'skyblue', 'whitesmoke' using QtGui.Color.
    """

    pattern = '\s*#?(?:[0-9a-fA-F]+)\s*'
    rex = re.compile(pattern, re.IGNORECASE)

    @classmethod
    def tryParse(cls, strValue, target=None):
        alpha = 0xFF
        red = 0
        green = 0
        blue = 0

        match = re.match(cls.rex, strValue)
        if match is None:
            if QtGui.QColor.isValidColor(strValue):
                qtColor = QtGui.QColor(strValue)
                alpha = qtColor.alpha()
                red = qtColor.red()
                green = qtColor.green()
                blue = qtColor.blue()
            else:
                return None, False
        else:
            value = match.group(0)
            if len(value) == 3:
                # short CSS style RGB
                red = int(value[0] * 2, 16)
                green = int(value[1] * 2, 16)
                blue = int(value[2] * 2, 16)
            elif len(value) == 6:
                # CSS RGB
                red = int(value[0:2], 16)
                green = int(value[2:4], 16)
                blue = int(value[4:6], 16)
            elif len(value) == 8:
                # ARGB
                alpha = int(value[0:2], 16)
                red = int(value[2:4], 16)
                green = int(value[4:6], 16)
                blue = int(value[6:8], 16)
            else:
                return None, False

        if target is None:
            target = cls(red, green, blue, alpha)
        else:
            target.red = red
            target.green = green
            target.blue = blue
            target.alpha = alpha

        return target, True

    @classmethod
    def fromQColor(cls, qtColor):
        return cls(qtColor.red(), qtColor.green(), qtColor.blue(), qtColor.alpha())

    @classmethod
    def fromColour(cls, other):
        return cls(other.red, other.green, other.blue, other.alpha)

    def __init__(self, red=0, green=0, blue=0, alpha=255):
        super(Colour, self).__init__()
        self.red = int(red)
        self.green = int(green)
        self.blue = int(blue)
        self.alpha = int(alpha)

    def getARGB(self):
        return self.blue | (self.green << 8) | (self.red << 16) | (self.alpha << 24)

    def __hash__(self):
        return hash(self.getARGB())

    def __eq__(self, other):
        if isinstance(other, self.__class__):
            return self.getARGB() == other.getARGB()
        return False

    def __repr__(self):
        return "{:08X}".format(self.getARGB())

    @classmethod
    def getPropertyType(cls):
        return ColourProperty

class ColourRect(Base):
    """ColourRect

    Can parse strings like:
        colour
        or
        tl:colour tr:colour bl:colour br:colour

        where colour is:
            [#]RGB
            [#]RRGGBB
            [#]AARRGGBB
            or a named color like 'green', 'skyblue', 'whitesmoke'
    """

    pattern = '\s*#?(?:[0-9a-fA-F]+)\s*'
    rex = re.compile(pattern, re.IGNORECASE)

    @classmethod
    def tryParse(cls, strValue, target=None):
        if not strValue:
            return None, False

        # try to parse as full ColourRect
        values = parsers.parseNamedValues(strValue,
                                             { "tl", "tr", "bl", "br" },
                                             { "tl", "tr", "bl", "br" })
        if values is not None:
            for name, value in values.items():
                colour, valid = Colour.tryParse(value)
                if not valid:
                    return None, False
                values[name] = colour
        else:
            # try to parse as one Colour
            colour, valid = Colour.tryParse(strValue)
            if not valid:
                return None, False
            # assign to all values of ColourRect
            # make copies or else all Colour components will be
            # the same and changing one will change all.
            values = {"tl": Colour.fromColour(colour),
                      "tr": Colour.fromColour(colour),
                      "bl": Colour.fromColour(colour),
                      "br": Colour.fromColour(colour) }

        if target is None:
            target = cls(values["tl"], values["tr"], values["bl"], values["br"])
        else:
            target.topLeft = values["tl"]
            target.topRight = values["tr"]
            target.bottomLeft = values["bl"]
            target.bottomRight = values["br"]

        return target, True

    def __init__(self, tl=Colour(), tr=Colour(), bl=Colour(), br=Colour()):
        super(ColourRect, self).__init__()
        self.topLeft = tl
        self.topRight = tr
        self.bottomLeft = bl
        self.bottomRight = br

    def __hash__(self):
        return hash((hash(self.topLeft), hash(self.topRight), hash(self.bottomLeft), hash(self.bottomRight)))

    def __eq__(self, other):
        if isinstance(other, self.__class__):
            return self.topLeft == other.topLeft and self.topRight == other.topRight and self.bottomLeft == other.bottomLeft and self.bottomRight == other.bottomRight
        return False

    def __repr__(self):
        return "tl:{} tr:{} bl:{} br:{}".format(self.topLeft, self.topRight, self.bottomLeft, self.bottomRight)

    @classmethod
    def getPropertyType(cls):
        return ColourRectProperty



class BaseProperty(properties.Property):
    """Base class for all Property types.

    Note that, by default, it expects the components to map
    directly to an attribute of it's value; with the first letter in lower case.

    For example the UDimProperty has two components, 'Scale' and 'Offset' and
    it also uses the UDim type that has the 'scale' and 'offset' attribute values.
    """

    def createComponents(self):
        super(BaseProperty, self).createComponents()

    def getComponents(self):
        return self.components

    @classmethod
    def getAttrName(cls, componentName):
        """Get the attribute name from the component name."""
        return componentName[:1].lower() + componentName[1:]

    def updateComponents(self, reason=properties.Property.ChangeValueReason.Unknown):
        components = self.getComponents()
        if components is not None:
            for compName, compValue in components.items():
                # set component value from attribute value
                compValue.setValue(getattr(self.value, self.getAttrName(compName)), reason)

    def componentValueChanged(self, component, reason):
        # set attribute value from component value
        setattr(self.value, self.getAttrName(component.name), component.value)
        # trigger our value changed event directly because
        # we didn't call 'setValue()' to do it for us.
        self.valueChanged.trigger(self, properties.Property.ChangeValueReason.ComponentValueChanged)

class UDimProperty(BaseProperty):
    """Property for UDim values."""

    def createComponents(self):
        self.components = OrderedDict()
        self.components["Scale"] = properties.Property(name="Scale", value=self.value.scale, defaultValue=self.defaultValue.scale,
                                            readOnly=self.readOnly, editorOptions=self.editorOptions)
        self.components["Offset"] = properties.Property(name="Offset", value=self.value.offset, defaultValue=self.defaultValue.offset,
                                            readOnly=self.readOnly, editorOptions=self.editorOptions)

        super(UDimProperty, self).createComponents()

    def isStringRepresentationEditable(self):
        return True

    def tryParse(self, strValue):
        return UDim.tryParse(strValue)

class USizeProperty(BaseProperty):
    """Property for USize values."""

    def createComponents(self):
        self.components = OrderedDict()
        self.components["Width"] = UDimProperty(name="Width", value=self.value.width, defaultValue=self.defaultValue.width,
                                                readOnly=self.readOnly, editorOptions=self.editorOptions)
        self.components["Height"] = UDimProperty(name="Height", value=self.value.height, defaultValue=self.defaultValue.height,
                                                readOnly=self.readOnly, editorOptions=self.editorOptions)

        super(USizeProperty, self).createComponents()

    def isStringRepresentationEditable(self):
        return True

    def tryParse(self, strValue):
        return USize.tryParse(strValue)

class UVector2Property(BaseProperty):
    """Property for UVector2 values."""

    def createComponents(self):
        self.components = OrderedDict()
        self.components["X"] = UDimProperty(name="X", value=self.value.x, defaultValue=self.defaultValue.x,
                                            readOnly=self.readOnly, editorOptions=self.editorOptions)
        self.components["Y"] = UDimProperty(name="Y", value=self.value.y, defaultValue=self.defaultValue.y,
                                            readOnly=self.readOnly, editorOptions=self.editorOptions)

        super(UVector2Property, self).createComponents()

    def isStringRepresentationEditable(self):
        return True

    def tryParse(self, strValue):
        return UVector2.tryParse(strValue)

class URectProperty(BaseProperty):
    """Property for URect values."""

    def createComponents(self):
        self.components = OrderedDict()
        self.components["Left"] = UDimProperty(name="Left", value=self.value.left, defaultValue=self.defaultValue.left,
                                               readOnly=self.readOnly, editorOptions=self.editorOptions)
        self.components["Top"] = UDimProperty(name="Top", value=self.value.top, defaultValue=self.defaultValue.top,
                                              readOnly=self.readOnly, editorOptions=self.editorOptions)
        self.components["Right"] = UDimProperty(name="Right", value=self.value.right, defaultValue=self.defaultValue.right,
                                                readOnly=self.readOnly, editorOptions=self.editorOptions)
        self.components["Bottom"] = UDimProperty(name="Bottom", value=self.value.bottom, defaultValue=self.defaultValue.bottom,
                                                 readOnly=self.readOnly, editorOptions=self.editorOptions)

        super(URectProperty, self).createComponents()

    def isStringRepresentationEditable(self):
        return True

    def tryParse(self, strValue):
        return URect.tryParse(strValue)

class QuaternionProperty(BaseProperty):
    """Property for Quaternion values."""

    def createComponents(self):
        self.components = OrderedDict()

        # TODO: Set min/max/step for W, X, Y, Z. See how it's done on XYZRotationProperty.
        self.components["W"] = properties.Property(name="W", value=self.value.w, defaultValue=self.defaultValue.w,
                                            readOnly=self.readOnly, editorOptions=self.editorOptions)
        self.components["X"] = properties.Property(name="X", value=self.value.x, defaultValue=self.defaultValue.x,
                                            readOnly=self.readOnly, editorOptions=self.editorOptions)
        self.components["Y"] = properties.Property(name="Y", value=self.value.y, defaultValue=self.defaultValue.y,
                                            readOnly=self.readOnly, editorOptions=self.editorOptions)
        self.components["Z"] = properties.Property(name="Z", value=self.value.z, defaultValue=self.defaultValue.z,
                                            readOnly=self.readOnly, editorOptions=self.editorOptions)

        self.components["Degrees"] = XYZRotationProperty(name="Degrees",
                                                         value=XYZRotation.fromQuaternion(self.value),
                                                         defaultValue=XYZRotation.fromQuaternion(self.defaultValue),
                                                         readOnly=self.readOnly,
                                                         editorOptions=self.editorOptions)

        super(QuaternionProperty, self).createComponents()

    def updateComponents(self, reason=properties.Property.ChangeValueReason.Unknown):
        components = self.getComponents()
        if components is not None:
            components["W"].setValue(self.value.w, reason)
            components["X"].setValue(self.value.x, reason)
            components["Y"].setValue(self.value.y, reason)
            components["Z"].setValue(self.value.z, reason)
            components["Degrees"].setValue(XYZRotation.fromQuaternion(self.value), reason)

    def componentValueChanged(self, component, reason):
        if component.name == "Degrees":
            (wv, xv, yv, zv) = Quaternion.convertEulerDegreesToQuaternion(component.value.x, component.value.y, component.value.z)
            self.components["W"].setValue(wv)
            self.components["X"].setValue(xv)
            self.components["Y"].setValue(yv)
            self.components["Z"].setValue(zv)
            self.valueChanged.trigger(self, properties.Property.ChangeValueReason.ComponentValueChanged)
        else:
            super(QuaternionProperty, self).componentValueChanged(component, reason)

    def isStringRepresentationEditable(self):
        return True

    def tryParse(self, strValue):
        return Quaternion.tryParse(strValue)

class XYZRotationProperty(BaseProperty):
    """Property for XYZRotation values."""

    def createComponents(self):
        editorOptions = { "numeric": { "min": -360, "max": 360, "wrapping": True } }

        self.components = OrderedDict()

        self.components["X"] = properties.Property(name="X", value=self.value.x, defaultValue=self.defaultValue.x,
                                            readOnly=self.readOnly, editorOptions=editorOptions)
        self.components["Y"] = properties.Property(name="Y", value=self.value.y, defaultValue=self.defaultValue.y,
                                            readOnly=self.readOnly, editorOptions=editorOptions)
        self.components["Z"] = properties.Property(name="Z", value=self.value.z, defaultValue=self.defaultValue.z,
                                            readOnly=self.readOnly, editorOptions=editorOptions)

        super(XYZRotationProperty, self).createComponents()

    def isStringRepresentationEditable(self):
        return True

    def tryParse(self, strValue):
        return XYZRotation.tryParse(strValue)

class ColourProperty(BaseProperty):
    """Property for Colour values."""

    def createComponents(self):
        editorOptions = { "numeric": { "min": 0, "max": 255 } }

        self.components = OrderedDict()

        self.components["Alpha"] = properties.Property(name="Alpha", value=self.value.alpha, defaultValue=self.defaultValue.alpha,
                                                       readOnly=self.readOnly, editorOptions=editorOptions)
        self.components["Red"] = properties.Property(name="Red", value=self.value.red, defaultValue=self.defaultValue.red,
                                                     readOnly=self.readOnly, editorOptions=editorOptions)
        self.components["Green"] = properties.Property(name="Green", value=self.value.green, defaultValue=self.defaultValue.green,
                                                       readOnly=self.readOnly, editorOptions=editorOptions)
        self.components["Blue"] = properties.Property(name="Blue", value=self.value.blue, defaultValue=self.defaultValue.blue,
                                                      readOnly=self.readOnly, editorOptions=editorOptions)

        super(ColourProperty, self).createComponents()

    def isStringRepresentationEditable(self):
        return True

    def tryParse(self, strValue):
        return Colour.tryParse(strValue)

class ColourRectProperty(BaseProperty):
    """Property for ColourRect values."""

    def createComponents(self):
        self.components = OrderedDict()

        self.components["TopLeft"] = ColourProperty(name="TopLeft", value=self.value.topLeft, defaultValue=self.defaultValue.topLeft,
                                                    readOnly=self.readOnly, editorOptions=self.editorOptions)
        self.components["TopRight"] = ColourProperty(name="TopRight", value=self.value.topRight, defaultValue=self.defaultValue.topRight,
                                                     readOnly=self.readOnly, editorOptions=self.editorOptions)
        self.components["BottomLeft"] = ColourProperty(name="BottomLeft", value=self.value.bottomLeft, defaultValue=self.defaultValue.bottomLeft,
                                                       readOnly=self.readOnly, editorOptions=self.editorOptions)
        self.components["BottomRight"] = ColourProperty(name="BottomRight", value=self.value.bottomRight, defaultValue=self.defaultValue.bottomRight,
                                                        readOnly=self.readOnly, editorOptions=self.editorOptions)

        super(ColourRectProperty, self).createComponents()

    def isStringRepresentationEditable(self):
        return True

    def tryParse(self, strValue):
        return ColourRect.tryParse(strValue)

class StringWrapper(Base):
    """Simple string that does no parsing but allows us to map editors to it"""

    def __init__(self, value):
        super(StringWrapper, self).__init__()
        self.value = value

    def __hash__(self):
        return hash(self.value)

    def __eq__(self, other):
        if isinstance(other, self.__class__):
            return self.value == other.value
        return False

    def __repr__(self):
        return self.value

    @classmethod
    def getPropertyType(cls):
        return BaseProperty

class FontRef(StringWrapper):
    @classmethod
    def tryParse(cls, strValue, target=None):
        return FontRef(strValue), True

class ImageRef(StringWrapper):
    @classmethod
    def tryParse(cls, strValue, target=None):
        return ImageRef(strValue), True
