#!/bin/bash

usage() {

  echo
  echo 'Usage:'
  echo "$(basename $0)"' <COMMAND>'
  echo
  echo 'COMMAND is a standard git command, e.g. "pull" to run "git pull"'

}

case $1 in 

  pull|status|branch ) 
    echo '==> Running "git '"$1"'" on main repository...'
    cd $(dirname $0)
    git $1
    if [ $? -ne 0 ]; then
      echo 'Error: Failed to run "git '"$1"'" on main repository.' >&2
      exit 2
    fi
    echo
    echo '==> Running "git '"$1"'" on submodules...'
    git submodule foreach git $1
    if [ $? -ne 0 ]; then
      echo 'Error: Failed to run "git '"$1"'" on submodules.' >&2
      exit 2
    fi
  ;;

  '' )
    echo 'Error: No command was provided.'"$1" >&2
    usage
    exit 1
  ;;

  * )
    echo 'Error: Unknown command: '"$1" >&2
    exit 1
  ;;

esac
