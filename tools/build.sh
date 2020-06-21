#!/bin/bash
DIR="$(dirname "$(readlink -f "$0")")"
BUILD=${1:-debug}
touch $DIR/../CMakeLists.txt
(cd `readlink -f $DIR/../build`/$BUILD && ninja)

