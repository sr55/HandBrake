#!/bin/bash

#####################################################
# Setup the AppVeyor Environment to build HandBrake
#####################################################


#
# System Setup
#
sudo apt-get update -y
sudo apt-get install -y automake autoconf build-essential cmake curl gcc git intltool libtool libtool-bin m4 make nasm patch pkg-config python tar yasm zlib1g-dev ninja-build zip
sudo apt-get install -y bison bzip2 flex g++ gzip pax
sudo apt-get install -y python3-pip
sudo apt-get install -y python3-setuptools
sudo pip3 install meson

#
# Toolchain Setup and Verificaiton
#
wget https://github.com/bradleysepos/mingw-w64-build/releases/download/9.1.0/mingw-w64-toolchain-9.1.0-linux-x86_64.tar.gz
SHA=$(sha1sum mingw-w64-toolchain-9.1.0-linux-x86_64.tar.gz)
EXPECTED="4c0fadeaaa0c72ed7107bf49ebddf5c8a35abd92  mingw-w64-toolchain-9.1.0-linux-x86_64.tar.gz"
if [ "$SHA" == "$EXPECTED" ];
then
    echo "Toolchain Verified. Extracting ..."
    mkdir toolchains
    mv mingw-w64-toolchain-9.1.0-linux-x86_64.tar.gz toolchains
    cd toolchains
    tar xvf mingw-w64-toolchain-9.1.0-linux-x86_64.tar.gz
    cd ..
    pwd
else
    echo "Toolchain Verification FAILED. Exiting!"
    return -1
fi
