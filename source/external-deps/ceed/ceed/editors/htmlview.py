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

from ceed import editors

from PySide import QtWebKit

class HTMLViewTabbedEditor(editors.TabbedEditor):
    """This is basically a stub tabbed editor, it simply displays a HTML message
    and doesn't allow any sort of editing at all, all functionality is stubbed

    This is for internal use only so there is no factory for this particular editor
    """
    def __init__(self, filePath, message):
        super(HTMLViewTabbedEditor, self).__init__(None, filePath)

        self.tabWidget = QtWebKit.QWebView()
        self.tabWidget.setHtml(message)

    def hasChanges(self):
        return False
