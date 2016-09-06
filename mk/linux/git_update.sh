#!/bin/bash
export LANG=C 

cd "$(dirname $(readlink -f $0))"

echo "STAGE 1/3 - GIT PULL"
echo
echo "Entering '../..'"
git pull
echo
git submodule foreach 'git pull; echo'

echo
echo
echo "STAGE 2/3 - GIT BRANCH"
echo
echo "Entering '../..'"
git branch
echo
git submodule foreach 'git branch; echo'

echo
echo "STAGE 3/3 - GIT STATUS"
echo
echo "Entering '../..'"
git status
echo
git submodule foreach 'git status; echo'
