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

from xml.etree import cElementTree as ElementTree

from ceed.editors import multi

class CodeEditing(multi.CodeEditMode):
    def __init__(self, tabbedEditor):
        super(CodeEditing, self).__init__()

        self.tabbedEditor = tabbedEditor

    def getNativeCode(self):
        return self.tabbedEditor.visual.generateNativeData()

    def propagateNativeCode(self, code):
        element = None

        try:
            element = ElementTree.fromstring(code)

        except:
            return False

        else:
            self.tabbedEditor.visual.loadFromElement(element)
            return True
