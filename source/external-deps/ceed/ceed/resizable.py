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

class GraphicsView(QtGui.QGraphicsView):
    """If you plan to use ResizableGraphicsRectItems, make sure you view them
    via a GraphicsView that is inherited from this exact class.

    The reason for that is that The ResizableRectItem needs to counter-scale
    resizing handles

    cegui.GraphicsView inherits from this class because you are likely to use
    resizables on top of CEGUI. If you don't need them, simply don't use them.
    The overhead is minimal.
    """

    def __init__(self, parent = None):
        super(GraphicsView, self).__init__(parent)

        # disabled by default
        self.wheelZoomEnabled = False
        self.zoomFactor = 1.0

        self.middleButtonDragScrollEnabled = False
        self.lastDragScrollMousePosition = None

        self.ctrlZoom = settings.getEntry("global/navigation/ctrl_zoom")

    def setTransform(self, transform):
        super(GraphicsView, self).setTransform(transform)

        sx = transform.m11()
        sy = transform.m22()

        self.scaleChanged(sx, sy)

    def scaleChanged(self, sx, sy):
        for item in self.scene().items():
            if isinstance(item, ResizableRectItem):
                item.scaleChanged(sx, sy)

    def performZoom(self):
        transform = QtGui.QTransform()
        transform.scale(self.zoomFactor, self.zoomFactor)
        self.setTransform(transform)

    def zoomOriginal(self):
        self.zoomFactor = 1
        self.performZoom()

    def zoomIn(self):
        self.zoomFactor *= 2

        if self.zoomFactor > 256:
            self.zoomFactor = 256

        self.performZoom()

    def zoomOut(self):
        self.zoomFactor /= 2

        if self.zoomFactor < 0.5:
            self.zoomFactor = 0.5

        self.performZoom()

    def wheelEvent(self, event):
        if self.wheelZoomEnabled and (not self.ctrlZoom.value or event.modifiers() & QtCore.Qt.ControlModifier):
            if event.delta() == 0:
                return

            if event.delta() > 0:
                self.zoomIn()
            else:
                self.zoomOut()

        else:
            super(GraphicsView, self).wheelEvent(event)

    def mousePressEvent(self, event):
        if self.middleButtonDragScrollEnabled and (event.buttons() == QtCore.Qt.MiddleButton):
            self.lastDragScrollMousePosition = event.pos()

        else:
            super(GraphicsView, self).mousePressEvent(event)

    def mouseReleaseEvent(self, event):
        """When mouse is released in a resizable view, we have to
        go through all selected items and notify them of the release.

        This helps track undo movement and undo resize way easier
        """

        for selectedItem in self.scene().selectedItems():
            if isinstance(selectedItem, ResizingHandle):
                selectedItem.mouseReleaseEventSelected(event)
            elif isinstance(selectedItem, ResizableRectItem):
                selectedItem.mouseReleaseEventSelected(event)

        super(GraphicsView, self).mouseReleaseEvent(event)

    def mouseMoveEvent(self, event):
        if self.middleButtonDragScrollEnabled and (event.buttons() == QtCore.Qt.MiddleButton):
            horizontal = self.horizontalScrollBar()
            horizontal.setSliderPosition(horizontal.sliderPosition() - (event.pos().x() - self.lastDragScrollMousePosition.x()))
            vertical = self.verticalScrollBar()
            vertical.setSliderPosition(vertical.sliderPosition() - (event.pos().y() - self.lastDragScrollMousePosition.y()))

        else:
            super(GraphicsView, self).mouseMoveEvent(event)

        self.lastDragScrollMousePosition = event.pos()

class ResizingHandle(QtGui.QGraphicsRectItem):
    """A rectangle that when moved resizes the parent resizable rect item.

    The reason to go with a child GraphicsRectItem instead of just overriding mousePressEvent et al
    is to easily support multi selection resizing (you can multi-select various edges in all imaginable
    combinations and resize many things at once).
    """

    def __init__(self, parent):
        super(ResizingHandle, self).__init__(parent)
        self.parentResizable = parent

        self.setFlags(QtGui.QGraphicsItem.ItemIsSelectable |
                      QtGui.QGraphicsItem.ItemIsMovable |
                      QtGui.QGraphicsItem.ItemSendsGeometryChanges)

        self.setAcceptHoverEvents(True)

        self.ignoreGeometryChanges = False
        self.ignoreTransformChanges = False
        self.mouseOver = False
        self.currentView = None

    def performResizing(self, value):
        """Adjusts the parent rectangle and returns a position to use for this handle
        (with restrictions accounted for)
        """

        # designed to be overriden
        # pylint: disable-msg=R0201

        return value

    def unselectAllSiblingHandles(self):
        """Makes sure all siblings of this handle are unselected."""

        assert(self.parentResizable)

        for item in self.parentResizable.childItems():
            if isinstance(item, ResizingHandle) and not self is item:
                item.setSelected(False)

    def itemChange(self, change, value):
        """This overriden method does most of the resize work
        """

        if change == QtGui.QGraphicsItem.ItemSelectedChange:
            if self.parentResizable.isSelected():
                # we disallow multi-selecting a resizable item and one of it's handles,
                return False

        elif change == QtGui.QGraphicsItem.ItemSelectedHasChanged:
            # if we have indeed been selected, make sure all our sibling handles are unselected
            # we allow multi-selecting multiple handles but only one handle per resizable is allowed

            self.unselectAllSiblingHandles()
            self.parentResizable.notifyHandleSelected(self)

        elif change == QtGui.QGraphicsItem.ItemPositionChange:
            # this is the money code
            # changing position of the handle resizes the whole resizable
            if not self.parentResizable.resizeInProgress and not self.ignoreGeometryChanges:
                self.parentResizable.resizeInProgress = True
                self.parentResizable.resizeOldPos = self.parentResizable.pos()
                self.parentResizable.resizeOldRect = self.parentResizable.rect()

                self.parentResizable.setPen(self.parentResizable.getPenWhileResizing())
                self.parentResizable.hideAllHandles(excluding = self)

                self.parentResizable.notifyResizeStarted()

            if self.parentResizable.resizeInProgress:
                ret = self.performResizing(value)

                newPos = self.parentResizable.pos() + self.parentResizable.rect().topLeft()
                newRect = QtCore.QRectF(0, 0, self.parentResizable.rect().width(), self.parentResizable.rect().height())

                self.parentResizable.notifyResizeProgress(newPos, newRect)

                return ret

        return super(ResizingHandle, self).itemChange(change, value)

    def mouseReleaseEventSelected(self, event):
        """Called when mouse is released whilst this was selected.
        This notifies us that resizing might have ended.
        """

        if self.parentResizable.resizeInProgress:
            # resize was in progress and just ended
            self.parentResizable.resizeInProgress = False
            self.parentResizable.setPen(self.parentResizable.getHoverPen() if self.parentResizable.mouseOver else self.parentResizable.getNormalPen())

            newPos = self.parentResizable.pos() + self.parentResizable.rect().topLeft()
            newRect = QtCore.QRectF(0, 0, self.parentResizable.rect().width(), self.parentResizable.rect().height())

            self.parentResizable.notifyResizeFinished(newPos, newRect)

    def hoverEnterEvent(self, event):
        super(ResizingHandle, self).hoverEnterEvent(event)

        self.mouseOver = True

    def hoverLeaveEvent(self, event):
        self.mouseOver = False

        super(ResizingHandle, self).hoverLeaveEvent(event)

    def scaleChanged(self, sx, sy):
        pass

class EdgeResizingHandle(ResizingHandle):
    """Resizing handle positioned on one of the 4 edges
    """

    def __init__(self, parent):
        super(EdgeResizingHandle, self).__init__(parent)

        self.setPen(self.parentResizable.getEdgeResizingHandleHiddenPen())

    def hoverEnterEvent(self, event):
        super(EdgeResizingHandle, self).hoverEnterEvent(event)

        self.setPen(self.parentResizable.getEdgeResizingHandleHoverPen())

    def hoverLeaveEvent(self, event):
        self.setPen(self.parentResizable.getEdgeResizingHandleHiddenPen())

        super(EdgeResizingHandle, self).hoverLeaveEvent(event)

class TopEdgeResizingHandle(EdgeResizingHandle):
    def __init__(self, parent):
        super(TopEdgeResizingHandle, self).__init__(parent)

        self.setCursor(QtCore.Qt.SizeVerCursor)

    def performResizing(self, value):
        delta = value.y() - self.pos().y()
        _, dy1, _, _ = self.parentResizable.performResizing(self, 0, delta, 0, 0)

        return QtCore.QPointF(self.pos().x(), dy1 + self.pos().y())

    def scaleChanged(self, sx, sy):
        super(TopEdgeResizingHandle, self).scaleChanged(sx, sy)

        transform = self.transform()
        transform = QtGui.QTransform(1.0, transform.m12(), transform.m13(),
                                     transform.m21(), 1.0 / sy, transform.m23(),
                                     transform.m31(), transform.m32(), transform.m33())
        self.setTransform(transform)

class BottomEdgeResizingHandle(EdgeResizingHandle):
    def __init__(self, parent):
        super(BottomEdgeResizingHandle, self).__init__(parent)

        self.setCursor(QtCore.Qt.SizeVerCursor)

    def performResizing(self, value):
        delta = value.y() - self.pos().y()
        _, _, _, dy2 = self.parentResizable.performResizing(self, 0, 0, 0, delta)

        return QtCore.QPointF(self.pos().x(), dy2 + self.pos().y())

    def scaleChanged(self, sx, sy):
        super(BottomEdgeResizingHandle, self).scaleChanged(sx, sy)

        transform = self.transform()
        transform = QtGui.QTransform(1.0, transform.m12(), transform.m13(),
                                     transform.m21(), 1.0 / sy, transform.m23(),
                                     transform.m31(), transform.m32(), transform.m33())
        self.setTransform(transform)

class LeftEdgeResizingHandle(EdgeResizingHandle):
    def __init__(self, parent):
        super(LeftEdgeResizingHandle, self).__init__(parent)

        self.setCursor(QtCore.Qt.SizeHorCursor)

    def performResizing(self, value):
        delta = value.x() - self.pos().x()
        dx1, _, _, _ = self.parentResizable.performResizing(self, delta, 0, 0, 0)

        return QtCore.QPointF(dx1 + self.pos().x(), self.pos().y())

    def scaleChanged(self, sx, sy):
        super(LeftEdgeResizingHandle, self).scaleChanged(sx, sy)

        transform = self.transform()
        transform = QtGui.QTransform(1.0 / sx, transform.m12(), transform.m13(),
                                     transform.m21(), 1.0, transform.m23(),
                                     transform.m31(), transform.m32(), transform.m33())
        self.setTransform(transform)

class RightEdgeResizingHandle(EdgeResizingHandle):
    def __init__(self, parent):
        super(RightEdgeResizingHandle, self).__init__(parent)

        self.setCursor(QtCore.Qt.SizeHorCursor)

    def performResizing(self, value):
        delta = value.x() - self.pos().x()
        _, _, dx2, _ = self.parentResizable.performResizing(self, 0, 0, delta, 0)

        return QtCore.QPointF(dx2 + self.pos().x(), self.pos().y())

    def scaleChanged(self, sx, sy):
        super(RightEdgeResizingHandle, self).scaleChanged(sx, sy)

        transform = self.transform()
        transform = QtGui.QTransform(1.0 / sx, transform.m12(), transform.m13(),
                                     transform.m21(), 1.0, transform.m23(),
                                     transform.m31(), transform.m32(), transform.m33())
        self.setTransform(transform)

class CornerResizingHandle(ResizingHandle):
    """Resizing handle positioned in one of the 4 corners.
    """

    def __init__(self, parent):
        super(CornerResizingHandle, self).__init__(parent)

        self.setPen(self.parentResizable.getCornerResizingHandleHiddenPen())
        self.setFlags(self.flags())

        self.setZValue(1)

    def scaleChanged(self, sx, sy):
        super(CornerResizingHandle, self).scaleChanged(sx, sy)

        transform = self.transform()
        transform = QtGui.QTransform(1.0 / sx, transform.m12(), transform.m13(),
                                     transform.m21(), 1.0 / sy, transform.m23(),
                                     transform.m31(), transform.m32(), transform.m33())
        self.setTransform(transform)

    def hoverEnterEvent(self, event):
        super(CornerResizingHandle, self).hoverEnterEvent(event)

        self.setPen(self.parentResizable.getCornerResizingHandleHoverPen())

    def hoverLeaveEvent(self, event):
        self.setPen(self.parentResizable.getCornerResizingHandleHiddenPen())

        super(CornerResizingHandle, self).hoverLeaveEvent(event)

class TopRightCornerResizingHandle(CornerResizingHandle):
    def __init__(self, parent):
        super(TopRightCornerResizingHandle, self).__init__(parent)

        self.setCursor(QtCore.Qt.SizeBDiagCursor)

    def performResizing(self, value):
        deltaX = value.x() - self.pos().x()
        deltaY = value.y() - self.pos().y()
        _, dy1, dx2, _ = self.parentResizable.performResizing(self, 0, deltaY, deltaX, 0)

        return QtCore.QPointF(dx2 + self.pos().x(), dy1 + self.pos().y())

class BottomRightCornerResizingHandle(CornerResizingHandle):
    def __init__(self, parent):
        super(BottomRightCornerResizingHandle, self).__init__(parent)

        self.setCursor(QtCore.Qt.SizeFDiagCursor)

    def performResizing(self, value):
        deltaX = value.x() - self.pos().x()
        deltaY = value.y() - self.pos().y()
        _, _, dx2, dy2 = self.parentResizable.performResizing(self, 0, 0, deltaX, deltaY)

        return QtCore.QPointF(dx2 + self.pos().x(), dy2 + self.pos().y())

class BottomLeftCornerResizingHandle(CornerResizingHandle):
    def __init__(self, parent):
        super(BottomLeftCornerResizingHandle, self).__init__(parent)

        self.setCursor(QtCore.Qt.SizeBDiagCursor)

    def performResizing(self, value):
        deltaX = value.x() - self.pos().x()
        deltaY = value.y() - self.pos().y()
        dx1, _, _, dy2 = self.parentResizable.performResizing(self, deltaX, 0, 0, deltaY)

        return QtCore.QPointF(dx1 + self.pos().x(), dy2 + self.pos().y())

class TopLeftCornerResizingHandle(CornerResizingHandle):
    def __init__(self, parent):
        super(TopLeftCornerResizingHandle, self).__init__(parent)

        self.setCursor(QtCore.Qt.SizeFDiagCursor)

    def performResizing(self, value):
        deltaX = value.x() - self.pos().x()
        deltaY = value.y() - self.pos().y()
        dx1, dy1, _, _ = self.parentResizable.performResizing(self, deltaX, deltaY, 0, 0)

        return QtCore.QPointF(dx1 + self.pos().x(), dy1 + self.pos().y())

class ResizableRectItem(QtGui.QGraphicsRectItem):
    """Rectangle that can be resized by dragging it's handles.

    Inherit from this class to gain resizing and moving capabilities.

    Depending on the size, the handles are shown outside the rectangle (if it's small)
    or inside (if it's large). All this is tweakable.
    """

    # This class is supposed to be inherited from and the "could be functions"
    # methods are supposed to be overriden, the refactor messages by pylint
    # therefore make no sense in this case

    # pylint: disable-msg=R0201

    def __init__(self, parentItem = None):
        super(ResizableRectItem, self).__init__(parentItem)

        self.setFlags(QtGui.QGraphicsItem.ItemSendsGeometryChanges |
                      QtGui.QGraphicsItem.ItemIsMovable)

        self.setAcceptsHoverEvents(True)
        self.mouseOver = False

        self.topEdgeHandle = TopEdgeResizingHandle(self)
        self.bottomEdgeHandle = BottomEdgeResizingHandle(self)
        self.leftEdgeHandle = LeftEdgeResizingHandle(self)
        self.rightEdgeHandle = RightEdgeResizingHandle(self)

        self.topRightCornerHandle = TopRightCornerResizingHandle(self)
        self.bottomRightCornerHandle = BottomRightCornerResizingHandle(self)
        self.bottomLeftCornerHandle = BottomLeftCornerResizingHandle(self)
        self.topLeftCornerHandle = TopLeftCornerResizingHandle(self)

        self.handlesDirty = True
        self.currentScaleX = 1
        self.currentScaleY = 1

        self.ignoreGeometryChanges = False

        self.resizeInProgress = False
        self.resizeOldPos = None
        self.resizeOldRect = None
        self.moveInProgress = False
        self.moveOldPos = None

        self.hideAllHandles()

        self.outerHandleSize = 0
        self.innerHandleSize = 0
        self.setOuterHandleSize(15)
        self.setInnerHandleSize(10)

        self.setCursor(QtCore.Qt.OpenHandCursor)
        self.setPen(self.getNormalPen())

    def getMinSize(self):
        ret = QtCore.QSizeF(1, 1)

        return ret

    def getMaxSize(self):
        return None

    def getNormalPen(self):
        ret = QtGui.QPen()
        ret.setColor(QtGui.QColor(255, 255, 255, 150))
        ret.setStyle(QtCore.Qt.DotLine)

        return ret

    def getHoverPen(self):
        ret = QtGui.QPen()
        ret.setColor(QtGui.QColor(0, 255, 255, 255))

        return ret

    def getPenWhileResizing(self):
        ret = QtGui.QPen(QtGui.QColor(255, 0, 255, 255))

        return ret

    def getPenWhileMoving(self):
        ret = QtGui.QPen(QtGui.QColor(255, 0, 255, 255))

        return ret

    def getEdgeResizingHandleHoverPen(self):
        ret = QtGui.QPen()
        ret.setColor(QtGui.QColor(0, 255, 255, 255))
        ret.setWidth(2)
        ret.setCosmetic(True)

        return ret

    def getEdgeResizingHandleHiddenPen(self):
        ret = QtGui.QPen()
        ret.setColor(QtCore.Qt.transparent)

        return ret

    def getCornerResizingHandleHoverPen(self):
        ret = QtGui.QPen()
        ret.setColor(QtGui.QColor(0, 255, 255, 255))
        ret.setWidth(2)
        ret.setCosmetic(True)

        return ret

    def getCornerResizingHandleHiddenPen(self):
        ret = QtGui.QPen()
        ret.setColor(QtCore.Qt.transparent)

        return ret

    def setOuterHandleSize(self, size):
        self.outerHandleSize = size
        self.handlesDirty = True

    def setInnerHandleSize(self, size):
        self.innerHandleSize = size
        self.handlesDirty = True

    def setRect(self, rect):
        super(ResizableRectItem, self).setRect(rect)

        self.handlesDirty = True
        self.ensureHandlesUpdated()

    def constrainMovePoint(self, point):
        return point

    def constrainResizeRect(self, rect, oldRect):
        minSize = self.getMinSize()
        maxSize = self.getMaxSize()

        if minSize:
            minRect = QtCore.QRectF(rect.center() - QtCore.QPointF(0.5 * minSize.width(), 0.5 * minSize.height()), minSize)
            rect = rect.united(minRect)
        if maxSize:
            maxRect = QtCore.QRectF(rect.center() - QtCore.QPointF(0.5 * maxSize.width(), 0.5 * maxSize.height()), maxSize)
            rect.intersected(maxRect)

        return rect

    def performResizing(self, handle, deltaX1, deltaY1, deltaX2, deltaY2):
        """Adjusts the rectangle and returns a 4-tuple of the actual used deltas
        (with restrictions accounted for)

        The default implementation doesn't use the handle parameter.
        """

        newRect = self.rect().adjusted(deltaX1, deltaY1, deltaX2, deltaY2)
        newRect = self.constrainResizeRect(newRect, self.rect())

        # TODO: the rect moves as a whole when it can't be sized any less
        #       this is probably not the behavior we want!

        topLeftDelta = newRect.topLeft() - self.rect().topLeft()
        bottomRightDelta = newRect.bottomRight() - self.rect().bottomRight()

        self.setRect(newRect)

        return topLeftDelta.x(), topLeftDelta.y(), bottomRightDelta.x(), bottomRightDelta.y()

    def hideAllHandles(self, excluding = None):
        """Hides all handles. If a handle is given as the 'excluding' parameter, this handle is
        skipped over when hiding
        """

        for item in self.childItems():
            if isinstance(item, ResizingHandle) and item is not excluding:
                if isinstance(item, EdgeResizingHandle):
                    item.setPen(self.getEdgeResizingHandleHiddenPen())

                elif isinstance(item, CornerResizingHandle):
                    item.setPen(self.getCornerResizingHandleHiddenPen())

    def setResizingEnabled(self, enabled = True):
        """Makes it possible to disable or enable resizing
        """

        for item in self.childItems():
            if isinstance(item, ResizingHandle):
                item.setVisible(enabled)

    def unselectAllHandles(self):
        """Unselects all handles of this resizable"""

        for item in self.childItems():
            if isinstance(item, ResizingHandle):
                item.setSelected(False)

    def notifyHandleSelected(self, handle):
        """A method meant to be overridden when you want to react when a handle is selected
        """

        pass

    def isAnyHandleSelected(self):
        """Checks whether any of the 8 handles is selected.
        note: At most 1 handle can be selected at a time!"""
        for item in self.childItems():
            if isinstance(item, ResizingHandle):
                if item.isSelected():
                    return True

        return False

    def ensureHandlesUpdated(self):
        """Makes sure handles are updated (if possible).
        Updating handles while resizing would mess things up big times, so we just ignore the
        update in that circumstance
        """

        if self.handlesDirty and not self.resizeInProgress:
            self.updateHandles()

    def absoluteXToRelative(self, value, transform):
        xScale = transform.m11()

        # this works in this special case, not in generic case!
        # I would have to undo rotation for this to work generically
        return value / xScale if xScale != 0 else 1

    def absoluteYToRelative(self, value, transform):
        yScale = transform.m22()

        # this works in this special case, not in generic case!
        # I would have to undo rotation for this to work generically
        return value / yScale if yScale != 0 else 1

    def updateHandles(self):
        """Updates all the handles according to geometry"""

        absoluteWidth = self.currentScaleX * self.rect().width()
        absoluteHeight = self.currentScaleY * self.rect().height()

        if absoluteWidth < 4 * self.outerHandleSize or absoluteHeight < 4 * self.outerHandleSize:
            self.topEdgeHandle.ignoreGeometryChanges = True
            self.topEdgeHandle.setPos(0, 0)
            self.topEdgeHandle.setRect(0, -self.innerHandleSize,
                                       self.rect().width(),
                                       self.innerHandleSize)
            self.topEdgeHandle.ignoreGeometryChanges = False

            self.bottomEdgeHandle.ignoreGeometryChanges = True
            self.bottomEdgeHandle.setPos(0, self.rect().height())
            self.bottomEdgeHandle.setRect(0, 0,
                                       self.rect().width(),
                                       self.innerHandleSize)
            self.bottomEdgeHandle.ignoreGeometryChanges = False

            self.leftEdgeHandle.ignoreGeometryChanges = True
            self.leftEdgeHandle.setPos(0, 0)
            self.leftEdgeHandle.setRect(-self.innerHandleSize, 0,
                                       self.innerHandleSize,
                                       self.rect().height())
            self.leftEdgeHandle.ignoreGeometryChanges = False

            self.rightEdgeHandle.ignoreGeometryChanges = True
            self.rightEdgeHandle.setPos(QtCore.QPointF(self.rect().width(), 0))
            self.rightEdgeHandle.setRect(0, 0,
                                       self.innerHandleSize,
                                       self.rect().height())
            self.rightEdgeHandle.ignoreGeometryChanges = False

            self.topRightCornerHandle.ignoreGeometryChanges = True
            self.topRightCornerHandle.setPos(self.rect().width(), 0)
            self.topRightCornerHandle.setRect(0, -self.innerHandleSize,
                                       self.innerHandleSize,
                                       self.innerHandleSize)
            self.topRightCornerHandle.ignoreGeometryChanges = False

            self.bottomRightCornerHandle.ignoreGeometryChanges = True
            self.bottomRightCornerHandle.setPos(self.rect().width(), self.rect().height())
            self.bottomRightCornerHandle.setRect(0, 0,
                                       self.innerHandleSize,
                                       self.innerHandleSize)
            self.bottomRightCornerHandle.ignoreGeometryChanges = False

            self.bottomLeftCornerHandle.ignoreGeometryChanges = True
            self.bottomLeftCornerHandle.setPos(0, self.rect().height())
            self.bottomLeftCornerHandle.setRect(-self.innerHandleSize, 0,
                                       self.innerHandleSize,
                                       self.innerHandleSize)
            self.bottomLeftCornerHandle.ignoreGeometryChanges = False

            self.topLeftCornerHandle.ignoreGeometryChanges = True
            self.topLeftCornerHandle.setPos(0, 0)
            self.topLeftCornerHandle.setRect(-self.innerHandleSize, -self.innerHandleSize,
                                       self.innerHandleSize,
                                       self.innerHandleSize)
            self.topLeftCornerHandle.ignoreGeometryChanges = False

        else:
            self.topEdgeHandle.ignoreGeometryChanges = True
            self.topEdgeHandle.setPos(0, 0)
            self.topEdgeHandle.setRect(0, 0,
                                       self.rect().width(),
                                       self.outerHandleSize)
            self.topEdgeHandle.ignoreGeometryChanges = False

            self.bottomEdgeHandle.ignoreGeometryChanges = True
            self.bottomEdgeHandle.setPos(0, self.rect().height())
            self.bottomEdgeHandle.setRect(0, -self.outerHandleSize,
                                       self.rect().width(),
                                       self.outerHandleSize)
            self.bottomEdgeHandle.ignoreGeometryChanges = False

            self.leftEdgeHandle.ignoreGeometryChanges = True
            self.leftEdgeHandle.setPos(QtCore.QPointF(0, 0))
            self.leftEdgeHandle.setRect(0, 0,
                                       self.outerHandleSize,
                                       self.rect().height())
            self.leftEdgeHandle.ignoreGeometryChanges = False

            self.rightEdgeHandle.ignoreGeometryChanges = True
            self.rightEdgeHandle.setPos(QtCore.QPointF(self.rect().width(), 0))
            self.rightEdgeHandle.setRect(-self.outerHandleSize, 0,
                                       self.outerHandleSize,
                                       self.rect().height())
            self.rightEdgeHandle.ignoreGeometryChanges = False

            self.topRightCornerHandle.ignoreGeometryChanges = True
            self.topRightCornerHandle.setPos(self.rect().width(), 0)
            self.topRightCornerHandle.setRect(-self.outerHandleSize, 0,
                                       self.outerHandleSize,
                                       self.outerHandleSize)
            self.topRightCornerHandle.ignoreGeometryChanges = False

            self.bottomRightCornerHandle.ignoreGeometryChanges = True
            self.bottomRightCornerHandle.setPos(self.rect().width(), self.rect().height())
            self.bottomRightCornerHandle.setRect(-self.outerHandleSize, -self.outerHandleSize,
                                       self.outerHandleSize,
                                       self.outerHandleSize)
            self.bottomRightCornerHandle.ignoreGeometryChanges = False

            self.bottomLeftCornerHandle.ignoreGeometryChanges = True
            self.bottomLeftCornerHandle.setPos(0, self.rect().height())
            self.bottomLeftCornerHandle.setRect(0, -self.outerHandleSize,
                                       self.outerHandleSize,
                                       self.outerHandleSize)
            self.bottomLeftCornerHandle.ignoreGeometryChanges = False

            self.topLeftCornerHandle.ignoreGeometryChanges = True
            self.topLeftCornerHandle.setPos(0, 0)
            self.topLeftCornerHandle.setRect(0, 0,
                                       self.outerHandleSize,
                                       self.outerHandleSize)
            self.topLeftCornerHandle.ignoreGeometryChanges = False

        self.handlesDirty = False

    def notifyResizeStarted(self):
        pass

    def notifyResizeProgress(self, newPos, newRect):
        pass

    def notifyResizeFinished(self, newPos, newRect):
        self.ignoreGeometryChanges = True
        self.setRect(newRect)
        self.setPos(newPos)
        self.ignoreGeometryChanges = False

    def notifyMoveStarted(self):
        pass

    def notifyMoveProgress(self, newPos):
        pass

    def notifyMoveFinished(self, newPos):
        pass

    def scaleChanged(self, sx, sy):
        self.currentScaleX = sx
        self.currentScaleY = sy

        for childItem in self.childItems():
            if isinstance(childItem, ResizingHandle):
                childItem.scaleChanged(sx, sy)

            elif isinstance(childItem, ResizableRectItem):
                childItem.scaleChanged(sx, sy)

        self.handlesDirty = True
        self.ensureHandlesUpdated()

    def itemChange(self, change, value):
        if change == QtGui.QGraphicsItem.ItemSelectedHasChanged:
            if value:
                self.unselectAllHandles()
            else:
                self.hideAllHandles()

        elif change == QtGui.QGraphicsItem.ItemPositionChange:
            value = self.constrainMovePoint(value)

            if not self.moveInProgress and not self.ignoreGeometryChanges:
                self.moveInProgress = True
                self.moveOldPos = self.pos()

                self.setPen(self.getPenWhileMoving())
                self.hideAllHandles()

                self.notifyMoveStarted()

            if self.moveInProgress:
                # value is the new position, self.pos() is the old position
                # we use value to avoid the 1 pixel lag
                self.notifyMoveProgress(value)

        return super(ResizableRectItem, self).itemChange(change, value)

    def hoverEnterEvent(self, event):
        super(ResizableRectItem, self).hoverEnterEvent(event)

        self.setPen(self.getHoverPen())
        self.mouseOver = True

    def hoverLeaveEvent(self, event):
        self.mouseOver = False
        self.setPen(self.getNormalPen())

        super(ResizableRectItem, self).hoverLeaveEvent(event)

    def mouseReleaseEventSelected(self, event):
        if self.moveInProgress:
            self.moveInProgress = False
            newPos = self.pos()

            self.notifyMoveFinished(newPos)

from ceed import settings
