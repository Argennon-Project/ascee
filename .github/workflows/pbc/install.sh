#!/bin/sh
echo Installing the PBC library...	
# curl -O https://crypto.stanford.edu/pbc/files/pbc-0.5.14.tar.gz
tar -xf pbc-0.5.14.tar.gz
cd pbc-0.5.14 || exit
./configure
make
sudo make install
echo Creating header files...
sudo cp -r include /usr/include/pbc
