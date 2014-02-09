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

class PersistenceProvider(object):
    def __init__(self):
        pass

    def upload(self, entry, value):
        pass

    def download(self, entry):
        pass

class QSettingsPersistenceProvider(PersistenceProvider):
    def __init__(self, qsettings):
        super(QSettingsPersistenceProvider, self).__init__()

        self.qsettings = qsettings

    def upload(self, entry, value):
        self.qsettings.setValue(entry.getPath(), value)

    def download(self, entry):
        return self.qsettings.value(entry.getPath())
