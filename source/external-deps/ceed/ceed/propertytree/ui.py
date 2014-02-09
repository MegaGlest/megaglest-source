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

"""Qt property tree widget supporting classes.

PropertyTreeItem -- Base item for all items.
PropertyTreeRow -- Pair of name and value items, manages it's child rows.
PropertyCategoryRow -- Special tree items placed at the root of the tree.
PropertyRow -- Tree row bound to a Property.
PropertyTreeItemDelegate -- Facilitates editing of the rows' values.
PropertyTreeView -- QTreeView with some modifications for better results.
PropertyTreeWidget -- The property tree widget.
"""

import fnmatch
import re

from .properties import Property
from .properties import StringWrapperProperty
from .editors import StringWrapperValidator

from PySide import QtGui
from PySide import QtCore

class PropertyTreeItem(QtGui.QStandardItem):
    """Base item for all items."""

    def __init__(self, propertyTreeRow):
        super(PropertyTreeItem, self).__init__()

        self.setSizeHint(QtCore.QSize(-1, 24))
        self.propertyTreeRow = propertyTreeRow

        self.finalised = False

    def finalise(self):
        self.propertyTreeRow = None
        self.finalised = True

    def bold(self):
        return self.font().bold()

    def setBold(self, value):
        font = self.font()
        if font.bold() == value:
            return

        font.setBold(value)
        self.setFont(font)

class PropertyTreeRow(object):
    """Pair of name and value items, manages it's child rows."""
    def __init__(self):
        self.nameItem = PropertyTreeItem(self)
        self.valueItem = PropertyTreeItem(self)
        self.editor = None
        self.finalised = False

        self.createChildRows()

    def finalise(self):
        #print("Finalising row with nameItem.text() = " + str(self.nameItem.text()))
        if not self.finalised:
            # Finalise children before clearing nameItem or we can't get them
            self.destroyChildRows()

            self.nameItem.finalise()
            self.nameItem = None
            self.valueItem.finalise()
            self.valueItem = None
            self.finalised = True

    def getName(self):
        """Return the name of the row (the text of the nameItem usually)."""
        return self.nameItem.text()

    def getParent(self):
        parentItem = self.nameItem.parent()
        if (parentItem is not None) and isinstance(parentItem, PropertyTreeItem):
            return parentItem.propertyTreeRow
        return None

    def getNamePath(self):
        """Return the path to this item, using its name and the names of its parents separated by a slash."""
        names = []
        parentRow = self
        while parentRow is not None:
            names.append(parentRow.getName())
            parentRow = parentRow.getParent()

        names.reverse()
        return "/".join(names)

    def rowFromPath(self, path):
        """Find and return the child row with the specified name-path,
        searching in children and their children too, or None if not found.

        Return self if the path is empty.

        See getNamePath().
        """
        if not path:
            return self

        # split path in two
        parts = path.split("/", 1)
        if len(parts) == 0:
            return self

        for row in self.childRows():
            if row.getName() == parts[0]:
                return row.rowFromPath(parts[1]) if len(parts) == 2 else row

        return None

    def childRows(self):
        """Get the child rows; self.nameItem must exist and be valid."""
        return [self.nameItem.child(childRowIndex).propertyTreeRow for childRowIndex in xrange(0, self.nameItem.rowCount())]

    def appendChildRow(self, row):
        self.nameItem.appendRow([row.nameItem, row.valueItem])

    def createChildRows(self):
        """Create and add child rows."""
        pass

    def destroyChildRows(self):
        for row in self.childRows():
            row.finalise()
        self.nameItem.setRowCount(0)

    def getState(self, view):
        """Return the state of the row and its children as a dictionary.

        The state includes the expanded state.

        Sample return value:
            { "expanded": True, "items": { "item1": <recurse>, "item2": <recurse> } }

        Note that the "items" key/value pair may be missing if the current row
        has no children.
        """
        state = dict()
        state["expanded"] = view.isExpanded(self.nameItem.index())
        if self.nameItem.hasChildren():
            items = dict()
            state["items"] = items
            for row in self.childRows():
                items[row.getName()] = row.getState(view)

        return state

    def setState(self, view, state):
        """Restore the state of the row and its children.

        See getState() for more information.
        """
        if state is None:
            return

        expanded = state.get("expanded", None)
        if expanded is not None:
            view.setExpanded(self.nameItem.index(), expanded)

        items = state.get("items", None)
        if items is not None and self.nameItem.hasChildren():
            for row in self.childRows():
                itemState = items.get(row.getName(), None)
                row.setState(view, itemState)

    def isModified(self):
        #pylint: disable-msg=R0201
        # "Method could be a function"
        # No, it couldn't, it's meant to be overriden but provides the default implementation.
        return False

    def setFilter(self, view, filterRegEx, hideUnmodified=False):
        """Filter children using the specified regular expression
        and return the count of children left visible.

        view -- The Tree View that manages the visibility state
        filterRegEx -- A regular expression that will be matched
                    against the names of the children. Only those
                    that match the regular expression will remain
                    visible.
        hideUnmodified -- If True, hide all children that have
                    their default values (haven't been modified).
        """
        i = 0
        visibleCount = 0
        while i < self.nameItem.rowCount():
            nameItem = self.nameItem.child(i, 0)

            matched = re.match(filterRegEx, nameItem.text()) is not None
            if hideUnmodified and not nameItem.propertyTreeRow.isModified():
                matched = False

            view.setRowHidden(nameItem.index().row(), nameItem.index().parent(), not matched)

            i += 1
            if matched:
                visibleCount += 1
        return visibleCount

class PropertyCategoryRow(PropertyTreeRow):
    """Special tree items placed at the root of the tree."""

    def __init__(self, propertyCategory):
        # set the category before super init because
        # we need it in createChildRows() which is
        # called by super init.
        self.category = propertyCategory

        super(PropertyCategoryRow, self).__init__()

        self.nameItem.setEditable(False)
        self.nameItem.setText(self.category.name)
        self.nameItem.setBold(True)

        self.valueItem.setEditable(False)

        # Change default colours
        palette = QtGui.QApplication.palette()
        self.nameItem.setForeground(palette.brush(QtGui.QPalette.Normal, QtGui.QPalette.BrightText))
        self.nameItem.setBackground(palette.brush(QtGui.QPalette.Normal, QtGui.QPalette.Dark))

    def createChildRows(self):
        for prop in self.category.properties.values():
            row = PropertyRow(prop)
            self.appendChildRow(row)

class PropertyRow(PropertyTreeRow):
    """Tree row bound to a Property."""

    def __init__(self, boundProperty):
        # set the property before super init because
        # we need it in createChildRows() which is
        # called by super init.
        self.property = boundProperty

        super(PropertyRow, self).__init__()

        self.nameItem.setEditable(False)
        self.nameItem.setText(self.property.name)
        if self.property.helpText:
            self.nameItem.setToolTip(self.property.helpText)

        self.valueItem.setEditable(not self.property.readOnly)
        self.valueItem.setText(self.property.valueToString())

        self.property.valueChanged.subscribe(self.cb_propertyValueChanged)
        self.property.componentsUpdate.subscribe(self.cb_propertyComponentsUpdate)

        self.updateStyle()

    def createChildRows(self):
        components = self.property.getComponents()
        if components:
            for component in components.values():
                row = PropertyRow(component)
                self.appendChildRow(row)

    def finalise(self):
        self.property.componentsUpdate.unsubscribe(self.cb_propertyComponentsUpdate)
        self.property.valueChanged.unsubscribe(self.cb_propertyValueChanged)

        super(PropertyRow, self).finalise()

    def isModified(self):
        return not self.property.hasDefaultValue()

    def cb_propertyComponentsUpdate(self, senderProperty, updateType):
        # destroy or recreate child rows as necessary
        if updateType == Property.ComponentsUpdateType.BeforeDestroy:
            self.destroyChildRows()
        elif updateType == Property.ComponentsUpdateType.AfterCreate:
            self.createChildRows()

    def cb_propertyValueChanged(self, senderProperty, dummyReason):
        self.valueItem.setText(self.property.valueToString())

        self.updateStyle()

    def updateStyle(self):
        """Update the style of the row,
        i.e. make the name bold if the property value is not the default.
        """
        self.nameItem.setBold(not self.property.hasDefaultValue())

class PropertyTreeItemDelegate(QtGui.QStyledItemDelegate):
    """Facilitates editing of the rows' values."""

    # Sample delegate
    # http://www.qtforum.org/post/81956/qtreeview-qstandarditem-and-singals.html#post81956

    def __init__(self, propertyTree, editorRegistry):
        super(PropertyTreeItemDelegate, self).__init__(propertyTree)

        self.registry = editorRegistry

        def cb_closeEditor(editWidget, dummyEndEditHint):
            editWidget.delegate_Editor.finalise()
            del editWidget.delegate_Editor
        self.closeEditor.connect(cb_closeEditor)

    def getPropertyRow(self, index):
        if index.isValid():
            return self.parent().model.itemFromIndex(index).propertyTreeRow
        return None

    def createEditor(self, parent, dummyOption, index):
        # get the PropertyRow from the index
        row = self.getPropertyRow(index)
        if row is None:
            return

        # try to create an editor for the property
        row.editor = self.registry.createEditor(row.property)
        if row.editor is None:
            # if no suitable editor was found and the property
            # supports editing it as a string, wrap it and fire
            # up the string editor.
            if row.property.isStringRepresentationEditable():
                # don't forget to finalise this property
                wrapperProperty = StringWrapperProperty(row.property)
                wrapperProperty.editorOptions["string"] = { "validator": StringWrapperValidator(row.property) }
                row.editor = self.registry.createEditor(wrapperProperty)
                if row.editor is None:
                    wrapperProperty.finalise()
                else:
                    # set the ownsProperty flag so the editor
                    # finalises it when it's finalised.
                    row.editor.ownsProperty = True
            if row.editor is None:
                return None

        # tell the newly created editor to create its widget
        editWidget = row.editor.createEditWidget(parent)
        # keep a reference to the editor inside the edit widget
        # so we can finalise the editor when the edit widget
        # is closed. See '__init__/cb_closeEditor'
        editWidget.delegate_Editor = row.editor

        return editWidget

    def setEditorData(self, dummyEditWidget, index):
        """Set the value of the editor to the property's value."""
        row = self.getPropertyRow(index)
        if row is None:
            return

        row.editor.setWidgetValueFromProperty()

    def setModelData(self, dummyEditWidget, dummyModel, index):
        """Set the value of the property to the editor's value."""
        row = self.getPropertyRow(index)
        if row is None:
            return

        row.editor.setPropertyValueFromWidget()

class PropertyTreeView(QtGui.QTreeView):
    """QTreeView with some modifications for better results."""

    def __init__(self, *args, **kwargs):
        QtGui.QTreeView.__init__(self, *args, **kwargs)

        # optional, set by 'setOptimalDefaults()'
        self.editTriggersForName = None
        self.editTriggersForValue = None

        self.setRequiredOptions()
        self.setOptimalDefaults()

    def setRequiredOptions(self):
        # We work with rows, not columns
        self.setAllColumnsShowFocus(True)
        self.setSelectionBehavior(QtGui.QAbstractItemView.SelectRows)
        # We need our items expandable
        self.setItemsExpandable(True)

    def setOptimalDefaults(self):
        #
        # Behavior
        #

        # We don't want 'SelectedClicked' on the name item - it's really annoyng when
        # you try to end the editing by clicking outside the editor but you hit the name
        # and then it starts to edit again.
        # See the re-implemented currentChanged() too.
        self.editTriggersForName = QtGui.QAbstractItemView.EditKeyPressed | QtGui.QAbstractItemView.DoubleClicked
        self.editTriggersForValue = self.editTriggersForName | QtGui.QAbstractItemView.SelectedClicked
        self.setEditTriggers(self.editTriggersForValue)
        # No tab key because we can already move through the rows using the up/down arrow
        # keys and since we usually show a ton of rows, getting tabs would make it practically
        # impossible for the user to move to the next control using tab.
        self.setTabKeyNavigation(False)

        # No branches on categories but allow expanding/collapsing
        # using double-click.
        # Note: We don't show branches on categories because:
        #    a) Not sure it's better
        #    b) I couldn't find a way to change the background color
        #    of the branch to match that of the category of the same
        #    row; changing the background color of the 'nameItem'
        #    does not change the background color of the branch.
        self.setRootIsDecorated(False)
        self.setExpandsOnDoubleClick(True)

        # No sorting by default because it's ugly until:
        # TODO: Do custom sorting that only sorts the items inside the
        # categories but not the categories themselves.
        self.setSortingEnabled(False)

        # No point in selecting more than one row unless we implement
        # something like "Copy" that copies the names and/or values
        # of the selected rows.
        self.setSelectionMode(QtGui.QAbstractItemView.SingleSelection)

        #
        # Visual
        #
        self.setAlternatingRowColors(True)
        #self.setIndentation(20)    # The default indentation is 20

        # Animation off because it plays when we do batch updates
        # even if setUpdatesEnabled(False) has been called.
        # (On Linux, at least)
        # See qtreeview.cpp, expandToDepth, it calls interruptDelayedItemsLayout()
        # which sounds promising.
        self.setAnimated(False)
        # Optimisation allowed because all of our rows have the same height
        self.setUniformRowHeights(True)

    def currentChanged(self, currentIndex, previousIndex):
        """Called when the current index changes."""
        # See comments in 'setOptimalDefaults()'
        if self.editTriggersForName is not None and self.editTriggersForValue is not None:
            if currentIndex.isValid():
                if (not previousIndex.isValid()) or (previousIndex.column() != currentIndex.column()):
                    self.setEditTriggers(self.editTriggersForName if currentIndex.column() == 0 else self.editTriggersForValue)

        super(PropertyTreeView, self).currentChanged(currentIndex, previousIndex)

    def drawRow(self, painter, option, index):
        """Draws grid lines.

        Yep, that's all it does.
        """
        super(PropertyTreeView, self).drawRow(painter, option, index)

        # get color for grid lines from the style
        returnData = QtGui.QStyleHintReturn()
        gridHint = self.style().styleHint(QtGui.QStyle.SH_Table_GridLineColor, option, self, returnData)
        gridColor = QtGui.QColor(gridHint & 0xFFFFFF)
        # setup a pen
        gridPen = QtGui.QPen(gridColor)
        # x coordinate to draw the vertical line (between the first and the second column)
        colX = self.columnViewportPosition(1) - 1
        # do not draw verticals on spanned rows (i.e. categories)
        drawVertical = not self.isFirstColumnSpanned(index.row(), index.parent())

        # save painter state so we can leave it as it was
        painter.save()
        painter.setPen(gridPen)
        painter.drawLine(option.rect.x(), option.rect.bottom(), option.rect.right(), option.rect.bottom())
        if drawVertical:
            painter.drawLine(colX, option.rect.y(), colX, option.rect.bottom())
        # aaaand restore
        painter.restore()

    def expandFromDepth(self, startingDepth):
        """Expand all items from the startingDepth and below."""
        model = self.model()

        def expand(item, currentDepth):
            if currentDepth >= startingDepth:
                self.setExpanded(item.index(), True)
            if item.hasChildren():
                i = 0
                while i < item.rowCount():
                    expand(item.child(i), currentDepth + 1)
                    i += 1

        i = 0
        while i < model.rowCount():
            item = model.item(i, 0)
            expand(item, 0)

            i += 1

class PropertyTreeItemModel(QtGui.QStandardItemModel):

    def buddy(self, index):
        """Point to the value item when the user tries to edit the name item."""
        # if on column 0 (the name item)
        if index.isValid() and index.column() == 0:
            # if it has a sibling (value item), get it
            valueIndex = index.sibling(index.row(), 1)
            # if the value item is valid...
            if valueIndex.isValid():
                flags = valueIndex.flags()
                # and it is editable but not disabled
                if (flags & QtCore.Qt.ItemIsEditable) and (flags & QtCore.Qt.ItemIsEnabled):
                    return valueIndex

        return super(PropertyTreeItemModel, self).buddy(index)

class PropertyTreeWidget(QtGui.QWidget):
    """The property tree widget.

    Sets up any options necessary.
    Provides easy access methods.
    """

    def __init__(self, parent = None):
        """Initialise the widget instance.

        'setupRegistry()' should be called next,
        before any property editing can happen.
        """
        super(PropertyTreeWidget, self).__init__(parent)

        # create model
        self.model = PropertyTreeItemModel()

        # finalise rows that are being removed
        def rowsAboutToBeRemoved(parentIndex, start, end):
            for i in xrange(start, end + 1):
                mi = self.model.index(i, 0, parentIndex)
                item = self.model.itemFromIndex(mi)
                if not item.finalised:
                    row = item.propertyTreeRow
                    row.finalise()
        self.model.rowsAboutToBeRemoved.connect(rowsAboutToBeRemoved)

        layout = QtGui.QVBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        self.setLayout(layout)

        self.view = PropertyTreeView(self)
        self.view.setObjectName("view")
        self.view.setModel(self.model)

        layout.addWidget(self.view)

        self.registry = None
        self.filterSettings = ("", False)
        self.previousPath = None

        self.clear()

    def setupRegistry(self, registry):
        """Setup the registry and the item delegate."""
        self.registry = registry
        itemDelegate = PropertyTreeItemDelegate(self, self.registry)
        self.view.setItemDelegate(itemDelegate)

    def clear(self):
        """Clear the tree.
        Does not clear the current filter.
        """
        self.model.clear()
        self.model.setHorizontalHeaderLabels(["Property", "Value"])

    def getCurrentPath(self):
        """Return the name path of the current row, or None."""
        index = self.view.currentIndex()
        if index.isValid():
            row = self.model.itemFromIndex(index).propertyTreeRow
            return row.getNamePath()
        return None

    def rowFromPath(self, path):
        """Find and return the row with the specified name-path, or None.

        See PropertyTreeRow.getNamePath()
        """
        if not path:
            return None

        # split path in two
        parts = path.split("/", 1)
        if len(parts) == 0:
            return None

        i = 0
        while i < self.model.rowCount():
            item = self.model.item(i, 0)
            categoryRow = item.propertyTreeRow
            if categoryRow.getName() == parts[0]:
                return categoryRow.rowFromPath(parts[1]) if len(parts) == 2 else categoryRow

            i += 1

        return None

    def setCurrentPath(self, path):
        """Set the current row by a name-path and return True
        on success, False on failure.
        """
        row = self.rowFromPath(path)
        if row is not None:
            self.view.setCurrentIndex(row.nameItem.index())
            return True
        return False

    def getRowsState(self):
        """Return the current state of the items.

        See PropertyTreeRow.getState().
        """
        state = dict()

        i = 0
        while i < self.model.rowCount():
            item = self.model.item(i, 0)
            categoryRow = item.propertyTreeRow
            state[categoryRow.getName()] = categoryRow.getState(self.view)

            i += 1

        return state

    def setRowsState(self, state, defaultCategoryExpansion=None):
        """Restore the state of the items to a saved state.

        defaultCategoryExpansion -- None, to leave categories that are not in
                                    the specified 'state' to their current
                                    expansion state; True to expand them;
                                    False to collapse them.

        Note: This does not call self.view.setUpdatesEnabled() before or
        after changing the items' state; it's left to the caller because
        this operation may be a part of another that handles updates
        already.

        See getRowsState() and PropertyTreeRow.getState().
        """

        defaultCategoryState = None
        if defaultCategoryExpansion is not None:
            defaultCategoryState = { "expanded": defaultCategoryExpansion }

        i = 0
        while i < self.model.rowCount():
            item = self.model.item(i, 0)
            categoryRow = item.propertyTreeRow
            catState = state.get(categoryRow.getName(), defaultCategoryState)
            categoryRow.setState(self.view, catState)

            i += 1

    def load(self, categories, resetState=False):
        """Clear tree and load the specified categories into it.

        categories -- Dictionary
        resetState -- False to try to maintain as much of the previous items' state
                    as possible, True to reset it.

        Note: This does not change the current filter.

        See getRowsState() and setRowsState().
        """

        # prevent flicker
        self.view.setUpdatesEnabled(False)

        # save current state
        itemsState = None
        currentPath = None
        if not resetState:
            itemsState = self.getRowsState()
            currentPath = self.getCurrentPath()
            if currentPath is not None:
                self.previousPath = currentPath

        # clear and setup
        self.clear()

        # add all categories
        for category in categories.values():
            self.appendCategory(category)

        # expand categories by default
        self.view.expandToDepth(0)
        # skip properties (depth == 1) but expand their components
        # so that when the user expands a property, its components
        # are expanded.
        self.view.expandFromDepth(2)

        # setup headers size
        self.view.header().setResizeMode(QtGui.QHeaderView.Stretch)
        #self.view.resizeColumnToContents(0)

        # apply the filter
        self.setFilter(self.filterSettings[0], self.filterSettings[1])

        # restore state
        if itemsState is not None:
            self.setRowsState(itemsState)
            self.setCurrentPath(self.previousPath)

        # reset updates
        self.view.setUpdatesEnabled(True)

    def appendCategory(self, category):
        row = PropertyCategoryRow(category)
        self.model.appendRow([row.nameItem, row.valueItem])
        # make the category name span two columns (all)
        self.view.setFirstColumnSpanned(self.model.rowCount() - 1, QtCore.QModelIndex(), True)

    def setFilter(self, filterText="", hideUnmodified=False):
        # we store the filter to be able to reapply it when our data changes
        self.filterSettings = (filterText, hideUnmodified)

        # we append star at the beginning and at the end by default (makes property filtering much more practical)
        filterText = "*" + filterText + "*"
        regex = re.compile(fnmatch.translate(filterText), re.IGNORECASE)

        i = 0
        while i < self.model.rowCount():
            categoryRow = self.model.item(i, 0).propertyTreeRow
            visibleItemsLeft = categoryRow.setFilter(self.view, regex, hideUnmodified)
            # if no items are visible in the category after applying the filter,
            # hide it, otherwise show it
            self.view.setRowHidden(i, QtCore.QModelIndex(), True if visibleItemsLeft == 0 else False)

            i += 1
