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

from ceed import resizable
from ceed.cegui import qtgraphics
import PyCEGUI

# This module contains helping classes for CEGUI widget handling

class GraphicsScene(qtgraphics.GraphicsScene):
    """If you plan to use widget manipulators, use a scene inherited from this class.
    """

    def __init__(self, ceguiInstance):
        super(GraphicsScene, self).__init__(ceguiInstance)

class Manipulator(resizable.ResizableRectItem):
    """This is a rectangle that is synchronised with given CEGUI widget,
    it provides moving and resizing functionality
    """

    def __init__(self, parent, widget, recursive = True, skipAutoWidgets = False):
        """Constructor

        widget - CEGUI::Widget to wrap
        recursive - if true, even children of given widget are wrapped
        skipAutoWidgets - if true, auto widgets are skipped (only applicable if recursive is True)
        """

        super(Manipulator, self).__init__(parent)

        self.setFlags(QtGui.QGraphicsItem.ItemIsFocusable |
                      QtGui.QGraphicsItem.ItemIsSelectable |
                      QtGui.QGraphicsItem.ItemIsMovable |
                      QtGui.QGraphicsItem.ItemSendsGeometryChanges)

        self.widget = widget
        self.updateFromWidget()

        if recursive:
            self.createChildManipulators(True, skipAutoWidgets)

        self.preResizePos = None
        self.preResizeSize = None
        self.lastResizeNewPos = None
        self.lastResizeNewRect = None

        self.preMovePos = None
        self.lastMoveNewPos = None

    def getWidgetPath(self):
        return self.widget.getNamePath() if self.widget is not None else "<Unknown>"

    def createChildManipulator(self, childWidget, recursive = True, skipAutoWidgets = False):
        """Creates a child manipulator suitable for a child widget of manipulated widget

        recursive - recurse into children?
        skipAutoWidgets - if true, auto widgets will be skipped over

        This is there to allow overriding (if user subclasses the Manipulator, child manipulators are
        likely to be also subclassed
        """

        return Manipulator(self, childWidget, recursive, skipAutoWidgets)

    def createChildManipulators(self, recursive = True, skipAutoWidgets = False):
        """Creates manipulators for child widgets of widget manipulated by this manipulator

        recursive - recurse into children?
        skipAutoWidgets - if true, auto widgets will be skipped over
        """

        idx = 0
        while idx < self.widget.getChildCount():
            childWidget = self.widget.getChildAtIdx(idx)

            if not skipAutoWidgets or not childWidget.isAutoWindow():
                # note: we don't have to assign or attach the child manipulator here
                #       just passing parent to the constructor is enough
                self.createChildManipulator(childWidget, recursive, skipAutoWidgets)

            idx += 1

    def createMissingChildManipulators(self, recursive = True, skipAutoWidgets = False):
        """Goes through child widgets of the manipulated widget and creates manipulator
        for each missing one.

        recursive - recurse into children?
        skipAutoWidgets - if true, auto widgets will be skipped over
        """

        idx = 0
        while idx < self.widget.getChildCount():
            childWidget = self.widget.getChildAtIdx(idx)

            try:
                # try to find a manipulator for currently examined child widget
                self.getManipulatorByPath(childWidget.getName())

            except LookupError:
                if not skipAutoWidgets or not childWidget.isAutoWindow():
                    # note: we don't have to assign or attach the child manipulator here
                    #       just passing parent to the constructor is enough
                    childManipulator = self.createChildManipulator(childWidget, recursive, skipAutoWidgets)
                    if recursive:
                        childManipulator.createMissingChildManipulators(True, skipAutoWidgets)

            idx += 1

    def detach(self, detachWidget = True, destroyWidget = True, recursive = True):
        """Detaches itself from the GUI hierarchy and the manipulator hierarchy.

        detachWidget - should we detach the CEGUI widget as well?
        destroyWidget - should we destroy the CEGUI widget after it's detached?
        recursive - recurse into children?

        This method doesn't destroy this instance immediately but it will be destroyed automatically
        when nothing is referencing it.
        """

        # descend if recursive
        if recursive:
            self.detachChildManipulators(detachWidget, destroyWidget, True)

        # detach from the GUI hierarchy
        if detachWidget:
            parentWidget = self.widget.getParent()
            if parentWidget is not None:
                parentWidget.removeChild(self.widget)

        # detach from the parent manipulator
        self.scene().removeItem(self)

        if detachWidget and destroyWidget:
            PyCEGUI.WindowManager.getSingleton().destroyWindow(self.widget)
            self.widget = None

    def detachChildManipulators(self, detachWidget = True, destroyWidget = True, recursive = True):
        """Detaches all child manipulators

        detachWidget - should we detach the CEGUI widget as well?
        destroyWidget - should we destroy the CEGUI widget after it's detached?
        recursive - recurse into children?
        """

        for child in self.childItems():
            if isinstance(child, Manipulator):
                child.detach(detachWidget, destroyWidget, recursive)

    def getManipulatorByPath(self, widgetPath):
        """Retrieves a manipulator relative to this manipulator by given widget path

        Throws LookupError on failure.
        """

        path = widgetPath.split("/", 1)
        assert(len(path) >= 1)

        baseName = path[0]
        remainder = ""
        if len(path) == 2:
            remainder = path[1]

        for item in self.childItems():
            if isinstance(item, Manipulator):
                if item.widget.getName() == baseName:
                    if remainder == "":
                        return item

                    else:
                        return item.getManipulatorByPath(remainder)

        raise LookupError("Can't find widget manipulator of path '" + widgetPath + "'")

    def getChildManipulators(self):
        ret = []

        for child in self.childItems():
            if isinstance(child, Manipulator):
                ret.append(child)

        return ret

    def getAllDescendantManipulators(self):
        ret = []

        for child in self.childItems():
            if isinstance(child, Manipulator):
                ret.append(child)

                ret.extend(child.getAllDescendantManipulators())

        return ret

    def updateFromWidget(self):
        assert(self.widget is not None)

        unclippedOuterRect = self.widget.getUnclippedOuterRect().getFresh(True)
        pos = unclippedOuterRect.getPosition()
        size = unclippedOuterRect.getSize()

        parentWidget = self.widget.getParent()
        if parentWidget:
            parentUnclippedOuterRect = parentWidget.getUnclippedOuterRect().get()
            pos -= parentUnclippedOuterRect.getPosition()

        self.ignoreGeometryChanges = True
        self.setPos(QtCore.QPointF(pos.d_x, pos.d_y))
        self.setRect(QtCore.QRectF(0, 0, size.d_width, size.d_height))
        self.ignoreGeometryChanges = False

        for item in self.childItems():
            if not isinstance(item, Manipulator):
                continue

            item.updateFromWidget()

    def moveToFront(self):
        self.widget.moveToFront()

        parentItem = self.parentItem()
        if parentItem:
            for item in parentItem.childItems():
                if item == self:
                    continue

                # For some reason this is the opposite of what (IMO) it should be
                # which is self.stackBefore(item)
                #
                # Is Qt documentation flawed or something?!
                item.stackBefore(self)

            parentItem.moveToFront()

    def itemChange(self, change, value):
        if change == QtGui.QGraphicsItem.ItemSelectedHasChanged:
            if value:
                self.moveToFront()

        return super(Manipulator, self).itemChange(change, value)

    def notifyHandleSelected(self, handle):
        super(Manipulator, self).notifyHandleSelected(handle)

        self.moveToFront()

    def getMinSize(self):
        if self.widget:
            minPixelSize = PyCEGUI.CoordConverter.asAbsolute(self.widget.getMinSize(),
                                                             PyCEGUI.System.getSingleton().getRenderer().getDisplaySize())

            return QtCore.QSizeF(minPixelSize.d_width, minPixelSize.d_height)

    def getMaxSize(self):
        if self.widget:
            maxPixelSize = PyCEGUI.CoordConverter.asAbsolute(self.widget.getMaxSize(),
                                                             PyCEGUI.System.getSingleton().getRenderer().getDisplaySize())

            return QtCore.QSizeF(maxPixelSize.d_width, maxPixelSize.d_height)

    def getBaseSize(self):
        if self.widget.getParent() is not None and not self.widget.isNonClient():
            return self.widget.getParent().getUnclippedInnerRect().get().getSize()

        else:
            return self.widget.getParentPixelSize()

    def useAbsoluteCoordsForMove(self):
        return False

    def useAbsoluteCoordsForResize(self):
        return False

    def notifyResizeStarted(self):
        super(Manipulator, self).notifyResizeStarted()

        self.preResizePos = self.widget.getPosition()
        self.preResizeSize = self.widget.getSize()

        for item in self.childItems():
            if isinstance(item, Manipulator):
                item.setVisible(False)

    def notifyResizeProgress(self, newPos, newRect):
        super(Manipulator, self).notifyResizeProgress(newPos, newRect)

        # absolute pixel deltas
        pixelDeltaPos = newPos - self.resizeOldPos
        pixelDeltaSize = newRect.size() - self.resizeOldRect.size()

        deltaPos = None
        deltaSize = None

        if self.useAbsoluteCoordsForResize():
            deltaPos = PyCEGUI.UVector2(PyCEGUI.UDim(0, pixelDeltaPos.x()), PyCEGUI.UDim(0, pixelDeltaPos.y()))
            deltaSize = PyCEGUI.USize(PyCEGUI.UDim(0, pixelDeltaSize.width()), PyCEGUI.UDim(0, pixelDeltaSize.height()))

        else:
            baseSize = self.getBaseSize()

            deltaPos = PyCEGUI.UVector2(PyCEGUI.UDim(pixelDeltaPos.x() / baseSize.d_width, 0), PyCEGUI.UDim(pixelDeltaPos.y() / baseSize.d_height, 0))
            deltaSize = PyCEGUI.USize(PyCEGUI.UDim(pixelDeltaSize.width() / baseSize.d_width, 0), PyCEGUI.UDim(pixelDeltaSize.height() / baseSize.d_height, 0))

        # because the Qt manipulator is always top left aligned in the CEGUI sense,
        # we have to process the size to factor in alignments if they differ
        processedDeltaPos = PyCEGUI.UVector2()

        hAlignment = self.widget.getHorizontalAlignment()
        if hAlignment == PyCEGUI.HorizontalAlignment.HA_LEFT:
            processedDeltaPos.d_x = deltaPos.d_x
        elif hAlignment == PyCEGUI.HorizontalAlignment.HA_CENTRE:
            processedDeltaPos.d_x = deltaPos.d_x + PyCEGUI.UDim(0.5, 0.5) * deltaSize.d_width
        elif hAlignment == PyCEGUI.HorizontalAlignment.HA_RIGHT:
            processedDeltaPos.d_x = deltaPos.d_x + deltaSize.d_width
        else:
            assert(False)

        vAlignment = self.widget.getVerticalAlignment()
        if vAlignment == PyCEGUI.VerticalAlignment.VA_TOP:
            processedDeltaPos.d_y = deltaPos.d_y
        elif vAlignment == PyCEGUI.VerticalAlignment.VA_CENTRE:
            processedDeltaPos.d_y = deltaPos.d_y + PyCEGUI.UDim(0.5, 0.5) * deltaSize.d_height
        elif vAlignment == PyCEGUI.VerticalAlignment.VA_BOTTOM:
            processedDeltaPos.d_y = deltaPos.d_y + deltaSize.d_height
        else:
            assert(False)

        self.widget.setPosition(self.preResizePos + processedDeltaPos)
        self.widget.setSize(self.preResizeSize + deltaSize)

        self.lastResizeNewPos = newPos
        self.lastResizeNewRect = newRect

    def notifyResizeFinished(self, newPos, newRect):
        super(Manipulator, self).notifyResizeFinished(newPos, newRect)

        self.updateFromWidget()

        for item in self.childItems():
            if isinstance(item, Manipulator):
                item.updateFromWidget()
                item.setVisible(True)

        self.lastResizeNewPos = None
        self.lastResizeNewRect = None

    def notifyMoveStarted(self):
        super(Manipulator, self).notifyMoveStarted()

        self.preMovePos = self.widget.getPosition()

        for item in self.childItems():
            if isinstance(item, Manipulator):
                item.setVisible(False)

    def notifyMoveProgress(self, newPos):
        super(Manipulator, self).notifyMoveProgress(newPos)

        # absolute pixel deltas
        pixelDeltaPos = newPos - self.moveOldPos

        deltaPos = None
        if self.useAbsoluteCoordsForMove():
            deltaPos = PyCEGUI.UVector2(PyCEGUI.UDim(0, pixelDeltaPos.x()), PyCEGUI.UDim(0, pixelDeltaPos.y()))

        else:
            baseSize = self.getBaseSize()

            deltaPos = PyCEGUI.UVector2(PyCEGUI.UDim(pixelDeltaPos.x() / baseSize.d_width, 0), PyCEGUI.UDim(pixelDeltaPos.y() / baseSize.d_height, 0))

        self.widget.setPosition(self.preMovePos + deltaPos)

        self.lastMoveNewPos = newPos

    def notifyMoveFinished(self, newPos):
        super(Manipulator, self).notifyMoveFinished(newPos)

        self.updateFromWidget()

        for item in self.childItems():
            if isinstance(item, Manipulator):
                item.updateFromWidget()
                item.setVisible(True)

        self.lastMoveNewPos = None

    def boundingClipPath(self):
        """Retrieves clip path containing the bounding rectangle"""

        ret = QtGui.QPainterPath()
        ret.addRect(self.boundingRect())

        return ret

    def isAboveItem(self, item):
        # undecidable otherwise
        assert(item.scene() == self.scene())

        # FIXME: nasty nasty way to do this
        for i in self.scene().items():
            if i is self:
                return True
            if i is item:
                return False

        assert(False)

    def paintHorizontalGuides(self, baseSize, painter, option, widget):
        """Paints horizontal dimension guides - position X and width guides"""

        widgetPosition = self.widget.getPosition()
        widgetSize = self.widget.getSize()

        # x coordinate
        scaleXInPixels = PyCEGUI.CoordConverter.asAbsolute(PyCEGUI.UDim(widgetPosition.d_x.d_scale, 0), baseSize.d_width)
        offsetXInPixels = widgetPosition.d_x.d_offset

        # width
        scaleWidthInPixels = PyCEGUI.CoordConverter.asAbsolute(PyCEGUI.UDim(widgetSize.d_width.d_scale, 0), baseSize.d_width)
        offsetWidthInPixels = widgetSize.d_width.d_offset

        hAlignment = self.widget.getHorizontalAlignment()
        startXPoint = 0
        if hAlignment == PyCEGUI.HorizontalAlignment.HA_LEFT:
            startXPoint = (self.rect().topLeft() + self.rect().bottomLeft()) / 2
        elif hAlignment == PyCEGUI.HorizontalAlignment.HA_CENTRE:
            startXPoint = self.rect().center()
        elif hAlignment == PyCEGUI.HorizontalAlignment.HA_RIGHT:
            startXPoint = (self.rect().topRight() + self.rect().bottomRight()) / 2
        else:
            assert(False)

        midXPoint = startXPoint - QtCore.QPointF(offsetXInPixels, 0)
        endXPoint = midXPoint - QtCore.QPointF(scaleXInPixels, 0)
        xOffset = QtCore.QPointF(0, 1) if scaleXInPixels * offsetXInPixels < 0 else QtCore.QPointF(0, 0)

        pen = QtGui.QPen()
        # 0 means 1px size no matter the transformation
        pen.setWidth(0)
        pen.setColor(QtGui.QColor(0, 255, 0, 255))
        painter.setPen(pen)
        painter.drawLine(startXPoint, midXPoint)
        pen.setColor(QtGui.QColor(255, 0, 0, 255))
        painter.setPen(pen)
        painter.drawLine(midXPoint + xOffset, endXPoint + xOffset)

        vAlignment = self.widget.getVerticalAlignment()
        startWPoint = 0
        if vAlignment == PyCEGUI.VerticalAlignment.VA_TOP:
            startWPoint = self.rect().bottomLeft()
        elif vAlignment == PyCEGUI.VerticalAlignment.VA_CENTRE:
            startWPoint = self.rect().bottomLeft()
        elif vAlignment == PyCEGUI.VerticalAlignment.VA_BOTTOM:
            startWPoint = self.rect().topLeft()
        else:
            assert(False)

        midWPoint = startWPoint + QtCore.QPointF(scaleWidthInPixels, 0)
        endWPoint = midWPoint + QtCore.QPointF(offsetWidthInPixels, 0)
        # FIXME: epicly unreadable
        wOffset = QtCore.QPointF(0, -1 if vAlignment == PyCEGUI.VerticalAlignment.VA_BOTTOM else 1) if scaleWidthInPixels * offsetWidthInPixels < 0 else QtCore.QPointF(0, 0)

        pen = QtGui.QPen()
        # 0 means 1px size no matter the transformation
        pen.setWidth(0)
        pen.setColor(QtGui.QColor(255, 0, 0, 255))
        painter.setPen(pen)
        painter.drawLine(startWPoint, midWPoint)
        pen.setColor(QtGui.QColor(0, 255, 0, 255))
        painter.setPen(pen)
        painter.drawLine(midWPoint + wOffset, endWPoint + wOffset)

    def paintVerticalGuides(self, baseSize, painter, option, widget):
        """Paints vertical dimension guides - position Y and height guides"""

        widgetPosition = self.widget.getPosition()
        widgetSize = self.widget.getSize()

        # y coordinate
        scaleYInPixels = PyCEGUI.CoordConverter.asAbsolute(PyCEGUI.UDim(widgetPosition.d_y.d_scale, 0), baseSize.d_height)
        offsetYInPixels = widgetPosition.d_y.d_offset

        # height
        scaleHeightInPixels = PyCEGUI.CoordConverter.asAbsolute(PyCEGUI.UDim(widgetSize.d_height.d_scale, 0), baseSize.d_height)
        offsetHeightInPixels = widgetSize.d_height.d_offset

        vAlignment = self.widget.getVerticalAlignment()
        startYPoint = 0
        if vAlignment == PyCEGUI.VerticalAlignment.VA_TOP:
            startYPoint = (self.rect().topLeft() + self.rect().topRight()) / 2
        elif vAlignment == PyCEGUI.VerticalAlignment.VA_CENTRE:
            startYPoint = self.rect().center()
        elif vAlignment == PyCEGUI.VerticalAlignment.VA_BOTTOM:
            startYPoint = (self.rect().bottomLeft() + self.rect().bottomRight()) / 2
        else:
            assert(False)

        midYPoint = startYPoint - QtCore.QPointF(0, offsetYInPixels)
        endYPoint = midYPoint - QtCore.QPointF(0, scaleYInPixels)
        yOffset = QtCore.QPointF(1, 0) if scaleYInPixels * offsetYInPixels < 0 else QtCore.QPointF(0, 0)

        pen = QtGui.QPen()
        # 0 means 1px size no matter the transformation
        pen.setWidth(0)
        pen.setColor(QtGui.QColor(0, 255, 0, 255))
        painter.setPen(pen)
        painter.drawLine(startYPoint, midYPoint)
        pen.setColor(QtGui.QColor(255, 0, 0, 255))
        painter.setPen(pen)
        painter.drawLine(midYPoint + yOffset, endYPoint + yOffset)

        hAlignment = self.widget.getHorizontalAlignment()
        startHPoint = 0
        if hAlignment == PyCEGUI.HorizontalAlignment.HA_LEFT:
            startHPoint = self.rect().topRight()
        elif hAlignment == PyCEGUI.HorizontalAlignment.HA_CENTRE:
            startHPoint = self.rect().topRight()
        elif hAlignment == PyCEGUI.HorizontalAlignment.HA_RIGHT:
            startHPoint = self.rect().topLeft()
        else:
            assert(False)

        midHPoint = startHPoint + QtCore.QPointF(0, scaleHeightInPixels)
        endHPoint = midHPoint + QtCore.QPointF(0, offsetHeightInPixels)
        # FIXME: epicly unreadable
        hOffset = QtCore.QPointF(-1 if hAlignment == PyCEGUI.HorizontalAlignment.HA_RIGHT else 1, 0) if scaleHeightInPixels * offsetHeightInPixels < 0 else QtCore.QPointF(0, 0)

        pen = QtGui.QPen()
        # 0 means 1px size no matter the transformation
        pen.setWidth(0)
        pen.setColor(QtGui.QColor(255, 0, 0, 255))
        painter.setPen(pen)
        painter.drawLine(startHPoint, midHPoint)
        pen.setColor(QtGui.QColor(0, 255, 0, 255))
        painter.setPen(pen)
        painter.drawLine(midHPoint + hOffset, endHPoint + hOffset)

    def getPreventManipulatorOverlap(self):
        """Returns whether the painting code should strive to prevent manipulator overlap (crossing outlines and possibly other things)
        Override to change the behavior
        """

        return False

    def paint(self, painter, option, widget):
        painter.save()

        if self.getPreventManipulatorOverlap():
            # NOTE: This is just an option because it's very performance intensive, most people editing big layouts will
            #       want this disabled. But it makes editing nicer and fancier :-)

            # We are drawing the outlines after CEGUI has already been rendered so he have to clip overlapping parts
            # we basically query all items colliding with ourselves and if that's a manipulator and is over us we subtract
            # that from the clipped path.
            clipPath = QtGui.QPainterPath()
            clipPath.addRect(QtCore.QRectF(-self.scenePos().x(), -self.scenePos().y(), self.scene().sceneRect().width(), self.scene().sceneRect().height()))
            # FIXME: I used self.collidingItems() but that seems way way slower than just going over everything on the scene
            #        in reality we need siblings of ancestors recursively up to the top
            #
            #        this just begs for optimisation in the future
            collidingItems = self.scene().items()
            for item in collidingItems:
                if item.isVisible() and item is not self and isinstance(item, Manipulator):
                    if item.isAboveItem(self):
                        clipPath = clipPath.subtracted(item.boundingClipPath().translated(item.scenePos() - self.scenePos()))

            # we clip using stencil buffers to prevent overlapping outlines appearing
            # FIXME: This could potentially get very slow for huge layouts
            painter.setClipPath(clipPath)

        self.impl_paint(painter, option, widget)

        painter.restore()

    def impl_paint(self, painter, option, widget):
        super(Manipulator, self).paint(painter, option, widget)

        if self.isSelected() or self.resizeInProgress or self.isAnyHandleSelected():
            baseSize = self.getBaseSize()
            self.paintHorizontalGuides(baseSize, painter, option, widget)
            self.paintVerticalGuides(baseSize, painter, option, widget)

    def triggerPropertyManagerCallback(self, propertyNames):
        """Notify the property manager that the values of the given
        properties have changed for this widget.
        """
        widget = self.widget

        # if the property manager has set callbacks on this widget
        if hasattr(widget, "propertyManagerCallbacks"):
            for propertyName in propertyNames:
                # if there's a callback for this property
                if propertyName in widget.propertyManagerCallbacks:
                    # call it
                    widget.propertyManagerCallbacks[propertyName]()

    def hasNonAutoWidgetDescendants(self):
        """Checks whether there are non-auto widgets nested in this widget

        Self is a descendant of self in this context!
        """

        def impl_hasNonAutoWidgetDescendants(widget):
            if not widget.isAutoWindow():
                return True

            for i in xrange(widget.getChildCount()):
                child = widget.getChildAtIdx(i)

                if impl_hasNonAutoWidgetDescendants(child):
                    return True

            return False

        return impl_hasNonAutoWidgetDescendants(self.widget)

class SerialisationData(object):
    """Allows to "freeze" CEGUI widget to data that is easy to retain in python,
    this is a helper class that can be used for copy/paste, undo commands, etc...
    """

    def __init__(self, widget = None, serialiseChildren = True):
        self.name = ""
        self.type = ""
        # full parent path at the time of serialisation
        self.parentPath = ""
        # if True, the widget was an auto-widget, therefore it will not be
        # created directly when reconstructing but instead just retrieved
        self.autoWidget = False
        self.properties = {}
        self.children = []

        if widget is not None:
            self.name = widget.getName()
            self.type = widget.getType()
            self.autoWidget = widget.isAutoWindow()

            parent = widget.getParent()
            if parent is not None:
                self.parentPath = parent.getNamePath()

            self.serialiseProperties(widget)

            if serialiseChildren:
                self.serialiseChildren(widget)

    def setParentPath(self, parentPath):
        """Recursively changes the parent path
        """

        self.parentPath = parentPath

        for child in self.children:
            child.setParentPath(self.parentPath + "/" + self.name)

    def createChildData(self, widget, serialiseChildren = True):
        """Creates child serialisation data of this widget
        """

        return SerialisationData(widget, serialiseChildren)

    def createManipulator(self, parentManipulator, widget, recursive = True, skipAutoWidgets = False):
        """Creates a manipulator suitable for the widget resulting from reconstruction
        """

        return Manipulator(parentManipulator, widget, recursive, skipAutoWidgets)

    def serialiseProperties(self, widget):
        """Takes a snapshot of all properties of given widget and stores them
        in a string form
        """

        it = widget.getPropertyIterator()
        while not it.isAtEnd():
            propertyName = it.getCurrentKey()

            if not widget.isPropertyBannedFromXML(propertyName) and not widget.isPropertyDefault(propertyName):
                self.properties[propertyName] = widget.getProperty(propertyName)

            it.next()

    def serialiseChildren(self, widget, skipAutoWidgets = False):
        """Serialises all child widgets of given widgets

        skipAutoWidgets - should auto widgets be skipped over?
        """

        i = 0

        while i < widget.getChildCount():
            child = widget.getChildAtIdx(i)
            if not skipAutoWidgets or not child.isAutoWindow():
                self.children.append(self.createChildData(child, True))

            i += 1

    def reconstruct(self, rootManipulator):
        """Reconstructs widget serialised in this SerialisationData.

        rootManipulator - manipulator at the root of the target tree
        """

        widget = None
        ret = None

        if rootManipulator is None:
            if self.autoWidget:
                raise RuntimeError("Root widget can't be an auto widget!")

            widget = PyCEGUI.WindowManager.getSingleton().createWindow(self.type, self.name)
            ret = self.createManipulator(None, widget)
            rootManipulator = ret

        else:
            parentManipulator = None
            parentPathSplit = self.parentPath.split("/", 1)
            assert(len(parentPathSplit) >= 1)

            if len(parentPathSplit) == 1:
                parentManipulator = rootManipulator
            else:
                parentManipulator = rootManipulator.getManipulatorByPath(parentPathSplit[1])

            widget = None
            if self.autoWidget:
                widget = parentManipulator.widget.getChild(self.name)
                if widget.getType() != self.type:
                    raise RuntimeError("Skipping widget construction because it's an auto widget, the types don't match though!")

            else:
                widget = PyCEGUI.WindowManager.getSingleton().createWindow(self.type, self.name)
                parentManipulator.widget.addChild(widget)

            # Extra code because of auto windows
            # NOTE: We don't have to do rootManipulator.createMissingChildManipulators
            #       because auto windows never get created outside their parent
            parentManipulator.createMissingChildManipulators(True, False)

            realPathSplit = widget.getNamePath().split("/", 1)
            ret = rootManipulator.getManipulatorByPath(realPathSplit[1])

        for name, value in self.properties.iteritems():
            widget.setProperty(name, value)

        for child in self.children:
            childWidget = child.reconstruct(rootManipulator).widget
            if not child.autoWidget:
                widget.addChild(childWidget)

        # refresh the manipulator using newly set properties
        ret.updateFromWidget()
        return ret
