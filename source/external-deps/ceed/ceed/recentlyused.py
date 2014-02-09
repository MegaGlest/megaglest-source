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

"""Implements reusable functionality for "recently used" lists/menus.

This is used mainly for "recent files" and "recent projects" in CEED.
We may use it for even more in the future
"""

# Stefan Stammberger is the original author of this file

from PySide import QtGui

class RecentlyUsed(object):
    """This class can be used to store pointers to Items like files and images
    for later reuse within the application.
    """

    def __init__(self, qsettings, sectionIdentifier):
        self.sectionIdentifier = "recentlyUsedIdentifier/" + sectionIdentifier
        self.qsettings = qsettings
        # how many recent items should the editor remember
        self.maxRecentItems = 10
        # to how many characters should the recent file names be trimmed to
        self.recentlItemsNameTrimLength = 60

    def addRecentlyUsed(self, itemName):
        """Add an item to the list"""

        items = []
        if self.qsettings.contains(self.sectionIdentifier):
            val = unicode(self.qsettings.value(self.sectionIdentifier))
            items = self.stringToStringList(val)

        # if something went wrong before, just drop recent projects and start anew,
        # recent projects aren't that important
        if not isinstance(items, list):
            items = []

        isInList = False
        for f in items:
            if f == itemName:
                items.remove(f)
                items.insert(0, f)
                isInList = True
                break

        # only insert the file if it is not already in list
        if not isInList:
            items.insert(0, itemName)

        # while because items could be in a bad state because of previously thrown exceptions
        # make sure we trim them correctly in all circumstances
        while len(items) > self.maxRecentItems:
            items.remove(items[self.maxRecentItems])

        self.qsettings.setValue(self.sectionIdentifier, self.stringListToString(items))

    def removeRecentlyUsed(self, itemname):
        """Removes an item from the list. Safe to call even if the item is
        not in the list.
        """

        items = []
        if self.qsettings.contains(self.sectionIdentifier):
            val = unicode(self.qsettings.value(self.sectionIdentifier))
            items = self.stringToStringList(val)

        # if something went wrong before, stop
        if not isinstance(items, list):
            return False

        if not itemname in items:
            return False

        items.remove(itemname)

        self.qsettings.setValue(self.sectionIdentifier, self.stringListToString(items))
        return True

    def clearRecentlyUsed(self):
        self.qsettings.remove(self.sectionIdentifier)

    def getRecentlyUsed(self):
        """Returns all items as a string list"""

        items = []
        if self.qsettings.contains(self.sectionIdentifier):
            val = unicode(self.qsettings.value(self.sectionIdentifier))
            items = self.stringToStringList(val)

        return items

    def trimItemName(self, itemName):
        """Trim the itemName to the max. length and return it"""

        if (len(itemName) > self.recentlItemsNameTrimLength):
            # + 3 because of the ...
            trimedItemName = "...%s" % (itemName[-self.recentlItemsNameTrimLength + 3:])
            return trimedItemName
        else:
            return itemName

    def stringListToString(self, list_):
        """Converts a list into a string for storage in QSettings"""

        temp = ""

        first = True
        for s in list_:
            t = s.replace( ';', '\\;' )
            if first is True:
                temp = t
                first = False
            else:
                temp = temp + ";" + t

        return temp

    def stringToStringList(self, instr):
        """Converts a string to a string list"""

        workStr = instr
        list_ = []

        if not workStr.find('\\;') > -1:
            return instr.split(";")
        else:
            tempList = []
            pos = workStr.find(';')
            while pos > -1:
                # if we find a string that is escaped split the text and add it
                # to the temporary list
                # the path will be reconstructed when we don't find any escaped
                # semicolons anymore (in else)

                if workStr[pos-1:pos+1] == "\\;":
                    tempList.append(workStr[:pos+1].replace("\\;", ";"))
                    workStr = workStr[pos+1:]
                    pos = workStr.find(';')
                    if pos < 0: # end reached, finalize
                        list_.append(self.reconstructString(tempList) + workStr)
                        workStr = workStr[pos+1:]
                        tempList = []
                else:
                    # found a unescaped ; so we reconstruct the string before it,
                    # it should be a complete item
                    list_.append(self.reconstructString(tempList) + workStr[:pos])
                    workStr = workStr[pos+1:]
                    pos = workStr.find(';')
                    tempList = []

            return list_

    def reconstructString(self, list_):
        """reconstructs the string from a list and return it"""

        reconst = ""
        for s in list_:
            reconst = reconst + s
        return reconst

class RecentlyUsedMenuEntry(RecentlyUsed):
    """This class can be used to manage a Qt Menu entry to items.
    """

    def __init__(self, qsettings, sectionIdentifier):
        super(RecentlyUsedMenuEntry, self).__init__(qsettings, sectionIdentifier)

        self.menu = None
        self.slot = None
        self.clearAction = None

    def setParentMenu(self, menu, slot, clearAction):
        """Sets the parent menu and the slot that is called when clicked on an item
        """

        self.menu = menu
        self.slot = slot
        self.clearAction = clearAction
        self.clearAction.triggered.connect(self.slot_clear)
        self.updateMenu()

    def addRecentlyUsed(self, itemName):
        """Adds an item to the list"""

        super(RecentlyUsedMenuEntry, self).addRecentlyUsed(itemName)
        self.updateMenu()

    def removeRecentlyUsed(self, itemName):
        """Removes an item from the list. Safe to call even if the item is not
        in the list.
        """

        super(RecentlyUsedMenuEntry, self).removeRecentlyUsed(itemName)
        self.updateMenu()

    def slot_clear(self):
        super(RecentlyUsedMenuEntry, self).clearRecentlyUsed()
        self.updateMenu()

    def updateMenu(self):
        self.menu.clear()
        items = self.getRecentlyUsed()

        i = 1
        for f in items:
            actionRP = QtGui.QAction(self.menu)
            text = self.trimItemName(f)
            if i <= 10:
                text = "&" + str(i % 10) + ". " + text

            actionRP.setText(text)
            actionRP.setData(f)
            actionRP.setVisible(True)
            actionRP.triggered.connect(self.slot)
            self.menu.addAction(actionRP)
            i += 1

        self.menu.addSeparator()
        self.menu.addAction(self.clearAction)
        self.clearAction.setEnabled(len(items) > 0)
