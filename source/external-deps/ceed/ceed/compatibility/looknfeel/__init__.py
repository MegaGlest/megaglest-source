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
from ceed.compatibility.looknfeel import cegui

class Manager(compatibility.Manager):
    """Manager of looknfeel compatibility layers"""

    def __init__(self):
        super(Manager, self).__init__()

        self.CEGUIVersionTypes = {
            "0.4" : cegui.CEGUILookNFeel1,
            # we only support non-obsolete major versions
            #"0.5" : cegui.CEGUILookNFeel2,
            "0.5" : cegui.CEGUILookNFeel3,
            "0.6" : cegui.CEGUILookNFeel4,
            # we only support non-obsolete major versions
            #"0.7.0" : cegui.CEGUILookNFeel5,
            "0.7" : cegui.CEGUILookNFeel6, # because of animations, since 0.7.2
            "1.0" : cegui.CEGUILookNFeel7
        }

        self.EditorNativeType = cegui.CEGUILookNFeel7

        self.detectors.append(cegui.LookNFeel6TypeDetector())
        self.detectors.append(cegui.LookNFeel7TypeDetector())

        self.layers.append(cegui.LookNFeel6To7Layer())
        self.layers.append(cegui.LookNFeel7To6Layer())

manager = Manager()
