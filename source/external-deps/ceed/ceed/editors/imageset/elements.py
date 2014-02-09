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

from xml.etree import cElementTree as ElementTree
import os

from ceed import resizable
from ceed import qtwidgets

class ImageLabel(QtGui.QGraphicsTextItem):
    """Text item showing image's label when the image is hovered or selected.
    You should not use this directly! Use ImageEntry.name instead to get the name.
    """

    def __init__(self, imageEntry):
        super(ImageLabel, self).__init__(imageEntry)

        self.imageEntry = imageEntry

        self.setFlags(QtGui.QGraphicsItem.ItemIgnoresTransformations)
        self.setOpacity(0.8)

        self.setPlainText("Unknown")

        # we make the label a lot more transparent when mouse is over it to make it easier
        # to work around the top edge of the image
        self.setAcceptHoverEvents(True)
        # the default opacity (when mouse is not over the label)
        self.setOpacity(0.8)

        # be invisible by default and wait for hover/selection events
        self.setVisible(False)

    def paint(self, painter, option, widget):
        palette = QtGui.QApplication.palette()

        painter.fillRect(self.boundingRect(), palette.color(QtGui.QPalette.Normal, QtGui.QPalette.Base))
        painter.drawRect(self.boundingRect())

        super(ImageLabel, self).paint(painter, option, widget)

    def hoverEnterEvent(self, event):
        super(ImageLabel, self).hoverEnterEvent(event)

        self.setOpacity(0.2)

    def hoverLeaveEvent(self, event):
        self.setOpacity(0.8)

        super(ImageLabel, self).hoverLeaveEvent(event)

class ImageOffset(QtGui.QGraphicsPixmapItem):
    """A crosshair showing where the imaginary (0, 0) point of the image is. The actual offset
    is just a negated vector of the crosshair's position but this is easier to work with from
    the artist's point of view.
    """

    def __init__(self, imageEntry):
        super(ImageOffset, self).__init__(imageEntry)

        self.imageEntry = imageEntry

        self.setFlags(QtGui.QGraphicsItem.ItemIsMovable |
                      QtGui.QGraphicsItem.ItemIsSelectable |
                      QtGui.QGraphicsItem.ItemIgnoresTransformations |
                      QtGui.QGraphicsItem.ItemSendsGeometryChanges)

        self.setCursor(QtCore.Qt.OpenHandCursor)

        self.setPixmap(QtGui.QPixmap("icons/imageset_editing/offset_crosshair.png"))
        # the crosshair pixmap is 15x15, (7, 7) is the centre pixel of it,
        # we want that to be the (0, 0) point of the crosshair
        self.setOffset(-7, -7)
        # always show this above the label (which has ZValue = 0)
        self.setZValue(1)

        self.setAcceptHoverEvents(True)
        # internal attribute to help decide when to hide/show the offset crosshair
        self.isHovered = False

        # used for undo
        self.potentialMove = False
        self.oldPosition = None

        # by default Qt considers parts of the image with alpha = 0 not part of the image,
        # that would make it very hard to move the crosshair, we consider the whole
        # bounding rectangle to be part of the image
        self.setShapeMode(QtGui.QGraphicsPixmapItem.BoundingRectShape)
        self.setVisible(False)

    def itemChange(self, change, value):
        if change == QtGui.QGraphicsItem.ItemPositionChange:
            if self.potentialMove and not self.oldPosition:
                self.oldPosition = self.pos()

            newPosition = value

            # now round the position to pixels
            newPosition.setX(round(newPosition.x() - 0.5) + 0.5)
            newPosition.setY(round(newPosition.y() - 0.5) + 0.5)

            return newPosition

        elif change == QtGui.QGraphicsItem.ItemSelectedChange:
            if not value:
                if not self.imageEntry.isSelected():
                    self.setVisible(False)
            else:
                self.setVisible(True)

        return super(ImageOffset, self).itemChange(change, value)

    def hoverEnterEvent(self, event):
        super(ImageOffset, self).hoverEnterEvent(event)

        self.isHovered = True

    def hoverLeaveEvent(self, event):
        self.isHovered = False

        super(ImageOffset, self).hoverLeaveEvent(event)

class ImageEntry(resizable.ResizableRectItem):
    """Represents the image of the imageset, can be drag moved, selected, resized, ...
    """

    # the image's "real parameters" are properties that directly access Qt's
    # facilities, this is done to make the code cleaner and save a little memory

    name = property(lambda self: self.label.toPlainText(),
                    lambda self, value: self.label.setPlainText(value))

    xpos = property(lambda self: int(self.pos().x()),
                    lambda self, value: self.setPos(value, self.pos().y()))
    ypos = property(lambda self: int(self.pos().y()),
                    lambda self, value: self.setPos(self.pos().x(), value))
    width = property(lambda self: int(self.rect().width()),
                     lambda self, value: self.setRect(QtCore.QRectF(0, 0, max(1, value), self.height)))
    height = property(lambda self: int(self.rect().height()),
                      lambda self, value: self.setRect(QtCore.QRectF(0, 0, self.width, max(1, value))))

    xoffset = property(lambda self: int(-(self.offset.pos().x() - 0.5)),
                       lambda self, value: self.offset.setX(-float(value) + 0.5))
    yoffset = property(lambda self: int(-(self.offset.pos().y() - 0.5)),
                       lambda self, value: self.offset.setY(-float(value) + 0.5))

    nativeRes = property(lambda self: (self.nativeHorzRes, self.nativeVertRes),
                         lambda self, value: self.setNativeRes(value))

    def __init__(self, imagesetEntry):
        super(ImageEntry, self).__init__(imagesetEntry)

        self.nativeHorzRes = 0
        self.nativeVertRes = 0
        self.autoScaled = ""

        self.imagesetEntry = imagesetEntry

        self.setAcceptsHoverEvents(True)
        self.isHovered = False

        # used for undo
        self.potentialMove = False
        self.oldPosition = None
        self.resized = False

        self.setFlags(QtGui.QGraphicsItem.ItemIsMovable |
                      QtGui.QGraphicsItem.ItemIsSelectable |
                      QtGui.QGraphicsItem.ItemSendsGeometryChanges)

        self.setVisible(True)

        self.label = ImageLabel(self)
        self.offset = ImageOffset(self)

        # list item in the dock widget's ListWidget
        # this allows fast updates of the list item without looking it up
        # It is save to assume that this is None or a valid QListWidgetItem
        self.listItem = None

    def constrainResizeRect(self, rect, oldRect):
        # we simply round the rectangle because we only support "full" pixels

        # NOTE: Imageset as such might support floating point pixels but it's never what you
        #       really want, image quality deteriorates a lot

        rect = QtCore.QRectF(QtCore.QPointF(round(rect.topLeft().x()), round(rect.topLeft().y())),
                             QtCore.QPointF(round(rect.bottomRight().x()), round(rect.bottomRight().y())))

        return super(ImageEntry, self).constrainResizeRect(rect, oldRect)

    def loadFromElement(self, element):
        self.name = element.get("name", "Unknown")

        self.xpos = int(element.get("xPos", 0))
        self.ypos = int(element.get("yPos", 0))
        self.width = int(element.get("width", 1))
        self.height = int(element.get("height", 1))

        self.xoffset = int(element.get("xOffset", 0))
        self.yoffset = int(element.get("yOffset", 0))

        self.nativeHorzRes = int(element.get("nativeHorzRes", 0))
        self.nativeVertRes = int(element.get("nativeVertRes", 0))
        self.autoScaled = element.get("autoScaled", "")

    def saveToElement(self):
        ret = ElementTree.Element("Image")

        ret.set("name", self.name)

        ret.set("xPos", str(self.xpos))
        ret.set("yPos", str(self.ypos))
        ret.set("width", str(self.width))
        ret.set("height", str(self.height))

        # we write none or both
        if self.xoffset != 0 or self.yoffset != 0:
            ret.set("xOffset", str(self.xoffset))
            ret.set("yOffset", str(self.yoffset))

        if self.nativeHorzRes != 0:
            ret.set("nativeHorzRes", str(self.nativeHorzRes))

        if self.nativeVertRes != 0:
            ret.set("nativeVertRes", str(self.nativeVertRes))

        if self.autoScaled != "":
            ret.set("autoScaled", self.autoScaled)

        return ret

    def getPixmap(self):
        """Creates and returns a pixmap containing what's in the underlying image in the rectangle
        that this ImageEntry has set.

        This is mostly used for preview thumbnails in the dock widget.
        """
        return self.imagesetEntry.pixmap().copy(int(self.pos().x()), int(self.pos().y()),
                                                int(self.rect().width()), int(self.rect().height()))

    def updateListItem(self):
        """Updates the list item associated with this image entry in the dock widget
        """

        if not self.listItem:
            return

        self.listItem.setText(self.name)

        previewWidth = 24
        previewHeight = 24

        preview = QtGui.QPixmap(previewWidth, previewHeight)
        painter = QtGui.QPainter(preview)
        painter.setBrush(qtwidgets.getCheckerboardBrush())
        painter.drawRect(0, 0, previewWidth, previewHeight)
        scaledPixmap = self.getPixmap().scaled(QtCore.QSize(previewWidth, previewHeight),
                                               QtCore.Qt.KeepAspectRatio, QtCore.Qt.SmoothTransformation)
        painter.drawPixmap((previewWidth - scaledPixmap.width()) / 2,
                           (previewHeight - scaledPixmap.height()) / 2,
                           scaledPixmap)
        painter.end()

        self.listItem.setIcon(QtGui.QIcon(preview))

    def updateListItemSelection(self):
        """Synchronises the selection in the dock widget's list. This makes sure that when you select
        this item the list sets the selection to this item as well.
        """

        if not self.listItem:
            return

        dockWidget = self.listItem.dockWidget

        # the dock widget itself is performing a selection, we shall not interfere
        if dockWidget.selectionUnderway:
            return

        dockWidget.selectionSynchronisationUnderway = True

        if self.isSelected() or self.isAnyHandleSelected() or self.offset.isSelected():
            self.listItem.setSelected(True)
        else:
            self.listItem.setSelected(False)

        dockWidget.selectionSynchronisationUnderway = False

    def updateDockWidget(self):
        """If we are selected in the dock widget, this updates the property box
        """

        self.updateListItem()

        if not self.listItem:
            return

        dockWidget = self.listItem.dockWidget
        if dockWidget.activeImageEntry == self:
            dockWidget.refreshActiveImageEntry()

    def itemChange(self, change, value):
        if change == QtGui.QGraphicsItem.ItemSelectedHasChanged:
            if value:
                if settings.getEntry("imageset/visual/overlay_image_labels").value:
                    self.label.setVisible(True)

                if self.imagesetEntry.showOffsets:
                    self.offset.setVisible(True)

                self.setZValue(self.zValue() + 1)
            else:
                if not self.isHovered:
                    self.label.setVisible(False)

                if not self.offset.isSelected() and not self.offset.isHovered:
                    self.offset.setVisible(False)

                self.setZValue(self.zValue() - 1)

            self.updateListItemSelection()

        elif change == QtGui.QGraphicsItem.ItemPositionChange:
            if self.potentialMove and not self.oldPosition:
                self.oldPosition = self.pos()
                # hide label when moving so user can see edges clearly
                self.label.setVisible(False)

            newPosition = value

            # if, for whatever reason, the loading of the pixmap failed,
            # we don't constrain to the empty null pixmap

            # only constrain when the pixmap is valid
            if not self.imagesetEntry.pixmap().isNull():

                rect = self.imagesetEntry.boundingRect()
                rect.setWidth(rect.width() - self.rect().width())
                rect.setHeight(rect.height() - self.rect().height())

                if not rect.contains(newPosition):
                    newPosition.setX(min(rect.right(), max(newPosition.x(), rect.left())))
                    newPosition.setY(min(rect.bottom(), max(newPosition.y(), rect.top())))

            # now round the position to pixels
            newPosition.setX(round(newPosition.x()))
            newPosition.setY(round(newPosition.y()))

            return newPosition

        return super(ImageEntry, self).itemChange(change, value)

    def notifyResizeStarted(self):
        super(ImageEntry, self).notifyResizeStarted()

        # hide label when resizing so user can see edges clearly
        self.label.setVisible(False)

    def notifyResizeFinished(self, newPos, newRect):
        super(ImageEntry, self).notifyResizeFinished(newPos, newRect)

        if self.mouseOver and settings.getEntry("imageset/visual/overlay_image_labels").value:
            # if mouse is over we show the label again when resizing finishes
            self.label.setVisible(True)

        # mark as resized so we can pick it up in VisualEditing.mouseReleaseEvent
        self.resized = True

    def hoverEnterEvent(self, event):
        super(ImageEntry, self).hoverEnterEvent(event)

        self.setZValue(self.zValue() + 1)

        if settings.getEntry("imageset/visual/overlay_image_labels").value:
            self.label.setVisible(True)

        mainwindow.MainWindow.instance.statusBar().showMessage("Image: '%s'\t\tXPos: %i, YPos: %i, Width: %i, Height: %i" %
                                                               (self.name, self.pos().x(), self.pos().y(), self.rect().width(), self.rect().height()))

        self.isHovered = True

    def hoverLeaveEvent(self, event):
        mainwindow.MainWindow.instance.statusBar().clearMessage()

        self.isHovered = False

        if not self.isSelected():
            self.label.setVisible(False)

        self.setZValue(self.zValue() - 1)

        super(ImageEntry, self).hoverLeaveEvent(event)

    def paint(self, painter, option, widget):
        super(ImageEntry, self).paint(painter, option, widget)

        # to be more visible, we draw yellow rect over the usual dashed double colour rect
        if self.isSelected():
            painter.setPen(QtGui.QColor(255, 255, 0, 255))
            painter.drawRect(self.rect())

    def setNativeRes(self, value):
        # NB: This is just a wrapper to make the property setter lambda work
        self.nativeHorzRes, self.nativeVertRes = value

class ImagesetEntry(QtGui.QGraphicsPixmapItem):
    """This is the whole imageset containing all the images (ImageEntries).

    The main reason for this is not to have multiple imagesets editing at once but rather
    to have the transparency background working properly.
    """

    def __init__(self, visual):
        super(ImagesetEntry, self).__init__()

        self.name = "Unknown"
        self.imageFile = ""
        self.nativeHorzRes = 800
        self.nativeVertRes = 600
        self.autoScaled = "false"

        self.setShapeMode(QtGui.QGraphicsPixmapItem.BoundingRectShape)
        self.setCursor(QtCore.Qt.ArrowCursor)

        self.visual = visual
        self.imageEntries = []

        self.showOffsets = False

        self.transparencyBackground = QtGui.QGraphicsRectItem()
        self.transparencyBackground.setParentItem(self)
        self.transparencyBackground.setFlags(QtGui.QGraphicsItem.ItemStacksBehindParent)

        self.transparencyBackground.setBrush(qtwidgets.getCheckerboardBrush())
        self.transparencyBackground.setPen(QtGui.QPen(QtGui.QColor(QtCore.Qt.transparent)))

        self.imageMonitor = None
        self.displayingReloadAlert = False

    def getImageEntry(self, name):
        for image in self.imageEntries:
            if image.name == name:
                return image

        assert(False)
        return None

    def slot_imageChangedByExternalProgram(self):
        """Monitor the image with a QFilesystemWatcher, ask user to reload
        if changes to the file were made."""

        if not self.displayingReloadAlert:
            self.displayingReloadAlert = True
            ret = QtGui.QMessageBox.question(self.visual.tabbedEditor.mainWindow,
                                             "Underlying image '%s' has been modified externally!" % (self.imageFile),
                                             "The file has been modified outside the CEGUI Unified Editor.\n\nReload the file?\n\nIf you select Yes, UNDO HISTORY MIGHT BE PARTIALLY BROKEN UNLESS THE NEW SIZE IS THE SAME OR LARGER THAN THE OLD!",
                                             QtGui.QMessageBox.No | QtGui.QMessageBox.Yes,
                                             QtGui.QMessageBox.No) # defaulting to No is safer IMO

            if ret == QtGui.QMessageBox.Yes:
                self.loadImage(self.imageFile)

            elif ret == QtGui.QMessageBox.No:
                pass

            else:
                # how did we get here?
                assert(False)

            self.displayingReloadAlert = False

    def loadImage(self, relativeImagePath):
        """
        Replaces the underlying image (if any is loaded) to the image on given relative path

        Relative path is relative to the directory where the .imageset file resides
        (which is usually your project's imageset resource group path)
        """

        # If imageMonitor is null, then no images are being watched or the
        # editor is first being opened up
        # Otherwise, the image is being changed or switched, and the monitor
        # should update itself accordingly
        if self.imageMonitor != None:
            self.imageMonitor.removePath(self.getAbsoluteImageFile())

        self.imageFile = relativeImagePath
        self.setPixmap(QtGui.QPixmap(self.getAbsoluteImageFile()))
        self.transparencyBackground.setRect(self.boundingRect())

        # go over all image entries and set their position to force them to be constrained
        # to the new pixmap's dimensions
        for imageEntry in self.imageEntries:
            imageEntry.setPos(imageEntry.pos())
            imageEntry.updateDockWidget()

        self.visual.refreshSceneRect()

        # If imageMonitor is null, allocate and watch the loaded file
        if self.imageMonitor == None:
            self.imageMonitor = QtCore.QFileSystemWatcher(None)
            self.imageMonitor.fileChanged.connect(self.slot_imageChangedByExternalProgram)
        self.imageMonitor.addPath(self.getAbsoluteImageFile())

    def getAbsoluteImageFile(self):
        """Returns an absolute (OS specific!) path of the underlying image
        """

        return os.path.join(os.path.dirname(self.visual.tabbedEditor.filePath), self.imageFile)

    def convertToRelativeImageFile(self, absoluteImageFile):
        """Converts given absolute underlying image path to relative path (relative to the directory where
        the .imageset file resides
        """

        return os.path.normpath(os.path.relpath(absoluteImageFile, os.path.dirname(self.visual.tabbedEditor.filePath)))

    def loadFromElement(self, element):
        self.name = element.get("name", "Unknown")

        self.loadImage(element.get("imagefile", ""))

        self.nativeHorzRes = int(element.get("nativeHorzRes", 800))
        self.nativeVertRes = int(element.get("nativeVertRes", 600))
        self.autoScaled = element.get("autoScaled")

        for imageElement in element.findall("Image"):
            image = ImageEntry(self)
            image.loadFromElement(imageElement)
            self.imageEntries.append(image)

    def saveToElement(self):
        ret = ElementTree.Element("Imageset")

        ret.set("version", "2")

        ret.set("name", self.name)
        ret.set("imagefile", self.imageFile)

        ret.set("nativeHorzRes", str(self.nativeHorzRes))
        ret.set("nativeVertRes", str(self.nativeVertRes))
        ret.set("autoScaled", self.autoScaled)

        for image in self.imageEntries:
            ret.append(image.saveToElement())

        return ret

# needs to be at the end, import to get the singleton
from ceed import mainwindow
from ceed import settings
