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

"""Uses the declaration API to declare generic actions in the application.

Also provides ConnectionGroup, which can be used to make mass connecting and
disconnecting more convenient.
"""

from ceed.action import declaration

from PySide.QtCore import Qt
from PySide.QtGui import QIcon, QKeySequence, QAction

class ConnectionGroup(object):
    """This object allows you to group signal slot connections and
    disconnect them and connect them again en masse.

    Very useful when switching editors
    """

    class Connection(object):
        """Very lightweight holding object that represents a signal slot connection.
        Receiver is the Python callable (free function, bound method, lambda function, anything really)

        Not intended for use outside the ConnectionGroup class, should be considered internal!
        """

        def __init__(self, action, receiver, signalName = "triggered", connectionType = Qt.AutoConnection):
            self.action = action
            self.signalName = signalName
            self.receiver = receiver
            self.connectionType = connectionType

            self.connected = False

            if not hasattr(self.action, self.signalName):
                raise RuntimeError("Given action doesn't have signal called '%s'!" % (self.signalName))

        def connect(self):
            if self.connected:
                raise RuntimeError("This Connection was already connected!")

            signal = getattr(self.action, self.signalName)
            signal.connect(self.receiver, self.connectionType)

            self.connected = True

        def disconnect(self):
            if not self.connected:
                raise RuntimeError("Can't disconnect this Connection, it isn't connected at the moment.")

            signal = getattr(self.action, self.signalName)
            signal.disconnect(self.receiver)

            self.connected = False

    def __init__(self, actionManager = None):
        self.actionManager = actionManager

        self.connections = []

    def add(self, action, **kwargs):
        if isinstance(action, basestring):
            # allow adding actions by their full names/paths for convenience
            if self.actionManager is None:
                raise RuntimeError("Action added by it's string name but action manager wasn't set, can't retrieve the action object!")

            action = self.actionManager.getAction(action)

        connection = ConnectionGroup.Connection(action, **kwargs)
        self.connections.append(connection)

        return connection

    def remove(self, connection, ensureDisconnected = True):
        if not connection in self.connections:
            raise RuntimeError("Can't remove given connection, it wasn't added!")

        self.connections.remove(connection)

        if ensureDisconnected and connection.connected:
            connection.disconnect()

    def connectAll(self, skipConnected = True):
        for connection in self.connections:
            if connection.connected and skipConnected:
                continue

            connection.connect()

    def disconnectAll(self, skipDisconnected = True):
        for connection in self.connections:
            if not connection.connected and skipDisconnected:
                continue

            connection.disconnect()

class ActionManager(declaration.ActionManager):
    """This is the CEED's action manager, all the "global" actions are declared in it.

    Includes general actions (like Quit, Undo & Redo, File Open, etc...) and also editor specific
    actions (layout align left, ...) - you should use ConnectionGroup for these to connect them when
    your editor is activated and disconnect them when it's deactivated.

    See ConnectionGroup
    """

    instance = None

    def __init__(self, mainWindow, settings):
        super(ActionManager, self).__init__(mainWindow, settings)

        general = self.createCategory(name = "general", label = "General")
        general.createAction(name = "application_settings", label = "Prefere&nces",
                             help_ = "Displays and allows changes of the application settings (persistent settings)",
                             icon = QIcon("icons/actions/application_settings.png"),
                             defaultShortcut = QKeySequence(QKeySequence.Preferences),
                             menuRole = QAction.PreferencesRole)
        general.createAction(name = "quit", label = "Quit",
                             help_ = "Exits the entire application safely with confirmation dialogs for all unsaved data.",
                             icon = QIcon("icons/actions/quit.png"),
                             defaultShortcut = QKeySequence(QKeySequence.Quit),
                             menuRole = QAction.QuitRole)
        general.createAction(name = "help_quickstart", label = "&Quickstart Guide",
                             help_ = "Opens the quick start guide (inside editor).",
                             icon = QIcon("icons/actions/help_quickstart.png"),
                             defaultShortcut = QKeySequence(QKeySequence.HelpContents))
        general.createAction(name = "help_user_manual", label = "&User manual",
                             help_ = "Opens the user manual (inside editor).",
                             icon = QIcon("icons/actions/help_user_manual.png"))
        general.createAction(name = "help_wiki_page", label = "&Wiki page",
                             help_ = "Opens the wiki page.",
                             icon = QIcon("icons/actions/help_wiki_page.png"))
        general.createAction(name = "send_feedback", label = "Send &Feedback",
                             help_ = "Opens the feedback forum.",
                             icon = QIcon("icons/actions/help_feedback.png"))
        general.createAction(name = "report_bug", label = "Report &Bug",
                             help_ = "Opens the issue tracker application.")
        general.createAction(name = "cegui_debug_info", label = "&CEGUI debug info",
                             help_ = "Opens CEGUI debug info (FPS and log). Only useful when you have issues.")
        general.createAction(name = "view_license", label = "L&icense",
                             help_ = "Displays the application license.")
        general.createAction(name = "about_qt", label = "About &Qt",
                             help_ = "Display information about the Qt framework.",
                             menuRole = QAction.AboutQtRole)
        general.createAction(name = "about", label = "&About",
                             help_ = "Display information about the application.",
                             icon = QIcon("icons/actions/help_about.png"),
                             menuRole = QAction.AboutRole)
        general.createAction(name = "full_screen", label = "&Full Screen", settingsLabel = "Toggle Full Screen",
                             help_ = "Switches between full screen view and normal view.",
                             icon = QIcon("icons/actions/view_fullscreen.png"),
                             defaultShortcut = QKeySequence("F11"))
        general.createAction(name = "statusbar", label = "&Statusbar", settingsLabel = "Toggle Statusbar",
                             help_ = "Hides and shows the visibility status bar.").setCheckable(True)

        projectManagement = self.createCategory(name = "project_management", label = "Project Management")
        projectManagement.createAction(name = "new_project", label = "Pro&ject...", settingsLabel = "New Project",
                                       help_ = "Creates a new project from scratch. Only one project can be opened at a time so you will be asked to close your current project if any.",
                                       icon = QIcon("icons/actions/new_project.png"))
        projectManagement.createAction(name = "open_project", label = "Open Pro&ject...",
                                       help_ = "Opens a pre-existing project file. Only one project can be opened at a time so you will be asked to close your current project if any.",
                                       icon = QIcon("icons/actions/open_project.png"))
        projectManagement.createAction(name = "save_project", label = "Save Pro&ject",
                                       help_ = "Saves currently opened project file to the same location from where it was opened.",
                                       icon = QIcon("icons/actions/save_project.png"))
        projectManagement.createAction(name = "close_project", label = "Close Project",
                                       help_ = "Closes currently opened project file.",
                                       icon = QIcon("icons/actions/close_project.png"))
        projectManagement.createAction(name = "project_settings", label = "&Settings",
                                       help_ = "Displays and allows changes of the project settings (of the currently opened project).",
                                       icon = QIcon("icons/actions/project_settings.png"),
                                       menuRole = QAction.NoRole)
        projectManagement.createAction(name = "reload_resources", label = "&Reload Resources",
                                       help_ = "Reloads all CEGUI resources associated with currently opened project.",
                                       icon = QIcon("icons/project_management/reload_resources.png"))

        files = self.createCategory(name = "files", label = "Tabs and Files")
        files.createAction(name = "new_file", label = "&Other...", settingsLabel = "New File",
                           help_ = "Creates a new empty file of any type.",
                           icon = QIcon("icons/actions/new_file.png"),
                           defaultShortcut = QKeySequence(QKeySequence.New))
        files.createAction(name = "new_layout", label = "&Layout...", settingsLabel = "New Layout",
                           help_ = "Creates a new layout file.",
                           icon = QIcon("icons/project_items/layout.png"))
        files.createAction(name = "new_imageset", label = "&Imageset...", settingsLabel = "New Imageset",
                           help_ = "Creates a new imageset file.",
                           icon = QIcon("icons/project_items/imageset.png"))
        files.createAction(name = "revert_file", label = "Re&vert", settingsLabel = "Revert File",
                           help_ = "Reverts the active file to version on disk.")
        files.createAction(name = "clear_recent_files", label = "&Clear", settingsLabel = "Clear Recent Files",
                           help_ = "Empties the recent files list.")
        files.createAction(name = "clear_recent_projects", label = "&Clear", settingsLabel = "Clear Recent Projects",
                           help_ = "Empties the recent projects list.")
        files.createAction(name = "open_file", label = "&Open File...",
                           help_ = "Opens a pre-existing file from any location (if the file is already opened the tab with it will be switched to).",
                           icon = QIcon("icons/actions/open_file.png"),
                           defaultShortcut = QKeySequence(QKeySequence.Open))
        files.createAction(name = "save_file", label = "&Save", settingsLabel = "Save File",
                           help_ = "Saves currently opened (and active - currently edited) file to its original location.",
                           icon = QIcon("icons/actions/save.png"),
                           defaultShortcut = QKeySequence(QKeySequence.Save))
        files.createAction(name = "save_file_as", label = "Save &As", settingsLabel = "Save File As",
                           help_ = "Saves currently opened (and active - currently edited) file to a custom location.",
                           icon = QIcon("icons/actions/save_as.png"),
                           defaultShortcut = QKeySequence(QKeySequence.SaveAs))
        files.createAction(name = "save_all", label = "Save A&ll",
                           help_ = "Saves currently opened project file (if any) and all currently opened files to their original location.",
                           icon = QIcon("icons/actions/save_all.png"),
                           defaultShortcut = QKeySequence())
        files.createAction(name = "close_current_tab", label = "&Close", settingsLabel = "Close Tab",
                           help_ = "Closes currently active (and switched to) tab - asks for confirmation if there are unsaved changes.",
                           icon = QIcon("icons/actions/close_tab.png"),
                           defaultShortcut = QKeySequence(QKeySequence.Close))
        files.createAction(name = "close_other_tabs", label = "Close &Other", settingsLabel = "Close Other Tabs",
                           help_ = "Closes all tabs except the one that is currently active - asks for confirmation if there are unsaved changes.",
                           icon = QIcon("icons/actions/close_other_tabs.png"),
                           defaultShortcut = QKeySequence())
        files.createAction(name = "close_all_tabs", label = "Close A&ll", settingsLabel = "Close All Tabs",
                           help_ = "Closes all tabs - asks for confirmation if there are unsaved changes.",
                           icon = QIcon("icons/actions/close_all_tabs.png"),
                           defaultShortcut = QKeySequence())
        files.createAction(name = "previous_tab", label = "&Previous Tab",
                           help_ = "Activates the previous (left) tab.",
                           defaultShortcut = QKeySequence("Ctrl+PgUp"))
        files.createAction(name = "next_tab", label = "&Next Tab",
                           help_ = "Activates the next (right) tab.",
                           defaultShortcut = QKeySequence("Ctrl+PgDown"))

        allEditors = self.createCategory(name = "all_editors", label = "All Editors")
        allEditors.createAction(name = "zoom_in", label = "Zoom &In",
                                help_ = "Increases the zoom level.",
                                icon = QIcon("icons/actions/zoom_in.png"),
                                defaultShortcut = QKeySequence(QKeySequence.ZoomIn))
        allEditors.createAction(name = "zoom_out", label = "Zoom &Out",
                                help_ = "Decreases the zoom level.",
                                icon = QIcon("icons/actions/zoom_out.png"),
                                defaultShortcut = QKeySequence(QKeySequence.ZoomOut))
        allEditors.createAction(name = "zoom_reset", label = "&Normal Size", settingsLabel = "Zoom Reset",
                                help_ = "Resets zoom level to the default (1:1).",
                                icon = QIcon("icons/actions/zoom_original.png"),
                                defaultShortcut = QKeySequence("Ctrl+0"))
        allEditors.createAction(name = "undo", label = "&Undo",
                                help_ = "Undoes the last operation (in the currently active tabbed editor)",
                                icon = QIcon("icons/actions/undo.png"),
                                defaultShortcut = QKeySequence(QKeySequence.Undo))
        allEditors.createAction(name = "redo", label = "&Redo",
                                help_ = "Redoes the last undone operation (in the currently active tabbed editor)",
                                icon = QIcon("icons/actions/redo.png"),
                                defaultShortcut = QKeySequence(QKeySequence.Redo))
        allEditors.createAction(name = "copy", label = "&Copy",
                                help_ = "Performs a clipboard copy",
                                icon = QIcon("icons/actions/copy.png"),
                                defaultShortcut = QKeySequence(QKeySequence.Copy))
        allEditors.createAction(name = "cut", label = "Cu&t",
                                help_ = "Performs a clipboard cut",
                                icon = QIcon("icons/actions/cut.png"),
                                defaultShortcut = QKeySequence(QKeySequence.Cut))
        allEditors.createAction(name = "paste", label = "&Paste",
                                help_ = "Performs a clipboard paste",
                                icon = QIcon("icons/actions/paste.png"),
                                defaultShortcut = QKeySequence(QKeySequence.Paste))
        allEditors.createAction(name = "delete", label = "&Delete",
                                help_ = "Deletes the selected items",
                                icon = QIcon("icons/actions/delete.png"),
                                defaultShortcut = QKeySequence(QKeySequence.Delete))
        # Imageset editor
        import ceed.editors.imageset.action_decl as imageset_actions
        imageset_actions.declare(self)

        # Layout editor
        import ceed.editors.layout.action_decl as layout_actions
        layout_actions.declare(self)

        assert(ActionManager.instance is None)
        ActionManager.instance = self

def getAction(path):
    """This is a convenience method to make action retrieval easier
    """

    assert(ActionManager.instance is not None)
    return ActionManager.instance.getAction(path)
