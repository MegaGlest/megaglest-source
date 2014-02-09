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

class Entry(object):
    """Is the value itself, inside a section. This is what's directly used when
    accessing settings.

    value represents the current value to use
    editedValue represents the value user directly edits
    (it is applied - value = editedValue - when user applies the settings)
    """

    STRING = "string"

    value = property(fset = lambda entry, value: entry._setValue(value),
                     fget = lambda entry: entry._value)
    editedValue = property(fset = lambda entry, value: entry._setEditedValue(value),
                           fget = lambda entry: entry._editedValue)

    def __init__(self, section, name, type_, defaultValue, label = None, help_ = "", widgetHint = STRING, sortingWeight = 0, changeRequiresRestart = False, optionList = None):
        self.section = section

        if label is None:
            label = name

        self.name = name
        self.type = type_

        defaultValue = self.sanitizeValue(defaultValue)
        self.defaultValue = defaultValue
        self._value = defaultValue
        self._editedValue = defaultValue

        self.label = label
        self.help = help_
        self.hasChanges = False
        self.widgetHint = widgetHint

        self.sortingWeight = sortingWeight

        self.changeRequiresRestart = changeRequiresRestart

        self.optionList = optionList

        self.subscribers = []

    def getPath(self):
        """Retrieves a unique path in the qsettings tree, this can be used by persistence providers for example
        """

        return "%s/%s" % (self.section.getPath(), self.name)

    def getPersistenceProvider(self):
        return self.section.getPersistenceProvider()

    def getSettings(self):
        return self.section.getSettings()

    def sanitizeValue(self, value):
        if not isinstance(value, self.type):
            value = self.type(value)

        return value

    def subscribe(self, callable_):
        """Subscribes a callable that gets called when the value changes (the real value, not edited value!)
        (with current value as the argument)
        """

        self.subscribers.append(callable_)

    def unsubscribe(self, callable_):
        self.subscribers.remove(callable_)

    def _setValue(self, value, uploadImmediately = True):
        value = self.sanitizeValue(value)

        self._value = value
        self._editedValue = value
        if uploadImmediately:
            self.upload()

        for callable_ in self.subscribers:
            callable_(value)

    def _setEditedValue(self, value):
        value = self.sanitizeValue(value)

        self._editedValue = value
        self.hasChanges = True

    def markAsChanged(self):
        if not self.label.startswith("* "):
            self.label = " ".join(["*", self.label])

    def markAsUnchanged(self):
        if self.label.startswith("* "):
            self.label = self.label[2:]

    def applyChanges(self):
        if self.value != self.editedValue:
            self.value = self.editedValue

            if self.changeRequiresRestart:
                self.getSettings().markRequiresRestart()

    def discardChanges(self):
        self.editedValue = self.value

        # - This is set via the property, but from this context we know that
        #   we really don't have any changes.
        self.hasChanges = False

    def upload(self):
        self.getPersistenceProvider().upload(self, self._value)
        self.hasChanges = False

    def download(self):
        persistedValue = self.getPersistenceProvider().download(self)
        if persistedValue is None:
            persistedValue = self.defaultValue

        # http://bugs.pyside.org/show_bug.cgi?id=345
        if self.widgetHint == "checkbox":
            if persistedValue == "false":
                persistedValue = False
            elif persistedValue == "true":
                persistedValue = True

        self._setValue(persistedValue, False)
        self.hasChanges = False

class Section(object):
    """Groups entries, is usually represented by a group box in the interface
    """

    def __init__(self, category, name, label = None, sortingWeight = 0):
        self.category = category

        if label is None:
            label = name

        self.name = name
        self.label = label
        self.sortingWeight = sortingWeight

        self.entries = []

    def getPath(self):
        """Retrieves a unique path in the qsettings tree, this can be used by persistence providers for example
        """

        return "%s/%s" % (self.category.getPath(), self.name)

    def getPersistenceProvider(self):
        return self.category.getPersistenceProvider()

    def getSettings(self):
        return self.category.getSettings()

    def createEntry(self, **kwargs):
        entry = Entry(section = self, **kwargs)
        self.entries.append(entry)

        return entry

    def getEntry(self, name):
        for entry in self.entries:
            if entry.name == name:
                return entry

        raise RuntimeError("Entry of name '" + name + "' not found inside section '" + self.name + "' (path: '" + self.getPath() + "').")

    # - Reserved for possible future use.
    def markAsChanged(self):
        pass

    def markAsUnchanged(self):
        pass

    def applyChanges(self):
        for entry in self.entries:
            entry.applyChanges()

    def discardChanges(self):
        for entry in self.entries:
            entry.discardChanges()

    def upload(self):
        for entry in self.entries:
            entry.upload()

    def download(self):
        for entry in self.entries:
            entry.download()

    def sort(self):
        # FIXME: This is obviously not the fastest approach
        self.entries = sorted(self.entries, key = lambda entry: entry.name)
        self.entries = sorted(self.entries, key = lambda entry: entry.sortingWeight)

class Category(object):
    """Groups sections, is usually represented by a tab in the interface
    """

    def __init__(self, settings, name, label = None, sortingWeight = 0):
        self.settings = settings

        if label is None:
            label = name

        self.name = name
        self.label = label
        self.sortingWeight = sortingWeight

        self.sections = []

    def getPath(self):
        """Retrieves a unique path in the qsettings tree, this can be used by persistence providers for example
        """

        return "%s/%s" % (self.settings.getPath(), self.name)

    def getPersistenceProvider(self):
        return self.settings.getPersistenceProvider()

    def getSettings(self):
        return self.settings

    def createSection(self, **kwargs):
        section = Section(category = self, **kwargs)
        self.sections.append(section)

        return section

    def getSection(self, name):
        for section in self.sections:
            if section.name == name:
                return section

        raise RuntimeError("Section '" + name + "' not found in category '" + self.name + "' of this settings")

    def createEntry(self, **kwargs):
        if self.getSection("") is None:
            section = self.createSection(name = "")
            section.sortingWeight = -1

        section = self.getSection("")
        return section.createEntry(**kwargs)

    def getEntry(self, path):
        # FIXME: Needs better error handling
        splitted = path.split("/", 1)
        assert(len(splitted) == 2)

        section = self.getSection(splitted[0])
        return section.getEntry(splitted[1])

    def markAsChanged(self):
        if not self.label.startswith("* "):
            self.label = " ".join(["*", self.label])

    def markAsUnchanged(self):
        if self.label.startswith("* "):
            self.label = self.label[2:]

    def applyChanges(self):
        for section in self.sections:
            section.applyChanges()

    def discardChanges(self):
        for section in self.sections:
            section.discardChanges()

    def upload(self):
        for section in self.sections:
            section.upload()

    def download(self):
        for section in self.sections:
            section.download()

    def sort(self, recursive = True):
        self.sections = sorted(self.sections, key = lambda section: (section.sortingWeight, section.name))

        if recursive:
            for section in self.sections:
                section.sort()

class Settings(object):
    def __init__(self, name, label = None, help_ = ""):
        if label is None:
            label = name

        self.name = name
        self.label = label
        self.help = help_

        self.categories = []
        self.persistenceProvider = None

        self.changesRequireRestart = False

    def getPath(self):
        return self.name

    def setPersistenceProvider(self, persistenceProvider):
        self.persistenceProvider = persistenceProvider

    def getPersistenceProvider(self):
        assert(self.persistenceProvider is not None)

        return self.persistenceProvider

    def createCategory(self, **kwargs):
        category = Category(settings = self, **kwargs)
        self.categories.append(category)

        return category

    def getCategory(self, name):
        for category in self.categories:
            if category.name == name:
                return category

        raise RuntimeError("Category '" + name + "' not found in this settings")

    def getEntry(self, path):
        # FIXME: Needs better error handling
        splitted = path.split("/", 1)
        assert(len(splitted) == 2)

        category = self.getCategory(splitted[0])
        return category.getEntry(splitted[1])

    def markRequiresRestart(self):
        self.changesRequireRestart = True

    def applyChanges(self):
        for category in self.categories:
            category.applyChanges()

    def discardChanges(self):
        for category in self.categories:
            category.discardChanges()

    def upload(self):
        for category in self.categories:
            category.upload()

    def download(self):
        for category in self.categories:
            category.download()

        self.changesRequireRestart = False

    def sort(self, recursive = True):
        # FIXME: This is obviously not the fastest approach
        self.categories = sorted(self.categories, key = lambda category: category.name)
        self.categories = sorted(self.categories, key = lambda category: category.sortingWeight)

        if recursive:
            for category in self.categories:
                category.sort()
