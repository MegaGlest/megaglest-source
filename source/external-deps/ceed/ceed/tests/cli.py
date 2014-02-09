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

import unittest
import subprocess
import os
import tempfile

class test_CommandLineTools(unittest.TestCase):
    def setUp(self):
        self.basePath = os.path.join(os.path.dirname(__file__), "..", "..")

    def _test_run(self, cliTool, args):
        try:
            fullCliTool = os.path.join(self.basePath, "bin", cliTool)
            fullArgs = [fullCliTool] + args

            subprocess.check_output(fullArgs, stderr = subprocess.STDOUT)

        except subprocess.CalledProcessError as e:
            self.fail(msg = "%s\n\nException message: %s" % (e.output, e))

    def test_ceed_mic(self):
        sampleMI = os.path.join(self.basePath, "data", "samples", "Basic.meta-imageset")
        self._test_run("ceed-mic", [sampleMI])

    def test_ceed_migrate(self):
        layoutPath = os.path.join(self.basePath, "ceed/tests/compatibility/layout_data", "VanillaWindows_0_7.layout")
        tempFile = tempfile.NamedTemporaryFile()

        self._test_run("ceed-migrate", ["--sourceType", "CEGUI layout 3", "layout", layoutPath, tempFile.name])
