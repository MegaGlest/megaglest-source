#!/usr/bin/env bash

# parent dir of this script
PARENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

python -m cProfile -o $PARENT_DIR/profile_data $1
