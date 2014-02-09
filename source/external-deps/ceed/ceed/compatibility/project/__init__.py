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

from ceed import compatibility
from ceed.compatibility import ceguihelpers

Project1 = "CEED Project 1"

class Project1TypeDetector(compatibility.TypeDetector):
    def getType(self):
        return Project1

    def getPossibleExtensions(self):
        return set(["project"])

    def matches(self, data, extension):
        if extension not in ["", "project"]:
            return False

        # should work as a pretty rigorous test for now, tests the root tag name and version
        # CEED project files have a similar version check to CEGUI, that's why we can use
        # the cegui helper function here.
        return ceguihelpers.checkDataVersion("Project", Project1, data)

class Manager(compatibility.Manager):
    """Manager of CEED project compatibility layers"""

    def __init__(self):
        super(Manager, self).__init__()

        self.EditorNativeType = Project1
        # doesn't make much sense
        self.CEGUIVersionType = {
            "0.6" : Project1,
            "0.7" : Project1,
            "1.0" : Project1
        }

        self.detectors.append(Project1TypeDetector())

manager = Manager()
