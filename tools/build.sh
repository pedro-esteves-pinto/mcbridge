#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD=${1:-debug}
(cd `readlink -f $DIR/../build`/$BUILD && ninja)

