#!/bin/sh

MYPATH=`dirname -z $0 | xargs --null readlink -f`
echo "MYPATH: $MYPATH" >&2

PATHS_IN_PYTHONPATH="\
`readlink -f \"$MYPATH/../../cegui-source/build/lib/\"` \
`readlink -f \"$MYPATH/../\"` \
`readlink -f \"$MYPATH/../../cegui-v0-8/build/lib/\"` \
`readlink -f \"$MYPATH/../../cegui/build/lib/\"` \
"

PYTHONPATH=''
for THISPATH in $PATHS_IN_PYTHONPATH
do
  PYTHONPATH="$THISPATH:$PYTHONPATH"
  echo "Adding PYTHONPATH component: $THISPATH" >&2
done

echo "Final PYTHONPATH:" >&2
echo "$PYTHONPATH" >&2

PYTHONPATH="$PYTHONPATH" python2 ./ceed-gui
