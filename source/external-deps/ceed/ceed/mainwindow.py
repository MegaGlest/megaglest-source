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

import os

from ceed import paths

from ceed import settings
from ceed import action

from ceed import commands
from ceed import project
from ceed import cegui
from ceed import filesystembrowser
import ceed.cegui.container as cegui_container
from ceed import editors

#from ceed import help
from ceed import recentlyused
from ceed import about

import ceed.ui.mainwindow

class MainWindow(QtGui.QMainWindow):
    """The central window of the application.

    This is a singleton class, it is assured to only be constructed once during runtime.
    """

    # singleton pattern implementation
    # (usage: mainwindow.MainWindow.instance.doSomething() where mainwindow is the module)
    instance = None

    # TODO: This class has grown too much, I think it has too many responsibilities
    #       and refactoring will be needed in the future.

    project = property(lambda self: self._project,
                       lambda self, value: self._setProject(value))

    activeEditor = property(lambda self: self._activeEditor,
                            lambda self, value: self._setActiveEditor(value))

    def _setProject(self, value):
        self._project = value
        if self.fileSystemBrowser:
            self.fileSystemBrowser.projectDirectoryButton.setEnabled(True if value else False)

    def _setActiveEditor(self, value):
        self._activeEditor = value
        if self.fileSystemBrowser:
            self.fileSystemBrowser.activeFileDirectoryButton.setEnabled(True if value else False)

    def __init__(self, app):
        # make sure multiple instantiation won't happen
        assert(MainWindow.instance is None)
        MainWindow.instance = self

        super(MainWindow, self).__init__()

        self.app = app

        # whether the app was maximized before going fullscreen
        self.wasMaximized = False

        # we have to construct ActionManager before settings interface (as it alters the settings declaration)!
        self.actionManager = action.ActionManager(self, self.app.settings)

        self.settingsInterface = settings.interface.QtSettingsInterface(self.app.settings)

        self.recentlyUsedProjects = recentlyused.RecentlyUsedMenuEntry(self.app.qsettings, "Projects")
        self.recentlyUsedFiles = recentlyused.RecentlyUsedMenuEntry(self.app.qsettings, "Files")

        import ceed.editors.animation_list as animation_list_editor
        import ceed.editors.bitmap as bitmap_editor
        import ceed.editors.imageset as imageset_editor
        import ceed.editors.layout as layout_editor
        #import ceed.editors.property_mappings as property_mappings_editor
        import ceed.editors.text as text_editor

        self.editorFactories = [
            animation_list_editor.AnimationListTabbedEditorFactory(),
            bitmap_editor.BitmapTabbedEditorFactory(),
            imageset_editor.ImagesetTabbedEditorFactory(),
            layout_editor.LayoutTabbedEditorFactory(),
            #property_mappings_editor.PropertyMappingsTabbedEditorFactory(),
            text_editor.TextTabbedEditorFactory()
        ]
        # File dialog filters, keep indices in sync with the list above
        self.editorFactoryFileFilters = [
            "Animation files (%s)" % ("*." + " *.".join(self.editorFactories[0].getFileExtensions())),
            "Bitmap files (%s)" % ("*." + " *.".join(self.editorFactories[1].getFileExtensions())),
            "Imageset files (%s)" % ("*." + " *.".join(self.editorFactories[2].getFileExtensions())),
            "Layout files (%s)" % ("*." + " *.".join(self.editorFactories[3].getFileExtensions())),
            #"Property Mapping files (%s)" % ("*." + " *.".join(self.editorFactories[4].getFileExtensions())),
            "Text files (%s)" % ("*." + " *.".join(self.editorFactories[4].getFileExtensions()))
        ]
        allExt = []
        for factory in self.editorFactories:
            allExt.extend(factory.getFileExtensions())
        self.editorFactoryFileFilters.insert(0, "All known files (*." + " *.".join(allExt) + ")")
        self.editorFactoryFileFilters.insert(1, "All files (*)")

        self._activeEditor = None
        self._project = None

        self.ui = ceed.ui.mainwindow.Ui_MainWindow()
        self.ui.setupUi(self)

        # for now we can't use unified title and toolbar, it doesn't have toolbar "ellipsis"
        # and that makes the main window jump around when switching tabs
        self.setUnifiedTitleAndToolBarOnMac(False)

        # we start CEGUI early and we always start it
        self.ceguiInstance = cegui.Instance()
        self.ceguiContainerWidget = cegui_container.ContainerWidget(self.ceguiInstance, self)

        self.tabs = self.centralWidget().findChild(QtGui.QTabWidget, "tabs")
        self.tabs.currentChanged.connect(self.slot_currentTabChanged)
        self.tabs.tabCloseRequested.connect(self.slot_tabCloseRequested)
        # TODO: this is potentially nasty since the tabBar method is protected
        # we need this to implement the context menu when you right click on the tab bar
        self.tabBar = self.tabs.tabBar()
        self.tabBar.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.tabBar.customContextMenuRequested.connect(self.slot_tabBarCustomContextMenuRequested)

        # stores all active tab editors
        self.tabEditors = []

        self.projectManager = project.ProjectManager()
        self.projectManager.fileOpenRequested.connect(self.slot_openFile)
        self.addDockWidget(QtCore.Qt.RightDockWidgetArea, self.projectManager)

        self.fileSystemBrowser = filesystembrowser.FileSystemBrowser()
        self.fileSystemBrowser.setVisible(False)
        self.fileSystemBrowser.fileOpenRequested.connect(self.slot_openFile)
        self.addDockWidget(QtCore.Qt.RightDockWidgetArea, self.fileSystemBrowser)

        self.undoViewer = commands.UndoViewer()
        self.undoViewer.setVisible(False)
        self.addDockWidget(QtCore.Qt.RightDockWidgetArea, self.undoViewer)

        #import propertytree
        #test = propertytree.TestDock()
        #self.addDockWidget(Qt.LeftDockWidgetArea, test)

        self.setupActions()
        self.setupMenus()
        self.setupToolbars()

        self.restoreSettings()

    def setupActions(self):
        # usage of a connection group in mainwindow may be unnecessary,
        # we never use disconnectAll and/or connectAll, it is just used as a convenient
        # way to group connections
        self.connectionGroup = action.ConnectionGroup(self.actionManager)

        #
        # get and connect all actions we care about
        #
        self.newFileAction = self.actionManager.getAction("files/new_file")
        self.connectionGroup.add(self.newFileAction, receiver = self.slot_newFileDialog)

        self.newLayoutAction = self.actionManager.getAction("files/new_layout")
        self.connectionGroup.add(self.newLayoutAction, receiver = self.slot_newLayoutDialog)

        self.newImagesetAction = self.actionManager.getAction("files/new_imageset")
        self.connectionGroup.add(self.newImagesetAction, receiver = self.slot_newImagesetDialog)

        self.openFileAction = self.actionManager.getAction("files/open_file")
        self.connectionGroup.add(self.openFileAction, receiver = self.slot_openFileDialog)

        self.saveAction = self.actionManager.getAction("files/save_file")
        self.saveAction.setEnabled(False)
        self.connectionGroup.add(self.saveAction, receiver = self.slot_save)

        self.saveAsAction = self.actionManager.getAction("files/save_file_as")
        self.saveAsAction.setEnabled(False)
        self.connectionGroup.add(self.saveAsAction, receiver = self.slot_saveAs)

        self.saveAllAction = self.actionManager.getAction("files/save_all")
        self.connectionGroup.add(self.saveAllAction, receiver = self.slot_saveAll)

        # tab bar context menu (but also added to the file menu so it's easy to discover)
        self.closeTabAction = self.actionManager.getAction("files/close_current_tab")
        self.closeTabAction.setEnabled(False)
        self.connectionGroup.add(self.closeTabAction, receiver = self.slot_closeTab)

        self.closeOtherTabsAction = self.actionManager.getAction("files/close_other_tabs")
        self.closeOtherTabsAction.setEnabled(False)
        self.connectionGroup.add(self.closeOtherTabsAction, receiver = self.slot_closeOtherTabs)

        self.closeAllTabsAction = self.actionManager.getAction("files/close_all_tabs")
        self.connectionGroup.add(self.closeAllTabsAction, receiver = self.slot_closeAllTabs)
        # end of tab bar context menu

        self.previousTabAction = self.actionManager.getAction("files/previous_tab")
        self.connectionGroup.add(self.previousTabAction, receiver = self.slot_previousTab)

        self.nextTabAction = self.actionManager.getAction("files/next_tab")
        self.connectionGroup.add(self.nextTabAction, receiver = self.slot_nextTab)

        # the clear action will be handled by the RecentlyUsed manager, no need to connect
        self.clearRecentFilesAction = self.actionManager.getAction("files/clear_recent_files")

        # the clear action will be handled by the RecentlyUsed manager, no need to connect
        self.clearRecentProjectsAction = self.actionManager.getAction("files/clear_recent_projects")

        self.undoAction = self.actionManager.getAction("all_editors/undo")
        self.undoAction.setEnabled(False)
        self.connectionGroup.add(self.undoAction, receiver = self.slot_undo)

        self.redoAction = self.actionManager.getAction("all_editors/redo")
        self.redoAction.setEnabled(False)
        self.connectionGroup.add(self.redoAction, receiver = self.slot_redo)

        self.revertAction = self.actionManager.getAction("files/revert_file")
        self.revertAction.setEnabled(False)
        self.connectionGroup.add(self.revertAction, receiver = self.slot_revert)

        self.cutAction = self.actionManager.getAction("all_editors/cut")
        self.connectionGroup.add(self.cutAction, receiver = self.slot_cut)

        self.copyAction = self.actionManager.getAction("all_editors/copy")
        self.connectionGroup.add(self.copyAction, receiver = self.slot_copy)

        self.pasteAction = self.actionManager.getAction("all_editors/paste")
        self.connectionGroup.add(self.pasteAction, receiver = self.slot_paste)

        self.deleteAction = self.actionManager.getAction("all_editors/delete")
        self.connectionGroup.add(self.deleteAction, receiver = self.slot_delete)

        self.projectSettingsAction = self.actionManager.getAction("project_management/project_settings")
        self.projectSettingsAction.setEnabled(False)
        self.connectionGroup.add(self.projectSettingsAction, receiver = self.slot_projectSettings)

        self.projectReloadResourcesAction = self.actionManager.getAction("project_management/reload_resources")
        self.projectReloadResourcesAction.setEnabled(False)
        self.connectionGroup.add(self.projectReloadResourcesAction, receiver = self.slot_projectReloadResources)

        self.preferencesAction = self.actionManager.getAction("general/application_settings")
        self.connectionGroup.add(self.preferencesAction, receiver = self.settingsInterface.show)

        self.newProjectAction = self.actionManager.getAction("project_management/new_project")
        self.connectionGroup.add(self.newProjectAction, receiver = self.slot_newProject)

        self.openProjectAction = self.actionManager.getAction("project_management/open_project")
        self.connectionGroup.add(self.openProjectAction, receiver = self.slot_openProject)

        self.saveProjectAction = self.actionManager.getAction("project_management/save_project")
        self.saveProjectAction.setEnabled(False)
        self.connectionGroup.add(self.saveProjectAction, receiver = self.slot_saveProject)

        self.closeProjectAction = self.actionManager.getAction("project_management/close_project")
        self.closeProjectAction.setEnabled(False)
        self.connectionGroup.add(self.closeProjectAction, receiver = self.slot_closeProject)

        self.quitAction = self.actionManager.getAction("general/quit")
        self.connectionGroup.add(self.quitAction, receiver = self.slot_quit)

        self.helpQuickstartAction = self.actionManager.getAction("general/help_quickstart")
        self.connectionGroup.add(self.helpQuickstartAction, receiver = self.slot_helpQuickstart)

        self.helpUserManualAction = self.actionManager.getAction("general/help_user_manual")
        self.connectionGroup.add(self.helpUserManualAction, receiver = self.slot_helpUserManual)

        self.helpWikiPageAction = self.actionManager.getAction("general/help_wiki_page")
        self.connectionGroup.add(self.helpWikiPageAction, receiver = self.slot_helpWikiPage)

        self.sendFeedbackAction = self.actionManager.getAction("general/send_feedback")
        self.connectionGroup.add(self.sendFeedbackAction, receiver = self.slot_sendFeedback)

        self.reportBugAction = self.actionManager.getAction("general/report_bug")
        self.connectionGroup.add(self.reportBugAction, receiver = self.slot_reportBug)

        self.ceguiDebugInfoAction = self.actionManager.getAction("general/cegui_debug_info")
        self.connectionGroup.add(self.ceguiDebugInfoAction, receiver = self.slot_ceguiDebugInfo)

        self.viewLicenseAction = self.actionManager.getAction("general/view_license")
        self.connectionGroup.add(self.viewLicenseAction, receiver = self.slot_license)

        self.aboutQtAction = self.actionManager.getAction("general/about_qt")
        self.connectionGroup.add(self.aboutQtAction, receiver = QtGui.QApplication.aboutQt)

        self.aboutAction = self.actionManager.getAction("general/about")
        self.connectionGroup.add(self.aboutAction, receiver = self.slot_about)

        self.zoomInAction = self.actionManager.getAction("all_editors/zoom_in")
        self.connectionGroup.add(self.zoomInAction, receiver = self.slot_zoomIn)
        self.zoomOutAction = self.actionManager.getAction("all_editors/zoom_out")
        self.connectionGroup.add(self.zoomOutAction, receiver = self.slot_zoomOut)
        self.zoomResetAction = self.actionManager.getAction("all_editors/zoom_reset")
        self.connectionGroup.add(self.zoomResetAction, receiver = self.slot_zoomReset)

        self.statusbarAction = self.actionManager.getAction("general/statusbar")
        self.statusbarAction.setChecked(True)
        self.connectionGroup.add(self.statusbarAction, receiver = self.slot_toggleStatusbar)

        self.fullScreenAction = self.actionManager.getAction("general/full_screen")
        self.connectionGroup.add(self.fullScreenAction, receiver = self.slot_toggleFullScreen)

        self.connectionGroup.connectAll()

    def setupMenus(self):
        #
        # Construct File menu
        #
        self.fileMenu = QtGui.QMenu("&File")
        self.menuBar().addMenu(self.fileMenu)
        menu = QtGui.QMenu("&New")
        # sorted, generic "file" last
        menu.addActions([self.newImagesetAction, self.newLayoutAction, self.newFileAction])
        menu.addSeparator()
        menu.addAction(self.newProjectAction)
        self.fileMenu.addMenu(menu)
        self.fileNewMenu = menu
        self.fileMenu.addActions([self.openFileAction, self.openProjectAction])
        self.fileMenu.addSeparator()
        self.fileMenu.addActions([self.closeTabAction, self.closeProjectAction])
        self.fileMenu.addSeparator()
        self.fileMenu.addActions([self.saveAction, self.saveAsAction, self.saveProjectAction, self.saveAllAction])
        # TODO: Revert
        #self.fileMenu.addSeparator()
        #self.fileMenu.addAction(self.revertAction)
        self.fileMenu.addSeparator()
        menu = QtGui.QMenu("&Recent Files")
        self.fileMenu.addMenu(menu)
        self.recentlyUsedFiles.setParentMenu(menu, self.slot_openRecentFile, self.clearRecentFilesAction)
        menu = QtGui.QMenu("&Recent Projects")
        self.fileMenu.addMenu(menu)
        self.recentlyUsedProjects.setParentMenu(menu, self.slot_openRecentProject, self.clearRecentProjectsAction)
        self.fileMenu.addSeparator()
        self.fileMenu.addAction(self.quitAction)

        #
        # Construct Edit menu
        #
        self.editMenu = QtGui.QMenu("&Edit")
        self.menuBar().addMenu(self.editMenu)
        self.editMenu.addActions([self.undoAction, self.redoAction])
        self.editMenu.addAction(self.revertAction)
        self.editMenu.addSeparator()
        self.editMenu.addActions([self.cutAction, self.copyAction, self.pasteAction, self.deleteAction])
        self.editMenu.addSeparator()
        self.editMenu.addAction(self.preferencesAction)

        #
        # Construct View menu
        #
        self.viewMenu = QtGui.QMenu("&View")
        self.menuBar().addMenu(self.viewMenu)

        # the first menu item of the view menu is "Docks & Toolbars"
        # this menu contains checkable "actions" that hide and show the panels
        # even though you can achieve the same thing by right clicking empty space in
        # mainwindow I believe having this has a benefit, it is much easier to find this way
        # this menu is created dynamically when the view menu is about to be shown and we
        # rely on the default implementation to create it.
        # the other option would be to create the menu by iterating all docks and toolbars,
        # like: https://qt.gitorious.org/qt/qt/blobs/4.8/src/gui/widgets/qmainwindow.cpp#line1643
        self._docksAndToolbarsMenu = None
        def viewMenuAboutShow():
            # call super to be safe in case we change our own createPopupMenu.
            menu = super(MainWindow, self).createPopupMenu()
            menu.setTitle("&Docks && Toolbars")
            # add before the statusbar action
            self.viewMenu.insertMenu(self.statusbarAction, menu)
            # keep reference so we can remove on hide
            self._docksAndToolbarsMenu = menu
        def viewMenuAboutHide():
            self.viewMenu.removeAction(self._docksAndToolbarsMenu.menuAction())
            self._docksAndToolbarsMenu = None
        self.viewMenu.aboutToShow.connect(viewMenuAboutShow)
        self.viewMenu.aboutToHide.connect(viewMenuAboutHide)

        # add the rest of the view menu items
        self.viewMenu.addAction(self.statusbarAction)
        self.viewMenu.addSeparator()
        self.viewMenu.addActions([self.zoomInAction, self.zoomOutAction, self.zoomResetAction])
        self.viewMenu.addSeparator()
        self.viewMenu.addAction(self.fullScreenAction)

        #
        # Construct Project menu
        #
        self.projectMenu = QtGui.QMenu("&Project")
        self.menuBar().addMenu(self.projectMenu)
        self.projectMenu.addAction(self.projectReloadResourcesAction)
        self.projectMenu.addSeparator()
        self.projectMenu.addAction(self.projectSettingsAction)

        #
        # Construct active editor menu
        # This is disabled & hidden by default
        # and it's managed by the active editor
        #
        self.editorMenu = QtGui.QMenu("EditorMenu")
        self.editorMenu.menuAction().setEnabled(False)
        self.editorMenu.menuAction().setVisible(False)
        self.menuBar().addMenu(self.editorMenu)

        #
        # Construct Tabs menu
        #
        self.tabsMenu = QtGui.QMenu("&Tabs")
        self.menuBar().addMenu(self.tabsMenu)
        self.tabsMenu.addActions([self.previousTabAction, self.nextTabAction])
        self.tabsMenu.addSeparator()
        self.tabsMenu.addActions([self.closeOtherTabsAction, self.closeAllTabsAction])
        # add menu items for open tabs
        self._tabsMenuSeparator = None
        def tabsMenuAboutShow():
            self._tabsMenuSeparator = self.tabsMenu.addSeparator()
            actions = []
            counter = 1
            # the items are taken from the 'self.tabEditors' list
            # which always has the order by which the files were
            # opened (not the order of the tabs in the tab bar).
            # this is a feature, maintains the same mnemonic
            # even if a tab is moved.
            for editor in self.tabEditors:
                # construct the label from the filePath
                name = editor.filePath
                # TODO: the next few lines are basically the
                # same as the code in recentlyused. refactor
                # so both places use the same (generic) code.

                # if name length greater than some value, trim
                if len(name) > 40:
                    name = "...%s" % (name[-40 + 3:])
                # if the first 10 tabs, add mnemonic (1 to 9, then 0)
                if counter <= 10:
                    name = "&%i. %s" % (counter % 10, name)
                counter += 1
                # create the action object
                action = QtGui.QAction(name, self.tabsMenu)
                action.setData(editor.filePath)
                action.triggered.connect(self.slot_tabsMenuActivateTab)
                actions.append(action)
            # add all to menu
            self.tabsMenu.addActions(actions)
        def tabsMenuAboutHide():
            # remove all from the separator to the end
            for action in self.tabsMenu.actions()[self.tabsMenu.actions().index(self._tabsMenuSeparator):]:
                self.tabsMenu.removeAction(action)
            self._tabsMenuSeparator = None
        self.tabsMenu.aboutToShow.connect(tabsMenuAboutShow)
        self.tabsMenu.aboutToHide.connect(tabsMenuAboutHide)

        #
        # Construct Help menu
        #
        self.helpMenu = QtGui.QMenu("&Help")
        self.menuBar().addMenu(self.helpMenu)
        self.helpMenu.addAction(self.helpQuickstartAction)
        self.helpMenu.addAction(self.helpUserManualAction)
        self.helpMenu.addAction(self.helpWikiPageAction)
        self.helpMenu.addSeparator()
        self.helpMenu.addActions([self.sendFeedbackAction, self.reportBugAction, self.ceguiDebugInfoAction])
        self.helpMenu.addSeparator()
        self.helpMenu.addActions([self.viewLicenseAction, self.aboutQtAction])
        self.helpMenu.addSeparator()
        self.helpMenu.addAction(self.aboutAction)

    def setupToolbars(self):
        def createToolbar(name):
            tb = QtGui.QToolBar(name, self)
            tb.setObjectName("%s toolbar" % (name))
            tbIconSizeEntry = self.app.settings.getEntry("global/ui/toolbar_icon_size")

            def updateToolbarIconSize(toolbar, size):
                if size < 16:
                    size = 16
                toolbar.setIconSize(QtCore.QSize(size, size))

            updateToolbarIconSize(tb, tbIconSizeEntry.value)
            tbIconSizeEntry.subscribe(lambda value: updateToolbarIconSize(tb, value))
            self.addToolBar(tb)
            return tb

        #
        # Standard toolbar
        #
        tbar = self.globalToolbar = createToolbar("Standard")
        newMenuButton = QtGui.QToolButton(tbar)
        newMenuButton.setText("New")
        newMenuButton.setToolTip("New File")
        newMenuButton.setIcon(QtGui.QIcon("icons/actions/new_file.png"))
        newMenuButton.setMenu(self.fileNewMenu)
        newMenuButton.setPopupMode(QtGui.QToolButton.InstantPopup)
        tbar.addWidget(newMenuButton)
        tbar.addActions([self.openFileAction, self.openProjectAction])
        tbar.addSeparator()
        tbar.addActions([self.saveAction, self.saveAsAction, self.saveProjectAction, self.saveAllAction])
        # The menubutton does not resize its icon correctly unless we tell it to do so
        tbar.iconSizeChanged.connect(newMenuButton.setIconSize)

        #
        # Edit toolbar
        #
        tbar = self.editToolbar = createToolbar("Edit")
        tbar.addActions([self.undoAction, self.redoAction])
        tbar.addSeparator()
        tbar.addActions([self.cutAction, self.copyAction, self.pasteAction, self.deleteAction])

        #
        # View toolbar
        #
        tbar = self.viewToolbar = createToolbar("View")
        tbar.addActions([self.zoomInAction, self.zoomOutAction, self.zoomResetAction])
        tbar.addSeparator()
        tbar.addAction(self.fullScreenAction)

        #
        # Project toolbar
        #
        tbar = self.projectToolbar = createToolbar("Project")
        tbar.addAction(self.projectReloadResourcesAction)
        tbar.addAction(self.projectSettingsAction)

    def syncProjectToCEGUIInstance(self, indicateErrorsWithDialogs = True):
        """Synchronises current project to the CEGUI instance.

        indicateErrorsWithDialogs - if True a dialog is opened in case of errors

        Returns True if the procedure was successful
        """

        try:
            self.ceguiInstance.syncToProject(self.project, self)

            return True

        except Exception as e:
            if indicateErrorsWithDialogs:
                QtGui.QMessageBox.warning(self, "Failed to synchronise embedded CEGUI to your project",
"""An attempt was made to load resources related to the project being opened, for some reason the loading didn't succeed so all resources were destroyed! The most likely reason is that the resource directories are wrong, this can be very easily remedied in the project settings.

This means that editing capabilities of CEED will be limited to editing of files that don't require a project opened (for example: imagesets).

Details of this error: %s""" % (e))

            return False

    def performProjectDirectoriesSanityCheck(self, indicateErrorsWithDialogs = True):
        try:
            self.project.checkAllDirectories()

            return True

        except IOError as e:
            if indicateErrorsWithDialogs:
                QtGui.QMessageBox.warning(self, "At least one of project's resource directories is invalid",
"""Project's resource directory paths didn't pass the sanity check, please check projects settings.

Details of this error: %s""" % (e))

            return False

    def openProject(self, path, openSettings = False):
        """Opens the project file given in 'path'. Assumes no project is opened at the point this is called.
        The slot_openProject method will test if a project is opened and close it accordingly (with a dialog
        being shown if there are changes to it)

        Errors aren't indicated by exceptions or return values, dialogs are shown in case of errors.

        path - Absolute path of the project file
        openSettings - if True, the settings dialog is opened instead of just loading the resources,
                       this is desirable when creating a new project
        """

        assert(self.project is None)

        # reset project manager to a clean state just in case
        self.projectManager.setProject(None)

        self.project = project.Project()
        try:
            self.project.load(path)
        except IOError:
            QtGui.QMessageBox.critical(self, "Error when opening project", "It seems project at path '%s' doesn't exist or you don't have rights to open it." % (path))

            self.project = None
            return

        self.performProjectDirectoriesSanityCheck()

        # view the newly opened project in the project manager
        self.projectManager.setProject(self.project)
        # and set the filesystem browser path to the base folder of the project
        # TODO: Maybe this could be configurable?
        projectBaseDirectory = self.project.getAbsolutePathOf("")
        if os.path.isdir(projectBaseDirectory):
            self.fileSystemBrowser.setDirectory(projectBaseDirectory)

        self.recentlyUsedProjects.addRecentlyUsed(self.project.projectFilePath)

        # and enable respective actions
        self.saveProjectAction.setEnabled(True)
        self.closeProjectAction.setEnabled(True)
        self.projectSettingsAction.setEnabled(True)
        self.projectReloadResourcesAction.setEnabled(True)

        if openSettings:
            self.slot_projectSettings()

        else:
            self.syncProjectToCEGUIInstance()

    def closeProject(self):
        """Closes currently opened project. Assumes one is opened at the point this is called.
        """

        assert(self.project is not None)

        # since we are effectively unloading the project and potentially nuking resources of it
        # we should definitely unload all tabs that rely on it to prevent segfaults and other
        # nasty phenomena
        if not self.closeAllTabsRequiringProject():
            return

        self.projectManager.setProject(None)
        # TODO: Do we really want to call this there? This was already called when the project was being opened.
        #       It doesn't do anything harmful but maybe is unnecessary.
        self.recentlyUsedProjects.addRecentlyUsed(self.project.projectFilePath)
        # clean resources that were potentially used with this project
        self.ceguiInstance.cleanCEGUIResources()

        self.project.unload()
        self.project = None

        # as the project was closed be will disable actions related to it
        self.saveProjectAction.setEnabled(False)
        self.closeProjectAction.setEnabled(False)
        self.projectReloadResourcesAction.setEnabled(False)

    def saveProject(self):
        """Saves currently opened project to the file it was opened from (or the last file it was saved to).
        """

        assert(self.project is not None)

        self.project.save()

    def saveProjectAs(self, newPath):
        """Saves currently opened project to a custom path. For best reliability, use absolute file path as newPath
        """

        self.project.save(newPath)
        # set the project's file path to newPath so that if you press save next time it will save to the new path
        # (This is what is expected from applications in general I think)
        self.project.projectFilePath = newPath

    def createEditorForFile(self, absolutePath):
        """Creates a new editor for file at given absolutePath. This always creates a new editor,
        it is not advised to use this method directly, use openEditorTab instead.
        """

        ret = None

        projectRelativePath = "N/A"
        try:
            projectRelativePath = os.path.relpath(absolutePath, self.project.getAbsolutePathOf("")) if self.project else "<No project opened>"
        except:
            pass

        if not os.path.exists(absolutePath):
            ret = editors.MessageTabbedEditor(absolutePath,
                   "Couldn't find '%s' (project relative path: '%s'), please check that that your project's "
                   "base directory is set up correctly and that you hadn't deleted "
                   "the file from your HDD. Consider removing the file from the project." % (absolutePath, projectRelativePath))
        else:
            possibleFactories = []

            for factory in self.editorFactories:
                if factory.canEditFile(absolutePath):
                    possibleFactories.append(factory)

            # at this point if possibleFactories is [], no registered tabbed editor factory wanted
            # to accept the file, so we create MessageTabbedEditor that will simply
            # tell the user that given file can't be edited
            #
            # IMO this is a reasonable compromise and plays well with the rest of
            # the editor without introducing exceptions, etc...
            if len(possibleFactories) == 0:
                if absolutePath.endswith(".project"):
                    # provide a more newbie-friendly message in case they are
                    # trying to open a project file as if it were a file
                    ret = editors.MessageTabbedEditor(absolutePath,
                        "You are trying to open '%s' (project relative path: '%s') which "
                        "seems to be a CEED project file. "
                        "This simply is not how things are supposed to work, please use "
                        "File -> Open Project to open your project file instead. "
                        "(CEED enforces proper extensions)" % (absolutePath, projectRelativePath))

                else:
                    ret = editors.MessageTabbedEditor(absolutePath,
                        "No included tabbed editor was able to accept '%s' "
                        "(project relative path: '%s'), please check that it's a file CEED "
                        "supports and that it has the correct extension "
                        "(CEED enforces proper extensions)" % (absolutePath, projectRelativePath))

            else:
                # one or more factories wants to accept the file
                factory = None

                if len(possibleFactories) == 1:
                    # it's decided, just one factory wants to accept the file
                    factory = possibleFactories[0]

                else:
                    assert(len(possibleFactories) > 1)

                    # more than 1 factory wants to accept the file, offer a dialog and let user choose
                    dialog = editors.MultiplePossibleFactoriesDialog(possibleFactories)
                    result = dialog.exec_()

                    if result == QtGui.QDialog.Accepted:
                        selection = dialog.factoryChoice.selectedItems()

                        if len(selection) == 1:
                            factory = selection[0].data(QtCore.Qt.UserRole)

                if factory is None:
                    ret = editors.MessageTabbedEditor(absolutePath,
                       "You failed to choose an editor to open '%s' with (project relative path: '%s')." % (absolutePath, projectRelativePath))

                else:
                    ret = factory.create(absolutePath)

        if self.project is None and ret.requiresProject:
            # the old editor will be destroyed automatically by python GC
            ret = editors.MessageTabbedEditor(absolutePath,
                       "Opening this file requires you to have a project opened!")

        try:
            ret.initialise(self)

            # add successfully opened file to the recent files list
            self.recentlyUsedFiles.addRecentlyUsed(absolutePath)

        except:
            # it may have been partly constructed at this point
            try:
                # make sure the finalisation doesn't early out or fail assertion
                ret.initialised = True

                ret.finalise()
                ret.destroy()

            except:
                # catch all exception the finalisation raises (we can't deal with them anyways)
                pass

            raise

        self.tabEditors.append(ret)
        return ret

    def activateEditorTabByFilePath(self, absolutePath):
        """Activates (makes current) the tab for the path specified and
        returns True on success, otherwise False.
        """
        absolutePath = os.path.normpath(absolutePath)

        for tabEditor in self.tabEditors:
            if tabEditor.filePath == absolutePath:
                tabEditor.makeCurrent()
                return True
        return False

    def openEditorTab(self, absolutePath):
        """Opens editor tab. Creates new editor if such file wasn't opened yet and if it was opened,
        it just makes the tab current.
        """
        if self.activateEditorTabByFilePath(absolutePath):
            return

        editor = self.createEditorForFile(absolutePath)
        editor.makeCurrent()

    def closeEditorTab(self, editor):
        """Closes given editor tab.

        note: Make sure you pass proper existing editor!
        """

        assert(editor in self.tabEditors)

        editor.finalise()
        editor.destroy()

        self.tabEditors.remove(editor)

    def saveSettings(self):
        """Saves geometry and state of this window to QSettings.
        """

        try:
            self.app.qsettings.setValue("window-geometry", self.saveGeometry())
            self.app.qsettings.setValue("window-state", self.saveState())

        except:
            # we don't really care if this fails
            pass

    def restoreSettings(self):
        """Restores geometry and state of this window from QSettings.
        """

        try:
            if self.app.qsettings.contains("window-geometry"):
                self.restoreGeometry(self.app.qsettings.value("window-geometry"))
            if self.app.qsettings.contains("window-state"):
                self.restoreState(self.app.qsettings.value("window-state"))

        except:
            # we don't really care if this fails
            pass

    def quit(self):
        """Safely quits the editor, prompting user to save changes to files and the project."""

        self.saveSettings()

        # we remember last tab we closed to check whether user pressed Cancel in any of the dialogs
        lastTab = None
        while len(self.tabEditors) > 0:
            currentTab = self.tabs.widget(0)
            if currentTab == lastTab:
                # user pressed cancel on one of the tab editor 'save without changes' dialog,
                # cancel the whole quit operation!
                return False
            lastTab = currentTab

            self.slot_tabCloseRequested(0)

        # Close project after all tabs have been closed, there may be tabs requiring a project opened!
        if self.project is not None:
            # if the slot returned False, user pressed Cancel
            if not self.slot_closeProject():
                # in case user pressed cancel the entire quitting processed has to be terminated
                return False

        self.app.quit()

    def closeAllTabsRequiringProject(self):
        """Attempts to close all tabs that require a project opened.

        This is usually done when project settings are altered and CEGUI instance has to be reloaded
        or when project is being closed and we can no longer rely on resource availability.
        """

        i = 0
        while i < self.tabs.count():
            tabbedEditor = self.tabs.widget(i).tabbedEditor

            if tabbedEditor.requiresProject:
                if not self.slot_tabCloseRequested(i):
                    # if the method returns False user pressed Cancel so in that case
                    # we cancel the entire operation
                    return False

                continue

            i += 1

        return True

    def getFilePathsOfAllTabsRequiringProject(self):
        """Queries file paths of all tabs that require a project opened

        This is used to bring previously closed tabs back up when reloading project resources
        """

        ret = []
        i = 0
        while i < self.tabs.count():
            tabbedEditor = self.tabs.widget(i).tabbedEditor

            if tabbedEditor.requiresProject:
                ret.append(tabbedEditor.filePath)

            i += 1

        return ret

    def slot_newProject(self):
        if self.project:
            # another project is already opened!
            result = QtGui.QMessageBox.question(self,
                                                "Another project already opened!",
                                                "Before creating a new project, you must close the one currently opened. "
                                                "Do you want to close currently opened project? (all unsaved changes will be lost!)",
                                                QtGui.QMessageBox.Yes | QtGui.QMessageBox.Cancel,
                                                QtGui.QMessageBox.Cancel)

            if result != QtGui.QMessageBox.Yes:
                return

            # don't close the project yet, close it after the user
            # accepts the New Project dialog below because they may cancel

        newProjectDialog = project.NewProjectDialog()
        if newProjectDialog.exec_() != QtGui.QDialog.Accepted:
            return

        # The dialog was accepted, close any open project
        if self.project:
            self.closeProject()

        newProject = newProjectDialog.createProject()
        newProject.save()

        # open the settings window after creation so that user can further customise their
        # new project file
        self.openProject(path = newProject.projectFilePath, openSettings = True)

        # save the project with the settings that were potentially set in the project settings dialog
        self.saveProject()

    def slot_openProject(self):
        if self.project:
            # another project is already opened!
            result = QtGui.QMessageBox.question(self,
                                                "Another project already opened!",
                                                "Before opening a project, you must close the one currently opened. "
                                                "Do you want to close currently opened project? (all unsaved changes will be lost!)",
                                                QtGui.QMessageBox.Yes | QtGui.QMessageBox.Cancel,
                                                QtGui.QMessageBox.Cancel)

            if result == QtGui.QMessageBox.Yes:
                self.closeProject()
            else:
                # User selected cancel, NOOP
                return

        file_, _ = QtGui.QFileDialog.getOpenFileName(self,
                                                    "Open existing project file",
                                                    "",
                                                    "Project files (*.project)")

        if file_ != "":
            # user actually selected something ;-)

            self.openProject(file_)

    def slot_saveProject(self):
        self.saveProject()

    def slot_closeProject(self):
        assert(self.project)

        if self.project.hasChanges():
            result = QtGui.QMessageBox.question(self,
                                                "Project file has changes!",
                                                "There are unsaved changes in the project file "
                                                "Do you want to save them? "
                                                "(Pressing Discard will discard the changes!)",
                                                QtGui.QMessageBox.Save | QtGui.QMessageBox.Discard | QtGui.QMessageBox.Cancel,
                                                QtGui.QMessageBox.Save)

            if result == QtGui.QMessageBox.Save:
                self.saveProject()

            elif result == QtGui.QMessageBox.Cancel:
                return False

        self.closeProject()
        return True

    def slot_projectSettings(self):
        # since we are effectively unloading the project and potentially nuking resources of it
        # we should definitely unload all tabs that rely on it to prevent segfaults and other
        # nasty phenomena
        if not self.closeAllTabsRequiringProject():
            QtGui.QMessageBox.information(self,
                                          "Project dependent tabs still open!",
                                          "You can't alter project's settings while having tabs that "
                                          "depend on the project and its resources opened!")
            return

        dialog = project.ProjectSettingsDialog(self.project)

        if dialog.exec_() == QtGui.QDialog.Accepted:
            dialog.apply(self.project)
            self.performProjectDirectoriesSanityCheck()
            self.syncProjectToCEGUIInstance()

    def slot_projectReloadResources(self):
        # since we are effectively unloading the project and potentially nuking resources of it
        # we should definitely unload all tabs that rely on it to prevent segfaults and other
        # nasty phenomena

        # we will remember previously opened tabs requiring a project so that we can load them up
        # after we are done
        filePathsToLoad = self.getFilePathsOfAllTabsRequiringProject()
        activeEditorPath = self.activeEditor.filePath if self.activeEditor else ""

        if not self.closeAllTabsRequiringProject():
            QtGui.QMessageBox.information(self,
                                          "Project dependent tabs still open!",
                                          "You can't reload project's resources while having tabs that "
                                          "depend on the project and its resources opened!")
            return

        self.performProjectDirectoriesSanityCheck()
        self.syncProjectToCEGUIInstance()

        # load previously loaded tabs requiring a project opened
        for filePath in filePathsToLoad:
            self.openEditorTab(filePath)

        # previously active editor to be loaded last, this makes it active again
        if activeEditorPath != "":
            self.openEditorTab(activeEditorPath)

    def slot_newFileDialog(self, title = "New File", filtersList = None, selectedFilterIndex = 0, autoSuffix = False):
        defaultDir = ""
        if self.project:
            defaultDir = self.project.getAbsolutePathOf("")

        # Qt (as of 4.8) does not support default suffix (extension) unless you use
        # non-native file dialogs with non-static methods (see QFileDialog.setDefaultSuffix).
        # HACK: We handle this differently depending on whether a default suffix is required

        if filtersList is None or len(filtersList) == 0 or not autoSuffix:
            fileName, selectedFilter = QtGui.QFileDialog.getSaveFileName(self,
                                                                         title,
                                                                         defaultDir,
                                                                         ";;".join(filtersList) if filtersList is not None and len(filtersList) > 0 else None,
                                                                         filtersList[selectedFilterIndex] if filtersList is not None and len(filtersList) > selectedFilterIndex else None)
            if fileName:
                try:
                    f = open(fileName, "w")
                    f.close()
                except IOError as e:
                    QtGui.QMessageBox.critical(self, "Error creating file!",
                                                     "CEED encountered an error trying to create a new file.\n\n(exception details: %s)" % (e))
                    return

                self.openEditorTab(fileName)
        else:
            while True:
                fileName, selectedFilter = QtGui.QFileDialog.getSaveFileName(self,
                                                                             title,
                                                                             defaultDir,
                                                                             ";;".join(filtersList) if filtersList is not None and len(filtersList) > 0 else None,
                                                                             filtersList[selectedFilterIndex] if filtersList is not None and len(filtersList) > selectedFilterIndex else None,
                                                                             QtGui.QFileDialog.DontConfirmOverwrite)
                if not fileName:
                    break

                # if there is no dot, append the selected filter's extension
                if fileName.find(".") == -1:
                    # really ugly, handle with care
                    # find last open paren
                    i = selectedFilter.rfind("(")
                    if i != -1:
                        # find next dot
                        i = selectedFilter.find(".", i)
                        if i != -1:
                            # find next space or close paren
                            k = selectedFilter.find(")", i)
                            l = selectedFilter.find(" ", i)
                            if l != -1 and l < k:
                                k = l
                            if k != -1:
                                selectedExt = selectedFilter[i:k]
                                if selectedExt.find("*") == -1 and selectedExt.find("?") == -1:
                                    fileName += selectedExt

                # and now test & confirm overwrite
                try:
                    if os.path.exists(fileName):
                        msgBox = QtGui.QMessageBox(self)
                        msgBox.setText("A file named \"%s\" already exists in \"%s\"." % (os.path.basename(fileName), os.path.dirname(fileName)))
                        msgBox.setInformativeText("Do you want to replace it, overwriting its contents?")
                        msgBox.addButton(QtGui.QMessageBox.Cancel)
                        replaceButton = msgBox.addButton("&Replace", QtGui.QMessageBox.YesRole)
                        msgBox.setDefaultButton(replaceButton)
                        msgBox.setIcon(QtGui.QMessageBox.Question)
                        msgBox.exec_()
                        if msgBox.clickedButton() != replaceButton:
                            continue
                    f = open(fileName, "w")
                    f.close()
                except IOError as e:
                    QtGui.QMessageBox.critical(self, "Error creating file!",
                                                     "CEED encountered an error trying to create a new file.\n\n(exception details: %s)" % (e))
                    return

                self.openEditorTab(fileName)
                break

    def slot_newLayoutDialog(self):
        self.slot_newFileDialog(title = "New Layout",
                                filtersList = ["Layout files (*.layout)"],
                                autoSuffix = True)

    def slot_newImagesetDialog(self):
        self.slot_newFileDialog(title = "New Imageset",
                                filtersList = ["Imageset files (*.imageset)"],
                                autoSuffix = True)

    def slot_openFile(self, absolutePath):
        self.openEditorTab(absolutePath)

    def slot_openFileDialog(self):
        defaultDir = ""
        if self.project:
            defaultDir = self.project.getAbsolutePathOf("")

        fileName, _ = QtGui.QFileDialog.getOpenFileName(self,
                                                       "Open File",
                                                       defaultDir,
                                                       ";;".join(self.editorFactoryFileFilters),
                                                       self.editorFactoryFileFilters[0])
        if fileName:
            self.openEditorTab(fileName)

    def slot_currentTabChanged(self, index):
        # to fight flicker
        self.tabs.setUpdatesEnabled(False)

        # FIXME: workaround for PySide 1.0.6, I suspect this is a bug in PySide! http://bugs.pyside.org/show_bug.cgi?id=988
        if index is None:
            index = -1

        elif isinstance(index, QtGui.QWidget):
            for i in xrange(0, self.tabs.count()):
                if index is self.tabs.widget(i):
                    index = i
                    break

            assert(not isinstance(index, QtGui.QWidget))
        # END OF FIXME

        wdt = self.tabs.widget(index)

        if self.activeEditor:
            self.activeEditor.deactivate()

        # it's the tabbed editor's responsibility to handle these,
        # we disable them by default
        self.undoAction.setEnabled(False)
        self.redoAction.setEnabled(False)
        # also reset their texts in case the tabbed editor messed with them
        self.undoAction.setText("Undo")
        self.redoAction.setText("Redo")
        # set undo stack to None as we have no idea whether the previous tab editor
        # set it to something else
        self.undoViewer.setUndoStack(None)

        # we also clear the status bar
        self.statusBar().clearMessage()

        if wdt:
            self.revertAction.setEnabled(True)

            self.saveAction.setEnabled(True)
            self.saveAsAction.setEnabled(True)

            self.closeTabAction.setEnabled(True)
            self.closeOtherTabsAction.setEnabled(True)

            wdt.tabbedEditor.activate()
        else:
            # None is selected right now, lets disable appropriate actions
            self.revertAction.setEnabled(False)

            self.saveAction.setEnabled(False)
            self.saveAsAction.setEnabled(False)

            self.closeTabAction.setEnabled(False)
            self.closeOtherTabsAction.setEnabled(False)

        self.tabs.setUpdatesEnabled(True)

    def slot_tabCloseRequested(self, index):
        wdt = self.tabs.widget(index)
        editor = wdt.tabbedEditor

        if not editor.hasChanges():
            # we can close immediately
            self.closeEditorTab(editor)
            return True

        else:
            # we have changes, lets ask the user whether we should dump them or save them
            result = QtGui.QMessageBox.question(self,
                                                "Unsaved changes!",
                                                "There are unsaved changes in '%s'. "
                                                "Do you want to save them? "
                                                "(Pressing Discard will discard the changes!)" % (editor.filePath),
                                                QtGui.QMessageBox.Save | QtGui.QMessageBox.Discard | QtGui.QMessageBox.Cancel,
                                                QtGui.QMessageBox.Save)

            if result == QtGui.QMessageBox.Save:
                # lets save changes and then kill the editor (This is the default action)
                # If there was an error saving the file, stop what we're doing
                # and let the user fix the problem.
                if not editor.save():
                    return False

                self.closeEditorTab(editor)
                return True

            elif result == QtGui.QMessageBox.Discard:
                # changes will be discarded
                # note: we don't have to call editor.discardChanges here

                self.closeEditorTab(editor)
                return True

            # don't do anything if user selected 'Cancel'
            return False

    def slot_closeTab(self):
        # a simple delegate
        self.slot_tabCloseRequested(self.tabs.currentIndex())

    def slot_closeOtherTabs(self):
        current = self.tabs.currentWidget()

        i = 0
        while i < self.tabs.count():
            if self.tabs.widget(i) == current:
                # we skip the current widget
                i += 1
            else:
                if not self.slot_tabCloseRequested(i):
                    # user selected Cancel, we skip this widget
                    i += 1

    def slot_closeAllTabs(self):
        i = 0
        while i < self.tabs.count():
            if not self.slot_tabCloseRequested(i):
                # user selected Cancel, we skip this widget
                i += 1

    def slot_previousTab(self):
        if self.tabs.count() <= 1:
            return
        index = self.tabs.currentIndex() - 1
        if index < 0:
            index = self.tabs.count() + index
        self.tabs.setCurrentIndex(index)

    def slot_nextTab(self):
        if self.tabs.count() <= 1:
            return
        index = (self.tabs.currentIndex() + 1) % self.tabs.count()
        self.tabs.setCurrentIndex(index)

    def slot_tabBarCustomContextMenuRequested(self, point):
        atIndex = self.tabBar.tabAt(point)
        self.tabs.setCurrentIndex(atIndex)

        menu = QtGui.QMenu(self)
        menu.addAction(self.closeTabAction)
        menu.addSeparator()
        menu.addAction(self.closeOtherTabsAction)
        menu.addAction(self.closeAllTabsAction)

        if atIndex != -1:
            tab = self.tabs.widget(atIndex)
            menu.addSeparator()
            dataTypeAction = QtGui.QAction("Data type: %s" % (tab.getDesiredSavingDataType()), self)
            dataTypeAction.setToolTip("Displays which data type this file will be saved to (the desired saving data type).")
            menu.addAction(dataTypeAction)

        menu.exec_(self.tabBar.mapToGlobal(point))

    def slot_save(self):
        if self.activeEditor:
            self.activeEditor.save()

    def slot_saveAs(self):
        if self.activeEditor:
            filePath, _ = QtGui.QFileDialog.getSaveFileName(self, "Save as", os.path.dirname(self.activeEditor.filePath))
            if filePath: # make sure user hasn't cancelled the dialog
                self.activeEditor.saveAs(filePath)

    def slot_saveAll(self):
        """Saves all opened tabbed editors and opened project (if any)
        """

        if self.project is not None:
            self.project.save()

        for editor in self.tabEditors:
            editor.save()

    def slot_undo(self):
        if self.activeEditor:
            self.activeEditor.undo()

    def slot_redo(self):
        if self.activeEditor:
            self.activeEditor.redo()

    def slot_revert(self):
        if self.activeEditor:
            ret = QtGui.QMessageBox.question(self,
                                             "Are you sure you want to revert to file on hard disk?",
                                             "Reverting means that the file will be reloaded to the "
                                             "state it is in on the HDD.\n\nRevert?\n\n"
                                             "If you select Yes, ALL UNDO HISTORY MIGHT BE DESTROYED!\n"
                                             "(though I will preserve it if possible)",
                                             QtGui.QMessageBox.No | QtGui.QMessageBox.Yes,
                                             QtGui.QMessageBox.No) # defaulting to No is safer IMO

            if ret == QtGui.QMessageBox.Yes:
                self.activeEditor.revert()

            elif ret == QtGui.QMessageBox.No:
                # user chickened out
                pass

            else:
                # how did we get here?
                assert(False)

    def slot_cut(self):
        if self.activeEditor:
            self.activeEditor.performCut()

    def slot_copy(self):
        if self.activeEditor:
            self.activeEditor.performCopy()

    def slot_paste(self):
        if self.activeEditor:
            self.activeEditor.performPaste()

    def slot_delete(self):
        if self.activeEditor:
            self.activeEditor.performDelete()

    def slot_zoomIn(self):
        if self.activeEditor:
            self.activeEditor.zoomIn()

    def slot_zoomOut(self):
        if self.activeEditor:
            self.activeEditor.zoomOut()

    def slot_zoomReset(self):
        if self.activeEditor:
            self.activeEditor.zoomReset()

    def slot_about(self):
        dialog = about.AboutDialog()
        dialog.exec_()

    def slot_license(self):
        dialog = about.LicenseDialog()
        dialog.exec_()

    def slot_quit(self):
        return self.quit()

    def closeEvent(self, event):
        handled = self.slot_quit()

        if not handled:
            event.ignore()

        else:
            event.accept()

    def slot_openRecentFile(self):
        if self.sender():
            absolutePath = unicode(self.sender().data())
            if os.path.exists(absolutePath):
                self.openEditorTab(absolutePath)
            else:
                msgBox = QtGui.QMessageBox(self)
                msgBox.setText("File \"%s\" was not found." % (absolutePath))
                msgBox.setInformativeText("The file does not exist; it may have been moved or deleted."
                                          " Do you want to remove it from the recently used list?")
                msgBox.addButton(QtGui.QMessageBox.Cancel)
                removeButton = msgBox.addButton("&Remove", QtGui.QMessageBox.YesRole)
                msgBox.setDefaultButton(removeButton)
                msgBox.setIcon(QtGui.QMessageBox.Question)
                msgBox.exec_()

                if msgBox.clickedButton() == removeButton:
                    self.recentlyUsedFiles.removeRecentlyUsed(absolutePath)

    def slot_openRecentProject(self):
        if self.sender():
            absolutePath = unicode(self.sender().data())
            if os.path.exists(absolutePath):
                if self.project:
                    # give user a chance to save changes if needed
                    if not self.slot_closeProject():
                        return

                self.openProject(absolutePath)
            else:
                msgBox = QtGui.QMessageBox(self)
                msgBox.setText("Project \"%s\" was not found." % (absolutePath))
                msgBox.setInformativeText("The project file does not exist; it may have been moved or deleted."
                                          " Do you want to remove it from the recently used list?")
                msgBox.addButton(QtGui.QMessageBox.Cancel)
                removeButton = msgBox.addButton("&Remove", QtGui.QMessageBox.YesRole)
                msgBox.setDefaultButton(removeButton)
                msgBox.setIcon(QtGui.QMessageBox.Question)
                msgBox.exec_()

                if msgBox.clickedButton() == removeButton:
                    self.recentlyUsedProjects.removeRecentlyUsed(absolutePath)

    def slot_tabsMenuActivateTab(self):
        if self.sender():
            absolutePath = unicode(self.sender().data())
            self.activateEditorTabByFilePath(absolutePath)

    def slot_toggleFullScreen(self):
        if self.isFullScreen():
            self.showNormal()
            if self.wasMaximized:
                self.showMaximized()
        else:
            self.wasMaximized = self.isMaximized()
            self.showFullScreen()

    def slot_toggleStatusbar(self):
        if self.statusBar().isVisible():
            self.statusBar().hide()

        else:
            self.statusBar().show()

    def slot_helpQuickstart(self):
        QtGui.QDesktopServices.openUrl("file://%s" % (os.path.join(paths.DOC_DIR, "quickstart-guide.pdf")))

    def slot_helpUserManual(self):
        QtGui.QDesktopServices.openUrl("file://%s" % (os.path.join(paths.DOC_DIR, "user-manual.pdf")))

    def slot_helpWikiPage(self):
        QtGui.QDesktopServices.openUrl("http://www.cegui.org.uk/wiki/index.php/CEED")

    def slot_sendFeedback(self):
        QtGui.QDesktopServices.openUrl("http://www.cegui.org.uk/phpBB2/viewforum.php?f=15")

    def slot_reportBug(self):
        QtGui.QDesktopServices.openUrl("http://www.cegui.org.uk/mantis/bug_report_page.php")

    def slot_ceguiDebugInfo(self):
        self.ceguiContainerWidget.debugInfo.show()
