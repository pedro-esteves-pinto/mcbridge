#/bin/bash


DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SRC_ROOT=`realpath $DIR/../`
BUILD_ROOT=`readlink -f $SRC_ROOT/build`
mkdir -p $BUILD_ROOT

rm -rf $BUILD_ROOT/debug
mkdir -p $BUILD_ROOT/debug
cd $BUILD_ROOT/debugcmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_CXX_COMPILER=clang++ $SRC_ROOT

rm -rf $BUILD_ROOT/release
mkdir -p $BUILD_ROOT/release
cd $BUILD_ROOT/release
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_CXX_COMPILER=clang++ $SRC_ROOT

rm -rf $BUILD_ROOT/debug_gcc
mkdir -p $BUILD_ROOT/debug_gcc
cd $BUILD_ROOT/debug_gcc
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_CXX_COMPILER=g++ $SRC_ROOT

rm -rf $BUILD_ROOT/release_gcc
mkdir -p $BUILD_ROOT/release_gcc
cd $BUILD_ROOT/release_gcc
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_CXX_COMPILER=g++ $SRC_ROOT
