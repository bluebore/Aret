#!/bin/sh
./configure --prefix=./webserver/ --add-module=./src/aret/
cp objs_Makefile objs/Makefile
make -j8
