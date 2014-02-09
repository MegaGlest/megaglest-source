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

from PySide import QtCore
from PySide import QtGui

from ceed import qtwidgets

# Implementation notes
# - The "change detection" scheme propagates upwards from the individual Entry
#   types to their parents (currently terminated at the Category/Tab level).
# - In contrast, when a user applies changes, this propagates downwards from
#   the Category/Tab level to the individual (modified) entries.
# - The reason is because the settings widgets (QLineEdit, QCheckBox, etc) are
#   used to notify the application when a change happens; and once changes are
#   applied, it is convenient to use an iterate/apply mechanism.

# Wrapper: Entry types
# - One for each 'widgetHint'.
class InterfaceEntry(QtGui.QHBoxLayout):
    def __init__(self, entry, parent):
        super(InterfaceEntry, self).__init__()
        self.entry = entry
        self.parent = parent

    def _addBasicWidgets(self):
        self.addWidget(self.entryWidget, 1)
        self.addWidget(self._buildResetButton())

    def _buildResetButton(self):
        self.entryWidget.slot_resetToDefault = self.resetToDefaultValue
        ret = QtGui.QPushButton()
        ret.setIcon(QtGui.QIcon("icons/settings/reset_entry_to_default.png"))
        ret.setIconSize(QtCore.QSize(16, 16))
        ret.setToolTip("Reset this settings entry to the default value")
        ret.clicked.connect(self.entryWidget.slot_resetToDefault)
        return ret

    def discardChanges(self):
        self.entry.hasChanges = False

    def onChange(self, _):
        self.markAsChanged()
        self.parent.onChange(self)

    def markAsChanged(self):
        self.entry.markAsChanged()

    def markAsUnchanged(self):
        self.entry.markAsUnchanged()

class InterfaceEntryString(InterfaceEntry):
    def __init__(self, entry, parent):
        super(InterfaceEntryString, self).__init__(entry, parent)
        self.entryWidget = QtGui.QLineEdit()
        self.entryWidget.setText(entry.value)
        self.entryWidget.setToolTip(entry.help)
        self.entryWidget.textEdited.connect(self.onChange)
        self._addBasicWidgets()

    def discardChanges(self):
        self.entryWidget.setText(str(self.entry.value))
        super(InterfaceEntryString, self).discardChanges()

    def onChange(self, text):
        self.entry.editedValue = str(text)
        super(InterfaceEntryString, self).onChange(text)

class InterfaceEntryInt(InterfaceEntry):
    def __init__(self, entry, parent):
        super(InterfaceEntryInt, self).__init__(entry, parent)
        self.entryWidget = QtGui.QLineEdit()
        self.entryWidget.setText(str(entry.value))
        self.entryWidget.setToolTip(entry.help)
        self.entryWidget.textEdited.connect(self.onChange)
        self._addBasicWidgets()

    def discardChanges(self):
        self.entryWidget.setText(str(self.entry.value))
        super(InterfaceEntryInt, self).discardChanges()

    def resetToDefaultValue(self):
        defValue = self.entry.defaultValue
        if self.entry.editedValue != defValue:
            self.onChange(defValue)
            self.entryWidget.setText(str(defValue))

    def onChange(self, text):
        try:
            self.entry.editedValue = int(text)
        except ValueError:
            ew = self.entryWidget
            ew.setText(ew.text()[:-1])

        super(InterfaceEntryInt, self).onChange(text)

class InterfaceEntryFloat(InterfaceEntry):
    def __init__(self, entry, parent):
        super(InterfaceEntryFloat, self).__init__(entry, parent)
        self.entryWidget = QtGui.QLineEdit()
        self.entryWidget.setText(str(entry.value))
        self.entryWidget.setToolTip(entry.help)
        self.entryWidget.textEdited.connect(self.onChange)
        self._addBasicWidgets()

    def discardChanges(self):
        self.entryWidget.setText(str(self.entry.value))
        super(InterfaceEntryFloat, self).discardChanges()

    def resetToDefaultValue(self):
        defValue = self.entry.defaultValue
        if self.entry.editedValue != defValue:
            self.onChange(defValue)
            self.entryWidget.setText(str(defValue))

    def onChange(self, text):
        try:
            self.entry.editedValue = float(text)
        except ValueError:
            ew = self.entryWidget
            ew.setText(ew.text()[:-1])

        super(InterfaceEntryFloat, self).onChange(text)

class InterfaceEntryCheckbox(InterfaceEntry):
    def __init__(self, entry, parent):
        super(InterfaceEntryCheckbox, self).__init__(entry, parent)
        self.entryWidget = QtGui.QCheckBox()
        self.entryWidget.setChecked(entry.value)
        self.entryWidget.setToolTip(entry.help)
        self.entryWidget.stateChanged.connect(self.onChange)
        self._addBasicWidgets()

    def discardChanges(self):
        self.entryWidget.setChecked(self.entry.value)
        super(InterfaceEntryCheckbox, self).discardChanges()

    def resetToDefaultValue(self):
        defValue = self.entry.defaultValue
        if self.entry.editedValue != defValue:
            self.onChange(defValue)
            self.entryWidget.setChecked(defValue)

    def onChange(self, state):
        self.entry.editedValue = state
        super(InterfaceEntryCheckbox, self).onChange(state)

class InterfaceEntryColour(InterfaceEntry):
    def __init__(self, entry, parent):
        super(InterfaceEntryColour, self).__init__(entry, parent)
        self.entryWidget = qtwidgets.ColourButton()
        self.entryWidget.colour = entry.value
        self.entryWidget.setToolTip(entry.help)
        self.entryWidget.colourChanged.connect(self.onChange)
        self._addBasicWidgets()

    def discardChanges(self):
        self.entryWidget.setColour(self.entry.value)
        super(InterfaceEntryColour, self).discardChanges()

    def resetToDefaultValue(self):
        defValue = self.entry.defaultValue
        if self.entry.editedValue != defValue:
            self.onChange(defValue)
            self.entryWidget.colour = defValue

    def onChange(self, colour):
        self.entry.editedValue = colour
        super(InterfaceEntryColour, self).onChange(colour)

class InterfaceEntryPen(InterfaceEntry):
    def __init__(self, entry, parent):
        super(InterfaceEntryPen, self).__init__(entry, parent)
        self.entryWidget = qtwidgets.PenButton()
        self.entryWidget.pen = entry.value
        self.entryWidget.setToolTip(entry.help)
        self.entryWidget.penChanged.connect(self.onChange)
        self._addBasicWidgets()

    def discardChanges(self):
        self.entryWidget.setPen(self.entry.value)
        super(InterfaceEntryPen, self).discardChanges()

    def resetToDefaultValue(self):
        defValue = self.entry.defaultValue
        if self.entry.editedValue != defValue:
            self.onChange(defValue)
            self.entryWidget.pen = defValue

    def onChange(self, pen):
        self.entry.editedValue = pen
        super(InterfaceEntryPen, self).onChange(pen)

class InterfaceEntryKeySequence(InterfaceEntry):
    def __init__(self, entry, parent):
        super(InterfaceEntryKeySequence, self).__init__(entry, parent)
        self.entryWidget = qtwidgets.KeySequenceButton()
        self.entryWidget.keySequence = entry.value
        self.entryWidget.setToolTip(entry.help)
        self.entryWidget.keySequenceChanged.connect(self.onChange)
        self._addBasicWidgets()

    def discardChanges(self):
        self.entryWidget.setKeySequence(self.entry.value)
        super(InterfaceEntryKeySequence, self).discardChanges()

    def resetToDefaultValue(self):
        defValue = self.entry.defaultValue
        if self.entry.editedValue != defValue:
            self.onChange(defValue)
            self.entryWidget.keySequence = defValue

    def onChange(self, keySequence):
        self.entry.editedValue = keySequence
        super(InterfaceEntryKeySequence, self).onChange(keySequence)

class InterfaceEntryCombobox(InterfaceEntry):
    def __init__(self, entry, parent):
        super(InterfaceEntryCombobox, self).__init__(entry, parent)
        self.entryWidget = QtGui.QComboBox()
        # optionList should be a list of lists where the first item is the key (data) and the second is the label
        for option in entry.optionList:
            self.entryWidget.addItem(option[1], option[0])
        self.setCurrentIndexByValue(entry.value)
        self.entryWidget.setToolTip(entry.help)
        self.entryWidget.currentIndexChanged.connect(self.onChange)
        self._addBasicWidgets()

    def setCurrentIndexByValue(self, value):
        index = self.entryWidget.findData(value)
        if index != -1:
            self.entryWidget.setCurrentIndex(index)

    def discardChanges(self):
        self.setCurrentIndexByValue(self.entry.value)
        super(InterfaceEntryCombobox, self).discardChanges()

    def resetToDefaultValue(self):
        defValue = self.entry.defaultValue
        if self.entry.editedValue != defValue:
            self.onChange(defValue)
            self.setCurrentIndexByValue(defValue)

    def onChange(self, index):
        if index != -1:
            self.entry.editedValue = self.entryWidget.itemData(index)

        super(InterfaceEntryCombobox, self).onChange(index)

# Factory: Return appropriate InterfaceEntry
# - Not exported; restricted to use within this module.
# - Could be replaced by a static mapping.
def interfaceEntryFactory(entry, parent):
    if entry.widgetHint == "string":
        return InterfaceEntryString(entry, parent)
    elif entry.widgetHint == "int":
        return InterfaceEntryInt(entry, parent)
    elif entry.widgetHint == "float":
        return InterfaceEntryFloat(entry, parent)
    elif entry.widgetHint == "checkbox":
        return InterfaceEntryCheckbox(entry, parent)
    elif entry.widgetHint == "colour":
        return InterfaceEntryColour(entry, parent)
    elif entry.widgetHint == "pen":
        return InterfaceEntryPen(entry, parent)
    elif entry.widgetHint == "keySequence":
        return InterfaceEntryKeySequence(entry, parent)
    elif entry.widgetHint == "combobox":
        return InterfaceEntryCombobox(entry, parent)
    else:
        raise RuntimeError("I don't understand widget hint '%s'" % (entry.widgetHint))

# Wrapper: Section
class InterfaceSection(QtGui.QGroupBox):
    def __init__(self, section, parent):
        super(InterfaceSection, self).__init__()
        self.section = section
        self.parent = parent
        self.modifiedEntries = []

        self.setTitle(section.label)

        self.layout = QtGui.QFormLayout()

        for entry in section.entries:
            lw = QtGui.QLabel(entry.label)
            lw.setMinimumWidth(200)
            lw.setWordWrap(True)

            self.layout.addRow(lw, interfaceEntryFactory(entry, self))

        self.setLayout(self.layout)

    def discardChanges(self):
        for entry in self.modifiedEntries:
            entry.discardChanges()

    def onChange(self, entry):
        self.modifiedEntries.append(entry)
        self.markAsChanged()
        # FIXME: This should be rolled into the InterfaceEntry types.
        self.layout.labelForField(entry).setText(entry.entry.label)
        self.parent.onChange(self)

    def markAsChanged(self):
        self.section.markAsChanged()

    def markAsUnchanged(self):
        self.section.markAsUnchanged()
        labelForField = self.layout.labelForField
        for entry in self.modifiedEntries:
            entry.markAsUnchanged()
            # FIXME: This should be rolled into the InterfaceEntry types.
            labelForField(entry).setText(entry.entry.label)
        self.modifiedEntries = []

# Wrapper: Category
class InterfaceCategory(QtGui.QScrollArea):
    def __init__(self, category, parent):
        super(InterfaceCategory, self).__init__()
        self.category = category
        self.parent = parent
        self.modifiedSections = []

        self.inner = QtGui.QWidget()
        self.layout = QtGui.QVBoxLayout()

        addWidget = self.layout.addWidget
        for section in category.sections:
            addWidget(InterfaceSection(section, self))

        self.layout.addStretch()
        self.inner.setLayout(self.layout)
        self.setWidget(self.inner)

        self.setWidgetResizable(True)

    def eventFilter(self, obj, event):
        if event.type() == QtCore.QEvent.Wheel:
            if event.delta() < 0:
                self.verticalScrollBar().triggerAction(QtGui.QAbstractSlider.SliderSingleStepAdd)
            else:
                self.verticalScrollBar().triggerAction(QtGui.QAbstractSlider.SliderSingleStepSub)
            return True

        return super(InterfaceCategory, self).eventFilter(obj, event)

    def discardChanges(self):
        for section in self.modifiedSections:
            section.discardChanges()

    def onChange(self, section):
        self.modifiedSections.append(section)
        self.markAsChanged()

    def markAsChanged(self):
        parent = self.parent
        self.category.markAsChanged()
        parent.setTabText(parent.indexOf(self), self.category.label)

    def markAsUnchanged(self):
        parent = self.parent
        self.category.markAsUnchanged()
        parent.setTabText(parent.indexOf(self), self.category.label)

        for section in self.modifiedSections:
            section.markAsUnchanged()

        self.modifiedSections = []

# Wrapper: Tabs
# TODO
