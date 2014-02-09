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

"""Settings for CEGUI properties.

PropertyMappingEntry -- Settings for one property, identified by its origin and name.
PropertyMap -- PropertyMappingEntry container.
"""

from collections import OrderedDict

from xml.etree import cElementTree as ElementTree

class PropertyMappingEntry(object):
    """Maps a CEGUI::Property (by origin and name) to a CEGUI Type and PropertyEditor
    to allow its viewing and editing.

    If target inspector name is \"\" then this mapping means that the property should
    be ignored in the property set inspector listing.
    """

    @classmethod
    def fromElement(cls, element):
        propertyOrigin = element.get("propertyOrigin")
        propertyName = element.get("propertyName")
        typeName = element.get("typeName")
        hidden = element.get("hidden", "False").lower() in ("true", "yes", "1")
        editorName = element.get("editorName")

        editorSettings = dict()
        for settings in element.findall("settings"):
            name = settings.get("name")
            entries = OrderedDict()

            for setting in settings.findall("setting"):
                entries[setting.get("name")] = setting.get("value")

            editorSettings[name] = entries

        return cls(propertyOrigin = propertyOrigin,
                   propertyName = propertyName,
                   typeName = typeName,
                   hidden = hidden,
                   editorName = editorName,
                   editorSettings = editorSettings)

    @classmethod
    def makeKey(cls, propertyOrigin, propertyName):
        return "/".join((propertyOrigin, propertyName))

    def __init__(self, propertyOrigin, propertyName,
                 typeName = None, hidden = False,
                 editorName = None, editorSettings = None):

        self.propertyOrigin = propertyOrigin
        self.propertyName = propertyName
        self.typeName = typeName
        self.hidden = hidden
        self.editorName = editorName
        self.editorSettings = editorSettings if editorSettings is not None else dict()

    def getPropertyKey(self):
        return self.makeKey(self.propertyOrigin, self.propertyName)

    def saveToElement(self):
        element = ElementTree.Element("mapping")

        element.set("propertyOrigin", self.propertyOrigin)
        element.set("propertyName", self.propertyName)
        if self.typeName:
            element.set("typeName", self.typeName)
        if self.hidden:
            element.set("hidden", True)
        if self.editorName:
            element.set("editorName", self.editorName)

        for name, value in self.editorSettings:
            settings = ElementTree.Element("settings")
            settings.set("name", name)
            for sname, svalue in value:
                setting = ElementTree.Element("setting")
                setting.set("name", sname)
                setting.set("value", svalue)
                element.append(setting)
            element.append(settings)

        return element

class PropertyMap(object):
    """Container for property mapping entries."""

    @classmethod
    def fromElement(cls, element):
        """Create and return an instance from an XML element."""
        assert(element.get("version") == compat.manager.EditorNativeType)

        pmap = cls()
        for entryElement in element.findall("mapping"):
            entry = PropertyMappingEntry.fromElement(entryElement)
            pmap.setEntry(entry)
        return pmap

    @classmethod
    def fromXMLString(cls, text):
        """Create and return an instance from an XML string."""
        element = ElementTree.fromstring(text)
        return cls.fromElement(element)

    @classmethod
    def fromFile(cls, absolutePath):
        """Create and return an instance from an XML file path."""
        text = open(absolutePath, "r").read()
        return cls.fromXMLString(text)

    @classmethod
    def fromFiles(cls, absolutePaths):
        """Create and return an instance from a list of XML file paths.

        Entries from files later in the list replace entries with the same
        property key from previous files.
        """
        pmap = cls()
        for absolutePath in absolutePaths:
            pmap.update(cls.fromFile(absolutePath))

        return pmap

    def __init__(self):
        """Initialise an empty property map."""
        self.entries = dict()

    def saveToElement(self):
        """Create and return an XML element for this map instance."""
        element = ElementTree.Element("mappings")
        element.set("version", compat.manager.EditorNativeType)

        for entry in sorted(self.entries, key = lambda entry: entry.getPropertyKey()):
            eel = entry.saveToElement()
            element.append(eel)

        return element

    def getEntry(self, propertyOrigin, propertyName):
        """Find and return the entry with the specified origin and name, or None."""
        entry = self.entries.get(PropertyMappingEntry.makeKey(propertyOrigin, propertyName))
        return entry

    def setEntry(self, entry):
        """Set or replace an entry with a new entry."""
        self.entries[entry.getPropertyKey()] = entry

    def update(self, pmap):
        """Update the entries using the entries of another map."""
        self.entries.update(pmap.entries)

from ceed.compatibility import property_mappings as compat
