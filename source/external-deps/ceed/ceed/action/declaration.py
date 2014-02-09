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

"""Provides API to declare actions that are remapable
(shortcuts can be changed) using the settings interface

All actions in the application should be declared using this API, unless
there are serious reasons not to do that.
"""

from PySide.QtCore import Qt
from PySide.QtGui import QAction, QKeySequence, QIcon

class Action(QAction):
    """The only thing different in this from QAction is the ability to change the shortcut of it
    using CEED's settings API/interface.

    While it isn't needed/required to use this everywhere where QAction is used, it is recommended.
    """

    def __init__(self, category, name, label = None, help_ = "", icon = QIcon(),
                 defaultShortcut = QKeySequence(), settingsLabel = None,
                 menuRole = QAction.TextHeuristicRole):
        if label is None:
            label = name
        if settingsLabel is None:
            # remove trailing ellipsis and mnemonics
            settingsLabel = label.rstrip(".").replace("&&", "%amp%").replace("&", "").replace("%amp%", "&&")

        self.category = category
        self.name = name
        self.defaultShortcut = defaultShortcut
        self.settingsLabel = settingsLabel

        # QAction needs a QWidget parent, we use the main window as that
        super(Action, self).__init__(icon, label, self.getManager().mainWindow)

        self.setToolTip(settingsLabel)
        self.setStatusTip(help_)
        self.setMenuRole(menuRole)

        # we default to application shortcuts and we make sure we always disable the ones we don't need
        # you can override this for individual actions via the setShortcutContext method as seen here
        self.setShortcutContext(Qt.ApplicationShortcut)
        self.setShortcut(defaultShortcut)

        self.settingsEntry = None
        self.declareSettingsEntry()

    def getManager(self):
        return self.category.manager

    def declareSettingsEntry(self):
        section = self.category.settingsSection

        self.settingsEntry = section.createEntry(name = "shortcut_%s" % (self.name), type_ = QKeySequence, label = "%s" % (self.settingsLabel),
                                              defaultValue = self.defaultShortcut, widgetHint = "keySequence")

        # when the entry changes, we want to change our shortcut too!
        self.settingsEntry.subscribe(self.setShortcut)

class ActionCategory(object):
    """Actions are grouped into categories

    examples: General for all round actions (Exit, Undo, Redo, ...), Layout editing for layout editing (duh!), ...
    """

    def __init__(self, manager, name, label = None):
        if label is None:
            label = name

        self.manager = manager
        self.name = name
        self.label = label

        self.settingsSection = None
        self.declareSettingsSection()

        self.actions = []

    def getManager(self):
        return self.manager

    def createAction(self, **kwargs):
        action = Action(self, **kwargs)
        self.actions.append(action)

        return action

    def getAction(self, name):
        for action in self.actions:
            if action.name == name:
                return action

        raise RuntimeError("Action '" + name + "' not found in category '" + self.name + "'.")

    def setEnabled(self, enabled):
        """Allows you to enable/disable actions en masse, especially useful when editors are switched.
        This gets rid of shortcut clashes and so on.
        """

        for action in self.actions:
            action.setEnabled(enabled)

    def declareSettingsSection(self):
        category = self.getManager().settingsCategory

        self.settingsSection = category.createSection(name = self.name, label = self.label)

class ActionManager(object):
    """Usually a singleton that manages all action categories and therefore
    actions within them.
    """

    def __init__(self, mainWindow, settings):
        self.mainWindow = mainWindow
        self.settings = settings

        self.settingsCategory = None
        self.declareSettingsCategory()

        self.categories = []

    def createCategory(self, **kwargs):
        category = ActionCategory(self, **kwargs)
        self.categories.append(category)

        return category

    def getCategory(self, name):
        for category in self.categories:
            if category.name == name:
                return category

        raise RuntimeError("Category '" + name + "' not found in this action manager")

    def getAction(self, path):
        # FIXME: Needs better error handling
        splitted = path.split("/", 1)
        assert(len(splitted) == 2)

        category = self.getCategory(splitted[0])
        return category.getAction(splitted[1])

    def declareSettingsCategory(self):
        self.settingsCategory = self.settings.createCategory(name = "action_shortcuts", label = "Shortcuts", sortingWeight = 999)
