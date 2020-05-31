#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT=$DIR/..

cd $ROOT

# stdlib includes
find /usr/include/c++/9 -maxdepth 1 | sed s/\\/usr\\/include\\/c\+\+\\/9\\///

# external includes
find /usr/include/ -name "*.h" -or -name "*.hpp" | sed s/\\/usr\\/include\\///

