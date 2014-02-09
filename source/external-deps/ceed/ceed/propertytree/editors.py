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

"""The standard editors for the property tree.

PropertyEditorRegistry -- Maintains a list of the available editors and the types they can edit; creates the appropriate editor for a property.
PropertyEditor -- The base class for all property editors.
StringPropertyEditor -- Editor for strings.
NumericPropertyEditor -- Editor for integers and floating point numbers.
StringWrapperValidator -- Edit widget validator for the StringWrapperProperty.
EnumValuePropertyEditor -- Editor for EnumValue-based values (Combo box).
"""

import abc

from . import utility
from .properties import Property
from .properties import EnumValue

from ceed.cegui import ceguitypes as ct

from PySide import QtGui

class PropertyEditorRegistry(object):
    """The registry contains a (sorted) list of property editor
    types that can be used for each value type.
    """

    # Standard editors place themselves in this set,
    # used by registerStandardEditors().
    _standardEditors = set()

    @staticmethod
    def addStandardEditor(editorType):
        """Add the editor to the list of standard editors."""
        PropertyEditorRegistry._standardEditors.add(editorType)

    def __init__(self, autoRegisterStandardEditors=False):
        """Initialise an empty instance by default.
        If "autoRegisterStandardsEditors" is True,
        automatically register the standard editors.
        """
        self.editorsForValueType = dict()
        self.registeredEditorTypes = set()
        if autoRegisterStandardEditors:
            self.registerStardardEditors()

    def register(self, editorType, priority=0):
        """Register an editor with the specified priority.
        Higher values equal higher priority.

        Note: The priority is added to the editor's preferred
        priority for each value type supported. For example. if
        an editor specifies priority 10 for integers, registering
        the editor with priority = -3 will register it for
        integers with priority 7.

        Note: Registering the same editor twice does nothing
        and does not change the priority.
        """
        # If the editor hasn't been registered here already
        if not editorType in self.registeredEditorTypes:
            self.registeredEditorTypes.add(editorType)
            # Get supported types and priorities
            for valueType, valuePriority in editorType.getSupportedValueTypes().items():
                tup = (priority + valuePriority, editorType)
                # If we don't know this value type at all,
                # create a new list for this value type having
                # only one element with this editor.
                if not valueType in self.editorsForValueType:
                    self.editorsForValueType[valueType] = [tup]
                # If we already know this value type,
                # append the new editor and sort again so make
                # those of higher priority be first.
                else:
                    self.editorsForValueType[valueType].append(tup)
                    self.editorsForValueType[valueType].sort(reverse = True)

    def registerStardardEditors(self):
        """Register the predefined editors to this instance."""
        for editor in self._standardEditors:
            self.register(editor)

    def createEditor(self, editProperty):
        """Create and return an editor for the specified property,
        or None if none can be found that have been registered and
        are capable of editing the property.

        If more than one editors that can edit the property have been
        registered, the one with the highest priority will be chosen.
        """
        valueType = editProperty.valueType()
        if valueType in self.editorsForValueType:
            return self.editorsForValueType[valueType][0][1](editProperty)

        return None

class PropertyEditor(object):
    """Abstract base class for a property editor.

    A property editor instance is created when required
    to edit the value of a (supported) property.
    """

    __metaclass__ = abc.ABCMeta

    @classmethod
    def getSupportedValueTypes(cls):
        """Return a dictionary containing the types supported by this
        editor and their priorities. The key is the type and the value
        is the priority.

        Example: ``{ int:0, float:-10 }``
            Uses the default priority for integers and is most likely
            the preferred editor unless another editor is registered
            with higher priority. Standard editors should always have
            the default (0) or less priority.
            It can also edit floats but it should have low priority
            for those types (maybe it truncates or rounds them).
        """
        return None

    def __init__(self, boundProperty, instantApply=True, ownsProperty=False):
        """Initialise an instance and keeps a reference
        to the property that will be edited.
        """
        self.editWidget = None
        self.property = boundProperty
        self.instantApply = self.property.getEditorOption("instantApply", instantApply)
        self.ownsProperty = ownsProperty

        # see valueChanging()
        self.widgetValueInitialized = False

    def finalise(self):
        if self.ownsProperty:
            self.property.finalise()

    @abc.abstractmethod
    def createEditWidget(self, parent):
        """Create and return a widget that will be used
        to edit the property.

        This is a good place to set up minimums and maximums,
        allowed patterns, step sizes etc. but setting the value
        is not required because a call to setWidgetValue
        will follow.
        """
        pass

    @abc.abstractmethod
    def getWidgetValue(self):
        """Read and return a tuple with the current value of the widget
        and a boolean specifying whether it's a valid value or not."""
        pass

    @abc.abstractmethod
    def setWidgetValueFromProperty(self):
        """Set the value of the widget to the value of the property.

        Implementors should set self.widgetValueInitialized to True
        *after* setting the widget value.
        """
        self.widgetValueInitialized = True

    def setPropertyValueFromWidget(self):
        """Set the value of the property to value of the widget."""
        value, valid = self.getWidgetValue()
        if valid and value != self.property.value:
            self.property.setValue(value, Property.ChangeValueReason.Editor)

    def valueChanging(self):
        """Callback-style method used when the 'instantApply' option
        is True to update the property's value without waiting for
        the editor to commit the edit.
        Should be called by implementors whenever the value of
        the widget changes, even if it hasn't been committed.
        """
        if not self.instantApply:
            return False

        # We ignore the first event because that one
        # occurs when we initialise the edit widget's value.
        if not self.widgetValueInitialized:
            return False

        self.setPropertyValueFromWidget()
        return True

class StringPropertyEditor(PropertyEditor):
    """Editor for strings.

    Supports line edit (the default) or combo-box mode.

    The combo-box mode is activated when the "combo/" editor options are set.
    For example, to set it via the property mappings:
        <mapping propertyOrigin="Element" propertyName="HorizontalAlignment">
            <settings name="combo">
                <setting name="Left" value="Left" />
                <setting name="Centre" value="Centre" />
                <setting name="Right" value="Right" />
            </settings>
        </mapping>

    Note that the EnumValuePropertyEditor has similar functionality but
    is based on known types and does not take the list of values from
    the editor options.
    """

    @classmethod
    def getSupportedValueTypes(cls):
        return { str:0, unicode:0 }

    def __init__(self, boundProperty, instantApply=True, ownsProperty=False):
        super(StringPropertyEditor, self).__init__(boundProperty, instantApply=instantApply, ownsProperty=ownsProperty)
        self.mode = "line"

    def createEditWidget(self, parent):
        options = self.property.getEditorOption("string/", {})

        comboOptions = self.property.getEditorOption("combo/", None)

        if comboOptions is None:
            self.mode = "line"
            self.editWidget = QtGui.QLineEdit(parent)
            self.editWidget.textEdited.connect(self.valueChanging)

            # setup options
            self.editWidget.setMaxLength(int(options.get("maxLength", self.editWidget.maxLength())))
            self.editWidget.setPlaceholderText(options.get("placeholderText", self.editWidget.placeholderText()))
            self.editWidget.setInputMask(options.get("inputMask", self.editWidget.inputMask()))
            self.editWidget.setValidator(options.get("validator", self.editWidget.validator()))
        else:
            self.mode = "combo"
            self.editWidget = QtGui.QComboBox(parent)
            for name, value in comboOptions.items():
                self.editWidget.addItem(name, value)
            self.editWidget.activated.connect(self.valueChanging)

        return self.editWidget

    def getWidgetValue(self):
        if self.mode == "combo":
            idx = self.editWidget.currentIndex()
            return (self.editWidget.itemData(idx), True) if idx != -1 else (None, False)
        else:
            return (self.editWidget.text(), True) if self.editWidget.hasAcceptableInput() else ("", False)

    def setWidgetValueFromProperty(self):
        value, valid = self.getWidgetValue()
        if (not valid) or (self.property.value != value):
            if self.mode == "combo":
                idx = self.editWidget.findData(self.property.value)
                self.editWidget.setCurrentIndex(idx)
            else:
                self.editWidget.setText(self.property.value)

        super(StringPropertyEditor, self).setWidgetValueFromProperty()

PropertyEditorRegistry.addStandardEditor(StringPropertyEditor)

class NumericPropertyEditor(PropertyEditor):

    DefaultDecimals = 16
    DefaultMin = -999999
    DefaultMax = 999999

    @classmethod
    def getSupportedValueTypes(cls):
        return { int:0, float:0 }

    def createEditWidget(self, parent):
        self.editWidget = QtGui.QDoubleSpinBox(parent)
        self.editWidget.valueChanged.connect(self.valueChanging)

        # setup options
        options = self.property.getEditorOption("numeric/", {})

        # set decimals first because according to the docs setting it can change the range.
        self.editWidget.setDecimals(float(options.get("decimals", 0 if self.property.valueType() == int else self.DefaultDecimals)))
        self.editWidget.setRange(float(options.get("min", self.DefaultMin)),
                                 float(options.get("max", self.DefaultMax)))
        # make the default step 0.1 if the value range is 1 or less, otherwise 1
        self.editWidget.setSingleStep(float(options.get("step", 0.1 if abs(self.editWidget.maximum() - self.editWidget.minimum()) <= 1.0 else 1)))
        self.editWidget.setWrapping(utility.boolFromString(options.get("wrapping", False)))
        self.editWidget.setButtonSymbols(QtGui.QAbstractSpinBox.ButtonSymbols(int(options.get("buttons", self.editWidget.buttonSymbols()))))

        return self.editWidget

    def getWidgetValue(self):
        # the call to the constructor of the type is done so we return
        # an integer and not a float if the property's type is an integer.
        # it should be future proof too.
        return self.property.valueType()(self.editWidget.value()), True

    def setWidgetValueFromProperty(self):
        value, valid = self.getWidgetValue()
        if (not valid) or (self.property.value != value):
            self.editWidget.setValue(self.property.value)

        super(NumericPropertyEditor, self).setWidgetValueFromProperty()

PropertyEditorRegistry.addStandardEditor(NumericPropertyEditor)

class BoolPropertyEditor(PropertyEditor):

    @classmethod
    def getSupportedValueTypes(cls):
        return { bool:0 }

    def createEditWidget(self, parent):
        self.editWidget = QtGui.QCheckBox(parent)
        self.editWidget.setAutoFillBackground(True)
        self.editWidget.stateChanged.connect(self.valueChanging)

        return self.editWidget

    def getWidgetValue(self):
        return self.editWidget.isChecked(), True

    def setWidgetValueFromProperty(self):
        value, valid = self.getWidgetValue()
        if (not valid) or (self.property.value != value):
            self.editWidget.setChecked(self.property.value)

        super(BoolPropertyEditor, self).setWidgetValueFromProperty()

PropertyEditorRegistry.addStandardEditor(BoolPropertyEditor)

class StringWrapperValidator(QtGui.QValidator):
    """Validate the edit widget value when editing
    a StringWrapperProperty.

    Using this prevents closing the edit widget using
    "Enter" when the value is invalid and allows the
    user the correct their mistake without losing any
    editing they have done.
    """
    def __init__(self, swProperty, parent=None):
        super(StringWrapperValidator, self).__init__(parent)
        self.property = swProperty

    def validate(self, inputStr, dummyPos):
        _, valid = self.property.tryParse(inputStr)
        return QtGui.QValidator.Intermediate if not valid else QtGui.QValidator.Acceptable

class EnumValuePropertyEditor(PropertyEditor):
    """Editor for EnumValue-based values (Combo box)."""

    @classmethod
    def getSupportedValueTypes(cls):
        # Support all types that are subclasses of EnumValue.
        vts = set()
        def addSubclasses(baseType):
            for vt in baseType.__subclasses__():
                vts.add(vt)
                addSubclasses(vt)
        addSubclasses(EnumValue)

        return dict((vt, -10) for vt in vts)

    def createEditWidget(self, parent):
        self.editWidget = QtGui.QComboBox(parent)
        for data, text in self.property.value.getEnumValues().items():
            self.editWidget.addItem(text, data)
        self.editWidget.activated.connect(self.valueChanging)

        return self.editWidget

    def getWidgetValue(self):
        idx = self.editWidget.currentIndex()
        return (self.property.valueType()(self.editWidget.itemData(idx)), True) if idx != -1 else (None, False)

    def setWidgetValueFromProperty(self):
        value, valid = self.getWidgetValue()
        if (not valid) or (self.property.value != value):
            idx = self.editWidget.findData(self.property.valueToString())
            if idx != -1:
                self.editWidget.setCurrentIndex(idx)

        super(EnumValuePropertyEditor, self).setWidgetValueFromProperty()

PropertyEditorRegistry.addStandardEditor(EnumValuePropertyEditor)

class DynamicChoicesEditor(PropertyEditor):
    """Editor for strings where user chooses from several options like in a combobox.

    The difference is that this combobox gets the values from an external place
    dynamically. You simply override the getChoices method of this class.
    """

    def __init__(self, boundProperty, instantApply=True, ownsProperty=False):
        super(DynamicChoicesEditor, self).__init__(boundProperty, instantApply=instantApply, ownsProperty=ownsProperty)

    def getChoices(self):
        raise NotImplementedError("All subclasses of DynamicChoicesEditor have to implement the 'getChoices' method!")

    def createEditWidget(self, parent):
        self.editWidget = QtGui.QComboBox(parent)
        for name, value in self.getChoices():
            self.editWidget.addItem(name, value)
        self.editWidget.activated.connect(self.valueChanging)

        return self.editWidget

    def getWidgetValue(self):
        idx = self.editWidget.currentIndex()
        return (self.editWidget.itemData(idx), True) if idx != -1 else (None, False)

    def setWidgetValueFromProperty(self):
        value, valid = self.getWidgetValue()
        if (not valid) or (self.property.value != value):
            for idx in xrange(self.editWidget.count()):
                if self.editWidget.itemData(idx) == self.property.value:
                    self.editWidget.setCurrentIndex(idx)
                    break

        super(DynamicChoicesEditor, self).setWidgetValueFromProperty()

class FontEditor(DynamicChoicesEditor):
    @classmethod
    def getSupportedValueTypes(cls):
        return { ct.FontRef:0 }

    def __init__(self, boundProperty, instantApply=True, ownsProperty=False):
        super(FontEditor, self).__init__(boundProperty, instantApply=instantApply, ownsProperty=ownsProperty)

    def getChoices(self):
        ceguiInstance = mainwindow.MainWindow.instance.ceguiInstance

        ret = [("", ct.FontRef(""))] # GUI Context default font

        if ceguiInstance is not None:
            ret.extend([(font, ct.FontRef(font)) for font in ceguiInstance.getAvailableFonts()])

        return ret

PropertyEditorRegistry.addStandardEditor(FontEditor)

class ImageEditor(DynamicChoicesEditor):
    @classmethod
    def getSupportedValueTypes(cls):
        return { ct.ImageRef:0 }

    def __init__(self, boundProperty, instantApply=True, ownsProperty=False):
        super(ImageEditor, self).__init__(boundProperty, instantApply=instantApply, ownsProperty=ownsProperty)

    def getChoices(self):
        ceguiInstance = mainwindow.MainWindow.instance.ceguiInstance

        ret = [("", ct.ImageRef(""))] # GUI Context default font

        if ceguiInstance is not None:
            ret.extend([(image, ct.ImageRef(image)) for image in ceguiInstance.getAvailableImages()])

        return ret

PropertyEditorRegistry.addStandardEditor(ImageEditor)

from ceed import mainwindow
