#!/bin/bash

LIBICAL_INSTALL_DIR=/home/ben/projects/libical/install


/usr/bin/gcc -g -I$LIBICAL_INSTALL_DIR/include -L$LIBICAL_INSTALL_DIR/lib -Wl,-rpath,$LIBICAL_INSTALL_DIR/lib -rdynamic $LIBICAL_INSTALL_DIR/lib/libical*.so -o demo demo.c -lical || exit 1

./demo <&0


