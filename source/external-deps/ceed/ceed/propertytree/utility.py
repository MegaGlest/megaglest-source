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

"""Misc utilities.

getDictionaryTreePath -- Retrieve a value from a dictionary tree.
"""

def getDictionaryTreePath(dtree, path, defaultValue=None):
    """Get the value of the dictionary tree at the specified path string.

    Return 'defaultValue' if the path can't be found.

    Example::
        tree = { "instantApply" : False, "numeric": { "max" : 10 } }
        path = "numeric/max"
        getDictionaryTreePath(tree, path, 100)
    """
    if dtree is None or path is None:
        return defaultValue

    # remove slashes from start because the path is always absolute.
    # we do not remove the final slash, if any, because it is
    # allowed (to return a subtree of options)
    path.lstrip("/")

    pcs = path.split("/")
    optRoot = dtree

    for pc in pcs:
        # if the path component is an empty string we've reached the destination,
        # getDictionaryTreePath("folder1/") for example.
        if pc == "":
            return optRoot

        # if the pc exists in the current root, make it root and
        # process the next pc
        if pc in optRoot:
            optRoot = optRoot[pc]
            continue

        # if it wasn't found in the current root, return the default value.
        return defaultValue

    # we've traversed the option tree, return whatever our root is
    return optRoot

def boolFromString(text):
    if isinstance(text, bool):
        return text
    return text.lower() in ("true", "yes", "1") if text else False
