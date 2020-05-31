FROM ubuntu:focal

run \
apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y  \
 clang \
 ninja-build \
 cmake \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*
  

