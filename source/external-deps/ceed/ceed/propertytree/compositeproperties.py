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

"""The standard composite properties.

DictionaryProperty -- Generic property based on a dictionary.
"""

from collections import OrderedDict

from .properties import Property

from .parsers import AstHelper

class DictionaryProperty(Property):
    """A generic composite property based on a dict (or OrderedDict).

    The key-value pairs are used as components. A value can be a Property
    itself, allowing nested properties and the creation of multi-level,
    hierarchical properties.

    Example::
        colourProp = DictionaryProperty(
                                        name = "Colour",
                                        value = OrderedDict([
                                                             ("Red", 160),
                                                             ("Green", 255),
                                                             ("Blue", 160)
                                                             ]),
                                        editorOptions = {"instantApply":False, "numeric": {"min":0, "max":255, "step": 8}}
                                        )
        DictionaryProperty("dictionary", OrderedDict([
                                                      ("X", 0),
                                                      ("Y", 0),
                                                      ("Width", 50),
                                                      ("Height", 50),
                                                      ("Colour", colourProp)
                                                      ]),
                           readOnly=False)
    """

    class StringRepresentationMode(object):
        #pylint: disable-msg=R0903
        # too few public methods - it's an enum dammit
        ReadOnly = 0
        EditValuesRestrictTypes = 1
        EditValuesFreeTypes = 2
        EditKeysAndValues = 3

    def __init__(self, name, value=None, category=None, helpText=None, readOnly=False, editorOptions=None,
                 strReprMode=StringRepresentationMode.ReadOnly,
                 strValueReplacements=None):
        super(DictionaryProperty, self).__init__(name = name,
                                                 value = value,
                                                 defaultValue = None,
                                                 category = category,
                                                 helpText = helpText,
                                                 readOnly = readOnly,
                                                 editorOptions = editorOptions)
        self.strReprMode = strReprMode
        self.strValueReplacements = strValueReplacements

    def createComponents(self):
        self.components = OrderedDict()
        for name, value in self.value.items():
            # if the value of the item is a Property, make sure
            # it's in a compatible state (i.e. readOnly) and add it
            # as a component directly.
            if isinstance(value, Property):
                # make it read only if we're read only
                if self.readOnly:
                    value.readOnly = True
                # ensure it's name is the our key name
                value.name = str(name)
                # add it
                self.components[name] = value
            # if it's any other value, create a Property for it.
            else:
                self.components[name] = Property(name=name, value=value, defaultValue=value, readOnly=self.readOnly, editorOptions=self.editorOptions)

        # call super to have it subscribe to our components;
        # it will call 'getComponents()' to get them.
        super(DictionaryProperty, self).createComponents()

    def hasDefaultValue(self):
        # it doesn't really make sense to maintain a default value for this property.
        # we check whether our components have their default values instead.
        for comp in self.components.values():
            if not comp.hasDefaultValue():
                return False
        return True

    def getComponents(self):
        return self.components

    def valueToString(self):
        gen = ("%s:%s" % (prop.name, prop.valueToString()) for prop in self.components.values())
        return "{%s}" % (", ".join(gen))

    def isStringRepresentationEditable(self):
        return self.strReprMode != self.StringRepresentationMode.ReadOnly

    def tryParse(self, strValue):
        try:
            value = AstHelper.parseOrderedDict(strValue, self.strValueReplacements)
            if isinstance(value, OrderedDict):
                # we parsed it successfully and it's a dictionary.
                # now make sure it agrees with our edit mode.
                valid = False

                # all changes allowed
                if self.strReprMode == self.StringRepresentationMode.EditKeysAndValues:
                    valid = True
                # keys must remain the same, values can change type or value
                elif self.strReprMode == self.StringRepresentationMode.EditValuesFreeTypes:
                    valid = self.value.keys() == value.keys()
                # only values can be changed but not their type
                elif self.strReprMode == self.StringRepresentationMode.EditValuesRestrictTypes:
                    # ensure keys are the same
                    valid = self.value.keys() == value.keys()
                    if valid:
                        # check value types
                        for key, value in self.value.items():
                            if type(value) != type(value[key]):
                                valid = False
                                break

                return value, valid
        except ValueError:
            pass
        except SyntaxError:
            pass
        return None, False

    def componentValueChanged(self, component, reason):
        self.value[component.name] = component.value
        self.valueChanged.trigger(self, Property.ChangeValueReason.ComponentValueChanged)

    def updateComponents(self, reason=Property.ChangeValueReason.Unknown):
        # check if our value and our components match
        # and if not, recreate our components.
        # we do this on this Property because our value is a dictionary
        # and its items are not fixed.

        # if our keys are the same as our components' keys, simply update the values
        if self.value.keys() == self.components.keys():
            for name in self.value:
                self.components[name].setValue(self.value[name], reason)
        else:
            # recreate our components
            self.finaliseComponents()
            self.createComponents()
