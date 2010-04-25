#!/bin/sh

grep -n '_("' *.[chm] scripts/*.lua |perl -w -p -e 's/\A.*?_\((".*?")\).*?\Z/$1/;' |sort |uniq |perl -w -p -e 'chomp; $_ = "    [$_] = {\n    };\n\n";'

