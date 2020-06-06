#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD=${1:-debug}
touch $DIR/../CMakeLists.txt
(cd `readlink -f $DIR/../build`/$BUILD && ninja)

