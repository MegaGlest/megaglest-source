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

from ceed import commands

from ceed.editors.layout import widgethelpers
import PyCEGUI

idbase = 1200

class MoveCommand(commands.UndoCommand):
    """This command simply moves given widgets from old positions to new
    """

    def __init__(self, visual, widgetPaths, oldPositions, newPositions):
        super(MoveCommand, self).__init__()

        self.visual = visual

        self.widgetPaths = widgetPaths
        self.oldPositions = oldPositions
        self.newPositions = newPositions

        self.refreshText()

    def refreshText(self):
        if len(self.widgetPaths) == 1:
            self.setText("Move '%s'" % (self.widgetPaths[0]))
        else:
            self.setText("Move %i widgets" % (len(self.widgetPaths)))

    def id(self):
        return idbase + 1

    def mergeWith(self, cmd):
        if self.widgetPaths == cmd.widgetPaths:
            # it is nearly impossible to do the delta guesswork right, the parent might get resized
            # etc, it might be possible in this exact scenario (no resizes) but in the generic
            # one it's a pain and can't be done consistently, so I don't even try and just merge if
            # the paths match
            self.newPositions = cmd.newPositions

            return True

            #for widgetPath in self.widgetPaths:
            #    delta = self.newPositions[widgetPath] - self.oldPositions[widgetPath]

        return False

    def undo(self):
        super(MoveCommand, self).undo()

        for widgetPath in self.widgetPaths:
            widgetManipulator = self.visual.scene.getManipulatorByPath(widgetPath)
            widgetManipulator.widget.setPosition(self.oldPositions[widgetPath])
            widgetManipulator.updateFromWidget()
            # in case the pixel position didn't change but the absolute and negative components changed and canceled each other out
            widgetManipulator.update()

            widgetManipulator.triggerPropertyManagerCallback({"Position", "Area"})

    def redo(self):
        for widgetPath in self.widgetPaths:
            widgetManipulator = self.visual.scene.getManipulatorByPath(widgetPath)
            widgetManipulator.widget.setPosition(self.newPositions[widgetPath])
            widgetManipulator.updateFromWidget()
            # in case the pixel position didn't change but the absolute and negative components changed and canceled each other out
            widgetManipulator.update()

            widgetManipulator.triggerPropertyManagerCallback({"Position", "Area"})

        super(MoveCommand, self).redo()

class ResizeCommand(commands.UndoCommand):
    """This command resizes given widgets from old positions and old sizes to new
    """

    def __init__(self, visual, widgetPaths, oldPositions, oldSizes, newPositions, newSizes):
        super(ResizeCommand, self).__init__()

        self.visual = visual

        self.widgetPaths = widgetPaths
        self.oldPositions = oldPositions
        self.oldSizes = oldSizes
        self.newPositions = newPositions
        self.newSizes = newSizes

        self.refreshText()

    def refreshText(self):
        if len(self.widgetPaths) == 1:
            self.setText("Resize '%s'" % (self.widgetPaths[0]))
        else:
            self.setText("Resize %i widgets" % (len(self.widgetPaths)))

    def id(self):
        return idbase + 2

    def mergeWith(self, cmd):
        if self.widgetPaths == cmd.widgetPaths:
            # it is nearly impossible to do the delta guesswork right, the parent might get resized
            # etc, so I don't even try and just merge if the paths match
            self.newPositions = cmd.newPositions
            self.newSizes = cmd.newSizes

            return True

        return False

    def undo(self):
        super(ResizeCommand, self).undo()

        for widgetPath in self.widgetPaths:
            widgetManipulator = self.visual.scene.getManipulatorByPath(widgetPath)
            widgetManipulator.widget.setPosition(self.oldPositions[widgetPath])
            widgetManipulator.widget.setSize(self.oldSizes[widgetPath])
            widgetManipulator.updateFromWidget()
            # in case the pixel size didn't change but the absolute and negative sizes changed and canceled each other out
            widgetManipulator.update()

            widgetManipulator.triggerPropertyManagerCallback({"Size", "Position", "Area"})

    def redo(self):
        for widgetPath in self.widgetPaths:
            widgetManipulator = self.visual.scene.getManipulatorByPath(widgetPath)
            widgetManipulator.widget.setPosition(self.newPositions[widgetPath])
            widgetManipulator.widget.setSize(self.newSizes[widgetPath])
            widgetManipulator.updateFromWidget()
            # in case the pixel size didn't change but the absolute and negative sizes changed and canceled each other out
            widgetManipulator.update()

            widgetManipulator.triggerPropertyManagerCallback({"Size", "Position", "Area"})

        super(ResizeCommand, self).redo()

class DeleteCommand(commands.UndoCommand):
    """This command deletes given widgets"""

    def __init__(self, visual, widgetPaths):
        super(DeleteCommand, self).__init__()

        self.visual = visual

        self.widgetPaths = widgetPaths
        self.widgetData = {}

        # we have to add all the child widgets of all widgets we are deleting
        for widgetPath in self.widgetPaths:
            manipulator = self.visual.scene.getManipulatorByPath(widgetPath)
            dependencies = manipulator.getAllDescendantManipulators()

            for dependency in dependencies:
                depencencyNamePath = dependency.widget.getNamePath()
                if depencencyNamePath not in self.widgetPaths:
                    self.widgetPaths.append(depencencyNamePath)

        # now we have to sort them in a way that ensures the most depending widgets come first
        # (the most deeply nested widgets get deleted first before their ancestors get deleted)
        class ManipulatorDependencyKey(object):
            def __init__(self, visual, path):
                self.visual = visual

                self.path = path
                self.manipulator = self.visual.scene.getManipulatorByPath(path)

            def __lt__(self, otherKey):
                # if this is the ancestor of other manipulator, it comes after it
                if self.manipulator.widget.isAncestor(otherKey.manipulator.widget):
                    return True
                # vice versa
                if otherKey.manipulator.widget.isAncestor(self.manipulator.widget):
                    return False

                # otherwise, we don't care but lets define a precise order
                return self.path < otherKey.path

        self.widgetPaths = sorted(self.widgetPaths, key = lambda path: ManipulatorDependencyKey(self.visual, path))

        # we have to store everything about these widgets before we destroy them,
        # we want to be able to restore if user decides to undo
        for widgetPath in self.widgetPaths:
            # serialiseChildren is False because we have already included all the children and they are handled separately
            self.widgetData[widgetPath] = widgethelpers.SerialisationData(self.visual, self.visual.scene.getManipulatorByPath(widgetPath).widget,
                                                                          serialiseChildren = False)

        self.refreshText()

    def refreshText(self):
        if len(self.widgetPaths) == 1:
            self.setText("Delete '%s'" % (self.widgetPaths[0]))
        else:
            self.setText("Delete %i widgets" % (len(self.widgetPaths)))

    def id(self):
        return idbase + 3

    def mergeWith(self, cmd):
        # we never merge deletes
        return False

    def undo(self):
        super(DeleteCommand, self).undo()

        manipulators = []

        # we have to undo in reverse to ensure widgets have their (potential) dependencies in place when they
        # are constructed
        for widgetPath in reversed(self.widgetPaths):
            data = self.widgetData[widgetPath]
            result = data.reconstruct(self.visual.scene.rootManipulator)

            manipulators.append(result)

        self.visual.notifyWidgetManipulatorsAdded(manipulators)

    def redo(self):
        for widgetPath in self.widgetPaths:
            manipulator = self.visual.scene.getManipulatorByPath(widgetPath)
            manipulator.detach(destroyWidget = True)

        self.visual.notifyWidgetManipulatorsRemoved(self.widgetPaths)

        super(DeleteCommand, self).redo()

class CreateCommand(commands.UndoCommand):
    """This command creates one widget"""

    def __init__(self, visual, parentWidgetPath, widgetType, widgetName):
        super(CreateCommand, self).__init__()

        self.visual = visual

        self.parentWidgetPath = parentWidgetPath
        self.widgetType = widgetType
        self.widgetName = widgetName

        self.refreshText()

    def refreshText(self):
        self.setText("Create '%s' of type '%s'" % (self.widgetName, self.widgetType))

    def id(self):
        return idbase + 4

    def mergeWith(self, cmd):
        # we never merge creates
        return False

    def undo(self):
        super(CreateCommand, self).undo()

        manipulator = self.visual.scene.getManipulatorByPath(self.parentWidgetPath + "/" + self.widgetName if self.parentWidgetPath != "" else self.widgetName)
        manipulator.detach(destroyWidget = True)

        self.visual.hierarchyDockWidget.refresh()

    def redo(self):
        data = widgethelpers.SerialisationData(self.visual)

        data.name = self.widgetName
        data.type = self.widgetType
        data.parentPath = self.parentWidgetPath

        result = data.reconstruct(self.visual.scene.rootManipulator)
        # if the size is 0x0, the widget will be hard to deal with, lets fix that in that case
        if result.widget.getSize() == PyCEGUI.USize(PyCEGUI.UDim(0, 0), PyCEGUI.UDim(0, 0)):
            result.widget.setSize(PyCEGUI.USize(PyCEGUI.UDim(0, 50), PyCEGUI.UDim(0, 50)))

        result.updateFromWidget()
        # ensure this isn't obscured by it's parent
        result.moveToFront()

        self.visual.hierarchyDockWidget.refresh()

        super(CreateCommand, self).redo()

class PropertyEditCommand(commands.UndoCommand):
    """This command resizes given widgets from old positions and old sizes to new
    """

    def __init__(self, visual, propertyName, widgetPaths, oldValues, newValue, ignoreNextPropertyManagerCallback=False):
        super(PropertyEditCommand, self).__init__()

        self.visual = visual

        self.propertyName = propertyName
        self.widgetPaths = widgetPaths
        self.oldValues = oldValues
        self.newValue = newValue

        self.refreshText()

        self.ignoreNextPropertyManagerCallback = ignoreNextPropertyManagerCallback

    def refreshText(self):
        if len(self.widgetPaths) == 1:
            self.setText("Change '%s' in '%s'" % (self.propertyName, self.widgetPaths[0]))
        else:
            self.setText("Change '%s' in %i widgets" % (self.propertyName, len(self.widgetPaths)))

    def id(self):
        return idbase + 5

    def mergeWith(self, cmd):
        if self.widgetPaths == cmd.widgetPaths and self.propertyName == cmd.propertyName:
            self.newValue = cmd.newValue

            return True

        return False

    def notifyPropertyManager(self, widgetManipulator, ignoreTarget):
        if not ignoreTarget:
            widgetManipulator.triggerPropertyManagerCallback({self.propertyName})
        # some properties are related to others so that
        # when one changes, the others change too.
        # the following ensures that notifications are sent
        # about the related properties as well.
        related = None
        if self.propertyName == "Size":
            related = set([ "Area" ])
        elif self.propertyName == "Area":
            related = { "Position", "Size" }
        elif self.propertyName == "Position":
            related = set([ "Area" ])

        if related is not None:
            widgetManipulator.triggerPropertyManagerCallback(related)

    def undo(self):
        super(PropertyEditCommand, self).undo()

        for widgetPath in self.widgetPaths:
            widgetManipulator = self.visual.scene.getManipulatorByPath(widgetPath)
            widgetManipulator.widget.setProperty(self.propertyName, self.oldValues[widgetPath])
            widgetManipulator.updateFromWidget()

            self.notifyPropertyManager(widgetManipulator, self.ignoreNextPropertyManagerCallback)
        self.ignoreNextPropertyManagerCallback = False

        # make sure to redraw the scene so the changes are visible
        self.visual.scene.update()

    def redo(self):
        for widgetPath in self.widgetPaths:
            widgetManipulator = self.visual.scene.getManipulatorByPath(widgetPath)
            widgetManipulator.widget.setProperty(self.propertyName, self.newValue)
            widgetManipulator.updateFromWidget()

            self.notifyPropertyManager(widgetManipulator, self.ignoreNextPropertyManagerCallback)
        self.ignoreNextPropertyManagerCallback = False

        # make sure to redraw the scene so the changes are visible
        self.visual.scene.update()

        super(PropertyEditCommand, self).redo()

class HorizontalAlignCommand(commands.UndoCommand):
    """This command aligns selected widgets accordingly
    """

    def __init__(self, visual, widgetPaths, oldAlignments, newAlignment):
        super(HorizontalAlignCommand, self).__init__()

        self.visual = visual

        self.widgetPaths = widgetPaths
        self.oldAlignments = oldAlignments
        self.newAlignment = newAlignment

        self.refreshText()

    def refreshText(self):
        alignStr = ""
        if self.newAlignment == PyCEGUI.HA_LEFT:
            alignStr = "left"
        elif self.newAlignment == PyCEGUI.HA_CENTRE:
            alignStr = "centre"
        elif self.newAlignment == PyCEGUI.HA_RIGHT:
            alignStr = "right"
        else:
            raise RuntimeError("Unknown horizontal alignment")

        if len(self.widgetPaths) == 1:
            self.setText("Horizontally align '%s' %s" % (self.widgetPaths[0], alignStr))
        else:
            self.setText("Horizontally align %i widgets %s" % (len(self.widgetPaths), alignStr))

    def id(self):
        return idbase + 6

    def mergeWith(self, cmd):
        if self.widgetPaths == cmd.widgetPaths:
            self.newAlignment = cmd.newAlignment
            self.refreshText()

            return True

        return False

    def undo(self):
        super(HorizontalAlignCommand, self).undo()

        for widgetPath in self.widgetPaths:
            widgetManipulator = self.visual.scene.getManipulatorByPath(widgetPath)
            widgetManipulator.widget.setHorizontalAlignment(self.oldAlignments[widgetPath])
            widgetManipulator.updateFromWidget()

            widgetManipulator.triggerPropertyManagerCallback(set(["HorizontalAlignment"]))

    def redo(self):
        for widgetPath in self.widgetPaths:
            widgetManipulator = self.visual.scene.getManipulatorByPath(widgetPath)
            widgetManipulator.widget.setHorizontalAlignment(self.newAlignment)
            widgetManipulator.updateFromWidget()

            widgetManipulator.triggerPropertyManagerCallback(set(["HorizontalAlignment"]))

        super(HorizontalAlignCommand, self).redo()

class VerticalAlignCommand(commands.UndoCommand):
    """This command aligns selected widgets accordingly
    """

    def __init__(self, visual, widgetPaths, oldAlignments, newAlignment):
        super(VerticalAlignCommand, self).__init__()

        self.visual = visual

        self.widgetPaths = widgetPaths
        self.oldAlignments = oldAlignments
        self.newAlignment = newAlignment

        self.refreshText()

    def refreshText(self):
        alignStr = ""
        if self.newAlignment == PyCEGUI.VA_TOP:
            alignStr = "top"
        elif self.newAlignment == PyCEGUI.VA_CENTRE:
            alignStr = "centre"
        elif self.newAlignment == PyCEGUI.VA_BOTTOM:
            alignStr = "bottom"
        else:
            raise RuntimeError("Unknown vertical alignment")

        if len(self.widgetPaths) == 1:
            self.setText("Vertically align '%s' %s" % (self.widgetPaths[0], alignStr))
        else:
            self.setText("Vertically align %i widgets %s" % (len(self.widgetPaths), alignStr))

    def id(self):
        return idbase + 7

    def mergeWith(self, cmd):
        if self.widgetPaths == cmd.widgetPaths:
            self.newAlignment = cmd.newAlignment
            self.refreshText()

            return True

        return False

    def undo(self):
        super(VerticalAlignCommand, self).undo()

        for widgetPath in self.widgetPaths:
            widgetManipulator = self.visual.scene.getManipulatorByPath(widgetPath)
            widgetManipulator.widget.setVerticalAlignment(self.oldAlignments[widgetPath])
            widgetManipulator.updateFromWidget()

            widgetManipulator.triggerPropertyManagerCallback(set(["VerticalAlignment"]))

    def redo(self):
        for widgetPath in self.widgetPaths:
            widgetManipulator = self.visual.scene.getManipulatorByPath(widgetPath)
            widgetManipulator.widget.setVerticalAlignment(self.newAlignment)
            widgetManipulator.updateFromWidget()

            widgetManipulator.triggerPropertyManagerCallback(set(["VerticalAlignment"]))

        super(VerticalAlignCommand, self).redo()

class ReparentCommand(commands.UndoCommand):
    """This command changes parent of given windows
    """

    def __init__(self, visual, oldWidgetPaths, newWidgetPaths):
        super(ReparentCommand, self).__init__()

        self.visual = visual

        self.oldWidgetPaths = oldWidgetPaths
        self.newWidgetPaths = newWidgetPaths

        self.refreshText()

    def refreshText(self):
        if len(self.oldWidgetPaths) == 1:
            self.setText("Reparent '%s' to '%s'" % (self.oldWidgetPaths[0], self.newWidgetPaths[0]))
        else:
            self.setText("Reparent %i widgets" % (len(self.oldWidgetPaths)))

    def id(self):
        return idbase + 8

    def mergeWith(self, cmd):
        if self.newWidgetPaths == cmd.oldWidgetPaths:
            self.newWidgetPaths = cmd.newWidgetPaths
            self.refreshText()

            return True

        return False

    def undo(self):
        super(ReparentCommand, self).undo()

        self.visual.scene.clearSelection()
        self.visual.hierarchyDockWidget.treeView.clearSelection()

        i = 0
        while i < len(self.newWidgetPaths):
            widgetPath = self.newWidgetPaths[i]
            oldWidgetPath = self.oldWidgetPaths[i]
            newWidgetName = widgetPath[widgetPath.rfind("/") + 1:]
            oldWidgetName = oldWidgetPath[oldWidgetPath.rfind("/") + 1:]

            widgetManipulator = self.visual.scene.getManipulatorByPath(widgetPath)
            oldParentPath = oldWidgetPath[0:oldWidgetPath.rfind("/")]
            oldParentManipulator = self.visual.scene.getManipulatorByPath(oldParentPath)

            # remove it from the current CEGUI parent widget
            ceguiParentWidget = widgetManipulator.widget.getParent()
            if ceguiParentWidget is not None:
                ceguiParentWidget.removeChild(widgetManipulator.widget)

            # rename it if necessary
            if oldWidgetName != newWidgetName:
                widgetManipulator.widget.setProperty("Name", oldWidgetName)

            # add it to the old CEGUI parent widget
            ceguiOldParentWidget = oldParentManipulator.widget
            ceguiOldParentWidget.addChild(widgetManipulator.widget)

            # and sort out the manipulators
            widgetManipulator.setParentItem(oldParentManipulator)

            # update sizes since relative sizes can alter these when the parent's size changes
            widgetManipulator.updateFromWidget()

            i += 1

        self.visual.hierarchyDockWidget.refresh()

    def redo(self):
        self.visual.scene.clearSelection()
        self.visual.hierarchyDockWidget.treeView.clearSelection()

        i = 0
        while i < len(self.oldWidgetPaths):
            widgetPath = self.oldWidgetPaths[i]
            newWidgetPath = self.newWidgetPaths[i]
            oldWidgetName = widgetPath[widgetPath.rfind("/") + 1:]
            newWidgetName = newWidgetPath[newWidgetPath.rfind("/") + 1:]

            widgetManipulator = self.visual.scene.getManipulatorByPath(widgetPath)
            newParentPath = newWidgetPath[0:newWidgetPath.rfind("/")]
            newParentManipulator = self.visual.scene.getManipulatorByPath(newParentPath)

            # remove it from the current CEGUI parent widget
            ceguiParentWidget = widgetManipulator.widget.getParent()
            if ceguiParentWidget is not None:
                ceguiParentWidget.removeChild(widgetManipulator.widget)

            # rename it if necessary
            if oldWidgetName != newWidgetName:
                widgetManipulator.widget.setProperty("Name", newWidgetName)

            # add it to the new CEGUI parent widget
            ceguiNewParentWidget = newParentManipulator.widget
            ceguiNewParentWidget.addChild(widgetManipulator.widget)

            # and sort out the manipulators
            widgetManipulator.setParentItem(newParentManipulator)

            # update sizes since relative sizes can alter these when the parent's size changes
            widgetManipulator.updateFromWidget()

            i += 1

        self.visual.hierarchyDockWidget.refresh()
        super(ReparentCommand, self).redo()

class PasteCommand(commands.UndoCommand):
    """This command pastes clipboard data to the given widget
    """

    def __init__(self, visual, clipboardData, targetWidgetPath):
        super(PasteCommand, self).__init__()

        self.visual = visual

        self.clipboardData = clipboardData
        self.targetWidgetPath = targetWidgetPath

        self.refreshText()

    def refreshText(self):
        if len(self.clipboardData) == 1:
            self.setText("Paste '%s' hierarchy to '%s'" % (self.clipboardData[0].name, self.targetWidgetPath))
        else:
            self.setText("Paste %i hierarchies to '%s'" % (len(self.clipboardData), self.targetWidgetPath))

    def id(self):
        return idbase + 9

    def mergeWith(self, cmd):
        # we never merge paste commands
        return False

    def undo(self):
        super(PasteCommand, self).undo()

        widgetPaths = []
        for serialisationData in self.clipboardData:
            widgetPath = serialisationData.parentPath + "/" + serialisationData.name
            widgetPaths.append(widgetPath)

            manipulator = self.visual.scene.getManipulatorByPath(widgetPath)
            wasRootWidget = manipulator.widget.getParent() is None

            manipulator.detach(destroyWidget = True)

            if wasRootWidget:
                # this was a root widget being deleted, handle this accordingly
                self.visual.setRootWidgetManipulator(None)

        self.visual.notifyWidgetManipulatorsRemoved(widgetPaths)

    def redo(self):
        targetManipulator = self.visual.scene.getManipulatorByPath(self.targetWidgetPath)

        for serialisationData in self.clipboardData:
            # make sure the name is unique and we will be able to paste smoothly
            serialisationData.name = targetManipulator.getUniqueChildWidgetName(serialisationData.name)
            serialisationData.setParentPath(self.targetWidgetPath)

            serialisationData.reconstruct(self.visual.scene.rootManipulator)

        self.visual.hierarchyDockWidget.refresh()

        super(PasteCommand, self).redo()

class NormaliseSizeCommand(ResizeCommand):
    def __init__(self, visual, widgetPaths, oldPositions, oldSizes):
        newSizes = {}
        for widgetPath, oldSize in oldSizes.iteritems():
            newSizes[widgetPath] = self.normaliseSize(widgetPath, oldSize)

        # we use oldPositions as newPositions because this command never changes positions of anything
        super(NormaliseSizeCommand, self).__init__(visual, widgetPaths, oldPositions, oldSizes, oldPositions, newSizes)

        self.refreshText()

    def normaliseSize(self, widgetPath, size):
        raise NotImplementedError("Each subclass of NormaliseSizeCommand must implement the normaliseSize method")

    def refreshText(self):
        raise NotImplementedError("Each subclass of NormaliseSizeCommand must implement the refreshText method")

    def id(self):
        raise NotImplementedError("Each subclass of NormaliseSizeCommand must implement the id method")

    def mergeWith(self, cmd):
        # we never merge size normalising commands
        return False

class NormaliseSizeToRelativeCommand(NormaliseSizeCommand):
    def __init__(self, visual, widgetPaths, oldPositions, oldSizes):
        # even though this will be set again in the ResizeCommand constructor we need to set it right now
        # because otherwise the normaliseSize will not work!
        self.visual = visual

        super(NormaliseSizeToRelativeCommand, self).__init__(visual, widgetPaths, oldPositions, oldSizes)

    def normaliseSize(self, widgetPath, size):
        manipulator = self.visual.scene.getManipulatorByPath(widgetPath)
        pixelSize = manipulator.widget.getPixelSize()
        baseSize = manipulator.getBaseSize()

        return PyCEGUI.USize(PyCEGUI.UDim(pixelSize.d_width / baseSize.d_width, 0),
                             PyCEGUI.UDim(pixelSize.d_height / baseSize.d_height, 0))

    def refreshText(self):
        if len(self.widgetPaths) == 1:
            self.setText("Normalise size of '%s' to relative" % (self.widgetPaths[0]))
        else:
            self.setText("Normalise size of %i widgets to relative" % (len(self.widgetPaths)))

    def id(self):
        return idbase + 10

class NormaliseSizeToAbsoluteCommand(NormaliseSizeCommand):
    def __init__(self, visual, widgetPaths, oldPositions, oldSizes):
        # even though this will be set again in the ResizeCommand constructor we need to set it right now
        # because otherwise the normaliseSize will not work!
        self.visual = visual

        super(NormaliseSizeToAbsoluteCommand, self).__init__(visual, widgetPaths, oldPositions, oldSizes)

    def normaliseSize(self, widgetPath, size):
        manipulator = self.visual.scene.getManipulatorByPath(widgetPath)
        pixelSize = manipulator.widget.getPixelSize()

        return PyCEGUI.USize(PyCEGUI.UDim(0, pixelSize.d_width),
                             PyCEGUI.UDim(0, pixelSize.d_height))

    def refreshText(self):
        if len(self.widgetPaths) == 1:
            self.setText("Normalise size of '%s' to absolute" % (self.widgetPaths[0]))
        else:
            self.setText("Normalise size of %i widgets to absolute" % (len(self.widgetPaths)))

    def id(self):
        return idbase + 11

class NormalisePositionCommand(MoveCommand):
    def __init__(self, visual, widgetPaths, oldPositions):
        newPositions = {}
        for widgetPath, oldPosition in oldPositions.iteritems():
            newPositions[widgetPath] = self.normalisePosition(widgetPath, oldPosition)

        super(NormalisePositionCommand, self).__init__(visual, widgetPaths, oldPositions, newPositions)

        self.refreshText()

    def normalisePosition(self, widgetPath, size):
        raise NotImplementedError("Each subclass of NormalisePositionCommand must implement the normalisePosition method")

    def refreshText(self):
        raise NotImplementedError("Each subclass of NormalisePositionCommand must implement the refreshText method")

    def id(self):
        raise NotImplementedError("Each subclass of NormalisePositionCommand must implement the id method")

    def mergeWith(self, cmd):
        # we never merge position normalising commands
        return False

class NormalisePositionToRelativeCommand(NormalisePositionCommand):
    def __init__(self, visual, widgetPaths, oldPositions):
        # even though this will be set again in the MoveCommand constructor we need to set it right now
        # because otherwise the normalisePosition method will not work!
        self.visual = visual

        super(NormalisePositionToRelativeCommand, self).__init__(visual, widgetPaths, oldPositions)

    def normalisePosition(self, widgetPath, size):
        manipulator = self.visual.scene.getManipulatorByPath(widgetPath)
        position = manipulator.widget.getPosition()
        baseSize = manipulator.getBaseSize()

        return PyCEGUI.UVector2(PyCEGUI.UDim((position.d_x.d_offset + position.d_x.d_scale * baseSize.d_width) / baseSize.d_width, 0),
                                PyCEGUI.UDim((position.d_y.d_offset + position.d_y.d_scale * baseSize.d_height) / baseSize.d_height, 0))

    def refreshText(self):
        if len(self.widgetPaths) == 1:
            self.setText("Normalise position of '%s' to relative" % (self.widgetPaths[0]))
        else:
            self.setText("Normalise position of %i widgets to relative" % (len(self.widgetPaths)))

    def id(self):
        return idbase + 12

class NormalisePositionToAbsoluteCommand(NormalisePositionCommand):
    def __init__(self, visual, widgetPaths, oldPositions):
        # even though this will be set again in the MoveCommand constructor we need to set it right now
        # because otherwise the normalisePosition method will not work!
        self.visual = visual

        super(NormalisePositionToAbsoluteCommand, self).__init__(visual, widgetPaths, oldPositions)

    def normalisePosition(self, widgetPath, size):
        manipulator = self.visual.scene.getManipulatorByPath(widgetPath)
        position = manipulator.widget.getPosition()
        baseSize = manipulator.getBaseSize()

        return PyCEGUI.UVector2(PyCEGUI.UDim(0, position.d_x.d_offset + position.d_x.d_scale * baseSize.d_width),
                                PyCEGUI.UDim(0, position.d_y.d_offset + position.d_y.d_scale * baseSize.d_height))

    def refreshText(self):
        if len(self.widgetPaths) == 1:
            self.setText("Normalise position of '%s' to absolute" % (self.widgetPaths[0]))
        else:
            self.setText("Normalise position of %i widgets to absolute" % (len(self.widgetPaths)))

    def id(self):
        return idbase + 13

class RenameCommand(commands.UndoCommand):
    """This command changes the name of the given widget
    """

    def __init__(self, visual, oldWidgetPath, newWidgetName):
        super(RenameCommand, self).__init__()

        self.visual = visual

        # NOTE: rfind returns -1 when '/' can't be found, so in case the widget
        #       is the root widget, -1 + 1 = 0 and the slice just returns
        #       full name of the widget

        self.oldWidgetPath = oldWidgetPath
        self.oldWidgetName = oldWidgetPath[oldWidgetPath.rfind("/") + 1:]

        self.newWidgetPath = oldWidgetPath[:oldWidgetPath.rfind("/") + 1] + newWidgetName
        self.newWidgetName = newWidgetName

        self.refreshText()

    def refreshText(self):
        self.setText("Rename '%s' to '%s'" % (self.oldWidgetName, self.newWidgetName))

    def id(self):
        return idbase + 14

    def mergeWith(self, cmd):
        # don't merge if the new rename command will simply revert to previous commands old name
        if self.newWidgetPath == cmd.oldWidgetPath and self.oldWidgetName != cmd.newWidgetName:
            self.newWidgetName = cmd.newWidgetName
            self.newWidgetPath = self.oldWidgetPath[0:self.oldWidgetPath.rfind("/") + 1] + self.newWidgetName

            self.refreshText()
            return True

        return False

    def undo(self):
        super(RenameCommand, self).undo()

        widgetManipulator = self.visual.scene.getManipulatorByPath(self.newWidgetPath)
        assert(hasattr(widgetManipulator, "treeItem"))
        assert(widgetManipulator.treeItem is not None)

        widgetManipulator.widget.setName(self.oldWidgetName)
        widgetManipulator.treeItem.setText(self.oldWidgetName)
        widgetManipulator.treeItem.refreshPathData()

        widgetManipulator.triggerPropertyManagerCallback({"Name", "NamePath"})


    def redo(self):
        widgetManipulator = self.visual.scene.getManipulatorByPath(self.oldWidgetPath)
        assert(hasattr(widgetManipulator, "treeItem"))
        assert(widgetManipulator.treeItem is not None)

        widgetManipulator.widget.setName(self.newWidgetName)
        widgetManipulator.treeItem.setText(self.newWidgetName)
        widgetManipulator.treeItem.refreshPathData()

        widgetManipulator.triggerPropertyManagerCallback({"Name", "NamePath"})

        super(RenameCommand, self).redo()
