#!/bin/sh

INSTALL_PATH="./webserver"
INCLUDE_PATH="-I./src/aret"
LD_PATH="-ltera_easy -ljson -L./src/aret"
./configure --prefix=$INSTALL_PATH --add-module=./src/aret/ --with-cc-opt="$INCLUDE_PATH" --with-ld-opt="$LD_PATH";

# update g++
sed -i '1087s/\$(CC)/g++/' objs/Makefile;
sed -i '215s/\$(LINK)/g++/' objs/Makefile; 

#cp objs_Makefile objs/Makefile
make -j8a && make install
