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

"""Declares global settings entries. Provides an entry point to the settings
system via getEntry(..).
"""

from ceed.settings import declaration
from ceed.settings import persistence
from ceed.settings import interface

class Settings(declaration.Settings):
    instance = None

    def __init__(self, qsettings):
        super(Settings, self).__init__(name = "settings",
                                       label = "CEGUI Unified Editor settings",
                                       help_ = "Provides all persistent settings of CEGUI Unified Editor (CEED), everything is divided into categories (see the tab buttons).")

        self.setPersistenceProvider(persistence.QSettingsPersistenceProvider(qsettings))

        # General settings
        global_ = self.createCategory(name = "global", label = "Global")
        undoRedo = global_.createSection(name = "undo", label = "Undo and Redo")
        # by default we limit the undo stack to 500 undo commands, should be enough and should
        # avoid memory drainage. keep in mind that every tabbed editor has it's own undo stack,
        # so the overall command limit is number_of_tabs * 500!
        undoRedo.createEntry(name = "limit", type_ = int, label = "Limit (number of steps)",
                          help_ = "Puts a limit on every tabbed editor's undo stack. You can undo at most the number of times specified here.",
                          defaultValue = 500, widgetHint = "int",
                          sortingWeight = 1, changeRequiresRestart = True)

        app = global_.createSection(name = "app", label = "Application")
        app.createEntry(name = "show_splash", type_ = bool, label = "Show splash screen",
                       help_ = "Show the splash screen on startup",
                       defaultValue = True, widgetHint = "checkbox",
                       sortingWeight = 1, changeRequiresRestart = False)

        ui = global_.createSection(name = "ui", label = "User Interface")
        ui.createEntry(name = "toolbar_icon_size", type_ = int, label = "Toolbar icon size",
                       help_ = "Sets the size of the toolbar icons",
                       defaultValue = 32, widgetHint = "combobox",
                       sortingWeight = 1, changeRequiresRestart = False,
                       optionList = [ [32, "Normal"], [24, "Small"], [16, "Smaller"] ])

        ceguiDebugInfo = global_.createSection(name = "cegui_debug_info", label = "CEGUI debug info")
        ceguiDebugInfo.createEntry(name = "log_limit", type_ = int, label = "Log messages limit",
                                     help_ = "Limits number of remembered log messages to given amount. This is there to prevent endless growth of memory consumed by CEED.",
                                     defaultValue = 20000, widgetHint = "int",
                                     sortingWeight = 1, changeRequiresRestart = True)

        navigation = global_.createSection(name = "navigation", label = "Navigation")
        navigation.createEntry(name = "ctrl_zoom", type_ = bool, label = "Only zoom when CTRL is pressed",
                               help_ = "Mouse wheel zoom is ignored unless the Control key is pressed when it happens.",
                               defaultValue = True, widgetHint = "checkbox",
                               sortingWeight = 1, changeRequiresRestart = False)

        import ceed.cegui.settings_decl as cegui_settings
        cegui_settings.declare(self)

        import ceed.editors.imageset.settings_decl as imageset_settings
        imageset_settings.declare(self)

        import ceed.editors.layout.settings_decl as layout_settings
        layout_settings.declare(self)

        assert(Settings.instance is None)
        Settings.instance = self

def getEntry(path):
    """This is a convenience method to make settings retrieval easier
    """

    assert(Settings.instance is not None)
    return Settings.instance.getEntry(path)

__all__ = ["declaration", "persistence", "interface", "Settings", "getEntry"]
