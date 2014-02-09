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

"""This module allows to compile .ui files made in QtDesigner into python
code that represents the same GUI layouts.

To enable automatic recompilation see version.DEVELOPER_MODE.
To recompile ui files manually see the "maintenance" script.
"""

import os
from pysideuic import compileUi

# written by Paul D Turner for CELayoutEditorII
def compileUIFiles(uiDir):
    for name in os.listdir(uiDir):
        uiFilePath = os.path.join(uiDir, name)

        if os.path.isfile(uiFilePath):
            if name.endswith(".ui"):
                uiResultPath = (name[:-3] + ".py").lower()

                with open(os.path.join(uiDir, uiResultPath), "w") as f:
                    compileUi(uiFilePath, f)

def main():
    from ceed import paths

    # FIXME: This is something that we may forget to maintain, ideally
    #        it should be done automatically by detecting all subfolders
    #        of UI_DIR
    compileUIFiles(paths.UI_DIR)
    compileUIFiles(os.path.join(paths.UI_DIR, "editors"))
    compileUIFiles(os.path.join(paths.UI_DIR, "editors", "animation_list"))
    compileUIFiles(os.path.join(paths.UI_DIR, "editors", "imageset"))
    compileUIFiles(os.path.join(paths.UI_DIR, "editors", "layout"))
    compileUIFiles(os.path.join(paths.UI_DIR, "widgets"))

if __name__ == "__main__":
    main()
