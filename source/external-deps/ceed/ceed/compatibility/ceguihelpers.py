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

"""Misc helper functionality often reused in compatibility layers
"""

from xml.sax import parseString, handler

def checkDataVersion(rootElement, version, data):
    """Checks that tag of the root element in data is as given
    and checks that version recorded in the root element is given
    (can be None if no version information should be there)

    Returns True if everything went well and all matches,
    False otherwise.

    NOTE: Implemented using SAX for speed
    """

    class RootElement(Exception):
        def __init__(self, tag, version):
            super(RootElement, self).__init__()

            self.tag = tag
            self.version = version

    class REHandler(handler.ContentHandler):
        def __init__(self):
            handler.ContentHandler.__init__(self)

        def startElement(self, name, attrs):
            version = None
            if attrs.has_key("version"):
                version = attrs["version"]

            raise RootElement(name, version)

    try:
        parseString(data, REHandler())

    except RootElement as re:
        if re.tag == rootElement and re.version == version:
            return True

    except:
        pass

    return False
