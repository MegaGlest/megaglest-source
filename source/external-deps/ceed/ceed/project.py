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

"""Implements project file data model and GUI for interacting with it
"""

from PySide import QtCore
from PySide import QtGui

import os
import sys
import logging

from xml.etree import cElementTree as ElementTree

from ceed import compatibility
from ceed import propertymapping
from ceed import qtwidgets
import ceed.compatibility.project as project_compatibility
from ceed import xmledit

import ceed.ui.projectmanager
import ceed.ui.newprojectdialog
import ceed.ui.projectsettingsdialog

# TODO: Should probably be moved somewhere else because it's reusable
def convertToPortablePath(inputPath):
    # very crude and basic for now
    return unicode(os.path.normpath(inputPath)).replace("\\", "/")

class Item(QtGui.QStandardItem):
    """One item in the project
    This is usually a file or a folder
    """

    Unknown = 1
    # A file is an item that can't have any children, it is directly opened instead of
    # being expanded/collapsed like folders
    File = 2
    # Folder is a group of files. Project folders don't necessarily have to have
    # a counterpart on the HDD, they could be virtual.
    Folder = 3

    itemType = property(lambda self: self.data(QtCore.Qt.UserRole + 1),
                        lambda self, value: self.setItemType(value))

    label = property(lambda self: self.text(),
                     lambda self, value: self.setText(value))

    icon = property(lambda self: self.icon(),
                    lambda self, value: self.setIcon(QtGui.QIcon(value)))

    # following properties are only applicable to files

    # path is the path relative to project's base dir
    path = property(lambda self: self.getPath(),
                    lambda self, value: self.setPath(value))
    # only applicable to folders
    name = property(lambda self: self.getName(),
                    lambda self, value: self.setName(value))

    def __init__(self, project):
        super(Item, self).__init__()

        self.project = project
        self.itemType = Item.Unknown

        self.project.changed = True

    def type(self):
        # Qt docs say we have to overload type() and return something > QStandardItem.UserType
        return QtGui.QStandardItem.UserType + 1

    def clone(self):
        ret = Item(self.project)
        ret.itemType = self.itemType

        if self.itemType == Item.File:
            ret.path = self.path

        elif self.itemType == Item.Folder:
            ret.name = self.name

        else:
            pass

        return ret

    def setItemType(self, value):
        self.setData(value, QtCore.Qt.UserRole + 1)

        if value == Item.File:
            # we can drag files but we can't drop anything to them
            self.setDragEnabled(True)
            self.setDropEnabled(False)

        elif value == Item.Folder:
            # we can drag folders and drop other items to them
            self.setDragEnabled(True)
            self.setDropEnabled(True)

        else:
            # in the unknown case, lets disable both
            self.setDragEnabled(False)
            self.setDropEnabled(False)

    def setPath(self, value):
        assert(self.itemType == Item.File)

        self.setData(value, QtCore.Qt.UserRole + 2)
        self.label = os.path.basename(value)

        # TODO: File type icons are completely independent from tabbed editors,
        #       do we want to couple them or not?
        #
        #       Project management right now has no idea about mainwindow
        #       or tabbed editing (and that's a good thing IMO)

        fileType = "unknown"
        if value.endswith(".font"):
            fileType = "font"
        elif value.endswith(".layout"):
            fileType = "layout"
        elif value.endswith(".imageset"):
            fileType = "imageset"
        elif value.endswith(".anim"):
            fileType = "animation"
        elif value.endswith(".scheme"):
            fileType = "scheme"
        elif value.endswith(".looknfeel"):
            fileType = "looknfeel"
        elif value.endswith(".py"):
            fileType = "python_script"
        elif value.endswith(".lua"):
            fileType = "lua_script"
        elif value.endswith(".xml"):
            fileType = "xml"
        elif value.endswith(".txt"):
            fileType = "text"
        else:
            extensions = [".png", ".jpg", ".jpeg", ".tga", ".dds"]

            for extension in extensions:
                if value.endswith(extension):
                    fileType = "bitmap"
                    break

        self.icon = "icons/project_items/%s.png" % (fileType)

        self.project.changed = True

    def getPath(self):
        assert(self.itemType == Item.File)

        return self.data(QtCore.Qt.UserRole + 2)

    def setName(self, value):
        assert(self.itemType == Item.Folder)

        self.setData(value, QtCore.Qt.UserRole + 2)

        # A hack to cause folders appear first when sorted
        # TODO: Override the sorting method and make this work more cleanly
        self.label = " %s" % (value)
        self.icon = "icons/project_items/folder.png"

        self.project.changed = True

    def getName(self):
        assert(self.itemType == Item.Folder)

        return self.data(QtCore.Qt.UserRole + 2)

    def getRelativePath(self):
        """Returns path relative to the projects base directory"""
        assert(self.itemType == Item.File)

        return self.path

    def getAbsolutePath(self):
        assert(self.itemType == Item.File)

        return self.project.getAbsolutePathOf(self.path)

    @staticmethod
    def loadFromElement(project, element):
        item = Item(project)

        typeString = element.get("type")
        if typeString == "file":
            item.itemType = Item.File
            item.path = os.path.normpath(element.get("path"))

        elif typeString == "folder":
            item.itemType = Item.Folder
            item.name = element.get("name")

            subItemElements = element.findall("Item")
            for subItemElement in subItemElements:
                subItem = Item.loadFromElement(project, subItemElement)
                item.appendRow(subItem)

        else:
            raise Exception("Unknown item type '%s'" % (type))

        return item

    def saveToElement(self):
        ret = ElementTree.Element("Item")

        if self.itemType == Item.File:
            ret.set("type", "file")
            ret.set("path", convertToPortablePath(self.path))

        elif self.itemType == Item.Folder:
            ret.set("type", "folder")
            ret.set("name", self.name)

            i = 0
            while i < self.rowCount():
                subItemElement = self.child(i).saveToElement()
                ret.append(subItemElement)
                i = i + 1

        return ret

    def referencesFilePath(self, filePath):
        """Checks whether given absolute path is referenced by this Item
        or any of its descendants"""

        if self.itemType == Item.File:
            # samefile isn't implemented on Windows and possibly other platforms
            if hasattr(os.path, "samefile"):
                return os.path.samefile(self.getAbsolutePath(), filePath)

            else:
                # Figuring out whether 2 paths lead to the same file is a tricky
                # business. We will do our best but this definitely doesn't
                # work in all cases!

                thisPath = os.path.normpath(self.getAbsolutePath())
                otherPath = os.path.normpath(filePath)

                # This clearly is just our best guess! The file might actually
                # be on a case sensitive filesystem AND have a sibling that has
                # the same name except uppercase, the test will fail in that case.
                thisPathCaseInsensitive = os.path.exists(unicode(thisPath).upper()) and \
                                          os.path.exists(unicode(thisPath).lower())

                otherPathCaseInsensitive = os.path.exists(unicode(otherPath).upper()) and \
                                           os.path.exists(unicode(otherPath).lower())

                if thisPathCaseInsensitive != otherPathCaseInsensitive:
                    return False

                if thisPathCaseInsensitive: # the other will also be case insensitive
                    thisPath = unicode(thisPath).lower()
                    otherPath = unicode(thisPath).lower()

                return thisPath == otherPath

        elif self.itemType == Item.Folder:
            i = 0
            while i < self.rowCount():
                subItemElement = self.child(i)
                if subItemElement.referencesFilePath(filePath):
                    return True

                i += 1

            return False

        else:
            # Unknown Item
            return False

class Project(QtGui.QStandardItemModel):
    """This class encapsulates a project edited by the editor

    A project is basically a set of files and folders that are CEGUI related
    (.font, .layout, ...)
    """

    def __init__(self):
        super(Project, self).__init__()

        self.setHorizontalHeaderLabels(["Name"])
        self.setSupportedDragActions(QtCore.Qt.MoveAction)
        self.prototype = Item(self)
        self.setItemPrototype(self.prototype)

        self.projectFilePath = ""

        self.baseDirectory = "./"

        # default to the best case, native version :-)
        self.CEGUIVersion = compatibility.EditorEmbeddedCEGUIVersion
        # 720p seems like a decent default nowadays, 16:9
        self.CEGUIDefaultResolution = "1280x720"

        self.imagesetsPath = "./imagesets"
        self.fontsPath = "./fonts"
        self.looknfeelsPath = "./looknfeel"
        self.schemesPath = "./schemes"
        self.layoutsPath = "./layouts"
        self.xmlSchemasPath = "./xml_schemas"

        self.changed = True

        pmappings = set([ "mappings/Base.pmappings" ])
        self.propertyMap = propertymapping.PropertyMap.fromFiles([os.path.abspath(path) for path in pmappings])

    def getSupportedDropActions(self):
        return QtCore.Qt.MoveAction

    def load(self, path):
        """Loads XML project file from given path (preferably absolute path)"""

        rawData = open(path, "r").read()
        nativeData = project_compatibility.manager.transformTo(project_compatibility.manager.EditorNativeType, rawData, path)

        root = ElementTree.fromstring(nativeData)

        self.baseDirectory = os.path.normpath(root.get("baseDirectory", "./"))
        self.CEGUIVersion = root.get("CEGUIVersion", compatibility.EditorEmbeddedCEGUIVersion)
        self.CEGUIDefaultResolution = root.get("CEGUIDefaultResolution", "1280x720")

        self.imagesetsPath = os.path.normpath(root.get("imagesetsPath", "./imagesets"))
        self.fontsPath = os.path.normpath(root.get("fontsPath", "./fonts"))
        self.looknfeelsPath = os.path.normpath(root.get("looknfeelsPath", "./looknfeel"))
        self.schemesPath = os.path.normpath(root.get("schemesPath", "./schemes"))
        self.layoutsPath = os.path.normpath(root.get("layoutsPath", "./layouts"))
        self.xmlSchemasPath = os.path.normpath(root.get("xmlSchemasPath", "./xml_schemas"))

        items = root.find("Items")

        for itemElement in items.findall("Item"):
            item = Item.loadFromElement(self, itemElement)
            self.appendRow(item)

        self.changed = False
        self.projectFilePath = path

    def unload(self):
        pass

    def save(self, path = ""):
        if path == "":
            # "save" vs "save as"
            path = self.projectFilePath
            self.changed = False

        root = ElementTree.Element("Project")

        root.set("version", project_compatibility.manager.EditorNativeType)

        root.set("baseDirectory", convertToPortablePath(self.baseDirectory))

        root.set("CEGUIVersion", self.CEGUIVersion)
        root.set("CEGUIDefaultResolution", self.CEGUIDefaultResolution)

        root.set("imagesetsPath", convertToPortablePath(self.imagesetsPath))
        root.set("fontsPath", convertToPortablePath(self.fontsPath))
        root.set("looknfeelsPath", convertToPortablePath(self.looknfeelsPath))
        root.set("schemesPath", convertToPortablePath(self.schemesPath))
        root.set("layoutsPath", convertToPortablePath(self.layoutsPath))
        root.set("xmlSchemasPath", convertToPortablePath(self.xmlSchemasPath))

        items = ElementTree.SubElement(root, "Items")

        i = 0
        while i < self.rowCount():
            items.append(self.item(i).saveToElement())
            i = i + 1

        xmledit.indent(root)

        nativeData = ElementTree.tostring(root, encoding = "utf-8")
        outputFile = open(path, "w")
        outputFile.write("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n")
        outputFile.write(nativeData)
        outputFile.close()

    def hasChanges(self):
        return self.changed

    def getAbsolutePathOf(self, path):
        """Converts project relative paths to absolute paths"""

        absoluteBaseDirectory = os.path.join(os.path.dirname(self.projectFilePath), self.baseDirectory)
        return os.path.normpath(os.path.join(absoluteBaseDirectory, path))

    def getRelativePathOf(self, path):
        return os.path.normpath(os.path.relpath(path, os.path.join(os.path.abspath(os.path.dirname(self.projectFilePath)), self.baseDirectory)))

    def getResourceFilePath(self, filename, resourceGroup):
        # FIXME: The whole resource provider wrapping should be done proper, see http://www.cegui.org.uk/mantis/view.php?id=552
        folder = ""
        if resourceGroup == "imagesets":
            folder = self.imagesetsPath
        elif resourceGroup == "fonts":
            folder = self.fontsPath
        elif resourceGroup == "looknfeels":
            folder = self.looknfeelsPath
        elif resourceGroup == "schemes":
            folder = self.schemesPath
        elif resourceGroup == "layouts":
            folder = self.layoutsPath
        elif resourceGroup == "xml_schemas":
            folder = self.xmlSchemasPath
        else:
            raise RuntimeError("Unknown resource group '%s'" % (resourceGroup))

        return self.getAbsolutePathOf(os.path.join(folder, filename))

    def checkAllDirectories(self):
        """Performs base directory and resource directories sanity check,
        raises IOError in case of a failure
        """

        # check the base directory
        if not os.path.isdir(self.getAbsolutePathOf("")):
            raise IOError("Base directory '%s' isn't a directory or isn't accessible." % (self.getAbsolutePathOf("")))

        for resourceCategory in ["imagesets", "fonts", "looknfeels", "schemes", "layouts", "xml_schemas"]:
            directoryPath = self.getResourceFilePath("", resourceCategory)
            if not os.path.isdir(directoryPath):
                raise IOError("Resource directory '%s' for resources of type '%s' isn't a directory or isn't accessible" % (directoryPath, resourceCategory))

    def referencesFilePath(self, filePath):
        """Checks whether given absolute path is referenced by any File item
        in the project"""

        i = 0
        while i < self.rowCount():
            item = self.item(i)

            if item.referencesFilePath(filePath):
                return True

            i += 1

        return False

class ProjectManager(QtGui.QDockWidget):
    """This is basically a view of the Project model class,
    it allows browsing and (in the future) changes
    """

    project = property(lambda self: self.view.model(),
                       lambda self, value: self.setProject(value))

    fileOpenRequested = QtCore.Signal(str)

    def __init__(self):
        super(ProjectManager, self).__init__()

        self.ui = ceed.ui.projectmanager.Ui_ProjectManager()
        self.ui.setupUi(self)

        self.view = self.findChild(QtGui.QTreeView, "view")
        self.view.sortByColumn(0, QtCore.Qt.AscendingOrder)
        self.view.doubleClicked.connect(self.slot_itemDoubleClicked)

        self.setupContextMenu()

        self.setProject(None)

    def setupContextMenu(self):
        self.view.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)

        self.contextMenu = QtGui.QMenu(self)

        self.createFolderAction = QtGui.QAction(QtGui.QIcon("icons/project_management/create_folder.png"), "Create folder", self)
        self.contextMenu.addAction(self.createFolderAction)
        self.createFolderAction.triggered.connect(self.slot_createFolder)

        self.contextMenu.addSeparator()

        self.addNewFileAction = QtGui.QAction(QtGui.QIcon("icons/project_management/add_new_file.png"), "Add new file", self)
        self.contextMenu.addAction(self.addNewFileAction)
        self.addNewFileAction.triggered.connect(self.slot_addNewFile)

        self.addExistingFileAction = QtGui.QAction(QtGui.QIcon("icons/project_management/add_existing_file.png"), "Add existing file(s)", self)
        self.contextMenu.addAction(self.addExistingFileAction)
        self.addExistingFileAction.triggered.connect(self.slot_addExistingFile)

        self.contextMenu.addSeparator()

        self.renameAction = QtGui.QAction(QtGui.QIcon("icons/project_management/rename.png"), "Rename file/folder", self)
        self.contextMenu.addAction(self.renameAction)
        self.renameAction.triggered.connect(self.slot_renameAction)

        self.removeAction = QtGui.QAction(QtGui.QIcon("icons/project_management/remove.png"), "Remove file(s)/folder(s)", self)
        self.contextMenu.addAction(self.removeAction)
        self.removeAction.triggered.connect(self.slot_removeAction)

        self.view.customContextMenuRequested.connect(self.slot_customContextMenu)

    def setProject(self, project):
        self.setEnabled(project is not None)

        self.view.setModel(project)

    @staticmethod
    def getItemFromModelIndex(modelIndex):
        # todo: is this ugly thing really the way to do this?

        # Qt says in the docs that returned parent is never None but can be invalid if it's the root
        if not modelIndex.parent().isValid():
            # if it's invalid, we can use the indices as absolute model top level indices
            return modelIndex.model().item(modelIndex.row(), modelIndex.column())
        else:
            # otherwise, resort to recursion
            return ProjectManager.getItemFromModelIndex(modelIndex.parent()).child(modelIndex.row(), modelIndex.column())

    def slot_itemDoubleClicked(self, modelIndex):
        if not modelIndex.model():
            return

        item = ProjectManager.getItemFromModelIndex(modelIndex)
        if item.itemType == Item.File: # only react to files, expanding folders is handled by Qt
            self.fileOpenRequested.emit(item.getAbsolutePath())

    def slot_customContextMenu(self, point):
        if self.isEnabled():
            # Qt fails at English a bit?
            selectedIndices = self.view.selectedIndexes()
            # </grammar-nazi-mode>

            # we set everything to disabled and then enable what's relevant
            self.createFolderAction.setEnabled(False)
            self.addNewFileAction.setEnabled(False)
            self.addExistingFileAction.setEnabled(False)
            self.renameAction.setEnabled(False)
            self.removeAction.setEnabled(False)

            if len(selectedIndices) == 0:
                # create root folder
                self.createFolderAction.setEnabled(True)
                self.addNewFileAction.setEnabled(True)
                self.addExistingFileAction.setEnabled(True)

            elif len(selectedIndices) == 1:
                index = selectedIndices[0]
                item = self.getItemFromModelIndex(index)

                if item.itemType == Item.Folder:
                    # create folder inside folder
                    self.createFolderAction.setEnabled(True)
                    self.addNewFileAction.setEnabled(True)
                    self.addExistingFileAction.setEnabled(True)

                self.renameAction.setEnabled(True)
                self.removeAction.setEnabled(True)

            else:
                # more than 1 selected item
                self.removeAction.setEnabled(True)

                #for index in selectedIndices:
                #    item = self.getItemFromModelIndex(index)

        self.contextMenu.exec_(self.mapToGlobal(point))

    def slot_createFolder(self):
        ## TODO: Name clashes!

        text, ok = QtGui.QInputDialog.getText(self,
                                              "Create a folder (only affects project file)",
                                              "Name",
                                              QtGui.QLineEdit.Normal,
                                              "New folder")

        if ok:
            item = Item(self.project)
            item.itemType = Item.Folder
            item.name = text

            selectedIndices = self.view.selectedIndexes()

            if len(selectedIndices) == 0:
                self.project.appendRow(item)

            else:
                assert(len(selectedIndices) == 1)

                parent = self.getItemFromModelIndex(selectedIndices[0])
                assert(parent.itemType == Item.Folder)

                parent.appendRow(item)

    def slot_addNewFile(self):
        ## TODO: name clashes, duplicates

        file_, _ = QtGui.QFileDialog.getSaveFileName(self,
                                                    "Create a new file and add it to the project",
                                                    self.project.getAbsolutePathOf(""))

        if file_ == "":
            # user cancelled
            return

        try:
            f = open(file_, "w")
            f.close()

        except OSError:
            QtGui.QMessageBox.question(self,
                                       "Can't create file!",
                                       "Creating file '%s' failed. Exception details follow:\n%s" % (file_, sys.exc_info()[1]),
                                       QtGui.QMessageBox.Ok)

            return

        selectedIndices = self.view.selectedIndexes()

        item = Item(self.project)
        item.itemType = Item.File
        item.path = self.project.getRelativePathOf(file_)

        if len(selectedIndices) == 0:
            self.project.appendRow(item)

        else:
            assert(len(selectedIndices) == 1)

            parent = self.getItemFromModelIndex(selectedIndices[0])
            assert(parent.itemType == Item.Folder)

            parent.appendRow(item)

    def slot_addExistingFile(self):
        ## TODO: name clashes, duplicates

        files, _ = QtGui.QFileDialog.getOpenFileNames(self,
                                                           "Select one or more files to add to the project",
                                                           self.project.getAbsolutePathOf(""))
        selectedIndices = self.view.selectedIndexes()

        # lets see if file user wants added isn't already there
        previousResponse = None
        for file_ in files:
            if self.view.model().referencesFilePath(file_):
                # file is already in the project
                response = None

                if previousResponse == QtGui.QMessageBox.YesToAll:
                    response = QtGui.QMessageBox.YesToAll
                elif previousResponse == QtGui.QMessageBox.NoToAll:
                    response = QtGui.QMessageBox.NoToAll
                else:
                    response = QtGui.QMessageBox.question(self, "File is already in the project!",
                            "File '%s' that you are trying to add is already referenced in the "
                            "project.\n\n"
                            "Do you want to add it as a duplicate?" % (file_),
                            QtGui.QMessageBox.Yes | QtGui.QMessageBox.No |
                            QtGui.QMessageBox.YesToAll | QtGui.QMessageBox.NoToAll,
                            QtGui.QMessageBox.Yes)

                    previousResponse = response

                if response in [QtGui.QMessageBox.No, QtGui.QMessageBox.NoToAll]:
                    continue

            item = Item(self.project)
            item.itemType = Item.File
            item.path = self.project.getRelativePathOf(file_)

            if len(selectedIndices) == 0:
                self.project.appendRow(item)

            else:
                assert(len(selectedIndices) == 1)

                parent = self.getItemFromModelIndex(selectedIndices[0])
                assert(parent.itemType == Item.Folder)

                parent.appendRow(item)

    def slot_renameAction(self):
        ## TODO: Name clashes!

        selectedIndices = self.view.selectedIndexes()
        assert(len(selectedIndices) == 1)

        item = self.getItemFromModelIndex(selectedIndices[0])
        if item.itemType == Item.File:
            text, ok = QtGui.QInputDialog.getText(self,
                                                  "Rename file (renames the file on the disk!)",
                                                  "New name",
                                                  QtGui.QLineEdit.Normal,
                                                  os.path.basename(item.path))

            if ok and text != os.path.basename(item.path):
                # legit change
                newPath = os.path.join(os.path.dirname(item.path), text)

                try:
                    os.rename(self.project.getAbsolutePathOf(item.path), self.project.getAbsolutePathOf(newPath))
                    item.path = newPath
                    self.project.changed = True

                except OSError:
                    QtGui.QMessageBox.question(self,
                                               "Can't rename!",
                                               "Renaming file '%s' to '%s' failed. Exception details follow:\n%s" % (item.path, newPath, sys.exc_info()[1]),
                                               QtGui.QMessageBox.Ok)

        elif item.itemType == Item.Folder:
            text, ok = QtGui.QInputDialog.getText(self,
                                                  "Rename folder (only affects the project file)",
                                                  "New name",
                                                  QtGui.QLineEdit.Normal,
                                                  item.name)

            if ok and text != item.name:
                item.name = text
                self.project.changed = True

    def slot_removeAction(self):
        if not self.isEnabled():
            return

        selectedIndices = self.view.selectedIndexes()
        # when this is called the selection must not be empty
        assert(len(selectedIndices) > 0)

        removeSpec = ""
        if len(selectedIndices) == 1:
            item = self.getItemFromModelIndex(selectedIndices[0])
            removeSpec = "'%s'" % (item.label)
        else:
            removeSpec = "%i project items" % (len(selectedIndices))

        # we have changes, lets ask the user whether we should dump them or save them
        result = QtGui.QMessageBox.question(self,
                                            "Remove items?",
                                            "Are you sure you want to remove %s from the project? "
                                            "This action can't be undone! "
                                            "(Pressing Cancel will cancel the operation!)" % (removeSpec),
                                            QtGui.QMessageBox.Yes | QtGui.QMessageBox.Cancel,
                                            QtGui.QMessageBox.Cancel)

        if result == QtGui.QMessageBox.Cancel:
            # user chickened out ;-)
            return

        elif result == QtGui.QMessageBox.Yes:
            selectedIndices = sorted(selectedIndices, key = lambda index: index.row(), reverse = True)
            removeCount = 0

            # we have to remove files first because multi-selection could screw us otherwise
            # (Parent also removes it's children)

            # first remove files
            for index in selectedIndices:
                item = self.getItemFromModelIndex(index)

                if not item:
                    continue

                if (item.itemType == Item.File):
                    index.model().removeRow(index.row(), index.parent())
                    removeCount += 1

            # then remove folders
            for index in selectedIndices:
                item = self.getItemFromModelIndex(index)

                if not item:
                    continue

                if (item.itemType == Item.Folder):
                    index.model().removeRow(index.row(), index.parent())
                    removeCount += 1

            if len(selectedIndices) - removeCount > 0:
                logging.error("%i selected project items are unknown and can't be deleted" % (len(selectedIndices)))

            if removeCount > 0:
                self.project.changed = True

class NewProjectDialog(QtGui.QDialog):
    """Dialog responsible for creation of entirely new projects.
    """

    def __init__(self):
        super(NewProjectDialog, self).__init__()

        self.ui = ceed.ui.newprojectdialog.Ui_NewProjectDialog()
        self.ui.setupUi(self)

        self.projectFilePath = self.findChild(qtwidgets.FileLineEdit, "projectFilePath")
        self.projectFilePath.filter = "Project file (*.project)"
        self.projectFilePath.mode = qtwidgets.FileLineEdit.NewFileMode

        self.createResourceDirs = self.findChild(QtGui.QCheckBox, "createResourceDirs")

    def accept(self):
        if self.projectFilePath.text() == "":
            QtGui.QMessageBox.critical(self, "Project file path empty!", "You must supply a valid project file path!")
            return

        if not os.path.exists(os.path.dirname(self.projectFilePath.text())):
            QtGui.QMessageBox.critical(self, "Project file path invalid!", "Its parent directory ('%s') is inaccessible!" % (os.path.dirname(self.projectFilePath.text())))
            return

        super(NewProjectDialog, self).accept()

    # creates the project using data from this dialog
    def createProject(self):
        ret = Project()
        ret.projectFilePath = self.projectFilePath.text()

        if not ret.projectFilePath.endswith(".project"):
            # enforce the "project" extension
            ret.projectFilePath += ".project"

        if self.createResourceDirs.checkState() == QtCore.Qt.Checked:
            try:
                prefix = os.path.dirname(ret.projectFilePath)
                dirs = ["fonts", "imagesets", "looknfeel", "schemes", "layouts", "xml_schemas"]

                for dir_ in dirs:
                    if not os.path.exists(os.path.join(prefix, dir_)):
                        os.mkdir(os.path.join(prefix, dir_))

            except OSError as e:
                QtGui.QMessageBox.critical(self, "Cannot create resource \
directories!", "There was a problem creating the resource \
directories.  Do you have the proper permissions on the \
parent directory? (exception info: %s)" % (e))

        return ret

class ProjectSettingsDialog(QtGui.QDialog):
    """Dialog able to change various project settings
    """

    def __init__(self, project):
        super(ProjectSettingsDialog, self).__init__()

        self.ui = ceed.ui.projectsettingsdialog.Ui_ProjectSettingsDialog()
        self.ui.setupUi(self)

        self.baseDirectory = self.findChild(qtwidgets.FileLineEdit, "baseDirectory")
        self.baseDirectory.mode = qtwidgets.FileLineEdit.ExistingDirectoryMode

        self.CEGUIVersion = self.findChild(QtGui.QComboBox, "CEGUIVersion")
        for version in compatibility.CEGUIVersions:
            self.CEGUIVersion.addItem(version)

        self.CEGUIVersion.setEditText(project.CEGUIVersion)

        self.CEGUIDefaultResolution = self.findChild(QtGui.QComboBox, "CEGUIDefaultResolution")
        self.CEGUIDefaultResolution.setEditText(project.CEGUIDefaultResolution)

        self.resourceDirectory = self.findChild(qtwidgets.FileLineEdit, "resourceDirectory")
        self.resourceDirectory.mode = qtwidgets.FileLineEdit.ExistingDirectoryMode
        self.resourceDirectoryApplyButton = self.findChild(QtGui.QPushButton, "resourceDirectoryApplyButton")
        self.resourceDirectoryApplyButton.pressed.connect(self.slot_applyResourceDirectory)

        self.imagesetsPath = self.findChild(qtwidgets.FileLineEdit, "imagesetsPath")
        self.imagesetsPath.mode = qtwidgets.FileLineEdit.ExistingDirectoryMode
        self.fontsPath = self.findChild(qtwidgets.FileLineEdit, "fontsPath")
        self.fontsPath.mode = qtwidgets.FileLineEdit.ExistingDirectoryMode
        self.looknfeelsPath = self.findChild(qtwidgets.FileLineEdit, "looknfeelsPath")
        self.looknfeelsPath.mode = qtwidgets.FileLineEdit.ExistingDirectoryMode
        self.schemesPath = self.findChild(qtwidgets.FileLineEdit, "schemesPath")
        self.schemesPath.mode = qtwidgets.FileLineEdit.ExistingDirectoryMode
        self.layoutsPath = self.findChild(qtwidgets.FileLineEdit, "layoutsPath")
        self.layoutsPath.mode = qtwidgets.FileLineEdit.ExistingDirectoryMode
        self.xmlSchemasPath = self.findChild(qtwidgets.FileLineEdit, "xmlSchemasPath")
        self.xmlSchemasPath.mode = qtwidgets.FileLineEdit.ExistingDirectoryMode

        self.baseDirectory.setText(project.getAbsolutePathOf(""))
        self.imagesetsPath.setText(project.getAbsolutePathOf(project.imagesetsPath))
        self.fontsPath.setText(project.getAbsolutePathOf(project.fontsPath))
        self.looknfeelsPath.setText(project.getAbsolutePathOf(project.looknfeelsPath))
        self.schemesPath.setText(project.getAbsolutePathOf(project.schemesPath))
        self.layoutsPath.setText(project.getAbsolutePathOf(project.layoutsPath))
        self.xmlSchemasPath.setText(project.getAbsolutePathOf(project.xmlSchemasPath))

    def apply(self, project):
        """Applies values from this dialog to given project
        """

        absBaseDir = os.path.normpath(os.path.abspath(self.baseDirectory.text()))
        project.baseDirectory = os.path.relpath(absBaseDir, os.path.dirname(project.projectFilePath))

        project.CEGUIVersion = self.CEGUIVersion.currentText()
        project.CEGUIDefaultResolution = self.CEGUIDefaultResolution.currentText()

        project.imagesetsPath = os.path.relpath(self.imagesetsPath.text(), absBaseDir)
        project.fontsPath = os.path.relpath(self.fontsPath.text(), absBaseDir)
        project.looknfeelsPath = os.path.relpath(self.looknfeelsPath.text(), absBaseDir)
        project.schemesPath = os.path.relpath(self.schemesPath.text(), absBaseDir)
        project.layoutsPath = os.path.relpath(self.layoutsPath.text(), absBaseDir)
        project.xmlSchemasPath = os.path.relpath(self.xmlSchemasPath.text(), absBaseDir)

    def slot_applyResourceDirectory(self):
        resourceDir = os.path.normpath(os.path.abspath(self.resourceDirectory.text()))

        self.imagesetsPath.setText(os.path.join(resourceDir, "imagesets"))
        self.fontsPath.setText(os.path.join(resourceDir, "fonts"))
        self.looknfeelsPath.setText(os.path.join(resourceDir, "looknfeel"))
        self.schemesPath.setText(os.path.join(resourceDir, "schemes"))
        self.layoutsPath.setText(os.path.join(resourceDir, "layouts"))
        self.xmlSchemasPath.setText(os.path.join(resourceDir, "xml_schemas"))

