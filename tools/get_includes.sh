#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SRC_ROOT=`realpath $DIR/../`
cd $SRC_ROOT

# our includes
(cd src && find -name "*.h" |\
    sed s/\\.\\/// |\
    awk '{print "\""$1"\""}')

# asio includes
(cd external/asio/asio/include && find asio -name "*.hpp")  |\
    awk '{print "<"$1">"}'

# cli11 includes
(cd external/ && find CLI11 -name "*.hpp") |\
    awk '{print "<"$1">"}'
    
# stdlib inclues 
(cd /usr/include/c++/9 && find -maxdepth 1 | sed s/\\.\\///) |\
    awk '{print "<"$1">"}'

