FROM mcbridge

RUN apt-get update && \
DEBIAN_FRONTEND=noninteractive apt-get install -y stow \
build-essential libx11-dev xaw3dg-dev libjpeg-dev libpng-dev \
libgif-dev libtiff-dev libncurses5-dev libxft-dev librsvg2-dev \
libmagickcore-dev libmagick++-dev libxml2-dev libgpm-dev \
libgconf2-dev libotf-dev libm17n-dev libgnutls28-dev \
ispell \
locales \
libjansson-dev \
flex bison gettext texinfo  libncurses5-dev libreadline-dev  \
git \
silversearcher-ag \
ninja-build \
gdb \
libvterm-dev \
clang-tools libclang-dev clangd clang-format 

# build cgdb
run git clone https://github.com/cgdb/cgdb.git && \
cd cgdb && \
git checkout v0.7.1 && \
./autogen.sh && \
./configure && \
make -j $(nproc) install && \
cd .. && 	 \
rm -rf cgdb

# build emacs
run git clone https://github.com/emacs-mirror/emacs.git && \
cd emacs &&\
git checkout emacs-27 &&\
./autogen.sh &&\
./configure --with-json --with-xft --with-x-toolkit=lucid &&\
make -j$(nproc) install &&\
cd .. && \
rm -rf emacs

# build rtags
run \
git clone --recursive https://github.com/Andersbakken/rtags.git && \
cd rtags && \
git submodule update && \
git checkout a42298e80dffae9a6b2e9816506174542f3cbbb7 && \
cmake . && \
make -j$(nproc) install && \
cd .. && \
rm -rf rtags


