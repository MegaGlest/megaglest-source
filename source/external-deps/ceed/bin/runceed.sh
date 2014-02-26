#!/bin/sh

echo; echo
MYPATH=`dirname -z $0 | xargs --null readlink -f`
echo "MYPATH: $MYPATH" >&2
CEGUI_E_LIBS_PATH="$MYPATH/../../cegui-source/build/lib/"
CEGUI_THEMES_PATH="../../../../data/glest_game/data/cegui/themes"
CEGUI_D_MG_THEME="$CEGUI_THEMES_PATH/default/glossyserpent.project"

PATHS_IN_PYTHONPATH="\
`readlink -f \"$CEGUI_E_LIBS_PATH\"` \
`readlink -f \"$MYPATH/../\"` \
`readlink -f \"$MYPATH/../../cegui-v0-8/build/lib/\"` \
`readlink -f \"$MYPATH/../../cegui/build/lib/\"` \
"

PYTHONPATH=''
for THISPATH in $PATHS_IN_PYTHONPATH
do
  PYTHONPATH="$THISPATH:$PYTHONPATH"
  # echo "Adding PYTHONPATH component: $THISPATH" >&2
done

echo "Final PYTHONPATH: $PYTHONPATH" >&2

if [ -d "$CEGUI_E_LIBS_PATH" ]; then
  LD_LIBRARY_PATH="$CEGUI_E_LIBS_PATH:$LD_LIBRARY_PATH"
  echo "The path to the embedded libraries has been set." >&2
fi

if [ -f "$CEGUI_D_MG_THEME" ]; then
  MG_T_PROJECT="--project $CEGUI_D_MG_THEME"
else
  MG_T_PROJECT=""
  echo "The project has not been found." >&2
fi

PYTHONPATH="$PYTHONPATH" python2 ./ceed-gui $MG_T_PROJECT
