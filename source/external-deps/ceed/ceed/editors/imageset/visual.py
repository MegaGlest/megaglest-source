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
from PySide import QtOpenGL

import fnmatch, re

from ceed.editors import multi
from ceed import qtwidgets
from ceed import resizable

from ceed.editors.imageset import elements
from ceed.editors.imageset import undo

import ceed.ui.editors.imageset.dockwidget
import cPickle

class ImageEntryItemDelegate(QtGui.QItemDelegate):
    """The only reason for this is to track when we are editing.

    We need this to discard key events when editor is open.
    TODO: Isn't there a better way to do this?
    """

    def __init__(self):
        super(ImageEntryItemDelegate, self).__init__()
        self.editing = False

    def setEditorData(self, editor, index):
        self.editing = True

        super(ImageEntryItemDelegate, self).setEditorData(editor, index)

    def setModelData(self, editor, model, index):
        super(ImageEntryItemDelegate, self).setModelData(editor, model, index)

        self.editing = False

class ImagesetEditorDockWidget(QtGui.QDockWidget):
    """Provides list of images, property editing of currently selected image and create/delete
    """

    def __init__(self, visual):
        super(ImagesetEditorDockWidget, self).__init__()

        self.visual = visual

        self.ui = ceed.ui.editors.imageset.dockwidget.Ui_DockWidget()
        self.ui.setupUi(self)

        self.name = self.findChild(QtGui.QLineEdit, "name")
        self.name.textEdited.connect(self.slot_nameEdited)
        self.image = self.findChild(qtwidgets.FileLineEdit, "image")
        # nasty, but at this point tabbedEditor.mainWindow isn't set yet
        project = mainwindow.MainWindow.instance.project
        self.image.startDirectory = lambda: project.getResourceFilePath("", "imagesets") if project is not None else ""
        self.imageLoad = self.findChild(QtGui.QPushButton, "imageLoad")
        self.imageLoad.clicked.connect(self.slot_imageLoadClicked)
        self.autoScaled = self.findChild(QtGui.QComboBox, "autoScaled")
        self.autoScaled.currentIndexChanged.connect(self.slot_autoScaledChanged)
        self.nativeHorzRes = self.findChild(QtGui.QLineEdit, "nativeHorzRes")
        self.nativeHorzRes.setValidator(QtGui.QIntValidator(0, 9999999, self))
        self.nativeHorzRes.textEdited.connect(self.slot_nativeResolutionEdited)
        self.nativeVertRes = self.findChild(QtGui.QLineEdit, "nativeVertRes")
        self.nativeVertRes.setValidator(QtGui.QIntValidator(0, 9999999, self))
        self.nativeVertRes.textEdited.connect(self.slot_nativeResolutionEdited)

        self.filterBox = self.findChild(QtGui.QLineEdit, "filterBox")
        self.filterBox.textChanged.connect(self.filterChanged)

        self.list = self.findChild(QtGui.QListWidget, "list")
        self.list.setItemDelegate(ImageEntryItemDelegate())
        self.list.itemSelectionChanged.connect(self.slot_itemSelectionChanged)
        self.list.itemChanged.connect(self.slot_itemChanged)

        self.selectionUnderway = False
        self.selectionSynchronisationUnderway = False

        self.positionX = self.findChild(QtGui.QLineEdit, "positionX")
        self.positionX.setValidator(QtGui.QIntValidator(0, 9999999, self))
        self.positionX.textChanged.connect(self.slot_positionXChanged)
        self.positionY = self.findChild(QtGui.QLineEdit, "positionY")
        self.positionY.setValidator(QtGui.QIntValidator(0, 9999999, self))
        self.positionY.textChanged.connect(self.slot_positionYChanged)
        self.width = self.findChild(QtGui.QLineEdit, "width")
        self.width.setValidator(QtGui.QIntValidator(0, 9999999, self))
        self.width.textChanged.connect(self.slot_widthChanged)
        self.height = self.findChild(QtGui.QLineEdit, "height")
        self.height.setValidator(QtGui.QIntValidator(0, 9999999, self))
        self.height.textChanged.connect(self.slot_heightChanged)
        self.offsetX = self.findChild(QtGui.QLineEdit, "offsetX")
        self.offsetX.setValidator(QtGui.QIntValidator(-9999999, 9999999, self))
        self.offsetX.textChanged.connect(self.slot_offsetXChanged)
        self.offsetY = self.findChild(QtGui.QLineEdit, "offsetY")
        self.offsetY.setValidator(QtGui.QIntValidator(-9999999, 9999999, self))
        self.offsetY.textChanged.connect(self.slot_offsetYChanged)

        self.autoScaledPerImage = self.findChild(QtGui.QComboBox, "autoScaledPerImage")
        self.autoScaledPerImage.currentIndexChanged.connect(self.slot_autoScaledPerImageChanged)
        self.nativeHorzResPerImage = self.findChild(QtGui.QLineEdit, "nativeHorzResPerImage")
        self.nativeHorzResPerImage.setValidator(QtGui.QIntValidator(0, 9999999, self))
        self.nativeHorzResPerImage.textEdited.connect(self.slot_nativeResolutionPerImageEdited)
        self.nativeVertResPerImage = self.findChild(QtGui.QLineEdit, "nativeVertResPerImage")
        self.nativeVertResPerImage.setValidator(QtGui.QIntValidator(0, 9999999, self))
        self.nativeVertResPerImage.textEdited.connect(self.slot_nativeResolutionPerImageEdited)

        self.setActiveImageEntry(None)

    def setImagesetEntry(self, imagesetEntry):
        self.imagesetEntry = imagesetEntry

    def refresh(self):
        """Refreshes the whole list

        Note: User potentially looses selection when this is called!
        """

        # FIXME: This is really really weird!
        #        If I call list.clear() it crashes when undoing image deletes for some reason
        #        I already spent several hours tracking it down and I couldn't find anything
        #
        #        If I remove items one by one via takeItem, everything works :-/
        #self.list.clear()

        self.selectionSynchronisationUnderway = True

        while self.list.takeItem(0):
            pass

        self.selectionSynchronisationUnderway = False

        self.setActiveImageEntry(None)

        self.name.setText(self.imagesetEntry.name)
        self.image.setText(self.imagesetEntry.getAbsoluteImageFile())
        self.autoScaled.setCurrentIndex(self.autoScaled.findText(self.imagesetEntry.autoScaled))
        self.nativeHorzRes.setText(str(self.imagesetEntry.nativeHorzRes))
        self.nativeVertRes.setText(str(self.imagesetEntry.nativeVertRes))

        for imageEntry in self.imagesetEntry.imageEntries:
            item = QtGui.QListWidgetItem()
            item.dockWidget = self
            item.setFlags(QtCore.Qt.ItemIsSelectable |
                          QtCore.Qt.ItemIsEditable |
                          QtCore.Qt.ItemIsEnabled)

            item.imageEntry = imageEntry
            imageEntry.listItem = item
            # nothing is selected (list was cleared) so we don't need to call
            #  the whole updateDockWidget here
            imageEntry.updateListItem()

            self.list.addItem(item)

        # explicitly call the filtering again to make sure it's in sync
        self.filterChanged(self.filterBox.text())

    def setActiveImageEntry(self, imageEntry):
        """Active image entry is the image entry that is selected when there are no
        other image entries selected. It's properties show in the property box.

        Note: Imageset editing doesn't allow multi selection property editing because
              IMO it doesn't make much sense.
        """

        self.activeImageEntry = imageEntry

        self.refreshActiveImageEntry()

    def refreshActiveImageEntry(self):
        """Refreshes the properties of active image entry (from image entry to the property box)
        """

        if not self.activeImageEntry:
            self.positionX.setText("")
            self.positionX.setEnabled(False)
            self.positionY.setText("")
            self.positionY.setEnabled(False)
            self.width.setText("")
            self.width.setEnabled(False)
            self.height.setText("")
            self.height.setEnabled(False)
            self.offsetX.setText("")
            self.offsetX.setEnabled(False)
            self.offsetY.setText("")
            self.offsetY.setEnabled(False)

            self.autoScaledPerImage.setCurrentIndex(0)
            self.autoScaledPerImage.setEnabled(False)
            self.nativeHorzResPerImage.setText("")
            self.nativeHorzResPerImage.setEnabled(False)
            self.nativeVertResPerImage.setText("")
            self.nativeVertResPerImage.setEnabled(False)

        else:
            self.positionX.setText(str(self.activeImageEntry.xpos))
            self.positionX.setEnabled(True)
            self.positionY.setText(str(self.activeImageEntry.ypos))
            self.positionY.setEnabled(True)
            self.width.setText(str(self.activeImageEntry.width))
            self.width.setEnabled(True)
            self.height.setText(str(self.activeImageEntry.height))
            self.height.setEnabled(True)
            self.offsetX.setText(str(self.activeImageEntry.xoffset))
            self.offsetX.setEnabled(True)
            self.offsetY.setText(str(self.activeImageEntry.yoffset))
            self.offsetY.setEnabled(True)

            self.autoScaledPerImage.setCurrentIndex(self.autoScaledPerImage.findText(self.activeImageEntry.autoScaled))
            self.autoScaledPerImage.setEnabled(True)
            self.nativeHorzResPerImage.setText(str(self.activeImageEntry.nativeHorzRes))
            self.nativeHorzResPerImage.setEnabled(True)
            self.nativeVertResPerImage.setText(str(self.activeImageEntry.nativeVertRes))
            self.nativeVertResPerImage.setEnabled(True)

    def keyReleaseEvent(self, event):
        # if we are editing, we should discard key events
        # (delete means delete character, not delete image entry in this context)

        if not self.list.itemDelegate().editing:
            if event.key() == QtCore.Qt.Key_Delete:
                selection = self.visual.scene().selectedItems()

                handled = self.visual.deleteImageEntries(selection)

                if handled:
                    return True

        return super(ImagesetEditorDockWidget, self).keyReleaseEvent(event)

    def slot_itemSelectionChanged(self):
        imageEntryNames = self.list.selectedItems()
        if len(imageEntryNames) == 1:
            imageEntry = imageEntryNames[0].imageEntry
            self.setActiveImageEntry(imageEntry)
        else:
            self.setActiveImageEntry(None)

        # we are getting synchronised with the visual editing pane, do not interfere
        if self.selectionSynchronisationUnderway:
            return

        self.selectionUnderway = True
        self.visual.scene().clearSelection()

        imageEntryNames = self.list.selectedItems()
        for imageEntryName in imageEntryNames:
            imageEntry = imageEntryName.imageEntry
            imageEntry.setSelected(True)

        if len(imageEntryNames) == 1:
            imageEntry = imageEntryNames[0].imageEntry
            self.visual.centerOn(imageEntry)

        self.selectionUnderway = False

    def slot_itemChanged(self, item):
        oldName = item.imageEntry.name
        newName = item.text()

        if oldName == newName:
            # most likely caused by RenameCommand doing it's work or is bogus anyways
            return

        cmd = undo.RenameCommand(self.visual, oldName, newName)
        self.visual.tabbedEditor.undoStack.push(cmd)

    def filterChanged(self, filter):
        # we append star at the beginning and at the end by default (makes property filtering much more practical)
        filter = "*" + filter + "*"
        regex = re.compile(fnmatch.translate(filter), re.IGNORECASE)

        i = 0
        while i < self.list.count():
            listItem = self.list.item(i)
            match = re.match(regex, listItem.text()) is not None
            listItem.setHidden(not match)

            i += 1

    def slot_nameEdited(self, newValue):
        oldName = self.imagesetEntry.name
        newName = self.name.text()

        if oldName == newName:
            return

        cmd = undo.ImagesetRenameCommand(self.visual, oldName, newName)
        self.visual.tabbedEditor.undoStack.push(cmd)

    def slot_imageLoadClicked(self):
        oldImageFile = self.imagesetEntry.imageFile
        newImageFile = self.imagesetEntry.convertToRelativeImageFile(self.image.text())

        if oldImageFile == newImageFile:
            return

        cmd = undo.ImagesetChangeImageCommand(self.visual, oldImageFile, newImageFile)
        self.visual.tabbedEditor.undoStack.push(cmd)

    def slot_autoScaledChanged(self, index):
        oldAutoScaled = self.imagesetEntry.autoScaled
        newAutoScaled = self.autoScaled.currentText()

        if oldAutoScaled == newAutoScaled:
            return

        cmd = undo.ImagesetChangeAutoScaledCommand(self.visual, oldAutoScaled, newAutoScaled)
        self.visual.tabbedEditor.undoStack.push(cmd)

    def slot_nativeResolutionEdited(self, newValue):
        oldHorzRes = self.imagesetEntry.nativeHorzRes
        oldVertRes = self.imagesetEntry.nativeVertRes

        try:
            newHorzRes = int(self.nativeHorzRes.text())
            newVertRes = int(self.nativeVertRes.text())

        except ValueError:
            return

        if oldHorzRes == newHorzRes and oldVertRes == newVertRes:
            return

        cmd = undo.ImagesetChangeNativeResolutionCommand(self.visual, oldHorzRes, oldVertRes, newHorzRes, newVertRes)
        self.visual.tabbedEditor.undoStack.push(cmd)

    def metaslot_propertyChangedInt(self, propertyName, newTextValue):
        if not self.activeImageEntry:
            return

        oldValue = getattr(self.activeImageEntry, propertyName)
        newValue = None

        try:
            newValue = int(newTextValue)
        except ValueError:
            # if the string is not a valid integer literal, we allow user to edit some more
            return

        if oldValue == newValue:
            return

        cmd = undo.PropertyEditCommand(self.visual, self.activeImageEntry.name, propertyName, oldValue, newValue)
        self.visual.tabbedEditor.undoStack.push(cmd)

    def metaslot_propertyChangedString(self, propertyName, newValue):
        if not self.activeImageEntry:
            return

        oldValue = getattr(self.activeImageEntry, propertyName)

        if oldValue == newValue:
            return

        cmd = undo.PropertyEditCommand(self.visual, self.activeImageEntry.name, propertyName, oldValue, newValue)
        self.visual.tabbedEditor.undoStack.push(cmd)

    def slot_positionXChanged(self, text):
        self.metaslot_propertyChangedInt("xpos", text)

    def slot_positionYChanged(self, text):
        self.metaslot_propertyChangedInt("ypos", text)

    def slot_widthChanged(self, text):
        self.metaslot_propertyChangedInt("width", text)

    def slot_heightChanged(self, text):
        self.metaslot_propertyChangedInt("height", text)

    def slot_offsetXChanged(self, text):
        self.metaslot_propertyChangedInt("xoffset", text)

    def slot_offsetYChanged(self, text):
        self.metaslot_propertyChangedInt("yoffset", text)

    def slot_autoScaledPerImageChanged(self, index):
        if index == 0:
            # first is the "default" / inheriting state
            text = ""
        else:
            text = self.autoScaledPerImage.currentText()

        self.metaslot_propertyChangedString("autoScaled", text)

    def slot_nativeResolutionPerImageEdited(self, newValue):
        oldHorzRes = self.activeImageEntry.nativeHorzRes
        oldVertRes = self.activeImageEntry.nativeVertRes

        newHorzRes = self.nativeHorzResPerImage.text()
        newVertRes = self.nativeVertResPerImage.text()

        if newHorzRes == "":
            newHorzRes = 0
        if newVertRes == "":
            newVertRes = 0

        try:
            newHorzRes = int(newHorzRes)
            newVertRes = int(newVertRes)

        except ValueError:
            return

        if oldHorzRes == newHorzRes and oldVertRes == newVertRes:
            return

        cmd = undo.PropertyEditCommand(self.visual, self.activeImageEntry.name, "nativeRes", (oldHorzRes, oldVertRes), (newHorzRes, newVertRes))
        self.visual.tabbedEditor.undoStack.push(cmd)

class VisualEditing(resizable.GraphicsView, multi.EditMode):
    """This is the "Visual" tab for imageset editing
    """

    def __init__(self, tabbedEditor):
        resizable.GraphicsView.__init__(self)
        multi.EditMode.__init__(self)

        self.wheelZoomEnabled = True
        self.middleButtonDragScrollEnabled = True

        self.lastMousePosition = None

        scene = QtGui.QGraphicsScene()
        self.setScene(scene)

        self.setFocusPolicy(QtCore.Qt.ClickFocus)
        self.setFrameStyle(QtGui.QFrame.NoFrame)

        if settings.getEntry("imageset/visual/partial_updates").value:
            # the commented lines are possible optimisation, I found out that they don't really
            # speed it up in a noticeable way so I commented them out

            #self.setOptimizationFlag(QGraphicsView.DontSavePainterState, True)
            #self.setOptimizationFlag(QGraphicsView.DontAdjustForAntialiasing, True)
            #self.setCacheMode(QGraphicsView.CacheBackground)
            self.setViewportUpdateMode(QtGui.QGraphicsView.MinimalViewportUpdate)
            #self.setRenderHint(QPainter.Antialiasing, False)
            #self.setRenderHint(QPainter.TextAntialiasing, False)
            #self.setRenderHint(QPainter.SmoothPixmapTransform, False)

        else:
            # use OpenGL for view redrawing
            # depending on the platform and hardware this may be faster or slower
            self.setViewport(QtOpenGL.QGLWidget())
            self.setViewportUpdateMode(QtGui.QGraphicsView.FullViewportUpdate)

        self.scene().selectionChanged.connect(self.slot_selectionChanged)

        self.tabbedEditor = tabbedEditor

        self.setDragMode(QtGui.QGraphicsView.RubberBandDrag)
        self.setBackgroundBrush(QtGui.QBrush(QtCore.Qt.lightGray))

        self.imagesetEntry = None

        self.dockWidget = ImagesetEditorDockWidget(self)

        self.setupActions()

    def setupActions(self):
        self.connectionGroup = action.ConnectionGroup(action.ActionManager.instance)

        self.editOffsetsAction = action.getAction("imageset/edit_offsets")
        self.connectionGroup.add(self.editOffsetsAction, receiver = self.slot_toggleEditOffsets, signalName = "toggled")

        self.cycleOverlappingAction = action.getAction("imageset/cycle_overlapping")
        self.connectionGroup.add(self.cycleOverlappingAction, receiver = self.cycleOverlappingImages)

        self.createImageAction = action.getAction("imageset/create_image")
        self.connectionGroup.add(self.createImageAction, receiver = self.createImageAtCursor)

        self.duplicateSelectedImagesAction = action.getAction("imageset/duplicate_image")
        self.connectionGroup.add(self.duplicateSelectedImagesAction, receiver = self.duplicateSelectedImageEntries)

        self.focusImageListFilterBoxAction = action.getAction("imageset/focus_image_list_filter_box")
        self.connectionGroup.add(self.focusImageListFilterBoxAction, receiver = lambda: self.focusImageListFilterBox())

        self.toolBar = QtGui.QToolBar("Imageset")
        self.toolBar.setObjectName("ImagesetToolbar")
        self.toolBar.setIconSize(QtCore.QSize(32, 32))

        self.toolBar.addAction(self.createImageAction)
        self.toolBar.addAction(self.duplicateSelectedImagesAction)
        self.toolBar.addSeparator() # ---------------------------
        self.toolBar.addAction(self.editOffsetsAction)
        self.toolBar.addAction(self.cycleOverlappingAction)

        self.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)

        self.contextMenu = QtGui.QMenu(self)
        self.customContextMenuRequested.connect(self.slot_customContextMenu)

        self.contextMenu.addAction(self.createImageAction)
        self.contextMenu.addAction(self.duplicateSelectedImagesAction)
        self.contextMenu.addAction(action.getAction("all_editors/delete"))
        self.contextMenu.addSeparator() # ----------------------
        self.contextMenu.addAction(self.cycleOverlappingAction)
        self.contextMenu.addSeparator() # ----------------------
        self.contextMenu.addAction(action.getAction("all_editors/zoom_in"))
        self.contextMenu.addAction(action.getAction("all_editors/zoom_out"))
        self.contextMenu.addAction(action.getAction("all_editors/zoom_reset"))
        self.contextMenu.addSeparator() # ----------------------
        self.contextMenu.addAction(self.editOffsetsAction)

    def rebuildEditorMenu(self, editorMenu):
        """Adds actions to the editor menu"""
        # similar to the toolbar, includes the focus filter box action
        editorMenu.addAction(self.createImageAction)
        editorMenu.addAction(self.duplicateSelectedImagesAction)
        editorMenu.addSeparator() # ---------------------------
        editorMenu.addAction(self.cycleOverlappingAction)
        editorMenu.addSeparator() # ---------------------------
        editorMenu.addAction(self.editOffsetsAction)
        editorMenu.addSeparator() # ---------------------------
        editorMenu.addAction(self.focusImageListFilterBoxAction)

    def initialise(self, rootElement):
        self.loadImagesetEntryFromElement(rootElement)

    def refreshSceneRect(self):
        boundingRect = self.imagesetEntry.boundingRect()

        # the reason to make the bounding rect 100px bigger on all the sides is to make
        # middle button drag scrolling easier (you can put the image where you want without
        # running out of scene

        boundingRect.adjust(-100, -100, 100, 100)
        self.scene().setSceneRect(boundingRect)

    def loadImagesetEntryFromElement(self, element):
        self.scene().clear()

        self.imagesetEntry = elements.ImagesetEntry(self)
        self.imagesetEntry.loadFromElement(element)
        self.scene().addItem(self.imagesetEntry)

        self.refreshSceneRect()

        self.dockWidget.setImagesetEntry(self.imagesetEntry)
        self.dockWidget.refresh()

    def moveImageEntries(self, imageEntries, delta):
        if delta.manhattanLength() > 0 and len(imageEntries) > 0:
            imageNames = []
            oldPositions = {}
            newPositions = {}

            for imageEntry in imageEntries:
                imageNames.append(imageEntry.name)
                oldPositions[imageEntry.name] = imageEntry.pos()
                newPositions[imageEntry.name] = imageEntry.pos() + delta

            cmd = undo.MoveCommand(self, imageNames, oldPositions, newPositions)
            self.tabbedEditor.undoStack.push(cmd)

            # we handled this
            return True

        # we didn't handle this
        return False

    def resizeImageEntries(self, imageEntries, topLeftDelta, bottomRightDelta):
        if (topLeftDelta.manhattanLength() > 0 or bottomRightDelta.manhattanLength() > 0) and len(imageEntries) > 0:
            imageNames = []
            oldPositions = {}
            oldRects = {}
            newPositions = {}
            newRects = {}

            for imageEntry in imageEntries:

                imageNames.append(imageEntry.name)
                oldPositions[imageEntry.name] = imageEntry.pos()
                newPositions[imageEntry.name] = imageEntry.pos() - topLeftDelta
                oldRects[imageEntry.name] = imageEntry.rect()

                newRect = imageEntry.rect()
                newRect.setBottomRight(newRect.bottomRight() - topLeftDelta + bottomRightDelta)

                if newRect.width() < 1:
                    newRect.setWidth(1)
                if newRect.height() < 1:
                    newRect.setHeight(1)

                newRects[imageEntry.name] = newRect

            cmd = undo.GeometryChangeCommand(self, imageNames, oldPositions, oldRects, newPositions, newRects)
            self.tabbedEditor.undoStack.push(cmd)

            # we handled this
            return True

        # we didn't handle this
        return False

    def cycleOverlappingImages(self):
        selection = self.scene().selectedItems()

        if len(selection) == 1:
            rect = selection[0].boundingRect()
            rect.translate(selection[0].pos())

            overlappingItems = self.scene().items(rect)

            # first we stack everything before our current selection
            successor = None
            for item in overlappingItems:
                if item == selection[0] or item.parentItem() != selection[0].parentItem():
                    continue

                if not successor and isinstance(item, elements.ImageEntry):
                    successor = item

            if successor:
                for item in overlappingItems:
                    if item == successor or item.parentItem() != successor.parentItem():
                        continue

                    successor.stackBefore(item)

                # we deselect current
                selection[0].setSelected(False)
                selection[0].hoverLeaveEvent(None)
                # and select what was at the bottom (thus getting this to the top)
                successor.setSelected(True)
                successor.hoverEnterEvent(None)

            # we handled this
            return True

        # we didn't handle this
        return False

    def createImage(self, centrePositionX, centrePositionY):
        """Centre position is the position of the centre of the newly created image,
        the newly created image will then 'encapsulate' the centrepoint
        """

        # find a unique image name
        name = "NewImage"
        index = 1

        while True:
            found = False
            for imageEntry in self.imagesetEntry.imageEntries:
                if imageEntry.name == name:
                    found = True
                    break

            if found:
                name = "NewImage_%i" % (index)
                index += 1
            else:
                break

        halfSize = 25

        xpos = centrePositionX - halfSize
        ypos = centrePositionY - halfSize
        width = 2 * halfSize
        height = 2 * halfSize
        xoffset = 0
        yoffset = 0

        cmd = undo.CreateCommand(self, name, xpos, ypos, width, height, xoffset, yoffset)
        self.tabbedEditor.undoStack.push(cmd)

    def createImageAtCursor(self):
        assert(self.lastMousePosition is not None)
        sceneCoordinates = self.mapToScene(self.lastMousePosition)

        self.createImage(int(sceneCoordinates.x()), int(sceneCoordinates.y()))

    def getImageByName(self, name):
        for imageEntry in self.imagesetEntry.imageEntries:
            if imageEntry.name == name:
                return imageEntry
        return None

    def getNewImageName(self, desiredName, copyPrefix = "", copySuffix = "_copy"):
        """Returns an image name that is not used in this imageset

        TODO: Can be used for the copy-paste functionality too
        """

        # Try the desired name exactly
        if self.getImageByName(desiredName) is None:
            return desiredName

        # Try with prefix and suffix
        desiredName = copyPrefix + desiredName + copySuffix
        if self.getImageByName(desiredName) is None:
            return desiredName

        # We're forced to append a counter, start with number 2 (_copy2, copy3, etc.)
        counter = 2
        while True:
            tmpName = desiredName + str(counter)
            if self.getImageByName(tmpName) is None:
                return tmpName
            counter += 1

    def duplicateImageEntries(self, imageEntries):
        if len(imageEntries) > 0:
            newNames = []

            newPositions = {}
            newRects = {}
            newOffsets = {}

            for imageEntry in imageEntries:
                newName = self.getNewImageName(imageEntry.name)
                newNames.append(newName)

                newPositions[newName] = imageEntry.pos()
                newRects[newName] = imageEntry.rect()
                newOffsets[newName] = imageEntry.offset.pos()

            cmd = undo.DuplicateCommand(self, newNames, newPositions, newRects, newOffsets)
            self.tabbedEditor.undoStack.push(cmd)

            return True

        else:
            # we didn't handle this
            return False

    def duplicateSelectedImageEntries(self):
        selection = self.scene().selectedItems()

        imageEntries = []
        for item in selection:
            if isinstance(item, elements.ImageEntry):
                imageEntries.append(item)

        return self.duplicateImageEntries(imageEntries)

    def deleteImageEntries(self, imageEntries):
        if len(imageEntries) > 0:
            oldNames = []

            oldPositions = {}
            oldRects = {}
            oldOffsets = {}

            for imageEntry in imageEntries:
                oldNames.append(imageEntry.name)

                oldPositions[imageEntry.name] = imageEntry.pos()
                oldRects[imageEntry.name] = imageEntry.rect()
                oldOffsets[imageEntry.name] = imageEntry.offset.pos()

            cmd = undo.DeleteCommand(self, oldNames, oldPositions, oldRects, oldOffsets)
            self.tabbedEditor.undoStack.push(cmd)

            return True

        else:
            # we didn't handle this
            return False

    def deleteSelectedImageEntries(self):
        selection = self.scene().selectedItems()

        imageEntries = []
        for item in selection:
            if isinstance(item, elements.ImageEntry):
                imageEntries.append(item)

        return self.deleteImageEntries(imageEntries)

    def showEvent(self, event):
        self.dockWidget.setEnabled(True)
        self.toolBar.setEnabled(True)
        if self.tabbedEditor.editorMenu() is not None:
            self.tabbedEditor.editorMenu().menuAction().setEnabled(True)

        # connect all our actions
        self.connectionGroup.connectAll()
        # call this every time the visual editing is shown to sync all entries up
        self.slot_toggleEditOffsets(self.editOffsetsAction.isChecked())

        super(VisualEditing, self).showEvent(event)

    def hideEvent(self, event):
        # disconnected all our actions
        self.connectionGroup.disconnectAll()

        self.dockWidget.setEnabled(False)
        self.toolBar.setEnabled(False)
        if self.tabbedEditor.editorMenu() is not None:
            self.tabbedEditor.editorMenu().menuAction().setEnabled(False)

        super(VisualEditing, self).hideEvent(event)

    def mousePressEvent(self, event):
        super(VisualEditing, self).mousePressEvent(event)

        if event.buttons() & QtCore.Qt.LeftButton:
            for selectedItem in self.scene().selectedItems():
                # selectedItem could be ImageEntry or ImageOffset!
                selectedItem.potentialMove = True
                selectedItem.oldPosition = None

    def mouseReleaseEvent(self, event):
        """When mouse is released, we have to check what items were moved and resized.

        AFAIK Qt doesn't give us any move finished notification so I do this manually
        """

        super(VisualEditing, self).mouseReleaseEvent(event)

        # moving
        moveImageNames = []
        moveImageOldPositions = {}
        moveImageNewPositions = {}

        moveOffsetNames = []
        moveOffsetOldPositions = {}
        moveOffsetNewPositions = {}

        # resizing
        resizeImageNames = []
        resizeImageOldPositions = {}
        resizeImageOldRects = {}
        resizeImageNewPositions = {}
        resizeImageNewRects = {}

        # we have to "expand" the items, adding parents of resizing handles
        # instead of the handles themselves
        expandedSelectedItems = []
        for selectedItem in self.scene().selectedItems():
            if isinstance(selectedItem, elements.ImageEntry):
                expandedSelectedItems.append(selectedItem)
            elif isinstance(selectedItem, elements.ImageOffset):
                expandedSelectedItems.append(selectedItem)
            elif isinstance(selectedItem, resizable.ResizingHandle):
                expandedSelectedItems.append(selectedItem.parentItem())

        for selectedItem in expandedSelectedItems:
            if isinstance(selectedItem, elements.ImageEntry):
                if selectedItem.oldPosition:
                    if selectedItem.mouseOver:
                        # show the label again if mouse is over because moving finished
                        selectedItem.label.setVisible(True)

                    # only include that if the position really changed
                    if selectedItem.oldPosition != selectedItem.pos():
                        moveImageNames.append(selectedItem.name)
                        moveImageOldPositions[selectedItem.name] = selectedItem.oldPosition
                        moveImageNewPositions[selectedItem.name] = selectedItem.pos()

                if selectedItem.resized:
                    # only include that if the position or rect really changed
                    if selectedItem.resizeOldPos != selectedItem.pos() or selectedItem.resizeOldRect != selectedItem.rect():
                        resizeImageNames.append(selectedItem.name)
                        resizeImageOldPositions[selectedItem.name] = selectedItem.resizeOldPos
                        resizeImageOldRects[selectedItem.name] = selectedItem.resizeOldRect
                        resizeImageNewPositions[selectedItem.name] = selectedItem.pos()
                        resizeImageNewRects[selectedItem.name] = selectedItem.rect()

                selectedItem.potentialMove = False
                selectedItem.oldPosition = None
                selectedItem.resized = False

            elif isinstance(selectedItem, elements.ImageOffset):
                if selectedItem.oldPosition:
                    # only include that if the position really changed
                    if selectedItem.oldPosition != selectedItem.pos():
                        moveOffsetNames.append(selectedItem.imageEntry.name)
                        moveOffsetOldPositions[selectedItem.imageEntry.name] = selectedItem.oldPosition
                        moveOffsetNewPositions[selectedItem.imageEntry.name] = selectedItem.pos()

                selectedItem.potentialMove = False
                selectedItem.oldPosition = None

        # NOTE: It should never happen that more than one of these sets is populated
        #       User moves images XOR moves offsets XOR resizes images
        #
        #       I don't do elif for robustness though, who knows what can happen ;-)

        if len(moveImageNames) > 0:
            cmd = undo.MoveCommand(self, moveImageNames, moveImageOldPositions, moveImageNewPositions)
            self.tabbedEditor.undoStack.push(cmd)

        if len(moveOffsetNames) > 0:
            cmd = undo.OffsetMoveCommand(self, moveOffsetNames, moveOffsetOldPositions, moveOffsetNewPositions)
            self.tabbedEditor.undoStack.push(cmd)

        if len(resizeImageNames) > 0:
            cmd = undo.GeometryChangeCommand(self, resizeImageNames, resizeImageOldPositions, resizeImageOldRects, resizeImageNewPositions, resizeImageNewRects)
            self.tabbedEditor.undoStack.push(cmd)

    def mouseMoveEvent(self, event):
        self.lastMousePosition = event.pos()

        super(VisualEditing, self).mouseMoveEvent(event)

    def keyReleaseEvent(self, event):
        # TODO: offset keyboard handling

        handled = False

        if event.key() in [QtCore.Qt.Key_A, QtCore.Qt.Key_D, QtCore.Qt.Key_W, QtCore.Qt.Key_S]:
            selection = []

            for item in self.scene().selectedItems():
                if item in selection:
                    continue

                if isinstance(item, elements.ImageEntry):
                    selection.append(item)

                elif isinstance(item, resizable.ResizingHandle):
                    parent = item.parentItem()
                    if not parent in selection:
                        selection.append(parent)

            if len(selection) > 0:
                delta = QtCore.QPointF()

                if event.key() == QtCore.Qt.Key_A:
                    delta += QtCore.QPointF(-1, 0)
                elif event.key() == QtCore.Qt.Key_D:
                    delta += QtCore.QPointF(1, 0)
                elif event.key() == QtCore.Qt.Key_W:
                    delta += QtCore.QPointF(0, -1)
                elif event.key() == QtCore.Qt.Key_S:
                    delta += QtCore.QPointF(0, 1)

                if event.modifiers() & QtCore.Qt.ControlModifier:
                    delta *= 10

                if event.modifiers() & QtCore.Qt.ShiftModifier:
                    handled = self.resizeImageEntries(selection, QtCore.QPointF(0, 0), delta)
                else:
                    handled = self.moveImageEntries(selection, delta)

        elif event.key() == QtCore.Qt.Key_Q:
            handled = self.cycleOverlappingImages()

        elif event.key() == QtCore.Qt.Key_Delete:
            handled = self.deleteSelectedImageEntries()

        if not handled:
            super(VisualEditing, self).keyReleaseEvent(event)

        else:
            event.accept()

    def slot_selectionChanged(self):
        # if dockWidget is changing the selection, back off
        if self.dockWidget.selectionUnderway:
            return

        selectedItems = self.scene().selectedItems()
        if len(selectedItems) == 1:
            if isinstance(selectedItems[0], elements.ImageEntry):
                self.dockWidget.list.scrollToItem(selectedItems[0].listItem)

    def slot_toggleEditOffsets(self, enabled):
        self.scene().clearSelection()

        if self.imagesetEntry is not None:
            self.imagesetEntry.showOffsets = enabled

    def slot_customContextMenu(self, point):
        self.contextMenu.exec_(self.mapToGlobal(point))

    def focusImageListFilterBox(self):
        """Focuses into image list filter

        This potentially allows the user to just press a shortcut to find images,
        instead of having to reach for a mouse.
        """

        filterBox = self.dockWidget.filterBox
        # selects all contents of the filter so that user can replace that with their search phrase
        filterBox.selectAll()
        # sets focus so that typing puts text into the filter box without clicking
        filterBox.setFocus()

    def performCut(self):
        if self.performCopy():
            self.deleteSelectedImageEntries()
            return True

        return False

    def performCopy(self):
        selection = self.scene().selectedItems()
        if len(selection) == 0:
            return False

        copyNames = []
        copyPositions = {}
        copyRects = {}
        copyOffsets = {}
        for item in selection:
            if isinstance(item, elements.ImageEntry):
                copyNames.append(item.name)
                copyPositions[item.name] = item.pos()
                copyRects    [item.name] = item.rect()
                copyOffsets  [item.name] = item.offset.pos()
        if len(copyNames) == 0:
            return False

        imageData = []
        imageData.append(copyNames)
        imageData.append(copyPositions)
        imageData.append(copyRects)
        imageData.append(copyOffsets)

        data = QtCore.QMimeData()
        data.setData("application/x-ceed-imageset-image-list", QtCore.QByteArray(cPickle.dumps(imageData)))
        QtGui.QApplication.clipboard().setMimeData(data)

        return True

    def performPaste(self):
        data = QtGui.QApplication.clipboard().mimeData()
        if not data.hasFormat("application/x-ceed-imageset-image-list"):
            return False

        imageData = cPickle.loads(data.data("application/x-ceed-imageset-image-list").data())
        if len(imageData) != 4:
            return False

        newNames = []
        newPositions = {}
        newRects = {}
        newOffsets = {}
        for copyName in imageData[0]:
            newName = self.getNewImageName(copyName)
            newNames.append(newName)
            newPositions[newName] = imageData[1][copyName]
            newRects    [newName] = imageData[2][copyName]
            newOffsets  [newName] = imageData[3][copyName]
        if len(newNames) == 0:
            return False

        cmd = undo.PasteCommand(self, newNames, newPositions, newRects, newOffsets)
        self.tabbedEditor.undoStack.push(cmd)

        # select just the pasted image definitions for convenience
        self.scene().clearSelection()
        for name in newNames:
            self.imagesetEntry.getImageEntry(name).setSelected(True)

        return True

    def performDelete(self):
        return self.deleteSelectedImageEntries()

from ceed import action
from ceed import settings
from ceed import mainwindow
